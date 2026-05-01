#pragma once

#include "SDL3/SDL.h"
#include "nuklear_include.h"


#include "arena.h"

typedef struct {
	_sArena* arena;
	SDL_Renderer* renderer;
	SDL_Texture* atlas_texture;
	struct nk_context ctx;
	struct nk_font_atlas atlas;
	struct {
		Uint64 last_left_click_ns;
		Uint8 insert_toggle : 1;
	} input_state;
}_sNkContextSdl;
extern _sNkContextSdl* nk_sdl;

/*
Inits a nuklear context with it's own fixed memory inside it's own arena
Does not own renderer
*/
_sNkContextSdl* NkContextSdlInit(SDL_Renderer* renderer, size_t fixed_memory_size);

/*
Frees a nuklear context with it's own fixed memory inside it's own arena, freeing the arena with it
*/
void NkContextSdlFree(_sNkContextSdl* nk);


/*
returns true if it did something with the event
returns false if the event was something unrelated to gui
*/
Uint8 NkContextSdlRecordEvent(_sNkContextSdl* nk, SDL_Event* event);

void NkContextSdlDraw(_sNkContextSdl* nk);

void NkTestTick(_sNkContextSdl* nk);