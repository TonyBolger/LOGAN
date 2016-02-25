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

// 24 bytes core
// Each SmerEntry is 4
// 64 bytes -> 40 spare -> 10 entries
// 128 bytes -> 104 spare -> 26 entries
// 256 bytes -> 232 spare -> 58 entries - 4MB per batch
// 512 bytes -> 488 spare -> 122 entries - 8MB per batch

typedef struct routingSmerEntryLookupStr {
	struct routingSmerEntryLookupStr *nextPtr; //8
	u32 *completionCountPtr; // 8
	u64 entryCount; // 8
	SmerEntry *entries; // 8

} RoutingSmerEntryLookup;

typedef struct routingReadBlockStr {
	RoutingReadData readData[TR_INGRESS_BLOCKSIZE];	// Holds packed read data and all smers
	RoutingSmerEntryLookup *smerEntryLookups[SMER_SLICES]; // Holds per-slice smer details for lookup

	MemDispenser *disp; // Unified dispenser
	u32 completionCount;
	u32 status; // 0 = idle, 1 = allocated, 2 = active, 3 = finished

	//MemDispenser *mainDisp; // For data structure itself
	//MemDispenser *packDisp; // For packed sequence and quality score
	//MemDispenser *smerDisp; // For smers and compflags
	//MemDispenser *lookupDisp; // For entry lookup
} RoutingReadBlock;



typedef struct routingBuilderStr {
	ParallelTask *pt;
	Graph *graph;

	RoutingReadBlock readBlocks[TR_READBLOCKS_INFLIGHT];
	u64 allocatedReadBlocks;

	RoutingSmerEntryLookup *smerEntryLookupPtr[SMER_SLICES];

} RoutingBuilder;



RoutingBuilder *allocRoutingBuilder(Graph *graph, int threads);
void freeRoutingBuilder(RoutingBuilder *rb);


#endif
