#ifndef __TASKINDEXING_H
#define __TASKINDEXING_H





typedef struct IndexingBuilderStr {
	ParallelTask *pt;
	Graph *graph;

} IndexingBuilder;



IndexingBuilder *allocIndexingBuilder(Graph *graph, int threads);
void freeIndexingBuilder(IndexingBuilder *ib);



#endif
