#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <term.h>
#include <ncurses.h>
#include <unistd.h>
#include "MauWin.h"

// ---------------------------------------------------------------------------
// CORES: TABELA DE PARES (FRENTE, FUNDO) COMPARTILHADA ENTRE TODAS AS JANELAS.
// O ncurses TEM UM NUMERO LIMITADO DE PARES; CADA COMBINACAO E CRIADA UMA VEZ
// E REAPROVEITADA. O PAR 0 E O PADRAO DO TERMINAL E NAO PODE SER ALTERADO.
#define MW_MAXPARES 255

static bool  mwCoresIniciadas = false;
static bool  mwFundoPadrao    = false;        // use_default_colors() FUNCIONOU (ACEITA -1)
static short mwParFrente[ MW_MAXPARES ];
static short mwParFundo [ MW_MAXPARES ];
static int   mwPares          = 0;

bool MauWin::temCores( void )
{
    return has_colors();
}

// DEVOLVE O ATRIBUTO (PAR DE CORES + extra, EX.: A_BOLD) PARA USAR EM
// print/printVar OU GUARDADO. frente/fundo: COLOR_BLACK..COLOR_WHITE, OU
// MW_PADRAO (-1) PARA A COR DO TERMINAL. SEM CORES NO TERMINAL, DEVOLVE SO extra.
attr_t MauWin::cor( short frente, short fundo, attr_t extra )
{
    if ( !has_colors() ) return extra;

    if ( !mwCoresIniciadas ) {
        start_color();
        // MANTEM O FUNDO DO TERMINAL (CLARO OU ESCURO) ONDE NAO HOUVER COR
        mwFundoPadrao    = ( use_default_colors() == OK );
        mwCoresIniciadas = true; }

    if ( !mwFundoPadrao ) {                   // TERMINAL NAO ACEITA -1
        if ( frente < 0 ) frente = COLOR_WHITE;
        if ( fundo  < 0 ) fundo  = COLOR_BLACK; }

    if ( frente >= COLORS ) frente %= 8;      // COR ALEM DO QUE O TERMINAL TEM
    if ( fundo  >= COLORS ) fundo  %= 8;

    for ( int i = 0; i < mwPares; i++ )
        if ( mwParFrente[i] == frente && mwParFundo[i] == fundo )
            return COLOR_PAIR( i + 1 ) | extra;

    if ( mwPares >= MW_MAXPARES || mwPares + 1 >= COLOR_PAIRS )
        return extra;                         // ACABARAM OS PARES: SEM COR

    init_pair( mwPares + 1, frente, fundo );
    mwParFrente[ mwPares ] = frente;
    mwParFundo [ mwPares ] = fundo;
    mwPares++;
    return COLOR_PAIR( mwPares ) | extra;
}

// CONSTRUTOR DA CLASSE
// ERRO: -100
MauWin::MauWin ( unsigned short linhas, unsigned short colunas, unsigned short posx, unsigned short posy, unsigned short boxed )
{
  // ZERA TODOS OS CAMPOS (INCLUSIVE O nome INTEIRO, GARANTINDO O '\0')
  memset( this->campoWin, 0, sizeof(this->campoWin) );

  memset( this->errText, '\0', sizeof(this->errText) );
  this->errorCode = 0;

   this->Window     = NULL;
   this->mainWindow = NULL;
   this->subWindow  = NULL;
   this->textWin    = NULL;
   this->menuWin    = NULL;
   this->menuWinSize= 0;

   this->nLinhas   = tigetnum( "lines" );
   this->nColunas  = tigetnum( "cols" );

   if ( linhas  > this->nLinhas  )        this->erro( -100, (char*)"NUMERO DE LINHAS MAIOR QUE MAXIMO" );
   if ( colunas > this->nColunas )        this->erro( -101, (char*)"NUMERO DE COLUNAS MAIOR QUE MAXIMO" );
   if ( posy + colunas > this->nColunas ) this->erro( -110, (char*)"POSICAO Y EXCEDE COLUNAS" );
   if ( posx + linhas  > this->nLinhas  ) this->erro( -120, (char*)"POSICAO X EXCEDE LINHAS" );

   this->linhas  =  linhas;
   this->colunas =  colunas;
   this->NX      =  posx;
   this->NY      =  posy;

   // SEM COR ATE O USUARIO PEDIR (setCor): A JANELA USA AS CORES DO TERMINAL
   for ( int i = 0; i < MW_ELEMENTOS; i++ ) this->corElem[ i ] = A_NORMAL;
   this->destaquePadrao = true;
   this->titulo[ 0 ]    = '\0';

   if ( boxed != 1 )
       this->boxed = 0;
   else
       this->boxed = 1;

   this->mainWindow  = newwin( this->linhas, this->colunas, this->NX, this->NY );

   this->desenhaBorda();

   touchwin( this->mainWindow );
   wrefresh( this->mainWindow );

   this->Window = this->mainWindow;
}

// DESTRUTOR DA CLASSE
MauWin::~MauWin ( void )
{
    if ( this->subWindow )  delwin( this->subWindow );
    if ( this->mainWindow ) delwin( this->mainWindow );

    if ( this->textWin != NULL ) {
        free( this->textWin->Texto );
        this->textWin->Texto  = NULL;
        this->textWin->pTexto = NULL; }
}

// REGISTRA OCORRENCIA DE ERRO
// INTERNO DA CLASSE
void MauWin::erro( int enumero, char *texto )
{
    memset ( this->errText, '\0',  sizeof(this->errText) );
    strncpy( this->errText, texto, sizeof(this->errText) - 1 );
    this->errorCode = enumero;
    return;
}

// ZERA O ERRO: CHAMADA NO INICIO DE CADA METODO PUBLICO QUE PODE FALHAR,
// PARA errorCode REFLETIR SEMPRE A ULTIMA OPERACAO
void MauWin::limpaErro( void )
{
    this->errorCode  = 0;
    this->errText[0] = '\0';
}

// ESCREVE NA JANELA PRINCIPAL, EM COORDENADAS DA JANELA PRINCIPAL,
// INDEPENDENTE DE A SUBJANELA (MENU/TEXTO) ESTAR SELECIONADA
int MauWin::escreve( unsigned short linha, unsigned short coluna, char *texto, attr_t atributo )
{
    if ( texto == NULL ) {
        this->erro( -11, (char*)"print: texto nulo" );
        return( -11 ); }

    if ( linha >= this->linhas - 1 || coluna >= this->colunas - 1 ) {
        this->erro( -10, (char*)"print(linha,coluna,texto):linha ou coluna fora das especificacoes" );
        return( -10 ); }

    wattrset( this->mainWindow, atributo );
    mvwprintw( this->mainWindow, linha, coluna, "%s", texto );
    wattrset( this->mainWindow, A_NORMAL );
    return( 0 );
}

// BORDA COM AS LINHAS GRAFICAS DO TERMINAL NA COR DE MW_BORDA
void MauWin::desenhaBorda( void )
{
    if ( !this->boxed || this->mainWindow == NULL ) return;

    attr_t a = this->corElem[ MW_BORDA ];
    wborder( this->mainWindow, ACS_VLINE | a, ACS_VLINE | a, ACS_HLINE | a, ACS_HLINE | a,
             ACS_ULCORNER | a, ACS_URCORNER | a, ACS_LLCORNER | a, ACS_LRCORNER | a );
}

// TITULO NA LINHA 1, OCUPANDO A LARGURA INTERNA NA COR DE MW_TITULO
// (COM FUNDO COLORIDO, VIRA UMA BARRA DE TITULO)
void MauWin::desenhaTitulo( void )
{
    int largura = this->colunas - 2;
    if ( largura <= 0 || this->linhas < 3 ) return;

    wattrset( this->mainWindow, this->corElem[ MW_TITULO ] );
    mvwprintw( this->mainWindow, 1, 1, "%-*.*s", largura, largura, this->titulo );
    wattrset( this->mainWindow, A_NORMAL );
}

// COR DO ITEM SELECIONADO NO MENU: PRETO SOBRE BRANCO POR PADRAO, OU VIDEO
// REVERSO SE O TERMINAL NAO TIVER CORES
attr_t MauWin::corDestaque( void )
{
    if ( this->destaquePadrao )
        return has_colors() ? MauWin::cor( COLOR_BLACK, COLOR_WHITE ) : A_REVERSE;
    return this->corElem[ MW_DESTAQUE ];
}

// DEFINE A COR DE UM ELEMENTO DA JANELA E REDESENHA O QUE JA ESTIVER NA TELA
// (BORDA, TITULO, FUNDO, ROTULOS). VALORES DE CAMPO JA ESCRITOS MUDAM DE COR
// NO PROXIMO printVar
void MauWin::setCor( int elemento, short frente, short fundo, attr_t extra )
{
    this->limpaErro();

    if ( elemento < 0 || elemento >= MW_ELEMENTOS ) {
        this->erro( -40, (char*)"setCor: elemento invalido" );
        return; }

    attr_t a = MauWin::cor( frente, fundo, extra );

    if ( elemento == MW_DESTAQUE ) {
        // SEM CORES E SEM ATRIBUTO EXTRA O ITEM SELECIONADO FICARIA INVISIVEL
        if ( !has_colors() && extra == A_NORMAL ) a = A_REVERSE;
        this->destaquePadrao = false; }

    this->corElem[ elemento ] = a;

    switch ( elemento ) {
        case MW_TEXTO:
            // FUNDO DA JANELA INTEIRA: RECOLORE O QUE JA FOI ESCRITO SEM COR PROPRIA
            wbkgd( this->mainWindow, a | ' ' );
            if ( this->subWindow ) wbkgdset( this->subWindow, a | ' ' );
            this->desenhaBorda();
            this->desenhaTitulo();
            break;
        case MW_BORDA:
            this->desenhaBorda();
            break;
        case MW_TITULO:
            this->desenhaTitulo();
            break;
        case MW_ROTULO:
            for ( int i = 0; i < MAXVARWIN; i++ )
                if ( this->campoWin[ i ].ready )
                    this->escreve( this->campoWin[i].linha, this->campoWin[i].coluna, this->campoWin[i].nome, a );
            break;
        }

    this->show();
}

// ESCREVE O TITULO NA LINHA 1 (APAGA O ANTERIOR, CORTA SE FOR GRANDE DEMAIS)
void MauWin::setTitulo( char *texto )
{
    this->limpaErro();

    if ( texto == NULL ) { this->erro( -11, (char*)"setTitulo: texto nulo" ); return; }

    strncpy( this->titulo, texto, sizeof( this->titulo ) - 1 );
    this->titulo[ sizeof( this->titulo ) - 1 ] = '\0';
    this->desenhaTitulo();
    this->show();
}

void MauWin::relogio( int variavel, char formato[2] ) {
    char            datahora[20];
    time_t          strclk;
    struct tm       *loctime;

    //Formatos: db -> DD/MM/YYYY
    //          du -> YYYY-MM-DD
    //          Db -> DD/MM/YYYY HH:MM:SS
    //          Du -> YYYY-MM-DD HH:MM:SS
    //          hr -> HH:MM:SS
    strclk = time(NULL);
    loctime = localtime( &strclk );
    memset( datahora, '\0', sizeof(datahora) );

    if ( !memcmp(formato, "db", 2) )
        strftime( datahora, sizeof(datahora), "%d/%m/%Y", loctime );

    if ( !memcmp(formato, "du", 2) )
        strftime( datahora, sizeof(datahora), "%Y-%m-%d", loctime );

    if ( !memcmp( formato, "Db", 2 ) )
        strftime( datahora, sizeof(datahora), "%d/%m/%Y %H:%M:%S", loctime );

    if ( !memcmp( formato, "Du", 2 ) )
        strftime( datahora, sizeof(datahora), "%Y-%m-%d %H:%M:%S", loctime );

    if ( !memcmp(formato, "hr", 2) )
        strftime( datahora, sizeof(datahora), "%H:%M:%S", loctime );

    this->printVar( variavel, datahora );
}

// RETORNA A QUANTIDADE DE COLUNAS DA JANELA
int MauWin::getColunas( void ) { return( (int) this->colunas ); }

// IMPRIME TEXTO NA POSICAO ATUAL DO CURSOR
void MauWin::print( char * texto )
{
    this->limpaErro();
    if ( texto == NULL ) { this->erro( -11, (char*)"print: texto nulo" ); return; }
    wprintw( this->mainWindow, "%s", texto );
}

// IMPRIME TEXTO NA LINHA E COLUNA ESPECIFICADOS
void MauWin::print( unsigned short linha, unsigned short coluna, char * texto )
{
   this->limpaErro();
   this->escreve( linha, coluna, texto );
}

// IMPRIME TEXTO NA LINHA E COLUNA ESPECIFICADOS COM UM ATRIBUTO/COR
// (USE MauWin::cor( frente, fundo ) PARA OBTER O ATRIBUTO)
void MauWin::print( unsigned short linha, unsigned short coluna, char * texto, attr_t atributo )
{
   this->limpaErro();
   this->escreve( linha, coluna, texto, atributo );
}

void MauWin::redraw( void )
{
   clear();

   this->desenhaBorda();

   this->show();
}

// APRESENTA O CONTEUDO DA JANELA E ESPERA tempo SEGUNDOS
void MauWin::show( int tempo )
{
   this->show();
   if ( tempo > 0 ) sleep( tempo );      // usleep NAO E GARANTIDO PARA 1 s OU MAIS
}//show

// APRESENTA O CONTEUDO DA JANELA
// ATUALIZA A JANELA PRINCIPAL INTEIRA (QUE INCLUI A SUBJANELA): ANTES SO A
// JANELA ATUAL ERA ATUALIZADA, E O QUE FOSSE ESCRITO NA PRINCIPAL ENQUANTO
// A SUBJANELA ESTAVA SELECIONADA (TITULO, POR EXEMPLO) NAO APARECIA
void MauWin::show( void )
{
   touchwin( this->mainWindow );
   wnoutrefresh( this->mainWindow );
   if ( this->Window != this->mainWindow )
       wnoutrefresh( this->Window );     // DEIXA O CURSOR NA JANELA ATUAL
   doupdate();
}//show

// CONFIGURA UMA VARIAVEL NA POSICAO ESPECIFICADA
// varId  => IDENFICACAO NUMERICA DA VARIAVEL
// linha  => LINHA ONDE SERA APRESENTADA
// coluna => COLUNA ONDE COMECA A IMPRESSAO DE nome
// qtd    => ESPACO MAXIMO RESERVADO PARA O NOME: + VALOR
void MauWin::setVar( unsigned short varId, unsigned short linha, unsigned short coluna, char *nome, int qtd )
{
    this->limpaErro();

    if ( linha > this->linhas || coluna > this->colunas || varId >= MAXVARWIN ) {
        this->erro( -20, (char*)"linha, coluna ou varId fora das especificacoes" );
        return;
    }

    if ( nome == NULL ) {
        this->erro( -23, (char*)"setVar: nome nulo" );
        return;
    }

    if ( qtd <= 0 || qtd + coluna > this->colunas ) return;

    this->campoWin [varId].qtd    = qtd;
    this->campoWin [varId].linha  = linha;
    this->campoWin [varId].coluna = coluna;
    // COPIA LIMITADA AO TAMANHO DO CAMPO E SEMPRE TERMINADA EM '\0'.
    // O NOME GUARDADO E O MESMO IMPRESSO, ENTAO printVar CALCULA A
    // POSICAO DO VALOR LOGO APOS O ROTULO REAL
    strncpy (this->campoWin[varId].nome, nome, MAXVARLEN - 1);
    this->campoWin[varId].nome[ MAXVARLEN - 1 ] = '\0';
    // ROTULO FORA DA AREA UTIL (NA BORDA, POR EXEMPLO): CAMPO NAO FICA PRONTO
    if ( this->escreve( linha, coluna, this->campoWin[varId].nome, this->corElem[ MW_ROTULO ] ) != 0 ) {
        this->campoWin [varId].ready = 0;
        return; }

    this->campoWin [varId].ready  = 1;
    this->show();
}//setVar

// IMPRIME VALOR NA TELA DE VARIAVEL PREVIAMENTE
// CONFIGURADA NA POSICACAO E TAMANHO DEFINIDOS EM 
// ERRO: -20
void MauWin::printVar( unsigned short varId, char *nome )
{
    this->printVar( varId, nome, this->corElem[ MW_VALOR ] );
}

// IMPRIME VALOR DO CAMPO COM UM ATRIBUTO/COR ESPECIFICO (EX.: ALERTA EM VERMELHO)
// O ESPACO RESERVADO INTEIRO RECEBE A COR, ENTAO UM FUNDO COLORIDO MARCA O CAMPO
void MauWin::printVar( unsigned short varId, char *nome, attr_t atributo )
{
    unsigned short  l, c, p, qtd;
    char *limpa;

    this->limpaErro();

    if ( varId >= MAXVARWIN )             { this->erro( -20, (char*)"printVar(int, char*):varId maior ou igual a MAXVARWIN" ); return; }
    if ( !this->campoWin[ varId ].ready ) { this->erro( -21, (char*)"Variavel nao permite apresentacao" ); return; }
    if ( nome == NULL )                   { this->erro( -23, (char*)"printVar(int, char*):valor nulo" ); return; }

    qtd   = this->campoWin[ varId ].qtd;

    limpa = (char *) calloc( qtd + 1, sizeof(char) );

    if ( limpa == NULL ) { this->erro( -22, (char*)"Impossivel alocar memoria para apresentacao da variavel" ); return; }

    memset( limpa, 0x20, qtd );
    limpa[ qtd ] = '\0';
    qtd          = strlen( nome );

    if ( qtd > this->campoWin[ varId ].qtd ) qtd = this->campoWin[ varId ].qtd;

    memcpy( limpa, nome, qtd );

    // CARACTERES DE CONTROLE ('\n', '\t', ESC...) SAO INTERPRETADOS PELO
    // ncurses: '\n' APAGA O RESTO DA LINHA E PULA PARA A DE BAIXO, '\t'
    // AVANCA ATE 8 COLUNAS. NUM CAMPO DE TAMANHO FIXO ISSO DESMONTA A
    // JANELA, ENTAO SAO TROCADOS POR '?'
    for ( int i = 0; i < qtd; i++ )
        if ( (unsigned char) limpa[ i ] < 0x20 || limpa[ i ] == 0x7f )
            limpa[ i ] = '?';

    l = this->campoWin[ varId ].linha;
    c = this->campoWin[ varId ].coluna;
    p = 1 + c + strlen( this->campoWin[ varId ].nome );

    // O VALOR NAO PODE PASSAR DA BORDA DIREITA: SE PASSASSE, O ncurses
    // QUEBRARIA PARA A LINHA DE BAIXO E APAGARIA A BORDA
    int livre = (int) this->colunas - (int) p - ( this->boxed ? 1 : 0 );
    if ( livre <= 0 ) {
        this->erro( -24, (char*)"printVar: rotulo ocupa a linha inteira, sem espaco para o valor" );
        free( limpa );
        return; }
    if ( (int) strlen( limpa ) > livre ) limpa[ livre ] = '\0';

    this->escreve( l, p, limpa, atributo );
    this->show();
    free(limpa);
}

// IMPRIMI UM LOGO ASCII NA TELA
void MauWin::printLogo( unsigned short linhas, unsigned short colunas, unsigned short X, unsigned short Y, char *logo )
{
    unsigned short int i;

    // SE ALGUMA LINHA FALHAR, O ERRO FICA EM errorCode (NAO E APAGADO
    // PELAS LINHAS SEGUINTES)
    this->limpaErro();
    if ( logo == NULL ) { this->erro( -11, (char*)"printLogo: logo nulo" ); return; }

    for ( i = 0; i < linhas; i++ ) {
        if ( this->escreve( X + i, Y, logo + i*colunas ) != 0 ) break;
        }
}

// SELECIONA A JANELA PRINCIPAL
void MauWin::selectMainWin( void )
{
   this->Window =  this->mainWindow;
   this->show();
}

// CONFIGURA SUB JANELA
void MauWin::setSubWin( void )
{
   if ( this->subWindow == NULL )
        // AREA INTERNA ABAIXO DA LINHA DE TITULO: DA LINHA 2 ATE A PENULTIMA,
        // SEM COBRIR A BORDA DE BAIXO (ANTES TINHA linhas-2 E INVADIA A BORDA)
        this->subWindow  = subwin( this->mainWindow, this->linhas-3, this->colunas-2, this->NX+2, this->NY+1 );

   if ( this->subWindow == NULL ) return;

   // A SUBJANELA USA O MESMO FUNDO (MW_TEXTO) DA JANELA
   wbkgdset( this->subWindow, this->corElem[ MW_TEXTO ] | ' ' );
   scrollok(this->subWindow, 1);
   this->selectSubWin();
   this->show();
}

// SELECT A SUB JANELA
void MauWin::selectSubWin( void )
{
   if ( this->subWindow == NULL ) return;
   this->Window =  this->subWindow;
   touchwin( this->mainWindow );
   this->show();
}

// LIMPA A JANELA SELECIONADA
void MauWin::cls(void)
{
    // APAGA A JANELA INTEIRA (INCLUSIVE A AREA DO MENU/TEXTO). werase EM VEZ
    // DE wclear: wclear FORCA O TERMINAL INTEIRO A SER APAGADO E REDESENHADO
    // NO PROXIMO REFRESH, FAZENDO AS OUTRAS JANELAS PISCAREM
    werase( this->mainWindow );

    this->titulo[ 0 ] = '\0';
    this->desenhaBorda();
}

// FUNCOES PARA TRATAMENTO DE MENUS
//
// APRESENTA MENU DE OPCOES NA TELA
// FUNCAO PRIVADA CHAMADA A PARTIR DE MENU
void MauWin::menuShow( int modo, int janela, int index ) {
    int     i,  j, k;
    char    *tela, *t, aux[16];
    int     colunas, MaxLinhas;
    winMenu *opcoes;

    MaxLinhas = this->linhas-4;

    if ( janela < 0 || janela >= this->menuWinSize ) janela = 0;
    if ( index  < 0 || index  >= this->menuWinSize ) index  = 0;

    colunas = this->getColunas();
    if ( colunas < 5 ) return;                    // JANELA ESTREITA DEMAIS PARA O MENU

    tela    = (char*) calloc( colunas, sizeof(char) );
    if ( tela == NULL ) return;

    opcoes = this->menuWin;
    opcoes += janela;

    for ( i = 0, j = janela; i < MaxLinhas && j < this->menuWinSize; i++, j++ ) {
        memset( tela , 0x20, colunas );
        memset( aux, '\0', sizeof(aux) );
        t = &tela[0];

        if ( modo ) snprintf( aux, sizeof(aux), "%02d ", opcoes->valor );
        else        snprintf( aux, sizeof(aux), "%02d ", janela + i + 1 );

        memcpy( t, aux, strlen(aux) );
        t += strlen(aux);

        k = strlen(opcoes->label);
        if ( k  > colunas - 4 - (int)strlen(aux) ) k = colunas - 4 - strlen(aux);
        memcpy( t, opcoes->label, k );
        t += k;

        tela[colunas - 4] = '\0';


        // ITEM SELECIONADO: PAR DE CORES 1 (INICIADO EM menu()) OU,
        // SE O TERMINAL NAO TIVER CORES, VIDEO REVERSO
        if ( index - janela  == i ) {
            wattrset( this->subWindow, this->corDestaque() ); }
        else
            wattrset( this->subWindow, A_NORMAL );

        opcoes++;
        mvwaddstr( this->subWindow, i + 1, 1, tela );
        }

    wattrset( this->subWindow, A_NORMAL );

    memset( tela, 0x20, colunas );
    tela[colunas - 4] = '\0';
    for( ; i < MaxLinhas; i++ ) {                 // LIMPA AS LINHAS RESTANTES (SEM PULAR NENHUMA)
        mvwaddstr( this->subWindow, i + 1, 1, tela ); }

    this->show();

    free(tela);
    tela = NULL;
}

// CONFIGURA UMA ESTRUTURA DE TEXTO PARA SER APRESENTADO
// ESPACO PARA TEXTO GERENCIADO NA CLASSE
void MauWin::textSet( winText *text, unsigned int linhas )
{
    this->limpaErro();

    if ( text == NULL ) {
        this->erro( -30, (char*)"textSet: estrutura de texto nula" );
        return; }

    // LIBERA O BUFFER DE UM textSet ANTERIOR (EVITA VAZAMENTO)
    if ( this->textWin != NULL && this->textWin->Texto != NULL ) {
        free( this->textWin->Texto );
        this->textWin->Texto  = NULL;
        this->textWin->pTexto = NULL; }

    this->textWin          = text;
    this->textWin->Linhas  = linhas;
    this->textWin->Colunas = this->colunas - 2;
    this->textWin->PosLin  = 0;
    this->textWin->PosCol  = 0;
    this->textWin->ErroMsg = 0;
    this->textWin->Formato = 0;
    this->textWin->Texto   = (char*) calloc( this->textWin->Linhas * this->textWin->Colunas, sizeof(char) );
    this->textWin->pTexto  = this->textWin->Texto;

    if ( this->textWin->Texto == NULL ) {
        this->textWin->Linhas = 0;
        this->erro( -31, (char*)"textSet: impossivel alocar memoria para o texto" );
        return; }

    this->setSubWin();
}

// RECEBE UMA ESTRUTURA DE MENUS PARA SEREM APRESENTADOS
// NA TELA
void MauWin::menuSet( winMenu *menu, char *titulo, int qtd )
{
    this->limpaErro();
    this->menuWin       = menu;
    this->menuWinTitulo = titulo;
    this->menuWinSize   = qtd;

    // TITULO NA LINHA 1 DA JANELA PRINCIPAL, NA COR DE MW_TITULO, OCUPANDO A
    // LARGURA INTERNA: APAGA UM TITULO ANTERIOR MAIS LONGO E CORTA UM GRANDE DEMAIS
    if ( titulo != NULL ) {
        strncpy( this->titulo, titulo, sizeof( this->titulo ) - 1 );
        this->titulo[ sizeof( this->titulo ) - 1 ] = '\0';
        this->desenhaTitulo(); }
    this->show();
}

// AGUARDA UMA TECLA POR ATE 1 SEGUNDO (halfdelay USA DECIMOS DE SEGUNDO)
// RETORNA ERR (-1) SE NENHUMA TECLA FOR PRESSIONADA.
// OBS: O MODO halfdelay CONTINUA ATIVO APOS A CHAMADA; menu() O DESFAZ.
int MauWin::getKey( void )
{
    cbreak();
    halfdelay( 10 );
    // SETAS E TECLAS DE FUNCAO CHEGAM COMO KEY_UP, KEY_DOWN... EM VEZ DE
    // UMA SEQUENCIA QUE COMECA COM ESC (QUE PARECERIA UM ESC SOZINHO)
    keypad( this->Window, TRUE );
    return( wgetch(this->Window) );
}

// MENU INTERATIVO NA TELA
//
// modo -1 => APENAS MOSTRA O MENU NUMERADO 01, 02, ...  E RETORNA 0
// modo -2 => APENAS MOSTRA O MENU COM O CAMPO valor     E RETORNA 0
// modo  0 => MENU INTERATIVO NUMERADO 01, 02, ...
// modo  1 => MENU INTERATIVO COM O CAMPO valor
//
// TECLAS: SETAS CIMA/BAIXO, PAGE UP/DOWN, HOME/END, ENTER CONFIRMA,
//         ESC CANCELA, DIGITOS ESCOLHEM PELA POSICAO (01, 02, ... 12 ...)
//
// RETORNO: INDICE (BASE 0) DO ITEM ESCOLHIDO, -1 SE CANCELADO COM ESC,
//          -2 SE NAO HA MENU CONFIGURADO (menuSet)
int MauWin::menu( int modo )
{
    int      key, index, janela, MaxLinhas, numero, escdelay;

    if ( this->menuWin == NULL || this->menuWinSize <= 0 ) return( -2 );

    MaxLinhas = this->linhas - 4;                 // ITENS VISIVEIS DE UMA VEZ
    if ( MaxLinhas < 1 ) return( -2 );


    this->setSubWin();

    if ( modo == -1 ) { this->menuShow( 0, 0, 0 ); return 0; }
    if ( modo == -2 ) { this->menuShow( 1, 0, 0 ); return 0; }

    // MODO DE ENTRADA CONHECIDO: nocbreak() DESFAZ UM halfdelay() DEIXADO
    // POR getKey(); SEM ISSO wgetch RETORNARIA ERR A CADA SEGUNDO
    noecho();
    nocbreak();
    cbreak();
    keypad( this->Window, TRUE );
    wtimeout( this->Window, -1 );                 // ESPERA BLOQUEANTE

    // ESC SOZINHO E RECONHECIDO EM 100ms (O PADRAO SAO 1000ms)
    escdelay = ESCDELAY;
    set_escdelay( 100 );

    index  = 0;
    janela = 0;
    numero = 0;                                   // NUMERO SENDO DIGITADO

    this->menuShow( modo, janela, index );        // MOSTRA ANTES DA 1a TECLA

    while ( 1 ) {
        key = wgetch( this->Window );

        if ( key >= '0' && key <= '9' ) {
            // ACUMULA DIGITOS ENQUANTO O NUMERO EXISTIR NO MENU;
            // SENAO RECOMECA A PARTIR DO DIGITO ATUAL ("1","2" => 12 OU 2)
            numero = numero * 10 + ( key - '0' );
            if ( numero < 1 || numero > this->menuWinSize )
                numero = key - '0';
            if ( numero >= 1 && numero <= this->menuWinSize )
                index = numero - 1;
            }
        else {
            numero = 0;

            switch ( key ) {
                case KEY_DOWN:  if ( index < this->menuWinSize - 1 ) index++;    break;
                case KEY_UP:    if ( index > 0 ) index--;                        break;
                case KEY_NPAGE: index += MaxLinhas;                              break;
                case KEY_PPAGE: index -= MaxLinhas;                              break;
                case KEY_HOME:  index = 0;                                       break;
                case KEY_END:   index = this->menuWinSize - 1;                   break;

                case '\n': case '\r': case KEY_ENTER:
                    set_escdelay( escdelay );
                    return( index );

                case ESCAPE:
                    set_escdelay( escdelay );
                    return( -1 );

                case KEY_RESIZE:                                                 break;
                default:        continue;         // TECLA SEM FUNCAO: IGNORA
                }
            }

        if ( index < 0 )                     index = 0;
        if ( index > this->menuWinSize - 1 ) index = this->menuWinSize - 1;

        // ROLAGEM: MANTEM O ITEM SELECIONADO DENTRO DA AREA VISIVEL
        if ( index < janela )               janela = index;
        if ( index >= janela + MaxLinhas )  janela = index - MaxLinhas + 1;

        this->menuShow( modo, janela, index );
        }// while
}

// FUNCOES PARA TEXTO EM JANELA
// ESTRUTURA textWin DEFINIA EM MauWin.h
//
//
// RETORNA A POSICAO ATUAL DE ESCRITA
int MauWin::textGetPos( void )
{
    int i;

    // textSet NAO FOI CHAMADO (OU FALHOU)
    if ( this->textWin == NULL || this->textWin->Texto == NULL )
        return( -1 );

    if ( this->textWin->PosLin >= this->textWin->Linhas ) {
        this->textWin->ErroMsg = -22;
        return( -1 ); }

    i = ( this->textWin->PosLin * this->textWin->Colunas + this->textWin->PosCol );

    return( i );
}

void MauWin::textIncMemTexto( unsigned int qtd )
{
    if ( qtd == 0 || qtd > ( this->textWin->Colunas - this->textWin->PosCol ) ) {
        this->textWin->ErroMsg = -1;
        return; }

    this->textWin->PosCol += qtd;

    if ( this->textWin->PosCol >= this->textWin->Colunas ) {
        this->textWin->PosLin++;
        this->textWin->PosCol = 0; }

}

// CENTRALIZA LABEL EM UMA LINHA
void MauWin::textAppend ( char* label )
{
    int centro;

    if ( this->textGetPos() < 0 )
        return;

    if ( this->textWin->PosCol != 0 )
        this->textNewLine();

    centro = floor( (this->textWin->Colunas - strlen( label) - 2 ) / 2 );

    this->textAppend( centro, label );
    this->textPadTexto( this->textWin->Colunas - 1 );

}

// JUSTIFICA A ESQUERDA DA POSICAO--TABULACAO DE VALORES   9,99|
//                                                       999,99|
void MauWin::textAppend ( char* conteudo, unsigned int posicao ) 
{
    unsigned int aux;

    if ( this->textGetPos() < 0 )
        return;

    if ( posicao == 0 ) {
        this->textWin->ErroMsg = -3;
        return; }

    if ( conteudo == NULL ) {
        this->textWin->ErroMsg = -6;
        return; }

    aux     = strlen( conteudo );

    // CONTEUDO MAIOR QUE A POSICAO: posicao -= aux DARIA UNDERFLOW (unsigned)
    if ( aux > posicao ) {
        this->textWin->ErroMsg = -7;
        return; }

    posicao -= aux;

    if ( posicao < this->textWin->PosCol ) {
        this->textWin->ErroMsg = -7;
        return; }

    if ( aux > ( this->textWin->Colunas - posicao ) ) {
        this->textWin->ErroMsg = -8;
        return; }


    if ( posicao != this->textWin->PosCol )
        this->textPadTexto( posicao );

    this->textWin->pTexto = this->textWin->Texto + this->textGetPos();
    aux = strlen( conteudo );
    memcpy( this->textWin->pTexto, conteudo, aux );
    this->textIncMemTexto( aux );

}

// ACRESCENTA UMA NOVA LINHA
void MauWin::textNewLine ( void )
{
    int qtdspc;
    unsigned int getpos;

    if ( this->textGetPos() < 0 )
        return;

    getpos = this->textGetPos();

    qtdspc  = 0;
    this->textWin->pTexto  = this->textWin->Texto + getpos;

    if ( this->textWin->Formato ) {
        *this->textWin->pTexto = '\n';
        this->textIncMemTexto( 1 );}
    else {
        qtdspc = this->textWin->Colunas - this->textWin->PosCol;
        memset( this->textWin->pTexto, 0x20, qtdspc );
        this->textIncMemTexto( qtdspc ); }

}//NewLine

// ADICIONA UMA LINHA EM BRANCO
void MauWin::textAddLine(void)
{
    if ( this->textGetPos() < 0 )
        return;

    // TERMINA A LINHA ATUAL, SE TIVER ALGO ESCRITO...
    if ( this->textWin->PosCol != 0 ) {
        this->textNewLine();
        if ( this->textGetPos() < 0 )       // CHEGOU AO FIM DO BUFFER
            return; }

    // ...E ACRESCENTA EXATAMENTE UMA LINHA EM BRANCO (textNewLine NO INICIO
    // DA LINHA PREENCHE A LINHA INTEIRA COM ESPACOS). ANTES, NO INICIO DE
    // UMA LINHA, ERAM GERADAS DUAS LINHAS EM BRANCO
    this->textNewLine();

} //AddLine

// LIMPA A AREA DE MEMORIA QUE ARMAZENA O TEXTO
void MauWin::textLimpa ( void )
{
    if ( this->textWin == NULL || this->textWin->Texto == NULL ) return;

    memset( this->textWin->Texto, '\0', this->textWin->Linhas * this->textWin->Colunas );
    this->textWin->pTexto = this->textWin->Texto;
    this->textWin->PosCol = 0;
    this->textWin->PosLin = 0;
}

// RETORNA CONTEUDO DE TEXTO
void MauWin::textGetTexto( char *retorna, unsigned int tamanho )
{
    if ( this->textGetPos() < 0 ) {
        retorna = NULL;
        return; }

    if ( (int)tamanho >= this->textGetPos() )
        memcpy( retorna, this->textWin->Texto, this->textGetPos() );
    else {
        this->textWin->ErroMsg = -2;
        retorna = NULL; }
}

// APRESENTA TEXTO A PARTIR DA POSICAO ESPECIFICADA
void MauWin::textShow( unsigned int linha )
{
    int     i, MaxLinhas, colunas, largura, tamanho;
    char    *texto, *final;

    if ( this->textWin == NULL || this->textWin->Texto == NULL ) return;

    this->selectSubWin();
    if ( this->Window != this->subWindow ) return;

    // A SUBJANELA TEM EXATAMENTE Colunas DE LARGURA: CADA LINHA DO BUFFER
    // OCUPA UMA LINHA INTEIRA A PARTIR DA COLUNA 0 (ANTES COMECAVA NA 1 E O
    // ULTIMO CARACTERE QUEBRAVA PARA A LINHA DE BAIXO)
    getmaxyx( this->subWindow, MaxLinhas, largura );
    colunas = this->textWin->Colunas;
    if ( colunas > largura ) colunas = largura;

    // FIM REAL DO TEXTO (pTexto NAO AVANCA SOBRE O PREENCHIMENTO DE ESPACOS)
    tamanho = this->textGetPos();
    if ( tamanho < 0 ) tamanho = this->textWin->Linhas * this->textWin->Colunas;

    if ( linha >= this->textWin->Linhas ) linha = 0;

    texto = this->textWin->Texto + linha * this->textWin->Colunas;
    final = this->textWin->Texto + tamanho;

    // ESCREVER NO CANTO INFERIOR DIREITO COM scrollok ATIVO ROLARIA A JANELA
    scrollok( this->Window, FALSE );
    wattrset( this->Window, A_NORMAL );

    for ( i = 0; i < MaxLinhas; i++, texto += this->textWin->Colunas ) {
        int n = 0;

        if ( texto < final ) {
            n = final - texto;
            if ( n > colunas ) n = colunas;
            // PARA NO '\0' (AREA AINDA NAO ESCRITA DO BUFFER)
            n = strnlen( texto, n );
            mvwaddnstr( this->Window, i, 0, texto, n ); }

        if ( n < colunas ) {                    // APAGA O RESTO DA LINHA
            wmove( this->Window, i, n );
            wclrtoeol( this->Window ); }
        }

    scrollok( this->Window, TRUE );
    this->show();
}

// ACRESCENTE ESPACOS ATE A POSICAO ESPECIFICADA
void MauWin::textPadTexto( unsigned int posicao ) 
{
    if ( this->textGetPos() < 0 ) return;

    if ( posicao < this->textWin->PosCol || posicao >= this->textWin->Colunas ) {
        this->textWin->ErroMsg = -5;
        return; }

    this->textWin->pTexto = this->textWin->Texto + this->textGetPos();

    memset( this->textWin->pTexto, 0x20, posicao - this->textWin->PosCol );

    this->textIncMemTexto( posicao - this->textWin->PosCol );
}

// LINHA CONTINUA DE CARACTERS CARAC
void MauWin::textAppend ( char carac )
{
    if ( this->textGetPos() < 0 )
        return;

    if ( this->textWin->PosCol != 0 )
        this->textNewLine();

    // textNewLine PODE TER CHEGADO AO FIM DO BUFFER
    if ( this->textGetPos() < 0 )
        return;

    this->textWin->pTexto = this->textWin->Texto + this->textGetPos();
    memset( this->textWin->pTexto, carac, this->textWin->Colunas );
    this->textIncMemTexto( this->textWin->Colunas );
}

void MauWin::textAppend ( unsigned int posicao, unsigned int largura, char* conteudo )
{
    unsigned int aux, pad;

    aux = pad = 0;

    if ( this->textGetPos() < 0 )
        return;

    if ( posicao == 0 ){
        this->textWin->ErroMsg = -3;
        return; }

    posicao--;                                                      //ZERO BASE INDEX

    if ( conteudo == NULL || posicao < this->textWin->PosCol ) {
        this->textWin->ErroMsg = -4;
        return; }

    if ( largura > ( this->textWin->Colunas - posicao ) ) {
        this->textWin->ErroMsg = -5;
        return; }

    aux = strlen( conteudo );

    if ( aux < largura )
        pad = largura - aux;


    if ( posicao != this->textWin->PosCol )
        this->textPadTexto( posicao );

    this->textWin->pTexto = this->textWin->Texto + this->textGetPos();

    if ( pad ) {
        aux = strlen( conteudo );
        memcpy( this->textWin->pTexto, conteudo, aux );
        this->textWin->pTexto += aux;
        memset( this->textWin->pTexto, 0x20, pad );
        this->textIncMemTexto( aux + pad ); }
    else {
        memcpy( this->textWin->pTexto, conteudo, largura );
        this->textIncMemTexto( largura ); }
}

// NAO LIMITA O TAMANHO - SE NAO COUBER ARMAZENA ATE O FINAL DA COLUNA
void MauWin::textAppend( unsigned int posicao, char* conteudo )
{
    unsigned int aux;

    aux        = 0;

    if ( this->textGetPos() < 0 )
        return;

    if ( posicao <= 0 ) {
        this->textWin->ErroMsg = -3;
        return; }

    posicao--;           // ZERO BASE INDEX

    if ( conteudo == NULL || posicao < this->textWin->PosCol ) {
        this->textWin->ErroMsg = -4;
        return; }

    if ( posicao != this->textWin->PosCol )  // COMPLETAR COM ESPACOS
        this->textPadTexto( posicao );

    this->textWin->pTexto  = this->textWin->Texto + this->textGetPos();

    aux = this->textWin->Colunas - posicao;

    // informacao maior que o espaco livre
    if ( strlen( conteudo )  > aux ) {
        memcpy( this->textWin->pTexto, conteudo, aux );
        this->textIncMemTexto( aux ); }
    else {
        memcpy( this->textWin->pTexto, conteudo, strlen( conteudo ) );
        this->textIncMemTexto( strlen( conteudo ) ); }

}//Append
