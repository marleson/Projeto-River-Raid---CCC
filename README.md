[river_raid_gasolina.c](https://github.com/user-attachments/files/22317810/river_raid_gasolina.c)
// ================================================================
// River Raid (versão de terminal) — C + ncurses
// NÍVEL: Iniciante 
// Objetivo: Criar um jogo simples de River Raid no terminal usando C e ncurses.
//           Agora com gasolina aparecendo de vez em quando.
// Controles: ←/→  ou  A/D  para mover;  R para reiniciar;  Q para sair
// Compilar:
//     gcc river_raid.c -o river_raid -lncurses
// Executar:
//     ./river_raid
// ================================================================

#include <ncurses.h> 
#include <stdlib.h>  
#include <time.h>    
#include <unistd.h>  

// ----------------------------
// ESTRUTURAS DE DADOS
// ----------------------------
typedef struct {
    int x;
    int y;
    int vivo;
    long score;
    int gasolina; // 0 a 100 (%)
} Player;

typedef struct {
    int x;
    int y;
    int ativo; // 1 = aparece, 0 = não existe
} Gasolina;

// ----------------------------
// VARIÁVEIS GLOBAIS
// ----------------------------
static int LARGURA = 0;
static int ALTURA = 0;

static int *margemEsq = NULL;
static int *margemDir = NULL;

static int LARGURA_MIN = 0;
static int LARGURA_MAX = 0;

static const useconds_t TICK_USEC = 60000; // velocidade do jogo

// ----------------------------
// PROTÓTIPOS
// ----------------------------
static void iniciarNcurses(void);
static void finalizarNcurses(void);
static void criarRioInicial(void);
static void gerarNovaLinhaNoTopo(Gasolina *g);
static void desenharTudo(const Player *p, const Gasolina *g);
static int haColisao(const Player *p);
static void reiniciarJogo(Player *p, Gasolina *g);

// ================================================================
// MAIN
// ================================================================
int main(void) {
    srand((unsigned)time(NULL));
    iniciarNcurses();

    Player jogador;
    Gasolina gas;
    reiniciarJogo(&jogador, &gas);

    while (1) {
        int ch = getch();

        if (ch == 'q' || ch == 'Q')
            break;

        if (jogador.vivo) {
            if (ch == KEY_LEFT || ch == 'a' || ch == 'A') jogador.x--;
            if (ch == KEY_RIGHT || ch == 'd' || ch == 'D') jogador.x++;

            if (jogador.x < 1) jogador.x = 1;
            if (jogador.x > LARGURA - 2) jogador.x = LARGURA - 2;

            gerarNovaLinhaNoTopo(&gas);
            jogador.score++;

            // Consome gasolina a cada linha
            if (jogador.gasolina > 0) jogador.gasolina--;
            else jogador.vivo = 0;

            // Colisão com margens
            if (haColisao(&jogador)) jogador.vivo = 0;

            // Coleta gasolina
            if (gas.ativo && gas.x == jogador.x && gas.y == jogador.y) {
                jogador.gasolina += 30; // recarrega
                if (jogador.gasolina > 100) jogador.gasolina = 100;
                gas.ativo = 0;
            }
        } else {
            if (ch == 'r' || ch == 'R')
                reiniciarJogo(&jogador, &gas);
        }

        desenharTudo(&jogador, &gas);
        usleep(TICK_USEC);
    }

    finalizarNcurses();
    return 0;
}

// ================================================================
// IMPLEMENTAÇÃO DAS FUNÇÕES
// ================================================================
static void iniciarNcurses(void) {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    getmaxyx(stdscr, ALTURA, LARGURA);

    if (ALTURA < 20 || LARGURA < 40) {
        endwin();
        fprintf(stderr, "Aumente o terminal para pelo menos 40x20.\n");
        exit(1);
    }

    LARGURA_MIN = LARGURA / 3;
    LARGURA_MAX = LARGURA / 2;

    margemEsq = (int*)malloc(sizeof(int) * ALTURA);
    margemDir = (int*)malloc(sizeof(int) * ALTURA);
    if (!margemEsq || !margemDir) {
        endwin();
        fprintf(stderr, "Falha ao alocar memória.\n");
        exit(1);
    }
}

static void finalizarNcurses(void) {
    endwin();
    free(margemEsq);
    free(margemDir);
}

static void criarRioInicial(void) {
    int centro = LARGURA / 2;
    int larguraRio = LARGURA / 2;
    if (larguraRio < LARGURA_MIN) larguraRio = LARGURA_MIN;
    if (larguraRio > LARGURA_MAX) larguraRio = LARGURA_MAX;

    int metade = larguraRio / 2;
    int L = centro - metade;
    int R = centro + metade;

    for (int y = 0; y < ALTURA; y++) {
        margemEsq[y] = L;
        margemDir[y] = R;
    }
}

static void gerarNovaLinhaNoTopo(Gasolina *g) {
    for (int y = ALTURA - 1; y > 0; y--) {
        margemEsq[y] = margemEsq[y - 1];
        margemDir[y] = margemDir[y - 1];
    }

    int Lant = margemEsq[0];
    int Rant = margemDir[0];
    int centroAnt = (Lant + Rant) / 2;
    int larguraAnt = (Rant - Lant);

    int centroNovo = centroAnt + (rand() % 3 - 1);
    int larguraNova = larguraAnt + (rand() % 3 - 1);

    if (larguraNova < LARGURA_MIN) larguraNova = LARGURA_MIN;
    if (larguraNova > LARGURA_MAX) larguraNova = LARGURA_MAX;

    int metade = larguraNova / 2;
    int Lnovo = centroNovo - metade;
    int Rnovo = centroNovo + metade;

    if (Lnovo < 1) { Lnovo = 1; Rnovo = Lnovo + larguraNova; }
    if (Rnovo > LARGURA - 2) { Rnovo = LARGURA - 2; Lnovo = Rnovo - larguraNova; }

    margemEsq[0] = Lnovo;
    margemDir[0] = Rnovo;

    // gasolina aparece no topo às vezes
    if (!g->ativo && rand() % 25 == 0) {
        g->x = Lnovo + 1 + rand() % (larguraNova - 2);
        g->y = 0;
        g->ativo = 1;
    } else if (g->ativo) {
        g->y++;
        if (g->y >= ALTURA) g->ativo = 0;
    }
}

static void desenharTudo(const Player *p, const Gasolina *g) {
    erase();

    for (int y = 0; y < ALTURA; y++) {
        int L = margemEsq[y];
        int R = margemDir[y];

        for (int x = 0; x <= L; x++) mvaddch(y, x, '#');
        for (int x = L + 1; x < R; x++) mvaddch(y, x, ' ');
        for (int x = R; x < LARGURA; x++) mvaddch(y, x, '#');
    }

    if (p->vivo) mvaddch(p->y, p->x, '^');
    else mvaddch(p->y, p->x, 'X');

    if (g->ativo) mvaddch(g->y, g->x, 'G');

    mvprintw(0, 2, "SCORE: %ld", p->score);

    // barra de gasolina
    int barraLen = 20;
    int cheio = (p->gasolina * barraLen) / 100;
    mvprintw(1, 2, "GASOLINA: [");
    for (int i = 0; i < barraLen; i++) {
        if (i < cheio) addch('|');
        else addch(' ');
    }
    addch(']');
    mvprintw(1, 25, "%d%%", p->gasolina);

    if (!p->vivo)
        mvprintw(2, 2, "[MORREU - pressione R para reiniciar]");

    refresh();
}

static int haColisao(const Player *p) {
    return (p->x <= margemEsq[p->y]) || (p->x >= margemDir[p->y]);
}

static void reiniciarJogo(Player *p, Gasolina *g) {
    criarRioInicial();
    p->x = LARGURA / 2;
    p->y = ALTURA - 4;
    p->vivo = 1;
    p->score = 0;
    p->gasolina = 100;
    g->ativo = 0;
}
