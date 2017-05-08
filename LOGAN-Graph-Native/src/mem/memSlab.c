
#include "common.h"


//static
void allocSlab(Slab *slab, int memTrackerId)
{
	if(slab->blockPtr!=NULL)
		LOG(LOG_CRITICAL,"Block already mapped");

	slab->blockPtr=mmap(NULL, slab->size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if(slab->blockPtr==MAP_FAILED)
		{
		char errBuf[ERRORBUF];
		strerror_r(errno,errBuf,ERRORBUF);

		LOG(LOG_CRITICAL,"Block mapping for size %li failed with %s",slab->size, errno);
		}


#ifdef FEATURE_ENABLE_MEMTRACK
	mtTrackAlloc(slab->size, memTrackerId);
#endif

}

//static
void freeSlab(Slab *slab, int memTrackerId)
{
	if(slab->blockPtr==NULL)
		LOG(LOG_CRITICAL,"Block not mapped");

	if(slab->blockPtr==MAP_FAILED)
		LOG(LOG_CRITICAL,"Block mapped failed");

	if(munmap(slab->blockPtr, slab->size))
		{
		char errBuf[ERRORBUF];
		strerror_r(errno,errBuf,ERRORBUF);

		LOG(LOG_CRITICAL,"Block unmap failed with %s",slab->size, errno);
		}

	slab->blockPtr=NULL;

#ifdef FEATURE_ENABLE_MEMTRACK
	mtTrackFree(slab->size, memTrackerId);
#endif
}

Slabocator *allocSlabocator(int minSizeShift, int maxSizeShift, int numBiggestSlabs, int memTrackerId, int policy)
{
	if(minSizeShift>maxSizeShift)
		LOG(LOG_CRITICAL,"Slabocator: MinSize %i exceeds MaxSize %i",minSizeShift,maxSizeShift);

	if(numBiggestSlabs<1)
		LOG(LOG_CRITICAL,"Slabocator: NumBiggest must be at least one. Got %i",numBiggestSlabs);

	int slabCount=maxSizeShift-minSizeShift+numBiggestSlabs;

	Slabocator *slabocator=G_ALLOC(sizeof(Slabocator)+slabCount*sizeof(Slab), memTrackerId);

	slabocator->minSizeShift=minSizeShift;
	slabocator->maxSizeShift=maxSizeShift;

	slabocator->memTrackerId=memTrackerId;
	slabocator->policy=policy;

	slabocator->numBiggestSlabs=numBiggestSlabs;
	slabocator->slabCount=slabCount;

	slabocator->currentSlab=0;

	s64 size=2<<minSizeShift;
	int slabIndex=0;

	for(int i=minSizeShift;i<maxSizeShift;i++)
		{
		slabocator->slabs[slabIndex].blockPtr=NULL;
		slabocator->slabs[slabIndex].size=size;

		size<<=1;
		slabIndex++;
		}

	for(int i=0;i<numBiggestSlabs;i++)
		{
		slabocator->slabs[slabIndex].blockPtr=NULL;
		slabocator->slabs[slabIndex].size=size;
		slabIndex++;
		}

	return slabocator;
}


void freeSlabocator(Slabocator *slabocator)
{
	for(int i=0;i<slabocator->slabCount;i++)
		{
		if(slabocator->slabs[i].blockPtr!=NULL)
			freeSlab(slabocator->slabs[i], slabocator->memTrackerId);
		}

	G_FREE(slabocator, sizeof(Slabocator)+slabocator->slabCount*sizeof(Slab), slabocator->memTrackerId);
}

Slab *slabAllocNext(Slabocator *slabocator)
{
	int nextSlab=slabocator->currentSlab;

	if(nextSlab>=slabocator->slabCount)
		{

		}

	return NULL;
}


Slab *slabGetCurrent(Slabocator *slabocator)
{

	return NULL;
}

void slabReset(Slabocator *slabocator)
{


}
