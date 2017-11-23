#ifndef __TASKROUTING_H
#define __TASKROUTING_H


#define TR_INGRESS_PER_TIDY_MIN 1000000000
#define TR_INGRESS_PER_TIDY_MAX 1000000000
#define TR_TIDYS_PER_BACKOFF 1


#define TR_INIT_MINEDGEPOSITION 0
#define TR_INIT_MAXEDGEPOSITION (INT_MAX>>1)



#define LINK_INDEX_DUMMY 0xFFFFFFFF


#define SEQUENCE_LINK_BYTES 56
#define SEQUENCE_LINK_BASES (SEQUENCE_LINK_BYTES*4)

typedef struct sequenceLinkStr {
	u32 nextIndex;			// Index of next SequenceLink in chain (or LINK_DUMMY at end)
	u8 length;				// Length of packed sequence (in bp)
	u8 position;			// Position of first unprocessed base (in bp)
	u8 packedSequence[SEQUENCE_LINK_BYTES];	// Packed sequence (2bits per bp) - up to 232 bases2
} SequenceLink;

typedef struct lookupLinkStr {
	u32 sourceIndex;		// Index of SeqLink or Dispatch
	u8 indexType;			// Indicates meaning of previous (SeqLink or Dispatch)
	u8 smerCount;			// Number of smers to lookup
	u16 revComp;			// Indicates if the original smer was rc
	SmerId smers[15];		// Specific smers to lookup
} LookupLink;


/* Defined in routeTable.h

typedef struct dispatchLinkSmerStr {
	SmerId smer;		// The actual smer ID
	u16 seqIndexOffset; // Distance to next smer, or number of additional kmers already tested (if last)
	u16 slice;			// Ranges (0-16383)
	u32 sliceIndex;		// Index within slice
} DispatchLinkSmer;

typedef struct dispatchLinkStr {
	u32 nextOrOriginIndex;		// Index of Next Dispatch or Origin SeqLink
	u8 indexType;				// Indicates meaning of previous
	u8 length;					// How many valid indexesSmers are there
	u8 position;				// The current indexed smer
	u8 revComp;					// Indicate if the original smer was rc
	s32 minEdgePosition;
	s32 maxEdgePosition;
	DispatchLinkSmer smers[7];
} DispatchLink;

*/


// 0 = idle, 1 = allocated, 2 = active, 3 = finished

#define BLOCK_STATUS_IDLE 0
#define BLOCK_STATUS_ALLOCATED 1
#define BLOCK_STATUS_ACTIVE 2
#define BLOCK_STATUS_COMPLETE 3

/*
typedef struct routingReadIngressSequenceStr {
	u32 length;
	u8 packedSeq[];
} RoutingReadIngressSequence;
*/
typedef struct routingReadIngressBlockStr {
//	RoutingReadIngressSequence *sequenceData[TR_INGRESS_BLOCKSIZE];

//	MemDispenser *disp; // Unified dispenser

//	u32 maxSequenceLength;

	u32 sequenceBrickIndex[TR_INGRESS_BLOCKSIZE];

	u32 sequenceCount;
	u32 sequencePosition;

	s32 completionCount;
	u32 status; // 0 = idle, 1 = allocated, 2 = active, 3 = finished

} RoutingReadIngressBlock;

/*

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

*/

typedef struct routingWorkerStateStr {
	int lookupSliceStart;
	int lookupSliceEnd;

	int dispatchGroupStart;
	int dispatchGroupEnd;

} RoutingWorkerState;

typedef struct routingBuilderStr {
	ParallelTask *pt;
	Graph *graph;

	u64 pogoDebugFlag;

	RoutingReadIngressBlock ingressBlocks[TR_READBLOCK_INGRESS_INFLIGHT]; // Batches of sequences in Ingress stage (currently only one)
	u64 allocatedIngressBlocks;

	MemSingleBrickPile sequenceLinkPile;	// Sequence chains: added during Ingress




	MemDoubleBrickPile lookupLinkPile;		// Lookup chains


	MemDoubleBrickPile dispatchLinkPile;	// Dispatch chains

/*


	RoutingReadLookupBlock readLookupBlocks[TR_READBLOCK_LOOKUPS_INFLIGHT]; // Batches of reads in lookup stage
	u64 allocatedReadLookupBlocks;

	RoutingSmerEntryLookup *smerEntryLookupPtr[SMER_SLICES]; // Per-slice list of lookups

	RoutingReadDispatchBlock readDispatchBlocks[TR_READBLOCK_DISPATCHES_INFLIGHT]; // Batches of reads in dispatch stage
	u64 allocatedReadDispatchBlocks;

	RoutingReadReferenceBlockDispatch *dispatchPtr[SMER_DISPATCH_GROUPS]; // Queued list of dispatches for each target SMER_DISPATCH_GROUP

	RoutingDispatchGroupState dispatchGroupState[SMER_DISPATCH_GROUPS];		// Intermediate representation of a dispatch group during routing
*/

} RoutingBuilder;



RoutingBuilder *allocRoutingBuilder(Graph *graph, int threads);
void freeRoutingBuilder(RoutingBuilder *rb);


#endif
