/*
 * implicitTree.h
 *
 *  Created on: Jun 08, 2014
 *      Author: tony
 */

#ifndef __IMPLICITTREE_H
#define __IMPLICITTREE_H

#include "../common.h"


SmerEntry *siitInitImplicitTree(SmerEntry *sortedSmers, u32 smerCount);
void siitFreeImplicitTree(SmerEntry *smerIds);

int siitFindSmer(SmerEntry *smerIA, u32 smerCount, SmerEntry key);

#endif


