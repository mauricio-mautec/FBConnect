#include "FBConnect.h"
#include <string>
#include <iostream>

#define FB25PATH ""
#define FB25PASS ""
#define FB25USER "SYSDBA"
#define FB30PATH "dev01:employee"
#define FB30PASS "masterkey"
#define FB30USER "SYSDBA"

using namespace std;
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
/*
int t007_SelectA (void) {
    printf ("\n--------------------------\n007_SelectA STATEMENT < 15    TEST\n");
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, FB30PASS);
    
    if (! db->Connected) {
        printf ("CONNECTION ERROR %s\n", db->ErrorMsg);
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
        printf ("ERRO SELECT STATEMENT: %s\n%s", stmt, db->ErrorMsg);
        printf ("TEST SELECT STATEMENT < 15 OK!\n");
        delete (db);
        db = NULL;
        return 0; }    

    printf ("STATEMENT ERROR < 15 !\n");
    return -1;
}
*/
/*
 *  Function t008_SelectB
 *
 *  This function tests if select statement
 *  is correct
 *  
 */
/*
int t008_SelectB (void) {
    printf ("\n--------------------------\n008_SelectB AutoTransaction    TEST\n");
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, FB30PASS);
    
    if (! db->Connected) {
        printf ("CONNECTION ERROR %s\n", db->ErrorMsg);
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
*/

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
    const char *stmt= "SELECT hire_date FROM EMPLOYEE";
     //const char *stmt= "SELECT FIRST_NAME, SALARY  FROM EMPLOYEE"; //OK
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
    int size0;
    int size1;
    int size2;
    do {
   
	// Size needed to store the response
    	size0 = db->getDataSize(0);
    	//size1 = db->getDataSize(1);
    	//size2 = db->getDataSize(2);

	// variable to store the data
        unsigned char *dados0 = new unsigned char [size0];
        //unsigned char *dados1 = new unsigned char [size1];
        //unsigned char *dados2 = new unsigned char [size2];
        memset (dados0, '\0', size0);
        //memset (dados1, '\0', size1);
        //memset (dados2, '\0', size2);
	
	string fbc_dado0;

    	fbc_dado0 = db->getColumn (size0, 0);

    	//printf("TEXTO : [%s] \n", dados1);
	cout << fbc_dado0 << endl;

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

int main (int argc, char* argv[]) {
     printf("############################################\n");
/*
    
    if (t001_Conexao25()) { printf ("\n001_Conexao25  FAIL\n--------------------------\n\n"); return 0; }
    else                    printf ("\n001_Conexao25  PASS\n--------------------------\n\n");
    if (t002_ConexaoOK()) { printf ("\n002_ConexaoOK  FAIL\n--------------------------\n\n"); return 0; }
    else                    printf ("\n002_ConexaoOK  PASS\n--------------------------\n\n");
    if (t003_ConexaoNOK()){ printf ("\n003_ConexaoNOK FAIL\n--------------------------\n\n"); return 0; }
    else                    printf ("\n003_ConexaoNOK PASS\n--------------------------\n\n");
   
    if (t004_StartTrans()){ printf ("\n004_StartTrans FAIL\n--------------------------\n\n"); return 0; }
    else                    printf ("\n004_StartTrans PASS\n--------------------------\n\n");
    
    if (t005_SelectUT())  { printf ("\n005_SelectUT   FAIL\n--------------------------\n\n"); return 0; }
    else                    printf ("\n005_SelectUT   PASS\n--------------------------\n\n");

    if (t006_Select()    ){ printf ("\n006_Select     FAIL\n--------------------------\n\n"); return 0; }
    else                    printf ("\n006_Select     PASS\n--------------------------\n\n");
    

    if (t007_SelectA()   ){ printf ("\n006_SelectA    FAIL\n--------------------------\n\n"); return 0; }
    else                    printf ("\n006_SelectA    PASS\n--------------------------\n\n");
    
    if (t008_SelectB()   ){ printf ("\n007_SelectB    FAIL\n--------------------------\n\n"); return 0; }
    else                    printf ("\n007_SelectB    PASS\n--------------------------\n\n");
    if (t008_SelectB()   ){ printf ("\n008_SelectB    FAIL\n--------------------------\n\n"); return 0; }
    else                    printf ("\n008_SelectB    PASS\n--------------------------\n\n");
    
   */ 
    if (t009_SelectM()   ){ printf ("\n009_SelectB    FAIL\n--------------------------\n\n"); return 0; }
    else                    printf ("\n009_SelectB    PASS\n--------------------------\n\n");
   
    return 0;
    /*
*/    
}    

