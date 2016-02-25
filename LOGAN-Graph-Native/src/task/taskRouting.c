/*
 * taskRouting.c
 *
 *  Created on: Oct 28, 2015
 *      Author: tony
 */

#include "../common.h"




static void trDoRegister(ParallelTask *pt, int workerNo)
{

}

static void trDoDeregister(ParallelTask *pt, int workerNo)
{

}

static int trAllocateIngressSlot(ParallelTask *pt, int workerNo)
{
	RoutingBuilder *rb=pt->dataPtr;

	if(rb->allocatedReadBlocks<TR_READBLOCKS_INFLIGHT)
		{
		rb->allocatedReadBlocks++;
		return 1;
		}

	return 0;
}


/*
static void checkPossibleSmers(u32 maxValidIndex, SmerId *smerIds1, u32 *compFlags1, SmerId *smerIds2, u32 *compFlags2)
{
	for(int i=0;i<maxValidIndex;i++)
	{
		if(smerIds1[i]!=smerIds2[i] || compFlags1[i]!=compFlags2[i])
			{
			LOG(LOG_INFO,"Mismatch %lx %i vs %lx %i",smerIds1[i],compFlags1[i],smerIds2[i],compFlags2[i]);
			}
	}

}
*/
/*

static void ingressOne(Graph *graph, u8 *packedSeq, int length)
{

	s32 nodeSize=graph->config.nodeSize;
	s32 maxValidIndex=length-nodeSize;

	s32 *indexes=lAlloc((maxValidIndex+1)*sizeof(u32));
	u32 indexCount=0;

	SmerId *smerIds=lAlloc((maxValidIndex+1)*sizeof(SmerId));
	u32 *compFlags=lAlloc((maxValidIndex+1)*sizeof(u32));

	calculatePossibleSmersComp(packedSeq, maxValidIndex, smerIds, compFlags);
	indexCount=saFindIndexesOfExistingSmers(&(graph->smerArray),packedSeq, maxValidIndex, indexes, smerIds);

	saVerifyIndexing(graph->config.sparseness, indexes, indexCount, length, maxValidIndex);

//	if(indexCount>0)
//		LOG(LOG_INFO,"Found %i",indexCount);


}

static int trDoIngress(ParallelTask *pt, int workerNo,void *ingressPtr, int ingressPosition, int ingressSize)
{
	SwqBuffer *rec=ingressPtr;
	RoutingBuilder *rb=pt->dataPtr;
	Graph *graph=rb->graph;

	u8 *packedSeq=malloc(PAD_2BITLENGTH_BYTE(rec->maxSequenceTotalLength));

	int i=0;
	int ingressLast=ingressPosition+ingressSize;

	for(i=ingressPosition;i<ingressLast;i++)
		{
		SequenceWithQuality *currentRec=rec->rec+i;



		packSequence(currentRec->seq, packedSeq, currentRec->length);

		ingressOne(graph, packedSeq, currentRec->length);
		}

	free(packedSeq);

	return 0;
}
*/


static RoutingSmerEntryLookup *allocEntryLookupBlock(MemDispenser *disp)
{

	RoutingSmerEntryLookup *block=dAlloc(disp, sizeof(RoutingSmerEntryLookup));

	block->nextPtr=NULL;

	block->entryCount=0;
	block->entries=dAlloc(disp, TR_LOOKUPS_PER_SLICE_BLOCK*sizeof(SmerEntry));
	//block->presenceAbsence=NULL;//dAlloc(disp, PAD_BYTELENGTH_8BYTE(PAD_1BITLENGTH_BYTE(TR_LOOKUPS_PER_SLICE_BLOCK)));

	return block;
}


void expandEntryLookupBlock(RoutingSmerEntryLookup *block, MemDispenser *disp)
{
	int oldSize=block->entryCount;
	int size=oldSize*2;

	int oldEntrySize=oldSize*sizeof(SmerEntry);

	SmerEntry *entries=dAlloc(disp, size*sizeof(SmerEntry));
	memcpy(entries,block->entries,oldEntrySize);
	block->entries=entries;
	//block->presenceAbsence=NULL;//dAlloc(disp, PAD_BYTELENGTH_8BYTE(PAD_1BITLENGTH_BYTE(size)));
}



void assignSmersToEntryLookups(SmerId *smers, int smerCount, RoutingSmerEntryLookup *smerEntryLookups[], MemDispenser *disp)
{

	for(int i=0;i<smerCount;i++)
		{
		SmerId smer=smers[i];
		SmerEntry smerEntry=SMER_GET_BOTTOM(smer);
		u64 hash=hashForSmer(smerEntry);
		u32 slice=sliceForSmer(smer,hash);

		RoutingSmerEntryLookup *lookupForSlice=smerEntryLookups[slice];
		u64 entryCount=lookupForSlice->entryCount;

		if(entryCount>=TR_LOOKUPS_PER_SLICE_BLOCK && !((entryCount & (entryCount - 1))))
			expandEntryLookupBlock(lookupForSlice, disp);

		lookupForSlice->entries[entryCount]=smerEntry;
		lookupForSlice->entryCount++;

		}

}


static void queueLookupForSlice(RoutingBuilder *rb, RoutingSmerEntryLookup *lookupForSlice, int sliceNum)
{
	RoutingSmerEntryLookup *current=NULL;
	int loopCount=0;

	do
		{
		if(loopCount++>1000000)
			{
			LOG(LOG_INFO,"Loop Stuck");
			exit(0);
			}

		current=rb->smerEntryLookupPtr[sliceNum];
		lookupForSlice->nextPtr=current;
		}
	while(!__sync_bool_compare_and_swap(rb->smerEntryLookupPtr+sliceNum,current,lookupForSlice));

}

static RoutingSmerEntryLookup *dequeueLookupForSliceList(RoutingBuilder *rb, int sliceNum)
{
	RoutingSmerEntryLookup *current=NULL;
	int loopCount=0;

	do
		{
		if(loopCount++>1000000)
			{
			LOG(LOG_INFO,"Loop Stuck");
			exit(0);
			}

		current=rb->smerEntryLookupPtr[sliceNum];

		if(current==NULL)
			return NULL;
		}
	while(!__sync_bool_compare_and_swap(rb->smerEntryLookupPtr+sliceNum,current,NULL));

	return current;
}


static RoutingReadBlock *reserveReadBlock(RoutingBuilder *rb)
{
	//LOG(LOG_INFO,"Queue readblock");

	for(int loopCount=0;loopCount<100000;loopCount++)
		{
		int i;

		for(i=0;i<TR_READBLOCKS_INFLIGHT;i++)
			{
			if(rb->readBlocks[i].status==0)
				{
				if(__sync_bool_compare_and_swap(&(rb->readBlocks[i].status),0,1))
					return rb->readBlocks+i;
				}
			}
		}

	LOG(LOG_INFO,"Failed to queue readblock, should never happen"); // Can actually happen, just stupidly unlikely
	exit(1);
}

static void queueReadBlock(RoutingReadBlock *readBlock)
{
	if(!__sync_bool_compare_and_swap(&(readBlock->status),1,2))
		{
		LOG(LOG_INFO,"Tried to queue readblock in wrong state, should never happen");
		exit(1);
		}
}


static RoutingReadBlock *dequeueCompleteReadBlock(RoutingBuilder *rb)
{
	//LOG(LOG_INFO,"Try dequeue readblock");
	RoutingReadBlock *current;
	int i;

	for(i=0;i<TR_READBLOCKS_INFLIGHT;i++)
		{
		current=rb->readBlocks+i;
		if(current->status == 2 && current->completionCount==0)
			{
			if(__sync_bool_compare_and_swap(&(current->status),2,3))
				return current;
			}
		}
	return NULL;
}

static void unreserveReadBlock(RoutingReadBlock *readBlock)
{
	if(!__sync_bool_compare_and_swap(&(readBlock->status),3,0))
		{
		LOG(LOG_INFO,"Tried to unreserve readblock in wrong state, should never happen");
		exit(1);
		}
}



static int trDoIngress(ParallelTask *pt, int workerNo,void *ingressPtr, int ingressPosition, int ingressSize)
{
	SwqBuffer *rec=ingressPtr;
	RoutingBuilder *rb=pt->dataPtr;
	Graph *graph=rb->graph;
	s32 nodeSize=graph->config.nodeSize;

	MemDispenser *disp=dispenserAlloc("Routing");

	RoutingReadBlock *readBlock=reserveReadBlock(rb);
	readBlock->disp=disp;

	//int smerCount=0;
	int i=0;
	int ingressLast=ingressPosition+ingressSize;

	for(i=0;i<SMER_SLICES;i++)
		readBlock->smerEntryLookups[i]=allocEntryLookupBlock(readBlock->disp);

	SequenceWithQuality *currentRec=rec->rec+ingressPosition;
	RoutingReadData *readData=readBlock->readData;

	for(i=ingressPosition;i<ingressLast;i++)
		{
		int length=currentRec->length;
		readData->seqLength=length;

		readData->packedSeq=dAlloc(disp,PAD_2BITLENGTH_BYTE(length)); // Consider extra padding on these allocs
		//readData->quality=dAlloc(disp,length+1);
		readData->smers=dAlloc(readBlock->disp,length*sizeof(SmerId));
		readData->compFlags=dAlloc(readBlock->disp,length*sizeof(u8));

		packSequence(currentRec->seq, readData->packedSeq, length);

		s32 maxValidIndex=(length-nodeSize);
		calculatePossibleSmersComp(readData->packedSeq, maxValidIndex, readData->smers, readData->compFlags);

		assignSmersToEntryLookups(readData->smers, maxValidIndex+1, readBlock->smerEntryLookups, readBlock->disp); // Fixes problem, but not necessarily directly

		currentRec++;
		readData++;
		}


	int usedSlices=0;
	for(i=0;i<SMER_SLICES;i++)
		{
		RoutingSmerEntryLookup *lookupForSlice=readBlock->smerEntryLookups[i];

		if(1)//lookupForSlice->entryCount>0)
			{
			lookupForSlice->completionCountPtr=&(readBlock->completionCount);
			usedSlices++;
			}
		else
			lookupForSlice->completionCountPtr=NULL;
		}

	readBlock->completionCount=usedSlices;

	for(i=0;i<SMER_SLICES;i++)
		{
		RoutingSmerEntryLookup *lookupForSlice=readBlock->smerEntryLookups[i];
		if(1)//lookupForSlice->entryCount>0)
			queueLookupForSlice(rb, lookupForSlice, i);
		}

	queueReadBlock(readBlock);

	//dispenserFree(disp);

	return 0;
}


static int scanForCompleteReadBlocks(RoutingBuilder *rb)
{
	RoutingReadBlock *readBlock=dequeueCompleteReadBlock(rb);

	if(readBlock==NULL)
		return 0;

	dispenserFree(readBlock->disp);

	unreserveReadBlock(readBlock);

	__sync_fetch_and_add(&(rb->allocatedReadBlocks),-1);

	//LOG(LOG_INFO,"Complete readblock. Reserved: %i",count);

	return 1;
}

static int scanForSmerLookups(RoutingBuilder *rb, int startSlice, int endSlice)
{
	int work=0;

	for(int i=startSlice;i<endSlice;i++)
		{
		RoutingSmerEntryLookup *lookupEntry=dequeueLookupForSliceList(rb, i);

		if(lookupEntry!=NULL)
			{
			work++;
			SmerArraySlice *slice=rb->graph->smerArray.slice+i;

			RoutingSmerEntryLookup *lookupEntryScan=lookupEntry;

			while(lookupEntryScan!=NULL)
				{

				for(int j=0;j<lookupEntryScan->entryCount;j++)
					lookupEntryScan->entries[j]=saFindSmerEntry(slice,lookupEntryScan->entries[j]);

				__sync_fetch_and_add(lookupEntryScan->completionCountPtr,-1);
				lookupEntryScan=lookupEntryScan->nextPtr;
				}
			}

		}

	return work>0;
}


/* Intermediate:

	Non-force:
		Ready read block
		Slice lookup scan (if read blocks is max)

	Force:
		Ready read block
		Slice lookup scan

*/


static int trDoIntermediate(ParallelTask *pt, int workerNo, int force)
{
	RoutingBuilder *rb=pt->dataPtr;

//	LOG(LOG_INFO,"doIntermediate %i %i %i %i",workerNo,force,countReadBlocks(rb),countCompleteReadBlocks(rb));


	if(scanForCompleteReadBlocks(rb))
		return 1;

	if((force)||(rb->allocatedReadBlocks==TR_READBLOCKS_INFLIGHT))
		{
		int startPos=(workerNo*SMER_SLICE_PRIME)&SMER_SLICE_MASK;

		int work=0;

		work=scanForSmerLookups(rb,startPos,SMER_SLICES);
		work+=scanForSmerLookups(rb, 0, startPos);

		if(work)
			return 1;
		}

	return 0;
}

static int trDoTidy(ParallelTask *pt, int workerNo, int tidyNo)
{
	return 0;
}


RoutingBuilder *allocRoutingBuilder(Graph *graph, int threads)
{
	RoutingBuilder *rb=tiRoutingBuilderAlloc();

	ParallelTaskConfig *ptc=allocParallelTaskConfig(trDoRegister,trDoDeregister,trAllocateIngressSlot,trDoIngress,trDoIntermediate,trDoTidy,threads,
			TR_INGRESS_BLOCKSIZE, TR_INGRESS_PER_TIDY_MIN, TR_INGRESS_PER_TIDY_MAX, TR_TIDYS_PER_BACKOFF,
			16);//SMER_HASH_SLICES);

	rb->pt=allocParallelTask(ptc,rb);
	rb->graph=graph;

	rb->allocatedReadBlocks=0;
	for(int i=0;i<TR_READBLOCKS_INFLIGHT;i++)
		{
		rb->readBlocks[i].disp=NULL;
		}

	for(int i=0;i<SMER_SLICES;i++)
		rb->smerEntryLookupPtr[i]=NULL;

	return rb;
}

void freeRoutingBuilder(RoutingBuilder *rb)
{
	ParallelTask *pt=rb->pt;
	ParallelTaskConfig *ptc=pt->config;

	freeParallelTask(pt);
	freeParallelTaskConfig(ptc);

	tiRoutingBuilderFree(rb);
}


