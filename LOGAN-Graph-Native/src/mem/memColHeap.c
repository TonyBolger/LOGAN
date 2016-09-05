#include "common.h"

#define COLHEAP_CONFIG_NUM 21
#define COLHEAP_CONFIG_MIN 0

/*
 * GC strategy: Multi-generational except in smallest (2-block) scenario
 *
 * Simple case: Allocate in current young block.
 *
 * Young generation:
 * 		Young blocks must be empty before reuse, by spilling live data to next generation (if any). *
 * 		Young generation serializes entries by last update.
 *
 * Middle generation(s):
 * 		Blocks at least partially empty before reuse, by spilling live data to next generation. Remaining data if any is compacted
 * 		Each block serializes entries internally by age, no strict ordering between blocks
 * 		 *
 * Old generation:
 * 		Compaction within blocks only.
 */
// Currently each is a factor of 2 total - perhaps too aggressive?
// blockSize, blocksPerGeneration[COLHEAP_MAX_GENERATIONS]

MemColHeapConfig COLHEAP_CONFIGS[]=
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


static int colHeapSizeInit=0;

//static void checkSizeInit()
void checkSizeInit()
{
	if(colHeapSizeInit)
		return;

	for(int i=0;i<COLHEAP_CONFIG_NUM;i++)
		{
		int currentBlock=0;

		for(int j=0;i<COLHEAP_MAX_GENERATIONS; j++)
			{
			if(COLHEAP_CONFIGS[i].blocksPerGeneration[j])
				{
				if(COLHEAP_CONFIGS[i].blocksPerGeneration[j]>COLHEAP_MAX_BLOCKS_PER_GENERATION)
					{
					LOG(LOG_CRITICAL,"Invalid ColHeap configuration %i",i);
					}

				COLHEAP_CONFIGS[i].startOffsetPerGeneration[j]=COLHEAP_CONFIGS[i].blockSize*currentBlock;
				currentBlock+=COLHEAP_CONFIGS[i].blocksPerGeneration[j];
				COLHEAP_CONFIGS[i].generationCount=j;
				}
			}

		COLHEAP_CONFIGS[i].totalBlocks=currentBlock;
		COLHEAP_CONFIGS[i].totalSize=COLHEAP_CONFIGS[i].blockSize*currentBlock;
		}
}


static int getConfigIndex(int suggestedConfigIndex, int minSize)
{
	int targetSize=minSize+sizeof(MemPackStack);

	if(suggestedConfigIndex<COLHEAP_CONFIG_NUM)
		{
		int size=COLHEAP_CONFIGS[suggestedConfigIndex].totalSize;

		while(suggestedConfigIndex<COLHEAP_CONFIG_NUM && size<targetSize)
			size=COLHEAP_CONFIGS[suggestedConfigIndex].totalSize;
		}

	if(suggestedConfigIndex>COLHEAP_CONFIG_NUM)
		{
		LOG(LOG_CRITICAL, "Attempt to allocate MemColHeap beyond maximum size %i (%i vs %i)",
				COLHEAP_CONFIGS[COLHEAP_CONFIG_NUM-1],COLHEAP_CONFIG_NUM-1,suggestedConfigIndex);
		}

	return suggestedConfigIndex;
}


static void colHeapInit(MemColHeap *colHeap, long (*itemSizeResolver)(u8 *item))
{
	for(int i=0;i<COLHEAP_MAX_GENERATIONS;i++)
		{
		for(int j=0;j<COLHEAP_MAX_BLOCKS_PER_GENERATION;j++)
			{
			colHeap->blocks[i][j].size=0;
			colHeap->blocks[i][j].alloc=0;
			colHeap->blocks[i][j].data=NULL;
			}

		colHeap->genBlockIndex[i]=0;
		}

	colHeap->peakAlloc=0;
	colHeap->realloc=0;

	for(int i=0;i<SMER_DISPATCH_GROUP_SLICES;i++)
		{
		colHeap->roots[i].rootPtrs=NULL;
		colHeap->roots[i].numRoots=0;
		}

	colHeap->itemSizeResolver=itemSizeResolver;
}

static void colHeapConfigure(MemColHeap *colHeap, int configIndex)
{
	if(configIndex>=COLHEAP_CONFIG_NUM)
		{
		LOG(LOG_CRITICAL,"Attempted to reconfigure MemConfigHeap beyond maximum (%i)",configIndex);
		}

	colHeap->configIndex=configIndex;

	for(int i=0;i<COLHEAP_CONFIGS[configIndex].generationCount;i++)
		{
		for(int j=0;j<COLHEAP_CONFIGS[configIndex].blocksPerGeneration[i];j++)
			{
			int newSize=COLHEAP_CONFIGS[configIndex].blockSize;

			int oldSize=colHeap->blocks[i][j].size;
			u8 *oldData=colHeap->blocks[i][j].data;

			if(newSize!=oldSize)
				{
				u8 *newData=NULL;

				if((posix_memalign((void **)(&newData), CACHE_ALIGNMENT_SIZE, newSize)!=0) || (newData==NULL))
					LOG(LOG_CRITICAL,"Failed to alloc MemColHeap datablock");

				if(oldData!=NULL)
					{
					memcpy(newData,oldData,colHeap->blocks[i][j].alloc);
					free(oldData);
					}

				colHeap->blocks[i][j].data=newData;
				colHeap->blocks[i][j].size=newSize;
				// colHeap->blocks[i].alloc remains unchanged
				}

			}

		}

}

//static
void colHeapReconfigure(MemColHeap *colHeap, size_t minimumSize)
{
	int configIndex=colHeap->configIndex+1;

	while(configIndex<COLHEAP_CONFIG_NUM && COLHEAP_CONFIGS[configIndex].blockSize<minimumSize)
		configIndex++;

	colHeapConfigure(colHeap, configIndex);
}


//static
int getNextBlockIndexForGeneration(MemColHeap *colHeap, int generation)
{
	s32 configIndex=colHeap->configIndex;

	if(generation>=COLHEAP_CONFIGS[configIndex].generationCount)
		{
		LOG(LOG_CRITICAL,"Attempt to move block for invalid generation");
		}

	int genBlockIndex=COLHEAP_MAX_BLOCKS_PER_GENERATION*generation;

	genBlockIndex=(genBlockIndex+1);
	if(genBlockIndex>=COLHEAP_CONFIGS[colHeap->configIndex].blocksPerGeneration[generation])
		genBlockIndex=0;

	return genBlockIndex;
}

static void colHeapSetCurrentYoungBlock(MemColHeap *colHeap, int youngBlockIndex)
{
	s32 configIndex=colHeap->configIndex;

	if(youngBlockIndex>=COLHEAP_CONFIGS[configIndex].blocksPerGeneration[0])
		{
		LOG(LOG_CRITICAL,"Attempt to move to block beyond young generation (%i)",youngBlockIndex);
		}

	colHeap->genBlockIndex[0]=youngBlockIndex;

	MemColHeapBlock *youngBlock=&(colHeap->blocks[0][youngBlockIndex]);
	colHeap->currentYoungBlock=youngBlock;

	if(youngBlock->alloc!=0)
		{
		LOG(LOG_CRITICAL, "Attempted to switch to a non-empty young block");
		}
}

static void colHeapRotateCurrentYoungBlock(MemColHeap *colHeap)
{
	int youngBlockIndex=getNextBlockIndexForGeneration(colHeap, 0);

	colHeap->genBlockIndex[0]=youngBlockIndex;

	MemColHeapBlock *youngBlock=&(colHeap->blocks[0][youngBlockIndex]);
	colHeap->currentYoungBlock=youngBlock;

}



static MemColHeap *colHeapAllocWithSize(int suggestedConfigIndex, int minSize, long (*itemSizeResolver)(u8 *item))
{
	MemColHeap *colHeap=NULL;

	if((posix_memalign((void **)&colHeap, CACHE_ALIGNMENT_SIZE, sizeof(MemColHeap))!=0) || (colHeap==NULL))
		LOG(LOG_CRITICAL,"Failed to alloc MemColHeap");

	int configIndex=getConfigIndex(suggestedConfigIndex, minSize);

	colHeapInit(colHeap, itemSizeResolver);
	colHeapConfigure(colHeap,configIndex);
	colHeapSetCurrentYoungBlock(colHeap,0);

	return colHeap;
}


MemColHeap *colHeapAlloc(long (*itemSizeResolver)(u8 *item))
{
	checkSizeInit();
	return colHeapAllocWithSize(COLHEAP_CONFIG_MIN, 0, itemSizeResolver);
}


void colHeapFree(MemColHeap *colHeap)
{
	for(int i=0;i<COLHEAP_MAX_GENERATIONS;i++)
		{
		for(int j=0;j<COLHEAP_MAX_BLOCKS_PER_GENERATION;j++)
			free(colHeap->blocks[i][j].data);
		}

	free(colHeap);
}


void chRegisterRoots(MemColHeap *colHeap, int rootSetIndex, u8 **rootPtrs, s32 numRoots)
{
	if(colHeap->roots[rootSetIndex].numRoots!=0)
		{
		LOG(LOG_CRITICAL,"Roots already registered for this ColHeap");
		}

	colHeap->roots[rootSetIndex].rootPtrs=rootPtrs;
	colHeap->roots[rootSetIndex].numRoots=numRoots;
}


static int checkForSpace(MemColHeapBlock *block, size_t newAllocSize)
{
	long totalAlloc=block->alloc+newAllocSize;

	return totalAlloc<=block->size;
}

static void *allocFromBlock(MemColHeapBlock *block, size_t newAllocSize)
{
	long totalAlloc=block->alloc+newAllocSize;

	if(totalAlloc<=block->size)
		{
		void *data=block->data+block->alloc;
		block->alloc=totalAlloc;
		return data;
		}

	return NULL;
}


static void colHeapGarbageCollect(MemColHeap *colHeap, int generation, int genBlockIndex, MemDispenser *disp)
{

}

static void colHeapEnsureSpace(MemColHeap *colHeap, size_t newAllocSize)
{
	s32 configIndex=colHeap->configIndex;

	// If even an empty block is insufficient, expand ColHeap, then re-test before GC
	if(COLHEAP_CONFIGS[configIndex].blockSize<newAllocSize)
		{
		colHeapReconfigure(colHeap,newAllocSize);

		MemColHeapBlock *block=colHeap->currentYoungBlock;
		long totalAlloc=block->alloc+newAllocSize;
		if(totalAlloc<block->size)
			return;
		}

	// Rotate young block
	colHeapRotateCurrentYoungBlock(colHeap);
	MemColHeapBlock *youngBlock=colHeap->currentYoungBlock;

	// If not empty, GC
	if(youngBlock->alloc!=0)
		{
		MemDispenser *disp=dispenserAlloc("ColHeapGC");
		colHeapGarbageCollect(colHeap, 0, colHeap->genBlockIndex[0], disp);
		dispenserFree(disp);

		// If still not empty, expand to prevent GC thrashing
		if(youngBlock->alloc!=0)
			colHeapReconfigure(colHeap,newAllocSize);

		// If enough space, happy
		if(checkForSpace(youngBlock, newAllocSize))
			return;

		// GC again
		MemDispenser *disp=dispenserAlloc("ColHeapGC");
		colHeapGarbageCollect(colHeap, 0, colHeap->genBlockIndex[0], disp);
		dispenserFree(disp);

		// If enough space, happy
		if(checkForSpace(youngBlock, newAllocSize))
			return;

		}

}

void *chAlloc(MemColHeap *colHeap, size_t size)
{
	if(size>COLHEAP_MAX_ALLOC)
		LOG(LOG_CRITICAL, "Request to allocate %i bytes of ColHeap memory refused since it is above the max allocation size %i",size,COLHEAP_MAX_ALLOC);

	// Simple case - alloc from the current young block
	void *usrPtr=allocFromBlock(colHeap->currentYoungBlock, size);
	if(usrPtr!=NULL)
		return usrPtr;

	colHeapEnsureSpace(colHeap, size);

	usrPtr=allocFromBlock(colHeap->currentYoungBlock, size);
	if(usrPtr!=NULL)
		return usrPtr;

	LOG(LOG_CRITICAL, "Failed to allocate %i bytes of ColHeap memory after compaction",size);
	return NULL;
}




