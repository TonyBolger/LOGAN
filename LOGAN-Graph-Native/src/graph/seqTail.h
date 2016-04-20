#ifndef __SEQ_TAIL_H
#define __SEQ_TAIL_H

#include "../common.h"

//#define SEQTAIL_BLOCKSIZE 16
//#define SEQTAIL_BLOCKMASK 0x0F

typedef struct seqTailBuilderStr
{
	u8 *data;

	s32 dataSize;
	s32 dataCount;

	MemStreamer *strm;
	MemDispenser *disp;
} SeqTailBuilder;



SeqTailBuilder *allocSeqTailBuilder(u8 *data, MemStreamer *strm, MemDispenser *disp);
u8 *finalizeSeqTailBuilder();

s32 findSeqTail(SmerId *smer, int tailLength);
s32 findOrCreateSeqTail(SmerId *smer, int tailLength);




/*

void setSeqTail(MemPool *memPool, SeqTail *seqTail, u8 *tail, u32 size);

void getSeqTailStats(SeqTail *seqTails, u32 *tails, u32 *tailBases, u32 *tailSize);
u8 *getSeqTailData(SeqTail *seqTails, u32 *sizePtr, MemDispenser *disp);

void getSeqTailSmers(SmerId rootSmer, SeqTail *seqTail, u32 rc, SmerId *tailSmers, u32 *tCompFlags);

void freeSeqTails(SeqTail *seqTails);
*/

#endif
