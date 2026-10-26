/* Menu pictures (NOTES §3.3, §4.7): .LEZ files, a packed 8-bit TGA, made 16-bit at load
   (0x43eb00), listed in the game's table of menu pictures and drawn by 0x43ede0. */
#include "bitmap.h"
#include "clock.h"
#include "exe.h"
#include "files.h"
#include "video.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* X_BITMAP_TABLE (GOG 0x4e3e20): records of 20 bytes, u32 ?, language folder, id, name,
   mode; ended by id -1; copied out by id at 0x415ad2. X_LANG_FOLDERS (GOG 0x4e8b60):
   "data\MenuUs\", "data\MenuFr\", ... */

int g_lang;

static unsigned u32(const unsigned char *p)
{
    return (unsigned)p[0] | (unsigned)p[1] << 8 | (unsigned)p[2] << 16 | (unsigned)p[3] << 24;
}

/* 0x43eb00, the unpacking: "LEZ1", u32 size, u32 offset of the copy words, u32 offset of
   the literal bytes, u32, then u32 flag words from 0x14. A flag word is read from its top
   bit down, a 1 put after its last bit marking its end: 1 = one literal byte, 0 = a u16
   copy word, back (low 10 bits) and length (high 6 bits + 3). */
static unsigned char *lez_unpack(const unsigned char *d, long len, unsigned *outlen)
{
    unsigned size, words, lits, fp = 0x18, bits, bit, n = 0;
    unsigned char *out;
    if (len < 0x18 || memcmp(d, "LEZ1", 4) != 0)
        return 0;
    size = u32(d + 4);
    words = u32(d + 8);
    lits = u32(d + 12);
    bits = u32(d + 0x14);
    bit = bits >> 31;
    bits = bits * 2 + 1;
    if (size == 0 || size > 0x1000000 || !(out = malloc(size)))
        return 0;
    for (;;) {
        if (bit) {
            if (lits >= (unsigned)len)
                break;
            out[n++] = d[lits++];
        } else {
            unsigned w, back, k;
            if (words + 2 > (unsigned)len)
                break;
            w = d[words] | d[words + 1] << 8;
            words += 2;
            back = w & 0x3ff;
            if (back > n)
                break;
            for (k = (w >> 10) + 3; k && n < size; k--, n++)
                out[n] = out[n - back];
        }
        if (n >= size) {
            *outlen = size;
            return out;
        }
        bit = bits >> 31;
        bits <<= 1;
        if (bits == 0) {
            unsigned w;
            if (fp + 4 > (unsigned)len)
                break;
            w = u32(d + fp);
            fp += 4;
            bit = w >> 31;
            bits = w * 2 + 1;
        }
    }
    free(out);
    return 0;
}

/* 0x43eb00, the rest: the TGA inside (palette length at +5, width +0xc, height +0xe,
   palette from 0x12 as B,G,R, then a byte a pixel, the rows as stored), each palette
   colour cut to the screen's 5-6-5 bits (no palette: a grey ramp). */
static int lez_to_bitmap(Bitmap *b, const unsigned char *t, unsigned len)
{
    uint16_t pal[256];
    int npal, w, h, i;
    const unsigned char *px;
    if (len < 0x12)
        return 0;
    npal = (int16_t)(t[5] | t[6] << 8);
    w = (int16_t)(t[12] | t[13] << 8);
    h = (int16_t)(t[14] | t[15] << 8);
    if (npal < 0 || npal > 256 || w <= 0 || h <= 0 || 0x12u + 3u * npal + (unsigned)(w * h) > len)
        return 0;
    if (npal == 0)
        for (i = 0; i < 256; i++)
            pal[i] = (uint16_t)((i >> 3) << 11 | (i >> 2) << 5 | i >> 3);
    for (i = 0; i < npal; i++) {
        const unsigned char *c = t + 0x12 + 3 * i;
        pal[i] = (uint16_t)((c[2] >> 3) << 11 | (c[1] >> 2) << 5 | c[0] >> 3);
    }
    for (i = npal ? npal : 256; i < 256; i++)
        pal[i] = 0;                    /* (the game reads past its palette here) */
    px = t + 0x12 + 3 * npal;
    b->w = w;
    b->h = h;
    if (b->mode == BM_TILE) {
        int x, y;
        if (w * h < 128 * 128)
            return 0;
        b->pix = malloc(256 * 128 * 2);
        if (!b->pix)
            return 0;
        for (y = 0; y < 128; y++)
            for (x = 0; x < 128; x++)
                b->pix[y * 256 + x] = b->pix[y * 256 + 128 + x] = pal[px[y * 128 + x]];
        return 1;
    }
    b->pix = malloc((size_t)w * h * 2);
    if (!b->pix)
        return 0;
    for (i = 0; i < w * h; i++)
        b->pix[i] = pal[px[i]];
    return 1;
}

int bitmap_load_lez(Bitmap *b, const char *path, int mode)
{
    long len = 0;
    unsigned tlen = 0;
    unsigned char *d = dsi_load(path, &len), *t;
    int ok;
    memset(b, 0, sizeof *b);
    b->mode = mode;
    if (!d)
        return 0;
    t = lez_unpack(d, len, &tlen);
    free(d);
    if (!t)
        return 0;
    ok = lez_to_bitmap(b, t, tlen);
    free(t);
    return ok;
}

int bitmap_load_id(Bitmap *b, int id)
{
    unsigned a;
    for (a = exe_addr(X_BITMAP_TABLE); exe_at(a, 20) && exe_u32(a + 8) != 0xffffffffu; a += 20) {
        const char *name = exe_str(exe_u32(a + 12)), *dot = strrchr(name, '.');
        unsigned lang = exe_u32(a + 4), mode = exe_u32(a + 16);
        char path[256];
        if (exe_u32(a + 8) != (unsigned)id)
            continue;
        if (!dot || strcasecmp(dot, ".lez") != 0) {
            fprintf(stderr, "bitmap %d: %s is not a .lez (24-bit TGA, 0x43e280: not yet)\n", id, name);
            return 0;
        }
        if (lang) {
            /* the language's folder, else the US one */
            snprintf(path, sizeof path, "%s%s", exe_str(exe_u32(exe_addr(X_LANG_FOLDERS) + 4 * g_lang)), name);
            if (bitmap_load_lez(b, path, (int)mode))
                return 1;
            snprintf(path, sizeof path, "%s%s", exe_str(exe_u32(exe_addr(X_LANG_FOLDERS))), name);
        } else
            snprintf(path, sizeof path, "data\\menu\\bitmap\\%s", name);
        if (bitmap_load_lez(b, path, (int)mode))
            return 1;
        fprintf(stderr, "bitmap %d: %s not found\n", id, path);
        return 0;
    }
    fprintf(stderr, "bitmap %d: not in the table\n", id);
    return 0;
}

void bitmap_free(Bitmap *b)
{
    free(b->pix);
    b->pix = 0;
}

void bitmap_place(Bitmap *b, int x, int y, int align)
{
    if (align == 1)
        x = (640 - b->w) >> 1;
    else if (align == 2) {
        x -= b->w >> 1;
        y -= b->h >> 1;
    }
    b->x = x;
    b->y = y;
}

void bitmap_slide(Bitmap *b, int x, int y, float seconds, int ease, int align)
{
    b->t0 = g_now;
    b->dur = (unsigned)(int)((double)seconds * 1000.0);    /* 0.7 s: 699 ms */
    b->t1 = b->t0 + b->dur;
    if (align == 1)
        x = (640 - b->w) >> 1;
    else if (align == 2) {
        y -= b->h >> 1;
        x -= b->w >> 1;
    }
    b->dx = x - b->x;
    b->dy = y - b->y;
    b->from_x = b->x;
    b->from_y = b->y;
    b->sliding = 1;
    b->ease = ease;
}

void bitmap_step(Bitmap *b, int dx, int dy)
{
    unsigned n;
    float f;
    double s;
    if (b->mode == BM_TILE) {
        /* 0x43f840: the scroll, each frame (it also moves a wobble the drawing never
           reads, NOTES §4.7); here each 60th of a second */
        b->sx = (b->sx + dx * g_ticks) & 0x7f;
        b->sy = (b->sy + dy * g_ticks) & 0x7f;
        return;
    }
    if (!b->sliding)
        return;
    n = g_now < b->t1 ? g_now : b->t1;
    f = (float)((double)(n - b->t0) / (double)b->dur);
    if (b->ease == 0)
        s = f;
    else if (b->ease == 1)
        s = (sin(f * 3.141592653589793 - 1.5707963267948966) + 1.0) * 0.5;
    else
        return;
    b->x = (int)(b->dx * s + b->from_x);
    b->y = (int)(b->dy * s + b->from_y);
    if (n == b->t1)
        b->sliding = 0;
}

/* 0x43f0f0: the tile over the whole 640x480 screen, 128 pixels at a time, rows going
   round the tile from sy, each row from sx */
static void draw_tile(const Bitmap *b)
{
    int y, k, row = b->sy;
    const uint16_t *src;
    uint16_t *dst = g_screen;
    for (y = 0; y < 480; y++) {
        src = b->pix + (row & 0x7f) * 256 + (b->sx & 0x7f);
        for (k = 0; k < 5; k++)
            memcpy(dst + 128 * k, src, 128 * 2);
        row = (row & 0x7f) + 1;
        dst += g_pitch;
    }
}

void bitmap_draw(const Bitmap *b, int mode, int pct_h, int pct_w)
{
    int x, y, rows, cols, skip, k;
    const uint16_t *src;
    uint16_t *dst;
    if (!b || !b->pix)
        return;
    if (b->mode == BM_TILE) {
        draw_tile(b);
        return;
    }
    if (pct_h < 0)
        pct_h = 0;
    if (pct_h > 100)
        pct_h = 100;
    if (pct_w < 0)
        pct_w = 0;
    if (pct_w > 100)
        pct_w = 100;
    skip = (100 - pct_h) * b->h / 100;
    x = b->x;
    y = b->y + skip;
    src = b->pix + skip * b->w;
    rows = b->h - skip;
    cols = b->w + pct_w * b->w / -100;
    if (x >= g_scr_w || x + b->w < 0 || y >= g_scr_h || y + rows < 0)
        return;
    if (x < 0) {
        cols += x;
        src -= x;
        x = 0;
    }
    if (y < 0) {
        src -= y * b->w;
        rows += y;
        y = 0;
    }
    if (b->w + x > g_scr_w)
        cols = cols - x + (g_scr_w - b->w);
    if (rows + y > g_scr_h)
        rows = g_scr_h - y;
    dst = g_screen + y * g_pitch + x;
    if (rows <= 0 || cols <= 0)
        return;
    if (mode == BM_OWN)
        mode = b->mode;
    switch (mode) {
    case BM_OPAQUE:
        for (; rows; rows--, src += b->w, dst += g_pitch)
            memcpy(dst, src, cols * 2);
        break;
    case BM_KEYED:
        for (; rows; rows--, src += b->w, dst += g_pitch)
            for (k = 0; k < cols; k++)
                if (src[k])
                    dst[k] = src[k];
        break;
    case BM_HALF:
    case BM_HALF_KEYED: {
        /* every other pixel, the row after starting one pixel later, then one earlier */
        int step = 1, n = cols >> 1, even = cols & ~1;
        for (; rows; rows--) {
            for (k = 0; k < n; k++, src += 2, dst += 2)
                if (mode == BM_HALF || *src)
                    *dst = *src;
            src += b->w + step - even;
            dst += g_pitch + step - even;
            step = -step;
        }
        break;
    }
    }
}
