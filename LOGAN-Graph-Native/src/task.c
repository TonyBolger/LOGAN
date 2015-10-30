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




	LOG(LOG_INFO,"Master: Done Shutdown");
}




int queueIngress(ParallelTask *pt, void *ingressPtr, int ingressTotal)
{
	return 0;
}


void queueShutdown(ParallelTask *pt)
{

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

	LOG(LOG_INFO,"Worker %i Register",workerNo);

	pt->config->doRegister(pt,workerNo);

	LOG(LOG_INFO,"Worker %i Startup Barrier wait",workerNo);

	//int startupWait=
	pthread_barrier_wait(&(pt->startupBarrier));

	LOG(LOG_INFO,"Worker %i Startup Barrier done",workerNo);

	// Lock should be held except when sleeping or working

	ret=pthread_mutex_lock(&(pt->mutex));
	if(ret!=0)
			{
			LOG(LOG_CRITICAL,"Failed to lock for performTask: %s",strerror(ret));
			}

	// If nobody else did it, update state and notify any waiting master
	if(pt->state==PTSTATE_STARTUP)
		{
		pt->state=PTSTATE_IDLE;
		pthread_cond_broadcast(&(pt->master_startup));
		}

	LOG(LOG_INFO,"Worker %i Entering main loop",workerNo);

	while(pt->state!=PTSTATE_SHUTDOWN)
		{
		LOG(LOG_INFO,"Worker %i looping",workerNo);

    	if(pt->reqShutdown)
    		pt->state=PTSTATE_SHUTDOWN;
    	else
    		{
    		pthread_cond_wait(&(pt->workers_idle),&(pt->mutex));
    		}
		}

	LOG(LOG_INFO,"Worker %i Completed main loop",workerNo);

	// Release Lock at shutdown

	pthread_mutex_unlock(&(pt->mutex));

	/*
	 * Leave steps:
	 * Worker shutdown barrier
	 * Call deregister
	 * Increment live threads
	 * If last out, set state to dead and wake masters
	 */

	LOG(LOG_INFO,"Worker %i Shutdown Barrier wait",workerNo);

	//int shutdownWait=
	pthread_barrier_wait(&(pt->shutdownBarrier));

	LOG(LOG_INFO,"Worker %i Deregister",workerNo);

	pt->config->doDeregister(pt,workerNo);
	int alive=__sync_sub_and_fetch(&(pt->liveThreads),1);

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
		int (*doIngress)(ParallelTask *pt, int workerNo, int ingressNo),
		int (*doIntermediate)(ParallelTask *pt, int workerNo, int force),
		int (*doTidy)(ParallelTask *pt, int workerNo, int tidyNo),
		int expectedThreads, int tasksPerTidy)
{
	ParallelTaskConfig *ptc=ptParallelTaskConfigAlloc();

	ptc->doRegister=doRegister;
	ptc->doDeregister=doDeregister;
	ptc->doIngress=doIngress;
	ptc->doIntermediate=doIntermediate;
	ptc->doTidy=doTidy;

	ptc->expectedThreads=expectedThreads;
	ptc->tasksPerTidy=tasksPerTidy;

	return ptc;
}

void freeParallelTaskConfig(ParallelTaskConfig *ptc)
{
	ptParallelTaskConfigFree(ptc);
}

ParallelTask *allocParallelTask(ParallelTaskConfig *ptc, void *dataPtr)
{
	ParallelTask *pt=ptParallelTaskAlloc();

	pt->config=ptc;
	pt->dataPtr=dataPtr;

	pt->reqIngressPtr=NULL;
	pt->reqIngressCount=0;
	pt->reqShutdown=0;

	pt->activeIngressPtr=NULL;
	pt->activeIngressCount=0;

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

