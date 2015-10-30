/*
 * smer.h
 *
 *  Created on: Oct 28, 2015
 *      Author: tony
 */

#ifndef __TASK_H
#define __TASK_H


// Ingress task: Take new (serial) block from queue, process into intermediate tasks

// Intermediate task: Process intermediate task, perhaps create more intermediate tasks

// Cleanup task: After end of Ingress/Intermediate processing (with barrier), perform cleanup tasks

typedef struct parallelTaskStr ParallelTask;

typedef struct parallelTaskConfigStr
{
	void (*doRegister)(ParallelTask *pt, int workerNo);
	void (*doDeregister)(ParallelTask *pt, int workerNo);

	int (*doIngress)(ParallelTask *pt, int workerNo,int ingressNo);
	int (*doIntermediate)(ParallelTask *pt, int workerNo, int force);
	int (*doTidy)(ParallelTask *pt, int workerNo, int tidyNo);

	s32 expectedThreads;
	int tasksPerTidy;
} ParallelTaskConfig;


struct parallelTaskStr
{
	ParallelTaskConfig *config;
	void *dataPtr;

	// Updated by master to make ingress or shutodwn requests
	void *reqIngressPtr;
	int reqIngressCount;
	int reqShutdown;

	// Updated by worker on ingress acceptance & completion

	void *activeIngressPtr;
	int activeIngressCount;

	// One lock to rule them all
	pthread_mutex_t mutex;

	// Master can sync here
	pthread_cond_t master_startup;
	pthread_cond_t master_shutdown;

	int state;

	// Barriers for state transitions
	pthread_barrier_t  startupBarrier;
	pthread_barrier_t  shutdownBarrier;
	pthread_barrier_t  tidyStartBarrier;
	pthread_barrier_t  tidyEndBarrier;

	// Workers sleep here
	pthread_cond_t workers_idle;

	s32 liveThreads; // Threads which are somewhere in performTask


};




#define PTSTATE_STARTUP 0
#define PTSTATE_IDLE 1
#define PTSTATE_ACTIVE 2
#define PTSTATE_TIDY_WAIT 3
#define PTSTATE_TIDY 4
#define PTSTATE_SHUTDOWN 5
#define PTSTATE_DEAD 6

/* Global States:

0 Startup initiated: Waiting for worker registration. Complete when regThreads = expected
1 Idle: Waiting for ingress or shutdown
2 Active: Some or all threads should be working. Ingress allowed if exhausted and no tidyTasks. If activeThread == 0, Transition to idle or waiting for Tidy (and kick idles)

4 TIDY_WAIT: No threads working
5 Tidy: Tidy or wait for complete


8 Shutdown initiated: Waiting for worker deregistration
9 Shutdown completed:

Thread logic:

  doRegister

  startup barrier

  while(!Global_SHUTDOWN)
    {
    if(Global_IDLE)
    	{
    	if(reqIngress)
    	    Copy Ingress
    	    Global_ACTIVE
    	else if(shutdownReq)
    		Global_SHUTDOWN;
    	else
    		sleep(workers_idle)
    	}
    else if(Global_ACTIVE)
    	{
    	active_flag

    	while(active_flag)
       	   {
       	   no_active_flag

       	   if (priority intermediate) doInt(0); active_flag
       	   if (!active_flag and ingress) doIng(); active_flag

       	   if (no active_flag)
       	      {
       	      if (reqIngress and no ingress)
       	          Copy Ingress
       	          active_flag
       	      }

       	   if (!active_flag and intermediate) doInt(1); active_flag

       	   if(active_flag)
       	   	   wake(workers_idle)
       	   }

		if(activeThreads>0)
			sleep(workers_idle)
		else
			if(tidyTasks)
				{
				Global_TIDY_WAIT;
				wake(workers_idle);
				}
   			else
   				Global_IDLE;

       	}
    else if(Global_TIDY_WAIT)
    	{
    	tidyWaitBarrier

   		Global_TIDY;
    	}
    else if(Global_TIDY)
    	{
    	active_flag

    	while(active_flag)
       	   {
       	   no_active_flag
       	   if (tidyTask) doTidy(); active_flag
       	   }

		tidyEndBarrier

   		if(ingress)
   			Global_ACTIVE;
   		else
   			Global_IDLE;

     	}
    }

  shutdown barrier

  doUnregister




*/

ParallelTaskConfig *allocParallelTaskConfig(
		void (*doRegister)(ParallelTask *pt, int workerNo),
		void (*doDeregister)(ParallelTask *pt, int workerNo),
		int (*doIngress)(ParallelTask *pt, int workerNo, int ingressNo),
		int (*doIntermediate)(ParallelTask *pt, int workerNo, int force),
		int (*doTidy)(ParallelTask *pt, int workerNo, int tidyNo),
		int expectedThreads, int tasksPerTidy);

void freeParallelTaskConfig(ParallelTaskConfig *ptc);

ParallelTask *allocParallelTask(ParallelTaskConfig *ptc, void *dataPtr);
void freeParallelTask(ParallelTask *pt);


// Master entry points

void waitForStartup(ParallelTask *pt);
void waitForShutdown(ParallelTask *pt);

int queueIngress(ParallelTask *pt, void *ingressPtr, int ingressTotal);
void queueShutdown(ParallelTask *pt);

// Worker entry point

void performTask(ParallelTask *pt);



#endif
