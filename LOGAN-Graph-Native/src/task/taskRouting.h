#ifndef __TASKROUTING_H
#define __TASKROUTING_H


#define TR_INGRESS_PER_TIDY_MIN 1000000000
#define TR_INGRESS_PER_TIDY_MAX 1000000000
#define TR_TIDYS_PER_BACKOFF 1


#define TR_INIT_MINEDGEPOSITION 0
#define TR_INIT_MAXEDGEPOSITION (INT_MAX>>1)



// Each block: 10000 reads * 150bp? * Smer: 8 * 2: Max 24MByte
// 20 Lookups: 480MBytes (full)
// 200 Dispatches: 2.4GBytes (50% real smers)

// Lookup work is in slices (max 16384)
//#define TR_LOOKUP_MAX_WORK 1024
#define TR_LOOKUP_MAX_WORK 16384
// Dispatch work is in groups (max TR_LOOKUPS_PER_SLICE_BLOCK)
//#define TR_DISPATCH_MAX_WORK 4
#define TR_DISPATCH_MAX_WORK 64

// TR_LOOKUPS_PER_SLICE_BLOCK / TR_LOOKUPS_PER_INTERMEDIATE_BLOCK must be a power of 2
#define TR_LOOKUPS_PER_SLICE_BLOCK 64
#define TR_LOOKUPS_PER_PERCOLATE_BLOCK 16384

//#define TR_DISPATCH_READS_PER_BLOCK 64
#define TR_DISPATCH_READS_PER_INTERMEDIATE_BLOCK 256

typedef struct routingReadLookupDataStr {
	u8 *packedSeq; // 8
	u8 *quality; // 8
	u32 seqLength; // 4 (+4)
	SmerId *smers; // 8 pairs of (fsmer, rsmer)
} __attribute__((aligned (32))) RoutingReadLookupData;


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

} __attribute__((aligned (32))) RoutingSmerEntryLookup;

typedef struct routingLookupPercolateStr {
	u32 entryCount; // 8
	u32 entryPosition; // 8
	u32 *entries; // 8 - pairs of (slice, smerEntry)
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




// Extends a block of read references with queueing and reversing pointers
typedef struct routingReadReferenceBlockDispatchStr {
	struct routingReadReferenceBlockDispatchStr *nextPtr;
	struct routingReadReferenceBlockDispatchStr *prevPtr; // Used to reverse ordering
	s32 *completionCountPtr; // 8

	RoutingReadReferenceBlock data;
} RoutingReadReferenceBlockDispatch;


// Represents an array of outbound read dispatch blocks, one per SMER_DISPATCH_GROUP, plus memory allocation tracking
typedef struct routingDispatchArray {
	struct routingDispatchArray *nextPtr;

	MemDispenser *disp;
	s32 completionCount;

	RoutingReadReferenceBlockDispatch dispatches[SMER_DISPATCH_GROUPS];
} RoutingReadReferenceBlockDispatchArray;


// Represents an array of reads which are undergoing routing
typedef struct routingReadDispatchBlockStr {
	RoutingReadData *readData[TR_INGRESS_BLOCKSIZE];
	u32 readCount;

	MemDispenser *disp;
	s32 completionCount;

	RoutingReadReferenceBlockDispatchArray *dispatchArray;

	u32 status; // 0 = idle, 1 = allocated, 2 = active, 3 = finished
} RoutingReadDispatchBlock;


// Represents the intermediate state of an SMER_DISPATCH_GROUP during read routing, including in and outbound reads
typedef struct routingDispatchGroupStateStr {

	u32 status; // 0 = idle, 1 = active
	s32 forceCount;
	MemDispenser *disp;

	RoutingReadReferenceBlock smerInboundDispatches[SMER_DISPATCH_GROUP_SLICES]; // Accumulator for inbound reads to this group
	RoutingReadReferenceBlockDispatchArray *outboundDispatches; // Array with partially routed reads going to next dispatch group

} RoutingDispatchGroupState;



typedef struct routingWorkerStateStr {
	int lookupSliceStart;
	int lookupSliceEnd;

	int dispatchGroupStart;
	int dispatchGroupEnd;

//	int lookupSliceDefault;
//	int lookupSliceCurrent;

//	int dispatchGroupDefault;
//	int dispatchGroupCurrent;

} RoutingWorkerState;

typedef struct routingBuilderStr {
	ParallelTask *pt;
	Graph *graph;

	u64 pogoDebugFlag;

	RoutingReadLookupBlock readLookupBlocks[TR_READBLOCK_LOOKUPS_INFLIGHT]; // Batches of reads in lookup stage
	u64 allocatedReadLookupBlocks;

	RoutingSmerEntryLookup *smerEntryLookupPtr[SMER_SLICES]; // Per-slice list of lookups

	RoutingReadDispatchBlock readDispatchBlocks[TR_READBLOCK_DISPATCHES_INFLIGHT]; // Batches of reads in dispatch stage
	u64 allocatedReadDispatchBlocks;

	RoutingReadReferenceBlockDispatch *dispatchPtr[SMER_DISPATCH_GROUPS]; // Queued list of dispatches for each target SMER_DISPATCH_GROUP

	RoutingDispatchGroupState dispatchGroupState[SMER_DISPATCH_GROUPS];

} RoutingBuilder;



RoutingBuilder *allocRoutingBuilder(Graph *graph, int threads);
void freeRoutingBuilder(RoutingBuilder *rb);


#endif
