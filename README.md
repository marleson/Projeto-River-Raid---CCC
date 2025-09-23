# River Raid (Terminal) — C + ncurses

Um clone didático do River Raid rodando **no terminal**, escrito em **C** usando **ncurses**. O objetivo é pilotar o avião pelo rio, desviando das margens e de inimigos, gerenciando o combustível e abatendo ameaças com tiros.

> Projeto pensado para estudantes de **primeiro ano**: funções curtas, código comentado e arquitetura direta.&#x20;

## ✨ Recursos

* Jogo 2D em modo texto com **ncurses**
* Avião do jogador em **ASCII 3×5**
* Inimigos em **ASCII 3×5** (descem a tela)
* **Tiros ilimitados** (lista ligada)
* **Combustível** que diminui com o tempo e **coleta** de `[FUEL]`
* **Rio procedural** com margens variando suavemente
* **Dificuldade dinâmica**: acelera conforme o score aumenta

## 🎮 Controles

* **Mover**: `←`/`→` ou `A`/`D`
* **Atirar**: `ESPAÇO`
* **Reiniciar**: `R`
* **Sair**: `Q`

## 🧰 Dependências

* **C toolchain** (Clang ou GCC)
* **ncurses** (já vem no macOS e na maioria das distros Linux)

## 🛠️ Como compilar

### macOS (Clang)

```bash
# normalmente basta:
clang -std=c11 -Wall -Wextra -O2 river_raid.c -lncurses -o river_raid
```

Se o seu mac não linkar a `ncurses` do sistema, use a do Homebrew:

```bash
brew install ncurses

clang -std=c11 -Wall -Wextra -O2 \
  -I"$(brew --prefix)/opt/ncurses/include" \
  -L"$(brew --prefix)/opt/ncurses/lib" \
  -Wl,-rpath,"$(brew --prefix)/opt/ncurses/lib" \
  river_raid.c -lncurses -o river_raid
```

### Linux (GCC/Clang)

```bash
gcc -std=c11 -Wall -Wextra -O2 river_raid.c -lncurses -o river_raid
# ou
clang -std=c11 -Wall -Wextra -O2 river_raid.c -lncurses -o river_raid
```

## ▶️ Como executar

```bash
./river_raid
```

## 🗂️ Organização do código

Projeto em **arquivo único**: `river_raid.c`.

Principais blocos/funções:

* **Inicialização/loop**: `iniciarNcurses`, `finalizarNcurses`, `reiniciarJogo`, `main`
* **Mundo (rio)**: `criarRioInicial`, `gerarNovaLinhaNoTopo`, vetores `margemEsq/margemDir`
* **Desenho**: `desenharTudo`, `desenharAviao`, `desenharInimigo`, `desenharBalas`
* **Jogador**: struct `Player` (pos, vivo, score, fuel) e `haColisao`
* **Inimigos**: array `inimigos[INIMIGOS_MAX]`, spawn/descida simples
* **Tiros**: lista ligada `Bala`, com `disparar`, `atualizarBalas`, `destruirBalas`
* **Combustível**: `postos[GASOLINA_MAX]` com texto `[FUEL]` descendo a tela

*(O arquivo contém comentários didáticos em quase todas as funções.)*&#x20;

## ⚙️ Ajustando dificuldade e comportamento

Abra `river_raid.c` e altere:

* **Velocidade do jogo** (aceleração automática):

  ```c
  #define TICK_START_USEC 80000  // mais alto = mais devagar no início
  #define TICK_MIN_USEC   20000  // mais baixo = mais rápido no máximo
  // Em main(): jogo acelera um pouco a cada 120 pontos
  ```
* **Densidade de inimigos**:

  ```c
  int limiteSpawn = 20;          // menor valor = nascem com mais frequência
  // Em main(): diminui a cada 500 pontos (mais difíceis com o tempo)
  ```
* **Largura do rio**:

  ```c
  LARGURA_MIN = LARGURA / 3;
  LARGURA_MAX = LARGURA / 2;     // ajuste para rios mais largos/estreitos
  ```
* **Sprites ASCII**:

  ```c
  const char *AVIAO[3] = { "  ^  ", "<-A->", " / \\ " };
  const char *INIMIGO[3] = { " | | ", "\\ooo/", " === " };
  ```

  Mude as strings mantendo a **mesma largura/altura** (ou atualize `AVIAO_W/H`, `INIMIGO_W/H`).

## 🧪 Regras de colisão (resumo)

* **Avião vs margens**: morre se **qualquer caractere** não-vazio do sprite encostar na margem.
* **Avião vs inimigo**: morre se **qualquer parte** do avião sobrepor o **retângulo** do inimigo.
* **Bala vs inimigo**: ao colidir, **remove** o inimigo e a bala.&#x20;

## 🛤️ Roadmap sugerido

* **Mapa mais “Atari-like”**: fases cíclicas (reto → ilha → canal estreito → reto)
* **Coleta de `[FUEL]` por área**: usar colisão retangular (AABB) no pickup (hoje coleta pelo ponto de origem)
* **Pontuação por abate** e **HUD** mais completo
* **Pausa** e **telas** de Menu/Game Over
* **Sons** (beep simples) e partículas ASCII
* **Modularização** em múltiplos arquivos (`player.c`, `world.c`, `bullets.c`…)

## ❗ Observações

* Certifique-se de que seu terminal está grande o suficiente (mínimo **40×20**).
* Em terminais sem suporte a cor, o jogo ainda funciona (só sem paleta).
* Em alguns ambientes macOS, pode ser necessário usar a `ncurses` do Homebrew (com `-I`/`-L`).

## 📄 Licença

Este projeto está licenciado sob **Creative Commons Attribution 4.0 International (CC BY 4.0)**.

Você **pode** copiar, redistribuir, remixar, transformar e criar a partir deste trabalho (inclusive para fins comerciais) **desde que** forneça **atribuição adequada** ao autor, inclua um link para a licença e indique se foram feitas alterações. Não são permitidas restrições adicionais que contrariem os termos da licença.

**Atribuição sugerida**  
“*River Raid (Terminal) — C + ncurses*, por **Márleson Rôndiner dos Santos Ferreira** (2025), licenciado sob **CC BY 4.0**.”

> Dica: Se você reutilizar trechos de código, mantenha o aviso de licença e a atribuição no seu README ou nos cabeçalhos dos arquivos.


