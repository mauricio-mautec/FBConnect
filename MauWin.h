#define MAUWIN_VERSAO 1.2

// DOCUMENTACAO: MauWin.md       EXEMPLO: exemploMauWin.cpp
//
// USO: O PROGRAMA DEVE CHAMAR setlocale( LC_ALL, "" ) ANTES DE initscr()
//      E LINKAR COM -lncursesw. SEM ISSO, EM TERMINAIS UTF-8 AS BORDAS
//      SAEM COMO "lqqqk" E ACENTOS APARECEM QUEBRADOS.
#include <ncurses.h>

// ELEMENTOS DA JANELA QUE PODEM TER COR PROPRIA (VER setCor)
enum MauWinElemento {
    MW_BORDA,           // MOLDURA DA JANELA
    MW_TITULO,          // LINHA DE TITULO (setTitulo, menuSet)
    MW_TEXTO,           // FUNDO E TEXTO DA JANELA INTEIRA (INCLUI MENU E RELATORIO)
    MW_DESTAQUE,        // ITEM SELECIONADO NO MENU
    MW_ROTULO,          // ROTULOS DOS CAMPOS (setVar)
    MW_VALOR,           // VALORES DOS CAMPOS (printVar)
    MW_ELEMENTOS        // QUANTIDADE DE ELEMENTOS (NAO E UM ELEMENTO)
};

#define MW_PADRAO (-1)  // COR PADRAO DO TERMINAL, PARA FRENTE OU FUNDO

#define MAXVARWIN 20
#define MAXVARLEN 80
#define ESCAPE    27

typedef struct wintext {
     unsigned int Linhas;
     unsigned int Colunas;
     unsigned int PosLin, PosCol;
     char         *Texto;
     char         *pTexto;
     int          ErroMsg;
    unsigned int  Formato;            // ESPECIFICA SE UTILIZAREMOS OU NAO CARACTERES DE FORMATACAO
} winText;

typedef struct winmenu {
     char   label[ MAXVARLEN ];
     int    valor;
     int    selecionado;
 } winMenu;

typedef struct winvars {
    unsigned short      linha;
    unsigned short      coluna;
    unsigned short      ready;
    char                nome[ MAXVARLEN ];
    unsigned short      qtd;
} winVars;


class  MauWin {
private:
    WINDOW  *Window, *mainWindow, *subWindow;

    winVars            campoWin[ MAXVARWIN ]; // VARIAVEIS FIXAS NA JANELA
    winMenu            *menuWin;              // PONTEIRO PARA EXTRUTURA DE MENU
    winText            *textWin;              // PONTEIRO PARA EXTRUTURA DE TEXTO
    int                menuWinSize;           // QTD DE ITENS DO MENU
    char               *menuWinTitulo;        // NOME DO MENU
    unsigned short     nColunas, nLinhas;     // MAXIMO DE LINHAS,COLUNAS DO TERMINAL
    unsigned short     NX,NY;                 // POSICAO DA JANELA EM RELACAO AO TERMINAL
    unsigned short     X,Y;                   // POSICAO DO CURSOR DENTRO DA JANELA
    unsigned short     boxed;
    unsigned short     colunas, linhas;       // TAMANHO DA JANELA REQUISITADA
    attr_t             corElem[ MW_ELEMENTOS ];// COR DE CADA ELEMENTO (setCor)
    bool               destaquePadrao;        // MW_DESTAQUE AINDA NAO DEFINIDO PELO USUARIO
    char               titulo[ 256 ];         // TITULO ATUAL (PARA REDESENHAR AO TROCAR A COR)
    unsigned short     key;                   // TECLA PRECIONADA

    unsigned short     modoOperacao;

    void erro( int, char* );
    void limpaErro( void );                                   // ZERA errorCode/errText NO INICIO DE CADA OPERACAO
    int  escreve( unsigned short, unsigned short, char*, attr_t = A_NORMAL ); // ESCREVE NA JANELA PRINCIPAL (SEM ZERAR O ERRO)
    void desenhaBorda( void );
    void desenhaTitulo( void );
    attr_t corDestaque( void );
    void menuShow( int, int, int );
    void textIncMemTexto( unsigned int );
    void textPadTexto( unsigned int );

public:
    // RESULTADO DA ULTIMA OPERACAO QUE PODE FALHAR: 0 = OK, NEGATIVO = ERRO
    char             errText[256];
    short            errorCode;

    MauWin( unsigned short, unsigned short, unsigned short, unsigned short, unsigned short );
   ~MauWin( void );

    // A CLASSE E DONA DE RECURSOS (WINDOW*, BUFFER DE TEXTO): COPIA PROIBIDA
    // PARA EVITAR delwin/free DUPLO
    MauWin( const MauWin& )            = delete;
    MauWin& operator=( const MauWin& ) = delete;

    // CORES
    static attr_t cor( short frente, short fundo, attr_t extra = A_NORMAL ); // ATRIBUTO PRONTO PARA print/printVar
    static bool   temCores( void );                         // O TERMINAL MOSTRA CORES?
    void    setCor( int elemento, short frente, short fundo, attr_t extra = A_NORMAL );
    void    setTitulo( char * );                            // ESCREVE O TITULO NA LINHA 1
    void    print( unsigned short, unsigned short, char *, attr_t );   // print COM COR
    void    printVar( unsigned short, char *, attr_t );                // printVar COM COR

    int     getColunas( void );
    int     getKey( void );
    int     menu( int );
    int     textGetPos( void );                             // devolve o tamanho em bytes do Texto
    void    cls(void);
    void    menuSet( winMenu *, char *, int);
    void    print( char * );
    void    print( unsigned short, unsigned short, char * );
    void    printLogo( unsigned short, unsigned short, unsigned short, unsigned short, char * );
    void    printVar( unsigned short, char * );
    void    relogio( int, char[2] );
    void    selectMainWin( void );
    void    selectSubWin( void );
    void    setSubWin( void );
    void    setVar( unsigned short, unsigned short, unsigned short, char *, int qtd );
    void    show( int );
    void    show( void );
    void    redraw( void );
    void    textAddLine(void);                              // adiciona uma linha em branco
    void    textAppend ( char );                            // cria um linha continua de caracteres
    void    textAppend ( char* );                           // centraliza uma linha
    void    textAppend ( char*, unsigned int );             // justifica à direita na posicao
    void    textAppend( unsigned int, char* );              // nao limita o tamanho
    void    textAppend( unsigned int, unsigned int, char* );// limita o tamanho
    void    textGetTexto( char*, unsigned int );            // retorna o conteudo da memoria Texto
    void    textLimpa ( void );                             // limpa a area de memoria que armazenas o texto
    void    textNewLine ( void );                           // insere um caracter de nova linha
    void    textSet( winText *, unsigned int );
    void    textShow( unsigned int );                       // apresenta texto a partir da posicao especificada

}; //class MauWin
