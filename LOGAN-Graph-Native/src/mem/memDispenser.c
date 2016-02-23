
#include "../common.h"


static MemDispenserBlock *dispenserBlockAlloc()
{
	MemDispenserBlock *block=malloc(sizeof(MemDispenserBlock));

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

	return usrPtr;
}

