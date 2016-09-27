#ifndef __MEM_COLHEAP_H
#define __MEM_COLHEAP_H

#include "../common.h"

#define COLHEAP_MAX_ALLOC (1024*1024*4)

#define COLHEAP_MAX_GENERATIONS 4
#define COLHEAP_MAX_BLOCKS_PER_GENERATION 8

#define COLHEAP_MAX_BLOCKS (COLHEAP_MAX_GENERATIONS*COLHEAP_MAX_BLOCKS_PER_GENERATION)

#define COLHEAP_NUM_HEAPS SMER_DISPATCH_GROUPS
#define COLHEAP_ROOTSETS_PER_HEAP SMER_DISPATCH_GROUP_SLICES

#define COLHEAP_SPACE_MARGIN 0x4000L

typedef struct memColHeapConfigStr
{
	s64 blockSize;
	s32 blockShift;
	s32 blocksPerGeneration[COLHEAP_MAX_GENERATIONS];

	s32 startBlockPerGeneration[COLHEAP_MAX_GENERATIONS];
	s64 startOffsetPerGeneration[COLHEAP_MAX_GENERATIONS];
	s32 generationCount;
	s32 totalBlocks;
	s64 totalSize;
} MemColHeapConfig;


/* Structures for tracking Memory ColHeaps */

typedef struct memColHeapBlockStr {
	s64 size;
	s64 alloc;
	u8 *data;
} MemColHeapBlock;

typedef struct memColHeapRootSetStr
{
	u8 **rootPtrs;
	s32 numRoots;
} MemColHeapRootSet;

typedef struct memColHeapStr
{
	u32 configIndex;
	s32 youngGeneration;

	MemColHeapBlock blocks[COLHEAP_MAX_BLOCKS];
	MemColHeapBlock *currentYoungBlock;

	s32 genCurrentActiveBlockIndex[COLHEAP_MAX_GENERATIONS]; // Zero-based per generation

	MemColHeapRootSet roots[COLHEAP_ROOTSETS_PER_HEAP];

	u8 *heapData;
	//s64 peakAlloc;
	//s64 realloc;

	s32 (*itemSizeResolver)(u8 *item);
} MemColHeap;


typedef struct memColHeapRootSetQueueEntryStr
{
	u8 **dataPtr;
	s32 size;
	s32 index;
} MemColHeapRootSetQueueEntry;

typedef struct memColHeapRootSetQueueStr
{
	MemColHeapRootSet *rootSet;
	s32 rootNum;
	s32 rootPos; // Needed?
	s32 rootBlockOffsets[COLHEAP_MAX_BLOCKS];

	MemColHeapRootSetQueueEntry entries[];
} MemColHeapRootSetQueue;

typedef struct memColHeapRootSetExtractStr
{
	s32 currentPositions[COLHEAP_ROOTSETS_PER_HEAP];
	s32 endPositions[COLHEAP_ROOTSETS_PER_HEAP];

} MemColHeapRootSetExtract;


typedef struct memColHeapGarbageCollectionBlockStr
{
	s64 liveSize;
	s64 deadSize;

	s64 currentSpace;
	s64 compactSpace;

	s32 liveCount;

} MemColHeapGarbageCollectionBlock;

typedef struct memColHeapGarbageCollectionGenerationStr
{
	s64 liveSize;
	s64 deadSize;

	s64 currentSpace;
	s64 compactSpace;

	s32 liveCount;

} MemColHeapGarbageCollectionGeneration;

typedef struct memColHeapGarbageCollectionStr
{
	MemColHeapRootSetQueue *rootSetQueues[COLHEAP_ROOTSETS_PER_HEAP];
	s32 totalBlocks;
	s32 rootSetTotalCount;
	s64 rootSetTotalSize;
	s64 totalAlloc;

	MemColHeapGarbageCollectionBlock blocks[COLHEAP_MAX_BLOCKS];
	MemColHeapGarbageCollectionGeneration generations[COLHEAP_MAX_GENERATIONS];

	MemColHeapBlock newBlocks[COLHEAP_MAX_BLOCKS];

} MemColHeapGarbageCollection;


MemColHeap *colHeapAlloc(s32 (*itemSizeResolver)(u8 *item));
void colHeapFree(MemColHeap *colHeap);

void chRegisterRoots(MemColHeap *colHeap, int rootSetIndex, u8 **rootPtrs, s32 numRoots);

void *chAlloc(MemColHeap *colHeap, size_t size);
void *chRealloc(MemColHeap *colHeap, void *oldPtr, size_t oldSize, size_t size);


#endif
