
#include "../common.h"

#define QUAD_ALIGNMENT_MASK 7
#define QUAD_ALIGNMENT_SIZE 8

#define CACHE_ALIGNMENT_MASK 63
#define CACHE_ALIGNMENT_SIZE 64


static MemDispenserBlock *dispenserBlockAlloc()
{
	MemDispenserBlock *block=memalign(CACHE_ALIGNMENT_SIZE, sizeof(MemDispenserBlock));

	block->prev=NULL;
	block->allocated=0;
	block->blocksize=DISPENSER_BLOCKSIZE;

	return block;
}


MemDispenser *dispenserAlloc(const char *name)
{
	MemDispenser *disp=malloc(sizeof(MemDispenser));

	disp->name=name;
	disp->block=dispenserBlockAlloc();
	disp->allocated=0;

	int i=0;
	for(i=0;i<MAX_ALLOCATORS;i++)
		disp->allocatorUsage[i]=0;

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

	/*
	if(totalAllocated>100000000)
		{
		fprintf(stderr,"Total Allocated %i\n",totalAllocated);
		int i;

		for(i=0;i<MAX_ALLOCATORS;i++)
			fprintf(stderr,"Allocated (%i) %i\n",i,disp->allocatorUsage[i]);
		}
*/

	free(disp);
}

int dispenserSize(MemDispenser *disp)
{
	return disp->allocated;
}


void *dAlloc(MemDispenser *disp, size_t size)
{
	if(size==0)
		return NULL;

	if(disp->allocated>DISPENSER_MAX)
		{
		fprintf(stderr,"Dispenser Overallocation detected - wanted %i\n",(int)size);
		int i;
		for(i=0;i<MAX_ALLOCATORS;i++)
			fprintf(stderr,"%i %i\n",i,disp->allocatorUsage[i]);

		return NULL;
		}

	//if(size>10000)
		//fprintf(stderr,"Large Alloc %i\n",(int)size);

	size_t allocSize=size;

	if(disp->block == NULL || disp->block->allocated+allocSize > disp->block->blocksize)
		{
		MemDispenserBlock *newBlock=dispenserBlockAlloc();

		newBlock->prev=disp->block;
		disp->block=newBlock;
		}

	MemDispenserBlock *block=disp->block;

	if(block->allocated+size > block->blocksize)
		{
		fprintf(stderr,"Dispenser Local overallocation detected - wanted %i\n",(int)size);
		return NULL;
		}

	void *usrPtr=block->data+block->allocated;

	block->allocated+=allocSize;
	disp->allocated+=allocSize;

//	LOG(LOG_INFO,"Alloced %i at %p",allocSize,usrPtr);

	return usrPtr;
}


void *dAllocQuadAligned(MemDispenser *disp, size_t size)
{
	if(size==0)
		return NULL;

	if(disp->allocated>DISPENSER_MAX)
		{
		fprintf(stderr,"Dispenser Overallocation detected - wanted %i\n",(int)size);
		int i;
		for(i=0;i<MAX_ALLOCATORS;i++)
			fprintf(stderr,"%i %i\n",i,disp->allocatorUsage[i]);

		return NULL;
		}

	//if(size>10000)
		//fprintf(stderr,"Large Alloc %i\n",(int)size);

	size_t allocSize=size;

	if(disp->block == NULL || disp->block->allocated+allocSize+QUAD_ALIGNMENT_MASK > disp->block->blocksize)
		{
		MemDispenserBlock *newBlock=dispenserBlockAlloc();

		newBlock->prev=disp->block;
		disp->block=newBlock;
		}

	MemDispenserBlock *block=disp->block;

	int padSize=0;
	int offset=block->allocated & QUAD_ALIGNMENT_MASK;
	if(offset>0)
		{
		padSize=QUAD_ALIGNMENT_SIZE-offset;

		block->allocated+=padSize;
		disp->allocated+=padSize;
		}

	if(block->allocated+size > block->blocksize)
		{
		fprintf(stderr,"Dispenser Local overallocation detected - wanted %i\n",(int)size);
		return NULL;
		}

	void *usrPtr=block->data+block->allocated;

	block->allocated+=allocSize;
	disp->allocated+=allocSize;

	//LOG(LOG_INFO,"Alloced quad aligned %i at %p",allocSize,usrPtr);

	return usrPtr;
}



void *dAllocCacheAligned(MemDispenser *disp, size_t size)
{
	if(size==0)
		return NULL;

	if(disp->allocated>DISPENSER_MAX)
		{
		fprintf(stderr,"Dispenser Overallocation detected - wanted %i\n",(int)size);
		int i;
		for(i=0;i<MAX_ALLOCATORS;i++)
			fprintf(stderr,"%i %i\n",i,disp->allocatorUsage[i]);

		return NULL;
		}

	//if(size>10000)
		//fprintf(stderr,"Large Alloc %i\n",(int)size);

	size_t allocSize=size;

	if(disp->block == NULL || disp->block->allocated+allocSize+CACHE_ALIGNMENT_MASK > disp->block->blocksize)
		{
		MemDispenserBlock *newBlock=dispenserBlockAlloc();

		newBlock->prev=disp->block;
		disp->block=newBlock;
		}

	MemDispenserBlock *block=disp->block;

	int padSize=0;
	int offset=block->allocated & CACHE_ALIGNMENT_MASK;
	if(offset>0)
		{
		padSize=CACHE_ALIGNMENT_SIZE-offset;

		block->allocated+=padSize;
		disp->allocated+=padSize;
		}

	if(block->allocated+size > block->blocksize)
		{
		fprintf(stderr,"Dispenser Local overallocation detected - wanted %i\n",(int)size);
		return NULL;
		}

	void *usrPtr=block->data+block->allocated;

	block->allocated+=allocSize;
	disp->allocated+=allocSize;

	//LOG(LOG_INFO,"Alloced cache aligned %i at %p",allocSize,usrPtr);

	return usrPtr;
}


