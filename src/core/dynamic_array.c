#include "dynamic_array.h"

_sDynamicArray* DArrayNew(Uint32 item_size, Uint32 reserve_count, Uint32 flags) {
	SDL_assert(item_size != 0);
	SDL_assert(reserve_count != 0);
	SDL_assert((flags & (DARRAY_FLAG_ORDERED | DARRAY_FLAG_UNORDERED)) != 0);

	_sArena* arena = ArenaNew((size_t)(reserve_count * item_size));
	_sDynamicArray* darray = ArenaAlloc(arena, DYNAMIC_ARRAY_HEADER_SIZE, alignof(_sDynamicArray));
	darray->data = (Uint8*)arena + arena->top;
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

static void DArraySetArenaTop(_sDynamicArray* darray) {
	darray->arena->top = (darray->count * darray->item_size) + ARENA_HEADER_SIZE + DYNAMIC_ARRAY_HEADER_SIZE;
}

static _sDynamicArray* DArrayGrow(_sDynamicArray* darray) {
	SDL_assert(darray != NULL);
	Uint32 new_capacity = darray->capacity + (darray->capacity >> 1);
	_sDynamicArray* new_darray = DArrayNew(darray->item_size, new_capacity, darray->flags);
	SDL_memcpy(new_darray->data, darray->data, darray->count * darray->item_size);
	new_darray->count = darray->count;
	SDL_assert(new_darray->capacity > darray->capacity);
	new_darray->arena->top = darray->arena->top;
	DArrayFree(darray);
	return new_darray;
}

void DArrayAppend(_sDynamicArray** darray_ptr, const void* item_ptr, Uint32 count) {
	SDL_assert(darray_ptr != NULL);
	SDL_assert(*darray_ptr != NULL); // This takes a pointer to a pointer, as address of the dynamic array might change
	_sDynamicArray* darray = *darray_ptr;
	if (count == 0) return darray;
	if (darray->count + count > darray->capacity) {
		darray = DArrayGrow(darray);
		*darray_ptr = darray;
	}

	size_t size = darray->item_size * count;
	void* append_ptr = (Uint8*)darray->arena + darray->arena->top;
	SDL_memcpy(append_ptr, item_ptr, size);
	darray->count += count;
	DArraySetArenaTop(darray);
}


void DArrayRemoveLast(_sDynamicArray* darray, Uint32 count) {
	SDL_assert(darray != NULL);
	if (count >= darray->count) {
		darray->count = 0;
	}
	else {
		darray->count -= count;
	}
	DArraySetArenaTop(darray);
}

void DArrayInsert(_sDynamicArray** darray_ptr, void* item_ptr, Uint32 index) {
	SDL_assert(darray_ptr != NULL);
	SDL_assert(*darray_ptr != NULL); // This takes a pointer to a pointer, as address of the dynamic array might change
	_sDynamicArray* darray = *darray_ptr;
	SDL_assert(index <= darray->count);
	if (darray->count + 1 > darray->capacity) {
		// ideally we would do a grow_without_copy function, copy part before insersion, insert and then copy the rest
		// saves copying twice, but we probably won't be on that stuation often
		// i'll implement that if this becomes a problem
		darray = DArrayGrow(darray);
		*darray_ptr = darray;
	}
	void* copy_dst = (Uint8*)darray->data + ((index + 1) * darray->item_size);
	void* insert_dst = (Uint8*)darray->data + (index * darray->item_size);
	SDL_memmove(copy_dst, insert_dst, (darray->count - index) * darray->item_size);
	SDL_memcpy(insert_dst, item_ptr, darray->item_size);
	darray->count++;
	DArraySetArenaTop(darray);
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
	DArraySetArenaTop(darray);
}

void DArrayRemoveUnordered(_sDynamicArray* darray, Uint32 index) {
	SDL_assert(darray != NULL);
	SDL_assert(darray->flags & DARRAY_FLAG_UNORDERED);
	SDL_assert(index <= darray->count);
	void* last = (Uint8*)darray->data + (--darray->count * darray->item_size);
	void* remove_dst = (Uint8*)darray->data + (index * darray->item_size);
	SDL_memcpy(remove_dst, last, darray->item_size);
	DArraySetArenaTop(darray);
}

void DynamicArrayRunTests() {
	_sDynamicArray* darray = DArrayNew(sizeof(Uint32), 100, DARRAY_FLAG_ORDERED);
	_sDynamicArray* old_addr = darray;
	for (Uint32 i = 0; i < 100; i++) {
		DArrayAppend(&darray, &i, 1);
	}
	SDL_assert(darray == old_addr);
	//SDL_Log("cap %i, count %i", darray->capacity, darray->count);
	//SDL_Log("int 45: %i", *DARRAY(darray, 45, Uint32));

	DArrayRemoveOrdered(darray, 45);
	SDL_assert(*DARRAY(darray, 45, Uint32) == 46);

	DArrayRemoveOrdered(darray, 0);

	Uint32 arr[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	DArrayInsert(&darray, &arr[1], 30);
	SDL_assert(darray == old_addr);
	//SDL_Log("cap %i, count %i", darray->capacity, darray->count);
	//SDL_Log("int 30: %i", *DARRAY(darray, 30, Uint32));
	SDL_assert(*DARRAY(darray, 30, Uint32) == 1);

	Uint32 cap = darray->capacity;
	//SDL_Log("cap %i, count %i", darray->capacity, darray->count);
	for (Uint32 i = 0; i < cap; i++) {
		DArrayAppend(&darray, arr, 10);
	}
	SDL_assert(darray != old_addr);
	//SDL_Log("cap %i, count %i", darray->capacity, darray->count);

	DArrayFree(darray);

	Uint64 arr2[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
	darray = DArrayNew(sizeof(Uint64), 100, DARRAY_FLAG_UNORDERED);
	old_addr = darray;
	cap = darray->capacity;
	//SDL_Log("cap %i, count %i", darray->capacity, darray->count);
	for (Uint32 i = 0; i < cap; i++) {
		DArrayAppend(&darray, arr2, 12);
	}
	SDL_assert(darray != old_addr);
	//SDL_Log("cap %i, count %i", darray->capacity, darray->count);
	Uint32 end = darray->count;
	Uint64 num = *DARRAY(darray, end - 1, Uint64);
	Uint32 count = darray->count;
	SDL_assert(num == 11);
	DArrayRemoveUnordered(darray, 1234);
	num = *DARRAY(darray, 1234, Uint64);
	SDL_assert(num == 11);
	num = *DARRAY(darray, end - 2, Uint64);
	SDL_assert(num == 10);
	DArrayRemoveUnordered(darray, 999);
	num = *DARRAY(darray, 999, Uint64);
	SDL_assert(num == 10);
	num = *DARRAY(darray, end - 3, Uint64);
	SDL_assert(num == 9);
	SDL_assert(darray->count == count - 2);
}