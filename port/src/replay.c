/* The race's recording: NOTES §4.21b. Two words a step, as the original's .Sav files hold
   them: dt in the top byte of the first and the key bits below, the two analog values in
   the second. */
#include "replay.h"
#include "config.h"
#include <stdint.h>

#define MAX 0x4fb0              /* 0x4e9838 stops here: 5 min 26 s of 16 ms steps */

static uint32_t g_rec[MAX][2];  /* 0x558aa8 */
static int g_count, g_index;    /* 0x4e9838, 0x4e983c */

void replay_clear(void)
{
    g_count = g_index = 0;
}

void replay_rewind(void)
{
    g_index = 0;
}

void replay_record(int ms, unsigned keys, int held, int pressed, int x, int y)
{
    if (g_count >= (behaviour_cd() ? 10000 : MAX))      /* 0x479c00 : 0x4577f0 */
        return;
    keys &= ~0xc0u;
    if (held)
        keys |= 0x80;
    if (pressed)
        keys |= 0x40;
    g_rec[g_count][0] = (uint32_t)(ms > 255 ? 255 : ms) << 24 | (keys & 0xffffff);
    g_rec[g_count][1] = (uint32_t)x << 16 | ((uint32_t)y & 0xffff);
    g_count++;
}

int replay_play(int *ms, unsigned *keys, int *held, int *pressed, int *x, int *y)
{
    uint32_t w0, w1;
    if (g_index >= g_count) {
        *keys = 0;
        return 0;
    }
    w0 = g_rec[g_index][0];
    w1 = g_rec[g_index][1];
    g_index++;
    *ms = (int)(w0 >> 24);
    *keys = w0 & 0xffffff;
    *x = (int)(w1 >> 16);
    *y = (int)(w1 & 0xffff);
    *held = (w0 & 0x80) != 0;
    *pressed = (w0 & 0x40) != 0;
    return 1;
}

int replay_count(void)
{
    return g_count;
}
