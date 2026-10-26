#ifndef BITMAP_H
#define BITMAP_H
#include <stdint.h>

/* A menu picture (the game's bitmap object, menu\bitmap.cpp): its place, size, how it is
   drawn and its pixels in the screen's 16-bit format. */
enum {
    BM_OPAQUE = 0,      /* copied as it is */
    BM_KEYED = 1,       /* colour 0 left out */
    BM_HALF = 2,        /* every other pixel, alternating by row (a see-through look) */
    BM_HALF_KEYED = 3,
    BM_TILE = 4,        /* a 128x128 tile filling the screen, scrolled by (sx, sy) */
    BM_OWN = 5          /* to bitmap_draw: the picture's own mode */
};

typedef struct {
    int x, y, w, h;     /* +0, +4, +8, +0xc */
    int mode;           /* +0x14 */
    int sx, sy;         /* +0x18, +0x1c: the tile's scroll */
    uint16_t *pix;      /* +0x2c: w*h pixels; a tile: 128 rows of 256 (the tile twice) */
} Bitmap;

extern int g_lang;      /* 0 US, 1 French, 2 German, 3 Italian, 4 Spanish, 5 Swedish (0x4dbce8) */

/* the picture numbered id in the game's table (0x4e3e20), loaded as 0x43e8e0 does */
int bitmap_load_id(Bitmap *b, int id);
/* a .LEZ file by the game's path */
int bitmap_load_lez(Bitmap *b, const char *path, int mode);
void bitmap_free(Bitmap *b);

/* 0x43ede0: draw at (x, y); only the bottom pct_h % of the rows, and pct_w % of the
   columns cut off on the right */
void bitmap_draw(const Bitmap *b, int mode, int pct_h, int pct_w);

#endif
