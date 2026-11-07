#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <syslog.h>
#include "FBConnect.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;
static IMaster *Master = fb_get_master_interface();

FBConnect::FBConnect (const char * banco, const char * user, const char * senha, const char * charset)
{

    this->TPB_RO      = NULL; 
    this->TPB_RW      = NULL;
    this->STATUS      = NULL; 
    this->PROVIDER    = NULL;
    this->ATTATCHMENT = NULL;
    this->UTIL        = NULL;
    this->CURSOR      = NULL;
    this->METAI       = NULL;
    this->METAO       = NULL;
    this->BUILDER     = NULL;
    this->STATEMENT   = NULL;
    this->FBCol       = NULL;
    this->BufferData  = NULL;
    this->blob        = NULL;
    this->TRANSACTION = NULL;
    this->BufferSize  = 0;

    this->STATUS   = Master->getStatus();
    this->PROVIDER = Master->getDispatcher();
    this->UTIL     = Master->getUtilInterface();
    this->FCount   = 0;
    this->InTrans  = false;
    this->Connected = false;
    this->isFetchOK = false;
    this->Status    = NOTOK;
    this->BlobNumChar = 0;
    memset(&this->BlobID, 0, sizeof(this->BlobID));
    memset(this->BlobSegment, 0, sizeof(this->BlobSegment));
    memset(this->ErrorMsg, 0, sizeof(this->ErrorMsg));
  
    ThrowStatusWrapper status(this->STATUS);

    IXpbBuilder* dpb = NULL;
    dpb = this->UTIL->getXpbBuilder (&status, IXpbBuilder::DPB, NULL, 0);
    
    dpb->insertString (&status, isc_dpb_user_name, user);
    dpb->insertString (&status, isc_dpb_password, senha);
    if (charset && charset[0] != '\0') {
        dpb->insertString (&status, isc_dpb_lc_ctype, charset);
    }
    // Firebird 5.x native server-side binding for extended types (INT128, DECFLOAT, TIME ZONE)
    dpb->insertString (&status, isc_dpb_set_bind,
                       "INT128 TO VARCHAR; DECFLOAT TO VARCHAR; TIME ZONE TO VARCHAR");

    try { 
        this->ATTATCHMENT = this->PROVIDER->attachDatabase (&status, 
                                                              banco, 
                                      dpb->getBufferLength(&status),
                                            dpb->getBuffer(&status));
        dpb->dispose();
        dpb = NULL;

        // PREPARA TPB RO (Firebird 5.x Read Committed Read Consistency / Rec Version)
        this->TPB_RO = this->UTIL->getXpbBuilder (&status, IXpbBuilder::TPB, NULL, 0);
        this->TPB_RO->insertTag (&status, isc_tpb_read_committed);
        this->TPB_RO->insertTag (&status, isc_tpb_rec_version);
        this->TPB_RO->insertTag (&status, isc_tpb_wait);
        this->TPB_RO->insertTag (&status, isc_tpb_read);

        // PREPARA TPB RW (Firebird 5.x Read Committed Read Consistency / Rec Version)
        this->TPB_RW = this->UTIL->getXpbBuilder (&status, IXpbBuilder::TPB, NULL, 0);
        this->TPB_RW->insertTag (&status, isc_tpb_read_committed);
        this->TPB_RW->insertTag (&status, isc_tpb_rec_version);
        this->TPB_RW->insertTag (&status, isc_tpb_wait);
        this->TPB_RW->insertTag (&status, isc_tpb_write);
        
        // Configura sessao Firebird 5.x para converter tipos estendidos (INT128, DECFLOAT, TIME ZONE)
        ITransaction* initTr = this->ATTATCHMENT->startTransaction(&status,
                                   this->TPB_RW->getBufferLength(&status),
                                   this->TPB_RW->getBuffer(&status));
        this->ATTATCHMENT->execute(&status, initTr, 0, "SET BIND OF INT128 TO VARCHAR", SAMPLES_DIALECT, NULL, NULL, NULL, NULL);
        this->ATTATCHMENT->execute(&status, initTr, 0, "SET BIND OF DECFLOAT TO VARCHAR", SAMPLES_DIALECT, NULL, NULL, NULL, NULL);
        this->ATTATCHMENT->execute(&status, initTr, 0, "SET BIND OF TIME ZONE TO VARCHAR", SAMPLES_DIALECT, NULL, NULL, NULL, NULL);
        initTr->commit(&status);

        this->Connected = true;
        this->Status    = OK;
    }
    catch (const FbException& error) {                                                                                                                                         
        char buf[354];                                                                                                                            
        
        this->UTIL->formatStatus (buf, sizeof(buf), error.getStatus());                                                                                   
        snprintf (this->ErrorMsg, sizeof(this->ErrorMsg), "%s\n", buf);                                                                                                             
        
        if (dpb)               dpb->dispose(); 
        this->Connected = false;
        this->Status    = NOTOK;
        if (this->TPB_RO)      this->TPB_RO->dispose();
        if (this->TPB_RW)      this->TPB_RW->dispose();
        if (this->ATTATCHMENT) this->ATTATCHMENT->release();
        if (this->PROVIDER)    this->PROVIDER->release();
        if (this->STATUS)      this->STATUS->dispose();
        this->TPB_RO      = NULL; 
        this->TPB_RW      = NULL;
        this->STATUS      = NULL; 
        this->PROVIDER    = NULL;
        this->ATTATCHMENT = NULL;
        this->STATEMENT   = NULL;
        this->UTIL        = NULL;

    }

    printf ("FBCONNECT VERSAO: %.1f (Firebird 5.x)\n", FBCONNECT_VERSAO);
}

void FBConnect::closeQueryResources (IStatus *status)
{
    ThrowStatusWrapper st(status);
    try {
        if (this->blob)      { this->blob->close(&st);     this->blob = NULL; }
        if (this->CURSOR)    { this->CURSOR->close(&st);   this->CURSOR = NULL; }
        if (this->STATEMENT) { this->STATEMENT->free(&st); this->STATEMENT = NULL; }
    }
    catch (const FbException&) {
        if (this->blob)      { this->blob->release();      this->blob = NULL; }
        if (this->CURSOR)    { this->CURSOR->release();    this->CURSOR = NULL; }
        if (this->STATEMENT) { this->STATEMENT->release(); this->STATEMENT = NULL; }
    }

    if (this->METAO)      { this->METAO->release();      this->METAO = NULL; }
    if (this->METAI)      { this->METAI->release();      this->METAI = NULL; }
    if (this->BUILDER)    { this->BUILDER->release();    this->BUILDER = NULL; }
    if (this->BufferData) { delete[] this->BufferData;   this->BufferData = NULL; }
    if (this->FBCol)      { delete[] this->FBCol;        this->FBCol = NULL; }

    this->BufferSize = 0;
    this->FCount     = 0;
    this->isFetchOK  = false;
}

FBConnect::~FBConnect()
{
    if (this->STATUS) {
        this->closeQueryResources(this->STATUS);
        ThrowStatusWrapper status(this->STATUS);
        try {
            if (this->TRANSACTION) { this->TRANSACTION->rollback(&status); this->TRANSACTION = NULL; }
            if (this->ATTATCHMENT) { this->ATTATCHMENT->detach(&status);   this->ATTATCHMENT = NULL; }
        }
        catch (const FbException&) {
            if (this->TRANSACTION) { this->TRANSACTION->release(); this->TRANSACTION = NULL; }
            if (this->ATTATCHMENT) { this->ATTATCHMENT->release(); this->ATTATCHMENT = NULL; }
        }
    }

    if (this->ATTATCHMENT) this->ATTATCHMENT->release();
    if (this->PROVIDER)    this->PROVIDER->release();
    if (this->TPB_RO)      this->TPB_RO->dispose();
    if (this->TPB_RW)      this->TPB_RW->dispose();
    if (this->STATUS)      this->STATUS->dispose();

    this->TPB_RO      = NULL; 
    this->TPB_RW      = NULL;
    this->STATUS      = NULL; 
    this->PROVIDER    = NULL;
    this->ATTATCHMENT = NULL;
    this->UTIL        = NULL;
    this->InTrans     = false;
    this->Connected   = false;
    printf ("DESTRUCTOR OK\n");
}

// INICIA UMA TRANSACAO RO/RW
int FBConnect::Start (char readwrite)
{
    if (! this->Connected) return -1;
   
    if (this->InTrans) return 0;

    ThrowStatusWrapper status(this->STATUS);
    
    try {
        if (readwrite == FBREAD)
            this->TRANSACTION = this->ATTATCHMENT->startTransaction (
                                &status, 
                                this->TPB_RO->getBufferLength(&status),
                                this->TPB_RO->getBuffer(&status));
        else                        
            this->TRANSACTION = this->ATTATCHMENT->startTransaction (
                                &status, 
                                this->TPB_RW->getBufferLength(&status),
                                this->TPB_RW->getBuffer(&status));
    }
    catch (const FbException& error) {
        char buf[354];
        this->UTIL->formatStatus (buf, sizeof(buf), error.getStatus());
        snprintf (this->ErrorMsg, sizeof(this->ErrorMsg), "%s\n", buf);
        this->InTrans = false;
        this->TRANSACTION = NULL;
        return -1;
    }

    this->InTrans = true;
    return 0;
}

int FBConnect::Commit (void)
{
    if (! this->InTrans || this->TRANSACTION == 0L) return -1;

    ThrowStatusWrapper status(this->STATUS);

    try {
        this->closeQueryResources(this->STATUS);
        this->TRANSACTION->commit (&status);
        this->InTrans     = false;
        this->TRANSACTION = NULL;
        return 0;
    }
    catch (const FbException& error) {                                                                                                                                         
        char buf[354];
        this->UTIL->formatStatus (buf, sizeof(buf), error.getStatus());
        snprintf (this->ErrorMsg, sizeof(this->ErrorMsg), "%s\n", buf);
        return -1;
    }
}

int FBConnect::CommitRetaining (void)
{
    if (! this->InTrans || this->TRANSACTION == 0L) return -1;

    ThrowStatusWrapper status(this->STATUS);

    try {
        this->TRANSACTION->commitRetaining (&status);
        return 0;
    }
    catch (const FbException& error) {                                                                                                                                         
        char buf[354];
        this->UTIL->formatStatus (buf, sizeof(buf), error.getStatus());
        snprintf (this->ErrorMsg, sizeof(this->ErrorMsg), "%s\n", buf);
        return -1;
    }
}

/* PRIVATE: PREPARE
** PREPARA CAMPOS DE RETORNO
** DOS DADOS SOLICITADOS NO STATEMENT
*/
int FBConnect::Prepare (const char *stmt)
{
    if (! this->InTrans || this->TRANSACTION == 0L) return -1;

    ThrowStatusWrapper status(this->STATUS);

    this->closeQueryResources(this->STATUS);

    try {
        this->STATEMENT = this->ATTATCHMENT->prepare (&status,
                                            this->TRANSACTION,
                                                            0,
                                                         stmt, 
                                               SAMPLES_DIALECT, //SQL_DIALECT_V6,
                         IStatement::PREPARE_PREFETCH_METADATA);
    }
    catch (const FbException &error) {
        this->UTIL->formatStatus (this->ErrorMsg, sizeof (this->ErrorMsg), error.getStatus());
        return -1;
    }

   // GETOUTPUT METADATA

    try {
        // NUMBER OF COLUMNS, NAMES AND DATA TYPES
        this->METAO    = this->STATEMENT->getOutputMetadata (&status);
        this->BUILDER  = this->METAO->getBuilder            (&status);
        this->FCount   = this->METAO->getCount              (&status);
        this->FBCol    = new FBCOL [this->FCount];

        memset (this->FBCol, 0 , sizeof (FBCOL) * this->FCount);

        for (unsigned i = 0; i < this->FCount; ++i) {
            this->FBCol[i].name = this->METAO->getField (&status, i);
            unsigned sub = 1;
            unsigned t   = this->METAO->getType (&status, i);
            this->FBCol[i].typeData = t;
            switch (t) {
            case SQL_VARYING:
                this->BUILDER->setType (&status, i, SQL_VARYING);
                break;
            case SQL_TEXT:
                this->BUILDER->setType (&status, i, SQL_TEXT);
                break;
            case SQL_SHORT:
                this->BUILDER->setType (&status, i, SQL_SHORT);
                break;
            case SQL_LONG:
                this->BUILDER->setType (&status, i, SQL_LONG);
                break;                              
            case SQL_INT64:                         
                this->BUILDER->setType (&status, i, SQL_INT64);
                break;
            case SQL_FLOAT:
                this->BUILDER->setType (&status, i, SQL_FLOAT);
                break;                             
            case SQL_DOUBLE:                       
                this->BUILDER->setType (&status, i, SQL_DOUBLE);
                break;                             
            case SQL_TIMESTAMP:                    
                this->BUILDER->setType (&status, i, SQL_TIMESTAMP);
                break;                             
            case SQL_TYPE_DATE:                    
                this->BUILDER->setType (&status, i, SQL_TYPE_DATE);
                break;
            case SQL_TYPE_TIME:
                this->BUILDER->setType (&status, i, SQL_TYPE_TIME);
                break;                             
            case SQL_BLOB:                         
                this->BUILDER->setType (&status, i, SQL_BLOB);
                sub = this->METAO->getSubType(&status, i); 
                this->FBCol[i].subData = sub;
                break;                             
            case SQL_ARRAY:                        
                this->BUILDER->setType (&status, i, SQL_ARRAY);
                break;                               
            case SQL_BOOLEAN:
                this->BUILDER->setType (&status, i, SQL_BOOLEAN);
                break;
            case SQL_INT128:
            case SQL_DEC16:
            case SQL_DEC34:
            case SQL_TIME_TZ:
            case SQL_TIMESTAMP_TZ:
            case SQL_TIME_TZ_EX:
            case SQL_TIMESTAMP_TZ_EX:
                // Firebird 5.x extended types: coerce to SQL_VARYING if not already bound by DPB
                this->BUILDER->setType (&status, i, SQL_VARYING);
                this->BUILDER->setLength (&status, i, 64);
                this->FBCol[i].typeData = SQL_VARYING;
                break;
            default:
                return -1;
            }                                        
            
        }                                          

        // FREE METADATA, BUILDER
        // RECEIVE NOVAMENTE AGAIN WITH 
        // DEFINED FIELDS
        this->METAO->release();
        this->METAO   = this->BUILDER->getMetadata (&status);
        this->BUILDER->release();
        this->BUILDER = NULL;

        for (unsigned i = 0; i < this->FCount; ++i) {
            if (FBCol[i].name)
            {
                this->FBCol[i].length     = this->METAO->getLength     (&status, i);
                this->FBCol[i].offset     = this->METAO->getOffset     (&status, i);
                this->FBCol[i].nullOffset = this->METAO->getNullOffset (&status, i);
            }
        }

        // OPEN CURSOR
        // ALLOCATE BUFFER, SET BUFFERDATA, BUFFERSIZE
        this->CURSOR     = this->STATEMENT->openCursor (&status, this->TRANSACTION, NULL , NULL, this->METAO, 0);
        this->BufferSize = this->METAO->getMessageLength (&status);
    }
    catch (const FbException &error) {
        this->UTIL->formatStatus (this->ErrorMsg, sizeof (this->ErrorMsg), error.getStatus());
        return -1;
    }

    if (! this->BufferSize) {
        snprintf (this->ErrorMsg, sizeof(this->ErrorMsg), "BUFFER SIZE IS ZERO");
        return -1;
    }
   
    this->BufferData = new unsigned char [this->BufferSize + 1];
    if (this->BufferData == NULL)  {
        snprintf (this->ErrorMsg, sizeof(this->ErrorMsg), "UNABLE TO ALLOCATE BUFFER");
        return -1;

    }
    memset(this->BufferData, 0, this->BufferSize + 1);

    return 0;
}

/* PRIVATE: FETCH
** LOAD A ROW OF DATA INTO BUFFER
*/
int FBConnect::Fetch(void)
{   
    int Result;

    if( ! this->isFetchOK || ! this->CURSOR || ! this->BufferData ) {
        return -1;
    }

    ThrowStatusWrapper status(this->STATUS);

    try {
        Result = this->CURSOR->fetchNext(&status, this->BufferData);
    }
    catch (const FbException &error) {
        this->UTIL->formatStatus (this->ErrorMsg, sizeof (this->ErrorMsg), error.getStatus());
        return -1;
    }

    switch (Result) {
        case IStatus::RESULT_OK:      return    0;
        case IStatus::RESULT_NO_DATA: return 100L;
        case IStatus::RESULT_ERROR:   return   -1; 
    
        default:
        {
            return -1;
        } 
    }
}


int FBConnect::Rollback        (void)
{
    if (! this->InTrans || this->TRANSACTION == 0L) return -1;

    ThrowStatusWrapper status(this->STATUS);
    if (this->TRANSACTION != 0L) {
        try {
            this->closeQueryResources(this->STATUS);
            this->TRANSACTION->rollback(&status);
            this->TRANSACTION = 0L;
            this->InTrans     = false;
            return 0;
        }
        catch (const FbException &error) {
            this->UTIL->formatStatus (this->ErrorMsg, sizeof (this->ErrorMsg), error.getStatus());
            return -1;
        }
    }
    else {
        snprintf(this->ErrorMsg, sizeof(this->ErrorMsg), "RollBack: Aborting Transaction");
        return -25;
    }
} 

/* PUBLIC: GETSIZE
** RETURN BUFFERSIZE VALUE
*/
int FBConnect::getSize ()
{
    if (! this->InTrans || this->TRANSACTION == 0L || ! this->BufferData || ! this->FBCol) return -1;
 
    ThrowStatusWrapper status(this->STATUS);

    if (this->blob) {
        this->blob->close(&status);
        this->blob = NULL;
    }

    this->blob = this->ATTATCHMENT->openBlob(&status, this->TRANSACTION, (ISC_QUAD*)(this->BufferData), 0, NULL);

    this->BlobNumChar = this->getBlobSize(this->blob);

    return this->BufferSize;
}

/* PUBLIC: GETCOLUMN
** RETURN COLUMN VALUE AS STD::STRING (FIREBIRD 5.x)
*/
string FBConnect::getColumn(int col)
{
    return this->getColumn(0, col);
}

string FBConnect::getColumn(int tam, int col)
{
    (void)tam;
    if (! this->InTrans    ||   this->TRANSACTION == 0L) return "-1" ;
    if (! this->BufferData || ! this->BufferSize)        return "-1";
    if (! this->FBCol      || col < 0 || (unsigned)col >= this->FCount) return "-1";

    ThrowStatusWrapper status(this->STATUS);

    if( ! this->isFetchOK ) {
        return "-1";
    }

    // Verifica indicador de NULL do Firebird (ISC_SHORT != 0 significa NULL)
    if (*(ISC_SHORT *)(this->BufferData + this->FBCol[col].nullOffset) != 0) {
        return "";
    }
 
    unsigned t = FBCol[col].typeData;
    short dscale = 0;
    unsigned year = 0, month = 0, day = 0;
    unsigned hours = 0, minutes = 0, seconds = 0, fractions = 0;
    char buf[128];
    memset(buf, 0, sizeof(buf));

    ISC_QUAD bid;
    memset(&bid, 0, sizeof(bid));

    string fc_a;	
    ISC_INT64 value = 0;
    unsigned char *colPtr = this->BufferData + this->FBCol[col].offset;

    switch (t)
    {
        case SQL_VARYING:
        {
            // Em SQL_VARYING do Firebird, os 2 primeiros bytes (uint16_t) contem o tamanho exato em bytes
            uint16_t strLen = *(const uint16_t *)colPtr;
            if (strLen > this->FBCol[col].length) {
                strLen = (uint16_t)this->FBCol[col].length;
            }
            fc_a.assign((const char *)(colPtr + sizeof(uint16_t)), strLen);
            break;
        }

        case SQL_TEXT:
        {
            // Em SQL_TEXT (CHAR(N)) com UTF8, o Firebird preenche ate 4*N bytes com espacos
            const char *raw = (const char *)colPtr;
            unsigned len = this->FBCol[col].length;
            while (len > 0 && raw[len - 1] == ' ') {
                len--;
            }
            fc_a.assign(raw, len);
            break;
        }

        case SQL_BOOLEAN:
            fc_a = (*colPtr != 0) ? "true" : "false";
            break;
		
        case SQL_SHORT:
        {
            dscale = this->METAO->getScale(&status, col);
            short sVal = *(const short *)colPtr;
            if (dscale < 0) {
                value = sVal;
            } else {
                snprintf(buf, sizeof(buf), "%d", sVal);
                fc_a = buf;
            }
            break;
        }
		
        case SQL_LONG:
        {
            dscale = this->METAO->getScale(&status, col);
            ISC_LONG lVal = *(const ISC_LONG *)colPtr;
            if (dscale < 0) {
                value = lVal;
            } else {
                snprintf(buf, sizeof(buf), "%ld", (long)lVal);
                fc_a = buf;
            }
            break;
        }
		
        case SQL_INT64:
            dscale = this->METAO->getScale (&status, col);
            value = (ISC_INT64) *(const ISC_INT64 ISC_FAR *)colPtr;
            if (dscale == 0) {
                snprintf(buf, sizeof(buf), "%" ISC_INT64_FORMAT "d", value);
                fc_a = buf;
            }
            break;
		
        case SQL_FLOAT:
            snprintf(buf, sizeof(buf), "%.6g", *(const float ISC_FAR *)colPtr);
            fc_a = buf;
            break;

        case SQL_DOUBLE:
            snprintf(buf, sizeof(buf), "%.6f", *(const double ISC_FAR *)colPtr);
            fc_a = buf;
            break;
		
        case SQL_TIMESTAMP:
            this->UTIL->decodeDate(((const ISC_TIMESTAMP ISC_FAR *)colPtr)->timestamp_date, &year, &month, &day);
            this->UTIL->decodeTime(((const ISC_TIMESTAMP ISC_FAR *)colPtr)->timestamp_time, &hours, &minutes, &seconds, &fractions);
            snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u.%04u", year, month, day, hours, minutes, seconds, fractions);
            fc_a = buf;
            break;

        case SQL_TYPE_DATE:
            this->UTIL->decodeDate(*(const ISC_DATE ISC_FAR *)colPtr, &year, &month, &day);
            snprintf(buf, sizeof(buf), "%04u-%02u-%02u", year, month, day);
            fc_a = buf;
            break;

        case SQL_TYPE_TIME:
            this->UTIL->decodeTime(*(const ISC_TIME ISC_FAR *)colPtr, &hours, &minutes, &seconds, &fractions);
            snprintf(buf, sizeof(buf), "%02u:%02u:%02u.%04u", hours, minutes, seconds, fractions);
            fc_a = buf;
            break;

        case SQL_ARRAY:
        case SQL_BLOB:
            bid = *(const ISC_QUAD *)colPtr;
            if (this->blob) {
                this->blob->close(&status);
                this->blob = NULL;
            }
            this->blob = this->ATTATCHMENT->openBlob(&status, this->TRANSACTION, (ISC_QUAD*)colPtr, 0, NULL);

            this->BlobID.gds_quad_high = bid.isc_quad_high;
            this->BlobID.gds_quad_low  = bid.isc_quad_low;

            snprintf(buf, sizeof(buf), "%08x:%08x", (unsigned int)bid.isc_quad_high, (unsigned int)bid.isc_quad_low);
            fc_a = buf;
            break;

        default:
            break;
    }		

    if ((t == SQL_INT64 || t == SQL_LONG || t == SQL_SHORT) && dscale < 0 && dscale >= -18) {
        int fracDigits = -dscale;
        ISC_INT64 tens = 1;
        for (int i = 0; i < fracDigits; i++) {
            tens *= 10;
        }

        bool isNeg = (value < 0);
        ISC_INT64 absVal   = isNeg ? -value : value;
        ISC_INT64 intPart  = absVal / tens;
        ISC_INT64 fracPart = absVal % tens;

        snprintf(buf, sizeof(buf), "%s%" ISC_INT64_FORMAT "d.%0*" ISC_INT64_FORMAT "d",
                 isNeg ? "-" : "", intPart, fracDigits, fracPart);
        fc_a = buf;
    }

    return fc_a;
}


/* PUBLIC: GETROW
** COPY FROM BUFFER TO USER VAR
*/
int FBConnect::getRow (unsigned char *dados)
{
    (void)dados;
    if (! this->InTrans    ||   this->TRANSACTION == 0L) return -1;
    if (! this->BufferData || ! this->BufferSize)        return -1;

    for (unsigned j = 0; j < this->FCount; ++j) {
        printf("%s: %s\n", this->FBCol[j].name ? this->FBCol[j].name : "", this->getColumn(j).c_str());
    }

    return 0;
}

/* PUBLIC : SELECT
** SELECIONAR VARIAS LINHAS DE DADOS
*/
int FBConnect::Select (const char *stmt)
{
    // 0. VERIFICA STATMENT
    if (!stmt || strlen (stmt) < 15) { 
        snprintf (this->ErrorMsg, sizeof(this->ErrorMsg), "FBConnect::Select: VERIFY STATMENT LESS THAN 15 CHARS");
        return -1;
    }

    // 1. VERIFICA STATUS DA TRANSACAO E PREPARA STATEMENT
    // Iniciar automaticamente uma transacao
    if (! this->InTrans && this->Start (FBREAD)) return -1;
    
    if (this->Prepare (stmt)) {
        if (this->ErrorMsg[0] == '\0') {
            snprintf (this->ErrorMsg, sizeof(this->ErrorMsg), "FBConnect::Select:Prepare: UNABLE TO SETUP STATEMENT");
        }
        return -1;
    }

    this->isFetchOK = true;

    return 0;
}

int FBConnect::Execute (const char *stmt)
{
    if (!stmt || strlen(stmt) == 0) {
        snprintf(this->ErrorMsg, sizeof(this->ErrorMsg), "FBConnect::Execute: EMPTY STATEMENT");
        return -1;
    }

    if (!this->InTrans && this->Start(FBWRITE)) return -1;

    ThrowStatusWrapper status(this->STATUS);

    try {
        this->ATTATCHMENT->execute(&status,
                                   this->TRANSACTION,
                                   0,
                                   stmt,
                                   SAMPLES_DIALECT,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL);
        return 0;
    }
    catch (const FbException &error) {
        this->UTIL->formatStatus(this->ErrorMsg, sizeof(this->ErrorMsg), error.getStatus());
        return -1;
    }
}

unsigned FBConnect::getCount (void)
{
    return this->FCount;
}

const char* FBConnect::getColumnName (int col)
{
    if (!this->FBCol || col < 0 || (unsigned)col >= this->FCount) return "";
    return this->FBCol[col].name ? this->FBCol[col].name : "";
}

string FBConnect::getServerVersion (void)
{
    if (!this->Connected || !this->ATTATCHMENT) return "";

    bool startedHere = false;
    if (!this->InTrans) {
        if (this->Start(FBREAD) != 0) return "";
        startedHere = true;
    }

    string ver;
    if (this->Select("SELECT RDB$GET_CONTEXT('SYSTEM', 'ENGINE_VERSION') FROM RDB$DATABASE") == 0) {
        if (this->Fetch() == 0) {
            ver = this->getColumn(0);
        }
    }

    if (startedHere) {
        this->Commit();
    }
    return ver;
}

int FBConnect::ExecuteBind (const char *stmt, const char **dados, int dimensao)
{
    if (!stmt || strlen(stmt) == 0 || !dados || dimensao <= 0) {
        snprintf(this->ErrorMsg, sizeof(this->ErrorMsg), "FBConnect::ExecuteBind: INVALID ARGUMENTS");
        return -1;
    }

    if (!this->InTrans && this->Start(FBWRITE)) return -1;

    ThrowStatusWrapper status(this->STATUS);
    IStatement *stmtObj = NULL;
    IMessageMetadata *inMeta = NULL;
    IMetadataBuilder *inBuilder = NULL;
    unsigned char *inBuf = NULL;

    try {
        stmtObj = this->ATTATCHMENT->prepare(&status,
                                             this->TRANSACTION,
                                             0,
                                             stmt,
                                             SAMPLES_DIALECT,
                                             IStatement::PREPARE_PREFETCH_METADATA);

        inMeta = stmtObj->getInputMetadata(&status);
        unsigned paramCount = inMeta->getCount(&status);
        if ((int)paramCount != dimensao) {
            snprintf(this->ErrorMsg, sizeof(this->ErrorMsg),
                     "FBConnect::ExecuteBind: EXPECTED %u PARAMS, GOT %d", paramCount, dimensao);
            inMeta->release();
            stmtObj->free(&status);
            return -1;
        }

        inBuilder = inMeta->getBuilder(&status);
        for (unsigned i = 0; i < paramCount; ++i) {
            unsigned strLen = dados[i] ? (unsigned)strlen(dados[i]) : 1;
            if (strLen == 0) strLen = 1;
            inBuilder->setType(&status, i, SQL_TEXT);
            inBuilder->setLength(&status, i, strLen);
        }

        inMeta->release();
        inMeta = inBuilder->getMetadata(&status);
        inBuilder->release();
        inBuilder = NULL;

        unsigned msgLen = inMeta->getMessageLength(&status);
        inBuf = new unsigned char[msgLen];
        memset(inBuf, 0, msgLen);

        for (unsigned i = 0; i < paramCount; ++i) {
            unsigned offset     = inMeta->getOffset(&status, i);
            unsigned nullOffset = inMeta->getNullOffset(&status, i);
            if (dados[i] == NULL) {
                *(ISC_SHORT *)(inBuf + nullOffset) = -1;
            } else {
                *(ISC_SHORT *)(inBuf + nullOffset) = 0;
                memcpy(inBuf + offset, dados[i], strlen(dados[i]));
            }
        }

        stmtObj->execute(&status, this->TRANSACTION, inMeta, inBuf, NULL, NULL);

        delete[] inBuf;
        inMeta->release();
        stmtObj->free(&status);
        return 0;
    }
    catch (const FbException &error) {
        this->UTIL->formatStatus(this->ErrorMsg, sizeof(this->ErrorMsg), error.getStatus());
        if (inBuf)     delete[] inBuf;
        if (inBuilder) inBuilder->release();
        if (inMeta)    inMeta->release();
        if (stmtObj)   stmtObj->release();
        return -1;
    }
}

int FBConnect::ExecuteBind (const char *stmt, char **dados, int dimensao)
{
    return this->ExecuteBind(stmt, (const char **)dados, dimensao);
}

/* PUBLIC : GetTextBlob
** RETURN THE CHARACTERS OF A BLOB TEXT COLUMN
*/
string FBConnect::GetTextBlob ()
{
    if (! this->InTrans    ||   this->TRANSACTION == 0L) return "Error";
    if (! this->BufferData || ! this->BufferSize)        return "Error";
    if (! this->blob)                                    return "Error";

    ThrowStatusWrapper status(this->STATUS);
    unsigned len = 0;
    string d;

    memset (this->BlobSegment, '\0', sizeof (this->BlobSegment));

    try {
        for(;;)
        {
            int cc = this->blob->getSegment(&status, sizeof(this->BlobSegment), this->BlobSegment, &len);
            
            if (cc != IStatus::RESULT_OK && cc != IStatus::RESULT_SEGMENT)
                break;
            
            d.append(this->BlobSegment, len);
        }
        this->blob->close(&status);
        this->blob = NULL;
    }
    catch (const FbException &error) {
        this->UTIL->formatStatus (this->ErrorMsg, sizeof (this->ErrorMsg), error.getStatus());
        if (this->blob) {
            this->blob->release();
            this->blob = NULL;
        }
        return "Error";
    }

    return d;
}

int FBConnect::GetTextBlob (unsigned char *buf)
{
    if (!buf) return -1;
    string s = this->GetTextBlob();
    if (s == "Error") return -1;
    memcpy(buf, s.data(), s.size());
    buf[s.size()] = '\0';
    return (int)s.size();
}

int FBConnect::GetFileBlob (FILE *file)
{
    if (!file || !this->InTrans || this->TRANSACTION == 0L || !this->blob) return -1;

    ThrowStatusWrapper status(this->STATUS);
    unsigned len = 0;
    int totalWritten = 0;

    try {
        for (;;) {
            int cc = this->blob->getSegment(&status, sizeof(this->BlobSegment), this->BlobSegment, &len);
            if (cc != IStatus::RESULT_OK && cc != IStatus::RESULT_SEGMENT)
                break;
            if (len > 0) {
                fwrite(this->BlobSegment, 1, len, file);
                totalWritten += (int)len;
            }
        }
        this->blob->close(&status);
        this->blob = NULL;
        fflush(file);
        return totalWritten;
    }
    catch (const FbException &error) {
        this->UTIL->formatStatus(this->ErrorMsg, sizeof(this->ErrorMsg), error.getStatus());
        if (this->blob) {
            this->blob->release();
            this->blob = NULL;
        }
        return -1;
    }
}

int FBConnect::SetTextBlob (const char *query, const char *texto, int bloblen)
{
    if (!query || !texto || bloblen < 0) return -1;
    if (!this->InTrans && this->Start(FBWRITE)) return -1;

    ThrowStatusWrapper status(this->STATUS);
    IBlob *newBlob = NULL;
    IStatement *stmtObj = NULL;
    IMessageMetadata *inMeta = NULL;
    IMetadataBuilder *inBuilder = NULL;
    unsigned char *inBuf = NULL;

    try {
        ISC_QUAD newBlobId;
        memset(&newBlobId, 0, sizeof(newBlobId));
        newBlob = this->ATTATCHMENT->createBlob(&status, this->TRANSACTION, &newBlobId, 0, NULL);

        int remaining = bloblen;
        const char *ptr = texto;
        while (remaining > 0) {
            unsigned chunk = (remaining > FB_BLOB_SEGMENT_SIZE) ? FB_BLOB_SEGMENT_SIZE : (unsigned)remaining;
            newBlob->putSegment(&status, chunk, ptr);
            ptr       += chunk;
            remaining -= (int)chunk;
        }
        newBlob->close(&status);
        newBlob = NULL;

        stmtObj = this->ATTATCHMENT->prepare(&status, this->TRANSACTION, 0, query, SAMPLES_DIALECT,
                                             IStatement::PREPARE_PREFETCH_METADATA);
        inMeta = stmtObj->getInputMetadata(&status);
        inBuilder = inMeta->getBuilder(&status);
        inBuilder->setType(&status, 0, SQL_BLOB);
        inBuilder->setLength(&status, 0, sizeof(ISC_QUAD));
        inMeta->release();
        inMeta = inBuilder->getMetadata(&status);
        inBuilder->release();
        inBuilder = NULL;

        unsigned msgLen = inMeta->getMessageLength(&status);
        inBuf = new unsigned char[msgLen];
        memset(inBuf, 0, msgLen);
        *(ISC_SHORT *)(inBuf + inMeta->getNullOffset(&status, 0)) = 0;
        *(ISC_QUAD *)(inBuf + inMeta->getOffset(&status, 0)) = newBlobId;

        stmtObj->execute(&status, this->TRANSACTION, inMeta, inBuf, NULL, NULL);

        delete[] inBuf;
        inMeta->release();
        stmtObj->free(&status);
        return 0;
    }
    catch (const FbException &error) {
        this->UTIL->formatStatus(this->ErrorMsg, sizeof(this->ErrorMsg), error.getStatus());
        if (newBlob)   newBlob->release();
        if (inBuf)     delete[] inBuf;
        if (inBuilder) inBuilder->release();
        if (inMeta)    inMeta->release();
        if (stmtObj)   stmtObj->release();
        return -1;
    }
}

int FBConnect::SetFileBlob (const char *query, FILE *file, int bloblen)
{
    if (!query || !file || bloblen <= 0) return -1;
    char *buf = new char[bloblen];
    size_t nRead = fread(buf, 1, (size_t)bloblen, file);
    int rc = this->SetTextBlob(query, buf, (int)nRead);
    delete[] buf;
    return rc;
}

int FBConnect::getDataSize  (int col)
{
    if (! this->InTrans    ||   this->TRANSACTION == 0L) return -1;
    if (! this->BufferData || ! this->BufferSize)        return -1;
    if (! this->FBCol      || col < 0 || (unsigned)col >= this->FCount) return -1;
    if (! this->isFetchOK )                              return -1;

    return (int)this->FBCol[col].length;
}

/*
 * RETURN IF A COLUMN VALUE IS NULL IN CURRENT FETCHED ROW
 * 0 - NOT NULL
 * 1 - NULL
 * */
int FBConnect::isNull(int col) {
	if (!this->InTrans || !this->BufferData || !this->isFetchOK) return 0;
	if (!this->FBCol || col < 0 || (unsigned)col >= this->FCount) return 0;
	return (*(const ISC_SHORT *)(this->BufferData + this->FBCol[col].nullOffset) != 0) ? 1 : 0;
}

/*
 * RETURN IF A COLUMN IS BLOB OR NOT
 * 0 - FALSE
 * 1 - TRUE 
 * */
int FBConnect::isBlob(int col) {
	if (!this->FBCol || col < 0 || (unsigned)col >= this->FCount) return 0;

	unsigned t = FBCol[col].typeData;
	switch(t)
	{
		case SQL_BLOB:
			return 1;
		default:
			return 0;
	}
}

/*
 * RETURN IF is a bynary or a text
 * 0 - BINARY
 * 1 - TEXT
 * */
int FBConnect::blobType(int col)
{
	if (!this->FBCol || col < 0 || (unsigned)col >= this->FCount) return 0;
	return FBCol[col].subData;
}

/*
 * GET THE HANDLER OF BLOB AND THE SIZE OF THE CHARACTERS
 * */
int FBConnect::workBlob  (unsigned char * bid)
{
    if (! this->InTrans    ||   this->TRANSACTION == 0L) return -1;
    if (! this->BufferData || ! this->BufferSize)        return -1;

    ThrowStatusWrapper status(this->STATUS);

    if( ! this->isFetchOK ) {
        return -1;
    }

    try {
        if(!this->blob)
            this->blob = this->ATTATCHMENT->openBlob(&status, this->TRANSACTION, (ISC_QUAD *)bid, 0, NULL);
    }
    catch (const FbException &error) {
        this->UTIL->formatStatus (this->ErrorMsg, sizeof (this->ErrorMsg), error.getStatus());
        return -1;
    }

	return 1;
}

/*
 * GET THE NUMBER OF CHARACTERS OF A BLOB
 * */
int FBConnect:: getBlobSize(IBlob * handler)
{
	if (!handler) return 0;

        unsigned char bbchar[200], *p, item;

        static unsigned char items[] = {
        isc_info_blob_max_segment,
        isc_info_blob_total_length};

        ThrowStatusWrapper status(this->STATUS);
        handler->getInfo(&status, 2, items, sizeof(bbchar), bbchar );

	ISC_LONG max_size = 0L;
        ISC_LONG num_segments = 0L;
	int total_length = 0;
	(void)max_size;
	(void)num_segments;

        short length;
	this->BlobNumChar = 0;

        for (p = bbchar; *p != isc_info_end ;) {
               item = *p++;
               length = (short)isc_portable_integer(p, 2);
               p += 2;
               switch (item) {
                       case isc_info_blob_max_segment:
                               max_size = isc_portable_integer(p, length);
                               break;
                       case isc_info_blob_num_segments:
                               num_segments = isc_portable_integer(p, length);
                               break;
			case isc_info_blob_total_length:
			       total_length = isc_portable_integer(p, length);
			       break;
                        case isc_info_truncated:
                               break;
                        default:
                               break;
                }
                p += length;
         };

	this->BlobNumChar = total_length;
        return this->BlobNumChar;
}
