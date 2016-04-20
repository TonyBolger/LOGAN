
#include "../common.h"


static MemStreamerBlock *dispenserBlockAlloc()
{
	MemStreamerBlock *block=memalign(CACHE_ALIGNMENT_SIZE, sizeof(MemStreamerBlock));

	if(block==NULL)
		{
		LOG(LOG_INFO,"Failed to alloc dispenser block");
		exit(1);
		}

	block->prev=NULL;
	block->allocated=0;
	block->blocksize=STREAMER_BLOCKSIZE;

	//memset(block->data,0,DISPENSER_BLOCKSIZE);

	return block;
}


MemStreamer *streamerAlloc()
{
	MemStreamer *strm=malloc(sizeof(MemStreamer));

	if(strm==NULL)
		{
		LOG(LOG_INFO,"Failed to alloc streamer");
		exit(1);
		}

	strm->block=dispenserBlockAlloc();
	strm->allocated=0;

	strm->currentPtr=NULL;
	strm->currentSize=0;

	return strm;
}


void streamerFree(MemStreamer *strm)
{
	if(strm==NULL)
		return;

	int totalAllocated=0;

	MemStreamerBlock *blockPtr=strm->block;

	while(blockPtr!=NULL)
		{
		MemStreamerBlock *prevPtr=blockPtr->prev;

		totalAllocated+=blockPtr->allocated;

		free(blockPtr);
		blockPtr=prevPtr;
		}

	free(strm);
}

//void streamerNukeFree(MemStreamer *strm, u8 val)


void streamerReset(MemStreamer *strm)
{
	MemStreamerBlock *blockPtr=strm->block;

	if(blockPtr!=NULL)
		{
		blockPtr->allocated=0;

		blockPtr=blockPtr->prev;

		while(blockPtr!=NULL)
			{
			MemDispenserBlock *prevPtr=blockPtr->prev;
			free(blockPtr);
			blockPtr=prevPtr;
			}
		}

	strm->block->prev=NULL;
	strm->allocated=0;

	strm->currentPtr=NULL;
	strm->currentSize=0;
}


void sBegin(MemStreamer *strm, size_t estimatedSize)
{
	if(strm->block == NULL || strm->block->allocated+estimatedSize > strm->block->blocksize)
		{
		MemStreamerBlock *newBlock=streamerBlockAlloc();

		newBlock->prev=strm->block;
		strm->block=newBlock;
		}

	strm->currentPtr=strm->block->data+strm->block->allocated;
	strm->currentSize=0;
}


void *sAlloc(MemStreamer *strm, size_t size)
{
	if(strm->currentPtr==NULL)
		{
		LOG(LOG_INFO,"Failed to begin streamer before calling alloc");
		exit(1);
		}

	size_t combinedSize=size+strm->currentSize;

	if(strm->block->allocated+combinedSize > strm->block->blocksize)
			{
			MemStreamerBlock *newBlock=streamerBlockAlloc();

			newBlock->prev=strm->block;
			strm->block=newBlock;

			memcpy(newBlock->data,strm->currentPtr,strm->currentSize);
			strm->currentPtr=newBlock->data;
			}

	void *usrPtr=strm->currentPtr+strm->currentSize;
	strm->currentSize=combinedSize;

	return usrPtr;
}


void *sEnd(MemStreamer *strm)
{
	if(strm->currentPtr==NULL)
		{
		LOG(LOG_INFO,"Failed to begin streamer before calling end");
		exit(1);
		}

	strm->block->allocated+=strm->currentSize;

	void *usrPtr=strm->currentPtr;

	strm->currentPtr=NULL;
	strm->currentSize=0;

	return usrPtr;
}
