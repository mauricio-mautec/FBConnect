#include "FBConnect.h"
#include <string>
#include <iostream>

#define FB25PATH "127.0.0.1:/var/lib/firebird/data/mautec.fdb"
#define FB25PASS "pqIobXtPui94hw9"
#define FB25USER "softgo"
#define FB30PATH "127.0.0.1:/var/lib/firebird/data/mautec.fdb"
#define FB30PASS "pqIobXtPui94hw9"
#define FB30USER "softgo"

using namespace std;

int t000_SetupDatabase(void) {
    printf ("\n--------------------------\n000_SetupDatabase TEST\n Preparing tables and populating data in mautec.fdb\n");
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, FB30PASS);
    if (! db->Connected) {
        printf ("Connection Error %s\n", db->ErrorMsg);
        delete(db);
        return 1;
    }

    printf ("Database Connected — Firebird Engine Version: %s\n", db->getServerVersion().c_str());

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

    for (size_t i = 0; i < sizeof(ddl) / sizeof(ddl[0]); ++i) {
        db->Start(FBWRITE);
        if (db->Execute(ddl[i]) == 0) {
            db->Commit();
        } else {
            printf("DDL Error: %s\n", db->ErrorMsg);
            db->Rollback();
            delete(db);
            return 1;
        }
    }

    const char *inserts[] = {
        // DEPARTMENT (6 rows)
        "INSERT INTO DEPARTMENT VALUES ('000', 'Corporate Headquarters', NULL, 2500000.00, 'Sao Paulo')",
        "INSERT INTO DEPARTMENT VALUES ('100', 'Sales and Marketing', '000', 1200000.50, 'Sao Paulo')",
        "INSERT INTO DEPARTMENT VALUES ('600', 'Engineering', '000', 3450000.75, 'Campinas')",
        "INSERT INTO DEPARTMENT VALUES ('621', 'Software Development', '600', 1850000.00, 'Campinas')",
        "INSERT INTO DEPARTMENT VALUES ('622', 'Quality Assurance', '600', 780000.25, 'Curitiba')",
        "INSERT INTO DEPARTMENT VALUES ('623', 'Customer Support', '600', 620000.00, 'Belo Horizonte')",

        // JOB (5 rows)
        "INSERT INTO JOB VALUES ('Eng', 5, 'Brazil', 'Senior Software Engineer', 10000.00, 18000.00, 'Dominio de C/C++, Linux, Firebird 3 OO API, depuracao com GDB/Valgrind e arquitetura de sistemas de alta performance.')",
        "INSERT INTO JOB VALUES ('Eng', 4, 'Brazil', 'Software Engineer', 7000.00, 12000.00, 'Experiencia com desenvolvimento C++, SQL, controle de versao Git e testes automatizados.')",
        "INSERT INTO JOB VALUES ('Mngr', 6, 'Brazil', 'Engineering Manager', 15000.00, 25000.00, 'Lideranca tecnica de equipes de engenharia de software, planejamento de arquitetura e entregas.')",
        "INSERT INTO JOB VALUES ('QA', 3, 'Brazil', 'QA Analyst', 5000.00, 9000.00, 'Criacao de planos de teste, automacao de testes de regressao e validacao de banco de dados.')",
        "INSERT INTO JOB VALUES ('Sales', 4, 'Brazil', 'Account Executive', 6000.00, 14000.00, 'Gestao de contas corporativas, prospeccao e fechamento de contratos B2B.')",

        // EMPLOYEE (7 rows)
        "INSERT INTO EMPLOYEE VALUES (101, 'Mauricio', 'Junqueira', '621', 'Mngr', 6, '2017-12-27', 19500.00)",
        "INSERT INTO EMPLOYEE VALUES (102, 'Ana', 'Beatriz', '621', 'Eng', 5, '2018-03-15', 14800.50)",
        "INSERT INTO EMPLOYEE VALUES (103, 'Roberto', 'Almeida', '621', 'Eng', 4, '2019-07-01', 9800.75)",
        "INSERT INTO EMPLOYEE VALUES (104, 'Fernanda', 'Costa', '622', 'QA', 3, '2020-02-10', 7250.00)",
        "INSERT INTO EMPLOYEE VALUES (105, 'Lucas', 'Mendes', '100', 'Sales', 4, '2021-05-20', 8900.25)",
        "INSERT INTO EMPLOYEE VALUES (106, 'Patricia', 'Rocha', '623', 'Eng', 4, '2022-09-12', 10200.00)",
        "INSERT INTO EMPLOYEE VALUES (107, 'Gustavo', 'Nunes', '600', 'Eng', 5, '2018-11-18', 15600.80)",

        // PROJECT (4 rows)
        "INSERT INTO PROJECT VALUES ('FBCON', 'FBConnect C++ Interface', 101, 'Middleware', 'Biblioteca C++ de integracao direta com as novas interfaces orientadas a objetos (IMaster, IProvider, IAttachment, ITransaction, IStatement, IResultSet, IBlob) do Firebird 3.0+.')",
        "INSERT INTO PROJECT VALUES ('SGERP', 'SoftGo ERP Core', 102, 'ERP', 'Nucleo de processamento transacional de alta concorrencia com suporte a multiplas conexoes, controle estrito de transacoes Read Committed e manipulacao de BLOBs.')",
        "INSERT INTO PROJECT VALUES ('BIAPI', 'Analytics & Telemetry', 107, 'Analytics', 'Pipeline de coleta e agregacao de indicadores operacionais em tempo real integrando Firebird, PostgreSQL, Redis e RabbitMQ.')",
        "INSERT INTO PROJECT VALUES ('QAAUT', 'Suite de Testes Automatizados', 104, 'Tooling', 'Bateria automatizada de testes de regressao e verificacao de vazamento de memoria com Valgrind.')",

        // EMPLOYEE_PROJECT (7 rows)
        "INSERT INTO EMPLOYEE_PROJECT VALUES (101, 'FBCON')",
        "INSERT INTO EMPLOYEE_PROJECT VALUES (102, 'FBCON')",
        "INSERT INTO EMPLOYEE_PROJECT VALUES (102, 'SGERP')",
        "INSERT INTO EMPLOYEE_PROJECT VALUES (103, 'SGERP')",
        "INSERT INTO EMPLOYEE_PROJECT VALUES (104, 'QAAUT')",
        "INSERT INTO EMPLOYEE_PROJECT VALUES (106, 'SGERP')",
        "INSERT INTO EMPLOYEE_PROJECT VALUES (107, 'BIAPI')",

        // SALARY_HISTORY (4 rows)
        "INSERT INTO SALARY_HISTORY VALUES (101, '2019-01-15 10:30:00', 'ADMIN', 16500.00, 18.18)",
        "INSERT INTO SALARY_HISTORY VALUES (102, '2020-06-01 14:15:30', 'MAURICIO', 12500.00, 18.404)",
        "INSERT INTO SALARY_HISTORY VALUES (103, '2021-08-10 09:00:00', 'MAURICIO', 8500.00, 15.3029)",
        "INSERT INTO SALARY_HISTORY VALUES (107, '2022-03-20 16:45:12', 'ADMIN', 13800.00, 13.0492)",

        // CUSTOMER (5 rows)
        "INSERT INTO CUSTOMER VALUES (1001, 'Mautec Tecnologia Ltda', 'Mauricio', 'Junqueira', '+55-11-99999-0001', 'Sao Paulo', 'Brazil')",
        "INSERT INTO CUSTOMER VALUES (1002, 'SoftGo Sistemas S.A.', 'Carlos', 'Eduardo', '+55-19-98888-0002', 'Campinas', 'Brazil')",
        "INSERT INTO CUSTOMER VALUES (1003, 'Horizonte Logistica', 'Mariana', 'Silva', '+55-31-97777-0003', 'Belo Horizonte', 'Brazil')",
        "INSERT INTO CUSTOMER VALUES (1004, 'Sul Cloud Services', 'Rafael', 'Kowalski', '+55-41-96666-0004', 'Curitiba', 'Brazil')",
        "INSERT INTO CUSTOMER VALUES (1005, 'Atlantico Comercio Exterior', 'Helena', 'Santos', '+55-21-95555-0005', 'Rio de Janeiro', 'Brazil')",

        // TYPE_SHOWCASE (2 rows covering all Firebird 5.x SQL types including INT128, DECFLOAT, TIME ZONE, BOOLEAN, NULL)
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

    db->Start(FBWRITE);
    for (size_t i = 0; i < sizeof(inserts) / sizeof(inserts[0]); ++i) {
        if (db->Execute(inserts[i]) != 0) {
            printf("Insert Error on [%zu]: %s\n", i, db->ErrorMsg);
            db->Rollback();
            delete(db);
            return 1;
        }
    }
    db->Commit();

    delete(db);
    return 0;
}
int t001_Conexao25(void) {
    printf ("\n--------------------------\n001_Conexao25  TEST\n Connecting to database Firebird 2.5\n");
    FBConnect *db = new FBConnect (FB25PATH, FB25USER, FB25PASS);
    if (! db->Connected) {
        printf ("Connection Error %s\n", db->ErrorMsg);
        delete(db);
        return 1;
        }
    printf ("Database Connected\n");
    delete(db);
    return 0;

}

/*
 *  t002_ConexaoOK
 *
 *  This function tests the connection with Firebird
 */
int t002_ConexaoOK(void) {
    printf ("\n--------------------------\n002_ConexaoOK  TEST\n Connecting to Database\n");
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, FB30PASS);

    if (! db->Connected) {
        printf ("Connection Error %s\n", db->ErrorMsg);
        delete(db);
        return 1; }

    printf ("Database Connected\n");

    delete(db);
    db = NULL;

    return 0;

}

/*
 *  Function t003_ConexaoNOK
 *
 *  This function tests the connection with Firebird
 *  with not valid user and password
 */
int t003_ConexaoNOK (void) {
    printf ("\n--------------------------\n003_ConexaoNOK TEST\nConnecting with invalid password\n");
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, "thisisawrongpass");
    
    if (db->Connected) {
        printf ("Database connected\n");
        delete (db);
        db = NULL;
        return 1;
    }
    printf ("Connection Error %s\n", db->ErrorMsg);
    delete (db);
    db = NULL;

    printf ("Connecting to database with invalid user\n");
    db = new FBConnect ("server:DB", "USER", "PASSWRONG");
    
    if (db->Connected) {
        printf ("Database Connection OK\n");
        delete (db);
        db = NULL;
        return 1;
    }

    printf ("Connection Error %s\n", db->ErrorMsg);
    delete (db);
    db = NULL;

    return 0;
}

/*
 *  Function t004_StartTrans
 *
 *  This function tests if Transaction is OK
 *  
 */
int t004_StartTrans (void) {
    printf ("\n--------------------------\n005_StartTrans TEST\nConnect with Firebird 3.0 e initiate a transaction\n");
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, FB30PASS);
    
    if (! db->Connected) {
        printf ("ERRO CONEXAO %s\n", db->ErrorMsg);
        delete (db);
        db = NULL;
        return 1;
    }

    printf ("Database connected\n");

    db->Start(FBREAD);
    if (! db->InTrans) {
        printf ("ERROR Reading Transaction\n");
        delete (db);
        db = NULL;
        return 1; }    
    printf ("Reading transaction successfully started!\n");


    db->Commit();

    db->Start(FBWRITE);
    if (! db->InTrans) {
        printf ("ERROR Write Transaction\n");
        delete (db);
        db = NULL;
        return 1; }    
    printf ("Write transaction successfully started!\n");

    db->Commit();
    delete (db);
    db = NULL;
    return 0;
}

/*
 *  Function t005_SelectUT
 *
 *  This function tests if  possible read
 *  from a unavaiable table
 *  
 */
int t005_SelectUT (void) {
    printf ("\n--------------------------\n005_SelectUnknowTable   TEST\n");
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, FB30PASS);
    
    if (! db->Connected) {
        printf ("ERRO CONEXAO %s\n", db->ErrorMsg);
        delete (db);
        db = NULL;
        return 1;
    }

    printf ("Database Connected!\n");

    if (db->Start (FBREAD ) || ! db->InTrans) {
        printf ("ERROR Read Transaction\n");
        delete (db);
        db = NULL;
        return 1; }    
    printf ("Read tansaction  sucessfully started!\n");

    const char *stmt= "SELECT * FROM UNKNOWTABLE";

    if (db->Select (stmt)) {
        printf ("ERROR SELECT STATEMENT: %s\n", stmt);
        printf ("MESSAGE ERROR FROM FBConnect:%s\n", db->ErrorMsg);
        delete (db);
        db = NULL;
        return 0; }    

    delete (db);
    db = NULL;
    return 1;
}

/*
 *  Function t006_Select
 *
 *  This function tests if select with a blob field
 *  and print the result of selected field 
 *  
 */
int t006_Select (void) {
    printf ("\n--------------------------\n006_Select           TEST\n");
    printf("BANCO------->>>>%s\n", FB30PATH);
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, FB30PASS);
    
    if (! db->Connected) {
        printf ("ERRO CONEXAO %s\n", db->ErrorMsg);
        delete (db);
        db = NULL;
        return 1;
    }


    printf ("Database Connected\n");

    if (db->Start (FBREAD ) || ! db->InTrans) {
        printf ("ERRO Transacao de LEITURA\n");
        delete (db);
        db = NULL;
        return 1; }    
    printf ("Transacao de LEITURA  iniciada com sucesso!\n");

     const char *stmt= "SELECT PROJ_DESC, PROJ_NAME, PROJ_DESC  FROM PROJECT";

    if (db->Select (stmt)) {
        printf ("ERRO SELECT STATEMENT: %s\n", stmt);
        printf ("MESSAGE ERROR FROM FBConnect:%s\n", db->ErrorMsg);
        delete (db);
        db = NULL;
        return 1; }    

    printf ("SELECT STATEMENT OK!\n");

    int ret = db->Fetch (); // DADOS ALOCADO INTERNAMENTE

    int intBlob;
    (void)intBlob;
    int tamanho;

    do {
   
	// Obtem o tamanho necessario para armazenar a resposta do getColumn
    	tamanho = db->getDataSize(0);

	// aloca a variavel para armazenameneto da resposta do getColumn
        unsigned char *dados = new unsigned char [tamanho];
        memset (dados, '\0', tamanho);

       // O getColumn vai retornar os dados que estao nas tuplas ou, caso blob, retorna o blobID
       db->getColumn (tamanho, 0);



	if( db->isBlob(0) && db->blobType(0))
	{
		// tamanho de caracteres para se alocar espaco
    		int t;
		// o retorno ira armazenar se tem um handler para trabalhar com o blob ou nao
		int retorno;
		// caso a alocacao do handler ocorra, chama o metodo getBlobSize
		// para se obter o tamanho de espaco necessario para alocar os dados
       		retorno = db->workBlob(dados) ;
		if(retorno) {
			// chamada do metodo para se obter a quantidade de characteres
			t = db->getBlobSize(db->blob);
		}
       		//t += 1;
        string basic_string;
       	//	unsigned char *texto = new unsigned char [t];
        basic_string = db->GetTextBlob();
            //string basic_string = "Oi";
       	//memset(texto, '\0', t);
		//intBlob = 	db->GetTextBlob(texto);
       	//	printf("TEXTO : [%s] \n", texto);
        cout << basic_string << endl;
        	t = 0;
        	(void)t;
	}	
				
   

/*

    	tamanho = db->getDataSize(1);

    	unsigned char *dados1 = new unsigned char [tamanho];
	string my_string;

    	memset (dados1, '\0', tamanho);

    	my_string = db->getColumn (tamanho, 1);

    	//printf("TEXTO : [%s] \n", dados1);
	cout << my_string << endl;
*/
	ret = db->Fetch();
        delete[] dados;
    } while (!ret);


    printf ("FETCH STATEMENT OK:\n");

    db->Commit();
    delete (db);
    db = NULL;
    
    return 0;
}

/*
 *  Function t007_SelectA
 *
 *  This function tests if select statement
 *  is correct
 *  
 */
int t007_SelectA (void) {
    printf ("\n--------------------------\n007_SelectA STATEMENT < 15    TEST\n");
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, FB30PASS);
    
    if (! db->Connected) {
        printf ("CONNECTION ERROR %s\n", db->ErrorMsg);
        delete (db);
        db = NULL;
        return 1;
    }

    printf ("CONNECTED\n");

    db->Start(FBREAD);
    if (! db->InTrans) {
        printf ("Read Transaction ERROR\n");
        delete (db);
        db = NULL;
        return 1; }    
    printf ("Read Transaction sucessfully started!\n");

    const char *stmt= "SELECT * FROM ";

    if (db->Select (stmt)) {
        printf ("ERRO SELECT STATEMENT: %s\n%s\n", stmt, db->ErrorMsg);
        printf ("TEST SELECT STATEMENT < 15 OK!\n");
        delete (db);
        db = NULL;
        return 0; }    

    printf ("STATEMENT ERROR < 15 !\n");
    delete (db);
    db = NULL;
    return -1;
}

/*
 *  Function t008_SelectB
 *
 *  This function tests if select statement
 *  is correct
 *  
 */
int t008_SelectB (void) {
    printf ("\n--------------------------\n008_SelectB AutoTransaction    TEST\n");
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, FB30PASS);
    
    if (! db->Connected) {
        printf ("CONNECTION ERROR %s\n", db->ErrorMsg);
        delete (db);
        db = NULL;
        return 1;
    }

    printf ("Database Connected\n");

    const char *stmt= "SELECT * FROM CUSTOMER";

    if (db->Select (stmt)) {
        printf ("ERRO SELECT STATEMENT: %s\n", stmt);
        delete (db);
        db = NULL;
        return 1; }    

    printf ("SELECT STATEMENT OK!\n");

    char dados[400];
    memset (dados, '\0', sizeof (dados));
    int ret = db->Fetch ();
    if (ret && ret != 100L) {
        printf ("ERRO FETCH STATEMENT: %s\n", stmt);
        delete (db);
        db = NULL;
        return 1; }    

    printf ("FETCH STATEMENT OK:\n%s", dados);

    db->Commit();

    delete (db);
    db = NULL;
    return 0;
}

/*
 *  Function t009_SelectM
 *
 *  This function tests if we can get more
 *  that one field
 *  
 */
/*
 */
int t009_SelectM (void) {
    printf ("\n--------------------------\n006_Select           TEST\n");
    printf("BANCO------->>>>%s\n", FB30PATH);
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, FB30PASS);
    
    if (! db->Connected) {
        printf ("ERROR CONNECTION %s\n", db->ErrorMsg);
        delete (db);
        db = NULL;
        return 1;
    }


    printf ("Database Connected\n");

    if (db->Start (FBREAD ) || ! db->InTrans) {
        printf ("ERROR Read Transaction\n");
        delete (db);
        db = NULL;
        return 1; }    
    printf ("Read Transaction sucessfully started!\n");

     //const char *stmt= "SELECT cust_NO FROM CUSTOMER"; // OK
    const char *stmt= "SELECT hire_date, salary, job_grade FROM EMPLOYEE";
    //const char *stmt= "SELECT  job_grade, salary, first_name FROM EMPLOYEE";
    //const char *stmt= "SELECT job_grade, hire_date, salary FROM EMPLOYEE";
    //const char *stmt= "SELECT job_grade FROM EMPLOYEE";
     //const char *stmt= "SELECT SALARY  FROM EMPLOYEE"; //OK
    //const char *stmt= "SELECT MAX_SALARY FROM JOB"; // OK
    //const char *stmt= "SELECT JOB_REQUIREMENT FROM JOB";
    //const char *stmt= "SELECT * FROM JOB";
    // const char *stmt= "SELECT CHANGE_DATE  FROM SALARY_HISTORY";
    //const char *stmt= "SELECT EMP_NO  FROM EMPLOYEE_PROJECT"; //OK
    //const char *stmt= "SELECT  CONTACT_FIRST, CONTACT_LAST, CITY   FROM CUSTOMER"; //OK
    // const char *stmt= "SELECT  CONTACT_FIRST   FROM CUSTOMER"; // OK
    // const char *stmt= "SELECT FIRST_NAME, HIRE_DATE, SALARY  FROM EMPLOYEE";  //OK

    if (db->Select (stmt)) {
        printf ("ERRO SELECT STATEMENT: %s\n", stmt);
        printf ("MESSAGE ERROR FROM FBConnect:%s\n", db->ErrorMsg);
        delete (db);
        db = NULL;
        return 1; }    

    printf ("SELECT STATEMENT OK!\n");

    int ret = db->Fetch (); // Allocated data

    int tamanho;
    (void)tamanho;
    int size0;
    int size1;
    int size2;
    do {
   
	// Size needed to store the response
    	size0 = db->getDataSize(0);
    	size1 = db->getDataSize(1);
    	size2 = db->getDataSize(2);

	// variable to store the data
        //unsigned char *dados0 = new unsigned char [size0];
        //unsigned char *dados1 = new unsigned char [size1];
        //unsigned char *dados2 = new unsigned char [size2];
        //memset (dados0, '\0', size0);
        //memset (dados1, '\0', size1);
        //memset (dados2, '\0', size2);
	
	string fbc_dado0;
	string fbc_dado1;
	string fbc_dado2;

    	fbc_dado0 = db->getColumn (size0, 0);
    	fbc_dado1 = db->getColumn (size1, 1);
    	fbc_dado2 = db->getColumn (size2, 2);

    	//printf("TEXTO : [%s] \n", dados1);
	cout << fbc_dado0 << endl;
	cout << fbc_dado1 << endl;
	cout << fbc_dado2 << endl;

       //db->getColumn (dados0, 0);
       //db->getColumn (dados1, 1);
       //db->getColumn (dados0, 0);

	
       		//printf("Field 0 : [%s] \n", dados0);
       		//printf("Field 1 : [%s] \n", dados1);
       		//printf("Field 2 : [%s] \n", dados2);
				
	ret = db->Fetch();

    } while (!ret);


    printf ("FETCH STATEMENT OK:\n");

    db->Commit();
    delete (db);
    db = NULL;
    
    return 0;
}

/*
 *  Function t010_AllDataTypes
 *
 *  Tests all SQL types supported by FBConnect on TYPE_SHOWCASE
 */
int t010_AllDataTypes (void) {
    printf ("\n--------------------------\n010_AllDataTypes TEST\n");
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, FB30PASS);
    if (! db->Connected) {
        printf ("ERROR CONNECTION %s\n", db->ErrorMsg);
        delete (db);
        return 1;
    }

    const char *stmt =
        "SELECT ID, CODE_FIXED, NAME_VAR, SMALL_VAL, BIG_VAL, DEC_VAL, FLT_VAL, DBL_VAL, "
        "DT_VAL, TM_VAL, TS_VAL, NOTES_BLOB, BOOL_VAL, INT128_VAL, NUM38_VAL, DEC16_VAL, "
        "DEC34_VAL, TIMETZ_VAL, TSTZ_VAL, NULLABLE_COL "
        "FROM TYPE_SHOWCASE ORDER BY ID";
    if (db->Select (stmt)) {
        printf ("ERRO SELECT: %s\n", db->ErrorMsg);
        delete (db);
        return 1;
    }

    printf ("Colunas retornadas (getCount): %u\n", db->getCount());

    int ret = db->Fetch();
    while (!ret) {
        cout << "----------------------------------------" << endl;
        cout << "ID (LONG)        : [" << db->getColumn(0) << "]" << endl;
        cout << "CODE (CHAR)      : [" << db->getColumn(1) << "]" << endl;
        cout << "NAME (VARCHAR)   : [" << db->getColumn(2) << "]" << endl;
        cout << "SMALL (SHORT)    : [" << db->getColumn(3) << "]" << endl;
        cout << "BIG (INT64)      : [" << db->getColumn(4) << "]" << endl;
        cout << "DEC (NUMERIC 15) : [" << db->getColumn(5) << "]" << endl;
        cout << "FLT (FLOAT)      : [" << db->getColumn(6) << "]" << endl;
        cout << "DBL (DOUBLE)     : [" << db->getColumn(7) << "]" << endl;
        cout << "DT (DATE)        : [" << db->getColumn(8) << "]" << endl;
        cout << "TM (TIME)        : [" << db->getColumn(9) << "]" << endl;
        cout << "TS (TIMESTAMP)   : [" << db->getColumn(10) << "]" << endl;

        string blobIdStr = db->getColumn(11);
        if (db->isBlob(11) && db->blobType(11)) {
            int blobLen = db->getBlobSize(db->blob);
            string blobText = db->GetTextBlob();
            cout << "BLOB (" << blobLen << " bytes) : [" << blobText << "]" << endl;
        }
        cout << "BOOL (BOOLEAN)   : [" << db->getColumn(12) << "]" << endl;
        cout << "INT128           : [" << db->getColumn(13) << "]" << endl;
        cout << "NUMERIC(38,6)    : [" << db->getColumn(14) << "]" << endl;
        cout << "DECFLOAT(16)     : [" << db->getColumn(15) << "]" << endl;
        cout << "DECFLOAT(34)     : [" << db->getColumn(16) << "]" << endl;
        cout << "TIME TZ          : [" << db->getColumn(17) << "]" << endl;
        cout << "TIMESTAMP TZ     : [" << db->getColumn(18) << "]" << endl;
        cout << "NULLABLE_COL     : " << (db->isNull(19) ? "<NULL>" : ("[" + db->getColumn(19) + "]")) << endl;
        ret = db->Fetch();
    }

    db->Commit();
    delete (db);
    return 0;
}

/*
 *  Function t011_JoinsAndRollback
 *
 *  Tests multi-table JOINs and Rollback vs Commit
 */
int t011_JoinsAndRollback (void) {
    printf ("\n--------------------------\n011_JoinsAndRollback TEST\n");
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, FB30PASS);
    if (! db->Connected) {
        printf ("ERROR CONNECTION %s\n", db->ErrorMsg);
        delete (db);
        return 1;
    }

    // 1. Testar UPDATE seguido de Rollback (nao deve persistir)
    db->Start(FBWRITE);
    if (db->Execute("UPDATE DEPARTMENT SET BUDGET = 9999999.99 WHERE DEPT_NO = '621'") != 0) {
        printf ("ERRO UPDATE: %s\n", db->ErrorMsg);
        delete (db);
        return 1;
    }
    db->Rollback();

    // 2. Consultar JOIN entre EMPLOYEE, DEPARTMENT, JOB e PROJECT
    const char *stmt =
        "SELECT E.FIRST_NAME, E.LAST_NAME, D.DEPARTMENT, D.BUDGET, J.JOB_TITLE, P.PROJ_NAME "
        "FROM EMPLOYEE E "
        "JOIN DEPARTMENT D ON D.DEPT_NO = E.DEPT_NO "
        "JOIN JOB J ON J.JOB_CODE = E.JOB_CODE AND J.JOB_GRADE = E.JOB_GRADE "
        "JOIN EMPLOYEE_PROJECT EP ON EP.EMP_NO = E.EMP_NO "
        "JOIN PROJECT P ON P.PROJ_ID = EP.PROJ_ID "
        "ORDER BY E.EMP_NO, P.PROJ_ID";

    if (db->Select(stmt)) {
        printf ("ERRO SELECT JOIN: %s\n", db->ErrorMsg);
        delete (db);
        return 1;
    }

    int rows = 0;
    int ret = db->Fetch();
    while (!ret) {
        rows++;
        cout << "Row " << rows << ": "
             << db->getColumn(db->getDataSize(0), 0) << " | "
             << db->getColumn(db->getDataSize(1), 1) << " | "
             << db->getColumn(db->getDataSize(2), 2) << " | Budget: "
             << db->getColumn(db->getDataSize(3), 3) << " | "
             << db->getColumn(db->getDataSize(4), 4) << " | Proj: "
             << db->getColumn(db->getDataSize(5), 5) << endl;
        ret = db->Fetch();
    }

    db->Commit();
    delete (db);
    return (rows == 7) ? 0 : 1;
}

int main (int argc, char* argv[]) {
     (void)argc;
     (void)argv;
     printf("############################################\n");

    if (t000_SetupDatabase()) { printf ("\n000_SetupDatabase FAIL\n--------------------------\n\n"); return 1; }
    else                        printf ("\n000_SetupDatabase PASS\n--------------------------\n\n");

    if (t002_ConexaoOK()) { printf ("\n002_ConexaoOK  FAIL\n--------------------------\n\n"); return 1; }
    else                    printf ("\n002_ConexaoOK  PASS\n--------------------------\n\n");

    if (t003_ConexaoNOK()){ printf ("\n003_ConexaoNOK FAIL\n--------------------------\n\n"); return 1; }
    else                    printf ("\n003_ConexaoNOK PASS\n--------------------------\n\n");
   
    if (t004_StartTrans()){ printf ("\n004_StartTrans FAIL\n--------------------------\n\n"); return 1; }
    else                    printf ("\n004_StartTrans PASS\n--------------------------\n\n");
    
    if (t005_SelectUT())  { printf ("\n005_SelectUT   FAIL\n--------------------------\n\n"); return 1; }
    else                    printf ("\n005_SelectUT   PASS\n--------------------------\n\n");

    if (t006_Select()    ){ printf ("\n006_Select     FAIL\n--------------------------\n\n"); return 1; }
    else                    printf ("\n006_Select     PASS\n--------------------------\n\n");

    if (t007_SelectA()   ){ printf ("\n007_SelectA    FAIL\n--------------------------\n\n"); return 1; }
    else                    printf ("\n007_SelectA    PASS\n--------------------------\n\n");

    if (t008_SelectB()   ){ printf ("\n008_SelectB    FAIL\n--------------------------\n\n"); return 1; }
    else                    printf ("\n008_SelectB    PASS\n--------------------------\n\n");

    if (t009_SelectM()   ){ printf ("\n009_SelectM    FAIL\n--------------------------\n\n"); return 1; }
    else                    printf ("\n009_SelectM    PASS\n--------------------------\n\n");

    if (t010_AllDataTypes()){ printf ("\n010_AllDataTypes FAIL\n--------------------------\n\n"); return 1; }
    else                      printf ("\n010_AllDataTypes PASS\n--------------------------\n\n");

    if (t011_JoinsAndRollback()){ printf ("\n011_JoinsAndRollback FAIL\n--------------------------\n\n"); return 1; }
    else                          printf ("\n011_JoinsAndRollback PASS\n--------------------------\n\n");
   
    return 0;
}    

