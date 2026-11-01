#include "clock.h"
#include "mister.h"
#include <SDL.h>

unsigned g_now = 1000;      /* (never 0: the game takes 0 as "not set yet" in places) */
int g_ticks = 1;
static int g_real;
static Uint64 g_t0, g_freq;
static unsigned long long g_frames, g_done;

void clock_start(int realtime)
{
    g_real = realtime;
    g_freq = SDL_GetPerformanceFrequency();
    g_t0 = SDL_GetPerformanceCounter();
}

void clock_frame(void)
{
    unsigned long long ticks;
    if (!g_real) {
        g_frames++;
        g_now = 1000 + (unsigned)(g_frames * 1000 / 60);
        g_ticks = 1;
        return;
    }
#ifdef MISTER_HYBRID
    /* the core's fields are the clock: each picture a whole number of them */
    if (mister_clock(&g_now))
        g_now += 1000;
    else
#endif
    {
        Uint64 dt = SDL_GetPerformanceCounter() - g_t0;
        g_now = 1000 + (unsigned)(dt / g_freq * 1000 + dt % g_freq * 1000 / g_freq);
    }
    ticks = (unsigned long long)(g_now - 1000) * 60 / 1000;
    /* a stall (a window dragged) counts 12 at most, as the game's own 200 ms cap */
    g_ticks = ticks - g_done > 12 ? 12 : (int)(ticks - g_done);
    g_done = ticks;
}
