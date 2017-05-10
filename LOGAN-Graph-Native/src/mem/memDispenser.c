
#include "common.h"


static void dispenserBlockInit(MemDispenser *disp, Slab *slab)
{
	disp->blockPtr=slab->blockPtr;
	disp->blockSize=slab->size;
	disp->blockAllocated=0;
}


MemDispenser *dispenserAlloc(s16 memTrackerId, s16 policy, u32 baseBlockSize, u32 maxBlocksize)
{
//	LOG(LOG_INFO,"Allocating Dispenser: %i",memTrackerId);
	MemDispenser *disp=G_ALLOC(sizeof(MemDispenser),MEMTRACKID_DISPENSER);

	if(disp==NULL)
		LOG(LOG_CRITICAL,"Failed to alloc dispenser");

	s32 minSizeShift=31-__builtin_clz(nextPowerOf2_32(baseBlockSize));
	u32 maxSizeShift=31-__builtin_clz(nextPowerOf2_32(maxBlocksize));

//	LOG(LOG_INFO,"Sizes %i (%i) %i (%i)",baseBlockSize,minSizeShift, maxBlocksize, maxSizeShift);

	Slabocator *slabocator=allocSlabocator(minSizeShift, maxSizeShift, 1, memTrackerId, policy);
	disp->slabocator=slabocator;

	dispenserBlockInit(disp, slabGetCurrent(slabocator));

	disp->allocated=0;

	return disp;
}

void dispenserFree(MemDispenser *disp)
{
	if(disp==NULL)
		return;

	freeSlabocator(disp->slabocator);

	gFree(disp, sizeof(MemDispenser), MEMTRACKID_DISPENSER);
}


void dispenserReset(MemDispenser *disp)
{
	//LOG(LOG_INFO,"DispReset %s",MEMTRACKER_NAMES[disp->slabocator->memTrackerId]);

	slabReset(disp->slabocator);
	dispenserBlockInit(disp,slabGetCurrent(disp->slabocator));

	disp->allocated=0;

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
		LOG(LOG_CRITICAL,"Dispenser %s refusing to allocate",MEMTRACKER_NAMES[disp->slabocator->memTrackerId]);

		return NULL;
		}

	int padSize=0;

	if(alignmentMask>0)
		{
		int offset=disp->blockAllocated & alignmentMask;
		if(offset>0)
			padSize=1+alignmentMask-offset;
		}

//	if(disp->block != NULL)

	//LOG(LOG_INFO,"Alloc of %i with %i padding",allocSize,padSize);

	int tempAllocSize=allocSize+padSize;

	if(disp->blockPtr == NULL)
		LOG(LOG_CRITICAL,"Dispenser blockPtr unexpectedly NULL");

	while(disp->blockAllocated+tempAllocSize > disp->blockSize)
		{
		padSize=0;

		dispenserBlockInit(disp, slabAllocNext(disp->slabocator));
		//LOG(LOG_INFO,"Block %s expanded to: %i",MEMTRACKER_NAMES[disp->slabocator->memTrackerId],disp->blockSize);
		}

	allocSize+=padSize;

	void *usrPtr=disp->blockPtr+disp->blockAllocated+padSize;

	disp->blockAllocated+=allocSize;
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


