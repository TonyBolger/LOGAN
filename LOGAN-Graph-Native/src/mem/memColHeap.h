#ifndef __MEM_COLHEAP_H
#define __MEM_COLHEAP_H

#include "../common.h"

#define COLHEAP_MAX_GENERATIONS 4
#define COLHEAP_MAX_BLOCKS_PER_GENERATION 8
#define COLHEAP_MAX_BLOCKS (COLHEAP_MAX_GENERATIONS*COLHEAP_MAX_BLOCKS_PER_GENERATION)

typedef struct memColHeapConfigStr
{
	s64 blockSize;
	s32 blocksPerGeneration[COLHEAP_MAX_GENERATIONS];

	s32 generationStartBlock[COLHEAP_MAX_GENERATIONS];
	s32 generationCount;
	s32 totalBlocks;
	s64 totalSize;
} MemColHeapConfig;


/* Structures for tracking Memory PackStacks */

typedef struct memColHeapBlockStr {
	s64 size;
	s64 alloc;
	u8 *data;
} MemColHeapBlock;

typedef struct memColHeapStr
{
	u32 configIndex;

	s64 peakAlloc;
	s64 realloc;

	MemColHeapBlock blocks[COLHEAP_MAX_BLOCKS];
	MemColHeapBlock *currentYoungBlock;

	s32 genBlockIndex[COLHEAP_MAX_GENERATIONS];

	u8 *data;
} MemColHeap;





MemColHeap *colHeapAlloc();
void colHeapFree(MemColHeap *colHeap);

void *chAlloc(MemColHeap *colHeap, size_t size);
void *chRealloc(MemColHeap *colHeap, void *oldPtr, size_t oldSize, size_t size);


#endif
