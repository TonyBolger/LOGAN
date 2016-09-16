#include "common.h"

#define COLHEAP_CONFIG_NUM 21
#define COLHEAP_CONFIG_MIN 0

/*
 * GC strategy: Multi-generational, oldest generations first
 *
 * Simple case: Allocate in current young block.
 *
 * Young generation: Initial allocation point
 * 		Rotate to next block when full
 * 		Young blocks must be empty before reuse, empty by GC and spill live data to next older generation
 * 		Young generation serializes entries by last update
 *
 * Middle generation(s): Catches survivors from younger GC
 * 		Rotate to next block when full
 * 		Blocks at least partially empty before reuse, by spilling live data to next older generation. Remaining data if any is compacted
 * 		Move to new block when full
 *
 * 		Each block serializes entries internally by age, no strict ordering between blocks
 *
 * Old generation:
 * 		Compaction within blocks only.
 * 		Expand heap when totalUsed * efficiencyFactor > blockSize*(blocks-1)
 *
 */
// Currently each is a factor of 2 total - perhaps too aggressive?
// blockSize, blocksPerGeneration[COLHEAP_MAX_GENERATIONS]

MemColHeapConfig COLHEAP_CONFIGS[]=
{
		{0x000010000L,16,{2,2,0,0}}, // 4*64K
		{0x000020000L,17,{2,2,0,0}}, // 4*128K
		{0x000040000L,18,{2,2,0,0}}, // 4*256K
		{0x000080000L,19,{2,2,0,0}}, // 4*512K
		{0x000080000L,19,{4,2,2,0}}, // 8*512K

		{0x000100000L,20,{4,2,2,0}}, // 8*1M
		{0x000100000L,20,{4,4,4,4}}, // 16*1M
		{0x000200000L,21,{4,4,4,4}}, // 16*2M
		{0x000400000L,22,{4,4,4,4}}, // 16*4M
		{0x000800000L,23,{4,4,4,4}}, // 16*8M

		{0x001000000L,24,{4,4,4,4}}, // 16*16M
		{0x002000000L,25,{4,4,4,4}}, // 16*32M
		{0x004000000L,26,{4,4,4,4}}, // 16*64M
		{0x008000000L,27,{4,4,4,4}}, // 16*128M
		{0x010000000L,28,{4,4,4,4}}, // 16*256M

		{0x020000000L,29,{4,4,4,4}}, // 16*512M
		{0x040000000L,30,{4,4,4,4}}, // 16*1G
		{0x080000000L,31,{4,4,4,4}}, // 16*2G
		{0x100000000L,32,{4,4,4,4}}, // 16*4G
		{0x200000000L,33,{4,4,4,4}}, // 16*8G

		{0x400000000L,34,{4,4,4,4}}  // 16*16G
};


static int colHeapSizeInit=0;

static u8 *_globalColHeap;
static u8 *_globalColNextHeap;
static s64 _globalColHeapTotalSize;
static s64 _globalColHeapPerHeapSize;

//static void checkSizeInit()
static long colHeapCheckConfigAndGetMaximumSize()
{
	long maxTotalSize=0;
	long maxBlockSize=0;

	for(int i=0;i<COLHEAP_CONFIG_NUM;i++)
		{
		int currentBlock=0;

		for(int j=0;j<COLHEAP_MAX_GENERATIONS; j++)
			{
			if(COLHEAP_CONFIGS[i].blocksPerGeneration[j])
				{
				if(COLHEAP_CONFIGS[i].blocksPerGeneration[j]>COLHEAP_MAX_BLOCKS_PER_GENERATION)
					{
					LOG(LOG_CRITICAL,"Invalid ColHeap configuration %i with %i blocks",i,COLHEAP_CONFIGS[i].blocksPerGeneration[j]);
					}

				COLHEAP_CONFIGS[i].startBlockPerGeneration[j]=currentBlock;
				COLHEAP_CONFIGS[i].startOffsetPerGeneration[j]=COLHEAP_CONFIGS[i].blockSize*currentBlock;

				currentBlock+=COLHEAP_CONFIGS[i].blocksPerGeneration[j];
				COLHEAP_CONFIGS[i].generationCount=j+1;
				}
			else
				{
				COLHEAP_CONFIGS[i].startBlockPerGeneration[j]=-1;
				COLHEAP_CONFIGS[i].startOffsetPerGeneration[j]=-1;
				}
			}

		COLHEAP_CONFIGS[i].totalBlocks=currentBlock;

		long blockSize=COLHEAP_CONFIGS[i].blockSize;
		long shiftBlockSize=1L<<COLHEAP_CONFIGS[i].blockShift;

		if(blockSize!=shiftBlockSize)
			{
			LOG(LOG_CRITICAL, "ColHeap configuration blocksize %li does not match size implied from block shift %li",blockSize,shiftBlockSize);
			}

		long totalSize=blockSize*currentBlock;
		COLHEAP_CONFIGS[i].totalSize=totalSize;

		if(maxTotalSize>totalSize)
			{
			LOG(LOG_CRITICAL, "ColHeap configurations not ordered by total size at position %i",i);
			}

		if(maxBlockSize>blockSize)
			{
			LOG(LOG_CRITICAL, "ColHeap configurations not ordered by block size at position %i",i);
			}

		maxTotalSize=totalSize;
		}

	if(__builtin_popcountl(maxTotalSize)!=1)
		LOG(LOG_CRITICAL, "Largest colHeap configurations not power of 2: %li (%lx)",maxTotalSize,maxTotalSize);

	return maxTotalSize;
}


static void colHeapGlobalHeapAlloc(long perHeapSize, int numHeaps)
{
	LOG(LOG_INFO,"ColHeap: Largest configuration requires %li",perHeapSize);

	long totalHeapSize=perHeapSize*numHeaps;

	u8 *rawHeap=mmap(NULL, totalHeapSize+perHeapSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

	if(rawHeap==NULL)
		{
		LOG(LOG_CRITICAL, "ColHeap global heap allocation failed");
		}
	else
		{
		LOG(LOG_INFO,"ColHeap raw global heap allocation ok %p",rawHeap);
		}

	long heapMask=~(perHeapSize-1);

	uintptr_t heapAsInt=(uintptr_t)rawHeap;
	uintptr_t paddedHeapAsInt=(heapAsInt+perHeapSize-1)&heapMask;

	u8 *paddedHeap=(u8 *)paddedHeapAsInt;

	long headPadding=paddedHeap-rawHeap;
	long tailPadding=perHeapSize-headPadding;

	LOG(LOG_INFO,"ColHeap passed heap allocation %p with padding %li %li",paddedHeap,headPadding,tailPadding);

	if(headPadding>0)
		{
		if(munmap(rawHeap,headPadding)!=0)
			LOG(LOG_CRITICAL,"ColHeap failed to munmap head padding");
		}

	if(tailPadding>0)
		{
		if(munmap(paddedHeap+totalHeapSize,tailPadding)!=0)
			LOG(LOG_CRITICAL,"ColHeap failed to munmap tail padding");
		}


	_globalColHeap=paddedHeap;
	_globalColNextHeap=paddedHeap;
	_globalColHeapTotalSize=totalHeapSize;
	_globalColHeapPerHeapSize=perHeapSize;

//	sleep(10);
//	LOG(LOG_CRITICAL,"Leaving now");

}

static void colHeapCheckGlobalInit()
{
	if(colHeapSizeInit)
		return;

	long maxSize=colHeapCheckConfigAndGetMaximumSize();
	colHeapGlobalHeapAlloc(maxSize, COLHEAP_NUM_HEAPS);
	colHeapSizeInit=1;
}


static int getConfigIndex(int suggestedConfigIndex, int minSize)
{
	if(suggestedConfigIndex<COLHEAP_CONFIG_NUM)
		{
		int blockSize=COLHEAP_CONFIGS[suggestedConfigIndex].blockSize;

		while(suggestedConfigIndex<COLHEAP_CONFIG_NUM && blockSize<minSize)
			blockSize=COLHEAP_CONFIGS[suggestedConfigIndex].blockSize;
		}

	if(suggestedConfigIndex>COLHEAP_CONFIG_NUM)
		{
		LOG(LOG_CRITICAL, "Attempt to allocate MemColHeap beyond maximum size %i (%i vs %i)",
				COLHEAP_CONFIGS[COLHEAP_CONFIG_NUM-1],COLHEAP_CONFIG_NUM-1,suggestedConfigIndex);
		}

	return suggestedConfigIndex;
}


static void colHeapInit(MemColHeap *colHeap, s32 (*itemSizeResolver)(u8 *item))
{
	for(int i=0;i<COLHEAP_MAX_BLOCKS;i++)
		{
		colHeap->blocks[i].size=0;
		colHeap->blocks[i].alloc=0;
		colHeap->blocks[i].data=NULL;
		}

	for(int i=0;i<COLHEAP_MAX_GENERATIONS;i++)
		{
		colHeap->genCurrentActiveBlockIndex[i]=0;
		}

	if(_globalColNextHeap>=(_globalColHeap+_globalColHeapTotalSize))
		{
		LOG(LOG_CRITICAL,"Global ColHeap fully allocated");
		}

	colHeap->heapData=_globalColNextHeap;
	_globalColNextHeap+=_globalColHeapPerHeapSize;

	//colHeap->peakAlloc=0;
	//colHeap->realloc=0;

	for(int i=0;i<COLHEAP_ROOTSETS_PER_HEAP;i++)
		{
		colHeap->roots[i].rootPtrs=NULL;
		colHeap->roots[i].numRoots=0;
		}

	colHeap->itemSizeResolver=itemSizeResolver;
}



static int getNextBlockIndexForGeneration(MemColHeap *colHeap, int generation)
{
	s32 configIndex=colHeap->configIndex;

	if(generation>=COLHEAP_CONFIGS[configIndex].generationCount)
		{
		LOG(LOG_CRITICAL,"Attempt to move block for invalid generation %i with max %i");
		}

	int genBlockIndex=colHeap->genCurrentActiveBlockIndex[generation];

	genBlockIndex=(genBlockIndex+1);
	if(genBlockIndex>=COLHEAP_CONFIGS[colHeap->configIndex].blocksPerGeneration[generation])
		genBlockIndex=0;

	return genBlockIndex;
}


static void colHeapSetCurrentYoungBlock(MemColHeap *colHeap, int youngBlockIndex)
{
	LOG(LOG_INFO,"Configuring young generation (%i) to use block %i",colHeap->youngGeneration,youngBlockIndex);
	s32 configIndex=colHeap->configIndex;

	if(youngBlockIndex>=COLHEAP_CONFIGS[configIndex].blocksPerGeneration[0] || youngBlockIndex<0)
		{
		LOG(LOG_CRITICAL,"Attempt to move to block beyond young generation (%i)",youngBlockIndex);
		}

	colHeap->genCurrentActiveBlockIndex[colHeap->youngGeneration]=youngBlockIndex;

	MemColHeapBlock *youngBlock=colHeap->blocks+COLHEAP_CONFIGS[configIndex].startBlockPerGeneration[colHeap->youngGeneration]+youngBlockIndex;
	colHeap->currentYoungBlock=youngBlock;

	/*
	if(youngBlock->alloc!=0)
		{
		LOG(LOG_CRITICAL, "Attempted to switch to a non-empty young block %i",youngBlockIndex);
		}
	*/
}



static int colHeapGetCurrentYoungBlockIndex(MemColHeap *colHeap)
{
	return colHeap->genCurrentActiveBlockIndex[colHeap->youngGeneration]+COLHEAP_CONFIGS[colHeap->configIndex].startBlockPerGeneration[colHeap->youngGeneration];
}


static void colHeapRotateCurrentYoungBlock(MemColHeap *colHeap)
{
	int youngBlockIndex=getNextBlockIndexForGeneration(colHeap, colHeap->youngGeneration);
	colHeapSetCurrentYoungBlock(colHeap,youngBlockIndex);
}


static void colHeapConfigure(MemColHeap *colHeap, int configIndex, int reconfigure)
{
	int oldConfigIndex=colHeap->configIndex;

	if(configIndex>=COLHEAP_CONFIG_NUM)
		{
		LOG(LOG_CRITICAL,"Attempted to reconfigure MemConfigHeap beyond maximum (%i)",configIndex);
		}

	colHeap->configIndex=configIndex;
	long blockSize=COLHEAP_CONFIGS[configIndex].blockSize;
	long totalSize=COLHEAP_CONFIGS[configIndex].totalSize;

	long oldBlockSize=blockSize;
	long oldTotalSize=totalSize;
	if(reconfigure)
		{
		oldBlockSize=COLHEAP_CONFIGS[oldConfigIndex].blockSize;
		oldTotalSize=COLHEAP_CONFIGS[oldConfigIndex].blockSize;

		u8 *oldEnd=colHeap->heapData+oldTotalSize;
		long extraLength=totalSize-oldTotalSize;

		if(extraLength>0)
			{
			u8 *newMap=mmap(oldEnd, extraLength, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, 0, 0);
			if(newMap==NULL)
				LOG(LOG_CRITICAL,"Failed to map extended ColHeap");
			}
		}
	else
		{
		u8 *newMap=mmap(colHeap->heapData, totalSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, 0, 0);
		if(newMap==NULL)
			LOG(LOG_CRITICAL,"Failed to map ColHeap");
		}

	for(int i=0;i<COLHEAP_CONFIGS[configIndex].totalBlocks;i++)
		{
		colHeap->blocks[i].data=colHeap->heapData+blockSize*i;
		colHeap->blocks[i].size=blockSize;

		if(blockSize!=oldBlockSize)
			colHeap->blocks[i].alloc=-1; // Fixed after GC
		}

	for(int i=COLHEAP_CONFIGS[configIndex].totalBlocks;i<COLHEAP_MAX_BLOCKS;i++)
		{
		colHeap->blocks[i].size=0;
		colHeap->blocks[i].alloc=0;
		colHeap->blocks[i].data=NULL;
		}

	for(int i=0;i<COLHEAP_MAX_GENERATIONS;i++)
		{
		colHeap->genCurrentActiveBlockIndex[i]=0;
		}

	colHeap->youngGeneration=COLHEAP_CONFIGS[configIndex].generationCount-1;
	colHeapSetCurrentYoungBlock(colHeap,0);
}



static MemColHeap *colHeapAllocWithSize(int suggestedConfigIndex, int minSize, s32 (*itemSizeResolver)(u8 *item))
{
	MemColHeap *colHeap=NULL;

	if(posix_memalign((void **)&colHeap, CACHE_ALIGNMENT_SIZE, sizeof(MemColHeap))!=0)
		LOG(LOG_CRITICAL,"Failed to alloc MemColHeap");

//	colHeap=memalign(CACHE_ALIGNMENT_SIZE, sizeof(MemColHeap));
	if(colHeap==NULL)
		LOG(LOG_CRITICAL,"Failed to alloc MemColHeap");

	int configIndex=getConfigIndex(suggestedConfigIndex, minSize);

	colHeapInit(colHeap, itemSizeResolver);
	colHeapConfigure(colHeap,configIndex, 0);

	return colHeap;
}


MemColHeap *colHeapAlloc(s32 (*itemSizeResolver)(u8 *item))
{
	colHeapCheckGlobalInit();
	return colHeapAllocWithSize(COLHEAP_CONFIG_MIN, 0, itemSizeResolver);
}


void colHeapFree(MemColHeap *colHeap)
{
	// Should free heap block

	free(colHeap);
}




void chRegisterRoots(MemColHeap *colHeap, int rootSetIndex, u8 **rootPtrs, s32 numRoots)
{
	int oldNumRoots=colHeap->roots[rootSetIndex].numRoots;
	if(oldNumRoots!=0)
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


static MemColHeapGarbageCollection *colHeapGC_BuildCollectionStructure(MemColHeap *colHeap, MemDispenser *disp)
{
	MemColHeapGarbageCollection *gc=dAlloc(disp, sizeof(MemColHeapGarbageCollection));

	//s32 maxCount=0;
	s32 totalCount=0;
	s64 totalSize=0;

	gc->totalBlocks=COLHEAP_CONFIGS[colHeap->configIndex].totalBlocks;

	for(int i=0;i<COLHEAP_MAX_BLOCKS;i++)
		{
		gc->blocks[i].liveSize=0;
		gc->blocks[i].deadSize=0;
		gc->blocks[i].currentSpace=0;
		gc->blocks[i].compactSpace=0;
		gc->blocks[i].liveCount=0;
		}

	for(int i=0;i<COLHEAP_MAX_GENERATIONS;i++)
		{
		gc->generations[i].liveSize=0;
		gc->generations[i].deadSize=0;
		gc->generations[i].currentSpace=0;
		gc->generations[i].compactSpace=0;
		gc->generations[i].liveCount=0;
		}

	for(int i=0;i<COLHEAP_ROOTSETS_PER_HEAP;i++)
		{
		u8 **rootPtr=colHeap->roots[i].rootPtrs;
		int maxRoots=colHeap->roots[i].numRoots;

		MemColHeapRootSetQueue *queue=dAlloc(disp, sizeof(MemColHeapRootSetQueue)+sizeof(MemColHeapRootSetQueueEntry)*maxRoots);

		int entryCount=0;
		int blockShift=COLHEAP_CONFIGS[colHeap->configIndex].blockShift;

		for(int j=0;j<maxRoots;j++)
			{
			queue->entries[entryCount].dataPtr=rootPtr[j];
			queue->entries[entryCount].index=j;
			entryCount++;

			if(rootPtr[j]!=NULL)
				{
				s32 size=(colHeap->itemSizeResolver)(rootPtr[j]);
				queue->entries[entryCount].size=size;
				totalSize+=size;

				int blockIndex=(rootPtr[j]-colHeap->heapData)>>blockShift;
				gc->blocks[blockIndex].liveSize+=size;
				gc->blocks[blockIndex].liveCount++;
				}
			else
				queue->entries[entryCount].size=0;
			}

		queue->rootSet=&(colHeap->roots[i]);
		queue->rootNum=entryCount;
		queue->rootPos=0;

		gc->rootSetQueues[i]=queue;
		totalCount+=entryCount;
		}

	gc->rootSetTotalCount=totalCount;
	gc->rootSetTotalSize=totalSize;

	int generation=0;
	int blocksInGeneration=0;
	for(int i=0;i<COLHEAP_MAX_BLOCKS;i++)
		{
		if(blocksInGeneration>=COLHEAP_CONFIGS[colHeap->configIndex].blocksPerGeneration[generation])
			{
			generation++;
			blocksInGeneration=0;

			if(generation>=COLHEAP_CONFIGS[colHeap->configIndex].generationCount)
				break;
			}

		gc->generations[generation].liveCount+=gc->blocks[i].liveCount;

		s64 allocSize=colHeap->blocks[i].alloc;
		s64 liveSize=gc->blocks[i].liveSize;
		gc->generations[generation].liveSize+=liveSize;

		s64 deadSize=allocSize-liveSize;
		gc->blocks[i].deadSize=deadSize;
		gc->generations[generation].deadSize+=deadSize;

		long marginalSize=colHeap->blocks[i].size-COLHEAP_SPACE_MARGIN;

		long currentSpace=MAX(0,marginalSize-allocSize);
		long compactSpace=MAX(0,marginalSize-liveSize);

		gc->blocks[i].currentSpace=currentSpace;
		gc->blocks[i].compactSpace=compactSpace;

		gc->generations[generation].currentSpace+=currentSpace;
		gc->generations[generation].compactSpace+=compactSpace;

		blocksInGeneration++;
		}

	return gc;
}

/*
static void colHeapGC_SortEntriesAndIndex(MemColHeap *colHeap, MemColHeapGarbageCollection *gc)
{

}
*/

/*
//static
void colHeapGC_Reconfigure(MemColHeap *colHeap, size_t minimumSize)
{
	int configIndex=colHeap->configIndex+1;

	while(configIndex<COLHEAP_CONFIG_NUM && COLHEAP_CONFIGS[configIndex].blockSize<minimumSize)
		configIndex++;

	LOG(LOG_INFO,"Switching to ColHeap config %i",configIndex);

	LOG(LOG_CRITICAL,"Not implemented");
}
*/

static void colHeapGC_doResize(MemColHeap *colHeap, MemColHeapGarbageCollection *gc, int resizeConfigIndex, MemDispenser *disp)
{
	LOG(LOG_INFO,"Need heap expand to config %i",resizeConfigIndex);

	LOG(LOG_CRITICAL,"Not implemented");
}


static void colHeapGC_doGarbageCollect(MemColHeap *colHeap, MemColHeapGarbageCollection *gc, int youngBlockIndex, int gcGeneration, MemDispenser *disp)
{
	for(int i=0;i<colHeap->youngGeneration;i++)
		{
		LOG(LOG_INFO,"GC of generation %i",gcGeneration);
		}

	LOG(LOG_INFO,"GC of young block %i",youngBlockIndex);

	LOG(LOG_CRITICAL,"Not implemented");

}

static void colHeapGC(MemColHeap *colHeap, int forceConfigIndex, MemDispenser *disp)
{
	// Aims to free up the current young block

	//LOG(LOG_INFO,"Collecting block %i from generation %i",genBlockIndex,generation);

	MemColHeapGarbageCollection *gc=colHeapGC_BuildCollectionStructure(colHeap, disp);

	LOG(LOG_INFO,"Built GC structure with %i roots totalling %li across %i blocks",gc->rootSetTotalCount,gc->rootSetTotalSize,gc->totalBlocks);
	for(int i=0;i<gc->totalBlocks;i++)
		LOG(LOG_INFO,"Block %i contains %li total in %i items with %li dead, %li currently available and %li if compacted",
				i,gc->blocks[i].liveSize,gc->blocks[i].liveCount,gc->blocks[i].deadSize,
				gc->blocks[i].currentSpace,gc->blocks[i].compactSpace);

	int generations=COLHEAP_CONFIGS[colHeap->configIndex].generationCount;

	for(int i=0;i<generations;i++)
		LOG(LOG_INFO,"Generation %i contains %li total in %i items with %li dead, %li currently available and %li if compacted",
				i,gc->generations[i].liveSize,gc->generations[i].liveCount,gc->generations[i].deadSize,
				gc->generations[i].currentSpace,gc->generations[i].compactSpace);


	int youngBlockIndex=colHeapGetCurrentYoungBlockIndex(colHeap);
	s64 requiredSpace=gc->blocks[youngBlockIndex].liveSize;

	LOG(LOG_INFO,"Need %li space", requiredSpace);
	s64 multiGenerationFree=0;

	// Heuristic - see if existing

	int gcGeneration=0;
	int resizeConfigIndex=forceConfigIndex;

	for(int i=colHeap->youngGeneration;i>0;i--)
		{
		long mgfWithNext=multiGenerationFree+gc->generations[i-1].currentSpace; // Try using gen-1 space without compacting

		if(mgfWithNext>=requiredSpace)
			{
			multiGenerationFree=mgfWithNext;
			gcGeneration=i;

			break;
			}

		multiGenerationFree+=gc->generations[i-1].compactSpace; // Gen-1 will also be compacted
		}

	if(multiGenerationFree < requiredSpace)
		resizeConfigIndex=colHeap->configIndex+1;

	if(!resizeConfigIndex)
		{
		colHeapGC_doGarbageCollect(colHeap, gc, youngBlockIndex, gcGeneration, disp);
		}
	else
		{
		colHeapGC_doResize(colHeap, gc, resizeConfigIndex, disp);
		}

	/*
	int configIndex=colHeap->configIndex;

	// Heuristic - there must be at least one block worth of space free
	if((COLHEAP_CONFIGS[configIndex].totalSize-COLHEAP_CONFIGS[configIndex].blockSize)<gc->rootSetTotalSize)
		{
		LOG(LOG_INFO,"Insufficient ColHeap space to free one block - will need to expand ColHeap");
		return;
		}

	colHeapGC_SortEntriesAndIndex(colHeap, gc);
*/

}


static void colHeapEnsureSpace(MemColHeap *colHeap, size_t newAllocSize)
{
	LOG(LOG_INFO,"Failed to allocate %i from block %i",newAllocSize, colHeap->genCurrentActiveBlockIndex[colHeap->youngGeneration]);
	s32 configIndex=colHeap->configIndex;

	// If even an empty block is insufficient, GC with force expand
	if(COLHEAP_CONFIGS[configIndex].blockSize<newAllocSize)
		{
		LOG(LOG_INFO,"Large alloc requested - expand forced");

		int newConfigIndex=getConfigIndex(colHeap->configIndex, newAllocSize);

		MemDispenser *disp0=dispenserAlloc("ColHeapGC0");
		colHeapGC(colHeap, newConfigIndex, disp0);
		dispenserFree(disp0);

		MemColHeapBlock *block=colHeap->currentYoungBlock;
		long totalAlloc=block->alloc+newAllocSize;
		if(totalAlloc<block->size)
			return;
		}

	// Rotate young block
	colHeapRotateCurrentYoungBlock(colHeap);
	MemColHeapBlock *youngBlock=colHeap->currentYoungBlock;

	LOG(LOG_INFO,"Rotating young block to %i", colHeap->genCurrentActiveBlockIndex[colHeap->youngGeneration]);

	// If not empty, GC
	if(youngBlock->alloc!=0)
		{
		LOG(LOG_INFO,"Young block has %li already allocated", youngBlock->alloc);

		MemDispenser *disp1=dispenserAlloc("ColHeapGC1");
		colHeapGC(colHeap, 0, disp1);
		dispenserFree(disp1);

		// If enough space, happy
		if(checkForSpace(youngBlock, newAllocSize))
			return;

		LOG(LOG_CRITICAL, "GC fail");
		}

}


void *chAlloc(MemColHeap *colHeap, size_t size)
{
	if(size>COLHEAP_MAX_ALLOC)
		LOG(LOG_CRITICAL, "Request to allocate %i bytes of ColHeap memory refused since it is above the max allocation size %i",size,COLHEAP_MAX_ALLOC);

//	LOG(LOG_INFO,"Allocating %i",size);

	// Simple case - alloc from the current young block
	void *usrPtr=allocFromBlock(colHeap->currentYoungBlock, size);
	if(usrPtr!=NULL)
		return usrPtr;

	colHeapEnsureSpace(colHeap, size);

	usrPtr=allocFromBlock(colHeap->currentYoungBlock, size);
	if(usrPtr!=NULL)
		return usrPtr;

	LOG(LOG_CRITICAL, "Failed to allocate %i bytes of ColHeap memory after trying really hard, honest",size);
	return NULL;
}




