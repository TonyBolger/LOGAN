/*
 * graph.h
 *
 *  Created on: Nov 10, 2010
 *      Author: tony
 */

#ifndef __GRAPH_H
#define __GRAPH_H

#include "common.h"

typedef struct graphContigStr
{
	s32 nodeSize;
	s32 edgeSize;
} GraphConfig;


#define GRAPH_MODE_INDEX 0
#define GRAPH_MODE_BUILD 1



typedef struct graphStr
{
	GraphConfig config;

	u32 mode;
	u32 pathCount;

//	SmerMap smerMap;
//	SmerSA smerSA;

	void *jni;
} Graph;



void indexPaths(Graph *graph);
void addPaths(Graph *graph);

void switchMode(Graph *graph);

Graph *allocGraph(void *jni, s32 nodeSize, s32 edgeSize);
void freeGraph(Graph *graph);


#endif
