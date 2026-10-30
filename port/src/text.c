/* The text objects (NOTES §4.8.3.6): the list of 48 at 0x53b8e8, each a text in a font
   with an effect (blinking, fading, see-through) and a motion, timed by the text clock
   0x4e4ed0. Also the sprite objects (item type 2, 0x418160: pictures side by side, NOTES
   §4.20 §3.5). The race's two lists (0x53ba68 and 0x53b8e8) share the one array here,
   each object marked with its list, 0x53ba68's drawn first as 0x4806e0 does. */
#include "text.h"
#include "clock.h"
#include "font.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEXTS 48
#define SCREEN_W 640    /* 0x5d93c8, 0x5d8948 in the menu */
#define SCREEN_H 480

struct Text {
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
    double fvx, fvy, px, py;    /* +0x18, +0x20, +0x28, +0x30: mode 1's flight, y up */
    int list;                   /* the port's: 1 = 0x53ba68 */
    int unseen;                 /* +0xa4 */
    int nspr;                   /* type 2: +0xac the sprites found */
    const Sprite *spr[8];       /* +0xb4 */
    int sdx[8], sdy[8];         /* +0xf4: the step before each */
};

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
    if (!t || !t->id || (!t->text && !t->nspr))      /* 0x4169b0, 0x416a60 */
        return 0;
    if (!force && t->lifetime && g_clock - t->start > (unsigned)t->lifetime)
        return 0;
    return t->unseen
        || (t->x < SCREEN_W && t->x + t->w > 0 && t->y < SCREEN_H && t->y + t->h > 0);
}

static void delete_text(int i)
{
    if (g_texts[i]) {
        free(g_texts[i]->text);
        free(g_texts[i]);
        g_texts[i] = 0;
    }
}

/* 0x418160's sizes and 0x416430 -> 0x4164b0: the parts' sprites (the hi-res ones at
   640x480 without the small fonts), placed by the sum of their widths and of their
   heights; the object is then as wide as the widths and steps, as high as the first */
static int sprite_parts(Text *t, const TextArgs *a, int *w, int *h)
{
    int hires = !g_small_fonts && g_screen_mode == 2, n = a->parts > 8 ? 8 : a->parts, i;
    *w = *h = 0;
    if (t)
        t->w = t->h = t->nspr = 0;
    for (i = 0; i < n; i++) {
        const Sprite *s = hires ? a->hi[i] : a->lo[i];
        if (!s)
            continue;
        *w += s->x2 - s->x1 + 1;
        *h += s->y2 - s->y1 + 1;
        if (!t)
            continue;
        if (!t->nspr)
            t->h = s->y2 - s->y1 + 1;
        t->w += s->x2 - s->x1 + a->part_dx[i] + 1;
        t->spr[t->nspr] = s;
        t->sdx[t->nspr] = a->part_dx[i];
        t->sdy[t->nspr++] = a->part_dy[i];
    }
    return n > 0 && *w > 0;
}

/* 0x416170 + 0x416300 + 0x4163e0 and the layout (slot 6): a text object of its own;
   with parts, 0x418160's sprite object (its first sprite's offset, added for the dodge
   and taken off again, cancels out here) */
Text *text_new(const TextArgs *a)
{
    Font *f = a->parts ? 0 : font_get(a->font);
    Text *t;
    int w, h, x = a->x, y = a->y, tx = a->tx, ty = a->ty;
    if (a->parts) {
        if (!sprite_parts(0, a, &w, &h))
            return 0;
    } else {
        if (!f)
            return 0;
        font_measure(f, a->text, &w, &h, 0);
        if (!a->bare)
            h += f->line;
    }
    if (a->motion != 1)
        align(w, h, &x, &y, a->align);
    align(w, h, &tx, &ty, a->align);
    t = calloc(1, sizeof *t);
    if (!t)
        return 0;
    if (!a->parts && !(t->text = malloc(strlen(a->text) + 1))) {
        free(t);
        return 0;
    }
    if (t->text)
        strcpy(t->text, a->text);
    t->list = a->list;
    t->unseen = a->bare;
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
    if (a->parts)
        sprite_parts(t, a, &w, &h);
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
    /* mode 1 flies in doubles, y counted up from the bottom */
    t->px = x;
    t->py = SCREEN_H - y;
    t->fvx = a->vx;
    t->fvy = a->vy;
    return t;
}

void text_free(Text *t)
{
    if (t) {
        free(t->text);
        free(t);
    }
}

int text_add(const TextArgs *a)
{
    Text *t;
    int i;
    for (i = 0; i < TEXTS; i++)
        if (!g_texts[i] || !alive(g_texts[i], 0))
            break;
    if (i == TEXTS)
        return 0;
    if (a->dodge_x || a->dodge_y || a->centre_if_full)
        fprintf(stderr, "openmotoracer: text dodging (0x417ac0) is not written yet\n");
    if (!(t = text_new(a)))
        return 0;
    delete_text(i);
    g_texts[i] = t;
    return 1;
}

/* 0x4179f0: an object with this id is in the list and alive */
int text_find(int id)
{
    int i;
    for (i = 0; i < TEXTS; i++)
        if (id && g_texts[i] && g_texts[i]->id == id && alive(g_texts[i], 0))
            return 1;
    return 0;
}

/* 0x4177e0: the object with this id taken out */
void text_remove(int id)
{
    int i;
    for (i = 0; i < TEXTS; i++)
        if (id && g_texts[i] && g_texts[i]->id == id)
            delete_text(i);
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

/* 0x416b40, mode 1: turn towards the target and close in on it (k the rate) */
static void fly(Text *t)
{
    double dx, dy, dist, aim, head, speed, diff;
    if (t->x == t->tx && t->y == t->ty)
        return;
    dx = t->tx - t->px;
    dy = (SCREEN_H - t->ty) - t->py;
    dist = sqrt(dx * dx + dy * dy);
    if (dist <= 1.5) {
        t->x = t->tx;
        t->y = t->ty;
        t->px = t->tx;
        t->py = SCREEN_H - t->ty;
        return;
    }
    aim = atan2(dy, dx);
    head = atan2(t->fvy, t->fvx);
    speed = sqrt(t->fvx * t->fvx + t->fvy * t->fvy);
    if (aim > M_PI / 2 && aim - M_PI > head) {
        head += 2 * M_PI;
        diff = aim - head;
    } else if (aim <= -M_PI / 2 && aim + M_PI < head)
        diff = aim + 2 * M_PI - head;
    else
        diff = aim - head;
    head += t->k * diff * 0.05;
    speed = (dist - speed) * t->k * 0.05;
    t->fvx = cos(head) * speed;
    t->fvy = sin(head) * speed;
    t->px += t->fvx;
    t->py += t->fvy;
    t->x = (int)t->px;
    t->y = (int)(SCREEN_H - t->py);
}

/* 0x416b40: one step a drawn frame (here each 60th of a second) */
static void move(Text *t)
{
    int i;
    for (i = 0; i < g_ticks; i++)
        if (t->motion == 2) {
            t->s += t->k;
            t->x += (int)(t->vx * t->s);
            t->y += (int)(t->vy * t->s);
        } else if (t->motion == 1)
            fly(t);
}

/* 0x416b00: a flying text put at its target */
void text_snap(Text *t)
{
    if (t->motion == 1 || t->motion == 3) {
        t->x = t->tx;
        t->y = t->ty;
        t->px = t->x;
        t->py = SCREEN_H - t->y;
    }
}

/* 0x4172d0: where the text is (mode 1: where it is going), inclusive */
void text_rect(const Text *t, int r[4])
{
    int x = t->motion == 1 ? t->tx : t->x, y = t->motion == 1 ? t->ty : t->y;
    r[0] = x;
    r[1] = y;
    r[2] = x + t->w - 1;
    r[3] = y + t->h - 1;
}

/* 0x417210: where it is now */
void text_now(const Text *t, int r[4])
{
    r[0] = t->x;
    r[1] = t->y;
    r[2] = t->x + t->w - 1;
    r[3] = t->y + t->h - 1;
}

void text_box(const Text *t, int *w, int *h)
{
    *w = t->w;
    *h = t->h;
}

void text_place(Text *t, int x, int y)
{
    t->x = x;
    t->y = y;
}

void text_life(Text *t, int ms)
{
    t->lifetime = ms;
}

void text_wake(Text *t)
{
    if (alive(t, 1))
        t->start = g_clock;
}

void text_seen(Text *t)
{
    if (t->x < SCREEN_W && t->x + t->w > 0 && t->y < SCREEN_H && t->y + t->h > 0)
        t->unseen = 0;
}

int text_alive(const Text *t)
{
    return alive(t, 0);
}

char *text_string(Text *t)
{
    return t->text;
}

/* what the closing slide copies (0x4354e0) */
void text_args_of(const Text *t, TextArgs *a)
{
    memset(a, 0, sizeof *a);
    a->text = t->text;
    a->font = t->font;
    a->x = t->x;
    a->y = t->y;
    a->own_colours = t->own_colours;
    a->fore = t->fore;
    a->back = t->back;
}

/* 0x417080: a sprite object; effects 3 and 4 only blink it. Each part is drawn at the
   right edge of the one before (its last column, as drawn) plus its step. */
static void draw_sprites(Text *t)
{
    unsigned elapsed = g_clock - t->start;
    int x = t->x, y, i, r[4];
    if (t->effect == FX_BLINK || t->effect == FX_BLINK_TRANSP) {
        if (t->period == 0) {
            if (((t->frames += g_ticks) & t->mask) == t->mask)
                return;
        } else if (elapsed % (2u * (unsigned)t->period) >= (unsigned)t->period)
            return;
    }
    for (i = 0; i < t->nspr; i++) {
        x += t->sdx[i];
        y = t->y + t->sdy[i];
        if (sprite_draw(t->spr[i], x, y, r))
            x = r[2];
    }
}

/* 0x416da0 */
void text_draw_one(Text *t)
{
    Font *f = font_get(t->font);
    unsigned elapsed = g_clock - t->start;
    int visible = 1, level = 0;
    move(t);
    if (t->nspr) {
        draw_sprites(t);
        return;
    }
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
            visible = ((t->frames += g_ticks) & t->mask) != t->mask;
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
    int i, list;
    for (list = 1; list >= 0; list--)
        for (i = 0; i < TEXTS; i++)
            if (alive(g_texts[i], 0) && g_texts[i]->list == list)
                text_draw_one(g_texts[i]);
}

void text_clear(void)
{
    int i;
    for (i = 0; i < TEXTS; i++)
        delete_text(i);
}
