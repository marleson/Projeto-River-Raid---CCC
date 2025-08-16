#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <locale.h>

#define LARGURA 80
#define ALTURA 24
#define ALTURA_LOGO 7
#define LARGURA_LOGO 30
#define ATRASO_TIQUE 100

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
    WORD cor;

} DVD;

DVD dvd;
HANDLE console;

CHAR_INFO bufferConsole[LARGURA * ALTURA];

COORD tamanhoBuffer = {LARGURA, ALTURA};
COORD posicaoCaractere = {0, 0};

SMALL_RECT areaEscritaConsole = {0, 0, LARGURA - 1, ALTURA - 1};

void reiniciar_dvd()
{
    dvd.x = rand() % (LARGURA - LARGURA_LOGO);
    dvd.y = rand() % (ALTURA - ALTURA_LOGO);
    dvd.dx = (rand() % 2 == 0) ? 1 : -1;
    dvd.dy = (rand() % 2 == 0) ? 1 : -1;
    dvd.cor = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
}

void alterar_cor()
{
    WORD nova_cor;
    do
    {
        nova_cor = FOREGROUND_RED | (rand() % 2 ? FOREGROUND_GREEN : 0) | (rand() % 2 ? FOREGROUND_BLUE : 0);
    } while (nova_cor == dvd.cor);
    dvd.cor = nova_cor;
}

void desenhar_tela()
{
    for (int i = 0; i < LARGURA * ALTURA; ++i)
    {
        bufferConsole[i].Char.UnicodeChar = L' ';
        bufferConsole[i].Attributes = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
    }

    for (int i = 0; i < ALTURA_LOGO; ++i)
    {
        for (int j = 0; j < LARGURA_LOGO; ++j)
        {
            int indice = (dvd.y + i) * LARGURA + (dvd.x + j);
            bufferConsole[indice].Char.UnicodeChar = LOGO_DVD[i][j];
            bufferConsole[indice].Attributes = dvd.cor;
        }
    }

    WriteConsoleOutputW(console, bufferConsole, tamanhoBuffer, posicaoCaractere, &areaEscritaConsole);
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
    reiniciar_dvd();
    console = GetStdHandle(STD_OUTPUT_HANDLE);

    while (1)
    {
        atualizar_posicao_dvd();
        desenhar_tela();
        Sleep(ATRASO_TIQUE);
    }

    return 0;
}
