/*
 * task.c
 *
 *  Created on: Oct 28, 2015
 *      Author: tony
 */

#include "../common.h"

// Measured in seconds - Should never be hit unless workers block
#define MASTER_TIMEOUT (60*15)




void waitForStartup(ParallelTask *pt)
{
	LOG(LOG_INFO,"Master: Waiting for startup");

	int lock=pthread_mutex_lock(&(pt->mutex));
	if(lock!=0)
			{
			LOG(LOG_CRITICAL,"Failed to lock for performTask");
			return;
			}

	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += MASTER_TIMEOUT;
	int rc = 0;
	while(pt->state==PTSTATE_STARTUP && rc==0)
		{
        rc = pthread_cond_timedwait(&(pt->master_startup),&(pt->mutex), &ts);
        if (rc != 0)
        	{
        	LOG(LOG_INFO,"Master: WaitForStartup Timeout");
        	exit(1);
        	}
		}

	pthread_mutex_unlock(&(pt->mutex));

	LOG(LOG_INFO,"Master: Done Startup");
}

void waitForShutdown(ParallelTask *pt)
{
	LOG(LOG_INFO,"Master: Waiting for shutdown");


	int lock=pthread_mutex_lock(&(pt->mutex));
	if(lock!=0)
		{
		LOG(LOG_CRITICAL,"Failed to lock for waitForShutdown");
		return;
		}

	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += MASTER_TIMEOUT;
	int rc = 0;
	while(pt->state!=PTSTATE_DEAD && rc==0)
		{
        rc = pthread_cond_timedwait(&(pt->master_shutdown),&(pt->mutex), &ts);
        if (rc != 0)
        	{
        	LOG(LOG_INFO,"Master: WaitForShutdown Timeout");
        	exit(1);
        	}
		}

	pthread_mutex_unlock(&(pt->mutex));

	LOG(LOG_INFO,"Master: Done Shutdown");
}


int queueIngress(ParallelTask *pt, void *ingressPtr, int ingressCount, int *ingressUsageCount)
{
//	LOG(LOG_INFO,"Master: QueueIngress %p %i",ingressPtr,ingressCount);

	int lock=pthread_mutex_lock(&(pt->mutex));
	if(lock!=0)
		{
		LOG(LOG_CRITICAL,"Failed to lock for QueueIngress");
		return 0;
		}

	// NEED TO CHECK EXISTING INGRESS

	//LOG(LOG_INFO,"Master: QueueIngress Wait");

	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += MASTER_TIMEOUT;
	int rc = 0;
	while(pt->reqIngressPtr!=NULL && rc==0)
		{
        rc = pthread_cond_timedwait(&(pt->master_ingress),&(pt->mutex), &ts);
        if (rc != 0)
        	{
        	LOG(LOG_INFO,"Master: QueueIngress Timeout");
        	exit(1);
        	}

		}

	//LOG(LOG_INFO,"Master: QueueIngress Wait Done");

	pt->reqIngressPtr=ingressPtr;
	pt->reqIngressTotal=ingressCount;
	pt->reqIngressUsageCount=ingressUsageCount;

	*ingressUsageCount=1;

	pthread_cond_broadcast(&(pt->workers_idle));

	pthread_mutex_unlock(&(pt->mutex));

//	LOG(LOG_INFO,"Master: QueueIngress Done");

	return 0;
}


void queueShutdown(ParallelTask *pt)
{
	LOG(LOG_INFO,"Master: QueueShutdown");

	int lock=pthread_mutex_lock(&(pt->mutex));
	if(lock!=0)
		{
		LOG(LOG_CRITICAL,"Failed to lock for queueShutdown");
		return;
		}

	LOG(LOG_INFO,"Master: QueueShutdown locked");

	pt->reqShutdown=1;

	pthread_cond_broadcast(&(pt->workers_idle));

	pthread_mutex_unlock(&(pt->mutex));

	LOG(LOG_INFO,"Master: QueueShutdown Complete");
}

static void performTaskNewIngress(ParallelTask *pt)
{
//	LOG(LOG_INFO,"New Ingress Req");

	if(pt->activeIngressPtr!=NULL)
		{
		LOG(LOG_INFO,"Have an active ingress");
		exit(1);
		}

	pt->activeIngressPtr=pt->reqIngressPtr;
	pt->activeIngressTotal=pt->reqIngressTotal;
	pt->activeIngressUsageCount=pt->reqIngressUsageCount;

	pt->activeIngressPosition=0;

	pt->reqIngressPtr=NULL;
	pthread_cond_broadcast(&(pt->master_ingress));
}

static int performTaskActive(ParallelTask *pt, int workerNo)
{
	int lockRet=0, ret=0;

	// 1st priority - high priority intermediates
	if(pt->config->doIntermediate!=NULL)
		{
		pthread_mutex_unlock(&(pt->mutex));

		ret = pt->config->doIntermediate(pt,workerNo,0);

		lockRet=pthread_mutex_lock(&(pt->mutex));
		if(lockRet!=0)
				{
				LOG(LOG_CRITICAL,"Failed to lock for performTask: %s",strerror(lockRet));
				}

		//LOG(LOG_INFO,"Hipri %i for worker %i",ret,workerNo);

		if(ret)
			return 1;
		}

	// 2nd priority - active ingress


	if((pt->activeIngressPtr!=NULL) && (pt->config->allocateIngressSlot == NULL || pt->config->allocateIngressSlot(pt,workerNo)))
		{
//		LOG(LOG_INFO,"Active 2 %i %i",pt->activeIngressCounter,pt->activeIngressTotal);

		void *ingressPtr=pt->activeIngressPtr;
		int *ingressUsageCount=pt->activeIngressUsageCount;
		int ingressPos=pt->activeIngressPosition;

		int ingressSize=pt->config->ingressBlocksize;
		if(pt->activeIngressPosition+ingressSize > pt->activeIngressTotal)
			ingressSize=pt->activeIngressTotal-pt->activeIngressPosition;

		pt->activeIngressPosition+=ingressSize;

		if(pt->activeIngressPosition>=pt->activeIngressTotal)
			{
	//		LOG(LOG_INFO,"Ingress done");

			pt->activeIngressPtr=NULL;
			pt->activeIngressUsageCount=NULL;
			pt->activeIngressPosition=0;

			if(pt->config->tasksPerTidy>0)
				pt->ingressPerTidyCounter--;

			if(pt->ingressPerTidyCounter<=0)
				{
				pt->activeTidyTotal=pt->config->tasksPerTidy;
				pt->activeTidyPosition=0;
				}
			else
				{
				if(pt->reqIngressPtr)
					performTaskNewIngress(pt);
				}
			}
		else
			__atomic_fetch_add(ingressUsageCount, 1, __ATOMIC_SEQ_CST); // Increase usage count if not last, to allow for 'queued usage'

		// Do Ingress

		pthread_mutex_unlock(&(pt->mutex));

		ret=pt->config->doIngress(pt,workerNo,ingressPtr, ingressPos, ingressSize);

		lockRet=pthread_mutex_lock(&(pt->mutex));
		if(lockRet!=0)
				{
				LOG(LOG_CRITICAL,"Failed to lock for performTask: %s",strerror(lockRet));
				}

		__atomic_fetch_sub(ingressUsageCount, 1, __ATOMIC_SEQ_CST);

		return 1;
		}
	/*
	else
		{
		//pthread_cond_broadcast(&(pt->master_ingress));

		//LOG(LOG_INFO,"Has Ingress %p and %p",pt->activeIngressPtr, pt->reqIngressPtr);
		}
*/

	// 3rd priority - low priority intermediates
	if(pt->config->doIntermediate!=NULL)
		{
		pthread_mutex_unlock(&(pt->mutex));

		ret = pt->config->doIntermediate(pt,workerNo,1);

		lockRet=pthread_mutex_lock(&(pt->mutex));
		if(lockRet!=0)
				{
				LOG(LOG_CRITICAL,"Failed to lock for performTask: %s",strerror(lockRet));
				}

		//LOG(LOG_INFO,"Lopri %i for worker %i",ret,workerNo);

		if(ret)
			return 1;
		}

	// 4th priority - tidy
	if(pt->activeTidyTotal>0)
		{
		//LOG(LOG_INFO,"Tidy needed");

		// Last non-sleeping thread
		if(pt->idleThreads==pt->liveThreads-1)
			{
			pt->state=PTSTATE_TIDY_WAIT;
			return 1;
			}
		else
			return 0;
		}

	// 5th priority - new ingress
	if((pt->reqIngressPtr) && (!pt->activeIngressPtr))
		{
		performTaskNewIngress(pt);
		return 1;
		}

	// 6th priority - shutdown
	else if(pt->reqShutdown)
		{
		//LOG(LOG_INFO,"New Shutdown Req");

   		pt->state=PTSTATE_SHUTDOWN_TIDY_WAIT;
   		return 1;
		}

	return 0;
}


void performTidyWait(ParallelTask *pt)
{
//	LOG(LOG_INFO,"TIDY WAIT");

	pthread_mutex_unlock(&(pt->mutex));

	pthread_barrier_wait(&(pt->tidyStartBarrier));

	int ret=pthread_mutex_lock(&(pt->mutex));
	if(ret!=0)
		{
		LOG(LOG_CRITICAL,"Failed to lock for performTask: %s",strerror(ret));
		}

//	LOG(LOG_INFO,"TIDY WAIT done");
}

void performTidy(ParallelTask *pt, int workerNo)
{
	int tidyPos=pt->activeTidyPosition;

	pt->activeTidyPosition++;

	if(pt->activeTidyPosition>=pt->activeTidyTotal)
		{
		pt->activeTidyTotal=0;
		pt->activeTidyPosition=0;
		}

	// Do Tidy if handler set
	if(pt->config->doTidy!=NULL)
		{
		pthread_mutex_unlock(&(pt->mutex));

		pt->config->doTidy(pt,workerNo,tidyPos);

		int ret=pthread_mutex_lock(&(pt->mutex));
		if(ret!=0)
			{
			LOG(LOG_CRITICAL,"Failed to lock for performTask: %s",strerror(ret));
			}
		}
}



void performTask(ParallelTask *pt)
{
	int workerNo=-1;
	int ret=0;

	/*
	 * Join steps:
	 * 	Increment live threads
	 * 	call register
	 *  Worker startup barrier
	 *  If first in, set state to idle and wake master
	 */

	workerNo=__atomic_fetch_add(&(pt->liveThreads),1, __ATOMIC_SEQ_CST);

	//LOG(LOG_INFO,"Worker %i Register",workerNo);

	pt->config->doRegister(pt,workerNo);

	//LOG(LOG_INFO,"Worker %i Startup Barrier wait",workerNo);

	//int startupWait=
	pthread_barrier_wait(&(pt->startupBarrier));

	//LOG(LOG_INFO,"Worker %i Startup Barrier done",workerNo);

	// Lock should be held except when working

	ret=pthread_mutex_lock(&(pt->mutex));
	if(ret!=0)
			{
			LOG(LOG_CRITICAL,"Failed to lock for performTask: %s",strerror(ret));
			}

	// If nobody else did it, update state and notify any waiting master
	if(pt->state==PTSTATE_STARTUP)
		{
		pt->state=PTSTATE_ACTIVE;
		pthread_cond_broadcast(&(pt->master_startup));
		}

	//LOG(LOG_INFO,"Worker %i Entering main loop",workerNo);


	while(pt->state!=PTSTATE_SHUTDOWN_TIDY_WAIT)
		{
		//LOG(LOG_INFO,"Worker %i looping %i",workerNo,pt->state);

		if(pt->state==PTSTATE_ACTIVE)
			{
			if(performTaskActive(pt,workerNo))
				{
				pthread_cond_broadcast(&(pt->workers_idle));
				}
		   	else
    			{
		   		pt->idleThreads++;
		   		pthread_cond_broadcast(&(pt->master_ingress));

//		   		if(pt->idleThreads > 6)
//		   			LOG(LOG_INFO,"Worker %i sleeping (idle %i)",workerNo, pt->idleThreads);

		   		pthread_cond_wait(&(pt->workers_idle),&(pt->mutex));

		   		pt->idleThreads--;

//		   		if(pt->idleThreads > 5)
//		   			LOG(LOG_INFO,"Worker %i waking (idle %i)",workerNo, pt->idleThreads);
    			}
			}
		else if(pt->state==PTSTATE_TIDY_WAIT)
			{
			performTidyWait(pt);

			if(pt->state==PTSTATE_TIDY_WAIT)
				pt->state=PTSTATE_TIDY;
			}
		else if(pt->state==PTSTATE_TIDY)
			{
			if(pt->activeTidyTotal>0)
				{
				performTidy(pt, workerNo);
				}
			else
				{
				pthread_mutex_unlock(&(pt->mutex));
				pthread_barrier_wait(&(pt->tidyEndBarrier));
				ret=pthread_mutex_lock(&(pt->mutex));
				if(ret!=0)
						{
						LOG(LOG_CRITICAL,"Failed to lock for performTask: %s",strerror(ret));
						}

				if(pt->state==PTSTATE_TIDY)
					{
					pt->state=PTSTATE_ACTIVE;

					pt->tidyBackoffCounter--;
					if(pt->tidyBackoffCounter==0)
						{
						pt->tidyBackoffCounter=pt->config->tidysPerBackoff;

						pt->ingressPerTidyTotal*=2;
						if(pt->ingressPerTidyTotal > pt->config->ingressPerTidyMax)
							pt->ingressPerTidyTotal = pt->config->ingressPerTidyMax;
						}

					pt->ingressPerTidyCounter=pt->ingressPerTidyTotal;

//					LOG(LOG_INFO,"IngressesPerTidy %i BackOffCount %i",pt->ingressPerTidyTotal,pt->tidyBackoffCounter);
					}
				}
			}

		}

	LOG(LOG_INFO,"Worker %i Completed main loop - waiting for shutdown tidy",workerNo);

	// Shutdown tidy - should configure separately

	performTidyWait(pt);


	if(pt->state==PTSTATE_SHUTDOWN_TIDY_WAIT)
		{
		pt->state=PTSTATE_SHUTDOWN_TIDY;

		//pt->activeTidyTotal=pt->config->tasksPerTidy;
		//pt->activeTidyPosition=0;
		}

	while(pt->activeTidyTotal>0)
		performTidy(pt, workerNo);


	// Release Lock at shutdown
	pt->state=PTSTATE_SHUTDOWN;

	pthread_mutex_unlock(&(pt->mutex));

	/*
	 * Leave steps:
	 * Worker shutdown barrier
	 * Call deregister
	 * Decrement live threads
	 * If last out, set state to dead and wake master(s)
	 */

//	LOG(LOG_INFO,"Worker %i Shutdown Barrier wait",workerNo);

	//int shutdownWait=
	pthread_barrier_wait(&(pt->shutdownBarrier));

//	LOG(LOG_INFO,"Worker %i Deregister",workerNo);

	pt->config->doDeregister(pt,workerNo);
	int alive=__atomic_sub_fetch(&(pt->liveThreads),1, __ATOMIC_SEQ_CST);

//	LOG(LOG_INFO,"Worker %i - still alive %i",workerNo,alive);

	if(alive==0)
		{
		LOG(LOG_INFO,"Worker %i last out, notifying master",workerNo);

		ret=pthread_mutex_lock(&(pt->mutex));
		if(ret!=0)
				{
				LOG(LOG_CRITICAL,"Failed to lock for performTask: %s",strerror(ret));
				return;
				}

		pt->state=PTSTATE_DEAD;
		pthread_cond_broadcast(&(pt->master_shutdown));

		pthread_mutex_unlock(&(pt->mutex));
		}

}







/* Alloc and Free */


ParallelTaskConfig *allocParallelTaskConfig(
		void (*doRegister)(ParallelTask *pt, int workerNo),
		void (*doDeregister)(ParallelTask *pt, int workerNo),
		int (*allocateIngressSlot)(ParallelTask *pt, int workerNo),
		int (*doIngress)(ParallelTask *pt, int workerNo,void *ingressPtr, int ingressPosition, int ingressSize),
		int (*doIntermediate)(ParallelTask *pt, int workerNo, int force),
		int (*doTidy)(ParallelTask *pt, int workerNo, int tidyNo),
		int expectedThreads, int ingressBlocksize,
		int ingressPerTidyMin, int ingressPerTidyMax, int tidysPerBackoff, int tasksPerTidy)
{
	LOG(LOG_INFO,"allocParallelTaskConfig");

	ParallelTaskConfig *ptc=ptParallelTaskConfigAlloc();

	ptc->doRegister=doRegister;
	ptc->doDeregister=doDeregister;
	ptc->allocateIngressSlot=allocateIngressSlot;
	ptc->doIngress=doIngress;
	ptc->doIntermediate=doIntermediate;
	ptc->doTidy=doTidy;

	ptc->expectedThreads=expectedThreads;
	ptc->ingressBlocksize=ingressBlocksize;

	ptc->ingressPerTidyMin=ingressPerTidyMin;
	ptc->ingressPerTidyMax=ingressPerTidyMax;
	ptc->tidysPerBackoff=tidysPerBackoff;

	ptc->tasksPerTidy=tasksPerTidy;



	return ptc;
}

void freeParallelTaskConfig(ParallelTaskConfig *ptc)
{
	ptParallelTaskConfigFree(ptc);
}

ParallelTask *allocParallelTask(ParallelTaskConfig *ptc, void *dataPtr)
{
	LOG(LOG_INFO,"allocParallelTask");
	ParallelTask *pt=ptParallelTaskAlloc();


	pt->config=ptc;
	pt->dataPtr=dataPtr;

	pt->reqIngressPtr=NULL;
	pt->reqIngressTotal=0;
	pt->reqShutdown=0;

	pt->activeIngressPtr=NULL;
	pt->activeIngressTotal=0;

	pt->activeTidyPosition=0;
	pt->activeTidyTotal=0;

	pt->ingressPerTidyTotal=ptc->ingressPerTidyMin;
	pt->tidyBackoffCounter=ptc->tidysPerBackoff;

	int ret;

	ret=pthread_mutex_init(&(pt->mutex),NULL);
	if(ret!=0)
		{
		LOG(LOG_CRITICAL,"Failed to init mutex: %s",strerror(ret));
		return NULL;
		}

	ret=pthread_cond_init(&(pt->master_startup),NULL);
	if(ret!=0)
		{
		LOG(LOG_CRITICAL,"Failed to init cond: %s",strerror(ret));
		return NULL;
		}

	ret=pthread_cond_init(&(pt->master_ingress),NULL);
	if(ret!=0)
		{
		LOG(LOG_CRITICAL,"Failed to init cond: %s",strerror(ret));
		return NULL;
		}

	ret=pthread_cond_init(&(pt->master_shutdown),NULL);
	if(ret!=0)
		{
		LOG(LOG_CRITICAL,"Failed to init cond: %s",strerror(ret));
		return NULL;
		}

	pt->state=0;

	ret=pthread_barrier_init(&(pt->startupBarrier),NULL,ptc->expectedThreads);
	if(ret!=0)
		{
		LOG(LOG_CRITICAL,"Failed to init barrier: %s",strerror(ret));
		return NULL;
		}

	ret=pthread_barrier_init(&(pt->shutdownBarrier),NULL,ptc->expectedThreads);
	if(ret!=0)
		{
		LOG(LOG_CRITICAL,"Failed to init barrier: %s",strerror(ret));
		return NULL;
		}

	ret=pthread_barrier_init(&(pt->tidyStartBarrier),NULL,ptc->expectedThreads);
	if(ret!=0)
		{
		LOG(LOG_CRITICAL,"Failed to init barrier: %s",strerror(ret));
		return NULL;
		}

	ret=pthread_barrier_init(&(pt->tidyEndBarrier),NULL,ptc->expectedThreads);
	if(ret!=0)
		{
		LOG(LOG_CRITICAL,"Failed to init barrier: %s",strerror(ret));
		return NULL;
		}

	ret=pthread_cond_init(&(pt->workers_idle),NULL);
	if(ret!=0)
		{
		LOG(LOG_CRITICAL,"Failed to init cond: %s",strerror(ret));
		return NULL;
		}

	pt->liveThreads=0;
	pt->idleThreads=0;

	return pt;
}
void freeParallelTask(ParallelTask *pt)
{
	int ret;

	ret=pthread_mutex_destroy(&(pt->mutex));
	if(ret!=0)
		{
		LOG(LOG_CRITICAL,"Failed to destroy mutex: %s",strerror(ret));
		return;
		}

	ret=pthread_cond_destroy(&(pt->master_startup));
	if(ret!=0)
		{
		LOG(LOG_CRITICAL,"Failed to destroy cond: %s",strerror(ret));
		return;
		}

	ret=pthread_cond_destroy(&(pt->master_ingress));
	if(ret!=0)
		{
		LOG(LOG_CRITICAL,"Failed to destroy cond: %s",strerror(ret));
		return;
		}

	ret=pthread_cond_destroy(&(pt->master_shutdown));
	if(ret!=0)
		{
		LOG(LOG_CRITICAL,"Failed to destroy cond: %s",strerror(ret));
		return;
		}

	ret=pthread_barrier_destroy(&(pt->startupBarrier));
	if(ret!=0)
		{
		LOG(LOG_CRITICAL,"Failed to destroy barrier: %s",strerror(ret));
		return;
		}

	ret=pthread_barrier_destroy(&(pt->shutdownBarrier));
	if(ret!=0)
		{
		LOG(LOG_CRITICAL,"Failed to destroy barrier: %s",strerror(ret));
		return;
		}

	ret=pthread_barrier_destroy(&(pt->tidyStartBarrier));
	if(ret!=0)
		{
		LOG(LOG_CRITICAL,"Failed to destroy barrier: %s",strerror(ret));
		return;
		}

	ret=pthread_barrier_destroy(&(pt->tidyEndBarrier));
	if(ret!=0)
		{
		LOG(LOG_CRITICAL,"Failed to destroy barrier: %s",strerror(ret));
		return;
		}

	ret=pthread_cond_destroy(&(pt->workers_idle));
	if(ret!=0)
		{
		LOG(LOG_CRITICAL,"Failed to destroy cond: %s",strerror(ret));
		return;
		}

	ptParallelTaskFree(pt);
}

