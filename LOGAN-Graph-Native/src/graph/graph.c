/*
 * graph.c
 *
 *  Created on: Nov 10, 2010
 *      Author: tony
 */

#include "common.h"





void switchMode(Graph *graph)
{
	if(graph->mode!=GRAPH_MODE_INDEX)
		{
		LOG(LOG_INFO,"Graph already in Routing Mode");
		return;
		}

	LOG(LOG_INFO,"Switching Graph to Routing Mode");
	graph->mode=GRAPH_MODE_ROUTE;

	int entries=saInitSmerArray(&(graph->smerArray), &(graph->smerMap), rtItemSizeResolver);

	smCleanupSmerMap(&(graph->smerMap));

	LOG(LOG_INFO,"Built SmerArrays with %i entries",entries);
}


Graph *allocGraph(s32 nodeSize, s32 sparseness, void *userPtr)
{
	LOG(LOG_INFO,"Allocating Graph with NodeSize %i and Sparseness %i",nodeSize,sparseness);

	Graph *graph=grGraphAlloc();

	graph->config.nodeSize=nodeSize;
	graph->config.sparseness=sparseness;
	graph->userPtr=userPtr;

	graph->mode=GRAPH_MODE_INDEX;

	LOG(LOG_INFO,"Allocating SmerMap");

	smInitSmerMap(&(graph->smerMap));

	return graph;
}

void freeGraph(Graph *graph)
{
	LOG(LOG_INFO,"Freeing Graph");

	if(graph->mode==GRAPH_MODE_INDEX)
		smCleanupSmerMap(&(graph->smerMap));

	else
		saCleanupSmerArray(&(graph->smerArray));


	grGraphFree(graph);

}
