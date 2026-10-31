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
int g_alt_return = 1;
int g_draw_2d = 1;
int g_no_3d;
static int g_no_3d_once;        /* 0x4e8940 */
static int g_kept = -1;         /* 0x4e8954: while another program has the focus */

int video_surface_h(void)
{
    return g_surf_h;
}

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

/* the start; a run without a window stays as it is, so that the pictures do not depend on
   the settings */
void video_set_fullscreen(int on)
{
    if (g_win && !g_fullscreen != !on)
        video_toggle_fullscreen();
}

int video_fullscreen(void)
{
    return g_fullscreen;
}

int video_has_window(void)
{
    return g_win != 0;
}

/* WM_ACTIVATEAPP (0x4333bf, 0x433425): the full screen gives way to a window while another
   program has the focus, and comes back with it; what is saved is what it was (0x432fda) */
void video_focus(int gained)
{
    if (!gained) {
        g_no_3d_once = 0;
        g_draw_2d = 0;
        if (g_kept < 0)
            g_kept = g_fullscreen;
        if (g_fullscreen) {
            video_set_fullscreen(0);
            g_no_3d = 1;
        }
    } else {
        g_no_3d = 0;
        g_no_3d_once = 1;
        if (g_kept >= 0)
            video_set_fullscreen(g_kept);
        g_kept = -1;
    }
}

/* 0x49ad90's test */
int video_no_3d(void)
{
    return g_no_3d || g_no_3d_once;
}

/* 0x49b1c0, each picture shown: back from another program, one without 3D, then all */
void video_frame_end(void)
{
    if (g_no_3d_once) {
        g_no_3d_once = 0;
        g_draw_2d = 1;
    }
}

int video_fullscreen_kept(void)
{
    return g_kept >= 0 ? g_kept : g_fullscreen;
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
