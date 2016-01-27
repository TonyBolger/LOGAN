/*
 * smer.h
 *
 *  Created on: Jun 11, 2014
 *      Author: tony
 */

#ifndef __FASTQ_PARSER_H
#define __FASTQ_PARSER_H

//#define FASTQ_BUFFER_SIZE 10000000

#define FASTQ_MAX_READ_LENGTH 1000000

#define FASTQ_RECORDS_PER_BATCH 100000
#define FASTQ_BASES_PER_BATCH 10000000

/*
typedef struct fileBufferStr {
	FILE *file;
	char buf[FASTQ_BUFFER_SIZE];
	int headOffset;
	int tailOffset;
} FileBuffer;
*/

typedef struct fastqRecordStr {
	char *seq;
	char *qual;
	int length;
} FastqRecord;


/*
int fastqOpenFile(FileBuffer *buffer, char *filePath);
void fastqCloseFile(FileBuffer *buffer);
int fastqNextRecord(FileBuffer *buffer, FastqRecord *rec);
*/

int parseAndProcess(char *path, int minSeqLength, int recordsToSkip, int recordsToUse, void *handlerContext, void (*handler)(FastqRecord *rec, int numRecords, void *handlerContext));

#endif
