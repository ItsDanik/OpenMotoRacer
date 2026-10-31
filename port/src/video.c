/* The window: the 16-bit screen handed to SDL as an RGB565 texture, kept 4:3. */
#include "video.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>

uint16_t *g_screen;
int g_scr_w = 640, g_scr_h = 480, g_pitch = SCREEN_MAX_W;

static uint16_t g_pixels[SCREEN_MAX_W * SCREEN_MAX_H];
static SDL_Window *g_win;
static SDL_Renderer *g_ren;
static SDL_Texture *g_tex;
static int g_fullscreen;
static int g_surf_h = 480;      /* the surface's height: below a 16:9 picture it stays black */

int video_init(int headless, char *err, int errsize)
{
    g_screen = g_pixels;
    if (headless)
        return 1;
    g_win = SDL_CreateWindow("OpenMotoRacer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             g_scr_w * 2, g_scr_h * 2, SDL_WINDOW_RESIZABLE);
    if (g_win)
        g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_PRESENTVSYNC);
    if (g_ren) {
        SDL_RenderSetLogicalSize(g_ren, 640, 480);
        g_tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
                                  g_scr_w, g_scr_h);
    }
    if (!g_tex) {
        snprintf(err, errsize, "No window: %s", SDL_GetError());
        return 0;
    }
    return 1;
}

void video_set_size(int w, int h, int view_h)
{
    if (w == g_scr_w && h == g_surf_h && view_h == g_scr_h)
        return;
    g_scr_w = w;
    g_scr_h = view_h;
    g_surf_h = h;
    memset(g_pixels, 0, sizeof g_pixels);
}

void video_mouse(int *x, int *y)
{
    *x = *x * g_scr_w / SCREEN_MAX_W;
    *y = *y * g_surf_h / SCREEN_MAX_H;
}

void video_present(void)
{
    if (!g_tex)
        return;
    SDL_Rect r = { 0, 0, g_scr_w, g_surf_h };
    SDL_UpdateTexture(g_tex, &r, g_screen, g_pitch * 2);
    SDL_RenderClear(g_ren);
    SDL_RenderCopy(g_ren, g_tex, &r, 0);
    SDL_RenderPresent(g_ren);
}

int video_shot(const char *file)
{
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormatFrom(g_screen, g_scr_w, g_surf_h, 16,
                                                         g_pitch * 2, SDL_PIXELFORMAT_RGB565);
    int ok = s && SDL_SaveBMP(s, file) == 0;
    SDL_FreeSurface(s);
    return ok;
}

void video_toggle_fullscreen(void)
{
    g_fullscreen ^= 1;
    if (g_win)
        SDL_SetWindowFullscreen(g_win, g_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

int video_fullscreen(void)
{
    return g_fullscreen;
}

void video_quit(void)
{
    if (g_tex)
        SDL_DestroyTexture(g_tex);
    if (g_ren)
        SDL_DestroyRenderer(g_ren);
    if (g_win)
        SDL_DestroyWindow(g_win);
}
