
#include "common.h"


// Idea for Unpacked format: store arrays of trees (b-tree like), each sharing a common upstream sequence. May require tweaking depending on max block size and GC
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



u8 *rttScanRouteTableTree(u8 *data)
{
	if(data!=NULL)
		{
		data+=sizeof(RouteTableSmallRoot);
		return data;
		}

	return NULL;
}

u8 *rttInitRouteTableTreeBuilder(RouteTableTreeBuilder *builder, u8 *data, MemDispenser *disp)
{
	builder->disp=disp;

	RouteTableSmallRoot *root=(RouteTableSmallRoot *)data;

	builder->rootPtr=root;
	data+=sizeof(RouteTableSmallRoot);

	builder->prefixBuilder=dAlloc(disp, sizeof(SeqTailBuilder));
	builder->suffixBuilder=dAlloc(disp, sizeof(SeqTailBuilder));
	builder->nestedBuilder=dAlloc(disp, sizeof(RouteTableArrayBuilder));

	initSeqTailBuilder(builder->prefixBuilder, root->prefixData, disp);
	initSeqTailBuilder(builder->suffixBuilder, root->suffixData, disp);
	rtaInitRouteTableArrayBuilder(builder->nestedBuilder, root->forwardRouteData[0], disp);

	return data;
}

void rttUpgradeToRouteTableTreeBuilder(RouteTableTreeBuilder *builder,
		SeqTailBuilder *prefixBuilder, SeqTailBuilder *suffixBuilder, RouteTableArrayBuilder *arrayBuilder, MemDispenser *disp)
{
	builder->prefixBuilder=prefixBuilder;
	builder->suffixBuilder=suffixBuilder;
	builder->nestedBuilder=arrayBuilder;

	builder->rootPtr=NULL;
}


void rttDumpRoutingTable(RouteTableTreeBuilder *builder)
{
	LOG(LOG_CRITICAL,"Not implemented");
}

s32 rttGetRouteTableTreeBuilderDirty(RouteTableTreeBuilder *builder)
{
	if(builder->rootPtr==NULL)
		return 1;

	return rtaGetRouteTableArrayBuilderDirty(builder->nestedBuilder);
}

s32 rttGetRouteTableTreeBuilderPackedSize(RouteTableTreeBuilder *builder)
{
	return sizeof(RouteTableSmallRoot);
}

u8 *rttWriteRouteTableTreeBuilderPackedData(RouteTableTreeBuilder *builder, u8 *data)
{
	return rtaWriteRouteTableArrayBuilderPackedData(builder->nestedBuilder, data);
}

void rttMergeRoutes(RouteTableTreeBuilder *builder,
		RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches, s32 forwardRoutePatchCount, s32 reverseRoutePatchCount,
		s32 maxNewPrefix, s32 maxNewSuffix, RoutingReadData **orderedDispatches, MemDispenser *disp)
{
	rtaMergeRoutes(builder->nestedBuilder, forwardRoutePatches, reverseRoutePatches, forwardRoutePatchCount, reverseRoutePatchCount,
		maxNewPrefix, maxNewSuffix, orderedDispatches, disp);
}

void rttUnpackRouteTableForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp)
{
	LOG(LOG_CRITICAL,"Not implemented");
}

