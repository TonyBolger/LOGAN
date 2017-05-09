#ifndef __MEM_DISPENSER_H
#define __MEM_DISPENSER_H

#include "../common.h"


#define MEM_BUMPER_EXITCODE 42
#define MAX_NAME 100

//#define DISPENSER_BLOCKSIZE (1024*256*10*8)
#define DISPENSER_MAX (1024*1024*1024*2L)

#define DISPENSER_BLOCKSIZE_SMALL (1024*256)
#define DISPENSER_BLOCKSIZE_MEDIUM (1024*1024)
#define DISPENSER_BLOCKSIZE_LARGE (1024*1024*4)
#define DISPENSER_BLOCKSIZE_HUGE (1024*1024*256)

// 500bp per read, 20bytes per base (8 per smer, forward/reverse) -> 10000*10000
//#define DISPENSER_BLOCKSIZE_ROUTING_LOOKUP (500L*20*TR_INGRESS_BLOCKSIZE)

// 50 smers per read, 30 bytes per base (8 per smer, forward/reverse, 4 byte read index, 8 byte slice/index)
//#define DISPENSER_BLOCKSIZE_ROUTING_DISPATCH (50L*30*TR_INGRESS_BLOCKSIZE)

/* Structures for tracking Memory Dispensers */

typedef struct memDispenserStr
{
	Slabocator *slabocator;
	u8 *blockPtr;
	s64 blockSize;

	s64 allocated;
	s64 blockAllocated;
} MemDispenser;


MemDispenser *dispenserAlloc(s16 memTrackerId, s16 policy, u32 blocksize, u32 maxBlocksize);
void dispenserFree(MemDispenser *dispenser);

void dispenserReset(MemDispenser *dispenser);

int dispenserSize(MemDispenser *disp);
void *dAlloc(MemDispenser *dispenser, size_t size);
void *dAllocLogged(MemDispenser *dispenser, size_t size);

void *dAllocQuadAligned(MemDispenser *disp, size_t size);
void *dAllocCacheAligned(MemDispenser *disp, size_t size);

#endif
