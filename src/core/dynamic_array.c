#include "dynamic_array.h"

_sDynamicArray* DArrayNew(Uint32 item_size, Uint32 reserve_count, Uint32 flags) {
	SDL_assert(item_size != 0);
	SDL_assert(reserve_count != 0);
	SDL_assert((flags & (DARRAY_FLAG_ORDERED | DARRAY_FLAG_UNORDERED)) != 0);

	_sArena* arena = ArenaNew(reserve_count * item_size);
	_sDynamicArray* darray = ArenaAlloc(arena, DYNAMIC_ARRAY_HEADER_SIZE, alignof(_sDynamicArray));
	darray->data = arena->top;
	darray->arena = arena;
	darray->count = 0;
	darray->item_size = item_size;
	size_t size = arena->capacity - arena->top;
	darray->capacity = size / item_size;
	darray->flags = flags;

	return darray;
}

void DArrayFree(_sDynamicArray* darray) {
	SDL_assert(darray != NULL);
	ArenaFree(darray->arena);
}

static _sDynamicArray* DArrayGrow(_sDynamicArray* darray) {
	SDL_assert(darray != NULL);
	Uint32 new_capacity = darray->capacity + (darray->capacity >> 1);
	_sDynamicArray* new_darray = DArrayNew(darray->item_size, new_capacity, darray->flags);
	SDL_memcpy(new_darray->data, darray->data, darray->count * darray->item_size);
	new_darray->count = darray->count;
	SDL_assert(new_darray->capacity > darray->capacity);
	DArrayFree(darray);
	return new_darray;
}

_sDynamicArray* DArrayAppend(_sDynamicArray* darray, void* item_ptr, Uint32 count) {
	SDL_assert(darray != NULL);
	if (count == 0)return;
	while (darray->count + count > darray->capacity) {
		darray = DArrayGrow(darray);
	}
	void* end = (Uint8*)darray->data + (darray->count * darray->item_size);
	SDL_memcpy(end, item_ptr, (size_t)(count * darray->item_size));
	darray->count += count;
	return darray;
}


void DArrayRemoveLast(_sDynamicArray* darray, Uint32 count) {
	SDL_assert(darray != NULL);
	if (count >= darray->count) {
		darray->count = 0;
	}
	else {
		darray->count -= count;
	}
}

_sDynamicArray* DArrayInsert(_sDynamicArray* darray, void* item_ptr, Uint32 index) {
	SDL_assert(darray != NULL);
	SDL_assert(index <= darray->count);
	if (darray->count + 1 > darray->capacity) {
		// ideally we would do a grow_without_copy function, copy part before insersion, insert and then copy the rest
		// saves copying twice, but we probably won't be on that stuation often
		// i'll implement that if this becomes a problem
		darray = DArrayGrow(darray);
	}
	void* copy_dst = (Uint8*)darray->data + ((index + 1) * darray->item_size);
	void* insert_dst = (Uint8*)darray->data + (index * darray->item_size);
	SDL_memmove(copy_dst, insert_dst, (darray->count - index) * darray->item_size);
	SDL_memcpy(insert_dst, item_ptr, darray->item_size);
	darray->count++;
	return darray;
}

void DArrayRemoveOrdered(_sDynamicArray* darray, Uint32 index) {
	SDL_assert(darray != NULL);
	SDL_assert(darray->flags & DARRAY_FLAG_ORDERED);
	SDL_assert(index <= darray->count);
	void* copy_src = (Uint8*)darray->data + ((index + 1) * darray->item_size);
	void* remove_dst = (Uint8*)darray->data + (index * darray->item_size);
	darray->count--;
	SDL_memmove(remove_dst, copy_src, (darray->count - index) * darray->item_size);
}

void DArrayRemoveUnordered(_sDynamicArray* darray, Uint32 index) {
	SDL_assert(darray != NULL);
	SDL_assert(darray->flags & DARRAY_FLAG_UNORDERED);
	SDL_assert(index <= darray->count);
	void* last = (Uint8*)darray->data + (--darray->count * darray->item_size);
	void* remove_dst = (Uint8*)darray->data + (index * darray->item_size);
	SDL_memcpy(remove_dst, last, darray->item_size);
}
