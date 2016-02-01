/*
 * graph.h
 *
 *  Created on: Nov 10, 2010
 *      Author: tony
 */

#ifndef __GRAPH_H
#define __GRAPH_H



typedef struct graphContigStr
{
	s32 nodeSize;
	s32 sparseness;
} GraphConfig;

#define GRAPH_MODE_INDEX 0
#define GRAPH_MODE_BUILD 1


typedef struct graphStr
{
	GraphConfig config;

	u32 mode;
	u32 pathCount;

	SmerMap smerMap;
//	SmerSA smerSA;

	void *userPtr;
} Graph;


typedef struct graphBatchReadContextStr
{
	u32 readLength;

	u8 *packedSequence;
	u64 *bloomCache;


} GraphBatchReadContext;

typedef struct graphBatchContextStr
{
	Graph *graph;

	int maxSeqLength;
	int maxBatchSize;

	GraphBatchReadContext *reads;
	int numReads;

	u8 *packedSequences;
	u8 *bloomCaches;
} GraphBatchContext;



void addPathSmers(Graph *graph, u32 dataLength, u8 *data);
int addPathRoutes(Graph *graph, u32 dataLength, u8 *data);




GraphBatchContext *graphBatchInitContext(Graph *graph, int maxSeqLength, int batchSize, int mode);
void graphBatchFreeContext(GraphBatchContext *context);

void graphBatchAddPathRoutes(GraphBatchContext *context);



void switchMode(Graph *graph);

Graph *allocGraph(s32 nodeSize, s32 sparseness, void *userPtr);
void freeGraph(Graph *graph);


#endif
