#ifndef __ROUTE_TABLE_TREE_ARRAY_H
#define __ROUTE_TABLE_TREE_ARRAY_H



typedef struct routeTableTreeArrayBlockStr
{
	u16 dataAlloc;
	u8 *data[];
} __attribute__((packed)) RouteTableTreeArrayBlock;


// Possible Array Formats:
// 		Single Level: Shallow Ptr -> Shallow Data (1B sub) [1024]
//      Two Level: Shallow Ptr-> Deep Ptr (1B sub) [256] -> Deep Data (2B sub) [1024]

struct routeTableTreeArrayProxyStr
{
	RouteTableTreeArrayBlock *ptrBlock; // If using two levels
	u16 ptrAlloc;
	u16 ptrCount;

	RouteTableTreeArrayBlock *dataBlock; // With Headers.
	u16 dataAlloc;
	u16 dataCount;

	u8 **newData; // Temporary space for new stuff (no headers)
	u16 newDataAlloc;
	u16 newDataCount;

	HeapDataBlock *heapDataBlock;
	u32 arrayType;
};




void rttBindBlockArrayProxy(RouteTableTreeArrayProxy *arrayProxy, u8 *heapDataPtr, u32 headerSize);
void initBlockArrayProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeArrayProxy *arrayProxy, HeapDataBlock *heapDataBlock, u8 *heapDataPtr, u32 arrayType);

s32 getBlockArraySize(RouteTableTreeArrayProxy *arrayProxy);
u8 *getBlockArrayEntry(RouteTableTreeArrayProxy *arrayProxy, s32 index);

void ensureBlockArrayWritable(RouteTableTreeArrayProxy *arrayProxy, MemDispenser *disp);

void setBlockArrayEntry(RouteTableTreeArrayProxy *arrayProxy, s32 index, u8 *data, MemDispenser *disp);
s32 appendBlockArrayEntry(RouteTableTreeArrayProxy *arrayProxy, u8 *data, MemDispenser *disp);

#endif
