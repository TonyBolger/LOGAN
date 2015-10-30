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

//	SmerMap smerMap;
//	SmerSA smerSA;

	void *userPtr;
} Graph;



void indexPaths(Graph *graph);
void addPaths(Graph *graph);

void switchMode(Graph *graph);

Graph *allocGraph(s32 nodeSize, s32 sparseness, void *userPtr);
void freeGraph(Graph *graph);


#endif
