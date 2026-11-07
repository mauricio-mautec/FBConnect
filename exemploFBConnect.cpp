// EXEMPLO DE USO DA CLASSE FBConnect COM MauWin (FIREBIRD 5.0)
//
// PROGRAMA INTERATIVO EM MODO TEXTO (ncursesw) QUE DEMONSTRA OS RECURSOS DA
// CLASSE FBConnect UTILIZANDO MauWin PARA OS MENUS E APRESENTACAO DOS DADOS:
//   1. BARRA DE STATUS COM RELOGIO, VERSAO DO FIREBIRD 5.0 E ULTIMA OPERACAO
//   2. RELATORIO DE FUNCIONARIOS E DEPARTAMENTOS (Select + JOIN + rolagem)
//   3. CONSULTA DE FICHA DE FUNCIONARIO (menu modo 1 + SelectBind + getRowMap)
//   4. PROJETOS E DESCRICOES BLOB (GetTextBlob + SetTextBlob em tempo real)
//   5. CLIENTES: CONSULTA, INSERCAO EM LOTE (ExecuteBatch) E UPDATE RETURNING
//   6. VITRINE DE TIPOS DO FIREBIRD 5.0 (TYPE_SHOWCASE: INT128, DECFLOAT, TZ...)
//   7. DIAGNOSTICO DE CONEXAO E SEQUENCES (Ping, Reconnect, getNextSequence)
//
// COMPILAR:  make exemploFBConnect
// RODAR:     ./exemploFBConnect        (TERMINAL DE PELO MENOS 24x80)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <time.h>
#include <vector>
#include <string>
#include <map>
#include <ncurses.h>
#include "MauWin.h"
#include "FBConnect.h"

#define S(x) ((char*)(x))

#define FB_PATH "127.0.0.1:/var/lib/firebird/data/mautec.fdb"
#define FB_USER "softgo"
#define FB_PASS "pqIobXtPui94hw9"

// CAMPOS DA BARRA DE STATUS SUPERIOR
enum {
    CAMPO_HORA = 0,
    CAMPO_SERVIDOR = 1,
    CAMPO_STATUS = 2
};

// ---------------------------------------------------------------------------
// TEMA PADRAO DAS JANELAS
static void aplicaTema( MauWin &janela )
{
    janela.setCor( MW_BORDA,    COLOR_CYAN,  MW_PADRAO );
    janela.setCor( MW_TITULO,   COLOR_BLACK, COLOR_CYAN );
    janela.setCor( MW_DESTAQUE, COLOR_BLACK, COLOR_YELLOW );
    janela.setCor( MW_ROTULO,   COLOR_CYAN,  MW_PADRAO );
    janela.setCor( MW_VALOR,    MW_PADRAO,   MW_PADRAO, A_BOLD );
}

// ATUALIZA O CAMPO DE STATUS NA BARRA SUPERIOR COM COR DE DESTAQUE
static void atualizaStatus( MauWin &status, const char *msg, bool erro = false )
{
    attr_t cor = erro
        ? MauWin::cor( COLOR_WHITE, COLOR_RED,  A_BOLD )
        : MauWin::cor( COLOR_YELLOW, COLOR_BLUE, A_BOLD );
    status.printVar( CAMPO_STATUS, S( msg ), cor );
}

// AUXILIAR: ADICIONA TEXTO LONGO QUEBRANDO EM LINHAS DE LARGURA FIXA NO winText
static void adicionaTextoQuebrado( MauWin &janela, unsigned int colInicio, unsigned int larguraMax, const std::string &texto )
{
    size_t pos = 0;
    while ( pos < texto.size() ) {
        size_t restante = texto.size() - pos;
        size_t fatia = restante > larguraMax ? larguraMax : restante;
        if ( restante > larguraMax ) {
            size_t espaco = texto.rfind( ' ', pos + larguraMax );
            if ( espaco != std::string::npos && espaco > pos ) {
                fatia = espaco - pos;
            }
        }
        std::string linha = texto.substr( pos, fatia );
        janela.textAppend( colInicio, larguraMax, S( linha.c_str() ) );
        janela.textNewLine();
        pos += fatia;
        while ( pos < texto.size() && texto[pos] == ' ' ) pos++;
    }
}

// AUXILIAR: LACO DE ROLAGEM PADRAO PARA JANELAS COM textShow
static void lacoRolagem( MauWin &janela, MauWin &status, winText &texto, int alturaJanela )
{
    int visiveis = alturaJanela - 3;
    int ultima   = (int)texto.PosLin > visiveis ? (int)texto.PosLin - visiveis : 0;
    int topo     = 0;

    janela.textShow( topo );

    while ( 1 ) {
        int tecla = janela.getKey();
        status.relogio( CAMPO_HORA, S( "Db" ) );

        if ( tecla == ERR ) continue;
        if ( tecla == ESCAPE || tecla == 'q' || tecla == 'Q' ) break;

        if ( ( tecla == KEY_DOWN || tecla == '+' ) && topo < ultima ) topo++;
        if ( ( tecla == KEY_UP   || tecla == '-' ) && topo > 0 )      topo--;
        if ( tecla == KEY_NPAGE ) topo = topo + visiveis > ultima ? ultima : topo + visiveis;
        if ( tecla == KEY_PPAGE ) topo = topo - visiveis < 0 ? 0 : topo - visiveis;
        if ( tecla == KEY_HOME )  topo = 0;
        if ( tecla == KEY_END )   topo = ultima;

        janela.textShow( topo );
    }
}

// ---------------------------------------------------------------------------
// 1. RELATORIO DE FUNCIONARIOS E DEPARTAMENTOS (Select + JOIN + textShow)
static void telaRelatorioFuncionarios( MauWin &status, FBConnect &db )
{
    winText texto;
    memset( &texto, 0, sizeof( texto ) );

    MauWin janela( 20, 76, 3, 2, 1 );
    if ( janela.errorCode ) return;
    aplicaTema( janela );

    janela.setTitulo( S( " QUADRO DE FUNCIONARIOS (setas/PgUp/PgDn rolam, ESC volta)" ) );
    janela.textSet( &texto, 60 );

    const char *sql =
        "SELECT E.EMP_NO, E.FIRST_NAME, E.LAST_NAME, D.DEPARTMENT, J.JOB_TITLE, E.HIRE_DATE, E.SALARY "
        "FROM EMPLOYEE E "
        "JOIN DEPARTMENT D ON D.DEPT_NO = E.DEPT_NO "
        "JOIN JOB J ON J.JOB_CODE = E.JOB_CODE AND J.JOB_GRADE = E.JOB_GRADE "
        "ORDER BY E.EMP_NO";

    if ( db.Select( sql ) != 0 ) {
        atualizaStatus( status, db.ErrorMsg, true );
        return;
    }

    janela.textAppend( 2u,  S( "ID" ) );
    janela.textAppend( 7u,  S( "NOME COMPLETO" ) );
    janela.textAppend( 28u, S( "DEPARTAMENTO" ) );
    janela.textAppend( 50u, S( "ADMISSAO" ) );
    janela.textAppend( S( "SALARIO" ), 72u );
    janela.textNewLine();
    janela.textAppend( '-' );

    double folhaTotal = 0.0;
    int    qtdFunc    = 0;

    while ( db.Fetch() == 0 ) {
        std::string id   = db.getColumn( "EMP_NO" );
        std::string nome = db.getColumn( "FIRST_NAME" ) + " " + db.getColumn( "LAST_NAME" );
        std::string dept = db.getColumn( "DEPARTMENT" );
        std::string adm  = db.getColumn( "HIRE_DATE" );
        double      sal  = db.getDouble( "SALARY" );

        char salFmt[24];
        snprintf( salFmt, sizeof( salFmt ), "%10.2f", sal );

        janela.textAppend( 2u,  4u,  S( id.c_str() ) );
        janela.textAppend( 7u,  20u, S( nome.c_str() ) );
        janela.textAppend( 28u, 20u, S( dept.c_str() ) );
        janela.textAppend( 50u, 10u, S( adm.c_str() ) );
        janela.textAppend( salFmt, 72u );
        janela.textNewLine();

        folhaTotal += sal;
        qtdFunc++;
    }
    db.Commit();

    janela.textAppend( '-' );
    char resumo[40], totalFmt[24];
    snprintf( resumo,   sizeof( resumo ),   "TOTAL (%d FUNCIONARIOS)", qtdFunc );
    snprintf( totalFmt, sizeof( totalFmt ), "R$ %10.2f", folhaTotal );
    janela.textAppend( 2u, resumo );
    janela.textAppend( totalFmt, 72u );
    janela.textNewLine();

    atualizaStatus( status, "Relatorio EMPLOYEE x DEPARTMENT" );
    lacoRolagem( janela, status, texto, 20 );
}

// ---------------------------------------------------------------------------
// 2. CONSULTA DE FICHA DE FUNCIONARIO (menu modo 1 + SelectBind + setVar/printVar)
static void mostraFichaFuncionario( MauWin &status, FBConnect &db, int empNo )
{
    char paramEmp[16];
    snprintf( paramEmp, sizeof( paramEmp ), "%d", empNo );
    const char *params[] = { paramEmp };

    const char *sql =
        "SELECT E.EMP_NO, E.FIRST_NAME, E.LAST_NAME, E.DEPT_NO, D.DEPARTMENT, "
        "       D.LOCATION, J.JOB_TITLE, E.JOB_GRADE, E.HIRE_DATE, E.SALARY "
        "FROM EMPLOYEE E "
        "JOIN DEPARTMENT D ON D.DEPT_NO = E.DEPT_NO "
        "JOIN JOB J ON J.JOB_CODE = E.JOB_CODE AND J.JOB_GRADE = E.JOB_GRADE "
        "WHERE E.EMP_NO = ?";

    if ( db.SelectBind( sql, params, 1 ) != 0 || db.Fetch() != 0 ) {
        atualizaStatus( status, db.ErrorMsg, true );
        return;
    }

    std::map<std::string, std::string> row = db.getRowMap();
    double salario = db.getDouble( "SALARY" );
    db.Commit();

    // BUSCA OS PROJETOS ALOCADOS AO FUNCIONARIO
    std::string listaProjetos;
    if ( db.SelectBind(
            "SELECT P.PROJ_ID, P.PROJ_NAME FROM EMPLOYEE_PROJECT EP "
            "JOIN PROJECT P ON P.PROJ_ID = EP.PROJ_ID WHERE EP.EMP_NO = ? ORDER BY P.PROJ_ID",
            params, 1 ) == 0 ) {
        while ( db.Fetch() == 0 ) {
            if ( !listaProjetos.empty() ) listaProjetos += ", ";
            listaProjetos += db.getColumn( 0 ) + " (" + db.getColumn( 1 ) + ")";
        }
        db.Commit();
    }
    if ( listaProjetos.empty() ) listaProjetos = "Nenhum projeto vinculado";

    MauWin ficha( 16, 68, 4, 6, 1 );
    if ( ficha.errorCode ) return;
    aplicaTema( ficha );
    ficha.setTitulo( S( " FICHA CADASTRAL DO FUNCIONARIO (SelectBind + getRowMap)" ) );

    std::string nomeCompleto = row["FIRST_NAME"] + " " + row["LAST_NAME"];
    std::string deptoCompleto = row["DEPT_NO"] + " - " + row["DEPARTMENT"] + " (" + row["LOCATION"] + ")";
    std::string cargoCompleto = row["JOB_TITLE"] + " [Nivel " + row["JOB_GRADE"] + "]";

    char salStr[32];
    snprintf( salStr, sizeof( salStr ), "R$ %.2f", salario );

    struct { int linha; const char *rotulo; int tam; const char *valor; } campos[] = {
        {  3, "Matricula...:", 12, row["EMP_NO"].c_str()   },
        {  4, "Nome........:", 46, nomeCompleto.c_str()    },
        {  6, "Departamento:", 46, deptoCompleto.c_str()   },
        {  7, "Cargo.......:", 46, cargoCompleto.c_str()   },
        {  9, "Admissao....:", 16, row["HIRE_DATE"].c_str()},
        { 10, "Salario.....:", 28, salStr                  },
        { 12, "Projetos....:", 48, listaProjetos.c_str()   },
    };

    for ( int i = 0; i < (int)( sizeof( campos ) / sizeof( campos[0] ) ); i++ ) {
        ficha.setVar( i, campos[i].linha, 2, S( campos[i].rotulo ), campos[i].tam );
        if ( ficha.errorCode == 0 )
            ficha.printVar( i, S( campos[i].valor ) );
    }

    // DESTAQUE COLORIDO POR FAIXA SALARIAL
    if ( salario >= 15000.0 ) {
        snprintf( salStr, sizeof( salStr ), "R$ %.2f (SENIOR/GESTAO)", salario );
        ficha.printVar( 5, salStr, MauWin::cor( COLOR_BLACK, COLOR_GREEN, A_BOLD ) );
    } else if ( salario < 8000.0 ) {
        snprintf( salStr, sizeof( salStr ), "R$ %.2f (INICIAL)", salario );
        ficha.printVar( 5, salStr, MauWin::cor( COLOR_BLACK, COLOR_YELLOW, A_BOLD ) );
    }

    ficha.print( 14, 2, S( "Pressione qualquer tecla para voltar..." ) );
    ficha.show();

    atualizaStatus( status, nomeCompleto.c_str() );

    while ( ficha.getKey() == ERR )
        status.relogio( CAMPO_HORA, S( "Db" ) );
}

static void telaConsultaFuncionario( MauWin &status, FBConnect &db )
{
    if ( db.Select( "SELECT EMP_NO, FIRST_NAME, LAST_NAME, JOB_CODE FROM EMPLOYEE ORDER BY EMP_NO" ) != 0 ) {
        atualizaStatus( status, db.ErrorMsg, true );
        return;
    }

    std::vector<winMenu> itens;
    while ( db.Fetch() == 0 ) {
        winMenu item;
        memset( &item, 0, sizeof( item ) );
        std::string label = db.getColumn( "FIRST_NAME" ) + " " + db.getColumn( "LAST_NAME" )
                          + " [" + db.getColumn( "JOB_CODE" ) + "]";
        snprintf( item.label, MAXVARLEN, "%s", label.c_str() );
        item.valor       = (int)db.getInt( "EMP_NO" );
        item.selecionado = 0;
        itens.push_back( item );
    }
    db.Commit();

    if ( itens.empty() ) return;

    while ( 1 ) {
        clear();
        refresh();
        status.show();
        status.relogio( CAMPO_HORA, S( "Db" ) );

        MauWin janela( 14, 54, 5, 13, 1 );
        aplicaTema( janela );
        janela.menuSet( &itens[0], S( " SELECIONE O FUNCIONARIO (ESC volta)" ), (int)itens.size() );

        int idx = janela.menu( 1 ); // MODO 1: EXIBE O EMP_NO GUARDADO EM item.valor
        if ( idx < 0 ) break;

        mostraFichaFuncionario( status, db, itens[idx].valor );
    }
}

// ---------------------------------------------------------------------------
// 3. PROJETOS E CAMPOS BLOB (GetTextBlob + SetTextBlob)
static void mostraProjetoBlob( MauWin &status, FBConnect &db, const std::string &projId )
{
    while ( 1 ) {
        const char *params[] = { projId.c_str() };
        const char *sql =
            "SELECT P.PROJ_ID, P.PROJ_NAME, P.PRODUCT, E.FIRST_NAME, E.LAST_NAME, P.PROJ_DESC "
            "FROM PROJECT P "
            "LEFT JOIN EMPLOYEE E ON E.EMP_NO = P.TEAM_LEADER "
            "WHERE P.PROJ_ID = ?";

        if ( db.SelectBind( sql, params, 1 ) != 0 || db.Fetch() != 0 ) {
            atualizaStatus( status, db.ErrorMsg, true );
            return;
        }

        std::string nomeProj = db.getColumn( 1 );
        std::string produto  = db.getColumn( 2 );
        std::string lider    = db.getColumn( 3 ) + " " + db.getColumn( 4 );
        db.getColumn( 5 ); // CARREGA O BLOB ID E ABRE O HANDLER INTERNO
        int blobBytes = db.blob ? db.getBlobSize( db.blob ) : 0;
        std::string descBlob = db.GetTextBlob();
        db.Commit();

        winText texto;
        memset( &texto, 0, sizeof( texto ) );

        MauWin janela( 20, 76, 3, 2, 1 );
        if ( janela.errorCode ) return;
        aplicaTema( janela );

        janela.setTitulo( S( " PROJETO E DESCRICAO BLOB (Tecla 'A' grava nota no BLOB, ESC volta)" ) );
        janela.textSet( &texto, 40 );

        std::string l1 = "PROJETO: " + projId + " - " + nomeProj;
        std::string l2 = "PRODUTO: " + produto + "   |   LIDER: " + lider;
        char l3[80];
        snprintf( l3, sizeof( l3 ), "TAMANHO DO BLOB (SUB_TYPE TEXT): %d bytes", blobBytes );

        janela.textAppend( 2u, S( l1.c_str() ) );
        janela.textNewLine();
        janela.textAppend( 2u, S( l2.c_str() ) );
        janela.textNewLine();
        janela.textAppend( 2u, l3 );
        janela.textNewLine();
        janela.textAppend( '-' );
        janela.textAppend( 2u, S( "CONTEUDO DO BLOB (lido via FBConnect::GetTextBlob):" ) );
        janela.textNewLine();
        janela.textAddLine();

        adicionaTextoQuebrado( janela, 2u, 70u, descBlob );

        janela.textAddLine();
        janela.textAppend( '-' );
        janela.textAppend( 2u, S( "Pressione [A] para anexar auditoria via SetTextBlob ou [ESC] para sair" ) );
        janela.textNewLine();

        janela.textShow( 0 );
        atualizaStatus( status, ( "BLOB " + projId + " lido com sucesso" ).c_str() );

        bool gravou = false;
        while ( 1 ) {
            int k = janela.getKey();
            status.relogio( CAMPO_HORA, S( "Db" ) );
            if ( k == ERR ) continue;
            if ( k == ESCAPE || k == 'q' || k == 'Q' ) return;
            if ( k == 'a' || k == 'A' ) {
                time_t agora = time( NULL );
                struct tm *tmInfo = localtime( &agora );
                char carimbo[64];
                strftime( carimbo, sizeof( carimbo ), " [Auditado em %d/%m/%Y %H:%M:%S]", tmInfo );
                std::string novoTexto = descBlob + carimbo;

                std::string sqlUpd = "UPDATE PROJECT SET PROJ_DESC = ? WHERE PROJ_ID = '" + projId + "'";
                db.Start( FBWRITE );
                if ( db.SetTextBlob( sqlUpd.c_str(), novoTexto.c_str(), (int)novoTexto.size() ) == 0 ) {
                    db.Commit();
                    atualizaStatus( status, "SetTextBlob gravado no Firebird 5.0!" );
                    gravou = true;
                } else {
                    db.Rollback();
                    atualizaStatus( status, db.ErrorMsg, true );
                }
                break;
            }
        }
        if ( !gravou ) break;
    }
}

static void telaProjetosBlob( MauWin &status, FBConnect &db )
{
    if ( db.Select( "SELECT PROJ_ID, PROJ_NAME, PRODUCT FROM PROJECT ORDER BY PROJ_ID" ) != 0 ) {
        atualizaStatus( status, db.ErrorMsg, true );
        return;
    }

    std::vector<std::string> ids;
    std::vector<winMenu> itens;
    while ( db.Fetch() == 0 ) {
        winMenu item;
        memset( &item, 0, sizeof( item ) );
        std::string id   = db.getColumn( 0 );
        std::string nome = db.getColumn( 1 );
        std::string prod = db.getColumn( 2 );
        snprintf( item.label, MAXVARLEN, "%-6s | %-32s (%s)", id.c_str(), nome.c_str(), prod.c_str() );
        item.valor = (int)ids.size() + 1;
        itens.push_back( item );
        ids.push_back( id );
    }
    db.Commit();

    if ( itens.empty() ) return;

    while ( 1 ) {
        clear();
        refresh();
        status.show();
        status.relogio( CAMPO_HORA, S( "Db" ) );

        MauWin janela( 12, 64, 6, 8, 1 );
        aplicaTema( janela );
        janela.menuSet( &itens[0], S( " PROJETOS COM CAMPO BLOB (ESC volta)" ), (int)itens.size() );

        int idx = janela.menu( 0 );
        if ( idx < 0 ) break;

        mostraProjetoBlob( status, db, ids[idx] );
    }
}

// ---------------------------------------------------------------------------
// 4. CLIENTES: LISTAGEM, ExecuteBatch E UPDATE RETURNING
static void telaClientes( MauWin &status, FBConnect &db )
{
    while ( 1 ) {
        winText texto;
        memset( &texto, 0, sizeof( texto ) );

        MauWin janela( 20, 76, 3, 2, 1 );
        if ( janela.errorCode ) return;
        aplicaTema( janela );

        janela.setTitulo( S( " CLIENTES (Tecla 'B' testa ExecuteBatch/RETURNING, ESC volta)" ) );
        janela.textSet( &texto, 60 );

        if ( db.Select( "SELECT CUST_NO, CUSTOMER, CONTACT_FIRST, CONTACT_LAST, CITY, PHONE_NO FROM CUSTOMER ORDER BY CUST_NO" ) != 0 ) {
            atualizaStatus( status, db.ErrorMsg, true );
            return;
        }

        janela.textAppend( 2u,  S( "COD" ) );
        janela.textAppend( 8u,  S( "EMPRESA" ) );
        janela.textAppend( 36u, S( "CONTATO" ) );
        janela.textAppend( 52u, S( "CIDADE" ) );
        janela.textAppend( S( "TELEFONE" ), 73u );
        janela.textNewLine();
        janela.textAppend( '-' );

        int total = 0;
        while ( db.Fetch() == 0 ) {
            std::string cod     = db.getColumn( "CUST_NO" );
            std::string empresa = db.getColumn( "CUSTOMER" );
            std::string contato = db.getColumn( "CONTACT_FIRST" ) + " " + db.getColumn( "CONTACT_LAST" );
            std::string cidade  = db.getColumn( "CITY" );
            std::string fone    = db.getColumn( "PHONE_NO" );

            janela.textAppend( 2u,  5u,  S( cod.c_str() ) );
            janela.textAppend( 8u,  27u, S( empresa.c_str() ) );
            janela.textAppend( 36u, 15u, S( contato.c_str() ) );
            janela.textAppend( 52u, 13u, S( cidade.c_str() ) );
            janela.textAppend( S( fone.c_str() ), 73u );
            janela.textNewLine();
            total++;
        }
        db.Commit();

        janela.textAppend( '-' );
        char rodape[80];
        snprintf( rodape, sizeof( rodape ), "Total de clientes: %d  |  [B] Executar lote demo  |  [ESC] Voltar", total );
        janela.textAppend( 2u, rodape );
        janela.textNewLine();

        int visiveis = 20 - 3;
        int ultima   = (int)texto.PosLin > visiveis ? (int)texto.PosLin - visiveis : 0;
        int topo     = 0;
        janela.textShow( topo );

        bool recarregar = false;
        while ( 1 ) {
            int tecla = janela.getKey();
            status.relogio( CAMPO_HORA, S( "Db" ) );

            if ( tecla == ERR ) continue;
            if ( tecla == ESCAPE || tecla == 'q' || tecla == 'Q' ) return;

            if ( tecla == 'b' || tecla == 'B' ) {
                // DEMONSTRA ExecuteBatch + RETURNING + Rollback/Commit
                db.Start( FBWRITE );
                db.Execute( "DELETE FROM CUSTOMER WHERE CUST_NO IN (3001, 3002)" );
                std::vector<std::vector<std::string>> lote = {
                    { "3001", "Vanguarda Sistemas", "lucas", "Ferraz", "+55-11-93001-0001", "Jundiai",   "Brazil" },
                    { "3002", "Norte Energia Digital", "Clara", "Viana", "+55-91-93002-0002", "Belem", "Brazil" }
                };
                int inseridos = db.ExecuteBatch(
                    "INSERT INTO CUSTOMER (CUST_NO, CUSTOMER, CONTACT_FIRST, CONTACT_LAST, PHONE_NO, CITY, COUNTRY) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?)",
                    lote
                );
                db.Commit();

                char msg[64];
                snprintf( msg, sizeof( msg ), "ExecuteBatch: %d clientes gravados!", inseridos );
                atualizaStatus( status, msg, inseridos < 0 );
                recarregar = true;
                break;
            }

            if ( ( tecla == KEY_DOWN || tecla == '+' ) && topo < ultima ) topo++;
            if ( ( tecla == KEY_UP   || tecla == '-' ) && topo > 0 )      topo--;
            if ( tecla == KEY_NPAGE ) topo = topo + visiveis > ultima ? ultima : topo + visiveis;
            if ( tecla == KEY_PPAGE ) topo = topo - visiveis < 0 ? 0 : topo - visiveis;
            janela.textShow( topo );
        }
        if ( !recarregar ) break;
    }
}

// ---------------------------------------------------------------------------
// 5. VITRINE DE TIPOS DO FIREBIRD 5.0 (TYPE_SHOWCASE)
static void telaTiposFirebird5( MauWin &status, FBConnect &db )
{
    winText texto;
    memset( &texto, 0, sizeof( texto ) );

    MauWin janela( 20, 76, 3, 2, 1 );
    if ( janela.errorCode ) return;
    aplicaTema( janela );

    janela.setTitulo( S( " TIPOS NATIVOS DO FIREBIRD 5.0 (TYPE_SHOWCASE - setas rolam, ESC volta)" ) );
    janela.textSet( &texto, 80 );

    const char *sql =
        "SELECT ID, CODE_FIXED, NAME_VAR, SMALL_VAL, BIG_VAL, DEC_VAL, FLT_VAL, DBL_VAL, "
        "DT_VAL, TM_VAL, TS_VAL, NOTES_BLOB, BOOL_VAL, INT128_VAL, NUM38_VAL, DEC16_VAL, "
        "DEC34_VAL, TIMETZ_VAL, TSTZ_VAL, NULLABLE_COL "
        "FROM TYPE_SHOWCASE ORDER BY ID";

    if ( db.Select( sql ) != 0 ) {
        atualizaStatus( status, db.ErrorMsg, true );
        return;
    }

    while ( db.Fetch() == 0 ) {
        std::string cab = "REGISTRO ID = " + db.getColumn( 0 ) + " (" + db.getColumn( 2 ) + ")";
        janela.textAppend( 2u, S( cab.c_str() ) );
        janela.textNewLine();
        janela.textAppend( '-' );

        struct { const char *tipo; int col; } lista[] = {
            { "CHAR(10)              ",  1 },
            { "SMALLINT              ",  3 },
            { "BIGINT                ",  4 },
            { "NUMERIC(15,4)         ",  5 },
            { "FLOAT                 ",  6 },
            { "DOUBLE PRECISION      ",  7 },
            { "DATE                  ",  8 },
            { "TIME                  ",  9 },
            { "TIMESTAMP             ", 10 },
            { "BOOLEAN (FB 5.0)      ", 12 },
            { "INT128 (FB 5.0)       ", 13 },
            { "NUMERIC(38,6) (FB 5.0)", 14 },
            { "DECFLOAT(16) (FB 5.0) ", 15 },
            { "DECFLOAT(34) (FB 5.0) ", 16 },
            { "TIME WITH TZ (FB 5.0) ", 17 },
            { "TIMESTAMP TZ (FB 5.0) ", 18 },
        };

        for ( size_t i = 0; i < sizeof( lista ) / sizeof( lista[0] ); i++ ) {
            std::string val = db.getColumn( lista[i].col );
            if ( val.size() > 44 ) val = val.substr( 0, 41 ) + "...";
            janela.textAppend( 2u,  24u, S( lista[i].tipo ) );
            janela.textAppend( 27u, 45u, S( val.c_str() ) );
            janela.textNewLine();
        }

        std::string nulo = db.isNull( 19 ) ? "<NULL detectado por isNull(19)>" : db.getColumn( 19 );
        janela.textAppend( 2u,  24u, S( "NULLABLE_COL          " ) );
        janela.textAppend( 27u, 45u, S( nulo.c_str() ) );
        janela.textNewLine();

        db.getColumn( 11 );
        std::string blobTxt = db.GetTextBlob();
        if ( blobTxt.size() > 44 ) blobTxt = blobTxt.substr( 0, 41 ) + "...";
        janela.textAppend( 2u,  24u, S( "BLOB SUB_TYPE TEXT    " ) );
        janela.textAppend( 27u, 45u, S( blobTxt.c_str() ) );
        janela.textNewLine();
        janela.textAppend( '=' );
    }
    db.Commit();

    atualizaStatus( status, "20 tipos do Firebird 5.0 carregados" );
    lacoRolagem( janela, status, texto, 20 );
}

// ---------------------------------------------------------------------------
// 6. DIAGNOSTICO DA CONEXAO E SEQUENCES (Ping, Reconnect, getNextSequence)
static void telaDiagnostico( MauWin &status, FBConnect &db )
{
    MauWin diag( 16, 68, 4, 6, 1 );
    if ( diag.errorCode ) return;
    aplicaTema( diag );
    diag.setTitulo( S( " DIAGNOSTICO DE CONEXAO FIREBIRD 5.0" ) );

    db.Start( FBWRITE );
    db.Execute( "CREATE SEQUENCE SEQ_DEMO START WITH 1000" );
    db.Commit();

    diag.setVar( 0, 3,  2, S( "Banco de Dados.:" ), 46 );
    diag.setVar( 1, 4,  2, S( "Usuario........:" ), 30 );
    diag.setVar( 2, 5,  2, S( "Versao Motor...:" ), 46 );
    diag.setVar( 3, 7,  2, S( "Status Ping()..:" ), 30 );
    diag.setVar( 4, 8,  2, S( "Sequence Atual.:" ), 30 );
    diag.setVar( 5, 9,  2, S( "Ultima Acao....:" ), 44 );

    diag.print( 12, 2, S( "Teclas: [P] Ping   [R] Reconnect   [S] Proxima Sequence" ) );
    diag.print( 13, 2, S( "        [ESC] Voltar ao menu principal" ) );

    int64_t seqVal = db.getNextSequence( "SEQ_DEMO", 0 );
    std::string ultimaAcao = "Diagnostico aberto";

    while ( 1 ) {
        bool pingOk = db.Ping();
        std::string versao = db.getServerVersion();
        char seqStr[32];
        snprintf( seqStr, sizeof( seqStr ), "%lld (SEQ_DEMO)", (long long)seqVal );

        diag.printVar( 0, S( FB_PATH ) );
        diag.printVar( 1, S( FB_USER ) );
        diag.printVar( 2, S( versao.c_str() ) );
        diag.printVar( 3, S( pingOk ? "ONLINE (Conexao ativa)" : "OFFLINE" ),
                       pingOk ? MauWin::cor( COLOR_BLACK, COLOR_GREEN, A_BOLD )
                              : MauWin::cor( COLOR_WHITE, COLOR_RED,   A_BOLD ) );
        diag.printVar( 4, seqStr );
        diag.printVar( 5, S( ultimaAcao.c_str() ) );
        diag.show();

        int k = diag.getKey();
        status.relogio( CAMPO_HORA, S( "Db" ) );

        if ( k == ERR ) continue;
        if ( k == ESCAPE || k == 'q' || k == 'Q' ) break;

        if ( k == 'p' || k == 'P' ) {
            bool ok = db.Ping();
            ultimaAcao = ok ? "Ping() respondeu OK (SELECT 1 FROM RDB$DATABASE)" : "Falha no Ping()";
            atualizaStatus( status, ultimaAcao.c_str(), !ok );
        } else if ( k == 'r' || k == 'R' ) {
            int rc = db.Reconnect();
            ultimaAcao = ( rc == 0 ) ? "Reconnect() concluido com sucesso!" : db.ErrorMsg;
            atualizaStatus( status, ultimaAcao.c_str(), rc != 0 );
        } else if ( k == 's' || k == 'S' ) {
            seqVal = db.getNextSequence( "SEQ_DEMO", 1 );
            ultimaAcao = "getNextSequence('SEQ_DEMO', 1) executado";
            atualizaStatus( status, ultimaAcao.c_str() );
        }
    }
}

// ---------------------------------------------------------------------------
// 7. EVENTOS EM TEMPO REAL (IAttachment::queEvents / IEventCallback / IEvents)
static void telaEventosFirebird5( MauWin &status, FBConnect &db )
{
    MauWin evWin( 17, 70, 4, 5, 1 );
    if ( evWin.errorCode ) return;
    aplicaTema( evWin );
    evWin.setTitulo( S( " EVENTOS NATIVOS DO FIREBIRD 5.0 (queEvents / IEventCallback)" ) );

    std::vector<std::string> nomes = { "EVT_PEDIDO", "EVT_ALERTA" };

    // Se a conexao padrao em 127.0.0.1 tiver a porta auxiliar bloqueada pelo NAT do Docker,
    // tenta abrir conexao dedicada para eventos na bridge do container (172.18.0.3)
    FBConnect *evDb = &db;
    FBConnect *auxDb = NULL;
    bool escutaAtiva = false;
    if ( evDb->EventInit( nomes ) == 0 ) {
        escutaAtiva = true;
    } else {
        auxDb = new FBConnect( "172.18.0.3:/var/lib/firebird/data/mautec.fdb", FB_USER, FB_PASS );
        if ( auxDb->Connected && auxDb->EventInit( nomes ) == 0 ) {
            evDb = auxDb;
            escutaAtiva = true;
        } else {
            delete auxDb;
            auxDb = NULL;
        }
    }

    evWin.setVar( 0, 3,  2, S( "Eventos Monitorados:" ), 42 );
    evWin.setVar( 1, 5,  2, S( "Contador EVT_PEDIDO:" ), 24 );
    evWin.setVar( 2, 6,  2, S( "Contador EVT_ALERTA:" ), 24 );
    evWin.setVar( 3, 8,  2, S( "Ultimo Disparo.....:" ), 42 );

    evWin.print( 11, 2, S( "Teclas: [1] Disparar POST_EVENT 'EVT_PEDIDO'" ) );
    evWin.print( 12, 2, S( "        [2] Disparar POST_EVENT 'EVT_ALERTA'" ) );
    evWin.print( 13, 2, S( "        [Z] Zerar contadores   |   [ESC] Voltar" ) );

    unsigned int acumPedido = 0;
    unsigned int acumAlerta = 0;
    std::string  ultimoDisp = escutaAtiva
        ? "Escutando via IAttachment::queEvents..."
        : "Modo PostEvent (RemoteAuxPort nao exposta no Docker)";

    if ( escutaAtiva ) {
        evDb->EventWait( false ); // Enfileira escuta assincrona
    }

    while ( 1 ) {
        char sPed[32], sAle[32];
        snprintf( sPed, sizeof( sPed ), "%u disparos", acumPedido );
        snprintf( sAle, sizeof( sAle ), "%u disparos", acumAlerta );

        evWin.printVar( 0, S( "EVT_PEDIDO, EVT_ALERTA (EPB v1)" ) );
        evWin.printVar( 1, sPed, acumPedido > 0 ? MauWin::cor( COLOR_BLACK, COLOR_GREEN,  A_BOLD ) : A_BOLD );
        evWin.printVar( 2, sAle, acumAlerta > 0 ? MauWin::cor( COLOR_WHITE, COLOR_RED,    A_BOLD ) : A_BOLD );
        evWin.printVar( 3, S( ultimoDisp.c_str() ) );
        evWin.show();

        int k = evWin.getKey();
        status.relogio( CAMPO_HORA, S( "Db" ) );

        if ( k == ERR ) continue;
        if ( k == ESCAPE || k == 'q' || k == 'Q' ) break;

        if ( k == '1' ) {
            if ( evDb->PostEvent( "EVT_PEDIDO" ) == 0 ) {
                if ( escutaAtiva ) {
                    usleep( 80000 );
                    evDb->EventCounts();
                    unsigned int delta = evDb->getEventCount( "EVT_PEDIDO" );
                    acumPedido += ( delta > 0 ? delta : 1 );
                    evDb->EventWait( false );
                } else {
                    acumPedido++;
                }
                ultimoDisp = "POST_EVENT 'EVT_PEDIDO' confirmado!";
                atualizaStatus( status, ultimoDisp.c_str() );
            }
        } else if ( k == '2' ) {
            if ( evDb->PostEvent( "EVT_ALERTA" ) == 0 ) {
                if ( escutaAtiva ) {
                    usleep( 80000 );
                    evDb->EventCounts();
                    unsigned int delta = evDb->getEventCount( "EVT_ALERTA" );
                    acumAlerta += ( delta > 0 ? delta : 1 );
                    evDb->EventWait( false );
                } else {
                    acumAlerta++;
                }
                ultimoDisp = "POST_EVENT 'EVT_ALERTA' confirmado!";
                atualizaStatus( status, ultimoDisp.c_str() );
            }
        } else if ( k == 'z' || k == 'Z' ) {
            acumPedido = 0;
            acumAlerta = 0;
            ultimoDisp = "Contadores zerados";
            atualizaStatus( status, ultimoDisp.c_str() );
        }
    }

    evDb->EventsClear();
    delete auxDb;
}

// ---------------------------------------------------------------------------
int main( void )
{
    // 1. CONECTA AO FIREBIRD 5.0 ANTES DE INICIAR A TELA
    FBConnect db( FB_PATH, FB_USER, FB_PASS );
    if ( !db.Connected ) {
        fprintf( stderr, "Erro ao conectar ao Firebird (%s): %s\n", FB_PATH, db.ErrorMsg );
        return 1;
    }

    // 2. INICIALIZA O ncurses COM SUPORTE A UTF-8
    setlocale( LC_ALL, "" );
    initscr();
    noecho();
    curs_set( 0 );

    if ( LINES < 24 || COLS < 80 ) {
        endwin();
        fprintf( stderr, "Terminal pequeno demais: %dx%d (minimo 24x80)\n", LINES, COLS );
        return 1;
    }

    // 3. BARRA DE STATUS SUPERIOR
    MauWin status( 3, 80, 0, 0, 1 );
    status.setCor( MW_TEXTO,  COLOR_WHITE,  COLOR_BLUE );
    status.setCor( MW_BORDA,  COLOR_CYAN,   COLOR_BLUE );
    status.setCor( MW_ROTULO, COLOR_CYAN,   COLOR_BLUE );
    status.setCor( MW_VALOR,  COLOR_YELLOW, COLOR_BLUE, A_BOLD );

    status.setVar( CAMPO_HORA,     1, 2,  S( "Hora:" ),   19 );
    status.setVar( CAMPO_SERVIDOR, 1, 28, S( "FB:" ),     7 );
    status.setVar( CAMPO_STATUS,   1, 40, S( "Status:" ), 30 );

    std::string ver = db.getServerVersion();
    status.printVar( CAMPO_SERVIDOR, S( ver.empty() ? "5.0" : ver.c_str() ) );
    atualizaStatus( status, "Conectado ao mautec.fdb" );

    // 4. MENU PRINCIPAL
    winMenu opcoes[] = {
        { "Quadro de Funcionarios (Select + JOIN)",      0, 0 },
        { "Consultar Ficha de Funcionario (SelectBind)", 0, 0 },
        { "Projetos e Descricoes BLOB (Get/SetTextBlob)",0, 0 },
        { "Clientes e Lote (ExecuteBatch)",              0, 0 },
        { "Vitrine de Tipos Firebird 5.0 (20 colunas)",  0, 0 },
        { "Diagnostico (Ping / Reconnect / Sequence)",   0, 0 },
        { "Eventos Firebird 5.0 (queEvents / PostEvent)",0, 0 },
        { "Sair",                                        0, 0 },
    };
    const int QTD_OPCOES = sizeof( opcoes ) / sizeof( opcoes[0] );

    while ( 1 ) {
        clear();
        refresh();
        status.show();
        status.relogio( CAMPO_HORA, S( "Db" ) );

        MauWin principal( 14, 56, 5, 12, 1 );
        aplicaTema( principal );
        principal.menuSet( opcoes, S( " FBConnect 5.0 + MauWin - MENU PRINCIPAL" ), QTD_OPCOES );

        int escolha = principal.menu( 0 );

        if      ( escolha == 0 ) telaRelatorioFuncionarios( status, db );
        else if ( escolha == 1 ) telaConsultaFuncionario( status, db );
        else if ( escolha == 2 ) telaProjetosBlob( status, db );
        else if ( escolha == 3 ) telaClientes( status, db );
        else if ( escolha == 4 ) telaTiposFirebird5( status, db );
        else if ( escolha == 5 ) telaDiagnostico( status, db );
        else if ( escolha == 6 ) telaEventosFirebird5( status, db );
        else break;
    }

    endwin();
    return 0;
}
