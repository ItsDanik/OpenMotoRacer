/* The race's dialogs (NOTES §4.19): a dialog is rows of text objects, row 0 the title,
   the others buttons with a normal item (flying in) and a highlighted one (blinking).
   Each button gets a letter of its text as a hotkey, the arrows move between them,
   Return presses; the box behind them grows from its centre and darkens the race. */
#include "dialog.h"
#include "clock.h"
#include "config.h"
#include "exe.h"
#include "font.h"
#include "menu.h"
#include "pointer.h"
#include "race.h"
#include "rules.h"
#include "scores.h"
#include "choices.h"
#include "sound.h"
#include "text.h"
#include "video.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROWS 8
#define SCREEN_W 640
#define SCREEN_H 480

typedef struct {                /* 0x6c bytes */
    Text *normal, *high;        /* +4, +0xc */
    Text *shown;                /* +0 */
    int is_static;              /* +0x58: the title */
    unsigned char hot[4];       /* +0x68: [0] the letter found each frame */
    void (*action)(void);       /* +0x20, +0x2c */
} Row;

typedef struct {                /* 0x78 bytes, 0x4338b0 */
    int kind;                   /* +0x00: 4 results, 5 continue, 6 pause */
    int keyboard;               /* +0x0c */
    int hold;                   /* +0x10 */
    int hud;                    /* +0x14: the dashboard's other form */
    int done;                   /* +0x20 */
    int frames;                 /* +0x60: drawn frames, the box grows over 15 */
    int box[4];                 /* +0x68 */
    Row row[ROWS];              /* +0x58 */
    int nrows;
    int sel;                    /* +0x54: -1 none */
} Dialog;

static Dialog *g_dlg;           /* the slot 0x4e8904 */
static int g_ctrl, g_shift;     /* 0x4dc108, 0x4dc10c */
static int g_left_held, g_right_held;   /* 0x4e897c, 0x4e8980 */

int dialog_open(void)
{
    return g_dlg != 0;
}

int dialog_holds(void)
{
    return g_dlg && g_dlg->hold;
}

void dialog_free(void)
{
    int i;
    if (!g_dlg)
        return;
    for (i = 0; i < g_dlg->nrows; i++) {
        text_free(g_dlg->row[i].normal);
        text_free(g_dlg->row[i].high);
    }
    free(g_dlg);
    g_dlg = 0;
}

/* 0x4354e0: every row's text slides off to the left, faster and faster, fading */
static void close_dialog(void)
{
    int i;
    sound_play(0x28, 0);
    for (i = 0; i < g_dlg->nrows; i++) {
        Text *t = g_dlg->row[i].shown;
        TextArgs a;
        if (!t)
            continue;
        text_args_of(t, &a);
        a.lifetime = 10000;
        a.align = 0x21;
        a.effect = FX_FADE_OUT;
        a.motion = 2;
        a.vx = -1;
        a.s = 1.0;
        a.k = 2.0;
        text_add(&a);
    }
    dialog_free();
}

/* ------------------------------------------------------------------ building */

static Dialog *new_dialog(int kind)
{
    dialog_free();              /* (the old one goes without its effects) */
    g_dlg = calloc(1, sizeof *g_dlg);
    if (!g_dlg)
        return 0;
    g_dlg->kind = kind;
    g_dlg->keyboard = g_dlg->hold = 1;
    g_dlg->sel = -1;
    return g_dlg;
}

/* an item as 0x433a30's layout leaves it: a font with a low / high resolution partner
   has its authored places halved (small fonts or not 640x480) */
static Text *item(const char *s, int font, int x, int y, int tx, int ty, int align,
                  int effect, int motion, int blink, int vx, int vy, double sp, double k,
                  const unsigned char *fore, const unsigned char *back)
{
    TextArgs a;
    memset(&a, 0, sizeof a);
    if (font_partnered(font) && (g_small_fonts || g_screen_mode != 2)) {
        x /= 2, y /= 2, tx /= 2, ty /= 2;
    }
    a.text = s;
    a.font = font;
    a.x = x;
    a.y = y;
    a.tx = tx;
    a.ty = ty;
    a.align = align;
    a.effect = effect;
    a.motion = motion;
    a.blink = blink;
    a.vx = vx;
    a.vy = vy;
    a.s = sp;
    a.k = k;
    if (fore) {
        a.own_colours = 1;
        a.fore = fore;
        a.back = back;
    }
    return text_new(&a);
}

static const unsigned char GREEN[3] = { 19, 188, 5 }, YELLOW[3] = { 255, 255, 0 };

static void title(const char *text, int x, int y, int tx, int ty, int vx, int vy, double k)
{
    Row *r = &g_dlg->row[g_dlg->nrows++];
    r->normal = r->shown = item(text, 9, x, y, tx, ty, 0xc2, FX_TRANSP25, 1, 0,
                                vx, vy, 4.0, k, GREEN, YELLOW);
    r->is_static = 1;
}

/* a button in font 32: flying in (or still) and its blinking twin at the target */
static void button(int text, int x, int y, int tx, int ty, int align, int motion, int blink,
                   int vx, int vy, int escape, void (*action)(void))
{
    Row *r = &g_dlg->row[g_dlg->nrows++];
    const char *s = exe_text(text);
    r->normal = r->shown = item(s, 32, x, y, tx, ty, align, FX_NONE, motion, blink, vx, vy,
                                4.0, 6.0, 0, 0);
    r->high = item(s, 32, tx, ty, tx, ty, align, FX_BLINK, 0, 2, 0, 0, 0, 0, 0, 0);
    if (escape)
        r->hot[1] = VK_ESCAPE;
    r->action = action;
}

/* 0x436150: a row chosen: its items at their places, the highlighted one shown */
static void enter(int i)
{
    Row *r = &g_dlg->row[i];
    if (r->normal)
        text_snap(r->normal);
    if (r->high)
        text_snap(r->high);
    r->shown = r->high ? r->high : r->normal;
    sound_play(0x29, 0);
}

static void select_row(int i)   /* the old one shows its normal item again (0x4361c0) */
{
    if (g_dlg->sel >= 0 && g_dlg->sel != i)
        g_dlg->row[g_dlg->sel].shown = g_dlg->row[g_dlg->sel].normal;
    g_dlg->sel = i;
}

/* a button standing at its place (the results' and the continue's) */
static void still(int text, int x, int y, int align, int escape, void (*action)(void))
{
    Row *r = &g_dlg->row[g_dlg->nrows++];
    const char *s = exe_text(text);
    r->normal = r->shown = item(s, 32, x, y, x, y, align, FX_NONE, 0, -500, 0, 0, 0, 0, 0, 0);
    r->high = item(s, 32, x, y, x, y, align, FX_BLINK, 0, 2, 0, 0, 0, 0, 0, 0);
    if (escape)
        r->hot[1] = VK_ESCAPE;
    r->action = action;
}

/* ------------------------------------------------------------------ the pause menu */

static void act_restart(void)   /* 0x437610 */
{
    g_dlg->done = 1;
    sound_pause_all(0);
    sound_stop_all();
    if (g_race_kind == 1)
        rules_restore_results();
    if (g_solo)
        scores_write_race();
    race_event(EV_NORMAL | EV_RESTART);
}

static void act_quit(void)      /* 0x437890 */
{
    g_dlg->done = 1;
    sound_pause_all(0);         /* 0x424090(0), then everything stopped (0x424020) */
    sound_stop_all();
    if (g_race_kind == 1)
        rules_restore_results();
    if (g_solo)
        scores_write_race();
    if (g_mode == MODE_CHAMPIONSHIP)
        g_racer[race_local()].state = RS_OUT;
    race_event(EV_NORMAL | EV_UNLOAD | EV_TOP5);
}

static void act_next_demo(void) /* 0x437680 */
{
    g_dlg->done = 1;
    sound_pause_all(0);
    race_event(EV_DEMO_NEXT | EV_DEMO_LOAD);
}

static void act_quit_demo(void) /* 0x4376d0 */
{
    g_dlg->done = 1;
    sound_pause_all(0);
    sound_stop_all();
    race_event(EV_UNLOAD | EV_CLOSE_DEMOS | EV_TITLE);
}

/* 0x437760: a championship race's replay left, on to what the race itself led to */
static void act_quit_replay(void)
{
    int me = race_local();
    g_dlg->done = 1;
    sound_pause_all(0);
    rules_restore_results();
    if (g_solo)
        scores_write_race();
    if (g_racer[me].state == RS_FINISHED && g_racer[me].place <= 2)
        race_event(EV_NORMAL | EV_UNLOAD | EV_TOP5);
    else
        race_event(EV_NORMAL | EV_CONTINUE);
}

static void act_continue(void)  /* 0x437ae0: the keys let go */
{
    g_dlg->done = 1;
    race_release_keys();
    g_ctrl = g_shift = 0;
    sound_pause_all(0);         /* 0x424090(0): they go on */
}

void dialog_pause(int arg)
{
    if (!new_dialog(6))
        return;
    sound_pause_all(1);         /* 0x424090(1): the looping sounds paused */
    sound_play(3, 0);
    g_dlg->hud = arg != 1;
    title("MOTO RACER", 0, 200, 0, 5, 0, -1, 5.0);      /* (0x4e8aec, not a word number) */
    if (g_playback) {
        button(107, 300, 0, 0, 0, 0x62, 1, -400, -1, 0, 0, act_next_demo);
        button(106, -300, 22, 0, 22, 0x62, 1, -500, 1, 0, 0, act_quit_demo);
    } else {
        button(104, 300, 0, 0, 0, 0x62, 1, -400, -1, 0, 0, act_restart);
        button(105, -300, 22, 0, 22, 0x62, 1, -500, 1, 0, 0, act_quit);
    }
    if (arg == 1)
        button(g_race_kind == 0 ? 110 : g_playback ? 108 : 109, 0, 200, 0, 44, 0x62, 1, -600,
               0, 1, 1, act_continue);
    if (arg == 1 && g_race_kind == 1 && !g_playback && g_mode == MODE_CHAMPIONSHIP)
        button(111, 0, -200, 0, 66, 0x62, 1, -700, 0, -1, 0, act_quit_replay);
    g_dlg->sel = 1;
    enter(1);
}

/* ------------------------------------------------------------------ the race's end */

/* 0x438690: no replay; the table written; a championship's top three go on, the others
   may try again; after a single race the pause menu without CONTINUE */
static void act_results_no(void)
{
    int me = race_local(), place;
    g_dlg->done = 1;
    rules_restore_results();
    if (g_solo)
        scores_write_race();
    if (g_mode == MODE_CHAMPIONSHIP) {
        place = g_racer[me].state == RS_FINISHED ? g_racer[me].place : rules_place_of(me);
        race_event(EV_NORMAL | (place > 2 ? EV_CONTINUE : EV_UNLOAD | EV_TOP5));
    } else
        race_event(EV_NORMAL | EV_PAUSE_AFTER);
}

static void act_results_yes(void)       /* 0x438650: the race again from its recording */
{
    g_dlg->done = 1;
    race_event(EV_WATCH | EV_RESTART);
}

/* 0x437b70 */
void dialog_results(void)
{
    if (!new_dialog(4))
        return;
    sound_stop_all();
    sound_play(3, 0);
    if (g_race_kind != 1)
        rules_save_results();
    g_dlg->hud = 1;
    title(exe_text(100), -300, 5, 0, 5, 1, 0, 2.0);    /* [REPLAY] */
    still(101, 10, 0, 0x6a, 0, act_results_yes);
    still(102, 10, 0, 0x63, 1, act_results_no);
    /* ([SAVE] 114 only with the original's -saverecord switch, and its file dialog: not
       written, the game itself has no way of playing such a file back) */
    g_dlg->sel = 1;
    enter(1);
}

static void act_again(void)     /* 0x436210: the same race again */
{
    g_dlg->done = 1;
    race_event(EV_NORMAL | EV_RESTART);
}

static void act_give_up(void)   /* 0x436250 */
{
    g_dlg->done = 1;
    race_event(EV_NORMAL | EV_UNLOAD | (g_mode == MODE_SINGLE ? EV_PAGE6 : EV_TOP5));
}

/* 0x4358b0: practice's end, the time over, a championship race not won */
void dialog_continue(void)
{
    if (!new_dialog(5))
        return;
    sound_stop_all();
    sound_play(0x44, 0);        /* TRYAGAIN */
    g_dlg->hud = 1;
    title(exe_text(99), -300, 5, 0, 5, 1, 0, 4.0);     /* [CONTINUE] */
    still(101, 10, 0, 0x6a, 0, act_again);
    still(102, 10, 0, 0x63, 0, act_give_up);
    g_dlg->sel = 1;
    enter(1);
}

/* ------------------------------------------------------------------ each frame */

/* 0x434860: the box round the rows' shown items (where the flying ones are going) */
static void layout(void)
{
    int i, *b = g_dlg->box;
    b[0] = b[1] = 0xffff;
    b[2] = b[3] = 0;
    for (i = 0; i < g_dlg->nrows; i++) {
        int r[4];
        Text *t = g_dlg->row[i].shown;
        if (!t || !text_alive(t))
            continue;
        text_rect(t, r);
        if (r[0] < b[0]) b[0] = r[0];
        if (r[1] < b[1]) b[1] = r[1];
        if (r[2] > b[2]) b[2] = r[2];
        if (r[3] > b[3]) b[3] = r[3];
    }
}

/* 0x43e0f0 + 0x434960: each button's hotkey, the first letter of its text that is not
   a control key's (nor one an earlier row took) */
void dialog_frame(void)
{
    unsigned char used[64];
    int n = 0, a, i;
    if (!g_dlg)
        return;
    for (a = 0; a < ACTIONS; a++)
        used[n++] = (unsigned char)toupper(g_binding[a][0] & 0xff);
    for (i = 0x70; i <= 0x77; i++)
        used[n++] = (unsigned char)i;
    used[n++] = 9;
    for (i = 0; i < g_dlg->nrows; i++) {
        Row *r = &g_dlg->row[i];
        const unsigned char *s;
        if (r->is_static || !r->shown || !r->normal)
            continue;
        r->hot[0] = 0;
        for (s = (const unsigned char *)text_string(r->normal); *s; s++) {
            int c = toupper(*s), k;
            if (!isalpha(*s))
                continue;
            for (k = 0; k < n && used[k] != c; k++)
                ;
            if (k == n) {
                r->hot[0] = (unsigned char)c;
                if (n < 64)
                    used[n++] = (unsigned char)c;
                break;
            }
        }
    }
    layout();
}

/* ------------------------------------------------------------------ keys */

/* 0x435090: the nearest row that way (no wrapping round) */
static int nearest(int vk)
{
    int from[4], i, best = -1;
    unsigned bestkey = 0;
    if (g_dlg->sel >= 0 && text_alive(g_dlg->row[g_dlg->sel].shown))
        text_now(g_dlg->row[g_dlg->sel].shown, from);
    else {
        from[0] = SCREEN_W - 1, from[1] = SCREEN_H - 1;
        from[2] = from[3] = 0;
    }
    for (i = 0; i < g_dlg->nrows; i++) {
        Row *r = &g_dlg->row[i];
        int b[4];
        unsigned key;
        if (r->is_static || !r->shown || !text_alive(r->shown))
            continue;
        text_now(r->shown, b);
        switch (vk) {
        case VK_LEFT:
            if (b[2] >= from[0])
                continue;
            key = (unsigned)b[2] << 16 | (unsigned)abs((from[1] + from[3]) / 2 - (b[1] + b[3]) / 2) << 8;
            break;
        case VK_RIGHT:
            if (b[0] <= from[2])
                continue;
            key = (unsigned)b[0] << 16 | (unsigned)abs((from[1] + from[3]) / 2 - (b[1] + b[3]) / 2) << 8;
            break;
        case VK_UP:
            if (b[3] >= from[1])
                continue;
            key = (unsigned)b[3] << 16 | (unsigned)abs((from[0] + from[2]) / 2 - (b[0] + b[2]) / 2) << 8;
            break;
        default:
            if (b[1] <= from[3])
                continue;
            key = (unsigned)b[1] << 16 | (unsigned)abs((from[0] + from[2]) / 2 - (b[0] + b[2]) / 2) << 8;
            break;
        }
        key |= (unsigned)i;
        /* left and up: the largest first (0x4353d0); right and down: the smallest */
        if (best < 0 || (vk == VK_LEFT || vk == VK_UP ? key > bestkey : key < bestkey))
            best = i, bestkey = key;
    }
    return best;
}

int dialog_key(int vk, int down)
{
    Dialog *d = g_dlg;
    int i, k, taken = 0;
    if (!d)
        return 0;
    if (!down) {
        if (vk == VK_CONTROL || vk == VK_SHIFT) {
            *(vk == VK_CONTROL ? &g_ctrl : &g_shift) = 0;
            return 1;
        }
        return d->keyboard;
    }
    if (vk == VK_CONTROL || vk == VK_SHIFT) {
        *(vk == VK_CONTROL ? &g_ctrl : &g_shift) = 1;
        return 1;
    }
    if (g_ctrl)
        return vk == 'F';
    if (g_shift || (vk >= 0x71 && vk <= 0x73) || vk == 0x6a || vk == 0x6f)
        return 0;
    for (i = 0; i < d->nrows && !taken; i++)
        for (k = 0; k < 4; k++)
            if (!d->row[i].is_static && d->row[i].hot[k] && d->row[i].hot[k] == vk) {
                select_row(i);
                if (d->row[i].action)
                    d->row[i].action();
                taken = 1;
                break;
            }
    if (!taken && d->keyboard) {
        if (vk == VK_LEFT || vk == VK_RIGHT || vk == VK_UP || vk == VK_DOWN) {
            int r = nearest(vk);
            if (r >= 0) {
                select_row(r);
                enter(r);
            }
        } else if (vk == VK_RETURN && d->sel >= 0 && d->row[d->sel].action)
            d->row[d->sel].action();
    }
    if (g_dlg && g_dlg->done)
        close_dialog();
    return taken || (g_dlg ? g_dlg->keyboard : 1);
}

/* ------------------------------------------------------------------ the mouse */

/* 0x435430: the first row whose shown item, where it is now, has the point */
static int row_at(int x, int y)
{
    int i, b[4];
    for (i = 0; i < g_dlg->nrows; i++) {
        const Row *r = &g_dlg->row[i];
        if (r->is_static || !r->shown || !text_alive(r->shown))
            continue;
        text_now(r->shown, b);
        if (b[0] <= x && x <= b[2] && b[1] <= y && y <= b[3])
            return i;
    }
    return -1;
}

/* 0x433ec0, 0x200: hovering chooses a row, and off the rows none is chosen. Not while a
   button is held. */
static void mouse_move(int x, int y)
{
    int r;
    if (g_left_held || g_right_held)
        return;
    r = row_at(x, y);
    if (r != g_dlg->sel) {
        select_row(r);
        if (r >= 0)
            enter(r);
    }
}

/* 0x433ec0, 0x201 and 0x202: a press belongs to the row under the button going down and
   happens as it comes up, wherever the pointer is by then */
static void mouse_left(int x, int y, int down)
{
    if (down) {
        g_left_held = 1;
        select_row(row_at(x, y));
    } else {
        g_left_held = 0;
        if (g_dlg->sel >= 0 && g_dlg->row[g_dlg->sel].action)
            g_dlg->row[g_dlg->sel].action();
        if (g_dlg->done)
            close_dialog();
    }
}

void dialog_mouse_move(int x, int y)
{
    pointer_touched(x, y);
    if (g_dlg)
        mouse_move(x, y);
}

void dialog_mouse_button(int left, int down)
{
    if (!g_dlg)
        return;
    if (left)
        mouse_left(g_mx, g_my, down);   /* (the place the last move left it) */
    else
        g_right_held = down;            /* (no row answers it: +0x28 is 0) */
}

/* the pointer's shape after a message (0x4329a5, 0x4e8960): the row's, or "no" off the rows */
static void set_pointer(void)
{
    if (!g_dlg)
        pointer_shape(CUR_ARROW);
    else
        pointer_shape(g_dlg->sel >= 0 ? CUR_ROW : CUR_NO);
}

/* ------------------------------------------------------------------ drawing */

/* 0x49a5a0: the inside at half brightness, a 5-pixel bevel from 230/256 to 128/256 */
static uint16_t scale(uint16_t p, int s)
{
    return (uint16_t)((((p & 0xf800u) * (unsigned)s >> 8) & 0xf800u)
                      | (((p & 0x07e0u) * (unsigned)s >> 8) & 0x07e0u)
                      | (((p & 0x001fu) * (unsigned)s >> 8) & 0x001fu));
}

void dialog_shade(int x0, int y0, int x1, int y1)
{
    const int E = 5;
    int x, y;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > g_scr_w - 1) x1 = g_scr_w - 1;
    if (y1 > g_scr_h - 1) y1 = g_scr_h - 1;
    if (x1 <= x0 || y1 <= y0)
        return;
    for (y = y0; y <= y1; y++) {
        uint16_t *p = g_screen + y * g_pitch;
        int t = -1;
        if (y < y0 + E - 1)
            t = y0 - y - 1 + E;
        else if (y > y1 - E + 1)
            t = y - y1 - 1 + E;
        for (x = x0; x <= x1; x++) {
            int s;
            if (x < x0 + E && (t < 0 || x < x0 + E - t))
                s = ((x0 - x + E - 1) * 256 / E + 256) >> 1;
            else if (x > x1 - E && (t < 0 || x > x1 - E + t))
                s = ((x - x1 + E - 1) * 256 / E + 256) >> 1;
            else if (t >= 0)
                s = (t * 256 / E + 256) >> 1;
            else {
                p[x] = (uint16_t)((p[x] >> 1) & ~0x0410u);
                continue;
            }
            p[x] = scale(p[x], s);
        }
    }
}

/* 0x434ae0 */
void dialog_draw(void)
{
    Dialog *d = g_dlg;
    int i, b[4];
    set_pointer();
    if (!d)
        return;
    d->frames += g_ticks;
    /* (+0x1c: the race's message list cleared, the box reserved in it) */
    layout();
    memcpy(b, d->box, sizeof b);
    if (d->frames < 15) {
        double f = d->frames * (1.0 / 15);
        int hw = (b[2] - b[0]) / 2, hh = (b[3] - b[1]) / 2;
        int cx = (b[0] + b[2]) / 2, cy = (b[1] + b[3]) / 2;
        b[0] = (int)(cx - hw * f);
        b[2] = (int)(cx + hw * f);
        b[1] = (int)(cy - hh * f);
        b[3] = (int)(cy + hh * f);
    }
    dialog_shade(b[0] - 5, b[1] - 5, b[2] + 5, b[3] + 5);
    for (i = 0; i < d->nrows; i++) {
        Row *r = &d->row[i];
        char *s;
        if (!r->shown)
            continue;
        if (!r->is_static) {    /* capitals, the hotkey in the font's small letters */
            int lowered = 0;
            for (s = text_string(r->shown); *s; s++) {
                if (*s >= 'a' && *s <= 'z')
                    *s = (char)(*s - 'a' + 'A');
                if (!lowered && *s >= 'A' && *s <= 'Z'
                    && memchr(r->hot, (unsigned char)*s, 4)) {
                    *s = (char)(*s - 'A' + 'a');
                    lowered = 1;
                }
            }
        }
        text_draw_one(r->shown);
    }
}
