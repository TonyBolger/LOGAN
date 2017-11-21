/*
 * bloom.c
 *
 *  Created on: Jun 6, 2014
 *      Author: tony
 */


#include "common.h"

/* Not sure if mid(2^x,2^(x+1)) primes make sense here */

const u32 PRIMES[]={12289, 24593, 49157, 98317,
		196613, 393241, 786433, 1572869,
	    3145739, 6291469, 12582917, 25165843,
	    50331653, 100663319, 201326611, 402653189};

const u32 OFFSETS[]={801331085, 1760893329, 3608146733, 3782855011,
		2848152293, 2012665461, 1538164031, 2900910286,
		2733125612, 3094107749, 635480902, 3719745339,
		2774511828, 383857110, 1036929602, 2619910889};

#define OFFSET_AND_MULT(V,I) (((V)*PRIMES[I])+OFFSETS[I])



void initBloom(Bloom *bloom, u32 size, u32 hashes, u32 bitsPerEntry)
{
	if(size<64)
		size=64;

	if(hashes>16)
		hashes=16;

	u32 pSize=nextPowerOf2_32(size*bitsPerEntry);

	bloom->data=G_ALLOC_C(pSize>>3, MEMTRACKID_BLOOM);
	bloom->mask=pSize-1;
	bloom->hashes=hashes;

	//LOG(LOG_INFO,"Bloom: Allocated with %i bytes and %i hashes",pSize,hashes);
}


void freeBloom(Bloom *bloom)
{
	G_FREE(bloom->data, (bloom->mask+1)>>3, MEMTRACKID_BLOOM);
	bloom->data=NULL;
}


void setBloom(Bloom *bloom, u64 value)
{
	u64 *dataPtr=bloom->data;
	u32 hashMask=bloom->mask;
	u32 hashes=bloom->hashes;

	int i;
	for(i=0;i<hashes;i++)
		{
		u32 hash=OFFSET_AND_MULT(value, i)&hashMask;
		dataPtr[(hash>>6)]|=1<<(hash&0x3F);
		}
}

int testBloom(Bloom *bloom, u64 value)
{
	u64 *dataPtr=bloom->data;
	u32 hashMask=bloom->mask;
	u32 hashes=bloom->hashes;

	int i;
	for(i=0;i<hashes;i++)
		{
		u32 hash=OFFSET_AND_MULT(value, i)&hashMask;

		if(!(dataPtr[(hash>>6)]&(1<<(hash&0x3F))))
			return 0;
		}

	return 1;
}
