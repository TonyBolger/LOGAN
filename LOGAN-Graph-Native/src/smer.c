/*
 * smer.c
 *
 *  Created on: Nov 10, 2010
 *      Author: tony
 */

#include "common.h"

/* Various functions to handle sequence to smerID translation */

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

SmerId complementSmerId(SmerId id)
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

SmerId shuffleSmerIdRight(SmerId id, u8 base)
{
	return ((id << 2) & SMER_MASK) | (base & 0x3);
}

SmerId shuffleSmerIdLeft(SmerId id, u8 base)
{
	return (id >> 2) | ((base & 0x3L) << (SMER_BITS-2));
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



/* Other common functionality related to smer IDs */

int smerIdcompar(const void *ptr1, const void *ptr2)
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

int smerIdcomparL(const void *ptr1, const void *ptr2)
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


int u32compar(const void *ptr1, const void *ptr2)
{
	if(*(u32 *)ptr1 < *(u32 *)ptr2)
		return -1;

	if(*(u32 *)ptr1 > *(u32 *)ptr2)
		return 1;

	return 0;
}



void calculatePossibleSmers(u8 *data, s32 maxIndex, SmerId *smerIds,
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







