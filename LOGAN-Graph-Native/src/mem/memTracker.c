#include "common.h"

static long allocated[MEMTRACKID_SIZE];

char *MEMTRACKER_NAMES[MEMTRACKID_SIZE]=
{
	"SWQ", "IOBUF", "THREADS", "INGRESS", "TASK", 			// 0-4
	"INDEXING_BUILDER", "INDEXING_PACKSEQ", 				// 5,6
	"ROUTING_BUILDER", "ROUTING_WORKERSTATE",				// 7,8
	"GRAPH", "SEQ_INDEX", 									// 9,10
	"SMER_ID", "SMER_ENTRY", "SMER_DATA",			        // 11-13
	"BLOOM", 												// 14
	"DISP","DISP_ROUTING",									// 15-16
	"DISP_ROUTING_INGRESS",									// 17
	"DISP_ROUTING_LOOKUP","DISP_ROUTING_LOOKUP_RECYCLE", 	// 18-19
	"DISP_ROUTING_DISPATCH","DISP_ROUTING_DISPATCH_GROUPSTATE","DISP_ROUTING_DISPATCH_ARRAY", // 20-22
	"DISP_GC", "DISP_LINKED_SMER", "DISP_STATS",			// 23-25
	"HEAP", "HEAP_BLOCK", "HEAP_BLOCKDATA_FIXED", "HEAP_BLOCKDATA_CIRC",	// 26-29
	"BRICK_SEQ", "BRICK_LOOKUP", "BRICK_DISPATCH",   		// 30-32
	"SERDES"												// 33
};

void mtInit()
{
#ifdef FEATURE_ENABLE_MEMTRACK
	for(int i=0;i<MEMTRACKID_SIZE;i++)
		__atomic_store_n(allocated+i, 0, __ATOMIC_SEQ_CST);
#endif
}

void mtTrackAlloc(size_t size, s16 memTrackerId)
{
	__atomic_fetch_add(allocated+memTrackerId, size, __ATOMIC_SEQ_CST);
}

void mtTrackFree(size_t size, s16 memTrackerId)
{
	__atomic_fetch_sub(allocated+memTrackerId, size, __ATOMIC_SEQ_CST);
}

void mtDump()
{
#ifdef FEATURE_ENABLE_MEMTRACK
	long total=0;
	LOG(LOG_INFO,"MemTracker Dump: Begin");
	for(int i=0;i<MEMTRACKID_SIZE;i++)
		{
		long size=__atomic_load_n(allocated+i, __ATOMIC_SEQ_CST);
		LOG(LOG_INFO,"%s using %li (%liM)",MEMTRACKER_NAMES[i], size, (size>>20));
		total+=size;
		}

	LOG(LOG_INFO,"MemTracker Dump: TOTAL USAGE: %li (%liM)", total, (total>>20));
#endif
}
