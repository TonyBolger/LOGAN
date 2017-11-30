#ifndef __SEQ_TAIL_H
#define __SEQ_TAIL_H

#include "../common.h"

//#define SEQTAIL_BLOCKSIZE 16
//#define SEQTAIL_BLOCKMASK 0x0F

typedef struct seqTailBuilderStr
{
	MemDispenser *disp;

	s32 oldDataSize;

	s64 *oldTails;
	s64 *newTails;

	s32 oldTailCount;
	s32 newTailCount;

	s32 newTailAlloc;

	s32 headerSize;
	s32 dataSize;

} SeqTailBuilder;


u8 *stScanTails(u8 *data);
u8 *stInitSeqTailBuilder(SeqTailBuilder *builder, u8 *data, MemDispenser *disp);

void stDumpSeqTailBuilder(SeqTailBuilder *builder);

s32 stGetSeqTailBuilderDirty(SeqTailBuilder *seqTailBuilder);
s32 stGetSeqTailBuilderPackedSize(SeqTailBuilder *seqTailBuilder);
s32 stGetSeqTailTotalTailCount(SeqTailBuilder *builder);

u8 *stWriteSeqTailBuilderPackedData(SeqTailBuilder *seqTailBuilder, u8 *data);

s32 stFindSeqTail(SeqTailBuilder *seqTailBuilder, SmerId smer, s32 tailLength);
s32 stFindOrCreateSeqTail(SeqTailBuilder *seqTailBuilder, SmerId smer, s32 tailLength);

void *stUnpackPrefixesForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp);
void *stUnpackSuffixesForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp);

#endif
