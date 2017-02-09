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

typedef struct routeTableTreeArrayEntriesStr
{
	u16 index;
	u8 *data; // Temporary space for new stuff (no heap headers)
} RouteTableTreeArrayEntry;

struct routeTableTreeArrayProxyStr
{
	RouteTableTreeArrayBlock *ptrBlock; // If using two levels
	u16 ptrAlloc;
	u16 ptrCount;

	RouteTableTreeArrayBlock *dataBlock; // With Headers.
	u16 dataAlloc;
	u16 dataCount;

	RouteTableTreeArrayEntry *newEntries; // Sorted array of indexed new entries
	u16 newEntriesAlloc;
	u16 newEntriesCount;

	//u8 **newData;
	u16 newPtrAlloc; // Number of future indirect entries allocated
	u16 newPtrCount; // Number of future indirect entries

	u16 newDataAlloc; // Number of future direct entries allocated
	u16 newDataCount; // Number of future direct entries used

	HeapDataBlock *heapDataBlock;
	u32 arrayType;
};




void rttBindBlockArrayProxy(RouteTableTreeArrayProxy *arrayProxy, u8 *heapDataPtr, u32 headerSize);
void initBlockArrayProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeArrayProxy *arrayProxy, HeapDataBlock *heapDataBlock, u8 *heapDataPtr, u32 arrayType);

s32 getBlockArraySize(RouteTableTreeArrayProxy *arrayProxy);
u8 *getBlockArrayEntry(RouteTableTreeArrayProxy *arrayProxy, s32 index);

void ensureBlockArrayWritable(RouteTableTreeArrayProxy *arrayProxy, MemDispenser *disp);

s32 appendBlockArrayEntry(RouteTableTreeArrayProxy *arrayProxy, u8 *data, MemDispenser *disp);
void setBlockArrayEntry(RouteTableTreeArrayProxy *arrayProxy, s32 index, u8 *data, MemDispenser *disp);

#endif
