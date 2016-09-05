#ifndef __MEM_COLHEAP_H
#define __MEM_COLHEAP_H

#include "../common.h"

#define COLHEAP_MAX_ALLOC (1024*1024)

#define COLHEAP_MAX_GENERATIONS 4
#define COLHEAP_MAX_BLOCKS_PER_GENERATION 8

typedef struct memColHeapConfigStr
{
	s64 blockSize;
	s32 blocksPerGeneration[COLHEAP_MAX_GENERATIONS];

	s32 startOffsetPerGeneration[COLHEAP_MAX_GENERATIONS];
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

typedef struct memColHeapRootStr
{
	u8 **rootPtrs;
	s32 numRoots;
} MemColHeapRoot;

typedef struct memColHeapStr
{
	u32 configIndex;

	s64 peakAlloc;
	s64 realloc;

	MemColHeapBlock blocks[COLHEAP_MAX_GENERATIONS][COLHEAP_MAX_BLOCKS_PER_GENERATION];
	MemColHeapBlock *currentYoungBlock;

	s32 genBlockIndex[COLHEAP_MAX_GENERATIONS];

	MemColHeapRoot roots[SMER_DISPATCH_GROUP_SLICES];

	long (*itemSizeResolver)(u8 *item);
} MemColHeap;





MemColHeap *colHeapAlloc(long (*itemSizeResolver)(u8 *item));
void colHeapFree(MemColHeap *colHeap);

void chRegisterRoots(MemColHeap *colHeap, int rootSetIndex, u8 **rootPtrs, s32 numRoots);

void *chAlloc(MemColHeap *colHeap, size_t size);
void *chRealloc(MemColHeap *colHeap, void *oldPtr, size_t oldSize, size_t size);


#endif
