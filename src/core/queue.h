#pragma once

#include "SDL3/SDL.h"
#include "arena.h"


/*
* Dual ended circular queue that grow in size, reallocating memory if necessary
* DO NOT STORE POINTERS TO ITEMS INSIDE THE QUEUE
*/
typedef struct {
	void* data;
	_sArena* arena; // handled by the queue itself
	Uint32 count;
	Uint32 item_size;
	Uint32 capacity; // amount of items that fit before needing to reallocate
	//Uint32 flags;
	Uint32 front; // index of front item of the queue
	Uint32 back;  // index of back item of the queue
}_sQueue;
#define QUEUE_HEADER_SIZE	64
SDL_COMPILE_TIME_ASSERT(queue_header_size, sizeof(_sQueue) <= QUEUE_HEADER_SIZE);


/*
* Makes a new queue, it manages it's own memory arena
* item_size is size (in bytes) of one item
* reserve_count is minimum amount of items it will fit before needing to reallocate
* free with QueueFree()
* retruns pointer to the new queue
* flags is one or more flags or'd together, currently no flags are implemented so it should just be NULL (only there for future use)
*/
_sQueue* QueueNew(Uint32 item_size, Uint32 reserve_count, Uint32 flags);
// frees the entire queue
void QueueFree(_sQueue* q);


/*
* Adds item(s) to the start/end of the queue, reallocating if necessary
* item_ptr is a pointer to the item to Adds(or array of items if multiple)
* count is number of items to add
* returns pointer to queue (changes if array had to be reallocated)
*/
_sQueue* QueuePushFront(_sQueue* q, void* item_ptr, Uint32 count);
_sQueue* QueuePushBack(_sQueue* q, void* item_ptr, Uint32 count);

/*
* Removes an item from the start/end of the queue
* The item gets freed immediately, so it doesn't return anything
* To actually do something with the item call QUEUE_FRONT()/QUEUE_BACK(), do whatever and once you are done call this
*/
void QueuePopFront(_sQueue* q);
void QueuePopBack(_sQueue* q);

// returns pointer to front/back item
#define QUEUE_FRONT(q)	((Uint8*)q->data + (q->front * q->item_size))
#define QUEUE_BACK(q)	((Uint8*)q->data + (q->back* q->item_size))


