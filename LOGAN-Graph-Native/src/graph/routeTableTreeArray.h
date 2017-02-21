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
	RouteTableTreeArrayBlock *ptrBlock; // If using two levels (After Headers in first level)
	u16 ptrAlloc; // First level allocation (currently equal ptrCount)
	u16 ptrCount; // First level used

	RouteTableTreeArrayBlock *dataBlock; // If using only one level (After Headers)

	u16 oldDataAlloc; // Direct, or combined indirect, allocation (can round up)
	u16 oldDataCount; // Direct, or combined indirect, used

	RouteTableTreeArrayEntry *newEntries; // Sorted array of indexed new entries
	u16 newEntriesAlloc;
	u16 newEntriesCount;

	u16 newDataAlloc; // Number of direct, or combined indirect entries allocated, including new
	u16 newDataCount; // Number of direct, or combined indirect entries used, including new

	HeapDataBlock *heapDataBlock;
};



s32 getRouteTableTreeArraySize_Existing(RouteTableTreeArrayBlock *arrayBlock);

void rttBindBlockArrayProxy(RouteTableTreeArrayProxy *arrayProxy, u8 *heapDataPtr, u32 headerSize, u32 isIndirect);
void initBlockArrayProxy(RouteTableTreeProxy *treeProxy, RouteTableTreeArrayProxy *arrayProxy, HeapDataBlock *heapDataBlock, u8 *heapDataPtr, u32 isIndirect);

void dumpBlockArrayProxy(RouteTableTreeArrayProxy *arrayProxy);

//s32 getBlockArraySize(RouteTableTreeArrayProxy *arrayProxy);
u8 *getBlockArrayDataEntryRaw(RouteTableTreeArrayProxy *arrayProxy, s32 index);
void setBlockArrayDataEntryRaw(RouteTableTreeArrayProxy *arrayProxy, s32 index, u8 *data);

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
