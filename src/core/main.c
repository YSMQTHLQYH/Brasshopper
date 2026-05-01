// MSVC is stoopid and throws this warning on assert(ptr != NULL)
// as it's not convinced you checked for it not being NULL thoroughly enough
// doesn't throw any warning if not using asserts btw, as it assumes it's not NULL because you didn't check...
#ifdef _MSC_VER
__pragma(warning(suppress: 6011))
#endif

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "arena.h"
#include "dynamic_array.h"

#include "nuklear_sdl.h"


static Uint64 time_last_iter_ns = 0;

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;


SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("examples/renderer/clear", 640, 480, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    ArenaRunTests();
    DynamicArrayRunTests();
    
    nk_sdl = NkContextSdlInit(renderer, KiB(64));


    time_last_iter_ns = SDL_GetTicksNS();

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }

    /* report event to nuklear */
    if (nk_sdl != NULL) {
        /* Remember to always rescale the event coordinates,
        * if your renderer uses custom scale. */
        SDL_ConvertEventToRenderCoordinates(nk_sdl->renderer, event);
        NkContextSdlRecordEvent(nk_sdl, event);
    }

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}


#define FPS_UPDATE_TIME 0.25
static float FpsCount(double dt) {
    static double acc = 0;
    static float count = 0;
    static float fps = 0;
    acc += dt;
    count++;
    while (acc >= FPS_UPDATE_TIME) {
        fps = count / FPS_UPDATE_TIME;
        acc -= FPS_UPDATE_TIME;
        count = 0;
    }
    return fps;
}
#undef FPS_UPDATE_TIME


/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate){
    Uint64 time_current_iter_ns = SDL_GetTicksNS();
    double dt = SDL_NS_TO_SECONDS((double)(time_current_iter_ns - time_last_iter_ns));
    time_last_iter_ns = time_current_iter_ns;

    const double now = ((double)SDL_GetTicks()) / 1000.0;  /* convert from milliseconds to seconds. */
    /* choose the color for the frame we will draw. The sine wave trick makes it fade between colors smoothly. */
    const float red = (float) (0.5 + 0.5 * SDL_sin(now));
    const float green = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
    const float blue = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 / 3));
    SDL_SetRenderDrawColorFloat(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);  /* new color, full alpha. */

    /* clear the window to the draw color. */
    SDL_RenderClear(renderer);


    /* nuklear gui */
    nk_input_end(&nk_sdl->ctx);
    NkTestTick(nk_sdl);
    nk_input_begin(&nk_sdl->ctx);
    NkContextSdlDraw(nk_sdl);


    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xA0);
    SDL_RenderDebugTextFormat(renderer, 4, 4,"FPS: %i", (Uint32)FpsCount(dt));


    /* put the newly-cleared rendering on the screen. */
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    NkContextSdlFree(nk_sdl);
    /* SDL will clean up the window/renderer for us. */
}

