
#include "cliCommon.h"

typedef struct tpfThreadDataStr
{
	Graph *graph;
	int threadCount;
	int threadIndex;
	char *pathTemplate;
} TpfThreadData;

typedef struct iptThreadDataStr
{
	Graph *graph;
	IndexingBuilder *indexingBuilder;

	int threadIndex;

} IptThreadData;


#define FASTQ_RECORDS_PER_BATCH 100
#define FASTQ_BASES_PER_BATCH 10000

//#define FASTQ_RECORDS_PER_BATCH 1000
//#define FASTQ_BASES_PER_BATCH 100000

//#define FASTQ_RECORDS_PER_BATCH 10000
//#define FASTQ_BASES_PER_BATCH 1000000

//#define FASTQ_RECORDS_PER_BATCH 100000
//#define FASTQ_BASES_PER_BATCH 10000000


void tpfAddPathSmersHandler(SequenceWithQuality *rec, int numRecords, void *context)
{
	Graph *graph=context;

	//GraphBatchContext *ctx=context;

	u8 *packedSeq=alloca(FASTQ_MAX_READ_LENGTH);
	int i=0;

	for(i=0;i<numRecords;i++)
		{
		SequenceWithQuality *currentRec=rec+i;

		packSequence(currentRec->seq, packedSeq, currentRec->length);
		addPathSmers(graph, currentRec->length, packedSeq);
		}
}

void tpfAddPathRoutesHandler(SequenceWithQuality *rec, int numRecords, void *context)
{
	/*
	GraphBatchContext *ctx=context;

	int i=0;

	u8 *packPtr=ctx->packedSequences;

	for(i=0;i<numRecords;i++)
		{
		FastqRecord *currentRec=rec+i;

		int packLen=packSequence(currentRec->seq, packPtr, currentRec->length);

		ctx->reads[i].readLength=currentRec->length;
		ctx->reads[i].packedSequence=packPtr;

		packPtr+=packLen;
		}

	ctx->numReads=numRecords;
	graphBatchAddPathRoutes(ctx);

	for(i=0;i<numRecords;i++)
		addPathRoutes(ctx->graph, ctx->reads[i].readLength, ctx->reads[i].packedSequence);
*/
}


void *tpfWorker(void *voidData)
{
	TpfThreadData *data=(TpfThreadData *)voidData;

	char path[1024];
	sprintf(path,data->pathTemplate,data->threadCount,data->threadIndex);

	LOG(LOG_INFO,"Thread %i of %i started for %s",data->threadIndex,data->threadCount,path);

	//char *path, int minSeqLength, int recordsToSkip, int recordsToUse, void *handlerContext, void (*handler)

	int bufSize=FASTQ_BASES_PER_BATCH;

	char *seqBuffer[1];
	char *qualBuffer[1];
	SequenceWithQuality *rec[1];

	seqBuffer[0]=malloc(bufSize);
	qualBuffer[0]=malloc(bufSize);
	rec[0]=malloc(sizeof(SequenceWithQuality)*FASTQ_RECORDS_PER_BATCH);


	int reads=parseAndProcess(path, FASTQ_MIN_READ_LENGTH, 0, 250000000,
			seqBuffer, qualBuffer, rec, 1, FASTQ_BASES_PER_BATCH, FASTQ_RECORDS_PER_BATCH,
			data->graph, tpfAddPathSmersHandler);
//	int reads=parseAndProcess(path, minLength, 0, 1, smerCtx, addPathSmersHandler);


	LOG(LOG_INFO,"Thread %i of %i processed %i",data->threadIndex,data->threadCount,reads);


	return NULL;
}



void runTpfMaster(char *pathTemplate, int fileCount, Graph *graph)
{
	pthread_t *threads=malloc(sizeof(pthread_t)*fileCount);
	TpfThreadData *data=malloc(sizeof(TpfThreadData)*fileCount);

	int i=0;
	for(i=0;i<fileCount;i++)
		{
		data[i].graph=graph;
		data[i].threadCount=fileCount;
		data[i].threadIndex=i;
		data[i].pathTemplate=pathTemplate;

		pthread_create(threads+i,NULL,tpfWorker,data+i);
		}


	void *status;

	for(i=0;i<fileCount;i++)
		pthread_join(threads[i], &status);
}







void *runIptWorker(void *voidData)
{
	IptThreadData *data=(IptThreadData *)voidData;

	IndexingBuilder *ib=data->indexingBuilder;

	performTask(ib->pt);

	return NULL;
}


void iptHandler(SequenceWithQuality *rec, int numRecords, void *context)
{
	IndexingBuilder *ib=(IndexingBuilder *)context;

//	LOG(LOG_INFO," ********************* IPT HANDLER GOT %i ****************************",numRecords);

	queueIngress(ib->pt, rec, numRecords);
}




void runIptMaster(char *pathTemplate, int fileCount, int threadCount, Graph *graph)
{
	IndexingBuilder *ib=allocIndexingBuilder(graph, threadCount);

	pthread_t *threads=malloc(sizeof(pthread_t)*threadCount);

	IptThreadData *data=malloc(sizeof(IptThreadData)*threadCount);

	int i=0;
	for(i=0;i<threadCount;i++)
		{
		data[i].graph=graph;
		data[i].indexingBuilder=ib;
		data[i].threadIndex=i;

		pthread_create(threads+i,NULL,runIptWorker,data+i);
		}

	waitForStartup(ib->pt);


	LOG(LOG_INFO,"Ready to parse");

	// Parse stuff here

	int bufSize=FASTQ_BASES_PER_BATCH;

	char *seqBuffer[PT_INGRESS_BUFFERS];
	char *qualBuffer[PT_INGRESS_BUFFERS];
	SequenceWithQuality *rec[PT_INGRESS_BUFFERS];

	for(int i=0;i<PT_INGRESS_BUFFERS;i++)
		{
		seqBuffer[i]=malloc(bufSize);
		qualBuffer[i]=malloc(bufSize);

		rec[i]=malloc(sizeof(SequenceWithQuality)*FASTQ_RECORDS_PER_BATCH);
		}


	for(i=0;i<fileCount;i++)
		{
		char path[1024];
		sprintf(path,pathTemplate,fileCount,i);

		int reads=parseAndProcess(path, FASTQ_MIN_READ_LENGTH, 0, 250000000,
				seqBuffer, qualBuffer, rec, PT_INGRESS_BUFFERS,  FASTQ_BASES_PER_BATCH, FASTQ_RECORDS_PER_BATCH,
				ib, iptHandler);

		LOG(LOG_INFO,"Parsed %i reads from %s",reads,path);
		}


	queueShutdown(ib->pt);

	waitForShutdown(ib->pt);

	void *status;

	for(i=0;i<threadCount;i++)
		pthread_join(threads[i], &status);

	for(int i=0;i<PT_INGRESS_BUFFERS;i++)
		{
		free(seqBuffer[i]);
		free(qualBuffer[i]);
		free(rec[i]);
		}

	LOG(LOG_INFO,"Graph has %i smers",smGetSmerCount(&(graph->smerMap)));

	freeIndexingBuilder(ib);
}




int main(int argc, char **argv)
{
	logInit();

	Graph *graph=allocGraph(23,23,NULL);

	char *fileTemplate=NULL;
	int fileCount=0;
	int threadCount=0;

	if(argc==3)
		{
		fileTemplate=argv[1];
		fileCount=atoi(argv[2]);
		threadCount=fileCount;
		}
	else if(argc==4)
		{
		fileTemplate=argv[1];
		fileCount=atoi(argv[2]);
		threadCount=atoi(argv[3]);
		}
	else
		{
		LOG(LOG_CRITICAL,"Expected arguments: template files");
		return 1;
		}

	//runTpfMaster(fileTemplate, fileCount, graph);
	runIptMaster(fileTemplate, fileCount, threadCount, graph);

	LOG(LOG_INFO,"Smer count: %i",smGetSmerCount(&(graph->smerMap)));


	freeGraph(graph);

	return 0;
}

