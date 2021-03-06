/*
 * fastqParser.c
 *
 *  Created on: Jun 11, 2014
 *      Author: tony
 */

#include "common.h"



static int readBufferedFastaLine(u8 *ioBuffer, int *offset, char *outPtr, int maxLength)
{
	u8 ch;
	u8 *inPtr=ioBuffer+*offset;
	int length=0;

	maxLength--; // Allow for null byte

	ch=*(inPtr++);
	if(ch==0)
		return 0; // EOF -> Early out without updating offset

	while(ch!=0 && ch!=10 && ch!=13 && length < maxLength) // 10=LineFeed, 13 = CarriageReturn
		{
		outPtr[length++]=ch;
		ch=*(inPtr++);
		}

	if(ch==13)	// Handle stupid newlines
		{
		ch=*inPtr;
		if(ch==10)
			inPtr++;
		}

	outPtr[length]=0;

	if(length==maxLength)
		return -1;

	*offset=inPtr-ioBuffer;

	return length;
}

#define PARSER_STATE_HEADER 1
#define PARSER_STATE_TRIM 2
#define PARSER_STATE_SEQUENCE 3
#define PARSER_STATE_EOF 4

int readBufferedFastaRecordHeader(u8 *ioBuffer, int *offset, FastaParserState *fastaRec)
{
	//LOG(LOG_INFO,"Read Header");

	memset(fastaRec->sequenceName, 0, FASTA_SEQUENCE_NAME_MAX_LENGTH);

	char *headerBuf=alloca(FASTA_SEQUENCE_HEADER_MAX_LENGTH);
	headerBuf[0]=0;

	int headerLength=readBufferedFastaLine(ioBuffer, offset, headerBuf, FASTA_SEQUENCE_HEADER_MAX_LENGTH);

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
	fastaRec->sequencePosition=0;
	fastaRec->parserState=PARSER_STATE_TRIM;

	LOG(LOG_INFO,"Name: '%s'",fastaRec->sequenceName);

	return 0;
}



static int readBufferedFastaTrimSequence(u8 *ioBuffer, int *offset, FastaParserState *fastaRec)
{
	//LOG(LOG_INFO,"Read Trim");

	u8 ch;
	u8 *inPtr=ioBuffer+*offset;
	ch=*inPtr;

	if(ch==0)
		{
		fastaRec->parserState=PARSER_STATE_EOF;
		return 0; // EOF -> Early out without updating offset
		}

	int basesConsumed=0;

	int newState=0;
	int lengthToRead=FASTA_SEQUENCE_TRIM_MAX_LENGTH;

	while(!newState && lengthToRead>0)
		{
		switch(ch)
			{
			case '>':
				newState=PARSER_STATE_HEADER;
				break;

			case 10:	// LineFeed
				newState=PARSER_STATE_SEQUENCE;
				inPtr++;
				break;

			case 13:	// CarriageReturn
				newState=PARSER_STATE_SEQUENCE;
				inPtr++;
				ch=*inPtr;	// Handle stupid newlines
				if(ch==10)
					inPtr++;
				break;

			case 'A':
			case 'C':
			case 'G':
			case 'T':
				newState=PARSER_STATE_SEQUENCE;
				break;

			case 0:		// EOF
				newState=PARSER_STATE_EOF;
				break;

			default:
				//LOG(LOG_INFO,"TRIM %c",ch);
				inPtr++;
				ch=*inPtr;
				lengthToRead--;
				basesConsumed++;
			}
		}

	if(lengthToRead>0)
		fastaRec->parserState=newState;

	*offset=inPtr-ioBuffer;

	fastaRec->sequencePosition+=basesConsumed;

	return 0;
}



static int readBufferedFastaValidSequence(u8 *ioBuffer, int *offset, FastaParserState *fastaRec)
{
	//LOG(LOG_INFO,"Read Valid");

	u8 ch;
	u8 *inPtr=ioBuffer+*offset;

	SequenceWithQuality *swq=fastaRec->currentSequenceFragment;
	u8 *outPtr=swq->seq+swq->seqLength;

	int basesConsumed=0;

	ch=*inPtr;
	if(ch==0)
		{
		fastaRec->parserState=PARSER_STATE_EOF;
		return 0; // EOF -> Early out without updating offset
		}

	int newState=0;
	int lengthToRead=FASTA_SEQUENCE_VALID_MAX_LENGTH-swq->seqLength;

	while(!newState && lengthToRead>0)
		{
		switch(ch)
			{
			case '>':
				newState=PARSER_STATE_HEADER;
				break;

			case 10:	// LineFeed
				newState=PARSER_STATE_SEQUENCE;
				inPtr++;
				break;

			case 13:	// CarriageReturn
				newState=PARSER_STATE_SEQUENCE;
				inPtr++;
				ch=*inPtr;	// Handle stupid newlines
				if(ch==10)
					inPtr++;
				break;

			case 'A':
			case 'C':
			case 'G':
			case 'T':
				*(outPtr++)=ch;
				basesConsumed++;

				inPtr++;
				ch=*inPtr;
				lengthToRead--;
				break;

			case 0:		// EOF
				newState=PARSER_STATE_EOF;
				break;

			default:
				newState=PARSER_STATE_TRIM;
			}
		}

	if(lengthToRead>0)
		fastaRec->parserState=newState;

	*offset=inPtr-ioBuffer;
	swq->seqLength+=basesConsumed;
	fastaRec->sequencePosition+=basesConsumed;

	return 0;
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

static IndexedSequenceSource *createSeqSrc(SequenceIndex *seqIndex, char *path)
{
	if(seqIndex==NULL)
		return NULL;

	int len=strlen(path);
	char *dupPath=alloca(len+1);
	strncpy(dupPath, path, len);
	dupPath[len]=0;

	char *slashPtr=strrchr(dupPath, '/');
	if(slashPtr==NULL)
		slashPtr=dupPath;
	else
		slashPtr++;

	char *dotPtr=strchr(slashPtr, '.');
	if(dotPtr!=NULL)
		*dotPtr=0;

	return siAddSequenceSource(seqIndex, slashPtr);
}




s64 faParseAndProcess(char *path, int minSeqLength, s64 recordsToSkip, s64 recordsToUse,
		u8 *ioBuffer, int ioBufferRecycleSize, int ioBufferPrimarySize,
		SwqBuffer *swqBuffers, ParallelTaskIngress *ingressBuffers, int bufferCount,
		void *handlerContext, void (*handler)(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *handlerContext),
		void (*monitor)(), SequenceIndex *seqIndex)
{
	FILE *file=fopen(path,"r");
	if(file==NULL)
		{
		LOG(LOG_INFO,"Failed to open input file: '%s'",path);
		return 0;
		}

	IndexedSequenceSource *seqSrc=createSeqSrc(seqIndex, path);
	IndexedSequence *seq=NULL;

	s64 currentBuffer=0;
	s64 batchReadCount=0;
	s64 batchBaseCount=0;

	prWaitForSwqBufferIdle(&swqBuffers[currentBuffer]);

	int maxReads=swqBuffers[currentBuffer].maxSequences;
	int maxBases=swqBuffers[currentBuffer].maxSequenceTotalLength;
	int maxBasesPerRead=swqBuffers[currentBuffer].maxSequenceLength;

	FastaParserState fastaParser;
	fastaParser.parserState=PARSER_STATE_HEADER;

	int ioOffset=0;
	int ioBufferEnd=ioBufferRecycleSize+ioBufferPrimarySize;

	int ioBufferValid=fread(ioBuffer,1,ioBufferRecycleSize+ioBufferPrimarySize,file);
	if(ioBufferValid<ioBufferEnd)
		ioBuffer[ioBufferValid]=0;

	s64 totalSequences=0;
	s64 totalBatch=0;

	swqBuffers[currentBuffer].rec[batchReadCount].seq=swqBuffers[currentBuffer].seqBuffer;
	swqBuffers[currentBuffer].rec[batchReadCount].qual=NULL;
	swqBuffers[currentBuffer].rec[batchReadCount].tagData=swqBuffers[currentBuffer].tagBuffer;

	fastaParser.currentSequenceFragment=swqBuffers[currentBuffer].rec+batchReadCount;
	fastaParser.currentSequenceFragment->seqLength=0;
	fastaParser.sequenceName[0]=0;
	fastaParser.fragmentStartPosition=0;

	int ret=0;
	while(fastaParser.parserState!=PARSER_STATE_EOF && ret >= 0)
		{
//		LOG(LOG_INFO,"Offset %li - Parser State is %i",ioOffset, fastaParser.parserState);

		switch(fastaParser.parserState)
			{
			case PARSER_STATE_HEADER:
				if(seq!=NULL)
					seq->totalLength=fastaParser.sequencePosition;

				ret=readBufferedFastaRecordHeader(ioBuffer, &ioOffset, &fastaParser);
				totalSequences++;
				if(seqSrc!=NULL)
					seq=siAddSequence(seqSrc, fastaParser.sequenceName);

				break;

			case PARSER_STATE_TRIM:
				ret=readBufferedFastaTrimSequence(ioBuffer, &ioOffset, &fastaParser);
				fastaParser.fragmentStartPosition=fastaParser.sequencePosition;
				fastaParser.currentSequenceFragment->seqLength=0;
				break;

			case PARSER_STATE_SEQUENCE:
				//LOG(LOG_INFO,"Parsing sequence. Fragment Length %i", fastaParser.currentSequenceFragment->seqLength);
				ret=readBufferedFastaValidSequence(ioBuffer, &ioOffset, &fastaParser);
				break;

			}

		s32 fragmentLength=fastaParser.currentSequenceFragment->seqLength;

		if((fragmentLength>=FASTA_SEQUENCE_VALID_MAX_LENGTH) || (fastaParser.parserState!=PARSER_STATE_SEQUENCE && fragmentLength>0))
			{
			u8 *overlapData=NULL;

			if((fragmentLength >= minSeqLength))
				{
				u32 *tagPtr=(u32 *)(swqBuffers[currentBuffer].rec[batchReadCount].tagData);
				u32 taggedReadCount=(u32)(batchReadCount+totalBatch);
				*tagPtr=taggedReadCount;

				swqBuffers[currentBuffer].rec[batchReadCount].tagLength=4;

				if(seq!=NULL)
					siAddSequenceFragment(seq, taggedReadCount, fastaParser.fragmentStartPosition, fastaParser.sequencePosition);

				if(fastaParser.parserState==PARSER_STATE_SEQUENCE && fragmentLength >= FASTA_SEQUENCE_VALID_MAX_LENGTH)
					overlapData=fastaParser.currentSequenceFragment->seq+fastaParser.currentSequenceFragment->seqLength-FASTA_SEQUENCE_VALID_OVERLAP;

				//LOG(LOG_INFO,"Accepting: Sequence %s Fragment %i", fastaParser.sequenceName, fastaParser.currentRecord->length);
				batchReadCount++;
				batchBaseCount+=((fragmentLength+3)&0xFFFFFFFC); // Round up to Dword

				if((batchReadCount>=maxReads) || batchBaseCount>=(maxBases-maxBasesPerRead))
					{
					swqBuffers[currentBuffer].numSequences=batchReadCount;

					(*handler)(swqBuffers+currentBuffer, ingressBuffers+currentBuffer, handlerContext);
					totalBatch+=batchReadCount;
					batchReadCount=0;
					batchBaseCount=0;

					currentBuffer=(currentBuffer+1)%PT_INGRESS_BUFFERS;
					prWaitForSwqBufferIdle(&swqBuffers[currentBuffer]);
					}

				swqBuffers[currentBuffer].rec[batchReadCount].seq=swqBuffers[currentBuffer].seqBuffer+batchBaseCount;
				swqBuffers[currentBuffer].rec[batchReadCount].qual=NULL;
				swqBuffers[currentBuffer].rec[batchReadCount].tagData=swqBuffers[currentBuffer].tagBuffer+batchReadCount*4;

				fastaParser.currentSequenceFragment=swqBuffers[currentBuffer].rec+batchReadCount;
				fastaParser.currentSequenceFragment->seqLength=0;
				fastaParser.fragmentStartPosition=fastaParser.sequencePosition;
				}

			if(overlapData!=NULL)
				{
				memcpy(swqBuffers[currentBuffer].rec[batchReadCount].seq, overlapData, FASTA_SEQUENCE_VALID_OVERLAP);
				fastaParser.currentSequenceFragment->seqLength=FASTA_SEQUENCE_VALID_OVERLAP;
				fastaParser.fragmentStartPosition=fastaParser.sequencePosition-FASTA_SEQUENCE_VALID_OVERLAP;
				}
			else
				{
				fastaParser.currentSequenceFragment->seqLength=0;
				fastaParser.fragmentStartPosition=fastaParser.sequencePosition;
				}
			}

		if(ioOffset>ioBufferPrimarySize)
			{
			int remaining=ioBufferValid-ioOffset;
			memcpy(ioBuffer,ioBuffer+ioOffset,remaining);
			ioOffset=0;

			ioBufferValid=remaining+fread(ioBuffer+remaining,1,ioBufferRecycleSize+ioBufferPrimarySize-remaining,file);
			if(ioBufferValid<ioBufferEnd)
				ioBuffer[ioBufferValid]=0;

			LOG(LOG_INFO,"Reading");
			}

	//LOG(LOG_INFO,"Offset %li - Ret %i - State is now %i", ioOffset, ret, fastaParser.parserState);
		}

	if(seq!=NULL)
		seq->totalLength=fastaParser.sequencePosition;

	if(batchReadCount>0)
		{
		swqBuffers[currentBuffer].numSequences=batchReadCount;

		(*handler)(swqBuffers+currentBuffer, ingressBuffers+currentBuffer, handlerContext);
		totalBatch+=batchReadCount;
		}


	fclose(file);

	return totalSequences;
}





s64 xfaParseAndProcess(char *path, int minSeqLength, s64 recordsToSkip, s64 recordsToUse,
		u8 *ioBuffer, int ioBufferRecycleSize, int ioBufferPrimarySize,
		SwqBuffer *swqBuffers, ParallelTaskIngress *ingressBuffers, int bufferCount,
		void *handlerContext, void (*handler)(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *handlerContext),
		void (*monitor)(), SequenceIndex *seqIndex)
{
	IndexedSequenceSource *seqSrc=createSeqSrc(seqIndex, path);
	IndexedSequence *seq=NULL;

	FILE *file=fopen(path,"r");

	if(file==NULL)
		{
		LOG(LOG_INFO,"Failed to open input file: '%s'",path);
		return 0;
		}

	s64 currentBuffer=0;
	s64 batchReadCount=0;

	s64 batchBaseCount=0;

//	s64 allRecordCount=0,validRecordCount=0,usedRecords=0;
//	s32 progressCounter=0;
//	s64 lastRecord=recordsToSkip+recordsToUse;

	prWaitForSwqBufferIdle(&swqBuffers[currentBuffer]);

	int maxReads=swqBuffers[currentBuffer].maxSequences;
	int maxBases=swqBuffers[currentBuffer].maxSequenceTotalLength;
	int maxBasesPerRead=swqBuffers[currentBuffer].maxSequenceLength;

	swqBuffers[currentBuffer].rec[batchReadCount].seq=swqBuffers[currentBuffer].seqBuffer;
	swqBuffers[currentBuffer].rec[batchReadCount].qual=NULL;
	swqBuffers[currentBuffer].rec[batchReadCount].tagData=swqBuffers[currentBuffer].tagBuffer;

	FastaParserState fastaParser;
	fastaParser.parserState=PARSER_STATE_HEADER;

	fastaParser.currentSequenceFragment=swqBuffers[currentBuffer].rec+batchReadCount;
	fastaParser.currentSequenceFragment->seqLength=0;
	fastaParser.sequenceName[0]=0;
	fastaParser.fragmentStartPosition=0;

	int ioOffset=0;
	int ioBufferEnd=ioBufferRecycleSize+ioBufferPrimarySize;

	int ioBufferValid=fread(ioBuffer,1,ioBufferRecycleSize+ioBufferPrimarySize,file);
	if(ioBufferValid<ioBufferEnd)
		ioBuffer[ioBufferValid]=0;

	s64 totalSequences=0;
	s64 totalBatch=0;
	int fragmentLength=0;

	int ret=0;
	while(fastaParser.parserState!=PARSER_STATE_EOF && ret >= 0)
		{
//		LOG(LOG_INFO,"Offset %li - Parser State is %i",ioOffset, fastaParser.parserState);

		switch(fastaParser.parserState)
			{
			case PARSER_STATE_HEADER:
				if(seq!=NULL)
					seq->totalLength=fastaParser.sequencePosition;

				ret=readBufferedFastaRecordHeader(ioBuffer, &ioOffset, &fastaParser);
				totalSequences++;
				if(seqSrc!=NULL)
					seq=siAddSequence(seqSrc, fastaParser.sequenceName);

				break;

			case PARSER_STATE_TRIM:
				ret=readBufferedFastaTrimSequence(ioBuffer, &ioOffset, &fastaParser);
				break;

			case PARSER_STATE_SEQUENCE:
				fastaParser.fragmentStartPosition=fastaParser.sequencePosition;
				ret=readBufferedFastaValidSequence(ioBuffer, &ioOffset, &fastaParser);
				break;

			}

		fragmentLength=fastaParser.currentSequenceFragment->seqLength;
		//LOG(LOG_INFO,"Frag length: %i", fastaParser.currentRecord->length);
		if((fragmentLength>=FASTA_SEQUENCE_VALID_MAX_LENGTH) || (fastaParser.parserState!=PARSER_STATE_SEQUENCE && fragmentLength>0))
			{
			if((fragmentLength >= minSeqLength))// && (len>1800) && (len<2000))
				{
				u32 *tagPtr=(u32 *)(swqBuffers[currentBuffer].rec[batchReadCount].tagData);
				u32 taggedReadCount=(u32)(batchReadCount+totalBatch);
				*tagPtr=taggedReadCount;

				swqBuffers[currentBuffer].rec[batchReadCount].tagLength=4;

				if(seq!=NULL)
					siAddSequenceFragment(seq, taggedReadCount, fastaParser.fragmentStartPosition, fastaParser.sequencePosition);

				//LOG(LOG_INFO,"Accepting: Sequence %s Fragment %i", fastaParser.sequenceName, fastaParser.currentRecord->length);
				batchReadCount++;
				batchBaseCount+=((fragmentLength+3)&0xFFFFFFFC); // Round up to Dword

				if((batchReadCount>=maxReads) || batchBaseCount>=(maxBases-maxBasesPerRead))
					{
					swqBuffers[currentBuffer].numSequences=batchReadCount;

					(*handler)(swqBuffers+currentBuffer, ingressBuffers+currentBuffer, handlerContext);
					totalBatch+=batchReadCount;
					batchReadCount=0;
					batchBaseCount=0;

					currentBuffer=(currentBuffer+1)%PT_INGRESS_BUFFERS;
					prWaitForSwqBufferIdle(&swqBuffers[currentBuffer]);
					}

				fastaParser.currentSequenceFragment=swqBuffers[currentBuffer].rec+batchReadCount;
				fastaParser.currentSequenceFragment->seqLength=0;
				fastaParser.fragmentStartPosition=fastaParser.sequencePosition;
				}

			swqBuffers[currentBuffer].rec[batchReadCount].seq=swqBuffers[currentBuffer].seqBuffer+batchBaseCount;
			swqBuffers[currentBuffer].rec[batchReadCount].qual=NULL;
			swqBuffers[currentBuffer].rec[batchReadCount].tagData=swqBuffers[currentBuffer].tagBuffer+batchReadCount*4;

			fastaParser.currentSequenceFragment->seqLength=0; //
			}

		if(ioOffset>ioBufferPrimarySize)
			{
			int remaining=ioBufferValid-ioOffset;
			memcpy(ioBuffer,ioBuffer+ioOffset,remaining);
			ioOffset=0;

			ioBufferValid=remaining+fread(ioBuffer+remaining,1,ioBufferRecycleSize+ioBufferPrimarySize-remaining,file);
			if(ioBufferValid<ioBufferEnd)
				ioBuffer[ioBufferValid]=0;

			LOG(LOG_INFO,"Reading");
			}

		//LOG(LOG_INFO,"Offset %li - Ret %i - State is now %i", ioOffset, ret, fastaParser.parserState);
		}

	if(seq!=NULL)
		seq->totalLength=fastaParser.sequencePosition;

	if(batchReadCount>0)
		{
		swqBuffers[currentBuffer].numSequences=batchReadCount;

		(*handler)(swqBuffers+currentBuffer, ingressBuffers+currentBuffer, handlerContext);
		totalBatch+=batchReadCount;
		}


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

	return totalSequences;
}



