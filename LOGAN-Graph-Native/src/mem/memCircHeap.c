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



static s64 getRequiredBlocksize(s64 minSize)
{
	minSize*=2;

	if(minSize>CIRCHEAP_MAX_BLOCKSIZE)
		LOG(LOG_CRITICAL,"Cannot use allocation blocksize above maximum %li %li",minSize,CIRCHEAP_MAX_BLOCKSIZE);

	s64 blockSize=CIRCHEAP_DEFAULT_BLOCKSIZE;

	while(blockSize<minSize)
		blockSize*=2;

	return blockSize;
}

static MemCircHeapBlock *allocBlock(s32 minSize)
{
	s64 size=getRequiredBlocksize(minSize);

	MemCircHeapBlock *blockPtr=NULL;
	u8 *dataPtr=NULL;

	if(posix_memalign((void **)&blockPtr, CACHE_ALIGNMENT_SIZE, sizeof(MemCircHeapBlock))!=0)
		LOG(LOG_CRITICAL,"Failed to alloc MemCircHeapBlockAllocation structure");

	if(posix_memalign((void **)&dataPtr, PAGE_ALIGNMENT_SIZE*16, size)!=0)
		LOG(LOG_CRITICAL,"Failed to alloc MemCircHeapBlockAllocation data");

	blockPtr->next=NULL;

	blockPtr->ptr=dataPtr;
	blockPtr->size=size;
	blockPtr->allocPosition=0;
	blockPtr->reclaimPosition=0;

	return blockPtr;
}

static void freeBlock(MemCircHeapBlock *block)
{
	free(block->ptr);
	free(block);
}


MemCircHeap *circHeapAlloc(MemCircHeapChunkIndex *(*reclaimIndexer)(u8 *data, s64 targetAmount, u8 tag, u8 **tagData, s32 tagDataLength, MemDispenser *disp),
		void (*relocater)(MemCircHeapChunkIndex *reclaimIndex, u8 tag, u8 **tagData, s32 tagDataLength))
{
	MemCircHeap *circHeap=NULL;

	if(posix_memalign((void **)&circHeap, CACHE_ALIGNMENT_SIZE, sizeof(MemCircHeap))!=0)
		LOG(LOG_CRITICAL,"Failed to alloc MemCircHeap structure");

	for(int i=0;i<CIRCHEAP_MAX_GENERATIONS;i++)
		{
		MemCircHeapBlock *block=allocBlock(0);

		circHeap->generations[i].allocBlock=block;
		circHeap->generations[i].reclaimBlock=block;

		circHeap->generations[i].allocCurrentChunkTag=0;
		circHeap->generations[i].allocCurrentChunkPtr=0;

		circHeap->generations[i].reclaimCurrentChunkTag=0;

		circHeap->generations[i].curReclaimSizeLive=0;
		circHeap->generations[i].curReclaimSizeDead=0;

		circHeap->generations[i].prevReclaimSizeLive=0;
		circHeap->generations[i].prevReclaimSizeDead=0;
		}

	circHeap->reclaimIndexer=reclaimIndexer;
	circHeap->relocater=relocater;

	return circHeap;
}

void circHeapFree(MemCircHeap *circHeap)
{
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


static s64 estimateSpaceInGeneration(MemCircGeneration *generation)
{
	MemCircHeapBlock *block=generation->allocBlock;

	s64 space;

	if(block->reclaimPosition<=block->allocPosition) // Start - reclaim - alloc - end (bigger of the two)
		{
		space=MAX(block->size-block->allocPosition, block->reclaimPosition)-CIRCHEAP_BLOCK_TAG_OVERHEAD;
		}
	else											// Start - alloc - reclaim  - end
		{
		space=block->reclaimPosition-block->allocPosition-CIRCHEAP_BLOCK_TAG_OVERHEAD-1;
		}

	return MAX(0,space);
}

// Allocates from a generation, creating a tag if needed
static void *allocFromGeneration(MemCircGeneration *generation, size_t newAllocSize, u8 tag)
{
	if(newAllocSize<0)
		{
		LOG(LOG_CRITICAL,"Cannot allocate negative space %i",newAllocSize);
		}

	int needTag=(generation->allocCurrentChunkTag!=tag || generation->allocCurrentChunkPtr==NULL);

	if(needTag)
		newAllocSize+=2; // Allow for chunk marker and tag

	/*
	LOG(LOG_INFO,"Alloc is %p %li, reclaim is %p %li",
			generation->allocBlock->ptr, generation->allocBlock->allocPosition,
			generation->reclaimBlock->ptr, generation->reclaimBlock->reclaimPosition);
*/

	MemCircHeapBlock *block=generation->allocBlock;
	s64 positionAfterAlloc=block->allocPosition+newAllocSize;

	// Avoid catching reclaim position

	if(block->reclaimPosition>block->allocPosition && block->reclaimPosition<=positionAfterAlloc)
		return NULL;

	// Easy case, alloc at current position, with optional tag
	if(positionAfterAlloc<block->size) // Always keep one byte gap
		{
		u8 *data=block->ptr+block->allocPosition;

		if(needTag)
			{
			generation->allocCurrentChunkTag=tag;
			generation->allocCurrentChunkPtr=data;

			*data++=CH_HEADER_CHUNK;
			*data++=tag;
			}

		block->allocPosition=positionAfterAlloc;
		return data;
		}

	// Wrap case, force tag and check for space before reclaim

	if(!needTag)
		newAllocSize+=2;

	if(block->reclaimPosition>newAllocSize)
		{
		// First mark end of used region
		u8 *data=block->ptr+block->allocPosition;
		*data=CH_HEADER_INVALID;

		data=block->ptr;

		generation->allocCurrentChunkTag=tag;
		generation->allocCurrentChunkPtr=data;

		*data++=CH_HEADER_CHUNK;
		*data++=tag;

		block->allocPosition=newAllocSize;

		return data;
		}

	return NULL;
}


static MemCircHeapChunkIndex *createReclaimIndex_single(MemCircHeap *circHeap, MemCircGeneration *generation, s64 reclaimTargetAmount, MemDispenser *disp)
{
	MemCircHeapBlock *reclaimBlock=generation->reclaimBlock;

	if(reclaimBlock->reclaimPosition==reclaimBlock->allocPosition)
		return NULL;

	// Could be 'INVALID', 'CHUNK' or other
	u8 data=reclaimBlock->ptr[reclaimBlock->reclaimPosition];

	if(data==CH_HEADER_INVALID) // Wrap reclaim
		{
		// Should verify if live/dead ratio acceptable
		long totalReclaim=generation->curReclaimSizeLive+generation->curReclaimSizeDead;

		if(totalReclaim>0)
			{
			double survivorRatio=(double)generation->curReclaimSizeLive/(double)totalReclaim;

			LOG(LOG_INFO,"Generation %i Survivor Ratio %lf",
					generation-circHeap->generations, survivorRatio);
			}

		generation->prevReclaimSizeLive=generation->curReclaimSizeLive;
		generation->prevReclaimSizeDead=generation->curReclaimSizeDead;

		generation->curReclaimSizeLive=0;
		generation->curReclaimSizeDead=0;
		reclaimBlock->reclaimPosition=0;

		if(reclaimBlock->reclaimPosition==reclaimBlock->allocPosition)
			return NULL;

		data=reclaimBlock->ptr[0];
		}

	if(data==CH_HEADER_CHUNK)
		{
		generation->reclaimCurrentChunkTag=reclaimBlock->ptr[reclaimBlock->reclaimPosition+1];
		reclaimBlock->reclaimPosition+=2;
		reclaimTargetAmount-=2;
		}

	u8 *reclaimPtr=reclaimBlock->ptr+reclaimBlock->reclaimPosition;

	if((reclaimBlock->reclaimPosition < reclaimBlock->allocPosition) && (reclaimBlock->allocPosition-reclaimBlock->reclaimPosition < reclaimTargetAmount))
		reclaimTargetAmount=reclaimBlock->allocPosition-reclaimBlock->reclaimPosition;

	u8 tag=generation->reclaimCurrentChunkTag;
	MemCircHeapChunkIndex *indexedChunk=(circHeap->reclaimIndexer)(reclaimPtr, reclaimTargetAmount, tag, circHeap->tagData[tag], circHeap->tagDataLength[tag], disp);

	indexedChunk->chunkTag=generation->reclaimCurrentChunkTag;

	indexedChunk->reclaimed=indexedChunk->heapEndPtr-reclaimPtr;
	reclaimBlock->reclaimPosition+=indexedChunk->reclaimed;

	if(data==CH_HEADER_CHUNK)
		indexedChunk->reclaimed+=2;

//	LOG(LOG_INFO,"Creating Reclaim Index for tag %i moving reclaim position by %li to %i",tag, indexedChunk->reclaimed, reclaimBlock->reclaimPosition);


	return indexedChunk;
}

static MemCircHeapChunkIndex *createReclaimIndex(MemCircHeap *circHeap, MemCircGeneration *generation, s64 reclaimTargetAmount, int splitShift, MemDispenser *disp)
{
	reclaimTargetAmount=MAX(reclaimTargetAmount, (generation->reclaimBlock->size)>>splitShift);

	//LOG(LOG_INFO,"Creating Reclaim Index with reclaim target amount: %li",reclaimTargetAmount);

	MemCircHeapChunkIndex *index=createReclaimIndex_single(circHeap,generation,reclaimTargetAmount,disp);

	MemCircHeapChunkIndex *firstIndex=index;

	while(index!=NULL)
		{
		reclaimTargetAmount-=index->reclaimed;
		if(reclaimTargetAmount<=0)
			{
			index->next=NULL;
			break;
			}

		index->next=createReclaimIndex_single(circHeap,generation,reclaimTargetAmount,disp);
		index=index->next;
		}

	return firstIndex;
}

static void dumpCircHeap(MemCircHeap *circHeap)
{
	LOG(LOG_INFO,"Heap dump");
	for(int i=0;i<CIRCHEAP_MAX_GENERATIONS;i++)
		LOG(LOG_INFO, "Generation %i has %li with alloc %li and reclaim %li",i,
			estimateSpaceInGeneration(circHeap->generations+i),
			circHeap->generations[i].allocBlock->allocPosition,
			circHeap->generations[i].allocBlock->reclaimPosition);
}

static MemCircHeapChunkIndex *moveIndexedHeap(MemCircHeap *circHeap, MemCircHeapChunkIndex *index, s32 allocGeneration)
{
	MemCircHeapChunkIndex *scan=index;

	while(scan!=NULL)
		{
		u8 *newChunk=allocFromGeneration(circHeap->generations+allocGeneration, scan->sizeLive, scan->chunkTag);

		if(newChunk==NULL) // Shouldn't happen, but just in case, demand more
			{
//			circHeapEnsureSpace_generation(circHeap,totalLive+CIRCHEAP_BLOCK_OVERHEAD,allocGeneration,disp);
//			newChunk=allocFromGeneration(circHeap->generations+allocGeneration, scan->sizeLive, scan->chunkTag);
//			if(newChunk==NULL)
//				LOG(LOG_CRITICAL,"GC generation promotion failed");

			return scan;
			}

		scan->newChunk=newChunk;
		u8 tag=scan->chunkTag;

		(circHeap->relocater)(scan, tag, circHeap->tagData[tag], circHeap->tagDataLength[tag]);

		scan=scan->next;
		}

	return NULL;
}

static s64 calcHeapReclaimStats(MemCircHeapChunkIndex *index, MemCircGeneration *generation, int generationNum)
{
	s64 totalReclaimed=0;
	s64 totalLive=0;
	s64 totalDead=0;

	MemCircHeapChunkIndex *scan=index;
	while(scan!=NULL)
		{
		totalReclaimed+=scan->reclaimed;
		totalLive+=scan->sizeLive+2;
		totalDead+=scan->sizeDead;

		scan=scan->next;
		}

	generation->curReclaimSizeLive+=totalLive;
	generation->curReclaimSizeDead+=totalDead;

	//LOG(LOG_INFO,"Generation %i can reclaim %li with %li live, %li dead",generationNum, totalReclaimed, totalLive, totalDead);

	return totalLive;
}


static void circHeapEnsureSpace_generation(MemCircHeap *circHeap, size_t newAllocSize, int generation, MemDispenser *disp)
{
	s64 spaceEstimate=estimateSpaceInGeneration(circHeap->generations+generation);
	spaceEstimate=MAX(0,spaceEstimate-CIRCHEAP_BLOCK_OVERHEAD);

	s64 targetFree=newAllocSize; // Can be doubled - Hacky - allows for start/end split reclaims not creating enough space

	if(spaceEstimate>newAllocSize)
		{
		//LOG(LOG_INFO,"Generation %i estimates %li space, sufficient for %li (adjusted to %li)",generation, spaceEstimate, newAllocSize, targetFree);
		return;
		}

	//LOG(LOG_INFO,"Generation %i estimates %li space, insufficient for %li (adjusted to %li)",generation, spaceEstimate, newAllocSize, targetFree);


	if(generation<CIRCHEAP_LAST_GENERATION)
		{
		int nextGeneration=generation+1;
		s64 reclaimTarget=targetFree-spaceEstimate; // Allows for start/end split reclaims not creating enough space

		int maxReclaims=2;

		while(spaceEstimate<targetFree && maxReclaims>0)
			{
			MemCircHeapChunkIndex *index=createReclaimIndex(circHeap,circHeap->generations+generation, reclaimTarget, CIRCHEAP_RECLAIM_SPLIT_SHIFT, disp);

			s64 totalLive=calcHeapReclaimStats(index,circHeap->generations+generation, generation);

			circHeapEnsureSpace_generation(circHeap,totalLive,nextGeneration,disp);
			MemCircHeapChunkIndex *remainingIndex=moveIndexedHeap(circHeap, index, nextGeneration);

			if(remainingIndex!=NULL)
				{
				LOG(LOG_CRITICAL,"GC generation promotion failed");
				}
			/*
				circHeapEnsureSpace_generation(circHeap,totalLive,nextGeneration,disp);
				MemCircHeapChunkIndex *remainingIndex2=moveIndexedHeap(circHeap, index, nextGeneration);

				if(remainingIndex2!=NULL)
					{
					LOG(LOG_CRITICAL,"GC generation promotion failed");
					}
				}
*/

			spaceEstimate=estimateSpaceInGeneration(circHeap->generations+generation);
			spaceEstimate=MAX(0,spaceEstimate-CIRCHEAP_BLOCK_OVERHEAD);

			maxReclaims--;
			}

//		LOG(LOG_INFO,"Reclaim completed for generation %i",generation);
		}
	else
		{
		LOG(LOG_INFO,"Reclaim via compaction for generation %i, to free %li",generation, targetFree);

		dumpCircHeap(circHeap);

		int maxReclaims=CIRCHEAP_RECLAIM_SPLIT_OLDEST;

		MemCircHeapChunkIndex *index=createReclaimIndex(circHeap,circHeap->generations+generation, 0, CIRCHEAP_RECLAIM_SPLIT_OLDEST_SHIFT, disp);
		calcHeapReclaimStats(index,circHeap->generations+generation, generation);

		moveIndexedHeap(circHeap, index, generation);

		LOG(LOG_INFO,"First");
		dumpCircHeap(circHeap);

		spaceEstimate=estimateSpaceInGeneration(circHeap->generations+generation);
		spaceEstimate=MAX(0,spaceEstimate-CIRCHEAP_BLOCK_OVERHEAD);

		while(spaceEstimate < targetFree && maxReclaims>0)
			{
			MemCircHeapChunkIndex *index=createReclaimIndex(circHeap,circHeap->generations+generation, 0, CIRCHEAP_RECLAIM_SPLIT_OLDEST_SHIFT, disp);
			calcHeapReclaimStats(index,circHeap->generations+generation, generation);

			moveIndexedHeap(circHeap, index, generation);

			LOG(LOG_INFO,"Iter");
			dumpCircHeap(circHeap);

			spaceEstimate=estimateSpaceInGeneration(circHeap->generations+generation);
			spaceEstimate=MAX(0,spaceEstimate-CIRCHEAP_BLOCK_OVERHEAD);

			maxReclaims--;
			}

		if(spaceEstimate<targetFree)
			{
			LOG(LOG_CRITICAL,"Need to grow heap for generation %i - could create only %li of %li needed",generation,spaceEstimate,targetFree);
			}

		}


}

static void circHeapEnsureSpace(MemCircHeap *circHeap, size_t newAllocSize)
{
	MemDispenser *disp=dispenserAlloc("GC");

	circHeapEnsureSpace_generation(circHeap, newAllocSize, 0, disp);


	dispenserFree(disp);
}


void *circAlloc(MemCircHeap *circHeap, size_t size, u8 tag)
{
	if(size>CIRCHEAP_MAX_ALLOC)
		{
		LOG(LOG_CRITICAL, "Request to allocate %i bytes of ColHeap memory refused since it is above the max allocation size %i",size,CIRCHEAP_MAX_ALLOC);
		}

//	LOG(LOG_INFO,"Allocating %i",size);

	// Simple case - alloc from the current young block
	void *usrPtr=allocFromGeneration(circHeap->generations, size, tag);
	if(usrPtr!=NULL)
		return usrPtr;

	//LOG(LOG_INFO, "Initial fail to allocate %i bytes of CircHeap memory, GC begins",size);

	circHeapEnsureSpace(circHeap, size);

	usrPtr=allocFromGeneration(circHeap->generations, size, tag);
	if(usrPtr!=NULL)
		return usrPtr;

	LOG(LOG_INFO, "Failed to allocate %i bytes of CircHeap memory after trying really hard, honest",size);

	for(int i=0;i<CIRCHEAP_MAX_GENERATIONS;i++)
		LOG(LOG_INFO, "Generation %i has %li with alloc %li and reclaim %li",i, estimateSpaceInGeneration(circHeap->generations+i),
				circHeap->generations[i].allocBlock->allocPosition, circHeap->generations[i].allocBlock->reclaimPosition);

	LOG(LOG_CRITICAL,"Bailing out");

	return NULL;
}




