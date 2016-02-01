/*
 * fastqParser.c
 *
 *  Created on: Jun 11, 2014
 *      Author: tony
 */


#include "cliCommon.h"


static int readFastqLine(FILE *file, char *ptr, int maxLength)
{
	int count=0;
	int ch;

	maxLength--; // Allow for null byte for N check

	ch=getc_unlocked(file);
	while(ch!=EOF && ch!=10 && ch!=13 && count < maxLength) // 10=LineFeed, 13 = CarriageReturn
		{
		ptr[count++]=ch;
		ch=getc_unlocked(file);
		}

	ptr[count]=0;

	if(count==maxLength)
		{
		return -1;
		}

	if(ch==13)	// Handle stupid newlines
		{
		ch=getc_unlocked(file);
		if(ch!=10 && ch!=EOF)
			ungetc(ch, file);
		}

	return count;
}

static int skipFastqLine(FILE *file)
{
	int ch;
	int count=0;

	ch=getc_unlocked(file);
	while(ch!=EOF && ch!=10 && ch!=13) // 10=LineFeed, 13 = CarriageReturn
		{
		count++;
		ch=getc_unlocked(file);
		}

	if(ch==13)	// Handle stupid newlines
		{
		ch=getc_unlocked(file);
		if(ch!=10 && ch!=EOF)
			ungetc(ch, file);
		}

	return count;
}


int readFastqRecord(FILE *file, SequenceWithQuality *rec, int maxLength)
{
	int len1,len2;

	rec->length=0;

	if(skipFastqLine(file)==0)	// Read Name
		return 0;

	len1=readFastqLine(file,rec->seq,maxLength);
	if(len1<=0)
		return len1;

	if(skipFastqLine(file)==0)
		return -2; // Read Comment

	len2=readFastqLine(file,rec->qual,maxLength);

	if(len2<=0)
		return len2;

	if(len1!=len2)
		return -3;

	rec->length=len1;

	return len1;
}

//#define FASTQ_RECORDS_PER_BATCH 100000
//#define FASTQ_BASES_PER_BATCH 10000000


int parseAndProcess(char *path, int minSeqLength, int recordsToSkip, int recordsToUse, void *handlerContext, void (*handler)(SequenceWithQuality *rec, int numRecords, void *handlerContext))
{
	FILE *file=fopen(path,"r");

	if(file==NULL)
		{
		LOG(LOG_INFO,"Failed to open input file: '%s'",path);
		return 0;
		}

	int bufSize=FASTQ_BASES_PER_BATCH;
	int currentBuffer=0;

	char *seqBuffer[PT_INGRESS_BUFFERS];
	char *qualBuffer[PT_INGRESS_BUFFERS];
	SequenceWithQuality *rec[PT_INGRESS_BUFFERS];

	for(int i=0;i<PT_INGRESS_BUFFERS;i++)
		{
		seqBuffer[i]=malloc(bufSize);
		qualBuffer[i]=malloc(bufSize);

		rec[i]=malloc(sizeof(SequenceWithQuality)*FASTQ_RECORDS_PER_BATCH);
		}

	int batchReadCount=0;
	long batchBaseCount=0;

	int allRecordCount=0,validRecordCount=0,usedRecords=0;

	int lastRecord=recordsToSkip+recordsToUse;

	rec[currentBuffer][batchReadCount].seq=seqBuffer[currentBuffer];
	rec[currentBuffer][batchReadCount].qual=qualBuffer[currentBuffer];

	int len=readFastqRecord(file, rec[currentBuffer], FASTQ_MAX_READ_LENGTH);

	int totalBatch=0;

	while(len>0 && validRecordCount<lastRecord)
		{
		char *seq=rec[currentBuffer][batchReadCount].seq;
		if((len>=minSeqLength) && (strchr(seq,'N')==NULL))
			{
			if(validRecordCount>=recordsToSkip)
				{
				batchReadCount++;
				batchBaseCount+=len;

				if((batchReadCount>=FASTQ_RECORDS_PER_BATCH) || batchBaseCount>=(FASTQ_BASES_PER_BATCH-FASTQ_MAX_READ_LENGTH))
					{
					(*handler)(rec[currentBuffer],batchReadCount,handlerContext);
					totalBatch+=batchReadCount;
					batchReadCount=0;
					batchBaseCount=0;

					currentBuffer=(currentBuffer+1)%PT_INGRESS_BUFFERS;
					}

				usedRecords++;

				if(usedRecords % 1000000 ==0)
					LOG(LOG_INFO,"Reads: %i", usedRecords);
				}

			validRecordCount++;
			}

		allRecordCount++;

		rec[currentBuffer][batchReadCount].seq=seqBuffer[currentBuffer]+batchBaseCount;
		rec[currentBuffer][batchReadCount].qual=qualBuffer[currentBuffer]+batchBaseCount;
		len=readFastqRecord(file, rec[currentBuffer]+batchReadCount, FASTQ_MAX_READ_LENGTH);
		}

	if(batchReadCount>0)
		{
		(*handler)(rec[currentBuffer],batchReadCount,handlerContext);
		totalBatch+=batchReadCount;
		}

	LOG(LOG_INFO,"Used %i %i",usedRecords,totalBatch);

	if(len<0)
		{
		switch (len)
			{
			case -1:
				LOG(LOG_INFO,"Over-long Seq/Qual in Fastq record");
				break;

			case -2:
				LOG(LOG_INFO,"Unexpected EOF in Fastq record");
				break;

			case -3:
				LOG(LOG_INFO,"Seq/Qual length mismatch in Fastq record");
				break;

			}
		}

	for(int i=0;i<PT_INGRESS_BUFFERS;i++)
		{
		free(seqBuffer[i]);
		free(qualBuffer[i]);
		free(rec[i]);
		}

	fclose(file);

	return usedRecords;
}

