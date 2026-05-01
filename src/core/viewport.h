#pragma once

#include <SDL3/SDL.h>
#include "camera.h"

typedef struct {
	SDL_Renderer* renderer;
	SDL_Texture* render_target;
	_sCamera* camera;
}_sViewport;

_sViewport* ViewportInit(SDL_Renderer* renderer);