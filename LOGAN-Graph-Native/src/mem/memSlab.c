
#include "common.h"


static void allocSlab(Slab *slab, int memTrackerId)
{
	if(slab->blockPtr!=NULL)
		LOG(LOG_CRITICAL,"Block already mapped");

	/*
	slab->blockPtr=mmap(NULL, slab->size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if(slab->blockPtr==MAP_FAILED)
		{
		char errBuf[ERRORBUF];
		strerror_r(errno,errBuf,ERRORBUF);

		LOG(LOG_CRITICAL,"Block mapping for size %li failed with %s",slab->size, errno);
		}
*/

	void *blockPtr=NULL;

	int err=hbw_posix_memalign(&blockPtr, 4096, slab->size);

	if(err)
		{
		char errBuf[ERRORBUF];
		strerror_r(err,errBuf,ERRORBUF);

		LOG(LOG_CRITICAL,"Block mapping for size %li failed with %s",slab->size, errBuf);
		}

	slab->blockPtr=blockPtr;

#ifdef FEATURE_ENABLE_MEMTRACK
	mtTrackAlloc(slab->size, memTrackerId);
#endif

}


void freeSlab(Slab *slab, int memTrackerId)
{
	if(slab->blockPtr==NULL)
		return;

	if(slab->blockPtr==MAP_FAILED)
		LOG(LOG_CRITICAL,"Block mapped failed");

	/*
	if(munmap(slab->blockPtr, slab->size))
		{
		char errBuf[ERRORBUF];
		strerror_r(errno,errBuf,ERRORBUF);

		LOG(LOG_CRITICAL,"Block unmap failed with %s",slab->size, errBuf);
		}
*/


	hbw_free(slab->blockPtr);

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

	slabocator->firstSlab=0;
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

	//LOG(LOG_INFO,"Slabocator: Created with %i %i %i (%i) for %s",minSizeShift, maxSizeShift, numBiggestSlabs, slabCount, MEMTRACKER_NAMES[memTrackerId]);

	return slabocator;
}


void freeSlabocator(Slabocator *slabocator)
{
	for(int i=0;i<slabocator->slabCount;i++)
		{
		if(slabocator->slabs[i].blockPtr!=NULL)
			freeSlab(slabocator->slabs+i, slabocator->memTrackerId);
		}

	G_FREE(slabocator, sizeof(Slabocator)+slabocator->slabCount*sizeof(Slab), slabocator->memTrackerId);
}

Slab *slabAllocNext(Slabocator *slabocator)
{
//	LOG(LOG_INFO,"Upsizing slabocator for %s",MEMTRACKER_NAMES[slabocator->memTrackerId]);

	int nextSlab=slabocator->currentSlab+1;

	if(nextSlab>=slabocator->slabCount)
		LOG(LOG_CRITICAL,"Slabocator %s reached slab limit %i", MEMTRACKER_NAMES[slabocator->memTrackerId], slabocator->slabCount);

	slabocator->currentSlab=nextSlab;

	Slab *slab=slabocator->slabs+nextSlab;
	if(slab->blockPtr==NULL)
		allocSlab(slab, slabocator->memTrackerId);

	return slab;
}


Slab *slabGetCurrent(Slabocator *slabocator)
{
	Slab *slab=slabocator->slabs+slabocator->currentSlab;
	if(slab->blockPtr==NULL)
		allocSlab(slab, slabocator->memTrackerId);

	return slab;
}


void slabReset(Slabocator *slabocator)
{
	s16 firstSlab=0;

	switch(slabocator->policy)
		{
		case SLAB_FREEPOLICY_INSTA_SHRINK:
			firstSlab=0;
			break;

		case SLAB_FREEPOLICY_INSTA_RACHET:
			firstSlab=slabocator->currentSlab;
			break;
		}

	slabocator->firstSlab=firstSlab;
	slabocator->currentSlab=firstSlab;

	for(int i=0;i<firstSlab;i++)
		freeSlab(slabocator->slabs+i,slabocator->memTrackerId);

	if(slabocator->policy & SLAB_FREEPOLICY_NUKE_FROM_ORBIT)
		freeSlab(slabocator->slabs+firstSlab,slabocator->memTrackerId);

	for(int i=firstSlab+1;i<slabocator->slabCount;i++)
		freeSlab(slabocator->slabs+i,slabocator->memTrackerId);
}
