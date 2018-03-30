
#include "common.h"


void siInitSequenceIndex(SequenceIndex *seqIndex)
{
	seqIndex->sequenceSource=NULL;
	seqIndex->sequenceSourceCount=0;
}


static SequenceSource *initSequenceSource(char *name)
{
	SequenceSource *seqSrc=siSequenceSourceAlloc();

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

static Sequence *initSequence(char *name)
{
	Sequence *seq=siSequenceAlloc();

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
	SequenceSource *seqSrc=seqIndex->sequenceSource;
	while(seqSrc!=NULL)
		{
		G_FREE(seqSrc->name, strlen(seqSrc->name)+1, MEMTRACKID_SEQINDEX);

		Sequence *seq=seqSrc->sequences;
		while(seq!=NULL)
			{
			G_FREE(seq->name, strlen(seq->name)+1, MEMTRACKID_SEQINDEX);

			SequenceFragment *seqFrag=seq->fragments;
			while(seqFrag!=NULL)
				{
				SequenceFragment *nextSeqFrag=seqFrag->next;
				siSequenceFragmentFree(seqFrag);
				seqFrag=nextSeqFrag;
				}

			Sequence *nextSeq=seq->next;
			siSequenceFree(seq);
			seq=nextSeq;
			}

		SequenceSource *nextSrcSeq=seqSrc->next;
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

	SequenceSource *seqSrc=seqIndex->sequenceSource;
	while(seqSrc!=NULL)
		{
		LOG(LOG_INFO,"  Source %s contains %i Sequences", seqSrc->name, seqSrc->sequenceCount);

		Sequence *seq=seqSrc->sequences;
		while(seq!=NULL)
			{
			LOG(LOG_INFO,"    Sequence %s (Length: %i) contains %i Fragments", seq->name, seq->totalLength, seq->fragmentCount);

			/*SequenceFragment *seqFrag=seq->fragments;
			while(seqFrag!=NULL)
				{
				LOG(LOG_INFO,"      Fragment ID: %u Range: %u:%u", seqFrag->sequenceId, seqFrag->startPosition, seqFrag->endPosition);
				seqFrag=seqFrag->next;
				}
*/

			SequenceFragment *seqFrag=seq->fragments;
			LOG(LOG_INFO,"      First Fragment ID: %u Range: %u:%u", seqFrag->sequenceId, seqFrag->startPosition, seqFrag->endPosition);

			seqFrag=seq->lastFragment;
			LOG(LOG_INFO,"      Last Fragment ID: %u Range: %u:%u", seqFrag->sequenceId, seqFrag->startPosition, seqFrag->endPosition);

			seq=seq->next;
			}

		seqSrc=seqSrc->next;
		}

	LOG(LOG_INFO,"Dump SequenceIndex complete");
}


SequenceSource *siAddSequenceSource(SequenceIndex *seqIndex, char *name)
{
	SequenceSource *seqSrc=initSequenceSource(name);

	if(seqIndex->sequenceSource==NULL)
		seqIndex->sequenceSource=seqSrc;
	else
		seqIndex->lastSequenceSource->next=seqSrc;

	seqIndex->lastSequenceSource=seqSrc;
	seqIndex->sequenceSourceCount++;

	return seqSrc;
}


Sequence *siAddSequence(SequenceSource *seqSrc, char *name)
{
	Sequence *seq=initSequence(name);

	if(seqSrc->sequences==NULL)
		seqSrc->sequences=seq;
	else
		seqSrc->lastSequence->next=seq;

	seqSrc->lastSequence=seq;
	seqSrc->sequenceCount++;

	return seq;
}


SequenceFragment *siAddSequenceFragment(Sequence *seq, u32 sequenceId, u32 startPosition, u32 endPosition)
{
	SequenceFragment *seqFrag=siSequenceFragmentAlloc();

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
