#ifndef __SEQ_TAIL_H
#define __SEQ_TAIL_H

#include "../common.h"

//#define SEQTAIL_BLOCKSIZE 16
//#define SEQTAIL_BLOCKMASK 0x0F

typedef struct seqTailBuilderStr
{
	MemDispenser *disp;

	s64 *oldTails;
	s64 *newTails;

	s32 oldTailCount;
	s32 newTailCount;

	s32 newTailAlloc;
	s32 totalPackedSize;

} SeqTailBuilder;



u8 *initSeqTailBuilder(SeqTailBuilder *builder, u8 *data, MemDispenser *disp);

s32 getSeqTailBuilderPackedSize(SeqTailBuilder *seqTailBuilder);
void writeSeqTailBuilderPackedData(SeqTailBuilder *seqTailBuilder, u8 *data);

s32 findSeqTail(SeqTailBuilder *seqTailBuilder, SmerId smer, s32 tailLength);
s32 findOrCreateSeqTail(SeqTailBuilder *seqTailBuilder, SmerId smer, s32 tailLength);



/*

void setSeqTail(MemPool *memPool, SeqTail *seqTail, u8 *tail, u32 size);

void getSeqTailStats(SeqTail *seqTails, u32 *tails, u32 *tailBases, u32 *tailSize);
u8 *getSeqTailData(SeqTail *seqTails, u32 *sizePtr, MemDispenser *disp);

void getSeqTailSmers(SmerId rootSmer, SeqTail *seqTail, u32 rc, SmerId *tailSmers, u32 *tCompFlags);

void freeSeqTails(SeqTail *seqTails);
*/

#endif
