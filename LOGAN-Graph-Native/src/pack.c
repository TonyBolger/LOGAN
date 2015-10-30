/*
 * pack.c
 *
 *  Created on: Jul 2, 2014
 *      Author: tony
 */

#include <common.h>


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

int packSequence(char *seq, u8 *packedSeq, int length)
{
	int packedLength=MEM_ROUND_BYTE(length*2);

	memset(packedSeq,0,packedLength);

	int i=0;
	for(i=0;i<length;i++)
		{
		int pack=packChar(seq[i]) << (2 * (3 - (i & 0x3)));
		packedSeq[i/4]|=pack;
		}

	return packedLength;
}

