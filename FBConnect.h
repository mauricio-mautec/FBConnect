#define FBCONNECT_VERSAO 0.1
// 27-DEC-17 M.JUNQUEIRA -

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(__cplusplus) && (__cplusplus >= 201103L)
#include <atomic>
typedef std::atomic_int FbSampleAtomic;
#else
typedef int FbSampleAtomic;
#endif

#include <ibase.h>
#include <firebird/Interface.h>

#if defined(_WIN32)
#define FB_DLL_EXPORT __declspec(dllexport)
#elif defined(__APPLE__)
#define FB_DLL_EXPORT __attribute__((visibility("default")))
#else
#define FB_DLL_EXPORT
#endif

using namespace Firebird;

#define SAMPLES_DIALECT SQL_DIALECT_V6
#define ZERA( a ) memset( a, '\0', sizeof( a ))

#define OK '1'
#define NOTOK '2'

#define FBREAD  '1'
#define FBWRITE '2'

typedef struct fbcol {
    const char *name;
    unsigned   length;
    unsigned   offset;
} FBCOL;

class FBConnect
{

private:
IStatus          *STATUS;
IProvider        *PROVIDER;
IUtil            *UTIL;
IXpbBuilder      *TPB_RO;
IXpbBuilder      *TPB_RW;
IAttachment      *ATTATCHMENT;
ITransaction     *TRANSACTION;
IResultSet       *CURSOR;
IMessageMetadata *METAI, *METAO;
IMetadataBuilder *BUILDER;
IStatement       *STATEMENT;

int Prepare (const char *stmt);

FBCOL          *FBCol;
unsigned char  *BufferData;
unsigned        BufferSize;   
unsigned        FCount;

public:
unsigned        getCount (void);
char            ErrorMsg[356];

unsigned char   Status;
bool            Connected;
bool            InTrans;


FBConnect (const char * banco, const char * user , const char * senha );
~FBConnect();

// INICIA UMA TRANSACAO RO/RW
int Start           (char readwrite);
int Rollback        (void);
int Commit          (void);
int CommitRetaining (void);

// SELECT COM RETORNO DE DADOS
int getRow          (unsigned char *);
int Fetch           (void);
int getSize         (void);
int Select          (const char *stmt);                               // RETURN MANY ROWS
int Execute         (const char *stmt);                               // RETURN ONE  ROWS 
int ExecuteBind     (const char *stmt, char **dados, int dimensao);   // EXECUTE MANY TIMES
}; //IbaseConnect
