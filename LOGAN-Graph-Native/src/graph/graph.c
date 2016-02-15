/*
 * graph.c
 *
 *  Created on: Nov 10, 2010
 *      Author: tony
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../common.h"


static int checkStealthIndexing(s32 *indexes, u32 indexCount, SmerId *smerIds, SmerId smerId)
{
	int i=0;

	for(i=0;i<indexCount;i++)
	{
		if(smerIds[indexes[i]]==smerId)
			return 1;
	}

	return 0;
}




void addPathSmers(Graph *graph, u32 dataLength, u8 *data)
{
	//LOG(LOG_INFO, "Asked to preindex path of %i", dataLength);

	SmerMap *smerMap=&(graph->smerMap);

	s32 indexMaxDistance=graph->config.sparseness;
	s32 maxValidIndex=dataLength-graph->config.nodeSize;

//	LOG(LOG_INFO,"Max dist: %i",indexMaxDistance);

	s32 *oldIndexes=alloca((maxValidIndex+1)*sizeof(u32));
	s32 *newIndexes=alloca((maxValidIndex+1)*sizeof(u32));
	u32 newIndexCount=0;

	SmerId *smerIds=alloca((maxValidIndex+1)*sizeof(SmerId));
	calculatePossibleSmers(data, maxValidIndex, smerIds, NULL);

	u32 oldIndexCount=smFindIndexesOfExistingSmers(smerMap, data, maxValidIndex, oldIndexes, smerIds, indexMaxDistance);

	//LOG(LOG_INFO, "Existing Indexes: ");
	//LogIndexes(oldIndexes, oldIndexCount, smerIds);

	s32 prevIndex=-1;

	int i,j;
	s32 distance;

	for(i=0;i<oldIndexCount;i++)
		{
		s32 index=oldIndexes[i];
		distance=index-prevIndex;

		if(distance>=indexMaxDistance)
			{
			for(j=prevIndex+1;j<index;j++)
				{
				SmerId smerId=smerIds[j];

				distance=j-prevIndex;

				if(checkStealthIndexing(newIndexes, newIndexCount, smerIds, smerId))
					{
					prevIndex=j;
					}
				else if (distance>=indexMaxDistance)
					{
					newIndexes[newIndexCount++]=j;
					prevIndex=j;
					}
				}
			}
		prevIndex=index;
		}

	for(j=prevIndex+1;j<=maxValidIndex;j++)
		{
		SmerId smerId=smerIds[j];

		distance=j-prevIndex;

		if(checkStealthIndexing(newIndexes, newIndexCount, smerIds, smerId))
			{
			prevIndex=j;
			}
		else if (distance>=indexMaxDistance)
			{
			newIndexes[newIndexCount++]=j;
			prevIndex=j;
			}
		}

	if(oldIndexCount==0 && newIndexCount==0)
		newIndexes[newIndexCount++]=maxValidIndex/2;

	//LOG(LOG_INFO, "New Indexes: ");
	//LogIndexes(newIndexes, newIndexCount, smerIds);

	//LOG(LOG_INFO, "Len %i Old %i New %i Existing %i",dataLength, oldIndexCount, newIndexCount, graph->smerMap.entryCount);

	if(newIndexCount>0)
		{
		//LOG(LOG_INFO, "Creating %i",newIndexCount);
		smCreateIndexedSmers(smerMap, newIndexCount, newIndexes, smerIds);
		}
}




/*
static void verifyIndexing(s32 maxAllowedDistance, s32 *indexes, u32 indexCount, int dataLength, int maxValidIndex)
{
		//LOG(LOG_INFO, "Maximum index distance %i",maxAllowedDistance);
		int i=0;

		int prevIndex=-1;
		s32 dist;
		s32 maxDist=0;

		for(i=0;i<indexCount;i++)
			{
			s32 index=indexes[i];

			dist=index-prevIndex;
			if(dist>maxDist)
				maxDist=dist;

			prevIndex=index;
			}

		dist=maxValidIndex-prevIndex;
		if(dist>maxDist)
			maxDist=dist;

		if(maxDist>maxAllowedDistance)
			LOG(LOG_INFO, "Warning: Asked to add path of %i with %i indexes at max dist %i", dataLength,indexCount,maxDist);
}


void addPathTails(Graph *graph, u32 dataLength, u8 *data)
{
	SmerSA *smerSA=&(graph->smerSA);
	s32 ssize=graph->config.s;
	s32 maxValidIndex=dataLength-graph->config.s;
	s32 indexMaxDistance=graph->config.k-graph->config.s+1;

	s32 *indexes=alloca((maxValidIndex+1)*sizeof(u32));
	u32 indexCount=0;

	SmerId *smerIds=alloca((maxValidIndex+1)*sizeof(SmerId));
	u32 *compFlags=alloca((maxValidIndex+1)*sizeof(u32));

	calculatePossibleSmers(data, maxValidIndex, smerIds, compFlags);

	indexCount=saFindIndexesOfExistingSmers(smerSA, data, maxValidIndex, indexes, smerIds);
	verifyIndexing(indexMaxDistance, indexes, indexCount, dataLength, maxValidIndex);

	//LOG(LOG_INFO,"Len: %i Max: %i",dataLength, maxValidIndex);

	saAddPathTails(smerSA,ssize,maxValidIndex,indexes,indexCount,smerIds,compFlags,data);
}


int addPathRoutes(Graph *graph, u32 dataLength, u8 *data)
{
	SmerSA *smerSA=&(graph->smerSA);
	s32 ssize=graph->config.s;
	s32 maxValidIndex=dataLength-graph->config.s;
	s32 indexMaxDistance=graph->config.k-graph->config.s+1;

	s32 *indexes=alloca((maxValidIndex+1)*sizeof(u32));
	u32 indexCount=0;

	SmerId *smerIds=alloca((maxValidIndex+1)*sizeof(SmerId));
	u32 *compFlags=alloca((maxValidIndex+1)*sizeof(u32));

	calculatePossibleSmers(data, maxValidIndex, smerIds, compFlags);

	indexCount=saFindIndexesOfExistingSmers(smerSA, data, maxValidIndex, indexes, smerIds);
	verifyIndexing(indexMaxDistance, indexes, indexCount, dataLength, maxValidIndex);

	return saAddPathRoutes(smerSA,ssize,maxValidIndex,indexes,indexCount,smerIds,compFlags,data);
}

*/

int addPathRoutes(Graph *graph, u32 dataLength, u8 *data)
{
	return 0;
}




void switchMode(Graph *graph)
{
	if(graph->mode!=GRAPH_MODE_INDEX)
		{
		LOG(LOG_INFO,"Graph already in Routing Mode");
		return;
		}


	LOG(LOG_INFO,"Switching Graph to Routing Mode");
	graph->mode=GRAPH_MODE_ROUTE;

	int entries=saInitSmerArray(&(graph->smerArray), &(graph->smerMap));

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


}
