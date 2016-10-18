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
	s32 subindexSize;
	s32 subindex;
	s32 headerSize;
	s32 dataSize;
	u8 *blockPtr; // Warning: Invalidated by (other) allocs
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

	RouteTableTreeBuilder *treeBuilder;
	HeapDataBlock topDataBlock;
	HeapDataBlock prefixDataBlock;
	HeapDataBlock suffixDataBlock;
	HeapDataBlock routeTableDataBlock;

	s32 upgradedToTree;

} RoutingComboBuilder;


#define ROUTING_TREE_THRESHOLD 256



void rtEncodeNonRootBlockHeader(u32 isLeaf, u32 indexSize, u32 index, u32 subindexSize, u32 subindex, u8 *data);
void rtDecodeNonRootBlockHeader(u8 *data, s32 *isLeafPtr, s32 *indexSizePtr, s32 *indexPtr, s32 *subindexSizePtr, s32 *subindexPtr);
s32 rtGetNonRootBlockHeaderSize(int indexSize, int subIndexSize);

MemCircHeapChunkIndex *rtReclaimIndexer(u8 *data, s64 targetAmount, u8 tag, u8 **tagData, s32 tagDataLength, s32 tagSearchOffset, MemDispenser *disp);
void rtRelocater(MemCircHeapChunkIndex *index, u8 tag, u8 **tagData, s32 tagDataLength);

int rtRouteReadsForSmer(RoutingIndexedReadReferenceBlock *rdi, SmerArraySlice *slice, RoutingReadData **orderedDispatches, MemDispenser *disp, MemCircHeap *circHeap, u8 sliceTag);

SmerLinked *rtGetLinkedSmer(SmerArray *smerArray, SmerId rootSmerId, MemDispenser *disp);


#endif

