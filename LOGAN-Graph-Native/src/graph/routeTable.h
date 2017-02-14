#ifndef __ROUTE_TABLE_H
#define __ROUTE_TABLE_H


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
	// Used for all heap data (first level object)
	s32 headerSize; // Including index/subindex
	s32 dataSize;

	// Used for 2-level arrays only
	s32 deepHeaderSize;
	s32 completeDeepCount;
	s32 completeDeepDataSize;
	s32 additionalDeepDataSize;
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


/* Structs wrapping the heap-format tree, allowing nicer manipulation: The details are defined in the corresponding headers */

typedef struct routeTableTreeArrayProxyStr RouteTableTreeArrayProxy;
typedef struct routeTableTreeBranchProxyStr RouteTableTreeBranchProxy;
typedef struct routeTableTreeLeafProxyStr  RouteTableTreeLeafProxy;
typedef struct routeTableTreeProxyStr RouteTableTreeProxy;



s32 rtHeaderIsLive(u8 data);
s32 rtHeaderIsLiveDirect(u8 data);
s32 rtHeaderIsLiveTop(u8 data);

void rtHeaderMarkDead(u8 *data);

s32 rtEncodeGapDirectBlockHeader(s32 tagOffsetDiff, u8 *data);
s32 rtEncodeGapTopBlockHeader(s32 tagOffsetDiff, u8 *data);
s32 rtDecodeGapBlockHeader(u8 *data, s32 *tagOffsetDiffPtr);
s32 rtGetGapBlockHeaderSize();

s32 rtEncodeTailBlockHeader(u32 prefixSuffix, u32 indexSize, u32 index, u8 *data);
s32 rtDecodeTailBlockHeader(u8 *data, u32 *prefixSuffix, u32 *indexSizePtr, u32 *indexPtr);
s32 rtGetTailBlockHeaderSize(int indexSize);

s32 rtEncodeArrayBlockHeader_Branch0(s32 fwdRev, s32 indexSize, s32 index, u8 *data);
s32 rtDecodeArrayBlockHeader_Branch0(u8 *data, s32 *indexSizePtr, s32 *indexPtr);
s32 rtEncodeArrayBlockHeader_Leaf0(s32 fwdRev, s32 levels, s32 indexSize, s32 index, u8 *data);
s32 rtDecodeArrayBlockHeader_Leaf0(u8 *data, s32 *levelsPtr, s32 *indexSizePtr, s32 *indexPtr);
s32 rtDecodeIndexedBlockHeader_0(u8 *data, s32 *indexSizePtr, s32 *indexPtr);
s32 rtGetIndexedBlockHeaderSize_0(s32 indexSize);

s32 rtEncodeArrayBlockHeader_Leaf1(s32 fwdRev, s32 indexSize, s32 index, s32 subindex, u8 *data);
s32 rtDecodeArrayBlockHeader_Leaf1(u8 *data, s32 *indexSizePtr, s32 *indexPtr, s32 subindexPtr);

s32 rtEncodeEntityBlockHeader_Branch1(s32 fwdRev, s32 indexSize, s32 index, s32 subindex, u8 *data);
s32 rtDecodeEntityBlockHeader_Branch1(u8 *data, s32 *indexSizePtr, s32 *indexPtr, s32 subindexPtr);
s32 rtEncodeEntityBlockHeader_Leaf1(s32 fwdRev, s32 indexSize, s32 index, s32 subindex, u8 *data);
s32 rtDecodeEntityBlockHeader_Leaf1(u8 *data, s32 *indexSizePtr, s32 *indexPtr, s32 subindexPtr);
s32 rtDecodeIndexedBlockHeader_1(u8 *data, s32 *indexSizePtr, s32 *indexPtr, s32 *subindexPtr);
s32 rtGetIndexedBlockHeaderSize_1(s32 indexSize);

s32 rtEncodeEntityBlockHeader_Leaf2(s32 fwdRev, s32 indexSize, s32 index, s32 subindex, u8 *data);
s32 rtDecodeEntityBlockHeader_Leaf2(u8 *data, s32 *indexSizePtr, s32 *indexPtr, s32 subindexPtr);
s32 rtGetIndexedBlockHeaderSize_2(s32 indexSize);
s32 rtDecodeIndexedBlockHeader(u8 *data, s32 *topindexPtr, s32 *indexSizePtr, s32 *indexPtr, s32 *subindexSizePtr, s32 *subindexPtr);


MemCircHeapChunkIndex *rtReclaimIndexer(u8 *data, s64 targetAmount, u8 tag, u8 **tagData, s32 tagDataLength, s32 tagSearchOffset, MemDispenser *disp);
void rtRelocater(MemCircHeapChunkIndex *index, u8 tag, u8 **tagData, s32 tagDataLength);

void initHeapDataBlock(HeapDataBlock *block);




#endif
