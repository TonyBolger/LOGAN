
#include "common.h"


void siInitSequenceIndex(SequenceIndex *seqIndex)
{
	seqIndex->sequenceSource=NULL;
	seqIndex->sequenceSourceCount=0;
}


static IndexedSequenceSource *initSequenceSource(char *name)
{
	IndexedSequenceSource *seqSrc=siSequenceSourceAlloc();

	seqSrc->next=NULL;

	int nameLen=strlen(name);
	seqSrc->name=G_ALLOC(nameLen+1, MEMTRACKID_SEQINDEX);
	strncpy(seqSrc->name, name, nameLen);
	seqSrc->name[nameLen]=0;

	seqSrc->sequences=NULL;
	seqSrc->lastSequence=NULL;
	seqSrc->sequenceCount=0;

	return seqSrc;
}

static IndexedSequence *initSequence(char *name)
{
	IndexedSequence *seq=siSequenceAlloc();

	seq->next=NULL;

	int nameLen=strlen(name);
	seq->name=G_ALLOC(nameLen+1, MEMTRACKID_SEQINDEX);
	strncpy(seq->name, name, nameLen);
	seq->name[nameLen]=0;

	seq->fragments=NULL;
	seq->lastFragment=NULL;
	seq->fragmentCount=0;

	seq->totalLength=0;

	return seq;
}


void siCleanupSequenceIndex(SequenceIndex *seqIndex)
{
	IndexedSequenceSource *seqSrc=seqIndex->sequenceSource;
	while(seqSrc!=NULL)
		{
		G_FREE(seqSrc->name, strlen(seqSrc->name)+1, MEMTRACKID_SEQINDEX);

		IndexedSequence *seq=seqSrc->sequences;
		while(seq!=NULL)
			{
			G_FREE(seq->name, strlen(seq->name)+1, MEMTRACKID_SEQINDEX);

			IndexedSequenceFragment *seqFrag=seq->fragments;
			while(seqFrag!=NULL)
				{
				IndexedSequenceFragment *nextSeqFrag=seqFrag->next;
				siSequenceFragmentFree(seqFrag);
				seqFrag=nextSeqFrag;
				}

			IndexedSequence *nextSeq=seq->next;
			siSequenceFree(seq);
			seq=nextSeq;
			}

		IndexedSequenceSource *nextSrcSeq=seqSrc->next;
		siSequenceSourceFree(seqSrc);
		seqSrc=nextSrcSeq;
		}

	seqIndex->sequenceSource=NULL;
	seqIndex->lastSequenceSource=NULL;
	seqIndex->sequenceSourceCount=0;
}


void siDumpSequenceIndex(SequenceIndex *seqIndex)
{
	LOG(LOG_INFO,"Dump SequenceIndex contains %i Sources", seqIndex->sequenceSourceCount);

	IndexedSequenceSource *seqSrc=seqIndex->sequenceSource;
	while(seqSrc!=NULL)
		{
		LOG(LOG_INFO,"  Source %s contains %i Sequences", seqSrc->name, seqSrc->sequenceCount);

		IndexedSequence *seq=seqSrc->sequences;
		while(seq!=NULL)
			{
			LOG(LOG_INFO,"    Sequence %s (Length: %i) contains %i Fragments", seq->name, seq->totalLength, seq->fragmentCount);

			IndexedSequenceFragment *seqFrag=seq->fragments;
			while(seqFrag!=NULL)
				{
				LOG(LOG_INFO,"      Fragment ID: %u Range: %u:%u", seqFrag->sequenceId, seqFrag->startPosition, seqFrag->endPosition);
				seqFrag=seqFrag->next;
				}


/*			SequenceFragment *seqFrag=seq->fragments;
			LOG(LOG_INFO,"      First Fragment ID: %u Range: %u:%u", seqFrag->sequenceId, seqFrag->startPosition, seqFrag->endPosition);

			seqFrag=seq->lastFragment;
			LOG(LOG_INFO,"      Last Fragment ID: %u Range: %u:%u", seqFrag->sequenceId, seqFrag->startPosition, seqFrag->endPosition);
*/
			seq=seq->next;
			}

		seqSrc=seqSrc->next;
		}

	LOG(LOG_INFO,"Dump SequenceIndex complete");
}


IndexedSequenceSource *siAddSequenceSource(SequenceIndex *seqIndex, char *name)
{
	IndexedSequenceSource *seqSrc=initSequenceSource(name);

	if(seqIndex->sequenceSource==NULL)
		seqIndex->sequenceSource=seqSrc;
	else
		seqIndex->lastSequenceSource->next=seqSrc;

	seqIndex->lastSequenceSource=seqSrc;
	seqIndex->sequenceSourceCount++;

	return seqSrc;
}


IndexedSequence *siAddSequence(IndexedSequenceSource *seqSrc, char *name)
{
	IndexedSequence *seq=initSequence(name);

	if(seqSrc->sequences==NULL)
		seqSrc->sequences=seq;
	else
		seqSrc->lastSequence->next=seq;

	seqSrc->lastSequence=seq;
	seqSrc->sequenceCount++;

	return seq;
}


IndexedSequenceFragment *siAddSequenceFragment(IndexedSequence *seq, u32 sequenceId, u32 startPosition, u32 endPosition)
{
	IndexedSequenceFragment *seqFrag=siSequenceFragmentAlloc();

	seqFrag->next=NULL;

	seqFrag->sequenceId=sequenceId;
	seqFrag->startPosition=startPosition;
	seqFrag->endPosition=endPosition;

	if(seq->fragments==NULL)
		seq->fragments=seqFrag;
	else
		seq->lastFragment->next=seqFrag;

	seq->lastFragment=seqFrag;
	seq->fragmentCount++;

	return seqFrag;
}
