
#include "common.h"




Slabocator *allocSlabocator(int minSizeShift, int maxSizeShift, int maxBiggestSlabs, int memTrackerId, int policy)
{
	if(minSizeShift>maxSizeShift)
		LOG(LOG_CRITICAL,"Slabocator: MinSize %i exceeds MaxSize %i",minSizeShift,maxSizeShift);

	if(maxBiggestSlabs<1)
		LOG(LOG_CRITICAL,"Slabocator: MaxBiggest must be at least one. Got %i",maxBiggestSlabs);

	int slabCount=maxSizeShift-minSizeShift+maxBiggestSlabs;

	Slabocator *slabocator=G_ALLOC(sizeof(Slabocator)+slabCount*sizeof(Slab), memTrackerId);

	slabocator->minSizeShift=minSizeShift;
	slabocator->maxSizeShift=maxSizeShift;
	slabocator->currentSizeShift=minSizeShift;

	slabocator->memTrackerId=memTrackerId;
	slabocator->policy=policy;

	slabocator->maxBiggestSlabs=maxBiggestSlabs;
	slabocator->slabCount=slabCount;

	return slabocator;
}


void freeSlabocator(Slabocator *slabocator)
{


}

Slab *slabAlloc(Slabocator *slabocator)
{

	return NULL;
}


Slab *slabCurrent(Slabocator *slabocator)
{

	return NULL;
}

void slabReset(Slabocator *slabocator)
{


}
