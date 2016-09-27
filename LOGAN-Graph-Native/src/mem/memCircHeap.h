#ifndef __MEM_CIRCHEAP_H
#define __MEM_CIRCHEAP_H

#include "../common.h"

#define CIRCHEAP_MAX_ALLOC (1024*1024*4)

#define CIRCHEAP_MAX_GENERATIONS 8
#define CIRCHEAP_LAST_GENERATION (CIRCHEAP_MAX_GENERATIONS-1)

#define CIRCHEAP_MAX_TAGS 256

#define CIRCHEAP_DEFAULT_GENERATIONS 2
//#define CIRCHEAP_DEFAULT_BLOCKSIZE 0x10000L // 64K per gen

#define CIRCHEAP_DEFAULT_BLOCKSIZE 0x100000L // 1M per gen
#define CIRCHEAP_MAX_BLOCKSIZE 0x100000000L // 4GB per gen

#define CIRCHEAP_BLOCK_OVERHEAD 0x1000L

#define CIRCHEAP_RECLAIM_SPLIT 64
#define CIRCHEAP_RECLAIM_SPLIT_SHIFT 6

#define CIRCHEAP_INDEXER_ENTRYALLOC 16

#define CH_HEADER_INVALID	0x00
#define CH_HEADER_CHUNK 0x80


typedef struct memCircHeapBlockStr
{
	struct memCircHeapBlockStr *next; // Pointer to larger block

	u8 *ptr;      		 // Pointer to block
	s64 size;			 // Total size of block
	s64 allocPosition; 	 // Current position for alloc
	s64 reclaimPosition; // Current position for reclaim
} MemCircHeapBlock;


typedef struct memCircHeapGenerationStr
{
	MemCircHeapBlock *allocBlock;
	MemCircHeapBlock *reclaimBlock;

	u8 allocCurrentChunkTag;	// Current allocation tag
	u8 *allocCurrentChunkPtr;	// Pointer to the chunk header

	u8 reclaimCurrentChunkTag;	// Current reclaim tag

	s64 curReclaimSizeLive;		// Total size of live data reclaimed
	s64 curReclaimSizeDead;		// Total size of dead data reclaimed

	s64 prevReclaimSizeLive;		// Total size of live data reclaimed
	s64 prevReclaimSizeDead;		// Total size of dead data reclaimed

} MemCircGeneration;



typedef struct memCircHeapChunkIndexEntryStr {
	s32 index;
	s32 size;
} MemCircHeapChunkIndexEntry;

typedef struct memCircHeapChunkIndexStr
{
	struct memCircHeapChunkIndexStr *next;

	u8 *heapStartPtr;
	u8 *heapEndPtr;

	u8 chunkTag;

	s64 reclaimed;
	s64 sizeLive;
	s64 sizeDead;

	u8 *newChunk;

	s32 entryAlloc;
	s32 entryCount;

	MemCircHeapChunkIndexEntry entries[];
} MemCircHeapChunkIndex;




typedef struct memCricHeapStr
{
	MemCircGeneration generations[CIRCHEAP_MAX_GENERATIONS]; // youngest first

	u8 **tagData[CIRCHEAP_MAX_TAGS];
	s32 tagDataLength[CIRCHEAP_MAX_TAGS];

	MemCircHeapChunkIndex *(*reclaimIndexer)(u8 *data, s64 targetAmount, u8 tag, u8 **tagData, s32 tagDataLength, MemDispenser *disp);
	void (*relocater)(MemCircHeapChunkIndex *reclaimIndex, u8 tag, u8 **tagData, s32 tagDataLength);

} MemCircHeap;



MemCircHeap *circHeapAlloc(MemCircHeapChunkIndex *(*reclaimIndexer)(u8 *heapDataPtr, s64 targetAmount, u8 tag, u8 **tagData, s32 tagDataLength, MemDispenser *disp),
		void (*relocater)(MemCircHeapChunkIndex *reclaimIndex, u8 tag, u8 **tagData, s32 tagDataLength));
void circHeapFree(MemCircHeap *circHeap);
void circHeapRegisterTagData(MemCircHeap *circHeap, u8 tag, u8 **data, s32 dataLength);

void *circAlloc(MemCircHeap *circHeap, size_t size, u8 tag);

#endif
