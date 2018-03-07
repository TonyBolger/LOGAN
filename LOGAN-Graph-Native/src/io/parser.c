#include "common.h"


static void initSequenceBuffer(SwqBuffer *swqBuffer, int bufSize, int recordsPerBatch, int maxReadLength)
{
	if(bufSize>0)
		{
		swqBuffer->seqBuffer=G_ALLOC(bufSize, MEMTRACKID_SWQ);
		swqBuffer->qualBuffer=G_ALLOC(bufSize, MEMTRACKID_SWQ);
		}
	else
		{
		swqBuffer->seqBuffer=NULL;
		swqBuffer->qualBuffer=NULL;
		}

	swqBuffer->rec=G_ALLOC(sizeof(SequenceWithQuality)*recordsPerBatch, MEMTRACKID_SWQ);

	swqBuffer->maxSequenceTotalLength=bufSize;
	swqBuffer->maxSequences=recordsPerBatch;
	swqBuffer->maxSequenceLength=maxReadLength;
	swqBuffer->numSequences=0;
	swqBuffer->usageCount=0;

}

void prInitParseBuffer(ParseBuffer *parseBuffer)
{
	parseBuffer->ioBuffer=G_ALLOC(PARSER_IO_RECYCLE_BUFFER+PARSER_IO_PRIMARY_BUFFER, MEMTRACKID_IOBUF);

	for(int i=0;i<PT_INGRESS_BUFFERS;i++)
		{
		parseBuffer->ingressBuffers[i].ingressPtr=NULL;
		parseBuffer->ingressBuffers[i].ingressTotal=0;
		parseBuffer->ingressBuffers[i].ingressUsageCount=NULL;
		initSequenceBuffer(parseBuffer->swqBuffers+i, PARSER_BASES_PER_BATCH, PARSER_SEQ_PER_BATCH, PARSER_MAX_SEQ_LENGTH);
		}
}

static void freeSequenceBuffer(SwqBuffer *swqBuffer)
{
	if(swqBuffer->seqBuffer!=NULL)
		G_FREE(swqBuffer->seqBuffer, swqBuffer->maxSequenceTotalLength, MEMTRACKID_SWQ);

	if(swqBuffer->qualBuffer!=NULL)
		G_FREE(swqBuffer->qualBuffer, swqBuffer->maxSequenceTotalLength, MEMTRACKID_SWQ);

	G_FREE(swqBuffer->rec, sizeof(SequenceWithQuality)*swqBuffer->maxSequences, MEMTRACKID_SWQ);

	memset(swqBuffer, 0, sizeof(SwqBuffer));
}


void prFreeParseBuffer(ParseBuffer *parseBuffer)
{
	G_FREE(parseBuffer->ioBuffer, FASTQ_IO_RECYCLE_BUFFER+FASTQ_IO_PRIMARY_BUFFER, MEMTRACKID_IOBUF);

	for(int i=0;i<PT_INGRESS_BUFFERS;i++)
		{
		freeSequenceBuffer(parseBuffer->swqBuffers+i);

		if(parseBuffer->ingressBuffers[i].ingressUsageCount!=NULL && *(parseBuffer->ingressBuffers[i].ingressUsageCount)>0)
			LOG(LOG_CRITICAL,"Ingress Buffer still in use after shutdown");
		}
}



static int guessFileFormatByExtensions(char *path)
{
	int format;

	int pathLen=strlen(path);
	char *pathClone=alloca(pathLen+1);
	strncpy(pathClone, path, pathLen+1);

	char *lastDot=strrchr(pathClone, '.');

	if(lastDot==NULL)
		return PARSE_FILE_CONTENT_UNKNOWN;

	if(!strcmp(lastDot,".gz"))
		format=PARSE_FILE_COMPRESSION_GZIP;
	else
		format=PARSE_FILE_COMPRESSION_NONE;

	if(format!=PARSE_FILE_COMPRESSION_NONE)
		{
		*lastDot=0;
		lastDot=strrchr(pathClone, '.');
		if(lastDot==NULL)
			return PARSE_FILE_CONTENT_UNKNOWN;
		}

	if(!strcmp(lastDot,".fq"))
		return format|PARSE_FILE_CONTENT_FASTQ;

	if(!strcmp(lastDot,".fastq"))
		return format|PARSE_FILE_CONTENT_FASTQ;


	if(!strcmp(lastDot,".fa"))
		return format|PARSE_FILE_CONTENT_FASTA;

	if(!strcmp(lastDot,".fasta"))
		return format|PARSE_FILE_CONTENT_FASTA;


	return PARSE_FILE_CONTENT_UNKNOWN;
}






s64 prParseAndProcess(char *path, int minSeqLength, s64 recordsToSkip, s64 recordsToUse, ParseBuffer *parseBuffer,
		void *handlerContext, void (*handler)(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *handlerContext),
		void (*monitor)())
{
	s64 sequences=0;
	int format=guessFileFormatByExtensions(path);

	switch(format)
		{
		case PARSE_FILE_CONTENT_FASTQ:
			LOG(LOG_INFO,"%s:- Content: FastQ Compression: None", path);
			sequences=fqParseAndProcess(path, PARSER_MIN_SEQ_LENGTH, 0, LONG_MAX,
					parseBuffer->ioBuffer, PARSER_IO_RECYCLE_BUFFER, PARSER_IO_PRIMARY_BUFFER,
					parseBuffer->swqBuffers, parseBuffer->ingressBuffers, PT_INGRESS_BUFFERS,
					handlerContext, handler, monitor);
			break;

		case PARSE_FILE_CONTENT_FASTQ | PARSE_FILE_COMPRESSION_GZIP:
		LOG(LOG_INFO,"%s:- Content: FastQ Compression: GZip", path);
			LOG(LOG_INFO,"Handle GZIP'ed Fastq - TODO");
			break;

		case PARSE_FILE_CONTENT_FASTA:
			LOG(LOG_INFO,"%s:- Content: FastA Compression: None", path);
			sequences=faParseAndProcess(path, PARSER_MIN_SEQ_LENGTH, 0, LONG_MAX,
					parseBuffer->ioBuffer, PARSER_IO_RECYCLE_BUFFER, PARSER_IO_PRIMARY_BUFFER,
					parseBuffer->swqBuffers, parseBuffer->ingressBuffers, PT_INGRESS_BUFFERS,
					handlerContext, handler, monitor);
			break;

		case PARSE_FILE_CONTENT_FASTA | PARSE_FILE_COMPRESSION_GZIP:
			LOG(LOG_INFO,"%s:- Content: FastA Compression: GZip", path);
			LOG(LOG_INFO,"Handle GZIP'ed Fasta - TODO");
			break;

		default:
			LOG(LOG_CRITICAL,"%s:- Unknown File Format", path);

		}

	return sequences;
}


void prIndexingBuilderDataHandler(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *context)
{
	IndexingBuilder *ib=(IndexingBuilder *)context;

	ingressBuffer->ingressPtr=swqBuffer;
	ingressBuffer->ingressTotal=swqBuffer->numSequences;
	ingressBuffer->ingressUsageCount=&swqBuffer->usageCount;

	queueIngress(ib->pt, ingressBuffer);
}


void prRoutingBuilderDataHandler(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *context)
{
	RoutingBuilder *rb=(RoutingBuilder *)context;

	ingressBuffer->ingressPtr=swqBuffer;
	ingressBuffer->ingressTotal=swqBuffer->numSequences;
	ingressBuffer->ingressUsageCount=&swqBuffer->usageCount;

	queueIngress(rb->pt, ingressBuffer);

}



