/*
 * implicitTree.c
 *
 *  Created on: Jun 8, 2014
 *      Author: tony
 */

#include "smerImplicitTree.h"

typedef struct implicitTreeLevel_str
{
	u32 startIndex;

	u32 maxCapacity;
	u32 allocCapacity;

	u32 position;
	u32 modPosition;

} ImplicitTreeLevel;

#define MAX_LEVELS 16
#define NARY 8

static int putEntry(ImplicitTreeLevel *levels, int level, SmerEntry *smerIA, SmerId smerId)
{
	int sameLevel=1;

	while(level > 0 && levels[level].modPosition==NARY)
		{
		levels[level].modPosition=0;
		level--;
		sameLevel=0;
		}

	int pos=levels[level].startIndex+levels[level].position;
	smerIA[pos]=smerId;

	//LOG(LOG_INFO,"%lli added at %i (level %i)",smerId,pos,level);

	levels[level].position++;
	levels[level].modPosition++;

	return sameLevel;
}

SmerEntry *siitInitImplicitTree(SmerEntry *sortedSmers, u32 smerCount)
{
	int paddedSmerCount=(smerCount+(NARY-1))&(~(NARY-1));

	SmerEntry *smerEntries=smSmerEntryArrayAlloc(paddedSmerCount);

	int level=0;
	ImplicitTreeLevel levels[MAX_LEVELS];
	u32 capacity=NARY;
	u32 remainingEntries=smerCount;
	u32 accumEntries=0;

	//LOG(LOG_INFO,"Tree needs %i nodes (%i without padding)",paddedSmerCount,smerCount);

	while(remainingEntries>0 && level<MAX_LEVELS)
	{
		levels[level].startIndex=accumEntries;
		levels[level].maxCapacity=capacity;

		if(remainingEntries<capacity)
			{
			levels[level].allocCapacity=remainingEntries;
			remainingEntries=0;
			accumEntries+=remainingEntries;
			}
		else
			{
			levels[level].allocCapacity=capacity;
			remainingEntries-=capacity;
			accumEntries+=capacity;
			}

		levels[level].position=0;
		levels[level].modPosition=0;

		//LOG(LOG_INFO,"Level %i max %i alloc %i",level, levels[level].maxCapacity, levels[level].allocCapacity);

		capacity*=(NARY+1);
		level++;
	}

//	LOG(LOG_INFO,"Requires %i levels",level)

	int i=0;
	int lastCount=0;

	int lastLevel=level-1;
	int lastLevelEntries=levels[lastLevel].allocCapacity;
	int penultimateLevel=lastLevel-1;


	while((lastCount<lastLevelEntries) && (i<smerCount))
		{
		lastCount+=putEntry(levels, lastLevel, smerEntries, sortedSmers[i]);
		i++;
		}

	while(i<smerCount)
	{
		putEntry(levels, penultimateLevel, smerEntries, sortedSmers[i]);
		i++;
	}

	while(i<paddedSmerCount)
	{
		putEntry(levels, lastLevel, smerEntries, SMER_DUMMY);
		i++;
	}

	return smerEntries;
}


void siitFreeImplicitTree(SmerEntry *smerEntries)
{
	smSmerEntryArrayFree(smerEntries);
}


int siitFindSmer(SmerEntry *smerIA, u32 smerCount, SmerEntry key)
{
	int nodeIndex=0;

	while(nodeIndex<smerCount)
	{
//		LOG(LOG_INFO,"Now on index %i",nodeIndex);

		/*

		for(int i=0;i<NARY;i++)
		{
			SmerId test=smerIA[nodeIndex+i];
			if(key<=test)
			{
				if(test==key)
					{
					//LOG(LOG_INFO,"Found %lli at %i",key,nodeIndex+i);
					return nodeIndex+i;
					}
				else
					{
					nodeIndex=(nodeIndex*(NARY+1))+(i+1)*NARY;
					goto skip;
					}
			}
		}
*/

		int i;

		SmerId test=smerIA[nodeIndex+3];
		if(key<=test)
			{
//			if(test==key)
//				return nodeIndex+3;
			i=1;
			}
		else
			{
			test=smerIA[nodeIndex+7];
			if(key<=test)
				{
//				if(test==key)
//					return nodeIndex+7;
				i=5;
				}
			else
				{
				nodeIndex=(nodeIndex*(NARY+1))+(NARY*NARY+NARY);
				continue;
				}
		}

		test=smerIA[nodeIndex+i]; // 1 or 5
		if(key<=test)
			{
//			if(test==key)
//				return nodeIndex+i;
			i--;	// 0 or 4
			}
		else
			i++;    // 2 or 6

		test=smerIA[nodeIndex+i];
		if(key>test)
			{
			i++; // 1 or 3 or 5 or 7
			test=smerIA[nodeIndex+i];

			// else 0 or 2 or 4 or 6
			}

		if(test==key)
			return nodeIndex+i;


		nodeIndex=(nodeIndex*(NARY+1))+(i+1)*NARY;
	}

	return -1;
}
