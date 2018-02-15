#ifndef __MEM_BRICKS_H
#define __MEM_BRICKS_H


// void *ptr2 = (void *) ((uintptr_t) ptr1 & ~(uintptr_t) 0xfff)

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


typedef struct memBrickChunkHeaderStr {
	u16 freeCount;
	u16 flagPosition;	// 0 means allocator locked (to prevent cache ping-pong)
	u32 padding;
} MemBrickChunkHeader;

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




void mbInitSingleBrickPile(MemSingleBrickPile *pile, s32 chunkCount, s32 chunkLimit, s32 memTrackId);
void mbFreeSingleBrickPile(MemSingleBrickPile *pile, char *pileName);

void mbInitDoubleBrickPile(MemDoubleBrickPile *pile, s32 chunkCount, s32 chunkLimit, s32 memTrackId);
void mbFreeDoubleBrickPile(MemDoubleBrickPile *pile, char *pileName);

s32 mbGetFreeSingleBrickPile(MemSingleBrickPile *pile);
s32 mbGetFreeSingleBrickChunk(MemSingleBrickChunk *chunk);
s32 mbCheckSingleBrickAvailability(MemSingleBrickPile *pile, s32 brickCount);
s32 mbGetSingleBrickPileCapacity(MemSingleBrickPile *pile);

s32 mbGetFreeDoubleBrickPile(MemDoubleBrickPile *pile);
s32 mbGetFreeDoubleBrickChunk(MemDoubleBrickChunk *chunk);
s32 mbCheckDoubleBrickAvailability(MemDoubleBrickPile *pile, s32 brickCount);
s32 mbGetDoubleBrickPileCapacity(MemDoubleBrickPile *pile);

s32 mbInitSingleBrickAllocator(MemSingleBrickAllocator *alloc, MemSingleBrickPile *pile, s32 pileResRequest);
void *mbSingleBrickAllocate(MemSingleBrickAllocator *alloc, u32 *brickIndexPtr);
void mbSingleBrickAllocatorCleanup(MemSingleBrickAllocator *alloc);

void *mbSingleBrickFindByIndex(MemSingleBrickPile *pile, u32 brickIndex);
void mbSingleBrickFreeByIndex(MemSingleBrickPile *pile, u32 brickIndex);


s32 mbInitDoubleBrickAllocator(MemDoubleBrickAllocator *alloc, MemDoubleBrickPile *pile, s32 pileResRequest);
void *mbDoubleBrickAllocate(MemDoubleBrickAllocator *alloc, u32 *brickIndexPtr);
void mbDoubleBrickAllocatorCleanup(MemDoubleBrickAllocator *alloc);

void *mbDoubleBrickFindByIndex(MemDoubleBrickPile *pile, u32 brickIndex);
void mbDoubleBrickFreeByIndex(MemDoubleBrickPile *pile, u32 brickIndex);


#endif
