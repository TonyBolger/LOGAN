
#include "common.h"



// Called during init() or while holding pile->lock, but still needs to be thread-safe against regular uses

static void initSingleBrickChunk(MemSingleBrickPile *pile)
{
	if(pile->chunkCount>=pile->chunkLimit)
		LOG(LOG_CRITICAL,"Cannot init another chunk: Pile at limit");

	MemSingleBrickChunk *chunk=G_ALLOC_ALIGNED(sizeof(MemSingleBrickChunk), SINGLEBRICK_ALIGNMENT, pile->memTrackId);

	chunk->freeCount=SINGLEBRICK_BRICKS_PER_CHUNK;
	chunk->scanPosition=0;

	memset(chunk->freeFlag,0xFF,sizeof(chunk->freeFlag));

	pile->chunks[pile->chunkCount]=chunk;

	__atomic_fetch_add(&pile->chunkCount, 1, __ATOMIC_SEQ_CST);
	__atomic_fetch_add(&pile->freeCount, SINGLEBRICK_BRICKS_PER_CHUNK, __ATOMIC_SEQ_CST);

}


void mbInitSingleBrickPile(MemSingleBrickPile *pile, s32 chunkCount, s32 chunkLimit, s32 memTrackId)
{
	pile->chunks=G_ALLOC(sizeof(MemSingleBrickChunk *)*chunkLimit, memTrackId);
	memset(pile->chunks, 0, sizeof(MemSingleBrickChunk *)*chunkLimit);

	pile->memTrackId=memTrackId;
	pile->chunkLimit=chunkLimit;
	pile->chunkCount=0;
	pile->freeCount=0;

	pile->scanChunk=0;

	for(int i=0;i<chunkCount;i++)
		initSingleBrickChunk(pile);

}


s32 mbGetFreeSingleBrickPile(MemSingleBrickPile *pile)
{
	return __atomic_load_n(&(pile->freeCount),__ATOMIC_RELAXED);
}

s32 mbGetFreeSingleBrickChunk(MemSingleBrickChunk *chunk)
{
	s32 free=__atomic_load_n(&(chunk->freeCount),__ATOMIC_RELAXED);
	LOG(LOG_INFO,"Free %i",free);
	return free;
}


s32 mbCheckSingleBrickAvailability(MemSingleBrickPile *pile, s32 brickCount)
{
	s32 availableCount=mbGetFreeSingleBrickPile(pile);
	int needed=brickCount+pile->chunkCount*SINGLEBRICK_PILE_MARGIN_PER_CHUNK;

	return needed<availableCount;
}


static s32 singleBrickAttemptPileReserve(MemSingleBrickPile *pile, s32 resRequest)
{
	for(int loopCount=0;loopCount<100000;loopCount++)
		{
		s32 freeCount=__atomic_load_n(&(pile->freeCount), __ATOMIC_RELAXED);

		if(freeCount<resRequest)
			return 0;

		s32 newFreeCount=freeCount-resRequest;

		if(__atomic_compare_exchange_n(&(pile->freeCount), &freeCount, newFreeCount, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
			return 1;
		}

	LOG(LOG_CRITICAL,"Failed to reserve Bricks, should never happen"); // Can actually happen, just stupidly unlikely
	return 0;
}


static s32 singleBrickPileScanForUsableChunk(MemSingleBrickPile *pile, s32 startChunk)
{
	LOG(LOG_INFO,"singleBrickPileScanForUsableChunk %i",startChunk);

	for(int i=startChunk;i<pile->chunkCount;i++)
		{
		if(mbGetFreeSingleBrickChunk(pile->chunks[i])>SINGLEBRICK_CHUNK_MARGIN_PER_CHUNK && __atomic_load_n(&(pile->chunks[i]->scanPosition),__ATOMIC_RELAXED)>=0)
			return i;
		}

	for(int i=0;i<startChunk;i++)
		{
		if(mbGetFreeSingleBrickChunk(pile->chunks[i])>SINGLEBRICK_CHUNK_MARGIN_PER_CHUNK && __atomic_load_n(&(pile->chunks[i]->scanPosition),__ATOMIC_RELAXED)>=0)
			return i;
		}

	return -1;
}


static s32 singleBrickLockChunkForAllocation(MemSingleBrickChunk *chunk)
{
	s32 scanPosition=__atomic_load_n(&(chunk->scanPosition), __ATOMIC_RELAXED);

	if(scanPosition==SINGLEBRICK_SCANPOSITION_LOCKED)
		return scanPosition;

	s32 newScanPosition=SINGLEBRICK_SCANPOSITION_LOCKED;

	if(__atomic_compare_exchange_n(&(chunk->scanPosition), &scanPosition, newScanPosition, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		return scanPosition;

	return SINGLEBRICK_SCANPOSITION_LOCKED;
}


static s32 singleBrickAllocatorScanForChunkAndLock(MemSingleBrickAllocator *alloc)
{
	MemSingleBrickPile *pile=alloc->pile;
	s32 startChunk=pile->scanChunk;

	for(int loopCount=0;loopCount<100000;loopCount++)
		{
		s32 scanResult=singleBrickPileScanForUsableChunk(pile, startChunk);

		LOG(LOG_INFO,"Scan result: %i",scanResult);

		if(scanResult>=0)
			{
			int scanPosition=singleBrickLockChunkForAllocation(pile->chunks[scanResult]);

			LOG(LOG_INFO,"Scan position: %i",scanPosition);

			if(scanPosition>=0)
				{
				alloc->chunk=pile->chunks[scanResult];
				alloc->scanChunk=scanResult;
				return scanPosition;
				}
			}

		startChunk++;
		if(startChunk>pile->chunkCount)
			startChunk=0;
		}

	LOG(LOG_CRITICAL,"Loop Stuck");
	return 0;
}


static s32 singleBlockAllocateGroup(MemSingleBrickAllocator *alloc)
{
	MemSingleBrickChunk *chunk=alloc->chunk;

	for(int i=alloc->scanPosition; i<SINGLEBRICK_FLAGS_PER_CHUNK; i++)
		{
		u32 allocMask=__atomic_load_n(chunk->freeFlag+i, __ATOMIC_RELAXED);

		if(allocMask!=0)
			{
			if(__atomic_compare_exchange_n(chunk->freeFlag+i, &allocMask, 0, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
				{
				s16 nextScanPosition=i+1;
				if(nextScanPosition>SINGLEBRICK_FLAGS_PER_CHUNK)
					nextScanPosition=0;

				alloc->scanPosition=nextScanPosition;
				alloc->allocMask=allocMask;

				s16 allocCount=__builtin_popcount(allocMask);
				alloc->allocCount=allocCount;

				__atomic_fetch_sub(&(chunk->freeCount), allocCount, __ATOMIC_RELAXED);

				return allocCount;
				}

			}
		}


	for(int i=0; i<alloc->scanPosition; i++)
		{
		u32 allocMask=__atomic_load_n(chunk->freeFlag+i, __ATOMIC_RELAXED);

		if(allocMask!=0)
			{
			if(__atomic_compare_exchange_n(chunk->freeFlag+i, &allocMask, 0, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
				{
				s16 nextScanPosition=i+1;
				if(nextScanPosition>SINGLEBRICK_FLAGS_PER_CHUNK)
					nextScanPosition=0;

				alloc->scanPosition=nextScanPosition;
				alloc->allocMask=allocMask;

				s16 allocCount=__builtin_popcount(allocMask);
				alloc->allocCount=allocCount;

				__atomic_fetch_sub(&(chunk->freeCount), allocCount, __ATOMIC_RELAXED);

				return allocCount;
				}

			}
		}

	LOG(LOG_CRITICAL,"Failed to alloc group");
	return 0;
}

s32 mbInitSingleBrickAllocator(MemSingleBrickAllocator *alloc, MemSingleBrickPile *pile)
{
	alloc->pile=pile;
	alloc->pileResRequest=SINGLEBRICK_DEFAULT_RESERVATION;

	if(!singleBrickAttemptPileReserve(pile, alloc->pileResRequest))
		{
		LOG(LOG_INFO,"Pile Reserve Failed");
		return 0;
		}

	alloc->pileResLeft=alloc->pileResRequest;

	int scanPosition=singleBrickAllocatorScanForChunkAndLock(alloc);

	if(scanPosition<0)
		{
		LOG(LOG_INFO,"Chunk Scan/Lock Failed");
		return 0;
		}

	alloc->scanPosition=scanPosition;

	s32 allocCount=singleBlockAllocateGroup(alloc);
	LOG(LOG_INFO,"Pre-Allocated %i", allocCount);
	return allocCount;
}

u8 *mbSingleBrickAllocate(MemSingleBrickAllocator *alloc, s32 *brickIndexPtr)
{
	return NULL;
}

void mbSingleBrickAllocatorCleanup(MemSingleBrickAllocator *alloc)
{

}


void mbSingleBrickFree(u8 *brick)
{

}


