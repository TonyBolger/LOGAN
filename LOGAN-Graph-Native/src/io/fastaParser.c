/*
 * fastqParser.c
 *
 *  Created on: Jun 11, 2014
 *      Author: tony
 */

#include "common.h"


static void waitForBufferIdle(int *usageCount)
{
	//LOG(LOG_INFO,"WaitForIdle");

	while(__atomic_load_n(usageCount,__ATOMIC_SEQ_CST)>0)
		{
		struct timespec req;

		req.tv_sec=DEFAULT_SLEEP_SECS;
		req.tv_nsec=DEFAULT_SLEEP_NANOS;

		nanosleep(&req, NULL);
		}

	if(__atomic_load_n(usageCount,__ATOMIC_SEQ_CST)<0)
		LOG(LOG_CRITICAL,"Negative usage");

	//LOG(LOG_INFO,"WaitForIdle Done");
}


static int readBufferedFastaLine(u8 *ioBuffer, int *offset, char *outPtr, int maxLength)
{
	u8 ch;
	u8 *inPtr=ioBuffer+*offset;
	int length=0;

	maxLength--; // Allow for null byte for N check

	ch=*(inPtr++);
	if(ch==0)
		return 0; // EOF -> Early out without updating offset

	while(ch!=0 && ch!=10 && ch!=13 && length < maxLength) // 10=LineFeed, 13 = CarriageReturn
		{
		outPtr[length++]=ch;
		ch=*(inPtr++);
		}

	outPtr[length]=0;

	if(length==maxLength)
		return -1;

	if(ch==13)	// Handle stupid newlines
		{
		ch=*(inPtr++);
		if(ch!=10 && ch!=0)
			inPtr--;
		}

	*offset=inPtr-ioBuffer;

	return length;
}

#define PARSER_STATE_HEADER 1
#define PARSER_STATE_TRIM 2
#define PARSER_STATE_SEQUENCE 3
#define PARSER_STATE_EOF 4

int readBufferedFastaRecordHeader(u8 *ioBuffer, int *offset, FastaParserState *fastaRec)
{
	memset(fastaRec, 0, sizeof(FastaParserState));

	char *headerBuf=alloca(FASTQ_SEQUENCE_HEADER_MAX_LENGTH+1);
	headerBuf[0]=0;

	int headerLength=readBufferedFastaLine(ioBuffer, offset, headerBuf, FASTQ_SEQUENCE_HEADER_MAX_LENGTH);
	headerBuf[headerLength]=0;

	LOG(LOG_INFO,"Header: '%s'",headerBuf);

	if(headerBuf[0]!='>')
		return -1;

	char *startOfName=headerBuf+1; // Start after '>'
	char *endOfName=strchr(headerBuf,' ');

	if(endOfName==NULL)
		endOfName=headerBuf+headerLength;

	int nameLength=endOfName-startOfName;

	strncpy(fastaRec->sequenceName, startOfName, nameLength);
	fastaRec->sequenceName[nameLength]=0;

	LOG(LOG_INFO,"Name: '%s'",fastaRec->sequenceName);

	fastaRec->parserState=PARSER_STATE_TRIM;

	return 0;
}



//static
int readBufferedFastaTrimSequence(u8 *ioBuffer, int *offset, int maxLength)
{
	u8 ch;

	u8 *inPtr=ioBuffer+*offset;
	int length=0;

	ch=*(inPtr++);
	if(ch==0)
		return 0; // EOF -> Early out without updating offset

	int done=0;

	while(!done && maxLength>0)
		{
		switch(ch)
			{
			case '>':
				done=PARSER_STATE_HEADER;
				inPtr--;
				break;

			case 10:	// LineFeed
			case 13:	// CarriageReturn
				done=PARSER_STATE_TRIM;
				if(ch==13)	// Handle stupid newlines
					{
					ch=*(inPtr++);
					if(ch!=10 && ch!=0)
						inPtr--;
					}
				break;

			case 'A':
			case 'C':
			case 'G':
			case 'T':
				done=PARSER_STATE_SEQUENCE;
				inPtr--;
				break;

			case 0:		// EOF
				done=PARSER_STATE_EOF;
				break;

			default:
				ch=*(inPtr++);
				maxLength--;
			}
		}

	*offset=inPtr-ioBuffer;

	return length;
}




/*
static int skipBufferedFastqLine(u8 *ioBuffer, int *offset)
{
	u8 ch;
	u8 *inPtr=ioBuffer+*offset;
	int length=0;

	ch=*(inPtr++);
	if(ch==0)
		return 0; // EOF -> Early out without updating offset

	while(ch!=0 && ch!=10 && ch!=13) // 10=LineFeed, 13 = CarriageReturn
		{
		length++;
		ch=*(inPtr++);
		}

	if(ch==13)	// Handle stupid newlines
		{
		ch=*(inPtr++);
		if(ch!=10 && ch!=0)
			inPtr--;
		}

	*offset=inPtr-ioBuffer;

	return length;
}
*/

/*
int readBufferedFastqRecord(u8 *ioBuffer, int *offset, SequenceWithQuality *rec, int maxLength)
{
	int len1,len2;

	rec->length=0;

	if(skipBufferedFastqLine(ioBuffer, offset)==0)	// Read Name
		return 0;

	len1=readBufferedFastqLine(ioBuffer, offset,rec->seq,maxLength);
	if(len1<=0)
		return len1;

	if(skipBufferedFastqLine(ioBuffer, offset)==0)
		return -2; // Read Comment

	len2=readBufferedFastqLine(ioBuffer, offset,rec->qual,maxLength);

	if(len2<=0)
		return len2;

	if(len1!=len2)
		{
		LOG(LOG_INFO,"%i vs %i  %s vs %s",len1,len2, rec->seq, rec->qual);
		return -3;
		}

	rec->length=len1;

	return len1;
}

*/





s64 faParseAndProcess(char *path, int minSeqLength, s64 recordsToSkip, s64 recordsToUse,
		u8 *ioBuffer, int ioBufferRecycleSize, int ioBufferPrimarySize,
		SwqBuffer *swqBuffers, ParallelTaskIngress *ingressBuffers, int bufferCount,
		void *handlerContext, void (*handler)(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *handlerContext),
		void (*monitor)())
{
	FILE *file=fopen(path,"r");

	if(file==NULL)
		{
		LOG(LOG_INFO,"Failed to open input file: '%s'",path);
		return 0;
		}

	s64 currentBuffer=0;
	s64 batchReadCount=0;
	/*
	s64 batchBaseCount=0;

	s64 allRecordCount=0,validRecordCount=0,usedRecords=0;
	s32 progressCounter=0;
	s64 lastRecord=recordsToSkip+recordsToUse;
*/
	waitForBufferIdle(&swqBuffers[currentBuffer].usageCount);

	/*
	int maxReads=swqBuffers[currentBuffer].maxSequences;
	int maxBases=swqBuffers[currentBuffer].maxSequenceTotalLength;
	int maxBasesPerRead=swqBuffers[currentBuffer].maxSequenceLength;
*/

	swqBuffers[currentBuffer].rec[batchReadCount].seq=swqBuffers[currentBuffer].seqBuffer;
	swqBuffers[currentBuffer].rec[batchReadCount].qual=swqBuffers[currentBuffer].qualBuffer;

	int ioOffset=0;
	int ioBufferEnd=ioBufferRecycleSize+ioBufferPrimarySize;

	int ioBufferValid=fread(ioBuffer,1,ioBufferRecycleSize+ioBufferPrimarySize,file);
	if(ioBufferValid<ioBufferEnd)
		ioBuffer[ioBufferValid]=0;

	//s64 totalBatch=0;
	//int len=0;


	FastaParserState fastaRec;
	fastaRec.parserState=PARSER_STATE_HEADER;

	while(fastaRec.parserState!=PARSER_STATE_EOF)
		{

		switch(fastaRec.parserState)
			{
			case PARSER_STATE_HEADER:
				break;

			case PARSER_STATE_TRIM:
				break;

			case PARSER_STATE_SEQUENCE:
				break;
			}

		}

	//int headerLen=




	readBufferedFastaRecordHeader(ioBuffer, &ioOffset, &fastaRec);



	/*

	int len = readBufferedFastaRecord(ioBuffer, &ioOffset, swqBuffers[currentBuffer].rec, maxBasesPerRead);
	swqBuffers[currentBuffer].rec[batchReadCount].length=len;

	while(len>0 && validRecordCount<lastRecord)
		{
		char *seq=swqBuffers[currentBuffer].rec[batchReadCount].seq;
		if((len>=minSeqLength) && (strchr(seq,'N')==NULL))
			{
			if(validRecordCount>=recordsToSkip)
				{
				batchReadCount++;
				batchBaseCount+=((len+3)&0xFFFFFFFC); // Round up to Dword

				if((batchReadCount>=maxReads) || batchBaseCount>=(maxBases-maxBasesPerRead))
					{
					swqBuffers[currentBuffer].numSequences=batchReadCount;

					(*handler)(swqBuffers+currentBuffer, ingressBuffers+currentBuffer, handlerContext);
					totalBatch+=batchReadCount;
					batchReadCount=0;
					batchBaseCount=0;

					currentBuffer=(currentBuffer+1)%PT_INGRESS_BUFFERS;
					waitForBufferIdle(&swqBuffers[currentBuffer].usageCount);
					}

				progressCounter++;
				usedRecords++;

				if(progressCounter >= 1000000)
					{
					LOG(LOG_INFO,"Reads: %li", usedRecords);

					if(monitor!=NULL)
						(*monitor)();

					progressCounter=0;
					}
				}

			validRecordCount++;
			}

		allRecordCount++;

		swqBuffers[currentBuffer].rec[batchReadCount].seq=swqBuffers[currentBuffer].seqBuffer+batchBaseCount;
		swqBuffers[currentBuffer].rec[batchReadCount].qual=swqBuffers[currentBuffer].qualBuffer+batchBaseCount;

		if(ioOffset>ioBufferPrimarySize)
			{
			int remaining=ioBufferValid-ioOffset;
			memcpy(ioBuffer,ioBuffer+ioOffset,remaining);
			ioOffset=0;

			ioBufferValid=remaining+fread(ioBuffer+remaining,1,ioBufferRecycleSize+ioBufferPrimarySize-remaining,file);
			if(ioBufferValid<ioBufferEnd)
				ioBuffer[ioBufferValid]=0;
			}

		len=readBufferedFastaRecord(ioBuffer, &ioOffset, swqBuffers[currentBuffer].rec+batchReadCount, maxBasesPerRead);

		swqBuffers[currentBuffer].rec[batchReadCount].length=len;
		}

	if(batchReadCount>0)
		{
		swqBuffers[currentBuffer].numSequences=batchReadCount;

		(*handler)(swqBuffers+currentBuffer,ingressBuffers+currentBuffer, handlerContext);
		totalBatch+=batchReadCount;
		}

*/

	/*
	LOG(LOG_INFO,"Used %li %li",usedRecords,totalBatch);

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
*/
	fclose(file);

	//return usedRecords;


	return 0;
}



