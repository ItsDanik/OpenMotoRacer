#include "rand.h"
#include "exe.h"

static uint32_t g_seed[8];

void rand_init(void)
{
    int i;
    for (i = 0; i < 8; i++)
        g_seed[i] = exe_u32(exe_addr(X_RAND_SEEDS) + 4 * i);
}

void rand_seed(int stream, uint32_t seed)
{
    g_seed[stream & 7] = seed;
}

int rand_step(uint32_t *state)
{
    int32_t x = (int32_t)*state, u, v;
    u = (int32_t)((uint32_t)(x >> 16) * 16807u);
    v = (int32_t)(((uint32_t)(u & 0x7fff) << 16) + (uint32_t)(x & 0xffff) * 16807u
                  + (uint32_t)(u >> 15));
    if (v < 0)
        v = (int32_t)((uint32_t)v - 0x7fffffffu);
    *state = (uint32_t)v;
    return v > 0x7fff ? v % 0x7fff : v;
}

int rand_next(int stream)
{
    return rand_step(&g_seed[stream & 7]);
}
