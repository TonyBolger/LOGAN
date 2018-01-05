/*
 * taskRouting.c
 *
 *  Created on: Oct 28, 2015
 *      Author: tony
 */

#include "common.h"

static char *decodeLinkIndexType(u8 indexType)
{
	switch(indexType)
		{
		case LINK_INDEXTYPE_SEQ:
			return "SEQ";
		case LINK_INDEXTYPE_LOOKUP:
			return "LOOKUP";
		case LINK_INDEXTYPE_DISPATCH:
			return "DISPATCH";
		default:
			LOG(LOG_INFO,"Unknown link index type %u",(u32)(indexType));
			return "UNKNOWN";
		}

}

void trDumpSequenceLink(SequenceLink *link, u32 linkIndex)
{
//	u32 nextIndex;			// Index of next SequenceLink in chain (or LINK_DUMMY at end)
//	u8 length;				// Length of packed sequence (in bp)
//	u8 position;			// Position of first unprocessed base (in bp)
//	u8 packedSequence[SEQUENCE_LINK_BYTES];

	LOG(LOG_INFO,"SequenceLink Dump: Idx %i (%p)",linkIndex, link);

	LOG(LOG_INFO,"  NextIdx: %u", link->nextIndex);
	LOG(LOG_INFO,"  Len: %u",(u32)(link->length));
	LOG(LOG_INFO,"  Pos: %u",(u32)(link->position));

	char buffer[SEQUENCE_LINK_BASES+1];
	unpackSequence(link->packedSequence, (u32)link->length, buffer);

	LOG(LOG_INFO,"  Seq: %s",buffer);
}

void trDumpLookupLink(LookupLink *link, u32 linkIndex)
{
//	u32 sourceIndex;		// Index of SeqLink or Dispatch
//	u8 indexType;			// Indicates meaning of previous (SeqLink or Dispatch)
//	u8 smerCount;			// Number of smers to lookup
//	u16 revComp;			// Indicates if the original smer was rc (lsb = first smer)
//	SmerId smers[LOOKUP_LINK_SMERS];		// Specific smers to lookup

	LOG(LOG_INFO,"LookupLink Dump: Idx %i (%p)",linkIndex, link);

	LOG(LOG_INFO,"  SrcIdx: %u (%s)", link->sourceIndex, decodeLinkIndexType(link->indexType));
	LOG(LOG_INFO,"  SmerCount: %u", (u32)link->smerCount);

	char buffer[SMER_BASES+1];

	u16 revComp=link->revComp;
	for(int i=0;i<link->smerCount;i++)
		{
		unpackSmer(link->smers[i], buffer);

		if(revComp&1)
			LOG(LOG_INFO, "  Smer: %s (rc)", buffer);
		else
			LOG(LOG_INFO, "  Smer: %s", buffer);

		revComp>>=1;
		}
}

void trDumpDispatchLink(DispatchLink *link, u32 linkIndex)
{
	//	u32 nextOrOriginIndex;		// Index of Next Dispatch or Origin SeqLink
	//	u8 indexType;				// Indicates meaning of previous
	//	u8 length;					// How many valid indexedSmers are there
	//	u8 position;				// The current indexed smer
	//	u8 revComp;					// Indicate if each original smer was rc
	//	s32 minEdgePosition;
	//	s32 maxEdgePosition;
	//	DispatchLinkSmer smers[DISPATCH_LINK_SMERS];

	LOG(LOG_INFO,"DispatchLink Dump: Idx %i (%p)",linkIndex, link);

	LOG(LOG_INFO,"  NosIdx: %u (%s)", link->nextOrSourceIndex, decodeLinkIndexType(link->indexType));
	LOG(LOG_INFO,"  Len: %u",(u32)(link->length));
	LOG(LOG_INFO,"  Pos: %u",(u32)(link->position));

	LOG(LOG_INFO,"  EdgePos: [%i %i]", link->minEdgePosition, link->maxEdgePosition);

	char buffer[SMER_BASES+1];

	u16 revComp=link->revComp;
	for(int i=0;i<link->length;i++)
		{
		DispatchLinkSmer *dls=link->smers+i;
		unpackSmer(dls->smer, buffer);

		if(revComp&1)
			LOG(LOG_INFO, "  Smer: %s (rc) Offset: %u Slice: %u SliceIndex: %u", buffer, (u32)dls->seqIndexOffset, (u32)dls->slice, dls->sliceIndex);
		else
			LOG(LOG_INFO, "  Smer: %s      Offset: %u Slice: %u SliceIndex: %u", buffer, (u32)dls->seqIndexOffset, (u32)dls->slice, dls->sliceIndex);

		revComp>>=1;
		}

}


static void *trDoRegister(ParallelTask *pt, int workerNo, int totalWorkers)
{
	RoutingWorkerState *workerState=G_ALLOC(sizeof(RoutingWorkerState), MEMTRACKID_ROUTING_WORKERSTATE);

	workerState->lookupSliceStart=(SMER_SLICES*workerNo)/totalWorkers;
	workerState->lookupSliceEnd=(SMER_SLICES*(workerNo+1))/totalWorkers;

	workerState->dispatchGroupStart=(SMER_DISPATCH_GROUPS*workerNo)/totalWorkers;
	workerState->dispatchGroupEnd=(SMER_DISPATCH_GROUPS*(workerNo+1))/totalWorkers;

	LOG(LOG_INFO,"Worker %i of %i - Lookup Range %i %i DispatchGroup Range %i %i",
			workerNo,totalWorkers,
			workerState->lookupSliceStart, workerState->lookupSliceEnd,
			workerState->dispatchGroupStart, workerState->dispatchGroupEnd);


	//workerState->lookupSliceDefault=workerState->lookupSliceCurrent=(workerNo*SMER_SLICE_PRIME)&SMER_SLICE_MASK;
	//workerState->dispatchGroupDefault=workerState->dispatchGroupCurrent=(workerNo*SMER_DISPATCH_GROUP_PRIME)&SMER_DISPATCH_GROUP_MASK;

	return workerState;
}

static void trDoDeregister(ParallelTask *pt, int workerNo, void *wState)
{
	G_FREE(wState, sizeof(RoutingWorkerState), MEMTRACKID_ROUTING_WORKERSTATE);
}

static int trAllocateIngressSlot(ParallelTask *pt, int workerNo)
{
	RoutingBuilder *rb=pt->dataPtr;

	int res=reserveReadIngressBlock(rb);

	return res;
}


static int trDoIngress(ParallelTask *pt, int workerNo,void *wState, void *ingressPtr, int ingressPosition, int ingressSize)
{
	SwqBuffer *rec=ingressPtr;
	RoutingBuilder *rb=pt->dataPtr;
	Graph *graph=rb->graph;
	s32 nodeSize=graph->config.nodeSize;

	populateReadIngressBlock(rec,ingressPosition,ingressSize,nodeSize,rb);

	return 1;
}




/*
static void dumpUncleanDispatchReadBlocks(int blockNum, RoutingReadDispatchBlock *readDispatchBlock)
{
	if(readDispatchBlock->completionCount>0)
		{
		LOG(LOG_INFO,"INCOMPLETE: RDB BlockNum: %i Status: %i RemainingCount: %i",blockNum,readDispatchBlock->status,readDispatchBlock->completionCount);

		for(int i=0;i<readDispatchBlock->readCount;i++)
			{
			if(readDispatchBlock->readData[i]->indexCount!=-1)
				{
				int indexCount=readDispatchBlock->readData[i]->indexCount;

				if(indexCount>=0)
					{
					LOG(LOG_INFO,"INCOMPLETE: Blocked read: %i at indexCount %i in slice %i",i,indexCount, readDispatchBlock->readData[i]->indexedData[indexCount].slice);
					}
				else
					{
					LOG(LOG_INFO,"INCOMPLETE: %i at indexCount %i",i,readDispatchBlock->readData[i]->indexCount);
					}
				}
			}
		}

}

static void dumpUncleanDispatchGroup(int groupNum, RoutingReadReferenceBlockDispatch *dispatchPtr, RoutingDispatchGroupState *dispatchGroupState)
{
	if(dispatchPtr!=NULL)
		{
		LOG(LOG_INFO,"INCOMPLETE: Dispatch ptr not null: %i", groupNum);
		}

	if(dispatchGroupState->status!=0)
		LOG(LOG_INFO,"INCOMPLETE: DispatchGroup Status for %i is %i", groupNum,dispatchGroupState->status);

	//dispatchGroupState->outboundDispatches;

	for(int i=0;i<SMER_DISPATCH_GROUP_SLICES;i++)
		{
		if(dispatchGroupState->smerInboundDispatches[i].entryCount>0)
			LOG(LOG_INFO,"INCOMPLETE: Inbound items: %i %i %i", groupNum, i, dispatchGroupState->smerInboundDispatches[i].entryCount);
		}

}
*/

static void dumpUnclean(RoutingBuilder *rb)
{
	LOG(LOG_INFO,"TODO: Dump unclean??");
	/*
	for(int i=0;i<TR_READBLOCK_DISPATCHES_INFLIGHT;i++)
		dumpUncleanDispatchReadBlocks(i, rb->readDispatchBlocks+i);

	for(int i=0;i<SMER_DISPATCH_GROUPS;i++)
		dumpUncleanDispatchGroup(i, rb->dispatchPtr[i], rb->dispatchGroupState+i);
		*/

}


/* Intermediate:

	Non-force:
		Handle complete dispatch block - top priority, frees up memory

		Handle complete lookup block - second priority, move into dispatch

		if allocated dispatch blocks is max: - if dispatch full, perform
			dispatch scan

		if allocated lookup blocks is max: - if lookup full, perform
			lookup scan


	Force:
		Handle complete dispatch block

		Handle complete lookup block

		if allocated dispatch blocks is max:
			dispatch scan

		lookup scan

		dispatch scan

*/


static void showWorkStatus(RoutingBuilder *rb)
{
	int arib=__atomic_load_n(&rb->allocatedIngressBlocks, __ATOMIC_SEQ_CST);
	int arlb=__atomic_load_n(&rb->allocatedReadLookupBlocks, __ATOMIC_SEQ_CST);
	int lrr=__atomic_load_n(&rb->lookupRecyclePtr, __ATOMIC_SEQ_CST)!=NULL;
	int ardb=0;//__atomic_load_n(&rb->allocatedReadDispatchBlocks, __ATOMIC_SEQ_CST);

	MemSingleBrickPile *seqPile=&(rb->sequenceLinkPile);
	MemDoubleBrickPile *lookupPile=&(rb->lookupLinkPile);
	MemDoubleBrickPile *dispatchPile=&(rb->dispatchLinkPile);

	int seqTotal=mbGetSingleBrickPileCapacity(seqPile);
	int seqFree=mbGetFreeSingleBrickPile(seqPile);
	int seqUsed=seqTotal-seqFree;

	int lookupTotal=mbGetDoubleBrickPileCapacity(lookupPile);
	int lookupFree=mbGetFreeDoubleBrickPile(lookupPile);
	int lookupUsed=lookupTotal-lookupFree;

	int dispatchTotal=mbGetDoubleBrickPileCapacity(dispatchPile);
	int dispatchFree=mbGetFreeDoubleBrickPile(dispatchPile);
	int dispatchUsed=dispatchTotal-dispatchFree;

	LOG(LOG_INFO,"TickTock: RB - IB: %i LB: %i LR: %i DB: %i", arib, arlb, lrr, ardb);
	LOG(LOG_INFO,"TickTock: BricksUFT - Seq %i %i %i Lookup %i %i %i Dispatch %i %i %i",
			seqUsed,seqFree,seqTotal, lookupUsed,lookupFree,lookupTotal, dispatchUsed,dispatchFree,dispatchTotal);

	ParallelTask *pt=rb->pt;

	int iat=__atomic_load_n(&pt->ingressAcceptToken, __ATOMIC_SEQ_CST);
	int ict=__atomic_load_n(&pt->ingressConsumeToken, __ATOMIC_SEQ_CST);
	ParallelTaskIngress *aiptr=__atomic_load_n(&pt->activeIngressPtr, __ATOMIC_SEQ_CST);
	int aip=__atomic_load_n(&pt->activeIngressPosition, __ATOMIC_SEQ_CST);

	int iTotal=0;
	if(aiptr!=NULL)
		iTotal=__atomic_load_n(&aiptr->ingressTotal, __ATOMIC_SEQ_CST);

	LOG(LOG_INFO,"TickTock: Ingress: IAT: %i ICT: %i IPtr: %p IPos: %i ITot: %i",iat,ict,aiptr,aip, iTotal);
}


static int checkForWork(RoutingBuilder *rb, int *lookupBlocksPtr, int *dispatchBlocksPtr)
{
	int arib=__atomic_load_n(&rb->allocatedIngressBlocks, __ATOMIC_SEQ_CST);
	int arlb=__atomic_load_n(&rb->allocatedReadLookupBlocks, __ATOMIC_SEQ_CST);
	int lrr=__atomic_load_n(&rb->lookupRecyclePtr, __ATOMIC_SEQ_CST)!=NULL;
	int ardb=0;//__atomic_load_n(&rb->allocatedReadDispatchBlocks, __ATOMIC_SEQ_CST);

	//*ingressBlocksPtr=arib;
	*lookupBlocksPtr=arlb;
	*dispatchBlocksPtr=ardb;

	return arib || arlb || lrr || ardb;
}


static int trDoIntermediate(ParallelTask *pt, int workerNo, void *wState, int force)
{
	//LOG(LOG_INFO,"trDoIntermediate");

	RoutingWorkerState *workerState=wState;
	RoutingBuilder *rb=pt->dataPtr;






//	sleep(1);

//	RoutingWorkerState *workerState=wState;
//	RoutingBuilder *rb=pt->dataPtr;

/*
	if(__atomic_load_n(&rb->pogoSuppressionFlag, __ATOMIC_RELAXED))
		{
		__atomic_store_n(&rb->pogoSuppressionFlag, 0, __ATOMIC_RELAXED);

		int lookupReads=countLookupReadsRemaining(rb);
		int dispatchReads=countDispatchReadsRemaining(rb);

		int arlb=__atomic_load_n(&rb->allocatedReadLookupBlocks, __ATOMIC_SEQ_CST);
		int ardb=__atomic_load_n(&rb->allocatedReadDispatchBlocks, __ATOMIC_SEQ_CST);

		LOG(LOG_INFO,"Lookup %i (%i) Dispatch %i (%i) - force %i",lookupReads,arlb, dispatchReads, ardb, force);
		}
*/

	/*
	if(scanForCompleteReadDispatchBlocks(rb))
		{
		//LOG(LOG_INFO,"scanForCompleteReadDispatchBlocks OK");

		if(!force)
			return 1;
		}
*/

	if(scanForAndDispatchLookupCompleteReadLookupBlocks(rb))
		{
		if(!force)
			return 1;
		}

	scanForAndProcessLookupRecycles(rb, force);

//	if(scanForAndProcessLookupRecycles(rb, force))
//		{
//		if(!force)
//			return 1;
//		}

	processIngressedReads(rb);

//	if(processIngressedReads(rb))
//		{
//		if(!force)
//			return 1;
//		}

	int arlb=0;
	int ardb=0;

	checkForWork(rb, &arlb, &ardb);

/*
	if(ardb==TR_READBLOCK_DISPATCHES_INFLIGHT)
		{
		if(scanForDispatches(rb, workerNo, workerState, force))
			{
			//LOG(LOG_INFO,"scanForDispatches 1");
			return 1;
			}
		}
*/
	if((force)||(arlb>TR_READBLOCK_LOOKUPS_THRESHOLD))
		{
		if(scanForSmerLookups(rb, workerNo, workerState))
			{
			//LOG(LOG_INFO,"scanForSmerLookups");
			return 1;
			}
		}
/*
	if(force)
		{
		if(scanForDispatches(rb, workerNo, workerState, force))
			{
			//LOG(LOG_INFO,"scanForDispatches 2");
			return 1;
			}
		}
*/

	if(checkForWork(rb, &arlb, &ardb)) // If in force mode, and not finished, rally the minions
		return force;

	//LOG(LOG_INFO,"Worker did nothing");
	return 0;
}



static int trDoTidy(ParallelTask *pt, int workerNo, void *wState, int tidyNo)
{
	return 0;
}

static void trDoTickTock(ParallelTask *pt)
{
	static int counter=0;
//	LOG(LOG_INFO,"Tick tock MF");

	RoutingBuilder *rb=pt->dataPtr;
	__atomic_store_n(&(rb->pogoDebugFlag), 1, __ATOMIC_RELAXED);

	if(++counter>1000)
		{
		showWorkStatus(rb);
		counter=0;
		}

}


RoutingBuilder *allocRoutingBuilder(Graph *graph, int threads)
{
	RoutingBuilder *rb=trRoutingBuilderAlloc();

	ParallelTaskConfig *ptc=allocParallelTaskConfig(trDoRegister,trDoDeregister,trAllocateIngressSlot,trDoIngress,trDoIntermediate,trDoTidy,trDoTickTock,
			threads,
			TR_INGRESS_BLOCKSIZE, TR_INGRESS_PER_TIDY_MIN, TR_INGRESS_PER_TIDY_MAX, TR_TIDYS_PER_BACKOFF, 0);//SMER_HASH_SLICES);

	rb->pt=allocParallelTask(ptc,rb);
	rb->graph=graph;

	rb->pogoDebugFlag=1;

	if(sizeof(SequenceLink)!=SINGLEBRICK_BRICKSIZE)
		LOG(LOG_CRITICAL,"SequenceLink size %i doesn't match expected %i", sizeof(SequenceLink), SINGLEBRICK_BRICKSIZE);

	mbInitSingleBrickPile(&(rb->sequenceLinkPile), TR_BRICKCHUNKS_SEQUENCE_MIN, TR_BRICKCHUNKS_SEQUENCE_MAX, MEMTRACKID_BRICK_SEQ);

	for(int i=0;i<TR_READBLOCK_INGRESS_INFLIGHT;i++)
		rb->ingressBlocks[i].status=BLOCK_STATUS_IDLE;
	rb->allocatedIngressBlocks=0;

	mbInitDoubleBrickPile(&(rb->lookupLinkPile), TR_BRICKCHUNKS_LOOKUP_MIN, TR_BRICKCHUNKS_LOOKUP_MAX, MEMTRACKID_BRICK_LOOKUP);

	for(int i=0;i<TR_READBLOCK_LOOKUPS_INFLIGHT;i++)
		{
		rb->readLookupBlocks[i].disp=dispenserAlloc(MEMTRACKID_DISPENSER_ROUTING_LOOKUP, DISPENSER_BLOCKSIZE_SMALL, DISPENSER_BLOCKSIZE_MEDIUM);
		rb->readLookupBlocks[i].compStat.split.status=BLOCK_STATUS_IDLE;
		}
	rb->allocatedReadLookupBlocks=0;

	for(int i=0;i<SMER_SLICES;i++)
		rb->smerEntryLookupPtr[i]=NULL;

	rb->lookupRecyclePtr=NULL;
	rb->lookupRecycleCount=0;

	mbInitDoubleBrickPile(&(rb->dispatchLinkPile), TR_BRICKCHUNKS_DISPATCH_MIN, TR_BRICKCHUNKS_DISPATCH_MAX, MEMTRACKID_BRICK_DISPATCH);



/*
	rb->allocatedReadDispatchBlocks=0;
	for(int i=0;i<TR_READBLOCK_DISPATCHES_INFLIGHT;i++)
		rb->readDispatchBlocks[i].disp=dispenserAlloc(MEMTRACKID_DISPENSER_ROUTING_DISPATCH, DISPENSER_BLOCKSIZE_MEDIUM, DISPENSER_BLOCKSIZE_HUGE);

	for(int i=0;i<SMER_DISPATCH_GROUPS;i++)
		{
		initRoutingDispatchGroupState(rb->dispatchGroupState+i);
		rb->dispatchPtr[i]=NULL;
		}
*/

	return rb;
}



void freeRoutingBuilder(RoutingBuilder *rb)
{
	dumpUnclean(rb);

	for(int i=0;i<TR_READBLOCK_LOOKUPS_INFLIGHT;i++)
		dispenserFree(rb->readLookupBlocks[i].disp);
/*
	for(int i=0;i<TR_READBLOCK_DISPATCHES_INFLIGHT;i++)
		dispenserFree(rb->readDispatchBlocks[i].disp);

	for(int i=0;i<SMER_DISPATCH_GROUPS;i++)
		freeRoutingDispatchGroupState(rb->dispatchGroupState+i);
*/

	ParallelTask *pt=rb->pt;
	ParallelTaskConfig *ptc=pt->config;

	freeParallelTask(pt);
	freeParallelTaskConfig(ptc);

	mbFreeSingleBrickPile(&(rb->sequenceLinkPile));
	mbFreeDoubleBrickPile(&(rb->lookupLinkPile));
	mbFreeDoubleBrickPile(&(rb->dispatchLinkPile));

	trRoutingBuilderFree(rb);
}


