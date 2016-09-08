#include "common.h"


// Short term: Allocation per slice (#16384)
// Per-slice min size of 0x400      (1,024) means 16MBs total
// Per-slice max size of 0x40000000 (1,073,741,824) means 16TBs total

// Long term: Allocation per block (Min #64, 256 slices each, up to Max #256, 64 slices each)

// Using 64 allocator blocks of 256 slices
// Per-block min size of 0x40000      (262,144) means 16MBs total
// Per-block max size of 0x4000000000 (274,877,906,944) means 16TBs total

// Using 128 allocator blocks of 128 slices
// Per-allocator min size of 0x20000      (131,072) means 16MBs total
// Per-allocator max size of 0x2000000000 (137,438,953,472) means 16TBs total

// Using 256 allocator blocks of 64 slices
// Per-allocator min size of 0x10000      (65,536) means 16MBs total
// Per-allocator max size of 0x1000000000 (68,719,476,736) means 16TBs total


#define PACKSTACK_CONFIG_NUM 21
#define PACKSTACK_CONFIG_MIN 0

MemPackStackConfig PACKSTACK_CONFIGS[]=
{
		{0x000020000L,{2,0,0,0}}, // 2*128K
		{0x000020000L,{2,2,0,0}}, // 4*128K
		{0x000040000L,{2,2,0,0}}, // 4*256K
		{0x000080000L,{2,2,0,0}}, // 4*512K
		{0x000080000L,{4,2,2,0}}, // 8*512K

		{0x000100000L,{4,2,2,0}}, // 8*1M
		{0x000100000L,{4,4,4,4}}, // 16*1M
		{0x000200000L,{4,4,4,4}}, // 16*2M
		{0x000400000L,{4,4,4,4}}, // 16*4M
		{0x000800000L,{4,4,4,4}}, // 16*8M

		{0x001000000L,{4,4,4,4}}, // 16*16M
		{0x002000000L,{4,4,4,4}}, // 16*32M
		{0x004000000L,{4,4,4,4}}, // 16*64M
		{0x008000000L,{4,4,4,4}}, // 16*128M
		{0x010000000L,{4,4,4,4}}, // 16*256M

		{0x020000000L,{4,4,4,4}}, // 16*512M
		{0x040000000L,{4,4,4,4}}, // 16*1G
		{0x080000000L,{4,4,4,4}}, // 16*2G
		{0x100000000L,{4,4,4,4}}, // 16*4G
		{0x200000000L,{4,4,4,4}}, // 16*8G

		{0x400000000L,{4,4,4,4}}  // 16*16G
};




#define PACKSTACK_SIZE_NUM 64
#define PACKSTACK_SIZE_MIN 0

// 0x400000000

// 16: 0x400 -> 0x4000
// 16: 0x4000 -> 0x40000
// 16: 0x40000 -> 0x400000
// 16: 0x400000 -> 0x0400000

u32 PACKSTACK_SIZES[]=
	{
	0x400,     0x500,     0x600,     0x700,
	0x800,     0xA00,     0xC00,     0xE00,
	0x1000,    0x1400,    0x1800,    0x1C00,
	0x2000,    0x2800,    0x3000,    0x3800,
	0x4000,    0x5000,    0x6000,    0x7000,
	0x8000,    0xA000,    0xC000,    0xE000,
	0x10000,   0x14000,   0x18000,   0x1C000,
	0x20000,   0x28000,   0x30000,   0x38000,
	0x40000,   0x50000,   0x60000,   0x70000,
	0x80000,   0xA0000,   0xC0000,   0xE0000,
	0x100000,  0x140000,  0x180000,  0x1C0000,
	0x200000,  0x280000,  0x300000,  0x380000,
	0x400000,  0x500000,  0x600000,  0x700000,
	0x800000,  0xA00000,  0xC00000,  0xE00000,
	0x1000000, 0x1400000, 0x1800000, 0x1C00000,
	0x2000000, 0x2800000, 0x3000000, 0x3800000,
	0x4000000};


static int getSizeIndex(int suggestedSizeIndex, int minSize)
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

	return suggestedSizeIndex;
}

static MemPackStack *packStackAllocWithSize(int suggestedSizeIndex, int minSize)
{
	suggestedSizeIndex=getSizeIndex(suggestedSizeIndex, minSize);

	int totalSize=PACKSTACK_SIZES[suggestedSizeIndex];

	MemPackStack *packStack=NULL;

	if((posix_memalign((void **)&packStack,CACHE_ALIGNMENT_SIZE, totalSize)!=0))
		LOG(LOG_CRITICAL,"Failed to alloc MemPackStack");

	//packStack=memalign(CACHE_ALIGNMENT_SIZE, totalSize);

	if(packStack==NULL)
		LOG(LOG_CRITICAL,"Failed to alloc MemPackStack");

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
	int size=packStack->currentSize;

	memset(packStack,0,size);
	free(packStack);
}

void *psAlloc(MemPackStack *packStack, size_t size)
{
	void *newPtr=(packStack->data+packStack->peakAlloc);
	packStack->peakAlloc=packStack->peakAlloc+size;

	if(packStack->peakAlloc>packStack->currentSize)
		return NULL;

	return newPtr;
}

void *psRealloc(MemPackStack *packStack, void *oldPtr, size_t oldSize, size_t size)
{
	void *newPtr=psAlloc(packStack,size); // May Fail if (near)-full

	if(newPtr!=NULL)
		packStack->realloc+=oldSize;

	return newPtr;
}

static int retainSorter(const void *a, const void *b)
{
	MemPackStackRetain *ra=(MemPackStackRetain *)a;
	MemPackStackRetain *rb=(MemPackStackRetain *)b;

	int diff=((u8 *)ra->oldPtr)-((u8 *)rb->oldPtr);

	return diff;
}


MemPackStack *psCompact(MemPackStack *packStack, MemPackStackRetain *retains, s32 retainCount)
{
	int stillAllocated=packStack->peakAlloc-packStack->realloc;
	int suggestedSize=stillAllocated+(stillAllocated>>3);
	MemPackStack *newPackStack=packStackAllocWithSize(packStack->currentSizeIndex,suggestedSize);

	qsort(retains, retainCount, sizeof(MemPackStackRetain), retainSorter);

	for(int i=0;i<retainCount;i++)
		{
		void *oldPtr=retains[i].oldPtr;
		if(oldPtr!=NULL)
			{
			int retainSize=retains[i].size;
			void *newPtr=psAlloc(newPackStack, retainSize);

			if(newPtr==NULL)
				{
				LOG(LOG_CRITICAL,"Alloc failed during psCompact");
				}

			retains[i].newPtr=newPtr;
			memcpy(newPtr, retains[i].oldPtr, retainSize);
			}
		else
			retains[i].newPtr=NULL;
		}


	packStackFree(packStack);

	return newPackStack;
}


