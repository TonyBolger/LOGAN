#include "common.h"


/*

	Multi-generation "GC":

		Use of one circular buffer per generation (vs multiple blocks):
			Less end of block fragmentation
			No need to compact after partial spill to next generation

		Dead Marking:
			Avoids the need to calculate complete root set or indirect references during stats
			Requires tag bit to indicate live/dead status

		(Future) Heat-based allocation:
			Use youngest generation only for most frequently modified/largest etc smers

		(Future) Padded allocation:
			Blocks in young generation can be padded sufficiently to allow for future additional data

		(Future) Double-mapping for proper ring buffer positions
			Avoids the end->start fragmentation problem

	Remaining issues:
		Updating inbound pointers for a given block when copying
			Option A: Scan & index all inbound pointers (root + indirect) and modify as moved
				Advantages:
					No persistent additional storage
				Disadvantages:
					Large pointer-gathering effort required even for minor collections

			Option B: In-place Forwarding pointers with immediate scan & apply to inbound
				Advantages:
					Less minimum effort than full scan/index of inbound
				Disadvantages:
					Minimum block size for pointer (or offset)

			Option C: In-place Forwarding pointers with deferred apply
				Advantages:
					No need to scan & update inbounds
				Disadvantages:
					Minimum block size for pointer (or offset)
					Root/Internal pointers need to be validated before use

			Option D: Change List with deferred apply
				Advantages:
					No need to scan & update inbounds
					No minimum block size
				Disadvantages:
					Root/Internal pointers need to be validated before use

			Option E: Targeted Evacuation - Slice X: Remove all data from range Y
				Advantages:
					Allows representation to be modified as part of relocation
				Disadvantages:
					Needs clearly defined boundary markers for targeting



	Optimization Goals:
	 	Majority of collections should be one (or few) generations
	 	Each generation should be sized to achieve similar survival percentages (e.g 10-20%)





 */




/*
 * Block sizes are: 8,9,10,11,12,13,14,15 * a basic size, then double the basic size.
 *
 * Block must be at least minSize and at least the index, if not NULL
 */

const double MAX_SURVIVOR[CIRCHEAP_MAX_GENERATIONS]={0.70, 0.75, 0.75, 0.80, 0.80, 0.85, 0.85, 0.90};

//const double MAX_SURVIVOR[CIRCHEAP_MAX_GENERATIONS]={0.50, 0.60, 0.70, 0.70, 0.70, 0.70, 0.70, 0.90};
//const double MAX_SURVIVOR[CIRCHEAP_MAX_GENERATIONS]={0.50, 0.80, 0.90, 0.90, 0.90, 0.90, 0.90, 0.95};
//const double MAX_SURVIVOR[CIRCHEAP_MAX_GENERATIONS]={0.80, 0.85, 0.90, 0.92, 0.94, 0.96, 0.97, 0.98};
//const double MAX_SURVIVOR[CIRCHEAP_MAX_GENERATIONS]={0.90, 0.91, 0.92, 0.93, 0.94, 0.96, 0.97, 0.98};
//const double MAX_SURVIVOR[CIRCHEAP_MAX_GENERATIONS]={0.90, 0.91, 0.92, 0.93, 0.94, 0.95, 0.96, 0.97};

//const int MIN_BLOCKSIZEINDEX[CIRCHEAP_MAX_GENERATIONS]={64, 32, 16, 8 ,4, 2, 1 ,0};

const int MIN_BLOCKSIZEINDEX[CIRCHEAP_MAX_GENERATIONS]={0,0,0,0,0,0,0,0};

static s64 getRequiredBlocksize(s64 minSize, s32 *blockSizeIndexPtr)
{
	if(minSize>CIRCHEAP_MAX_BLOCKSIZE)
		LOG(LOG_CRITICAL,"Cannot use allocation blocksize above maximum %li %li",minSize,CIRCHEAP_MAX_BLOCKSIZE);

	s32 blockSizeIndex=0;

	if(blockSizeIndexPtr!=NULL)
		blockSizeIndex=MAX(0, *blockSizeIndexPtr);

//	LOG(LOG_INFO,"getRequiredBlocksize: MinSize %li MinSizeIndex %i",minSize, blockSizeIndex);

	// blockSizeIncrement is the difference between steps with the same exponent
	// Every CIRCHEAP_BLOCKSIZE_INCREMENT_STEPS is a doubling, so this removes the 'mantissa' part then applies the exponent

	s64 blockSizeIncrement=CIRCHEAP_BLOCKSIZE_BASE_INCREMENT<<(blockSizeIndex>>CIRCHEAP_BLOCKSIZE_INCREMENT_SHIFT);

	// testBlockSize is the maximum which can be achieved with this exponent

	s64 testBlockSize=blockSizeIncrement<<(CIRCHEAP_BLOCKSIZE_INCREMENT_SHIFT+1);
//	LOG(LOG_INFO,"Initial test block size %li",testBlockSize);

	if(testBlockSize<minSize)
		{
		blockSizeIndex&=~CIRCHEAP_BLOCKSIZE_INCREMENT_MASK; // Reset mantissa if increasing exponent

		while(testBlockSize<minSize) // Much too small - more doublings
			{
			blockSizeIncrement<<=1;
			testBlockSize<<=1;
			blockSizeIndex+=CIRCHEAP_BLOCKSIZE_INCREMENT_STEPS;
			}
		}


	int mult=CIRCHEAP_BLOCKSIZE_INCREMENT_STEPS+(blockSizeIndex&CIRCHEAP_BLOCKSIZE_INCREMENT_MASK);
	s64 blockSize=blockSizeIncrement*mult;

//	LOG(LOG_INFO,"Estimate block size of %li with index %i",blockSize,blockSizeIndex);

	while(mult<=(CIRCHEAP_BLOCKSIZE_INCREMENT_STEPS*2) && blockSize<minSize)
		{
		blockSize+=blockSizeIncrement;
		blockSizeIndex++;
		mult++;
		}

//	LOG(LOG_INFO,"Using block size of %li with index %i",blockSize,blockSizeIndex);

	if(blockSizeIndexPtr!=NULL)
		*blockSizeIndexPtr=blockSizeIndex;

	return blockSize;
}

static MemCircHeapBlock *allocBlock(s32 minSize, int minblockSizeIndex)
{
	s32 sizeIndex=minblockSizeIndex;
	s64 size=getRequiredBlocksize(minSize, &sizeIndex);

	MemCircHeapBlock *blockPtr=NULL;
	u8 *dataPtr=NULL;

	if(posix_memalign((void **)&blockPtr, CACHE_ALIGNMENT_SIZE, sizeof(MemCircHeapBlock))!=0)
		LOG(LOG_CRITICAL,"Failed to alloc MemCircHeapBlockAllocation structure");

	if(posix_memalign((void **)&dataPtr, PAGE_ALIGNMENT_SIZE*16, size)!=0)
		LOG(LOG_CRITICAL,"Failed to alloc MemCircHeapBlockAllocation data");

	blockPtr->next=NULL;

	blockPtr->ptr=dataPtr;
	blockPtr->size=size;
	blockPtr->sizeIndex=sizeIndex;
	blockPtr->needsExpanding=0;
	blockPtr->allocPosition=0;
	blockPtr->reclaimLimit=0;
	blockPtr->reclaimPosition=0;

	blockPtr->reclaimSizeLive=0;
	blockPtr->reclaimSizeDead=0;

//	LOG(LOG_INFO,"Alloc block %p : allocated",blockPtr);

	return blockPtr;
}

static void freeBlock(MemCircHeapBlock *block)
{
//	LOG(LOG_INFO,"Alloc block %p : freed",block->ptr);

	free(block->ptr);
	free(block);
}

static void freeBlockSafe(MemCircHeapBlock *block)
{
//	LOG(LOG_INFO, "Free block %p size %li (alloc %li and reclaim %li)",
//		block->ptr, block->size, block->allocPosition, block->reclaimPosition);

	if(block->allocPosition!=block->reclaimPosition)
		LOG(LOG_CRITICAL,"Attempt to free non-empty block");

	freeBlock(block);
}


static s64 estimateSpaceInGeneration(MemCircGeneration *generation)
{
	MemCircHeapBlock *block=generation->allocBlock;

	s64 space;

	if(block->reclaimLimit)  // USED - allocPos - FREE - reclaimPos - USED reclaimLimit
		{
		space=block->reclaimPosition-block->allocPosition;
		}
	else // FREE - reclaimPos - USED - allocPos - FREE
		{
		space=MAX(block->size-block->allocPosition-1, block->reclaimPosition);
		}

	/*

	if(block->reclaimPosition<=block->allocPosition) // Start - reclaim - alloc - end (bigger of the two)
		{
		space=MAX(block->size-block->allocPosition, block->reclaimPosition)-CIRCHEAP_BLOCK_TAG_OVERHEAD;
		}
	else											// Start - alloc - reclaim  - end
		{
		space=block->reclaimPosition-block->allocPosition-CIRCHEAP_BLOCK_TAG_OVERHEAD-1;
		}
*/
	return MAX(0,space-CIRCHEAP_BLOCK_TAG_OVERHEAD-CIRCHEAP_BLOCK_OVERHEAD);
}


//static
void dumpCircHeap(MemCircHeap *circHeap)
{
	LOG(LOG_INFO,"Heap dump:");

	s64 totalSize=0, totalLive=0, totalDead=0;
	for(int i=0;i<CIRCHEAP_MAX_GENERATIONS;i++)
		{
		s64 size=circHeap->generations[i].allocBlock->size;
		s64 space=estimateSpaceInGeneration(circHeap->generations+i);
		s64 occupied=size-space;

		s64 live=circHeap->generations[i].prevReclaimSizeLive;
		s64 dead=circHeap->generations[i].prevReclaimSizeDead;
		s64 reclaimed=live+dead;

		double survivorRatio=live/(double)reclaimed;

		totalSize+=size;
		totalLive+=live;
		totalDead+=dead;

		LOGN(LOG_INFO, "HEAPGEN: Gen %i: %li occupied / %li free with survivors: live %li dead %li ratio %lf. Alloc block %p size %li (alloc %li and reclaim %li) Reclaim block %p size %li (alloc %li and reclaim %li)",
			i,occupied,space, live, dead, survivorRatio,
			circHeap->generations[i].allocBlock->ptr, circHeap->generations[i].allocBlock->size,
			circHeap->generations[i].allocBlock->allocPosition,circHeap->generations[i].allocBlock->reclaimPosition,
			circHeap->generations[i].reclaimBlock->ptr, circHeap->generations[i].reclaimBlock->size,
			circHeap->generations[i].reclaimBlock->allocPosition,circHeap->generations[i].reclaimBlock->reclaimPosition);
		}

	LOGN(LOG_INFO,"HEAPSUM: %li %li %li",totalSize,totalLive,totalDead);

}



MemCircHeap *circHeapAlloc(MemCircHeapChunkIndex *(*reclaimIndexer)(u8 *data, s64 targetAmount, u8 tag, u8 **tagData, s32 tagDataLength, s32 tagSearchOffset, MemDispenser *disp),
		void (*relocater)(MemCircHeapChunkIndex *reclaimIndex, u8 tag, u8 **tagData, s32 tagDataLength))
{
	MemCircHeap *circHeap=NULL;

	if(posix_memalign((void **)&circHeap, CACHE_ALIGNMENT_SIZE, sizeof(MemCircHeap))!=0)
		LOG(LOG_CRITICAL,"Failed to alloc MemCircHeap structure");

	for(int i=0;i<CIRCHEAP_MAX_GENERATIONS;i++)
		{
		MemCircHeapBlock *block=allocBlock(0, MIN_BLOCKSIZEINDEX[i]);

		circHeap->generations[i].allocBlock=block;
		circHeap->generations[i].reclaimBlock=block;

		circHeap->generations[i].allocCurrentChunkTag=0;
		circHeap->generations[i].allocCurrentChunkTagOffset=0;
		circHeap->generations[i].allocCurrentChunkPtr=0;

		circHeap->generations[i].reclaimCurrentChunkTag=0;
		circHeap->generations[i].reclaimCurrentChunkTagOffset=0;

//		circHeap->generations[i].curReclaimSizeLive=0;
//		circHeap->generations[i].curReclaimSizeDead=0;

		circHeap->generations[i].prevReclaimSizeLive=0;
		circHeap->generations[i].prevReclaimSizeDead=0;
		}

	circHeap->reclaimIndexer=reclaimIndexer;
	circHeap->relocater=relocater;

	return circHeap;
}

void circHeapFree(MemCircHeap *circHeap)
{
#ifdef FEATURE_ENABLE_HEAP_STATS
	dumpCircHeap(circHeap);
#endif

	for(int i=0;i<CIRCHEAP_MAX_GENERATIONS;i++)
		{
		MemCircHeapBlock *block=circHeap->generations[i].reclaimBlock;

		while(block!=NULL)
			{
			MemCircHeapBlock *next=block->next;
			freeBlock(block);
			block=next;
			}
		}
}


void circHeapRegisterTagData(MemCircHeap *circHeap, u8 tag, u8 **data, s32 dataLength)
{
	circHeap->tagData[tag]=data;
	circHeap->tagDataLength[tag]=dataLength;
}



// Allocates from a generation, creating a tag if needed
static void *allocFromGeneration(MemCircGeneration *generation, size_t newAllocSize, u8 tag, s32 firstTagOffset, s32 lastTagOffset, s32 *oldTagOffset)
{
	if(newAllocSize<0)
		{
		LOG(LOG_CRITICAL,"Cannot allocate negative space %i",newAllocSize);
		}

	int needTag=(generation->allocCurrentChunkTag!=tag || generation->allocCurrentChunkTagOffset > firstTagOffset || generation->allocCurrentChunkPtr==NULL);

	//LOG(LOG_INFO,"Need tag %i - %i %i %i", needTag, generation->allocCurrentChunkTag!=tag, generation->allocCurrentChunkTagOffset > firstTagOffset, generation->allocCurrentChunkPtr==NULL);

	if(needTag)
		newAllocSize+=2; // Allow for chunk marker and tag


//	LOG(LOG_INFO,"Request is %li, Alloc is %p %li, reclaim is %p %li",
//			newAllocSize,
//			generation->allocBlock->ptr, generation->allocBlock->allocPosition,
//			generation->reclaimBlock->ptr, generation->reclaimBlock->reclaimPosition);


	MemCircHeapBlock *block=generation->allocBlock;
	s64 positionAfterAlloc=block->allocPosition+newAllocSize;

	if(block->reclaimLimit)	// Before: USED - allocPos - FREE - reclaimPos - USED
		{
		if(block->reclaimPosition==block->allocPosition)
			return NULL;

		// Avoid overtaking reclaim position

		if(block->reclaimPosition>block->allocPosition && block->reclaimPosition<positionAfterAlloc)
			return NULL;
		}

		// Easy case, alloc at current position, with optional tag
	if(positionAfterAlloc<block->size) // Always keep one byte gap
		{
		u8 *data=block->ptr+block->allocPosition;

		if(needTag)
			{
			generation->allocCurrentChunkTag=tag;
			generation->allocCurrentChunkTagOffset=0;
			generation->allocCurrentChunkPtr=data;

			*data++=CH_HEADER_CHUNK;
			*data++=tag;
			}

		block->allocPosition=positionAfterAlloc;

		if(oldTagOffset!=NULL)
			{
			*oldTagOffset=generation->allocCurrentChunkTagOffset;
			generation->allocCurrentChunkTagOffset=lastTagOffset;
			}

		return data;
		}

	// Wrap case, force tag and check for space before reclaim

	if(!needTag)
		newAllocSize+=2;

	if(block->reclaimPosition>=newAllocSize)
		{
		// First mark end of used region
		u8 *data=block->ptr+block->allocPosition;
		*data=CH_HEADER_INVALID;

		block->reclaimLimit=block->allocPosition;

		data=block->ptr;

		generation->allocCurrentChunkTag=tag;
		generation->allocCurrentChunkTagOffset=0;
		generation->allocCurrentChunkPtr=data;

		*data++=CH_HEADER_CHUNK;
		*data++=tag;

		block->allocPosition=newAllocSize;

		if(oldTagOffset!=NULL)
			{
			*oldTagOffset=generation->allocCurrentChunkTagOffset;
			generation->allocCurrentChunkTagOffset=lastTagOffset;
			}

		return data;
		}

	return NULL;
}


static MemCircHeapChunkIndex *createReclaimIndex_single(MemCircHeap *circHeap, MemCircGeneration *generation, int generationNum, s64 reclaimTargetAmount, MemDispenser *disp)
{
	MemCircHeapBlock *reclaimBlock=generation->reclaimBlock;

	if(reclaimBlock->reclaimPosition==reclaimBlock->allocPosition && reclaimBlock->reclaimLimit==0)
		return NULL;

	// Could be 'INVALID', 'CHUNK' or other
	u8 data=reclaimBlock->ptr[reclaimBlock->reclaimPosition];

	if(data==CH_HEADER_INVALID) // Wrap reclaim
		{
		// Should verify if live/dead ratio acceptable
		long totalReclaim=generation->reclaimBlock->reclaimSizeLive+generation->reclaimBlock->reclaimSizeDead;

		if(totalReclaim>0)
			{
			double survivorRatio=(double)generation->reclaimBlock->reclaimSizeLive/(double)totalReclaim;

			if(survivorRatio>MAX_SURVIVOR[generationNum])
				{
				//LOG(LOG_INFO,"Generation %i Survivor Ratio %lf exceeds threshold, marked for expansion",
//						generation-circHeap->generations, survivorRatio);

				reclaimBlock->needsExpanding=1;
				}
			}

		if(reclaimBlock->reclaimPosition != reclaimBlock->reclaimLimit)
			{
			LOG(LOG_INFO,"GC Heap Corruption detected at %p",reclaimBlock->ptr+reclaimBlock->reclaimPosition, data);
			LOG(LOG_CRITICAL,"Attempt to wrap reclaimPosition %i before reaching reclaimLimit %i",reclaimBlock->reclaimPosition, reclaimBlock->reclaimLimit);
			}

		generation->prevReclaimSizeLive=generation->reclaimBlock->reclaimSizeLive;
		generation->prevReclaimSizeDead=generation->reclaimBlock->reclaimSizeDead;

		//LOG(LOG_INFO,"Wrap Reclaim: %p -  %li %li",generation->reclaimBlock, generation->prevReclaimSizeLive, generation->prevReclaimSizeDead);

		reclaimBlock->reclaimLimit=0;

		reclaimBlock->reclaimSizeLive=0;
		reclaimBlock->reclaimSizeDead=0;

		reclaimBlock->reclaimPosition=0;

		if(reclaimBlock->reclaimPosition==reclaimBlock->allocPosition)
			return NULL;

		data=reclaimBlock->ptr[0];
		}

	if(data==CH_HEADER_CHUNK)
		{
		generation->reclaimCurrentChunkTag=reclaimBlock->ptr[reclaimBlock->reclaimPosition+1];
		generation->reclaimCurrentChunkTagOffset=0;
		reclaimBlock->reclaimPosition+=2;
		reclaimTargetAmount-=2;
		}

	u8 *reclaimPtr=reclaimBlock->ptr+reclaimBlock->reclaimPosition;

	int reclaimTargetMax=reclaimBlock->reclaimLimit? (reclaimBlock->reclaimLimit-reclaimBlock->reclaimPosition) : (reclaimBlock->allocPosition-reclaimBlock->reclaimPosition);
	reclaimTargetAmount=MIN(reclaimTargetAmount,reclaimTargetMax);

//	LOG(LOG_INFO,"Reclaim from ptr %p (RecPos: %i AllocPos: %i)",reclaimPtr,reclaimBlock->reclaimPosition, reclaimBlock->allocPosition);

	u8 tag=generation->reclaimCurrentChunkTag;
	MemCircHeapChunkIndex *indexedChunk=(circHeap->reclaimIndexer)(reclaimPtr, reclaimTargetAmount, tag,
			circHeap->tagData[tag], circHeap->tagDataLength[tag], generation->reclaimCurrentChunkTagOffset, disp);

	if(indexedChunk==NULL)
		{
		LOG(LOG_CRITICAL,"Trouble in tag %i",tag);
		}

//	LOG(LOG_INFO,"Got an indexedChunk with %i %i",indexedChunk->firstLiveTagOffset, indexedChunk->lastLiveTagOffset);

	generation->reclaimCurrentChunkTagOffset=indexedChunk->lastScannedTagOffset;
	indexedChunk->chunkTag=generation->reclaimCurrentChunkTag;

	indexedChunk->reclaimed=indexedChunk->heapEndPtr-reclaimPtr;
	reclaimBlock->reclaimPosition+=indexedChunk->reclaimed;

	reclaimBlock->reclaimSizeLive+=indexedChunk->sizeLive;
	reclaimBlock->reclaimSizeDead+=indexedChunk->sizeDead;

	if(reclaimBlock->reclaimSizeLive>reclaimBlock->reclaimPosition)
		{
		LOG(LOG_INFO,"Blocks %p %p",generation->allocBlock, generation->reclaimBlock);

		LOG(LOG_CRITICAL,"Generation total status invalid: Gen %i with %li live (now %li), %li dead (now %li) size %li reclaimPos %li",
				generationNum,
				generation->reclaimBlock->reclaimSizeLive, indexedChunk->sizeLive,
				generation->reclaimBlock->reclaimSizeDead, indexedChunk->sizeDead,
				generation->reclaimBlock->size,
				generation->reclaimBlock->reclaimPosition);
		}



	if(data==CH_HEADER_CHUNK)
		{
		indexedChunk->reclaimed+=2;
		reclaimBlock->reclaimSizeLive+=2;
		}

	//u8 *reclaimNewPtr=reclaimBlock->ptr+reclaimBlock->reclaimPosition;
	//LOG(LOG_INFO,"Post Reclaim from ptr %p now %p (RecPos: %i AllocPos: %i) L: %li of %li D: %li of %li",
			//reclaimPtr, reclaimNewPtr, reclaimBlock->reclaimPosition, reclaimBlock->allocPosition,
			//indexedChunk->sizeLive, reclaimBlock->reclaimSizeLive, indexedChunk->sizeDead, reclaimBlock->reclaimSizeDead);

//	LOG(LOG_INFO,"Creating Reclaim Index for tag %i moving reclaim position by %li to %i",tag, indexedChunk->reclaimed, reclaimBlock->reclaimPosition);


	return indexedChunk;
}

static MemCircHeapChunkIndex *createReclaimIndex(MemCircHeap *circHeap, MemCircGeneration *generation, int generationNum, s64 reclaimTargetAmount, int splitShift, MemDispenser *disp)
{
	if(!generation->reclaimBlock->reclaimLimit && generation->reclaimBlock->reclaimPosition==generation->reclaimBlock->allocPosition)
		return NULL;

	reclaimTargetAmount=MAX(reclaimTargetAmount, (generation->reclaimBlock->size)>>splitShift);

	//LOG(LOG_INFO,"Creating Reclaim Index with reclaim target amount: %li",reclaimTargetAmount);

	MemCircHeapChunkIndex *index=createReclaimIndex_single(circHeap,generation,generationNum, reclaimTargetAmount,disp);

	MemCircHeapChunkIndex *firstIndex=index;

	while(index!=NULL)
		{
		reclaimTargetAmount-=index->reclaimed;
		if(reclaimTargetAmount<=0)
			{
			index->next=NULL;
			break;
			}

		index->next=createReclaimIndex_single(circHeap,generation,generationNum, reclaimTargetAmount,disp);
		index=index->next;
		}

	return firstIndex;
}


static MemCircHeapChunkIndex *moveIndexedHeap(MemCircHeap *circHeap, MemCircHeapChunkIndex *index, s32 allocGeneration)
{
	MemCircHeapChunkIndex *scan=index;

	while(scan!=NULL)
		{
		if(scan->sizeLive>0)
			{
			/*
			if(scan->firstLiveTagOffset==-1)
				{
				int currentOffset=circHeap->generations[allocGeneration].allocCurrentChunkTagOffset;
				scan->firstLiveTagOffset=currentOffset;
				scan->lastLiveTagOffset=currentOffset;
				}
*/

//			LOG(LOG_INFO,"MovedPreAlloc");
//			dumpCircHeap(circHeap);

			u8 *newChunk=NULL;

			if(scan->firstLiveTagOffset==-1)
				{
//				LOG(LOG_INFO,"moveIndexHeap alloc 1");
				newChunk=allocFromGeneration(circHeap->generations+allocGeneration, scan->sizeLive,
									scan->chunkTag, INT_MAX, INT_MAX, NULL);
				}
			else
				{
//				LOG(LOG_INFO,"moveIndexHeap alloc 2");
				newChunk=allocFromGeneration(circHeap->generations+allocGeneration, scan->sizeLive,
					scan->chunkTag, scan->firstLiveTagOffset, scan->lastLiveTagOffset, &(scan->newChunkOldTagOffset));
				}

//			LOG(LOG_INFO,"moveIndexedHeap Alloc: %p Size: %li ChunkTag: %i Generation: %i",newChunk,scan->sizeLive,scan->chunkTag,allocGeneration);

//			LOG(LOG_INFO,"MovedPostAlloc");
//			dumpCircHeap(circHeap);

			if(newChunk==NULL) // Shouldn't happen, but just in case
				{
/*
				LOG(LOG_INFO,"GC generation promotion failed");

				LOG(LOG_INFO,"Unable to allocate %li in generation %i",scan->sizeLive,allocGeneration);
				dumpCircHeap(circHeap);
*/
//			circHeapEnsureSpace_generation(circHeap,totalLive+CIRCHEAP_BLOCK_OVERHEAD,allocGeneration,disp);
//			newChunk=allocFromGeneration(circHeap->generations+allocGeneration, scan->sizeLive, scan->chunkTag);
//			if(newChunk==NULL)
//				LOG(LOG_CRITICAL,"GC generation promotion failed");

				return scan;
				}

			scan->newChunk=newChunk;
			u8 tag=scan->chunkTag;

			(circHeap->relocater)(scan, tag, circHeap->tagData[tag], circHeap->tagDataLength[tag]);
			}

		scan=scan->next;
		}


	return NULL;
}

// This can be calculated during indexing

static s64 calcHeapReclaimStats(MemCircHeapChunkIndex *index, MemCircGeneration *generation, int generationNum)
{
	s64 totalLive=0;
	s64 totalTag=0;

	MemCircHeapChunkIndex *scan=index;
	while(scan!=NULL)
		{
		totalLive+=scan->sizeLive;
		totalTag+=2;
		scan=scan->next;
		}

	//LOG(LOG_INFO,"Generation %i can reclaim %li with %li live, %li dead",generationNum, totalReclaimed, totalLive, totalDead);

	return totalLive+totalTag;
}


static void circHeapCompact(MemCircHeap *circHeap, int generation, s64 targetFree, MemDispenser *disp)
{
	// This implementation can fail due to the 'compaction' slightly growing the memory block due to the need for chunk headers, or if alloc catches reclaim (100% retained)

//	LOG(LOG_INFO,"CircHeapCompact");

	//dumpCircHeap(circHeap);

	s64 spaceEstimate=0;
	int maxReclaims=CIRCHEAP_RECLAIM_SPLIT_FULL;
	MemCircHeapChunkIndex *index=createReclaimIndex(circHeap,circHeap->generations+generation, generation, 0, CIRCHEAP_RECLAIM_SPLIT_FULL_SHIFT, disp);

	while(index!=NULL)
		{
		calcHeapReclaimStats(index,circHeap->generations+generation, generation);

		MemCircHeapChunkIndex *remainingIndex=moveIndexedHeap(circHeap, index, generation);
		if(remainingIndex!=NULL)
			{
			LOG(LOG_INFO,"Unable to move %li in generation %i",remainingIndex->sizeLive,generation);
			dumpCircHeap(circHeap);

			LOG(LOG_CRITICAL,"GC heap compact failed");
			}

//		LOG(LOG_INFO,"Iter");
//		dumpCircHeap(circHeap);

		spaceEstimate=estimateSpaceInGeneration(circHeap->generations+generation);

		maxReclaims--;

		if(spaceEstimate < targetFree && maxReclaims>0)
			index=createReclaimIndex(circHeap,circHeap->generations+generation, generation, 0, CIRCHEAP_RECLAIM_SPLIT_FULL_SHIFT, disp);
		else
			index=NULL;
		}

}


static void circHeapEnsureSpace_generation(MemCircHeap *circHeap, size_t supportedAllocSize, int generation, MemDispenser *disp)
{
	s64 spaceEstimate=estimateSpaceInGeneration(circHeap->generations+generation);

	s64 targetFree=supportedAllocSize; // Can be doubled - Hacky - allows for start/end split reclaims not creating enough space

	if(spaceEstimate>supportedAllocSize)
		{
		//LOG(LOG_INFO,"Generation %i estimates %li space, sufficient for %li (adjusted to %li)",generation, spaceEstimate, newAllocSize, targetFree);
		return;
		}

	if(targetFree*2 > circHeap->generations[generation].allocBlock->size)
		{
		LOG(LOG_INFO,"Gonna need a bigger boat");
		circHeap->generations[generation].allocBlock->needsExpanding=1;
		}

//	LOG(LOG_INFO,"Generation %i estimates %li space, insufficient for %li (adjusted to %li)",generation, spaceEstimate, supportedAllocSize, targetFree);


	if(generation<CIRCHEAP_LAST_GENERATION)
		{
//		LOG(LOG_INFO,"GC Promote for %i",generation);
//		dumpCircHeap(circHeap);

		int nextGeneration=generation+1;
		s64 reclaimTarget=targetFree-spaceEstimate; // Allows for start/end split reclaims not creating enough space

		int maxReclaims=CIRCHEAP_RECLAIM_SPLIT;

		MemCircHeapChunkIndex *index=createReclaimIndex(circHeap,circHeap->generations+generation, generation, reclaimTarget, CIRCHEAP_RECLAIM_SPLIT_SHIFT, disp);

		while(index!=NULL)
			{
			s64 totalLive=calcHeapReclaimStats(index,circHeap->generations+generation, generation);
			circHeapEnsureSpace_generation(circHeap,totalLive,nextGeneration,disp);
			MemCircHeapChunkIndex *remainingIndex=moveIndexedHeap(circHeap, index, nextGeneration);

			if(remainingIndex!=NULL)
				{
				LOG(LOG_INFO,"GC generation promotion failed - retrying");

				circHeapEnsureSpace_generation(circHeap,totalLive,nextGeneration,disp);
				MemCircHeapChunkIndex *remainingIndex2=moveIndexedHeap(circHeap, index, nextGeneration);

				if(remainingIndex2!=NULL)
					{
					LOG(LOG_CRITICAL,"GC generation promotion failed");
					}
				}

			spaceEstimate=estimateSpaceInGeneration(circHeap->generations+generation);

			maxReclaims--;

			if(spaceEstimate<targetFree && maxReclaims>0)
				index=createReclaimIndex(circHeap,circHeap->generations+generation, generation, reclaimTarget, CIRCHEAP_RECLAIM_SPLIT_SHIFT, disp);
			else
				index=NULL;
			}

		if(circHeap->generations[generation].reclaimBlock->needsExpanding)
			{
//			LOG(LOG_INFO,"Generation %i marked for expansion", generation);
//			LOG(LOG_INFO,"Alloc block: %p - needs expansion", circHeap->generations[generation].allocBlock->ptr);

			MemCircHeapBlock *newAllocBlock=allocBlock(circHeap->generations[generation].reclaimBlock->size+supportedAllocSize, circHeap->generations[generation].reclaimBlock->sizeIndex+1);
			circHeap->generations[generation].allocBlock=newAllocBlock;

			circHeap->generations[generation].allocCurrentChunkPtr=NULL;

			//LOG(LOG_INFO,"Reclaim via compaction for expanded generation %i, to free %li",generation, targetFree);
			circHeapCompact(circHeap, generation, newAllocBlock->size, disp);

			if(circHeap->generations[generation].reclaimBlock->allocPosition!=circHeap->generations[generation].reclaimBlock->reclaimPosition)
				{
				LOG(LOG_CRITICAL,"GC generation expansion failed");
				}

			circHeap->generations[generation].prevReclaimSizeLive=circHeap->generations[generation].reclaimBlock->reclaimSizeLive;
			circHeap->generations[generation].prevReclaimSizeDead=circHeap->generations[generation].reclaimBlock->reclaimSizeDead;

			freeBlockSafe(circHeap->generations[generation].reclaimBlock);
			circHeap->generations[generation].reclaimBlock=newAllocBlock;
			}


		//LOG(LOG_INFO,"GC completed for generation %i",generation);
		}
	else
		{
//		LOG(LOG_INFO,"Reclaim via compaction for generation %i, to free %li, currently space %i",generation, targetFree, spaceEstimate);
		circHeapCompact(circHeap, generation, targetFree, disp);
//		LOG(LOG_INFO,"Reclaim via compaction ok");

		spaceEstimate=estimateSpaceInGeneration(circHeap->generations+generation);

		if(spaceEstimate<targetFree || circHeap->generations[generation].reclaimBlock->needsExpanding)
			{
//			LOG(LOG_INFO,"Generation %i marked for expansion", generation);
//			LOG(LOG_INFO,"Alloc block: %p - needs expansion", circHeap->generations[generation].allocBlock->ptr);

			MemCircHeapBlock *newAllocBlock=allocBlock(circHeap->generations[generation].reclaimBlock->size+supportedAllocSize, circHeap->generations[generation].reclaimBlock->sizeIndex+1);
			circHeap->generations[generation].allocBlock=newAllocBlock;

			circHeap->generations[generation].allocCurrentChunkPtr=NULL;

//			LOG(LOG_INFO,"Reclaim via compaction for expanded generation %i, to free %li",generation, targetFree);
			circHeapCompact(circHeap, generation, newAllocBlock->size, disp);
//			LOG(LOG_INFO,"Reclaim via compaction ok");

			if(circHeap->generations[generation].reclaimBlock->allocPosition!=circHeap->generations[generation].reclaimBlock->reclaimPosition)
				{
				LOG(LOG_CRITICAL,"GC generation expansion failed");
				}

			circHeap->generations[generation].prevReclaimSizeLive=circHeap->generations[generation].reclaimBlock->reclaimSizeLive;
			circHeap->generations[generation].prevReclaimSizeDead=circHeap->generations[generation].reclaimBlock->reclaimSizeDead;

			freeBlockSafe(circHeap->generations[generation].reclaimBlock);
			circHeap->generations[generation].reclaimBlock=newAllocBlock;
			}

		}


}

static void circHeapEnsureSpace(MemCircHeap *circHeap, size_t newAllocSize)
{
	MemDispenser *disp=dispenserAlloc("GC", DISPENSER_BLOCKSIZE_MEDIUM, DISPENSER_BLOCKSIZE_LARGE);

	circHeapEnsureSpace_generation(circHeap, newAllocSize, 0, disp);


	dispenserFree(disp);
}


void *circAlloc_nobumper(MemCircHeap *circHeap, size_t size, u8 tag, s32 newTagOffset, s32 *oldTagOffset)
{
	if(size>CIRCHEAP_MAX_ALLOC)
		{
		LOG(LOG_CRITICAL, "Request to allocate %i bytes of ColHeap memory refused since it is above the max allocation size %i",size,CIRCHEAP_MAX_ALLOC);
		}
	if(size>CIRCHEAP_MAX_ALLOC_WARN)
		{
		LOG(LOG_INFO, "Request to allocate %i bytes of ColHeap memory logged since it is above the warn allocation size %i",size,CIRCHEAP_MAX_ALLOC_WARN);
		}


	//LOG(LOG_INFO,"circAlloc_nobumper: Requesting %i with tag %i offset %i",size);

	// Simple case - alloc from the current young block
	void *usrPtr=allocFromGeneration(circHeap->generations, size, tag, newTagOffset, newTagOffset, oldTagOffset);
	if(usrPtr!=NULL)
		{
//		LOG(LOG_INFO,"circAlloc_nobumper: Alloc of %i to %p (to %p) with tag %i newTagOffset %i",size,usrPtr,((u8 *)usrPtr)+size-1,tag,newTagOffset);
		return usrPtr;
		}

	//LOG(LOG_INFO, "Initial fail to allocate %i bytes of CircHeap memory, GC begins",size);

	circHeapEnsureSpace(circHeap, size);

	usrPtr=allocFromGeneration(circHeap->generations, size, tag, newTagOffset, newTagOffset, oldTagOffset);
	if(usrPtr!=NULL)
		{
//		LOG(LOG_INFO,"circAlloc_nobumper: Alloc of %i to %p (to %p) with tag %i newTagOffset %i",size,usrPtr,((u8 *)usrPtr)+size-1,tag,newTagOffset);
		return usrPtr;
		}

	LOG(LOG_INFO, "Failed to allocate %i bytes of CircHeap memory after trying really hard, honest",size);
	dumpCircHeap(circHeap);

	LOG(LOG_CRITICAL,"Bailing out");

	return NULL;
}


void *circAlloc(MemCircHeap *circHeap, size_t size, u8 tag, s32 newTagOffset, s32 *oldTagOffset)
{
	void *ptr=circAlloc_nobumper(circHeap, size, tag, newTagOffset, oldTagOffset);
	return ptr;
}

/*
 * Later: Need a way to add bumpers to multiple allocations
 *
void *circAlloc(MemCircHeap *circHeap, size_t size, u8 tag, s32 newTagOffset, s32 *oldTagOffset)
{
#ifdef CH_BUMPER
	int allocSize=size+1;
#else
	int allocSize=size;
#endif

	void *usrPtr=circAlloc_test(circHeap, allocSize, tag, newTagOffset, oldTagOffset);

#ifdef CH_BUMPER
	if(usrPtr!=NULL)
		{
		u8 *dataPtr=(u8 *)usrPtr;
		dataPtr[size]=CH_BUMPER;
		}
#endif

	return usrPtr;
}
*/
