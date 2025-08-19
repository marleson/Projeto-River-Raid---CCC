// ================================================================
// River Raid (versão de terminal) — C + ncurses
// NÍVEL: Iniciante 
// Objetivo: Criar um jogo simples de River Raid no terminal usando C e ncurses.
//           O jogo envolve um jogador (avião) que deve navegar por um rio, evitando as margens.
// Controles: ←/→  ou  A/D  para mover;  R para reiniciar;  Q para sair
// Compilar (macOS):
//   Opção A (usa a ncurses e gcc):
//     gcc river_raid.c  -o river_raid -lncurses
//   Opção B (usa a ncurses do sistema):
//     clang -std=c11 -Wall -Wextra -O2 river_raid.c -lncurses -o river_raid
//   Opção C (se precisar da do Homebrew):
//     clang -std=c11 -Wall -Wextra -O2 \
//       -I"$(brew --prefix)/opt/ncurses/include" \
//       -L"$(brew --prefix)/opt/ncurses/lib" \
//       -Wl,-rpath,"$(brew --prefix)/opt/ncurses/lib" \
//       river_raid.c -lncurses -o river_raid
// Executar:
//     ./river_raid      
// ================================================================

#include <ncurses.h> // biblioteca para desenhar no terminal
#include <stdlib.h>  // malloc, free, rand
#include <time.h>    // time (semente do rand)
#include <unistd.h>  // usleep (pausa em microsegundos)

// ----------------------------
// ESTRUTURAS DE DADOS SIMPLES
// ----------------------------
// Representa o jogador (avião)
typedef struct
{
    int x;      // coluna (horizontal)
    int y;      // linha (vertical)
    int vivo;   // 1 = vivo; 0 = morto (colisão)
    long score; // pontuação simples: conta linhas percorridas
} Player; 

// Dimensões atuais do terminal (serão lidas na inicialização)
static int LARGURA = 0; // número de colunas
static int ALTURA = 0;  // número de linhas

// Vetores que guardam as margens do rio para CADA linha da tela.
// Para cada y (linha), temos:
//   margemEsq[y]  = posição da margem esquerda
//   margemDir[y]  = posição da margem direita
static int *margemEsq = NULL;
static int *margemDir = NULL;

// Parâmetros da largura do rio (em colunas)
static int LARGURA_MIN = 0; // rio nunca fica mais estreito que isso
static int LARGURA_MAX = 0; // nem mais largo que isso

// Velocidade do jogo (quanto menor, mais rápido)
// 40000 µs ≈ 0.04 s por quadro ≈ ~25 FPS
static const useconds_t TICK_USEC = 40000;

// ----------------------------
// FUNÇÕES — CABEÇALHOS (protótipos)
// ----------------------------
static void iniciarNcurses(void);
static void finalizarNcurses(void);
static void criarRioInicial(void);
static void gerarNovaLinhaNoTopo(void);
static void desenharTudo(const Player *p);
static int haColisao(const Player *p);
static void reiniciarJogo(Player *p);

// ================================================================
// FUNÇÃO PRINCIPAL
// ================================================================
int main(void)
{
    // Semente para números aleatórios (deixamos o rio "balançar")
    srand((unsigned)time(NULL));

    iniciarNcurses();

    Player jogador;
    reiniciarJogo(&jogador); // posiciona jogador e cria rio inicial

    while (1)
    {
        // Leitura de tecla SEM travar o jogo (nodelay foi ativado)
        int ch = getch();

        // Tecla para sair
        if (ch == 'q' || ch == 'Q')
        {
            break; // encerra o loop e finaliza ncurses
        }

        if (jogador.vivo)
        {
            // MOVIMENTO HORIZONTAL do avião
            if (ch == KEY_LEFT || ch == 'a' || ch == 'A')
                jogador.x--;
            if (ch == KEY_RIGHT || ch == 'd' || ch == 'D')
                jogador.x++;

            // Mantém o jogador dentro da tela (1 até LARGURA-2)
            if (jogador.x < 1)
                jogador.x = 1;
            if (jogador.x > LARGURA - 2)
                jogador.x = LARGURA - 2;

            // Faz o MUNDO descer uma linha e gera uma nova no topo
            gerarNovaLinhaNoTopo();
            jogador.score++; // andou mais uma linha → ganha pontos

            // Verifica colisão com as margens do rio
            if (haColisao(&jogador))
            {
                jogador.vivo = 0; // game over (morreu)
            }
        }
        else
        {
            // Se já morreu, permite reiniciar com R
            if (ch == 'r' || ch == 'R')
            {
                reiniciarJogo(&jogador);
            }
        }

        // Desenha toda a tela (rio, HUD e avião)
        desenharTudo(&jogador);

        // Pequena pausa para controlar a velocidade do jogo
        usleep(TICK_USEC);
    }

    finalizarNcurses();
    return 0;
}

// ================================================================
// IMPLEMENTAÇÕES DAS FUNÇÕES
// ================================================================

// Inicializa a ncurses e configura o terminal para o jogo
static void iniciarNcurses(void)
{
    initscr();             // inicia modo ncurses
    cbreak();              // lê tecla sem precisar de ENTER
    noecho();              // não mostra as teclas digitadas na tela
    curs_set(0);           // esconde o cursor
    keypad(stdscr, TRUE);  // permite usar teclas especiais (setas)
    nodelay(stdscr, TRUE); // getch() não bloqueia

    // Lê o tamanho atual do terminal
    getmaxyx(stdscr, ALTURA, LARGURA);

    // Validação simples do tamanho mínimo
    if (ALTURA < 20 || LARGURA < 40)
    {
        endwin();
        fprintf(stderr, "Aumente o terminal para pelo menos 40x20.\n");
        exit(1);
    }

    // Define larguras mínima e máxima do rio baseadas no tamanho da tela
    LARGURA_MIN = LARGURA / 3; // mais estreito permitido (talvez melhor 3)
    LARGURA_MAX = LARGURA / 2; // mais largo permitido

    // Aloca os vetores das margens (um valor por linha)
    margemEsq = (int *)malloc(sizeof(int) * ALTURA);
    margemDir = (int *)malloc(sizeof(int) * ALTURA);
    if (!margemEsq || !margemDir)
    {
        endwin();
        fprintf(stderr, "Falha ao alocar memória para o rio.\n");
        exit(1);
    }
}

// Finaliza a ncurses e libera memória
static void finalizarNcurses(void)
{
    endwin();
    free(margemEsq);
    free(margemDir);
}

// Cria um rio "reto" inicial (mesma largura em todas as linhas)
static void criarRioInicial(void)
{
    int centro = LARGURA / 2;     // começa no meio da tela
    int larguraRio = LARGURA / 2; // largura inicial
    if (larguraRio < LARGURA_MIN)
        larguraRio = LARGURA_MIN;
    if (larguraRio > LARGURA_MAX)
        larguraRio = LARGURA_MAX;

    int metade = larguraRio / 2;
    int L = centro - metade; // margem esquerda
    int R = centro + metade; // margem direita

    // Preenche TODAS as linhas com as mesmas margens iniciais
    for (int y = 0; y < ALTURA; y++)
    {
        margemEsq[y] = L;
        margemDir[y] = R;
    }
}

// Faz o mundo "descer" (shift para baixo) e cria uma nova linha no topo
// com pequenas variações (curvas suaves e largura mudando de leve)
static void gerarNovaLinhaNoTopo(void)
{
    // 1) Empurra todas as linhas para BAIXO (de ALTURA-1 até 1)
    for (int y = ALTURA - 1; y > 0; y--)
    {
        margemEsq[y] = margemEsq[y - 1];
        margemDir[y] = margemDir[y - 1];
    }

    // 2) Calcula a nova linha 0 (topo) baseada na linha 0 anterior
    int Lant = margemEsq[0];
    int Rant = margemDir[0];
    int centroAnt = (Lant + Rant) / 2;
    int larguraAnt = (Rant - Lant);

    // Pequenas variações aleatórias (−1, 0, +1)
    int centroNovo = centroAnt + (rand() % 3 - 1);
    int larguraNova = larguraAnt + (rand() % 3 - 1);

    // Mantém dentro dos limites definidos
    if (larguraNova < LARGURA_MIN)
        larguraNova = LARGURA_MIN;
    if (larguraNova > LARGURA_MAX)
        larguraNova = LARGURA_MAX;

    int metade = larguraNova / 2;
    int Lnovo = centroNovo - metade;
    int Rnovo = centroNovo + metade;

    // Garante que as margens fiquem dentro da tela (1 até LARGURA-2)
    if (Lnovo < 1)
    {
        Lnovo = 1;
        Rnovo = Lnovo + larguraNova;
    }
    if (Rnovo > LARGURA - 2)
    {
        Rnovo = LARGURA - 2;
        Lnovo = Rnovo - larguraNova;
    }

    // 3) Escreve a nova linha no topo
    margemEsq[0] = Lnovo;
    margemDir[0] = Rnovo;
}

// Desenha todo o conteúdo na tela: rio, HUD e jogador
static void desenharTudo(const Player *p)
{
    erase(); // limpa a tela virtual (ncurses usa double-buffering)

    // Desenha o rio, linha por linha
    for (int y = 0; y < ALTURA; y++)
    {
        int L = margemEsq[y];
        int R = margemDir[y];

        // Parte sólida à esquerda (margem + "terra" fora do rio)
        for (int x = 0; x <= L; x++)
        {
            mvaddch(y, x, '#');
        }
        // Parte de água do rio (espaços em branco)
        for (int x = L + 1; x < R; x++)
        {
            mvaddch(y, x, ' ');
        }
        // Parte sólida à direita
        for (int x = R; x < LARGURA; x++)
        {
            mvaddch(y, x, '#');
        }
    }

    // Desenha o jogador: '^' se vivo, 'X' se morto
    if (p->vivo)
        mvaddch(p->y, p->x, '^');
    else
        mvaddch(p->y, p->x, 'X');

    // HUD (informações no topo)
    mvprintw(0, 2, "SCORE: %ld  %s",
             p->score,
             p->vivo ? "" : "[MORREU — pressione R para reiniciar]");

    // Mostra tudo de uma vez na tela real
    refresh();
}

// Verifica se o jogador bateu na margem do rio
static int haColisao(const Player *p)
{
    // Se a posição X do jogador for MENOR/IGUAL à margem esquerda
    // ou MAIOR/IGUAL à margem direita da linha onde ele está,
    // então houve colisão.
    return (p->x <= margemEsq[p->y]) || (p->x >= margemDir[p->y]);
}

// Reinicia o estado do jogo (novo rio, jogador vivo no mesmo lugar)
static void reiniciarJogo(Player *p)
{
    criarRioInicial();  // preenche as margens para todas as linhas
    p->x = LARGURA / 2; // começa no meio (horizontal)
    p->y = ALTURA - 4;  // um pouco acima do rodapé
    p->vivo = 1;        // jogador está vivo
    p->score = 0;       // zera pontuação
}
