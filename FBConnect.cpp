#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/time.h>
#include <syslog.h>
#include "FBConnect.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;
static IMaster *Master = fb_get_master_interface();

static bool isDmlReturningStmt(const char *stmt)
{
    if (!stmt) return false;
    while (*stmt && isspace((unsigned char)*stmt)) stmt++;
    return (strncasecmp(stmt, "INSERT", 6) == 0 ||
            strncasecmp(stmt, "UPDATE", 6) == 0 ||
            strncasecmp(stmt, "DELETE", 6) == 0 ||
            strncasecmp(stmt, "MERGE",  5) == 0);
}

FBConnect::FBConnect (const char * banco, const char * user, const char * senha, const char * charset)
{
    this->TPB_RO       = NULL; 
    this->TPB_RW       = NULL;
    this->STATUS       = NULL; 
    this->PROVIDER     = NULL;
    this->ATTATCHMENT  = NULL;
    this->UTIL         = NULL;
    this->CURSOR       = NULL;
    this->METAI        = NULL;
    this->METAO        = NULL;
    this->BUILDER      = NULL;
    this->STATEMENT    = NULL;
    this->FBCol        = NULL;
    this->BufferData   = NULL;
    this->blob         = NULL;
    this->TRANSACTION  = NULL;
    this->BufferSize   = 0;
    this->FCount       = 0;
    this->AffectedRows = 0;
    this->InTrans      = false;
    this->Connected    = false;
    this->isFetchOK    = false;
    this->Status       = NOTOK;
    this->BlobNumChar  = 0;
    memset(&this->BlobID, 0, sizeof(this->BlobID));
    memset(this->BlobSegment, 0, sizeof(this->BlobSegment));
    memset(this->ErrorMsg, 0, sizeof(this->ErrorMsg));

    this->dbPath    = banco   ? banco   : "";
    this->dbUser    = user    ? user    : "";
    this->dbPass    = senha   ? senha   : "";
    this->dbCharset = charset ? charset : "UTF8";

    this->connectInternal();

    printf ("FBCONNECT VERSAO: %.1f (Firebird 5.x)\n", FBCONNECT_VERSAO);
}

int FBConnect::connectInternal (void)
{
    this->STATUS   = Master->getStatus();
    this->PROVIDER = Master->getDispatcher();
    this->UTIL     = Master->getUtilInterface();

    ThrowStatusWrapper status(this->STATUS);

    IXpbBuilder* dpb = NULL;
    dpb = this->UTIL->getXpbBuilder (&status, IXpbBuilder::DPB, NULL, 0);
    
    dpb->insertString (&status, isc_dpb_user_name, this->dbUser.c_str());
    dpb->insertString (&status, isc_dpb_password, this->dbPass.c_str());
    if (!this->dbCharset.empty()) {
        dpb->insertString (&status, isc_dpb_lc_ctype, this->dbCharset.c_str());
    }
    dpb->insertString (&status, isc_dpb_set_bind,
                       "INT128 TO VARCHAR; DECFLOAT TO VARCHAR; TIME ZONE TO VARCHAR");

    try { 
        this->ATTATCHMENT = this->PROVIDER->attachDatabase (&status, 
                                                            this->dbPath.c_str(), 
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
        return 0;
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
        return -1;
    }
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

bool FBConnect::Ping (void)
{
    if (!this->Connected || !this->ATTATCHMENT || !this->STATUS) return false;
    ThrowStatusWrapper status(this->STATUS);
    try {
        this->ATTATCHMENT->ping(&status);
        return true;
    }
    catch (const FbException &error) {
        this->UTIL->formatStatus(this->ErrorMsg, sizeof(this->ErrorMsg), error.getStatus());
        return false;
    }
}

int FBConnect::Reconnect (void)
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
    if (this->ATTATCHMENT) { this->ATTATCHMENT->release(); this->ATTATCHMENT = NULL; }
    if (this->PROVIDER)    { this->PROVIDER->release();    this->PROVIDER = NULL; }
    if (this->TPB_RO)      { this->TPB_RO->dispose();      this->TPB_RO = NULL; }
    if (this->TPB_RW)      { this->TPB_RW->dispose();      this->TPB_RW = NULL; }
    if (this->STATUS)      { this->STATUS->dispose();      this->STATUS = NULL; }
    this->InTrans   = false;
    this->Connected = false;

    return this->connectInternal();
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

int FBConnect::setupOutputMetadata (IStatus *rawStatus)
{
    ThrowStatusWrapper status(rawStatus);

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
            this->BUILDER->setType (&status, i, SQL_VARYING);
            this->BUILDER->setLength (&status, i, 64);
            this->FBCol[i].typeData = SQL_VARYING;
            break;
        default:
            return -1;
        }
    }

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

    this->BufferSize = this->METAO->getMessageLength (&status);
    if (! this->BufferSize) {
        snprintf (this->ErrorMsg, sizeof(this->ErrorMsg), "BUFFER SIZE IS ZERO");
        return -1;
    }

    this->BufferData = new unsigned char [this->BufferSize + 1];
    if (this->BufferData == NULL) {
        snprintf (this->ErrorMsg, sizeof(this->ErrorMsg), "UNABLE TO ALLOCATE BUFFER");
        return -1;
    }
    memset(this->BufferData, 0, this->BufferSize + 1);
    return 0;
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
                                               SAMPLES_DIALECT,
                         IStatement::PREPARE_PREFETCH_METADATA);

        if (this->setupOutputMetadata(this->STATUS) != 0) {
            return -1;
        }

        this->CURSOR = this->STATEMENT->openCursor (&status, this->TRANSACTION, NULL, NULL, this->METAO, 0);
    }
    catch (const FbException &error) {
        this->UTIL->formatStatus (this->ErrorMsg, sizeof (this->ErrorMsg), error.getStatus());
        return -1;
    }

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

string FBConnect::getColumn(const char *colName)
{
    int idx = this->getColumnIndex(colName);
    if (idx < 0) return "";
    return this->getColumn(0, idx);
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
            uint16_t strLen = *(const uint16_t *)colPtr;
            if (strLen > this->FBCol[col].length) {
                strLen = (uint16_t)this->FBCol[col].length;
            }
            fc_a.assign((const char *)(colPtr + sizeof(uint16_t)), strLen);
            break;
        }

        case SQL_TEXT:
        {
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

/* TYPED GETTERS */
long FBConnect::getInt(int col)
{
    return (long)this->getInt64(col);
}

long FBConnect::getInt(const char *colName)
{
    return (long)this->getInt64(this->getColumnIndex(colName));
}

int64_t FBConnect::getInt64(int col)
{
    if (this->isNull(col)) return 0;
    string s = this->getColumn(col);
    if (s.empty() || s == "-1") return 0;
    return (int64_t)strtoll(s.c_str(), NULL, 10);
}

int64_t FBConnect::getInt64(const char *colName)
{
    return this->getInt64(this->getColumnIndex(colName));
}

double FBConnect::getDouble(int col)
{
    if (this->isNull(col)) return 0.0;
    string s = this->getColumn(col);
    if (s.empty() || s == "-1") return 0.0;
    return strtod(s.c_str(), NULL);
}

double FBConnect::getDouble(const char *colName)
{
    return this->getDouble(this->getColumnIndex(colName));
}

bool FBConnect::getBool(int col)
{
    if (this->isNull(col)) return false;
    string s = this->getColumn(col);
    return (s == "true" || s == "TRUE" || s == "1" || s == "T" || s == "Y");
}

bool FBConnect::getBool(const char *colName)
{
    return this->getBool(this->getColumnIndex(colName));
}

/* STRUCTURED ROW GETTERS */
vector<string> FBConnect::getRowVector(void)
{
    vector<string> row;
    if (!this->InTrans || !this->BufferData || !this->isFetchOK) return row;
    row.reserve(this->FCount);
    for (unsigned i = 0; i < this->FCount; ++i) {
        row.push_back(this->getColumn((int)i));
    }
    return row;
}

map<string, string> FBConnect::getRowMap(void)
{
    map<string, string> row;
    if (!this->InTrans || !this->BufferData || !this->isFetchOK) return row;
    for (unsigned i = 0; i < this->FCount; ++i) {
        const char *name = this->getColumnName((int)i);
        string key = (name && name[0] != '\0') ? name : ("COL_" + to_string(i));
        row[key] = this->getColumn((int)i);
    }
    return row;
}

/* PUBLIC: GETROW
** COPY FROM BUFFER TO USER VAR
*/
int FBConnect::getRow (unsigned char *dados)
{
    if (! this->InTrans    ||   this->TRANSACTION == 0L) return -1;
    if (! this->BufferData || ! this->BufferSize)        return -1;
    if (! this->isFetchOK)                               return -1;

    string line;
    for (unsigned j = 0; j < this->FCount; ++j) {
        if (j > 0) line += " | ";
        line += this->getColumn((int)j);
    }
    if (dados) {
        strcpy((char *)dados, line.c_str());
    }
    return 0;
}

/* PUBLIC : SELECT
** SELECIONAR VARIAS LINHAS DE DADOS (INCLUINDO DML COM RETURNING NO FB 5.x)
*/
int FBConnect::Select (const char *stmt)
{
    if (!stmt || strlen (stmt) < 15) { 
        snprintf (this->ErrorMsg, sizeof(this->ErrorMsg), "FBConnect::Select: VERIFY STATMENT LESS THAN 15 CHARS");
        return -1;
    }

    char txMode = isDmlReturningStmt(stmt) ? FBWRITE : FBREAD;
    if (! this->InTrans && this->Start (txMode)) return -1;
    
    if (this->Prepare (stmt)) {
        if (this->ErrorMsg[0] == '\0') {
            snprintf (this->ErrorMsg, sizeof(this->ErrorMsg), "FBConnect::Select:Prepare: UNABLE TO SETUP STATEMENT");
        }
        return -1;
    }

    this->isFetchOK = true;

    return 0;
}

/* PUBLIC : SELECTBIND
** SELECT PARAMETRIZADO COM ? (PROTECAO CONTRA SQL INJECTION E SUPORTE A RETURNING)
*/
int FBConnect::SelectBind (const char *stmt, const char **dados, int dimensao)
{
    if (!stmt || strlen(stmt) < 15 || !dados || dimensao <= 0) {
        snprintf(this->ErrorMsg, sizeof(this->ErrorMsg), "FBConnect::SelectBind: INVALID ARGUMENTS");
        return -1;
    }

    char txMode = isDmlReturningStmt(stmt) ? FBWRITE : FBREAD;
    if (!this->InTrans && this->Start(txMode)) return -1;

    ThrowStatusWrapper status(this->STATUS);
    this->closeQueryResources(this->STATUS);

    IMessageMetadata *inMeta = NULL;
    IMetadataBuilder *inBuilder = NULL;
    unsigned char *inBuf = NULL;

    try {
        this->STATEMENT = this->ATTATCHMENT->prepare(&status,
                                                     this->TRANSACTION,
                                                     0,
                                                     stmt,
                                                     SAMPLES_DIALECT,
                                                     IStatement::PREPARE_PREFETCH_METADATA);

        inMeta = this->STATEMENT->getInputMetadata(&status);
        unsigned paramCount = inMeta->getCount(&status);
        if ((int)paramCount != dimensao) {
            snprintf(this->ErrorMsg, sizeof(this->ErrorMsg),
                     "FBConnect::SelectBind: EXPECTED %u PARAMS, GOT %d", paramCount, dimensao);
            inMeta->release();
            this->closeQueryResources(this->STATUS);
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

        if (this->setupOutputMetadata(this->STATUS) != 0) {
            delete[] inBuf;
            inMeta->release();
            return -1;
        }

        this->CURSOR = this->STATEMENT->openCursor(&status, this->TRANSACTION, inMeta, inBuf, this->METAO, 0);

        delete[] inBuf;
        inMeta->release();
        this->isFetchOK = true;
        return 0;
    }
    catch (const FbException &error) {
        this->UTIL->formatStatus(this->ErrorMsg, sizeof(this->ErrorMsg), error.getStatus());
        if (inBuf)     delete[] inBuf;
        if (inBuilder) inBuilder->release();
        if (inMeta)    inMeta->release();
        this->closeQueryResources(this->STATUS);
        return -1;
    }
}

int FBConnect::SelectBind (const char *stmt, char **dados, int dimensao)
{
    return this->SelectBind(stmt, (const char **)dados, dimensao);
}

int FBConnect::Execute (const char *stmt)
{
    if (!stmt || strlen(stmt) == 0) {
        snprintf(this->ErrorMsg, sizeof(this->ErrorMsg), "FBConnect::Execute: EMPTY STATEMENT");
        return -1;
    }

    if (!this->InTrans && this->Start(FBWRITE)) return -1;

    ThrowStatusWrapper status(this->STATUS);
    this->AffectedRows = 0;
    IStatement *stmtObj = NULL;

    try {
        stmtObj = this->ATTATCHMENT->prepare(&status,
                                             this->TRANSACTION,
                                             0,
                                             stmt,
                                             SAMPLES_DIALECT,
                                             0);
        stmtObj->execute(&status, this->TRANSACTION, NULL, NULL, NULL, NULL);
        this->AffectedRows = stmtObj->getAffectedRecords(&status);
        stmtObj->free(&status);
        return 0;
    }
    catch (const FbException &error) {
        this->UTIL->formatStatus(this->ErrorMsg, sizeof(this->ErrorMsg), error.getStatus());
        if (stmtObj) stmtObj->release();
        return -1;
    }
}

uint64_t FBConnect::getAffectedRows (void)
{
    return this->AffectedRows;
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

int FBConnect::getColumnIndex (const char *colName)
{
    if (!colName || !this->FBCol) return -1;
    for (unsigned i = 0; i < this->FCount; ++i) {
        if (this->FBCol[i].name && strcasecmp(this->FBCol[i].name, colName) == 0) {
            return (int)i;
        }
    }
    return -1;
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

int64_t FBConnect::getNextSequence (const char *seqName, int step)
{
    if (!seqName || seqName[0] == '\0') return -1;
    for (const char *p = seqName; *p; ++p) {
        if (!isalnum((unsigned char)*p) && *p != '_' && *p != '$') {
            snprintf(this->ErrorMsg, sizeof(this->ErrorMsg), "FBConnect::getNextSequence: INVALID SEQUENCE NAME");
            return -1;
        }
    }

    bool startedHere = false;
    if (!this->InTrans) {
        if (this->Start(FBREAD) != 0) return -1;
        startedHere = true;
    }

    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT GEN_ID(%s, %d) FROM RDB$DATABASE", seqName, step);

    int64_t val = -1;
    if (this->Select(sql) == 0 && this->Fetch() == 0) {
        val = this->getInt64(0);
    }

    if (startedHere) {
        this->Commit();
    }
    return val;
}

int FBConnect::ExecuteBind (const char *stmt, const char **dados, int dimensao)
{
    if (!stmt || strlen(stmt) == 0 || !dados || dimensao <= 0) {
        snprintf(this->ErrorMsg, sizeof(this->ErrorMsg), "FBConnect::ExecuteBind: INVALID ARGUMENTS");
        return -1;
    }

    if (!this->InTrans && this->Start(FBWRITE)) return -1;

    ThrowStatusWrapper status(this->STATUS);
    this->AffectedRows = 0;
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
        this->AffectedRows = stmtObj->getAffectedRecords(&status);

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

int FBConnect::ExecuteBatch (const char *stmt, const vector<vector<string>> &rows)
{
    if (!stmt || strlen(stmt) == 0 || rows.empty()) {
        snprintf(this->ErrorMsg, sizeof(this->ErrorMsg), "FBConnect::ExecuteBatch: INVALID ARGUMENTS");
        return -1;
    }

    if (!this->InTrans && this->Start(FBWRITE)) return -1;

    ThrowStatusWrapper status(this->STATUS);
    this->AffectedRows = 0;
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

        vector<unsigned> maxLens(paramCount, 1);
        for (size_t r = 0; r < rows.size(); ++r) {
            if (rows[r].size() != paramCount) {
                snprintf(this->ErrorMsg, sizeof(this->ErrorMsg),
                         "FBConnect::ExecuteBatch: ROW %zu HAS %zu PARAMS, EXPECTED %u",
                         r, rows[r].size(), paramCount);
                inMeta->release();
                stmtObj->free(&status);
                return -1;
            }
            for (unsigned i = 0; i < paramCount; ++i) {
                if (rows[r][i].size() > maxLens[i]) {
                    maxLens[i] = (unsigned)rows[r][i].size();
                }
            }
        }

        inBuilder = inMeta->getBuilder(&status);
        for (unsigned i = 0; i < paramCount; ++i) {
            inBuilder->setType(&status, i, SQL_VARYING);
            inBuilder->setLength(&status, i, maxLens[i]);
        }

        inMeta->release();
        inMeta = inBuilder->getMetadata(&status);
        inBuilder->release();
        inBuilder = NULL;

        unsigned msgLen = inMeta->getMessageLength(&status);
        inBuf = new unsigned char[msgLen];

        uint64_t totalAffected = 0;
        for (size_t r = 0; r < rows.size(); ++r) {
            memset(inBuf, 0, msgLen);
            for (unsigned i = 0; i < paramCount; ++i) {
                unsigned offset     = inMeta->getOffset(&status, i);
                unsigned nullOffset = inMeta->getNullOffset(&status, i);
                *(ISC_SHORT *)(inBuf + nullOffset) = 0;
                uint16_t len = (uint16_t)rows[r][i].size();
                *(uint16_t *)(inBuf + offset) = len;
                if (len > 0) {
                    memcpy(inBuf + offset + sizeof(uint16_t), rows[r][i].data(), len);
                }
            }
            stmtObj->execute(&status, this->TRANSACTION, inMeta, inBuf, NULL, NULL);
            totalAffected += stmtObj->getAffectedRecords(&status);
        }

        this->AffectedRows = totalAffected;
        delete[] inBuf;
        inMeta->release();
        stmtObj->free(&status);
        return (int)rows.size();
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
        this->AffectedRows = stmtObj->getAffectedRecords(&status);

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

int FBConnect::isNull(const char *colName) {
	return this->isNull(this->getColumnIndex(colName));
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
