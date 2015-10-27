
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>

#include "JNI_graph.h"

#include "graph.h"


JNIEXPORT jlong JNICALL Java_logan_graph_Graph_alloc_1Native
  (JNIEnv *env, jobject this, jint n, jint e)
{
	printf("Hello Java World!\n");

	return 0;
}

JNIEXPORT jlong JNICALL Java_pagan_dbg_DeBruijnGraph_alloc_1Native(JNIEnv *env,
		jobject this, jobject config) {

/*
	logInit();

	GraphJni *jni=gAlloc(sizeof(GraphJni), "GraphJni");

	if (prepareJniRefs(env, jni) != 0)
		return 0;

	Graph *graph = allocGraph(jni,
			(*env)->GetIntField(env, config, jni->graphConfigFieldK),
			(*env)->GetIntField(env, config, jni->graphConfigFieldS));

	return (long) (graph);
	*/
	return (long) 0;
}

/* Main API: free: Destroys an Smer Graph instance */

JNIEXPORT void JNICALL Java_pagan_dbg_DeBruijnGraph_free_1Native(JNIEnv *env,
		jobject this, jlong handle) {
	/*
	Graph *graph = (Graph *) handle;

	cleanupJniRefs(env, graph->jni);
	gFree(graph->jni);
	graph->jni=NULL;

	freeGraph(graph);
	*/
}

