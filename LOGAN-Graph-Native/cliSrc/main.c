
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
				ib, prIndexingBuilderDataHandler, monitor, &(graph->seqIndex));

		LOG(LOG_INFO,"Indexing: Parsed %li sequences from %s",reads,path);
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
				rb, prRoutingBuilderDataHandler, monitor, NULL);

		LOG(LOG_INFO,"Routing: Parsed %li sequences from %s",reads,path);
		}


	#ifdef FEATURE_ENABLE_MEMTRACK
		mtDump();
	#endif

	queueShutdown(rb->pt);

	waitForShutdown(rb->pt);

	prFreeParseBuffer(&parseBuffer);
	freeRoutingBuilder(rb);
}





/*

//static
void buildGraphFromSequenceFiles(char **filePaths, int fileCount, Graph *graph, int threadCountIndexing, int threadCountRouting)
{
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
#endif

}
*/


/*

	Index indexingThreads graph files...
	Route routingThreads graph files...
	IndexAndRoute indexingThreads routingThreads graph files...

	TestIndex indexingThreads files...
	TestRoute routingThreads graph files...
	TestIndexAndRoute indexingThreads routingThreads files...

*/


int main(int argc, char **argv)
{
	logInit();

	int argsOK=1;

	int threadCountIndexing=0;
	int threadCountRouting=0;

	int filePathOffset=1;
	char *inGraphBase=NULL;
	char *outGraphBase=NULL;

	int doIndex=0;
	int doReadNodes=0;
	int doReadEdges=0;
	int doReadRoutes=0;
	int doRoute=0;

	int doWriteNodes=0;
	int doWriteEdges=0;
	int doWriteRoutes=0;

	if(argc>=2)
		{
		char *command=argv[1];

		if(!strcmp(command,"Index") && argc>4)
			{
			threadCountIndexing=atoi(argv[2]);
			outGraphBase=argv[3];
			filePathOffset=4;

			doIndex=1;
			doWriteNodes=1;
			}
		else if(!strcmp(command,"Route") && argc>4)
			{
			threadCountRouting=atoi(argv[2]);
			inGraphBase=argv[3];
			outGraphBase=argv[3];
			filePathOffset=4;

			doReadNodes=1;
			doRoute=1;
			doWriteEdges=1;
			doWriteRoutes=1;
			}
		else if(!strcmp(command,"IndexAndRoute") && argc>5)
			{
			threadCountIndexing=atoi(argv[2]);
			threadCountRouting=atoi(argv[3]);
			outGraphBase=argv[4];
			filePathOffset=5;

			doIndex=1;
			doWriteNodes=1;
			doRoute=1;
			doWriteEdges=1;
			doWriteRoutes=1;
			}

		else if(!strcmp(command,"TestIndex") && argc>3)
			{
			threadCountIndexing=atoi(argv[2]);
			filePathOffset=3;

			doIndex=1;
			}
		else if(!strcmp(command,"TestRoute") && argc>4)
			{
			threadCountRouting=atoi(argv[2]);
			inGraphBase=argv[3];
			filePathOffset=4;

			doReadNodes=1;
			doRoute=1;
			}
		else if(!strcmp(command,"TestIndexAndRoute") && argc>4)
			{
			threadCountIndexing=atoi(argv[2]);
			threadCountRouting=atoi(argv[3]);
			filePathOffset=4;

			doIndex=1;
			doRoute=1;
			}
		else if(!strcmp(command,"ReadGraph") && argc==2)
			{
			inGraphBase=argv[2];

			doReadNodes=1;
			doReadEdges=1;
			doReadRoutes=1;
			}
		else
			{
			LOG(LOG_INFO,"Invalid command (%s) / arguments (%i)", argv[1], argc);
			}
		}
	else
		{
		LOG(LOG_INFO,"Insufficient arguments (%i)", argc);
		argsOK=0;
		}

	if(!argsOK)
		{
		LOGN(LOG_INFO,"Expected one of:", argc);

		LOGN(LOG_INFO,"\tIndex <indexingThreads> <graph> <seqFiles>...");
		LOGN(LOG_INFO,"\tRoute <routingThreads> <graph> <seqFiles>...");
		LOGN(LOG_INFO,"\tIndexAndRoute <indexingThreads> <routingThreads> <graph> <seqFiles>...");

		LOGN(LOG_INFO,"\tTestIndex <indexingThreads> <seqFiles>...");
		LOGN(LOG_INFO,"\tTestRoute <routingThreads> <graph> <seqFiles>...");
		LOGN(LOG_INFO,"\tTestIndexAndRoute <indexingThreads> <routingThreads> <seqFiles>...");

		LOGN(LOG_INFO,"\tReadGraph <graph>");

		exit(1);
		}

	char **filePaths=argv+filePathOffset;
	int fileCount=argc-filePathOffset;

	mtInit();
	mtDump();

	Graph *graph=grAllocGraph(23,23,NULL);
	mtDump();

	if(doIndex)
		{
		runIptMaster(filePaths, fileCount, threadCountIndexing, graph);
		mtDump();
		}

	grSwitchMode(graph);
	mtDump();

	if(doReadNodes)
		{
		char *path=alloca(strlen(inGraphBase)+6);
		strcpy(path, inGraphBase);
		strcat(path, ".nodes");

		LOG(LOG_INFO, "Read Nodes from %s",path);

		serReadNodesFromFile(graph, path);

		mtDump();
		}

	if(doReadEdges)
		{
		char *path=alloca(strlen(inGraphBase)+6);
		strcpy(path, inGraphBase);
		strcat(path, ".edges");

		LOG(LOG_INFO, "Read Edges from %s",path);

		serReadEdgesFromFile(graph, path);

		mtDump();
		}

	if(doReadRoutes)
		{
		char *path=alloca(strlen(inGraphBase)+7);
		strcpy(path, inGraphBase);
		strcat(path, ".routes");

		LOG(LOG_INFO, "Read Routes from %s",path);

		serReadRoutesFromFile(graph, path);

		mtDump();
		}

	if(doWriteNodes)
		{
		char *path=alloca(strlen(outGraphBase)+6);
		strcpy(path, outGraphBase);
		strcat(path, ".nodes");

		LOG(LOG_INFO, "Write Nodes to %s",path);

		serWriteNodesToFile(graph, path);

		mtDump();
		}

	if(doRoute)
		{
		runRptMaster(filePaths, fileCount, threadCountRouting, graph);
		mtDump();
		}

	if(doWriteEdges)
		{
		char *path=alloca(strlen(outGraphBase)+6);
		strcpy(path, outGraphBase);
		strcat(path, ".edges");

		LOG(LOG_INFO, "Write Edges to %s",path);

		serWriteEdgesToFile(graph, path);

		mtDump();
		}

	if(doWriteRoutes)
		{
		char *path=alloca(strlen(outGraphBase)+7);
		strcpy(path, outGraphBase);
		strcat(path, ".routes");

		LOG(LOG_INFO, "Write Routes to %s",path);

		serWriteRoutesToFile(graph, path);

		mtDump();
		}



	grFreeGraph(graph);

	mtDump();

	return 0;
}

