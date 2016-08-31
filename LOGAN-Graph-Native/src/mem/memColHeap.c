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

				COLHEAP_CONFIGS[i].generationStartBlock[j]=currentBlock;
				COLHEAP_CONFIGS[i].generationCount=j;
				}
			else
				{
				COLHEAP_CONFIGS[i].generationStartBlock[j]=-1;
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
				PACKSTACK_SIZES[COLHEAP_CONFIG_NUM-1],COLHEAP_CONFIG_NUM-1,suggestedConfigIndex);
		}

	return suggestedConfigIndex;
}


static void colHeapInit(MemColHeap *colHeap)
{
	for(int i=0;i<COLHEAP_MAX_BLOCKS;i++)
		{
		colHeap->blocks[i].size=0;
		colHeap->blocks[i].alloc=0;
		colHeap->blocks[i].data=NULL;
		}

	for(int i=0;i<COLHEAP_MAX_GENERATIONS;i++)
		{
		colHeap->genBlockIndex[i]=-1;
		}

	colHeap->peakAlloc=0;
	colHeap->realloc=0;
}

static void colHeapConfigure(MemColHeap *colHeap, int configIndex)
{
	if(configIndex>=COLHEAP_CONFIG_NUM)
		{
		LOG(LOG_CRITICAL,"Attempted to reconfigure MemConnfigHeap beyond maximum");
		}

	colHeap->configIndex=configIndex;

	for(int i=0;i<COLHEAP_CONFIGS[configIndex].totalBlocks;i++)
		{
		int newSize=COLHEAP_CONFIGS[configIndex].blockSize;

		int oldSize=colHeap->blocks[i].size;
		u8 *oldData=colHeap->blocks[i].data;

		if(newSize!=oldSize)
			{
			u8 *newData=NULL;

			if((posix_memalign(&(newData), CACHE_ALIGNMENT_SIZE, newSize)!=0) || (newData==NULL))
				LOG(LOG_CRITICAL,"Failed to alloc MemColHeap datablock");

			if(oldData!=NULL)
				{
				memcpy(newData,oldData,colHeap->blocks[i].alloc);
				free(oldData);
				}
			colHeap->blocks[i].data=newData;
			colHeap->blocks[i].size=newSize;
			// colHeap->blocks[i].alloc remains unchanged
			}

		}

	for(int i=0;i<COLHEAP_CONFIGS[configIndex].generationCount;i++)
		{
		colHeap->genBlockIndex[i]=0;
		}
}

static void colHeapReconfigure(MemColHeap *colHeap)
{
	colHeapConfigure(colHeap->configIndex+1);
}


static int getNextBlockIndexForGeneration(MemColHeap *colHeap, int generation)
{
	int genBlockIndex=colHeap->genBlockIndex[generation];

	if(genBlockIndex==-1)
		{
		LOG(LOG_CRITICAL,"Attempt to move block for invalid generation");
		}

	genBlockIndex=(genBlockIndex+1);
	if(genBlockIndex>=COLHEAP_CONFIGS[colHeap->configIndex].generationCount)
		genBlockIndex=0;

	return genBlockIndex;
}


static void colHeapSetCurrentYoungBlock(MemColHeap *colHeap, int youngBlockIndex)
{
	s32 configIndex=colHeap->configIndex;

	int youngestGen=COLHEAP_CONFIGS[configIndex].generationCount-1;
	int blockIndex=COLHEAP_CONFIGS[configIndex].generationStartBlock[youngestGen]+youngBlockIndex;

	MemColHeapBlock *youngBlock=colHeap->blocks+blockIndex;

	colHeap->currentYoungBlock=youngBlock;

	if(youngBlock->alloc!=0)
		{
		LOG(LOG_CRITICAL, "Attempted to switch to a non-empty young block");
		}
}


static MemColHeap *colHeapAllocWithSize(int suggestedConfigIndex, int minSize)
{
	MemColHeap *colHeap=NULL;

	if((posix_memalign(&colHeap, CACHE_ALIGNMENT_SIZE, sizeof(MemColHeap))!=0) || (colHeap==NULL))
		LOG(LOG_CRITICAL,"Failed to alloc MemColHeap");

	int configIndex=getConfigIndex(suggestedConfigIndex, minSize);

	colHeapInit(colHeap);
	colHeapConfigure(colHeap,configIndex);
	colHeapSetCurrentYoungBlock(colHeap,0);

	return colHeap;
}


MemColHeap *colHeapAlloc()
{
	checkSizeInit();
	return colHeapAllocWithSize(COLHEAP_CONFIG_MIN, 0);
}


void colHeapFree(MemColHeap *colHeap)
{
	for(int i=0;i<COLHEAP_MAX_BLOCKS;i++)
		free(colHeap->blocks[i].data);

	free(colHeap);
}


