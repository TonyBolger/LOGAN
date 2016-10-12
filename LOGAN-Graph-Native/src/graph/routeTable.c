
#include "common.h"


// Unpacked format stores arrays of trees (b-tree like), each sharing a common upstream sequence. May require tweaking depending on max block size and GC
// Unpack format: 1 1 1 ? B B B B  B B B B B B B B
//                F F F F F F F F  R R R R R R R R
//				  Bptr* (node block pointers)
//                Fptr* (forward root nodes)
//                Rptr* (reverse root nodes)

// Block:		  Z Z Z Z Z Z Z Z  Z Z Z Z Z Z Z Z (block size in nodes)
//                Z Z Z Z Z Z Z Z  Z Z Z Z Z Z Z Z (block size in nodes)
//				  A A A A A A A A  A A A A A A A A (allocated node count)
//				  A A A A A A A A  A A A A A A A A (allocated node count)
//                Nodes*
//

// Root/Branch node: (type, usage count?)

//                U U U U U U U U  U U U U U U U U (upstream tail)
//                U U U U U U U U  U U U U U U U U (upstream tail)

//				  C C C C C C C C  C C C C C C C C (child ptrs)
//				  C C C C C C C C  C C C C C C C C
//				  C C C C C C C C  C C C C C C C C
//				  C C C C C C C C  C C C C C C C C

//                W W W W W W W W  W W W W W W W W (child width - 64bit?)
//                W W W W W W W W  W W W W W W W W



// Leaf node: 	  (type, usage count?)

//                D D D D D D D D  D D D D D D D D (downstream tails)
//				  D D D D D D D D  D D D D D D D D

//				  W W W W W W W W  W W W W W W W W (widths)
//				  W W W W W W W W  W W W W W W W W



// PackedArray format stores arrays of pointers, referencing a block of route entries grouped by upstream sequence. The blocks consist of packed entries
// PckArr format: 1 1 0 W W W W W  P P P P S S S S
//                F F F F F F F F  R R R R R R R R
//                Fptr*
//                Rptr*

// PA Block:      E E E E E E E E  E E E E E E E E (entry count)
//                E E E E E E E E  E E E E E E E E (entry count)
//			 	  U*, (D*, W*)*


u8 *scanRouteTable(u8 *data)
{
	return rtaScanRouteTableArray(data);
}


u8 *initRouteTableBuilder(RouteTableBuilder *builder, u8 *data, MemDispenser *disp)
{
	builder->disp=disp;

	RouteTableArrayBuilder *arrayBuilder=dAlloc(disp, sizeof(RouteTableArrayBuilder));
	builder->arrayBuilder=arrayBuilder;

	return rtaInitRouteTableArrayBuilder(arrayBuilder,data,disp);
}

s32 getRouteTableBuilderDirty(RouteTableBuilder *builder)
{
	return rtaGetRouteTableBuilderDirty(builder->arrayBuilder);
}

s32 getRouteTableBuilderPackedSize(RouteTableBuilder *builder)
{
	return rtaGetRouteTableBuilderPackedSize(builder->arrayBuilder);
}

void dumpRoutingTable(RouteTableBuilder *builder)
{
	rtaDumpRoutingTable(builder->arrayBuilder);
}

u8 *writeRouteTableBuilderPackedData(RouteTableBuilder *builder, u8 *data)
{
	return rtaWriteRouteTableBuilderPackedData(builder->arrayBuilder, data);
}



void mergeRoutes(RouteTableBuilder *builder,
 		RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches, s32 forwardRoutePatchCount, s32 reverseRoutePatchCount,
		s32 maxNewPrefix, s32 maxNewSuffix, RoutingReadData **orderedDispatches, MemDispenser *disp)
{
	rtaMergeRoutes(builder->arrayBuilder, forwardRoutePatches, reverseRoutePatches, forwardRoutePatchCount, reverseRoutePatchCount,
		maxNewPrefix, maxNewSuffix, orderedDispatches, disp);
}



void unpackRouteTableForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp)
{
	rtaUnpackRouteTableForSmerLinked(smerLinked, data, disp);
}







