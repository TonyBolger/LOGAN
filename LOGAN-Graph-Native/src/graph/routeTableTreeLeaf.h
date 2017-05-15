#ifndef __ROUTE_TABLE_TREE_LEAF_H
#define __ROUTE_TABLE_TREE_LEAF_H


typedef s32 RouteTableTreeLeafOffset;

typedef struct routeTableTreeLeafEntryStr
{
	s16 downstream;
	s32 width;
} __attribute__((packed)) RouteTableTreeLeafEntry;

typedef struct routeTableTreeLeafEntryBlockStr
{
	s16 upstream;

	u16 entryAlloc;
	u16 entryCount;
	RouteTableTreeLeafEntry entries[];
} __attribute__((packed)) RouteTableTreeLeafEntryBlock;

typedef struct routeTableLeafBlockStr
{
	u8 leafHeader; // Indicate width of totalSize, upstream Counts, downstream Counts, offsets and entry widths (x 4 bits)
	u16 dataSize; // (16 bit fixed for now)

	s16 parentBrindex;

	u16 upstreamFirst; // (16 bit fixed for now)
	u16 upstreamAlloc; // (16 bit fixed for now)
	u16 downstreamFirst; // (16 bit fixed for now)
	u16 downstreamAlloc; // (16 bit fixed for now)

	u8 data[];	//s32 upstream offsets, downstream offsets, packed entries(entry count, (downstream, width));


	/*
	s16 offsetAlloc;
	s16 entryAlloc;

	s16 parentBrindex;
	s16 upstream;
	s32 upstreamOffset;

	u8 extraData[];
	*/

	//RouteTableTreeLeafOffset offsets[offsetAlloc]
	//RouteTableTreeLeafEntry entries[entryAlloc]; // Max is ROUTE_TABLE_TREE_LEAF_ENTRIES

} __attribute__((packed)) RouteTableTreeLeafBlock;


struct routeTableTreeLeafProxyStr
{
	RouteTableTreeLeafBlock *dataBlock;
	s16 lindex;

	u16 upstreamAlloc;
	s32 *upstreamOffsets;

	u16 downstreamAlloc;
	s32 *downstreamOffsets;

	u16 blockAlloc;
	u16 blockCount;
	RouteTableTreeLeafEntryBlock *blocks;
};


s32 getRouteTableTreeLeafSize_Expected(s16 offsetAlloc, s16 entryAlloc);
s32 getRouteTableTreeLeafSize_Existing(RouteTableTreeLeafBlock *leafBlock);
RouteTableTreeLeafOffset *getRouteTableTreeLeaf_OffsetPtr(RouteTableTreeLeafBlock *leafBlock);
RouteTableTreeLeafEntry *getRouteTableTreeLeaf_EntryPtr(RouteTableTreeLeafBlock *leafBlock);

void getRouteTableTreeLeafProxy_scan(RouteTableTreeLeafBlock *leafBlock, u16 *entryAllocPtr, u16 *entryCountPtr);

void flushRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leafProxy);
RouteTableTreeLeafProxy *allocRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, s32 offsetAlloc, s32 entryAlloc);

void ensureRouteTableTreeLeafOffsetCapacity(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leafProxy, s32 offsetAlloc);

void expandRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leafProxy, s32 offsetAlloc);

//void dumpLeafProxy(RouteTableTreeLeafProxy *leafProxy);
void dumpLeafBlock(RouteTableTreeLeafBlock *leafBlock);

void leafMakeEntryInsertSpace(RouteTableTreeLeafProxy *leaf, s16 entryPosition, s16 entryCount);

void initRouteTableTreeLeafOffsets(RouteTableTreeLeafProxy *leafProxy, s16 upstream);
void recalcRouteTableTreeLeafOffsets(RouteTableTreeLeafProxy *leafProxy);
void validateRouteTableTreeLeafOffsets(RouteTableTreeLeafProxy *leafProxy);

#endif
