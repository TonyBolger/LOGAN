#ifndef __ROUTE_TABLE_PACKING_H
#define __ROUTE_TABLE_PACKING_H


typedef struct routeTableUnpackedEntryStr
{
	s32 downstream;
	s32 width;
} __attribute__((packed)) RouteTableUnpackedEntry;

typedef struct routeTableUnpackedEntryArrayStr
{
	s32 upstream;

	u32 entryAlloc;
	u32 entryCount;
	RouteTableUnpackedEntry entries[];
} __attribute__((packed)) RouteTableUnpackedEntryArray;

typedef struct routeTableUnpackedSingleBlockStr
{
	MemDispenser *disp;

	s32 packedDataSize;
	s32 packedUpstreamOffsetFirst;
	s32 packedUpstreamOffsetAlloc;
	s32 packedDownstreamOffsetFirst;
	s32 packedDownstreamOffsetAlloc;

	s32 upstreamOffsetAlloc;
	s32 downstreamOffsetAlloc;
	s32 entryArrayAlloc;

	s32 *upstreamOffsets;
	s32 *downstreamOffsets;
	RouteTableUnpackedEntryArray *entryArrays;

} RouteTableUnpackedSingleBlock;



// Represents part of a single routing table. Used in RouteTableTreeLeaf
typedef struct routeTablePackedSingleBlockStr
{
	//packedSize(8/16 bit)     1
	//upstreamRange(8/16 bit)  1
	//downstreamRange(8/16 bit) 1
	//offsetSize(8/16/24/32 bit) 2
	//entryCountSize(4-32 bit) 3
	//widthSize(1-32 bit) 5

	u16 blockHeader; // Indicate size for packedSize, upstream Counts, downstream Counts, offsets, entry count and entry widths
	u8 data[];	//packedSize, s32 upstream offsets, downstream offsets, packed entries(entry count, (downstream, width));

//	packedSize dataSize;
//	upstreamRange upstreamFirst;
//	upstreamRange upstreamAlloc;
//	downstreamRange downstreamFirst;
//	downstreamRange downstreamAlloc;
//  offsetSize upstreamOffsets[upstreamAlloc]
//  offsetSize downstreamOffsets[downstreamAlloc]
//  EntryArray: upstream, entryCount (downstream, width)

} __attribute__((packed)) RouteTablePackedSingleBlock;


RouteTableUnpackedEntryArray *rtpInsertNewEntry(RouteTableUnpackedSingleBlock *unpackedBlock, s32 arrayIndex, s32 entryIndex, s32 downstream, s32 width);
void rtpInsertNewEntryArray(RouteTableUnpackedSingleBlock *unpackedBlock, s32 arrayIndex, s32 upstream, s32 entryAlloc);

RouteTableUnpackedSingleBlock *rtpUnpackSingleBlock(RouteTablePackedSingleBlock *packedBlock, MemDispenser *disp);
void rtpUpdateUnpackedSingleBlockSize(RouteTableUnpackedSingleBlock *unpackedBlock);

void rtpPackSingleBlock(RouteTableUnpackedSingleBlock *unpackedBlock, RouteTablePackedSingleBlock *packedBlock);

s32 rtpGetPackedSingleBlockSize(RouteTablePackedSingleBlock *packedBlock);

#endif

