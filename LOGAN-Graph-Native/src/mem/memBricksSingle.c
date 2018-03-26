#include "common.h"



// Called during init() (eventually also while holding pile->lock) but still needs to be thread-safe against regular uses

static void initSingleBrickChunk(MemSingleBrickPile *pile)
{
	s32 chunkCount=__atomic_load_n(&pile->chunkCount, __ATOMIC_SEQ_CST);

	if(chunkCount>=pile->chunkLimit)
		LOG(LOG_CRITICAL,"Cannot init another chunk: Pile at limit");

	MemSingleBrickChunk *chunk=G_ALLOC_ALIGNED(sizeof(MemSingleBrickChunk), SINGLEBRICK_ALIGNMENT, pile->memTrackId);

	chunk->header.freeCount=SINGLEBRICK_BRICKS_PER_CHUNK;
	chunk->header.flagPosition=SINGLEBRICK_SKIP_FLAGS_PER_CHUNK;

	memset(chunk->freeFlag+SINGLEBRICK_SKIP_FLAGS_PER_CHUNK,0xFF,sizeof(u64)*(SINGLEBRICK_FLAGS_PER_CHUNK));

	pile->chunks[chunkCount]=chunk;

	__atomic_thread_fence(__ATOMIC_SEQ_CST);

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
	pile->lock=0;

	pile->freeCount=0;
	pile->scanChunk=0;

	for(int i=0;i<chunkCount;i++)
		initSingleBrickChunk(pile);
}

void mbFreeSingleBrickPile(MemSingleBrickPile *pile, char *pileName)
{
	for(int i=0;i<pile->chunkCount;i++)
		{
		int alloc=SINGLEBRICK_BRICKS_PER_CHUNK-pile->chunks[i]->header.freeCount;

		if(alloc>0)
			{
			LOG(LOG_INFO,"SingleBrickPile %s Chunk %i has %i still allocated",pileName, i, alloc);

			u32 chunkBaseIndex=i<<16;

			for(int j=SINGLEBRICK_SKIP_FLAGS_PER_CHUNK;j<SINGLEBRICK_ALLOC_FLAGS_PER_CHUNK;j++)
				{
				u64 flags=pile->chunks[i]->freeFlag[j];
				if(flags!=0xFFFFFFFFFFFFFFFFl)
					{
					u32 flagBaseIndex=chunkBaseIndex+(j<<6);

					for(int k=0;k<64;k++)
						{
						if(!(flags & 0x1))
							LOG(LOG_INFO,"SingleBrick %s %u", pileName, (flagBaseIndex+k));
						flags>>=1;
						}
					}
				}
			}

		G_FREE(pile->chunks[i], sizeof(MemSingleBrickChunk), pile->memTrackId);
		}

	G_FREE(pile->chunks, sizeof(MemSingleBrickChunk *)*pile->chunkLimit, pile->memTrackId);
}



s32 mbGetFreeSingleBrickPile(MemSingleBrickPile *pile)
{
	return __atomic_load_n(&(pile->freeCount),__ATOMIC_SEQ_CST);
}

s32 mbGetFreeSingleBrickChunk(MemSingleBrickChunk *chunk)
{
	s32 free=__atomic_load_n(&(chunk->header.freeCount),__ATOMIC_SEQ_CST);
	return free;
}

s32 mbCheckSingleBrickAvailability(MemSingleBrickPile *pile, s32 brickCount)
{
	s32 availableCount=mbGetFreeSingleBrickPile(pile);
	s32 chunkCount=__atomic_load_n(&pile->chunkCount, __ATOMIC_SEQ_CST);

	s32 expandCount=(pile->chunkLimit-chunkCount)*(SINGLEBRICK_BRICKS_PER_CHUNK-SINGLEBRICK_PILE_MARGIN_PER_CHUNK);
	int needed=brickCount+chunkCount*SINGLEBRICK_PILE_MARGIN_PER_CHUNK;

	return needed<(availableCount+expandCount);
}

s32 mbGetSingleBrickPileCapacity(MemSingleBrickPile *pile)
{
	return pile->chunkCount*SINGLEBRICK_BRICKS_PER_CHUNK;
}





static s32 singleBrickLockPile(MemSingleBrickPile *pile)
{
	for(int loopCount=0;loopCount<100000;loopCount++)
		{
		s32 lock=__atomic_load_n(&(pile->lock), __ATOMIC_SEQ_CST);

		if(lock)
			return 0;

		if(__atomic_compare_exchange_n(&(pile->lock), &lock, 1, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
				return 1;
		}

	LOG(LOG_CRITICAL,"Failed to lock pile, should never happen"); // Can actually happen, just stupidly unlikely
	return 0;
}

static s32 singleBrickUnlockPile(MemSingleBrickPile *pile)
{
	for(int loopCount=0;loopCount<100000;loopCount++)
		{
		s32 lock=__atomic_load_n(&(pile->lock), __ATOMIC_SEQ_CST);

		if(!lock)
			return 0;

		if(__atomic_compare_exchange_n(&(pile->lock), &lock, 0, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
			return 1;
		}

	LOG(LOG_CRITICAL,"Failed to lock pile, should never happen"); // Can actually happen, just stupidly unlikely
	return 0;
}



static s32 singleBrickAttemptPileReserve(MemSingleBrickPile *pile, s32 resRequest)
{
	for(int loopCount=0;loopCount<100000;loopCount++)
		{
		s32 freeCount=__atomic_load_n(&(pile->freeCount), __ATOMIC_RELAXED);
		s32 pileMargin=pile->chunkCount*SINGLEBRICK_PILE_MARGIN_PER_CHUNK;

		if(freeCount<(resRequest+pileMargin))
			return 0;

		s32 newFreeCount=freeCount-resRequest;

		if(__atomic_compare_exchange_n(&(pile->freeCount), &freeCount, newFreeCount, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
			return 1;
		}

	LOG(LOG_CRITICAL,"Failed to reserve Bricks, should never happen"); // Can actually happen, just stupidly unlikely
	return 0;
}


static s32 singleBrickAttemptPileReserveOrExpand(MemSingleBrickPile *pile, s32 resRequest)
{
	int ret=singleBrickAttemptPileReserve(pile, resRequest);

	if(ret)
		return ret;

	s32 chunkCount=__atomic_load_n(&pile->chunkCount, __ATOMIC_SEQ_CST);

	if(chunkCount == pile->chunkLimit)
		return 0;

	for(int loopCount=0;loopCount<1000;loopCount++)
		{
		if(singleBrickLockPile(pile))
			{
			s32 freeCount=__atomic_load_n(&(pile->freeCount), __ATOMIC_RELAXED);
			s32 pileMargin=pile->chunkCount*SINGLEBRICK_PILE_MARGIN_PER_CHUNK;

			chunkCount=__atomic_load_n(&pile->chunkCount, __ATOMIC_SEQ_CST);

			while(freeCount < (resRequest+pileMargin) && chunkCount < pile->chunkLimit)
				{
//				LOG(LOG_INFO,"Expand SingleBrick pile");
				initSingleBrickChunk(pile);
				freeCount=__atomic_load_n(&(pile->freeCount), __ATOMIC_RELAXED);
				}

			singleBrickUnlockPile(pile);
			}

		ret=singleBrickAttemptPileReserve(pile, resRequest);

		if(ret)
			return ret;

		chunkCount=__atomic_load_n(&pile->chunkCount, __ATOMIC_SEQ_CST);
		if(chunkCount == pile->chunkLimit)
			return 0;

		struct timespec req;

		req.tv_sec=DEFAULT_SLEEP_SECS;
		req.tv_nsec=DEFAULT_SLEEP_NANOS;

		nanosleep(&req, NULL);
		}

	return 0;
}



static s32 singleBrickPileUnreserve(MemSingleBrickPile *pile, s32 resRequest)
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







s32 mbInitSingleBrickAllocator(MemSingleBrickAllocator *alloc, MemSingleBrickPile *pile, s32 pileResRequest)
{
	alloc->pile=pile;
	alloc->pileResRequest=pileResRequest+SINGLEBRICK_DEFAULT_RESERVATION;

	if(!singleBrickAttemptPileReserveOrExpand(pile, alloc->pileResRequest))
		return 0;

	alloc->pileResLeft=alloc->pileResRequest;

	s32 allocCount=0,loopCount=0;

	while(!allocCount && loopCount++<1000)
		{
		int scanFlag=singleBrickAllocatorScanForChunkAndLock(alloc);

		if(scanFlag<0)
			{
			LOG(LOG_INFO,"Chunk Scan/Lock Failed");
			return 0;
			}

		alloc->scanFlag=scanFlag;

		allocCount=singleBlockAllocateGroup(alloc);
		}

	if(loopCount>=1000)
		LOG(LOG_CRITICAL,"Loop stuck");

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

			if(!singleBrickAttemptPileReserveOrExpand(alloc->pile, additionalReserve))
				{
				LOG(LOG_INFO,"Pile Reserve Failed");
				return NULL;
				}

			alloc->pileResLeft=DOUBLEBRICK_DEFAULT_RESERVATION;
			}

		s32 allocCount=singleBlockAllocateGroup(alloc);
		s32 loopCount=0;

		if(allocCount==0)
			{
			singleBrickUnlockChunk(alloc->chunk, singleBrickPileNextFlag(alloc->scanFlag));

			while(allocCount==0 && loopCount++<1000)
				{
				int scanFlag=singleBrickAllocatorScanForChunkAndLock(alloc);

				if(scanFlag<0)
					{
					LOG(LOG_INFO,"Chunk Scan/Lock Failed");
					return NULL;
					}

				alloc->scanFlag=scanFlag;

				allocCount=singleBlockAllocateGroup(alloc);
				}
			}

		if(loopCount>=1000)
			LOG(LOG_CRITICAL,"Loop stuck");
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

	//__atomic_fetch_add(&pile->freeCount, 1, __ATOMIC_RELAXED);
	__atomic_fetch_add(&pile->freeCount, 1, __ATOMIC_SEQ_CST);
}






