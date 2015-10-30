#ifndef __TASKROUTING_H
#define __TASKROUTING_H





typedef struct RoutingBuilderStr {

	ParallelTask *pt;


} RoutingBuilder;



RoutingBuilder *allocRoutingBuilder(Graph *graph, int threads);
void freeRoutingBuilder(RoutingBuilder *rb);


#endif
