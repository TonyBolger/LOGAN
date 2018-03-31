#ifndef __SEQ_INDEX_H
#define __SEQ_INDEX_H

typedef struct indexedSequenceFragmentStr {
	struct indexedSequenceFragmentStr *next;
	u32 sequenceId;
	u32 startPosition;
	u32 endPosition;
} IndexedSequenceFragment;

typedef struct indexedSequenceStr {
	struct indexedSequenceStr *next;
	char *name;
	IndexedSequenceFragment *fragments;
	IndexedSequenceFragment *lastFragment;
	u32 fragmentCount;
	u32 totalLength;
} IndexedSequence;

typedef struct indexedSequenceSourceStr {
	struct indexedSequenceSourceStr *next;
	char *name;
	IndexedSequence *sequences;
	IndexedSequence *lastSequence;
	int sequenceCount;
} IndexedSequenceSource;

typedef struct sequenceIndexStr {
	IndexedSequenceSource *sequenceSource;
	IndexedSequenceSource *lastSequenceSource;
	int sequenceSourceCount;
} SequenceIndex;


void siInitSequenceIndex(SequenceIndex *seqIndex);
void siCleanupSequenceIndex(SequenceIndex *seqIndex);

void siDumpSequenceIndex(SequenceIndex *seqIndex);

IndexedSequenceSource *siAddSequenceSource(SequenceIndex *seqIndex, char *name);
IndexedSequence *siAddSequence(IndexedSequenceSource *seqSrc, char *name);
IndexedSequenceFragment *siAddSequenceFragment(IndexedSequence *seq, u32 sequenceId, u32 startPosition, u32 endPosition);


#endif
