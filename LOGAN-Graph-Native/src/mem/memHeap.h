#ifndef __MEM_HEAP_H
#define __MEM_HEAP_H


typedef struct memHeapStr
{
	MemFixedHeap fixed;
	MemCircHeap circ;
} MemHeap;



MemHeap *mhHeapAlloc(MemCircHeapChunkIndex *(*reclaimIndexer)(u8 *heapDataPtr, s64 targetAmount, u8 tag, u8 **tagData, s32 tagDataLength, s32 tagSearchOffset, MemDispenser *disp),
		void (*relocater)(MemCircHeapChunkIndex *reclaimIndex, u8 tag, u8 **tagData, s32 tagDataLength));
void mhHeapFree(MemHeap *heap);

void mhHeapRegisterTagData(MemHeap *heap, u8 tag, u8 **data, s32 dataLength);

void *mhAlloc(MemHeap *heap, size_t size, u8 tag, s32 newTagOffset, s32 *oldTagOffset);
void mhFree(MemHeap *heap, void *oldData, size_t size);

#endif
