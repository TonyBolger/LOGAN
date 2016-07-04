
#include "../common.h"

SeqTailBuilder *allocSeqTailBuilder(u8 *data, MemStreamer *strm, MemDispenser *disp)
{
	SeqTailBuilder *builder=dAlloc(disp,sizeof(SeqTailBuilder));

	builder->data=data;
	builder->strm=strm;
	builder->disp=disp;

	if(strm!=NULL) // read only mode
		{

		}

	return builder;
}

u8 *finalizeSeqTailBuilder()
{
	return NULL;
}

s32 findSeqTail(SmerId *smer, int tailLength)
{
	return 0;
}

s32 findOrCreateSeqTail(SmerId *smer, int tailLength)
{
	return 0;
}

