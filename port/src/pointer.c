/* The Windows cursors of game.EXE, 32x32 and one bit deep, put on the picture as Windows
   would put them: the screen ANDed with one mask and XORed with the other. Drawn into the
   picture, so that it scales with it like the menus' pointer. */
#include "pointer.h"
#include "exe.h"
#include "video.h"
#include <stdlib.h>
#include <string.h>

enum { CUR_W = 32, CUR_H = 32 };

typedef struct {
    int id, loaded, ok;
    int hx, hy;
    unsigned char xorm[CUR_H][4], andm[CUR_H][4];
} Cursor;

/* 116 arrow, 228 the rows' chequered arrow, 251 the "no" circle; then the dissolving
   arrows and circles (0x45a0a0) */
static Cursor g_cur[] = {
    { .id = CUR_ARROW }, { .id = CUR_ROW }, { .id = CUR_NO },
    { .id = 248 }, { .id = 249 }, { .id = 250 }, { .id = 253 }, { .id = 254 }, { .id = 255 }
};
static int g_timer = 3000;      /* 0x4dbfb4 */
static int g_shape = CUR_ARROW; /* 0x4dbfac */
static int g_x, g_y;            /* the mouse */
static int g_seen;              /* the port's: no mouse message yet, no pointer to show */
/* what the pointer covers, to put back while the race is paused */
static uint16_t g_under[CUR_H][CUR_W];
static int g_under_x, g_under_y, g_under_on;

static unsigned u16le(const unsigned char *p)
{
    return (unsigned)p[0] | (unsigned)p[1] << 8;
}

static unsigned u32le(const unsigned char *p)
{
    return u16le(p) | u16le(p + 2) << 16;
}

/* a cursor group's first entry, then the cursor: hot spot, BITMAPINFOHEADER, two palette
   entries, the XOR rows and the AND rows, bottom up, each row 4 bytes */
static void load(Cursor *c)
{
    unsigned len;
    const unsigned char *g = exe_resource(12, c->id, &len), *d;
    int y;
    c->loaded = 1;
    if (!g || len < 6 + 14 || !(d = exe_resource(1, (int)u16le(g + 6 + 12), &len)))
        return;
    if (len < 4 + 40 + 8 + 2 * CUR_H * 4 || u32le(d + 4 + 4) != CUR_W
        || u32le(d + 4 + 8) != 2 * CUR_H || u16le(d + 4 + 14) != 1)
        return;
    c->hx = (int)u16le(d);
    c->hy = (int)u16le(d + 2);
    for (y = 0; y < CUR_H; y++) {
        memcpy(c->xorm[y], d + 4 + 40 + 8 + (CUR_H - 1 - y) * 4, 4);
        memcpy(c->andm[y], d + 4 + 40 + 8 + CUR_H * 4 + (CUR_H - 1 - y) * 4, 4);
    }
    c->ok = 1;
}

static Cursor *cursor(int id)
{
    unsigned i;
    for (i = 0; i < sizeof g_cur / sizeof g_cur[0]; i++)
        if (g_cur[i].id == id) {
            if (!g_cur[i].loaded)
                load(&g_cur[i]);
            return g_cur[i].ok ? &g_cur[i] : 0;
        }
    return 0;
}

void pointer_touched(int x, int y)
{
    g_timer = 3000;
    g_x = x;
    g_y = y;
    g_seen = 1;
}

void pointer_step(int ms)
{
    g_timer -= ms;
}

void pointer_shape(int c)
{
    g_shape = c;
}

/* 0x45a190: below zero the set of three by the shape, A B C B A B C, 90 ms each, then none */
static int shown_now(void)
{
    static const signed char order[7] = { 0, 1, 2, 1, 0, 1, 2 };
    int base = g_shape == CUR_NO ? 253 : 248, n;
    if (g_timer >= 0)
        return g_shape;
    n = (-g_timer - 1) / 90;
    return n < 7 ? base + order[n] : CUR_NONE;
}

void pointer_draw(void)
{
    const Cursor *c;
    int x, y, id = shown_now();
    g_under_on = 0;
    if (!g_seen || id == CUR_NONE || !(c = cursor(id)))
        return;
    g_under_x = g_x - c->hx;
    g_under_y = g_y - c->hy;
    g_under_on = 1;
    for (y = 0; y < CUR_H; y++) {
        int sy = g_under_y + y;
        if (sy < 0 || sy >= g_scr_h)
            continue;
        for (x = 0; x < CUR_W; x++) {
            int sx = g_under_x + x;
            uint16_t *p;
            if (sx < 0 || sx >= g_scr_w)
                continue;
            VIDEO_ROWS(sy, sy);
            p = g_screen + sy * g_pitch + sx;
            g_under[y][x] = *p;
            if (!(c->andm[y][x >> 3] & 0x80 >> (x & 7)))
                *p = 0;
            if (c->xorm[y][x >> 3] & 0x80 >> (x & 7))
                *p ^= 0xffff;
        }
    }
}

void pointer_undraw(void)
{
    int x, y;
    if (!g_under_on)
        return;
    g_under_on = 0;
    for (y = 0; y < CUR_H; y++) {
        int sy = g_under_y + y;
        if (sy < 0 || sy >= g_scr_h)
            continue;
        for (x = 0; x < CUR_W; x++) {
            int sx = g_under_x + x;
            if (sx >= 0 && sx < g_scr_w) {
                VIDEO_ROWS(sy, sy);
                g_screen[sy * g_pitch + sx] = g_under[y][x];
            }
        }
    }
}
