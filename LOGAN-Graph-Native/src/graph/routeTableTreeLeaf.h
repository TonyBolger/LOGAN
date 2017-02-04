#ifndef __ROUTE_TABLE_TREE_LEAF_H
#define __ROUTE_TABLE_TREE_LEAF_H


typedef s32 RouteTableTreeLeafOffset;

typedef struct routeTableTreeLeafEntryStr
{
	s16 downstream;
	s32 width;
} __attribute__((packed)) RouteTableTreeLeafEntry;

typedef struct routeTableLeafBlockStr
{
	s16 offsetAlloc;
	s16 entryAlloc;

	s16 parentBrindex;
	s16 upstream;
	s32 upstreamOffset;

	u8 extraData[];

	//RouteTableTreeLeafOffset offsets[offsetAlloc]
	//RouteTableTreeLeafEntry entries[entryAlloc]; // Max is ROUTE_TABLE_TREE_LEAF_ENTRIES

} __attribute__((packed)) RouteTableTreeLeafBlock;


struct routeTableTreeLeafProxyStr
{
	RouteTableTreeLeafBlock *dataBlock;
	s16 lindex;

	u16 entryAlloc;
	u16 entryCount;

};


s32 getRouteTableTreeLeafSize_Expected(s16 offsetAlloc, s16 entryAlloc);
s32 getRouteTableTreeLeafSize_Existing(RouteTableTreeLeafBlock *leafBlock);
RouteTableTreeLeafOffset *getRouteTableTreeLeaf_OffsetPtr(RouteTableTreeLeafBlock *leafBlock);
RouteTableTreeLeafEntry *getRouteTableTreeLeaf_EntryPtr(RouteTableTreeLeafBlock *leafBlock);

RouteTableTreeLeafBlock *getRouteTableTreeLeafRaw(RouteTableTreeProxy *treeProxy, s32 lindex);
RouteTableTreeLeafProxy *getRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, s32 lindex);

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
