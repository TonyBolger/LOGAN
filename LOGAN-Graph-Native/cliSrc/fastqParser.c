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


void waitForIdle(int *usageCount)
{
	while(!__sync_bool_compare_and_swap(usageCount,0,0))
		{
		struct timespec req, rem;

		req.tv_sec=0;
		req.tv_nsec=1000000;

		nanosleep(&req, &rem);
		}
}



int parseAndProcess(char *path, int minSeqLength, int recordsToSkip, int recordsToUse,
		SwqBuffer *buffers, int bufferCount, void *handlerContext, void (*handler)(SwqBuffer *buffer, void *handlerContext))
{
	FILE *file=fopen(path,"r");

	if(file==NULL)
		{
		LOG(LOG_INFO,"Failed to open input file: '%s'",path);
		return 0;
		}

	int currentBuffer=0;
	int batchReadCount=0;
	long batchBaseCount=0;

	int allRecordCount=0,validRecordCount=0,usedRecords=0;

	int lastRecord=recordsToSkip+recordsToUse;

	waitForIdle(&buffers[currentBuffer].usageCount);

	int maxReads=buffers[currentBuffer].maxSequences;
	int maxBases=buffers[currentBuffer].maxSequenceTotalLength;
	int maxBasesPerRead=buffers[currentBuffer].maxSequenceLength;

	buffers[currentBuffer].rec[batchReadCount].seq=buffers[currentBuffer].seqBuffer;
	buffers[currentBuffer].rec[batchReadCount].qual=buffers[currentBuffer].qualBuffer;

	int len = readFastqRecord(file, buffers[currentBuffer].rec, maxBasesPerRead);
	buffers[currentBuffer].rec[batchReadCount].length=len;

	int totalBatch=0;

	while(len>0 && validRecordCount<lastRecord)
		{
		char *seq=buffers[currentBuffer].rec[batchReadCount].seq;
		if((len>=minSeqLength) && (strchr(seq,'N')==NULL))
			{
			if(validRecordCount>=recordsToSkip)
				{
				batchReadCount++;
				batchBaseCount+=len;

				if((batchReadCount>=maxReads) || batchBaseCount>=(maxBases-maxBasesPerRead))
					{
					buffers[currentBuffer].numSequences=batchReadCount;

					(*handler)(buffers+currentBuffer,handlerContext);
					totalBatch+=batchReadCount;
					batchReadCount=0;
					batchBaseCount=0;

					currentBuffer=(currentBuffer+1)%PT_INGRESS_BUFFERS;


//					int usage=buffers[currentBuffer].usageCount;
//					if(usage>0)
//						LOG(LOG_INFO, "New buffer usage count was %i",usage);

					waitForIdle(&buffers[currentBuffer].usageCount);

//					if(usage>0)
//						LOG(LOG_INFO, "New buffer usage count is now %i",buffers[currentBuffer].usageCount);
					}

				usedRecords++;

				if(usedRecords % 1000000 ==0)
					LOG(LOG_INFO,"Reads: %i", usedRecords);
				}

			validRecordCount++;
			}

		allRecordCount++;

		buffers[currentBuffer].rec[batchReadCount].seq=buffers[currentBuffer].seqBuffer+batchBaseCount;
		buffers[currentBuffer].rec[batchReadCount].qual=buffers[currentBuffer].qualBuffer+batchBaseCount;

		len=readFastqRecord(file, buffers[currentBuffer].rec+batchReadCount, maxBasesPerRead);

		buffers[currentBuffer].rec[batchReadCount].length=len;
		}

	if(batchReadCount>0)
		{
		buffers[currentBuffer].numSequences=batchReadCount;

		(*handler)(buffers+currentBuffer,handlerContext);
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

	fclose(file);

	return usedRecords;
}

