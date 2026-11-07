# MauWin — janelas em modo texto

`MauWin` é uma classe C++ sobre o **ncurses** para montar telas de terminal com pouco código: janelas com borda, menus navegáveis, campos rotulados atualizados em tempo real (incluindo relógio), relatórios montados em memória com rolagem e cores por elemento da janela.

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ Hora: 07/11/2026 16:58:05   Ultima consulta: Fita isolante 20m               │
└──────────────────────────────────────────────────────────────────────────────┘
    ┌──────────────────────────────────────────────────────────────────────┐
    │ ESTOQUE   (setas ou +/- rolam, ESC volta)                            │
    │ CODIGO  PRODUTO                               QTD         PRECO      │
    │----------------------------------------------------------------------│
    │ 1001    Parafuso sextavado 1/4               1500          0.35      │
    │ 1002    Porca sextavada 1/4                  1820          0.12      │
```

**Arquivos**

| Arquivo | Conteúdo |
|---|---|
| `MauWin.h`, `MauWin.cpp` | a classe |
| `exemploMauWin.cpp` | programa de exemplo comentado (consulta de estoque) |
| `testMauWin.cpp`, `testMauWinMenu.cpp` | testes automáticos |
| `testMauWinDemo.cpp` | demonstração interativa para conferir a tela |

---

## 1. Requisitos e compilação

- Linux com `libncurses-dev` instalado (fornece `ncurses` e `ncursesw`).
- Compilador C++11 ou mais novo (`g++` padrão serve).
- Terminal com pelo menos o tamanho das janelas que você criar (o exemplo pede 24×80).

Compile **ligando com `-lncursesw`** (a versão com suporte a UTF-8):

```bash
g++ -Wall meu_programa.cpp MauWin.cpp -lncursesw -o meu_programa
```

Pelo `Makefile`:

```bash
make exemploMauWin      # programa de exemplo
make test-mauwin        # roda os testes automáticos
make testMauWinDemo     # demonstração interativa
```

> **Se as bordas aparecerem como `lqqqk` / `x    x`**: falta chamar `setlocale(LC_ALL, "")` antes de `initscr()`, ou o programa foi ligado com `-lncurses` em vez de `-lncursesw`.

---

## 2. Início rápido

```cpp
#include <locale.h>
#include <ncurses.h>
#include "MauWin.h"

#define S(x) ((char*)(x))      // a classe recebe char*, não const char*

int main()
{
    setlocale(LC_ALL, "");     // 1. sempre antes de initscr()
    initscr();                 // 2. o programa inicia o ncurses, não a classe
    noecho();
    curs_set(0);

    {
        MauWin janela(10, 40, 2, 5, 1);   // 10 linhas x 40 colunas, na linha 2 / coluna 5, com borda
        if (janela.errorCode) {           // não coube no terminal, por exemplo
            endwin();
            return 1;
        }

        janela.print(1, 2, S("Ola, MauWin!"));
        janela.setVar(0, 3, 2, S("Hora:"), 20);   // campo 0: rótulo na linha 3, coluna 2

        while (janela.getKey() == ERR)            // espera até 1 s por uma tecla
            janela.relogio(0, S("hr"));           // sem tecla: atualiza o relógio
    }                                             // a janela é destruída aqui

    endwin();                  // 3. devolve o terminal ao normal
    return 0;
}
```

Três regras que valem para todo programa:

1. `setlocale(LC_ALL, "")` → `initscr()` → criar janelas. A classe **não** chama `initscr()`.
2. Destrua as janelas (fim do escopo) e chame `endwin()` antes de sair.
3. Ligue com `-lncursesw`.

---

## 3. Conceitos

### 3.1 Geometria de uma janela

```cpp
MauWin janela(linhas, colunas, linhaNaTela, colunaNaTela, borda);
```

Atenção à ordem: primeiro a **linha** (vertical), depois a **coluna** (horizontal). No código-fonte os parâmetros se chamam `posx`/`posy`, mas `posx` é a linha.

Para uma janela de `L` linhas e `C` colunas com borda:

```
coluna:  0  1  2 ...                              C-2 C-1
linha 0  ┌───────────────────────────────────────────┐   borda
linha 1  │ título (menuSet escreve aqui)              │   linha de título
linha 2  │┌─────────────────────────────────────────┐│ ┐
  ...    ││ subjanela: área do menu e do texto       ││ │ L-3 linhas
linha L-2│└─────────────────────────────────────────┘│ ┘
linha L-1└───────────────────────────────────────────┘   borda
            └──────────── C-2 colunas ──────────────┘
```

- **Janela principal**: a janela inteira. `print`, `printLogo`, `setVar`, `printVar` e `menuSet` usam **sempre** coordenadas relativas a ela: `(1, 2)` é a linha 1, coluna 2, dentro da borda, a qualquer momento.
- **Subjanela**: a área interna abaixo da linha de título, com `L-3` linhas e `C-2` colunas. É criada automaticamente por `menu()` e `textSet()`.
- **Menu**: mostra `L-4` itens por vez.
- **Texto** (`textShow`): mostra `L-3` linhas de `C-2` colunas.

Use `borda = 1` sempre que for usar menu ou texto: a subjanela assume que a borda existe.

### 3.2 Coordenadas: sempre da janela principal

Todos os métodos que escrevem na janela (`print`, `printLogo`, `setVar`, `printVar`, `relogio`, `menuSet`) usam coordenadas da **janela principal**, mesmo depois de `menu()` ou `textSet()` terem criado a subjanela. Títulos, rodapés e campos podem ser escritos em qualquer ordem:

```cpp
janela.textSet(&texto, 50);                 // cria a subjanela de texto
janela.print(1, 2, S("RELATORIO"));         // continua sendo a linha 1 da janela
```

A subjanela é usada só internamente, pelo menu e pelo texto. `cls()` apaga a janela inteira, incluindo menu e texto.

**Onde escrever numa janela com menu ou texto:** só a **linha 1** (título) fica fora da área deles. As linhas 2 a `L-2` pertencem ao menu/texto: o que você escrever ali é apagado no próximo `menu()` ou `textShow()`. Para um rodapé ou uma legenda, use outra janela (uma barra de status, por exemplo) ou escreva na linha 1.

> Na versão 1.0, depois de `menu()` ou `textSet()` essas coordenadas passavam a ser relativas à subjanela. Veja o [histórico de mudanças](#9-histórico-de-mudanças).

### 3.3 Quem é dono de quê (memória)

| Dado | Quem cria | Precisa existir até... |
|---|---|---|
| vetor `winMenu[]` e o título do menu | você | o fim do uso de `menu()` (a classe guarda só os ponteiros) |
| struct `winText` | você | **a destruição da janela** |
| buffer de texto (`winText.Texto`) | a classe, em `textSet` | é liberado pelo destrutor da janela |
| rótulos de `setVar` | você | não precisa, a classe copia |

Por causa da linha do `winText`, **declare o `winText` antes da janela**:

```cpp
winText texto;                       // 1º: vive mais
MauWin  janela(20, 72, 3, 4, 1);     // 2º: é destruída primeiro e ainda encontra o texto
```

Na ordem inversa, o destrutor da janela acessa uma struct que já deixou de existir.

`MauWin` **não pode ser copiada** (o construtor de cópia é `delete`). Passe por referência (`MauWin &`).

### 3.4 Sobreposição e redesenho

- Janelas criadas depois aparecem por cima das anteriores.
- **Destruir uma janela não apaga o que ela desenhou.** Ao voltar de uma tela, limpe e redesenhe o que deve continuar visível:

```cpp
clear();
refresh();
barra.show();       // redesenha a barra de status por cima da tela limpa
```

- `show()` atualiza a janela na tela. Os métodos que desenham (`printVar`, `menu`, `textShow`...) já chamam `show()`; `print()` e `cls()` **não**, então chame `show()` depois deles.

### 3.5 Erros

Os métodos não lançam exceções. Em caso de erro eles não fazem nada e registram um código:

- **Erros da janela**: `janela.errorCode` (um `short`) e a mensagem em `janela.errText` guardam o resultado da **última chamada** que pode falhar: `0` se deu certo, um número negativo se falhou. Cada uma dessas chamadas zera o erro ao começar, então confira **logo depois** da chamada que interessa:

  ```cpp
  janela.setVar(25, 1, 1, S("X:"), 5);
  if (janela.errorCode)
      fprintf(stderr, "erro %d: %s\n", janela.errorCode, janela.errText);
  ```

  Os métodos que zeram e preenchem `errorCode` são o construtor, `print`, `printLogo`, `setTitulo`, `setVar`, `printVar`, `relogio`, `menuSet`, `setCor` e `textSet`. Os demais (`show`, `cls`, `menu`, `getKey`, `text*`) não mexem nele.

- **Erros do buffer de texto**: ficam em `texto.ErroMsg`, na sua struct `winText`, e funcionam de outro jeito: o código **não é zerado** pelas chamadas seguintes (só por `textSet`). Assim você monta o relatório inteiro e confere uma vez no final; se algo falhou no meio, o código continua lá:

  ```cpp
  // ... várias chamadas textAppend / textNewLine ...
  if (texto.ErroMsg != 0)
      fprintf(stderr, "alguma escrita no texto falhou: %d\n", texto.ErroMsg);
  ```

A lista completa está na [seção 5](#5-códigos-de-erro).

---

## 4. Referência

Todos os parâmetros de texto são `char*`. Para literais, use um cast `(char*)"texto"` ou a macro `S("texto")` dos exemplos.

### 4.1 Janela

#### `MauWin(linhas, colunas, linhaNaTela, colunaNaTela, borda)`
Cria a janela e a desenha. `borda = 1` desenha a borda.
Se a janela não cabe no terminal, `errorCode` recebe `-100`, `-101`, `-110` ou `-120`. **Confira `errorCode` logo após criar.**

#### `void print(linha, coluna, char *texto)`
Escreve `texto` na posição dada da janela. Não atualiza a tela: chame `show()` depois.
Erro `-10` se a posição cair na borda ou fora da janela, `-11` se `texto` for `NULL`. O texto não é cortado: se for longo demais, o ncurses quebra a linha.

#### `void print(char *texto)`
Escreve na posição atual do cursor da janela, isto é, logo depois da última coisa escrita.

#### `void print(linha, coluna, char *texto, attr_t cor)`
Igual ao `print` acima, com uma cor própria (ver [4.5](#45-cores)):

```cpp
janela.print(6, 2, S("ATENCAO"), MauWin::cor(COLOR_RED, MW_PADRAO, A_BOLD));
```

#### `void setTitulo(char *titulo)`
Escreve `titulo` na linha 1, ocupando a largura interna, na cor de `MW_TITULO`. Apaga o título anterior e corta um título grande demais. Com um fundo colorido em `MW_TITULO`, a linha vira uma barra de título. Dica: comece o texto com um espaço para afastá-lo da borda.

#### `void printLogo(linhas, colunas, linha, coluna, char *logo)`
Escreve um desenho de várias linhas guardado numa matriz `char logo[linhas][colunas]`. Cada linha da matriz deve terminar em `'\0'` e o parâmetro `colunas` é o tamanho da segunda dimensão. Se uma linha cair fora da janela, o desenho para ali e o erro fica em `errorCode`:

```cpp
char logo[3][12] = { " __  __    ", "|  \\/  |   ", "|_|  |_|   " };
janela.printLogo(3, 12, 2, 4, &logo[0][0]);
janela.show();
```

#### `void show()` / `void show(int segundos)`
Atualiza a janela na tela. A segunda forma espera `segundos` depois.

#### `void cls()`
Apaga a janela inteira, incluindo título, menu e texto, e redesenha a borda. Chame `show()` depois.

#### `void redraw()`
Redesenha a borda e atualiza a janela.

#### `int getColunas()`
Largura da janela.

#### `int getKey()`
Espera **até 1 segundo** por uma tecla. Retorna o código da tecla ou `ERR` se nada foi apertado. Setas e teclas especiais chegam como `KEY_UP`, `KEY_DOWN`, `KEY_NPAGE`... O ESC chega como `ESCAPE` (27).
Bom para laços que precisam fazer algo enquanto esperam, como atualizar um relógio:

```cpp
int k;
while ((k = janela.getKey()) == ERR)
    barra.relogio(0, S("hr"));
```

#### `void selectMainWin()` / `void selectSubWin()` / `void setSubWin()`
Uso interno; normalmente você não precisa delas. Desde a versão 1.1 a escrita usa sempre a janela principal ([3.2](#32-coordenadas-sempre-da-janela-principal)). Elas só escolhem em qual janela o cursor fica e de qual janela `getKey()` lê. `setSubWin()` cria a subjanela, se ainda não existir; `menu()` e `textSet()` já a chamam.

### 4.2 Campos (variáveis na tela)

Um campo é um **rótulo fixo** seguido de um **valor** que muda. Cada janela tem até `MAXVARWIN` (20) campos, identificados de `0` a `19`.

#### `void setVar(id, linha, coluna, char *rotulo, int tamanho)`
Configura o campo `id`: escreve `rotulo` em `(linha, coluna)` e reserva `tamanho` posições para o valor, que começa **1 espaço depois do rótulo**. Se o rótulo cair na borda ou fora da janela, o campo não é configurado (`errorCode = -10`) e um `printVar` nele dá `-21`.

#### `void printVar(id, char *valor)`
Escreve `valor` no campo, apagando o valor anterior e atualizando a tela:
- valores maiores que `tamanho` são cortados;
- o valor também é cortado na borda direita, para nunca invadir a borda;
- caracteres de controle (`\n`, `\t`, ESC...) viram `?`.

```cpp
janela.setVar(0, 3, 2, S("Quantidade:"), 10);
janela.printVar(0, S("140 un"));      // │ Quantidade: 140 un
```

#### `void printVar(id, char *valor, attr_t cor)`
Igual a `printVar`, mas com uma cor própria em vez da cor de `MW_VALOR`. A cor vale para todo o espaço reservado do campo, então um fundo colorido destaca o campo inteiro:

```cpp
if (quantidade < 10)
    janela.printVar(0, S("6 un"), MauWin::cor(COLOR_WHITE, COLOR_RED, A_BOLD));   // alerta
```

Erros: `-20` (`id` ≥ 20), `-21` (campo não configurado), `-23` (texto `NULL`), `-24` (o rótulo ocupa a linha toda).

#### `void relogio(id, char *formato)`
Escreve a data/hora atual no campo `id`, que precisa ter sido configurado com `setVar`, na cor de `MW_VALOR`.

| `formato` | Resultado | `tamanho` mínimo |
|---|---|---|
| `"hr"` | `16:58:05` | 8 |
| `"db"` | `07/11/2026` | 10 |
| `"du"` | `2026-11-07` | 10 |
| `"Db"` | `07/11/2026 16:58:05` | 19 |
| `"Du"` | `2026-11-07 16:58:05` | 19 |

Um formato desconhecido apaga o campo.

### 4.3 Menu

O menu é um vetor de `winMenu`:

```cpp
typedef struct winmenu {
    char label[MAXVARLEN];   // texto do item (até 79 caracteres)
    int  valor;              // número livre: aparece no menu em modo 1
    int  selecionado;        // não usado pela classe
} winMenu;
```

#### `void menuSet(winMenu *itens, char *titulo, int quantidade)`
Associa o menu à janela e escreve o título na linha de título (linha 1), na cor de `MW_TITULO`, ocupando a largura interna: um título anterior mais longo é apagado e um título grande demais é cortado. Pode ser chamada de novo a qualquer momento para trocar de menu. O vetor e o título precisam continuar existindo enquanto o menu for usado.

#### `int menu(int modo)`

| `modo` | Comportamento | Retorno |
|---|---|---|
| `0` | interativo, itens numerados `01`, `02`... | índice escolhido |
| `1` | interativo, mostra o campo `valor` de cada item | índice escolhido |
| `-1` | só desenha (numerado), sem esperar tecla | `0` |
| `-2` | só desenha (com `valor`), sem esperar tecla | `0` |

O retorno dos modos interativos é o **índice** do item no vetor (começa em `0`), `-1` se o usuário apertou ESC, ou `-2` se `menuSet` não foi chamado.

**Teclas:**

| Tecla | Ação |
|---|---|
| ↑ / ↓ | item anterior / seguinte |
| Page Up / Page Down | uma página |
| Home / End | primeiro / último |
| dígitos | vai direto ao item pela posição: `1`, `2` → item 12; se não existir, vale o último dígito |
| Enter | confirma |
| ESC | cancela (retorna `-1`) |

Outras teclas são ignoradas.

```cpp
winMenu opcoes[] = {
    { "Relatorio de estoque", 0, 0 },
    { "Consultar produto",    0, 0 },
    { "Sair",                 0, 0 },
};
MauWin principal(9, 40, 7, 20, 1);
principal.menuSet(opcoes, S("MENU PRINCIPAL"), 3);

switch (principal.menu(0)) {
    case 0:  relatorio(); break;
    case 1:  consulta();  break;
    default: /* Sair ou ESC */ break;
}
```

### 4.4 Texto (relatórios com rolagem)

O texto é montado em um buffer de **linhas de largura fixa** (a largura interna da janela, `colunas - 2`) e mostrado com `textShow`. Funciona como uma folha de relatório: você posiciona cada pedaço por coluna e quebra a linha.

As **posições de coluna começam em 1**. Usar 2 como primeira coluna deixa uma margem de um espaço em relação à borda.

#### `void textSet(winText *texto, unsigned linhas)`
Prepara um buffer para `linhas` linhas, zera a posição e seleciona a subjanela. A struct `winText` é sua e **deve ser declarada antes da janela** (ver [3.3](#33-quem-é-dono-de-quê-memória)). Chamar de novo libera o buffer anterior e recomeça.
Campos úteis da struct (só leitura):

| Campo | Significado |
|---|---|
| `Colunas` | largura de cada linha (`colunas da janela - 2`) |
| `PosLin`, `PosCol` | posição de escrita atual (base 0) |
| `ErroMsg` | último erro do buffer (0 = nenhum) |

#### Escrevendo

| Chamada | O que faz |
|---|---|
| `textAppend(pos, txt)` | escreve `txt` a partir da coluna `pos`, completando com espaços desde a posição atual. Se não couber, corta no fim da linha |
| `textAppend(pos, largura, txt)` | campo de largura fixa: escreve `txt` na coluna `pos`, cortado ou completado com espaços até `largura` |
| `textAppend(txt, posFim)` | **alinha à direita**: o último caractere de `txt` fica na coluna `posFim` (bom para valores) |
| `textAppend('c')` | linha inteira do caractere `c` (separador). Quebra a linha antes, se necessário |
| `textAppend(txt)` | `txt` centralizado numa linha própria. Chame `textNewLine()` depois |
| `textNewLine()` | completa a linha atual com espaços e vai para a próxima. No início de uma linha, gera uma linha em branco |
| `textAddLine()` | termina a linha atual, se houver algo escrito nela, e acrescenta **exatamente uma** linha em branco |

Para usar as formas com número e texto, passe o número como `unsigned` (`2u`), para o compilador escolher a sobrecarga certa:

```cpp
winText texto;
memset(&texto, 0, sizeof(texto));
MauWin janela(20, 72, 3, 4, 1);

janela.textSet(&texto, 50);
janela.print(1, 2, S("ESTOQUE"));            // título na linha 1 da janela

janela.textAppend(2u,  S("CODIGO"));
janela.textAppend(10u, S("PRODUTO"));
janela.textAppend(S("PRECO"), 64u);           // termina na coluna 64
janela.textNewLine();
janela.textAppend('-');

janela.textAppend(2u, S("1001"));
janela.textAppend(10u, 32u, S("Parafuso sextavado 1/4"));
janela.textAppend(S("0.35"), 64u);
janela.textNewLine();

janela.textShow(0);
```

Uma escrita que não é possível (posição atrás do cursor, buffer cheio...) **não faz nada** e registra o motivo em `texto.ErroMsg`. O código fica lá até o próximo `textSet`, então basta conferir esse campo ao final da montagem.

#### Mostrando e rolando

#### `void textShow(unsigned linha)`
Mostra o texto a partir da linha `linha` do buffer (base 0), preenchendo as `L-3` linhas da subjanela e apagando o que sobrar.

```cpp
int visiveis = 20 - 3;                       // altura da janela - 3
int ultima   = (int) texto.PosLin > visiveis ? (int) texto.PosLin - visiveis : 0;
int topo     = 0;
int k;

janela.textShow(topo);
while ((k = janela.getKey()) != ESCAPE) {
    if (k == KEY_DOWN && topo < ultima) topo++;
    if (k == KEY_UP   && topo > 0)      topo--;
    janela.textShow(topo);
}
```

#### Outras

| Chamada | O que faz |
|---|---|
| `int textGetPos()` | quantidade de caracteres escritos (`PosLin * Colunas + PosCol`), ou `-1` com o buffer cheio ou sem `textSet` |
| `void textGetTexto(char *destino, unsigned tamanho)` | copia o texto para `destino`. **Não** coloca `'\0'` no final: zere o destino antes. Se `tamanho` for menor que o texto, não copia e `ErroMsg = -2` |
| `void textLimpa()` | apaga o buffer e volta ao início |

> **`Formato`**: colocando `texto.Formato = 1` depois do `textSet`, `textNewLine()` passa a gravar um `'\n'` em vez de completar a linha com espaços. Serve para exportar o texto com `textGetTexto` (gravar em arquivo, imprimir). **Não** use com `textShow`, que espera linhas de largura fixa.

### 4.5 Cores

Cada janela tem um esquema de cores com seis **elementos**:

| Elemento | O que colore | Sem `setCor` |
|---|---|---|
| `MW_BORDA` | a moldura | cor do terminal |
| `MW_TITULO` | a linha de título (`setTitulo`, `menuSet`) | cor do terminal |
| `MW_TEXTO` | o fundo e o texto da janela inteira, incluindo menu e relatório | cor do terminal |
| `MW_DESTAQUE` | o item selecionado do menu | preto sobre branco (ou vídeo reverso, sem cores) |
| `MW_ROTULO` | os rótulos dos campos (`setVar`) | cor do terminal |
| `MW_VALOR` | os valores dos campos (`printVar`, `relogio`) | cor do terminal |

#### `void setCor(elemento, frente, fundo, attr_t extra = A_NORMAL)`
Define a cor de um elemento. `frente` e `fundo` são cores do ncurses:

`COLOR_BLACK` · `COLOR_RED` · `COLOR_GREEN` · `COLOR_YELLOW` · `COLOR_BLUE` · `COLOR_MAGENTA` · `COLOR_CYAN` · `COLOR_WHITE`

ou `MW_PADRAO`, a cor do próprio terminal. Use `MW_PADRAO` no fundo para a janela ficar boa tanto em terminal escuro quanto claro. `extra` acrescenta atributos como `A_BOLD` (negrito, que em muitos terminais também clareia a cor) ou `A_UNDERLINE`.

A mudança aparece na hora para o que já está na tela: borda, título, fundo e rótulos são redesenhados. Valores de campos já escritos mudam de cor no próximo `printVar`.

```cpp
// barra de status branca sobre azul
MauWin barra(3, 80, 0, 0, 1);
barra.setCor(MW_TEXTO,  COLOR_WHITE,  COLOR_BLUE);
barra.setCor(MW_BORDA,  COLOR_CYAN,   COLOR_BLUE);
barra.setCor(MW_ROTULO, COLOR_CYAN,   COLOR_BLUE);
barra.setCor(MW_VALOR,  COLOR_YELLOW, COLOR_BLUE, A_BOLD);

// janela com barra de título e destaque amarelo, no fundo do terminal
MauWin janela(14, 50, 5, 15, 1);
janela.setCor(MW_BORDA,    COLOR_CYAN,  MW_PADRAO);
janela.setCor(MW_TITULO,   COLOR_BLACK, COLOR_CYAN);
janela.setCor(MW_DESTAQUE, COLOR_BLACK, COLOR_YELLOW);
```

Para usar o mesmo visual em várias janelas, junte as chamadas numa função, como a `aplicaTema()` do `exemploMauWin.cpp`, e chame logo depois de criar cada janela.

Erro `-40` se `elemento` não for um dos seis.

#### `static attr_t MauWin::cor(frente, fundo, attr_t extra = A_NORMAL)`
Devolve o atributo pronto para uma combinação de cores, para usar em `print(..., cor)` e `printVar(..., cor)`. Pode ser guardado numa variável e reaproveitado:

```cpp
attr_t alerta = MauWin::cor(COLOR_WHITE, COLOR_RED, A_BOLD);
janela.printVar(0, S("ESTOQUE BAIXO"), alerta);
```

#### `static bool MauWin::temCores()`
`true` se o terminal mostra cores.

**Detalhes:**

- **Terminal sem cores:** `setCor` e `cor()` não dão erro; o texto sai sem cor, e os atributos `extra` (negrito, sublinhado) continuam valendo. O destaque do menu cai para vídeo reverso, para continuar visível.
- **Quantidade de cores:** as 8 cores acima funcionam em praticamente qualquer terminal. Números de cor maiores que o terminal suporta são convertidos para uma das 8.
- **Pares de cor:** o ncurses guarda cada combinação frente/fundo num "par", em quantidade limitada. A classe cria cada combinação uma vez e a reaproveita entre todas as janelas, até 255 combinações; além disso, o texto sai sem cor.
- **Ordem:** `setCor` só pode ser chamado depois de `initscr()` (vale para toda a classe). A primeira chamada de cor inicializa as cores do ncurses (`start_color`) e mantém o fundo do terminal (`use_default_colors`).

---

## 5. Códigos de erro

### `errorCode` (na janela)

| Código | Onde | Motivo |
|---|---|---|
| -100 | construtor | mais linhas que o terminal |
| -101 | construtor | mais colunas que o terminal |
| -110 | construtor | `colunaNaTela + colunas` passa da largura do terminal |
| -120 | construtor | `linhaNaTela + linhas` passa da altura do terminal |
| -10 | `print`, `printLogo`, `setVar` | posição na borda ou fora da janela |
| -11 | `print`, `printLogo`, `setTitulo` | texto `NULL` |
| -20 | `setVar`, `printVar` | `id` ≥ 20, ou linha/coluna fora da janela |
| -21 | `printVar` | campo não configurado com `setVar` |
| -22 | `printVar` | falta de memória |
| -23 | `setVar`, `printVar` | rótulo ou valor `NULL` |
| -24 | `printVar` | o rótulo ocupa a linha inteira; não há espaço para o valor |
| -30 | `textSet` | `winText` `NULL` |
| -31 | `textSet` | falta de memória para o buffer |
| -40 | `setCor` | elemento inválido |

`setVar` também **ignora sem erro** um campo com `tamanho <= 0` ou com `coluna + tamanho` maior que a largura.

### `ErroMsg` (no `winText`)

| Código | Motivo |
|---|---|
| -1 | avanço maior que o espaço restante na linha (interno) |
| -2 | `textGetTexto`: destino menor que o texto |
| -3 | posição `0` (as posições começam em 1) |
| -4 | posição atrás do ponto de escrita, ou texto `NULL` |
| -5 | largura ou preenchimento além do fim da linha |
| -6 | `textAppend(txt, posFim)` com `txt` `NULL` |
| -7 | `textAppend(txt, posFim)`: `txt` maior que `posFim` ou `posFim` atrás do ponto de escrita |
| -8 | `textAppend(txt, posFim)`: não cabe na linha |
| -22 | buffer cheio |

---

## 6. Limitações conhecidas

- **Acentos desalinham.** Larguras e posições contam **bytes**. Em UTF-8, `ç` ou `ã` ocupam 2 bytes e 1 coluna, então uma linha com acentos fica mais curta na tela e as colunas seguintes desalinham. Os acentos aparecem corretamente (com `setlocale` + `ncursesw`), mas para tabelas alinhadas prefira texto sem acentos.
- **Redimensionar o terminal** não é tratado: as janelas mantêm tamanho e posição.
- **`print()` não filtra caracteres de controle**: um `\n` no texto pula de linha. `printVar` filtra.
- **`print()` não corta o texto** que passa da borda.
- **Uma subjanela por janela**: menu e texto na mesma janela compartilham a mesma área.
- **O relatório tem uma cor só** (`MW_TEXTO`): o buffer de texto guarda caracteres, não cores, então não dá para colorir uma linha do relatório. Para destacar valores, use campos (`printVar` com cor).
- **Só as 8 cores básicas** são garantidas; 256 cores não foram testadas.
- A classe não é segura para uso por várias threads ao mesmo tempo (o ncurses também não é).

---

## 7. Programa de exemplo

`exemploMauWin.cpp` é uma consulta de estoque completa e comentada. Ela mostra:

- inicialização e encerramento corretos do ncurses;
- barra de status com relógio e campo "Última consulta";
- menu principal (modo 0) e menu de produtos com o código no `valor` (modo 1);
- relatório com cabeçalho, colunas alinhadas à esquerda e à direita, separadores, total e rolagem com setas e Page Up/Down;
- ficha do produto com campos `setVar`/`printVar`;
- tema de cores aplicado a todas as janelas (`aplicaTema`), barra de status azul e alerta de estoque baixo em vermelho;
- conferência de `errorCode` e `ErroMsg`.

```bash
make exemploMauWin
./exemploMauWin
```

---

## 8. Testes

```bash
make test-mauwin
```

Roda os dois testes automáticos, compilados com AddressSanitizer: qualquer escrita fora de buffer ou vazamento de memória derruba o teste.

- `testMauWin`: janela, campos, texto, limites do buffer e menus em modo de exibição. Desenha num terminal virtual e confere o conteúdo exato da tela.
- `testMauWinMenu`: abre o menu num pseudo-terminal e simula as teclas (setas, Page Up/Down, dígitos, ESC...). Leva cerca de 20 s.

Os dois retornam `0` se tudo passou, o que permite usá-los em scripts e CI.

---

## 9. Histórico de mudanças

### 1.2

- **Cores** ([4.5](#45-cores)): `setCor` por elemento da janela (borda, título, fundo, destaque do menu, rótulos e valores), `MauWin::cor()` e versões de `print` e `printVar` com cor própria.
- **`setTitulo`**: escreve a linha de título na cor de `MW_TITULO`.
- Novo código de erro `-40`.
- Sem `setCor`, a aparência é a mesma da 1.1.

### 1.1

Correções de memória e de tela, mais três mudanças de comportamento. Programas escritos para a 1.0 compilam sem alteração, mas podem precisar de ajuste nos pontos marcados com **⚠**.

- **⚠ Coordenadas sempre da janela principal.** `print`, `printLogo`, `setVar`, `printVar` e `menuSet` não mudam mais de referencial depois de `menu()`/`textSet()`. Quem compensava o deslocamento da subjanela (somando 2 à linha e 1 à coluna) deve remover a compensação.
- **⚠ `cls()` apaga a janela inteira**, não só a área de menu/texto.
- **⚠ `textAddLine()` gera exatamente uma linha em branco.** Antes gerava duas quando o cursor estava no início da linha.
- **⚠ `errorCode` passou a ser `short` e reflete a última chamada.** Ele é zerado no início de cada método que pode falhar. Casts `(short) errorCode` continuam funcionando, mas sobram. Quem conferia `errorCode` uma vez depois de várias chamadas deve conferir após cada uma.
- **Bordas gráficas** (`┌─┐`) no lugar de `|` e `-`. Requer `setlocale` + `-lncursesw` ([seção 1](#1-requisitos-e-compilação)).
- **Menu interativo**: aparece antes da primeira tecla, ESC cancela (retorna `-1`), Page Up/Down, Home/End, e as letras são ignoradas. Não se perde mais depois de `getKey()`.
- **`getKey()`** reconhece setas e teclas especiais (`KEY_UP`...).
- **`printVar`** corta o valor na borda e troca caracteres de controle por `?`.
- **`textShow`** usa toda a área interna, sem quebrar linhas sobre a borda.
- **`show()`** atualiza a janela inteira, então títulos escritos depois de `textSet()` aparecem.
- **`menuSet`** apaga o título anterior.
- **Memória**: corrigidas escritas fora do buffer de texto e do vetor de campos, uso de ponteiro nulo sem `textSet`, vazamento em `textSet` repetido, e cópia acidental da classe (agora proibida).
- **Novos códigos de erro**: `-11`, `-23`, `-24`, `-30`, `-31`.
