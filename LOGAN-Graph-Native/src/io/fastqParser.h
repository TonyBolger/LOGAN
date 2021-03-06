/*
 * smer.h
 *
 *  Created on: Jun 11, 2014
 *      Author: tony
 */

#ifndef __FASTQ_PARSER_H
#define __FASTQ_PARSER_H

#define FASTQ_PARSER_ERROR_OVERLONG_LINE_LENGTH -1
#define FASTQ_PARSER_ERROR_UNEXPECTED_EOF -2
#define FASTQ_PARSER_ERROR_LENGTH_MISMATCH -3

s64 fqParseAndProcess(char *path, int minSeqLength, s64 recordsToSkip, s64 recordsToUse,
		u8 *ioBuffer, int ioBufferRecycleSize, int ioBufferPrimarySize,
		SwqBuffer *swqBuffers, ParallelTaskIngress *ingressBuffers, int bufferCount,
		void *handlerContext, void (*handler)(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *handlerContext),
		void (*monitor)());


#endif
