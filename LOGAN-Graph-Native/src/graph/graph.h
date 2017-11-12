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
#define GRAPH_MODE_ROUTE 1


typedef struct graphStr
{
	GraphConfig config;

	u32 mode;
	u32 pathCount;

	SmerMap smerMap;
	SmerArray smerArray;

	void *userPtr;
} Graph;


void grSwitchMode(Graph *graph);

Graph *grAllocGraph(s32 nodeSize, s32 sparseness, void *userPtr);
void grFreeGraph(Graph *graph);


#endif
