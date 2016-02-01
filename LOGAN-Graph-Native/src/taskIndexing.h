#ifndef __TASKINDEXING_H
#define __TASKINDEXING_H



#define TI_INGRESS_BLOCKSIZE 10240

typedef struct IndexingBuilderStr {
	ParallelTask *pt;
	Graph *graph;

} IndexingBuilder;



IndexingBuilder *allocIndexingBuilder(Graph *graph, int threads);
void freeIndexingBuilder(IndexingBuilder *ib);



#endif
