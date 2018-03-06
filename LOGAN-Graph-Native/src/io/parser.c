#include "common.h"


void prInitSequenceBuffer(SwqBuffer *swqBuffer, int bufSize, int recordsPerBatch, int maxReadLength)
{
	if(bufSize>0)
		{
		swqBuffer->seqBuffer=G_ALLOC(bufSize, MEMTRACKID_SWQ);
		swqBuffer->qualBuffer=G_ALLOC(bufSize, MEMTRACKID_SWQ);
		}
	else
		{
		swqBuffer->seqBuffer=NULL;
		swqBuffer->qualBuffer=NULL;
		}

	swqBuffer->rec=G_ALLOC(sizeof(SequenceWithQuality)*recordsPerBatch, MEMTRACKID_SWQ);

	swqBuffer->maxSequenceTotalLength=bufSize;
	swqBuffer->maxSequences=recordsPerBatch;
	swqBuffer->maxSequenceLength=maxReadLength;
	swqBuffer->numSequences=0;
	swqBuffer->usageCount=0;

}

void prFreeSequenceBuffer(SwqBuffer *swqBuffer)
{
	if(swqBuffer->seqBuffer!=NULL)
		G_FREE(swqBuffer->seqBuffer, swqBuffer->maxSequenceTotalLength, MEMTRACKID_SWQ);

	if(swqBuffer->qualBuffer!=NULL)
		G_FREE(swqBuffer->qualBuffer, swqBuffer->maxSequenceTotalLength, MEMTRACKID_SWQ);

	G_FREE(swqBuffer->rec, sizeof(SequenceWithQuality)*swqBuffer->maxSequences, MEMTRACKID_SWQ);

	memset(swqBuffer, 0, sizeof(SwqBuffer));
}


void prWaitForBufferIdle(int *usageCount)
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


void prIndexingBuilderDataHandler(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *context)
{
	IndexingBuilder *ib=(IndexingBuilder *)context;

	ingressBuffer->ingressPtr=swqBuffer;
	ingressBuffer->ingressTotal=swqBuffer->numSequences;
	ingressBuffer->ingressUsageCount=&swqBuffer->usageCount;

	queueIngress(ib->pt, ingressBuffer);
}


void prRoutingBuilderDataHandler(SwqBuffer *swqBuffer, ParallelTaskIngress *ingressBuffer, void *context)
{
	RoutingBuilder *rb=(RoutingBuilder *)context;

	ingressBuffer->ingressPtr=swqBuffer;
	ingressBuffer->ingressTotal=swqBuffer->numSequences;
	ingressBuffer->ingressUsageCount=&swqBuffer->usageCount;

	queueIngress(rb->pt, ingressBuffer);

}



