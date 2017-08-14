#ifndef __ROUTE_TABLE_PACKING_H
#define __ROUTE_TABLE_PACKING_H


// Represents packed form of a leaf or whole routing table.
typedef struct routeTablePackedSingleBlockStr
{
	//SPARE 					2

	//packedSize(8/16 bit)     1
	//upstreamRange(8/16 bit)  1
	//downstreamRange(8/16 bit) 1
	//offsetSize(8/16/24/32 bit) 2
	//arrayCountSize(8/16 bit) 1

	//entryCountSize(4-32 bit) 3
	//widthSize(1-32 bit) 5

	u16 blockHeader; // Indicate size for packedSize, upstream Counts, downstream Counts, offsets, entry count and entry widths
	u8 data[];	//packedSize, s32 upstream offsets, downstream offsets, packed entries(entry count, (downstream, width));

//	packedSize dataSize;

//  Range of upstream, used to avoid many zero entries in offset array by head/tail trimming. Size determined by max upstream
//	upstreamRange upstreamFirst;
//	upstreamRange upstreamAlloc;

//  Range of downstream, used to avoid many zero entries in offset array by head/tail trimming. Size determined by max downstream
//	downstreamRange downstreamFirst;
//	downstreamRange downstreamAlloc;

// 	Offset Indexes: Summarises the total width of routes for each upstream/downstream within the ranges. Size determined by max offset
//  offsetSize upstreamOffsets[upstreamAlloc]
//  offsetSize downstreamOffsets[downstreamAlloc]

//  Route entries array, each entry uses a common upstream and an array of downstream/width pairs. Sizes determined by array count, max array entries and max width
//  arrayCountSize: arrayCount;
//  EntryArray: upstream, entryCountSize: entryCount, (downstream, width)

} __attribute__((packed)) RouteTablePackedSingleBlock;


// Represents the

typedef struct routeTablePackingInfoStr
{
	s32 oldPackedSize;

	s32 packedSize;

	s32 packedUpstreamOffsetFirst;
	s32 packedUpstreamOffsetLast;
	s32 packedUpstreamOffsetAlloc;

	s32 packedDownstreamOffsetFirst;
	s32 packedDownstreamOffsetLast;
	s32 packedDownstreamOffsetAlloc;

	s32 maxOffset;

	s32 arrayCount;
	s32 entryCount;

	s32 maxEntryCount;
	s32 maxEntryWidth;

	u16 blockHeader;
} RouteTablePackingInfo;



// Represents a single entry (downstream, width) in an array (which defines the upstream)
typedef struct routeTableUnpackedEntryStr
{
	s32 downstream;
	s32 width;
} __attribute__((packed)) RouteTableUnpackedEntry;

// Represents a list of entries (downstream, width) with a common upstream
typedef struct routeTableUnpackedEntryArrayStr
{
	s32 upstream;

	u32 entryAlloc;
	u32 entryCount;
	RouteTableUnpackedEntry entries[];
} __attribute__((packed)) RouteTableUnpackedEntryArray;

// Represents the unpacked form of leaf or whole routing table
typedef struct routeTableUnpackedSingleBlockStr
{
	MemDispenser *disp;

	RouteTablePackingInfo packingInfo;

	s32 upstreamOffsetAlloc;
	s32 downstreamOffsetAlloc;
	s32 entryArrayAlloc;
	s32 entryArrayCount;

	s32 *upstreamOffsets;
	s32 *downstreamOffsets;
	RouteTableUnpackedEntryArray **entryArrays;

} RouteTableUnpackedSingleBlock;

#define ROUTEPACKING_ENTRYARRAYS_CHUNK 4
//#define ROUTEPACKING_ENTRYARRAYS_MAX 16
#define ROUTEPACKING_ENTRYARRAYS_MAX 64

#define ROUTEPACKING_ENTRYS_CHUNK 8
//#define ROUTEPACKING_ENTRYS_MAX 1024
#define ROUTEPACKING_ENTRYS_MAX 16


void rtpDumpUnpackedSingleBlock(RouteTableUnpackedSingleBlock *block);

void rtpRecalculateUnpackedBlockOffsets(RouteTableUnpackedSingleBlock *unpackedBlock);

// Insert new entry (downstream,width) into specified position in specified entryArray
RouteTableUnpackedEntryArray *rtpInsertNewEntry(RouteTableUnpackedSingleBlock *unpackedBlock, s32 arrayIndex, s32 entryIndex, s32 downstream, s32 width);

// Insert two new entries (downstream1,width1),(downstream2,width2) into specified position in specified entryArray
RouteTableUnpackedEntryArray *rtpInsertNewDoubleEntry(RouteTableUnpackedSingleBlock *unpackedBlock, s32 arrayIndex, s32 entryIndex, s32 downstream1, s32 width1, s32 downstream2, s32 width2);

// Insert new array (upstream) into a specified position in the block
RouteTableUnpackedEntryArray *rtpInsertNewEntryArray(RouteTableUnpackedSingleBlock *unpackedBlock, s32 arrayIndex, s32 upstream, s32 entryAlloc);

// Split an existing array
RouteTableUnpackedEntryArray *rtpSplitArray(RouteTableUnpackedSingleBlock *block, s16 arrayIndex, s16 entryIndex, s16 *updatedArrayIndexPtr, s16 *updatedEntryIndexPtr);

// Allocate a new (empty) unpackedBlock
RouteTableUnpackedSingleBlock *rtpAllocUnpackedSingleBlock(MemDispenser *disp, s32 upstreamOffsetAlloc, s32 downstreamOffsetAlloc, s32 entryArrayAlloc);

RouteTableUnpackedSingleBlock *rtpUnpackSingleBlock(RouteTablePackedSingleBlock *packedBlock, MemDispenser *disp);

void rtpUpdateUnpackedSingleBlockPackingInfo(RouteTableUnpackedSingleBlock *unpackedBlock);

void rtpPackSingleBlock(RouteTableUnpackedSingleBlock *unpackedBlock, RouteTablePackedSingleBlock *packedBlock);

s32 rtpGetPackedSingleBlockSize(RouteTablePackedSingleBlock *packedBlock);

#endif

