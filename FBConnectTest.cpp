#include "FBConnect.h"
#define FB25PATH ""
#define FB25PASS ""
#define FB25USER "SYSDBA"
#define FB30PATH "dev01:employee"
#define FB30PASS "masterkey"
#define FB30USER "SYSDBA"

int t001_Conexao25(void) {
    printf ("\n--------------------------\n001_Conexao25  TEST\nConectando no banco VERSAO 2.5\n");
    FBConnect *db = new FBConnect (FB25PATH, FB25USER, FB25PASS);
    if (! db->Connected) {
        printf ("ERRO CONEXAO %s\n", db->ErrorMsg);
        delete(db);
        return 1;
        }
    printf ("Banco conectado\n");
    delete(db);
    return 0;

}

int t002_ConexaoOK(void) {
    printf ("\n--------------------------\n002_ConexaoOK  TEST\nConectando no banco\n");
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, FB30PASS);

    if (! db->Connected) {
        printf ("ERRO CONEXAO %s\n", db->ErrorMsg);
        delete(db);
        return 1; }

    printf ("Banco conectado\n");

    delete(db);
    db = NULL;

    return 0;

}

int t003_ConexaoNOK (void) {
    printf ("\n--------------------------\n003_ConexaoNOK TEST\nConectando no banco com senha incorreta\n");
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, "thisisawrongpass");
    
    if (db->Connected) {
        printf ("Banco conectado\n");
        delete (db);
        db = NULL;
        return 1;
    }
    printf ("ERRO CONEXAO %s\n", db->ErrorMsg);
    delete (db);
    db = NULL;

    printf ("Conectando no banco com usuario incorreto\n");
    db = new FBConnect ("ada:BSOICA3", "SSSDBA", "aeShoo0amaike7aF");
    
    if (db->Connected) {
        printf ("Banco conectado\n");
        delete (db);
        db = NULL;
        return 1;
    }

    printf ("ERRO CONEXAO %s\n", db->ErrorMsg);
    delete (db);
    db = NULL;

    return 0;
}

int t004_StartTrans (void) {
    printf ("\n--------------------------\n005_StartTrans TEST\nConectando no banco 3.0 e iniciando uma transacao\n");
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, FB30PASS);
    
    if (! db->Connected) {
        printf ("ERRO CONEXAO %s\n", db->ErrorMsg);
        return 1;
    }

    printf ("Banco conectado\n");

    db->Start(FBREAD);
    if (! db->InTrans) {
        printf ("ERRO Transacao de LEITURA\n");
        delete (db);
        db = NULL;
        return 1; }    
    printf ("Transacao de LEITURA  iniciada com sucesso!\n");


    db->Commit();

    db->Start(FBWRITE);
    if (! db->InTrans) {
        printf ("ERRO Transacao de GRAVACAO\n");
        delete (db);
        db = NULL;
        return 1; }    
    printf ("Transacao de GRAVACAO iniciada com sucesso!\n");

    db->Commit();
    delete (db);
    db = NULL;
    return 0;
}


int t005_SelectUT (void) {
    printf ("\n--------------------------\n005_SelectUnknowTable   TEST\n");
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, FB30PASS);
    
    if (! db->Connected) {
        printf ("ERRO CONEXAO %s\n", db->ErrorMsg);
        return 1;
    }

    printf ("Banco conectado\n");

    if (db->Start (FBREAD ) || ! db->InTrans) {
        printf ("ERRO Transacao de LEITURA\n");
        delete (db);
        db = NULL;
        return 1; }    
    printf ("Transacao de LEITURA  iniciada com sucesso!\n");

    const char *stmt= "SELECT * FROM UNKNOWTABLE";

    if (db->Select (stmt)) {
        printf ("ERRO SELECT STATEMENT: %s\n", stmt);
        printf ("MESSAGE ERROR FROM FBConnect:%s\n", db->ErrorMsg);
        delete (db);
        db = NULL;
        return 0; }    

    delete (db);
    db = NULL;
    return 1;
}

/*
 *  Teste numero 6, usado para avaliar retorno simples
 *  de um BLOB
 * */

int t006_Select (void) {
    printf ("\n--------------------------\n006_Select           TEST\n");
    printf("BANCO------->>>>%s\n", FB30PATH);
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, FB30PASS);
    
    if (! db->Connected) {
        printf ("ERRO CONEXAO %s\n", db->ErrorMsg);
        return 1;
    }


    printf ("Banco conectado\n");

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
/*
    if (ret && ret != 100L) {
        printf ("ERRO FETCH STATEMENT: %s\n", stmt);
        delete (db);
        db = NULL;
        return 1; }    
*/

    int intBlob;
    int tamanho;

    do {
   
	// Obtem o tamanho necessario para armazenar a resposta do getColumn
    	tamanho = db->getDataSize(0);

	// aloca a variavel para armazenameneto da resposta do getColumn
        unsigned char *dados = new unsigned char [tamanho];
        memset (dados, '\0', tamanho);

       // O getColumn vai retornar os dados que estao nas tuplas ou, caso blob, retorna o blobID
       db->getColumn (dados, 0);



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
       		unsigned char *texto = new unsigned char [t];
       		memset(texto, '\0', t);
        	printf("QUANTIDADE DE CARACTERES ---------->>>>>>>>>>>%d\n", t);
		intBlob = 	db->GetTextBlob(texto);
       		printf("TEXTO : [%s] \n", texto);
        	t = 0;
	}	
				
   


    	tamanho = db->getDataSize(1);

    	unsigned char *dados1 = new unsigned char [tamanho];

    	memset (dados1, '\0', tamanho);

    	db->getColumn (dados1, 1);

    	printf("TEXTO : [%s] \n", dados1);
	ret = db->Fetch();
    } while (!ret);


    printf ("FETCH STATEMENT OK:\n");

    db->Commit();
    delete (db);
    db = NULL;
    
    return 0;
}

int t007_SelectA (void) {
    printf ("\n--------------------------\n007_SelectA STATMENT < 15    TEST\n");
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, FB30PASS);
    
    if (! db->Connected) {
        printf ("ERRO CONEXAO %s\n", db->ErrorMsg);
        return 1;
    }

    printf ("Banco conectado\n");

    db->Start(FBREAD);
    if (! db->InTrans) {
        printf ("ERRO Transacao de LEITURA\n");
        delete (db);
        db = NULL;
        return 1; }    
    printf ("Transacao de LEITURA  iniciada com sucesso!\n");

    const char *stmt= "SELECT * FROM ";

    if (db->Select (stmt)) {
        printf ("ERRO SELECT STATEMENT: %s\n%s", stmt, db->ErrorMsg);
        printf ("TEST SELECT STATEMENT < 15 OK!\n");
        delete (db);
        db = NULL;
        return 0; }    

    printf ("ERRO TESTE STATEMENT < 15 !\n");
    return -1;
}

int t008_SelectB (void) {
    printf ("\n--------------------------\n008_SelectB AutoTransaction    TEST\n");
    FBConnect *db = new FBConnect (FB30PATH, FB30USER, FB30PASS);
    
    if (! db->Connected) {
        printf ("ERRO CONEXAO %s\n", db->ErrorMsg);
        return 1;
    }

    printf ("Banco conectado\n");

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
*/
    if (t006_Select()    ){ printf ("\n006_Select     FAIL\n--------------------------\n\n"); return 0; }
    else                    printf ("\n006_Select     PASS\n--------------------------\n\n");
/*
    if (t007_SelectA()   ){ printf ("\n006_SelectA    FAIL\n--------------------------\n\n"); return 0; }
    else                    printf ("\n006_SelectA    PASS\n--------------------------\n\n");
    
    if (t008_SelectB()   ){ printf ("\n007_SelectB    FAIL\n--------------------------\n\n"); return 0; }
    else                    printf ("\n007_SelectB    PASS\n--------------------------\n\n");
*/    
    return 0;
}    

