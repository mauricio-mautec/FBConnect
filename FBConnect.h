#define FBCONNECT_VERSAO 0.1
// 27-DEC-17 M.JUNQUEIRA -

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>

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

#define OK '1'
#define NOTOK '2'

#define FBREAD  '1'
#define FBWRITE '2'

typedef struct fbcol {
    
    const char *name;
    unsigned   typeData;
    unsigned   subData; // if 0 binary, 1 text
    unsigned   length;
    //unsigned   blobLength;
    unsigned   offset;
} FBCOL;

struct fbc_tm
    {
      int tm_sec;
      int tm_min;
      int tm_hour;
      int tm_mday;
      int tm_mon;
      int tm_year;
      int tm_wday;
      int tm_yday;
      int tm_isdst;

    # ifdef __USE_BSD
      long int tm_gmtoff;       
      __const char *tm_zone;   
    # else
      long int __tm_gmtoff;   
      __const char *__tm_zone;
    # endif
    };




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

isc_blob_handle BlobHandler;
isc_db_handle   BlobDbHandle;
isc_tr_handle   BlobTransHandle;
short           BlobSegmentLen;
char            BlobSegment[8];
long            BlobStat;
int		BlobNumChar;
ISC_QUAD        BlobID;
unsigned char            BlobAddress; 



public:
unsigned        getCount (void);
char            ErrorMsg[356];

unsigned char   Status;
bool            Connected;
bool            InTrans;

bool 		isFetchOK;

IBlob*          blob;
//int  GetTextBlob    (char *); 
//int  GetTextBlob    (unsigned char *, int); 
string GetTextBlob ();

int  GetTextBlob    (unsigned char *); 
int  GetFileBlob    (FILE *blob);
int  SetTextBlob    (char *, char *, int);
int  SetFileBlob    (char *query, FILE *blob, int bloblen);
int  isBlob(int);
int  blobType(int);
//int  workBlob (unsigned char *);
int  workBlob (unsigned char *);



FBConnect (const char * banco, const char * user , const char * senha );
~FBConnect();

// INICIA UMA TRANSACAO RO/RW
int Start           (char readwrite);
int Rollback        (void);
int Commit          (void);
int CommitRetaining (void);

// SELECT COM RETORNO DE DADOS
//int getColumn(unsigned char *, int col);
string getColumn(int tam, int col);
int getRow          (unsigned char *);
int Fetch           (void);
int getSize         (void);
int getDataSize     (int col);
int Select          (const char *stmt);                               // RETURN MANY ROWS
int Execute         (const char *stmt);                               // RETURN ONE  ROWS 
int ExecuteBind     (const char *stmt, char **dados, int dimensao);   // EXECUTE MANY TIMES
int getBlobSize(IBlob *); 
}; //IbaseConnect
