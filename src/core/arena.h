#pragma once

#include "SDL3/SDL.h"
#include <stdalign.h>

#define KiB(s)	s * 1024
#define MiB(s)	KiB(s) * 1024

typedef struct _sArena {
	struct _sArena* next;
	size_t capacity;
	size_t top;
}_sArena;
#define ARENA_HEADER_SIZE	64
SDL_COMPILE_TIME_ASSERT(arena_header_size, sizeof(_sArena) <= ARENA_HEADER_SIZE);



/*
* Finds an empty arena that was already allocated but is no longer used
* If it couldn't find any it creates a new arena and allocates memory for it
* Arg capacity is the minimun capacity of the arena, actual capacity may be higher
* Call ArenaAlloc on it to get a pointer to the actual memory you can use inside it
* Returns pointer to the new arena
* TODO: actually implement this (for now it just creates)
*/
_sArena* ArenaNew(size_t capacity);
/*
* Frees the entire arena, instead of deleting it and deallocating the memory,
* the arena is just kept for the next time an arena is requested
* TODO: actually implement this (for now it just deletes)
*/
void ArenaFree(_sArena* arena);


void* ArenaAlloc(_sArena* arena, size_t size, size_t align);

// gets position of top of the arena (offset from the start, not the actual pointer)
// if there are multiple arenas linked together it will return a value greater than arena->capacity
size_t ArenaGetTopPosition(const _sArena* arena);

// "proper" pop is not supported, due to padding being potentially variable and we aren't tracking it
// instead, you store the result of ArenaGetTopPosition() before allocating temporary stuff
// and use this function to reset to that 
void ArenaPopTo(_sArena* arena, size_t new_position);
#define ArenaReset(arena)	ArenaPopTo(arena, ARENA_HEADER_SIZE)


Uint32 ArenaGetChainLength(_sArena* arena);
void ArenaLog(_sArena* arena, SDL_LogCategory category, SDL_LogPriority priority);





void ArenaRunTests();


