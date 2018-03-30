#ifndef __SEQ_INDEX_H
#define __SEQ_INDEX_H

typedef struct sequenceFragmentStr {
	struct sequenceFragmentStr *next;
	u32 sequenceId;
	u32 startPosition;
	u32 endPosition;
} SequenceFragment;

typedef struct sequenceStr {
	struct sequenceStr *next;
	char *name;
	SequenceFragment *fragments;
	SequenceFragment *lastFragment;
	u32 fragmentCount;
	u32 totalLength;
} Sequence;

typedef struct sequenceSourceStr {
	struct sequenceSourceStr *next;
	char *name;
	Sequence *sequences;
	Sequence *lastSequence;
	int sequenceCount;
} SequenceSource;

typedef struct sequenceIndexStr {
	SequenceSource *sequenceSource;
	SequenceSource *lastSequenceSource;
	int sequenceSourceCount;
} SequenceIndex;


void siInitSequenceIndex(SequenceIndex *seqIndex);
void siCleanupSequenceIndex(SequenceIndex *seqIndex);

void siDumpSequenceIndex(SequenceIndex *seqIndex);

SequenceSource *siAddSequenceSource(SequenceIndex *seqIndex, char *name);
Sequence *siAddSequence(SequenceSource *seqSrc, char *name);
SequenceFragment *siAddSequenceFragment(Sequence *seq, u32 sequenceId, u32 startPosition, u32 endPosition);


#endif
