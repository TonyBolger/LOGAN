#ifndef __ROUTE_TABLE_H
#define __ROUTE_TABLE_H



/*
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
*/




#define DISPATCH_LINK_SMERS 7
#define DISPATCH_LINK_SMER_THRESHOLD 5

typedef struct dispatchLinkSmerStr {
	SmerId smer;		// The actual smer ID
	u16 seqIndexOffset; // Offset from previous smer, or from start of first seqLink (if first)
	u16 slice;			// Ranges (0-16383)
	u32 sliceIndex;		// Index within slice
} DispatchLinkSmer;

typedef struct dispatchLinkStr {
	u32 nextOrSourceIndex;		// Index of Next Dispatch or Source SeqLink
	u8 indexType;				// Indicates meaning of previous
	u8 length;					// How many valid indexedSmers are there
	u8 position;				// The current indexed smer
	u8 revComp;					// Indicate if each original smer was rc (lsb = first smer)
	s32 minEdgePosition;
	s32 maxEdgePosition;
	DispatchLinkSmer smers[DISPATCH_LINK_SMERS];
} DispatchLink;




typedef struct routePatchStr
{
	DispatchLink **rdiPtr; // Needs double ptr to enable sorting by inbound position
	u32 dispatchLinkIndex;

	s32 prefixIndex;
	s32 suffixIndex;
} RoutePatch;

// Need new more flexible dispatchLink queue options here
// RoutingSmerAssignedDispatchLink - indexes of all the dispatchLinks queued for a single smer
// RoutingSliceAssignedDispatchLinkQueue - all dispatchLinks queued for a single SliceGroup

typedef struct routingSmerAssignedDispatchLinkQueueStr {
	s32 sliceIndex; // 4
	u32 entryCount; // 4
	u32 position; // 4
	u32 *dispatchLinkIndexEntries; // 8
} RoutingSmerAssignedDispatchLink;

typedef struct routingSliceAssignedDispatchLinkQueueStr {
	IohHash *smerQueueMap;

} RoutingSliceAssignedDispatchLinkQueue;


typedef struct routingIndexedDispatchLinkIndexBlockStr {
	s32 sliceIndex; // 4
	u32 entryCount; // 4
	u32 *linkIndexEntries; // 8
	DispatchLink **linkEntries; // 8
} __attribute__((aligned (16))) RoutingIndexedDispatchLinkIndexBlock;


typedef struct routingDispatchLinkIndexBlockStr {
	u64 entryCount; // 8
	u32 *entries; // 4
} __attribute__((aligned (16))) RoutingDispatchLinkIndexBlock;


typedef union s32floatUnion {
	s32 s32Val;
	float floatVal;
} s32Float;



#define ROUTING_TABLE_FORWARD 0
#define ROUTING_TABLE_REVERSE 1


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

void rtInitHeapDataBlock(HeapDataBlock *block, s32 sliceIndex);

//RoutePatch *rtCloneRoutePatches(MemDispenser *disp, RoutePatch *inPatches, s32 inPatchCount);


#endif
