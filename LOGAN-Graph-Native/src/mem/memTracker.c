#include "common.h"

static long allocated[MEMTRACKID_SIZE];

char *MEMTRACKER_NAMES[MEMTRACKID_SIZE]=
{
	"SWQ", "IOBUF", "THREADS", "INGRESS", "TASK", 	// 0-4
	"INDEXING_BUILDER", "INDEXING_PACKSEQ", 		// 5,6
	"ROUTING_BUILDER", "ROUTING_WORKERSTATE",		// 7,8
	"GRAPH", "SMER_ID", "SMER_ENTRY", "SMER_DATA",	// 9-12
	"BLOOM", 										// 13
	"DISP","DISP_ROUTING",							// 14-15
	"DISP_ROUTING_INGRESS",							// 16
	"DISP_ROUTING_LOOKUP",							// 17
	"DISP_ROUTING_DISPATCH","DISP_ROUTING_DISPATCH_GROUPSTATE","DISP_ROUTING_DISPATCH_ARRAY", // 18-20
	"DISP_GC", "DISP_LINKED_SMER", "DISP_STATS",	// 21-23
	"HEAP", "HEAP_BLOCK", "HEAP_BLOCKDATA",			// 24-26
	"BRICK_SEQ", "BRICK_LOOKUP", "BRICK_DISPATCH"   // 27-29
};

void mtInit()
{
	for(int i=0;i<MEMTRACKID_SIZE;i++)
		__atomic_store_n(allocated+i, 0, __ATOMIC_SEQ_CST);
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
	long total=0;
	LOG(LOG_INFO,"MemTracker Dump: Begin");
	for(int i=0;i<MEMTRACKID_SIZE;i++)
		{
		long size=__atomic_load_n(allocated+i, __ATOMIC_SEQ_CST);
		LOG(LOG_INFO,"%s using %li (%liM)",MEMTRACKER_NAMES[i], size, (size>>20));
		total+=size;
		}

	LOG(LOG_INFO,"MemTracker Dump: TOTAL USAGE: %li (%liM)", total, (total>>20));
}
