#ifndef FBCONNECT_H
#define FBCONNECT_H

#define FBCONNECT_VERSAO 5.0
// Ported to Firebird 5.x OO API

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <map>

#include <ibase.h>
#include <firebird/Interface.h>

#ifndef FB_DLL_EXPORT
#if defined(_WIN32)
#define FB_DLL_EXPORT __declspec(dllexport)
#elif defined(__APPLE__)
#define FB_DLL_EXPORT __attribute__((visibility("default")))
#else
#define FB_DLL_EXPORT
#endif
#endif

template <typename T>
T as(unsigned char* ptr)
{
	return *((T*) ptr);
}

using namespace Firebird;
using namespace std;
#define ISC_INT64_FORMAT	"ll" 
#define SAMPLES_DIALECT SQL_DIALECT_V6
#define ZERA( a ) memset( a, '\0', sizeof( a ))

#define FB_OK '1'
#ifndef OK
#define OK '1'
#endif
#define NOTOK '2'

#define FBREAD  '1'
#define FBWRITE '2'

// Firebird 5.x SQL Data Type & DPB Constants
#ifndef isc_dpb_set_bind
#define isc_dpb_set_bind     89
#endif
#ifndef SQL_BOOLEAN
#define SQL_BOOLEAN          32764
#endif
#ifndef SQL_DEC16
#define SQL_DEC16            32760
#endif
#ifndef SQL_DEC34
#define SQL_DEC34            32762
#endif
#ifndef SQL_INT128
#define SQL_INT128           32752
#endif
#ifndef SQL_TIME_TZ
#define SQL_TIME_TZ          32756
#endif
#ifndef SQL_TIMESTAMP_TZ
#define SQL_TIMESTAMP_TZ     32754
#endif
#ifndef SQL_TIME_TZ_EX
#define SQL_TIME_TZ_EX       32750
#endif
#ifndef SQL_TIMESTAMP_TZ_EX
#define SQL_TIMESTAMP_TZ_EX  32748
#endif

#define FB_BLOB_SEGMENT_SIZE 4096

typedef struct fbcol {
    
    const char *name;
    unsigned   typeData;
    unsigned   subData; // if 0 binary, 1 text
    unsigned   length;
    unsigned   offset;
    unsigned   nullOffset;
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
int setupOutputMetadata (IStatus *status);
void closeQueryResources (IStatus *status);
int connectInternal (void);

string          dbPath;
string          dbUser;
string          dbPass;
string          dbCharset;

FBCOL          *FBCol;
unsigned char  *BufferData;
unsigned        BufferSize;   
unsigned        FCount;
uint64_t        AffectedRows;

char            BlobSegment[FB_BLOB_SEGMENT_SIZE];
int             BlobNumChar;
ISC_QUAD        BlobID;

public:
unsigned        getCount (void);
uint64_t        getAffectedRows (void);
const char*     getColumnName (int col);
int             getColumnIndex (const char *colName);
string          getServerVersion (void);
bool            Ping (void);
int             Reconnect (void);
int64_t         getNextSequence (const char *seqName, int step = 1);
char            ErrorMsg[356];

unsigned char   Status;
bool            Connected;
bool            InTrans;
bool            isFetchOK;

IBlob*          blob;
string GetTextBlob ();

int  GetTextBlob    (unsigned char *); 
int  GetFileBlob    (FILE *blob);
int  SetTextBlob    (const char *query, const char *texto, int bloblen);
int  SetFileBlob    (const char *query, FILE *blob, int bloblen);
int  isBlob(int);
int  blobType(int);
int  isNull(int col);
int  isNull(const char *colName);
int  workBlob (unsigned char *);

FBConnect (const char * banco, const char * user , const char * senha, const char * charset = "UTF8");
~FBConnect();

// INICIA UMA TRANSACAO RO/RW
int Start           (char readwrite);
int Rollback        (void);
int Commit          (void);
int CommitRetaining (void);

// SELECT COM RETORNO DE DADOS
string getColumn(int col);
string getColumn(int tam, int col);
string getColumn(const char *colName);
long    getInt   (int col);
long    getInt   (const char *colName);
int64_t getInt64 (int col);
int64_t getInt64 (const char *colName);
double  getDouble(int col);
double  getDouble(const char *colName);
bool    getBool  (int col);
bool    getBool  (const char *colName);

vector<string>      getRowVector(void);
map<string, string> getRowMap   (void);
int getRow          (unsigned char *);
int Fetch           (void);
int getSize         (void);
int getDataSize     (int col);
int Select          (const char *stmt);                                     // RETURN MANY ROWS
int SelectBind      (const char *stmt, const char **dados, int dimensao);   // SELECT WITH PARAMETERS
int SelectBind      (const char *stmt, char **dados, int dimensao);         // SELECT WITH PARAMETERS
int Execute         (const char *stmt);                                     // EXECUTE DDL / DML
int ExecuteBind     (const char *stmt, const char **dados, int dimensao);   // EXECUTE WITH PARAMETERS
int ExecuteBind     (const char *stmt, char **dados, int dimensao);         // EXECUTE WITH PARAMETERS
int ExecuteBatch    (const char *stmt, const vector<vector<string>> &rows); // BATCH EXECUTION
int getBlobSize(IBlob *); 
}; //FBConnect

#endif // FBCONNECT_H
