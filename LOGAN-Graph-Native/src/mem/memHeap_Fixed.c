#include "common.h"



// 4016 divided by itemSize (start at MEMFIXED_MIN_SIZE)
static const u16 MEMFIXED_COUNTS[MEMFIXED_SIZES]={
		502, 446, 401, 365, 334, 308, 286, 267, 			//   8 - 15
		251, 236, 223, 211, 200, 191, 182, 174, 			//  16 - 23
		167, 160, 154, 148, 143, 138, 133, 129,				//  24 - 31
		125, 121, 118, 114, 111, 108, 105, 102,				//  32 - 39
		100,  97,  95,  93,  91,  89,  87,  85, 			//  40 - 47
		 83,  81,  80,  78,  77,  75,  74,  73, 			//  48 - 55
		 71,  70,  69,  68,  66,  65,  64,  63, 			//  56 - 63
		 62,  61,  60,  59,  59,  58,  57,  56, 			//  64 - 71
		 55,  55,  54,  53,  52,  52,  51,  50,				//  72 - 79
		 50,  49,  48,  48,  47,  47,  46,  46,  			//  80 - 87
		 45,  45,  44,  44,  43,  43,  42,  42,  			//  88 - 95
		 41,  41,  40,  40,  40,  39,  39,  38,  			//  96 - 103
		 38,  38,  37,  37,  37,  36,  36,  36,  			// 104 - 111
		 35,  35,  35,  34,  34,  34,  34,  33,  			// 112 - 119
		 33,  33,  32,  32,  32,  32,  31,  31				// 120 - 127
};


#define MEMFIXED_PAD_MAGIC 0x4242

static MemFixedHeapBlock *allocateBlock(u16 size)
{

	MemFixedHeapBlock *block=G_ALLOC_ALIGNED(PAGE_ALIGNMENT_SIZE, PAGE_ALIGNMENT_SIZE, MEMTRACKID_HEAP_BLOCKDATA);

	u16 itemCount=MEMFIXED_COUNTS[size-MEMFIXED_MIN_SIZE];

	block->nextPtr=NULL;
	block->itemSize=size;
	block->totalCount=itemCount;
	block->freeCount=itemCount;
	block->pad1=MEMFIXED_PAD_MAGIC;

	int freeCount=itemCount;
	for(int i=0;i<MEMFIXED_FLAGS_PER_BLOCK;i++)
		{
		if(freeCount>=64)
			block->freeFlags[i]=0xFFFFFFFFFFFFFFFFL;
		else if(freeCount<=0)
			block->freeFlags[i]=0;
		else
			block->freeFlags[i]=~(0xFFFFFFFFFFFFFFFFL<<freeCount);

		freeCount-=64;
		}

	return block;
}



void mhfHeapAlloc(MemFixedHeap *fixedHeap)
{
	for(int i=0;i<MEMFIXED_SIZES;i++)
		{
		fixedHeap->current[i]=NULL;
		fixedHeap->full[i]=NULL;
		}
}

void mhfHeapFree(MemFixedHeap *fixedHeap)
{
	for(int i=0;i<MEMFIXED_SIZES;i++)
		{
		fixedHeap->current[i]=NULL;

		MemFixedHeapBlock *block=fixedHeap->full[i];

		while(block!=NULL)
			{
			MemFixedHeapBlock *nextBlock=block->nextPtr;
			G_FREE(block, PAGE_ALIGNMENT_SIZE, MEMTRACKID_HEAP_BLOCKDATA);
			block=nextBlock;
			}

		fixedHeap->full[i]=NULL;
		}
}

void *mhfAlloc(MemFixedHeap *fixedHeap, size_t size)
{
	size=MAX(size,MEMFIXED_MIN_SIZE);

	int sizeIndex=size-MEMFIXED_MIN_SIZE;

	MemFixedHeapBlock *currentBlock=fixedHeap->current[sizeIndex];

	if(currentBlock!=NULL && currentBlock->freeCount==0)
		{
		while(currentBlock!=NULL && currentBlock->freeCount==0)
			currentBlock=currentBlock->nextPtr;

		if(currentBlock!=NULL)
			fixedHeap->current[sizeIndex]=currentBlock;
		}

	if(currentBlock==NULL)
		{
		currentBlock=allocateBlock(size);

		currentBlock->nextPtr=fixedHeap->full[sizeIndex];

		fixedHeap->full[sizeIndex]=currentBlock;
		fixedHeap->current[sizeIndex]=currentBlock;
		}

	for(int i=0;i<MEMFIXED_FLAGS_PER_BLOCK;i++)
		{
		u64 flags=currentBlock->freeFlags[i];
		if(flags!=0)
			{
			s32 index=i*64+__builtin_ctzl(flags);
			currentBlock->freeFlags[i]=flags & (flags-1); // Clear lowest bit
			currentBlock->freeCount--;

			void *ptr=currentBlock->data+index*size;
			LOG(LOG_INFO,"Allocated %i at %p",size,ptr);
			return ptr;
			}
		}


	LOG(LOG_CRITICAL,"mhfAlloc Failed for %i",size);
	return NULL;
}

void mhfFree(MemFixedHeap *fixedHeap, void *oldData, size_t size)
{
	uintptr_t dataAddr = (uintptr_t)oldData;
	MemFixedHeapBlock *block=(MemFixedHeapBlock *)(dataAddr&~((u64)PAGE_ALIGNMENT_MASK));

	if(block->pad1!=MEMFIXED_PAD_MAGIC)
		LOG(LOG_CRITICAL,"Invalid magic number in block containing %p (size %i)",oldData, size);

	LOG(LOG_INFO,"Block size is %i vs expected %i",block->itemSize, size);

}

