#include "generic.h"
#include "bitmap.h"
#include "clock.h"
#include "dialog.h"
#include "exe.h"
#include "files.h"
#include "font.h"
#include "sound.h"
#include "text.h"
#include "video.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MAX_LINES = 0x200, MAX_BLOCKS = 0x80, BLOCK_LINES = 0x50 };

typedef struct {                /* a piece of a line (one text object) */
    Text *t;
    int width;                  /* +0xa8: [W], -1 its own */
    double x, y;                /* +0x28, +0x30 */
} Piece;

typedef struct {                /* 0x3c bytes */
    Piece *p;                   /* +4 */
    int n;                      /* +8 */
} Line;

typedef struct {                /* 0x4d bytes */
    Line *line[BLOCK_LINES];    /* +0 */
    int nlines;                 /* +0x1c */
    int flags;                  /* +0x14: [H], [V] */
    int life;                   /* +0x20: [L] */
    int mh, mv, mx, my;         /* +0x24..+0x30 */
    int id;                     /* +0x45: [I] */
    int dark;                   /* +0x49: [D], the box under it */
    char name[17];              /* +0x34: [S] */
} Block;

typedef struct {                /* 0x20 bytes */
    Line lines[MAX_LINES];      /* +0, +8 */
    int nlines;
    Block blocks[MAX_BLOCKS];   /* +4, +0xc */
    int nblocks;
    Block *list[MAX_BLOCKS];    /* +0x10, +0x18 */
    int nlist;
    int cur;                    /* +0x14 */
} Script;

/* what the tags leave for the texts after them, reset for each file */
static int g_font, g_effect, g_blink, g_flags, g_life, g_mh, g_mv, g_mx, g_my, g_id, g_dark;
static unsigned char g_fore[3], g_back[3];
static char g_name[17];
static unsigned g_last;         /* 0x4f1630 */

static void tag_defaults(void)  /* 0x48a4a0 */
{
    g_font = 12;
    g_effect = FX_NONE;
    g_fore[0] = 0xff, g_fore[1] = g_fore[2] = 0;
    memset(g_back, 0, sizeof g_back);
    g_blink = -500;
    g_flags = 0x42;
    g_life = 4000;
    g_mh = g_mv = g_mx = g_my = 0;
    g_id = -1;
    g_dark = 0;
}

/* a tag's number: n characters, all digits or signs, read by %d */
static int number(const char *s, int n, int *out)
{
    char buf[32];
    int i;
    if (n <= 0 || !*s || n >= (int)sizeof buf)
        return 0;
    for (i = 0; i < n; i++)
        if (!isdigit((unsigned char)s[i]) && s[i] != '-' && s[i] != '+')
            return 0;
    memcpy(buf, s, (size_t)n);
    buf[n] = 0;
    return sscanf(buf, "%d", out) == 1;
}

/* [C r,g,b], [G r,g,b]: each number kept if it reads */
static void colour(unsigned char *c, const char *s)
{
    const char *comma = strchr(s, ',');
    int v;
    if (!*s || !comma)
        return;
    if (number(s, (int)(comma - s), &v))
        c[0] = (unsigned char)v;
    s = comma + 1;
    if (!*s || !(comma = strchr(s, ',')))
        return;
    if (number(s, (int)(comma - s), &v))
        c[1] = (unsigned char)v;
    s = comma + 1;
    if (*s && number(s, (int)strlen(s), &v))
        c[2] = (unsigned char)v;
}

/* [E name]: as much of the name as is given (the first three, then the other five) */
static void effect(const char *s, size_t n)
{
    static const char *const a[] = { "None", "FadeIn", "FadeOut" };
    static const char *const b[] = { "Blink", "BlinkTransp", "Transp75", "Transp50", "Transp25" };
    size_t i;
    for (i = 0; i < 3; i++)
        if (strncmp(a[i], s, n) == 0) {
            g_effect = (int)i;
            break;
        }
    for (i = 0; i < 5; i++)
        if (strncmp(b[i], s, n) == 0) {
            g_effect = (int)i + 3;
            break;
        }
}

/* 0x490690: the font by its name, the 8th if none has it */
static int font_named(const char *s)
{
    int i;
    if (*s)
        for (i = 0; i < 35; i++) {
            Font *f = font_raw(i);
            if (f && strncmp(f->name, s, 15) == 0)
                return i;
        }
    return 8;
}

static void tag(char *s, size_t n)
{
    int v, rest = (int)n - 1;
    char *a = s + 1;
    *s = (char)toupper((unsigned char)*s);
    switch (*s) {
    case 'B':
        if (number(a, rest, &v))
            g_blink = v;
        break;
    case 'C':
        colour(g_fore, a);
        break;
    case 'D':
        if (number(a, rest, &v))
            g_dark = v;
        break;
    case 'E':
        effect(a, (size_t)rest);
        break;
    case 'F':
        g_font = font_named(a);
        break;
    case 'G':
        colour(g_back, a);
        break;
    case 'H':
        switch (toupper((unsigned char)*a)) {
        case 'C': g_flags = (g_flags & ~0x1d) | 2; break;
        case 'L': g_flags = (g_flags & ~0x1e) | 1; break;
        case 'R': g_flags = (g_flags & ~0x17) | 8; break;
        }
        break;
    case 'I':
        if (number(a, rest, &v))
            g_id = v;
        break;
    case 'L':
        if (number(a, rest, &v))
            g_life = v;
        break;
    case 'M':
        switch (toupper((unsigned char)*a)) {
        case 'H': if (number(a + 1, rest - 1, &v)) g_mh = v; break;
        case 'V': if (number(a + 1, rest - 1, &v)) g_mv = v; break;
        case 'X': if (number(a + 1, rest - 1, &v)) g_mx = v; break;
        case 'Y': if (number(a + 1, rest - 1, &v)) g_my = v; break;
        }
        break;
    case 'S':
        memcpy(g_name, a, rest > 16 ? 16 : (size_t)rest);
        break;
    case 'V':
        switch (toupper((unsigned char)*a)) {
        case 'C': g_flags = (g_flags & ~0xa0) | 0x40; break;
        case 'D': g_flags = (g_flags & ~0x60) | 0x80; break;
        case 'U': g_flags = (g_flags & ~0xc0) | 0x20; break;
        }
        break;
    }
}

/* a text's \ codes: \\ \< \> \( \) the characters they stand for, \NNN a character's number */
static void unescape(char *o, const char *s, size_t n)
{
    size_t i = 0;
    while (i < n) {
        if (s[i] != '\\') {
            *o++ = s[i++];
            continue;
        }
        i++;
        switch (s[i]) {
        case '\\': *o++ = '\\'; i++; break;
        case '<': *o++ = '{'; i++; break;
        case '>': *o++ = '}'; i++; break;
        case '(': *o++ = '['; i++; break;
        case ')': *o++ = ']'; i++; break;
        default:
            if (isdigit((unsigned char)s[i])) {
                unsigned char c = 0;
                while (isdigit((unsigned char)s[i]))
                    c = (unsigned char)(c * 10 + s[i++] - '0');
                *o++ = (char)c;
            } else {
                *o++ = 0;
                i++;
            }
        }
    }
    *o = 0;
}

static void add_piece(Line *l, const char *s, size_t n, int width)
{
    TextArgs a;
    char *text = malloc(n + 1);
    Text *t;
    if (!text)
        return;
    unescape(text, s, n);
    memset(&a, 0, sizeof a);
    a.text = text;
    a.lifetime = g_life;
    a.font = g_font;
    a.align = 0x21;
    a.effect = g_effect;
    a.blink = g_blink;
    a.own_colours = 1;
    a.fore = g_fore;
    a.back = g_back;
    a.bare = 1;
    t = text_new(&a);
    free(text);
    if (!t)
        return;
    l->p[l->n].t = t;
    l->p[l->n].width = width;
    l->n++;
}

/* 0x488a30: a line's tags and texts. 1 if it has a text */
static int parse_line(Line *l, char *s)
{
    int count = 1, width = -1;
    const char *c;
    for (c = s; (c = strchr(c, '[')) != 0; c++)
        count++;
    if (!*s || !(l->p = calloc((size_t)count, sizeof *l->p)))
        return 0;
    while (*s) {
        char *e;
        if (*s != '[') {
            for (e = s; *e && *e != '['; e++)
                ;
            if (l->n < count)
                add_piece(l, s, (size_t)(e - s), width);
            width = -1;
            s = e;
        } else if (s[1] && (e = strchr(s + 1, ']')) != 0 && e != s + 1) {
            size_t n = (size_t)(e - s - 1);
            char *one = malloc(n + 1);
            if (one) {
                int v;
                memcpy(one, s + 1, n);
                one[n] = 0;
                if (toupper((unsigned char)*one) != 'W')
                    tag(one, n);
                else if (number(one + 1, (int)n - 1, &v))
                    width = v;
                free(one);
            }
            s = e + 1;
        } else
            s++;
    }
    return l->n > 0;
}

/* 0x48a7c0: a block's lines. 1 if the last line with a text went in */
static int parse_block(Script *sc, Block *b, char *s)
{
    int ok = 0;
    while (*s) {
        char *e = s, keep;
        size_t n;
        while (*e && *e != '\n')
            e++;
        n = (size_t)(e - s);
        keep = *e;
        *e = 0;
        if (n > 0 && s[n - 1] == '\r')
            s[n - 1] = 0;
        if (sc->nlines < MAX_LINES) {
            Line *l = &sc->lines[sc->nlines++];
            if (parse_line(l, s)) {
                ok = 0;
                if (b->nlines < BLOCK_LINES) {
                    b->line[b->nlines++] = l;
                    ok = 1;
                }
            }
        }
        s = e;
        if (keep == '\n')
            s++;
    }
    return ok;
}

/* 0x48a000 and 0x489cf0: the lines put together and on the screen, from y0; next: where the
   block after it would begin */
static void layout(Block *b, int y0, int *next)
{
    int i, j, y = 0, dy = 0;
    for (i = 0; i < b->nlines; i++) {
        Line *l = b->line[i];
        int x = 0, sum = 0, h = 0, dx = 0;
        for (j = 0; j < l->n; j++) {
            int pw, ph, w;
            text_box(l->p[j].t, &pw, &ph);
            w = l->p[j].width == -1 ? pw : l->p[j].width;
            l->p[j].x = x;
            x += w + 10;
            sum += w;
            if (h < ph)
                h = ph;
        }
        if (l->n > 1)
            sum += (l->n * 5 - 5) * 2;
        if (b->flags & 2)
            dx = (g_scr_w - sum) / 2;
        else if (b->flags & 8)
            dx = g_scr_w - sum;
        for (j = 0; j < l->n; j++) {
            l->p[j].x += dx;
            l->p[j].y = y0 + y;
        }
        y += h;
    }
    if (b->flags & 0x40)
        dy = (g_scr_h - y) / 2;
    else if (b->flags & 0x80)
        dy = g_scr_h - y;
    if (next)
        *next = y0 + y + dy;
    for (i = 0; i < b->nlines; i++) {
        Line *l = b->line[i];
        for (j = 0; j < l->n; j++) {
            l->p[j].y += dy;
            text_place(l->p[j].t, (int)l->p[j].x, (int)l->p[j].y);
            text_life(l->p[j].t, b->life);
        }
    }
}

static void script_free(Script *sc)
{
    int i, j;
    if (!sc)
        return;
    for (i = 0; i < sc->nlines; i++) {
        for (j = 0; j < sc->lines[i].n; j++)
            text_free(sc->lines[i].p[j].t);
        free(sc->lines[i].p);
    }
    free(sc);
}

/* 0x48a2f0, 0x48a4a0: a script read. 0 if the file is missing or empty */
static Script *script_load(const char *path)
{
    long len = 0;
    char *d = (char *)dsi_load(path, &len), *s, *t;
    Script *sc;
    int i;
    if (!d)
        return 0;
    if (len <= 0 || !(sc = calloc(1, sizeof *sc))) {
        free(d);
        return 0;
    }
    if (!(t = malloc((size_t)len + 1))) {
        free(d);
        free(sc);
        return 0;
    }
    memcpy(t, d, (size_t)len);
    t[len] = 0;
    free(d);
    sc->cur = -1;
    tag_defaults();
    for (s = t; (s = strchr(s, '{')) != 0;) {
        char *e = strchr(++s, '}');
        Block *b;
        if (!e)
            break;
        if (e > s && sc->nblocks < MAX_BLOCKS) {
            b = &sc->blocks[sc->nblocks++];
            *e = 0;
            memset(g_name, 0, sizeof g_name);
            if (parse_block(sc, b, s)) {
                b->mh = g_mh, b->mv = g_mv, b->mx = g_mx, b->my = g_my;
                b->flags = g_flags;
                b->life = g_life;
                b->id = g_id;
                b->dark = g_dark;
                memcpy(b->name, g_name, 16);
                if (sc->nlist < MAX_BLOCKS)
                    sc->list[sc->nlist++] = b;
            }
            s = e + 1;
        } else
            s = e;
    }
    free(t);
    for (i = 0; i < sc->nlist; i++)
        layout(sc->list[i], 0, 0);
    return sc;
}

static int block_alive(const Block *b)
{
    int i, j;
    for (i = 0; i < b->nlines; i++)
        for (j = 0; j < b->line[i]->n; j++)
            if (text_alive(b->line[i]->p[j].t))
                return 1;
    return 0;
}

static void block_begin(Block *b)
{
    int i, j;
    for (i = 0; i < b->nlines; i++)
        for (j = 0; j < b->line[i]->n; j++)
            text_wake(b->line[i]->p[j].t);
    g_last = g_now;
}

/* 0x48a150: the darkened box, then the lines still alive, moving (0x489de0); a played script's
   piece is marked once it has been on the screen, the help's never are, so that they stay
   alive off it */
static void block_draw(Block *b, int played)
{
    unsigned dt = g_now - g_last;
    double dx = 0.0, dy = 0.0;
    int i, j;
    if (dt) {
        if (b->mx && b->mh)
            dx = b->mx * ((double)((unsigned)g_scr_w * dt) / b->mh);
        if (b->my && b->mv)
            dy = b->my * ((double)((unsigned)g_scr_h * dt) / b->mv);
    }
    if (b->dark) {              /* 0x489ef0, 0x49a5a0 */
        int r[4] = { g_scr_w - 1, g_scr_h - 1, 0, 0 }, n[4], found = 0;
        for (i = 0; i < b->nlines; i++)
            for (j = 0; j < b->line[i]->n; j++)
                if (text_alive(b->line[i]->p[j].t)) {
                    text_now(b->line[i]->p[j].t, n);
                    found = 1;
                    if (n[0] < r[0]) r[0] = n[0];
                    if (n[1] < r[1]) r[1] = n[1];
                    if (n[2] > r[2]) r[2] = n[2];
                    if (n[3] > r[3]) r[3] = n[3];
                }
        if (found)
            dialog_shade(r[0], r[1], r[2], r[3]);
    }
    for (i = 0; i < b->nlines; i++) {
        Line *l = b->line[i];
        int alive = 0;
        for (j = 0; j < l->n && !alive; j++)
            alive = text_alive(l->p[j].t);
        if (!alive)
            continue;
        for (j = 0; j < l->n; j++) {
            l->p[j].x += dx;
            l->p[j].y += dy;
            text_place(l->p[j].t, (int)l->p[j].x, (int)l->p[j].y);
            text_draw_one(l->p[j].t);
            if (played)
                text_seen(l->p[j].t);
        }
    }
}

/* 0x48a9e0: the block whose time it is drawn; 0 when none is left */
static int script_play(Script *sc)
{
    if (sc->cur == -1) {
        sc->cur = 0;
        if (sc->nlist > 0)
            block_begin(sc->list[0]);
    }
    while (sc->cur < sc->nlist && !block_alive(sc->list[sc->cur]))
        if (++sc->cur < sc->nlist)
            block_begin(sc->list[sc->cur]);
    if (sc->cur >= sc->nlist)
        return 0;
    block_draw(sc->list[sc->cur], 1);
    g_last = g_now;
    return 1;
}

/* ------------------------------------------------------------------ the sequence */

static Script *g_seq;           /* 0x4f15f0 */
static Script *g_help;          /* 0x4f15f8 */
static int g_help_at;           /* its +0x20 */
static int g_running;           /* 0x4f15f4 */
static int g_brief;             /* 0x4f1634 */
static int g_index;             /* 0x4f1638 */

void credits_start(int brief)
{
    g_brief = brief;
    g_index = -1;
    g_running = 1;
    music_play(12, 0);
}

int credits_running(void)
{
    return g_running;
}

/* 0x48ae40: the next script of the list when none is open (0x4f16a0 the long list,
   0x4f1640 the short; the GP edition's are never used). 0 if none opened */
static int next_script(void)
{
    if (g_seq)
        return 1;
    if (g_index >= 10) {
        g_index = 11;
        g_running = 0;
        return 0;
    }
    g_index++;
    g_seq = script_load(exe_str(exe_u32(exe_addr(g_brief ? X_GEN_SHORT : X_GEN_LONG)
                                        + 4u * (unsigned)g_index)));
    return g_seq != 0;
}

void credits_frame(void)
{
    if (!g_running || g_help)   /* (0x47ff90: 0x4f15f8) */
        return;
    if (g_index > 10) {
        g_running = 0;
        return;
    }
    if (!next_script() || script_play(g_seq))
        return;
    script_free(g_seq);
    g_seq = 0;
    if (!next_script() || script_play(g_seq))
        return;
    script_free(g_seq);
    g_seq = 0;
}

void credits_stop(void)
{
    g_index = 11;
    script_free(g_seq);
    g_seq = 0;
    g_running = 0;
}

/* ------------------------------------------------------------------ the help */

static void help_stack(void)
{
    int i, y = 0;
    for (i = 0; i < g_help->nlist; i++)
        layout(g_help->list[i], y, &y);
}

int help_open(void)
{
    char path[64];
    help_close();
    snprintf(path, sizeof path, "%sHelp.gen", exe_str(exe_u32(exe_addr(X_LANG_FOLDERS)
                                                              + 4u * (unsigned)g_lang)));
    if (!(g_help = script_load(path)))
        return 0;
    help_stack();
    g_help_at = 0;
    return 1;
}

void help_close(void)
{
    script_free(g_help);
    g_help = 0;
}

int help_pages(void)
{
    return g_help ? g_help->nlist : 0;
}

int help_at(void)
{
    return g_help_at;
}

void help_page(int page)
{
    Piece *first;
    int i, j, k, y;
    if (!g_help || page < 0 || page >= g_help->nlist)
        return;
    help_stack();
    if (g_help->list[page]->nlines < 1)
        return;
    first = &g_help->list[page]->line[0]->p[0];
    y = (int)first->y;
    for (i = 0; i < g_help->nlist; i++)
        for (j = 0; j < g_help->list[i]->nlines; j++) {
            Line *l = g_help->list[i]->line[j];
            for (k = 0; k < l->n; k++) {
                l->p[k].y -= y;
                text_place(l->p[k].t, (int)l->p[k].x, (int)l->p[k].y);
            }
        }
    g_help_at = page;
}

void help_draw(void)
{
    int i;
    for (i = 0; g_help && i < g_help->nlist; i++)
        block_draw(g_help->list[i], 0);
}
