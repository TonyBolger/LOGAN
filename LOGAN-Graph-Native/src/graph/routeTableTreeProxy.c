
#include "common.h"






void initTreeProxy(RouteTableTreeProxy *treeProxy,
		HeapDataBlock *leafBlock, u8 *leafDataPtr,
		HeapDataBlock *branchBlock, u8 *branchDataPtr,
		MemDispenser *disp)
{
	treeProxy->disp=disp;

	initBlockArrayProxy(treeProxy, &(treeProxy->leafArrayProxy), leafBlock, leafDataPtr, 2);
	initBlockArrayProxy(treeProxy, &(treeProxy->branchArrayProxy), branchBlock, branchDataPtr, 2);

	if(getBlockArraySize(&(treeProxy->branchArrayProxy))>0)
		treeProxy->rootProxy=getRouteTableTreeBranchProxy(treeProxy, BRANCH_NINDEX_ROOT);
	else
		treeProxy->rootProxy=allocRouteTableTreeBranchProxy(treeProxy, ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK);

}


void treeProxySeekStart(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchProxyPtr, s16 *branchChildSibdexPtr, RouteTableTreeLeafProxy **leafProxyPtr)
{
	RouteTableTreeBranchProxy *branchProxy=treeProxy->rootProxy;
	RouteTableTreeLeafProxy *leafProxy=NULL;
	s16 sibdex=-1;

	s16 childNindex=0;

	while(branchProxy->childCount>0 && childNindex>=0)
		{
		childNindex=branchProxy->dataBlock->childNindex[0];

		if(childNindex>0)
			branchProxy=getRouteTableTreeBranchProxy(treeProxy, childNindex);
		}

	if(childNindex<0)
		{
		sibdex=0;

//		LOG(LOG_INFO,"Get %i as %i",childNindex, NINDEX_TO_LINDEX(childNindex));

		leafProxy=getRouteTableTreeLeafProxy(treeProxy,NINDEX_TO_LINDEX(childNindex));
		}

	/*
	if(leafProxy!=NULL)
		{
		LOG(LOG_INFO,"Branch %i Child %i (%i of %i) Sibdex %i",branchProxy->brindex,leafProxy->lindex,leafProxy->entryCount,leafProxy->entryAlloc, sibdex);
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


void treeProxySeekEnd(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy **branchProxyPtr,  s16 *branchChildSibdexPtr, RouteTableTreeLeafProxy **leafProxyPtr)
{
	RouteTableTreeBranchProxy *branchProxy=treeProxy->rootProxy;
	RouteTableTreeLeafProxy *leafProxy=NULL;
	s16 sibdex=-1;

	s16 childNindex=0;

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



void treeProxySplitRoot(RouteTableTreeProxy *treeProxy, s16 childPosition, RouteTableTreeBranchProxy **newParentBranchProxyPtr, s16 *newChildPositionPtr)
{
//	LOG(LOG_INFO,"Root Split");

	RouteTableTreeBranchProxy *rootBranchProxy=treeProxy->rootProxy;

	if(rootBranchProxy->brindex!=BRANCH_NINDEX_ROOT)
		{
		LOG(LOG_CRITICAL,"Asked to root-split a non-root node");
		}

//	LOG(LOG_INFO,"Root %i contains %i of %i",root->brindex, root->childCount, root->childAlloc);
/*
	for(int i=0;i<root->childCount;i++)
		{
		LOG(LOG_INFO,"Child %i is %i",i,root->dataBlock->childNindex[i]);
		}
*/

	s32 firstHalfChild=((1+rootBranchProxy->childCount)/2);
	s32 secondHalfChild=rootBranchProxy->childCount-firstHalfChild;

	RouteTableTreeBranchProxy *branchProxy1=allocRouteTableTreeBranchProxy(treeProxy, firstHalfChild+ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK);
	RouteTableTreeBranchProxy *branchProxy2=allocRouteTableTreeBranchProxy(treeProxy, secondHalfChild+ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK);

	s32 branchBrindex1=branchProxy1->brindex;
	s32 branchBrindex2=branchProxy2->brindex;

	//LOG(LOG_INFO,"Split - new branches %i %i",branchBrindex1, branchBrindex2);

	for(int i=0;i<firstHalfChild;i++)
		{
		s32 childNindex=rootBranchProxy->dataBlock->childNindex[i];
		branchProxy1->dataBlock->childNindex[i]=childNindex;

//		LOG(LOG_INFO,"Moving Child %i to %i",childNindex, branchBrindex1);

		if(childNindex<0)
			{
			RouteTableTreeLeafBlock *leafRaw=getRouteTableTreeLeafRaw(treeProxy, NINDEX_TO_LINDEX(childNindex));

			if(leafRaw==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find leaf with index %i",childNindex);
				}

			leafRaw->parentBrindex=branchBrindex1;
			}
		else
			{
			RouteTableTreeBranchBlock *branchRaw=getRouteTableTreeBranchRaw(treeProxy, childNindex);

			if(branchRaw==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find branch with index %i",childNindex);
				}

			branchRaw->parentBrindex=branchBrindex1;
			}
		}

	branchProxy1->childCount=firstHalfChild;

	for(int i=0;i<secondHalfChild;i++)
		{
		s32 childNindex=rootBranchProxy->dataBlock->childNindex[i+firstHalfChild];
		branchProxy2->dataBlock->childNindex[i]=childNindex;

//		LOG(LOG_INFO,"Moving Child %i to %i",childNindex, branchBrindex2);

		if(childNindex<0)
			{
			RouteTableTreeLeafBlock *leafRaw=getRouteTableTreeLeafRaw(treeProxy, NINDEX_TO_LINDEX(childNindex));

			if(leafRaw==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find leaf with index %i",childNindex);
				}

			leafRaw->parentBrindex=branchBrindex2;
			}
		else
			{
			RouteTableTreeBranchBlock *branchRaw=getRouteTableTreeBranchRaw(treeProxy, childNindex);

			if(branchRaw==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find branch with index %i",childNindex);
				}

			branchRaw->parentBrindex=branchBrindex2;
			}
		}

	branchProxy2->childCount=secondHalfChild;

	branchProxy1->dataBlock->parentBrindex=BRANCH_NINDEX_ROOT;
	branchProxy2->dataBlock->parentBrindex=BRANCH_NINDEX_ROOT;

	RouteTableTreeBranchBlock *newRoot=reallocRouteTableTreeBranchBlockEntries(NULL, treeProxy->disp, ROUTE_TABLE_TREE_BRANCH_CHILDREN_CHUNK);

	newRoot->parentBrindex=BRANCH_NINDEX_INVALID;
	newRoot->childNindex[0]=branchBrindex1;
	newRoot->childNindex[1]=branchBrindex2;

	rootBranchProxy->childCount=2;
	rootBranchProxy->childAlloc=newRoot->childAlloc;
	rootBranchProxy->dataBlock=newRoot;

	flushRouteTableTreeBranchProxy(treeProxy, rootBranchProxy);

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

RouteTableTreeBranchProxy *treeProxySplitBranch(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *branchProxy, s16 childPosition,
		RouteTableTreeBranchProxy **newParentBranchProxyPtr, s16 *newChildPositionPtr)
{
//	LOG(LOG_INFO,"Non Root Branch Split: %i",branch->brindex);

	s32 halfChild=((1+branchProxy->childCount)/2);
	s32 otherHalfChild=branchProxy->childCount-halfChild;

	s32 halfChildAlloc=halfChild+1;

	RouteTableTreeBranchProxy *newBranchProxy=allocRouteTableTreeBranchProxy(treeProxy, halfChildAlloc);

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
			RouteTableTreeLeafBlock *leafRaw=getRouteTableTreeLeafRaw(treeProxy, NINDEX_TO_LINDEX(childNindex));

			if(leafRaw==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find leaf with index %i",childNindex);
				}

			leafRaw->parentBrindex=newBranchBrindex;
			}
		else
			{
			RouteTableTreeBranchBlock *branchRaw=getRouteTableTreeBranchRaw(treeProxy, childNindex);

			if(branchRaw==NULL)
				{
				LOG(LOG_CRITICAL,"Failed to find branch with index %i",childNindex);
				}

			branchRaw->parentBrindex=newBranchBrindex;
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



// Split a leaf, creating and returning a new sibling which _MUST_ be attached by the caller. Location of an existing entry is updated

RouteTableTreeLeafProxy *treeProxySplitLeaf(RouteTableTreeProxy *treeProxy, RouteTableTreeLeafProxy *leafProxy, s16 leafEntryPosition, s16 space,
		 RouteTableTreeLeafProxy **newLeafProxyPtr, s16 *newEntryPositionPtr)
{
//	LOG(LOG_INFO,"Leaf Split: %i",leaf->lindex);

	s32 toKeepHalfEntry=((1+leafProxy->entryCount)/2);
	s32 toMoveHalfEntry=leafProxy->entryCount-toKeepHalfEntry;

	s32 toMoveHalfEntryAlloc=toMoveHalfEntry+space;

	RouteTableTreeLeafProxy *newLeafProxy=allocRouteTableTreeLeafProxy(treeProxy, leafProxy->dataBlock->offsetAlloc, toMoveHalfEntryAlloc);

	RouteTableTreeLeafEntry *oldEntryPtr=getRouteTableTreeLeaf_EntryPtr(leafProxy->dataBlock);
	RouteTableTreeLeafEntry *newEntryPtr=getRouteTableTreeLeaf_EntryPtr(newLeafProxy->dataBlock);

	memcpy(newEntryPtr, oldEntryPtr+toKeepHalfEntry, sizeof(RouteTableTreeLeafEntry)*toMoveHalfEntry);

	//memset(leaf->dataBlock->entries+halfEntry, 0, sizeof(RouteTableTreeLeafEntry)*otherHalfEntry);

	leafProxy->entryCount=toKeepHalfEntry;
	newLeafProxy->entryCount=toMoveHalfEntry;


	//LOG(LOG_INFO,"Clearing New Leaf %i %i",newLeaf->entryCount, newLeaf->entryAlloc);
	for(int i=newLeafProxy->entryCount; i<newLeafProxy->entryAlloc; i++)
		{
		newEntryPtr[i].downstream=-1;
		newEntryPtr[i].width=0;
		}

	//LOG(LOG_INFO,"Clearing Old Leaf %i %i",leaf->entryCount, leaf->entryAlloc);
	for(int i=leafProxy->entryCount; i<leafProxy->entryAlloc; i++)
		{
		oldEntryPtr[i].downstream=-1;
		oldEntryPtr[i].width=0;
		}


	newLeafProxy->dataBlock->upstream=leafProxy->dataBlock->upstream;

	if(leafEntryPosition<toKeepHalfEntry)
		{
		*newLeafProxyPtr=leafProxy;
		*newEntryPositionPtr=leafEntryPosition;
		}
	else
		{
		*newLeafProxyPtr=newLeafProxy;
		*newEntryPositionPtr=leafEntryPosition-toKeepHalfEntry;
		}

	return newLeafProxy;
}





// Insert the given branch child to the branch parent, splitting the parent if needed


void treeProxyInsertBranchChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeBranchProxy *childBranchProxy, s16 childPosition,
		RouteTableTreeBranchProxy **newParentBranchProxyPtr, s16 *newChildPositionPtr)
{
//	LOG(LOG_INFO,"Append Branch Child P: %i C: %i",parent->brindex, child->brindex);

/*
	LOG(LOG_INFO,"Parent %i contains %i of %i",parent->brindex, parent->childCount, parent->childAlloc);

	for(int i=0;i<parent->childCount;i++)
		{
		LOG(LOG_INFO,"Child %i is %i",i,parent->dataBlock->childNindex[i]);
		}
*/

	if(parentBranchProxy->childCount>=parentBranchProxy->childAlloc) // No Space
		{
		if(parentBranchProxy->childAlloc<ROUTE_TABLE_TREE_BRANCH_CHILDREN) // Expand
			{
			expandRouteTableTreeBranchProxy(treeProxy, parentBranchProxy);
			}
		else if(parentBranchProxy->brindex!=BRANCH_NINDEX_ROOT) // Add sibling
			{
//			LOG(LOG_INFO,"AppendBranch %i: Sibling",parent->brindex);

			RouteTableTreeBranchProxy *grandParentBranchProxy;
			s32 grandParentBrindex=parentBranchProxy->dataBlock->parentBrindex;

			if(grandParentBrindex!=BRANCH_NINDEX_ROOT)
				grandParentBranchProxy=getRouteTableTreeBranchProxy(treeProxy, grandParentBrindex);
			else
				grandParentBranchProxy=treeProxy->rootProxy;

			s16 parentSibdex=getBranchChildSibdex_Branch(grandParentBranchProxy, parentBranchProxy);
			RouteTableTreeBranchProxy *newParentBranchProxy=treeProxySplitBranch(treeProxy, parentBranchProxy, childPosition, &parentBranchProxy, &childPosition);

			RouteTableTreeBranchProxy *dummyParentBranchProxy; // Don't track splits of grandparent / position
			s16 dummyChildPositionPtr;

			treeProxyInsertBranchChild(treeProxy, grandParentBranchProxy, newParentBranchProxy, parentSibdex+1, &dummyParentBranchProxy, &dummyChildPositionPtr);

			}
		else	// Split root
			{
//			LOG(LOG_INFO,"InsertBranchChild %i: Split Root",parent->brindex);

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

	branchMakeChildInsertSpace(parentBranchProxy, childPosition, 1);

	parentBranchProxy->dataBlock->childNindex[childPosition]=childBrindex;
	childBranchProxy->dataBlock->parentBrindex=parentBrindex;

	*newParentBranchProxyPtr=parentBranchProxy;
	*newChildPositionPtr=childPosition;
/*
	LOG(LOG_INFO,"ParentNow %i contains %i of %i",parent->brindex, parent->childCount, parent->childAlloc);

	for(int i=0;i<parent->childCount;i++)
		{
		LOG(LOG_INFO,"ChildNow %i is %i",i,parent->dataBlock->childNindex[i]);
		}
*/
}

// Insert the given leaf child into the branch parent, splitting the parent if needed
void treeProxyInsertLeafChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeLeafProxy *childLeafProxy, s16 childPosition,
		RouteTableTreeBranchProxy **newParentBranchProxyPtr, s16 *newChildPositionPtr)
{
	//LOG(LOG_INFO,"Append Leaf Child P: %i C: %i",parent->brindex, child->lindex);

	//LOG(LOG_INFO,"Parent contains %i of %i",parent->childCount, parent->childAlloc);

	if(parentBranchProxy->childCount>=parentBranchProxy->childAlloc)
		{
		if(parentBranchProxy->childAlloc<ROUTE_TABLE_TREE_BRANCH_CHILDREN)
			{
			expandRouteTableTreeBranchProxy(treeProxy, parentBranchProxy);
			}
		else if(parentBranchProxy->brindex!=BRANCH_NINDEX_ROOT)
			{
			RouteTableTreeBranchProxy *grandParentBranchProxy;
			s32 grandParentBrindex=parentBranchProxy->dataBlock->parentBrindex;

			if(grandParentBrindex!=BRANCH_NINDEX_ROOT)
				grandParentBranchProxy=getRouteTableTreeBranchProxy(treeProxy, grandParentBrindex);
			else
				grandParentBranchProxy=treeProxy->rootProxy;

			s16 parentSibdex=getBranchChildSibdex_Branch(grandParentBranchProxy, parentBranchProxy);
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
	branchMakeChildInsertSpace(parentBranchProxy, childPosition, 1);

	parentBranchProxy->dataBlock->childNindex[childPosition]=LINDEX_TO_NINDEX(childLindex);
	childLeafProxy->dataBlock->parentBrindex=parentBrindex;

	*newParentBranchProxyPtr=parentBranchProxy;
	*newChildPositionPtr=childPosition;

//	LOG(LOG_INFO,"leafEntry(childPosition) %i",childPosition);
}

void treeProxyAppendLeafChild(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy, RouteTableTreeLeafProxy *childLeafProxy,
		RouteTableTreeBranchProxy **newParentBranchProxyPtr, s16 *childPositionPtr)
{
	s16 childPosition=parentBranchProxy->childCount;

	treeProxyInsertLeafChild(treeProxy, parentBranchProxy, childLeafProxy, childPosition, newParentBranchProxyPtr, &childPosition);
	*childPositionPtr=childPosition;
}


// Split a leaf, allowing space to insert one/two leaf entries at the specified position, returning the entry index of the first space, and providing the new context

RouteTableTreeLeafProxy *treeProxySplitLeafInsertChildEntrySpace(RouteTableTreeProxy *treeProxy, RouteTableTreeBranchProxy *parentBranchProxy,
		s16 childPosition, RouteTableTreeLeafProxy *childLeafProxy, s16 insertEntryPosition, s16 insertEntryCount,
		RouteTableTreeBranchProxy **newParentBranchProxyPtr, s16 *newChildPositionPtr, RouteTableTreeLeafProxy **newChildLeafProxyPtr, s16 *newEntryPositionPtr)
{
	//RouteTableTreeLeafProxy *oldLeaf=child;
	RouteTableTreeLeafProxy *newLeafProxy=treeProxySplitLeaf(treeProxy, childLeafProxy, insertEntryPosition, insertEntryCount,  &childLeafProxy, &insertEntryPosition);

	treeProxyInsertLeafChild(treeProxy, parentBranchProxy, newLeafProxy, childPosition+1, newParentBranchProxyPtr, newChildPositionPtr);

	/*
	LOG(LOG_INFO,"treeProxySplitLeafInsertChildEntrySpace: Old");
	dumpLeafProxy(oldLeaf);

	LOG(LOG_INFO,"treeProxySplitLeafInsertChildEntrySpace: New");
	dumpLeafProxy(newLeaf);
*/
	leafMakeEntryInsertSpace(childLeafProxy, insertEntryPosition, insertEntryCount);
/*
	LOG(LOG_INFO,"Post MakeEntryInsertSpace");

	LOG(LOG_INFO,"treeProxySplitLeafInsertChildEntrySpace: Old");
	dumpLeafProxy(oldLeaf);

	LOG(LOG_INFO,"treeProxySplitLeafInsertChildEntrySpace: New");
	dumpLeafProxy(newLeaf);

	LOG(LOG_INFO,"treeProxySplitLeafInsertChildEntrySpace: Child with Space (%i)",insertEntryPosition);
	dumpLeafProxy(child);
*/
	*newChildLeafProxyPtr=childLeafProxy;
	*newEntryPositionPtr=insertEntryPosition;

	return newLeafProxy;
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

		s16 sibdex=getBranchChildSibdex_Branch(parentBranchProxy, branchProxy);
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
	s16 sibdex=getBranchChildSibdex_Leaf_withEstimate(branchProxy, leafProxy, sibdexEst);

//	LOG(LOG_INFO,"getNextLeafSibling from %i %i %i",branch->brindex, sibdex, leaf->lindex);


	if(sibdexEst!=sibdex)
		{
		LOG(LOG_CRITICAL,"Sibdex did not match expected Est: %i Actual: %i",sibdexEst, sibdex);
		}

	sibdex++;
	if(sibdex<branchProxy->childCount)
		{
//		LOG(LOG_INFO,"In branch %i, looking at sib %i giving %i",branch->brindex, sibdex, branch->dataBlock->childNindex[sibdex]);

		*branchChildSibdex=sibdex;
		*leafProxyPtr=getRouteTableTreeLeafProxy(treeProxy, NINDEX_TO_LINDEX(branchProxy->dataBlock->childNindex[sibdex]));
		return 1;
		}

	if(getNextBranchSibling(treeProxy, &branchProxy)) // Moved branch
		{
		*branchProxyPtr=branchProxy;
		if(branchProxy->childCount>0)
			{
//			LOG(LOG_INFO,"Moved to new branch");
			sibdex=0;
			}
		else
			{
			sibdex=-1;
			LOG(LOG_CRITICAL,"Moved to empty branch");
			}
		}
	else
		sibdex=-1;


	if(sibdex>=0)
		{
		//LOG(LOG_INFO,"In branch %i, looking at sib %i giving %i",branch->brindex, sibdex, branch->dataBlock->childNindex[sibdex]);

		*branchChildSibdex=sibdex;
		*leafProxyPtr=getRouteTableTreeLeafProxy(treeProxy, NINDEX_TO_LINDEX(branchProxy->dataBlock->childNindex[sibdex]));
		return 1;
		}
	else
		{
		return 0;
		}

}

