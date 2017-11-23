#ifndef __MEM_BRICKS_H
#define __MEM_BRICKS_H


// void *ptr2 = (void *) ((uintptr_t) ptr1 & ~(uintptr_t) 0xfff)

#define SINGLEBRICK_ALIGNMENT 2097152
#define SINGLEBRICK_ALIGNMENT_MASK (SINGLEBRICK_ALIGNMENT-1)

#define SINGLEBRICK_BRICKSIZE 64
#define SINGLEBRICK_FLAGS_PER_CHUNK 1022
#define SINGLEBRICK_BRICKS_PER_CHUNK (SINGLEBRICK_FLAGS_PER_CHUNK*32)

#define SINGLEBRICK_PILE_MARGIN_PER_CHUNK (SINGLEBRICK_FLAGS_PER_CHUNK*2)
#define SINGLEBRICK_CHUNK_MARGIN_PER_CHUNK (SINGLEBRICK_FLAGS_PER_CHUNK)

#define SINGLEBRICK_DEFAULT_RESERVATION 32

#define SINGLEBRICK_SCANPOSITION_LOCKED -1

typedef struct memSingleBrickStr {
	u8 data[SINGLEBRICK_BRICKSIZE];
} MemSingleBrick;

typedef struct memSingleBrickChunkStr {
	u16 freeCount;
	u16 scanPosition;	// -1 means allocator locked (to prevent cache ping-pong)
	u32 padding;
	u32 freeFlag[SINGLEBRICK_FLAGS_PER_CHUNK];
	MemSingleBrick bricks[SINGLEBRICK_BRICKS_PER_CHUNK];
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
	s32 scanChunk;
	s16 scanPosition;

	s32 allocMask;
	s16 allocCount;

} MemSingleBrickAllocator;




#define DOUBLEBRICK_BRICKSIZE 128
#define DOUBLEBRICK_FLAGS_PER_CHUNK 1023
#define DOUBLEBRICK_BRICKS_PER_CHUNK (DOUBLEBRICK_FLAGS_PER_CHUNK*32)

typedef struct memDoubleBrickStr {
	u8 data[DOUBLEBRICK_BRICKSIZE];
} MemDoubleBrick;

typedef struct memDoubleBrickChunkStr {
	u16 freeCount;
	u16 scanPosition;
	u32 usageFlag[DOUBLEBRICK_FLAGS_PER_CHUNK];
	MemDoubleBrick bricks[DOUBLEBRICK_BRICKS_PER_CHUNK];
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

s32 mbGetFreeSingleBrickPile(MemSingleBrickPile *pile);
s32 mbGetFreeSingleBrickChunk(MemSingleBrickChunk *chunk);
s32 mbCheckSingleBrickAvailability(MemSingleBrickPile *pile, s32 brickCount);

void mbInitSingleBrickPile(MemSingleBrickPile *pile, s32 chunkCount, s32 chunkLimit, s32 memTrackId);

s32 mbInitSingleBrickAllocator(MemSingleBrickAllocator *alloc, MemSingleBrickPile *pile);

#endif
