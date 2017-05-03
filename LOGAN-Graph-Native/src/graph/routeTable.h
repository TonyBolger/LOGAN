#ifndef __ROUTE_TABLE_H
#define __ROUTE_TABLE_H

// Route Table Array: Single block for both forward/reverse routing tables

// Huge format:   1 1 1 W W W W W  P P P P S S S S
//                F F F F F F F F  F F F F F F F F
//                F F F F F F F F  F F F F F F F F
// 				  R R R R R R R R  R R R R R R R R
// 				  R R R R R R R R  R R R R R R R R
//
// Max: W=2^31, P=2^15, S=2^15, F=2^32, R=2^32

// Large format:  1 1 0 W W W W W  P P P P S S S S
//                F F F F F F F F  F F F F F F F F
// 				  R R R R R R R R  R R R R R R R R
//
// Max: W=2^31, P=2^15, S=2^15, F=2^16, R=2^16

// Medium format: 1 0 W W W W P P  P S S S F F F F
//                F F R R R R R R
//
// Max: W=2^15, P=2^7, S=2^7, F=2^6, R=2^6

// Small format:  0 W W W P P S S  F F F F R R R R
//
// Max: W=2^7, P=2^3, S=2^3, F=2^4, R=2^4

// Route Table Tree: Separate trees for forward and reverse routing tables, split leaf/branches with arrays, with offset indexes
//
// Top:		u8 *data[6]; // Tail(P,S), Branch(F,R), Leaf(F,R)
// 1 Level:	top[]->array[1024]->leaf/branch
// 2 Level: top[]->array[256]->array[1024]->leaf
//
// Array:	u16 dataAlloc
//			u8 *data[dataAlloc]
//
// Branch:	u16 childAlloc
//			u16 parentBrindex
//			s16 children[childAlloc]
//
// Leaf:	s16 offsetAlloc; Downstream
//			s16 entryAlloc;
//
//			s16 parentBrindex;
//			s16 upstream;
//			s32 upstreamOffset;
//
//			u8 extraData[];

// PackedTree:
//
// Branch:  u8 header
//				totalSize(8/16 bit) 1?
//				upstreamSize(8/16 bit) 1
//				downstreamSize(8/16 bit) 1
//				offsetSize(8/16/24/32 bit) 2
//
//			totalSize totalSize
//
//			upstreamSize upstreamFirst
//			upstreamSize upstreamAlloc
//			downstreamSize downstreamFirst
//			downstreamSize downstreamAlloc
//			childAlloc
//
//			upstreamOffsets[upstreamAlloc]
//			downstreamOffsets[downstreamAlloc]
//			children[childAlloc]
//
// Leaf:	u8 header
//				totalSize(8/16 bit)     1?
//				upstreamSize(8/16 bit)  1
//				downstreamSize(8/16 bit) 1
//				offsetSize(8/16/24/32 bit) 2
//				width(8/16/24/32 bit) 2
//
//			totalSize totalSize
//
//			upstreamFirst
//			upstreamAlloc
//			downstreamFirst
//			downstreamAlloc
//
//			upstreamOffsets[upstreamAlloc]
//			downstreamOffsets[downstreamAlloc]
//			perUpstreamData[upstreamAlloc?] // Could be just the upstream with non-zero offsets
//				entryCount
//					downstream[entryCount]
//					width[entryCount]


typedef struct routingReadDataEntryStr {
	u32 readIndex; // 4
	SmerId fsmer; // 8
	SmerId rsmer; // 8
	u32 slice; // 4
	u32 sliceIndex; // 4
} RoutingReadIndexedDataEntry;

typedef struct routingReadDataStr {
	//u8 *packedSeq; // 8
	//u8 *quality; // 8
	//u32 seqLength; // 4

	s32 *completionCountPtr; // 8
	s32 indexCount; // 4

	// Tracking of edge positions

	s32 minEdgePosition;
	s32 maxEdgePosition;

	RoutingReadIndexedDataEntry indexedData[];

	// Split into aux RoutingReadDataEntry structure with []. Add first/last.
	//u32 *readIndexes; // 8
	//SmerId *fsmers; // 8
	//SmerId *rsmers; // 8
	//u32 *slices; // 8
	//u32 *sliceIndexes; // 8

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
	u32 sliceIndex;

	// Used for all heap data (first level object)
	s32 headerSize; // Including index/subindex
	s32 dataSize;

	s32 variant; // For Leaf Array: 0 => single level, 1 => two level
	// Used for 2-level arrays only
	s32 midHeaderSize;
	s32 completeMidCount;
	s32 completeMidDataSize;
	s32 partialMidDataSize;
} HeapDataBlock;


typedef struct routeTableArrayBuilderStr RouteTableArrayBuilder;
typedef struct routeTableTreeBuilderStr RouteTableTreeBuilder;

typedef struct routingComboBuilderStr
{
	MemDispenser *disp;
	u8 **rootPtr;
	s32 sliceIndex;
	u8 sliceTag;

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
s32 rtDecodeIndexedBlockHeader_0(u8 *data, s32 *variantPtr, s32 *indexSizePtr, s32 *indexPtr);
s32 rtGetIndexedBlockHeaderSize_0(s32 indexSize);

s32 rtEncodeArrayBlockHeader_Leaf1(s32 fwdRev, s32 indexSize, s32 index, s32 subindex, u8 *data);
s32 rtDecodeArrayBlockHeader_Leaf1(u8 *data, s32 *indexSizePtr, s32 *indexPtr, s32 *subindexPtr);

s32 rtEncodeEntityBlockHeader_Branch1(s32 fwdRev, s32 indexSize, s32 index, s32 subindex, u8 *data);
s32 rtDecodeEntityBlockHeader_Branch1(u8 *data, s32 *indexSizePtr, s32 *indexPtr, s32 *subindexPtr);
s32 rtEncodeEntityBlockHeader_Leaf1(s32 fwdRev, s32 indexSize, s32 index, s32 subindex, u8 *data);
s32 rtDecodeEntityBlockHeader_Leaf1(u8 *data, s32 *indexSizePtr, s32 *indexPtr, s32 *subindexPtr);
s32 rtDecodeIndexedBlockHeader_1(u8 *data, s32 *indexSizePtr, s32 *indexPtr, s32 *subindexPtr);

s32 rtGetIndexedBlockHeaderSize_1(s32 indexSize);

s32 rtEncodeEntityBlockHeader_Leaf2(s32 fwdRev, s32 indexSize, s32 index, s32 subindex, u8 *data);
s32 rtDecodeEntityBlockHeader_Leaf2(u8 *data, s32 *indexSizePtr, s32 *indexPtr, s32 *subindexPtr);
s32 rtGetIndexedBlockHeaderSize_2(s32 indexSize);

s32 rtDecodeIndexedBlockHeader(u8 *data, s32 *topindexPtr, s32 *indexSizePtr, s32 *indexPtr, s32 *subindexSizePtr, s32 *subindexPtr);
s32 rtDecodeIndexedBlockHeaderSize(u8 *data);

MemCircHeapChunkIndex *rtReclaimIndexer(u8 *data, s64 targetAmount, u8 tag, u8 **tagData, s32 tagDataLength, s32 tagSearchOffset, MemDispenser *disp);
void rtRelocater(MemCircHeapChunkIndex *index, u8 tag, u8 **tagData, s32 tagDataLength);

void initHeapDataBlock(HeapDataBlock *block, s32 sliceIndex);




#endif
