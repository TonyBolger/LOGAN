
#include "common.h"


u32 nextPowerOf2_32(u32 in)
{
	in--;
	in|=in>>1;
	in|=in>>2;
	in|=in>>4;
	in|=in>>8;
	in|=in>>16;
	in++;

	return in;
}



u64 nextPowerOf2_64(u64 in)
{
	in--;
	in|=in>>1;
	in|=in>>2;
	in|=in>>4;
	in|=in>>8;
	in|=in>>16;
	in|=in>>32;
	in++;

	return in;
}
