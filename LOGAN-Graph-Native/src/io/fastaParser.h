/*
 * smer.h
 *
 *  Created on: Jun 11, 2014
 *      Author: tony
 */

#ifndef __FASTA_PARSER_H
#define __FASTA_PARSER_H

#define FASTA_SEQUENCE_HEADER_MAX_LENGTH 250
#define FASTA_SEQUENCE_NAME_MAX_LENGTH 250

#define FASTA_SEQUENCE_TRIM_MAX_LENGTH 1000

#define FASTA_SEQUENCE_VALID_MAX_LENGTH 1050
#define FASTA_SEQUENCE_VALID_OVERLAP 50

typedef struct fastaParserStateStr {
	int parserState;
	char sequenceName[FASTA_SEQUENCE_NAME_MAX_LENGTH];
	int sequencePosition;
	int fragmentStartPosition;
	SequenceWithQuality *currentSequenceFragment;
} FastaParserState;


s64 faParseAndProcess(char *path, int minSeqLength, s64 recordsToSkip, s64 recordsToUse,
		u8 *ioBuffer, int ioBufferRecycleSize, int ioBufferPrimarySize,
		SwqBuffer *swqBuffers, ParallelTaskIngress *ingressBuffers, int bufferCount,
		void *handlerContext, void (*handler)(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *handlerContext),
		void (*monitor)(), SequenceIndex *seqIndex);


#endif
