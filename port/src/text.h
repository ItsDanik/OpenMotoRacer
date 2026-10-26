#ifndef TEXT_H
#define TEXT_H
#include "sprite.h"

/* The moving, blinking, fading texts of the menus and the race: the list of 48 at
   0x53b8e8 (NOTES §4.8.3.6). */

enum { FX_NONE, FX_FADE_IN, FX_FADE_OUT, FX_BLINK, FX_BLINK_TRANSP, FX_TRANSP75,
       FX_TRANSP50, FX_TRANSP25 };

typedef struct {                /* 0x417d20's arguments */
    const char *text;           /* 1 */
    int lifetime;               /* 2: ms, 0 = until removed */
    int font;                   /* 3: a font number (through font_get) */
    int x, y;                   /* 4, 5 */
    int align;                  /* 6: 0x417900's flags */
    int effect;                 /* 7: FX_* */
    int motion;                 /* 8: 0 still, 1 fly to the target, 2 drift, 3 and 4 wait
                                   the lifetime then 1 or 2 */
    int blink;                  /* 9: < 0 a period of -n ms, > 0 a frame mask */
    int *id_out;                /* 10 */
    int dodge_x, dodge_y;       /* 11, 12: steps to get out of other texts' way */
    int centre_if_full;         /* 13 */
    int vx, vy;                 /* 16, 17 */
    int tx, ty;                 /* 18, 19: the target */
    double s, k;                /* 20+21, 22+23 */
    int own_colours;            /* 24 */
    const unsigned char *fore, *back;   /* 25, 26: R,G,B */
    /* the port's: which of the race's lists (0 = 0x53b8e8, 1 = 0x53ba68, drawn first) */
    int list;
    /* a sprite object instead of a text (0x418160, item type 2; text and font unused):
       up to 8 parts, each a low-res and a hi-res sprite and a step before it */
    int parts;
    const Sprite *lo[8], *hi[8];
    int part_dx[8], part_dy[8];
} TextArgs;

typedef struct Text Text;      /* a text object (0xc2 bytes) */

/* one of one's own, outside the list (the race's dialogs) */
Text *text_new(const TextArgs *a);
void text_free(Text *t);
void text_draw_one(Text *t);            /* 0x416da0: a step of its motion, then drawn */
void text_snap(Text *t);                /* 0x416b00 */
void text_rect(const Text *t, int r[4]);        /* 0x4172d0 */
void text_now(const Text *t, int r[4]);         /* 0x417210 */
int text_alive(const Text *t);
char *text_string(Text *t);
void text_args_of(const Text *t, TextArgs *a);

void text_clock(unsigned ms);           /* 0x4178c0 */
int text_add(const TextArgs *a);        /* 0x417d20 (a sprite object: 0x418160): 1 if added */
int text_find(int id);                  /* 0x4179f0: alive in the list */
void text_remove(int id);               /* 0x4177e0 */
void text_update(void);                 /* 0x418790 */
int text_empty(void);                   /* 0x417840 */
void text_draw(void);                   /* 0x418860 */
void text_clear(void);                  /* 0x417750 */

#endif
