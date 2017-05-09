#ifndef __MEM_SLAB_H
#define __MEM_SLAB_H



// Initially only 'extremes' implemented:

// Insta Shrink - Use the original min size for new allocation
// Insta Rachet - Use the largest size yet allocated for new allocations

// Slow Shrink - Use the one smaller than the largest allocated
// Slow Rachet - Use the one larger then the smallest allocated


#define SLAB_FREEPOLICY_INSTA_SHRINK 0
//#define SLAB_FREEPOLICY_SLOW_SHRINK 1
//#define SLAB_FREEPOLICY_SLOW_RACHET 2
#define SLAB_FREEPOLICY_INSTA_RACHET 3

// NukeFromOrbit - free all slabs
#define SLAB_FREEPOLICY_NUKE_FROM_ORBIT 4


typedef struct slabStr
{
	s64 size;
	u8 *blockPtr;
} Slab;


typedef struct slabocatorStr
{
	s16 minSizeShift; // power of 2
	s16 maxSizeShift; // power of 2
	s16 memTrackerId;
	s16 policy;
	s16 numBiggestSlabs;
	s16 slabCount;
	s16 firstSlab;	// Since last reset
	s16 currentSlab;

	Slab slabs[];
} Slabocator;


void freeSlab(Slab *slab, int memTrackerId);

Slabocator *allocSlabocator(int minSizeShift, int maxSizeShift, int maxBiggestSlabs, int memTrackerId, int policy);
void freeSlabocator(Slabocator *slabocator);

Slab *slabAllocNext(Slabocator *slabocator);
Slab *slabGetCurrent(Slabocator *slabocator);

void slabReset(Slabocator *slabocator);

#endif
