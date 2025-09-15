// ================================================================
// River Raid (versão de terminal) — C + ncurses
// NÍVEL: Iniciante
// Objetivo: Criar um jogo simples de River Raid no terminal usando C e ncurses.
//           O jogo envolve um jogador (avião) que deve navegar por um rio, evitando as margens.
// Controles: ←/→  ou  A/D  para mover;  R para reiniciar;  Q para sair
// Compilar (macOS):
//     gcc river_raid.c  -o river_raid -lncurses
// Executar:
//     ./river_raid
// ================================================================

#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

// ----------------------------
// ESTRUTURAS DE DADOS SIMPLES
// ----------------------------
typedef struct
{
    int x;
    int y;
    int vivo;
    long score;
    int fuel; // NOVO: combustível
} Player;

typedef struct
{
    int x;
    int y;
    int vivo;
} Inimigo;

#define INIMIGOS_MAX 20
static Inimigo inimigos[INIMIGOS_MAX];

// ============================
// TIROS
// ============================
typedef struct Bala
{
    int x, y;
    struct Bala *prox;
} Bala;

static Bala *balas = NULL;

// ============================
// GASOLINA
// ============================
typedef struct
{
    int x;
    int y;
    int vivo;
} Gasolina;

#define GASOLINA_MAX 5
static Gasolina postos[GASOLINA_MAX];

// ----------------------------
static int LARGURA = 0;
static int ALTURA = 0;

static int *margemEsq = NULL;
static int *margemDir = NULL;

#define AVIAO_H 3
#define AVIAO_W 5
static const char *AVIAO[AVIAO_H] = {
    "  ^  ",
    "<-A->",
    " / \\ "};

#define INIMIGO_H 3
#define INIMIGO_W 5

static const char *INIMIGO[INIMIGO_H] = {
    " | | ",
    "\\ooo/",
    " === "};

static int LARGURA_MIN = 0;
static int LARGURA_MAX = 0;

static const useconds_t TICK_USEC = 40000;

// ----------------------------
static void iniciarNcurses(void);
static void finalizarNcurses(void);
static void criarRioInicial(void);
static void gerarNovaLinhaNoTopo(void);
static void desenharTudo(const Player *p);
static void desenharAviao(const Player *p);
static int haColisao(const Player *p);
static void ReiniciarInimigos(void);
static void reiniciarJogo(Player *p);
// Tiros
static void iniciarBalas(void);
static void destruirBalas(void);
static void disparar(const Player *p);
static void atualizarBalas(void);
static void desenharBalas(void);
// Gasolina
static void ReiniciarGasolina(void);

// ================================================================
// MAIN
// ================================================================
int main(void)
{
    srand((unsigned)time(NULL));
    iniciarNcurses();

    Player jogador;
    reiniciarJogo(&jogador);

    int contadorSpawn = 0;
    int limiteSpawn = 20;

    while (1)
    {
        int ch = getch();
        if (ch == 'q' || ch == 'Q')
            break;

        if (jogador.vivo)
        {
            if (ch == KEY_LEFT || ch == 'a' || ch == 'A')
                jogador.x--;
            if (ch == KEY_RIGHT || ch == 'd' || ch == 'D')
                jogador.x++;
            if (ch == ' ')
                disparar(&jogador);

            if (jogador.x < 1)
                jogador.x = 1;
            int maxX = LARGURA - AVIAO_W - 1;
            if (jogador.x > maxX)
                jogador.x = maxX;

            gerarNovaLinhaNoTopo();

            for (int i = 0; i < INIMIGOS_MAX; i++)
            {
                if (inimigos[i].vivo)
                {
                    inimigos[i].y++;
                    if (inimigos[i].y >= ALTURA)
                        inimigos[i].vivo = 0;
                }
            }

            contadorSpawn++;
            if (contadorSpawn > limiteSpawn)
            {
                contadorSpawn = 0;
                for (int i = 0; i < INIMIGOS_MAX; i++)
                {
                    if (!inimigos[i].vivo)
                    {
                        inimigos[i].vivo = 1;
                        inimigos[i].y = 0;
                        inimigos[i].x = margemEsq[0] + 1 + rand() % (margemDir[0] - margemEsq[0] - INIMIGO_W - 1);
                        break;
                    }
                }
                    
            }  

            // ======== GASOLINA DESCENDO =========
            for (int i = 0; i < GASOLINA_MAX; i++)
            {
                if (postos[i].vivo)
                {
                    postos[i].y++;
                    if (postos[i].y >= ALTURA)
                        postos[i].vivo = 0;
                }
            }

            // ======== NASCER GASOLINA =========
            if (rand() % 100 < 3)
            {
                for (int i = 0; i < GASOLINA_MAX; i++)
                {
                    if (!postos[i].vivo)
                    {
                        postos[i].vivo = 1;
                        postos[i].y = 0;
                        postos[i].x = margemEsq[0] + 1 + rand() % (margemDir[0] - margemEsq[0] - 2);
                        break;
                    }
                }
            }

            atualizarBalas();

            jogador.score++;

            if (jogador.score % 500 == 0 && limiteSpawn > 3)
                limiteSpawn--;

            
            // Coleta gasolina
            for (int i = 0; i < GASOLINA_MAX; i++)
            {
                if (postos[i].vivo &&
                    postos[i].x >= jogador.x &&
                    postos[i].x < jogador.x + AVIAO_W &&
                    postos[i].y >= jogador.y &&
                    postos[i].y < jogador.y + AVIAO_H)
                {
                    postos[i].vivo = 0;
                    jogador.fuel = 100;
                }
            }

            // Consome combustível
            jogador.fuel--;
            if (jogador.fuel <= 0)
                jogador.vivo = 0;


            if (haColisao(&jogador))
                jogador.vivo = 0;
        }
        else
        {
            if (ch == 'r' || ch == 'R')
                reiniciarJogo(&jogador);
        }

        desenharTudo(&jogador);
        usleep(TICK_USEC);
    }

    finalizarNcurses();
    return 0;
}

// ================================================================
// BALAS
// ================================================================
static void iniciarBalas(void) { balas = NULL; }

static void destruirBalas(void)
{
    while (balas)
    {
        Bala *tmp = balas;
        balas = balas->prox;
        free(tmp);
    }
}

static void disparar(const Player *p)
{
    int narizX = p->x + AVIAO_W / 2;
    int startY = p->y - 1;
    if (startY <= 0)
        return;

    Bala *b = (Bala *)malloc(sizeof(Bala));
    if (!b)
        return;
    b->x = narizX;
    b->y = startY;
    b->prox = balas;
    balas = b;
}

static void atualizarBalas(void)
{
    Bala **pp = &balas;
    while (*pp)
    {
        Bala *b = *pp;
        b->y--;

        int remover = 0;

        if (b->y <= 0)
        {
            remover = 1;
        }
        else
        {
            if (b->x <= margemEsq[b->y] || b->x >= margemDir[b->y])
            {
                remover = 1;
            }
            else
            {
                for (int i = 0; i < INIMIGOS_MAX; i++)
                {
                    if (inimigos[i].vivo &&
                        b->x >= inimigos[i].x &&
                        b->x < inimigos[i].x + INIMIGO_W &&
                        b->y >= inimigos[i].y &&
                        b->y < inimigos[i].y + INIMIGO_H)
                    {
                        inimigos[i].vivo = 0;
                        remover = 1;
                        break;
                    }
                }
            }
        }

        if (remover)
        {
            *pp = b->prox;
            free(b);
        }
        else
        {
            pp = &b->prox;
        }
    }
}

static void desenharBalas(void)
{
    for (Bala *b = balas; b != NULL; b = b->prox)
    {
        mvaddch(b->y, b->x, '|');
    }
}

// ================================================================
// NCURSES E RIO
// ================================================================
static void iniciarNcurses(void)
{
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    getmaxyx(stdscr, ALTURA, LARGURA);

    if (ALTURA < 20 || LARGURA < 40)
    {
        endwin();
        fprintf(stderr, "Aumente o terminal para pelo menos 40x20.\n");
        exit(1);
    }

    LARGURA_MIN = LARGURA / 3;
    LARGURA_MAX = LARGURA / 2;

    margemEsq = (int *)malloc(sizeof(int) * ALTURA);
    margemDir = (int *)malloc(sizeof(int) * ALTURA);
    if (!margemEsq || !margemDir)
    {
        endwin();
        fprintf(stderr, "Falha ao alocar memória.\n");
        exit(1);
    }
}

static void finalizarNcurses(void)
{
    destruirBalas();
    endwin();
    free(margemEsq);
    free(margemDir);
}

static void criarRioInicial(void)
{
    int centro = LARGURA / 2;
    int larguraRio = LARGURA / 2;
    if (larguraRio < LARGURA_MIN)
        larguraRio = LARGURA_MIN;
    if (larguraRio > LARGURA_MAX)
        larguraRio = LARGURA_MAX;

    int metade = larguraRio / 2;
    int L = centro - metade;
    int R = centro + metade;

    for (int y = 0; y < ALTURA; y++)
    {
        margemEsq[y] = L;
        margemDir[y] = R;
    }
}

static void gerarNovaLinhaNoTopo(void)
{
    for (int y = ALTURA - 1; y > 0; y--)
    {
        margemEsq[y] = margemEsq[y - 1];
        margemDir[y] = margemDir[y - 1];
    }

    int Lant = margemEsq[0];
    int Rant = margemDir[0];
    int centroAnt = (Lant + Rant) / 2;
    int larguraAnt = (Rant - Lant);

    int centroNovo = centroAnt + (rand() % 3 - 1);
    int larguraNova = larguraAnt + (rand() % 3 - 1);

    if (larguraNova < LARGURA_MIN)
        larguraNova = LARGURA_MIN;
    if (larguraNova > LARGURA_MAX)
        larguraNova = LARGURA_MAX;

    int metade = larguraNova / 2;
    int Lnovo = centroNovo - metade;
    int Rnovo = centroNovo + metade;

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

    margemEsq[0] = Lnovo;
    margemDir[0] = Rnovo;
}

// ================================================================
// DESENHO
// ================================================================
static void desenharAviao(const Player *p)
{
    for (int r = 0; r < AVIAO_H; r++)
    {
        int y = p->y + r;
        for (int c = 0; c < AVIAO_W; c++)
        {
            char ch = AVIAO[r][c];
            if (ch == ' ')
                continue;
            int x = p->x + c;
            mvaddch(y, x, p->vivo ? ch : 'X');
        }
    }
}

static void desenharInimigo(const Inimigo *in)

{
   for (int r = 0; r < INIMIGO_H; r++)
   {
    int y = in->y + r;
    if (y < 0 || y >= ALTURA) continue;

    for (int c = 0; c < INIMIGO_W; c++)
    {
         char ch = INIMIGO[r][c];
         if (ch == ' ')continue;
         
         int x = in-> x + c;
         if (x >= 0 && x < LARGURA)
            mvaddch(y,x,ch);
    }
   }
}



static void desenharTudo(const Player *p)
{
    erase();

    for (int y = 0; y < ALTURA; y++)
    {
        int L = margemEsq[y];
        int R = margemDir[y];

        for (int x = 0; x <= L; x++)
            mvaddch(y, x, '#');
        for (int x = L + 1; x < R; x++)
            mvaddch(y, x, ' ');
        for (int x = R; x < LARGURA; x++)
            mvaddch(y, x, '#');
    }

    for (int i = 0; i < INIMIGOS_MAX; i++)
    {
        if (inimigos[i].vivo)
            desenharInimigo(&inimigos[i]);
    }

    for (int i = 0; i < GASOLINA_MAX; i++)
    {
        if (postos[i].vivo)
            mvaddch(postos[i].y, postos[i].x, 'G');
    }

    desenharBalas();
    desenharAviao(p);

    mvprintw(0, 2, "SCORE: %ld  FUEL: %d  %s  | ESPACO=tiro  Q=sair",
             p->score, p->fuel,
             p->vivo ? "" : "[MORREU — R=recomecar]");

    refresh();
}

// ================================================================
// COLISÃO E RESET
// ================================================================
static int haColisao(const Player *p)
{
    for (int r = 0; r < AVIAO_H; r++)
    {
        int y = p->y + r;
        if (y < 0 || y >= ALTURA)
            continue;
        for (int c = 0; c < AVIAO_W; c++)
        {
            char ch = AVIAO[r][c];
            if (ch == ' ')
                continue;
            int x = p->x + c;
            if (x <= margemEsq[y] || x >= margemDir[y])
                return 1;
        }
    }

    for (int r = 0; r < AVIAO_H; r++)
    {
        int y = p->y + r;
        if (y < 0 || y >= ALTURA)
            continue;
        for (int c = 0; c < AVIAO_W; c++)
        {
            char ch = AVIAO[r][c];
            if (ch == ' ')
                continue;
            int x = p->x + c;
            for (int i = 0; i < INIMIGOS_MAX; i++)
            {
                if (inimigos[i].vivo &&
                    x >= inimigos[i].x && 
                    x < inimigos[i].x + INIMIGO_W &&
                    y >= inimigos[i].y &&
                    y < inimigos[i].y +INIMIGO_H)
                    return 1;
            }
        }
    }

    return 0;
}

static void reiniciarJogo(Player *p)
{
    destruirBalas();
    iniciarBalas();
    criarRioInicial();
    p->x = LARGURA / 2;
    p->y = ALTURA - 4;
    p->vivo = 1;
    p->score = 0;
    p->fuel = 100; // NOVO: tanque cheio
    ReiniciarInimigos();
    ReiniciarGasolina();
}

static void ReiniciarInimigos(void)
{
    for (int i = 0; i < INIMIGOS_MAX; i++)
        inimigos[i].vivo = 0;
}

static void ReiniciarGasolina(void)
{
    for (int i = 0; i < GASOLINA_MAX; i++)
        postos[i].vivo = 0;
}
