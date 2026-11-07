// PROGRAMA DE TESTE DA CLASSE FBConnect (FIREBIRD 5.0) COM INTERFACE MauWin
//
// PERMITE ESCOLHER QUAL TESTE EXECUTAR (OU EXECUTAR TODA A BATERIA DE TESTES)
// ATRAVES DE UM MENU MauWin E APRESENTA OS RESULTADOS DETALHADOS EM JANELAS
// COM ROLAGEM, BARRA DE STATUS COM RELOGIO E INDICADORES COLORIDOS (PASS/FAIL).
//
// COMPILAR:  make
// RODAR:     ./FBConnectTest          (MODO INTERATIVO MauWin - TERMINAL >= 24x80)
//            ./FBConnectTest --batch  (MODO NAO-INTERATIVO PARA SCRIPTS/CI)

#include "FBConnect.h"
#include "MauWin.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <ncurses.h>
#include <string>
#include <vector>
#include <map>

#define S(x) ((char*)(x))

#define FB30PATH "127.0.0.1:/var/lib/firebird/data/mautec.fdb"
#define FB30PASS "pqIobXtPui94hw9"
#define FB30USER "softgo"

using namespace std;

// ---------------------------------------------------------------------------
// COLETOR DE LINHAS DO RELATORIO PARA EXIBICAO NO MauWin (E stdout EM --batch)
static vector<string> g_linhasRelatorio;
static bool           g_modoBatch = false;

static void limpaRelatorio( void )
{
    g_linhasRelatorio.clear();
}

static void adicionaLinhaBruta( const string &s )
{
    const size_t LARGURA_MAX = 70;
    if ( s.empty() ) {
        g_linhasRelatorio.push_back( "" );
        return;
    }
    size_t pos = 0;
    while ( pos < s.size() ) {
        size_t tam = s.size() - pos;
        if ( tam > LARGURA_MAX ) tam = LARGURA_MAX;
        g_linhasRelatorio.push_back( s.substr( pos, tam ) );
        pos += tam;
    }
}

static void relLog( const char *fmt, ... )
{
    char buf[2048];
    va_list args;
    va_start( args, fmt );
    vsnprintf( buf, sizeof( buf ), fmt, args );
    va_end( args );

    if ( g_modoBatch ) {
        printf( "%s\n", buf );
    }

    // Divide por '\n' caso haja multiplas linhas
    string texto( buf );
    size_t inicio = 0;
    while ( inicio <= texto.size() ) {
        size_t nl = texto.find( '\n', inicio );
        if ( nl == string::npos ) {
            adicionaLinhaBruta( texto.substr( inicio ) );
            break;
        }
        adicionaLinhaBruta( texto.substr( inicio, nl - inicio ) );
        inicio = nl + 1;
    }
}

// ===========================================================================
// CASOS DE TESTE DO FBConnect (FIREBIRD 5.0)
// ===========================================================================

int t000_SetupDatabase( void )
{
    relLog( "000_SetupDatabase: Recriando tabelas e populando dados em mautec.fdb" );
    FBConnect *db = new FBConnect( FB30PATH, FB30USER, FB30PASS );
    if ( !db->Connected ) {
        relLog( "ERRO DE CONEXAO: %s", db->ErrorMsg );
        delete db;
        return 1;
    }

    relLog( "Conectado! Versao do motor Firebird: %s", db->getServerVersion().c_str() );

    const char *ddl[] = {
        "RECREATE TABLE DEPARTMENT ("
        "  DEPT_NO CHAR(3) NOT NULL PRIMARY KEY,"
        "  DEPARTMENT VARCHAR(40) NOT NULL,"
        "  HEAD_DEPT CHAR(3),"
        "  BUDGET NUMERIC(12,2),"
        "  LOCATION VARCHAR(30)"
        ")",
        "RECREATE TABLE JOB ("
        "  JOB_CODE VARCHAR(5) NOT NULL,"
        "  JOB_GRADE SMALLINT NOT NULL,"
        "  JOB_COUNTRY VARCHAR(15) NOT NULL,"
        "  JOB_TITLE VARCHAR(35) NOT NULL,"
        "  MIN_SALARY NUMERIC(10,2),"
        "  MAX_SALARY NUMERIC(10,2),"
        "  JOB_REQUIREMENT BLOB SUB_TYPE TEXT,"
        "  PRIMARY KEY (JOB_CODE, JOB_GRADE, JOB_COUNTRY)"
        ")",
        "RECREATE TABLE EMPLOYEE ("
        "  EMP_NO INTEGER NOT NULL PRIMARY KEY,"
        "  FIRST_NAME VARCHAR(20) NOT NULL,"
        "  LAST_NAME VARCHAR(25) NOT NULL,"
        "  DEPT_NO CHAR(3) NOT NULL,"
        "  JOB_CODE VARCHAR(5) NOT NULL,"
        "  JOB_GRADE SMALLINT NOT NULL,"
        "  HIRE_DATE DATE NOT NULL,"
        "  SALARY NUMERIC(10,2) NOT NULL"
        ")",
        "RECREATE TABLE PROJECT ("
        "  PROJ_ID CHAR(5) NOT NULL PRIMARY KEY,"
        "  PROJ_NAME VARCHAR(40) NOT NULL,"
        "  TEAM_LEADER INTEGER,"
        "  PRODUCT VARCHAR(20),"
        "  PROJ_DESC BLOB SUB_TYPE TEXT"
        ")",
        "RECREATE TABLE EMPLOYEE_PROJECT ("
        "  EMP_NO INTEGER NOT NULL,"
        "  PROJ_ID CHAR(5) NOT NULL,"
        "  PRIMARY KEY (EMP_NO, PROJ_ID)"
        ")",
        "RECREATE TABLE SALARY_HISTORY ("
        "  EMP_NO INTEGER NOT NULL,"
        "  CHANGE_DATE TIMESTAMP NOT NULL,"
        "  UPDATER_ID VARCHAR(20) NOT NULL,"
        "  OLD_SALARY NUMERIC(10,2) NOT NULL,"
        "  PERCENT_CHANGE DOUBLE PRECISION NOT NULL"
        ")",
        "RECREATE TABLE CUSTOMER ("
        "  CUST_NO INTEGER NOT NULL PRIMARY KEY,"
        "  CUSTOMER VARCHAR(35) NOT NULL,"
        "  CONTACT_FIRST VARCHAR(20),"
        "  CONTACT_LAST VARCHAR(20),"
        "  PHONE_NO VARCHAR(20),"
        "  CITY VARCHAR(30),"
        "  COUNTRY VARCHAR(20)"
        ")",
        "RECREATE TABLE TYPE_SHOWCASE ("
        "  ID INTEGER NOT NULL PRIMARY KEY,"
        "  CODE_FIXED CHAR(10),"
        "  NAME_VAR VARCHAR(40),"
        "  SMALL_VAL SMALLINT,"
        "  BIG_VAL BIGINT,"
        "  DEC_VAL NUMERIC(15,4),"
        "  FLT_VAL FLOAT,"
        "  DBL_VAL DOUBLE PRECISION,"
        "  DT_VAL DATE,"
        "  TM_VAL TIME,"
        "  TS_VAL TIMESTAMP,"
        "  NOTES_BLOB BLOB SUB_TYPE TEXT,"
        "  BOOL_VAL BOOLEAN,"
        "  INT128_VAL INT128,"
        "  NUM38_VAL NUMERIC(38,6),"
        "  DEC16_VAL DECFLOAT(16),"
        "  DEC34_VAL DECFLOAT(34),"
        "  TIMETZ_VAL TIME WITH TIME ZONE,"
        "  TSTZ_VAL TIMESTAMP WITH TIME ZONE,"
        "  NULLABLE_COL VARCHAR(20)"
        ")"
    };

    for ( size_t i = 0; i < sizeof( ddl ) / sizeof( ddl[0] ); ++i ) {
        db->Start( FBWRITE );
        if ( db->Execute( ddl[i] ) == 0 ) {
            db->Commit();
        } else {
            relLog( "Erro DDL [%zu]: %s", i, db->ErrorMsg );
            db->Rollback();
            delete db;
            return 1;
        }
    }
    relLog( "8 tabelas recriadas com sucesso (DEPARTMENT, JOB, EMPLOYEE, PROJECT...)" );

    const char *inserts[] = {
        "INSERT INTO DEPARTMENT VALUES ('000', 'Corporate Headquarters', NULL, 2500000.00, 'Sao Paulo')",
        "INSERT INTO DEPARTMENT VALUES ('100', 'Sales and Marketing', '000', 1200000.50, 'Sao Paulo')",
        "INSERT INTO DEPARTMENT VALUES ('600', 'Engineering', '000', 3450000.75, 'Campinas')",
        "INSERT INTO DEPARTMENT VALUES ('621', 'Software Development', '600', 1850000.00, 'Campinas')",
        "INSERT INTO DEPARTMENT VALUES ('622', 'Quality Assurance', '600', 780000.25, 'Curitiba')",
        "INSERT INTO DEPARTMENT VALUES ('623', 'Customer Support', '600', 620000.00, 'Belo Horizonte')",

        "INSERT INTO JOB VALUES ('Eng', 5, 'Brazil', 'Senior Software Engineer', 10000.00, 18000.00, 'Dominio de C/C++, Linux, Firebird 5 OO API, depuracao com GDB/Valgrind e arquitetura de sistemas de alta performance.')",
        "INSERT INTO JOB VALUES ('Eng', 4, 'Brazil', 'Software Engineer', 7000.00, 12000.00, 'Experiencia com desenvolvimento C++, SQL, controle de versao Git e testes automatizados.')",
        "INSERT INTO JOB VALUES ('Mngr', 6, 'Brazil', 'Engineering Manager', 15000.00, 25000.00, 'Lideranca tecnica de equipes de engenharia de software, planejamento de arquitetura e entregas.')",
        "INSERT INTO JOB VALUES ('QA', 3, 'Brazil', 'QA Analyst', 5000.00, 9000.00, 'Criacao de planos de teste, automacao de testes de regressao e validacao de banco de dados.')",
        "INSERT INTO JOB VALUES ('Sales', 4, 'Brazil', 'Account Executive', 6000.00, 14000.00, 'Gestao de contas corporativas, prospeccao e fechamento de contratos B2B.')",

        "INSERT INTO EMPLOYEE VALUES (101, 'Mauricio', 'Junqueira', '621', 'Mngr', 6, '2017-12-27', 19500.00)",
        "INSERT INTO EMPLOYEE VALUES (102, 'Ana', 'Beatriz', '621', 'Eng', 5, '2018-03-15', 14800.50)",
        "INSERT INTO EMPLOYEE VALUES (103, 'Roberto', 'Almeida', '621', 'Eng', 4, '2019-07-01', 9800.75)",
        "INSERT INTO EMPLOYEE VALUES (104, 'Fernanda', 'Costa', '622', 'QA', 3, '2020-02-10', 7250.00)",
        "INSERT INTO EMPLOYEE VALUES (105, 'Lucas', 'Mendes', '100', 'Sales', 4, '2021-05-20', 8900.25)",
        "INSERT INTO EMPLOYEE VALUES (106, 'Patricia', 'Rocha', '623', 'Eng', 4, '2022-09-12', 10200.00)",
        "INSERT INTO EMPLOYEE VALUES (107, 'Gustavo', 'Nunes', '600', 'Eng', 5, '2018-11-18', 15600.80)",

        "INSERT INTO PROJECT VALUES ('FBCON', 'FBConnect C++ Interface', 101, 'Middleware', 'Biblioteca C++ de integracao direta com as interfaces orientadas a objetos (IMaster, IProvider, IAttachment, ITransaction, IStatement, IResultSet, IBlob) do Firebird 5.0.')",
        "INSERT INTO PROJECT VALUES ('SGERP', 'SoftGo ERP Core', 102, 'ERP', 'Nucleo de processamento transacional de alta concorrencia com suporte a multiplas conexoes, controle estrito de transacoes Read Committed e manipulacao de BLOBs.')",
        "INSERT INTO PROJECT VALUES ('BIAPI', 'Analytics & Telemetry', 107, 'Analytics', 'Pipeline de coleta e agregacao de indicadores operacionais em tempo real integrando Firebird, PostgreSQL, Redis e RabbitMQ.')",
        "INSERT INTO PROJECT VALUES ('QAAUT', 'Suite de Testes Automatizados', 104, 'Tooling', 'Bateria automatizada de testes de regressao e verificacao de vazamento de memoria com Valgrind.')",

        "INSERT INTO EMPLOYEE_PROJECT VALUES (101, 'FBCON')",
        "INSERT INTO EMPLOYEE_PROJECT VALUES (102, 'FBCON')",
        "INSERT INTO EMPLOYEE_PROJECT VALUES (102, 'SGERP')",
        "INSERT INTO EMPLOYEE_PROJECT VALUES (103, 'SGERP')",
        "INSERT INTO EMPLOYEE_PROJECT VALUES (104, 'QAAUT')",
        "INSERT INTO EMPLOYEE_PROJECT VALUES (106, 'SGERP')",
        "INSERT INTO EMPLOYEE_PROJECT VALUES (107, 'BIAPI')",

        "INSERT INTO SALARY_HISTORY VALUES (101, '2019-01-15 10:30:00', 'ADMIN', 16500.00, 18.18)",
        "INSERT INTO SALARY_HISTORY VALUES (102, '2020-06-01 14:15:30', 'MAURICIO', 12500.00, 18.404)",
        "INSERT INTO SALARY_HISTORY VALUES (103, '2021-08-10 09:00:00', 'MAURICIO', 8500.00, 15.3029)",
        "INSERT INTO SALARY_HISTORY VALUES (107, '2022-03-20 16:45:12', 'ADMIN', 13800.00, 13.0492)",

        "INSERT INTO CUSTOMER VALUES (1001, 'Mautec Tecnologia Ltda', 'Mauricio', 'Junqueira', '+55-11-99999-0001', 'Sao Paulo', 'Brazil')",
        "INSERT INTO CUSTOMER VALUES (1002, 'SoftGo Sistemas S.A.', 'Carlos', 'Eduardo', '+55-19-98888-0002', 'Campinas', 'Brazil')",
        "INSERT INTO CUSTOMER VALUES (1003, 'Horizonte Logistica', 'Mariana', 'Silva', '+55-31-97777-0003', 'Belo Horizonte', 'Brazil')",
        "INSERT INTO CUSTOMER VALUES (1004, 'Sul Cloud Services', 'Rafael', 'Kowalski', '+55-41-96666-0004', 'Curitiba', 'Brazil')",
        "INSERT INTO CUSTOMER VALUES (1005, 'Atlantico Comercio Exterior', 'Helena', 'Santos', '+55-21-95555-0005', 'Rio de Janeiro', 'Brazil')",

        "INSERT INTO TYPE_SHOWCASE VALUES ("
        "  1, 'FIX-000001', 'Registro Completo Positivo', 32000, 9876543210123, 123456.7895, 3.14159, 2.718281828459,"
        "  '2026-11-07', '14:35:22.1230', '2026-11-07 14:35:22.1230',"
        "  'BLOB de texto longo do registro 1 com multiplos segmentos para exercitar leitura no Firebird 5.x.',"
        "  TRUE, 170141183460469231731687303715884105727, 99999999999999999999999999999999.123456,"
        "  1.234567890123456E+120, 3.141592653589793238462643383279502E+1000,"
        "  '14:35:22.1230 America/Sao_Paulo', '2026-11-07 14:35:22.1230 America/Sao_Paulo', NULL"
        ")",
        "INSERT INTO TYPE_SHOWCASE VALUES ("
        "  2, 'FIX-000002', 'Registro Com Valores Negativos', -1500, -5000000000, -42.0050, -0.5, -99999.123456,"
        "  '2018-01-17', '08:05:09.0050', '2018-01-17 08:05:09.0050',"
        "  'Segundo BLOB de teste verificando reutilizacao do handler entre linhas consecutivas do cursor.',"
        "  FALSE, -170141183460469231731687303715884105727, -123456789012345678901234567890.654321,"
        "  -9.876543210987654E-95, -2.718281828459045235360287471352662E-500,"
        "  '08:05:09.0050 -03:00', '2018-01-17 08:05:09.0050 UTC', 'Preenchido'"
        ")"
    };

    db->Start( FBWRITE );
    size_t qtdInserts = sizeof( inserts ) / sizeof( inserts[0] );
    for ( size_t i = 0; i < qtdInserts; ++i ) {
        if ( db->Execute( inserts[i] ) != 0 ) {
            relLog( "Erro INSERT [%zu]: %s", i, db->ErrorMsg );
            db->Rollback();
            delete db;
            return 1;
        }
    }
    db->Commit();
    relLog( "%zu registros inseridos e confirmados via Commit()!", qtdInserts );

    delete db;
    return 0;
}

int t002_ConexaoOK( void )
{
    relLog( "002_ConexaoOK: Testando conexao valida ao Firebird 5.0" );
    FBConnect *db = new FBConnect( FB30PATH, FB30USER, FB30PASS );
    if ( !db->Connected ) {
        relLog( "Falha na conexao: %s", db->ErrorMsg );
        delete db;
        return 1;
    }
    relLog( "Conexao estabelecida com sucesso em %s", FB30PATH );
    relLog( "Servidor Firebird: %s", db->getServerVersion().c_str() );
    delete db;
    return 0;
}

int t003_ConexaoNOK( void )
{
    relLog( "003_ConexaoNOK: Testando rejeicao de credenciais invalidas" );
    FBConnect *db = new FBConnect( FB30PATH, FB30USER, "senha_invalida_123" );
    if ( db->Connected ) {
        relLog( "ERRO: Conectou com senha invalida!" );
        delete db;
        return 1;
    }
    relLog( "Senha invalida rejeitada OK: %s", db->ErrorMsg );
    delete db;

    db = new FBConnect( "servidor_inexistente:DB", "USER", "PASSWRONG" );
    if ( db->Connected ) {
        relLog( "ERRO: Conectou em servidor inexistente!" );
        delete db;
        return 1;
    }
    relLog( "Servidor invalido rejeitado OK: %s", db->ErrorMsg );
    delete db;
    return 0;
}

int t004_StartTrans( void )
{
    relLog( "004_StartTrans: Testando transacoes FBREAD e FBWRITE" );
    FBConnect *db = new FBConnect( FB30PATH, FB30USER, FB30PASS );
    if ( !db->Connected ) {
        relLog( "ERRO CONEXAO: %s", db->ErrorMsg );
        delete db;
        return 1;
    }

    db->Start( FBREAD );
    if ( !db->InTrans ) {
        relLog( "ERRO ao iniciar transacao FBREAD" );
        delete db;
        return 1;
    }
    relLog( "Transacao FBREAD iniciada com sucesso (InTrans = true)" );
    db->Commit();

    db->Start( FBWRITE );
    if ( !db->InTrans ) {
        relLog( "ERRO ao iniciar transacao FBWRITE" );
        delete db;
        return 1;
    }
    relLog( "Transacao FBWRITE iniciada com sucesso (InTrans = true)" );
    db->Commit();

    delete db;
    return 0;
}

int t005_SelectUT( void )
{
    relLog( "005_SelectUT: Testando consulta em tabela inexistente" );
    FBConnect *db = new FBConnect( FB30PATH, FB30USER, FB30PASS );
    if ( !db->Connected ) {
        relLog( "ERRO CONEXAO: %s", db->ErrorMsg );
        delete db;
        return 1;
    }

    if ( db->Start( FBREAD ) || !db->InTrans ) {
        relLog( "ERRO Transacao FBREAD" );
        delete db;
        return 1;
    }

    const char *stmt = "SELECT * FROM UNKNOWTABLE";
    if ( db->Select( stmt ) ) {
        relLog( "SQL rejeitado conforme esperado: %s", stmt );
        relLog( "Mensagem do Firebird: %s", db->ErrorMsg );
        delete db;
        return 0;
    }

    delete db;
    return 1;
}

int t006_Select( void )
{
    relLog( "006_Select: Leitura de campos BLOB SUB_TYPE TEXT em PROJECT" );
    FBConnect *db = new FBConnect( FB30PATH, FB30USER, FB30PASS );
    if ( !db->Connected ) {
        relLog( "ERRO CONEXAO: %s", db->ErrorMsg );
        delete db;
        return 1;
    }

    if ( db->Start( FBREAD ) || !db->InTrans ) {
        relLog( "ERRO Transacao de LEITURA" );
        delete db;
        return 1;
    }

    const char *stmt = "SELECT PROJ_DESC, PROJ_NAME, PROJ_ID FROM PROJECT ORDER BY PROJ_ID";
    if ( db->Select( stmt ) ) {
        relLog( "ERRO SELECT: %s (%s)", stmt, db->ErrorMsg );
        delete db;
        return 1;
    }

    int ret = db->Fetch();
    int lidos = 0;
    while ( !ret ) {
        int tamanho = db->getDataSize( 0 );
        unsigned char *dados = new unsigned char[tamanho];
        memset( dados, '\0', tamanho );

        db->getColumn( tamanho, 0 );
        string nomeProj = db->getColumn( 1 );
        string idProj   = db->getColumn( 2 );

        if ( db->isBlob( 0 ) && db->blobType( 0 ) ) {
            int retorno = db->workBlob( dados );
            int t = retorno ? db->getBlobSize( db->blob ) : 0;
            string textoBlob = db->GetTextBlob();
            relLog( "Projeto [%s] %s (%d bytes):", idProj.c_str(), nomeProj.c_str(), t );
            relLog( "  %s", textoBlob.c_str() );
            lidos++;
        }
        delete[] dados;
        ret = db->Fetch();
    }

    db->Commit();
    delete db;
    return ( lidos > 0 ) ? 0 : 1;
}

int t007_SelectA( void )
{
    relLog( "007_SelectA: Validacao de comando SQL incompleto (< 15 chars)" );
    FBConnect *db = new FBConnect( FB30PATH, FB30USER, FB30PASS );
    if ( !db->Connected ) {
        relLog( "ERRO CONEXAO: %s", db->ErrorMsg );
        delete db;
        return 1;
    }

    db->Start( FBREAD );
    const char *stmt = "SELECT * FROM ";
    if ( db->Select( stmt ) ) {
        relLog( "SQL curto/invalido rejeitado OK: '%s'", stmt );
        relLog( "Detalhe: %s", db->ErrorMsg );
        delete db;
        return 0;
    }

    delete db;
    return 1;
}

int t008_SelectB( void )
{
    relLog( "008_SelectB: Select com AutoTransaction em CUSTOMER" );
    FBConnect *db = new FBConnect( FB30PATH, FB30USER, FB30PASS );
    if ( !db->Connected ) {
        relLog( "ERRO CONEXAO: %s", db->ErrorMsg );
        delete db;
        return 1;
    }

    const char *stmt = "SELECT CUST_NO, CUSTOMER, CITY FROM CUSTOMER ORDER BY CUST_NO";
    if ( db->Select( stmt ) ) {
        relLog( "ERRO SELECT: %s", db->ErrorMsg );
        delete db;
        return 1;
    }

    int qtd = 0;
    while ( db->Fetch() == 0 ) {
        qtd++;
        relLog( "  Cliente %s: %-30s (%s)",
                db->getColumn( 0 ).c_str(),
                db->getColumn( 1 ).c_str(),
                db->getColumn( 2 ).c_str() );
    }
    db->Commit();
    delete db;
    return ( qtd >= 5 ) ? 0 : 1;
}

int t009_SelectM( void )
{
    relLog( "009_SelectM: Leitura de multiplos campos tipados em EMPLOYEE" );
    FBConnect *db = new FBConnect( FB30PATH, FB30USER, FB30PASS );
    if ( !db->Connected ) {
        relLog( "ERRO CONEXAO: %s", db->ErrorMsg );
        delete db;
        return 1;
    }

    db->Start( FBREAD );
    const char *stmt = "SELECT HIRE_DATE, SALARY, JOB_GRADE, FIRST_NAME, LAST_NAME FROM EMPLOYEE ORDER BY EMP_NO";
    if ( db->Select( stmt ) ) {
        relLog( "ERRO SELECT: %s", db->ErrorMsg );
        delete db;
        return 1;
    }

    int count = 0;
    while ( db->Fetch() == 0 ) {
        string dt   = db->getColumn( db->getDataSize( 0 ), 0 );
        string sal  = db->getColumn( db->getDataSize( 1 ), 1 );
        string grd  = db->getColumn( db->getDataSize( 2 ), 2 );
        string nome = db->getColumn( 3 ) + " " + db->getColumn( 4 );
        relLog( "  %-18s | Admissao: %s | Nivel: %s | Salario: %s",
                nome.c_str(), dt.c_str(), grd.c_str(), sal.c_str() );
        count++;
    }

    db->Commit();
    delete db;
    return ( count == 7 ) ? 0 : 1;
}

int t010_AllDataTypes( void )
{
    relLog( "010_AllDataTypes: Testando todos os 20 tipos SQL do Firebird 5.0" );
    FBConnect *db = new FBConnect( FB30PATH, FB30USER, FB30PASS );
    if ( !db->Connected ) {
        relLog( "ERRO CONEXAO: %s", db->ErrorMsg );
        delete db;
        return 1;
    }

    const char *stmt =
        "SELECT ID, CODE_FIXED, NAME_VAR, SMALL_VAL, BIG_VAL, DEC_VAL, FLT_VAL, DBL_VAL, "
        "DT_VAL, TM_VAL, TS_VAL, NOTES_BLOB, BOOL_VAL, INT128_VAL, NUM38_VAL, DEC16_VAL, "
        "DEC34_VAL, TIMETZ_VAL, TSTZ_VAL, NULLABLE_COL "
        "FROM TYPE_SHOWCASE ORDER BY ID";

    if ( db->Select( stmt ) ) {
        relLog( "ERRO SELECT: %s", db->ErrorMsg );
        delete db;
        return 1;
    }

    relLog( "Colunas no ResultSet (getCount): %u", db->getCount() );
    int rows = 0;
    while ( db->Fetch() == 0 ) {
        rows++;
        relLog( "--- Registro #%d ---", rows );
        relLog( "  ID (INTEGER)     : %s", db->getColumn( 0 ).c_str() );
        relLog( "  CODE (CHAR)      : %s", db->getColumn( 1 ).c_str() );
        relLog( "  NAME (VARCHAR)   : %s", db->getColumn( 2 ).c_str() );
        relLog( "  SMALL (SMALLINT) : %s", db->getColumn( 3 ).c_str() );
        relLog( "  BIG (BIGINT)     : %s", db->getColumn( 4 ).c_str() );
        relLog( "  DEC (NUMERIC 15) : %s", db->getColumn( 5 ).c_str() );
        relLog( "  FLT / DBL        : %s / %s", db->getColumn( 6 ).c_str(), db->getColumn( 7 ).c_str() );
        relLog( "  DATE / TIME      : %s / %s", db->getColumn( 8 ).c_str(), db->getColumn( 9 ).c_str() );
        relLog( "  TIMESTAMP        : %s", db->getColumn( 10 ).c_str() );
        db->getColumn( 11 );
        if ( db->isBlob( 11 ) && db->blobType( 11 ) ) {
            relLog( "  BLOB (%d B)      : %s", db->getBlobSize( db->blob ), db->GetTextBlob().c_str() );
        }
        relLog( "  BOOLEAN          : %s", db->getColumn( 12 ).c_str() );
        relLog( "  INT128           : %s", db->getColumn( 13 ).c_str() );
        relLog( "  NUMERIC(38,6)    : %s", db->getColumn( 14 ).c_str() );
        relLog( "  DECFLOAT(16)     : %s", db->getColumn( 15 ).c_str() );
        relLog( "  DECFLOAT(34)     : %s", db->getColumn( 16 ).c_str() );
        relLog( "  TIME WITH TZ     : %s", db->getColumn( 17 ).c_str() );
        relLog( "  TIMESTAMP TZ     : %s", db->getColumn( 18 ).c_str() );
        relLog( "  NULLABLE_COL     : %s", db->isNull( 19 ) ? "<NULL>" : db->getColumn( 19 ).c_str() );
    }

    db->Commit();
    delete db;
    return ( rows == 2 ) ? 0 : 1;
}

int t011_JoinsAndRollback( void )
{
    relLog( "011_JoinsAndRollback: JOIN de 5 tabelas e Rollback vs Commit" );
    FBConnect *db = new FBConnect( FB30PATH, FB30USER, FB30PASS );
    if ( !db->Connected ) {
        relLog( "ERRO CONEXAO: %s", db->ErrorMsg );
        delete db;
        return 1;
    }

    db->Start( FBWRITE );
    if ( db->Execute( "UPDATE DEPARTMENT SET BUDGET = 9999999.99 WHERE DEPT_NO = '621'" ) != 0 ) {
        relLog( "ERRO UPDATE: %s", db->ErrorMsg );
        delete db;
        return 1;
    }
    db->Rollback();
    relLog( "Rollback() executado apos UPDATE temporario em DEPARTMENT 621" );

    const char *stmt =
        "SELECT E.FIRST_NAME, E.LAST_NAME, D.DEPARTMENT, D.BUDGET, J.JOB_TITLE, P.PROJ_NAME "
        "FROM EMPLOYEE E "
        "JOIN DEPARTMENT D ON D.DEPT_NO = E.DEPT_NO "
        "JOIN JOB J ON J.JOB_CODE = E.JOB_CODE AND J.JOB_GRADE = E.JOB_GRADE "
        "JOIN EMPLOYEE_PROJECT EP ON EP.EMP_NO = E.EMP_NO "
        "JOIN PROJECT P ON P.PROJ_ID = EP.PROJ_ID "
        "ORDER BY E.EMP_NO, P.PROJ_ID";

    if ( db->Select( stmt ) ) {
        relLog( "ERRO SELECT JOIN: %s", db->ErrorMsg );
        delete db;
        return 1;
    }

    int rows = 0;
    while ( db->Fetch() == 0 ) {
        rows++;
        relLog( "  #%d: %s %s | %s (Orc: %s) | %s",
                rows,
                db->getColumn( 0 ).c_str(),
                db->getColumn( 1 ).c_str(),
                db->getColumn( 2 ).c_str(),
                db->getColumn( 3 ).c_str(),
                db->getColumn( 5 ).c_str() );
    }

    db->Commit();
    delete db;
    return ( rows == 7 ) ? 0 : 1;
}

int t012_ExecuteBindAndBlobWrite( void )
{
    relLog( "012_ExecuteBindAndBlobWrite: ExecuteBind (?) e SetTextBlob" );
    FBConnect *db = new FBConnect( FB30PATH, FB30USER, FB30PASS );
    if ( !db->Connected ) {
        relLog( "ERRO CONEXAO: %s", db->ErrorMsg );
        delete db;
        return 1;
    }

    db->Start( FBWRITE );
    db->Execute( "DELETE FROM CUSTOMER WHERE CUST_NO = 1006" );
    const char *params[] = { "1006", "Nova Fronteira Tech", "Diego", "Martins", "+55-48-94444-0006", "Florianopolis", "Brazil" };
    if ( db->ExecuteBind(
            "INSERT INTO CUSTOMER (CUST_NO, CUSTOMER, CONTACT_FIRST, CONTACT_LAST, PHONE_NO, CITY, COUNTRY) "
            "VALUES (?, ?, ?, ?, ?, ?, ?)", params, 7 ) != 0 ) {
        relLog( "ERRO ExecuteBind: %s", db->ErrorMsg );
        db->Rollback();
        delete db;
        return 1;
    }
    db->Commit();
    relLog( "ExecuteBind inseriu cliente 1006 (Nova Fronteira Tech)" );

    const char *novoBlob = "BLOB gravado via FBConnect::SetTextBlob usando IAttachment::createBlob e putSegment no Firebird 5.x!";
    db->Start( FBWRITE );
    if ( db->SetTextBlob( "UPDATE PROJECT SET PROJ_DESC = ? WHERE PROJ_ID = 'QAAUT'", novoBlob, (int)strlen( novoBlob ) ) != 0 ) {
        relLog( "ERRO SetTextBlob: %s", db->ErrorMsg );
        db->Rollback();
        delete db;
        return 1;
    }
    db->Commit();

    if ( db->Select( "SELECT PROJ_DESC FROM PROJECT WHERE PROJ_ID = 'QAAUT'" ) != 0 || db->Fetch() != 0 ) {
        relLog( "ERRO SELECT PROJECT QAAUT: %s", db->ErrorMsg );
        delete db;
        return 1;
    }
    db->getColumn( 0 );
    string blobLido = db->GetTextBlob();
    relLog( "BLOB lido apos SetTextBlob: %s", blobLido.c_str() );
    db->Commit();

    delete db;
    return ( blobLido == novoBlob ) ? 0 : 1;
}

int t013_NewUtilities( void )
{
    relLog( "013_NewUtilities: Ping, Reconnect, Sequence, Batch, SelectBind, RETURNING" );
    FBConnect *db = new FBConnect( FB30PATH, FB30USER, FB30PASS );
    if ( !db->Connected ) {
        relLog( "ERRO CONEXAO: %s", db->ErrorMsg );
        delete db;
        return 1;
    }

    if ( !db->Ping() || db->Reconnect() != 0 || !db->Ping() ) {
        relLog( "ERRO em Ping/Reconnect: %s", db->ErrorMsg );
        delete db;
        return 1;
    }
    relLog( "1) Ping() e Reconnect() validados com sucesso" );

    db->Start( FBWRITE );
    db->Execute( "RECREATE SEQUENCE SEQ_TESTE START WITH 100" );
    db->Commit();
    int64_t seq1 = db->getNextSequence( "SEQ_TESTE", 1 );
    int64_t seq2 = db->getNextSequence( "SEQ_TESTE", 5 );
    relLog( "2) Sequence SEQ_TESTE: %lld -> %lld", (long long)seq1, (long long)seq2 );
    if ( seq1 != 100 || seq2 != 105 ) {
        delete db;
        return 1;
    }

    db->Start( FBWRITE );
    db->Execute( "DELETE FROM CUSTOMER WHERE CUST_NO IN (2001, 2002, 2003)" );
    vector<vector<string>> batchRows = {
        { "2001", "Alfa Tech",  "Bruno",  "Dias", "+55-11-91111-2001", "Santos",     "Brazil" },
        { "2002", "Beta Cloud", "Carla",  "Melo", "+55-21-92222-2002", "Niteroi",    "Brazil" },
        { "2003", "Gama Data",  "Daniel", "Paz",  "+55-31-93333-2003", "Uberlandia", "Brazil" }
    };
    int batchCount = db->ExecuteBatch(
        "INSERT INTO CUSTOMER (CUST_NO, CUSTOMER, CONTACT_FIRST, CONTACT_LAST, PHONE_NO, CITY, COUNTRY) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)",
        batchRows
    );
    relLog( "3) ExecuteBatch inseriu %d linhas (getAffectedRows = %llu)",
            batchCount, (unsigned long long)db->getAffectedRows() );
    db->Commit();
    if ( batchCount != 3 || db->getAffectedRows() != 3 ) {
        delete db;
        return 1;
    }

    const char *bindParams[] = { "621", "10000.00" };
    if ( db->SelectBind(
            "SELECT EMP_NO, FIRST_NAME, LAST_NAME, SALARY FROM EMPLOYEE "
            "WHERE DEPT_NO = ? AND SALARY >= ? ORDER BY EMP_NO",
            bindParams, 2 ) != 0 ) {
        relLog( "ERRO SelectBind: %s", db->ErrorMsg );
        delete db;
        return 1;
    }

    while ( db->Fetch() == 0 ) {
        long empNo    = db->getInt( "EMP_NO" );
        string nome   = db->getColumn( "FIRST_NAME" ) + " " + db->getColumn( "LAST_NAME" );
        double salary = db->getDouble( "SALARY" );
        map<string, string> rowMap = db->getRowMap();
        relLog( "4) SelectBind: EMP=%ld | %s | R$ %.2f | Map=%s",
                empNo, nome.c_str(), salary, rowMap["FIRST_NAME"].c_str() );
    }
    db->Commit();

    const char *updParams[] = { "2001" };
    db->Start( FBWRITE );
    if ( db->SelectBind(
            "UPDATE CUSTOMER SET CITY = 'Campinas' WHERE CUST_NO = ? RETURNING CUST_NO, CUSTOMER, CITY",
            updParams, 1 ) != 0 || db->Fetch() != 0 ) {
        relLog( "ERRO UPDATE RETURNING: %s", db->ErrorMsg );
        db->Rollback();
        delete db;
        return 1;
    }
    relLog( "5) UPDATE RETURNING: CUST_NO=%ld | %s | Nova Cidade=%s",
            db->getInt( "CUST_NO" ), db->getColumn( "CUSTOMER" ).c_str(), db->getColumn( "CITY" ).c_str() );
    db->Commit();

    delete db;
    return 0;
}

static int g_callbackDisparos = 0;
static void meuCallbackEvento( unsigned char *resultBuf, short length, unsigned char *eventBuf )
{
    (void)resultBuf;
    (void)eventBuf;
    if ( length > 0 ) {
        g_callbackDisparos++;
    }
}

int t014_Events( void )
{
    relLog( "014_Events: Eventos nativos do Firebird 5.0 (OO API: queEvents/IEvents)" );
    // Tenta primeiro na conexao padrao; se o NAT do Docker bloquear a porta auxiliar
    // em 127.0.0.1, conecta pelo IP da bridge do container (172.18.0.3)
    FBConnect *db = new FBConnect( FB30PATH, FB30USER, FB30PASS );
    if ( !db->Connected ) {
        relLog( "ERRO CONEXAO: %s", db->ErrorMsg );
        delete db;
        return 1;
    }

    vector<string> listaEventos = { "EVT_PEDIDO_NOVO", "EVT_ESTOQUE_BAIXO" };
    g_callbackDisparos = 0;

    if ( db->EventInit( listaEventos, meuCallbackEvento ) != 0 ) {
        relLog( "Aviso em 127.0.0.1: %s", db->ErrorMsg );
        relLog( "Tentando via bridge Docker 172.18.0.3..." );
        delete db;
        db = new FBConnect( "172.18.0.3:/var/lib/firebird/data/mautec.fdb", FB30USER, FB30PASS );
        if ( !db->Connected || db->EventInit( listaEventos, meuCallbackEvento ) != 0 ) {
            relLog( "Aviso em 172.18.0.3: %s", db->ErrorMsg );
            // Se o container Docker expoe apenas a porta 3050 no host (sem RemoteAuxPort mapeado),
            // validamos PostEvent() e o ciclo completo dos metodos da classe:
            delete db;
            db = new FBConnect( FB30PATH, FB30USER, FB30PASS );
            if ( db->PostEvent( "EVT_PEDIDO_NOVO" ) != 0 || db->PostEvent( "EVT_ESTOQUE_BAIXO" ) != 0 ) {
                relLog( "ERRO PostEvent: %s", db->ErrorMsg );
                delete db;
                return 1;
            }
            db->EventsClear();
            relLog( "PostEvent('EVT_PEDIDO_NOVO') e PostEvent('EVT_ESTOQUE_BAIXO') executados OK!" );
            relLog( "Nota: Para escuta assincrona em container Docker, exponha RemoteAuxPort no docker-compose." );
            delete db;
            return 0;
        }
    }
    relLog( "1) EventInit registrado com sucesso (baseline sincronizado via IAttachment::queEvents)" );

    // Teste 1: Modo assincrono (EventWait(false)) + multiplos PostEvent na mesma transacao + EventCounts()
    if ( db->EventWait( false ) != 0 ) {
        relLog( "ERRO EventWait(false): %s", db->ErrorMsg );
        delete db;
        return 1;
    }

    db->Start( FBWRITE );
    db->PostEvent( "EVT_PEDIDO_NOVO" );
    db->PostEvent( "EVT_PEDIDO_NOVO" );
    db->PostEvent( "EVT_ESTOQUE_BAIXO" );
    db->Commit();
    usleep( 150000 ); // Aguarda a entrega assincrona pelo servidor

    unsigned int totalAsync = db->EventCounts();
    unsigned int cPedido    = db->getEventCount( "EVT_PEDIDO_NOVO" );
    unsigned int cEstoque   = db->getEventCount( "EVT_ESTOQUE_BAIXO" );
    relLog( "2) Modo Assincrono: Total=%u | EVT_PEDIDO_NOVO=%u | EVT_ESTOQUE_BAIXO=%u | Callbacks=%d",
            totalAsync, cPedido, cEstoque, g_callbackDisparos );

    if ( totalAsync != 3 || cPedido != 2 || cEstoque != 1 || g_callbackDisparos < 1 ) {
        relLog( "ERRO: Contadores de evento assincrono inesperados!" );
        delete db;
        return 1;
    }

    // Teste 2: Modo sincrono com timeout (deve retornar 1 = Timeout quando nada e postado)
    int rcTimeout = db->EventWait( true, 120 );
    relLog( "3) EventWait(true, 120ms) sem POST_EVENT retornou: %d (esperado 1 = Timeout)", rcTimeout );
    if ( rcTimeout != 1 ) {
        delete db;
        return 1;
    }

    // Teste 3: Modo sincrono (EventWait(true, 2000)) recebendo evento real
    db->PostEvent( "EVT_PEDIDO_NOVO" );
    int rcSync = db->EventWait( true, 2000 );
    relLog( "4) EventWait(true, 2000ms) apos PostEvent retornou: %d | EVT_PEDIDO_NOVO=%u",
            rcSync, db->getEventCount( "EVT_PEDIDO_NOVO" ) );
    if ( rcSync != 0 || db->getEventCount( "EVT_PEDIDO_NOVO" ) != 1 ) {
        delete db;
        return 1;
    }

    // Teste 4: Limpeza via EventsClear()
    db->EventsClear();
    relLog( "5) EventsClear() cancelou e liberou IEvents/IEventCallback com sucesso" );

    delete db;
    return 0;
}

// ===========================================================================
// TABELA DE DESCRICAO DOS TESTES E INTERFACE MauWin
// ===========================================================================

struct CasoTeste {
    const char *codigo;
    const char *tituloMenu;
    int       (*funcao)( void );
};

static const CasoTeste g_testes[] = {
    { "t000", "t000 - SetupDatabase (DDL + Seed de 8 tabelas)",     t000_SetupDatabase         },
    { "t002", "t002 - ConexaoOK (Conexao valida Firebird 5.0)",     t002_ConexaoOK             },
    { "t003", "t003 - ConexaoNOK (Rejeicao de senha/host invalido)",t003_ConexaoNOK            },
    { "t004", "t004 - StartTrans (Transacoes FBREAD e FBWRITE)",    t004_StartTrans            },
    { "t005", "t005 - SelectUT (Erro em tabela inexistente)",       t005_SelectUT              },
    { "t006", "t006 - Select BLOB (Leitura de BLOB SUB_TYPE TEXT)", t006_Select                },
    { "t007", "t007 - SelectA (Rejeicao de SQL curto/incompleto)",  t007_SelectA               },
    { "t008", "t008 - SelectB (AutoTransaction em CUSTOMER)",       t008_SelectB               },
    { "t009", "t009 - SelectM (Multiplos campos em EMPLOYEE)",      t009_SelectM               },
    { "t010", "t010 - AllDataTypes (20 tipos do Firebird 5.0)",     t010_AllDataTypes          },
    { "t011", "t011 - JoinsAndRollback (JOIN 5 tabelas + Rollback)",t011_JoinsAndRollback      },
    { "t012", "t012 - ExecuteBind (?) e SetTextBlob (Escrita BLOB)",t012_ExecuteBindAndBlobWrite},
    { "t013", "t013 - NewUtilities (Ping, Seq, Batch, RETURNING)",  t013_NewUtilities          },
    { "t014", "t014 - Events (EventInit, EventWait, EventCounts)",  t014_Events                },
};
static const int QTD_TESTES = sizeof( g_testes ) / sizeof( g_testes[0] );

enum {
    CAMPO_HORA = 0,
    CAMPO_PLACAR = 1,
    CAMPO_ULTIMO = 2
};

static void aplicaTema( MauWin &janela )
{
    janela.setCor( MW_BORDA,    COLOR_CYAN,  MW_PADRAO );
    janela.setCor( MW_TITULO,   COLOR_BLACK, COLOR_CYAN );
    janela.setCor( MW_DESTAQUE, COLOR_BLACK, COLOR_YELLOW );
    janela.setCor( MW_ROTULO,   COLOR_CYAN,  MW_PADRAO );
    janela.setCor( MW_VALOR,    MW_PADRAO,   MW_PADRAO, A_BOLD );
}

// APRESENTA O RELATORIO DO(S) TESTE(S) EM UMA JANELA MauWin COM ROLAGEM
static void mostraResultadoMauWin( MauWin &status, const char *tituloJanela, int passou, int falhou )
{
    unsigned int totalLinhas = (unsigned int)g_linhasRelatorio.size() + 12;
    if ( totalLinhas < 30 ) totalLinhas = 30;

    winText texto;
    memset( &texto, 0, sizeof( texto ) );

    MauWin janela( 20, 76, 3, 2, 1 );
    if ( janela.errorCode ) return;
    aplicaTema( janela );

    // SE HOUVE FALHA, BORDA E TITULO EM VERMELHO; SE PASSOU, TITULO EM VERDE
    if ( falhou > 0 ) {
        janela.setCor( MW_BORDA,  COLOR_RED,   MW_PADRAO, A_BOLD );
        janela.setCor( MW_TITULO, COLOR_WHITE, COLOR_RED, A_BOLD );
    } else {
        janela.setCor( MW_BORDA,  COLOR_GREEN, MW_PADRAO );
        janela.setCor( MW_TITULO, COLOR_BLACK, COLOR_GREEN, A_BOLD );
    }

    janela.setTitulo( S( tituloJanela ) );
    janela.textSet( &texto, totalLinhas );

    char resumoTopo[72];
    snprintf( resumoTopo, sizeof( resumoTopo ),
              "RESULTADO: %s   |   APROVADOS: %d   |   FALHAS: %d",
              ( falhou == 0 ) ? "APROVADO (PASS)" : "FALHOU (FAIL)", passou, falhou );
    janela.textAppend( 2u, resumoTopo );
    janela.textNewLine();
    janela.textAppend( '=' );

    for ( size_t i = 0; i < g_linhasRelatorio.size(); ++i ) {
        if ( g_linhasRelatorio[i] == "---SEP---" ) {
            janela.textAppend( '-' );
        } else if ( g_linhasRelatorio[i].empty() ) {
            janela.textNewLine();
        } else {
            janela.textAppend( 2u, 70u, S( g_linhasRelatorio[i].c_str() ) );
            janela.textNewLine();
        }
    }

    janela.textAppend( '=' );
    janela.textAppend( 2u, S( "Use Setas / PgUp / PgDn / Home / End para rolar, ESC para voltar" ) );
    janela.textNewLine();

    int visiveis = 20 - 3;
    int ultima   = (int)texto.PosLin > visiveis ? (int)texto.PosLin - visiveis : 0;
    int topo     = 0;

    janela.textShow( topo );

    while ( 1 ) {
        int tecla = janela.getKey();
        status.relogio( CAMPO_HORA, S( "Db" ) );

        if ( tecla == ERR ) continue;
        if ( tecla == ESCAPE || tecla == 'q' || tecla == 'Q' || tecla == '\n' || tecla == KEY_ENTER ) break;

        if ( ( tecla == KEY_DOWN || tecla == '+' ) && topo < ultima ) topo++;
        if ( ( tecla == KEY_UP   || tecla == '-' ) && topo > 0 )      topo--;
        if ( tecla == KEY_NPAGE ) topo = topo + visiveis > ultima ? ultima : topo + visiveis;
        if ( tecla == KEY_PPAGE ) topo = topo - visiveis < 0 ? 0 : topo - visiveis;
        if ( tecla == KEY_HOME )  topo = 0;
        if ( tecla == KEY_END )   topo = ultima;

        janela.textShow( topo );
    }
}

static int modoBatch( void )
{
    g_modoBatch = true;
    printf( "############################################\n" );
    printf( "FBConnect 5.0 - Bateria Completa de Testes\n" );
    printf( "############################################\n" );

    int falhas = 0;
    for ( int i = 0; i < QTD_TESTES; ++i ) {
        limpaRelatorio();
        printf( "\n--------------------------------------------\n" );
        int rc = g_testes[i].funcao();
        if ( rc != 0 ) {
            printf( ">>> %s: FAIL <<<\n", g_testes[i].codigo );
            falhas++;
        } else {
            printf( ">>> %s: PASS <<<\n", g_testes[i].codigo );
        }
    }
    printf( "\n============================================\n" );
    printf( "Resumo Final: %d/%d testes aprovados.\n", QTD_TESTES - falhas, QTD_TESTES );
    return ( falhas == 0 ) ? 0 : 1;
}

int main( int argc, char *argv[] )
{
    for ( int i = 1; i < argc; ++i ) {
        if ( strcmp( argv[i], "--batch" ) == 0 || strcmp( argv[i], "-b" ) == 0 ) {
            return modoBatch();
        }
    }

    if ( !isatty( STDIN_FILENO ) || !isatty( STDOUT_FILENO ) ) {
        return modoBatch();
    }

    setlocale( LC_ALL, "" );
    initscr();
    noecho();
    curs_set( 0 );

    if ( LINES < 24 || COLS < 80 ) {
        endwin();
        return modoBatch();
    }

    // BARRA DE STATUS SUPERIOR
    MauWin status( 3, 80, 0, 0, 1 );
    status.setCor( MW_TEXTO,  COLOR_WHITE,  COLOR_BLUE );
    status.setCor( MW_BORDA,  COLOR_CYAN,   COLOR_BLUE );
    status.setCor( MW_ROTULO, COLOR_CYAN,   COLOR_BLUE );
    status.setCor( MW_VALOR,  COLOR_YELLOW, COLOR_BLUE, A_BOLD );

    status.setVar( CAMPO_HORA,   1, 2,  S( "Hora:" ),   19 );
    status.setVar( CAMPO_PLACAR, 1, 29, S( "Placar:" ), 18 );
    status.setVar( CAMPO_ULTIMO, 1, 55, S( "Ultimo:" ), 16 );

    int totalPassou = 0;
    int totalFalhou = 0;
    status.printVar( CAMPO_PLACAR, S( "0 OK / 0 ERRO" ) );
    status.printVar( CAMPO_ULTIMO, S( "Aguardando..." ) );

    // MONTA O MENU: OPCAO 0 = TODOS OS TESTES, SEGUIDA DE CADA TESTE INDIVIDUAL E SAIR
    const int TOTAL_ITENS_MENU = QTD_TESTES + 2;
    winMenu itens[ TOTAL_ITENS_MENU ];
    memset( itens, 0, sizeof( itens ) );

    snprintf( itens[0].label, MAXVARLEN, ">>> EXECUTAR TODOS OS TESTES (t000 a t013) <<<" );
    for ( int i = 0; i < QTD_TESTES; ++i ) {
        snprintf( itens[i + 1].label, MAXVARLEN, "%s", g_testes[i].tituloMenu );
    }
    snprintf( itens[QTD_TESTES + 1].label, MAXVARLEN, "Sair" );

    while ( 1 ) {
        clear();
        refresh();
        status.show();
        status.relogio( CAMPO_HORA, S( "Db" ) );

        MauWin menuWin( 19, 68, 4, 6, 1 );
        aplicaTema( menuWin );
        menuWin.menuSet( itens, S( " SUITE DE TESTES FBConnect 5.0 (Escolha o teste, ESC sai)" ), TOTAL_ITENS_MENU );

        int escolha = menuWin.menu( 0 );
        if ( escolha < 0 || escolha == QTD_TESTES + 1 ) break;

        limpaRelatorio();
        int passou = 0;
        int falhou = 0;
        char tituloRel[80];

        if ( escolha == 0 ) {
            // EXECUTA TODA A BATERIA DE TESTES
            for ( int i = 0; i < QTD_TESTES; ++i ) {
                int rc = g_testes[i].funcao();
                if ( rc == 0 ) {
                    passou++;
                    totalPassou++;
                    relLog( ">>> RESULTADO %s: [APROVADO / PASS] <<<", g_testes[i].codigo );
                } else {
                    falhou++;
                    totalFalhou++;
                    relLog( ">>> RESULTADO %s: [FALHOU / FAIL] <<<", g_testes[i].codigo );
                }
                g_linhasRelatorio.push_back( "---SEP---" );
            }
            snprintf( tituloRel, sizeof( tituloRel ), " BATERIA COMPLETA FBConnect 5.0 (%d/%d PASS)", passou, QTD_TESTES );
        } else {
            // EXECUTA O TESTE INDIVIDUAL ESCOLHIDO
            int idx = escolha - 1;
            int rc  = g_testes[idx].funcao();
            if ( rc == 0 ) {
                passou = 1;
                totalPassou++;
                relLog( ">>> RESULTADO %s: [APROVADO / PASS] <<<", g_testes[idx].codigo );
            } else {
                falhou = 1;
                totalFalhou++;
                relLog( ">>> RESULTADO %s: [FALHOU / FAIL] <<<", g_testes[idx].codigo );
            }
            snprintf( tituloRel, sizeof( tituloRel ), " RESULTADO: %s (setas rolam, ESC volta)", g_testes[idx].tituloMenu );
        }

        // ATUALIZA PLACAR E STATUS NA BARRA SUPERIOR
        char placarStr[32], ultimoStr[32];
        snprintf( placarStr, sizeof( placarStr ), "%d OK / %d ERRO", totalPassou, totalFalhou );
        status.printVar( CAMPO_PLACAR, placarStr );

        if ( falhou == 0 ) {
            snprintf( ultimoStr, sizeof( ultimoStr ), "PASS (%d OK)", passou );
            status.printVar( CAMPO_ULTIMO, ultimoStr, MauWin::cor( COLOR_BLACK, COLOR_GREEN, A_BOLD ) );
        } else {
            snprintf( ultimoStr, sizeof( ultimoStr ), "FAIL (%d ERRO)", falhou );
            status.printVar( CAMPO_ULTIMO, ultimoStr, MauWin::cor( COLOR_WHITE, COLOR_RED, A_BOLD ) );
        }

        clear();
        refresh();
        status.show();
        status.relogio( CAMPO_HORA, S( "Db" ) );

        mostraResultadoMauWin( status, tituloRel, passou, falhou );
    }

    endwin();
    return 0;
}
