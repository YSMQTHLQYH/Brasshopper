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
	font = nk_font_atlas_add_default(&nk->atlas, 16, NULL);
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

// separated this one as it's own function for readability, full of comments basically ranting at the nonexistant documentation on this
static void DrawTextCommand(_sNkContextSdl* nk, const struct nk_command_text* cmd) {
	/* decoding */

	// docs say absolutely nothing about how to actually render text from font baker...
	// guessing my way through every single line of code until it works I suppose
	//SDL_Log(cmd->string);
	//SDL_Log("size: %f, scale %f", font->config->size, font->scale);
	struct nk_font* font = cmd->font->userdata.ptr; // the docs doesn't even explain what a nk_handle is btw

	// looking at source code, individual characters are stored as nk_rune, which is just allias for Uint...
	// but the nk_command_text only has a plain old string... what

	/*
	int nk_utf_decode(const char *c, nk_rune *u, int clen);
	only know about this function existing from looking at implementation of calculating text width...
	not even a comment on what clen is for, let alone if this just decodes on char or an entire array
	*/

	// assuming it returns number of bytes read
	int len = 0;
	Uint32 count = 0;
	nk_rune* unicode = SDL_stack_alloc(nk_rune, cmd->length);
	while (len < cmd->length) {
		len += nk_utf_decode(&cmd->string[len], &unicode[count++], cmd->length - len);
		//SDL_Log("len %i", len);
		// SDL_Log("rune?? %i, len: %i", unicode, len);
		// alright it's working as I guessed this far
	}


	/* drawing background */
	SDL_FRect bounding_box = { .x = cmd->x, .y = cmd->y, .w = cmd->w, .h = cmd->h };
	SDL_SetRenderDrawColor(nk->renderer, cmd->background.r, cmd->background.g, cmd->background.b, cmd->background.a);
	SDL_RenderFillRect(nk->renderer, &bounding_box);


	/* drawing characters as individual textured quads */
	const Uint8 indices[6] = {
		0, 1, 2,
		0, 2, 3
	};
	const SDL_FColor foreground_color = {
		.r = (float)cmd->foreground.r / 256,
		.g = (float)cmd->foreground.g / 256,
		.b = (float)cmd->foreground.b / 256,
		.a = (float)cmd->foreground.a / 256,
	};
	float advance = 0;
	for (Uint32 i = 0; i < count; i++) {

		// also a function from looking at source code, uncommented
		struct nk_font_glyph* glyph = nk_font_find_glyph(font, unicode[i]);
		/*
		glyph struct has x0/x1/y0/y1, and w/h, and u0/u1/v0/v1...
		I hope it's just redundant positions in texture atlas because idk what to do with all that
		SDL_Log("x0: %f, x1: %f, w: %f", glyph->x0, glyph->x1, glyph->w);
		SDL_Log("y0: %f, y1: %f, h: %f", glyph->y0, glyph->y1, glyph->h);
		SDL_Log("u0: %f, u1: %f", glyph->u0, glyph->u1);
		SDL_Log("v0: %f, v1: %f", glyph->v0, glyph->v1);
		x0 + x1 seems to be always equal to w and so on, I can breathe in relief
		nvm x and y are all in the top left corner, probably an offset inside character bounding box? edit: yup
		*/

		float x0 = bounding_box.x + advance, y0 = bounding_box.y + glyph->y0;
		advance += glyph->xadvance - glyph->x0; // not sure if  - glyph->x0 goes here but this looks the least bad
		SDL_FPoint xy[4] = {
			{x0,			y0},
			{x0 + glyph->w,	y0},
			{x0 + glyph->w,	y0 + glyph->h},
			{x0,			y0 + glyph->h}
		};
		SDL_FPoint uv[4] = {
			{glyph->u0, glyph->v0},
			{glyph->u1, glyph->v0},
			{glyph->u1, glyph->v1},
			{glyph->u0, glyph->v1}
		};

		SDL_RenderGeometryRaw(nk->renderer, nk->atlas_texture,
			xy, sizeof(SDL_FPoint),
			&foreground_color, 0,
			uv, sizeof(SDL_FPoint),
			4,
			&indices, 6, 1);
		//SDL_Log("char: %i", unicode[i]);
		//SDL_Log("u0: %f, u1: %f, v0: %f, v1: % f", glyph->u0, glyph->u1, glyph->v0, glyph->v1);
	}

	SDL_stack_free(unicode);
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
			DrawTextCommand(nk, cmd);
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