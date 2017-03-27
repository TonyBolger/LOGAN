#ifndef __QUEUE_H
#define __QUEUE_H


typedef struct queueStr {
	u32 size;
	u32 mask;

	u32 headAlloc;
	u32 headValid;
	u32 tail;

	u32 pad0;
	u32 pad1;
	u32 pad2;

	void *entries[];
} Queue;


/*

	TryInsert: Reserve slot (Atomic Inc headAlloc while checking space), wait until slot+size<tailAlloc, write entry, wait for other writers, set headValid

	Poll: Check for entries, tmp copy next entry, consume (CAS inc tailAlloc), return tmp entry

*/


Queue *queueAlloc(int sizePower);
void queueFree(Queue *queue);

u32 queueTryInsert(Queue *queue, void *entry);
void queueInsert(Queue *queue, void *entry);

void *queuePoll(Queue *queue);
void *queueRemove(Queue *queue);

int queueIsEmpty(Queue *queue);

#endif
