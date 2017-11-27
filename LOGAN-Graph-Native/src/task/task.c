/*
 * task.c
 *
 *  Created on: Oct 28, 2015
 *      Author: tony
 */

#include "common.h"

// Measured in seconds - Should never be hit unless workers block
//#define MASTER_TIMEOUT (60*60*24)


static void wakeIdleWorkers(ParallelTask *pt)
{
	__atomic_fetch_add(&(pt->idleThreadPokeCount),1, __ATOMIC_SEQ_CST);
}


static void sleepIdleWorker(ParallelTask *pt, int workerNo)
{
	//LOG(LOG_INFO,"Sleep %i", workerNo);
	int pokeCount=__atomic_load_n(&(pt->idleThreadPokeCount),__ATOMIC_SEQ_CST);

	__atomic_fetch_add(&pt->idleThreads, 1, __ATOMIC_SEQ_CST);

	while(pokeCount==__atomic_load_n(&(pt->idleThreadPokeCount),__ATOMIC_SEQ_CST))
		{
		struct timespec req;

		req.tv_sec=DEFAULT_SLEEP_SECS;
		req.tv_nsec=DEFAULT_SLEEP_NANOS;

		nanosleep(&req, NULL);
		}

	__atomic_fetch_add(&pt->idleThreads, -1, __ATOMIC_SEQ_CST);
//	LOG(LOG_INFO,"Awake %i", workerNo);
}



void waitForStartup(ParallelTask *pt)
{
	LOG(LOG_INFO,"Master: Waiting for startup");

	int state=__atomic_load_n(&(pt->state), __ATOMIC_SEQ_CST);

	while(state==PTSTATE_STARTUP)
		{
		LOG(LOG_INFO,"Master: State is %i",state);

		struct timespec req;

		req.tv_sec=DEFAULT_SLEEP_SECS;
		req.tv_nsec=DEFAULT_SLEEP_NANOS;

		nanosleep(&req, NULL);
		state=__atomic_load_n(&(pt->state), __ATOMIC_SEQ_CST);
		}

	LOG(LOG_INFO,"Master: Done Startup");
}

void waitForShutdown(ParallelTask *pt)
{
	LOG(LOG_INFO,"Master: Waiting for shutdown");

	int state=__atomic_load_n(&(pt->state), __ATOMIC_SEQ_CST);

	while(state!=PTSTATE_DEAD)
		{
		struct timespec req;

		req.tv_sec=DEFAULT_SLEEP_SECS;
		req.tv_nsec=DEFAULT_SLEEP_NANOS;

		nanosleep(&req, NULL);

		if(pt->config->doTickTock != NULL)
			pt->config->doTickTock(pt);

		//wakeIdleWorkers(pt);

		state=__atomic_load_n(&(pt->state), __ATOMIC_SEQ_CST);
		}

	LOG(LOG_INFO,"Master: Done Shutdown %i %i",
			__atomic_load_n(&(pt->accumulatedIngressArrived),__ATOMIC_SEQ_CST),
			__atomic_load_n(&(pt->accumulatedIngressProcessed),__ATOMIC_SEQ_CST));
}


int queueIngress(ParallelTask *pt, ParallelTaskIngress *ingressData)
{
//	LOG(LOG_INFO,"Master: QueueIngress %p %i",ingressPtr,ingressCount);

	__atomic_store_n(ingressData->ingressUsageCount,1,__ATOMIC_SEQ_CST); // Allow for 'queued usage'

//	LOG(LOG_INFO,"Master: Queue Ingress %p %i - Usage %i", ingressData->ingressPtr, ingressData->ingressTotal, *(ingressData->ingressUsageCount));

	while(!queueTryInsert(pt->ingressQueue, ingressData))
		{
		struct timespec req;

		req.tv_sec=DEFAULT_SLEEP_SECS;
		req.tv_nsec=DEFAULT_SLEEP_NANOS;

		nanosleep(&req, NULL);

		if(pt->config->doTickTock != NULL)
			pt->config->doTickTock(pt);

		wakeIdleWorkers(pt);
		}

	if(pt->config->doTickTock != NULL)
		pt->config->doTickTock(pt);

	wakeIdleWorkers(pt);

	return 0;
}


void queueShutdown(ParallelTask *pt)
{
	LOG(LOG_INFO,"Master: QueueShutdown");

	__atomic_store_n(&pt->reqShutdown, 1, __ATOMIC_SEQ_CST);
	wakeIdleWorkers(pt);

	LOG(LOG_INFO,"Master: QueueShutdown Complete");
}

static int performTaskAcceptNewIngress(ParallelTask *pt)
{
	int oldToken=1;

	if(!__atomic_compare_exchange_n(&pt->ingressAcceptToken, &oldToken, 0, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		return 0;

	if(__atomic_load_n(&(pt->activeIngressPtr), __ATOMIC_SEQ_CST)!=NULL)
		{
		__atomic_store_n(&pt->ingressAcceptToken, 1, __ATOMIC_SEQ_CST);
		return 0;
		}

	ParallelTaskIngress *newIngress=queuePoll(pt->ingressQueue);

	if(newIngress!=NULL)
		{
		__atomic_fetch_add(&(pt->accumulatedIngressArrived), newIngress->ingressTotal, __ATOMIC_SEQ_CST);

		__atomic_store_n(&pt->activeIngressPosition, 0, __ATOMIC_RELAXED);
		__atomic_store_n(&pt->activeIngressPtr, newIngress, __ATOMIC_RELAXED);

		wakeIdleWorkers(pt);
		}

	__atomic_store_n(&pt->ingressAcceptToken, 1, __ATOMIC_SEQ_CST);

	return newIngress!=NULL;
}


static int changeState(ParallelTask *pt, int originState, int destinationState)
{
	int oldState=originState;

	if(__atomic_compare_exchange_n(&pt->state, &oldState, destinationState, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		return 1;

	return 0;
}

static int performTaskConsumeActiveIngress(ParallelTask *pt, int workerNo, RoutingWorkerState *wState)
{
	int oldToken=1;

	if(!__atomic_compare_exchange_n(&pt->ingressConsumeToken, &oldToken, 0, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		{
		return 0;
		}

	ParallelTaskIngress *ptIngressPtr=__atomic_load_n(&(pt->activeIngressPtr), __ATOMIC_SEQ_CST);

	if(ptIngressPtr==NULL)
		{
		__atomic_store_n(&pt->ingressConsumeToken, 1, __ATOMIC_SEQ_CST);
		return 0;
		}

	if((pt->config->allocateIngressSlot != NULL) && !(pt->config->allocateIngressSlot(pt,workerNo)))
		{
		__atomic_store_n(&pt->ingressConsumeToken, 1, __ATOMIC_SEQ_CST);
		return 0;
		}

	int ingressPos=__atomic_load_n(&(pt->activeIngressPosition), __ATOMIC_RELAXED);
	int ingressTotal=__atomic_load_n(&(ptIngressPtr->ingressTotal), __ATOMIC_RELAXED);
	int *ingressUsageCount = __atomic_load_n(&(ptIngressPtr->ingressUsageCount), __ATOMIC_RELAXED);

	int ingressSize=pt->config->ingressBlocksize;

	if(ingressPos+ingressSize > ingressTotal)
		ingressSize=ingressTotal-ingressPos;

	int ingressNewPosition=ingressPos+ingressSize;

	int newIngressFlag=0;

	if(ingressNewPosition>=ingressTotal) // If all consumed
		{
		newIngressFlag=1;

		__atomic_store_n(&(pt->activeIngressPtr), NULL, __ATOMIC_RELAXED);
		__atomic_store_n(&(pt->activeIngressPosition), 0, __ATOMIC_RELAXED);


		if(pt->config->tasksPerTidy>0)							// If tidy is configured
			{
			if(__atomic_sub_fetch(&(pt->ingressPerTidyCounter),1, __ATOMIC_SEQ_CST)<=0)
				{
				//LOG(LOG_INFO,"TIDY triggered");

				__atomic_store_n(&(pt->activeTidyTotal), pt->config->tasksPerTidy, __ATOMIC_SEQ_CST);
				__atomic_store_n(&(pt->activeTidyPosition) ,0, __ATOMIC_SEQ_CST);

				newIngressFlag=0;
				}
			}
		}
	else
		{
		__atomic_store_n(&(pt->activeIngressPosition), ingressNewPosition, __ATOMIC_SEQ_CST);
		__atomic_fetch_add(ingressUsageCount, 1, __ATOMIC_SEQ_CST); // Increase usage count if not last, to allow for 'queued usage'
		}

	__atomic_store_n(&(pt->ingressConsumeToken), 1, __ATOMIC_SEQ_CST);

	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	int ret=pt->config->doIngress(pt,workerNo, wState, ptIngressPtr->ingressPtr, ingressPos, ingressSize);
	__atomic_thread_fence(__ATOMIC_SEQ_CST);

	__atomic_fetch_add(&(pt->accumulatedIngressProcessed), ingressSize, __ATOMIC_SEQ_CST);

	__atomic_sub_fetch(ingressUsageCount, 1, __ATOMIC_RELEASE);

	if(newIngressFlag)
		performTaskAcceptNewIngress(pt);

	return ret;
}

static int performTaskActive(ParallelTask *pt, int workerNo, RoutingWorkerState *wState)
{
	int ret=0;

//	LOG(LOG_INFO,"performTaskActive %p ", pt->activeIngressPtr);


	// 1st priority - high priority intermediates
	if(pt->config->doIntermediate!=NULL)
		{
		__atomic_thread_fence(__ATOMIC_SEQ_CST);
		ret = pt->config->doIntermediate(pt,workerNo,wState,0);
		__atomic_thread_fence(__ATOMIC_SEQ_CST);

		if(ret)
			return 1;
		}

	// 2nd priority - active ingress

	if(__atomic_load_n(&(pt->activeIngressPtr), __ATOMIC_SEQ_CST)!=NULL)
		{
		if(performTaskConsumeActiveIngress(pt, workerNo, wState))
			return 1;
		}

	// 3rd priority - low priority intermediates
	if(pt->config->doIntermediate!=NULL)
		{
		__atomic_thread_fence(__ATOMIC_SEQ_CST);
		ret = pt->config->doIntermediate(pt,workerNo,wState, 1);
		__atomic_thread_fence(__ATOMIC_SEQ_CST);

		if(ret)
			return 1;
		}

	// 4th priority - tidy
	if(__atomic_load_n(&(pt->activeTidyTotal), __ATOMIC_SEQ_CST)>0)
		{
		// Last non-sleeping thread
		if(pt->idleThreads==pt->liveThreads-1)
			{
			//LOG(LOG_INFO,"Tidy activate");

			changeState(pt, PTSTATE_ACTIVE, PTSTATE_TIDY_WAIT);
			return 1;
			}
		else
			return 0;
		}

	// 5th priority - new ingress

	if(__atomic_load_n(&(pt->activeIngressPtr), __ATOMIC_SEQ_CST)==NULL)
		{
		if(performTaskAcceptNewIngress(pt))
			return 1;
		}

	// 6th priority - shutdown (hacky for now)
	if(queueIsEmpty(pt->ingressQueue) &&
			__atomic_load_n(&(pt->activeIngressPtr),__ATOMIC_SEQ_CST)==NULL &&
			__atomic_load_n(&(pt->reqShutdown), __ATOMIC_SEQ_CST))
		{
//		LOG(LOG_INFO,"New Shutdown Req");

		changeState(pt, PTSTATE_ACTIVE, PTSTATE_SHUTDOWN_TIDY_WAIT);
   		return 1;
		}

	return 0;
}


void performTidyWait(ParallelTask *pt)
{
//	LOG(LOG_INFO,"TIDY WAIT");

	pthread_barrier_wait(&(pt->tidyStartBarrier));

//	LOG(LOG_INFO,"TIDY WAIT done");
}

void performTidy(ParallelTask *pt, int workerNo, RoutingWorkerState *wState)
{
	int tidyTotal=__atomic_load_n(&(pt->activeTidyTotal), __ATOMIC_SEQ_CST);
	if(tidyTotal==0)
		return;

	int tidyPos=__atomic_fetch_add(&(pt->activeTidyPosition),1,__ATOMIC_SEQ_CST);

	if(tidyPos>=tidyTotal)
		{
		__atomic_store_n(&(pt->activeTidyTotal), 0, __ATOMIC_SEQ_CST);
		__atomic_store_n(&(pt->activeTidyPosition), 0, __ATOMIC_SEQ_CST);

		return;
		}

	// Do Tidy if handler set
	if(pt->config->doTidy!=NULL)
		{
		__atomic_thread_fence(__ATOMIC_SEQ_CST);
		pt->config->doTidy(pt,workerNo,wState, tidyPos);
		__atomic_thread_fence(__ATOMIC_SEQ_CST);
		}
}



void performTask_worker(ParallelTask *pt)
{
	int workerNo=-1;
	//int ret=0;

	/*
	 * Join steps:
	 * 	Increment live threads
	 * 	call register
	 *  Worker startup barrier
	 *  If first in, set state to idle and wake master
	 */

	workerNo=__atomic_fetch_add(&(pt->liveThreads),1, __ATOMIC_SEQ_CST);

	//LOG(LOG_INFO,"Worker %i Register",workerNo);

	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	RoutingWorkerState *wState=pt->config->doRegister(pt,workerNo,pt->config->expectedThreads);
	__atomic_thread_fence(__ATOMIC_SEQ_CST);

	//LOG(LOG_INFO,"Worker %i Startup Barrier wait",workerNo);

	//int startupWait=
	pthread_barrier_wait(&(pt->startupBarrier));

	//LOG(LOG_INFO,"Worker %i Startup Barrier done",workerNo);

	// If nobody else did it, update state and notify any waiting master

	changeState(pt, PTSTATE_STARTUP, PTSTATE_ACTIVE);

	int state=__atomic_load_n(&(pt->state), __ATOMIC_SEQ_CST);

	if(state!=PTSTATE_ACTIVE)
		{
		LOG(LOG_CRITICAL,"Failed / Unexpected state change %i at startup",state);
		}


	while(state!=PTSTATE_SHUTDOWN_TIDY_WAIT)
		{
		//LOG(LOG_INFO,"Worker %i looping %i",workerNo,pt->state);

		if(state==PTSTATE_ACTIVE)
			{
			if(!performTaskActive(pt,workerNo, wState))
				{
				sleepIdleWorker(pt, workerNo);
				}
			else
				{
				if(__atomic_load_n(&(pt->idleThreads),__ATOMIC_SEQ_CST)>0)
					wakeIdleWorkers(pt);
				}
			}
		else if(state==PTSTATE_TIDY_WAIT)
			{
			performTidyWait(pt);
			changeState(pt, PTSTATE_TIDY_WAIT, PTSTATE_TIDY);
			}
		else if(state==PTSTATE_TIDY)
			{
			if(pt->activeTidyTotal>0)
				{
				performTidy(pt, workerNo, wState);
				}
			else
				{
				pthread_barrier_wait(&(pt->tidyEndBarrier));

				if(changeState(pt, PTSTATE_TIDY, PTSTATE_ACTIVE))
					{
					if(__atomic_sub_fetch(&(pt->tidyBackoffCounter), 1, __ATOMIC_SEQ_CST)<=0)
						{
						__atomic_store_n(&(pt->tidyBackoffCounter), pt->config->tidysPerBackoff, __ATOMIC_SEQ_CST);

						int ingressPerTidyTotal=__atomic_load_n(&(pt->ingressPerTidyTotal), __ATOMIC_SEQ_CST)*2;

						if(ingressPerTidyTotal > pt->config->ingressPerTidyMax)
							ingressPerTidyTotal = pt->config->ingressPerTidyMax;

						__atomic_store_n(&(pt->ingressPerTidyTotal), ingressPerTidyTotal, __ATOMIC_SEQ_CST);
						}

					__atomic_store_n(&(pt->ingressPerTidyCounter), __atomic_load_n(&(pt->ingressPerTidyTotal), __ATOMIC_SEQ_CST), __ATOMIC_SEQ_CST);
					}
				}
			}

		state=__atomic_load_n(&(pt->state), __ATOMIC_SEQ_CST);
		}

	LOG(LOG_INFO,"Worker %i Completed main loop - waiting for shutdown tidy",workerNo);

	// Shutdown tidy - should configure separately

	performTidyWait(pt);

	changeState(pt, PTSTATE_SHUTDOWN_TIDY_WAIT, PTSTATE_SHUTDOWN_TIDY);

	while(pt->activeTidyTotal>0)
		performTidy(pt, workerNo, wState);

	changeState(pt, PTSTATE_SHUTDOWN_TIDY, PTSTATE_SHUTDOWN);

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

	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	pt->config->doDeregister(pt,workerNo,wState);
	__atomic_thread_fence(__ATOMIC_SEQ_CST);

	int alive=__atomic_sub_fetch(&(pt->liveThreads),1, __ATOMIC_SEQ_CST);

//	LOG(LOG_INFO,"Worker %i - still alive %i",workerNo,alive);

	if(alive==0)
		{
		LOG(LOG_INFO,"Worker %i last out, notifying master",workerNo);
		__atomic_store_n(&(pt->state), PTSTATE_DEAD, __ATOMIC_SEQ_CST);
		}

}







/* Alloc and Free */


ParallelTaskConfig *allocParallelTaskConfig(
		void *(*doRegister)(ParallelTask *pt, int workerNo, int totalWorkers),
		void (*doDeregister)(ParallelTask *pt, int workerNo, void *workerState),
		int (*allocateIngressSlot)(ParallelTask *pt, int workerNo),
		int (*doIngress)(ParallelTask *pt, int workerNo, void *workerState, void *ingressPtr, int ingressPosition, int ingressSize),
		int (*doIntermediate)(ParallelTask *pt, int workerNo, void *workerState, int force),
		int (*doTidy)(ParallelTask *pt, int workerNo, void *workerState, int tidyNo),
		void (*doTickTock)(ParallelTask *pt),
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
	ptc->doTickTock=doTickTock;

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

	pt->ingressQueue=queueAlloc(2);

	pt->reqShutdown=0;

	pt->activeIngressPtr=NULL;
	pt->ingressAcceptToken=1;
	pt->ingressConsumeToken=1;

	pt->accumulatedIngressArrived=0;
	pt->accumulatedIngressProcessed=0;

	pt->activeTidyPosition=0;
	pt->activeTidyTotal=0;

	pt->ingressPerTidyTotal=pt->ingressPerTidyCounter=ptc->ingressPerTidyMin;
	pt->tidyBackoffCounter=ptc->tidysPerBackoff;

	int ret;

	pt->state=PTSTATE_STARTUP;

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

	pt->idleThreadPokeCount=0;

	pt->liveThreads=0;
	pt->idleThreads=0;

	return pt;
}
void freeParallelTask(ParallelTask *pt)
{
	int ret;

	queueFree(pt->ingressQueue);

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


	ptParallelTaskFree(pt);
}

