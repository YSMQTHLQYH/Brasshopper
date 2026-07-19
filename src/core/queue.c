#include "queue.h"

_sQueue* QueueNew(Uint32 item_size, Uint32 reserve_count, Uint32 flags) {
	SDL_assert(item_size != 0);
	SDL_assert(reserve_count != 0);
	SDL_assert(flags == NULL); // make sure we can't set any flags by accident, as there aren't any flags to add yet

	_sArena* arena = ArenaNew((size_t)(reserve_count * item_size));
	_sQueue* q = ArenaAlloc(arena, QUEUE_HEADER_SIZE, alignof(_sQueue));
	q->data = (Uint8*)arena + arena->top;
	q->arena = arena;
	q->count = 0;
	q->item_size = item_size;
	size_t size = arena->capacity - arena->top;
	q->capacity = size / item_size;
	//q->flags = flags;
	q->front = 0;
	q->back = q->capacity - 1;

	arena->top = arena->capacity; // just to mark entire arena as used, TODO: add a function to get memory utilization of queue?

	return q;
}

void QueueFree(_sQueue* q) {
	SDL_assert(q != NULL);
	ArenaFree(q->arena);
}

static _sQueue* QueueGrow(_sQueue* q) {
	SDL_assert(q != NULL);
	SDL_assert(q->count <= q->capacity);
	Uint32 new_capacity = q->capacity + (q->capacity >> 1);
	//_sQueue* new_queue = QueueNew(q->item_size, new_capacity, q->flags);
	_sQueue* new_queue = QueueNew(q->item_size, new_capacity, NULL);
	new_queue->count = q->count;
	SDL_assert(new_queue->capacity > q->capacity);

	// copy data
	Uint32 front_to_end = q->capacity - q->front;
	void* src_ptr = (Uint8*)q->data + (q->front * q->item_size);
	if (front_to_end >= q->count) {
		// no wrap around
		SDL_assert(q->back == q->front + q->count - 1);
		SDL_memcpy(new_queue->data, src_ptr, q->count * q->item_size);
	}
	else {
		// wrap around
		SDL_assert(q->back < q->front);
		Uint32 end_count = q->capacity - q->front;
		Uint32 start_count = q->count - end_count;
		SDL_memcpy(new_queue->data, src_ptr, end_count * q->item_size);
		SDL_memcpy((Uint8*)new_queue->data + (end_count * q->item_size), q->data, start_count * q->item_size);
	}
	new_queue->front = 0;
	new_queue->back = q->count - 1;

	QueueFree(q);
	return new_queue;
}

void QueuePushFront(_sQueue** q_ptr, void* item_ptr, Uint32 count) {
	SDL_assert(q_ptr != NULL);
	SDL_assert(*q_ptr != NULL); // This takes a pointer to a pointer, as address of the queue might change
	_sQueue* q = *q_ptr;
	SDL_assert(q->count <= q->capacity);
	if (count == 0) return q;
	if (q->count + count > q->capacity) {
		q = QueueGrow(q);
		*q_ptr = q;
	}

	if (q->front < count) {
		/* data wraps around to the end of the arena */
		Uint32 start_count = q->front; // count of items that go at the start of arena
		size_t start_size = q->item_size * start_count;
		Uint32 end_count = count - start_count; // count of items that go at the end of arena (wrapping around)
		size_t end_size = q->item_size * end_count;
		q->count += count;
		q->front = q->capacity - end_count;
		SDL_assert(end_count > 0); // if this happens we didn't need to wrap around

		if (start_count > 0) {
			// copy part before wrap around
			void* src_ptr = (Uint8*)item_ptr + end_size;
			SDL_memcpy(q->data, src_ptr, start_size);
		}
		// copy part after wrap around
		void* dst_ptr = (Uint8*)q->data + ((q->capacity - end_count) * q->item_size);
		SDL_memcpy(dst_ptr, item_ptr, end_size);

		return;
	}

	// append to the front, no need to wrap around data
	q->front -= count;
	q->front %= q->capacity;
	q->count += count;
	size_t size = q->item_size * count;
	void* dst_ptr = (Uint8*)q->data + (q->front * q->item_size);
	SDL_memcpy(dst_ptr, item_ptr, size);

}

void QueuePushBack(_sQueue** q_ptr, void* item_ptr, Uint32 count) {
	SDL_assert(q_ptr != NULL);
	SDL_assert(*q_ptr != NULL); // This takes a pointer to a pointer, as address of the queue might change
	_sQueue* q = *q_ptr;
	SDL_assert(q->count <= q->capacity);
	if (count == 0) return q;
	if (q->count + count > q->capacity) {
		q = QueueGrow(q);
		*q_ptr = q;
	}

	if (q->back > q->capacity - count) {
		/* data wraps around to the start of the arena */
		Uint32 end_count = q->capacity - q->back; // count of items that go at the end of arena
		size_t end_size = q->item_size * end_count;
		Uint32 start_count = count - end_count; // count of items that go at the start of arena (wrapping around)
		size_t start_size = q->item_size * start_count;
		q->count += count;
		q->back = start_count;

		SDL_assert(start_count > 0); // if this happens we didn't need to wrap around

		if (end_count > 0) {
			// copy part before wrap around
			void* dst_ptr = (Uint8*)q->data + ((q->capacity - end_count) * q->item_size);
			SDL_memcpy(dst_ptr, item_ptr, end_size);
		}
		// copy part after wrap around
		void* src_ptr = (Uint8*)item_ptr + end_size;
		SDL_memcpy(q->data, src_ptr, start_size);

		return;
	}

	// append to the end, no need to wrap around data
	q->back += count;
	q->back %= q->capacity;
	q->count += count;
	size_t size = q->item_size * count;
	void* dst_ptr = (Uint8*)q->data + (q->back * q->item_size);
	SDL_memcpy(dst_ptr, item_ptr, size);

}


void QueuePopFront(_sQueue* q) {
	SDL_assert(q != NULL);
	SDL_assert(q->count <= q->capacity);
	SDL_assert(q->front <= q->capacity);
	q->count--;
	q->front++;
	if (q->front == q->capacity) {
		q->front = 0;
	}


}
void QueuePopBack(_sQueue* q) {
	SDL_assert(q != NULL);
	SDL_assert(q->count <= q->capacity);
	SDL_assert(q->back < q->capacity);
	q->count--;
	if (q->back == 0) {
		q->back = q->capacity - 1;
	}
	else {
		q->back--;
	}

}





void QueueRunTests() {
	_sQueue* q = QueueNew(sizeof(Uint32), 100, NULL);
	for (Uint32 i = 1; i < 101; i++) {
		QueuePushBack(&q, &i, 1);
		QueuePushBack(&q, &i, 1);
	}
	Uint32 cap = q->capacity;
	Uint32 count = q->count;
	//SDL_Log("cap %i, count %i", cap, count);
	Uint32 front = *QUEUE_FRONT(q, Uint32);
	Uint32 back = *QUEUE_BACK(q, Uint32);
	//SDL_Log("front: %i; back: %i", front, back);
	SDL_assert(front == 1 && back == 100);
	SDL_assert(q->front == 0 && q->back == 199);
	Uint32 bign = 1000000000;
	QueuePushFront(&q, &bign, 1);
	SDL_assert(q->front == cap - 1 && q->capacity == cap && q->count == count + 1);

	Uint32 a = *QUEUE_FRONT(q, Uint32);
	SDL_assert(a == bign);
	QueuePopFront(q);
	SDL_assert(q->count == count && q->capacity == cap && q->front == 0);

	Uint32 n = (q->capacity - q->count);
	for (Uint32 i = 0; i < n; i++) {
		Uint32 a = i + 100000;
		QueuePushBack(&q, &a, 1);
	}
	/*
	for (Uint32 i = 0; i < q->capacity; i++) {
		SDL_Log("int n %i: %i	ptr: %p", i, *((Uint32*)q->data + i), ((Uint32*)q->data + i));
	}
	*/
	SDL_assert(q->count == cap && q->capacity == cap && q->back == cap - 1);

	//loop back without allocating more
	QueuePopFront(q);
	QueuePushBack(&q, &n, 1);
	SDL_assert(q->count == cap && q->capacity == cap && q->back == 0 && q->front == 1);
	//needs to reallocate here
	QueuePushBack(&q, &n, 1);
	SDL_assert(q->count == cap + 1 && q->capacity > cap && q->back == cap && q->front == 0);
	/*
	for (Uint32 i = 0; i < q->capacity; i++) {
		SDL_Log("int n %i: %i	ptr: %p", i, *((Uint32*)q->data + i), ((Uint32*)q->data + i));
	}
	*/

	Uint32 cap2 = q->capacity;

	a = *QUEUE_BACK(q, Uint32);
	SDL_assert(a == n);
	QueuePopBack(q); // add just enough to fill without reallocating again
	n = (q->capacity - q->count);
	for (Uint32 i = 0; i < n; i++) {
		Uint32 a = i + 200000;
		QueuePushBack(&q, &a, 1);
	}
	/*
	for (Uint32 i = 0; i < q->capacity; i++) {
		SDL_Log("int n %i: %i	ptr: %p", i, *((Uint32*)q->data + i), ((Uint32*)q->data + i));
	}
	*/
	SDL_assert(q->count == cap2 && q->capacity == cap2 && q->back == cap2 - 1 && q->front == 0);
	//needs to reallocate here
	QueuePushFront(&q, &n, 1);
	SDL_assert(q->count == cap2 + 1 && q->capacity > cap2 && q->back == cap2 - 1 && q->front == q->capacity - 1); //allocating to front, so the end of new allocated block
	/*
	for (Uint32 i = 0; i < q->capacity; i++) {
		SDL_Log("int n %i: %i	ptr: %p", i, *((Uint32*)q->data + i), ((Uint32*)q->data + i));
	}
	*/

	QueueFree(q);
}