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
#define TR_LOOKUPS_PER_INTERMEDIATE_BLOCK 16384

#define TR_DISPATCH_READS_PER_BLOCK 64
#define TR_DISPATCH_READS_PER_INTERMEDIATE_BLOCK 16384


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
	u32 *completionCountPtr; // 8
	u64 entryCount; // 8
	SmerEntry *entries; // 8

} __attribute__((aligned (64))) RoutingSmerEntryLookup;

typedef struct routingLookupIntermediateStr {
	u64 entryCount; // 8
	u32 *entries; // 8
} __attribute__((aligned (16))) RoutingLookupIntermediate;

typedef struct routingReadLookupBlockStr {
	RoutingReadLookupData readData[TR_INGRESS_BLOCKSIZE];	// Holds packed read data and all smers
	u32 readCount;
	u32 maxReadLength;

	RoutingLookupIntermediate *smerIntermediateLookups[SMER_LOOKUPSLICEGROUPS]; // Holds intermediate lookup data
	RoutingSmerEntryLookup *smerEntryLookups[SMER_SLICES]; // Holds per-slice smer details for lookup

	MemDispenser *disp; // Unified dispenser
	u32 completionCount;
	u32 status; // 0 = idle, 1 = allocated, 2 = active, 3 = finished
} RoutingReadLookupBlock;



typedef struct routingReadDispatchDataStr {
	u8 *packedSeq; // 8
	u8 *quality; // 8
	u32 seqLength; // 4

	u32 indexCount; // 4
	u32 *indexes; // 8
	SmerId *smers; // 8
	u8 *compFlags; // 8
	u32 *completionCountPtr; // 8

} __attribute__((aligned (64))) RoutingReadDispatchData;

typedef struct routingReadDispatchBlockStr {
	RoutingReadDispatchData readData[TR_INGRESS_BLOCKSIZE];

	MemDispenser *disp;
	u32 completionCount;
	u32 status; // 0 = idle, 1 = allocated, 2 = active, 3 = finished
} RoutingReadDispatchBlock;

typedef struct routingDispatchIntermediateStr {
	u64 entryCount; // 8
	RoutingReadDispatchData *entries; // 8
} __attribute__((aligned (16))) RoutingDispatchIntermediate;


typedef struct routingDispatchStr {
	struct routingSmerEntryLookupStr *nextPtr;
	RoutingReadDispatchData *readData[TR_DISPATCH_READS_PER_BLOCK];

} RoutingDispatch;


typedef struct routingBuilderStr {
	ParallelTask *pt;
	Graph *graph;

	RoutingReadLookupBlock readLookupBlocks[TR_READBLOCK_LOOKUPS_INFLIGHT]; // Batches of reads in lookup stage
	u64 allocatedReadLookupBlocks;
	RoutingSmerEntryLookup *smerEntryLookupPtr[SMER_SLICES]; // Per-slice list of lookups

	RoutingReadDispatchBlock readDispatchBlocks[TR_READBLOCK_DISPATCHES_INFLIGHT]; // Batches of reads in dispatch stage
	u64 allocatedReadDispatchBlocks;

	RoutingDispatch *dispatchPtr[SMER_DISPATCHSLICEGROUPS]; // Per-slicegroup list of dispatches
	RoutingDispatchIntermediate *smerIntermediateDispatches[SMER_DISPATCHSLICEGROUPS][SMER_DISPATCHSLICEGROUPS]; // Accumulator for partially


} RoutingBuilder;



RoutingBuilder *allocRoutingBuilder(Graph *graph, int threads);
void freeRoutingBuilder(RoutingBuilder *rb);


#endif
