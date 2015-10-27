/*
 * JNI_graph.h
 *
 *  Created on: Feb 22, 2012
 *      Author: tony
 */

#ifndef __JNI_GRAPH_H
#define __JNI_GRAPH_H


#include <logan_graph_Graph.h>

typedef struct graphJniStr
{
	jclass graphCls;

	jclass graphConfigCls;
	jfieldID graphConfigFieldK;
	jfieldID graphConfigFieldS;

	jclass linkedSmerCls;
	jmethodID linkedSmerMethodInit;
	jfieldID linkedSmerFieldSmerId;
	jfieldID linkedSmerFieldPrefixes;
	jfieldID linkedSmerFieldSuffixes;
	jfieldID linkedSmerFieldRoutes;

	jclass linkedSmerTailCls;
	jmethodID linkedSmerTailMethodInit;
	jfieldID linkedSmerTailFieldData;
	jfieldID linkedSmerTailFieldSmerId;
	jfieldID linkedSmerTailFieldCompFlag;
	jfieldID linkedSmerTailFieldSmerExists;

	jclass linkedSmerRouteCls;
	jmethodID linkedSmerRouteMethodInit;
	jfieldID linkedSmerRouteFieldPrefix;
	jfieldID linkedSmerRouteFieldSuffix;
	jfieldID linkedSmerRouteFieldWidth;
} GraphJni;


#endif
