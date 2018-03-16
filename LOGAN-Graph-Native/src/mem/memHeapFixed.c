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

// static
void dumpFixedHeap(MemFixedHeap *fixedHeap)
{
	LOG(LOG_INFO,"FixedHeap dump:");

	s64 totalSize=0, totalUsed=0, totalFree=0;
	for(int i=0;i<MEMFIXED_SIZES;i++)
		{
		int itemSize=MEMFIXED_MIN_SIZE+i;

		s64 sizeItemCount=0;
		s64 sizeFreeCount=0;
		s64 sizeBlockCount=0;

		MemFixedHeapSizeBlock *block=fixedHeap->sizer[i].full;
		while(block!=NULL)
			{
			sizeItemCount+=block->totalCount;
			sizeFreeCount+=block->freeCount;
			sizeBlockCount++;

			block=block->nextPtr;
			}

		s64 sizeTotalBytes=sizeBlockCount*sizeof(MemFixedHeapSizeBlock);
		s64 sizeUsedBytes=(sizeItemCount-sizeFreeCount)*itemSize;
		u64 sizeFreeBytes=sizeFreeCount*itemSize;

		double usedRatio=sizeUsedBytes/(double)sizeTotalBytes;

		totalSize+=sizeTotalBytes;
		totalUsed+=sizeUsedBytes;
		totalFree+=sizeFreeBytes;

		LOGN(LOG_INFO, "HEAPSIZE: ItemSize %i: %li blocks using %li bytes. Items %li used / %li free. Byte Used ratio %lf",
				itemSize, sizeBlockCount, sizeTotalBytes, (sizeItemCount-sizeFreeCount), sizeFreeCount, usedRatio);
		}

	LOGN(LOG_INFO,"FIXEDSUM: %li %li %li",totalSize,totalUsed,totalFree);

}


static MemFixedHeapSizeBlock *allocateBlock(u16 size)
{

	MemFixedHeapSizeBlock *block=G_ALLOC_ALIGNED(PAGE_ALIGNMENT_SIZE, PAGE_ALIGNMENT_SIZE, MEMTRACKID_HEAP_BLOCKDATA_FIXED);

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
	if(sizeof(MemFixedHeapSizeBlock)!=PAGE_ALIGNMENT_SIZE)
		{
		LOG(LOG_CRITICAL,"FixedHeap SizeBlock not expected size %i vs %i", sizeof(MemFixedHeapSizeBlock), PAGE_ALIGNMENT_SIZE);
		}

	for(int i=0;i<MEMFIXED_SIZES;i++)
		{
		fixedHeap->sizer[i].current=NULL;
		fixedHeap->sizer[i].full=NULL;
		}
}

void mhfHeapFree(MemFixedHeap *fixedHeap)
{
	#ifdef FEATURE_ENABLE_HEAP_STATS
		dumpFixedHeap(fixedHeap);
	#endif


	for(int i=0;i<MEMFIXED_SIZES;i++)
		{
		fixedHeap->sizer[i].current=NULL;

		MemFixedHeapSizeBlock *block=fixedHeap->sizer[i].full;

		while(block!=NULL)
			{
			MemFixedHeapSizeBlock *nextBlock=block->nextPtr;
			G_FREE(block, PAGE_ALIGNMENT_SIZE, MEMTRACKID_HEAP_BLOCKDATA_FIXED);
			block=nextBlock;
			}

		fixedHeap->sizer[i].full=NULL;
		}
}

void *mhfAlloc(MemFixedHeap *fixedHeap, size_t size)
{
	size=MAX(size,MEMFIXED_MIN_SIZE);

	int sizeIndex=size-MEMFIXED_MIN_SIZE;

	MemFixedHeapSizer *sizer=&(fixedHeap->sizer[sizeIndex]);
	MemFixedHeapSizeBlock *currentBlock=sizer->current;

	if(currentBlock!=NULL && currentBlock->freeCount==0)
		{
		while(currentBlock!=NULL && currentBlock->freeCount==0)
			currentBlock=currentBlock->nextPtr;

		if(currentBlock!=NULL)
			sizer->current=currentBlock;
		}

	if(currentBlock==NULL && sizer->recycleCount>MEMFIXED_RECYCLE_THRESHOLD)
		{
		sizer->recycleCount=0;

		currentBlock=sizer->full;

		while(currentBlock!=NULL && currentBlock->freeCount==0)
			currentBlock=currentBlock->nextPtr;

		if(currentBlock!=NULL)
			sizer->current=currentBlock;
		}

	if(currentBlock==NULL)
		{
		currentBlock=allocateBlock(size);

		currentBlock->nextPtr=sizer->full;

		sizer->full=currentBlock;
		sizer->current=currentBlock;
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
			//LOG(LOG_INFO,"Allocated %i at %p",size,ptr);
			return ptr;
			}
		}


	LOG(LOG_CRITICAL,"mhfAlloc Failed for %i",size);
	return NULL;
}

void mhfFree(MemFixedHeap *fixedHeap, void *oldData, size_t size)
{
	uintptr_t dataAddr = (uintptr_t)oldData;
	MemFixedHeapSizeBlock *block=(MemFixedHeapSizeBlock *)(dataAddr&~((u64)PAGE_ALIGNMENT_MASK));

	if(block->pad1!=MEMFIXED_PAD_MAGIC)
		LOG(LOG_CRITICAL,"Invalid magic number in block containing %p (size %i)",oldData, size);

	if(block->itemSize!=size)
		LOG(LOG_CRITICAL,"Block size is %i vs expected %i",block->itemSize, size);

	int sizeIndex=size-MEMFIXED_MIN_SIZE;
	fixedHeap->sizer[sizeIndex].recycleCount++;

	int dataOffset=((u8 *)oldData)-block->data;
	int dataIndex=dataOffset/size;

	int flagIndex=dataIndex>>6;
	int bitIndex=dataIndex&0x3F;

	u64 freeMask=(1L<<bitIndex);
	block->freeFlags[flagIndex]|=freeMask;
	block->freeCount++;



}


