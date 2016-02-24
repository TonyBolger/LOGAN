#ifndef __MEM_UTIL_H
#define __MEM_UTIL_H

#include "../common.h"


#define MEM_BUMPER_EXITCODE 42
#define MAX_NAME 100
#define DISPENSER_BLOCKSIZE (1024*1024*16L)
#define DISPENSER_MAX (1024*1024*1024*2L)


/* Structures for tracking Memory Dispensers */

typedef struct memDispenserBlockStr
{
	struct memDispenserBlockStr *prev;
	u32 blocksize;
	u32 allocated;
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
int dispenserSize(MemDispenser *disp);
void *dAlloc(MemDispenser *dispenser, size_t size);

#endif