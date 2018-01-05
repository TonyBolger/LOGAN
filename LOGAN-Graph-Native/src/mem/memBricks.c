
#include "common.h"



// Called during init() (eventually also while holding pile->lock) but still needs to be thread-safe against regular uses

static void initSingleBrickChunk(MemSingleBrickPile *pile)
{
	if(pile->chunkCount>=pile->chunkLimit)
		LOG(LOG_CRITICAL,"Cannot init another chunk: Pile at limit");

	MemSingleBrickChunk *chunk=G_ALLOC_ALIGNED(sizeof(MemSingleBrickChunk), SINGLEBRICK_ALIGNMENT, pile->memTrackId);

	chunk->header.freeCount=SINGLEBRICK_BRICKS_PER_CHUNK;
	chunk->header.flagPosition=SINGLEBRICK_SKIP_FLAGS_PER_CHUNK;

	memset(chunk->freeFlag+SINGLEBRICK_SKIP_FLAGS_PER_CHUNK,0xFF,sizeof(u64)*(SINGLEBRICK_FLAGS_PER_CHUNK));

	pile->chunks[pile->chunkCount]=chunk;

	__atomic_fetch_add(&pile->chunkCount, 1, __ATOMIC_SEQ_CST);
	__atomic_fetch_add(&pile->freeCount, SINGLEBRICK_BRICKS_PER_CHUNK, __ATOMIC_SEQ_CST);

}


void mbInitSingleBrickPile(MemSingleBrickPile *pile, s32 chunkCount, s32 chunkLimit, s32 memTrackId)
{
	if(sizeof(MemSingleBrickChunk)!=SINGLEBRICK_ALIGNMENT)
		LOG(LOG_CRITICAL,"Unexpected MemSingleBrickChunk Size/Alignment mismatch: %i vs %i", sizeof(MemSingleBrickChunk), SINGLEBRICK_ALIGNMENT);

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

void mbFreeSingleBrickPile(MemSingleBrickPile *pile)
{
	for(int i=0;i<pile->chunkCount;i++)
		G_FREE(pile->chunks[i], sizeof(MemSingleBrickChunk), pile->memTrackId);

	G_FREE(pile->chunks, sizeof(MemSingleBrickChunk *)*pile->chunkLimit, pile->memTrackId);
}


// Called during init() (eventually also while holding pile->lock) but still needs to be thread-safe against regular uses

static void initDoubleBrickChunk(MemDoubleBrickPile *pile)
{
	if(pile->chunkCount>=pile->chunkLimit)
		LOG(LOG_CRITICAL,"Cannot init another chunk: Pile at limit");

	MemDoubleBrickChunk *chunk=G_ALLOC_ALIGNED(sizeof(MemDoubleBrickChunk), DOUBLEBRICK_ALIGNMENT, pile->memTrackId);

	chunk->header.freeCount=DOUBLEBRICK_BRICKS_PER_CHUNK;
	chunk->header.flagPosition=DOUBLEBRICK_SKIP_FLAGS_PER_CHUNK;

	memset(chunk->freeFlag+DOUBLEBRICK_SKIP_FLAGS_PER_CHUNK,0xFF,sizeof(u64)*(DOUBLEBRICK_FLAGS_PER_CHUNK));

	pile->chunks[pile->chunkCount]=chunk;

	__atomic_fetch_add(&pile->chunkCount, 1, __ATOMIC_SEQ_CST);
	__atomic_fetch_add(&pile->freeCount, DOUBLEBRICK_BRICKS_PER_CHUNK, __ATOMIC_SEQ_CST);

}


void mbInitDoubleBrickPile(MemDoubleBrickPile *pile, s32 chunkCount, s32 chunkLimit, s32 memTrackId)
{
	if(sizeof(MemDoubleBrickChunk)!=DOUBLEBRICK_ALIGNMENT)
		LOG(LOG_CRITICAL,"Unexpected MemDoubleBrickChunk Size/Alignment mismatch: %i vs %i", sizeof(MemDoubleBrickChunk), DOUBLEBRICK_ALIGNMENT);

	pile->chunks=G_ALLOC(sizeof(MemDoubleBrickChunk *)*chunkLimit, memTrackId);
	memset(pile->chunks, 0, sizeof(MemDoubleBrickChunk *)*chunkLimit);

	pile->memTrackId=memTrackId;
	pile->chunkLimit=chunkLimit;
	pile->chunkCount=0;
	pile->freeCount=0;

	pile->scanChunk=0;

	for(int i=0;i<chunkCount;i++)
		initDoubleBrickChunk(pile);
}


void mbFreeDoubleBrickPile(MemDoubleBrickPile *pile)
{
	for(int i=0;i<pile->chunkCount;i++)
		G_FREE(pile->chunks[i], sizeof(MemDoubleBrickChunk), pile->memTrackId);

	G_FREE(pile->chunks, sizeof(MemDoubleBrickChunk *)*pile->chunkLimit, pile->memTrackId);
}


s32 mbGetFreeSingleBrickPile(MemSingleBrickPile *pile)
{
	return __atomic_load_n(&(pile->freeCount),__ATOMIC_RELAXED);
}

s32 mbGetFreeSingleBrickChunk(MemSingleBrickChunk *chunk)
{
	s32 free=__atomic_load_n(&(chunk->header.freeCount),__ATOMIC_RELAXED);
	//LOG(LOG_INFO,"Chunk Free %i",free);
	return free;
}

s32 mbCheckSingleBrickAvailability(MemSingleBrickPile *pile, s32 brickCount)
{
	s32 availableCount=mbGetFreeSingleBrickPile(pile);
	int needed=brickCount+pile->chunkCount*SINGLEBRICK_PILE_MARGIN_PER_CHUNK;

	return needed<availableCount;
}

s32 mbGetSingleBrickPileCapacity(MemSingleBrickPile *pile)
{
	return pile->chunkCount*(SINGLEBRICK_FLAGS_PER_CHUNK*64);
}



s32 mbGetFreeDoubleBrickPile(MemDoubleBrickPile *pile)
{
	return __atomic_load_n(&(pile->freeCount),__ATOMIC_RELAXED);
}

s32 mbGetFreeDoubleBrickChunk(MemDoubleBrickChunk *chunk)
{
	s32 free=__atomic_load_n(&(chunk->header.freeCount),__ATOMIC_RELAXED);
	//LOG(LOG_INFO,"Chunk Free %i",free);
	return free;
}


s32 mbCheckDoubleBrickAvailability(MemDoubleBrickPile *pile, s32 brickCount)
{
	s32 availableCount=mbGetFreeDoubleBrickPile(pile);
	int needed=brickCount+pile->chunkCount*DOUBLEBRICK_PILE_MARGIN_PER_CHUNK;

	return needed<availableCount;
}

s32 mbGetDoubleBrickPileCapacity(MemDoubleBrickPile *pile)
{
	return pile->chunkCount*(DOUBLEBRICK_FLAGS_PER_CHUNK*64);
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

static s32 singleBrickPileUnreserve(MemSingleBrickPile *pile, s32 resRequest)
{
	__atomic_fetch_add(&(pile->freeCount), resRequest, __ATOMIC_RELAXED);

	return 0;
}


static s32 doubleBrickAttemptPileReserve(MemDoubleBrickPile *pile, s32 resRequest)
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

static s32 doubleBrickPileUnreserve(MemDoubleBrickPile *pile, s32 resRequest)
{
	__atomic_fetch_add(&(pile->freeCount), resRequest, __ATOMIC_RELAXED);

	return 0;
}







static s32 singleBrickPileScanForUsableChunk(MemSingleBrickPile *pile, s32 startChunk)
{
	for(int i=startChunk;i<pile->chunkCount;i++)
		{
		if(mbGetFreeSingleBrickChunk(pile->chunks[i])>SINGLEBRICK_CHUNK_MARGIN_PER_CHUNK && __atomic_load_n(&(pile->chunks[i]->header.flagPosition),__ATOMIC_RELAXED)>=0)
			return i;
		}

	for(int i=0;i<startChunk;i++)
		{
		if(mbGetFreeSingleBrickChunk(pile->chunks[i])>SINGLEBRICK_CHUNK_MARGIN_PER_CHUNK && __atomic_load_n(&(pile->chunks[i]->header.flagPosition),__ATOMIC_RELAXED)>=0)
			return i;
		}

	return -1;
}

static s32 doubleBrickPileScanForUsableChunk(MemDoubleBrickPile *pile, s32 startChunk)
{
	for(int i=startChunk;i<pile->chunkCount;i++)
		{
		if(mbGetFreeDoubleBrickChunk(pile->chunks[i])>DOUBLEBRICK_CHUNK_MARGIN_PER_CHUNK && __atomic_load_n(&(pile->chunks[i]->header.flagPosition),__ATOMIC_RELAXED)>=0)
			return i;
		}

	for(int i=0;i<startChunk;i++)
		{
		if(mbGetFreeDoubleBrickChunk(pile->chunks[i])>DOUBLEBRICK_CHUNK_MARGIN_PER_CHUNK && __atomic_load_n(&(pile->chunks[i]->header.flagPosition),__ATOMIC_RELAXED)>=0)
			return i;
		}

	return -1;
}




static s32 singleBrickLockChunkForAllocation(MemSingleBrickChunk *chunk)
{
	s32 flagPosition=__atomic_load_n(&(chunk->header.flagPosition), __ATOMIC_RELAXED);

	if(flagPosition==SINGLEBRICK_SCANPOSITION_LOCKED)
		return flagPosition;

	s32 newScanPosition=SINGLEBRICK_SCANPOSITION_LOCKED;

	if(__atomic_compare_exchange_n(&(chunk->header.flagPosition), &flagPosition, newScanPosition, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		return flagPosition;

	return SINGLEBRICK_SCANPOSITION_LOCKED;
}


static s32 singleBrickUnlockChunk(MemSingleBrickChunk *chunk, u16 flagPosition)
{
	s32 currentFlagPosition=__atomic_load_n(&(chunk->header.flagPosition), __ATOMIC_RELAXED);

	if(currentFlagPosition!=SINGLEBRICK_SCANPOSITION_LOCKED)
		{
		LOG(LOG_CRITICAL,"Cannot unlock chunk not currently locked");
		return SINGLEBRICK_SCANPOSITION_LOCKED;
		}

	if(__atomic_compare_exchange_n(&(chunk->header.flagPosition), &currentFlagPosition, flagPosition, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		return flagPosition;

	LOG(LOG_CRITICAL,"Failed to unlock chunk");
	return SINGLEBRICK_SCANPOSITION_LOCKED;
}


static s32 doubleBrickLockChunkForAllocation(MemDoubleBrickChunk *chunk)
{
	s32 flagPosition=__atomic_load_n(&(chunk->header.flagPosition), __ATOMIC_RELAXED);

	if(flagPosition==DOUBLEBRICK_SCANPOSITION_LOCKED)
		return flagPosition;

	s32 newScanPosition=DOUBLEBRICK_SCANPOSITION_LOCKED;

	if(__atomic_compare_exchange_n(&(chunk->header.flagPosition), &flagPosition, newScanPosition, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		return flagPosition;

	return DOUBLEBRICK_SCANPOSITION_LOCKED;
}


static s32 doubleBrickUnlockChunk(MemDoubleBrickChunk *chunk, u16 flagPosition)
{
	s32 currentFlagPosition=__atomic_load_n(&(chunk->header.flagPosition), __ATOMIC_RELAXED);

	if(currentFlagPosition!=DOUBLEBRICK_SCANPOSITION_LOCKED)
		{
		LOG(LOG_CRITICAL,"Cannot unlock chunk not currently locked");
		return DOUBLEBRICK_SCANPOSITION_LOCKED;
		}

	if(__atomic_compare_exchange_n(&(chunk->header.flagPosition), &currentFlagPosition, flagPosition, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		return flagPosition;

	LOG(LOG_CRITICAL,"Failed to unlock chunk");
	return DOUBLEBRICK_SCANPOSITION_LOCKED;
}








static s32 singleBrickPileNextChunk(MemSingleBrickPile *pile, s32 chunk)
{
	chunk++;
	if(chunk>=pile->chunkCount)
		return 0;
	return chunk;
}

static s32 singleBrickAllocatorScanForChunkAndLock(MemSingleBrickAllocator *alloc)
{
	MemSingleBrickPile *pile=alloc->pile;
	s32 startChunk=pile->scanChunk;

	for(int loopCount=0;loopCount<100000;loopCount++)
		{
		s32 scanChunkResult=singleBrickPileScanForUsableChunk(pile, startChunk);
		if(scanChunkResult>=0)
			{
			int flagPosition=singleBrickLockChunkForAllocation(pile->chunks[scanChunkResult]);

			if(flagPosition!=SINGLEBRICK_SCANPOSITION_LOCKED)
				{
				alloc->chunk=pile->chunks[scanChunkResult];
				alloc->chunkIndex=(((u32)scanChunkResult)<<16);
				alloc->scanChunk=scanChunkResult;

				pile->scanChunk=singleBrickPileNextChunk(pile, scanChunkResult);

				return flagPosition;
				}
			}

		startChunk=singleBrickPileNextChunk(pile,startChunk);
		}

	LOG(LOG_CRITICAL,"Loop Stuck");
	return 0;
}

static u32 singleBrickPileNextFlag(u32 flag)
{
	flag++;
	if(flag>=SINGLEBRICK_ALLOC_FLAGS_PER_CHUNK)
		return SINGLEBRICK_SKIP_FLAGS_PER_CHUNK;
	return flag;
}



static s32 doubleBrickPileNextChunk(MemDoubleBrickPile *pile, s32 chunk)
{
	chunk++;
	if(chunk>=pile->chunkCount)
		return 0;
	return chunk;
}


static s32 doubleBrickAllocatorScanForChunkAndLock(MemDoubleBrickAllocator *alloc)
{
	MemDoubleBrickPile *pile=alloc->pile;
	s32 startChunk=pile->scanChunk;

	for(int loopCount=0;loopCount<100000;loopCount++)
		{
		s32 scanChunkResult=doubleBrickPileScanForUsableChunk(pile, startChunk);
		if(scanChunkResult>=0)
			{
			int flagPosition=doubleBrickLockChunkForAllocation(pile->chunks[scanChunkResult]);

			if(flagPosition!=DOUBLEBRICK_SCANPOSITION_LOCKED)
				{
				alloc->chunk=pile->chunks[scanChunkResult];
				alloc->chunkIndex=(((u32)scanChunkResult)<<16);
				alloc->scanChunk=scanChunkResult;

				pile->scanChunk=doubleBrickPileNextChunk(pile, scanChunkResult);

				return flagPosition;
				}
			}

		startChunk=doubleBrickPileNextChunk(pile,startChunk);
		}

	LOG(LOG_CRITICAL,"Loop Stuck");
	return 0;
}

static u32 doubleBrickPileNextFlag(u32 flag)
{
	flag++;
	if(flag>=DOUBLEBRICK_ALLOC_FLAGS_PER_CHUNK)
		return DOUBLEBRICK_SKIP_FLAGS_PER_CHUNK;
	return flag;
}




static s32 singleBlockAllocateGroup(MemSingleBrickAllocator *alloc)
{
	MemSingleBrickChunk *chunk=alloc->chunk;

	if(mbGetFreeSingleBrickChunk(alloc->chunk)<SINGLEBRICK_CHUNK_MARGIN_PER_CHUNK)
		return 0;

	for(int i=alloc->scanFlag; i<SINGLEBRICK_ALLOC_FLAGS_PER_CHUNK; i++)
		{
		u64 allocMask=__atomic_load_n(chunk->freeFlag+i, __ATOMIC_RELAXED);

		if(allocMask!=0)
			{
			if(__atomic_compare_exchange_n(chunk->freeFlag+i, &allocMask, 0, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
				{
				alloc->scanFlag=i;
				alloc->allocMask=allocMask;

				s32 allocCount=__builtin_popcountl(allocMask);
				alloc->allocCount=allocCount;
				alloc->allocIndex=alloc->scanFlag<<6;

				__atomic_fetch_sub(&(chunk->header.freeCount), allocCount, __ATOMIC_RELAXED);

				return allocCount;
				}
			}
		}

	for(int i=SINGLEBRICK_SKIP_FLAGS_PER_CHUNK; i<alloc->scanFlag; i++)
		{
		u64 allocMask=__atomic_load_n(chunk->freeFlag+i, __ATOMIC_RELAXED);

		if(allocMask!=0)
			{
			if(__atomic_compare_exchange_n(chunk->freeFlag+i, &allocMask, 0, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
				{
				alloc->scanFlag=i;
				alloc->allocMask=allocMask;

				s32 allocCount=__builtin_popcountl(allocMask);
				alloc->allocCount=allocCount;
				alloc->allocIndex=alloc->scanFlag<<6;

				__atomic_fetch_sub(&(chunk->header.freeCount), allocCount, __ATOMIC_RELAXED);

				return allocCount;
				}

			}
		}

	return 0;
}

static s32 singleBlockUnallocateGroup(MemSingleBrickAllocator *alloc)
{
	if(!alloc->allocCount)
		return 0;

	MemSingleBrickChunk *chunk=alloc->chunk;
	u32 scanFlag=alloc->scanFlag;

	int loopCount=10000;

	do
		{
		u64 allocMask=__atomic_load_n(chunk->freeFlag+scanFlag, __ATOMIC_SEQ_CST);
		u64 newAllocMask=allocMask|alloc->allocMask;

		if(__atomic_compare_exchange_n(chunk->freeFlag+scanFlag, &allocMask, newAllocMask, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
			{
			int tmp=alloc->allocCount;
			alloc->allocCount=0;

			__atomic_fetch_add(&(chunk->header.freeCount), tmp, __ATOMIC_RELAXED);

			return tmp;
			}

		}
	while(--loopCount>0);

	LOG(LOG_CRITICAL,"Failed to unallocate group");
	return 0;

}


static s32 singleBlockUnallocateEntry(MemSingleBrickChunk *chunk, u32 scanFlag, u32 allocIndex)
{
	int loopCount=10000;

	u64 freeAllocMask=(1L<<allocIndex);

	do
		{
		u64 allocMask=__atomic_load_n(chunk->freeFlag+scanFlag, __ATOMIC_SEQ_CST);

		if(allocMask & freeAllocMask)
			{
			LOG(LOG_INFO,"Entry is already free %i %i (%i) - %016lx %016lx", scanFlag, allocIndex, loopCount, allocMask, freeAllocMask);
			return 0;
			}

		u64 newAllocMask=allocMask|freeAllocMask;

		if(__atomic_compare_exchange_n(chunk->freeFlag+scanFlag, &allocMask, newAllocMask, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
			{
			__atomic_fetch_add(&(chunk->header.freeCount), 1, __ATOMIC_SEQ_CST);

			return 1;
			}

		}
	while(--loopCount>0);

	LOG(LOG_CRITICAL,"Failed to unallocate entry");
	return 0;

}





static s32 doubleBlockAllocateGroup(MemDoubleBrickAllocator *alloc)
{
	MemDoubleBrickChunk *chunk=alloc->chunk;

	if(mbGetFreeDoubleBrickChunk(alloc->chunk)<DOUBLEBRICK_CHUNK_MARGIN_PER_CHUNK)
		return 0;

	for(int i=alloc->scanFlag; i<DOUBLEBRICK_ALLOC_FLAGS_PER_CHUNK; i++)
		{
		u64 allocMask=__atomic_load_n(chunk->freeFlag+i, __ATOMIC_RELAXED);

		if(allocMask!=0)
			{
			if(__atomic_compare_exchange_n(chunk->freeFlag+i, &allocMask, 0, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
				{
				alloc->scanFlag=i;
				alloc->allocMask=allocMask;

				s32 allocCount=__builtin_popcountl(allocMask);
				alloc->allocCount=allocCount;
				alloc->allocIndex=alloc->scanFlag<<6;

				__atomic_fetch_sub(&(chunk->header.freeCount), allocCount, __ATOMIC_RELAXED);

				return allocCount;
				}
			}
		}

	for(int i=DOUBLEBRICK_SKIP_FLAGS_PER_CHUNK; i<alloc->scanFlag; i++)
		{
		u64 allocMask=__atomic_load_n(chunk->freeFlag+i, __ATOMIC_RELAXED);

		if(allocMask!=0)
			{
			if(__atomic_compare_exchange_n(chunk->freeFlag+i, &allocMask, 0, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
				{
				alloc->scanFlag=i;
				alloc->allocMask=allocMask;

				s32 allocCount=__builtin_popcountl(allocMask);
				alloc->allocCount=allocCount;
				alloc->allocIndex=alloc->scanFlag<<6;

				__atomic_fetch_sub(&(chunk->header.freeCount), allocCount, __ATOMIC_RELAXED);

				return allocCount;
				}

			}
		}

	return 0;
}

static s32 doubleBlockUnallocateGroup(MemDoubleBrickAllocator *alloc)
{
	if(!alloc->allocCount)
		return 0;

	MemDoubleBrickChunk *chunk=alloc->chunk;
	u32 scanFlag=alloc->scanFlag;

	int loopCount=10000;

	do
		{
		u64 allocMask=__atomic_load_n(chunk->freeFlag+scanFlag, __ATOMIC_SEQ_CST);
		u64 newAllocMask=allocMask|alloc->allocMask;

		if(__atomic_compare_exchange_n(chunk->freeFlag+scanFlag, &allocMask, newAllocMask, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
			{
			int tmp=alloc->allocCount;
			alloc->allocCount=0;

			__atomic_fetch_add(&(chunk->header.freeCount), tmp, __ATOMIC_RELAXED);

			return tmp;
			}

		}
	while(--loopCount>0);

	LOG(LOG_CRITICAL,"Failed to unallocate group");
	return 0;

}


static s32 doubleBlockUnallocateEntry(MemDoubleBrickChunk *chunk, u32 scanFlag, u32 allocIndex)
{
	int loopCount=10000;

	u64 freeAllocMask=(1L<<allocIndex);

	do
		{
		u64 allocMask=__atomic_load_n(chunk->freeFlag+scanFlag, __ATOMIC_SEQ_CST);

		if(allocMask & freeAllocMask)
			{
			LOG(LOG_CRITICAL,"Entry is already free (%i)", loopCount);
			return 0;
			}

		u64 newAllocMask=allocMask|freeAllocMask;

		if(__atomic_compare_exchange_n(chunk->freeFlag+scanFlag, &allocMask, newAllocMask, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
			{
			__atomic_fetch_add(&(chunk->header.freeCount), 1, __ATOMIC_SEQ_CST);

			return 1;
			}

		}
	while(--loopCount>0);

	LOG(LOG_CRITICAL,"Failed to unallocate entry");
	return 0;

}







s32 mbInitSingleBrickAllocator(MemSingleBrickAllocator *alloc, MemSingleBrickPile *pile, s32 pileResRequest)
{
	alloc->pile=pile;
	alloc->pileResRequest=pileResRequest+SINGLEBRICK_DEFAULT_RESERVATION;

	if(!singleBrickAttemptPileReserve(pile, alloc->pileResRequest))
		{
		LOG(LOG_INFO,"Pile Reserve Failed");
		return 0;
		}

	alloc->pileResLeft=alloc->pileResRequest;

	int scanFlag=singleBrickAllocatorScanForChunkAndLock(alloc);

	if(scanFlag<0)
		{
		LOG(LOG_INFO,"Chunk Scan/Lock Failed");
		return 0;
		}

	alloc->scanFlag=scanFlag;

	s32 allocCount=singleBlockAllocateGroup(alloc);
//	LOG(LOG_INFO,"Pre-Allocated %i", allocCount);
	return allocCount;
}

void *mbSingleBrickAllocate(MemSingleBrickAllocator *alloc, u32 *brickIndexPtr)
{
	if(alloc->allocCount==0)
		{
		alloc->scanFlag=singleBrickPileNextFlag(alloc->scanFlag);

		if(alloc->pileResLeft < SINGLEBRICK_DEFAULT_RESERVATION)
			{
			int additionalReserve=SINGLEBRICK_DEFAULT_RESERVATION-alloc->pileResLeft;
			LOG(LOG_CRITICAL,"Pile Reserve extension %i",additionalReserve);

			if(!singleBrickAttemptPileReserve(alloc->pile, additionalReserve))
				{
				LOG(LOG_INFO,"Pile Reserve Failed");
				return NULL;
				}

			alloc->pileResLeft=DOUBLEBRICK_DEFAULT_RESERVATION;
			}

		if(!singleBlockAllocateGroup(alloc))
			{
			singleBrickUnlockChunk(alloc->chunk, singleBrickPileNextFlag(alloc->scanFlag));

			int scanFlag=singleBrickAllocatorScanForChunkAndLock(alloc);

			if(scanFlag<0)
				{
				LOG(LOG_INFO,"Chunk Scan/Lock Failed");
				return NULL;
				}

			alloc->scanFlag=scanFlag;

			if(!singleBlockAllocateGroup(alloc))
				LOG(LOG_CRITICAL,"Failed to alloc using new chunk");
			}


		}

	if(alloc->allocMask==0)
		{
		LOG(LOG_CRITICAL,"Positive count but no bits");
		return NULL;
		}

	s32 index=alloc->allocIndex+__builtin_ctzl(alloc->allocMask);
	alloc->allocMask&=alloc->allocMask-1; // Clear lowest bit
	alloc->allocCount--;

	alloc->pileResLeft--;

	if(brickIndexPtr!=NULL)
		*brickIndexPtr=alloc->chunkIndex+index;

	return alloc->chunk->bricks[index].data;
}

void mbSingleBrickAllocatorCleanup(MemSingleBrickAllocator *alloc)
{
	//LOG(LOG_INFO,"Should return %i",alloc->allocCount);

	singleBlockUnallocateGroup(alloc);
	singleBrickUnlockChunk(alloc->chunk, alloc->scanFlag);
	singleBrickPileUnreserve(alloc->pile, alloc->pileResLeft);
}

void *mbSingleBrickFindByIndex(MemSingleBrickPile *pile, u32 brickIndex)
{
	if(brickIndex==LINK_INDEX_DUMMY)
		{
		LOG(LOG_CRITICAL,"Invalid Index %i",brickIndex);
		}

	u32 chunkIndex=brickIndex>>16;
	u32 index=(brickIndex &0xFFFF);

	if(chunkIndex>pile->chunkCount)
		LOG(LOG_CRITICAL,"Index beyond end of pile");

	return pile->chunks[chunkIndex]->bricks+index;
}


void mbSingleBrickFreeByIndex(MemSingleBrickPile *pile, u32 brickIndex)
{
	if(brickIndex==LINK_INDEX_DUMMY)
		{
		LOG(LOG_CRITICAL,"Invalid Index %i",brickIndex);
		}

	u32 chunkIndex=brickIndex>>16;
	u32 flagIndex=(brickIndex &0xFFFF)>>6;
	u32 allocIndex=(brickIndex & 0x3F);

	if(chunkIndex>pile->chunkCount)
		LOG(LOG_CRITICAL,"Index beyond end of pile");

	if(!singleBlockUnallocateEntry(pile->chunks[chunkIndex], flagIndex, allocIndex))
		LOG(LOG_CRITICAL,"Failed to free %i -> %i %i %i", brickIndex, chunkIndex, flagIndex, allocIndex);

	__atomic_fetch_add(&pile->freeCount, 1, __ATOMIC_RELAXED);
}







s32 mbInitDoubleBrickAllocator(MemDoubleBrickAllocator *alloc, MemDoubleBrickPile *pile, s32 pileResRequest)
{
	alloc->pile=pile;
	alloc->pileResRequest=pileResRequest+DOUBLEBRICK_DEFAULT_RESERVATION;

	if(!doubleBrickAttemptPileReserve(pile, alloc->pileResRequest))
		{
		LOG(LOG_INFO,"Pile Reserve Failed");
		return 0;
		}

	alloc->pileResLeft=alloc->pileResRequest;

	int scanFlag=doubleBrickAllocatorScanForChunkAndLock(alloc);

	if(scanFlag<0)
		{
		LOG(LOG_INFO,"Chunk Scan/Lock Failed");
		return 0;
		}

	alloc->scanFlag=scanFlag;

	s32 allocCount=doubleBlockAllocateGroup(alloc);
//	LOG(LOG_INFO,"Pre-Allocated %i", allocCount);
	return allocCount;
}

void *mbDoubleBrickAllocate(MemDoubleBrickAllocator *alloc, u32 *brickIndexPtr)
{
	if(alloc->allocCount==0)
		{
		alloc->scanFlag=doubleBrickPileNextFlag(alloc->scanFlag);

		if(alloc->pileResLeft < DOUBLEBRICK_DEFAULT_RESERVATION)
			{
			int additionalReserve=DOUBLEBRICK_DEFAULT_RESERVATION-alloc->pileResLeft;
			LOG(LOG_CRITICAL,"Pile Reserve extension %i",additionalReserve);

			if(!doubleBrickAttemptPileReserve(alloc->pile, additionalReserve))
				{
				LOG(LOG_INFO,"Pile Reserve Failed");
				return NULL;
				}

			alloc->pileResLeft=DOUBLEBRICK_DEFAULT_RESERVATION;
			}

		if(!doubleBlockAllocateGroup(alloc))
			{
			doubleBrickUnlockChunk(alloc->chunk, doubleBrickPileNextFlag(alloc->scanFlag));

			int scanFlag=doubleBrickAllocatorScanForChunkAndLock(alloc);

			if(scanFlag<0)
				{
				LOG(LOG_INFO,"Chunk Scan/Lock Failed");
				return NULL;
				}

			alloc->scanFlag=scanFlag;

			if(!doubleBlockAllocateGroup(alloc))
				LOG(LOG_CRITICAL,"Failed to alloc using new chunk");
			}


		}

	if(alloc->allocMask==0)
		{
		LOG(LOG_CRITICAL,"Positive count but no bits");
		return NULL;
		}

	s32 index=alloc->allocIndex+__builtin_ctzl(alloc->allocMask);
	alloc->allocMask&=alloc->allocMask-1; // Clear lowest bit
	alloc->allocCount--;

	alloc->pileResLeft--;

	if(brickIndexPtr!=NULL)
		*brickIndexPtr=alloc->chunkIndex+index;

	return alloc->chunk->bricks[index].data;
}

void mbDoubleBrickAllocatorCleanup(MemDoubleBrickAllocator *alloc)
{
	//LOG(LOG_INFO,"Should return %i",alloc->allocCount);

	doubleBlockUnallocateGroup(alloc);
	doubleBrickUnlockChunk(alloc->chunk, alloc->scanFlag);
	doubleBrickPileUnreserve(alloc->pile, alloc->pileResLeft);
}

void *mbDoubleBrickFindByIndex(MemDoubleBrickPile *pile, u32 brickIndex)
{
	if(brickIndex==LINK_INDEX_DUMMY)
		{
		LOG(LOG_CRITICAL,"Invalid Index %i",brickIndex);
		}

	u32 chunkIndex=brickIndex>>16;
	u32 index=(brickIndex &0xFFFF);

	if(chunkIndex>pile->chunkCount)
		LOG(LOG_CRITICAL,"Index beyond end of pile");

	return pile->chunks[chunkIndex]->bricks+index;
}

void mbDoubleBrickFreeByIndex(MemDoubleBrickPile *pile, u32 brickIndex)
{
	u32 chunkIndex=brickIndex>>16;
	u32 flagIndex=(brickIndex &0xFFFF)>>6;
	u32 allocIndex=(brickIndex & 0x3F);

	if(chunkIndex>pile->chunkCount)
		LOG(LOG_CRITICAL,"Index beyond end of pile");

	doubleBlockUnallocateEntry(pile->chunks[chunkIndex], flagIndex, allocIndex);

	__atomic_fetch_add(&pile->freeCount, 1, __ATOMIC_RELAXED);
}



