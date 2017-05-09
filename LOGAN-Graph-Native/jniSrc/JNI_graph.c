/*
 * JNI_Graph.c
 *
 *  Created on: Nov 10, 2010
 *      Author: tony
 */

#include "../jniSrc/JNI_graph.h"

#include "common.h"


#define EXCEPTION_LOG_AND_CLEAR(env) { if((*env)->ExceptionOccurred(env)) { (*env)->ExceptionDescribe(env); } }


/* Some useful utility methods */

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

/*
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
*/

/*
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

static jlongArray toLongArray(JNIEnv *env, s64 *data, u32 size)
{
	if(data!=NULL)
		{
		jlongArray out = (*env)->NewLongArray(env, size);
		(*env)->SetLongArrayRegion(env, out, 0, size, (s64 *) data);
		return out;
		}

	return NULL;
}

*/

static void waitForIdle(int *usageCount)
{
	while(__atomic_load_n(usageCount,__ATOMIC_SEQ_CST))
		{
		struct timespec req;

		req.tv_sec=DEFAULT_SLEEP_SECS;
		req.tv_nsec=DEFAULT_SLEEP_NANOS;

		nanosleep(&req, NULL);
		}
}

static void processReadFiles(JNIEnv *env, jobjectArray jFilePaths, void *handlerContext, void (*handler)(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *handlerContext))
{
	SwqBuffer swqBuffer[PT_INGRESS_BUFFERS];
	ParallelTaskIngress ingressBuffers[PT_INGRESS_BUFFERS];

	for(int i=0;i<PT_INGRESS_BUFFERS;i++)
		initSequenceBuffer(swqBuffer+i, FASTQ_BASES_PER_BATCH, FASTQ_RECORDS_PER_BATCH, FASTQ_MAX_READ_LENGTH);

	u8 *ioBuffer=G_ALLOC(FASTQ_IO_RECYCLE_BUFFER+FASTQ_IO_PRIMARY_BUFFER, MEMTRACKID_IOBUF);

	int fileCount=(*env)->GetArrayLength(env, jFilePaths);

	for(int i=0;i<fileCount;i++)
		{
		jobject jFilePath=(*env)->GetObjectArrayElement(env,jFilePaths,i);

		int filePathLength=(*env)->GetArrayLength(env, jFilePath);
		char *filePath=alloca(filePathLength+1);

		(*env)->GetByteArrayRegion(env, jFilePath, 0, filePathLength, (signed char *)filePath);
		filePath[filePathLength]=0;

		LOG(LOG_INFO,"Parse %s",filePath);

		int reads=parseAndProcess(filePath, FASTQ_MIN_READ_LENGTH, 0, 2000000000,
			ioBuffer, FASTQ_IO_RECYCLE_BUFFER, FASTQ_IO_PRIMARY_BUFFER,
			swqBuffer, ingressBuffers, PT_INGRESS_BUFFERS,
			handlerContext, handler, NULL);

		LOG(LOG_INFO,"Parsed %i reads from %s",reads,filePath);

		(*env)->DeleteLocalRef(env, jFilePath);
		}

	for(int i=0;i<PT_INGRESS_BUFFERS;i++)
		{
		waitForIdle(&swqBuffer[i].usageCount);
		freeSequenceBuffer(swqBuffer+i);
		}

	G_FREE(ioBuffer, FASTQ_IO_RECYCLE_BUFFER+FASTQ_IO_PRIMARY_BUFFER, MEMTRACKID_IOBUF);
}



#define JNIREF_NULL_CHECK(v, m) if((v) == NULL) { LOG(LOG_CRITICAL,"JNILookup of "m" failed"); return 1; }


static int prepareJniRefs(JNIEnv *env, GraphJni *graphJni) {

	(*env)->EnsureLocalCapacity(env, 40);

	// Graph

	JNIREF_NULL_CHECK(graphJni->graphCls = (*env)->NewGlobalRef(env, (*env)->FindClass(env,"logan/graph/Graph")),"class logan.graph.Graph");

	// LinkedSmer

	JNIREF_NULL_CHECK(graphJni->linkedSmerCls=(*env)->NewGlobalRef(env, (*env)->FindClass(env,"logan/graph/LinkedSmer")) ,"class logan.graph.LinkedSmer");
	JNIREF_NULL_CHECK(graphJni->linkedSmerMethodInit=(*env)->GetMethodID(env, graphJni->linkedSmerCls, "<init>",
			"(J[Llogan/graph/LinkedSmer$Tail;[Llogan/graph/LinkedSmer$Tail;[Llogan/graph/LinkedSmer$Route;[Llogan/graph/LinkedSmer$Route;)V"), "method LinkedSmer.<init>");

	// LinkedSmer.Tail

	JNIREF_NULL_CHECK(graphJni->linkedSmerTailCls=(*env)->NewGlobalRef(env, (*env)->FindClass(env,"logan/graph/LinkedSmer$Tail")),"class LinkedSmer.Tail");
	JNIREF_NULL_CHECK(graphJni->linkedSmerTailMethodInit=(*env)->GetMethodID(env, graphJni->linkedSmerTailCls, "<init>","(JJB)V"), "LinkedSmer.Tail.<init>");

	// LinkedSmer.Route

	JNIREF_NULL_CHECK(graphJni->linkedSmerRouteCls=(*env)->NewGlobalRef(env, (*env)->FindClass(env,"logan/graph/LinkedSmer$Route")),"class LinkedSmer.Route");
	JNIREF_NULL_CHECK(graphJni->linkedSmerRouteMethodInit=(*env)->GetMethodID(env, graphJni->linkedSmerRouteCls, "<init>","(SSI)V"), "LinkedSmer.Route.<init>");

	return 0;
}

static int cleanupJniRefs(JNIEnv *env, GraphJni *graphJni)
{
	(*env)->DeleteGlobalRef(env, graphJni->graphCls);
	(*env)->DeleteGlobalRef(env, graphJni->linkedSmerCls);
	(*env)->DeleteGlobalRef(env, graphJni->linkedSmerTailCls);
	(*env)->DeleteGlobalRef(env, graphJni->linkedSmerRouteCls);

	return 0;
}



/* logan.graph.Graph.IndexBuilder:

JNIEXPORT void JNICALL Java_logan_graph_Graph_00024IndexBuilder_waitStartup_1Native
  (JNIEnv *, jobject, jlong);

JNIEXPORT void JNICALL Java_logan_graph_Graph_00024IndexBuilder_processReadFiles_1Native
  (JNIEnv *, jobject, jlong, jobjectArray);

JNIEXPORT void JNICALL Java_logan_graph_Graph_00024IndexBuilder_shutdown_1Native
  (JNIEnv *, jobject, jlong);

JNIEXPORT void JNICALL Java_logan_graph_Graph_00024IndexBuilder_waitShutdown_1Native
  (JNIEnv *, jobject, jlong);

JNIEXPORT void JNICALL Java_logan_graph_Graph_00024IndexBuilder_workerPerformTasks_1Native
  (JNIEnv *, jobject, jlong);

JNIEXPORT void JNICALL Java_logan_graph_Graph_00024IndexBuilder_free_1Native
  (JNIEnv *, jobject, jlong);
*/

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
JNIEXPORT void JNICALL Java_logan_graph_Graph_00024IndexBuilder_processReadFiles_1Native
  (JNIEnv *env, jobject this, jlong handle, jobjectArray jFilePaths)
{
	IndexingBuilder *ib = (IndexingBuilder *) handle;

	processReadFiles(env, jFilePaths, ib, indexingBuilderDataHandler);
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
JNIEXPORT void JNICALL Java_logan_graph_Graph_00024IndexBuilder_workerPerformTasks_1Native
  (JNIEnv *env, jobject this, jlong handle)
{
	IndexingBuilder *ib = (IndexingBuilder *) handle;

	performTask_worker(ib->pt);
}


JNIEXPORT void JNICALL Java_logan_graph_Graph_00024IndexBuilder_free_1Native
  (JNIEnv *env, jobject this, jlong handle)
{
	IndexingBuilder *ib = (IndexingBuilder *) handle;

	freeIndexingBuilder(ib);
}

/* logan.graph.Graph.RouteBuilder:

JNIEXPORT void JNICALL Java_logan_graph_Graph_00024RouteBuilder_waitStartup_1Native
  (JNIEnv *, jobject, jlong);

JNIEXPORT void JNICALL Java_logan_graph_Graph_00024RouteBuilder_processReadFiles_1Native
  (JNIEnv *, jobject, jlong, jobjectArray);

JNIEXPORT void JNICALL Java_logan_graph_Graph_00024RouteBuilder_shutdown_1Native
  (JNIEnv *, jobject, jlong);

JNIEXPORT void JNICALL Java_logan_graph_Graph_00024RouteBuilder_waitShutdown_1Native
  (JNIEnv *, jobject, jlong);

JNIEXPORT void JNICALL Java_logan_graph_Graph_00024RouteBuilder_workerPerformTasks_1Native
  (JNIEnv *, jobject, jlong);

JNIEXPORT void JNICALL Java_logan_graph_Graph_00024RouteBuilder_free_1Native
  (JNIEnv *, jobject, jlong);

 */


JNIEXPORT void JNICALL Java_logan_graph_Graph_00024RouteBuilder_waitStartup_1Native
  (JNIEnv *env, jobject this, jlong handle)
{
	RoutingBuilder *rb = (RoutingBuilder *) handle;

	waitForStartup(rb->pt);
}

JNIEXPORT void JNICALL Java_logan_graph_Graph_00024RouteBuilder_processReadFiles_1Native
  (JNIEnv *env, jobject this, jlong handle, jobjectArray jFilePaths)
{
	RoutingBuilder *rb = (RoutingBuilder *) handle;

	processReadFiles(env, jFilePaths, rb, routingBuilderDataHandler);
}

JNIEXPORT void JNICALL Java_logan_graph_Graph_00024RouteBuilder_shutdown_1Native
  (JNIEnv *env, jobject this, jlong handle)
{
	RoutingBuilder *rb = (RoutingBuilder *) handle;

	queueShutdown(rb->pt);
}

JNIEXPORT void JNICALL Java_logan_graph_Graph_00024RouteBuilder_waitShutdown_1Native
  (JNIEnv *env, jobject this, jlong handle)
{
	RoutingBuilder *rb = (RoutingBuilder *) handle;

	waitForShutdown(rb->pt);
}

JNIEXPORT void JNICALL Java_logan_graph_Graph_00024RouteBuilder_workerPerformTasks_1Native
  (JNIEnv *env, jobject this, jlong handle)
{
	RoutingBuilder *rb = (RoutingBuilder *) handle;

	performTask_worker(rb->pt);
}

JNIEXPORT void JNICALL Java_logan_graph_Graph_00024RouteBuilder_free_1Native
  (JNIEnv *env, jobject this, jlong handle)
{
	RoutingBuilder *rb = (RoutingBuilder *) handle;

	freeRoutingBuilder(rb);
}



/*

logan.graph.Graph:

JNIEXPORT jint JNICALL Java_logan_graph_Graph_getSmerCount_1Native
  (JNIEnv *, jobject, jlong);

JNIEXPORT jlongArray JNICALL Java_logan_graph_Graph_getSmerIds_1Native
  (JNIEnv *, jobject, jlong);


JNIEXPORT jobject JNICALL Java_logan_graph_Graph_getLinkedSmer_1Native
  (JNIEnv *, jobject, jlong, jlong);

JNIEXPORT jbyteArray JNICALL Java_logan_graph_Graph_getRawSmerData_1Native
  (JNIEnv *, jobject, jlong, jlong);


JNIEXPORT jlong JNICALL Java_logan_graph_Graph_makeIndexBuilder_1Native
  (JNIEnv *, jobject, jlong, jint);

JNIEXPORT void JNICALL Java_logan_graph_Graph_addRawSmers_1Native
  (JNIEnv *, jobject, jlong, jlongArray);


JNIEXPORT jlong JNICALL Java_logan_graph_Graph_makeRouteBuilder_1Native
  (JNIEnv *, jobject, jlong, jint);

JNIEXPORT void JNICALL Java_logan_graph_Graph_setRawSmerData_1Native
  (JNIEnv *, jobject, jlong, jlong, jbyteArray);


JNIEXPORT void JNICALL Java_logan_graph_Graph_switchMode_1Native
  (JNIEnv *, jobject, jlong);

JNIEXPORT jlong JNICALL Java_logan_graph_Graph_alloc_1Native
  (JNIEnv *, jobject, jint, jint);

JNIEXPORT void JNICALL Java_logan_graph_Graph_free_1Native
  (JNIEnv *, jobject, jlong);

JNIEXPORT void JNICALL Java_logan_graph_Graph_dump_1Native
*/



JNIEXPORT jint JNICALL Java_logan_graph_Graph_getSmerCount_1Native
  (JNIEnv *env, jobject this, jlong handle)
{
	Graph *graph = (Graph *) handle;

	SmerMap *smerMap;
	SmerArray *smerArray;

	switch(graph->mode)
	{
		case GRAPH_MODE_INDEX:
			smerMap=&(graph->smerMap);
			return smGetSmerCount(smerMap);

		case GRAPH_MODE_ROUTE:
			smerArray=&(graph->smerArray);
			return saGetSmerCount(smerArray);
	}

	return -1;
}

JNIEXPORT jlongArray JNICALL Java_logan_graph_Graph_getSmerIds_1Native
  (JNIEnv *env, jobject this, jlong handle)
{
	Graph *graph = (Graph *) handle;

	SmerMap *smerMap;
	SmerArray *smerArray;

	int smerCount;
	SmerId *smerIds;

	switch(graph->mode)
	{
		case GRAPH_MODE_INDEX:
			smerMap=&(graph->smerMap);
			smerCount=smGetSmerCount(smerMap);
			smerIds = smSmerIdArrayAlloc(smerCount);
			smGetSmerIds(smerMap, smerIds);
			break;

		case GRAPH_MODE_ROUTE:
			smerArray=&(graph->smerArray);
			smerCount=saGetSmerCount(smerArray);
			smerIds = smSmerIdArrayAlloc(smerCount);
			saGetSmerIds(smerArray, smerIds);
			break;

		default:
			return NULL;
	}

	jlongArray jsmerIds = (*env)->NewLongArray(env, smerCount);
	EXCEPTION_LOG_AND_CLEAR(env);

	JNIutil_setLongArray(env, jsmerIds, (jlong *)smerIds, smerCount);
	smSmerIdArrayFree(smerIds, smerCount);

	return jsmerIds;
}


JNIEXPORT jobject JNICALL Java_logan_graph_Graph_getLinkedSmer_1Native
  (JNIEnv *env, jobject this, jlong handle, jlong jSmerId)
{
	Graph *graph = (Graph *) handle;
	GraphJni *jni = graph->userPtr;

	SmerArray *smerArray = &(graph->smerArray);

	MemDispenser *dispenser=dispenserAlloc(MEMTRACKID_DISPENSER_LINKED_SMER, SLAB_FREEPOLICY_INSTA_SHRINK, DISPENSER_BLOCKSIZE_SMALL, DISPENSER_BLOCKSIZE_LARGE);

	SmerLinked *linked=rtGetLinkedSmer(smerArray, (SmerId)jSmerId, dispenser);
	jobject jObj=NULL;

	//LOG(LOG_INFO,"Linked Smer: %p",linked);

	if(linked!=NULL)
		{
		int i;

		int prefixCount=linked->prefixCount;
		int suffixCount=linked->suffixCount;

		int forwardRouteCount=linked->forwardRouteCount;
		int reverseRouteCount=linked->reverseRouteCount;

		//(*env)->EnsureLocalCapacity(env, 16+prefixes+suffixes+routes);

		jarray prefixesObjectArray=(*env)->NewObjectArray(env, prefixCount, jni->linkedSmerTailCls, NULL);
		jarray suffixesObjectArray=(*env)->NewObjectArray(env, suffixCount, jni->linkedSmerTailCls, NULL);
		jarray forwardRouteObjectArray=(*env)->NewObjectArray(env, forwardRouteCount, jni->linkedSmerRouteCls, NULL);
		jarray reverseRouteObjectArray=(*env)->NewObjectArray(env, reverseRouteCount, jni->linkedSmerRouteCls, NULL);

		if(prefixesObjectArray==NULL)
			LOG(LOG_INFO,"PrefixArray Null");

		if(suffixesObjectArray==NULL)
			LOG(LOG_INFO,"SuffixArray Null");

		if(forwardRouteObjectArray==NULL)
			LOG(LOG_INFO,"ForwardRouteArray Null");

		if(reverseRouteObjectArray==NULL)
			LOG(LOG_INFO,"ReverseRouteArray Null");

		for(i=0;i<prefixCount;i++)
			{
			jobject prefixObject=(*env)->NewObject(env, jni->linkedSmerTailCls, jni->linkedSmerTailMethodInit,
					(jlong)linked->prefixes[i], (jlong)linked->prefixSmers[i], (jboolean)linked->prefixSmerExists[i]);

			(*env)->SetObjectArrayElement(env, prefixesObjectArray, i, prefixObject);
			(*env)->DeleteLocalRef(env,prefixObject);
			}


		for(i=0;i<suffixCount;i++)
			{
			jobject suffixObject=(*env)->NewObject(env, jni->linkedSmerTailCls, jni->linkedSmerTailMethodInit,
					(jlong)linked->suffixes[i],  (jlong)linked->suffixSmers[i], (jboolean)linked->suffixSmerExists[i]);

			(*env)->SetObjectArrayElement(env, suffixesObjectArray, i, suffixObject);
			(*env)->DeleteLocalRef(env,suffixObject);
			}


		for(i=0;i<forwardRouteCount;i++)
			{
			RouteTableEntry *routeEntry=linked->forwardRouteEntries+i;

			jobject routeObject=(*env)->NewObject(env, jni->linkedSmerRouteCls, jni->linkedSmerRouteMethodInit,
					(jshort)routeEntry->prefix, (jshort)routeEntry->suffix, (jint)routeEntry->width);

			(*env)->SetObjectArrayElement(env, forwardRouteObjectArray, i, routeObject);
			(*env)->DeleteLocalRef(env,routeObject);
			}

		for(i=0;i<reverseRouteCount;i++)
			{
			RouteTableEntry *routeEntry=linked->reverseRouteEntries+i;

			jobject routeObject=(*env)->NewObject(env, jni->linkedSmerRouteCls, jni->linkedSmerRouteMethodInit,
					(jshort)routeEntry->prefix, (jshort)routeEntry->suffix, (jint)routeEntry->width);

			(*env)->SetObjectArrayElement(env, reverseRouteObjectArray, i, routeObject);
			(*env)->DeleteLocalRef(env,routeObject);
			}

		jObj=(*env)->NewObject(env, jni->linkedSmerCls, jni->linkedSmerMethodInit, (jlong)linked->smerId,
				prefixesObjectArray, suffixesObjectArray, forwardRouteObjectArray, reverseRouteObjectArray);
		}

	dispenserFree(dispenser);

	return jObj;
}

JNIEXPORT jbyteArray JNICALL Java_logan_graph_Graph_getRawSmerData_1Native
  (JNIEnv *env, jobject this, jlong handle, jlong jSmerId)
{
	return NULL;
}


/* logan.graph.Graph: makeIndexBuilder: Creates an IndexBuilder instance */


JNIEXPORT jlong JNICALL Java_logan_graph_Graph_makeIndexBuilder_1Native
  (JNIEnv *env, jobject this, jlong handle, jint threads)
{
	Graph *graph = (Graph *) handle;

	IndexingBuilder *ib=allocIndexingBuilder(graph,threads);

	return (long)ib;
}

JNIEXPORT void JNICALL Java_logan_graph_Graph_addRawSmers_1Native
  (JNIEnv *env, jobject this, jlong handle, jlongArray jSmerIds)
{

}



/* logan.graph.Graph: makeRouteBuilder: Creates a RouteBuilder instance */

JNIEXPORT jlong JNICALL Java_logan_graph_Graph_makeRouteBuilder_1Native
  (JNIEnv *env, jobject this, jlong handle, jint threads)
{
	Graph *graph = (Graph *) handle;

	RoutingBuilder *rb=allocRoutingBuilder(graph,threads);

	return (long)rb;
}

JNIEXPORT void JNICALL Java_logan_graph_Graph_setRawSmerData_1Native
  (JNIEnv *env, jobject this, jlong handle, jlong jSmerId, jbyteArray jData)
{

}


/* logan.graph.Graph: switchMode: Changes from SmerMap (indexing) to SmerArray (routing) mode */

JNIEXPORT void JNICALL Java_logan_graph_Graph_switchMode_1Native
  (JNIEnv *env, jobject this, jlong handle)
{
	Graph *graph = (Graph *) handle;

	switchMode(graph);
}



/* logan.graph.Graph: alloc: Creates a Graph instance */

JNIEXPORT jlong JNICALL Java_logan_graph_Graph_alloc_1Native(
		JNIEnv *env, jobject this, jint nodeSize, jint sparseness)
{
	logInit();

	GraphJni *jni=G_ALLOC(sizeof(GraphJni), MEMTRACKID_GRAPH);

	if (prepareJniRefs(env, jni) != 0)
		return 0;

	Graph *graph = allocGraph(nodeSize, sparseness, jni);

	return (long) (graph);
}


/* logan.graph.Graph: free: Destroys a Graph instance */
JNIEXPORT void JNICALL Java_logan_graph_Graph_free_1Native(
		JNIEnv *env, jobject this, jlong handle)
{
	Graph *graph = (Graph *) handle;

	cleanupJniRefs(env, graph->userPtr);

	G_FREE(graph->userPtr, sizeof(GraphJni), MEMTRACKID_GRAPH);
	graph->userPtr=NULL;

	freeGraph(graph);
}

/* logan.graph.Graph: dump: Shows informatino about a Graph instance */
JNIEXPORT void JNICALL Java_logan_graph_Graph_dump_1Native(
		JNIEnv *env, jobject this, jlong handle)
{
//	Graph *graph = (Graph *) handle;


}





