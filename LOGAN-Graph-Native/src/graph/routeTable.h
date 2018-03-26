#ifndef __ROUTE_TABLE_H
#define __ROUTE_TABLE_H

#define ALLOC_HEADER_LIVE_MASK 0x80


#define LINK_INDEX_DUMMY 0x0


#define SEQUENCE_LINK_BYTES 56
#define SEQUENCE_LINK_BASES (SEQUENCE_LINK_BYTES*4)

#define SEQUENCE_LINK_MAX_TAG_LENGTH 255

#define LOOKUP_LINK_SMERS 15

typedef struct sequenceLinkStr {
	u32 nextIndex;			// Index of next SequenceLink in chain (or LINK_DUMMY at end)
	u8 position;			// Position of next base for lookup (in bp, from start of this brick)
	u8 seqLength;			// Length of packed sequence (in bp)
	u8 tagLength;			// Length of additional tag data (in bytes)
	u8 pad;
	u8 packedSequence[SEQUENCE_LINK_BYTES];	// Packed sequence (2bits per bp) - up to 224 bases2
} SequenceLink;

#define LINK_INDEXTYPE_SEQ 0
#define LINK_INDEXTYPE_LOOKUP 1
#define LINK_INDEXTYPE_DISPATCH 2

#define LOOKUPLINK_EOS_FLAG 0x8000

typedef struct lookupLinkStr {
	u32 sourceIndex;		// Index of SeqLink or Dispatch
	u8 indexType;			// Indicates meaning of previous (SeqLink or Dispatch)
	s8 smerCount;			// Number of smers to lookup (negative means in progress, positive means complete)
	u16 revComp;			// Indicates if the original smer was rc (lsb = first smer). Top bit indicates if last lookup is end-of-sequence
	SmerId smers[LOOKUP_LINK_SMERS];		// Specific smers to lookup
} LookupLink;


#define DISPATCH_LINK_SMERS 7
#define DISPATCH_LINK_SMER_THRESHOLD 5

#define DISPATCHLINK_EOS_FLAG 0x80

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
	DispatchLink *rdiPtr;
	u8 *tagData;
	u32 dispatchLinkIndex;

	s32 nodeOffset;
	s32 prefixIndex;
	s32 suffixIndex;
} RoutePatch;

// Need new more flexible dispatchLink queue options here
// RoutingSmerAssignedDispatchLink - indexes of all the dispatchLinks queued for a single smer
// RoutingSliceAssignedDispatchLinkQueue - all dispatchLinks queued for a single SliceGroup

#define DISPATCH_LINK_QUEUE_DEFAULT_BOOST 2
#define DISPATCH_LINK_QUEUE_FORCE_THRESHOLD 100

typedef struct routingSmerAssignedDispatchLinkQueueStr {
	s32 sliceIndex; // 4
	u32 entryCount; // 4
	u32 position; // 4
	u32 boost; // 4
	u32 *dispatchLinkIndexEntries; // 8
} RoutingSmerAssignedDispatchLinkQueue;

typedef struct routingSliceAssignedDispatchLinkQueueStr {

	IohHash *smerQueueMap[SMER_DISPATCH_GROUP_SLICES]; // Map of sliceIndex -> RoutingSmerAssignedDispatchLink
} RoutingSliceAssignedDispatchLinkQueue;


typedef struct routingIndexedDispatchLinkIndexBlockStr {
	MemSingleBrickPile *sequenceLinkPile;
	MemDoubleBrickPile *lookupLinkPile;
	MemDoubleBrickPile *dispatchLinkPile;

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

typedef struct routeTableTagBuilderStr RouteTableTagBuilder;


typedef struct routingComboBuilderStr
{
	MemDispenser *disp;
	u8 **rootPtr;
	s32 sliceIndex;
	u8 sliceTag;

	SeqTailBuilder prefixBuilder;
	SeqTailBuilder suffixBuilder;

	RouteTableArrayBuilder *arrayBuilder;
	RouteTableTagBuilder *tagBuilder;

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
