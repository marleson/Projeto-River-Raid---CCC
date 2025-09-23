// ============================================================================
// River Raid (terminal) — C + ncurses  |  VERSÃO BEM COMENTADA
// ----------------------------------------------------------------------------
// Objetivo: código didático (nível iniciante) com funções curtas e comentários
//           explicando as decisões principais de lógica, desenho e colisão.
// Baseado no seu arquivo atual (mesma organização e comportamento geral).
// ----------------------------------------------------------------------------
// Controles:  ←/→  ou  A/D  para mover   |   ESPAÇO = atirar   |   R = reiniciar
//             Q = sair
// Compilação (macOS):
//   clang -std=c11 -Wall -Wextra -O2 river_raid.c -lncurses -o river_raid
//   # Se precisar da ncurses do Homebrew, acrescente -I / -L conforme README.
// Execução:
//   ./river_raid
// ============================================================================

#include <ncurses.h>   // biblioteca para desenhar em modo texto
#include <stdlib.h>    // malloc, free, rand
#include <time.h>      // time (semente do rand)
#include <unistd.h>    // usleep (pausa em microssegundos)
#include <stdio.h>     // fprintf (mensagens de erro)

// ============================================================================
// ESTRUTURAS BÁSICAS
// ----------------------------------------------------------------------------
// Coordenadas: (x = coluna, y = linha). 0,0 fica no canto superior esquerdo.
// O terminal é uma grade de ALTURA linhas por LARGURA colunas.
// ============================================================================

typedef struct {
    int x;       // coluna do canto superior esquerdo do sprite do jogador
    int y;       // linha  do canto superior esquerdo do sprite do jogador
    int vivo;    // 1 = jogando; 0 = morto (mostra 'X' no sprite)
    long score;  // pontuação atual (neste projeto: somada ao abater inimigos)
    int fuel;    // combustível (0..100). Zera => game over
} Player;

// Inimigo é um sprite fixo HxW que desce a tela
typedef struct {
    int x;       // coluna do topo-esquerdo do inimigo
    int y;       // linha  do topo-esquerdo do inimigo
    int vivo;    // 1 = ativo; 0 = livre para respawn
} Inimigo;

#define INIMIGOS_MAX 20
Inimigo inimigos[INIMIGOS_MAX];  // vetor simples de inimigos

// ============================================================================
// TIROS: lista ligada de balas (permite "ilimitado" sem tamanho fixo)
// ----------------------------------------------------------------------------
// Cada tiro é 1x1 (um caractere '|') que sobe a tela. Ao acertar inimigo:
//   - remove o inimigo e o próprio tiro
//   - soma pontos (p->score += 30)
// ============================================================================

typedef struct Bala {
    int x, y;           // posição do tiro
    struct Bala *prox;  // encadeamento (lista ligada simples)
} Bala;

Bala *balas = NULL;     // início da lista de tiros (NULL = sem tiros)

// ============================================================================
// GASOLINA: pickups em texto "[FUEL]" que descem a tela (6x1)
// ----------------------------------------------------------------------------
// Observação: o spawn e a coleta permanecem como no seu arquivo. Há comentários
// indicando onde melhorar para usar colisão em área (AABB) se desejar.
// ============================================================================

typedef struct {
    int x;      // coluna do início da string "[FUEL]"
    int y;      // linha
    int vivo;   // 1 = ativo; 0 = livre para respawn
} Gasolina;

#define GASOLINA_MAX 5
Gasolina postos[GASOLINA_MAX];

// ============================================================================
// TELA / MUNDO (rio)
// ----------------------------------------------------------------------------
// - LARGURA, ALTURA: dimensões atuais do terminal (lidas na inicialização)
// - margemEsq[y], margemDir[y]: limites da água na linha y (inclusive)
//   Fora [margemEsq..margemDir] é "terra" (desenhada com '#').
// ============================================================================

int LARGURA = 0;        // nº de colunas da tela
int ALTURA  = 0;        // nº de linhas da tela

int *margemEsq = NULL;  // vetor por linha (y) com a margem esquerda
int *margemDir = NULL;  // vetor por linha (y) com a margem direita

// ============================================================================
// SPRITES ASCII (larguras/alturas fixas)
// ----------------------------------------------------------------------------
// IMPORTANTE: use apenas caracteres de largura 1 (ASCII) para manter alinhado.
// ============================================================================

#define AVIAO_H 3
#define AVIAO_W 5
const char *AVIAO[AVIAO_H] = {
    "  ^  ",
    "<-A->",
    " / \\ "
};

#define INIMIGO_H 3
#define INIMIGO_W 5
const char *INIMIGO[INIMIGO_H] = {
    " | | ",
    "\\ooo/",
    " === "
};

// Larguras mín./máx. do espelho d'água (parte navegável do rio)
int LARGURA_MIN = 0;
int LARGURA_MAX = 0;

// ============================================================================
// TEMPO DE JOGO (tick) — QUADROS POR SEGUNDO
// ----------------------------------------------------------------------------
// O jogo avança 1 "linha" de mundo por quadro. Controlamos a velocidade
// dormindo alguns microssegundos ao fim do loop.
// Começa mais lento, acelera até um limite (curva simples por score/tempo).
// ============================================================================

#define TICK_START_USEC 100000  // 100k µs = 0,1 s por quadro (~10 FPS)
#define TICK_MIN_USEC    20000  //  20k µs = 0,02 s por quadro (~50 FPS)
useconds_t tick_usec = TICK_START_USEC;

// ============================================================================
// PROTÓTIPOS (cabeçalhos)
// ============================================================================

void iniciarNcurses(void);
void finalizarNcurses(void);
void criarRioInicial(void);
void gerarNovaLinhaNoTopo(void);

void desenharAviao(const Player *p);
void desenharInimigo(const Inimigo *in);
void desenharBalas(void);
void desenharTudo(const Player *p);

int  haColisao(const Player *p);

void reiniciarJogo(Player *p);
void ReiniciarInimigos(void);
void ReiniciarGasolina(void);

// Tiros
void iniciarBalas(void);
void destruirBalas(void);
void disparar(const Player *p);
void atualizarBalas(Player *p);

// ============================================================================
// FUNÇÃO PRINCIPAL
// ============================================================================

int main(void) {
    // Semente aleatória para variar o mapa/spawn a cada execução
    srand((unsigned)time(NULL));

    iniciarNcurses();

    Player jogador;
    reiniciarJogo(&jogador);

    // Contadores simples usados no loop do jogo
    int contadorLinha = 0;   // quantas linhas de "mundo" já foram geradas
    int contadorSpawn = 0;   // controla quando tentar nascer inimigos
    int limiteSpawn   = 20;  // menor => nasce com mais frequência
    int fuelTick      = 0;   // consome combustível a cada N ciclos

    while (1) {
        // Leitura de teclado (não-bloqueante por causa do nodelay)
        int ch = getch();
        if (ch == 'q' || ch == 'Q') break;  // sair do jogo

        if (jogador.vivo) {
            // ------------------------------
            // INPUT do jogador
            // ------------------------------
            if (ch == KEY_LEFT  || ch == 'a' || ch == 'A') jogador.x--;
            if (ch == KEY_RIGHT || ch == 'd' || ch == 'D') jogador.x++;
            if (ch == ' ')                               disparar(&jogador);

            // Mantém o avião dentro da tela (reserva 1 coluna de cada lado)
            if (jogador.x < 1) jogador.x = 1;
            int maxX = LARGURA - AVIAO_W - 1;
            if (jogador.x > maxX) jogador.x = maxX;

            // ------------------------------
            // AVANÇO DO MUNDO (1 linha por quadro)
            // ------------------------------
            contadorLinha++;
            gerarNovaLinhaNoTopo();

            // ------------------------------
            // INIMIGOS: descem 1 linha/quadros
            // ------------------------------
            for (int i = 0; i < INIMIGOS_MAX; i++) {
                if (inimigos[i].vivo) {
                    inimigos[i].y++;
                    if (inimigos[i].y >= ALTURA) inimigos[i].vivo = 0; // saiu da tela
                }
            }

            // Spawn simples de inimigos (periodicamente)
            contadorSpawn++;
            if (contadorSpawn > limiteSpawn) {
                contadorSpawn = 0;
                for (int i = 0; i < INIMIGOS_MAX; i++) {
                    if (!inimigos[i].vivo) {
                        inimigos[i].vivo = 1;
                        inimigos[i].y = 0;  // nasce no topo visível
                        // posição X aleatória dentro da água (deixa 1 col. de folga)
                        inimigos[i].x = margemEsq[0] + 1 + rand() % (margemDir[0] - margemEsq[0] - INIMIGO_W - 1);
                        break;
                    }
                }
            }

            // ------------------------------
            // GASOLINA: desce e nasce de forma simples
            // ------------------------------
            for (int i = 0; i < GASOLINA_MAX; i++) {
                if (postos[i].vivo) {
                    postos[i].y++;
                    if (postos[i].y >= ALTURA) postos[i].vivo = 0; // saiu
                }
            }
            if (rand() % 100 < 3) { // ~3% de chance por quadro
                for (int i = 0; i < GASOLINA_MAX; i++) {
                    if (!postos[i].vivo) {
                        postos[i].vivo = 1;
                        postos[i].y = 0;
                        postos[i].x = margemEsq[0] + 1 + rand() % (margemDir[0] - margemEsq[0] - 2);
                        break;
                    }
                }
            }

            // ------------------------------
            // TIROS: movem, colidem e somam pontos (+30 por inimigo)
            // ------------------------------
            atualizarBalas(&jogador);

            // ------------------------------
            // (Opcional) Pontos por sobrevivência: remova este bloco
            // se quiser pontuar APENAS por abates de inimigos.
            // ------------------------------
            if (contadorLinha % 100 == 0) {
                jogador.score += 10; // +10 a cada ~100 linhas sobrevividas
            }

            // ------------------------------
            // Aceleração progressiva: diminui o tick até um limite
            // ------------------------------
            if (contadorLinha % 120 == 0 && tick_usec > TICK_MIN_USEC) {
                tick_usec -= 2000;                  // acelera um pouquinho
                if (tick_usec < TICK_MIN_USEC) tick_usec = TICK_MIN_USEC;
            }

            // Diminui o intervalo de spawn (mais inimigos com o tempo)
            if (contadorLinha % 500 == 0 && limiteSpawn > 3) limiteSpawn--;

            // ------------------------------
            // Coleta de combustível (forma simples: testa ponto de origem)
            // Para coletar por ÁREA, troque por colisão retangular AABB
            // entre o retângulo do avião (AVIAO_W x AVIAO_H) e [FUEL] (6x1).
            // ------------------------------
            for (int i = 0; i < GASOLINA_MAX; i++) {
                if (postos[i].vivo &&
                    postos[i].x >= jogador.x && postos[i].x < jogador.x + AVIAO_W &&
                    postos[i].y >= jogador.y && postos[i].y < jogador.y + AVIAO_H) {
                    postos[i].vivo = 0;   // coletado
                    jogador.fuel = 100;   // tanque cheio
                }
            }

            // ------------------------------
            // Consumo de combustível: 1 unidade a cada 8 quadros
            // (Se quiser por tempo real, some tick_usec e consuma por segundo.)
            // ------------------------------
            fuelTick++;
            if (fuelTick >= 8) { fuelTick = 0; jogador.fuel--; }
            if (jogador.fuel <= 0) jogador.vivo = 0; // sem combustível => game over

            // ------------------------------
            // Colisão do avião com margens e inimigos (pixel-a-pixel do sprite)
            // ------------------------------
            if (haColisao(&jogador)) jogador.vivo = 0;
        }
        else {
            // Morto: permite recomeçar
            if (ch == 'r' || ch == 'R') reiniciarJogo(&jogador);
        }

        // DESENHO de toda a cena (rio, inimigos, tiros, HUD, avião)
        desenharTudo(&jogador);

        // Aguarda o tempo do quadro (controla a velocidade geral)
        usleep(tick_usec);
    }

    finalizarNcurses();
    return 0;
}

// ============================================================================
// TIROS — implementação
// ============================================================================

void iniciarBalas(void) { balas = NULL; }

void destruirBalas(void) {
    // Libera toda a lista ao reiniciar/sair
    while (balas) {
        Bala *tmp = balas;
        balas = balas->prox;
        free(tmp);
    }
}

// Cria uma bala no "nariz" do avião (centro do topo do sprite)
void disparar(const Player *p) {
    int narizX = p->x + AVIAO_W / 2;
    int startY = p->y - 1;        // nasce uma linha acima do avião
    if (startY <= 0) return;      // segurança

    Bala *b = (Bala*)malloc(sizeof(Bala));
    if (!b) return;               // sem memória → ignora
    b->x = narizX;
    b->y = startY;
    b->prox = balas;              // insere no começo (O(1))
    balas = b;
}

// Move balas para cima; remove ao sair da tela, tocar margem ou acertar inimigo
// Ao acertar: remove inimigo e soma +30 pontos no jogador
void atualizarBalas(Player *p) {
    Bala **pp = &balas;           // ponteiro para ponteiro: facilita remoção
    while (*pp) {
        Bala *b = *pp;
        b->y--;                   // sobe 1 linha por quadro

        int remover = 0;

        if (b->y <= 0) {
            remover = 1;          // saiu da tela
        } else if (b->x <= margemEsq[b->y] || b->x >= margemDir[b->y]) {
            remover = 1;          // entrou na margem (fora da água)
        } else {
            // Testa colisão com cada inimigo (AABB do sprite do inimigo)
            for (int i = 0; i < INIMIGOS_MAX; i++) {
                if (inimigos[i].vivo &&
                    b->x >= inimigos[i].x && b->x < inimigos[i].x + INIMIGO_W &&
                    b->y >= inimigos[i].y && b->y < inimigos[i].y + INIMIGO_H) {
                    inimigos[i].vivo = 0;  // destrói inimigo
                    p->score += 30;        // +30 pontos por abate
                    remover = 1;           // bala se consome
                    break;
                }
            }
        }

        if (remover) {
            *pp = b->prox;        // retira da lista
            free(b);
        } else {
            pp = &b->prox;        // avança
        }
    }
}

void desenharBalas(void) {
    // Desenha cada bala como um '|'
    for (Bala *b = balas; b != NULL; b = b->prox) {
        mvaddch(b->y, b->x, '|');
    }
}

// ============================================================================
// NCURSES + RIO (inicialização, destruição, geração e desenho do mundo)
// ============================================================================

void iniciarNcurses(void) {
    initscr();            // entra no modo ncurses
    cbreak();             // lê tecla sem exigir ENTER
    noecho();             // não ecoa entradas do teclado
    curs_set(0);          // cursor invisível
    keypad(stdscr, TRUE); // setas e teclas especiais
    nodelay(stdscr, TRUE);// getch() não bloqueia o loop

    // Lê dimensões atuais da janela do terminal
    getmaxyx(stdscr, ALTURA, LARGURA);

    // Tamanho mínimo para caber HUD/sprites e jogabilidade
    if (ALTURA < 20 || LARGURA < 40) {
        endwin();
        fprintf(stderr, "Aumente o terminal para pelo menos 40x20.\n");
        exit(1);
    }

    // Define limites de largura "navegável" do rio com base na tela
    LARGURA_MIN = LARGURA / 3;    // mais estreito permitido
    LARGURA_MAX = LARGURA / 2;    // mais largo permitido

    // Aloca vetores das margens (um valor por linha)
    margemEsq = (int*)malloc(sizeof(int) * ALTURA);
    margemDir = (int*)malloc(sizeof(int) * ALTURA);
    if (!margemEsq || !margemDir) {
        endwin();
        fprintf(stderr, "Falha ao alocar memória.\n");
        exit(1);
    }
}

void finalizarNcurses(void) {
    destruirBalas();    // libera tiros restantes
    endwin();
    free(margemEsq);
    free(margemDir);
}

// Preenche a tela com um rio reto (mesma largura em todas as linhas)
void criarRioInicial(void) {
    int centro = LARGURA / 2;     // começa centralizado
    int larguraRio = LARGURA / 2; // largura inicial (ajustada por limites)
    if (larguraRio < LARGURA_MIN) larguraRio = LARGURA_MIN;
    if (larguraRio > LARGURA_MAX) larguraRio = LARGURA_MAX;

    int metade = larguraRio / 2;
    int L = centro - metade;      // margem esquerda inicial
    int R = centro + metade;      // margem direita inicial

    for (int y = 0; y < ALTURA; y++) {
        margemEsq[y] = L;
        margemDir[y] = R;
    }
}

// Gera a linha do topo (y=0) com pequenas variações no centro/largura
// E empurra as linhas antigas para baixo (scroll/infinito)
void gerarNovaLinhaNoTopo(void) {
    // 1) Scroll: copia de cima para baixo
    for (int y = ALTURA - 1; y > 0; y--) {
        margemEsq[y] = margemEsq[y - 1];
        margemDir[y] = margemDir[y - 1];
    }

    // 2) Calcula nova linha no topo com leve ruído (±1)
    int Lant = margemEsq[0];
    int Rant = margemDir[0];
    int centroAnt  = (Lant + Rant) / 2;
    int larguraAnt = (Rant - Lant);

    int centroNovo  = centroAnt  + (rand() % 3 - 1);
    int larguraNova = larguraAnt + (rand() % 3 - 1);

    // 3) Limita a largura
    if (larguraNova < LARGURA_MIN) larguraNova = LARGURA_MIN;
    if (larguraNova > LARGURA_MAX) larguraNova = LARGURA_MAX;

    // 4) Converte centro/largura em margens e mantém dentro da tela
    int metade = larguraNova / 2;
    int Lnovo = centroNovo - metade;
    int Rnovo = centroNovo + metade;

    if (Lnovo < 1) { Lnovo = 1; Rnovo = Lnovo + larguraNova; }
    if (Rnovo > LARGURA - 2) { Rnovo = LARGURA - 2; Lnovo = Rnovo - larguraNova; }

    margemEsq[0] = Lnovo;
    margemDir[0] = Rnovo;
}

// ============================================================================
// DESENHO
// ============================================================================

// Desenha o avião: percorre o sprite e imprime cada célula não-vazia
void desenharAviao(const Player *p) {
    for (int r = 0; r < AVIAO_H; r++) {
        int y = p->y + r;
        for (int c = 0; c < AVIAO_W; c++) {
            char ch = AVIAO[r][c];
            if (ch == ' ') continue;  // ignora "vazios" do sprite
            int x = p->x + c;
            mvaddch(y, x, p->vivo ? ch : 'X'); // se morto, pinta 'X'
        }
    }
}

// Desenha um inimigo (sprite ASCII HxW)
void desenharInimigo(const Inimigo *in) {
    for (int r = 0; r < INIMIGO_H; r++) {
        int y = in->y + r; if (y < 0 || y >= ALTURA) continue;
        for (int c = 0; c < INIMIGO_W; c++) {
            char ch = INIMIGO[r][c]; if (ch == ' ') continue;
            int x = in->x + c; if (x >= 0 && x < LARGURA) mvaddch(y, x, ch);
        }
    }
}

// Desenha toda a cena: rio, inimigos, pickups, tiros, avião e HUD
void desenharTudo(const Player *p) {
    erase();            // limpa o buffer virtual

    // Ativa suporte a cores. (Obs.: para performance, poderia ser feito 1x)
    start_color();
    // Pares de cor (id, cor_texto, cor_fundo)
    init_pair(1, COLOR_GREEN,  COLOR_BLACK);   // margens/terra
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);   // HUD / elementos destaque
    init_pair(3, COLOR_RED,    COLOR_BLACK);   // inimigos
    init_pair(4, COLOR_MAGENTA,COLOR_BLACK);   // pickups [FUEL]
    init_pair(5, COLOR_BLUE,   COLOR_BLACK);   // água
    init_pair(6, COLOR_BLACK,  COLOR_YELLOW);  // HUD invertido

    // ----- RIO (toda a tela) -----
    for (int y = 0; y < ALTURA; y++) {
        int L = margemEsq[y];
        int R = margemDir[y];

        // Esquerda sólida (terra)
        attron(COLOR_PAIR(1));
        for (int x = 0; x <= L; x++) mvaddch(y, x, '#');
        attroff(COLOR_PAIR(1));

        // Água do rio (espaço). Pintamos com a cor de água para diferenciar
        attron(COLOR_PAIR(5));
        for (int x = L + 1; x < R; x++) mvaddch(y, x, ' ');
        attroff(COLOR_PAIR(5));   // <— corrige o par correto aqui

        // Direita sólida (terra)
        attron(COLOR_PAIR(1));
        for (int x = R; x < LARGURA; x++) mvaddch(y, x, '#');
        attroff(COLOR_PAIR(1));
    }

    // ----- INIMIGOS -----
    attron(COLOR_PAIR(3));
    for (int i = 0; i < INIMIGOS_MAX; i++) {
        if (inimigos[i].vivo) desenharInimigo(&inimigos[i]);
    }
    attroff(COLOR_PAIR(3));

    // ----- PICKUPS [FUEL] -----
    attron(COLOR_PAIR(4));
    for (int i = 0; i < GASOLINA_MAX; i++) {
        if (postos[i].vivo) mvprintw(postos[i].y, postos[i].x, "[FUEL]");
    }
    attroff(COLOR_PAIR(4));

    // ----- TIROS -----
    attron(COLOR_PAIR(2));
    desenharBalas();
    attroff(COLOR_PAIR(2));

    // ----- AVIÃO -----
    attron(COLOR_PAIR(2));
    desenharAviao(p);
    attroff(COLOR_PAIR(2));

    // ----- HUD -----
    attron(COLOR_PAIR(6));
    mvprintw(0, 2, "SCORE: %ld  FUEL: %d  %s  | ESPACO=tiro  Q=sair",
             p->score, p->fuel,
             p->vivo ? "" : "[MORREU — R=recomecar]");
    attroff(COLOR_PAIR(6));

    refresh();          // mostra tudo de uma vez na tela real
}

// ============================================================================
// COLISÕES
// ----------------------------------------------------------------------------
// A colisão do avião é feita "pixel-a-pixel do sprite": percorremos todas as
// células NÃO-vazias do desenho e testamos contra margens e inimigos.
// ============================================================================

int haColisao(const Player *p) {
    // 1) Avião x Margens do rio (fora da água = colisão)
    for (int r = 0; r < AVIAO_H; r++) {
        int y = p->y + r; if (y < 0 || y >= ALTURA) continue;
        for (int c = 0; c < AVIAO_W; c++) {
            char ch = AVIAO[r][c]; if (ch == ' ') continue;
            int x = p->x + c;
            if (x <= margemEsq[y] || x >= margemDir[y]) return 1; // bateu
        }
    }

    // 2) Avião x Inimigos (AABB do sprite do inimigo)
    for (int r = 0; r < AVIAO_H; r++) {
        int y = p->y + r; if (y < 0 || y >= ALTURA) continue;
        for (int c = 0; c < AVIAO_W; c++) {
            char ch = AVIAO[r][c]; if (ch == ' ') continue;
            int x = p->x + c;
            for (int i = 0; i < INIMIGOS_MAX; i++) {
                if (inimigos[i].vivo &&
                    x >= inimigos[i].x && x < inimigos[i].x + INIMIGO_W &&
                    y >= inimigos[i].y && y < inimigos[i].y + INIMIGO_H) {
                    return 1; // encostou em qualquer parte do avião
                }
            }
        }
    }

    return 0; // sem colisão
}

// ============================================================================
// CICLO DE VIDA DO JOGO (reinício) + ARRAYS AUXILIARES
// ============================================================================

void reiniciarJogo(Player *p) {
    tick_usec = TICK_START_USEC;  // reinicia velocidade

    destruirBalas();              // limpa tiros existentes
    iniciarBalas();

    criarRioInicial();            // preenche o rio inicial (reto)

    // Posição inicial do avião (canto superior esquerdo do sprite)
    p->x = LARGURA / 2;           // centralizado (ajuste fino opcional)
    p->y = ALTURA  - 4;           // um pouco acima do rodapé
    p->vivo  = 1;
    p->score = 0;
    p->fuel  = 100;               // tanque cheio

    ReiniciarInimigos();
    ReiniciarGasolina();
}

void ReiniciarInimigos(void) {
    for (int i = 0; i < INIMIGOS_MAX; i++) inimigos[i].vivo = 0;
}

void ReiniciarGasolina(void) {
    for (int i = 0; i < GASOLINA_MAX; i++) postos[i].vivo = 0;
}
