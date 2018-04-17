
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


// static
void writeNodes(Graph *graph)
{
	LOG(LOG_INFO,"Writing Nodes");
	int fd=open("test.nodes", O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);

	GraphSerdes serdes;
	serInitSerdes(&serdes, graph);

	serWriteNodes(&serdes, fd);

	serCleanupSerdes(&serdes);

	close(fd);
}


//static
void writeEdges(Graph *graph)
{
	LOG(LOG_INFO,"Writing Edges");
	int fd=open("test.edges", O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);

	GraphSerdes serdes;
	serInitSerdes(&serdes, graph);

	serWriteEdges(&serdes, fd);

	serCleanupSerdes(&serdes);

	close(fd);
}


//static
void writeRoutes(Graph *graph)
{
	LOG(LOG_INFO,"Writing Routes");
	int fd=open("test.routes", O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);

	GraphSerdes serdes;
	serInitSerdes(&serdes, graph);

	serWriteRoutes(&serdes, fd);

	serCleanupSerdes(&serdes);

	close(fd);
}

//static
void readNodes(Graph *graph)
{
	LOG(LOG_INFO,"Reading Nodes");

	int fd=open("test.nodes", O_RDONLY);

	GraphSerdes serdes;
	serInitSerdes(&serdes, graph);

	serReadNodes(&serdes, fd);

	serCleanupSerdes(&serdes);

	close(fd);
}

//static
void readEdges(Graph *graph)
{
	LOG(LOG_INFO,"Reading Edges");

	int fd=open("test.edges", O_RDONLY);

	GraphSerdes serdes;
	serInitSerdes(&serdes, graph);

	serReadEdges(&serdes, fd);

	serCleanupSerdes(&serdes);

	close(fd);
}

//static
void readRoutes(Graph *graph)
{
	LOG(LOG_INFO,"Reading Routes");

	int fd=open("test.routes", O_RDONLY);

	GraphSerdes serdes;
	serInitSerdes(&serdes, graph);

	serReadRoutes(&serdes, fd);

	serCleanupSerdes(&serdes);

	close(fd);
}








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

#ifdef FEATURE_ENABLE_MEMTRACK
	mtInit();
	mtDump();
#endif

	Graph *graph=grAllocGraph(23,23,NULL);
/*
	int threadCountIndexing=atoi(argv[1]);
	int threadCountRouting=atoi(argv[2]);

	char **filePaths=argv+3;
	int fileCount=argc-3;

	buildGraphFromSequenceFiles(filePaths, fileCount, graph, threadCountIndexing, threadCountRouting);

	writeNodes(graph);
	writeEdges(graph);
	writeRoutes(graph);
*/
	grSwitchMode(graph);

	readNodes(graph);
	readEdges(graph);
	readRoutes(graph);

	//runRptMaster(filePaths, fileCount, threadCountRouting, graph);

	grFreeGraph(graph);

#ifdef FEATURE_ENABLE_MEMTRACK
	mtDump();
#endif

	return 0;
}

