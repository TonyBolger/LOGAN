#ifndef __MEM_UTIL_H
#define __MEM_UTIL_H

#include "../common.h"


#define MEM_BUMPER_EXITCODE 42
#define MAX_NAME 100
#define DISPENSER_BLOCKSIZE (1024*256*1L)
#define DISPENSER_MAX (1024*1024*1024*2L)


/* Structures for tracking Memory Dispensers */

typedef struct memDispenserBlockStr
{
	struct memDispenserBlockStr *prev; // 8
	u32 blocksize; // 4
	u32 allocated; // 4
	u8 pad[48];
	u8 data[DISPENSER_BLOCKSIZE];
} MemDispenserBlock;

#define MAX_ALLOCATORS 32

typedef struct memDispenserStr
{
	const char *name;
	struct memDispenserBlockStr *block;

	int allocatorUsage[MAX_ALLOCATORS];
	int allocated;
} MemDispenser;


MemDispenser *dispenserAlloc(const char *name);
void dispenserFree(MemDispenser *dispenser);
//void dispenserNukeFree(MemDispenser *disp, u8 val);

void dispenserReset(MemDispenser *dispenser);

int dispenserSize(MemDispenser *disp);
void *dAlloc(MemDispenser *dispenser, size_t size);

void *dAllocQuadAligned(MemDispenser *disp, size_t size);
void *dAllocCacheAligned(MemDispenser *disp, size_t size);

#endif
