/* The text objects (NOTES §4.8.3.6): the list of 48 at 0x53b8e8, each a text in a font
   with an effect (blinking, fading, see-through) and a motion, timed by the text clock
   0x4e4ed0. */
#include "text.h"
#include "font.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEXTS 48
#define SCREEN_W 640    /* 0x5d93c8, 0x5d8948 in the menu */
#define SCREEN_H 480

typedef struct {
    int align;                  /* +8 */
    int effect, motion;         /* +0x10, +0x14 */
    double s, k;                /* +0x40, +0x38 */
    int vx, vy;                 /* +0x58, +0x5c */
    int tx, ty;                 /* +0x60, +0x64 */
    int x, y;                   /* +0x70, +0x74 */
    int w, h;                   /* +0x80, +0x84 */
    unsigned start;             /* +0x88 */
    int lifetime;               /* +0x8c */
    int period;                 /* +0x90 */
    int frames, mask;           /* +0x94, +0x98 */
    int id;                     /* +0x9c */
    char *text;                 /* +0xac */
    int font;                   /* +0xb4 */
    int own_colours;            /* +0xb8 */
    unsigned char fore[3], back[3]; /* +0xbc, +0xbf */
} Text;

static Text *g_texts[TEXTS];
static unsigned g_clock;        /* 0x4e4ed0 */
static int g_next_id;           /* 0x4e4ecc */

void text_clock(unsigned ms)
{
    g_clock = ms;
}

/* 0x417900 */
static void align(int w, int h, int *x, int *y, int flags)
{
    int fx = flags & 0x1f, fy = flags & 0xe0;
    if (fx & 6) {
        if (fx & 0x18)
            *x = SCREEN_W / 2 - *x - w;
        else if (fx & 1)
            *x += SCREEN_W / 2;
        else
            *x += (SCREEN_W - w) / 2;
    } else if (fx & 0x18)
        *x = SCREEN_W - *x - w;
    if (fy & 0x40) {
        if (fy & 0x80)
            *y = SCREEN_H / 2 - *y - h;
        else if (fy & 0x20)
            *y += SCREEN_H / 2;
        else
            *y += (SCREEN_H - h) / 2;
    } else if (fy & 0x80)
        *y = SCREEN_H - *y - h;
}

/* 0x4169b0 */
static int alive(const Text *t, int force)
{
    if (!t || !t->id || !t->text)
        return 0;
    if (!force && t->lifetime && g_clock - t->start > (unsigned)t->lifetime)
        return 0;
    return t->x < SCREEN_W && t->x + t->w > 0 && t->y < SCREEN_H && t->y + t->h > 0;
}

static void delete_text(int i)
{
    if (g_texts[i]) {
        free(g_texts[i]->text);
        free(g_texts[i]);
        g_texts[i] = 0;
    }
}

int text_add(const TextArgs *a)
{
    Font *f = font_get(a->font);
    Text *t;
    int i, w, h, x = a->x, y = a->y, tx = a->tx, ty = a->ty;
    if (!f)
        return 0;
    for (i = 0; i < TEXTS; i++)
        if (!g_texts[i] || !alive(g_texts[i], 0))
            break;
    if (i == TEXTS)
        return 0;
    delete_text(i);
    font_measure(f, a->text, &w, &h, 0);
    h += f->line;
    if (a->motion != 1)
        align(w, h, &x, &y, a->align);
    align(w, h, &tx, &ty, a->align);
    if (a->dodge_x || a->dodge_y || a->centre_if_full)
        fprintf(stderr, "openmotoracer: text dodging (0x417ac0) is not written yet\n");
    if (a->motion == 1 || a->motion == 3)
        fprintf(stderr, "openmotoracer: text motion 1 (0x416b40) is not written yet\n");
    t = calloc(1, sizeof *t);
    if (!t || !(t->text = malloc(strlen(a->text) + 1))) {
        free(t);
        return 0;
    }
    strcpy(t->text, a->text);
    t->align = a->align;
    t->effect = a->effect;
    t->motion = a->motion;
    t->s = a->s;
    t->k = a->k;
    t->vx = a->vx;
    t->vy = a->vy;
    t->tx = tx;
    t->ty = ty;
    t->x = x;
    t->y = y;
    t->w = w;
    t->h = h;
    t->start = g_clock;
    t->lifetime = a->lifetime;
    if (a->blink < 0)
        t->period = -a->blink;
    else if (a->blink > 0)
        t->mask = a->blink;
    else if (a->effect == FX_BLINK || a->effect == FX_BLINK_TRANSP)
        t->mask = 2;
    t->id = ++g_next_id;
    if (a->id_out)
        *a->id_out = t->id;
    t->font = a->font;
    t->own_colours = a->own_colours;
    memcpy(t->fore, a->fore ? a->fore : named_colour(COL_WHITE), 3);
    memcpy(t->back, a->back ? a->back : named_colour(COL_GREY), 3);
    g_texts[i] = t;
    return 1;
}

void text_update(void)
{
    int i;
    for (i = 0; i < TEXTS; i++) {
        Text *t = g_texts[i];
        if (!t)
            continue;
        if (t->lifetime && g_clock - t->start > (unsigned)t->lifetime) {
            if (t->motion == 3) {
                t->motion = 1;
                t->start = g_clock;
            } else if (t->motion == 4) {
                t->motion = 2;
                t->lifetime = 0;
                if (!t->vx && !t->vy)
                    t->vy = 1;
                if (t->s == 0)
                    t->s = 1.0;
            }
        }
        if (!alive(t, 0))
            delete_text(i);
    }
}

int text_empty(void)
{
    int i;
    for (i = 0; i < TEXTS; i++)
        if (alive(g_texts[i], 0))
            return 0;
    return 1;
}

/* 0x416b40: one step a drawn frame */
static void move(Text *t)
{
    if (t->motion == 2) {
        t->s += t->k;
        t->x += (int)(t->vx * t->s);
        t->y += (int)(t->vy * t->s);
    }
}

/* 0x416da0 */
static void draw_one(Text *t)
{
    Font *f = font_get(t->font);
    unsigned elapsed = g_clock - t->start;
    int visible = 1, level = 0;
    move(t);
    switch (t->effect) {
    case FX_TRANSP75: level = 75; break;
    case FX_TRANSP50: level = 50; break;
    case FX_TRANSP25: level = 25; break;
    case FX_FADE_IN:
        if (t->lifetime > 0)
            level = 100 - (int)(elapsed * 100 / (unsigned)t->lifetime);
        else if (t->period > 0)
            level = (1 - (int)(elapsed % (unsigned)t->period)) * 100;
        break;
    case FX_FADE_OUT:
        if (t->lifetime > 0)
            level = (int)(elapsed * 100 / (unsigned)t->lifetime);
        else if (t->period > 0)
            level = (int)(elapsed % (unsigned)t->period) * 100;
        break;
    case FX_BLINK:
        if (t->period == 0)
            visible = (++t->frames & t->mask) != t->mask;
        else
            visible = elapsed % (2u * (unsigned)t->period) < (unsigned)t->period;
        break;
    case FX_BLINK_TRANSP: {
        unsigned p = t->period ? (unsigned)t->period : 500, e = elapsed % (2 * p);
        if (e > p)
            e = 2 * p - e;
        level = (int)((((e << 4) / p) * 100) >> 4);
        break;
    }
    }
    if (!visible || !f)
        return;
    if (t->own_colours) {
        unsigned char fore[3], back[3];
        memcpy(fore, f->fore, 3);
        memcpy(back, f->back, 3);
        font_set_colors(f, t->fore, t->back);
        font_draw(f, t->text, t->x, t->y, 0x21, level);
        font_set_colors(f, fore, back);
    } else
        font_draw(f, t->text, t->x, t->y, 0x21, level);
}

void text_draw(void)
{
    int i;
    for (i = 0; i < TEXTS; i++)
        if (alive(g_texts[i], 0))
            draw_one(g_texts[i]);
}

void text_clear(void)
{
    int i;
    for (i = 0; i < TEXTS; i++)
        delete_text(i);
}
