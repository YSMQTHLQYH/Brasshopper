#include "nuklear_sdl.h"

#define CIRCLE_VERTEX_COUNT	8
#define CIRCLE_INDEX_COUNT	((CIRCLE_VERTEX_COUNT - 2) * 3)

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
	if (!nk_init_fixed(&nk->ctx, ArenaAlloc(arena, total_size, 2 * alignof(Uint64) ), total_size, &font->handle)) {
		SDL_Log("Could not create nuklear context");
		SDL_assert(0); // TODO: handle this better
	}
	
	return nk;
}

void NkContextSdlFree(_sNkContextSdl* nk) {
	SDL_assert(nk != NULL);
	SDL_assert(nk->arena != NULL);
	nk_font_atlas_clear(&nk->atlas);
	SDL_DestroyTexture(nk->atlas_texture);
	//nk_free(&nk->ctx); //not needed as we are using nk_init_fixed
	ArenaFree(nk->arena); //nk is stored inside the arena itself so this also frees that
}

// hardcoded for NkContextSdlDraw to not repeat code, not for general purposes
static void CreateCircleVertices(SDL_FPoint* vertex, SDL_Rect rect) {
	for (Uint8 i = 0; i < CIRCLE_VERTEX_COUNT; i++) {
		float x = SDL_cosf(i * 2 * SDL_PI_F / CIRCLE_VERTEX_COUNT);
		float y = SDL_sinf(i * 2 * SDL_PI_F / CIRCLE_VERTEX_COUNT);
		x = (x + 1) / 2;
		y = (y + 1) / 2;
		x *= rect.w;
		y *= rect.h;
		x += rect.x;
		y += rect.y;
		vertex[i].x = x;
		vertex[i].y = y;
	}
}

void NkContextSdlDraw(_sNkContextSdl* nk) {
	SDL_assert(nk != NULL);
	SDL_assert(nk->renderer != NULL);

	SDL_SetRenderDrawBlendMode(nk->renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderClipRect(nk->renderer, NULL);

	const struct nk_command* command = 0;
	nk_foreach(command, &nk->ctx) {
		switch (command->type) {
		case NK_COMMAND_SCISSOR:
		{
			//I suppose this is what it means by scissor, the docs doesn't explain at all
			const struct nk_command_scissor* cmd = command;
			SDL_Rect rect = { .x = cmd->x, .y = cmd->y, .w = cmd->w, .h = cmd->h };
			SDL_SetRenderClipRect(nk->renderer, &rect);
		}
		break;
		case NK_COMMAND_LINE:
		{
			const struct nk_command_line* cmd = command;
			//TODO: figure out line_thickness
			SDL_SetRenderDrawColor(nk->renderer, cmd->color.r, cmd->color.g, cmd->color.b, cmd->color.a);
			SDL_RenderLine(nk->renderer, cmd->begin.x, cmd->begin.y, cmd->end.x, cmd->end.y);
		}
		break;
		case NK_COMMAND_RECT:
		{
			const struct nk_command_rect* cmd = command;
			SDL_FRect rect = { .x = cmd->x, .y = cmd->y, .w = cmd->w, .h = cmd->h };
			//TODO: figure out line_thickness and rounding
			SDL_SetRenderDrawColor(nk->renderer, cmd->color.r, cmd->color.g, cmd->color.b, cmd->color.a);
			SDL_RenderRect(nk->renderer, &rect);
		}
		break;
		case NK_COMMAND_RECT_FILLED:
		{
			const struct nk_command_rect_filled* cmd = command;
			SDL_FRect rect = { .x = cmd->x, .y = cmd->y, .w = cmd->w, .h = cmd->h };
			//TODO: figure out rounding
			SDL_SetRenderDrawColor(nk->renderer, cmd->color.r, cmd->color.g, cmd->color.b, cmd->color.a);
			SDL_RenderFillRect(nk->renderer, &rect);
		}
		break;
		case NK_COMMAND_CIRCLE:
		{
			const struct nk_command_circle* cmd = command;
			SDL_FPoint vertex[CIRCLE_VERTEX_COUNT + 1] = { 0 };
			//assuming x, y, w, h is bounding box (very likely, docs don't specify tho)
			const SDL_Rect rect = { cmd->x, cmd->y, cmd->w, cmd->h };
			CreateCircleVertices(vertex, rect);
			vertex[CIRCLE_VERTEX_COUNT].x = vertex[0].x;
			vertex[CIRCLE_VERTEX_COUNT].y = vertex[0].y;
			//TODO: figure out line_thickness
			SDL_SetRenderDrawColor(nk->renderer, cmd->color.r, cmd->color.g, cmd->color.b, cmd->color.a);
			SDL_RenderLines(nk->renderer, &vertex, CIRCLE_VERTEX_COUNT + 1);
		}
		break;
		case NK_COMMAND_CIRCLE_FILLED:
		{
			const struct nk_command_circle_filled* cmd = command;
			SDL_FPoint vertex[CIRCLE_VERTEX_COUNT] = { 0 };
			//assuming x, y, w, h is bounding box (very likely, docs don't specify tho)
			const SDL_Rect rect = { cmd->x, cmd->y, cmd->w, cmd->h };
			CreateCircleVertices(vertex, rect);
			const SDL_FColor color = { cmd->color.r, cmd->color.g, cmd->color.b, cmd->color.a };

			Uint8 index[CIRCLE_INDEX_COUNT] = { 0 };
			for (Uint8 i = 0; i < CIRCLE_VERTEX_COUNT - 2; i++) {
				index[(i * 3) + 0] = 0;
				index[(i * 3) + 1] = i + 1;
				index[(i * 3) + 2] = i + 2;
			}

			SDL_RenderGeometryRaw(nk->renderer, NULL,
				vertex, sizeof(SDL_FPoint),
				&color, 0,
				NULL, 0,
				CIRCLE_VERTEX_COUNT,
				&index, CIRCLE_INDEX_COUNT, 1);
		}
		break;
		case NK_COMMAND_TEXT:
		{
			const struct nk_command_text* cmd = command;
			SDL_FRect rect = { .x = cmd->x, .y = cmd->y, .w = cmd->w, .h = cmd->h };
			SDL_SetRenderDrawColor(nk->renderer, cmd->background.r, cmd->background.g, cmd->background.b, cmd->background.a);
			SDL_RenderFillRect(nk->renderer, &rect);

			//docs say absolutely nothing about how to actually render text from font baker...
			SDL_Log(cmd->string);
		}
		break;
		default:
			SDL_Log("Drawn unimplemented nk_command_type: %i", command->type);
		}
	}

	// we are done with this frame
	nk_clear(&nk->ctx);
}

void NkTestTick(_sNkContextSdl* nk) {
	SDL_assert(nk != NULL);

	enum { EASY, HARD };
	static int op = EASY;
	static float value = 0.6f;
	static int i = 20;

	if (nk_begin(&nk->ctx, "Show", nk_rect(50, 50, 220, 220),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE)) {
		// fixed widget pixel width
		nk_layout_row_static(&nk->ctx, 30, 80, 1);
		if (nk_button_label(&nk->ctx, "button")) {
			// event handling
			SDL_Log("button pressed");
		}

		// fixed widget window ratio width
		nk_layout_row_dynamic(&nk->ctx, 30, 2);
		if (nk_option_label(&nk->ctx, "easy", op == EASY)) op = EASY;
		if (nk_option_label(&nk->ctx, "hard", op == HARD)) op = HARD;

		// custom widget pixel width
		nk_layout_row_begin(&nk->ctx, NK_STATIC, 30, 2);
		{
			nk_layout_row_push(&nk->ctx, 50);
			nk_label(&nk->ctx, "Volume:", NK_TEXT_LEFT);
			nk_layout_row_push(&nk->ctx, 110);
			nk_slider_float(&nk->ctx, 0, &value, 1.0f, 0.1f);
		}
		nk_layout_row_end(&nk->ctx);
	}
	nk_end(&nk->ctx);
}