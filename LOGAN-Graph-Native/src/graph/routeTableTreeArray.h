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
	u8 *proxy; // Temporary space for new stuff, via proxy
} RouteTableTreeArrayEntry;

struct routeTableTreeArrayProxyStr
{
	RouteTableTreeArrayBlock *ptrBlock; // If using two levels
	u16 ptrAlloc;
	u16 ptrCount;

	RouteTableTreeArrayBlock *dataBlock; // After Headers.
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
};




void rttBindBlockArrayProxy(RouteTableTreeArrayProxy *arrayProxy, u8 *heapDataPtr, u32 headerSize);
void initBlockArrayProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeArrayProxy *arrayProxy, HeapDataBlock *heapDataBlock, u8 *heapDataPtr);

s32 getBlockArraySize(RouteTableTreeArrayProxy *arrayProxy);

void *getBlockArrayNewEntryProxy(RouteTableTreeArrayProxy *arrayProxy, s32 index);
u8 *getBlockArrayExistingEntryData(RouteTableTreeArrayProxy *arrayProxy, s32 index);

void ensureBlockArrayWritable(RouteTableTreeArrayProxy *arrayProxy, MemDispenser *disp);

s32 appendBlockArrayEntryProxy(RouteTableTreeArrayProxy *arrayProxy, void *proxy, MemDispenser *disp);
void setBlockArrayEntryProxy(RouteTableTreeArrayProxy *arrayProxy, s32 index, void *proxy, MemDispenser *disp);

s32 rttGetArrayDirty(RouteTableTreeArrayProxy *arrayProxy);

s32 rttCalcFirstLevelArrayEntries(s32 entries);
s32 rttCalcFirstLevelArrayCompleteEntries(s32 entries);
s32 rttCalcFirstLevelArrayAdditionalEntries(s32 entries);

s32 rttCalcArraySize(s32 entries);
s32 rttGetArrayEntries(RouteTableTreeArrayProxy *arrayProxy);

#endif
