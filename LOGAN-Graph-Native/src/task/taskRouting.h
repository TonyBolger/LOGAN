#ifndef __TASKROUTING_H
#define __TASKROUTING_H


#define TR_INGRESS_BLOCKSIZE 10000

#define TR_INGRESS_PER_TIDY_MIN 1000000000
#define TR_INGRESS_PER_TIDY_MAX 1000000000
#define TR_TIDYS_PER_BACKOFF 1


#define TR_READBLOCKS_INFLIGHT 20

// TR_LOOKUPS_PER_SLICE_BLOCK must be a power of 2
#define TR_LOOKUPS_PER_SLICE_BLOCK 64

typedef struct routingReadDataStr {
	u8 *packedSeq;
	//u8 *quality;
	u32 seqLength;

	SmerId *smers;
	u8 *compFlags;
} RoutingReadData;

typedef struct routingSmerEntryLookupStr {
	struct routingSmerEntryLookupStr *nextPtr;
	SmerEntry *entries;
	u64 *presenceAbsence;
	u64 entryCount;

	u32 *completionCountPtr;
} RoutingSmerEntryLookup;

typedef struct routingReadBlockStr {
	struct routingReadBlockStr *nextPtr;

	RoutingReadData readData[TR_INGRESS_BLOCKSIZE];	// Holds packed read data and all smers
	RoutingSmerEntryLookup *smerEntryLookups[SMER_SLICES]; // Holds per-slice smer details for lookup

	MemDispenser *disp; // Unified dispenser
	u32 completionCount;

	//	MemDispenser *mainDisp; // For data structure itself, packed sequence and quality score
	//	MemDispenser *smerDisp; // For smers and compflags
	//  MemDispenser *lookupDisp; // For entry lookup
} RoutingReadBlock;



typedef struct routingBuilderStr {
	ParallelTask *pt;
	Graph *graph;

	RoutingReadBlock *readBlockPtr;
	u64 numReadsBlocks;

	RoutingSmerEntryLookup *smerEntryLookupPtr[SMER_SLICES];

} RoutingBuilder;



RoutingBuilder *allocRoutingBuilder(Graph *graph, int threads);
void freeRoutingBuilder(RoutingBuilder *rb);


#endif
