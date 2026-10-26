#ifndef TEXT_H
#define TEXT_H

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
} TextArgs;

void text_clock(unsigned ms);           /* 0x4178c0 */
int text_add(const TextArgs *a);        /* 0x417d20: 1 if added */
void text_update(void);                 /* 0x418790 */
int text_empty(void);                   /* 0x417840 */
void text_draw(void);                   /* 0x418860 */
void text_clear(void);                  /* 0x417750 */

#endif
