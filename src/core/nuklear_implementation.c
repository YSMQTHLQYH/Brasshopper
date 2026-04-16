#pragma once
#include "SDL3/SDL.h"


/* defines that need to be in implementation only */
#define NK_ZERO_COMMAND_MEMORY
#define NK_MEMSET	SDL_memset
#define NK_MEMCPY	SDL_memcpy
#define NK_SIN		SDL_sin
#define NK_COS		SDL_cos


/* defines that need to be in both header and implementation */
#define NK_INT8 Sint8
#define NK_UINT8 Uint8
#define NK_INT16 Sint16
#define NK_UINT16 Uint16
#define NK_INT32 Sint32
#define NK_UINT32 Uint32
#define NK_SIZE_TYPE uintptr_t
#define NK_POINTER_TYPE uintptr_t
#define NK_INCLUDE_STANDARD_BOOL
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_ASSERT(condition)	SDL_assert(condition)
#define NK_STATIC_ASSERT(exp)     SDL_COMPILE_TIME_ASSERT(, exp)

// actually implement nuklear
#define NK_IMPLEMENTATION
#include "nuklear.h"
