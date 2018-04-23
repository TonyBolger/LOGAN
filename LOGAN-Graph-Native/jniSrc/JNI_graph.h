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

#include <logan_graph_GraphReader.h>
#include <logan_graph_GraphWriter.h>

typedef struct graphJniStr
{
	jclass graphCls;


	// SequenceIndex and friends

	jclass sequenceIndexCls;
	jmethodID sequenceIndexMethodInit;

	jclass sequenceSourceCls;
	jmethodID sequenceSourceMethodInit;

	jclass sequenceCls;
	jmethodID sequenceMethodInit;

	jclass sequenceFragmentCls;
	jmethodID sequenceFragmentMethodInit;


	// LinkedSmer and friends

	jclass linkedSmerCls;
	jmethodID linkedSmerMethodInit;

	jclass linkedSmerTailCls;
	jmethodID linkedSmerTailMethodInit;

	jclass linkedSmerRouteEntryCls;
	jmethodID linkedSmerRouteEntryMethodInit;

	jclass linkedSmerRouteTagCls;
	jmethodID linkedSmerRouteTagMethodInit;

} GraphJni;


#endif
