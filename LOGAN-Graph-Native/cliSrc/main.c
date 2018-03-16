
#include "cliCommon.h"

void runIptMaster(char **filePaths, int fileCount, int threadCount, Graph *graph)
{
	IndexingBuilder *ib=allocIndexingBuilder(graph, threadCount);
	createIndexingBuilderWorkers(ib);
	waitForStartup(ib->pt);

	LOG(LOG_INFO,"Indexing: Ready to parse");

	ParseBuffer parseBuffer;
	prInitParseBuffer(&parseBuffer);

	void (*monitor)() = NULL;

#ifdef FEATURE_ENABLE_MEMTRACK_CONTINUOUS
	monitor=mtDump;
#endif

	for(int i=0;i<fileCount;i++)
		{
		char *path=filePaths[i];

		LOG(LOG_INFO,"Indexing: Parsing %s",path);

		s64 reads=prParseAndProcess(path, PARSER_MIN_SEQ_LENGTH, 0, LONG_MAX, &parseBuffer,
				ib, prIndexingBuilderDataHandler, monitor);

		LOG(LOG_INFO,"Indexing: Parsed %i reads from %s",reads,path);
		}

	#ifdef FEATURE_ENABLE_MEMTRACK
		mtDump();
	#endif

	queueShutdown(ib->pt);

	waitForShutdown(ib->pt);

	prFreeParseBuffer(&parseBuffer);
	freeIndexingBuilder(ib);
}


void runRptMaster(char **filePaths, int fileCount, int threadCount, Graph *graph)
{
	RoutingBuilder *rb=allocRoutingBuilder(graph, threadCount);

	createRoutingBuilderWorkers(rb);

	waitForStartup(rb->pt);

	LOG(LOG_INFO,"Routing: Ready to parse");

	ParseBuffer parseBuffer;
	prInitParseBuffer(&parseBuffer);

	void (*monitor)() = NULL;

#ifdef FEATURE_ENABLE_MEMTRACK_CONTINUOUS
	monitor=mtDump;
#endif

	for(int i=0;i<fileCount;i++)
		{
		char *path=filePaths[i];

		LOG(LOG_INFO,"Routing: Parsing %s",path);

		s64 reads=prParseAndProcess(path, PARSER_MIN_SEQ_LENGTH, 0, LONG_MAX, &parseBuffer,
				rb, prRoutingBuilderDataHandler, monitor);

		LOG(LOG_INFO,"Routing: Parsed %i reads from %s",reads,path);
		}


	#ifdef FEATURE_ENABLE_MEMTRACK
		mtDump();
	#endif

	queueShutdown(rb->pt);

	waitForShutdown(rb->pt);

	prFreeParseBuffer(&parseBuffer);
	freeRoutingBuilder(rb);
}



int main(int argc, char **argv)
{
	logInit();

	// argv[1] = IndexingThreads
	// argv[2] = RoutingThreads
	// argv[3..] = Files

	if(argc<3)
		{
		LOG(LOG_CRITICAL,"Expected arguments: indexingThreads routingThreads files...");
		return 1;
		}

	int threadCountIndexing=atoi(argv[1]);
	int threadCountRouting=atoi(argv[2]);

	char **filePaths=argv+3;
	int fileCount=argc-3;

//	LOG(LOG_INFO,"RoutingReadLookupData: %i (20) RoutingReadData: %i (20) RoutingReadIndexedDataEntry: %i (28)",
//			sizeof(RoutingReadLookupData), sizeof(RoutingReadData), sizeof(RoutingReadIndexedDataEntry));

	LOG(LOG_INFO,"SequenceLink: %i (64) LookupLink: %i (128) DispatchLink: %i (128)",
			sizeof(SequenceLink), sizeof(LookupLink), sizeof(DispatchLink));

	LOG(LOG_INFO,"MemSingleBrickChunk: %i (4194304) MemDoubleBrickChunk %i (8388608)", sizeof(MemSingleBrickChunk), sizeof(MemDoubleBrickChunk));


#ifdef FEATURE_ENABLE_MEMTRACK
	mtInit();
	mtDump();
#endif

	Graph *graph=grAllocGraph(23,23,NULL);

	//runTpfMaster(fileTemplate, fileCount, graph);

#ifdef FEATURE_ENABLE_MEMTRACK
	mtDump();
#endif

	runIptMaster(filePaths, fileCount, threadCountIndexing, graph);

#ifdef FEATURE_ENABLE_MEMTRACK
	mtDump();
#endif

	LOG(LOG_INFO,"Smer count: %i",smGetSmerCount(&(graph->smerMap)));

	//smDumpSmerMap(&(graph->smerMap));

	grSwitchMode(graph);

#ifdef FEATURE_ENABLE_MEMTRACK
	mtDump();
#endif

	runRptMaster(filePaths, fileCount, threadCountRouting, graph);

#ifdef FEATURE_ENABLE_MEMTRACK
	mtDump();
//	LOG(LOG_INFO,"Graph construction complete");
//	sleep(60);

#endif

	grFreeGraph(graph);

#ifdef FEATURE_ENABLE_MEMTRACK
	mtDump();
#endif

	return 0;
}

