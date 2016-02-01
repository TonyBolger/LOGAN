/*
 * task.c
 *
 *  Created on: Oct 28, 2015
 *      Author: tony
 */

#include "common.h"







void waitForStartup(ParallelTask *pt)
{
	LOG(LOG_INFO,"Master: Waiting for startup");

	int lock=pthread_mutex_lock(&(pt->mutex));
	if(lock!=0)
			{
			LOG(LOG_CRITICAL,"Failed to lock for performTask");
			return;
			}

	while(pt->state==PTSTATE_STARTUP)
		pthread_cond_wait(&(pt->master_startup),&(pt->mutex));

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

	while(pt->state!=PTSTATE_DEAD)
		pthread_cond_wait(&(pt->master_shutdown), &(pt->mutex));

	pthread_mutex_unlock(&(pt->mutex));

	LOG(LOG_INFO,"Master: Done Shutdown");
}




int queueIngress(ParallelTask *pt, void *ingressPtr, int ingressCount)
{
	LOG(LOG_INFO,"Master: QueueIngress %p %i",ingressPtr,ingressCount);

	int lock=pthread_mutex_lock(&(pt->mutex));
	if(lock!=0)
		{
		LOG(LOG_CRITICAL,"Failed to lock for QueueIngress");
		return 0;
		}

	// NEED TO CHECK EXISTING INGRESS

	LOG(LOG_INFO,"Master: QueueIngress Wait");

	while(pt->reqIngressPtr!=NULL)
		pthread_cond_wait(&(pt->master_ingress),&(pt->mutex));

	LOG(LOG_INFO,"Master: QueueIngress Wait Done");

	pt->reqIngressPtr=ingressPtr;
	pt->reqIngressTotal=ingressCount;

	pthread_cond_broadcast(&(pt->workers_idle));

	pthread_mutex_unlock(&(pt->mutex));

	LOG(LOG_INFO,"Master: QueueIngress Done");

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

	pt->reqShutdown=1;

	pthread_cond_broadcast(&(pt->workers_idle));

	pthread_mutex_unlock(&(pt->mutex));

	LOG(LOG_INFO,"Master: QueueShutdown Complete");
}

static void performTaskNewIngress(ParallelTask *pt)
{
	LOG(LOG_INFO,"New Ingress Req");

	pt->activeIngressPtr=pt->reqIngressPtr;
	pt->activeIngressTotal=pt->reqIngressTotal;
	pt->activeIngressPosition=0;

	pt->reqIngressPtr=NULL;
	pthread_cond_broadcast(&(pt->master_ingress));
}

static int performTaskActive(ParallelTask *pt, int workerNo)
{
	int ret=0;

	// 1st priority - high priority intermediates
	if(0)
		{
		LOG(LOG_INFO,"Active 1");
		return 1;
		}

	// 2nd priority - active ingress
	if(pt->activeIngressPtr!=NULL)
		{
		LOG(LOG_INFO,"Active 2 %i %i",pt->activeIngressPosition,pt->activeIngressTotal);

		void *ingressPtr=pt->activeIngressPtr;
		int ingressPos=pt->activeIngressPosition;

		int ingressSize=pt->config->ingressBlocksize;
		if(pt->activeIngressPosition+ingressSize > pt->activeIngressTotal)
			ingressSize=pt->activeIngressTotal-pt->activeIngressPosition;

		pt->activeIngressPosition+=ingressSize;

		if(pt->activeIngressPosition>=pt->activeIngressTotal)
			{
			LOG(LOG_INFO,"Ingress done");

			pt->activeIngressPtr=NULL;
			pt->activeIngressPosition=0;

			if(1)
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

		// Do Ingress

		pthread_mutex_unlock(&(pt->mutex));

		pt->config->doIngress(pt,workerNo,ingressPtr, ingressPos, ingressSize);

		ret=pthread_mutex_lock(&(pt->mutex));
		if(ret!=0)
				{
				LOG(LOG_CRITICAL,"Failed to lock for performTask: %s",strerror(ret));
				}

		return 1;
		}

	// 3rd priority - low priority intermediates
	if(0)
		{
		LOG(LOG_INFO,"Active 3");
		return 1;
		}

	// 4th priority - tidy
	if(pt->activeTidyTotal>0)
		{
		LOG(LOG_INFO,"Tidy needed");

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
	if(pt->reqIngressPtr)
		{
		performTaskNewIngress(pt);
		return 1;
		}

	// 6th priority - shutdown
	else if(pt->reqShutdown)
		{
		LOG(LOG_INFO,"New Shutdown Req");

   		pt->state=PTSTATE_SHUTDOWN;
   		return 1;
		}

	return 0;
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

	workerNo=__sync_fetch_and_add(&(pt->liveThreads),1);

	//LOG(LOG_INFO,"Worker %i Register",workerNo);

	pt->config->doRegister(pt,workerNo);

	//LOG(LOG_INFO,"Worker %i Startup Barrier wait",workerNo);

	//int startupWait=
	pthread_barrier_wait(&(pt->startupBarrier));

	//LOG(LOG_INFO,"Worker %i Startup Barrier done",workerNo);

	// Lock should be held except when sleeping or working

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


	while(pt->state!=PTSTATE_SHUTDOWN)
		{
		LOG(LOG_INFO,"Worker %i looping %i",workerNo,pt->state);

		if(pt->state==PTSTATE_ACTIVE)
			{
			if(performTaskActive(pt,workerNo))
				{
				pthread_cond_broadcast(&(pt->workers_idle));
				}
		   	else
    			{
		   		pt->idleThreads++;
		   		pthread_cond_wait(&(pt->workers_idle),&(pt->mutex));
		   		pt->idleThreads--;
    			}
			}
		else if(pt->state==PTSTATE_TIDY_WAIT)
			{
			LOG(LOG_INFO,"TIDY WAIT");

			pthread_mutex_unlock(&(pt->mutex));

			pthread_barrier_wait(&(pt->tidyStartBarrier));

			ret=pthread_mutex_lock(&(pt->mutex));
			if(ret!=0)
					{
					LOG(LOG_CRITICAL,"Failed to lock for performTask: %s",strerror(ret));
					}

			LOG(LOG_INFO,"TIDY WAIT done");

			if(pt->state==PTSTATE_TIDY_WAIT)
				{
				pt->state=PTSTATE_TIDY;
				}
			}
		else if(pt->state==PTSTATE_TIDY)
			{
			if(pt->activeTidyTotal>0)
				{
				int tidyPos=pt->activeTidyPosition;

				pt->activeTidyPosition++;

				if(pt->activeTidyPosition>=pt->activeTidyTotal)
					{
					pt->activeTidyTotal=0;
					pt->activeTidyPosition=0;
					}

				// Do Tidy
				if(pt->config->doTidy!=NULL)
					{
					pthread_mutex_unlock(&(pt->mutex));

					pt->config->doTidy(pt,workerNo,tidyPos);

					ret=pthread_mutex_lock(&(pt->mutex));
					if(ret!=0)
						{
						LOG(LOG_CRITICAL,"Failed to lock for performTask: %s",strerror(ret));
						}
					}
				}
			else
				{
//				LOG(LOG_INFO,"TIDY END WAIT");

				pthread_mutex_unlock(&(pt->mutex));

				pthread_barrier_wait(&(pt->tidyEndBarrier));

				ret=pthread_mutex_lock(&(pt->mutex));
				if(ret!=0)
						{
						LOG(LOG_CRITICAL,"Failed to lock for performTask: %s",strerror(ret));
						}

//				LOG(LOG_INFO,"TIDY END WAIT done");

				if(pt->state==PTSTATE_TIDY)
					pt->state=PTSTATE_ACTIVE;
				}
			}

		}

	LOG(LOG_INFO,"Worker %i Completed main loop",workerNo);

	// Release Lock at shutdown

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
	int alive=__sync_sub_and_fetch(&(pt->liveThreads),1);

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
		int (*doIngress)(ParallelTask *pt, int workerNo,void *ingressPtr, int ingressPosition, int ingressSize),
		int (*doIntermediate)(ParallelTask *pt, int workerNo, int force),
		int (*doTidy)(ParallelTask *pt, int workerNo, int tidyNo),
		int expectedThreads, int ingressBlocksize, int tasksPerTidy)
{
	LOG(LOG_INFO,"allocParallelTaskConfig");

	ParallelTaskConfig *ptc=ptParallelTaskConfigAlloc();

	ptc->doRegister=doRegister;
	ptc->doDeregister=doDeregister;
	ptc->doIngress=doIngress;
	ptc->doIntermediate=doIntermediate;
	ptc->doTidy=doTidy;

	ptc->expectedThreads=expectedThreads;
	ptc->ingressBlocksize=ingressBlocksize;
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

