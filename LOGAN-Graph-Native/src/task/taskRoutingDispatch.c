/*
 * taskRoutingDispatch.c
 *
 *  Created on: Mar 3, 2016
 *      Author: tony
 */

#include "../common.h"



int reserveReadDispatchBlock(RoutingBuilder *rb)
{
	int allocated=rb->allocatedReadDispatchBlocks;
	int checkAllocated;

	if(allocated>=TR_READBLOCK_DISPATCHES_INFLIGHT)
		return 0;

	while(!__sync_bool_compare_and_swap(&(rb->allocatedReadDispatchBlocks),allocated,allocated+1))
		{
		allocated=rb->allocatedReadDispatchBlocks;

		if(allocated>=TR_READBLOCK_DISPATCHES_INFLIGHT)
			return 0;
		}

	return 1;
}


void unreserveReadDispatchBlock(RoutingBuilder *rb)
{
	__sync_fetch_and_add(&(rb->allocatedReadDispatchBlocks),-1);
}

