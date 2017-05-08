#ifndef __MEM_SLAB_H
#define __MEM_SLAB_H



// Insta Shrink - Use the original min size for new allocation
// Slow Shrink - Use the one smaller than the largest allocated
// Slow Rachet - Use the one larger then the smallest allocated
// Insta Rachet - Use the largest size yet allocated for new allocations

#define SLAB_FREEPOLICY_INSTA_SHRINK 0
//#define SLAB_FREEPOLICY_SLOW_SHRINK 1
//#define SLAB_FREEPOLICY_SLOW_RACHET 2
#define SLAB_FREEPOLICY_INSTA_RACHET 3

// NukeFromOrbit - free all slabs
#define SLAB_FREEPOLICY_NUKE_FROM_ORBIT 4


typedef struct slabStr
{
	int size;
	u8 *blockPtr;
} Slab;


typedef struct slabocatorStr
{
	int minSizeShift; // power of 2
	int maxSizeShift; // power of 2
	int currentSizeShift; // power of 2
	int memTrackerId;
	int policy;

	int maxBiggestSlabs;
	int slabCount;
	Slab *slabs[];
} Slabocator;



Slabocator *allocSlabocator(int minSizeShift, int maxSizeShift, int maxBiggestSlabs, int memTrackerId, int policy);
void freeSlabocator(Slabocator *slabocator);

Slab *slabAlloc(Slabocator *slabocator);
Slab *slabCurrent(Slabocator *slabocator);

void slabReset(Slabocator *slabocator);

#endif
