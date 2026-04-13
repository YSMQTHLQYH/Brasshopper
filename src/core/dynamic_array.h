#pragma once

#include "arena.h"


/*
* Array that grow in size, reallocating memory if necessary
* DO NOT STORE POINTERS TO ITEMS INSIDE THE DYNAMIC ARRAY
* guaranteed to be continious so it can be itterated like a normal array
*/
typedef struct {
	void* data;
	_sArena* arena; // handled by the dynamic array itself
	Uint32 count;
	Uint32 item_size;
	Uint32 capacity; // amount of items that fit before needing to reallocate
	Uint32 flags;
}_sDynamicArray;
#define DYNAMIC_ARRAY_HEADER_SIZE	64
SDL_COMPILE_TIME_ASSERT(dynamic_array_header_size, sizeof(_sDynamicArray) <= DYNAMIC_ARRAY_HEADER_SIZE);

enum {
	DARRAY_FLAG_ORDERED		= (1 << 0),
	DARRAY_FLAG_UNORDERED	= (1 << 1),
};

/*
* Makes a new dynamic array, it manages it's own memory arena
* item_size is size (in bytes) of one item
* reserve_count is minimum amount of items it will fit before needing to reallocate
* flags  is one or more flags or'd together, it needs to at least be DARRAY_FLAG_ORDERED or DARRAY_FLAG_UNORDERED
* free with DArrayFree()
* retruns pointer to the new dynamic array
*/
_sDynamicArray* DArrayNew(Uint32 item_size, Uint32 reserve_count, Uint32 flags);
void DArrayFree(_sDynamicArray* darray);

// returns pointer to "darray[i]"
#define DARRAY(darray, i, type) (type*)( (size_t)darray->data + (i * sizeof(type) ) )

/*
* Appends item(s) to the end of the dynamic array, reallocating if necessary
* item_ptr is a pointer to the item to append (or array of items if multiple)
* count is number of items to append
* returns pointer to dynamic array (changes if array had to be reallocated)
*/
_sDynamicArray* DArrayAppend(_sDynamicArray* darray, void* item_ptr, Uint32 count);

/*
* Removes items starting from the end of the dynamic array
*/
void DArrayRemoveLast(_sDynamicArray* darray, Uint32 count);

/*
* Inserts an item at a specific index in the middle of dynamic array
* This moves all items after that index, note that it is relatively expensive
* If you don't need the dynamic array to be ordered use DArrayAppend() instead
* item_ptr is a pointer to the item to append
* index is index where item inserted will be at (so my_da[index] will be the new item)
* * returns pointer to dynamic array (changes if array had to be reallocated)
*/
_sDynamicArray* DArrayInsert(_sDynamicArray* darray, void* item_ptr, Uint32 index);

/*
* Removes an item at a specific index in the middle of dynamic array
* This moves all items after that index, note that it is relatively expensive
* If you don't need the dynamic array to be ordered use DArrayRemoveUnordered() instead
* index is index of the item to be deleted (ei my_da[index])
*/
void DArrayRemoveOrdered(_sDynamicArray* darray, Uint32 index);

/*
* Removes an item at a specific index in the middle of dynamic array
* And replaces it with the item at the end of the dynamic array
* This breaks the order but saves on moving several items while keeping the data contiguous 
* If you do need the dynamic array to be ordered use DArrayRemoveOrdered() instead
* index is index of the item to be deleted (ei my_da[index])
*/
void DArrayRemoveUnordered(_sDynamicArray* darray, Uint32 index);


void DynamicArrayRunTests();