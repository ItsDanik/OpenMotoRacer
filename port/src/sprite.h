#ifndef SPRITE_H
#define SPRITE_H
#include <stdint.h>

/* The game's 2D sprites: rectangles of a .RAW picture held by one of the banks at
   0x5d3a20 (NOTES §4.13.3.2), drawn by 0x477400 with a colour key. */

typedef struct {                /* a bank's picture (its surface), RGB565 */
    uint16_t *pix;
    int w, h;
} SpritePic;

typedef struct {                /* the sprite record (0x4e bytes, 0x477790) */
    const SpritePic *pic;       /* +0x30: the bank */
    int x1, y1, x2, y2;         /* +0x1c..+0x28, inclusive */
    int ox, oy;                 /* +0xc - +4, +0x10 - +8: where it is drawn from x, y */
    int key;                    /* +0x3a: black (0x4f0dd8) is see-through */
} Sprite;

/* 0x477b30: a .RAW picture as the game reads it (dsi_load), 0 if missing */
int sprite_pic_load(SpritePic *p, const char *path);
void sprite_pic_free(SpritePic *p);

/* 0x477400 -> 0x4788b0 -> 0x478990: s drawn at (x, y) clipped to the screen; r, if
   given, gets the rectangle drawn (inclusive). 0 when nothing was drawn (r untouched). */
int sprite_draw(const Sprite *s, int x, int y, int r[4]);

#endif
