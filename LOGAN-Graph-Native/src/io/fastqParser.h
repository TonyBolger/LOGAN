/*
 * smer.h
 *
 *  Created on: Jun 11, 2014
 *      Author: tony
 */

#ifndef __FASTQ_PARSER_H
#define __FASTQ_PARSER_H



#define FASTQ_MIN_READ_LENGTH 40
#define FASTQ_MAX_READ_LENGTH 1000000

//#define FASTQ_RECORDS_PER_BATCH 100
//#define FASTQ_BASES_PER_BATCH 10000

//#define FASTQ_RECORDS_PER_BATCH 1000
//#define FASTQ_BASES_PER_BATCH 100000

//#define FASTQ_RECORDS_PER_BATCH 10000
//#define FASTQ_BASES_PER_BATCH 1000000

#define FASTQ_RECORDS_PER_BATCH 100000
#define FASTQ_BASES_PER_BATCH 10000000

// RECYCLE_BUFFER should be at least FASTQ_MAX_READ_LENGTH * 4

#define FASTQ_IO_RECYCLE_BUFFER (64*1024)
#define FASTQ_IO_PRIMARY_BUFFER (1024*1024)


void initSequenceBuffer(SwqBuffer *swqBuffer, int bufSize, int recordsPerBatch, int maxReadLength);
void freeSequenceBuffer(SwqBuffer *swqBuffer);


void indexingBuilderDataHandler(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *context);
void routingBuilderDataHandler(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *context);


int parseAndProcess(char *path, int minSeqLength, int recordsToSkip, int recordsToUse,
		u8 *ioBuffer, int ioBufferRecycleSize, int ioBufferPrimarySize,
		SwqBuffer *swqBuffers, ParallelTaskIngress *ingressBuffers, int bufferCount,
		void *handlerContext, void (*handler)(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *handlerContext));


#endif
