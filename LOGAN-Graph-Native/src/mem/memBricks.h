#ifndef __MEM_BRICKS_H
#define __MEM_BRICKS_H

typedef struct memBrickChunkHeaderStr {
	u16 freeCount;
	u16 flagPosition;	// 0 means allocator locked (to prevent cache ping-pong)
	u32 padding;
} MemBrickChunkHeader;


#endif
