#include "nuklear_sdl.h"


struct nk_font* font = NULL;

/* TODO: make a proper allocator for this? */
static void* font_alloc(nk_handle userdata, void* old, nk_size size) {
	if (old == NULL) {
		SDL_Log("allocating %i bytes for font baking", size);
		return SDL_malloc(size);
	}
	SDL_Log("reallocating %i bytes for font baking", size);
	return SDL_realloc(old, size);
}
static void font_free(nk_handle userdata, void* old) {
	if (old == NULL) {
		return;
	}
	SDL_Log("freeing some bytes from font baking");
	SDL_free(old);
}
struct nk_allocator font_allocator = { 0 };

static void BakeDefaultFont(_sNkContextSdl* nk) {
	SDL_assert(nk != NULL);	
	font_allocator.alloc = &font_alloc;
	font_allocator.free = &font_free;

	nk_font_atlas_init(&nk->atlas, &font_allocator);
	nk_font_atlas_begin(&nk->atlas);
	font = nk_font_atlas_add_default(&nk->atlas, 14, NULL);
	int w, h;
	void* img = nk_font_atlas_bake(&nk->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);

	SDL_Surface* surface = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, img, 4 * w);
	if (!surface) { SDL_Log("Couldn't create atlas surface: %s", SDL_GetError()); return; }
	if (nk->atlas_texture) { SDL_DestroyTexture(nk->atlas_texture); }
	nk->atlas_texture = SDL_CreateTextureFromSurface(nk->renderer, surface);
	if (!nk->atlas_texture) { SDL_Log("Couldn't create atlas texture: %s", SDL_GetError()); return; }
	SDL_DestroySurface(surface);

	/* docs cassually mentions this needs handle to texture, absolute no mention of what that is or what it's for.
	example code pulls nk_handle_id(texture) out of it's ass, texture isn't even defined in the example code...
	basically guessing what this is upposed to be*/
	nk_font_atlas_end(&nk->atlas, nk_handle_ptr(nk->atlas_texture), NULL);
	nk_font_atlas_cleanup(&nk->atlas);
}

_sNkContextSdl* NkContextSdlInit(SDL_Renderer* renderer, size_t fixed_memory_size) {
	if (fixed_memory_size < KiB(16)) fixed_memory_size = KiB(16);
	_sArena* arena = ArenaNew(fixed_memory_size - ARENA_HEADER_SIZE);
	_sNkContextSdl* nk = ArenaAlloc(arena, sizeof(_sNkContextSdl), alignof(_sNkContextSdl));
	nk->arena = arena;
	nk->renderer = renderer;
	size_t padding = ( ( (size_t)arena + arena->top) % (2*alignof(Uint64)) );
	if (padding != 0) {
		padding = (2 * alignof(Uint64)) - padding;
		ArenaAlloc(arena, padding, 1);
	}
	size_t total_size = arena->capacity - arena->top;
	ArenaAlloc(arena, total_size, 2 * alignof(Uint64));
	SDL_assert(arena->top == arena->capacity);
	SDL_assert(arena->next == NULL);

	if (font == NULL) {
		// bake font
		BakeDefaultFont(nk);

	}
	if (!nk_init_fixed(&nk->ctx, ArenaAlloc(arena, total_size, 2 * alignof(Uint64) ), total_size, &font)) {
		SDL_Log("Could not create nuklear context");
		SDL_assert(0); // TODO: handle this better
	}
	
}

void NkContextSdlFree(_sNkContextSdl* nk) {
	SDL_assert(nk != NULL);
	nk_font_atlas_clear(&nk->atlas);
	SDL_DestroyTexture(nk->atlas_texture);
	nk_free(&nk->ctx);
	ArenaFree(nk->arena); //nk is stored inside the arena itself so this also frees that
}

void NkTestTick(_sNkContextSdl* nk) {
	SDL_assert(nk != NULL);
}