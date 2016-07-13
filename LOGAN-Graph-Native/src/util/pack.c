/*
 * pack.c
 *
 *  Created on: Jul 2, 2014
 *      Author: tony
 */

#include "common.h"


u32 packChar(u8 ch)
{
	switch(ch)
	{
		case 'A':
			return 0;
		case 'C':
			return 1;
		case 'G':
			return 2;
		case 'T':
			return 3;
	}
	return 0;
}

/*
void packSequence(char *seq, u8 *packedSeq, int length)
{
	int packedLength=PAD_2BITLENGTH_BYTE(length);
	memset(packedSeq,0,packedLength);

	int i=0;
	for(i=0;i<length;i++)
		{
		int pack=packChar(seq[i]) << (2 * (3 - (i & 0x3)));
		packedSeq[i/4]|=pack;
		}
}
*/

void packSequence(char *seq, u8 *packedSeq, int length)
{
	u8 data=0;

	while(length>0)
	{
		length-=4;

		u8 ch=*(seq++);
		data=(ch&0x6)<<5;

		ch=*(seq++);
		data|=(ch&0x6)<<3;

		ch=*(seq++);
		data|=(ch&0x6)<<1;

		ch=*(seq++);
		data|=(ch&0x6)>>1;

		*(packedSeq++)=data^((data&0xAA)>>1);
	}

}

/*

void packSequence(char *seq, u8 *packedSeq, int length)
{
	u8 data=0;

	while(length>3)
	{
		length-=4;
		data=0;
		switch(*(seq++))
			{
			case 'C':
				data=0x40;
				break;

			case 'G':
				data=0x80;
				break;

			case 'T':
				data=0xC0;
				break;
			}

		switch(*(seq++))
			{
			case 'C':
				data|=0x10;
				break;

			case 'G':
				data|=0x20;
				break;

			case 'T':
				data|=0x30;
				break;
			}

		switch(*(seq++))
			{
			case 'A':
				break;

			case 'C':
				data|=0x04;
				break;

			case 'G':
				data|=0x08;
				break;

			case 'T':
				data|=0x0C;
				break;
			}

		switch(*(seq++))
			{
			case 'C':
				data|=0x1;
				break;

			case 'G':
				data|=0x2;
				break;

			case 'T':
				data|=0x3;
				break;
			}

		*(packedSeq++)=data;
	}

	if(length)
		{
		data=0;
		while(length--)
			{
			data<<=2;
			switch(*(seq++))
				{
				case 'C':
					data=0x1;
					break;

				case 'G':
					data|=0x2;
					break;

				case 'T':
					data|=0x3;
					break;
				}
			}

		*(packedSeq++)=data;
		}


}

*/








char unpackChar(u32 pack)
{
	switch(pack)
	{
		case 0:
			return 'A';
		case 1:
			return 'C';
		case 2:
			return 'G';
		case 3:
			return 'T';
	}
	return 0;
}

void unpackSmer(SmerId smer, char *out)
{
	int pos=SMER_BASES-1;

	out[SMER_BASES]=0;

	for(int i=0;i<SMER_BASES;i++)
		{
		out[pos]=unpackChar(smer&0x3);
		smer>>=2;
		pos--;
		}
}


