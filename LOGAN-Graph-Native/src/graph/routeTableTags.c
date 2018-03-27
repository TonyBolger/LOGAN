
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

#define ROUTETAG_HEADER_FWD_NONE 0
#define ROUTETAG_HEADER_FWD 0x1

#define ROUTETAG_HEADER_REV_NONE 0
#define ROUTETAG_HEADER_REV 0x10

static void dumpRouteTableTagEntryArray(char *name, RouteTableTag *entries, u32 entryCount)
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

	dumpRouteTableTagEntryArray("Old Forward", builder->oldForwardEntries, builder->oldForwardEntryCount);
	dumpRouteTableTagEntryArray("New Forward", builder->newForwardEntries, builder->newForwardEntryCount);

	LOG(LOG_INFO,"Forward Packed Size: %i",builder->forwardPackedSize);

	dumpRouteTableTagEntryArray("Old Reverse", builder->oldReverseEntries, builder->oldReverseEntryCount);
	dumpRouteTableTagEntryArray("New Reverse", builder->newReverseEntries, builder->newReverseEntryCount);

	LOG(LOG_INFO,"Reverse Packed Size: %i",builder->reversePackedSize);
}

u8 *rtgScanRouteTableTags(u8 *data)
{
	u8 header=0;

	if(data!=NULL)
		header=*(data++);

	if(header & ROUTETAG_HEADER_FWD)
		{
		u32 *sizePtr=(u32 *)data;
		u32 size=*sizePtr;
		data+=4+size;
		}

	if(header & ROUTETAG_HEADER_REV)
		{
		u32 *sizePtr=(u32 *)data;
		u32 size=*sizePtr;
		data+=4+size;
		}

	return data;

}


static void readRouteTableTagArray(RouteTableTagBuilder *builder, RouteTableTag **entriesPtr, u32 *entryCountPtr, u8 *dataPtr, u8 *dataEndPtr)
{
	u8 *scanPtr=dataPtr;
	u32 entryCount=0;

	while(scanPtr<dataEndPtr)
		{
		scanPtr+=4; // skip node position
		u8 tagLength=*scanPtr;
		scanPtr+=tagLength+1;
		entryCount++;
		}

	if(scanPtr!=dataEndPtr)
		LOG(LOG_CRITICAL,"Tag Array end did not match expected. Actual %p Expected %p",scanPtr, dataEndPtr);

	RouteTableTag *entries=dAlloc(builder->disp, sizeof(RouteTableTag)*entryCount);
	*entriesPtr=entries;
	*entryCountPtr=entryCount;

	while(dataPtr<dataEndPtr)
		{
		u32 *positionPtr=(u32 *)dataPtr;
		entries->nodePosition=*positionPtr;
		dataPtr+=4;

		u8 tagLength=*dataPtr;
		u8 *tagData=dAlloc(builder->disp, tagLength+1);
		memcpy(tagData, dataPtr, tagLength+1);
		entries->tagData=tagData;

		dataPtr+=tagLength+1;
		entries++;
		}

	if(dataPtr!=dataEndPtr)
		LOG(LOG_CRITICAL,"Tag Array end did not match expected. Actual %p Expected %p",dataPtr, dataEndPtr);


}

static u8 *readRouteTableTagBuilderPackedData(RouteTableTagBuilder *builder, u8 *data)
{
	u8 header=0;

	if(data!=NULL)
		header=*(data++);

	//u8 *dataOrig=data;

	if(header & ROUTETAG_HEADER_FWD)
		{
		u32 *sizePtr=(u32 *)data;
		u32 size=*sizePtr;
		data+=4;

		u8 *dataEnd=data+size;
		readRouteTableTagArray(builder, &(builder->oldForwardEntries), &(builder->oldForwardEntryCount), data, dataEnd);
		data=dataEnd;

		builder->forwardPackedSize=size+4;
		}
	else
		{
		builder->oldForwardEntryCount=0;
		builder->forwardPackedSize=0;
		}

	if(header & ROUTETAG_HEADER_REV)
		{
		u32 *sizePtr=(u32 *)data;
		u32 size=*sizePtr;
		data+=4;

		u8 *dataEnd=data+size;
		readRouteTableTagArray(builder, &(builder->oldReverseEntries), &(builder->oldReverseEntryCount), data, dataEnd);
		data=dataEnd;

		builder->reversePackedSize=size+4;
		}
	else
		{
		builder->oldReverseEntryCount=0;
		builder->reversePackedSize=0;
		}

	/*
	int size=data-dataOrig;
	if(size>100)
		{
		LOG(LOG_INFO,"readRouteTableTagBuilderPackedData: Parsed %i",size);
		rtgDumpRouteTableTags(builder);
		}
*/

	return data;
}

u8 *rtgInitRouteTableTagBuilder(RouteTableTagBuilder *builder, u8 *data, MemDispenser *disp)
{
	builder->disp=disp;

	builder->newForwardEntries=NULL;
	builder->newReverseEntries=NULL;

	builder->newForwardEntryCount=0;
	builder->newReverseEntryCount=0;

	data=readRouteTableTagBuilderPackedData(builder,data);

	return data;

}





s32 rtgGetRouteTableTagBuilderDirty(RouteTableTagBuilder *builder)
{
	return (builder->newForwardEntryCount!=0) || (builder->newReverseEntryCount!=0);
}

s32 rtgGetRouteTableTagBuilderPackedSize(RouteTableTagBuilder *builder)
{
	return 1+builder->forwardPackedSize+builder->reversePackedSize;
}

static u8 *writeRouteTableTagArray(RouteTableTag *tagEntries, u32 entryCount, u8 *data)
{
	for(int i=0;i<entryCount;i++)
		{
		u32 *positionPtr=(u32 *)data;
		*positionPtr=tagEntries[i].nodePosition;
		data+=4;

		u8 *tagData=tagEntries[i].tagData;

		u8 tagLength=*(tagData);
		memcpy(data, tagData, tagLength+1);
		data+=tagLength+1;
		}

	return data;
}

u8 *rtgWriteRouteTableTagBuilderPackedData(RouteTableTagBuilder *builder, u8 *data)
{
	u8 fwdHeader=(builder->forwardPackedSize>0)?ROUTETAG_HEADER_FWD:0;
	u8 revHeader=(builder->reversePackedSize>0)?ROUTETAG_HEADER_REV:0;

	*(data++)=fwdHeader | revHeader;

	u8 *expectedData=data+builder->forwardPackedSize;

	if(builder->forwardPackedSize>0)
		{
		u32 *sizePtr=(u32 *)data;
		*sizePtr=builder->forwardPackedSize-4;
		data+=4;

		if(builder->newForwardEntries!=NULL)
			data=writeRouteTableTagArray(builder->newForwardEntries, builder->newForwardEntryCount, data);
		else
			data=writeRouteTableTagArray(builder->oldForwardEntries, builder->oldForwardEntryCount, data);
		}

	if(data!=expectedData)
		LOG(LOG_CRITICAL,"Forward tag data did not end at expected location Actual: %p vs Expected: %p", data, expectedData);

	expectedData=data+builder->reversePackedSize;

	if(builder->reversePackedSize>0)
		{
		u32 *sizePtr=(u32 *)data;
		*sizePtr=builder->reversePackedSize-4;
		data+=4;

		if(builder->newReverseEntries!=NULL)
			data=writeRouteTableTagArray(builder->newReverseEntries, builder->newReverseEntryCount, data);
		else
			data=writeRouteTableTagArray(builder->oldReverseEntries, builder->oldReverseEntryCount, data);
		}

	if(data!=expectedData)
		LOG(LOG_CRITICAL,"Reverse tag data did not end at expected location Actual: %p vs Expected: %p", data, expectedData);

	return data;
}

void rtgMergeForwardRoutes(RouteTableTagBuilder *builder, int forwardTagCount, RoutePatch *patchPtr, RoutePatch *endPatchPtr)
{
	s32 totalTags=builder->oldForwardEntryCount+forwardTagCount;

	if(totalTags==0)
		return;

	//LOG(LOG_INFO, "rtgMergeForwardRoutes Tags %i OldTags %i Total %i",forwardTagCount, builder->oldForwardEntryCount, totalTags);

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
	if(totalTags==0)
		return;

	//LOG(LOG_INFO, "rtgMergeReverseRoutes Tags %i OldTags %i Total %i",reverseTagCount, builder->oldReverseEntryCount, totalTags);

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



