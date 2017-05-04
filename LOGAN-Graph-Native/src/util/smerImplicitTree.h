/*
 * implicitTree.h
 *
 *  Created on: Jun 08, 2014
 *      Author: tony
 */

#ifndef __IMPLICITTREE_H
#define __IMPLICITTREE_H


SmerEntry *siitInitImplicitTree(SmerEntry *sortedSmers, u32 smerCount);
void siitFreeImplicitTree(SmerEntry *smerIds, u32 smerCount);

int siitFindSmer(SmerEntry *smerIA, u32 smerCount, SmerEntry key);

#endif


