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
	void *(*doRegister)(ParallelTask *pt, int workerNo, int totalWorkers);
	void (*doDeregister)(ParallelTask *pt, int workerNo, void *workerState);

	int (*allocateIngressSlot)(ParallelTask *pt, int workerNo);
	int (*doIngress)(ParallelTask *pt, int workerNo, void *workerState, void *ingressPtr, int ingressPosition, int ingressSize);
	int (*doIntermediate)(ParallelTask *pt, int workerNo, void *workerState, int force);
	int (*doTidy)(ParallelTask *pt, int workerNo, void *workerState, int tidyNo);

	s32 expectedThreads;
	int ingressBlocksize;

	int ingressPerTidyMin;
	int ingressPerTidyMax;
	int tidysPerBackoff;

	int tasksPerTidy;
} ParallelTaskConfig;


struct parallelTaskStr
{
	ParallelTaskConfig *config;
	void *dataPtr;

	// Updated by master to make ingress or shutodwn requests
	void *reqIngressPtr;
	int reqIngressTotal;
	int *reqIngressUsageCount;

	int reqShutdown;

	// Updated by worker on ingress acceptance & completion

	void *activeIngressPtr;
	int activeIngressTotal;
	int *activeIngressUsageCount;

	int activeIngressPosition;

	int activeTidyPosition;
	int activeTidyTotal;

	// Tidy

	int ingressPerTidyCounter;
	int ingressPerTidyTotal;
	int tidyBackoffCounter;

	// One lock to rule them all
	pthread_mutex_t mutex;

	// Master can sync here
	pthread_cond_t master_startup;
	pthread_cond_t master_ingress;
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
	s32 idleThreads; // Threads which are sleeping
};

// # Ingress buffers held by submitting task

#define PT_INGRESS_BUFFERS 4


#define PTSTATE_STARTUP 0
//#define PTSTATE_IDLE 1
#define PTSTATE_ACTIVE 1
#define PTSTATE_TIDY_WAIT 2
#define PTSTATE_TIDY 3
#define PTSTATE_SHUTDOWN_TIDY_WAIT 4
#define PTSTATE_SHUTDOWN_TIDY 5
#define PTSTATE_SHUTDOWN 6
#define PTSTATE_DEAD 7

/* Global States:

0 Startup initiated: Waiting for worker registration. Complete when regThreads = expected
1 Idle: Waiting for ingress or shutdown
2 Active: Some or all threads should be working. Ingress allowed if exhausted and no tidyTasks. If activeThread == 0, Transition to idle or waiting for Tidy (and kick idles)

4 TIDY_WAIT: No threads working
5 Tidy: Tidy or wait for complete


8 Shutdown initiated: Waiting for worker deregistration
9 Shutdown completed:



*/

ParallelTaskConfig *allocParallelTaskConfig(
		void *(*doRegister)(ParallelTask *pt, int workerNo, int totalWorkers),
		void (*doDeregister)(ParallelTask *pt, int workerNo, void *workerState),
		int (*allocateIngressSlot)(ParallelTask *pt, int workerNo),
		int (*doIngress)(ParallelTask *pt, int workerNo, void *workerState, void *ingressPtr, int ingressPosition, int ingressSize),
		int (*doIntermediate)(ParallelTask *pt, int workerNo, void *workerState, int force),
		int (*doTidy)(ParallelTask *pt, int workerNo, void *workerState, int tidyNo),
		int expectedThreads, int ingressBlocksize,
		int ingressPerTidyMin, int ingressPerTidyMax, int tidysPerBackoff, int tasksPerTidy);

void freeParallelTaskConfig(ParallelTaskConfig *ptc);

ParallelTask *allocParallelTask(ParallelTaskConfig *ptc, void *dataPtr);
void freeParallelTask(ParallelTask *pt);


// Master entry points

void waitForStartup(ParallelTask *pt);
void waitForShutdown(ParallelTask *pt);

// queueIngress may wait if an ingress already exists
int queueIngress(ParallelTask *pt, void *ingressPtr, int ingressCount, int *ingressUsageCount);

void queueShutdown(ParallelTask *pt);

// Worker entry point

void performTask_worker(ParallelTask *pt);



#endif
