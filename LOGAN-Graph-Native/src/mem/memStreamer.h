#ifndef __MEM_STREAMER_H
#define __MEM_STREAMER_H

#include "../common.h"


#define STREAMER_BLOCKSIZE (1024*256*1L)
#define STREAMER_MAX (1024*1024*1024*2L)

/* Structures for tracking Memory Streamers */

typedef struct memStreamerBlockStr
{
	struct memStreamerBlockStr *prev; // 8
	u32 blocksize; // 4
	u32 allocated; // 4
	u8 pad[48];
	u8 data[STREAMER_BLOCKSIZE];
} MemStreamerBlock;

typedef struct memStreamerStr
{
	struct memStreamerBlockStr *block;

	int allocated;

	void *currentPtr;
	u32 currentSize;
} MemStreamer;


MemStreamer *streamerAlloc();
void streamerFree(MemStreamer *strm);
//void streamerNukeFree(MemStreamer *strm, u8 val);
void streamerReset(MemStreamer *strm);

void sBegin(MemStreamer *strm, size_t estimatedSize);
void *sAlloc(MemStreamer *strm, size_t size);
void *sEnd(MemStreamer *strm);

#endif
