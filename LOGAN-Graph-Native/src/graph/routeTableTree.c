
#include "common.h"

const s32 ROUTE_TABLE_ROOT_SIZE[]={sizeof(RouteTableSmallRoot),sizeof(RouteTableMediumRoot),sizeof(RouteTableLargeRoot),sizeof(RouteTableHugeRoot)};

#define ROUTING_TABLE_FORWARD 0
#define ROUTING_TABLE_REVERSE 1

static void initTreeProxyElement_existing(RouteTableTreeElementProxy *proxyElement, RouteTableTreeElementProxy *parentProxy, s32 inParentIndex, u8 *data)
{
	proxyElement->next=NULL;
	proxyElement->parentProxy=parentProxy;
	proxyElement->inParentIndex=inParentIndex;

	proxyElement->blockData=data;

	s32 isLeaf=0, indexSize=0, subindexSize=0, subindex=0;

	rtDecodeNonRootBlockHeader(data, &isLeaf, &indexSize, NULL, &subindexSize, &subindex);
	int headerSize=rtGetNonRootBlockHeaderSize(indexSize, subindexSize);

	if(isLeaf)
		{
		proxyElement->branchData=NULL;
		proxyElement->leafData=(RouteTableLeaf *)(data+headerSize);

		int i=0;
		while(i<ROUTE_TABLE_TREE_LEAF_ENTRIES && proxyElement->leafData->entries[i].width > 0)
			i++;

		proxyElement->leafEntryCount=i;
		}
	else
		{
		proxyElement->branchData=(RouteTableBranch *)(data+headerSize);
		proxyElement->leafData=NULL;
		}

	proxyElement->childProxies=NULL;
}

//static
void initTreeProxyElement_newLeaf(RouteTableTreeElementProxy *proxyElement, RouteTableTreeElementProxy *parentProxy, s32 inParentIndex, MemDispenser *disp)
{
	proxyElement->next=NULL;
	proxyElement->parentProxy=parentProxy;
	proxyElement->inParentIndex=inParentIndex;

	proxyElement->blockData=NULL;

	proxyElement->branchData=NULL;

	proxyElement->childProxies=NULL;
	proxyElement->childProxyCount=0;

	proxyElement->leafData=dAlloc(disp, sizeof(RouteTableLeaf));
	memset(proxyElement->leafData, 0, sizeof(RouteTableLeaf));

	proxyElement->leafEntryCount=0;
}

void initTreeProxyElement_newBranch(RouteTableTreeElementProxy *proxyElement, RouteTableTreeElementProxy *parentProxy, s32 inParentIndex, MemDispenser *disp)
{
	proxyElement->next=NULL;
	proxyElement->parentProxy=parentProxy;
	proxyElement->inParentIndex=inParentIndex;

	proxyElement->blockData=NULL;

	proxyElement->branchData=dAlloc(disp, sizeof(RouteTableBranch));
	memset(proxyElement->branchData, 0, sizeof(RouteTableBranch));

	proxyElement->childProxies=NULL;
	proxyElement->childProxyCount=0;

	proxyElement->leafData=NULL;

	proxyElement->leafEntryCount=0;
}

void expandProxyElementChildren(RouteTableTreeElementProxy *proxyElement, MemDispenser *disp)
{
	if(proxyElement->branchData==NULL)
		{
		LOG(LOG_CRITICAL,"Cannot expand children elements on leaf node");
		}

	if(proxyElement->childProxies!=NULL)
		{
		LOG(LOG_CRITICAL,"Children elements already expanded on node");
		}

	struct routeTableTreeElementProxyStr **childProxies=dAlloc(disp, sizeof(struct routeTableTreeElementProxyStr *)*ROUTE_TABLE_TREE_BRANCH_CHILDREN);
	proxyElement->childProxies=childProxies;

	int i=0;

	while(i<ROUTE_TABLE_TREE_BRANCH_CHILDREN && proxyElement->branchData->childData[i]!=NULL)
		{
		struct routeTableTreeElementProxyStr *childProxy=dAlloc(disp, sizeof(struct routeTableTreeElementProxyStr *));
		initTreeProxyElement_existing(childProxy, proxyElement, i, proxyElement->branchData->childData[i]);
		childProxies[i++]=childProxy;
		}

	proxyElement->childProxyCount=i;

	while(i<ROUTE_TABLE_TREE_BRANCH_CHILDREN)
		childProxies[i++]=NULL;

}


void walkerReset(RouteTableTreeWalker *walker)
{
	walker->rootIndex=0;

	walker->firstBranchProxy=NULL;
	walker->firstBranchIndex=0;

	walker->secondBranchProxy=NULL;
	walker->secondBranchIndex=0;

	walker->leafProxy=NULL;
	walker->leafIndex=0;

}


void initTreeWalker(RouteTableTreeProxy *treeProxy, RouteTableTreeWalker *walker)
{
	walker->treeProxy=treeProxy;
	walkerReset(walker);
}


RouteTableTreeElementProxy *walkerInsertLeafEmpty(RouteTableTreeWalker *walker, MemDispenser *disp)
{
	RouteTableTreeElementProxy *leaf=dAlloc(disp, sizeof(RouteTableTreeElementProxy));
	initTreeProxyElement_newLeaf(leaf, NULL, 0, disp);

	walker->treeProxy->directChildren[0]=leaf;
	walker->treeProxy->directChildrenCount=1;

	walker->leafProxy=leaf;
	walker->leafIndex=0;

	return leaf;
}


// Insert extra tree level by splitting root. Result is a single-child root. Existing children demoted
// Existing children maintain indexes, and gain parent proxies
// Walker positions/proxies shift due to additional level

void walker_GrowTree(RouteTableTreeWalker *walker, MemDispenser *disp)
{
	if(walker->secondBranchProxy!=NULL)
		{
		LOG(LOG_CRITICAL,"Tree is already maximum height");
		}

	RouteTableTreeElementProxy *newBranch=dAlloc(disp,sizeof(RouteTableTreeElementProxy));

	initTreeProxyElement_newBranch(newBranch, NULL, 0, disp);
	newBranch->childProxies=dAlloc(disp, sizeof(struct routeTableTreeElementProxyStr *));

	s32 directChildrenCount=walker->treeProxy->directChildrenCount;

	for(int i=0;i<directChildrenCount;i++)
		{
		RouteTableTreeElementProxy *childProxy=walker->treeProxy->directChildren[i];

		newBranch->childProxies[i]=childProxy;
		newBranch->branchData->childData[i]=childProxy->blockData;

		childProxy->parentProxy=newBranch;
		walker->treeProxy->directChildren[i]=NULL;
		}

	newBranch->childProxyCount=directChildrenCount;

	walker->treeProxy->directChildren[0]=newBranch;
	walker->treeProxy->directChildrenCount=1;

	walker->treeProxy->contentDirty=1;

	walker->secondBranchIndex=walker->firstBranchIndex;
	walker->secondBranchProxy=walker->firstBranchProxy;

	walker->firstBranchIndex=walker->rootIndex;
	walker->firstBranchProxy=newBranch;

	walker->rootIndex=0;
}


// Insert extra element into a non-full node with Existing children indexes shuffled as needed

void walker_BranchInsert(RouteTableTreeElementProxy *parent, RouteTableTreeElementProxy *newElement, s32 position)
{

}

// Add an entry at the second level (child of root). May result in adding a level if root is full

int walker_WidenTree_2Level(RouteTableTreeWalker *walker, RouteTableTreeElementProxy *newElement, s32 position, MemDispenser *disp) // Insert additional root children
{
	int currentDirectCount=walker->treeProxy->directChildrenCount;

	if(position>currentDirectCount)
		{
		LOG(LOG_CRITICAL,"Attempt to add nodes beyond end of parent");
		}

	int newCount=currentDirectCount+1;
	if(newCount>ROUTE_TABLE_TREE_HUGEROOT_ENTRIES)
		{
		walker_GrowTree(walker, disp);

		RouteTableTreeElementProxy *newBranch=walker->treeProxy->directChildren[0];

		return 1;
		}

	for(int i=currentDirectCount;i>position;i--)
		{
		walker->treeProxy->directChildren[i]=walker->treeProxy->directChildren[i-1];
		walker->treeProxy->directChildren[i]->inParentIndex=i;
		}

	walker->treeProxy->directChildrenCount=newCount;
	walker->rootIndex=position;

	return 0;
}

int walker_SplitTree_3Level(RouteTableTreeWalker *walker, RouteTableTreeElementProxy *parent,
		RouteTableTreeElementProxy *newElement, s32 position, MemDispenser *disp) // Perform split of L1 branch
{
	return 0;
}



int walker_SplitTree_4Level(RouteTableTreeWalker *walker, RouteTableTreeElementProxy *parent,
		RouteTableTreeElementProxy *newElement, s32 position, MemDispenser *disp) // Perform split of L2 branch
{
	return 0;
}


RouteTableTreeElementProxy *walkerInsertLeafAfter(RouteTableTreeWalker *walker)
{
	return NULL;

	//RouteTableTreeElementProxy *currentLeaf=walker->leafProxy;

	/*
	if(walker->secondBranchProxy) // 4-level tree
		{
		int insertPosition=walker->secondBranchIndex+1;
		int newCount=walker->secondBranchProxy->childProxyCount+1;

		if(newCount>=ROUTE_TABLE_TREE_LEAF_ENTRIES) // No space in second-level branch parent
			{



			}

		}
*/


}


void walkerAppendPreorderedEntry(RouteTableTreeWalker *walker, RouteTableEntry *entry, int routingTable)
{
	MemDispenser *disp=walker->treeProxy->disp;

	int upstream=routingTable==ROUTING_TABLE_FORWARD?entry->prefix:entry->suffix;
	//int downstream=routingTable==ROUTING_TABLE_FORWARD?entry->suffix:entry->prefix;

	RouteTableTreeElementProxy *leaf=walker->leafProxy;

	if(walker->leafProxy==NULL) // Possibilities: No more tree (root with no (remaining) children), tree with one, two or 3 levels
		{
		int childCount=walker->treeProxy->directChildrenCount;
		if(childCount==0) // Empty Tree
			{
			leaf=walkerInsertLeafEmpty(walker,disp);
			leaf->leafData->upstream=upstream;
			}
		else
			{
			walker->rootIndex=childCount-1;
			RouteTableTreeElementProxy *rootChild=walker->treeProxy->directChildren[walker->rootIndex];

			if(rootChild->leafData==NULL) // Multi level tree
				{
				walker->firstBranchProxy=rootChild;

				if(rootChild->childProxies==NULL)
					expandProxyElementChildren(rootChild, disp);

				walker->firstBranchIndex=rootChild->childProxyCount-1;
				if(walker->firstBranchIndex<0)
					{
					LOG(LOG_CRITICAL,"Found empty first level branch node");
					}

				RouteTableTreeElementProxy *firstBranchChild=rootChild->childProxies[walker->firstBranchIndex];
				if(firstBranchChild->leafData==NULL) // Two branch levels
					{
					walker->secondBranchProxy=firstBranchChild;

					if(firstBranchChild->childProxies==NULL)
						expandProxyElementChildren(firstBranchChild, disp);

					walker->secondBranchIndex=firstBranchChild->childProxyCount-1;
					if(walker->secondBranchIndex<0)
						{
						LOG(LOG_CRITICAL,"Found empty second level branch node");
						}

					RouteTableTreeElementProxy *secondBranchChild=firstBranchChild->childProxies[walker->secondBranchIndex];
					leaf=secondBranchChild;
					}
				else
					leaf=firstBranchChild;
				}
			else
				leaf=rootChild;

			if(leaf->leafData!=NULL)
				{
				walker->leafProxy=leaf;
				walker->leafIndex=leaf->leafEntryCount;
				}
			else
				{
				LOG(LOG_INFO,"Failed to find leaf")
				}
			}
		}

	if((leaf->leafData->upstream!=upstream) || walker->leafIndex>=ROUTE_TABLE_TREE_LEAF_ENTRIES)
		{
		LOG(LOG_CRITICAL,"Not implemented - need leaf append");
		}

	LOG(LOG_INFO,"Indexes are %i %i %i %i",walker->rootIndex,walker->firstBranchIndex,walker->secondBranchIndex, walker->leafIndex);
	LOG(LOG_CRITICAL,"Got this far: leaf is %p",leaf);

	// All walker fields should now be valid

}


void walkerAppendPreorderedEntries(RouteTableTreeWalker *walker, RouteTableEntry *entries, u32 entryCount, int routingTable)
{
	walkerReset(walker);

	for(int i=0;i<entryCount;i++)
		walkerAppendPreorderedEntry(walker,entries+i, routingTable);

}


static void initTreeProxy(RouteTableTreeProxy *proxy, u8 *blockData[], s32 routeDataCount, MemDispenser *disp)
{
	proxy->disp=disp;

	for(int i=0;i<routeDataCount;i++)
		{
		proxy->directChildren[i]=dAlloc(disp, sizeof(RouteTableTreeElementProxy));
		initTreeProxyElement_existing(proxy->directChildren[i], NULL, i, blockData[i]);
		}

	for(int i=routeDataCount; i< ROUTE_TABLE_TREE_HUGEROOT_ENTRIES; i++)
		proxy->directChildren[i]=NULL;

	proxy->directChildrenCount=routeDataCount;

	proxy->newBranches=NULL;
	proxy->newLeaves=NULL;
}





void rttInitRouteTableTreeBuilder(RouteTableTreeBuilder *builder, HeapDataBlock *dataBlock, MemDispenser *disp)
{
	builder->disp=disp;
	builder->newEntryCount=0;

	LOG(LOG_CRITICAL,"Not implemented: rttInitRouteTableTreeBuilder");

}

void rttUpgradeToRouteTableTreeBuilder(RouteTableArrayBuilder *arrayBuilder,  RouteTableTreeBuilder *treeBuilder, MemDispenser *disp)
{
	treeBuilder->disp=arrayBuilder->disp;
	treeBuilder->rootBlockPtr=NULL;
	treeBuilder->rootSize=0;

	initTreeProxy(&(treeBuilder->forwardProxy), NULL, 0, disp);
	initTreeProxy(&(treeBuilder->reverseProxy), NULL, 0, disp);

	initTreeWalker(&(treeBuilder->forwardProxy), &(treeBuilder->forwardWalker));
	initTreeWalker(&(treeBuilder->reverseProxy), &(treeBuilder->reverseWalker));

	walkerAppendPreorderedEntries(&(treeBuilder->forwardWalker), arrayBuilder->oldForwardEntries, arrayBuilder->oldForwardEntryCount, ROUTING_TABLE_FORWARD);
	walkerAppendPreorderedEntries(&(treeBuilder->reverseWalker), arrayBuilder->oldReverseEntries, arrayBuilder->oldReverseEntryCount, ROUTING_TABLE_REVERSE);

	treeBuilder->newEntryCount=arrayBuilder->oldForwardEntryCount+arrayBuilder->oldReverseEntryCount;

	LOG(LOG_CRITICAL,"Not implemented: rttUpgradeToRouteTableTreeBuilder");
}


void rttDumpRoutingTable(RouteTableTreeBuilder *builder)
{
	LOG(LOG_CRITICAL,"Not implemented: rttDumpRoutingTable");
}

s32 rttGetRouteTableTreeBuilderDirty(RouteTableTreeBuilder *builder)
{
	return builder->newEntryCount>0;
}

s32 rttGetRouteTableTreeBuilderPackedSize(RouteTableTreeBuilder *builder)
{
	return ROUTE_TABLE_ROOT_SIZE[builder->rootSize];
}

u8 *rttWriteRouteTableTreeBuilderPackedData(RouteTableTreeBuilder *builder, u8 *data)
{
	LOG(LOG_CRITICAL,"Not implemented: rttWriteRouteTableTreeBuilderPackedData");

	return data;
}

void rttMergeRoutes(RouteTableTreeBuilder *builder,
		RoutePatch *forwardRoutePatches, RoutePatch *reverseRoutePatches, s32 forwardRoutePatchCount, s32 reverseRoutePatchCount,
		s32 maxNewPrefix, s32 maxNewSuffix, RoutingReadData **orderedDispatches, MemDispenser *disp)
{
	LOG(LOG_CRITICAL,"Not implemented: rttMergeRoutes");

	}

void rttUnpackRouteTableForSmerLinked(SmerLinked *smerLinked, u8 *data, MemDispenser *disp)
{
	LOG(LOG_CRITICAL,"Not implemented: rttUnpackRouteTableForSmerLinked");
}

