#include "common.h"


Queue *queueAlloc(int sizePower)
{
	int size=1<<sizePower;

	Queue *queue=gAlloc(sizeof(Queue)+sizeof(void *)*size);

	queue->size=size;
	queue->mask=size-1;
	queue->headAlloc=queue->headValid=queue->tail=0;

	return queue;
}

void queueFree(Queue *queue)
{
	gFree(queue);
}

u32 queueTryInsert(Queue *queue, void *entry)
{
//	LOG(LOG_INFO,"Queue tryInsert");

    u32 testHead;
    u32 newTestHead;
    u32 tail;

    do
    	{
        testHead = __atomic_load_n(&(queue->headAlloc), __ATOMIC_SEQ_CST);
        newTestHead = testHead+1;
        tail = __atomic_load_n(&(queue->tail), __ATOMIC_SEQ_CST);

        if ((newTestHead & queue->mask) == (tail & queue->mask))
            return 0; // No space

    } while (!__atomic_compare_exchange_n(&(queue->headAlloc), &testHead, newTestHead, 0,__ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));

    __atomic_store(queue->entries+(testHead&queue->mask), &entry, __ATOMIC_SEQ_CST);

    // Allows for multiple parallel inserts: should be rare
    while (!__atomic_compare_exchange_n(&(queue->headValid), &testHead, newTestHead, 0,__ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
        sched_yield();

//    LOG(LOG_INFO,"Inserted to %i", testHead);

    return 1;
}

void queueInsert(Queue *queue, void *entry)
{
//	LOG(LOG_INFO,"Queue Insert");

	int res=queueTryInsert(queue, entry);

	while(!res)
		{
		struct timespec req;

		req.tv_sec=DEFAULT_SLEEP_SECS;
		req.tv_nsec=DEFAULT_SLEEP_NANOS;

		nanosleep(&req, NULL);

		res=queueTryInsert(queue, entry);
		}

//	LOG(LOG_INFO,"Queue Insert Done");
}



void *queuePoll(Queue *queue)
{
//	LOG(LOG_INFO,"Queue poll");

	u32 head;
	u32 tail;

    do
    {
        head=__atomic_load_n(&(queue->headValid), __ATOMIC_SEQ_CST);
        tail=__atomic_load_n(&(queue->tail), __ATOMIC_SEQ_CST);

        if (tail == head)
        	{
//        	LOG(LOG_INFO,"Queue poll - empty %i %i",head,tail);

            return NULL;
        	}

        void *tmpEntry = __atomic_load_n(queue->entries+(tail&queue->mask), __ATOMIC_SEQ_CST);

        if (__atomic_compare_exchange_n(&(queue->tail), &tail, (tail + 1), 0,__ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
        	{
//        	LOG(LOG_INFO,"Queue poll - got entry");

            return tmpEntry;
        	}

    } while(1); // loop if CAS fails

	return NULL;
}


void *queueRemove(Queue *queue)
{
	void *entry=queuePoll(queue);

	while(entry==NULL)
		{
		struct timespec req;

		req.tv_sec=DEFAULT_SLEEP_SECS;
		req.tv_nsec=DEFAULT_SLEEP_NANOS;

		nanosleep(&req, NULL);

		entry=queuePoll(queue);
		}

	return entry;
}

int queueIsEmpty(Queue *queue)
{
	int headAlloc=__atomic_load_n(&(queue->headValid), __ATOMIC_SEQ_CST);
	int tail=__atomic_load_n(&(queue->tail), __ATOMIC_SEQ_CST);

	if(headAlloc==tail)
		return 1;

	return 0;
}

