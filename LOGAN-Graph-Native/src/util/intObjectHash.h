#ifndef __INTOBJECTHASH_H
#define __INTOBJECTHASH_H

typedef struct iohHashStr
{
	MemDispenser *disp;
	u32 entryCount;
	u32 bucketMask;

	u32 *keys;
	void **values;

} IohHash;



IohHash *iohInitMap(MemDispenser *disp, int capacityOrder);

void *iohGet(IohHash *map, s32 key);
void **iohGetAllValues(IohHash *map);

void iohPut(IohHash *map, s32 key, void *value);

#endif
