#ifndef __MEM_HEAP_CIRC_H
#define __MEM_HEAP_CIRC_H

#define CIRCHEAP_MAX_ALLOC_WARN (1024*1024*2)
#define CIRCHEAP_MAX_ALLOC (1024*1024*32)


#define CIRCHEAP_MAX_GENERATIONS 8
#define CIRCHEAP_LAST_GENERATION (CIRCHEAP_MAX_GENERATIONS-1)

#define CIRCHEAP_MAX_TAGS 256

#define CIRCHEAP_BLOCKSIZE_INCREMENT_SHIFT 5
#define CIRCHEAP_BLOCKSIZE_INCREMENT_STEPS (1<<CIRCHEAP_BLOCKSIZE_INCREMENT_SHIFT)
#define CIRCHEAP_BLOCKSIZE_INCREMENT_MASK (CIRCHEAP_BLOCKSIZE_INCREMENT_STEPS-1)

#define CIRCHEAP_BLOCKSIZE_BASE_INCREMENT 0x1000L // Increment is 4K - blocksize is Increment * CIRCHEAP_BLOCKSIZE_INCREMENT_STEPS

#define CIRCHEAP_MAX_BLOCKSIZE 0x100000000L // 4GB per gen


// For CIRCHEAP_BLOCKSIZE_INCREMENT_SHIFT = 3, 8 steps per doubling

// 0 -> 32K
// 1 -> 36K
// 2 -> 40K
// 3 -> 44K
// 4 -> 48K
// 5 -> 52K
// 6 -> 56K
// 7 -> 60K
// 8 -> 64K
// 16 -> 128K
// 24 -> 256K
// 32 -> 512K
// 40 -> 1024K
// 48 -> 2048K
// 56 -> 4096K
// 64 -> 8192K

// For CIRCHEAP_BLOCKSIZE_INCREMENT_SHIFT = 4, 16 steps per doubling

// 0 -> 64K
// 1 -> 68K
// 2 -> 72K
// 3 -> 76K
// 4 -> 80K
// 5 -> 84K
// 6 -> 88K
// 7 -> 92K
// 8 -> 96K
// 9 -> 100K
// 10 -> 104K
// 11 -> 108K
// 12 -> 112K
// 13 -> 116K
// 14 -> 120K
// 15 -> 124K
// 16 -> 128K

// 32 -> 256K
// 48 -> 512K
// 64 -> 1024K
// 80 -> 2048K
// 96 -> 4096K
// 112 -> 8192K

// For CIRCHEAP_BLOCKSIZE_INCREMENT_SHIFT = 5, 32 steps per doubling





// Allocation margin for GC - should be zero if all is good
//#define CIRCHEAP_BLOCK_OVERHEAD 0x1000L
#define CIRCHEAP_BLOCK_OVERHEAD 0x0L

#define CIRCHEAP_BLOCK_TAG_OVERHEAD 2

#define CIRCHEAP_RECLAIM_SPLIT 64
#define CIRCHEAP_RECLAIM_SPLIT_SHIFT 6

#define CIRCHEAP_RECLAIM_SPLIT_FULL 4
#define CIRCHEAP_RECLAIM_SPLIT_FULL_SHIFT 2


#define CIRCHEAP_INDEXER_ENTRYALLOC 16

#define CH_HEADER_INVALID	0x00
#define CH_HEADER_CHUNK 0x80

// Comment out to disable bumpers
//#define CH_BUMPER 0x42

typedef struct memCircHeapBlockStr
{
	struct memCircHeapBlockStr *next; // Pointer to larger block

	u8 *ptr;      		 // Pointer to block
	s64 size;			 // Total size of block
	s32 sizeIndex;		 // Index of total size
	s32 needsExpanding;  // Flag
	s64 allocPosition; 	 // Current position for alloc
	s64 reclaimLimit;    // Top Limit of reclaim. Set when alloc is wrapped, cleared when reclaim is wrapped
	s64 reclaimPosition; // Current position for reclaim

	s64 reclaimSizeLive;		// Total size of live data reclaimed
	s64 reclaimSizeDead;		// Total size of dead data reclaimed

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

	s64 prevReclaimSizeLive;		// Total size of live data reclaimed
	s64 prevReclaimSizeDead;		// Total size of dead data reclaimed

} MemCircGeneration;


typedef struct memCircHeapChunkIndexEntryStr {
	s32 index;
	s32 topindex; // -2, -1, 0-7
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

	MemDispenser *gcDisp;

} MemCircHeap;



void mhcHeapAlloc(MemCircHeap *circHeap,
		MemCircHeapChunkIndex *(*reclaimIndexer)(u8 *heapDataPtr, s64 targetAmount, u8 tag, u8 **tagData, s32 tagDataLength, s32 tagSearchOffset, MemDispenser *disp),
		void (*relocater)(MemCircHeapChunkIndex *reclaimIndex, u8 tag, u8 **tagData, s32 tagDataLength));
void mhcHeapFree(MemCircHeap *circHeap);
void mhcHeapRegisterTagData(MemCircHeap *circHeap, u8 tag, u8 **data, s32 dataLength);

void *mhcAlloc(MemCircHeap *circHeap, size_t size, u8 tag, s32 newTagOffset, s32 *oldTagOffset);

#endif
