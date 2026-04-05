#include "arena.h"

#define ARENA_MAX_CHAIN_LENGTH	16

static _sArena* ArenaCreate(size_t capacity);
static void ArenaDelete(_sArena* arena);

//TODO actually implement this
_sArena* ArenaNew(size_t capacity) {
	SDL_assert(capacity != 0);
	return ArenaCreate(capacity);
}
//TODO actually implement this
void ArenaFree(_sArena* arena) {
	SDL_assert(arena != NULL);
	ArenaDelete(arena);
}

void* ArenaAlloc(_sArena* arena, size_t size, size_t align) {
	SDL_assert(arena != NULL);
	//check if alignment is power of two
	SDL_assert(SDL_HasExactlyOneBitSet32((Uint32)align));
	//check only works on 32 bit numbers but if we are asking to align by more than 4GB we are doing something wrong anyways
	SDL_assert(align <= UINT32_MAX);
	SDL_assert(align <= arena->capacity);
	SDL_assert(size <= arena->capacity - ARENA_HEADER_SIZE);
	SDL_assert(arena->top <= arena->capacity);

	if (arena->next != NULL) {
		return ArenaAlloc(arena->next, size, align);
	}
	//this IS the last arena in chain

	size_t padding = arena->top % align;
	if (padding != 0) {
		padding = align - padding;
	}

	//check if it fits in this arena or we need to make next arena in chain
	if ((arena->top + padding + size) > arena->capacity) {
		//arena->capacity is the actual memory allocated, the argument is the minimun usable required (after the header takes it piece)
		arena->next = ArenaNew(arena->capacity - ARENA_HEADER_SIZE); 
		return ArenaAlloc(arena->next, size, align);
	}

	void* ret = arena + arena->top + padding;
	arena->top += size + padding;

	return ret;
}


size_t ArenaGetTopPosition(const _sArena* arena) {
	SDL_assert(arena != NULL);
	if (arena->next == NULL) {
		return arena->top;
	}
	return arena->capacity + ArenaGetTopPosition(arena->next);
}

void ArenaPopTo(_sArena* arena, size_t new_top_position){
	SDL_assert(arena != NULL);
	SDL_assert(new_top_position >= ARENA_HEADER_SIZE);

	//check if new top is inside this arena
	if (new_top_position <= arena->capacity) {
		arena->top = new_top_position;
		if (arena->next != NULL) {
			//we just popped off everything on the next arena
			ArenaFree(arena->next);
			arena->next = NULL;
		}
		return;
	}
	SDL_assert(arena->next != NULL);
	ArenaPopTo(arena->next, new_top_position - arena->capacity);
}

Uint32 ArenaGetChainLength(_sArena* arena) {
	SDL_assert(arena != NULL);
	_sArena* a = arena;
	Uint32 length = 0;
	while (a != NULL) {
		length++;
		a = a->next;
		// technically allowed but if this happens we are surely doing something wrong (probably memory leak)
		SDL_assert(length < ARENA_MAX_CHAIN_LENGTH);
	}
#ifdef DEBUG
	if (length >= (ARENA_MAX_CHAIN_LENGTH / 4)) {
		SDL_LogMessage(SDL_LOG_CATEGORY_ASSERT, SDL_LOG_PRIORITY_WARN, "WARNING: Arena %p chain length: %i", arena, length);
	}
#endif

	return length;
}
void ArenaLog(_sArena* arena, SDL_LogCategory category, SDL_LogPriority priority) {
	SDL_assert(arena != NULL);
	Uint32 length = ArenaGetChainLength(arena);
	SDL_LogMessage(category, priority, "Arena %p chain length: %i", arena, length);
	_sArena* a = arena;
	for (Uint32 i = 0; i < length;i++) {
		if (a == NULL)break;
		SDL_LogMessage(category, priority, "Arena %p depth: %i Capacity: %"SDL_PRIu64" Top: %"SDL_PRIu64, a, i, a->capacity, a->top);
		a = a->next;
	}
}


/*
* Makes a new arena, allocates memory for it
* Arg capacity is the minimun capacity of the arena, actual capacity may be higher
* Call ArenaAlloc on it to get a pointer to the actual memory you can use inside it
* Returns pointer to the new arena
*/
static _sArena* ArenaCreate(size_t capacity) {
	SDL_assert(capacity != 0);
	size_t page_size = SDL_GetSystemPageSize();
	if (page_size == 0) page_size = KiB(4);

	capacity += ARENA_HEADER_SIZE;
	size_t mod = capacity % page_size;
	if (mod != 0) {
		capacity -= mod;
		capacity += page_size;
	}

	_sArena* arena = SDL_aligned_alloc(page_size, capacity);
	if (!arena) return (void*)SDL_OutOfMemory();

	arena->next = NULL;
	arena->capacity = capacity;
	arena->top = ARENA_HEADER_SIZE;

	return arena;
}
/*
* Deletes the entire arena and deallocates it's memory
*/
static void ArenaDelete(_sArena* arena) {
	SDL_assert(arena != NULL);
	if (arena->next != NULL) {
		ArenaDelete(arena->next);
	}
	SDL_aligned_free(arena);
}

void ArenaRunTests() {
	_sArena* arena = ArenaNew(KiB(16) - ARENA_HEADER_SIZE);
	int page_size = SDL_GetSystemPageSize();
	if (page_size == 0) page_size = KiB(4);
	SDL_assert(arena->capacity == KiB(16));
	SDL_assert((Uint64)arena % page_size == 0);
	//ArenaLog(arena, SDL_LOG_CATEGORY_TEST, SDL_LOG_PRIORITY_VERBOSE);
	ArenaAlloc(arena, KiB(2) - ARENA_HEADER_SIZE, alignof(Uint32));
	size_t pos = ArenaGetTopPosition(arena);
	SDL_assert(pos == KiB(2));
	ArenaAlloc(arena, KiB(6), alignof(Uint32));
	ArenaAlloc(arena, KiB(8), alignof(Uint32));
	SDL_assert(ArenaGetChainLength(arena) == 1);
	ArenaAlloc(arena, 1, alignof(Uint32));
	SDL_assert(ArenaGetChainLength(arena) == 2);
	ArenaAlloc(arena, KiB(16) - ARENA_HEADER_SIZE - 1, alignof(Uint32));
	SDL_assert(ArenaGetChainLength(arena) == 3);
	//ArenaLog(arena, SDL_LOG_CATEGORY_TEST, SDL_LOG_PRIORITY_VERBOSE);
	ArenaPopTo(arena, pos);
	SDL_assert(ArenaGetChainLength(arena) == 1);
	ArenaAlloc(arena, 1, alignof(Uint32));
	ArenaAlloc(arena, 1, alignof(Uint32));
	SDL_assert(arena->top == KiB(2) + 5);
	//ArenaLog(arena, SDL_LOG_CATEGORY_TEST, SDL_LOG_PRIORITY_VERBOSE);
	for (Uint32 i = 0; i < 15; i++) {
		ArenaAlloc(arena, KiB(12), alignof(Uint32));
	}
	//ArenaLog(arena, SDL_LOG_CATEGORY_TEST, SDL_LOG_PRIORITY_VERBOSE);
	ArenaFree(arena);
}