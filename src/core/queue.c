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
	q->flags = flags;
	q->front = 0;
	q->back = 0;

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
	_sQueue* new_queue = QueueNew(q->item_size, new_capacity, q->flags);
	new_queue->count = q->count;
	SDL_assert(new_queue->capacity > q->capacity);
	new_queue->arena->top = new_queue->arena->capacity;

	// copy data
	Uint32 front_to_end = q->capacity - q->front;
	void* src_ptr = (Uint8)q->data + (q->front * q->item_size);
	if (front_to_end <= q->count) {
		// no wrap around
		SDL_assert(q->back == q->front + q->count);
		SDL_memcpy(new_queue->data, src_ptr, q->count * q->item_size);
		new_queue->front = 0;
		new_queue->back = q->count;
	}
	else {
		// wrap around
		SDL_assert(q->back < q->front);
		Uint32 end_count = q->capacity - q->front;
		Uint32 start_count = q->count - end_count;
		SDL_memcpy(new_queue->data, src_ptr, q->end_count * q->item_size);
		SDL_memcpy(new_queue->data, q->data, start_count * q->item_size);
	}

	QueueFree(q);
	return new_queue;
}

_sQueue* QueuePushFront(_sQueue* q, void* item_ptr, Uint32 count) {
	SDL_assert(q != NULL);
	SDL_assert(q->count <= q->capacity);
	if (count == 0) return q;
	if (q->count + count > q->capacity) {
		q = QueueGrow(q);
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

		return q;
	}

	// append to the front, no need to wrap around data
	q->front -= count;
	q->front %= q->capacity;
	q->count += count;
	size_t size = q->item_size * count;
	void* dst_ptr = (Uint8*)q->data + (q->front * q->item_size);
	SDL_memcpy(dst_ptr, item_ptr, size);

	return q;
}

_sQueue* QueuePushBack(_sQueue* q, void* item_ptr, Uint32 count) {
	SDL_assert(q != NULL);
	SDL_assert(q->count <= q->capacity);
	if (count == 0) return q;
	if (q->count + count > q->capacity) {
		q = QueueGrow(q);
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

		return q;
	}

	// append to the end, no need to wrap around data
	q->back += count;
	q->back %= q->capacity;
	q->count += count;
	size_t size = q->item_size * count;
	void* dst_ptr = (Uint8*)q->data + (q->back * q->item_size);
	SDL_memcpy(dst_ptr, item_ptr, size);

	return q;
}


void QueuePopFront(_sQueue* q) {
	SDL_assert(q != NULL);
	SDL_assert(q->count <= q->capacity);
	SDL_assert(q->front <= q->capacity);
	q->count--;
	if (q->front == q->capacity) {
		q->front = 0;
	}
	else {
		q->front++;
	}

}
void QueuePopBack(_sQueue* q) {
	SDL_assert(q != NULL);
	SDL_assert(q->count <= q->capacity);
	SDL_assert(q->back < q->capacity);
	q->count--;
	if (q->back == 0) {
		q->back = q->capacity;
	}
	q->back--;
}