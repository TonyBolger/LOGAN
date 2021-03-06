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

	LOGN(LOG_INFO,"SequenceLink Dump: Idx %i (%p)",linkIndex, link);

	LOGN(LOG_INFO,"  NextIdx: %u", link->nextIndex);
	LOGN(LOG_INFO,"  Pos: %u",(u32)(link->position));
	LOGN(LOG_INFO,"  Len: %u",(u32)(link->seqLength));

	u8 seqBuffer[SEQUENCE_LINK_BASES+1];
	unpackSequence(link->packedSequence, (u32)link->seqLength, seqBuffer);

	LOGN(LOG_INFO,"  Seq: %s",seqBuffer);

	LOGN(LOG_INFO,"  Tag Length: %i", link->tagLength);

	if(link->tagLength>0)
		{
		LOGS(LOG_INFO,"  TagData: ");

		int tagPosition=(link->seqLength+3)>>2;
		for(int i=0;i<link->tagLength;i++)
			LOGS(LOG_INFO,"%02x ",link->packedSequence[tagPosition+i]);

		LOGN(LOG_INFO,"");
		}
}


void trDumpSequenceLinkChain(MemSingleBrickPile *seqPile, SequenceLink *link, u32 linkIndex)
{
	trDumpSequenceLink(link, linkIndex);

	while(link->nextIndex!=LINK_INDEX_DUMMY)
		{
		linkIndex=link->nextIndex;
		link=mbSingleBrickFindByIndex(seqPile, linkIndex);

		trDumpSequenceLink(link, linkIndex);
		}
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
	LOG(LOG_INFO,"  SmerCount: %i", link->smerCount);

	u8 buffer[SMER_BASES+1];

	u16 revComp=link->revComp;

	int smerCount=link->smerCount;
	smerCount=(smerCount<0)?-smerCount:smerCount;

	for(int i=0;i<smerCount;i++)
		{
		unpackSmer(link->smers[i], buffer);

		if(revComp&1)
			LOG(LOG_INFO, "  Smer: %s (rc) %012lx", buffer, link->smers[i]);
		else
			LOG(LOG_INFO, "  Smer: %s      %012lx", buffer, link->smers[i]);

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

	u8 buffer[SMER_BASES+1];

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


void trDumpDispatchLinkChain(MemDoubleBrickPile *dispatchPile, DispatchLink *link, u32 linkIndex)
{
	trDumpDispatchLink(link, linkIndex);

	while(link->indexType==LINK_INDEXTYPE_DISPATCH)
		{
		linkIndex=link->nextOrSourceIndex;
		link=mbDoubleBrickFindByIndex(dispatchPile, linkIndex);

		trDumpDispatchLink(link, linkIndex);
		}
}

void trDumpLinkChain(MemSingleBrickPile *sequencePile, MemDoubleBrickPile *lookupPile, MemDoubleBrickPile *dispatchPile, u32 linkIndex, u8 indexType)
{
	while(linkIndex!=LINK_INDEX_DUMMY)
		{
		SequenceLink *seqLink=NULL;
		LookupLink *lookupLink=NULL;
		DispatchLink *dispatchLink=NULL;

		switch(indexType)
			{
			case LINK_INDEXTYPE_SEQ:
				seqLink=mbSingleBrickFindByIndex(sequencePile, linkIndex);
				trDumpSequenceLink(seqLink, linkIndex);
				linkIndex=seqLink->nextIndex;
				indexType=LINK_INDEXTYPE_SEQ;
				break;

			case LINK_INDEXTYPE_LOOKUP:
				lookupLink=mbDoubleBrickFindByIndex(lookupPile, linkIndex);
				trDumpLookupLink(lookupLink, linkIndex);
				linkIndex=lookupLink->sourceIndex;
				indexType=lookupLink->indexType;
				break;

			case LINK_INDEXTYPE_DISPATCH:
				dispatchLink=mbDoubleBrickFindByIndex(dispatchPile, linkIndex);
				trDumpDispatchLink(dispatchLink, linkIndex);
				linkIndex=dispatchLink->nextOrSourceIndex;
				indexType=dispatchLink->indexType;
				break;

			default:
				LOG(LOG_INFO,"Cannot dump unknown link type %i", indexType);
				return;
			}
		}
}


void trDumpLinkChainFromLookupLink(MemSingleBrickPile *sequencePile, MemDoubleBrickPile *lookupPile, MemDoubleBrickPile *dispatchPile,
		LookupLink *lookupLink, u32 lookupLinkIndex)
{
	trDumpLookupLink(lookupLink, lookupLinkIndex);
	trDumpLinkChain(sequencePile, lookupPile, dispatchPile, lookupLink->sourceIndex, lookupLink->indexType);
}


void trDumpLinkChainFromDispatchLink(MemSingleBrickPile *sequencePile, MemDoubleBrickPile *lookupPile, MemDoubleBrickPile *dispatchPile,
		DispatchLink *dispatchLink, u32 dispatchLinkIndex)
{
	trDumpDispatchLink(dispatchLink, dispatchLinkIndex);
	trDumpLinkChain(sequencePile, lookupPile, dispatchPile, dispatchLink->nextOrSourceIndex, dispatchLink->indexType);
}





static void *trDoRegister(ParallelTask *pt, int workerNo, int totalWorkers)
{
	RoutingWorkerState *workerState=G_ALLOC(sizeof(RoutingWorkerState), MEMTRACKID_ROUTING_WORKERSTATE);

	//int workerGroups=(totalWorkers+3)/4;
	//int workerGroupNo=workerNo%workerGroups;

	//workerState->lookupSliceStart=(SMER_SLICES*workerNo)/totalWorkers;
	//workerState->lookupSliceEnd=(SMER_SLICES*(workerNo+1))/totalWorkers;

//	workerState->dispatchGroupStart=(SMER_DISPATCH_GROUPS*workerNo)/workerGroups;
//	workerState->dispatchGroupEnd=(SMER_DISPATCH_GROUPS*(workerNo+1))/workerGroups;

	//workerState->dispatchGroupStart=(SMER_DISPATCH_GROUPS*workerGroupNo)/workerGroups;
	//workerState->dispatchGroupEnd=(SMER_DISPATCH_GROUPS*(workerGroupNo+1))/workerGroups;

	//workerState->dispatchGroupCurrent=workerState->dispatchGroupStart;

	/*
	LOG(LOG_INFO,"Worker %i of %i - Lookup Range %i %i DispatchGroup Range %i %i",
			workerNo,totalWorkers,
			workerState->lookupSliceStart, workerState->lookupSliceEnd,
			workerState->dispatchGroupStart, workerState->dispatchGroupEnd);
*/

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
	u64 sif=__atomic_load_n(&rb->sequencesInFlight, __ATOMIC_SEQ_CST);

	int arib=__atomic_load_n(&rb->allocatedIngressBlocks, __ATOMIC_SEQ_CST);
	int arlb=__atomic_load_n(&rb->allocatedReadLookupBlocks, __ATOMIC_SEQ_CST);
	int lrpre=__atomic_load_n(&rb->lookupPredispatchRecycleCount, __ATOMIC_SEQ_CST);
	int lrpost=__atomic_load_n(&rb->lookupPostdispatchRecycleCount, __ATOMIC_SEQ_CST);

	LOG(LOG_INFO,"TickTock: SiF %li: IB: %i LB: %i LR_pre: %i LR_post: %i", sif, arib, arlb, lrpre, lrpost);

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


static u64 checkForWork(RoutingBuilder *rb)
{
	u64 sif=__atomic_load_n(&rb->sequencesInFlight, __ATOMIC_SEQ_CST);
	return sif;
}


static u64 getNextRoutingWorkToken(RoutingBuilder *rb, int workerNo)
{
	return __atomic_fetch_add(&(rb->routingWorkToken) ,1, __ATOMIC_SEQ_CST);
}


static int trDoIntermediate(ParallelTask *pt, int workerNo, void *wState, int force)
{
	//LOG(LOG_INFO,"trDoIntermediate");

	RoutingWorkerState *workerState=wState;
	RoutingBuilder *rb=pt->dataPtr;

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

//	scanForAndProcessLookupRecycles(rb, force);

	if(scanForAndProcessLookupRecycles(rb, force))
		{
		if(!force)
			return 1;
		}

//	processIngressedReads(rb);

	if(processIngressedReads(rb))
		return 1;

	int sequencesActive=checkForWork(rb);

	/*
	if((force)||(arlb>TR_READBLOCK_LOOKUPS_THRESHOLD))
		{
		if(scanForSmerLookups(rb, workerNo, workerState))
			{
			//LOG(LOG_INFO,"scanForSmerLookups");
			return 1;
			}
		}

	if(scanForDispatches(rb, workerNo, workerState, force))
		{
		//LOG(LOG_INFO,"scanForDispatches 2");
		return 1;
		}
*/


//	if(sequencesActive>TR_SEQUENCE_IN_FLIGHT_ROUTING_THRESHOLD)
//		LOG(LOG_INFO,"Above thresh %lu %lu", sequencesActive, TR_SEQUENCE_IN_FLIGHT_ROUTING_THRESHOLD);


	if((force)||(sequencesActive>TR_SEQUENCE_IN_FLIGHT_ROUTING_THRESHOLD))
		{


		u64 workToken=getNextRoutingWorkToken(rb, workerNo);

		int work=scanForSmerLookups(rb, workToken, workerNo, workerState, force);
		work+=scanForDispatches(rb, workToken, workerNo, workerState, force);

		if(work)
			return 1;
		}

	if(checkForWork(rb)) // If in force mode, and not finished, rally the minions
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
	RoutingBuilder *rb=pt->dataPtr;

	showWorkStatus(rb);
}



RoutingBuilder *allocRoutingBuilder(Graph *graph, int threads)
{
	RoutingBuilder *rb=trRoutingBuilderAlloc();

	ParallelTaskConfig *ptc=allocParallelTaskConfig(trDoRegister,trDoDeregister,trAllocateIngressSlot,trDoIngress,trDoIntermediate,trDoTidy,trDoTickTock,
			threads,
			TR_INGRESS_BLOCKSIZE, TR_INGRESS_PER_TIDY_MIN, TR_INGRESS_PER_TIDY_MAX, TR_TIDYS_PER_BACKOFF, 0);//SMER_HASH_SLICES);

	rb->pt=allocParallelTask(ptc,rb);
	rb->graph=graph;

	rb->thread=NULL;
	rb->threadData=NULL;

	rb->pogoDebugFlag=1;
	rb->routingWorkToken=0;

	rb->sequencesInFlight=0;

	if(sizeof(SequenceLink)!=SINGLEBRICK_BRICKSIZE)
		LOG(LOG_CRITICAL,"SequenceLink size %i doesn't match expected %i", sizeof(SequenceLink), SINGLEBRICK_BRICKSIZE);

	mbInitSingleBrickPile(&(rb->sequenceLinkPile), TR_BRICKCHUNKS_SEQUENCE_MIN, TR_BRICKCHUNKS_SEQUENCE_MAX, MEMTRACKID_BRICK_SEQ);

	for(int i=0;i<TR_READBLOCK_INGRESS_INFLIGHT;i++)
		{
		rb->ingressBlocks[i].status=INGRESS_BLOCK_STATUS_IDLE;
		}

	rb->allocatedIngressBlocks=0;

	mbInitDoubleBrickPile(&(rb->lookupLinkPile), TR_BRICKCHUNKS_LOOKUP_MIN, TR_BRICKCHUNKS_LOOKUP_MAX, MEMTRACKID_BRICK_LOOKUP);

	for(int i=0;i<TR_READBLOCK_LOOKUPS_INFLIGHT;i++)
		{
		rb->readLookupBlocks[i].disp=dispenserAlloc(MEMTRACKID_DISPENSER_ROUTING_LOOKUP, DISPENSER_BLOCKSIZE_MEDIUM, DISPENSER_BLOCKSIZE_LARGE);
		rb->readLookupBlocks[i].compStat=LOOKUP_BLOCK_STATUS_IDLE;
		rb->readLookupBlocks[i].outboundDispatches=NULL;
		}
	rb->allocatedReadLookupBlocks=0;

	for(int i=0;i<SMER_SLICES;i++)
		rb->smerEntryLookupPtr[i]=NULL;

	rb->lookupPredispatchRecyclePtr=NULL;
	rb->lookupPredispatchRecycleCount=0;

	rb->lookupPostdispatchRecyclePtr=NULL;
	rb->lookupPostdispatchRecycleCount=0;

	mbInitDoubleBrickPile(&(rb->dispatchLinkPile), TR_BRICKCHUNKS_DISPATCH_MIN, TR_BRICKCHUNKS_DISPATCH_MAX, MEMTRACKID_BRICK_DISPATCH);



/*
	rb->allocatedReadDispatchBlocks=0;
	for(int i=0;i<TR_READBLOCK_DISPATCHES_INFLIGHT;i++)
		rb->readDispatchBlocks[i].disp=dispenserAlloc(MEMTRACKID_DISPENSER_ROUTING_DISPATCH, DISPENSER_BLOCKSIZE_MEDIUM, DISPENSER_BLOCKSIZE_HUGE);
*/
	for(int i=0;i<SMER_DISPATCH_GROUPS;i++)
		{
		initRoutingDispatchGroupState(rb->dispatchGroupState+i);
		rb->dispatchPtr[i]=NULL;
		}

	return rb;
}


void *runRptWorker(void *voidData)
{
	RptThreadData *data=(RptThreadData *)voidData;
	RoutingBuilder *rb=data->routingBuilder;

    //setitimer(ITIMER_PROF, &(data->profilingTimer), NULL);

	performTask_worker(rb->pt);

	return NULL;
}


void createRoutingBuilderWorkers(RoutingBuilder *rb)
{
	int threadCount=rb->pt->config->expectedThreads;

	rb->thread=G_ALLOC(sizeof(pthread_t)*threadCount, MEMTRACKID_THREADS);
	rb->threadData=G_ALLOC(sizeof(IptThreadData)*threadCount, MEMTRACKID_THREADS);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, THREAD_INDEXING_STACKSIZE);

	int i=0;
	for(i=0;i<threadCount;i++)
		{
		rb->threadData[i].routingBuilder=rb;
		rb->threadData[i].threadIndex=i;

		int err=pthread_create(rb->thread+i,&attr,runRptWorker,rb->threadData+i);

		if(err)
			{
			char errBuf[ERRORBUF];
			strerror_r(err,errBuf,ERRORBUF);

			LOG(LOG_CRITICAL,"Error %i (%s) creating worker thread %i",err,errBuf,i);
			}
		}
}




void freeRoutingBuilder(RoutingBuilder *rb)
{
	dumpUnclean(rb);

	for(int i=0;i<TR_READBLOCK_LOOKUPS_INFLIGHT;i++)
		{
		cleanupRoutingDispatchArrays(rb->readLookupBlocks[i].outboundDispatches);
		dispenserFree(rb->readLookupBlocks[i].disp);
		}
/*
	for(int i=0;i<TR_READBLOCK_DISPATCHES_INFLIGHT;i++)
		dispenserFree(rb->readDispatchBlocks[i].disp);
*/

	for(int i=0;i<SMER_DISPATCH_GROUPS;i++)
		freeRoutingDispatchGroupState(rb->dispatchGroupState+i);

	ParallelTask *pt=rb->pt;
	ParallelTaskConfig *ptc=pt->config;

	int threadCount=rb->pt->config->expectedThreads;

        if(rb->thread!=NULL)
	    {
   	    void *status;

	    for(int i=0;i<threadCount;i++)
		pthread_join(rb->thread[i], &status);

	    G_FREE(rb->thread, sizeof(pthread_t)*threadCount, MEMTRACKID_THREADS);
	    }

	if(rb->threadData!=NULL)
		G_FREE(rb->threadData, sizeof(IptThreadData)*threadCount, MEMTRACKID_THREADS);

	freeParallelTask(pt);
	freeParallelTaskConfig(ptc);

	mbFreeSingleBrickPile(&(rb->sequenceLinkPile),"SequenceLink");
	mbFreeDoubleBrickPile(&(rb->lookupLinkPile),"LookupLink");
	mbFreeDoubleBrickPile(&(rb->dispatchLinkPile),"DispatchLink");

	trRoutingBuilderFree(rb);

	LOG(LOG_INFO,"Freed RoutingBuilder");
}


