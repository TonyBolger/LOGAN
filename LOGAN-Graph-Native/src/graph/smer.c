/*
 * smer.c
 *
 *  Created on: Nov 10, 2010
 *      Author: tony
 */

#include "../common.h"



/* Comparisons related to smer IDs */

int smerIdCompar(const void *ptr1, const void *ptr2)
{
	return ( (*(SmerId *)ptr1) > (*(SmerId *)ptr2) ) - ( (*(SmerId *)ptr1) < (*(SmerId *)ptr2) ) ;

	/*
	if(*(SmerId *)ptr1 < *(SmerId *)ptr2)
		return -1;

	if(*(SmerId *)ptr1 > *(SmerId *)ptr2)
			return 1;

	return 0;
	*/
}

// Diagnostic version
int smerIdComparL(const void *ptr1, const void *ptr2)
{
	SmerId id1=*(SmerId *)ptr1;
	SmerId id2=*(SmerId *)ptr2;

	LOG(LOG_INFO, "Compare %08x to %08x",id1,id2);

	if(*(SmerId *)ptr1 < *(SmerId *)ptr2)
		return -1;

	if(*(SmerId *)ptr1 > *(SmerId *)ptr2)
		return 1;

	return 0;
}

int smerEntryCompar(const void *ptr1, const void *ptr2)
{
	return ( (*(SmerEntry *)ptr1) > (*(SmerEntry *)ptr2) ) - ( (*(SmerEntry *)ptr1) < (*(SmerEntry *)ptr2) ) ;

}

/*
int u32Compar(const void *ptr1, const void *ptr2)
{
	if(*(u32 *)ptr1 < *(u32 *)ptr2)
		return -1;

	if(*(u32 *)ptr1 > *(u32 *)ptr2)
		return 1;

	return 0;
}
*/


/* Various functions to handle sequence to smerID translation */

/*

static SmerId shuffleSmerIdRight(SmerId id, u8 base)
{
	return ((id << 2) & SMER_MASK) | (base & 0x3);
}

static SmerId shuffleSmerIdLeft(SmerId id, u8 base)
{
	return (id >> 2) | ((base & 0x3L) << (SMER_BITS-2));
}
*/

/*
static SmerId convertDataToSmerId(u8 *data, u32 offset) {
	SmerId id = 0;
	int i;

	for (i = 0; i < SMER_BASES; i++) {
		u8 base;
		u8 *dataPtr = data + (offset >> 2);

		base = (*dataPtr)>>((3-(offset & 0x3))<<1);
		id = (id << 2) | (base & 0x3);

		offset++;
	}

	return id & SMER_MASK;
}

static SmerId complementSmerId(SmerId id)
{
	if(SMER_CBITS==32)
		{
		id = ((id >> 2) & 0x33333333) | ((id << 2) & 0xcccccccc);
		id = ((id >> 4) & 0x0f0f0f0f) | ((id << 4) & 0xf0f0f0f0);
		id = ((id >> 8) & 0x00ff00ff) | ((id << 8) & 0xff00ff00);
		id = ((id >> 16) & 0x0000ffff) | ((id << 16) & 0xffff0000);

		id = COMP_BASE_HEX(id);
		}
	else if(SMER_CBITS==64)
		{
		id = ((id >> 2) & 0x3333333333333333L) | ((id << 2) & 0xccccccccccccccccL);
		id = ((id >> 4) & 0x0f0f0f0f0f0f0f0fL) | ((id << 4) & 0xf0f0f0f0f0f0f0f0L);
		id = ((id >> 8) & 0x00ff00ff00ff00ffL) | ((id << 8) & 0xff00ff00ff00ff00L);
		id = ((id >> 16)& 0x0000ffff0000ffffL) | ((id << 16)& 0xffff0000ffff0000L);
		id = ((id >> 32)& 0x00000000ffffffffL) | ((id << 32)& 0xffffffff00000000L);

		id = COMP_BASE_32(id);
		}

	id>>=(SMER_CBITS-SMER_BITS);

	return id;
}





static SmerId shuffleSmerIdWithOffset(SmerId id, u8 *data, u32 offset) {
	u8 base;
	u8 *dataPtr = data + (offset >> 2);

	base = (*dataPtr)>>((3-(offset & 0x3))<<1);
	id = ((id << 2) & SMER_MASK) | (base & 0x3);

	return id;
}

static SmerId shuffleComplementSmerIdWithOffset(SmerId id, u8 *data, u32 offset) {
	u8 base;
	u8 *dataPtr = data + (offset >> 2);

	base = COMP_BASE_SINGLE((*dataPtr)>>((3-(offset & 0x3))<<1));
	id = (id >> 2) | ((base & 0x3L) << (SMER_BITS-2));

	return id;
}




void calculatePossibleSmers2(u8 *data, s32 maxIndex, SmerId *smerIds,
		u32 *compFlags) {
	int i;

	SmerId smerId=0;
	SmerId cSmerId=0;

	for (i = 0; i <= maxIndex; i++) {
		if (i == 0) {
			smerId = convertDataToSmerId(data, 0);
			cSmerId = complementSmerId(smerId);
		} else {
			int offset=i+(SMER_BASES-1);
			smerId = shuffleSmerIdWithOffset(smerId, data, offset);
			cSmerId = shuffleComplementSmerIdWithOffset(cSmerId, data, offset);
		}

		if (smerId <= cSmerId) {
			smerIds[i] = smerId;
			if (compFlags != NULL)
				compFlags[i] = 0;
		} else {
			smerIds[i] = cSmerId;
			if (compFlags != NULL)
				compFlags[i] = 1;
		}

	}
}

*/



static u64 reverseWithinBytes(u64 in)
{
	in = ((in >> 2) & 0x3333333333333333L) | ((in << 2) & 0xccccccccccccccccL);
	in = ((in >> 4) & 0x0f0f0f0f0f0f0f0fL) | ((in << 4) & 0xf0f0f0f0f0f0f0f0L);

	return in;
}


void calculatePossibleSmers(u8 *data, s32 maxIndex, SmerId *smerIds)
{
	int i=0;

	u64 rmerGen=*((SmerId *)data);
	u64 fmerGen= __builtin_bswap64(rmerGen);

	data+=6; // First 24bp are now consumed

	rmerGen=COMP_BASE_32(reverseWithinBytes(rmerGen));

	fmerGen >>= 16;
	rmerGen <<= 4;

	maxIndex++;

	while(maxIndex>0)
		{
		SmerId fmer, rmer;

		// Offset 0
		fmer=(fmerGen>>2)&SMER_MASK;
		rmer=(rmerGen>>4)&SMER_MASK;
		if(fmer<rmer)
			{
			smerIds[i] = fmer;
			}
		else
			{
			smerIds[i] = rmer;
			}
		i++;

		if(!--maxIndex)
			break;

		// Offset 1
		fmer=fmerGen&SMER_MASK;
		rmer=(rmerGen>>6)&SMER_MASK;
		if(fmer<rmer)
			{
			smerIds[i] = fmer;
			}
		else
			{
			smerIds[i] = rmer;
			}
		i++;

		if(!--maxIndex)
			break;

		// Load next 4bp
		u64 tmp=*(data++);
		fmerGen=(fmerGen<<8) | tmp;
		rmerGen=(rmerGen>>8) | (COMP_BASE_QUAD(reverseWithinBytes(tmp)) << (SMER_BITS-2));

		// Offset 2
		fmer=(fmerGen>>6)&SMER_MASK;
		rmer=(rmerGen)&SMER_MASK;
		if(fmer<rmer)
			{
			smerIds[i] = fmer;
			}
		else
			{
			smerIds[i] = rmer;
			}
		i++;

		if(!--maxIndex)
			break;

		// Offset 3
		fmer=(fmerGen>>4)&SMER_MASK;
		rmer=(rmerGen>>2)&SMER_MASK;
		if(fmer<rmer)
			{
			smerIds[i] = fmer;
			}
		else
			{
			smerIds[i] = rmer;
			}
		i++;

		maxIndex--;
		}


}



void calculatePossibleSmersAndCompSmers(u8 *data, s32 maxIndex, SmerId *smerAndCompIds)
{
	int i=0;

	u64 rmerGen=*((SmerId *)data);
	u64 fmerGen= __builtin_bswap64(rmerGen);

	data+=6; // First 24bp are now consumed

	rmerGen=COMP_BASE_32(reverseWithinBytes(rmerGen));

	fmerGen >>= 16;
	rmerGen <<= 4;

	maxIndex++;

	while(maxIndex>0)
		{
		// Offset 0
		smerAndCompIds[i++]=(fmerGen>>2)&SMER_MASK;
		smerAndCompIds[i++]=(rmerGen>>4)&SMER_MASK;

		if(!--maxIndex)
			break;

		// Offset 1
		smerAndCompIds[i++]=fmerGen&SMER_MASK;
		smerAndCompIds[i++]=(rmerGen>>6)&SMER_MASK;

		if(!--maxIndex)
			break;

		// Load next 4bp
		u64 tmp=*(data++);
		fmerGen=(fmerGen<<8) | tmp;
		rmerGen=(rmerGen>>8) | (COMP_BASE_QUAD(reverseWithinBytes(tmp)) << (SMER_BITS-2));

		// Offset 2
		smerAndCompIds[i++]=(fmerGen>>6)&SMER_MASK;
		smerAndCompIds[i++]=(rmerGen)&SMER_MASK;

		if(!--maxIndex)
			break;

		// Offset 3
		smerAndCompIds[i++]=(fmerGen>>4)&SMER_MASK;
		smerAndCompIds[i++]=(rmerGen>>2)&SMER_MASK;

		maxIndex--;
		}


}







// Aims to produce the maximum entropy in the lower 16 bits from the input 32 bits for primary hash.
// Mixed into upper 32-bits using shift/mix for slice hash.

//static const u64 HASH_MAGIC = 1379830596609ULL; // (3*97*4229*1121231 - use with 28bit shift)
//static const u64 HASH_MAGIC = 1379829564673ULL; // (7*19*1279*8111539 - use with 28bit shift)
static const u64 HASH_MAGIC = 1560361322401ULL; // (7*8861*25156163 - use with 28bit shift)

u64 hashForSmer(SmerEntry entry) {
	u64 hash = (((u64)entry) * HASH_MAGIC);
	hash ^= (hash >> 28) ^ (hash << 17);

	return hash;
}

u32 sliceForSmer(SmerId smer, u64 hash)
{
	return ((smer ^ hash) >> 32) & SMER_SLICE_MASK;
}


SmerId recoverSmerId(u32 sliceNum, SmerEntry entry)
{
	u64 sliceTmp=((u64)sliceNum)<<32;

	u64 hash=hashForSmer(entry);
	u64 comp = hash & ((u64)SMER_SLICE_MASK << 32);

	return (sliceTmp ^ comp) | entry;
}




