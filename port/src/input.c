/* The keys held, for the race (the menu takes its keys as messages, menu.c) */
#include "input.h"

static unsigned char g_down[256], g_hit[256];

void input_key(int vk, int down)
{
    if (vk <= 0 || vk > 255)
        return;
    if (down && !g_down[vk])
        g_hit[vk] = 1;
    g_down[vk] = (unsigned char)(down != 0);
}

int input_down(int vk)
{
    return vk > 0 && vk < 256 && g_down[vk];
}

int input_pressed(int vk)
{
    int r;
    if (vk <= 0 || vk > 255)
        return 0;
    r = g_hit[vk];
    g_hit[vk] = 0;
    return r;
}
