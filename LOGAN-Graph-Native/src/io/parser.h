#ifndef __PARSER_H
#define __PARSER_H


#define PARSER_MIN_SEQ_LENGTH 40
#define PARSER_MAX_SEQ_LENGTH 1000000


// Record count can be larger than ingress blocksize, but base limit must match
#define PARSER_SEQ_PER_BATCH 100000
#define PARSER_BASES_PER_BATCH TASK_INGRESS_BASESTOTAL

// RECYCLE_BUFFER should be at least FASTQ_MAX_READ_LENGTH * 4

#define PARSER_IO_RECYCLE_BUFFER (4*1024*1024)
#define PARSER_IO_PRIMARY_BUFFER (10*1024*1024)






void prInitSequenceBuffer(SwqBuffer *swqBuffer, int bufSize, int recordsPerBatch, int maxReadLength);
void prFreeSequenceBuffer(SwqBuffer *swqBuffer);

void prWaitForBufferIdle(int *usageCount);

void prIndexingBuilderDataHandler(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *context);
void prRoutingBuilderDataHandler(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *context);

#endif
