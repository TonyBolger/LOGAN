#ifndef __SEQ_INDEX_H
#define __SEQ_INDEX_H

typedef struct sequenceFragmentStr {
	struct sequenceFragmentStr *next;
	u32 sequenceId;
	u32 startPosition;
	u32 endPosition;
} SequenceFragment;

typedef struct sequenceStr {
	struct SequenceStr *next;
	SequenceFragment *fragments;
	u32 fragmentCount;
	u32 totalLength;
} Sequence;

typedef struct sequenceSourceStr {
	struct sequenceSourceStr *next;
	char *name;
	Sequence *sequences;
	int sequenceCount;
} SequenceSource;


#endif
