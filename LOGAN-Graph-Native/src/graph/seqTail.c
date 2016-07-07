
#include "../common.h"

u8 *initSeqTailBuilder(SeqTailBuilder *builder, u8 *data, MemDispenser *disp)
{
	builder->disp=disp;

	int oldCount=0;
	s32 totalPackedSize=1;

	if(data!=NULL)
		oldCount=*data++;

	if(oldCount>0)
		{
		builder->oldTails=dAlloc(disp, sizeof(s64) * oldCount);

		for(int i=0;i<oldCount;i++)
			{
			s64 len=*data++;// Packing: Length byte, followed by packed 4 base per byte, as needed

			int bytes=(len+3)>>2;
			totalPackedSize+=bytes+1;

			SmerId smer=0;
			while(--bytes)
				smer=smer<<8 | *(data++);

			builder->oldTails[i]=(len<<48) | smer;
			}
		}
	else
		{
		builder->oldTails=NULL;
		}

	builder->oldTailCount=oldCount;
	builder->totalPackedSize=totalPackedSize;

	builder->newTails=NULL;
	builder->newTailCount=0;
	builder->newTailAlloc=0;

	return data;
}


s32 getSeqTailBuilderPackedSize(SeqTailBuilder *seqTailBuilder)
{
	return seqTailBuilder->totalPackedSize;
}

static s32 writeSeqTailBuilderPackedDataSingle(u8 *data, SmerId smer, s32 smerLength)
{
	int bytes=(smerLength+7)>>2;

	*data=smerLength;
	u8 *pos=data+bytes;

	while(smerLength>0)
		{
		*(--pos)=smer;
		smer>>=8;
		smerLength-=4;
		}

	return bytes;
}

void writeSeqTailBuilderPackedData(SeqTailBuilder *seqTailBuilder, u8 *data)
{
	u8 *initData=data;

	int oldCount=seqTailBuilder->oldTailCount;
	int newCount=seqTailBuilder->newTailCount;

	*(data++)=oldCount+newCount;

	for(int i=0;i<oldCount;i++)
		{
		s64 tail=seqTailBuilder->oldTails[i];
		SmerId smer=tail & SMER_MASK;
		s32 smerLength = tail >> 48;

		data+=writeSeqTailBuilderPackedDataSingle(data, smer, smerLength);
		}

	for(int i=0;i<newCount;i++)
		{
		s64 tail=seqTailBuilder->newTails[i];
		SmerId smer=tail & SMER_MASK;
		s32 smerLength = tail >> 48;

		data+=writeSeqTailBuilderPackedDataSingle(data, smer, smerLength);
		}

	int size=data-initData;

	if(size!=seqTailBuilder->totalPackedSize)
	{
		LOG(LOG_INFO,"Size %i did not match expected size %i",size,seqTailBuilder->totalPackedSize);
	}

}



static s32 findSeqTailIndex(SeqTailBuilder *seqTailBuilder, s64 tail)
{
	int oldCount=seqTailBuilder->oldTailCount;

	for(int i=0;i<oldCount;i++)
		if(seqTailBuilder->oldTails[i]==tail)
			return i+1;

	int newCount=seqTailBuilder->newTailCount;

	for(int i=0;i<newCount;i++)
		if(seqTailBuilder->newTails[i]==tail)
			return oldCount+i+1;

	return -1;
}

s32 findSeqTail(SeqTailBuilder *seqTailBuilder, SmerId smer, s32 tailLength)
{
	if(tailLength==0)
		return 0;

	s64 tail=((s64)smer)|(((s64)tailLength)<<48);

	return findSeqTailIndex(seqTailBuilder, tail);
}

#define NEWTAIL_MINCOUNT 1

s32 findOrCreateSeqTail(SeqTailBuilder *seqTailBuilder, SmerId smer, s32 tailLength)
{
	if(tailLength==0)
		return 0;

	// Mask high part since existing sequences lose it
	long smerMask=~(-1L<<(tailLength*2));

	s64 tail=(((s64)smer) & smerMask) | (((s64)tailLength)<<48);

	s32 index=findSeqTailIndex(seqTailBuilder, tail);

	if(index>=0)
		{
		return index;
		}

	if(seqTailBuilder->newTailAlloc==0)
		{
		seqTailBuilder->newTails=dAlloc(seqTailBuilder->disp, sizeof(s64) * NEWTAIL_MINCOUNT);
		seqTailBuilder->newTailAlloc=NEWTAIL_MINCOUNT;
		}
	else if(seqTailBuilder->newTailCount>=seqTailBuilder->newTailAlloc)
		{
		int newTailAlloc=seqTailBuilder->newTailAlloc*2;
		s64 *newTails=dAlloc(seqTailBuilder->disp, sizeof(s64)*newTailAlloc);
		memcpy(newTails, seqTailBuilder->newTails, sizeof(s64)*seqTailBuilder->newTailAlloc);

		seqTailBuilder->newTails=newTails;
		seqTailBuilder->newTailAlloc=newTailAlloc;
		}

	seqTailBuilder->newTails[seqTailBuilder->newTailCount++]=tail;
	seqTailBuilder->totalPackedSize+=(tailLength+7)>>2; // Round up plus size byte

	return seqTailBuilder->oldTailCount+seqTailBuilder->newTailCount; // Increment provides offset
}

