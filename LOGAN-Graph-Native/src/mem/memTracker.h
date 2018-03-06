#ifndef __MEM_TRACKER_H
#define __MEM_TRACKER_H


// Top level stuff
#define MEMTRACKID_SWQ 0
#define MEMTRACKID_IOBUF 1
#define MEMTRACKID_THREADS 2
#define MEMTRACKID_INGRESS_QUEUE 3
#define MEMTRACKID_TASK 4

// Indexing
#define MEMTRACKID_INDEXING_BUILDER 5
#define MEMTRACKID_INDEXING_PACKSEQ 6

// Routing
#define MEMTRACKID_ROUTING_BUILDER 7
#define MEMTRACKID_ROUTING_WORKERSTATE 8

// Graph
#define MEMTRACKID_GRAPH 9
#define MEMTRACKID_SMER_ID 10
#define MEMTRACKID_SMER_ENTRY 11
#define MEMTRACKID_SMER_DATA 12
#define MEMTRACKID_BLOOM 13

// Dispenser
#define MEMTRACKID_DISPENSER 14

#define MEMTRACKID_DISPENSER_ROUTING 15
#define MEMTRACKID_DISPENSER_ROUTING_INGRESS 16
#define MEMTRACKID_DISPENSER_ROUTING_LOOKUP 17
#define MEMTRACKID_DISPENSER_ROUTING_LOOKUP_RECYCLE 18
#define MEMTRACKID_DISPENSER_ROUTING_DISPATCH 19
#define MEMTRACKID_DISPENSER_ROUTING_DISPATCH_GROUPSTATE 20
#define MEMTRACKID_DISPENSER_ROUTING_DISPATCH_ARRAY 21
#define MEMTRACKID_DISPENSER_GARBAGE_COLLECTOR 22
#define MEMTRACKID_DISPENSER_LINKED_SMER 23
#define MEMTRACKID_DISPENSER_STATS 24

// Heap
#define MEMTRACKID_HEAP 25
#define MEMTRACKID_HEAP_BLOCK 26
#define MEMTRACKID_HEAP_BLOCKDATA 27

// Bricks

#define MEMTRACKID_BRICK_SEQ 28
#define MEMTRACKID_BRICK_LOOKUP 29
#define MEMTRACKID_BRICK_DISPATCH 30

#define MEMTRACKID_SIZE 31


extern char *MEMTRACKER_NAMES[];

void mtInit();

void mtTrackAlloc(size_t size, s16 memTrackerId);
void mtTrackFree(size_t size, s16 memTrackerId);

void mtDump();

#endif
