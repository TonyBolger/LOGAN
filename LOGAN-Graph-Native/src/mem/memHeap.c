#include "common.h"


MemHeap *mhHeapAlloc(MemCircHeapChunkIndex *(*reclaimIndexer)(u8 *heapDataPtr, s64 targetAmount, u8 tag, u8 **tagData, s32 tagDataLength, s32 tagSearchOffset, MemDispenser *disp),
		void (*relocater)(MemCircHeapChunkIndex *reclaimIndex, u8 tag, u8 **tagData, s32 tagDataLength))
{
	MemHeap *heap=G_ALLOC_ALIGNED(sizeof(MemHeap), CACHE_ALIGNMENT_SIZE, MEMTRACKID_HEAP);

	mhfHeapAlloc(&(heap->fixed));
	mhcHeapAlloc(&(heap->circ), reclaimIndexer, relocater);

	return heap;
}

void mhHeapFree(MemHeap *heap)
{
	mhfHeapFree(&(heap->fixed));
	mhcHeapFree(&(heap->circ));

	G_FREE(heap, sizeof(MemHeap), MEMTRACKID_HEAP);

}

void mhHeapRegisterTagData(MemHeap *heap, u8 tag, u8 **data, s32 dataLength)
{
	mhcHeapRegisterTagData(&(heap->circ), tag, data, dataLength);
}

void *mhAlloc(MemHeap *heap, size_t size, u8 tag, s32 newTagOffset, s32 *oldTagOffset)
{
	//LOG(LOG_INFO,"mhAlloc %i", size);
	if(size==0)
		return NULL;

	return mhcAlloc(&(heap->circ), size, tag, newTagOffset, oldTagOffset);
}

void mhFree(MemHeap *heap, void *oldData, size_t size)
{
	if(oldData!=NULL)
		{
		//LOG(LOG_INFO, "mhFree: %i at %p", size, oldData);
		u8 *data=oldData;
		*data&=~ALLOC_HEADER_LIVE_MASK;
		}
}
