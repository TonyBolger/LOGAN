/*
 * smer.h
 *
 *  Created on: Jun 11, 2014
 *      Author: tony
 */

#ifndef __FASTQ_PARSER_H
#define __FASTQ_PARSER_H



int parseAndProcess(char *path, int minSeqLength, int recordsToSkip, int recordsToUse,
		SwqBuffer *buffers, int bufferCount, void *handlerContext, void (*handler)(SwqBuffer *buffer, void *handlerContext));

#endif
