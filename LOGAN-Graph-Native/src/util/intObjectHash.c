#include "common.h"

#define KEY_DUMMY 0xFFFFFFFF
#define HASH(i) (((i)+7)*43)

static void allocKeyValueBuckets(u32 **keyPtr, void ***valuePtr, s32 buckets, MemDispenser *disp)
{
	u32 *keys=dAlloc(disp, sizeof(u32)*buckets);
	void **values=dAlloc(disp, sizeof(void *)*buckets);

	for(int i=0;i<buckets;i++)
		{
		keys[i]=KEY_DUMMY;
		values[i]=NULL;
		}

	*keyPtr=keys;
	*valuePtr=values;
}

IohHash *iohInitMap(MemDispenser *disp, int capacityOrder)
{
	IohHash *map=dAlloc(disp, sizeof(IohHash));
	map->disp=disp;
	map->entryCount=0;

	if(capacityOrder<3)
		capacityOrder=3;

	if(capacityOrder>12)
		capacityOrder=12;

	int buckets=1<<capacityOrder;

	map->bucketMask=buckets-1;

	allocKeyValueBuckets(&(map->keys), &(map->values), buckets, disp);

	return map;
}

static s32 scanForKey(u32 *keys, u32 key, u32 hash, u32 mask)
{
	s32 position = hash & mask;
	u32 scanCount = 0;

	u32 tmp=keys[position];

	while (tmp != KEY_DUMMY)
		{
		if (tmp == key)
			return position;

		position++;
		scanCount++;

		if (position > mask)
			{
			position = 0;

			if (scanCount > 200)
				{
				LOG(LOG_CRITICAL,"Filled hashmap: count %i from %i for %x (got %x)",scanCount,(hash & mask), key, tmp);
				}
			}

		tmp=keys[position];
		}

	if (scanCount > 60) {
		LOG(LOG_INFO,"Scan count %i from %i for %i",scanCount,(hash & mask), key);
	}

	return position;
}


void *iohGet(IohHash *map, s32 key)
{
	u32 hash=HASH(key);
	u32 pos=scanForKey(map->keys, key, hash, map->bucketMask);

	if(map->keys[pos]==key)
		return map->values[pos];

	return NULL;
}


void **iohGetAllValues(IohHash *map)
{
	void **out=dAlloc(map->disp, sizeof(void *)*map->entryCount);

	int index=0;

	for(int i=0;i<=map->bucketMask;i++)
		{
		int key=map->keys[i];
		if(key!=KEY_DUMMY)
			out[index++]=map->values[i];
		}

	return out;
}

int iohGetNext(IohHash *map, int startIndex, int *keyPtr, void **valuePtr)
{
	for(int i=startIndex;i<=map->bucketMask;i++)
		{
		int key=map->keys[i];
		if(key!=KEY_DUMMY)
			{
			*keyPtr=key;
			*valuePtr=map->values[i];
			return i+1;
			}
		}

	return 0;
}



static void resize(IohHash *map)
{
	//LOG(LOG_INFO,"Hash Resize");

	u32 oldSize=map->bucketMask+1;
	u32 newSize=oldSize*2;
	u32 newBucketMask=newSize-1;

	u32 *newKeys;
	void **newValues;

	allocKeyValueBuckets(&newKeys, &newValues, newSize, map->disp);

	for(int i=0;i<oldSize;i++)
		{
		int key=map->keys[i];
		if(key!=KEY_DUMMY)
			{
			u32 hash=HASH(key);
			s32 newPos=scanForKey(newKeys, map->keys[i], hash, newBucketMask);

			if(newKeys[newPos]==KEY_DUMMY)
				{
				newKeys[newPos]=key;
				newValues[newPos]=map->values[i];
				}
			else
				{
				LOG(LOG_CRITICAL,"Unable to set %i to %p",key,map->values[i]);
				}
			}
		}

	map->bucketMask=newBucketMask;
	map->keys=newKeys;
	map->values=newValues;
}


void iohPut(IohHash *map, s32 key, void *value)
{
	u32 hash=HASH(key);
	s32 pos=scanForKey(map->keys, key, hash, map->bucketMask);

	if(map->keys[pos]==key) // Overwrite
		map->values[pos]=value;
	else if(map->keys[pos]==KEY_DUMMY)
		{
		map->keys[pos]=key;
		map->values[pos]=value;

		map->entryCount++;

		if((map->entryCount*2)>map->bucketMask)
			resize(map);
		}

	else
		LOG(LOG_CRITICAL,"Unable to set %i to %p",key,value);

}





