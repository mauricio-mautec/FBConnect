#include "FBConnect.h"
int t001_Conexao25(void) {
    printf ("\n--------------------------\n001_Conexao25  TEST\nConectando no banco VERSAO 2.5\n");
    FBConnect *db = new FBConnect ("rondon:TESTE", "SYSDBA", "aeShoo0a");
    if (! db->Conectado) {
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
    FBConnect *db = new FBConnect ("ada:BSOICA3", "SYSDBA", "aeShoo0amaike7aF");

    if (! db->Conectado) {
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
    FBConnect *db = NULL;
    
    db = new FBConnect ("ada:BSOICA3", "SYSDBA", "aeshoo0amaike7aF");
    
    if (db->Conectado) {
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
    
    if (db->Conectado) {
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
    FBConnect *db = new FBConnect ("ada:employee", "SYSDBA", "aeShoo0amaike7aF");
    
    if (! db->Conectado) {
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
int t005_Select (void) {
    printf ("\n--------------------------\n005_Select     TEST\n");
    FBConnect *db = new FBConnect ("ada:employee", "SYSDBA", "aeShoo0amaike7aF");
    
    if (! db->Conectado) {
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

    char stmt[40];
    
    strcpy (stmt, "SELECT * FROM CUSTOMERS");

    if (db->Select (stmt)) {
        printf ("ERRO SELECT STATEMENT: %s\n", stmt);
        delete (db);
        db = NULL;
        return 1; }    

    printf ("SELECT STATEMENT OK!\n");

    char dados[400];
    memset (dados, '\0', sizeof (dados));
    int ret = db->Fetch (dados);
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

int main (void) {
    if (t001_Conexao25()) { printf ("\n001_Conexao25  FAIL\n--------------------------\n\n"); return 0; }
    else                    printf ("\n001_Conexao25  PASS\n--------------------------\n\n");
    if (t002_ConexaoOK()) { printf ("\n002_ConexaoOK  FAIL\n--------------------------\n\n"); return 0; }
    else                    printf ("\n002_ConexaoOK  PASS\n--------------------------\n\n");
    if (t003_ConexaoNOK()){ printf ("\n003_ConexaoNOK FAIL\n--------------------------\n\n"); return 0; }
    else                    printf ("\n003_ConexaoNOK PASS\n--------------------------\n\n");
    if (t004_StartTrans()){ printf ("\n004_StartTrans FAIL\n--------------------------\n\n"); return 0; }
    else                    printf ("\n004_StartTrans PASS\n--------------------------\n\n");
    if (t005_Select()    ){ printf ("\n005_Select     FAIL\n--------------------------\n\n"); return 0; }
    else                    printf ("\n005_Select     PASS\n--------------------------\n\n");
    
    return 0;
}    
