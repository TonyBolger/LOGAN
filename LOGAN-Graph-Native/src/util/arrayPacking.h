#ifndef __ARRAY_PACKING_H
#define __ARRAY_PACKING_H


u8 *apPackArray(u8 *packPtr, u32 *unpackPtr, s32 bytesPerEntry, s32 entries);

u8 *apUnpackArray(u8 *packPtr, u32 *unpackPtr, s32 bytesPerEntry, s32 entries);

#endif
