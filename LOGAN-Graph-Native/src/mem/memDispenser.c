
#include "common.h"



static MemDispenserBlock *dispenserBlockAlloc(s32 blocksize)
{
	MemDispenserBlock *block=NULL;

	if(sizeof(MemDispenserBlock)&(CACHE_ALIGNMENT_SIZE-1))
		{
		LOG(LOG_CRITICAL,"Dispenser block not a multiple of cache alignment size %i vs %i",sizeof(MemDispenserBlock),CACHE_ALIGNMENT_SIZE);
		}

	if(posix_memalign((void **)&block,CACHE_ALIGNMENT_SIZE, sizeof(MemDispenserBlock)+blocksize)!=0)
			LOG(LOG_CRITICAL,"Failed to alloc dispenser block");

//	block=memalign(CACHE_ALIGNMENT_SIZE, sizeof(MemDispenserBlock));

	if(block==NULL)
		LOG(LOG_CRITICAL,"Failed to alloc dispenser block");

	block->prev=NULL;
	block->allocated=0;
	block->blocksize=blocksize;

	//memset(block->data,0,DISPENSER_BLOCKSIZE);

	return block;
}


MemDispenser *dispenserAlloc(const char *name, u32 baseBlocksize, u32 maxBlocksize)
{
	//LOG(LOG_INFO,"Allocating Dispenser: %s",name);
	MemDispenser *disp=malloc(sizeof(MemDispenser));

	if(disp==NULL)
		LOG(LOG_CRITICAL,"Failed to alloc dispenser");

	disp->name=name;
	disp->blocksize=baseBlocksize;
	disp->maxBlocksize=maxBlocksize;

	disp->block=dispenserBlockAlloc(baseBlocksize);
	disp->allocated=0;

//	int i=0;
//	for(i=0;i<MAX_ALLOCATORS;i++)
//		disp->allocatorUsage[i]=0;

//	memset(disp->allocatorUsage,0,sizeof(int)*MAX_ALLOCATORS);

	return disp;
}

void dispenserFree(MemDispenser *disp)
{
	if(disp==NULL)
		return;

	int totalAllocated=0;

	MemDispenserBlock *blockPtr=disp->block;

	while(blockPtr!=NULL)
		{
		MemDispenserBlock *prevPtr=blockPtr->prev;

		totalAllocated+=blockPtr->allocated;

		free(blockPtr);
		blockPtr=prevPtr;
		}


	free(disp);
}

void dispenserFreeLogged(MemDispenser *disp)
{
	if(disp==NULL)
		return;

	int totalAllocated=0;

	MemDispenserBlock *blockPtr=disp->block;

	while(blockPtr!=NULL)
		{
		MemDispenserBlock *prevPtr=blockPtr->prev;

		totalAllocated+=blockPtr->allocated;

		free(blockPtr);
		blockPtr=prevPtr;
		}


	if(totalAllocated>0)
		{
		LOG(LOG_INFO,"Freeing Dispenser %s: Total Allocated %i",disp->name,totalAllocated);
		}

	free(disp);
}



/*
void dispenserNukeFree(MemDispenser *disp, u8 val)
{
	if(disp==NULL)
		return;

	int totalAllocated=0;

	MemDispenserBlock *blockPtr=disp->block;

	while(blockPtr!=NULL)
		{
		MemDispenserBlock *prevPtr=blockPtr->prev;

		totalAllocated+=blockPtr->allocated;

		memset(&(blockPtr->data),val,blockPtr->allocated);
		free(blockPtr);
		blockPtr=prevPtr;
		}

	//LOG(LOG_INFO,"NukeFreeing disp: %s",disp->name);

	free(disp);
}
*/

void dispenserReset(MemDispenser *dispenser)
{
	MemDispenserBlock *blockPtr=dispenser->block;

	if(blockPtr!=NULL)
		{
		blockPtr->allocated=0;

		blockPtr=blockPtr->prev;

		while(blockPtr!=NULL)
			{
			MemDispenserBlock *prevPtr=blockPtr->prev;
			free(blockPtr);
			blockPtr=prevPtr;
			}
		}

	dispenser->block->prev=NULL;
	dispenser->allocated=0;

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
		LOG(LOG_CRITICAL,"Dispenser %s refusing to allocate",disp->name);

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
//		LOG(LOG_INFO,"Alloc of %i with %i padding using blocksize ",allocSize,padSize, disp->blocksize);

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
					LOG(LOG_INFO,"Warning: Block %s expanded to max: %i",disp->name,disp->maxBlocksize);
					blocksize=disp->maxBlocksize;
					break;
					}

				LOG(LOG_INFO,"Block %s expanded to: %i",disp->name,blocksize);
				}
			}

		MemDispenserBlock *newBlock=dispenserBlockAlloc(blocksize);

		newBlock->prev=disp->block;
		disp->block=newBlock;
		}

	allocSize+=padSize;

	MemDispenserBlock *block=disp->block;

	if(block->allocated+allocSize > block->blocksize)
		{
		LOG(LOG_INFO,"Dispenser Local overallocation detected - wanted %i\n",(int)allocSize);

		LOG(LOG_CRITICAL,"Dispenser %s refusing to allocate",disp->name);
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


