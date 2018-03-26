#ifndef __TASKROUTING_H
#define __TASKROUTING_H


#define TR_INGRESS_PER_TIDY_MIN 1000000000
#define TR_INGRESS_PER_TIDY_MAX 1000000000
#define TR_TIDYS_PER_BACKOFF 1


#define TR_INIT_EDGEPOSITION_INVALID -1
#define TR_INIT_MINEDGEPOSITION 0
#define TR_INIT_MAXEDGEPOSITION (INT_MAX>>1)


// 0 = idle, 1 = allocated,2 = active,3 = locked, 4 = complete

// Ingress uses a simple integer status

#define INGRESS_BLOCK_STATUS_IDLE 0x00000
#define INGRESS_BLOCK_STATUS_ALLOCATED 0x10000
#define INGRESS_BLOCK_STATUS_ACTIVE 0x20000
#define INGRESS_BLOCK_STATUS_LOCKED 0x30000
#define INGRESS_BLOCK_STATUS_COMPLETE 0x40000


// Lookup use a shifted status combined with completion count

#define LOOKUP_BLOCK_STATUS_MASK 0xFFFF0000
#define LOOKUP_BLOCK_COUNT_MASK 0x0000FFFF

#define LOOKUP_BLOCK_STATUS_IDLE 0x00000
#define LOOKUP_BLOCK_STATUS_ALLOCATED 0x10000
#define LOOKUP_BLOCK_STATUS_ACTIVE 0x20000
#define LOOKUP_BLOCK_STATUS_LOCKED 0x30000
#define LOOKUP_BLOCK_STATUS_COMPLETE 0x40000

/*
typedef struct routingReadIngressSequenceStr {
	u32 length;
	u8 packedSeq[];
} RoutingReadIngressSequence;
*/

//#define BLOCK_INGRESS_UNLOCKED 0
//#define BLOCK_INGRESS_LOCKED 1

typedef struct routingReadIngressBlockStr {
//	RoutingReadIngressSequence *sequenceData[TR_INGRESS_BLOCKSIZE];

//	MemDispenser *disp; // Unified dispenser

//	u32 maxSequenceLength;

	u32 sequenceLinkIndex[TR_INGRESS_BLOCKSIZE];

	u32 sequenceCount;
	u32 sequencePosition;

	u32 status; // 0 = idle: Available
				// 1 = allocated: Reads being ingressed
				// 2 = active: Reads need lookups
				// 3 = locked: Reads being transferred to lookup
				// 4 = complete: May be superfluous

} RoutingReadIngressBlock;


// Extends a block of read references with queueing and reversing pointers
typedef struct routingReadReferenceBlockDispatchStr {
	struct routingReadReferenceBlockDispatchStr *nextPtr;
	struct routingReadReferenceBlockDispatchStr *prevPtr; // Used to reverse ordering
	s32 *completionCountPtr; // 8

	RoutingDispatchLinkIndexBlock data;
} RoutingReadReferenceBlockDispatch;


// Represents an array of outbound read dispatch blocks, one per SMER_DISPATCH_GROUP, plus memory allocation tracking
typedef struct routingDispatchArray {
	struct routingDispatchArray *nextPtr;

	MemDispenser *disp;
	s32 completionCount;

	RoutingReadReferenceBlockDispatch dispatches[SMER_DISPATCH_GROUPS];
} RoutingReadReferenceBlockDispatchArray;






// RoutingSmerEntryLookup has 24 bytes core
// Each SmerEntry is 4
// 64 bytes -> 40 spare -> 10 entries
// 128 bytes -> 104 spare -> 26 entries
// 256 bytes -> 232 spare -> 58 entries - 4MB per batch
// 512 bytes -> 488 spare -> 122 entries - 8MB per batch
// Probably best to pick non-binary values to lower thrashing

typedef struct routingSmerEntryLookupStr {
	struct routingSmerEntryLookupStr *nextPtr; //
	u64 *completionCountPtr; // 8
	u64 entryCount; // 8
	SmerEntry *entries; // 8

} __attribute__((aligned (32))) RoutingSmerEntryLookup;

typedef struct routingLookupPercolateStr {
	u32 entryCount; // 8
	u32 entryPosition; // 8
	u32 *entries; // 8 - pairs of (slice, smerEntry)
} __attribute__((aligned (16))) RoutingLookupPercolate;


typedef struct routingReadLookupBlockStr {
	u32 lookupLinkIndex[TR_ROUTING_BLOCKSIZE];

	s32 blockType;
	u32 readCount;

	RoutingLookupPercolate *smerEntryLookupsPercolates[SMER_LOOKUP_PERCOLATES]; // Holds intermediate lookup data
	RoutingSmerEntryLookup *smerEntryLookups[SMER_SLICES]; // Holds per-slice smer details for lookup

	MemDispenser *disp; // Used for percolate / entryLookups

	u64 compStat; // Low half is completion count, high half is status

	RoutingReadReferenceBlockDispatchArray *outboundDispatches;

} RoutingReadLookupBlock;


#define LOOKUP_RECYCLE_BLOCK_PREDISPATCH 0
#define LOOKUP_RECYCLE_BLOCK_POSTDISPATCH 1

typedef struct routingReadLookupRecycleBlockStr {
	struct routingReadLookupRecycleBlockStr *nextPtr;

	MemDispenser *disp;

	s32 blockType;
	s32 readCount;
	s32 readPosition;

	u32 lookupLinkIndex[TR_LOOKUP_RECYCLE_BLOCKSIZE];

} RoutingReadLookupRecycleBlock;



#define TR_ROUTING_DISPATCHES_PER_LOOKUP 3



/*

// Lookup work is in slices (max 16384)
//#define TR_LOOKUP_MAX_WORK 1024
#define TR_LOOKUP_MAX_WORK 16384
// Dispatch work is in groups (max TR_LOOKUPS_PER_SLICE_BLOCK)
//#define TR_DISPATCH_MAX_WORK 4
#define TR_DISPATCH_MAX_WORK 64
*/
// TR_LOOKUPS_PER_SLICE_BLOCK / TR_LOOKUPS_PER_INTERMEDIATE_BLOCK must be a power of 2

//#define TR_LOOKUPS_PER_SLICE_BLOCK 64
//#define TR_LOOKUPS_PER_PERCOLATE_BLOCK 16384

#define TR_LOOKUPS_PER_SLICE_BLOCK 16
#define TR_LOOKUPS_PER_PERCOLATE_BLOCK 8192

//#define TR_DISPATCH_READS_PER_BLOCK 64
#define TR_DISPATCH_READS_PER_INTERMEDIATE_BLOCK 256

#define TR_DISPATCH_READS_PER_QUEUE_BLOCK 256

/*
//__attribute__((aligned (32)))

typedef struct routingReadLookupDataStr {
	u8 *packedSeq; // 8
	//u8 *quality; // 8
	u32 seqLength; // 4 (+4)
	SmerId *smers; // 8 pairs of (fsmer, rsmer)
} RoutingReadLookupData;


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


*/


/*
// Represents an array of reads which are undergoing routing
typedef struct routingReadDispatchBlockStr {
	RoutingReadData *readData[TR_ROUTING_BLOCKSIZE];
	u32 readCount;

	MemDispenser *disp;

	RoutingReadReferenceBlockDispatchArray *dispatchArray;

	u32 status; // 0 = idle, 1 = allocated, 2 = active, 3 = finished
	s32 completionCount;

} RoutingReadDispatchBlock;

*/

// Represents the intermediate state of an SMER_DISPATCH_GROUP during read routing, including in and outbound reads
typedef struct routingDispatchGroupStateStr {

	u32 status; // 0 = idle, 1 = active
	s32 forceCount;
	MemDispenser *disp;
	MemDispenser *swapDisp;

	RoutingDispatchLinkIndexBlock smerInboundDispatches[SMER_DISPATCH_GROUP_SLICES]; // Accumulator for inbound reads to this group
	RoutingReadReferenceBlockDispatchArray *outboundDispatches; // Array with partially routed reads going to next dispatch group

	RoutingSliceAssignedDispatchLinkQueue dispatchLinkQueue;

} RoutingDispatchGroupState;



typedef struct routingWorkerStateStr {
	int dummy;

	//int lookupSliceStart;
	//int lookupSliceEnd;

	//int dispatchGroupStart;
	//int dispatchGroupEnd;

	//int dispatchGroupCurrent;

} RoutingWorkerState;


typedef struct routingBuilderStr RoutingBuilder;

typedef struct rptThreadDataStr
{
	RoutingBuilder *routingBuilder;
	int threadIndex;
} RptThreadData;


struct routingBuilderStr {
	ParallelTask *pt;
	Graph *graph;

	pthread_t *thread;
	RptThreadData *threadData;

	u64 pogoDebugFlag;
	u64 routingWorkToken;

	u64 sequencesInFlight;

	RoutingReadIngressBlock ingressBlocks[TR_READBLOCK_INGRESS_INFLIGHT]; // Batches of sequences in Ingress stage (currently only one)
	u64 allocatedIngressBlocks;

	MemSingleBrickPile sequenceLinkPile;	// Sequence chains: added during Ingress

	MemDoubleBrickPile lookupLinkPile;		// Lookup chains
	RoutingReadLookupBlock readLookupBlocks[TR_READBLOCK_LOOKUPS_INFLIGHT]; // Batches of reads in lookup stage
	u64 allocatedReadLookupBlocks;

	RoutingSmerEntryLookup *smerEntryLookupPtr[SMER_SLICES]; // Per-slice list of lookups

	RoutingReadLookupRecycleBlock *lookupPredispatchRecyclePtr; // Queue of lookups for recycling (pre-dispatch)
	u64 lookupPredispatchRecycleCount; // Approximate length of recycle queue

	RoutingReadLookupRecycleBlock *lookupPostdispatchRecyclePtr; // Queue of lookups for recycling (post-dispatch)
	u64 lookupPostdispatchRecycleCount; // Approximate length of recycle queue

	MemDoubleBrickPile dispatchLinkPile;	// Dispatch chains

	//RoutingReadDispatchBlock readDispatchBlocks[TR_READBLOCK_DISPATCHES_INFLIGHT]; // Batches of reads in dispatch stage
	//u64 allocatedReadDispatchBlocks;

	RoutingReadReferenceBlockDispatch *dispatchPtr[SMER_DISPATCH_GROUPS]; // Queued list of dispatches for each target SMER_DISPATCH_GROUP

	RoutingDispatchGroupState dispatchGroupState[SMER_DISPATCH_GROUPS];		// Intermediate representation of a dispatch group during routing

};


void trDumpSequenceLink(SequenceLink *link, u32 linkIndex);
void trDumpSequenceLinkChain(MemSingleBrickPile *seqPile, SequenceLink *link, u32 linkIndex);

void trDumpLookupLink(LookupLink *link, u32 linkIndex);
void trDumpDispatchLink(DispatchLink *link, u32 linkIndex);
void trDumpDispatchLinkChain(MemDoubleBrickPile *dispatchPile, DispatchLink *link, u32 linkIndex);

void trDumpLinkChain(MemSingleBrickPile *sequencePile, MemDoubleBrickPile *lookupPile, MemDoubleBrickPile *dispatchPile, u32 linkIndex, u8 indexType);

void trDumpLinkChainFromLookupLink(MemSingleBrickPile *sequencePile, MemDoubleBrickPile *lookupPile, MemDoubleBrickPile *dispatchPile, LookupLink *lookupLink, u32 lookupLinkIndex);
void trDumpLinkChainFromDispatchLink(MemSingleBrickPile *sequencePile, MemDoubleBrickPile *lookupPile, MemDoubleBrickPile *dispatchPile, DispatchLink *dispatchLink, u32 dispatchLinkIndex);

RoutingBuilder *allocRoutingBuilder(Graph *graph, int threads);

void createRoutingBuilderWorkers(RoutingBuilder *rb);

void freeRoutingBuilder(RoutingBuilder *rb);


#endif
