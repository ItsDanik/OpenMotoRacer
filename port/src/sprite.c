/* The game's 2D sprites (NOTES §4.13.3.2, §4.20 §3.5): .RAW pictures turned into the
   screen's RGB565 and rectangles of them copied to the screen, black see-through. */
#include "sprite.h"
#include "files.h"
#include "video.h"
#include <stdlib.h>
#include <string.h>

/* 0x477b30: 24-bit rows of R, G, B, or a palette (index 0 stays 0, a near-black entry
   gets red 8, off the colour key) */
int sprite_pic_load(SpritePic *p, const char *path)
{
    unsigned char *d;
    long len;
    int w, h, npal, i;
    memset(p, 0, sizeof *p);
    if (!(d = dsi_load(path, &len)))
        return 0;
    w = len >= 32 ? d[8] << 8 | d[9] : 0;
    h = len >= 32 ? d[10] << 8 | d[11] : 0;
    npal = len >= 32 ? d[12] << 8 | d[13] : 0;
    if (len < 32 || memcmp(d, "mhwanh", 6) || w <= 0 || h <= 0
        || len < 32 + 3L * npal + (long)w * h * (npal ? 1 : 3)
        || !(p->pix = malloc(sizeof(uint16_t) * (size_t)w * (size_t)h))) {
        free(d);
        return 0;
    }
    p->w = w, p->h = h;
    for (i = 0; i < w * h; i++) {
        unsigned char rgb[3];
        if (npal) {
            int k = d[32 + 3 * npal + i];
            if (!k) {
                p->pix[i] = 0;
                continue;
            }
            memcpy(rgb, d + 32 + 3 * k, 3);
            if (rgb[0] < 8 && rgb[1] < 4 && rgb[2] < 8)
                rgb[0] = 8;
        } else
            memcpy(rgb, d + 32 + 3 * i, 3);
        p->pix[i] = (uint16_t)((rgb[0] >> 3) << 11 | (rgb[1] >> 2) << 5 | rgb[2] >> 3);
    }
    free(d);
    return 1;
}

void sprite_pic_free(SpritePic *p)
{
    free(p->pix);
    memset(p, 0, sizeof *p);
}

int sprite_draw(const Sprite *s, int x, int y, int r[4])
{
    const SpritePic *p = s->pic;
    int sl, st, sr, sb, L, T, R, B, xx, yy;
    if (!p || !p->pix)
        return 0;
    /* 0x477400: the source rectangle kept inside the picture */
    sl = s->x1 < 0 ? 0 : s->x1;
    st = s->y1 < 0 ? 0 : s->y1;
    sr = s->x2 >= p->w ? p->w - 1 : s->x2;
    sb = s->y2 >= p->h ? p->h - 1 : s->y2;
    if (!(st < sb && sl < sr))
        return 0;
    /* 0x4788b0: the screen rectangle of the source's size (inclusive) */
    L = x + s->ox, T = y + s->oy;
    R = L + (sr - sl), B = T + (sb - st);
    /* 0x478990 with flags 1 (+0x4a): clipped to the screen */
    if (T < 0)
        st -= T, T = 0;
    if (B > g_scr_h - 1)
        B = g_scr_h - 1;
    if (L < 0)
        sl -= L, L = 0;
    if (R > g_scr_w - 1)
        R = g_scr_w - 1;
    if (!(T < B && L < R))
        return 0;
    if (r)
        r[0] = L, r[1] = T, r[2] = R, r[3] = B;
    for (yy = T; yy <= B; yy++) {
        const uint16_t *src = p->pix + (st + yy - T) * p->w + sl;
        uint16_t *dst = g_screen + yy * g_pitch + L;
        if (!s->key)
            memcpy(dst, src, sizeof(uint16_t) * (size_t)(R - L + 1));
        else
            for (xx = 0; xx <= R - L; xx++)
                if (src[xx])            /* the key: black, low = high (0x4f0dd8) */
                    dst[xx] = src[xx];
    }
    return 1;
}
