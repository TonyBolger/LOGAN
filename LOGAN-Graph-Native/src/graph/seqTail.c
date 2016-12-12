
#include "common.h"



static u32 unpackTails(u8 *data, s64 *tails, s32 tailCount)
{
	u32 packedSize=0;

	for(int i=0;i<tailCount;i++)
		{
		s64 len=*data++;// Packing: Length byte, followed by packed 4 base per byte, as needed

		int bytes=(len+3)>>2;
		packedSize+=bytes+1;

		SmerId smer=0;
		while(bytes--)
			smer=smer<<8 | *(data++);

		tails[i]=(len<<48) | smer;
		}

	return packedSize;
}

static u8 *skipTails(u8 *data,  s32 tailCount)
{
	for(int i=0;i<tailCount;i++)
		{
		s64 len=*data++;
		int bytes=(len+3)>>2;
		data+=bytes;
		}

	return data;
}

static u8 *readSeqTailBuilderPackedData(SeqTailBuilder *builder, u8 *data)
{
	s32 totalPackedSize=0;
	int oldCount=0;

	if(data!=NULL)
		{
		u16 *countPtr=(u16 *)data;
		oldCount=*countPtr;
		data+=2;
		//LOG(LOG_INFO,"Loading %i tails from %p",oldCount,(data-1));
		}

	if(oldCount>0)
		{
		builder->oldTails=dAlloc(builder->disp, sizeof(s64) * oldCount);
		totalPackedSize+=unpackTails(data, builder->oldTails, oldCount);
		}
	else
		{
		builder->oldTails=NULL;
		}

	builder->oldTailCount=oldCount;
	builder->totalPackedSize=totalPackedSize+2;

	builder->oldDataSize=totalPackedSize;

	return data+totalPackedSize;

}

u8 *scanTails(u8 *data)
{
	if(data!=NULL)
		{
		u16 *countPtr=(u16 *)data;
		int count=*countPtr;
		data+=2;

		data=skipTails(data, count);
		return data;
		}

	return NULL;
}

u8 *initSeqTailBuilder(SeqTailBuilder *builder, u8 *data, MemDispenser *disp)
{
	builder->disp=disp;

	data=readSeqTailBuilderPackedData(builder,data);

	builder->newTails=NULL;
	builder->newTailCount=0;
	builder->newTailAlloc=0;

	return data;
}


s32 getSeqTailBuilderDirty(SeqTailBuilder *seqTailBuilder)
{
	return seqTailBuilder->newTailCount!=0;
}

s32 getSeqTailBuilderPackedSize(SeqTailBuilder *seqTailBuilder)
{
	return seqTailBuilder->totalPackedSize;
}

s32 getSeqTailTotalTailCount(SeqTailBuilder *builder)
{
	return builder->oldTailCount+builder->newTailCount;
}

void dumpSeqTailBuilder(SeqTailBuilder *builder)
{
	LOG(LOG_INFO,"Tail Count: %i %i", builder->oldTailCount,builder->newTailCount);

	for(int i=0;i<builder->oldTailCount;i++)
		{
		LOG(LOG_INFO,"Old: %16lx",builder->oldTails[i]);
		}

	for(int i=0;i<builder->newTailCount;i++)
		{
		LOG(LOG_INFO,"New: %16lx",builder->newTails[i]);
		}

}

static s32 writeSeqTailBuilderPackedDataSingle(u8 *data, SmerId smer, s32 smerLength)
{
	//LOG(LOG_INFO,"Writing tail of %i to %p",smerLength,data);
	int bytes=(smerLength+7)>>2;

	*data=smerLength;
	u8 *pos=data+bytes;

	while(smerLength>0)
		{
		*(--pos)=smer;

//		LOG(LOG_INFO,"Writing at %p",pos);

		smer>>=8;
		smerLength-=4;
		}

	return bytes;
}


u8 *writeSeqTailBuilderPackedData(SeqTailBuilder *builder, u8 *data)
{
	u8 *initData=data;

	int oldCount=builder->oldTailCount;
	int newCount=builder->newTailCount;

	//if(oldCount+newCount>4)
//		LOG(LOG_INFO,"Writing %i %i tails to %p",oldCount,newCount,data);

	u16 *countPtr=(u16 *)data;
	*countPtr=oldCount+newCount;
	data+=2;

	for(int i=0;i<oldCount;i++)
		{
		s64 tail=builder->oldTails[i];
		SmerId smer=tail & SMER_MASK;
		s32 smerLength = tail >> 48;

		data+=writeSeqTailBuilderPackedDataSingle(data, smer, smerLength);
		}

	for(int i=0;i<newCount;i++)
		{
		s64 tail=builder->newTails[i];
		SmerId smer=tail & SMER_MASK;
		s32 smerLength = tail >> 48;

		data+=writeSeqTailBuilderPackedDataSingle(data, smer, smerLength);
		}

	int size=data-initData;

	if(size!=builder->totalPackedSize)
	{
		LOG(LOG_INFO,"Problem writing seqTails");

		dumpSeqTailBuilder(builder);

		LOG(LOG_CRITICAL,"Size %i did not match expected size %i",size,builder->totalPackedSize);


	}

	return data;
}



static s32 findSeqTailIndex(SeqTailBuilder *builder, s64 tail)
{
	int oldCount=builder->oldTailCount;

	for(int i=0;i<oldCount;i++)
		if(builder->oldTails[i]==tail)
			{
//			LOG(LOG_INFO,"found old");
			return i+1;
			}

	int newCount=builder->newTailCount;

	for(int i=0;i<newCount;i++)
		if(builder->newTails[i]==tail)
			{
//			LOG(LOG_INFO,"found new");
			return oldCount+i+1;
			}


	return -1;
}

s32 findSeqTail(SeqTailBuilder *builder, SmerId smer, s32 tailLength)
{
	if(tailLength==0)
		return 0;

	s64 tail=((s64)smer)|(((s64)tailLength)<<48);

	return findSeqTailIndex(builder, tail);
}

#define NEWTAIL_MINCOUNT 1

s32 findOrCreateSeqTail(SeqTailBuilder *builder, SmerId smer, s32 tailLength)
{
	if(tailLength==0)
		return 0;

	// Mask high part since existing sequences lose it
	long smerMask=~(-1L<<(tailLength*2));

	s64 tail=(((s64)smer) & smerMask) | (((s64)tailLength)<<48);

	s32 index=findSeqTailIndex(builder, tail);

	if(index>=0)
		{
		return index;
		}

	if(builder->newTailAlloc==0)
		{
		builder->newTails=dAlloc(builder->disp, sizeof(s64) * NEWTAIL_MINCOUNT);
		builder->newTailAlloc=NEWTAIL_MINCOUNT;
		}
	else if(builder->newTailCount>=builder->newTailAlloc)
		{
		int newTailAlloc=builder->newTailAlloc*2;
		s64 *newTails=dAlloc(builder->disp, sizeof(s64)*newTailAlloc);
		memcpy(newTails, builder->newTails, sizeof(s64)*builder->newTailAlloc);

		builder->newTails=newTails;
		builder->newTailAlloc=newTailAlloc;
		}

	//LOG(LOG_INFO,"Make new tail %li %i",smer,tailLength);

	builder->newTails[builder->newTailCount++]=tail;
	builder->totalPackedSize+=(tailLength+7)>>2; // Round up plus size byte

	return builder->oldTailCount+builder->newTailCount; // Increment provides offset
}


void *unpackPrefixesForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp)
{
	if(data!=NULL)
		{
		u16 *countPtr=(u16 *)data;
		smerLinked->prefixCount=*countPtr;
		data+=2;

		smerLinked->prefixes=dAlloc(disp, sizeof(s64)*smerLinked->prefixCount);

		data+=unpackTails(data, smerLinked->prefixes, smerLinked->prefixCount);

		smerLinked->prefixSmers=dAlloc(disp, sizeof(SmerId)*smerLinked->prefixCount);
		smerLinked->prefixSmerExists=dAlloc(disp, sizeof(s8)*smerLinked->prefixCount);

		SmerId smer=complementSmerId(smerLinked->smerId);

		for(int i=0;i<smerLinked->prefixCount;i++)
			{
			s64 prefix=smerLinked->prefixes[i];
			int lenBits=(prefix>>47);

			SmerId targetRC=(prefix|(smer<<lenBits))&SMER_MASK;
			SmerId target=complementSmerId(targetRC);

			//LOG(LOG_INFO,"Prefix Check %li vs %li",target,targetRC);

			if(target<targetRC)
				{
				smerLinked->prefixSmers[i]=target;
				smerLinked->prefixSmerExists[i]=1;
				}
			else
				{
				smerLinked->prefixSmers[i]=targetRC;
				smerLinked->prefixSmerExists[i]=-1;
				}
			}

		return data;
		}
	else
		{
		smerLinked->prefixCount=0;
		smerLinked->prefixes=NULL;
		smerLinked->prefixSmers=NULL;
		smerLinked->prefixSmerExists=NULL;

		return NULL;
		}

}


void *unpackSuffixesForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp)
{
	if(data!=NULL)
		{
		u16 *countPtr=(u16 *)data;
		smerLinked->suffixCount=*countPtr;
		data+=2;

		smerLinked->suffixes=dAlloc(disp, sizeof(s64)*smerLinked->suffixCount);

		data+=unpackTails(data, smerLinked->suffixes, smerLinked->suffixCount);

		smerLinked->suffixSmers=dAlloc(disp, sizeof(SmerId)*smerLinked->suffixCount);
		smerLinked->suffixSmerExists=dAlloc(disp, sizeof(s8)*smerLinked->suffixCount);

		SmerId smer=smerLinked->smerId;

		for(int i=0;i<smerLinked->suffixCount;i++)
			{
			s64 suffix=smerLinked->suffixes[i];
			int lenBits=(suffix>>47);

			SmerId target=(suffix|(smer<<lenBits))&SMER_MASK;
			SmerId targetRC=complementSmerId(target);

			//LOG(LOG_INFO,"Suffix Check %li vs %li",target,targetRC);

			if(target<targetRC)
				{
				smerLinked->suffixSmers[i]=target;
				smerLinked->suffixSmerExists[i]=1;
				}
			else
				{
				smerLinked->suffixSmers[i]=targetRC;
				smerLinked->suffixSmerExists[i]=-1;
				}
			}

		return data;
		}
	else
		{
		smerLinked->suffixCount=0;
		smerLinked->suffixes=NULL;
		smerLinked->suffixSmers=NULL;
		smerLinked->suffixSmerExists=NULL;

		return NULL;
		}
}
