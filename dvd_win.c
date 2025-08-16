#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>

#define LARGURA 40
#define ALTURA 20
#define SIMBOLO_DVD "DVD"
#define ESPACO_VAZIO ' '
#define ATRASO_TIQUE 100

// Estrutura para representar o logotipo "DVD" e sua posição/direção
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

// Função para reinicializar o logotipo "DVD" em uma posição e direção aleatória
void reiniciar_dvd()
{
    dvd.x = rand() % (LARGURA - 3);
    dvd.y = rand() % ALTURA;
    dvd.dx = (rand() % 2 == 0) ? 1 : -1;
    dvd.dy = (rand() % 2 == 0) ? 1 : -1;
}

// Função para desenhar o logotipo e atualizar o console
void desenhar_tela()
{
    // Limpar o buffer de console preenchendo-o com espaços vazios
    // Este loop varre todas as posições do buffer (LARGURA * ALTURA) e coloca ' '
    // Isso evita "resíduos" de quadros anteriores no console
    for (int i = 0; i < LARGURA * ALTURA; ++i)
    {
        bufferConsole[i].Char.AsciiChar = ESPACO_VAZIO;                                    // Preenche com espaços
        bufferConsole[i].Attributes = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED; // Define cor padrão
    }

    // Desenhar o logotipo "DVD" na posição atual
    // O índice do buffer é calculado com dvd.y * LARGURA + dvd.x, que mapeia
    // as coordenadas 2D (x, y) para o índice 1D no array linear `bufferConsole`
    int indice = dvd.y * LARGURA + dvd.x;
    bufferConsole[indice].Char.AsciiChar = SIMBOLO_DVD[0];
    bufferConsole[indice + 1].Char.AsciiChar = SIMBOLO_DVD[1];
    bufferConsole[indice + 2].Char.AsciiChar = SIMBOLO_DVD[2];

    bufferConsole[indice].Attributes = dvd.cor;
    bufferConsole[indice + 1].Attributes = dvd.cor;
    bufferConsole[indice + 2].Attributes = dvd.cor;

    // **Por que usamos o buffer?**
    // Usamos o buffer de console para preparar toda a "imagem" da tela na memória
    // O buffer contém todos os caracteres e cores que queremos desenhar
    // A função `WriteConsoleOutputA` copia o buffer inteiro para a tela do console
    // de uma vez, o que evita a cintilação (flickering) que ocorreria se atualizássemos
    // caractere por caractere diretamente na tela. O buffer permite uma atualização
    // suave da tela.

    // Após preencher o buffer, usamos a função `WriteConsoleOutputA`
    // para copiar o conteúdo do buffer para a tela do console
    // Isso atualiza a tela inteira de uma vez, evitando cintilação
    WriteConsoleOutputA(console, bufferConsole, tamanhoBuffer, posicaoCaractere, &areaEscritaConsole);
}

// Função para garantir que cores iguais nao se repitam em sequencia
void alterar_cor()
{
    WORD nova_cor;
    do
    {
        nova_cor = FOREGROUND_RED | (rand() % 2 ? FOREGROUND_GREEN : 0) | (rand() % 2 ? FOREGROUND_BLUE : 0);
    } while (nova_cor == dvd.cor);
    dvd.cor = nova_cor;
}

// Função para atualizar a posição do logotipo "DVD"
void atualizar_posicao_dvd()
{
    dvd.x += dvd.dx;
    dvd.y += dvd.dy;

    if (dvd.x <= 0 || dvd.x >= LARGURA - 3)
    { // -3 porque o "DVD" tem 3 caracteres
        dvd.dx *= -1;
        alterar_cor();
    }
    if (dvd.y <= 0 || dvd.y >= ALTURA - 1)
    {
        dvd.dy *= -1;
        alterar_cor();
    }
}

int main()
{
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