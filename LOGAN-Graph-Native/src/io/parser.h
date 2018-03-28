#ifndef __PARSER_H
#define __PARSER_H


#define PARSER_MIN_SEQ_LENGTH 40
#define PARSER_MAX_SEQ_LENGTH 1000000

#define PARSER_MAX_TAG_LENGTH 4


// Record count can be larger than ingress blocksize, but base limit must match
#define PARSER_SEQ_PER_BATCH 100000
#define PARSER_BASES_PER_BATCH TASK_INGRESS_BASESTOTAL
#define PARSER_TAG_PER_BATCH (PARSER_SEQ_PER_BATCH*PARSER_MAX_TAG_LENGTH)


// RECYCLE_BUFFER should be at least FASTQ_MAX_READ_LENGTH * 4

#define PARSER_IO_RECYCLE_BUFFER (4*1024*1024)
#define PARSER_IO_PRIMARY_BUFFER (10*1024*1024)

// File Content and compression can be combined
#define PARSE_FILE_CONTENT_MASK 0xFF

#define PARSE_FILE_CONTENT_UNKNOWN 0
#define PARSE_FILE_CONTENT_FASTQ 1
#define PARSE_FILE_CONTENT_FASTA 2


#define PARSE_FILE_COMPRESSION_MASK 0xFF00

#define PARSE_FILE_COMPRESSION_NONE 0
#define PARSE_FILE_COMPRESSION_GZIP 256





typedef struct parseBufferStr {
	u8 *ioBuffer;

	SwqBuffer swqBuffers[PT_INGRESS_BUFFERS];
	ParallelTaskIngress ingressBuffers[PT_INGRESS_BUFFERS];
} ParseBuffer;


void prInitParseBuffer(ParseBuffer *parseBuffer);
void prFreeParseBuffer(ParseBuffer *parseBuffer);

void prWaitForSwqBufferIdle(SwqBuffer *swqBuffer);
void prWaitForParseBufferIdle(ParseBuffer *parseBuffer);

s64 prParseAndProcess(char *path, int minSeqLength, s64 recordsToSkip, s64 recordsToUse, ParseBuffer *parseBuffer,
		void *handlerContext, void (*handler)(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *handlerContext),
		void (*monitor)());

void prIndexingBuilderDataHandler(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *context);
void prRoutingBuilderDataHandler(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *context);

#endif
