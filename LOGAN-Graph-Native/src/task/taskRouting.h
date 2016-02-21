#ifndef __TASKROUTING_H
#define __TASKROUTING_H


#define TR_INGRESS_BLOCKSIZE 1024

#define TR_INGRESS_PER_TIDY_MIN 1000000000
#define TR_INGRESS_PER_TIDY_MAX 1000000000
#define TR_TIDYS_PER_BACKOFF 1


typedef struct RoutingBuilderStr {
	ParallelTask *pt;
	Graph *graph;

} RoutingBuilder;



typedef struct RoutingReadDataStr {
	u8 *packedSeq;
	u32 seqLength;
	u32 *smers;
	u32 *compFlags;
} RoutingReadData;

typedef struct RoutingSmerEntryLookupStr {
	u32 entryCount;
	SmerEntry entries[];
} RoutingSmerEntryLookup;


typedef struct RoutingReadBlockStr {
	RoutingReadData readData[TR_INGRESS_BLOCKSIZE];	// Holds packed read data and all smers

	RoutingSmerEntryLookup *smerEntryLookups[SMER_SLICES]; // Holds per-slice smer details for lookup

} RoutingReadBlock;


RoutingBuilder *allocRoutingBuilder(Graph *graph, int threads);
void freeRoutingBuilder(RoutingBuilder *rb);


#endif
