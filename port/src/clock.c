#include "clock.h"

unsigned g_now = 1000;      /* (never 0: the game takes 0 as "not set yet" in places) */
static unsigned g_frames;

void clock_frame(void)
{
    g_frames++;
    g_now = 1000 + (unsigned)((unsigned long long)g_frames * 1000 / 60);
}
