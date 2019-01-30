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

FBConnect::FBConnect (const char * banco, const char * user, const char * senha)
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
    this->isFetchOK = false;
  
    ThrowStatusWrapper status(this->STATUS);

    IXpbBuilder* dpb = NULL;
    dpb = this->UTIL->getXpbBuilder (&status, IXpbBuilder::DPB, NULL, 0);
    
    dpb->insertString (&status, isc_dpb_user_name, user);
    dpb->insertString (&status, isc_dpb_password, senha);
    

    try { 
        this->ATTATCHMENT = this->PROVIDER->attachDatabase (&status, 
                                                              banco, 
                                      dpb->getBufferLength(&status),
                                            dpb->getBuffer(&status));
        // PREPARA TPB RO
        this->TPB_RO = this->UTIL->getXpbBuilder (&status, IXpbBuilder::TPB, NULL, 0);
        this->TPB_RO->insertTag (&status, isc_tpb_read_committed);
        this->TPB_RO->insertTag (&status, isc_tpb_no_rec_version);
        this->TPB_RO->insertTag (&status, isc_tpb_wait);
        this->TPB_RO->insertTag (&status, isc_tpb_read);

        // PREPARA TPB RW
        this->TPB_RW = this->UTIL->getXpbBuilder (&status, IXpbBuilder::TPB, NULL, 0);
        this->TPB_RW->insertTag (&status, isc_tpb_read_committed);
        this->TPB_RW->insertTag (&status, isc_tpb_no_rec_version);
        this->TPB_RW->insertTag (&status, isc_tpb_wait);
        this->TPB_RW->insertTag (&status, isc_tpb_write);
        
        this->Connected = true;
    }
    catch (const FbException& error) {                                                                                                                                         
        char buf[356];                                                                                                                            
        
        this->UTIL->formatStatus (buf, sizeof(buf), error.getStatus());                                                                                   
        sprintf (this->ErrorMsg, "%s\n", buf);                                                                                                             
        
        dpb->dispose(); 
        this->Connected = false;
        this->PROVIDER->release();
        this->STATUS->dispose();
        this->TPB_RO      = NULL; 
        this->TPB_RW      = NULL;
        this->STATUS      = NULL; 
        this->PROVIDER    = NULL;
        this->ATTATCHMENT = NULL;
        this->STATEMENT   = NULL;
        this->UTIL        = NULL;

    }

    printf ("FBCONNECT VERSAO: %f\n", FBCONNECT_VERSAO);
}

FBConnect::~FBConnect()
{
    ThrowStatusWrapper status(this->STATUS);

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
    printf ("DESTRUCTOR OK\n");
}

// INICIA UMA TRANSACAO RO/RW
int FBConnect::Start (char readwrite)
{
    if (! this->Connected) return -1;
   
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
    printf ("SAIDO DO RETURN 0 FINAL\n");
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

        if (this->BufferData != NULL) delete[] this->BufferData;
        if (this->FBCol      != NULL) delete[] this->FBCol;

        this->BufferData = NULL;
        this->FBCol      = NULL;

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

/* PRIVATE: PREPARE
** PREPARA CAMPOS DE RETORNO
** DOS DADOS SOLICITADOS NO STATEMENT
*/
int FBConnect::Prepare (const char *stmt)
{
    if (! this->InTrans || this->TRANSACTION == 0L) return -1;

    ThrowStatusWrapper status(this->STATUS);

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
        case SQL_TEXT:
           //printf ("TEXT OR VARYING\n");            
            this->BUILDER->setType (&status, i, SQL_TEXT);
            break;
        case SQL_SHORT:
           //printf ("SHORT          \n");            
            this->BUILDER->setType (&status, i, SQL_SHORT);
            break;
        case SQL_LONG:
           // printf ("LONG           \n");            
            this->BUILDER->setType (&status, i, SQL_LONG);
            break;                              
        case SQL_INT64:                         
            //printf ("INT64          \n");            
            this->BUILDER->setType (&status, i, SQL_INT64);
            break;
        case SQL_FLOAT:
            //printf ("FLOAT          \n");            
            this->BUILDER->setType (&status, i, SQL_FLOAT);
            break;                             
        case SQL_DOUBLE:                       
            //printf ("DOUBLE         \n");            
            this->BUILDER->setType (&status, i, SQL_DOUBLE);
            break;                             
        case SQL_TIMESTAMP:                    
            //printf ("TIMESTAMP      \n");            
            this->BUILDER->setType (&status, i, SQL_TIMESTAMP);
            break;                             
        case SQL_TYPE_DATE:                    
            //printf ("DATE           \n");            
            this->BUILDER->setType (&status, i, SQL_TYPE_DATE);
            break;
        case SQL_TYPE_TIME:
            //printf ("TIME           \n");            
            this->BUILDER->setType (&status, i, SQL_TYPE_TIME);
            break;                             
        case SQL_BLOB:                         
            //printf ("BLOB           \n");            
            this->BUILDER->setType (&status, i, SQL_BLOB);
	    sub = this->METAO->getSubType(&status, i); 
	    this->FBCol[i].subData = sub;
            break;                             
        case SQL_ARRAY:                        
            //printf ("ARRAY          \n");            
            this->BUILDER->setType (&status, i, SQL_ARRAY);
            break;                               
        default:
            //printf ("CASE NOT FOUND![%d]", t);
	    return 0;
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
        	this->FBCol[i].length   = this->METAO->getLength (&status, i);
   	    	this->FBCol[i].offset   = this->METAO->getOffset (&status, i);
    	}
    }

    // OPEN CURSOR
    // ALLOCATE BUFFER, SET BUFFERDATA, BUFFERSIZE
     //IMessageMetadata* inMetadata;
    this->CURSOR     = this->STATEMENT->openCursor (&status, this->TRANSACTION, NULL , NULL, this->METAO, 0);
    this->BufferSize = this->METAO->getMessageLength (&status);
    if (! this->BufferSize) {
        sprintf (this->ErrorMsg, (char*)"BUFFER SIZE IS ZERO");
        return -1;
    }
   
    this->BufferData = new unsigned char [this->BufferSize +1];
    if (this->BufferData == NULL)  {
        sprintf (this->ErrorMsg, (char*)"UNABLE TO ALLOCATE BUFFER");
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
    

    if( ! this->isFetchOK ) {
        return -1;
    }

    	ThrowStatusWrapper status(this->STATUS);


	Result = this->CURSOR->fetchNext(&status, this->BufferData);
	

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

/* PUBLIC: GETSIZE
** RETURN BUFFERSIZE VALUE
*/
int FBConnect::getSize ()
{
 
 
    ThrowStatusWrapper status(this->STATUS);

    unsigned t = FBCol[0].typeData;


    this->blob = this->ATTATCHMENT->openBlob(&status, this->TRANSACTION, (ISC_QUAD*)(this->BufferData), 0, NULL);



    unsigned char bbchar[200], *p, item;

    static unsigned char items[] = {
    isc_info_blob_max_segment,
    isc_info_blob_total_length};

    blob->getInfo(&status, 2, items, sizeof(bbchar), bbchar );


    ISC_LONG max_size;
    ISC_LONG num_segments;
    int total_length;

    max_size = 0L;
    num_segments = 0L;

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
            case isc_info_truncated:
                 break;
            default:
                 break;
         }
         p += length;
    };
    this->BlobNumChar = total_length;




    return this->BufferSize;



}

/* PUBLIC: MEMORYPRINT
** COPY COLUMN TO USER VAR
*/
//int FBConnect::memoryPrint(unsigned char *dados, int col)
//int FBConnect::getColumn(unsigned char *p, int col)
string FBConnect::getColumn(int tam, int col)
{
    if (! this->InTrans    ||   this->TRANSACTION == 0L) return "-1" ;
    if (! this->BufferData || ! this->BufferSize)        return "-1";

    ThrowStatusWrapper status(this->STATUS);

    if( ! this->isFetchOK ) {
        return "-1";
    }
 
    // Armazena o retorno do sprintf
    int n = 0;
    int len; 
    unsigned t = FBCol[col].typeData;
    short dscale;
    struct fbc_tm times;
    char        blob_s[20], date_s[25];
    //unsigned char p2 = new unsigned char [tam];
    //printf("TAMANHO ---->>> %d\n", tam);
    //exit(0);
    unsigned char *p2 = new unsigned char [tam];
    unsigned char *pTemp = new unsigned char [30];

    ISC_QUAD    bid;
    

    stringstream tmp;
    // return string
    string fc_a;	

    ISC_INT64 value;
    switch (t)
    {
	    case SQL_TEXT:
    		//n = sprintf((char *)p, "%-*s",  this->FBCol[col].length, this->BufferData + this->FBCol[col].offset);
		break;
	    case SQL_VARYING:
    		//n = sprintf((char *)p, "%-*s",  this->FBCol[col].length, this->BufferData + this->FBCol[col].offset);
    		n = sprintf((char *)p2, "%-*s",  this->FBCol[col].length, this->BufferData + this->FBCol[col].offset);
		tmp << p2;
		fc_a = tmp.str();
		//printf("##################################\n" );
		break;
		
	    case SQL_SHORT:
		sprintf((char *)p2, "%d", *(short *) this->BufferData + this->FBCol[col].offset); 
		tmp << p2;
		fc_a = tmp.str();

		break;
		
	    case SQL_LONG:
		sprintf((char *)p2, "%ld", *(long *) this->BufferData + this->FBCol[col].offset);
		tmp << p2;
		fc_a = tmp.str();
		break;
		
	    case SQL_INT64:
		len = this->FBCol[col].length;
		dscale = this->METAO->getScale (&status, col);
		//ISC_INT64 value;
		ISC_INT64 tens;

	        value = 0;	
		value = (ISC_INT64) *(ISC_INT64 ISC_FAR *) (this->BufferData + this->FBCol[col].offset);
		break;
		
                case SQL_FLOAT:
                sprintf ((char *)p2, "%15g", *(float ISC_FAR *) (this->BufferData + this->FBCol[col].offset));
		tmp << p2;
		fc_a = tmp.str();
                break;

            case SQL_DOUBLE:
                sprintf((char *)p2, "%24f", *(double ISC_FAR *) (this->BufferData + this->FBCol[col].offset));
		tmp << p2;
		fc_a = tmp.str();
                //len = 24;
                break;
		
	    //case   
            case SQL_TIMESTAMP:
                isc_decode_timestamp((ISC_TIMESTAMP ISC_FAR *)this->BufferData + this->FBCol[col].offset, &times);
                sprintf(date_s, "%04d-%02d-%02d %02d:%02d:%02d.%04d", times.tm_year + 1900, times.tm_mon+1, times.tm_mday, times.tm_hour, times.tm_min, times.tm_sec, ((ISC_TIMESTAMP *)this->BufferData + this->FBCol[col].offset)-> timestamp_time % 10000);
                sprintf((char *)pTemp, "%*s", this->FBCol[col].length, date_s);

		//printf("##################################\n" );
		//printf("%s\n", p2);
		tmp << pTemp;

		
		fc_a = tmp.str();
		//exit(0);
                break;

            case SQL_TYPE_DATE:
                isc_decode_sql_date((ISC_DATE ISC_FAR *)this->BufferData + this->FBCol[col].offset, &times);
                sprintf(date_s, "%04d-%02d-%02d", times.tm_year + 1900, times.tm_mon+1, times.tm_mday); sprintf((char *)p2, "%*s", this->FBCol[col].length, date_s);
		tmp << p2;
		fc_a = tmp.str();
                break;

            case SQL_TYPE_TIME:
                isc_decode_sql_time((ISC_TIME ISC_FAR *)this->BufferData + this->FBCol[col].offset, &times);
                sprintf(date_s, "%02d:%02d:%02d.%04d", times.tm_hour, times.tm_min, times.tm_sec, (*((ISC_TIME *)this->BufferData + this->FBCol[col].offset)) % 10000);
                sprintf((char *)p2, "%*s", this->FBCol[col].length, date_s);
		printf("Olha o tempo %s\n", p2);
		tmp << p2;
		fc_a = tmp.str();
                break;

	   case SQL_ARRAY:
           case SQL_BLOB:
	        //printf("TAMANHO DO BLOB:)--->: %d\n", this->FBCol[col].length);	
		int pos;
		unsigned len;
		pos = 0;
		int valor;
		
    		this->blob = this->ATTATCHMENT->openBlob(&status, this->TRANSACTION, (ISC_QUAD*)(this->BufferData), 0, NULL);

		this->BlobID.gds_quad_high = bid.isc_quad_high;
                this->BlobID.gds_quad_low  = bid.isc_quad_low;

                sprintf(blob_s, "%08x:%08x", bid.isc_quad_high, bid.isc_quad_low);
                sprintf((char *)p2, "%17s", blob_s);
                //sprintf((string *)p, "%17s", blob_s);
		break;

	        default:
			break;
	    //case 
    }		

    if( dscale < 0 ) {

		int tens = 1;;
		for(int i = 0; i > dscale; i--)
		{
			tens *= 10;
			if(value > 0)
			{
                        	sprintf ((char *)p2, "%0*" "ll" "d.%0*" "ll" "d", this->FBCol[col].length - 1 + dscale, (ISC_INT64) (value / tens), -dscale, (ISC_INT64) (value % tens));
			} // End if


		} // End For

		tmp << p2;
		fc_a = tmp.str();
    
    }
		
return fc_a;

}


/* PUBLIC: GETROW
** COPY FROM BUFFER TO USER VAR
*/
int FBConnect::getRow (unsigned char *dados)
{
    if (! this->InTrans    ||   this->TRANSACTION == 0L) return -1;
    if (! this->BufferData || ! this->BufferSize)        return -1;


   // IMPLEMENTAR MEMORY PRINT

	//printf("TAMANHO: %d\n", this->FCount);
	//printf("TAMANHO:\n");
	//exit(0);
    int a = 2;	
    
    for (unsigned j = 0; j < this->FCount; ++j) {
		printf("%d\n", as<int>(this->BufferData));
	printf(" %s --> ", this->FBCol[j].name);
    }
    

    return 0;
}

/* PUBLIC : SELECT
** SELECIONAR VARIAS LINHAS DE DADOS
*/
int FBConnect::Select (const char *stmt)
{
    // 0. VERIFICA STATMENT
    // {
    if (strlen (stmt) < 15) { 
        strcpy (this->ErrorMsg, "FBConnect::Select: VERIFY STATMENT LESS THAN 15 CHARS");
        return -1; }
    // }

    // 1. VERIFICA STATUS DA TRANSACAO E PREPARA STATEMENT
    // {
    // Iniciar automaticamente uma transacao
    if (! this->InTrans && this->Start (FBREAD)) return -1;
    // Prepara a transacao
    
    if (this->Prepare (stmt)) {
        strcpy (this->ErrorMsg, "FBConnect::Select:Prepare: UNABLE TO SETUP STATEMENT");
        return -1; }

    this->isFetchOK = true;

    return 0;
}

int FBConnect::Execute (const char *stmt)
{
    return 0;
}

int FBConnect::ExecuteBind (const char *stmt, char **dados, int dimensao)
{
    return 0;
}

/* PUBLIC : GetTextBlob
** RETURN THE CHARACTERS OF A BLOB TEXT COLUMN
*/
//int FBConnect::GetTextBlob  (unsigned char* d)
string FBConnect::GetTextBlob ()
{

    char *p;
    //ThrowStatusWrapper status(this->STATUS);
    if (! this->InTrans    ||   this->TRANSACTION == 0L) "Error";
    if (! this->BufferData || ! this->BufferSize)        "Error";
    

    ThrowStatusWrapper status(this->STATUS);
    unsigned len;
    string d;



    	memset (this->BlobSegment, '\0', sizeof (this->BlobSegment));


	unsigned soma = 0;
		for(;;)

		{

			int cc = this->blob->getSegment(&status, sizeof(this->BlobSegment) , this->BlobSegment, &len);
			
			if (cc != IStatus::RESULT_OK && cc != IStatus::RESULT_SEGMENT)
				break;
			
			//strcat((char *)d, this->BlobSegment);
            stringstream tmp;
            tmp << this->BlobSegment;
            d += tmp.str();
			
    		memset (this->BlobSegment, '\0', sizeof (this->BlobSegment));

		}	

		
   this->blob->close(&status);
   this->blob = NULL;

   if (this->blob)
	   this->blob->release();
    //blob->close(&status);
    this->blob = NULL;
    return d;
}

// ATUALIZACAO 18 DE NOVEMBRO DE 2018
//int FBConnect::getDataSize  (XSQLVAR ISC_FAR * var , char * p)
int FBConnect::getDataSize  (int col)
{

    if (! this->InTrans    ||   this->TRANSACTION == 0L) return -1;
    if (! this->BufferData || ! this->BufferSize)        return -1;

    ThrowStatusWrapper status(this->STATUS);

    if( ! this->isFetchOK ) {
        return -1;
    }
 
    // Armazena o retorno do sprintf
    int n = 0;
    int len; 
    len = 0;
    unsigned t = FBCol[col].typeData;
    ISC_INT64 value;
    switch (t)
    {
	    case SQL_TEXT:
		len = this->FBCol[col].length;
		break;
	    case SQL_VARYING:
		len = this->FBCol[col].length;
		break;
	    case SQL_SHORT:
		len = this->FBCol[col].length;
		break;
	    case SQL_LONG:
		len = this->FBCol[col].length;
		break;
	    case SQL_INT64:
		len = this->FBCol[col].length;
                break;
            case SQL_DOUBLE:
		len = this->FBCol[col].length;
                break;
            case SQL_TIMESTAMP:
		len = this->FBCol[col].length;
		//len = this->BufferSize;
		//printf("VALOR ----> %d\n", len);
		//exit(0);
                break;
            case SQL_TYPE_DATE:
		len = this->FBCol[col].length;
                break;
            case SQL_TYPE_TIME:
		len = this->FBCol[col].length;
                break;
	   case SQL_ARRAY:
           case SQL_BLOB:
		//len = this->blobSize(col);
		len = this->FBCol[col].length;
		//len = 10;
		break;
	        default:
			//printf("OK\n");
			break;
		} // End switch
   return  len;
 }

/*
 * RETURN IF A COLUMN IS BLOB OR NOT
 * 0 - FALSE
 * 1 - TRUE 
 * */
int FBConnect::isBlob(int col) {

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
	return FBCol[col].subData;
}
/*
 * GET THE HANDLER OF BLOB AND THE SIZE OF THE CHARACTERS
 *
 * */
int FBConnect::workBlob  (unsigned char * bid)
{
    if (! this->InTrans    ||   this->TRANSACTION == 0L) return -1;
    if (! this->BufferData || ! this->BufferSize)        return -1;

    ThrowStatusWrapper status(this->STATUS);

    if( ! this->isFetchOK ) {
        return -1;
    }


        if(!this->blob)
        	this->blob = this->ATTATCHMENT->openBlob(&status, this->TRANSACTION, (ISC_QUAD *)bid, 0, NULL);

	return 1;
}

/*
 * GET THE NUMBER OF CHARACTERS OF A BLOB
 *
 * */
int FBConnect:: getBlobSize(IBlob * handler)
{
	/*
	if( this->blob )
	{
		return 0;
	}
*/

	unsigned itemsLength;
        unsigned bufferLength;
        unsigned char* buffer;
        unsigned bb;
        unsigned char bbchar[200], *p, item;
        //unsigned char* bbchar;

        static unsigned char items[] = {
        isc_info_blob_max_segment,
        isc_info_blob_total_length};

        ThrowStatusWrapper status(this->STATUS);
        //this->blob->getInfo(&status, 2, items, sizeof(bbchar), bbchar );
        handler->getInfo(&status, 2, items, sizeof(bbchar), bbchar );


	ISC_LONG max_size;
        ISC_LONG num_segments;
	int total_length;
	
	max_size = 0L;
	num_segments = 0L;

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

