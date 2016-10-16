#ifndef __MEM_CIRCHEAP_H
#define __MEM_CIRCHEAP_H

#include "../common.h"

#define CIRCHEAP_MAX_ALLOC (1024*1024*4)

#define CIRCHEAP_MAX_GENERATIONS 8
#define CIRCHEAP_LAST_GENERATION (CIRCHEAP_MAX_GENERATIONS-1)

#define CIRCHEAP_MAX_TAGS 256


#define CIRCHEAP_DEFAULT_BLOCKSIZE_INCREMENT 0x2000L // blocksize is * 8

//#define CIRCHEAP_DEFAULT_BLOCKSIZE 0x10000L // 64K per gen

//#define CIRCHEAP_DEFAULT_BLOCKSIZE 0x4A000L // 256+80?K * 6 * 64

#define CIRCHEAP_DEFAULT_BLOCKSIZE 0x100000L

#define CIRCHEAP_MAX_BLOCKSIZE 0x100000000L // 4GB per gen

#define CIRCHEAP_BLOCK_OVERHEAD 0x1000L
#define CIRCHEAP_BLOCK_TAG_OVERHEAD 2

#define CIRCHEAP_RECLAIM_SPLIT 64
#define CIRCHEAP_RECLAIM_SPLIT_SHIFT 6

#define CIRCHEAP_RECLAIM_SPLIT_FULL 4
#define CIRCHEAP_RECLAIM_SPLIT_FULL_SHIFT 2


#define CIRCHEAP_INDEXER_ENTRYALLOC 16

#define CH_HEADER_INVALID	0x00
#define CH_HEADER_CHUNK 0x80


typedef struct memCircHeapBlockStr
{
	struct memCircHeapBlockStr *next; // Pointer to larger block

	u8 *ptr;      		 // Pointer to block
	s64 size;			 // Total size of block
	s32 sizeIndex;		 // Index of total size
	s32 needsExpanding;  // Flag
	s64 allocPosition; 	 // Current position for alloc
	s64 reclaimPosition; // Current position for reclaim
} MemCircHeapBlock;


typedef struct memCircHeapGenerationStr
{
	MemCircHeapBlock *allocBlock;
	MemCircHeapBlock *reclaimBlock;

	u8 allocCurrentChunkTag;	    // Current allocation tag
	s32 allocCurrentChunkTagOffset; // Current offset in tag data
	u8 *allocCurrentChunkPtr;	    // Pointer to the chunk header

	u8 reclaimCurrentChunkTag;	      // Current reclaim tag
	s32 reclaimCurrentChunkTagOffset; // Current offset in tag data

	s64 curReclaimSizeLive;		// Total size of live data reclaimed
	s64 curReclaimSizeDead;		// Total size of dead data reclaimed

	s64 prevReclaimSizeLive;		// Total size of live data reclaimed
	s64 prevReclaimSizeDead;		// Total size of dead data reclaimed

} MemCircGeneration;



typedef struct memCircHeapChunkIndexEntryStr {
	s32 index;
	s32 subindex;
	s32 size;
} MemCircHeapChunkIndexEntry;

typedef struct memCircHeapChunkIndexStr
{
	struct memCircHeapChunkIndexStr *next;

	u8 *heapStartPtr;
	u8 *heapEndPtr;

	u8 chunkTag;
	s32 firstLiveTagOffset;
	s32 lastLiveTagOffset;
	s32 lastScannedTagOffset;

	s64 reclaimed;
	s64 sizeLive;
	s64 sizeDead;

	u8 *newChunk;
	s32 newChunkOldTagOffset;

	s32 entryAlloc;
	s32 entryCount;

	MemCircHeapChunkIndexEntry entries[];
} MemCircHeapChunkIndex;




typedef struct memCricHeapStr
{
	MemCircGeneration generations[CIRCHEAP_MAX_GENERATIONS]; // youngest first

	u8 **tagData[CIRCHEAP_MAX_TAGS];
	s32 tagDataLength[CIRCHEAP_MAX_TAGS];

	MemCircHeapChunkIndex *(*reclaimIndexer)(u8 *data, s64 targetAmount, u8 tag, u8 **tagData, s32 tagDataLength, s32 tagSearchOffset, MemDispenser *disp);
	void (*relocater)(MemCircHeapChunkIndex *reclaimIndex, u8 tag, u8 **tagData, s32 tagDataLength);

} MemCircHeap;



MemCircHeap *circHeapAlloc(MemCircHeapChunkIndex *(*reclaimIndexer)(u8 *heapDataPtr, s64 targetAmount, u8 tag, u8 **tagData, s32 tagDataLength, s32 tagSearchOffset, MemDispenser *disp),
		void (*relocater)(MemCircHeapChunkIndex *reclaimIndex, u8 tag, u8 **tagData, s32 tagDataLength));
void circHeapFree(MemCircHeap *circHeap);
void circHeapRegisterTagData(MemCircHeap *circHeap, u8 tag, u8 **data, s32 dataLength);

void *circAlloc(MemCircHeap *circHeap, size_t size, u8 tag, s32 newTagOffset, s32 *oldTagOffset);

#endif
