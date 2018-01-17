#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <syslog.h>
#include "FBConnect.h"

static IMaster *Master = fb_get_master_interface();

FBConnect::FBConnect (const char * banco, const char * user, const char * senha)
{

    this->TPB_RO      = NULL; 
    this->TPB_RW      = NULL;
    this->STATUS      = NULL; 
    this->PROVIDER    = NULL;
    this->ATTATCHMENT = NULL;
    this->UTIL        = NULL;

    this->STATUS   = Master->getStatus();
    this->PROVIDER = Master->getDispatcher();
    this->UTIL     = Master->getUtilInterface();
    this->InTrans  = false;

    ThrowStatusWrapper status(this->STATUS);

    IXpbBuilder* dpb = NULL;
    dpb = this->UTIL->getXpbBuilder(&status, IXpbBuilder::DPB, NULL, 0);
    
    dpb->insertString (&status, isc_dpb_user_name, user);
    dpb->insertString (&status, isc_dpb_password, senha);
    

    try { 
        this->ATTATCHMENT = this->PROVIDER->attachDatabase (&status, 
                                                              banco, 
                                      dpb->getBufferLength(&status),
                                            dpb->getBuffer(&status));
    
        // PREPARA TPB RO
        this->TPB_RO = this->UTIL->getXpbBuilder (&status, IXpbBuilder::TPB, NULL, 0);
        this->TPB_RO->insertTag(&status, isc_tpb_read_committed);
        this->TPB_RO->insertTag(&status, isc_tpb_no_rec_version);
        this->TPB_RO->insertTag(&status, isc_tpb_wait);
        this->TPB_RO->insertTag(&status, isc_tpb_read);

        // PREPARA TPB RW
        this->TPB_RW = this->UTIL->getXpbBuilder (&status, IXpbBuilder::TPB, NULL, 0);
        this->TPB_RW->insertTag(&status, isc_tpb_read_committed);
        this->TPB_RW->insertTag(&status, isc_tpb_no_rec_version);
        this->TPB_RW->insertTag(&status, isc_tpb_wait);
        this->TPB_RW->insertTag(&status, isc_tpb_write);
        
        this->Conectado = true;
    }
    catch (const FbException& error) {                                                                                                                                         
        char buf[356];                                                                                                                            
        
        this->UTIL->formatStatus (buf, sizeof(buf), error.getStatus());                                                                                   
        sprintf (this->ErrorMsg, "%s\n", buf);                                                                                                             
        
        dpb->dispose(); 
        this->Conectado = false;
        this->PROVIDER->release();
        this->STATUS->dispose();
        this->TPB_RO      = NULL; 
        this->TPB_RW      = NULL;
        this->STATUS      = NULL; 
        this->PROVIDER    = NULL;
        this->ATTATCHMENT = NULL;
        this->UTIL        = NULL;

    }

    printf ("FBCONNECT VERSAO: %f\n", FBCONNECT_VERSAO);
}

FBConnect::~FBConnect()
{
    ThrowStatusWrapper status(this->STATUS);

    if (this->ATTATCHMENT) {
        this->ATTATCHMENT->detach(&status);
        this->ATTATCHMENT->release(); }

    if (this->PROVIDER) this->PROVIDER->release();
    if (this->TPB_RO)   this->TPB_RO->dispose();
    if (this->TPB_RW)   this->TPB_RW->dispose();
    if (this->STATUS)   this->STATUS->dispose();

    this->TPB_RO      = NULL; 
    this->TPB_RW      = NULL;
    this->STATUS      = NULL; 
    this->PROVIDER    = NULL;
    this->ATTATCHMENT = NULL;
    this->UTIL        = NULL;
    printf ("DESTRUCTOR OK\n");
}

// INICIA UMA TRANSACAO RO/RW
int FBConnect::Start (char readwrite)
{
    if (! this->Conectado) return -1;
   
    if (this->InTrans) return 0;

    ThrowStatusWrapper status(this->STATUS);
    
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
    this->InTrans = true;

    return 0;
}

int FBConnect::Commit (void)
{
    if (! this->InTrans || this->TRANSACTION == 0L) return -1;

    ThrowStatusWrapper status(this->STATUS);

    try {
        this->TRANSACTION->commit (&status);
        this->InTrans     = false;
        this->TRANSACTION = NULL;
        return 0;
    }
    catch (const FbException& error) {                                                                                                                                         
        char buf[356];
        this->UTIL->formatStatus (buf, sizeof(buf), error.getStatus());
        sprintf (this->ErrorMsg, "%s\n", buf);
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
        char buf[356];
        this->UTIL->formatStatus (buf, sizeof(buf), error.getStatus());
        sprintf (this->ErrorMsg, "%s\n", buf);
        return -1;
    }
}

int FBConnect::Fetch  (char *dados)
{
    return 0;
}

int FBConnect::Select (char *stmt)
{
    return 0;
}

int FBConnect::Execute (char *stmt)
{
    return 0;
}

int FBConnect::ExecuteBind (char *stmt, char **dados, int dimensao)
{
    return 0;
}
/*
void IbaseConnect::EventClear(void)
{
    if (! this->connectOK) return;

    isc_cancel_events (this->status, &DBHandler, &EventID);

    if (this->status[0] == 1 && this->status[1])
        this->Error((char*) "SelectStart: COULD NOT CLEAR A EVENT", this->status);
}

int IbaseConnect::EventInit (char *event)
{
    if (! this->connectOK) return -1;

    memset (this->EventName, '\0', sizeof (this->EventName));

    if (strlen (event) >= sizeof (this->EventName))
        strncpy (this->EventName, event, sizeof(this->EventName) - 1);
    else
        memcpy (this->EventName, event, strlen (event));

    EventBlockLength = isc_event_block (&EventBuffer, &ResultBuffer, (short) 1, this->EventName);
    return  isc_sqlcode (this->status);
}

int IbaseConnect::EventInit (char          *event,
                             unsigned char *resultbuff,
                             unsigned char *eventbuff,
                             void          (*funcao)(unsigned char*, short, unsigned char*))
{
    if (! this->connectOK) return -1;

    this->EventFunction = funcao;
    this->ResultBuffer  = resultbuff;
    this->EventBuffer   = eventbuff;

    memset (this->EventName, '\0', sizeof (this->EventName));

    if (strlen (event) >= sizeof (EventName))
        strncpy (EventName, event, sizeof (EventName) - 1);
    else
        memcpy (EventName, event, strlen (event));

    EventBlockLength = isc_event_block (&EventBuffer, &ResultBuffer, (short) 1, EventName);
    return  isc_sqlcode (this->status);
}

void IbaseConnect::EventCounts(void)
{
    if (! this->connectOK) return;
    isc_event_counts (Events, EventBlockLength, EventBuffer, ResultBuffer);
}

int IbaseConnect::EventWait (bool block = true)
{

    if (! this->connectOK) return -1;

    if (block) {
        isc_wait_for_event (this->status, &DBHandler, EventBlockLength, EventBuffer, ResultBuffer);
        isc_event_counts   (Events, EventBlockLength, EventBuffer, ResultBuffer); }
    else {
        isc_que_events (   this->status,
                             &DBHandler,
                               &EventID,
                       EventBlockLength,
                            EventBuffer, (ISC_EVENT_CALLBACK) (this->EventFunction),
                           ResultBuffer);

        if (this->status[0] == 1 && this->status[1]) {
            this->Error((char*) "SelectStart: COULD NOT QUEUE A EVENT", this->status);
            return 1; };
    }

    return  isc_sqlcode (this->status);
}

void IbaseConnect::Error (char * text, long * status)
{
    long *pError;
    long SQLCODE;
    char msg[512];

    if (! this->connectOK) return;

    if (status == NULL) {
        strcpy (this->TextError, text);
        return; }

    pError  = this->status;
    SQLCODE = isc_sqlcode(pError);
    memset  (msg, '\0', sizeof (msg));
    memset  (this->TextError, '\0', sizeof (this->TextError));

    isc_sql_interprete (SQLCODE, (char *) &msg[0], sizeof (msg));
    fb_interpret       (this->TextError, sizeof (this->TextError), (const long int**) &pError);

    if (text != NULL) {
        strcat (this->TextError, " - ");
        strcat (this->TextError, text); }

    this->Log (this->TextError);
    if (strlen (msg) > 0) this->Log (msg);
}

void IbaseConnect::Log (char * texto) { syslog   (LOG_ALERT | LOG_WARNING, "IbaseConnect: %s", texto); }

void IbaseConnect::Trace (char *texto)
{
    if (! this->connectOK) return;

    IClog    = fopen( "./IC_log.log", "a+");
    fprintf  (IClog, "%s\n", texto);
    printf   ("%s\n", texto);
    fclose   (IClog);
}


int IbaseConnect::RollBack (void)
{
    if (! this->connectOK) return -1;

    if (this->TransactionHdl  != 0L) {
        isc_rollback_transaction (this->status, &this->TransactionHdl);
        this->TransactionHdl  =   0L;
        return 0; }
    else {
        this->Error ((char*)  "RollBack: Problemas abortando Transacao", this->status);
        return -25; }
}

int IbaseConnect::CommitRetaining (void)
{
    if (! this->connectOK) return -1;

    if (this->TransactionHdl != 0L)
        isc_commit_retaining (this->status, &this->TransactionHdl);
    else {
        this->Error ((char*) "CommitRetaining: Transacao nao Confirmada!", this->status);
        return -24; }

    return 0;
}

int IbaseConnect::Commit (void)
{
    if (! this->connectOK) return -1;

    if (this->TransactionHdl != 0L) {
        isc_commit_transaction (this->status, &this->TransactionHdl);
        this->TransactionHdl  = 0L; }
    else {
        this->Error ((char*) "CommitTransaction: Transacao nao Confirmada!", this->status);
        return  -240; }

    return 0;
}

// SEM RETORNO DE DADOS
int IbaseConnect::Execute (char * query)
{
    if (! this->connectOK) return -1;

    if (! this->TransactionHdl) {
        this->Error ((char*) "Execute: Transacao nao Inicializada", NULL);
        return -20; }

    if (query == NULL || strlen (query) == 0) {
        this->Error ((char*) "Execute: Nada para ser Executado", NULL);
        return -21; }

    isc_dsql_execute_immediate (this->status,&DBHandler,&this->TransactionHdl, 0, query, SQL_DIALECT_V6, NULL);

    if (this->status[0] == 1 && this->status[1]) {
        this->Error ((char*) "Execute: Problemas no Execute Immediate", NULL);
        return -22; }

    return 0;
}

// N STATEMENTS SEM RETORNO DE DADOS 
int IbaseConnect::BindExecute (char * query, char ** dados, int dimensao)
{
    short             i, *psqlind, sqlind[ IC_MAXNUNCOLUMN ];
    XSQLVAR ISC_FAR   *var;

    if (! this->connectOK) return -1;

    // 1. CONDICIONAIS , SETUP E LIBERACAO DE MEMORIA
    // {
    this->State    = 0;
    this->StmtHdl  = 0;

    if (this->TransactionHdl == 0L) {
        this->Error ((char*) "BindExecute: Transacao nao Inicializada", NULL);
        return  -20; }

    // se tem algo a ser trabalhado
    if (query == NULL || strlen (query) == 0) {
        this->Error ((char*) "BindExecute: Nada para ser Trabalhado", NULL);
        return  -21; }

    // se existe memoria a ser liberada
    if (this->InDados) {
        free (this->Dados );
        free (this->ColunSize);
        this->InDados = 0; }

    // se existe memoria a ser liberada
    if (this->InSqlda) {
        free (this->outSqlda);
        this->InSqlda = 0; }
    // }

    // 2. ALOCACAO DE MEMORIA PARA DADOS DE ENTRADA
    //    PREPARACAO VERIFICACAO E DESCRICAO DOS DADOS DE SAIDA
    // {
    // Verifica dados de entrada
    this->inSqlda             = (XSQLDA ISC_FAR *) malloc(XSQLDA_LENGTH (1));
    this->inSqlda->sqln       = 1;
    this->inSqlda->version    = 1;

    if (isc_dsql_allocate_statement (this->status, &this->DBHandler, &this->StmtHdl)) {
        this->Error ((char*) "Select: PROBLEMAS AO ALOCAR STATEMENT ", this->status);
        return -11; }

    if (isc_dsql_prepare (this->status, &this->TransactionHdl, &this->StmtHdl, 0, query, SQL_DIALECT_V6, this->inSqlda)) {
        this->Error ((char*) "Select Input: PROBLEMAS AO PREPARAR O SQL", this->status);
        this->Error (query, this->status);
        return -12; }

    if (isc_dsql_describe_bind (this->status, &this->StmtHdl, SQL_DIALECT_V6, this->inSqlda)) {
        this->Error ((char*) "Bind: PROBLEMAS AO DESCREVER O BIND", this->status); 
        return -13; }

    // ha espaco suficiente para as variaveis de entrada ?
    if (this->inSqlda->sqln < this->inSqlda->sqld) {
        this->inSqlda          =  (XSQLDA ISC_FAR *) realloc (this->inSqlda, XSQLDA_LENGTH (this->inSqlda->sqld));
        this->inSqlda->sqln    = this->inSqlda->sqld;
        this->inSqlda->version = 1;

       if (isc_dsql_describe_bind (this->status, &this->StmtHdl, SQL_DIALECT_V6, this->inSqlda)) {
           this->Error ((char*) "Bind: PROBLEMAS AO DESCREVER O BIND", this->status);
           return -13; }}

    this->InSqlda = 1;
    // }

    // 3. AJUSTES FINAIS PARA EXECUCAO
    //    LIBERACAO DE MEMORIA NAO UTILIZADA
    // {
    // ha realmente variaveis de entrada?
    if (this->inSqlda->sqld == 0) {
        free (this->inSqlda);
        this->inSqlda = NULL; }

    // Associando dados de entrada
    var     = this->inSqlda->sqlvar;
    psqlind = &sqlind[0];
    i       = 0;
    // }

    // 4. OPERACAO DOS DADOS
    // {
    do {
        if (i >= dimensao) {
            i = 0;

            if (isc_dsql_execute (this->status, &this->TransactionHdl, &this->StmtHdl, SQL_DIALECT_V6, this->inSqlda)) {
                this->Error ((char*) "BindExecute: PROBLEMAS AO EXECUTAR O BIND", this->status);
                return -14; }
            else
                var = this->inSqlda->sqlvar; }
        else {
            var->sqldata = *dados;
            var->sqllen  = strlen (*dados);
            var->sqltype = SQL_TEXT;
            var->sqlind  = psqlind;
            var++; dados++; psqlind++, i++; }

        } while (*dados != NULL);

    if (isc_dsql_execute (this->status, &this->TransactionHdl, &this->StmtHdl, SQL_DIALECT_V6, this->inSqlda)) {
        this->Error ((char*) "BindExecute: PROBLEMAS AO EXECUTAR O BIND", this->status);
        return -14; }
    // }

    // 6. LIBERACAO DE MEMORIA ALOCADA
    //
    if (this->inSqlda  != NULL) free (this->inSqlda);
    this->State   = 0;
    psqlind       = NULL;
    this->inSqlda = NULL;

    return 0;

} // BINDEXEC

// RETORNA N LINHAS
int IbaseConnect::Select (char * query)
{
    short             i;
    short             length, alignment, type, offset;
    XSQLVAR ISC_FAR   *var;

    if (! this->connectOK) return -1;

    // 1. SETUP LIBERACAO DE MEMORIA E CONDICIONAIS
    // {
    if (this->TransactionHdl == 0L) {
        this->Error ((char*) "Select: Transacao nao Inicializada", NULL);
        return  -20; }

    if (query == NULL || strlen (query) == 0) {
        this->Error ((char*) "Select: Nada para ser Selecionado", NULL);
        return  -21; }

    if (this->InDados) {
        free (this->Dados);
        free (this->ColunSize);
        this->InDados = 0; }

    if (this->InSqlda) {
        free (this->outSqlda);
        this->InSqlda = 0; }

    this->State  = 0;
    // }

    // 2. ALOCACAO DE MEMORIA VARIAVEIS DE SAIDA
    // {
    this->outSqlda             = (XSQLDA ISC_FAR *) malloc(XSQLDA_LENGTH (1));
    this->outSqlda->sqln       = 1;
    this->outSqlda->sqld       = 1;
    this->outSqlda->version    = 1;
    this->StmtHdl              = 0;

    if (this->outSqlda == NULL) {
        this->Error((char*) "Select: PROBLEMAS AO ALOCAR MEMORIA ", this->status);
        return -11; }

    if (isc_dsql_allocate_statement (this->status, &DBHandler, &this->StmtHdl)) {
        this->Error((char*) "Select: PROBLEMAS AO ALOCAR STATEMENT ", this->status);
        return -11; }

    if (isc_dsql_prepare (this->status, &this->TransactionHdl, &this->StmtHdl, 0, query, SQL_DIALECT_V6, this->outSqlda)) {
        this->Error((char*) "Select Output: PROBLEMAS AO PREPARAR O SQL", this->status);
        this->Error(query, this->status);
        return -12; }

    this->Colunas     =   this->outSqlda->sqld;

    // Need more room.
    if (this->outSqlda->sqln < this->Colunas) {
        this->outSqlda          = (XSQLDA ISC_FAR *) realloc (this->outSqlda, XSQLDA_LENGTH(this->Colunas));
        this->outSqlda->sqln    = this->Colunas;
        this->outSqlda->version = 1;

        if (isc_dsql_describe (this->status, &this->StmtHdl, SQL_DIALECT_V6, this->outSqlda)) { 
            this->Error((char*) "Select: PROBLEMAS AO DESCREVER O QUERY", this->status); 
            return -13; }

        this->Colunas = this->outSqlda->sqld; }

    // Alocacao do buffer de saida
    for  (var = this->outSqlda->sqlvar, offset = 0, i = 0; i < this->Colunas; var++, i++) {
        length = alignment = var->sqllen;

        if ( (var->sqltype & ~1) == SQL_TEXT) alignment = 1;
        else
        if ( (var->sqltype & ~1) == SQL_VARYING) {
            length    +=  sizeof  (short) + 1;
            alignment =   sizeof  (short); }

        offset        =   FB_ALIGN(offset, alignment);
        offset        +=  length;
        offset        =   FB_ALIGN(offset, sizeof  (short));
        offset        +=  sizeof  (short); }

    this->ColunSize   =   (int *)   calloc  (this->Colunas, sizeof (int));
    this->Dados       =   (long *)  calloc  (1, offset);
    this->InDados     =  1;
    this->DadosSize   =  offset;

    //     Set up SQLDA.
    for  (var = this->outSqlda->sqlvar, offset = 0, i = 0; i < this->Colunas; var++, i++) {
        length = alignment = var->sqllen;
        type   = var->sqltype & ~1;

        if (type == SQL_TEXT) alignment = 1;
        else
        if (type == SQL_VARYING) {
            length    +=  sizeof  (short) + 1;
            alignment =   sizeof  (short); }

        //  RISC machines are finicky about word alignment So the output buffer values must be placed on word boundaries where appropriate
        offset        =   FB_ALIGN(offset, alignment);
        var->sqldata  =    (char ISC_FAR *) this->Dados + offset;
        offset        +=  length;
        offset        =   FB_ALIGN(offset, sizeof  (short));
        var->sqlind   =    (short*) ( (char ISC_FAR *) this->Dados + offset);
        offset        +=  sizeof  (short); }

    this->InSqlda              = 1;
    // }

    // 3. EXECUCAO DO SELECT
    if (isc_dsql_execute (this->status, &this->TransactionHdl, &this->StmtHdl, SQL_DIALECT_V6, NULL)) {
        this->Error ((char*) "Select: PROBLEMAS AO EXECUTAR O QUERY", this->status);
        return -14; }

    this->State = 1;

    return 0;

} //Select

int IbaseConnect::Fetch (char *local)
{
    long   fetchStat;
    int    i, tamanho;
    char   *onde;
    char   pcoluna[ IC_MAXCOLUMN ];

    if (! this->connectOK) return -1;

    onde      = local;
    i         = 0;
    tamanho   = 0;
    fetchStat = 0;

    if (! this->TransactionHdl || !this->InDados || !this->outSqlda || this->State != 1) {
        this->Error ((char*) "Fetch: Transaction not initialized", NULL);
        return -100; }

    memset (this->Dados, '\0', this->DadosSize);
    fetchStat    =   isc_dsql_fetch (this->status, &this->StmtHdl, SQL_DIALECT_V6, this->outSqlda);

    if (fetchStat == 0) {
        for  (i = 0; i < this->outSqlda->sqld; i++) {
            memset (pcoluna, '\0', sizeof (pcoluna));
            tamanho = MemoryPrint ( (XSQLVAR ISC_FAR *) &this->outSqlda->sqlvar[ i ], pcoluna);
            this->ColunSize[ i ] = tamanho;
            memcpy (onde, pcoluna, tamanho);
            onde     += tamanho; } }
    else
    if (fetchStat == 100L) {
        //isc_commit_retaining (this->status, &this->TransactionHdl);
        free (this->Dados);
        free (this->ColunSize);
        free (this->outSqlda);
        this->InDados  =   0;
        this->InSqlda  =   0;
        this->State    =   0; }

    return  fetchStat;

}

void IbaseConnect::AllTrim (char *string)
{
    int     tamanho;
    char    *pini, *pfim, *resultado;

    if (! this->connectOK) return;

    tamanho     = strlen (string);
    resultado   =  (char *) calloc (1, tamanho);
    memset (resultado, '\0', tamanho);

    // LIMPAR INICIO 
    pini  =   string;
    pfim  =   pini + tamanho - 1;

    while  (*pini == 0x20 && pini < pfim)
          pini++;

    if (pini == pfim) goto fim;

    while  (*pfim == 0x20 && pini < pfim)
          pfim--;

    memcpy (resultado, pini, pfim - pini + 1);

    strcpy (string, resultado);

    fim:

    free (resultado);

    return;

}

int IbaseConnect::GetTextBlob  (char *blob)
{
    char *p;

    if (isc_open_blob (this->status, &this->DBHandler, &this->TransactionHdl, &this->BlobHandler, &this->BlobID)) {
        this->Error((char*) "GetTextBlob: PROBLEMAS AO ABRIR O BLOB", this->status);
        return -13; }

    else printf ("SEGMENT LEN %d\n", (int)this->BlobSegmentLen);

    memset (this->BlobSegment, '\0', sizeof (this->BlobSegment));
    this->BlobStat = isc_get_segment (this->status, &this->BlobHandler, (unsigned short *) &this->BlobSegmentLen, sizeof (this->BlobSegment), this->BlobSegment);
    p = blob;

    while (this->BlobStat == 0 || this->status[1] == isc_segment) {
        memcpy (p, this->BlobSegment, this->BlobSegmentLen);
        p += this->BlobSegmentLen;
        memset (this->BlobSegment, '\0', sizeof (this->BlobSegment));
        this->BlobStat = isc_get_segment (this->status, &this->BlobHandler, (unsigned short *) &this->BlobSegmentLen, sizeof (this->BlobSegment), this->BlobSegment); }

    if (this->status[1] == isc_segstr_eof) {
        if (isc_close_blob (this->status, &this->BlobHandler)) {
            this->Error ((char*) "GetTextBlob: PROBLEMAS AO FECHAR O BLOB", this->status);
            return -13; }}
    else isc_print_status (this->status);

    return 0;
}

int IbaseConnect::SetTextBlob (char *query, char *blob, int bloblen)
{
    char *p;
    int   sizebytes, len;

    if (this->TransactionHdl == 0L) {
        this->Error ((char*) "Select: Transacao nao Inicializada", NULL);
        return  -20; }

    // 1. ALOCACAO DE MEMORIA PARA DADOS DE ENTRADA
    //    PREPARACAO VERIFICACAO E DESCRICAO DOS DADOS DE SAIDA
    // {
    // Verifica dados de entrada
    this->inSqlda                     = (XSQLDA *) malloc(XSQLDA_LENGTH (1));
    this->inSqlda->sqln               = 1;
    this->inSqlda->sqld               = 1;
    this->inSqlda->version            = 1;
    this->inSqlda->sqlvar[0].sqldata  = (char*) &this->BlobID;
    this->inSqlda->sqlvar[0].sqltype  = SQL_BLOB;
    this->inSqlda->sqlvar[0].sqllen   = sizeof (ISC_QUAD);
    // }

    // 2. CRIANDO E ARMAZENANDO O BLOB
    // {
    this->BlobHandler = 0;
    if (isc_create_blob (this->status, &this->DBHandler, &this->TransactionHdl, &this->BlobHandler, &this->BlobID)) {
        this->Error ((char*) "SetTextBlob: problemas ao criar o blob", this->status);
        free        (this->inSqlda);
        return -13; }

    if (bloblen <= 0) {
        this->Error ((char*) "SetTextBlob: tamanho incorreto do o blob", this->status);
        free        (this->inSqlda);
        return -13; }

    sizebytes = 0;
    while (sizebytes < bloblen) {
        p   = blob + sizebytes;
        len = bloblen > 1024?1024:bloblen;

        if (isc_put_segment (this->status, &this->BlobHandler, len, p)) {
            this->Error ((char*) "SetTextBlob: FALHA NO CARREGAMENTO DO BLOB", this->status);
            free        (this->inSqlda);
            return -13; }

        sizebytes += len; }

    if (isc_close_blob (this->status, &this->BlobHandler)) {
        this->Error ((char*) "SetTextBlob: PROBLEMAS AO FECHAR O BLOB", this->status);
        free        (this->inSqlda);
        return -13; }
    // }

    // 3. EXECUTANDO O QUERY
    if (isc_dsql_execute_immediate (this->status, &this->DBHandler, &this->TransactionHdl, 0, query, 1, this->inSqlda)) {
        this->Error ((char*) "SetTextBlob: FALHA NA EXECUCAO", this->status);
        this->Log   (query);
        free        (this->inSqlda);
        return -13; }

    return 0;
}


int IbaseConnect::GetFileBlob  (FILE *blob)
{
    size_t len;

    if (isc_open_blob (this->status, &this->DBHandler, &this->TransactionHdl, &this->BlobHandler, &this->BlobID)) {
        this->Error ((char*) "GetFileBlob: PROBLEMAS AO ABRIR O BLOB", this->status);
        printf ((char*) "GetFileBlob: PROBLEMAS AO ABRIR O BLOB");
        return -13; }
else printf ("BLOB ABERTO COM SUCESSO\n");

    memset (this->BlobSegment, '\0', sizeof (this->BlobSegment));
    this->BlobStat = isc_get_segment (              this->status,
                                              &this->BlobHandler,
                        (unsigned short *) &this->BlobSegmentLen,
                                      sizeof (this->BlobSegment),
                                              this->BlobSegment);

printf ("VALOR DE BLOBSTAT [%ld]\nVALOR DE STATUS[%ld][%ld]\nVALOR DE SEGMENTLEN %d\n", this->BlobStat, this->status[1], isc_segment, this->BlobSegmentLen);

    while (this->BlobStat == 0 || this->status[1] == isc_segment) {
        len = this->BlobSegmentLen;
        if (len != fwrite (this->BlobSegment, 1, this->BlobSegmentLen, blob)) {
            this->Error((char*) "GetFileBlob: fwrite FAIL", this->status);
            return -13;
        }
        memset (this->BlobSegment, '\0', sizeof (this->BlobSegment));
        this->BlobStat = isc_get_segment (this->status, &this->BlobHandler, (unsigned short *) &this->BlobSegmentLen, sizeof (this->BlobSegment), this->BlobSegment); }

    if (this->status[1] == isc_segstr_eof) {
        if (isc_close_blob (this->status, &this->BlobHandler)) {
            this->Error ((char*) "GetFileBlob: PROBLEMAS AO FECHAR O BLOB", this->status);
            return -13; }}
    else isc_print_status (this->status);

    return 0;
}

int IbaseConnect::SetFileBlob (char *query, FILE *blob, int bloblen)
{
    char arquivo[1024];

    size_t   sizebytes, len;

    if (this->TransactionHdl == 0L) {
        this->Error ((char*) "Select: Transacao nao Inicializada", NULL);
        return  -20; }

    // 1. ALOCACAO DE MEMORIA PARA DADOS DE ENTRADA
    //    PREPARACAO VERIFICACAO E DESCRICAO DOS DADOS DE SAIDA
    // {
    // Verifica dados de entrada
    this->inSqlda                     = (XSQLDA *) malloc(XSQLDA_LENGTH (1));
    this->inSqlda->sqln               = 1;
    this->inSqlda->sqld               = 1;
    this->inSqlda->version            = 1;
    this->inSqlda->sqlvar[0].sqldata  = (char*) &this->BlobID;
    this->inSqlda->sqlvar[0].sqltype  = SQL_BLOB;
    this->inSqlda->sqlvar[0].sqllen   = sizeof (ISC_QUAD);
    // }

    // 2. CRIANDO E ARMAZENANDO O BLOB
    // {
    this->BlobHandler = 0;
    if (isc_create_blob (this->status, &this->DBHandler, &this->TransactionHdl, &this->BlobHandler, &this->BlobID)) {
        this->Error ((char*) "SetFileBlob: problemas ao criar o blob", this->status);
        free        (this->inSqlda);
        return -13; }

    if (bloblen <= 0) {
        this->Error ((char*) "SetFileBlob: tamanho incorreto do o blob", this->status);
        free        (this->inSqlda);
        return -13; }

    memset (arquivo, '\0', sizeof (arquivo));
    sizebytes = 0;
    while (sizebytes < (size_t)bloblen) {

        len = (bloblen - sizebytes) > 1024?1024:(bloblen - sizebytes); // QUANTIDADE A SER LIDA. MAX 1024

        if (len != fread (&arquivo[0], 1, len, blob)) {
            this->Error ((char*) "SetFileBlob: fread FAIL", this->status);
            free        (this->inSqlda);
            return -13; }

        if (isc_put_segment (this->status, &this->BlobHandler, len, &arquivo[0])) {
            this->Error ((char*) "SetFileBlob: FALHA NO CARREGAMENTO DO BLOB", this->status);
            free        (this->inSqlda);
            return -13; }

        sizebytes += len; }

    if (isc_close_blob (this->status, &this->BlobHandler)) {
        this->Error ((char*) "SetFileBlob: PROBLEMAS AO FECHAR O BLOB", this->status);
        free        (this->inSqlda);
        return -13; }
    // }

    // 3. EXECUTANDO O QUERY
    if (isc_dsql_execute_immediate (this->status, &this->DBHandler, &this->TransactionHdl, 0, query, 1, this->inSqlda)) {
        this->Error ((char*) "SetFileBlob: FALHA NA EXECUCAO", this->status);
        this->Log   (query);
        free        (this->inSqlda);
        return -13; }

    return 0;
}

int IbaseConnect::MemoryPrint  (XSQLVAR ISC_FAR * var , char * p)
{
    struct tm
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

    typedef struct vary {
    short          vary_length;
    char           vary_string [1];
    } VARY;

    short       dtype;
    char        blob_s[20], date_s[25];
    VARY        *vary;
    int         len; 
    struct tm   times;
    ISC_QUAD    bid;

    len = 0;

    if (! this->connectOK) return -1;

    dtype = var->sqltype & ~1;

    if ((var->sqltype & 1) && (*var->sqlind < 0)) {
        switch (dtype) {
            case SQL_TEXT:
            case SQL_VARYING:
                len = var->sqllen;
                break;

            case SQL_SHORT:
                len = 6;
                if (var->sqlscale > 0) len += var->sqlscale;
                break;

            case SQL_LONG:
                len = 11;
                if (var->sqlscale > 0) len += var->sqlscale;
                break;

            case SQL_INT64:
                len = 21;
                if (var->sqlscale > 0) len += var->sqlscale;
                break;

            case SQL_FLOAT:
                len = 15;
                break;

            case SQL_DOUBLE:
                len = 24;
                break;

            case SQL_TIMESTAMP:
                len = 24;
                break;

            case SQL_TYPE_DATE:
                len = 10;
                break;

            case SQL_TYPE_TIME:
                len = 13;
                break;

            case SQL_BLOB:

            case SQL_ARRAY:

            default:

                len = 17;
                break;
        }//switch

        if ( (dtype == SQL_TEXT) ||  (dtype == SQL_VARYING))
             sprintf(p, "%-*s", len, "NULL", len);
        else sprintf(p, "%*s", len, "NULL", len); }// if ((var->sqltype & 1) && (*var->sqlind < 0))

    else {
        switch (dtype) {
            case SQL_TEXT:
                sprintf(p, "%.*s", var->sqllen, var->sqldata);
                len = var->sqllen;
                break;

            case SQL_VARYING:
                vary = (VARY*) var->sqldata;
                vary->vary_string[vary->vary_length] = '\0';
                sprintf(p, "%-*s", var->sqllen, vary->vary_string);
                len = vary->vary_length;
                break;

            case SQL_SHORT:

            case SQL_LONG:

            case SQL_INT64: {
                ISC_INT64  value;
                short      field_width;
                short      dscale;
                field_width = 0;
                value       = 0;

                switch (dtype) {
                case SQL_SHORT:
                    value = (ISC_INT64) *(short ISC_FAR *) var->sqldata;
                    field_width = 6;
                    break;

                case SQL_LONG:
                    value = (ISC_INT64) *(long ISC_FAR *) var->sqldata;
                    field_width = 11;
                    break;

                case SQL_INT64:
                    value = (ISC_INT64) *(ISC_INT64 ISC_FAR *) var->sqldata;
                    field_width = 21;
                    break; }

                dscale = var->sqlscale;

                if (dscale < 0) {
                    ISC_INT64   tens;
                    short       i;
                    tens = 1;

                    for (i = 0; i > dscale; i--)
                         tens *= 10;

                    if (value >= 0) {
                        sprintf (p, "%0*" "ll" "d.%0*" "ll" "d", field_width - 1 + dscale, (ISC_INT64) (value / tens), -dscale, (ISC_INT64) (value % tens));
                        len = strlen (p); }
                    else 
                    if ((value / tens) != 0) {
                        sprintf (p, "%0*" "ll" "d.%0*" "ll" "d", field_width - 1 + dscale, (ISC_INT64) (value / tens), -dscale, (ISC_INT64) -(value % tens));
                        len = field_width; }
                    else {
                        sprintf (p,  "%*s.%0*" "ll" "d", field_width - 1 + dscale, "-0", -dscale, (ISC_INT64) -(value % tens));
                        len = field_width - 1 + dscale; }
                        }
                else {
                    if (dscale) {
                        sprintf (p, "%*" "ll" "d%0*d", field_width, (ISC_INT64) value, dscale, 0);
                        len = field_width; }

                    else {
                        switch (field_width) {
                            case 11:
                                sprintf(p, "%-11.11d", (ISC_INT64) value);
                                break;
                            case 21:
                                sprintf(p, "%-21.21d", (ISC_INT64) value);
                                break;
                        }
                        len = field_width;
                    }
                }
           };
                break;

            case SQL_FLOAT:
                sprintf(p, "%15g", *(float ISC_FAR *) (var->sqldata));
                len = 15;
                break;

            case SQL_DOUBLE:
                sprintf(p, "%24f", *(double ISC_FAR *) (var->sqldata));
                len = 24;
                break;

            case SQL_TIMESTAMP:
                isc_decode_timestamp((ISC_TIMESTAMP ISC_FAR *)var->sqldata, &times);
                sprintf(date_s, "%04d-%02d-%02d %02d:%02d:%02d.%04d", times.tm_year + 1900, times.tm_mon+1, times.tm_mday, times.tm_hour, times.tm_min, times.tm_sec, ((ISC_TIMESTAMP *)var->sqldata)->timestamp_time % 10000);
                sprintf(p, "%*s", 24, date_s);
                len = 24;
                break;

            case SQL_TYPE_DATE:
                isc_decode_sql_date((ISC_DATE ISC_FAR *)var->sqldata, &times);
                sprintf(date_s, "%04d-%02d-%02d", times.tm_year + 1900, times.tm_mon+1, times.tm_mday); sprintf(p, "%*s", 10, date_s);
                len = 10;
                break;

            case SQL_TYPE_TIME:
                isc_decode_sql_time((ISC_TIME ISC_FAR *)var->sqldata, &times);
                sprintf(date_s, "%02d:%02d:%02d.%04d", times.tm_hour, times.tm_min, times.tm_sec, (*((ISC_TIME *)var->sqldata)) % 10000);
                sprintf(p, "%*s", 13, date_s);
                len = 13;
                break;

           case SQL_BLOB:
           case SQL_ARRAY:
                bid = *(ISC_QUAD ISC_FAR *) var->sqldata;
                this->BlobID.gds_quad_high = bid.isc_quad_high;
                this->BlobID.gds_quad_low  = bid.isc_quad_low;
                sprintf(blob_s, "%08x:%08x", bid.isc_quad_high, bid.isc_quad_low);
                sprintf(p, "%17s", blob_s);
                len = 17;
                break;

            default:
                break;
        }//switch

    }// else

   return  len;

 }
 */
