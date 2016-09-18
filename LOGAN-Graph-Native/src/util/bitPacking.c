/*
 * bitPacking.c
 *
 *  Created on: Nov 14, 2011
 *      Author: tony
 */

#include "common.h"



#define POS2INT32(x) ((x)>>5)
#define POS2BIT32(x) ((x)&0x1F)

#define POS2INT8(x) ((x)>>3)
#define POS2BIT8(x) ((x)&0x7)


/*
static u32 MASK_UPPER[]={
		0xFFFFFFFF,0x7FFFFFFF,0x3FFFFFFF,0x1FFFFFFF,0x0FFFFFFF,0x07FFFFFF,0x03FFFFFF,0x01FFFFFF,
		0x00FFFFFF,0x007FFFFF,0x003FFFFF,0x001FFFFF,0x000FFFFF,0x0007FFFF,0x0003FFFF,0x0001FFFF,
		0x0000FFFF,0x00007FFF,0x00003FFF,0x00001FFF,0x00000FFF,0x000007FF,0x000003FF,0x000001FF,
		0x000000FF,0x0000007F,0x0000003F,0x0000001F,0x0000000F,0x00000007,0x00000003,0x00000001,
		0x00000000};

static u32 MASK_KEEP_UPPER[]={
		0x00000000,0x80000000,0xC0000000,0xE0000000,0xF0000000,0xF8000000,0xFC000000,0xFE000000,
		0xFF000000,0xFF800000,0xFFC00000,0xFFE00000,0xFFF00000,0xFFF80000,0xFFFC0000,0xFFFE0000,
		0xFFFF0000,0xFFFF8000,0xFFFFC000,0xFFFFE000,0xFFFFF000,0xFFFFF800,0xFFFFFC00,0xFFFFFE00,
		0xFFFFFF00,0xFFFFFF80,0xFFFFFFC0,0xFFFFFFE0,0xFFFFFFF0,0xFFFFFFF8,0xFFFFFFFC,0xFFFFFFFE,
		0xFFFFFFFF};

static u32 MASK_LOWER_32[]={
		0xFFFFFFFF,0xFFFFFFFE,0xFFFFFFFC,0xFFFFFFF8,0xFFFFFFF0,0xFFFFFFE0,0xFFFFFFC0,0xFFFFFF80,
		0xFFFFFF00,0xFFFFFE00,0xFFFFFC00,0xFFFFF800,0xFFFFF000,0xFFFFE000,0xFFFFC000,0xFFFF8000,
		0xFFFF0000,0xFFFE0000,0xFFFC0000,0xFFF80000,0xFFF00000,0xFFE00000,0xFFC00000,0xFF800000,
		0xFF000000,0xFE000000,0xFC000000,0xF8000000,0xF0000000,0xE0000000,0xC0000000,0x80000000,
		0x00000000
};
*/

static u32 MASK_KEEP_LOWER_32[]={
		0x00000000,0x00000001,0x00000003,0x00000007,0x0000000F,0x0000001F,0x0000003F,0x0000007F,
		0x000000FF,0x000001FF,0x000003FF,0x000007FF,0x00000FFF,0x00001FFF,0x00003FFF,0x00007FFF,
		0x0000FFFF,0x0001FFFF,0x0003FFFF,0x0007FFFF,0x000FFFFF,0x001FFFFF,0x003FFFFF,0x007FFFFF,
		0x00FFFFFF,0x01FFFFFF,0x03FFFFFF,0x07FFFFFF,0x0FFFFFFF,0x1FFFFFFF,0x3FFFFFFF,0x7FFFFFFF,
		0xFFFFFFFF
};

#define SELECT_AND_SHIFTLEFT_32(x, a, b) (((x) & MASK_KEEP_LOWER_32[a]) << (b))
#define SHIFTRIGHT_AND_SELECT_32(x, a, b) (((x) >> (a)) & MASK_KEEP_LOWER_32[b])

#define MASK_RANGE_32(x, u, l) ((x) & (MASK_LOWER_32[u+1] | MASK_KEEP_LOWER_32[l]))


static u8 MASK_LOWER_8[]={0xFF,0xFE,0xFC,0xF8,0xF0,0xE0,0xC0,0x80,0x00};
static u8 MASK_KEEP_LOWER_8[]={0x00,0x01,0x03,0x07,0x0F,0x1F,0x3F,0x7F,0xFF};

#define SELECT_AND_SHIFTLEFT_8(x, a, b) (((x) & MASK_KEEP_LOWER_8[a]) << (b))
#define SHIFTRIGHT_AND_SELECT_8(x, a, b) (((x) >> (a)) & MASK_KEEP_LOWER_8[b])

#define MASK_RANGE_8(x, u, l) ((x) & (MASK_LOWER_8[u+1] | MASK_KEEP_LOWER_8[l]))



void initPacker(BitPacker *packer, u8 *ptr, int position)
{
	packer->data=ptr;
	packer->position=position;
}

void seekPacker(BitPacker *packer, int position)
{
	packer->position+=position;
}


void packBits(BitPacker *packer, int count, u32 data)
{
	if(count==0)
		return;

	u8 *packPtr=packer->data;

	int position=packer->position;
	int intPosition=POS2INT8(position);
	int bitPosition=POS2BIT8(position);

	int originalCount=count;

	while(count>0)
		{
		if(bitPosition+count<=8) // Consume remaining 'count'
			{
			int endBitPosition=bitPosition+count-1;
			int tmpData=SELECT_AND_SHIFTLEFT_8(data,count,bitPosition);

			*(packPtr+intPosition)=MASK_RANGE_8(*(packPtr+intPosition), endBitPosition, bitPosition)|tmpData;
			count=0;
			}
		else // Consume available space in current byte
			{
			int consume=8-bitPosition;
			int endBitPosition=bitPosition+consume-1;

			int tmpData=SELECT_AND_SHIFTLEFT_8(data, consume, bitPosition);
			*(packPtr+intPosition)=MASK_RANGE_8(*(packPtr+intPosition), endBitPosition, bitPosition)|tmpData;

			data>>=consume;
			count-=consume;
			bitPosition=0;
			intPosition++;
			}
		}

	packer->position+=originalCount;
}

/*
void packBits(BitPacker *packer, int count, u32 data)
{
	if(count==0)
		return;

	u32 *packPtr=packer->data;

	int position=packer->position;
	int intPosition=POS2INT32(position);
	int bitPosition=POS2BIT32(position);

	int endPosition=position+count-1;
	int endIntPosition=POS2INT32(endPosition);
	int endBitPosition=POS2BIT32(endPosition);

	if(intPosition==endIntPosition)
		{
		// No-wrap case: Copy 'count' bits from bits count-1:0 of data into packPtr[position], bits endBitPosition:bitPosition inclusive

		int tmpData=SELECT_AND_SHIFTLEFT_32(data,count,bitPosition);
		*(packPtr+intPosition)=MASK_RANGE(*(packPtr+intPosition), endBitPosition, bitPosition)|tmpData;
		}
	else
		{
		// Wrap case: Copy 'count1' bits from bits count1-1:0 of data into packPtr[intPosition], bits 31:bitPosition inclusive
		//            Copy 'count2' bits from bits count-1:count1 of data into packPtr[intEndPosition], bits endBitPosition:0

		int count1=32-bitPosition;
		int count2=count-count1;

		int tmpData=data<<bitPosition;
		*(packPtr+intPosition)=(*(packPtr+intPosition)&MASK_KEEP_LOWER_32[bitPosition])|tmpData;

		tmpData=SHIFTRIGHT_AND_SELECT_32(data, count1 ,count2);
		*(packPtr+endIntPosition)=(*(packPtr+endIntPosition)&MASK_LOWER_32[count2])|tmpData;
		}

	packer->position+=count;
}

*/

void initUnpacker(BitUnpacker *unpacker, u8 *ptr, int position)
{
	uintptr_t alignChecker=(uintptr_t)ptr;

	int cor=alignChecker & 0x3;
	ptr-=cor;
	position+=8*cor;

	unpacker->data=(u32 *)ptr;
	unpacker->position=position;
}

void seekUnpacker(BitUnpacker *unpacker, int position)
{
	unpacker->position+=position;
}


u32 unpackBits(BitUnpacker *unpacker, int count)
{
	if(count==0)
		return 0;

	u32 *packPtr=unpacker->data;

	int position=unpacker->position;
	int intPosition=POS2INT32(position);
	int bitPosition=POS2BIT32(position);

	int endPosition=position+count-1;
	int endIntPosition=POS2INT32(endPosition);

	u32 out;

	if(intPosition==endIntPosition)
		{
		// No-wrap case: Copy 'count' bits from packPtr[position], bits endBitPosition:bitPosition inclusive to bits count-1:0 of out

		out=SHIFTRIGHT_AND_SELECT_32(*(packPtr+intPosition), bitPosition, count);
		}
	else
		{
		// Wrap case: Copy 'count1' bits packPtr[intPosition], bits 31:bitPosition inclusive to bits count1-1:0 of out
		//            Copy 'count2' bits packPtr[intEndPosition], bits endBitPosition:0 to bits count-1:count1 of out

		int count1=32-bitPosition;
		int count2=count-count1;

		out=SHIFTRIGHT_AND_SELECT_32(*(packPtr+intPosition), bitPosition, count1)|SELECT_AND_SHIFTLEFT_32(*(packPtr+endIntPosition),count2,count1);
		}

	unpacker->position+=count;

	return out;
}



