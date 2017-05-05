#ifndef __MEM_SLAB_H
#define __MEM_SLAB_H


// NukeFromOrbit - free all slabs
// InstaShrink -

#define SLAB_FREEPOLICY_NUKE_FROM_ORBIT 0
#define SLAB_FREEPOLICY_INSTA_SHRINK 1
#define SLAB_FREEPOLICY_INSTA_RACHET 2


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

	int maxSlabs;
	int slabCount;
	Slab *slabs[];
} Slabocator;



Slabocator *allocSlabocator(int minSizeShift, int maxSizeShift, int maxBiggestSlabs, int memTrackerId, int policy);
void freeSlabocator(Slabocator *slabocator);

Slab *slabAlloc(Slabocator *slabocator);
Slab *slabCurrent(Slabocator *slabocator);

void slabReset(Slabocator *slabocator);

#endif
