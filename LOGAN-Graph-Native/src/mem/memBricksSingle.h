#ifndef __MEM_BRICKS_SINGLE_H
#define __MEM_BRICKS_SINGLE_H


// SingleBlock is 4M
#define SINGLEBRICK_ALIGNMENT (65536*64)
#define SINGLEBRICK_ALIGNMENT_MASK (SINGLEBRICK_ALIGNMENT-1)

#define SINGLEBRICK_BRICKSIZE 64

#define SINGLEBRICK_ALLOC_FLAGS_PER_CHUNK 1024
#define SINGLEBRICK_ALLOC_BRICKS_PER_CHUNK (SINGLEBRICK_ALLOC_FLAGS_PER_CHUNK*64)

#define SINGLEBRICK_FLAGS_PER_CHUNK 1022
#define SINGLEBRICK_BRICKS_PER_CHUNK (SINGLEBRICK_FLAGS_PER_CHUNK*64)

#define SINGLEBRICK_SKIP_FLAGS_PER_CHUNK 2
#define SINGLEBRICK_SKIP_BRICKS_PER_CHUNK (SINGLEBRICK_SKIP_FLAGS_PER_CHUNK*64)

#define SINGLEBRICK_PILE_MARGIN_PER_CHUNK (SINGLEBRICK_FLAGS_PER_CHUNK*2)
#define SINGLEBRICK_CHUNK_MARGIN_PER_CHUNK (SINGLEBRICK_FLAGS_PER_CHUNK)

#define SINGLEBRICK_DEFAULT_RESERVATION 64
#define SINGLEBRICK_SCANPOSITION_LOCKED 0



typedef struct memSingleBrickStr {
	u8 data[SINGLEBRICK_BRICKSIZE];
} MemSingleBrick;

typedef union {
	MemBrickChunkHeader header;
	u64 freeFlag[SINGLEBRICK_ALLOC_FLAGS_PER_CHUNK];
	MemSingleBrick bricks[SINGLEBRICK_ALLOC_BRICKS_PER_CHUNK];
} MemSingleBrickChunk;

typedef struct memSingleBrickPileStr {
	MemSingleBrickChunk **chunks;
	s32 memTrackId;
	s32 chunkLimit;
	s32 chunkCount;
	s32 lock;

	s32 scanChunk;
	s32 freeCount;

} MemSingleBrickPile;


typedef struct memSingleBrickAllocatorStr {
	MemSingleBrickPile *pile;
	s32 pileResRequest;
	s32 pileResLeft;

	MemSingleBrickChunk *chunk;
	u32 chunkIndex;
	s32 scanChunk;

	u32 scanFlag;

	s32 allocCount;
	u64 allocMask;
	s32 allocIndex;

} MemSingleBrickAllocator;



void mbInitSingleBrickPile(MemSingleBrickPile *pile, s32 chunkCount, s32 chunkLimit, s32 memTrackId);
void mbFreeSingleBrickPile(MemSingleBrickPile *pile, char *pileName);

s32 mbGetFreeSingleBrickPile(MemSingleBrickPile *pile);
s32 mbGetFreeSingleBrickChunk(MemSingleBrickChunk *chunk);
s32 mbCheckSingleBrickAvailability(MemSingleBrickPile *pile, s32 brickCount);
s32 mbGetSingleBrickPileCapacity(MemSingleBrickPile *pile);

void mbShowSingleBrickPileStatus(MemSingleBrickPile *pile);

s32 mbInitSingleBrickAllocator(MemSingleBrickAllocator *alloc, MemSingleBrickPile *pile, s32 pileResRequest);
void *mbSingleBrickAllocate(MemSingleBrickAllocator *alloc, u32 *brickIndexPtr);
void mbSingleBrickAllocatorCleanup(MemSingleBrickAllocator *alloc);

void *mbSingleBrickFindByIndex(MemSingleBrickPile *pile, u32 brickIndex);
void mbSingleBrickFreeByIndex(MemSingleBrickPile *pile, u32 brickIndex);

#endif
