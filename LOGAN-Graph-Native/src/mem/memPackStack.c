#include "common.h"


//
// Per-slice min size of 0x400      (1024) means 16MBs
// Per-slice max size of 0x40000000 (1,073,741,824) means 16TBs

#define PACKSTACK_SIZE_NUM 18
#define PACKSTACK_SIZE_MIN 1


u32 PACKSTACK_SIZES[]=
	{0x200,
	0x400,    0x800,    0x1000,    0x2000,
	0x4000,   0x8000,   0x10000,   0x20000,
	0x40000,  0x80000,  0x100000,  0x200000,
	0x400000, 0x800000, 0x1000000, 0x2000000,
	0x4000000};



static MemPackStack *packStackAllocWithSize(int suggestedSizeIndex, int minSize)
{
	int targetSize=minSize+sizeof(MemPackStack);

	if(suggestedSizeIndex<PACKSTACK_SIZE_NUM)
		{
		int size=PACKSTACK_SIZES[suggestedSizeIndex];

		while(suggestedSizeIndex<PACKSTACK_SIZE_NUM && size<targetSize)
			size=PACKSTACK_SIZES[++suggestedSizeIndex];
		}

	if(suggestedSizeIndex>PACKSTACK_SIZE_NUM)
		{
		LOG(LOG_CRITICAL, "Attempt to allocate PackStack beyond maximum size %i (%i vs %i)",
				PACKSTACK_SIZES[PACKSTACK_SIZE_NUM-1],PACKSTACK_SIZE_NUM-1,suggestedSizeIndex);
		}

	int totalSize=PACKSTACK_SIZES[suggestedSizeIndex];

	MemPackStack *packStack=memalign(CACHE_ALIGNMENT_SIZE, totalSize);

	if(packStack==NULL)
		LOG(LOG_CRITICAL,"Failed to alloc packstack");

	packStack->currentSizeIndex=suggestedSizeIndex;
	packStack->currentSize=totalSize-sizeof(MemPackStack);

	packStack->peakAlloc=0;
	packStack->realloc=0;


	return packStack;
}


MemPackStack *packStackAlloc()
{
	return packStackAllocWithSize(PACKSTACK_SIZE_MIN, 0);

}

void packStackFree(MemPackStack *packStack)
{

}

void *psAlloc(MemPackStack *packStack, size_t size)
{
	return NULL;
}

void *psRealloc(MemPackStack *packStack, void *oldPtr, size_t oldSize, size_t size)
{
	return NULL;
}

MemPackStack *psCompact(MemPackStack *packStack, MemPackStackCompact *compacts, s32 compactCount)
{
	return NULL;
}


