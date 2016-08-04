#ifndef __MEM_PACKSTACK_H
#define __MEM_PACKSTACK_H

#include "../common.h"


/* Structures for tracking Memory PackStacks */

typedef struct memPackStackStr
{
	u32 currentSizeIndex;
	u32 currentSize;

	u32 peakAlloc;
	u32 realloc;

	u8 data[];
} MemPackStack;

typedef struct memPackStackRetainStr
{
	void *oldPtr;
	void *newPtr;
	s32 size;
	s32 index;

} MemPackStackRetain;


MemPackStack *packStackAlloc();
void packStackFree(MemPackStack *packStack);

void *psAlloc(MemPackStack *packStack, size_t size);
void *psRealloc(MemPackStack *packStack, void *oldPtr, size_t oldSize, size_t size);

MemPackStack *psCompact(MemPackStack *packStack, MemPackStackRetain *retains, s32 retainCount);

#endif
