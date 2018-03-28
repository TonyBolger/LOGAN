/*
 * JNI_graph.h
 *
 *  Created on: Feb 22, 2012
 *      Author: tony
 */

#ifndef __JNI_GRAPH_H
#define __JNI_GRAPH_H

#include <jni.h>

#include <logan_graph_Graph.h>
#include <logan_graph_Graph_IndexBuilder.h>
#include <logan_graph_Graph_Mode.h>
#include <logan_graph_Graph_RouteBuilder.h>

typedef struct graphJniStr
{
	jclass graphCls;


	jclass linkedSmerCls;
	jmethodID linkedSmerMethodInit;
	/*
	jfieldID linkedSmerFieldSmerId;
	jfieldID linkedSmerFieldPrefixes;
	jfieldID linkedSmerFieldSuffixes;
	jfieldID linkedSmerFieldForwardRoutes;
	jfieldID linkedSmerFieldReverseRoutes;
*/
	jclass linkedSmerTailCls;
	jmethodID linkedSmerTailMethodInit;
	/*
	jfieldID linkedSmerTailFieldData;
	jfieldID linkedSmerTailFieldSmerId;
	jfieldID linkedSmerTailFieldSmerExists;
*/
	jclass linkedSmerRouteEntryCls;
	jmethodID linkedSmerRouteEntryMethodInit;
	/*
	jfieldID linkedSmerRouteFieldPrefix;
	jfieldID linkedSmerRouteFieldSuffix;
	jfieldID linkedSmerRouteFieldWidth;
*/
	jclass linkedSmerRouteTagCls;
	jmethodID linkedSmerRouteTagMethodInit;

} GraphJni;


#endif
