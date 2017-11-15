
#include "common.h"



void initTreeProxy(RouteTableTreeProxy *treeProxy,
		HeapDataBlock *leafBlock, u8 *leafDataPtr,
		HeapDataBlock *branchBlock, u8 *branchDataPtr,
		MemDispenser *disp)
{
	treeProxy->disp=disp;
	treeProxy->upstreamCount=0;
	treeProxy->downstreamCount=0;

	rttaInitBlockArrayProxy(treeProxy, &(treeProxy->leafArrayProxy), leafBlock, leafDataPtr, leafBlock->variant);
	rttaInitBlockArrayProxy(treeProxy, &(treeProxy->branchArrayProxy), branchBlock, branchDataPtr, 0);

	if(rttaGetArrayEntries(&(treeProxy->branchArrayProxy))>0)
		treeProxy->rootProxy=getRouteTableTreeBranchProxy(treeProxy, BRANCH_NINDEX_ROOT);
	else
		treeProxy->rootProxy=rttbAllocRouteTableTreeBranchProxy(treeProxy, ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK);

}

void updateTreeProxyTailCounts(RouteTableTreeProxy *treeProxy, s32 upstreamCount, s32 downstreamCount)
{
	treeProxy->upstreamCount=upstreamCount;
	treeProxy->downstreamCount=downstreamCount;
}


RouteTableTreeBranchBlock *getRouteTableTreeBranchBlock(RouteTableTreeProxy *treeProxy, s32 brindex)
{
	if(brindex<0)
		{
		LOG(LOG_CRITICAL,"Brindex must be positive: %i",brindex);
		}

	RouteTableTreeBranchProxy *branchProxy=rttaGetBlockArrayNewEntryProxy(&(treeProxy->branchArrayProxy), brindex);

	if(branchProxy!=NULL)
		return branchProxy->dataBlock;

	u8 *data=rttaGetBlockArrayExistingEntryData(&(treeProxy->branchArrayProxy),brindex);

	return (RouteTableTreeBranchBlock *)data;
}


RouteTableTreeBranchProxy *getRouteTableTreeBranchProxy(RouteTableTreeProxy *treeProxy, s32 brindex)
{
	if(brindex<0)
		{
		LOG(LOG_CRITICAL,"Brindex must be positive: %i",brindex);
		}

	RouteTableTreeBranchProxy *branchProxy=rttaGetBlockArrayNewEntryProxy(&(treeProxy->branchArrayProxy), brindex);

	if(branchProxy!=NULL)
		return branchProxy;

	u8 *data=rttaGetBlockArrayExistingEntryData(&(treeProxy->branchArrayProxy),brindex);

	branchProxy=dAlloc(treeProxy->disp, sizeof(RouteTableTreeBranchProxy));

	branchProxy->dataBlock=(RouteTableTreeBranchBlock *)data;
	branchProxy->brindex=brindex;

	rttbGetRouteTableTreeBranchProxy_scan(branchProxy->dataBlock, &branchProxy->childAlloc, &branchProxy->childCount);

	//LOG(LOG_INFO,"Got branch with %i of %i", branchProxy->childCount, branchProxy->childAlloc);

	return branchProxy;
}



RouteTableTreeLeafBlock *getRouteTableTreeLeafBlock(RouteTableTreeProxy *treeProxy, s32 lindex)
{
	if(lindex<0)
		{
		LOG(LOG_CRITICAL,"Lindex must be positive: %i",lindex);
		}

	RouteTableTreeLeafProxy *leafProxy=rttaGetBlockArrayNewEntryProxy(&(treeProxy->leafArrayProxy), lindex);

	if(leafProxy!=NULL)
		{
		LOG(LOG_INFO,"Leaf Proxy Block is %p",leafProxy->leafBlock);
		return leafProxy->leafBlock;
		}

	u8 *data=rttaGetBlockArrayExistingEntryData(&(treeProxy->leafArrayProxy), lindex);

	return (RouteTableTreeLeafBlock *)data;
}


RouteTableTreeLeafProxy *getRouteTableTreeLeafProxy(RouteTableTreeProxy *treeProxy, s32 lindex)
{
	if(lindex<0)
		{
		LOG(LOG_CRITICAL,"Lindex must be positive: %i",lindex);
		}

	RouteTableTreeLeafProxy *leafProxy=rttaGetBlockArrayNewEntryProxy(&(treeProxy->leafArrayProxy), lindex);

	if(leafProxy!=NULL)
		return leafProxy;

	u8 *data=rttaGetBlockArrayExistingEntryData(&(treeProxy->leafArrayProxy), lindex);

	leafProxy=dAlloc(treeProxy->disp, sizeof(RouteTableTreeLeafProxy));

	leafProxy->leafBlock=(RouteTableTreeLeafBlock *)data;
	leafProxy->parentBrindex=leafProxy->leafBlock->parentBrindex;
	leafProxy->lindex=lindex;
	//leafProxy->status=LEAFPROXY_STATUS_FULLYPACKED;


//	LOG(LOG_INFO,"GetRouteTableTreeLeaf : %i",lindex);

//	LOG(LOG_INFO,"PackLeaf: getRouteTableTreeLeafProxy %i from %p",lindex, data);

	leafProxy->unpackedBlock=rtpUnpackSingleBlockHeaderAndOffsets((RouteTablePackedSingleBlock *)(leafProxy->leafBlock->packedBlockData),
			treeProxy->disp, treeProxy->upstreamCount, treeProxy->downstreamCount);

	leafProxy->status=LEAFPROXY_STATUS_SEMIUNPACKED;
	//rttlEnsureFullyUnpacked(treeProxy, leafProxy);

	//rtpUnpackSingleBlockEntryArrays((RouteTablePackedSingleBlock *)(leafProxy->leafBlock->packedBlockData), leafProxy->unpackedBlock);
	//leafProxy->status=LEAFPROXY_STATUS_FULLYUNPACKED;

//	dumpLeafProxy(leafProxy);

	return leafProxy;
}






void treeProxySeekStart(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchProxyPtr, s16 *branchChildSibdexPtr, RouteTableTreeLeafProxy **leafProxyPtr)
{
	RouteTableTreeBranchProxy *branchProxy=treeProxy->rootProxy;
	RouteTableTreeLeafProxy *leafProxy=NULL;
	s16 sibdex=-1;

	s16 childNindex=0;
	int iter=20;

//	LOG(LOG_INFO,"Seek start: Root contains %i",branchProxy->childCount);

	while(branchProxy->childCount>0 && childNindex>=0 && --iter>0)
		{
		childNindex=branchProxy->dataBlock->childNindex[0];
		if(childNindex>0)
			{
			branchProxy=getRouteTableTreeBranchProxy(treeProxy, childNindex);
			}
		}

	if(iter<=0)
		LOG(LOG_CRITICAL,"Failed to seek start");

//	LOG(LOG_INFO,"Seek start %i",childNindex);


	if(childNindex<0)
		{
		sibdex=0;
		leafProxy=getRouteTableTreeLeafProxy(treeProxy,NINDEX_TO_LINDEX(childNindex));
		}

	if(branchProxyPtr!=NULL)
		*branchProxyPtr=branchProxy;

	if(branchChildSibdexPtr!=NULL)
		*branchChildSibdexPtr=sibdex;

	if(leafProxyPtr!=NULL)
		*leafProxyPtr=leafProxy;

}


void treeProxySeekEnd(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchProxyPtr,  s16 *branchChildSibdexPtr, RouteTableTreeLeafProxy **leafProxyPtr)
{
	RouteTableTreeBranchProxy *branchProxy=treeProxy->rootProxy;
	RouteTableTreeLeafProxy *leafProxy=NULL;
	s16 sibdex=-1;

	s16 childNindex=0;

//	LOG(LOG_INFO,"Seek end: Root contains %i",branchProxy->childCount);

	while(branchProxy->childCount>0 && childNindex>=0)
		{
		childNindex=branchProxy->dataBlock->childNindex[branchProxy->childCount-1];

		if(childNindex>0)
			{
			branchProxy=getRouteTableTreeBranchProxy(treeProxy, childNindex);
			}
		}

	if(childNindex<0)
		{
		sibdex=branchProxy->childCount-1;
		leafProxy=getRouteTableTreeLeafProxy(treeProxy,NINDEX_TO_LINDEX(childNindex));
		}
/*
	if(leafProxy!=NULL)
		{
		LOG(LOG_INFO,"Branch %i Child %i Sibdex %i",branchProxy->brindex,leafProxy->lindex,sibdex);
		}
	else
		{
		LOG(LOG_INFO,"Branch %i Null Leaf",branchProxy->brindex);
		}
*/
	if(branchProxyPtr!=NULL)
		*branchProxyPtr=branchProxy;

	if(branchChildSibdexPtr!=NULL)
		*branchChildSibdexPtr=sibdex;

	if(leafProxyPtr!=NULL)
		*leafProxyPtr=leafProxy;

}



static void treeProxySplitRoot(RouteTableTreeProxy *treeProxy, s16 childPosition, RouteTableTreeBranchProxy **newParentBranchProxyPtr, s16 *newChildPositionPtr)
{
	RouteTableTreeBranchProxy *rootBranchProxy=treeProxy->rootProxy;

	if(rootBranchProxy->brindex!=BRANCH_NINDEX_ROOT)
		{
		LOG(LOG_CRITICAL,"Asked to root-split a non-root node");
		}

	s32 firstHalfChild=((1+rootBranchProxy->childCount)/2);
	s32 secondHalfChild=rootBranchProxy->childCount-firstHalfChild;

	RouteTableTreeBranchProxy *branchProxy1=rttbAllocRouteTableTreeBranchProxy(treeProxy, firstHalfChild+ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK);
	RouteTableTreeBranchProxy *branchProxy2=rttbAllocRouteTableTreeBranchProxy(treeProxy, secondHalfChild+ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK);

	s32 branchBrindex1=branchProxy1->brindex;
	s32 branchBrindex2=branchProxy2->brindex;

	for(int i=0;i<firstHalfChild;i++)
		{
		s32 childNindex=rootBranchProxy->dataBlock->childNindex[i];
		branchProxy1->dataBlock->childNindex[i]=childNindex;

		if(childNindex<0)
			{
			RouteTableTreeLeafProxy *leafProxy=getRouteTableTreeLeafProxy(treeProxy, NINDEX_TO_LINDEX(childNindex));

			if(leafProxy==NULL)
				LOG(LOG_CRITICAL,"Failed to find leaf with index %i",childNindex);

			leafProxy->parentBrindex=branchBrindex1;
			}
		else
			{
			RouteTableTreeBranchBlock *branchBlock=getRouteTableTreeBranchBlock(treeProxy, childNindex);

			if(branchBlock==NULL)
				LOG(LOG_CRITICAL,"Failed to find branch with index %i",childNindex);

			branchBlock->parentBrindex=branchBrindex1;
			}
		}

	branchProxy1->childCount=firstHalfChild;

	for(int i=0;i<secondHalfChild;i++)
		{
		s32 childNindex=rootBranchProxy->dataBlock->childNindex[i+firstHalfChild];
		branchProxy2->dataBlock->childNindex[i]=childNindex;

		if(childNindex<0)
			{
			RouteTableTreeLeafProxy *leafProxy=getRouteTableTreeLeafProxy(treeProxy, NINDEX_TO_LINDEX(childNindex));

			if(leafProxy==NULL)
				LOG(LOG_CRITICAL,"Failed to find leaf with index %i",childNindex);

			leafProxy->parentBrindex=branchBrindex2;
			}
		else
			{
			RouteTableTreeBranchBlock *branchBlock=getRouteTableTreeBranchBlock(treeProxy, childNindex);

			if(branchBlock==NULL)
				LOG(LOG_CRITICAL,"Failed to find branch with index %i",childNindex);

			branchBlock->parentBrindex=branchBrindex2;
			}
		}

	branchProxy2->childCount=secondHalfChild;

	branchProxy1->dataBlock->parentBrindex=BRANCH_NINDEX_ROOT;
	branchProxy2->dataBlock->parentBrindex=BRANCH_NINDEX_ROOT;

	RouteTableTreeBranchBlock *newRoot=rttbReallocRouteTableTreeBranchBlockEntries(NULL, treeProxy->disp, ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK);

	newRoot->parentBrindex=BRANCH_NINDEX_INVALID;
	newRoot->childNindex[0]=branchBrindex1;
	newRoot->childNindex[1]=branchBrindex2;

	rootBranchProxy->childCount=2;
	rootBranchProxy->childAlloc=newRoot->childAlloc;
	rootBranchProxy->dataBlock=newRoot;

	rttbFlushRouteTableTreeBranchProxy(treeProxy, rootBranchProxy);

	if(childPosition<firstHalfChild)
		{
		*newParentBranchProxyPtr=branchProxy1;
		*newChildPositionPtr=childPosition;
		}
	else
		{
		*newParentBranchProxyPtr=branchProxy2;
		*newChildPositionPtr=childPosition-firstHalfChild;
		}
}



// Split a branch, creating and returning a new sibling which _MUST_ be attached by the caller. Location of an existing child is updated

static RouteTableTreeBranchProxy *treeProxySplitBranch(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *branchProxy, s16 childPosition,
		RouteTableTreeBranchProxy **newParentBranchProxyPtr, s16 *newChildPositionPtr)
{
//	LOG(LOG_INFO,"Non Root Branch Split: %i",branch->brindex);

	s32 halfChild=((1+branchProxy->childCount)/2);
	s32 otherHalfChild=branchProxy->childCount-halfChild;

	s32 halfChildAlloc=halfChild+1;

	RouteTableTreeBranchProxy *newBranchProxy=rttbAllocRouteTableTreeBranchProxy(treeProxy, halfChildAlloc);

	s32 newBranchBrindex=newBranchProxy->brindex;
	branchProxy->childCount=halfChild;

	for(int i=0;i<otherHalfChild;i++)
		{
//		LOG(LOG_INFO,"Index %i",i);

		s32 childNindex=branchProxy->dataBlock->childNindex[i+halfChild];
		newBranchProxy->dataBlock->childNindex[i]=childNindex;
		branchProxy->dataBlock->childNindex[i+halfChild]=BRANCH_NINDEX_INVALID;

//		LOG(LOG_INFO,"Moving Child %i to %i",childNindex, newBranchBrindex);

		if(childNindex<0)
			{
			RouteTableTreeLeafProxy *leafProxy=getRouteTableTreeLeafProxy(treeProxy, NINDEX_TO_LINDEX(childNindex));

			if(leafProxy==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find leaf with index %i",childNindex);
				}

			leafProxy->parentBrindex=newBranchBrindex;
			}
		else
			{
			RouteTableTreeBranchBlock *branchBlock=getRouteTableTreeBranchBlock(treeProxy, childNindex);

			if(branchBlock==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find branch with index %i",childNindex);
				}

			branchBlock->parentBrindex=newBranchBrindex;
			}
		}

	newBranchProxy->childCount=otherHalfChild;

	if(childPosition<halfChild)
		{
		*newParentBranchProxyPtr=branchProxy;
		*newChildPositionPtr=childPosition;
		}
	else
		{
		*newParentBranchProxyPtr=newBranchProxy;
		*newChildPositionPtr=childPosition-halfChild;
		}

	return newBranchProxy;
}


// Should probably move the unpacked block splitting to routeTablePacking
// Split a leaf, creating and returning a new sibling which _MUST_ be attached by the caller. Location of an existing entry is updated

static RouteTableTreeLeafProxy *treeProxySplitLeafContents(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leafProxy, s16 leafArrayIndex,
		 RouteTableTreeLeafProxy **updatedLeafProxyPtr, s16 *updatedEntryPositionPtr)
{
	RouteTableUnpackedSingleBlock *oldBlock=leafProxy->unpackedBlock;

	s32 toKeepHalfArray=((1+oldBlock->entryArrayCount)/2);
	s32 toMoveHalfArray=oldBlock->entryArrayCount-toKeepHalfArray;

	s32 toMoveHalfArrayAlloc=toMoveHalfArray+ROUTEPACKING_ENTRYARRAYS_CHUNK;

	RouteTableTreeLeafProxy *newLeafProxy=rttlAllocRouteTableTreeLeafProxy(treeProxy, oldBlock->upstreamOffsetAlloc, oldBlock->downstreamOffsetAlloc, toMoveHalfArrayAlloc);
	RouteTableUnpackedSingleBlock *newBlock=newLeafProxy->unpackedBlock;

	RouteTableUnpackedEntryArray **oldEntryArrayPtr=oldBlock->entryArrays;
	RouteTableUnpackedEntryArray **newEntryArrayPtr=newBlock->entryArrays;

	memcpy(newEntryArrayPtr, oldEntryArrayPtr+toKeepHalfArray, sizeof(RouteTableUnpackedEntryArray *)*toMoveHalfArray);

	//memset(leaf->dataBlock->entries+halfEntry, 0, sizeof(RouteTableTreeLeafEntry)*otherHalfEntry);

	oldBlock->entryArrayCount=toKeepHalfArray;
	newBlock->entryArrayCount=toMoveHalfArray;

	for(int i=newBlock->entryArrayCount; i<newBlock->entryArrayAlloc; i++)
		newBlock->entryArrays[i]=NULL;

	//LOG(LOG_INFO,"Clearing Old Leaf %i %i",leaf->entryCount, leaf->entryAlloc);
	for(int i=oldBlock->entryArrayCount; i<oldBlock->entryArrayAlloc; i++)
		oldBlock->entryArrays[i]=NULL;

	rtpRecalculateUnpackedBlockOffsets(oldBlock);
	rtpRecalculateUnpackedBlockOffsets(newBlock);

	if(leafArrayIndex<toKeepHalfArray)
		{
		*updatedLeafProxyPtr=leafProxy;
		*updatedEntryPositionPtr=leafArrayIndex;
		}
	else
		{
		*updatedLeafProxyPtr=newLeafProxy;
		*updatedEntryPositionPtr=leafArrayIndex-toKeepHalfArray;
		}

	return newLeafProxy;
}





// Insert the given branch child to the branch parent, splitting the parent if needed


void treeProxyInsertBranchChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeBranchProxy *childBranchProxy, s16 childPosition,
		RouteTableTreeBranchProxy **updatedParentBranchProxyPtr, s16 *updatedChildPositionPtr)
{
	if(parentBranchProxy->childCount>=parentBranchProxy->childAlloc) // No Space
		{
		if(parentBranchProxy->childAlloc<ROUTE_TABLE_TREE_BRANCH_CHILDREN) // Expand
			{
			rttbExpandRouteTableTreeBranchProxy(treeProxy, parentBranchProxy);
			}
		else if(parentBranchProxy->brindex!=BRANCH_NINDEX_ROOT) // Add sibling
			{
			RouteTableTreeBranchProxy *grandParentBranchProxy;
			s32 grandParentBrindex=parentBranchProxy->dataBlock->parentBrindex;

			if(grandParentBrindex!=BRANCH_NINDEX_ROOT)
				grandParentBranchProxy=getRouteTableTreeBranchProxy(treeProxy, grandParentBrindex);
			else
				grandParentBranchProxy=treeProxy->rootProxy;

			s16 parentSibdex=rttbGetBranchChildSibdex_Branch(grandParentBranchProxy, parentBranchProxy);
			RouteTableTreeBranchProxy *newParentBranchProxy=treeProxySplitBranch(treeProxy, parentBranchProxy, childPosition, &parentBranchProxy, &childPosition);

			RouteTableTreeBranchProxy *dummyParentBranchProxy; // Don't track splits of grandparent / position
			s16 dummyChildPositionPtr;

			treeProxyInsertBranchChild(treeProxy, grandParentBranchProxy, newParentBranchProxy, parentSibdex+1, &dummyParentBranchProxy, &dummyChildPositionPtr);

			}
		else	// Split root
			{
			treeProxySplitRoot(treeProxy, childPosition, &parentBranchProxy, &childPosition);
			}

		}

	if(parentBranchProxy->childCount>=parentBranchProxy->childAlloc)
		{
		LOG(LOG_CRITICAL,"No space after split/sibling add in branch node %i", parentBranchProxy->brindex);
		}

	s16 parentBrindex=parentBranchProxy->brindex;
	s16 childBrindex=childBranchProxy->brindex;

//	LOG(LOG_INFO,"Inserting Branch %i to %i",childBrindex, parentBrindex);

	rttbBranchMakeChildInsertSpace(parentBranchProxy, childPosition, 1);

	parentBranchProxy->dataBlock->childNindex[childPosition]=childBrindex;
	childBranchProxy->dataBlock->parentBrindex=parentBrindex;

	*updatedParentBranchProxyPtr=parentBranchProxy;
	*updatedChildPositionPtr=childPosition;
}

// Insert the given leaf child into the branch parent, splitting the parent if needed
void treeProxyInsertLeafChildBefore(RouteTableTreeProxy *treeProxy,
		RouteTableTreeBranchProxy *branchProxy, RouteTableTreeLeafProxy *existingChildLeafProxy, s16 existingChildPosition,
		RouteTableTreeLeafProxy *newChildLeafProxy, RouteTableTreeBranchProxy **updatedBranchProxyPtr, s16 *updatedExistingChildPositionPtr, s16 *updatedNewChildPositionPtr)
{
	if(branchProxy->childCount>=branchProxy->childAlloc)
		{
		if(branchProxy->childAlloc<ROUTE_TABLE_TREE_BRANCH_CHILDREN)
			{
			rttbExpandRouteTableTreeBranchProxy(treeProxy, branchProxy);
			}
		else if(branchProxy->brindex!=BRANCH_NINDEX_ROOT)
			{
			RouteTableTreeBranchProxy *grandParentBranchProxy;
			s32 grandParentBrindex=branchProxy->dataBlock->parentBrindex;

			if(grandParentBrindex!=BRANCH_NINDEX_ROOT)
				grandParentBranchProxy=getRouteTableTreeBranchProxy(treeProxy, grandParentBrindex);
			else
				grandParentBranchProxy=treeProxy->rootProxy;

			s16 parentSibdex=rttbGetBranchChildSibdex_Branch(grandParentBranchProxy, branchProxy);

			RouteTableTreeBranchProxy *newParentBranchProxy=treeProxySplitBranch(treeProxy, branchProxy, existingChildPosition, &branchProxy, &existingChildPosition);
			RouteTableTreeBranchProxy *dummyParentBranchProxy; // Don't track splits of grandparent / position
			s16 dummyChildPositionPtr;

//			LOG(LOG_INFO,"Old Parent is %i, New parent is %i, Grandparent is %i",parent->brindex, newParent->brindex, grandParentBrindex);

			treeProxyInsertBranchChild(treeProxy, grandParentBranchProxy, newParentBranchProxy, parentSibdex+1, &dummyParentBranchProxy, &dummyChildPositionPtr);
			}
		else
			{
			treeProxySplitRoot(treeProxy, existingChildPosition, &branchProxy, &existingChildPosition);
			}
		}

	if(branchProxy->childCount>=branchProxy->childAlloc)
		{
		LOG(LOG_CRITICAL,"No space after split/sibling add");
		}

	s16 parentBrindex=branchProxy->brindex;
	s16 childLindex=newChildLeafProxy->lindex;
	rttbBranchMakeChildInsertSpace(branchProxy, existingChildPosition, 1);

	branchProxy->dataBlock->childNindex[existingChildPosition]=LINDEX_TO_NINDEX(childLindex);
	newChildLeafProxy->parentBrindex=parentBrindex;

	if(updatedBranchProxyPtr!=NULL)
		*updatedBranchProxyPtr=branchProxy;

	if(updatedExistingChildPositionPtr!=NULL)
		*updatedExistingChildPositionPtr=existingChildPosition+1;

	if(updatedNewChildPositionPtr!=NULL)
		*updatedNewChildPositionPtr=existingChildPosition;

//	LOG(LOG_INFO,"leafEntry(childPosition) %i",childPosition);
}

// Insert the given leaf child into the branch parent, splitting the parent if needed
void treeProxyInsertLeafChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeLeafProxy *childLeafProxy, s16 childPosition,
		RouteTableTreeBranchProxy **updatedParentBranchProxyPtr, s16 *updatedChildPositionPtr)
{
	if(parentBranchProxy->childCount>=parentBranchProxy->childAlloc)
		{
		if(parentBranchProxy->childAlloc<ROUTE_TABLE_TREE_BRANCH_CHILDREN)
			{
			rttbExpandRouteTableTreeBranchProxy(treeProxy, parentBranchProxy);
			}
		else if(parentBranchProxy->brindex!=BRANCH_NINDEX_ROOT)
			{
			RouteTableTreeBranchProxy *grandParentBranchProxy;
			s32 grandParentBrindex=parentBranchProxy->dataBlock->parentBrindex;

			if(grandParentBrindex!=BRANCH_NINDEX_ROOT)
				grandParentBranchProxy=getRouteTableTreeBranchProxy(treeProxy, grandParentBrindex);
			else
				grandParentBranchProxy=treeProxy->rootProxy;

			s16 parentSibdex=rttbGetBranchChildSibdex_Branch(grandParentBranchProxy, parentBranchProxy);
			RouteTableTreeBranchProxy *newParentBranchProxy=treeProxySplitBranch(treeProxy, parentBranchProxy, childPosition, &parentBranchProxy, &childPosition);
			RouteTableTreeBranchProxy *dummyParentBranchProxy; // Don't track splits of grandparent / position
			s16 dummyChildPositionPtr;

//			LOG(LOG_INFO,"Old Parent is %i, New parent is %i, Grandparent is %i",parent->brindex, newParent->brindex, grandParentBrindex);

			treeProxyInsertBranchChild(treeProxy, grandParentBranchProxy, newParentBranchProxy, parentSibdex+1, &dummyParentBranchProxy, &dummyChildPositionPtr);
			}
		else
			{
			treeProxySplitRoot(treeProxy, childPosition, &parentBranchProxy, &childPosition);
			}
		}

	if(parentBranchProxy->childCount>=parentBranchProxy->childAlloc)
		{
		LOG(LOG_CRITICAL,"No space after split/sibling add");
		}

	s16 parentBrindex=parentBranchProxy->brindex;
	s16 childLindex=childLeafProxy->lindex;
	rttbBranchMakeChildInsertSpace(parentBranchProxy, childPosition, 1);

	parentBranchProxy->dataBlock->childNindex[childPosition]=LINDEX_TO_NINDEX(childLindex);
	childLeafProxy->parentBrindex=parentBrindex;

	*updatedParentBranchProxyPtr=parentBranchProxy;
	*updatedChildPositionPtr=childPosition;

//	LOG(LOG_INFO,"leafEntry(childPosition) %i",childPosition);
}

void treeProxyAppendLeafChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeLeafProxy *childLeafProxy,
		RouteTableTreeBranchProxy **newParentBranchProxyPtr, s16 *childPositionPtr)
{
	s16 childPosition=parentBranchProxy->childCount;

	treeProxyInsertLeafChild(treeProxy, parentBranchProxy, childLeafProxy, childPosition, newParentBranchProxyPtr, &childPosition);
	*childPositionPtr=childPosition;
}


// Split a leaf, allowing space to insert an array at the specified position, returning the entry index of the first space, and providing the new context

RouteTableTreeLeafProxy *treeProxySplitLeaf(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy,
		s16 childPosition, RouteTableTreeLeafProxy *childLeafProxy, s16 insertArrayIndex,
		RouteTableTreeBranchProxy **updatedParentBranchProxyPtr, s16 *updatedChildPositionPtr, RouteTableTreeLeafProxy **updatedChildLeafProxyPtr, s16 *updatedArrayIndexPtr)
{
	//RouteTableTreeLeafProxy *oldLeaf=child;
	RouteTableTreeLeafProxy *newLeafProxy=treeProxySplitLeafContents(treeProxy, childLeafProxy, insertArrayIndex,  &childLeafProxy, &insertArrayIndex);

	treeProxyInsertLeafChild(treeProxy, parentBranchProxy, newLeafProxy, childPosition+1, updatedParentBranchProxyPtr, updatedChildPositionPtr);

	//leafMakeEntryInsertSpace(childLeafProxy, insertEntryPosition, insertEntryCount);

	*updatedChildLeafProxyPtr=childLeafProxy;
	*updatedArrayIndexPtr=insertArrayIndex;

	return newLeafProxy;
}


s32 hasMoreSiblings(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *branchProxy, s16 sibdexEst, RouteTableTreeLeafProxy *leafProxy)
{
	s16 sibdex=rttbGetBranchChildSibdex_Leaf_withEstimate(branchProxy, leafProxy, sibdexEst);

	if(sibdexEst!=sibdex)
		{
		LOG(LOG_CRITICAL,"Sibdex did not match expected Est: %i Actual: %i",sibdexEst, sibdex);
		}

	sibdex++;
	if(sibdex<branchProxy->childCount) // More leaves on branch
		return 1;

	while(branchProxy->brindex!=BRANCH_NINDEX_ROOT)
		{
		if(branchProxy->dataBlock->parentBrindex==BRANCH_NINDEX_INVALID)
			LOG(LOG_CRITICAL,"Invalid parent index");

		RouteTableTreeBranchProxy *parentBranchProxy=getRouteTableTreeBranchProxy(treeProxy, branchProxy->dataBlock->parentBrindex);

		sibdex=rttbGetBranchChildSibdex_Branch(parentBranchProxy, branchProxy);
		sibdex++;

		if(sibdex<parentBranchProxy->childCount) // More branches off ancestor
			return 1;

		branchProxy=parentBranchProxy;
		}

	return 0;
}



s32 getNextBranchSibling(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchProxyPtr)
{
	RouteTableTreeBranchProxy *branchProxy=*branchProxyPtr;

//	LOG(LOG_INFO,"getNextBranchSibling for %i",branch->brindex);

	//LOG(LOG_CRITICAL,"FIXME - %i",branch->brindex);

	if(branchProxy->dataBlock->parentBrindex!=BRANCH_NINDEX_INVALID) // Should never happen
		{
		RouteTableTreeBranchProxy *parentBranchProxy=getRouteTableTreeBranchProxy(treeProxy, branchProxy->dataBlock->parentBrindex);

		//LOG(LOG_INFO,"Branch is %i - Parent is %i",branch->brindex,parent->brindex);

		s16 sibdex=rttbGetBranchChildSibdex_Branch(parentBranchProxy, branchProxy);
		sibdex++;

		if(sibdex<parentBranchProxy->childCount) // Parent contains more
			{
			branchProxy=getRouteTableTreeBranchProxy(treeProxy, parentBranchProxy->dataBlock->childNindex[sibdex]);
			*branchProxyPtr=branchProxy;

//			LOG(LOG_INFO,"getNextBranchSibling direct branch at %i",branch->brindex);

			return 1;
			}
		else if((branchProxy->dataBlock->parentBrindex!=BRANCH_NINDEX_ROOT) && getNextBranchSibling(treeProxy, &parentBranchProxy))
			{
//			LOG(LOG_INFO,"getNextBranchSibling recursive parent at %i",parent->brindex);

			if(parentBranchProxy->childCount>0)
				{
				if(parentBranchProxy->dataBlock->childNindex[0]<0)
					LOG(LOG_CRITICAL,"Parent %i contains child %i",parentBranchProxy->brindex, parentBranchProxy->dataBlock->childNindex[0]);

				branchProxy=getRouteTableTreeBranchProxy(treeProxy, parentBranchProxy->dataBlock->childNindex[0]);
				*branchProxyPtr=branchProxy;

//				LOG(LOG_INFO,"getNextBranchSibling recursive branch at %i",branch->brindex);

				return 1;
				}
			else
				LOG(LOG_CRITICAL,"Moved to empty branch");

			}
		}

	//LOG(LOG_INFO,"getNextBranchSibling: end of tree");

	return 0;
}


s32 getNextLeafSibling(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchProxyPtr, s16 *branchChildSibdex, RouteTableTreeLeafProxy **leafProxyPtr)
{
	RouteTableTreeBranchProxy *branchProxy=*branchProxyPtr;
	RouteTableTreeLeafProxy *leafProxy=*leafProxyPtr;

	s16 sibdexEst=*branchChildSibdex;
	s16 sibdex=rttbGetBranchChildSibdex_Leaf_withEstimate(branchProxy, leafProxy, sibdexEst);

//	LOG(LOG_INFO,"getNextLeafSibling from %i %i %i",branch->brindex, sibdex, leaf->lindex);


	if(sibdexEst!=sibdex)
		{
		LOG(LOG_CRITICAL,"Sibdex did not match expected Est: %i Actual: %i",sibdexEst, sibdex);
		}

	sibdex++;
	if(sibdex<branchProxy->childCount) // Same branch, next leaf
		{
//		LOG(LOG_INFO,"In branch %i, looking at sib %i giving %i",branch->brindex, sibdex, branch->dataBlock->childNindex[sibdex]);

		*branchChildSibdex=sibdex;
		*leafProxyPtr=getRouteTableTreeLeafProxy(treeProxy, NINDEX_TO_LINDEX(branchProxy->dataBlock->childNindex[sibdex]));
		return 1;
		}

	if(getNextBranchSibling(treeProxy, &branchProxy)) // Attempt to move branch
		{
		*branchProxyPtr=branchProxy;
		if(branchProxy->childCount>0) // Non-empty branch
			{
			*branchChildSibdex=0;
			*leafProxyPtr=getRouteTableTreeLeafProxy(treeProxy, NINDEX_TO_LINDEX(branchProxy->dataBlock->childNindex[0]));
			return 1;
			}
		else
			{
			sibdex=-1;
			LOG(LOG_CRITICAL,"Moved to empty branch");
			return 0;
			}
		}
	else
		return 0; // No further branches or leaves - references unchanged
}

