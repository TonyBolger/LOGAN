#ifndef __TASKROUTING_H
#define __TASKROUTING_H


#define TR_INGRESS_BLOCKSIZE 10000

#define TR_INGRESS_PER_TIDY_MIN 1000000000
#define TR_INGRESS_PER_TIDY_MAX 1000000000
#define TR_TIDYS_PER_BACKOFF 1

#define TR_READBLOCK_LOOKUPS_INFLIGHT 20
#define TR_READBLOCK_DISPATCHES_INFLIGHT 20

// TR_LOOKUPS_PER_SLICE_BLOCK / TR_LOOKUPS_PER_INTERMEDIATE_BLOCK must be a power of 2
#define TR_LOOKUPS_PER_SLICE_BLOCK 64
#define TR_LOOKUPS_PER_PERCOLATE_BLOCK 16384

//#define TR_DISPATCH_READS_PER_BLOCK 64
#define TR_DISPATCH_READS_PER_INTERMEDIATE_BLOCK 4096


typedef struct routingReadLookupDataStr {
	u8 *packedSeq; // 8
	u8 *quality; // 8
	u32 seqLength; // 4 (+4)

	SmerId *smers; // 8
	u8 *compFlags; // 8
} __attribute__((aligned (8))) RoutingReadLookupData;


// RoutingSmerEntryLookup has 24 bytes core
// Each SmerEntry is 4
// 64 bytes -> 40 spare -> 10 entries
// 128 bytes -> 104 spare -> 26 entries
// 256 bytes -> 232 spare -> 58 entries - 4MB per batch
// 512 bytes -> 488 spare -> 122 entries - 8MB per batch
// Probably best to pick non-binary values to lower thrashing

typedef struct routingSmerEntryLookupStr {
	struct routingSmerEntryLookupStr *nextPtr; //
	s32 *completionCountPtr; // 8
	u64 entryCount; // 8
	SmerEntry *entries; // 8

} __attribute__((aligned (64))) RoutingSmerEntryLookup;

typedef struct routingLookupPercolateStr {
	u64 entryCount; // 8
	u32 *entries; // 8
} __attribute__((aligned (16))) RoutingLookupPercolate;

typedef struct routingReadLookupBlockStr {
	RoutingReadLookupData readData[TR_INGRESS_BLOCKSIZE];	// Holds packed read data and all smers
	u32 readCount;
	u32 maxReadLength;

	RoutingLookupPercolate *smerEntryLookupsPercolates[SMER_LOOKUP_PERCOLATES]; // Holds intermediate lookup data
	RoutingSmerEntryLookup *smerEntryLookups[SMER_SLICES]; // Holds per-slice smer details for lookup

	MemDispenser *disp; // Unified dispenser
	s32 completionCount;
	u32 status; // 0 = idle, 1 = allocated, 2 = active, 3 = finished
} RoutingReadLookupBlock;



typedef struct routingReadDispatchDataStr {
	u8 *packedSeq; // 8
	u8 *quality; // 8
	u32 seqLength; // 4

	s32 indexCount; // 4
	u32 *indexes; // 8
	SmerId *smers; // 8
	u8 *compFlags; // 8
	u32 *slices; // 8

	s32 *completionCountPtr; // 8

} __attribute__((aligned (64))) RoutingReadDispatchData;


typedef struct routingDispatchIntermediateStr {
	u64 entryCount; // 8
	RoutingReadDispatchData **entries; // 8
} __attribute__((aligned (16))) RoutingDispatchIntermediate;


typedef struct routingDispatchStr {
	struct routingDispatchStr *nextPtr;
	struct routingDispatchStr *prevPtr; // Used to reverse ordering
	s32 *completionCountPtr; // 8

	RoutingDispatchIntermediate data;
} RoutingDispatch;


typedef struct routingDispatchArray {
	struct routingDispatchArray *nextPtr;

	MemDispenser *disp;
	s32 completionCount;

	RoutingDispatch dispatches[SMER_DISPATCH_GROUPS];
} RoutingDispatchArray;

typedef struct routingReadDispatchBlockStr {
	RoutingReadDispatchData readData[TR_INGRESS_BLOCKSIZE];
	u32 readCount;

	MemDispenser *disp;
	s32 completionCount;

	RoutingDispatchArray *dispatchArray;

	u32 status; // 0 = idle, 1 = allocated, 2 = active, 3 = finished
} RoutingReadDispatchBlock;



typedef struct routingDispatchGroupStateStr {

	u32 status; // 0 = idle, 1 = active
	s32 forceCount;
	MemDispenser *disp;

	RoutingDispatchIntermediate smerInboundDispatches[SMER_DISPATCH_GROUP_SLICES]; // Accumulator for inbound reads to this group
	RoutingDispatchArray *outboundDispatches; // Lists with partially dispatched reads going to next dispatch group

} RoutingDispatchGroupState;


typedef struct routingBuilderStr {
	ParallelTask *pt;
	Graph *graph;

	RoutingReadLookupBlock readLookupBlocks[TR_READBLOCK_LOOKUPS_INFLIGHT]; // Batches of reads in lookup stage
	u64 allocatedReadLookupBlocks;

	RoutingSmerEntryLookup *smerEntryLookupPtr[SMER_SLICES]; // Per-slice list of lookups

	RoutingReadDispatchBlock readDispatchBlocks[TR_READBLOCK_DISPATCHES_INFLIGHT]; // Batches of reads in dispatch stage
	u64 allocatedReadDispatchBlocks;

	RoutingDispatch *dispatchPtr[SMER_DISPATCH_GROUPS]; // List of dispatches for each target group

	RoutingDispatchGroupState dispatchGroupState[SMER_DISPATCH_GROUPS];

} RoutingBuilder;



RoutingBuilder *allocRoutingBuilder(Graph *graph, int threads);
void freeRoutingBuilder(RoutingBuilder *rb);


#endif
