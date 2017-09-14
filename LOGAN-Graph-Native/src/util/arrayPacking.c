#include "common.h"


u8 *apPackArray(u8 *packPtr, u32 *unpackPtr, s32 bytesPerEntry, s32 entries)
{
	switch(bytesPerEntry)
		{
		case 1:
			for(int i=0;i<entries;i++)
				*(packPtr++)=(*(unpackPtr++));
			break;

		case 2:
			for(int i=0;i<entries;i++)
				{
				*((u16 *)packPtr)=*(unpackPtr++);
				packPtr+=2;
				}
			break;

		case 3:
			for(int i=0;i<entries;i++)
				{
				u32 data=*(unpackPtr++);
				*((u16 *)(packPtr))=(u16)(data);
				*(packPtr+2)=(u8)(data>>16);
				packPtr+=3;
				}
			break;

		case 4:
			memcpy(packPtr, unpackPtr, bytesPerEntry*entries);
			packPtr+=bytesPerEntry*entries;
			break;

		default:
			LOG(LOG_CRITICAL,"Invalid bytesPerEntry size %i for array packing",bytesPerEntry);
		}

	return packPtr;
}

u8 *apUnpackArray(u8 *packPtr, u32 *unpackPtr, s32 bytesPerEntry, s32 entries)
{
	switch(bytesPerEntry)
		{
		case 1:
			for(int i=0;i<entries;i++)
				*(unpackPtr++)=*(packPtr++);
			break;

		case 2:
			for(int i=0;i<entries;i++)
				{
				*(unpackPtr++)=*((u16 *)packPtr);
				packPtr+=2;
				}
			break;

		case 3:
			for(int i=0;i<entries;i++)
				{
				u32 data1=*((u16 *)(packPtr));
				u32 data2=*(packPtr+2);
				*(unpackPtr++)=data1 | (data2<<16);
				packPtr+=3;
				}
			break;

		case 4:
			memcpy(unpackPtr, packPtr, bytesPerEntry*entries);
			packPtr+=bytesPerEntry*entries;
			break;

		default:
			LOG(LOG_CRITICAL,"Invalid bytesPerEntry size %i for array packing",bytesPerEntry);
		}

	return packPtr;


	return packPtr;
}
