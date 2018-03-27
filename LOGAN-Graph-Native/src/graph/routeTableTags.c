
#include "common.h"

/*

Tags store per-route auxiliary information, beyond the bases within the sequence. In principle, each route can have zero or more bytes associated with it.
Interpretation of the meaning of these bytes, and offering high-level indexing based on them is a future task.

Requirements:

    Tags can be expected to be rare, therefore the feature should have low storage / processing overhead if not used in a given node or route.

    Tag Tables can be expected to store little or no information about most routes (<1 byte), but potentially a lot of information for a small minority of routes.

Limitations:

	During insertion, tags can only be added to the final route (i.e. node) in a sequence, in general, since the association of specific routes may not
	be possible until the final node/route has been determined.

Implementation:

	Low storage / processing overhead suggests storing tags in an optional tag table, which is separate but associated to an existing routing table.

    Empty case requires a 'negative' flag, preferably per routing table, to indicate the absence of the optional tag table.
    	For the RouteTableArray case, this can be one additional byte per node.
    	For the RouteTableTree case, this can be two additional entries in the top-level node.

	Tag tables need to be have positional synchronization with routing tables.

	During insertion, only 'ending' edges can have tags (edge zero and dangling edges).

*/


u8 *rtgScanRouteTableTags(u8 *data)
{
	//u8 tagTableFlag=*data;

	return data+1;
}


static u8 *readRouteTableTagBuilderPackedData(RouteTableTagBuilder *builder, u8 *data)
{
	u8 header=0;

	if(data!=NULL)
		header=*(data++);

	if(header!=0)
		LOG(LOG_CRITICAL,"Non zero tag table header");

	builder->oldForwardEntryCount=0;
	builder->oldReverseEntryCount=0;

	builder->forwardPackedSize=0;
	builder->reversePackedSize=0;

	return data;
}

u8 *rtgInitRouteTableTagBuilder(RouteTableTagBuilder *builder, u8 *data, MemDispenser *disp)
{
	builder->disp=disp;

	data=readRouteTableTagBuilderPackedData(builder,data);

	builder->newForwardEntries=NULL;
	builder->newReverseEntries=NULL;

	builder->newForwardEntryCount=0;
	builder->newReverseEntryCount=0;

	return data;

}

static void rtgDumpRouteTableTagEntryArray(char *name, RouteTableTag *entries, u32 entryCount)
{
	LOGN(LOG_INFO,"%s: EntryCount %i",name, entryCount);

	for(int i=0;i<entryCount;i++)
		{
		LOGN(LOG_INFO,"  Position %i", entries[i].nodePosition);

		u8 *tagData=entries[i].tagData;

		if(tagData!=NULL)
			{
			u32 tagLength=*(tagData++);
			LOGN(LOG_INFO,"  Tag Length: %i", tagLength);

			if(tagLength>0)
				{
				LOGS(LOG_INFO,"  TagData: ");

				for(int j=0;j<tagLength;j++)
					LOGS(LOG_INFO,"%02x ",tagData[j]);

				LOGN(LOG_INFO,"");
				}
			}
		else
			LOGN(LOG_INFO,"  Tag Data is NULL");
		}
	LOGN(LOG_INFO,"");
}

void rtgDumpRouteTableTags(RouteTableTagBuilder *builder)
{
	LOG(LOG_INFO,"Dump RouteTableTagBuilder");

	LOG(LOG_INFO,"Old Data Size: %i",builder->oldDataSize);

	rtgDumpRouteTableTagEntryArray("Old Forward", builder->oldForwardEntries, builder->oldForwardEntryCount);
	rtgDumpRouteTableTagEntryArray("New Forward", builder->newForwardEntries, builder->newForwardEntryCount);

	LOG(LOG_INFO,"Forward Packed Size: %i",builder->forwardPackedSize);

	rtgDumpRouteTableTagEntryArray("Old Reverse", builder->oldReverseEntries, builder->oldReverseEntryCount);
	rtgDumpRouteTableTagEntryArray("New Reverse", builder->newReverseEntries, builder->newReverseEntryCount);

	LOG(LOG_INFO,"Reverse Packed Size: %i",builder->reversePackedSize);
}

s32 rtgGetRouteTableTagBuilderDirty(RouteTableTagBuilder *builder)
{
	return 0;
}

s32 rtgGetRouteTableTagBuilderPackedSize(RouteTableTagBuilder *builder)
{
	return 1+builder->forwardPackedSize+builder->reversePackedSize;
}

u8 *rtgWriteRouteTableTagBuilderPackedData(RouteTableTagBuilder *builder, u8 *data)
{
	if(builder->forwardPackedSize>0 || builder->reversePackedSize>0)
		{
		rtgDumpRouteTableTags(builder);
		LOG(LOG_CRITICAL,"TODO");
		}


	*data=0;
	return data+1;
}

void rtgMergeForwardRoutes(RouteTableTagBuilder *builder, int forwardTagCount, RoutePatch *patchPtr, RoutePatch *endPatchPtr)
{
	s32 totalTags=builder->oldForwardEntryCount+forwardTagCount;
	LOG(LOG_INFO, "rtgMergeForwardRoutes Tags %i OldTags %i Total %i",forwardTagCount, builder->oldForwardEntryCount, totalTags);

	if(totalTags==0)
		return;

	s32 packedSize=0;

	RouteTableTag *mergeTagPtr=dAlloc(builder->disp, sizeof(RouteTableTag)*totalTags);
	builder->newForwardEntries=mergeTagPtr;
	builder->newForwardEntryCount=totalTags;

	RouteTableTag *oldTagPtr=builder->oldForwardEntries;
	RouteTableTag *oldTagEndPtr=oldTagPtr+builder->oldForwardEntryCount;

	s32 oldTagNodePosition=(oldTagPtr<oldTagEndPtr)?oldTagPtr->nodePosition:INT_MAX/2;
	s32 positionShift=0;

	while(patchPtr<endPatchPtr)
		{
		s32 patchNodePosition=patchPtr->nodePosition;

		while(patchNodePosition>(oldTagNodePosition+positionShift)) // Merge earlier old tags, with position adjustment
			{
			mergeTagPtr->tagData=oldTagPtr->tagData;
			mergeTagPtr->nodePosition=oldTagPtr->nodePosition+positionShift;
			mergeTagPtr++;

			packedSize+=(4+1+*oldTagPtr->tagData); // 4 bytes offset, 1 byte tag length, plus tag length

			oldTagPtr++;
			oldTagNodePosition=(oldTagPtr<oldTagEndPtr)?oldTagPtr->nodePosition:INT_MAX/2;
			}

		// Merge new tag if not null
		if(patchPtr->tagData!=NULL)
			{
			mergeTagPtr->tagData=patchPtr->tagData;
			mergeTagPtr->nodePosition=patchPtr->nodePosition;
			mergeTagPtr++;

			packedSize+=(4+1+*patchPtr->tagData); // 4 bytes offset, 1 byte tag length, plus tag length
			}
		patchPtr++;
		positionShift++;
		}

	while(oldTagPtr<oldTagEndPtr) // Merge remaining old tags, with position adjustment
		{
		mergeTagPtr->tagData=oldTagPtr->tagData;
		mergeTagPtr->nodePosition=oldTagPtr->nodePosition+positionShift;
		mergeTagPtr++;

		packedSize+=(4+1+*oldTagPtr->tagData); // 4 bytes offset, 1 byte tag length, plus tag length

		oldTagPtr++;
		}

	builder->forwardPackedSize=packedSize+4;

}

void rtgMergeReverseRoutes(RouteTableTagBuilder *builder, s32 reverseTagCount, RoutePatch *patchPtr, RoutePatch *endPatchPtr)
{
	s32 totalTags=builder->oldReverseEntryCount+reverseTagCount;
	LOG(LOG_INFO, "rtgMergeReverseRoutes Tags %i OldTags %i Total %i",reverseTagCount, builder->oldReverseEntryCount, totalTags);

	if(totalTags==0)
		return;

	s32 packedSize=0;

	RouteTableTag *mergeTagPtr=dAlloc(builder->disp, sizeof(RouteTableTag)*totalTags);
	builder->newReverseEntries=mergeTagPtr;
	builder->newReverseEntryCount=totalTags;

	RouteTableTag *oldTagPtr=builder->oldReverseEntries;
	RouteTableTag *oldTagEndPtr=oldTagPtr+builder->oldReverseEntryCount;

	s32 oldTagNodePosition=(oldTagPtr<oldTagEndPtr)?oldTagPtr->nodePosition:INT_MAX/2;
	s32 positionShift=0;

	while(patchPtr<endPatchPtr)
		{
		s32 patchNodePosition=patchPtr->nodePosition;

		while(patchNodePosition>(oldTagNodePosition+positionShift)) // Merge earlier old tags, with position adjustment
			{
			mergeTagPtr->tagData=oldTagPtr->tagData;
			mergeTagPtr->nodePosition=oldTagPtr->nodePosition+positionShift;
			mergeTagPtr++;

			packedSize+=(4+1+*oldTagPtr->tagData); // 4 bytes offset, 1 byte tag length, plus tag length

			oldTagPtr++;
			oldTagNodePosition=(oldTagPtr<oldTagEndPtr)?oldTagPtr->nodePosition:INT_MAX/2;
			}

		// Merge new tag if not null
		if(patchPtr->tagData!=NULL)
			{
			mergeTagPtr->tagData=patchPtr->tagData;
			mergeTagPtr->nodePosition=patchPtr->nodePosition;
			mergeTagPtr++;

			packedSize+=(4+1+*patchPtr->tagData); // 4 bytes offset, 1 byte tag length, plus tag length
			}
		patchPtr++;
		positionShift++;
		}

	while(oldTagPtr<oldTagEndPtr) // Merge remaining old tags, with position adjustment
		{
		mergeTagPtr->tagData=oldTagPtr->tagData;
		mergeTagPtr->nodePosition=oldTagPtr->nodePosition+positionShift;
		mergeTagPtr++;

		packedSize+=(4+1+*oldTagPtr->tagData); // 4 bytes offset, 1 byte tag length, plus tag length

		oldTagPtr++;
		}

	builder->reversePackedSize=packedSize+4; // 4 byte total size

}



