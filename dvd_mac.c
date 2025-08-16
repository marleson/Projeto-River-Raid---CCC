#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>
#include <ncurses.h>

#define LARGURA 80
#define ALTURA 24
#define ALTURA_LOGO 7
#define LARGURA_LOGO 30
#define ATRASO_TIQUE 100000 // microsegundos = 0.1s

// --- Prototipação das funções ---
void reiniciar_dvd();
void alterar_cor();
void desenhar_tela();
void atualizar_posicao_dvd();

const wchar_t *LOGO_DVD[ALTURA_LOGO] = {
    L"⠀⠀⣸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⢀⣾⣿⣿⣿⣿⣿⣿⣿⣿⣶⣦⡀",
    L"⠀⢠⣿⣿⡿⠀⠀⠈⢹⣿⣿⡿⣿⣿⣇⠀⣠⣿⣿⠟⣽⣿⣿⠇⠀⠀⢹⣿⣿⣿",
    L"⠀⢸⣿⣿⡇⠀⢀⣠⣾⣿⡿⠃⢹⣿⣿⣶⣿⡿⠋⢰⣿⣿⡿⠀⠀⣠⣼⣿⣿⠏",
    L"⠀⣿⣿⣿⣿⣿⣿⠿⠟⠋⠁⠀⠀⢿⣿⣿⠏⠀⠀⢸⣿⣿⣿⣿⣿⡿⠟⠋⠁⠀",
    L"⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣀⣀⣸⣟⣁⣀⣀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀",
    L"⣠⣴⣶⣾⣿⣿⣻⡟⣻⣿⢻⣿⡟⣛⢻⣿⡟⣛⣿⡿⣛⣛⢻⣿⣿⣶⣦⣄⡀⠀",
    L"⠉⠛⠻⠿⠿⠿⠷⣼⣿⣿⣼⣿⣧⣭⣼⣿⣧⣭⣿⣿⣬⡭⠾⠿⠿⠿⠛⠉⠀"};

typedef struct
{
    int x, y;
    int dx, dy;
    int cor;
} DVD;

DVD dvd;

void reiniciar_dvd()
{
    dvd.x = rand() % (LARGURA - LARGURA_LOGO);
    dvd.y = rand() % (ALTURA - ALTURA_LOGO);
    dvd.dx = (rand() % 2 == 0) ? 1 : -1;
    dvd.dy = (rand() % 2 == 0) ? 1 : -1;
    dvd.cor = 1;
}

void alterar_cor()
{
    int nova_cor;
    do
    {
        nova_cor = 1 + rand() % 6;
    } while (nova_cor == dvd.cor);
    dvd.cor = nova_cor;
}

void desenhar_tela()
{
    clear();
    attron(COLOR_PAIR(dvd.cor));
    for (int i = 0; i < ALTURA_LOGO; i++)
    {
        mvaddwstr(dvd.y + i, dvd.x, LOGO_DVD[i]);
    }
    attroff(COLOR_PAIR(dvd.cor));
    refresh();
}

void atualizar_posicao_dvd()
{
    dvd.x += dvd.dx;
    dvd.y += dvd.dy;

    if (dvd.x <= 0 || dvd.x >= LARGURA - LARGURA_LOGO)
    {
        dvd.dx *= -1;
        alterar_cor();
    }

    if (dvd.y <= 0 || dvd.y >= ALTURA - ALTURA_LOGO)
    {
        dvd.dy *= -1;
        alterar_cor();
    }
}

int main()
{
    setlocale(LC_ALL, "");
    srand(time(NULL));

    initscr();       // Inicializa ncurses
    noecho();        // Não mostrar entrada do usuário
    curs_set(FALSE); // Oculta cursor
    start_color();   // Ativa cores

    // Define pares de cores
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_CYAN, COLOR_BLACK);

    reiniciar_dvd();

    while (1)
    {
        atualizar_posicao_dvd();
        desenhar_tela();
        usleep(ATRASO_TIQUE);
    }

    endwin(); // Finaliza ncurses
    return 0;
}
