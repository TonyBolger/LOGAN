/*
 * JNI_Graph.c
 *
 *  Created on: Nov 10, 2010
 *      Author: tony
 */

#include "common.h"
#include "JNI_graph.h"



#define EXCEPTION_LOG_AND_CLEAR(env) { if((*env)->ExceptionOccurred(env)) { (*env)->ExceptionDescribe(env); } }


/* Some useful utility methods */

/*

#define JNI_ARRAY_COPY_MAX 100000000

static int JNIutil_setLongArray(JNIEnv *env, jlongArray dest, jlong *src, int elements)
{
	int offset=0;

	while(offset<elements)
		{
		int sizeToCopy=elements-offset;

		if(sizeToCopy>JNI_ARRAY_COPY_MAX)
			sizeToCopy=JNI_ARRAY_COPY_MAX;

		(*env)->SetLongArrayRegion(env, dest, offset, sizeToCopy, src+offset);
		EXCEPTION_LOG_AND_CLEAR(env);

		offset+=sizeToCopy;
		}

	return 0;
}

static int JNIutil_getLongArray(JNIEnv *env, jlong *dest, jlongArray src, int elements)
{
	int offset=0;

	while(offset<elements)
		{
		int sizeToCopy=elements-offset;

		if(sizeToCopy>JNI_ARRAY_COPY_MAX)
			sizeToCopy=JNI_ARRAY_COPY_MAX;

		(*env)->GetLongArrayRegion(env, src, offset, sizeToCopy, dest+offset);
		EXCEPTION_LOG_AND_CLEAR(env);

		offset+=sizeToCopy;
		}

	return 0;
}


static jbyteArray toByteArray(JNIEnv *env, u8 *data, u32 size)
{
	if(data!=NULL)
		{
		jbyteArray out = (*env)->NewByteArray(env, size);
		(*env)->SetByteArrayRegion(env, out, 0, size, (s8 *) data);
		return out;
		}

	return NULL;
}

*/



#define JNIREF_NULL_CHECK(v, m) if((v) == NULL) { LOG(LOG_CRITICAL,"JNILookup of "m" failed"); return 1; }


static int prepareJniRefs(JNIEnv *env, GraphJni *graphJni) {

	(*env)->EnsureLocalCapacity(env, 40);

	// Graph

	JNIREF_NULL_CHECK(graphJni->graphCls = (*env)->NewGlobalRef(env, (*env)->FindClass(env,"logan/graph/Graph")),"class logan.graph.Graph");

	// GraphConfig

//	JNIREF_NULL_CHECK(graphJni->graphConfigCls = (*env)->NewGlobalRef(env,(*env)->FindClass(env,"pagan/dbg/DeBruijnGraphConfig")), "class GraphConfig");

//	JNIREF_NULL_CHECK(graphJni->graphConfigFieldK = (*env)->GetFieldID(env, graphJni->graphConfigCls, "k", "I"),"field GraphConfig.k");
//	JNIREF_NULL_CHECK(graphJni->graphConfigFieldS = (*env)->GetFieldID(env, graphJni->graphConfigCls, "s", "I"),"field GraphConfig.s");

	// LinkedSmer
/*
	JNIREF_NULL_CHECK(graphJni->linkedSmerCls=(*env)->NewGlobalRef(env, (*env)->FindClass(env,"pagan/dbg/LinkedSmer")) ,"class LinkedSmer");

	JNIREF_NULL_CHECK(graphJni->linkedSmerMethodInit=(*env)->GetMethodID(env, graphJni->linkedSmerCls, "<init>",
			"(J[Lpagan/dbg/LinkedSmer$Tail;[Lpagan/dbg/LinkedSmer$Tail;[Lpagan/dbg/LinkedSmer$Route;II)V"), "method LinkedSmer.<init>");

	JNIREF_NULL_CHECK(graphJni->linkedSmerFieldSmerId=(*env)->GetFieldID(env, graphJni->linkedSmerCls, "smerId", "J"), "field LinkedSmer.smerId");
	JNIREF_NULL_CHECK(graphJni->linkedSmerFieldPrefixes=(*env)->GetFieldID(env, graphJni->linkedSmerCls, "prefixes", "[Lpagan/dbg/LinkedSmer$Tail;"), "field LinkedSmer.prefixes");
	JNIREF_NULL_CHECK(graphJni->linkedSmerFieldSuffixes=(*env)->GetFieldID(env, graphJni->linkedSmerCls, "suffixes", "[Lpagan/dbg/LinkedSmer$Tail;"), "field LinkedSmer.suffixes");
	JNIREF_NULL_CHECK(graphJni->linkedSmerFieldRoutes=(*env)->GetFieldID(env, graphJni->linkedSmerCls, "routes", "[Lpagan/dbg/LinkedSmer$Route;"), "field LinkedSmer.routes");

	// LinkedSmer.Tail

	JNIREF_NULL_CHECK(graphJni->linkedSmerTailCls=(*env)->NewGlobalRef(env, (*env)->FindClass(env,"pagan/dbg/LinkedSmer$Tail")),"class LinkedSmer.Tail");

	JNIREF_NULL_CHECK(graphJni->linkedSmerTailMethodInit=(*env)->GetMethodID(env, graphJni->linkedSmerTailCls, "<init>","([BJZZ)V"), "LinkedSmerTail.<init>");

	JNIREF_NULL_CHECK(graphJni->linkedSmerTailFieldData=(*env)->GetFieldID(env, graphJni->linkedSmerTailCls, "data", "[B"), "field LinkedSmer.Tail.data");
	JNIREF_NULL_CHECK(graphJni->linkedSmerTailFieldSmerId=(*env)->GetFieldID(env, graphJni->linkedSmerTailCls, "smerId", "J"), "field LinkedSmer.Tail.smerId");
	JNIREF_NULL_CHECK(graphJni->linkedSmerTailFieldCompFlag=(*env)->GetFieldID(env, graphJni->linkedSmerTailCls, "compFlag", "Z"), "field LinkedSmer.Tail.compFlag");
	JNIREF_NULL_CHECK(graphJni->linkedSmerTailFieldSmerExists=(*env)->GetFieldID(env, graphJni->linkedSmerTailCls, "smerExists", "Z"), "field LinkedSmer.Tail.smerExists");

	// LinkedSmer.Route

	JNIREF_NULL_CHECK(graphJni->linkedSmerRouteCls=(*env)->NewGlobalRef(env, (*env)->FindClass(env,"pagan/dbg/LinkedSmer$Route")),"class LinkedSmer.Route");

	JNIREF_NULL_CHECK(graphJni->linkedSmerRouteMethodInit=(*env)->GetMethodID(env, graphJni->linkedSmerRouteCls, "<init>","(SSI)V"), "LinkedSmerTail.<init>");

	JNIREF_NULL_CHECK(graphJni->linkedSmerRouteFieldPrefix=(*env)->GetFieldID(env, graphJni->linkedSmerRouteCls, "prefix", "S"),"field LinkedSmer.Route.prefix");
	JNIREF_NULL_CHECK(graphJni->linkedSmerRouteFieldSuffix=(*env)->GetFieldID(env, graphJni->linkedSmerRouteCls, "suffix", "S"),"field LinkedSmer.Route.suffix");
	JNIREF_NULL_CHECK(graphJni->linkedSmerRouteFieldWidth=(*env)->GetFieldID(env, graphJni->linkedSmerRouteCls, "width", "I"),"field LinkedSmer.Route.width");
*/
	return 0;
}

static int cleanupJniRefs(JNIEnv *env, GraphJni *graphJni)
{
	(*env)->DeleteGlobalRef(env, graphJni->graphCls);
	/*
	(*env)->DeleteGlobalRef(env, graphJni->graphConfigCls);
	(*env)->DeleteGlobalRef(env, graphJni->linkedSmerCls);
	(*env)->DeleteGlobalRef(env, graphJni->linkedSmerTailCls);
	(*env)->DeleteGlobalRef(env, graphJni->linkedSmerRouteCls);
*/

	return 0;
}



/* logan.graph.Graph.IndexBuilder */

JNIEXPORT void JNICALL Java_logan_graph_Graph_00024IndexBuilder_waitStartup_1Native
  (JNIEnv *env, jobject this, jlong handle)
{
	IndexingBuilder *ib = (IndexingBuilder *) handle;

	waitForStartup(ib->pt);
}

/*
 * Class:     logan_graph_Graph_IndexBuilder
 * Method:    processReads_Native
 * Signature: (J[B[II)V
 */
JNIEXPORT void JNICALL Java_logan_graph_Graph_00024IndexBuilder_processReads_1Native
  (JNIEnv *env, jobject this, jlong handle, jbyteArray ba, jintArray ia, jint i)
{
	IndexingBuilder *ib = (IndexingBuilder *) handle;

	queueIngress(ib->pt, ib, i);
}

/*
 * Class:     logan_graph_Graph_IndexBuilder
 * Method:    shutdown_Native
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_logan_graph_Graph_00024IndexBuilder_shutdown_1Native
  (JNIEnv *env, jobject this, jlong handle)
{
	IndexingBuilder *ib = (IndexingBuilder *) handle;

	queueShutdown(ib->pt);
}

/*
 * Class:     logan_graph_Graph_IndexBuilder
 * Method:    waitShutdown_Native
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_logan_graph_Graph_00024IndexBuilder_waitShutdown_1Native
  (JNIEnv *env, jobject this, jlong handle)
{
	IndexingBuilder *ib = (IndexingBuilder *) handle;

	waitForShutdown(ib->pt);
}

/*
 * Class:     logan_graph_Graph_IndexBuilder
 * Method:    perform_Native
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_logan_graph_Graph_00024IndexBuilder_perform_1Native
  (JNIEnv *env, jobject this, jlong handle)
{
	IndexingBuilder *ib = (IndexingBuilder *) handle;

	performTask(ib->pt);
}




/* logan.graph.Graph.RouteBuilder */





/* logan.graph.Graph: makeIndexBuilder: Creates an IndexBuilder instance */


JNIEXPORT jlong JNICALL Java_logan_graph_Graph_makeIndexBuilder_1Native
  (JNIEnv *env, jobject this, jlong handle, jint threads)
{
	Graph *graph = (Graph *) handle;

	IndexingBuilder *ib=allocIndexingBuilder(graph,threads);

	return (long)ib;
}



/* logan.graph.Graph: makeRouteBuilder: Creates a RouteBuilder instance */

JNIEXPORT jlong JNICALL Java_logan_graph_Graph_makeRouteBuilder_1Native
  (JNIEnv *env, jobject this, jlong handle, jint threads)
{
	Graph *graph = (Graph *) handle;

	RoutingBuilder *rb=allocRoutingBuilder(graph,threads);

	return (long)rb;
}




/* logan.graph.Graph: alloc: Creates a Graph instance */

JNIEXPORT jlong JNICALL Java_logan_graph_Graph_alloc_1Native(
		JNIEnv *env, jobject this, jint nodeSize, jint sparseness)
{
	logInit();

	GraphJni *jni=gAlloc(sizeof(GraphJni));

	if (prepareJniRefs(env, jni) != 0)
		return 0;

	Graph *graph = allocGraph(nodeSize, sparseness, jni);

	return (long) (graph);
}


/* logan.graph.Graph: free: Destroys a Graph instance */

JNIEXPORT void JNICALL Java_pagan_dbg_DeBruijnGraph_free_1Native(
		JNIEnv *env, jobject this, jlong handle) {

	Graph *graph = (Graph *) handle;

	cleanupJniRefs(env, graph->userPtr);

	gFree(graph->userPtr);
	graph->userPtr=NULL;

	freeGraph(graph);

}
