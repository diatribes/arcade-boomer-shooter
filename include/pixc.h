#ifndef PIXC_H
#define PIXC_H
#include <SDL2/SDL.h>

typedef Uint32 u32;
typedef Uint16 u16;
typedef Uint8 u8;

typedef Sint32 s32;
typedef Sint16 s16;
typedef Sint8 s8;

typedef struct texture32
{
    SDL_Surface *surface;
    int w, h;
    u32 *pixels;
} texture32;

/* pixc_init */
void pixc_init(SDL_Renderer *renderer, int logicalScreenWidth,
        int logicalScreenHeight);

/* pixc_update */
void pixc_update(SDL_Renderer *renderer, texture32 texture);

/* pixc_load_texture32 */
void pixc_load_texture32(const char *filepath, texture32 *texture);

/* pixc_make_texture32 */
void pixc_make_texture32(int w, int h, texture32 *texture);

/* pixc_toggle_fullscreen */
void pixc_toggle_fullscreen(SDL_Window* Window, SDL_Renderer *renderer);

#if SDL_BYTEORDER != SDL_BIG_ENDIAN
    #define rmask 0x00ff0000
    #define gmask 0x0000ff00
    #define bmask 0x000000ff
    #define amask 0xff000000
#else
    #define rmask 0x0000ff00
    #define gmask 0x00ff0000
    #define bmask 0xff000000
    #define amask 0x000000ff
#endif

//#define RGB(format, r, g, b) SDL_MapRGB(format, 255, 255, 255)
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define MIN(a, b) ((a) < (b) ? : (a) : (b))

#endif

#ifdef PIXC_IMPLEMENTATION
#undef PIXC_IMPLEMENTATION

static SDL_Texture *screenBuffer;

void pixc_init(SDL_Renderer * renderer, int logicalScreenWidth,
	       int logicalScreenHeight)
{
    screenBuffer = SDL_CreateTexture(renderer,
				     SDL_PIXELFORMAT_ARGB8888,
				     SDL_TEXTUREACCESS_STREAMING,
				     logicalScreenWidth,
				     logicalScreenHeight);
    SDL_RenderSetLogicalSize(renderer,
			     logicalScreenWidth, logicalScreenHeight);
    SDL_SetRenderTarget(renderer, screenBuffer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
}

void pixc_toggle_fullscreen(SDL_Window * Window, SDL_Renderer * renderer)
{
    SDL_RenderClear(renderer);
    //Uint32 FullscreenFlag = SDL_WINDOW_FULLSCREEN;
    Uint32 FullscreenFlag = SDL_WINDOW_FULLSCREEN_DESKTOP;
    int IsFullscreen = SDL_GetWindowFlags(Window) & FullscreenFlag;
    SDL_SetWindowFullscreen(Window, IsFullscreen ? 0 : FullscreenFlag);
    SDL_SetRelativeMouseMode(!IsFullscreen);
    SDL_RenderClear(renderer);
}

#if 1
void pixc_update(SDL_Renderer * renderer, texture32 texture)
{
    SDL_Rect r = {.w = texture.w,.h = texture.h,.x = 0,.y = 0 };

    SDL_UpdateTexture(screenBuffer, &r, texture.pixels,
		      texture.w * sizeof(u32));

    SDL_RenderCopy(renderer, screenBuffer, NULL, NULL);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderPresent(renderer);
}
#else
void pixc_update(SDL_Renderer * renderer, texture32 texture)
{
    void *pixels;
    int pitch;

    SDL_LockTexture(screenBuffer, NULL, &pixels, &pitch);
    memcpy(pixels, texture.pixels, texture.w * texture.h * sizeof(u32));
    SDL_UnlockTexture(screenBuffer);
    SDL_RenderCopy(renderer, screenBuffer, NULL, NULL);
    SDL_RenderPresent(renderer);
}
#endif

void pixc_load_texture32(const char *filepath, texture32 * texture)
{
    SDL_Surface *surface = SDL_LoadBMP(filepath);
    if (!surface) {
	puts(SDL_GetError());
    }

    SDL_Surface *surface32 =
	SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(surface);

    texture->surface = surface32;
    texture->w = surface32->w;
    texture->h = surface32->h;
    texture->pixels = (u32 *) surface32->pixels;
}

void pixc_make_texture32(int w, int h, texture32 * texture)
{
    SDL_Surface *surface = SDL_CreateRGBSurface(0, w, h, 32, rmask, gmask, bmask, amask);
    texture->surface = surface;
    texture->w = surface->w;
    texture->h = surface->h;
    texture->pixels = (u32 *) surface->pixels;
}


/*
void pixc_free_texture32(texture32 *texture)
{
    free(texture->surface)
    texture->surface = NULL;
    texture->w = 0;
    texture->h = 0;
    texture->pixels = NULL;
}*/

#endif
