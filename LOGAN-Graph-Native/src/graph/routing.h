#ifndef __ROUTING_H
#define __ROUTING_H

typedef struct routingReadDataStr {
	u8 *packedSeq; // 8
	u8 *quality; // 8
	u32 seqLength; // 4

	s32 indexCount; // 4
	s32 *completionCountPtr; // 8

	// Tracking of edge positions

	s32 minEdgePosition;
	s32 maxEdgePosition;

	// Split into aux structure with []. Add first/last.
	u32 *readIndexes; // 8
	SmerId *fsmers; // 8
	SmerId *rsmers; // 8
	u32 *slices; // 8
	u32 *sliceIndexes; // 8

} __attribute__((aligned (32))) RoutingReadData;


typedef struct routePatchStr
{
	struct routePatchStr *next;
	RoutingReadData **rdiPtr; // Needs double ptr to enable sorting by inbound position

	s32 prefixIndex;
	s32 suffixIndex;
} RoutePatch;


typedef struct routingReadReferenceBlockStr {
	u64 entryCount; // 8
	RoutingReadData **entries; // 8
} __attribute__((aligned (16))) RoutingReadReferenceBlock;

typedef struct routingIndexedReadReferenceBlockStr {
	s32 sliceIndex; // 4
	u32 entryCount; // 4
	RoutingReadData **entries; // 8
} __attribute__((aligned (16))) RoutingIndexedReadReferenceBlock;


typedef union s32floatUnion {
	s32 s32Val;
	float floatVal;
} s32Float;



#define ROUTING_TABLE_FORWARD 0
#define ROUTING_TABLE_REVERSE 1

#define ARRAY_TYPE_SHALLOW_PTR 0
#define ARRAY_TYPE_DEEP_PTR 1
#define ARRAY_TYPE_SHALLOW_DATA 2
#define ARRAY_TYPE_DEEP_DATA 3



typedef struct routePatchMergeWideReadsetStr // Represents a set of reads with same upstream, flexible positions, but potentially varied downstream
{
	struct routePatchMergeWideReadsetStr *next;

	RoutePatch *firstRoutePatch;

	s32 minEdgeOffset;
	s32 maxEdgeOffset; // Closed interval, includes both max and min

} RoutePatchMergeWideReadset;


typedef struct routePatchMergePositionOrderedReadtreeStr // Represents sets of reads with same upstream and defined, consecutive, relative order.
{
	struct routePatchMergePositionOrderedReadtreeStr *next;

	RoutePatchMergeWideReadset *firstWideReadset;

	s32 minEdgePosition;
	s32 maxEdgePosition; // Closed interval, includes both max and min
} RoutePatchMergePositionOrderedReadtree;


typedef struct heapDataBlockStr
{
	s32 headerSize; // Including index/subindex
	//s32 subindexSize;
	//s32 subindex;
	s32 dataSize;
	//u8 *blockPtr; // Warning: Invalidated by (other) allocs
} HeapDataBlock;


typedef struct routeTableArrayBuilderStr RouteTableArrayBuilder;
typedef struct routeTableTreeBuilderStr RouteTableTreeBuilder;

typedef struct routingComboBuilderStr
{
	MemDispenser *disp;
	u8 **rootPtr;

	SeqTailBuilder prefixBuilder;
	SeqTailBuilder suffixBuilder;

	RouteTableArrayBuilder *arrayBuilder;
	HeapDataBlock combinedDataBlock;
	u8 *combinedDataPtr;

	RouteTableTreeBuilder *treeBuilder;
	HeapDataBlock topDataBlock;
	u8 *topDataPtr;

	//HeapDataBlock dataBlocks[8];

	s32 upgradedToTree;
} RoutingComboBuilder;



typedef struct smerRoutingStatsStr
{
	SmerId smerId;
	char smerStr[SMER_BASES+1];

	s32 prefixTails;
	s32 prefixBytes;

	s32 suffixTails;
	s32 suffixBytes;

	s32 routeTableFormat; // 0 - null, 1 - array, 2 - tree

	s64 routeTableForwardRouteEntries;
	s64 routeTableForwardRoutes;

	s64 routeTableReverseRouteEntries;
	s64 routeTableReverseRoutes;

	s64 routeTableArrayBytes;

	s64 routeTableTreeTopBytes;
	s64 routeTableTreeArrayBytes;
	s64 routeTableTreeLeafBytes;
	s64 routeTableTreeBranchBytes;

	s64 routeTableTotalBytes;
	s64 smerTotalBytes;
} SmerRoutingStats;



//#define ROUTING_TREE_THRESHOLD 4
//#define ROUTING_TREE_THRESHOLD 40
//#define ROUTING_TREE_THRESHOLD 400
#define ROUTING_TREE_THRESHOLD 10000
//#define ROUTING_TREE_THRESHOLD 1000000000

s32 rtEncodeTailBlockHeader(u32 prefixSuffix, u32 indexSize, u32 index, u8 *data);
s32 rtDecodeTailBlockHeader(u8 *data, u32 *prefixSuffix, u32 *indexSizePtr, u32 *indexPtr);
s32 rtGetTailBlockHeaderSize(int indexSize);

s32 rtEncodeArrayBlockHeader(u32 arrayNum, u32 arrayType, u32 indexSize, u32 index, u32 subindex, u8 *data);
s32 rtDecodeArrayBlockHeader(u8 *data, u32 *arrayNumPtr, u32 *arrayTypePtr, u32 *indexSizePtr, u32 *indexPtr, u32 *subindexSizePtr, u32 *subindexPtr);
s32 rtGetArrayBlockHeaderSize(int indexSize, int subindexSize);


MemCircHeapChunkIndex *rtReclaimIndexer(u8 *data, s64 targetAmount, u8 tag, u8 **tagData, s32 tagDataLength, s32 tagSearchOffset, MemDispenser *disp);
void rtRelocater(MemCircHeapChunkIndex *index, u8 tag, u8 **tagData, s32 tagDataLength);

int rtRouteReadsForSmer(RoutingIndexedReadReferenceBlock *rdi, SmerArraySlice *slice, RoutingReadData **orderedDispatches, MemDispenser *disp, MemCircHeap *circHeap, u8 sliceTag);

SmerLinked *rtGetLinkedSmer(SmerArray *smerArray, SmerId rootSmerId, MemDispenser *disp);

SmerRoutingStats *rtGetRoutingStats(SmerArraySlice *smerArraySlice, u32 sliceNum, MemDispenser *disp);


#endif

