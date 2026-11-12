#ifndef FBCONNECT_ENV_H
#define FBCONNECT_ENV_H

// CONFIGURACAO DE CONEXAO DOS PROGRAMAS DE TESTE/EXEMPLO VIA VARIAVEIS DE AMBIENTE.
// NENHUMA CREDENCIAL FICA NO CODIGO: DEFINA-AS NO AMBIENTE OU EM .fbconnect.env
// (CARREGADO PELOS SCRIPTS test.sh / FBConnectTest.sh; VER .fbconnect.env.example)
//
//   FBCONNECT_DB      host:/caminho/banco.fdb  (PADRAO: 127.0.0.1:/var/lib/firebird/data/mautec.fdb)
//   FBCONNECT_DB_AUX  CAMINHO ALTERNATIVO PARA EVENTOS VIA BRIDGE DOCKER (OPCIONAL)
//   FBCONNECT_USER    USUARIO  (OBRIGATORIO)
//   FBCONNECT_PASS    SENHA    (OBRIGATORIO)

#include <stdlib.h>
#include <stdio.h>

static inline const char *fbEnv( const char *nome, const char *padrao )
{
    const char *v = getenv( nome );
    return ( v && v[0] ) ? v : padrao;
}

#define FBENV_DB      fbEnv( "FBCONNECT_DB",     "127.0.0.1:/var/lib/firebird/data/mautec.fdb" )
#define FBENV_DB_AUX  fbEnv( "FBCONNECT_DB_AUX", "172.18.0.3:/var/lib/firebird/data/mautec.fdb" )
#define FBENV_USER    fbEnv( "FBCONNECT_USER",   "" )
#define FBENV_PASS    fbEnv( "FBCONNECT_PASS",   "" )

// RETORNA false (E EXPLICA NO stderr) SE USUARIO OU SENHA NAO FORAM DEFINIDOS
static inline bool fbEnvOk( void )
{
    if ( FBENV_USER[0] && FBENV_PASS[0] ) return true;
    fprintf( stderr,
             "Defina FBCONNECT_USER e FBCONNECT_PASS (e opcionalmente FBCONNECT_DB)\n"
             "no ambiente ou em .fbconnect.env (modelo: .fbconnect.env.example).\n" );
    return false;
}

#endif // FBCONNECT_ENV_H
