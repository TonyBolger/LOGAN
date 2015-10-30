/*
 * graph.c
 *
 *  Created on: Nov 10, 2010
 *      Author: tony
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "common.h"

Graph *allocGraph(s32 nodeSize, s32 sparseness, void *userPtr)
{
	LOG(LOG_INFO,"Allocating Graph with NodeSize %i and Sparseness %i",nodeSize,sparseness);

	Graph *graph=grGraphAlloc();

	graph->config.nodeSize=nodeSize;
	graph->config.sparseness=sparseness;
	graph->userPtr=userPtr;

	graph->mode=GRAPH_MODE_INDEX;



	return graph;
}

void freeGraph(Graph *graph)
{
	LOG(LOG_INFO,"Freeing Graph");


}
