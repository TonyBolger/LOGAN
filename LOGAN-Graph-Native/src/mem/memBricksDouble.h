#ifndef __MEM_BRICKS_DOUBLE_H
#define __MEM_BRICKS_DOUBLE_H


// This differs
#define DOUBLEBRICK_ALIGNMENT (65536*128)
#define DOUBLEBRICK_ALIGNMENT_MASK (DOUBLEBRICK_ALIGNMENT-1)

// This differs
#define DOUBLEBRICK_BRICKSIZE 128

#define DOUBLEBRICK_ALLOC_FLAGS_PER_CHUNK 1024
#define DOUBLEBRICK_ALLOC_BRICKS_PER_CHUNK (DOUBLEBRICK_ALLOC_FLAGS_PER_CHUNK*64)

#define DOUBLEBRICK_FLAGS_PER_CHUNK 1023
#define DOUBLEBRICK_BRICKS_PER_CHUNK (DOUBLEBRICK_FLAGS_PER_CHUNK*64)

// This differs
#define DOUBLEBRICK_SKIP_FLAGS_PER_CHUNK 1
#define DOUBLEBRICK_SKIP_BRICKS_PER_CHUNK (DOUBLEBRICK_SKIP_FLAGS_PER_CHUNK*64)

#define DOUBLEBRICK_PILE_MARGIN_PER_CHUNK (DOUBLEBRICK_FLAGS_PER_CHUNK*2)
#define DOUBLEBRICK_CHUNK_MARGIN_PER_CHUNK (DOUBLEBRICK_FLAGS_PER_CHUNK)

#define DOUBLEBRICK_DEFAULT_RESERVATION 64
#define DOUBLEBRICK_SCANPOSITION_LOCKED 0

typedef struct memDoubleBrickStr {
	u8 data[DOUBLEBRICK_BRICKSIZE];
} MemDoubleBrick;

typedef union {
	MemBrickChunkHeader header;
	u64 freeFlag[DOUBLEBRICK_ALLOC_FLAGS_PER_CHUNK];
	MemDoubleBrick bricks[DOUBLEBRICK_ALLOC_BRICKS_PER_CHUNK];
} MemDoubleBrickChunk;

typedef struct memDoubleBrickPileStr {
	MemDoubleBrickChunk **chunks;
	s32 memTrackId;
	s32 chunkLimit;
	s32 chunkCount;
	s32 lock;

	s32 scanChunk;
	s32 freeCount;

} MemDoubleBrickPile;

typedef struct memDoubleBrickAllocatorStr {
	MemDoubleBrickPile *pile;
	s32 pileResRequest;
	s32 pileResLeft;

	MemDoubleBrickChunk *chunk;
	u32 chunkIndex;
	s32 scanChunk;

	u32 scanFlag;

	s32 allocCount;
	u64 allocMask;
	s32 allocIndex;

} MemDoubleBrickAllocator;



void mbInitDoubleBrickPile(MemDoubleBrickPile *pile, s32 chunkCount, s32 chunkLimit, s32 memTrackId);
void mbFreeDoubleBrickPile(MemDoubleBrickPile *pile, char *pileName);

s32 mbGetFreeDoubleBrickPile(MemDoubleBrickPile *pile);
s32 mbGetFreeDoubleBrickChunk(MemDoubleBrickChunk *chunk);
s32 mbCheckDoubleBrickAvailability(MemDoubleBrickPile *pile, s32 brickCount);
s32 mbGetDoubleBrickPileCapacity(MemDoubleBrickPile *pile);

void mbShowDoubleBrickPileStatus(MemDoubleBrickPile *pile);

s32 mbInitDoubleBrickAllocator(MemDoubleBrickAllocator *alloc, MemDoubleBrickPile *pile, s32 pileResRequest);
void *mbDoubleBrickAllocate(MemDoubleBrickAllocator *alloc, u32 *brickIndexPtr);
void mbDoubleBrickAllocatorCleanup(MemDoubleBrickAllocator *alloc);

void *mbDoubleBrickFindByIndex(MemDoubleBrickPile *pile, u32 brickIndex);
void mbDoubleBrickFreeByIndex(MemDoubleBrickPile *pile, u32 brickIndex);


#endif
