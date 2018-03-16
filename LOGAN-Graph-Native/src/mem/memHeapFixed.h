#ifndef __MEM_HEAP_FIXED_H
#define __MEM_HEAP_FIXED_H

#define MEMFIXED_FLAGS_PER_BLOCK 8
#define MEMFIXED_DATABYTES_PER_BLOCK 4016

typedef struct memFixedHeapSizeBlockStr
{
	struct memFixedHeapSizeBlockStr *nextPtr;
	u16 itemSize;
	u16 totalCount;
	u16 freeCount;
	u16 pad1;

	u64 freeFlags[MEMFIXED_FLAGS_PER_BLOCK];
	u8 data[MEMFIXED_DATABYTES_PER_BLOCK];
} MemFixedHeapSizeBlock;

typedef struct memFixedHeapSizerStr
{
	MemFixedHeapSizeBlock *current;
	MemFixedHeapSizeBlock *full;
	u32 recycleCount;
	u32 padding;
} MemFixedHeapSizer;

//#define MEMFIXED_FREE_MIN 2

#define MEMFIXED_MIN_SIZE 8
#define MEMFIXED_MAX_SIZE 127
#define MEMFIXED_SIZES (MEMFIXED_MAX_SIZE-MEMFIXED_MIN_SIZE+1)

#define MEMFIXED_RECYCLE_THRESHOLD 4

typedef struct memFixedHeapStr
{
	MemFixedHeapSizer sizer[MEMFIXED_SIZES];

} MemFixedHeap;



void mhfHeapAlloc(MemFixedHeap *fixedHeap);
void mhfHeapFree(MemFixedHeap *fixedHeap);

void *mhfAlloc(MemFixedHeap *fixedHeap, size_t size);
void mhfFree(MemFixedHeap *fixedHeap, void *oldData, size_t size);

#endif
