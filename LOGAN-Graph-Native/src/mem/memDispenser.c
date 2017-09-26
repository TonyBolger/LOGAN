
#include "common.h"



static MemDispenserBlock *dispenserBlockAlloc(int memTrackerId, s32 blocksize)
{
	MemDispenserBlock *block=NULL;

	if(sizeof(MemDispenserBlock)&(CACHE_ALIGNMENT_SIZE-1))
		{
		LOG(LOG_CRITICAL,"Dispenser block not a multiple of cache alignment size %i vs %i",sizeof(MemDispenserBlock),CACHE_ALIGNMENT_SIZE);
		}

	block=G_ALLOC_ALIGNED(sizeof(MemDispenserBlock)+blocksize, CACHE_ALIGNMENT_SIZE, memTrackerId);

	block->prev=NULL;
	block->allocated=0;
	block->blocksize=blocksize;

//	LOG(LOG_INFO,"AllocDispBlock %p %i",block,blocksize);

	//memset(block->data,0,DISPENSER_BLOCKSIZE);

	return block;
}


MemDispenser *dispenserAlloc(int memTrackerId, u32 baseBlocksize, u32 maxBlocksize)
{
	//LOG(LOG_INFO,"Allocating Dispenser: %s",name);
	MemDispenser *disp=G_ALLOC(sizeof(MemDispenser), MEMTRACKID_DISPENSER);

	if(disp==NULL)
		LOG(LOG_CRITICAL,"Failed to alloc dispenser");

	disp->memTrackerId=memTrackerId;
	disp->blocksize=baseBlocksize;
	disp->maxBlocksize=maxBlocksize;

	disp->block=dispenserBlockAlloc(memTrackerId, baseBlocksize);
	disp->allocated=0;

	return disp;
}

void dispenserFree(MemDispenser *disp)
{
//	LOG(LOG_INFO,"DispFree %p",disp);

	if(disp==NULL)
		return;

	int totalAllocated=0;

	MemDispenserBlock *blockPtr=disp->block;

	while(blockPtr!=NULL)
		{
		MemDispenserBlock *prevPtr=blockPtr->prev;

		totalAllocated+=blockPtr->allocated;

		if(blockPtr!=NULL)
			G_FREE(blockPtr, sizeof(MemDispenserBlock)+blockPtr->blocksize, disp->memTrackerId);
		blockPtr=prevPtr;
		}

	G_FREE(disp, sizeof(MemDispenser), MEMTRACKID_DISPENSER);
}


void dispenserReset(MemDispenser *disp)
{
//	LOG(LOG_INFO,"DispReset %p",dispenser);

	MemDispenserBlock *blockPtr=disp->block;

	if(blockPtr!=NULL)
		{
		blockPtr->allocated=0;

		blockPtr=blockPtr->prev;

		while(blockPtr!=NULL)
			{
			MemDispenserBlock *prevPtr=blockPtr->prev;

			if(blockPtr!=NULL)
				G_FREE(blockPtr, sizeof(MemDispenserBlock)+blockPtr->blocksize, disp->memTrackerId);

			blockPtr=prevPtr;
			}
		}

	disp->block->prev=NULL;
	disp->allocated=0;

//	int i=0;
//	for(i=0;i<MAX_ALLOCATORS;i++)
//		dispenser->allocatorUsage[i]=0;

}


int dispenserSize(MemDispenser *disp)
{
	return disp->allocated;
}


static void *dAllocAligned(MemDispenser *disp, size_t allocSize, s32 alignmentMask)
{
	if(allocSize==0)
		return NULL;

	if(disp->allocated>DISPENSER_MAX)
		{
		LOG(LOG_INFO,"Dispenser Overallocation detected - wanted %i\n",(int)allocSize);
		LOG(LOG_CRITICAL,"Dispenser %s refusing to allocate",MEMTRACKER_NAMES[disp->memTrackerId]);

		return NULL;
		}

	int padSize=0;

	if(alignmentMask>0)
		{
		int offset=disp->block->allocated & alignmentMask;
		if(offset>0)
			padSize=1+alignmentMask-offset;
		}

//	if(disp->block != NULL)

	//LOG(LOG_INFO,"Alloc of %i with %i padding",allocSize,padSize);

	int tempAllocSize=allocSize+padSize;

	if(disp->block == NULL || disp->block->allocated+tempAllocSize > disp->block->blocksize)
		{
		u32 blocksize=disp==NULL? disp->blocksize: disp->block->blocksize;

		if(tempAllocSize>blocksize)
			{
			padSize=0;

			while(allocSize>blocksize)
				{
				blocksize=blocksize*2;
				if(blocksize>=disp->maxBlocksize)
					{
					LOG(LOG_INFO,"Warning: Block %s expanded to max: %i",MEMTRACKER_NAMES[disp->memTrackerId],disp->maxBlocksize);
					blocksize=disp->maxBlocksize;
					break;
					}

				LOG(LOG_INFO,"Block %s expanded to: %i",MEMTRACKER_NAMES[disp->memTrackerId],blocksize);
				}
			}

		MemDispenserBlock *newBlock=dispenserBlockAlloc(disp->memTrackerId, blocksize);

		newBlock->prev=disp->block;
		disp->block=newBlock;
		}

	allocSize+=padSize;

	MemDispenserBlock *block=disp->block;

	if(block->allocated+allocSize > block->blocksize)
		{
		LOG(LOG_INFO,"Dispenser Local overallocation detected - wanted %i\n",(int)allocSize);

		LOG(LOG_CRITICAL,"Dispenser %s refusing to allocate",MEMTRACKER_NAMES[disp->memTrackerId]);
		return NULL;
		}

	void *usrPtr=block->data+block->allocated+padSize;

	block->allocated+=allocSize;
	disp->allocated+=allocSize;

	//memset(usrPtr,0,allocSize);

//	LOG(LOG_INFO,"Alloced %i at %p",allocSize,usrPtr);

	return usrPtr;
}


void *dAlloc(MemDispenser *disp, size_t size)
{
	return dAllocAligned(disp,size,0);
}

void *dAllocQuadAligned(MemDispenser *disp, size_t size)
{
	return dAllocAligned(disp,size,QUAD_ALIGNMENT_SIZE-1);
}


void *dAllocCacheAligned(MemDispenser *disp, size_t size)
{
	return dAllocAligned(disp,size,CACHE_ALIGNMENT_SIZE-1);
}


