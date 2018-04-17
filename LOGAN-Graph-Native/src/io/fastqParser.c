/*
 * fastqParser.c
 *
 *  Created on: Jun 11, 2014
 *      Author: tony
 */

#include "common.h"



static int readBufferedFastqLine(u8 *ioBuffer, int *offset, u8 *outPtr, int maxLength)
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
		return FASTQ_PARSER_ERROR_OVERLONG_LINE_LENGTH;

	if(ch==13)	// Handle stupid newlines
		{
		ch=*inPtr;
		if(ch==10)
			inPtr++;
		}

	*offset=inPtr-ioBuffer;

	return length;
}

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
		ch=*inPtr;
		if(ch==10)
			inPtr++;
		}

	*offset=inPtr-ioBuffer;

	return length;
}



int readBufferedFastqRecord(u8 *ioBuffer, int *offset, SequenceWithQuality *rec, int maxLength)
{
	int len1,len2;

	rec->seqLength=0;

	if(skipBufferedFastqLine(ioBuffer, offset)==0)	// Read Name
		return 0;

	len1=readBufferedFastqLine(ioBuffer, offset,rec->seq,maxLength);
	if(len1<=0)
		return len1;

	if(skipBufferedFastqLine(ioBuffer, offset)==0)
		return FASTQ_PARSER_ERROR_UNEXPECTED_EOF; // Read Comment

	len2=readBufferedFastqLine(ioBuffer, offset,rec->qual,maxLength);

	if(len2<=0)
		return len2;

	if(len1!=len2)
		{
		LOG(LOG_INFO,"%i vs %i  %s vs %s",len1,len2, rec->seq, rec->qual);
		return FASTQ_PARSER_ERROR_LENGTH_MISMATCH;
		}

	rec->seqLength=len1;

	return len1;
}



// Allows suppression of reads based on content

static s32 validateSequence(char *seq)
{
	if(strchr(seq,'N')!=NULL)
		return 0;

	if(strstr(seq,"CAACAACAACAACAACAACAACA")!=NULL)
		return 0;

	if(strstr(seq,"ACAACAACAACAACAACAACAAC")!=NULL)
		return 0;

	if(strstr(seq,"AACAACAACAACAACAACAACAA")!=NULL)
		return 0;

	if(strstr(seq,"TTGTTGTTGTTGTTGTTGTTGTT")!=NULL)
		return 0;

	if(strstr(seq,"TGTTGTTGTTGTTGTTGTTGTTG")!=NULL)
		return 0;

	if(strstr(seq,"GTTGTTGTTGTTGTTGTTGTTGT")!=NULL)
		return 0;

	if(strstr(seq,"GAAGAAGAAGAAGAAGAAGAAGA")!=NULL)
		return 0;

	if(strstr(seq,"AGAAGAAGAAGAAGAAGAAGAAG")!=NULL)
		return 0;

	if(strstr(seq,"AAGAAGAAGAAGAAGAAGAAGAA")!=NULL)
		return 0;

	if(strstr(seq,"TTCTTCTTCTTCTTCTTCTTCTT")!=NULL)
		return 0;

	if(strstr(seq,"TCTTCTTCTTCTTCTTCTTCTTC")!=NULL)
		return 0;

	if(strstr(seq,"CTTCTTCTTCTTCTTCTTCTTCT")!=NULL)
		return 0;


	return 1;
}


s64 fqParseAndProcess(char *path, int minSeqLength, s64 recordsToSkip, s64 recordsToUse,
		u8 *ioBuffer, int ioBufferRecycleSize, int ioBufferPrimarySize,
		SwqBuffer *swqBuffers, ParallelTaskIngress *ingressBuffers, int bufferCount,
		void *handlerContext, void (*handler)(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *handlerContext),
		void (*monitor)())
{
	int fd=open(path, O_RDONLY);

	if(fd<0)
		{
		LOG(LOG_CRITICAL,"Failed to open input file: '%s'",path);
		return 0;
		}

	posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);

	s64 currentBuffer=0;
	s64 batchReadCount=0;
	s64 batchBaseCount=0;

	s64 allRecordCount=0,validRecordCount=0,usedRecords=0;
	s32 progressCounter=0;
	s64 lastRecord=recordsToSkip+recordsToUse;

	prWaitForSwqBufferIdle(&swqBuffers[currentBuffer]);

	int maxReads=swqBuffers[currentBuffer].maxSequences;
	int maxBases=swqBuffers[currentBuffer].maxSequenceTotalLength;
	int maxBasesPerRead=swqBuffers[currentBuffer].maxSequenceLength;

	swqBuffers[currentBuffer].rec[batchReadCount].seq=swqBuffers[currentBuffer].seqBuffer;
	swqBuffers[currentBuffer].rec[batchReadCount].qual=swqBuffers[currentBuffer].qualBuffer;
	swqBuffers[currentBuffer].rec[batchReadCount].tagData=NULL;
	swqBuffers[currentBuffer].rec[batchReadCount].tagLength=0;

	int ioOffset=0;
	int ioBufferEnd=ioBufferRecycleSize+ioBufferPrimarySize;

	ssize_t bytesRead=read(fd, ioBuffer,ioBufferRecycleSize+ioBufferPrimarySize);
	if(bytesRead<0)
		LOG(LOG_CRITICAL,"Error reading file");

	s64 fileOffset=bytesRead;

	if(PARSER_IO_PREFETCH_MULTIPLIER>0)
		posix_fadvise(fd, fileOffset, ioBufferPrimarySize*PARSER_IO_PREFETCH_MULTIPLIER, POSIX_FADV_WILLNEED);

	int ioBufferValid=bytesRead;

	if(ioBufferValid<ioBufferEnd)
		ioBuffer[ioBufferValid]=0;


	s64 totalBatch=0;

	int len = readBufferedFastqRecord(ioBuffer, &ioOffset, swqBuffers[currentBuffer].rec, maxBasesPerRead);
	swqBuffers[currentBuffer].rec[batchReadCount].seqLength=len;

	while(len>0 && validRecordCount<lastRecord)
		{
		u8 *seq=swqBuffers[currentBuffer].rec[batchReadCount].seq;
		if((len>=minSeqLength) && validateSequence((char *)seq))
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


//					int usage=buffers[currentBuffer].usageCount;
//					if(usage>0)
//						LOG(LOG_INFO, "New buffer usage count was %i",usage);

					prWaitForSwqBufferIdle(&swqBuffers[currentBuffer]);

//					if(usage>0)
//						LOG(LOG_INFO, "New buffer usage count is now %i",buffers[currentBuffer].usageCount);
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
		swqBuffers[currentBuffer].rec[batchReadCount].tagData=NULL;
		swqBuffers[currentBuffer].rec[batchReadCount].tagLength=0;

		if(ioOffset>ioBufferPrimarySize)
			{
			int remaining=ioBufferValid-ioOffset;
			u8 *remainingPtr=ioBuffer+ioOffset;
			ioOffset-=ioBufferPrimarySize;

			memcpy(ioBuffer+ioOffset,remainingPtr,remaining);

			bytesRead=read(fd, ioBuffer+ioOffset+remaining,ioBufferPrimarySize);

			if(bytesRead<0)
				LOG(LOG_CRITICAL,"Error reading file");

			fileOffset+=bytesRead;

			if(PARSER_IO_PREFETCH_MULTIPLIER>0)
				posix_fadvise(fd, fileOffset, ioBufferPrimarySize*PARSER_IO_PREFETCH_MULTIPLIER, POSIX_FADV_WILLNEED);

			ioBufferValid=remaining+ioOffset+bytesRead;
			if(ioBufferValid<ioBufferEnd)
				ioBuffer[ioBufferValid]=0;
			}

		len=readBufferedFastqRecord(ioBuffer, &ioOffset, swqBuffers[currentBuffer].rec+batchReadCount, maxBasesPerRead);

		swqBuffers[currentBuffer].rec[batchReadCount].seqLength=len;
		}

	if(batchReadCount>0)
		{
		swqBuffers[currentBuffer].numSequences=batchReadCount;

		(*handler)(swqBuffers+currentBuffer,ingressBuffers+currentBuffer, handlerContext);
		totalBatch+=batchReadCount;
		}

	LOG(LOG_INFO,"Used %li %li",usedRecords,totalBatch);

	if(len<0)
		{
		switch (len)
			{
			case FASTQ_PARSER_ERROR_OVERLONG_LINE_LENGTH:
				LOG(LOG_INFO,"Over-long Seq/Qual in Fastq record");
				break;

			case FASTQ_PARSER_ERROR_UNEXPECTED_EOF:
				LOG(LOG_INFO,"Unexpected EOF in Fastq record");
				break;

			case FASTQ_PARSER_ERROR_LENGTH_MISMATCH:
				LOG(LOG_INFO,"Seq/Qual length mismatch in Fastq record");
				break;

			}
		}

	close(fd);

	return usedRecords;

}

