#ifndef __MEM_DISPENSER_H
#define __MEM_DISPENSER_H

#include "../common.h"


#define MEM_BUMPER_EXITCODE 42
#define MAX_NAME 100

//#define DISPENSER_BLOCKSIZE (1024*256*10*8)
#define DISPENSER_MAX (1024*1024*1024*2L)

#define DISPENSER_BLOCKSIZE_SMALL (1024*64)
#define DISPENSER_BLOCKSIZE_MEDIUM (1024*256)
#define DISPENSER_BLOCKSIZE_LARGE (1024*1024)
#define DISPENSER_BLOCKSIZE_HUGE (1024*1024*4)

/* Structures for tracking Memory Dispensers */

typedef struct memDispenserBlockStr
{
	struct memDispenserBlockStr *prev; // 8
	u32 blocksize; // 4
	u32 allocated; // 4
	u8 pad[48];
	u8 data[];
} MemDispenserBlock;

#define MAX_ALLOCATORS 32

typedef struct memDispenserStr
{
	const char *name;
	struct memDispenserBlockStr *block;
	u32 blocksize;
	u32 maxBlocksize;

	//int allocatorUsage[MAX_ALLOCATORS];
	int allocated;
} MemDispenser;


MemDispenser *dispenserAlloc(const char *name, u32 blocksize, u32 maxBlocksize);
void dispenserFree(MemDispenser *dispenser);
void dispenserFreeLogged(MemDispenser *dispenser);
//void dispenserNukeFree(MemDispenser *disp, u8 val);

void dispenserReset(MemDispenser *dispenser);

int dispenserSize(MemDispenser *disp);
void *dAlloc(MemDispenser *dispenser, size_t size);
void *dAllocLogged(MemDispenser *dispenser, size_t size);

void *dAllocQuadAligned(MemDispenser *disp, size_t size);
void *dAllocCacheAligned(MemDispenser *disp, size_t size);

#endif
