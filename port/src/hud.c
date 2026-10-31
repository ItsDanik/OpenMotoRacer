/* The race's dashboard (NOTES §4.20 §3): what 0x4806e0 draws each race picture. The
   panels are drawn straight onto the screen in 640-pixel units (halved with the small
   fonts, the default); the countdown and the messages are text objects; the dials (§3b)
   are a model of the track's and two needles put into the 3D's depth lists. Not yet: the
   gear-change hint of the digits-only dashboard, the split against the leader (0x412e50), the riders' labels (Link and
   time attack). GAME OVER, and the messages on the CD's data (0x4dc180 = 0), are sprite
   objects (0x482120, 0x482030, §3.5). */
#include "hud.h"
#include "ftol.h"
#include "bikephys.h"
#include "bikes.h"
#include "choices.h"
#include "clock.h"
#include "config.h"
#include "exe.h"
#include "font.h"
#include "land.h"
#include "r3d.h"
#include "menu.h"
#include "race.h"
#include "rules.h"
#include "sound.h"
#include "sprite.h"
#include "text.h"
#include "video.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

enum { F_LOW_CHRONO = 4, F_TIME = 7, F_PLAYER = 6, F_SPRITES = 11, F_GAMEMSG = 14,
       F_POS = 20, F_MESSAGES = 12, F_FINALPOS = 25 };      /* 0x4816c0's (hi-res numbers) */

static int g_frames;            /* 0x4f0dc4: race pictures (here 60ths of a second) */
static int g_time_blink;        /* 0x4f0e1c */
static int g_laps_mode = -1;    /* 0x4f0e18 */
static int g_last_count = -1;   /* 0x542854: the countdown number shown */
static int g_go;                /* 0x4dbe44: GO still to show, its lifetime */
static int g_last_cp;           /* the race object's +4: the checkpoint last told */
static int g_loaded;            /* 0x4dc100 */
static int id_laps, id_count, id_ready, id_finish, id_timeout, id_final, id_wrong, id_check;
static int id_gameover;         /* 0x4e987c */

static uint32_t ubits(float f)
{
    uint32_t u;
    memcpy(&u, &f, 4);
    return u;
}

static int halving(void)        /* the small fonts, or not 640x480 */
{
    return g_small_fonts || g_screen_mode != 2;
}

/* 0x493ee0 / 0x493e60 (the place halved when halving) and 0x493f20 / 0x493f60 (as
   given); r, if asked, gets the block's place on the screen */
static void put(int font, const char *s, int x, int y, int flags, int noscale, int r[4])
{
    Font *f = font_get(font);
    int bx, by, w, h;
    if (!f)
        return;
    if (!noscale && halving())
        x /= 2, y /= 2;
    bx = x, by = y;
    if (flags != 0x21)
        font_place(f, s, &bx, &by, flags, &w, &h);
    else
        font_measure(f, s, &w, &h, 0);
    if (r)
        r[0] = bx, r[1] = by, r[2] = bx + w, r[3] = by + h;
    font_draw(f, s, x, y, flags, 0);
}

static void colours(int font, int fore, int back)       /* the font's slot 0 */
{
    Font *f = font_get(font);
    if (f)
        font_set_colors(f, named_colour(fore), back < 0 ? 0 : named_colour(back));
}

static void clock_text(char *b, int n, int ms, const char *fmt)    /* 0x481730's split */
{
    int s = ms / 1000;
    snprintf(b, (size_t)n, fmt, s / 60 % 60, s % 60, ms % 1000 / 10);
}

static int g_seconds;           /* 0x4f0d8c: the seconds of the last clock split: the time left
                                   (0x481730), then the lap times of the panel (0x483af9) */

static int id_replay;           /* 0x4e9898 */

static int me(void)
{
    return race_player();
}

/* the rider's name (+0x2dd4): the player's, the others "COMPUTER" (text 183, 0x410a50) */
static void rider_name(int k, char out[9])
{
    const char *s = g_racer[k].human ? g_player_name : exe_text(183);
    int i;
    for (i = 0; i < 8 && s[i]; i++)
        out[i] = g_racer[k].human ? s[i]
                 : (char)(s[i] >= 'a' && s[i] <= 'z' ? s[i] - 'a' + 'A' : s[i]);
    out[i] = 0;
}

/* ------------------------------------------------------------------ text objects */

/* 0x4188b0: the object holding *id replaced, red on black, still */
static void say(int *id, const char *s, int life, int align, int x, int y, int effect,
                int font, int blink)
{
    TextArgs a;
    text_remove(*id);
    if (life == 0 && (effect == FX_FADE_IN || effect == FX_FADE_OUT))
        effect = FX_NONE;
    memset(&a, 0, sizeof a);
    a.text = s;
    a.lifetime = life;
    a.font = font;
    a.x = x;
    a.y = y;
    a.align = align;
    a.effect = effect;
    a.blink = blink;
    a.id_out = id;
    a.own_colours = 1;
    a.fore = named_colour(COL_RED);
    a.back = named_colour(COL_BLACK);
    a.list = 1;         /* 0x53ba68 */
    a.dodge_x = a.dodge_y = 5;
    a.centre_if_full = 1;
    text_add(&a);
}

/* 0x481af0, 0x481c50, 0x481db0, 0x481f10: a picture character of font 11 blinking in the
   middle, then drifting off to the right */
static void message(int *id, int c, int life, int blink, int fore)
{
    TextArgs a;
    char s[2] = { (char)c, 0 };
    if (text_find(*id))
        return;
    memset(&a, 0, sizeof a);
    a.text = s;
    a.lifetime = life;
    a.font = F_SPRITES;
    a.align = 0x42;
    a.effect = FX_BLINK;
    a.motion = 4;
    a.blink = blink;
    a.id_out = id;
    a.vx = 2;
    a.ty = 0x5a;
    a.s = 2.0;
    a.k = 5.0;
    a.own_colours = 1;
    a.fore = named_colour(fore);
    a.back = named_colour(COL_BLACK);
    a.list = 1;         /* 0x53ba68 */
    text_add(&a);
}

/* 0x480930's sprites (§3.5): the pictures of banks 0x5d3cb2, 0x5d3b69 and 0x5d3dfb, loaded
   once. The messages' only on the CD's data: on GOG's, those are font 11's characters. */
static SpritePic pic_hires, pic_lowres, pic_gameover;
static Sprite spr_over1, spr_over2, spr_over_lr;        /* 0x4eaa94, 0x4eaa98, 0x4eaac4 */
enum { MSG_TIME_OUT, MSG_FINAL_LAP, MSG_WRONG_WAY, MSG_CHECK, MSGS };
static Sprite spr_msg_lo[MSGS], spr_msg_hi[MSGS];
static int g_msg_pictures;      /* 0x4dc180 == 0 */

static void load_sprites(void)
{
    static int done;
    static const Sprite over1 = { &pic_gameover, 0, 89, 255, 255, 0, 0, 1 },
        over2 = { &pic_hires, 0, 98, 113, 247, 0, 9, 1 },
        over_v2 = { &pic_lowres, 0, 15, 255, 127, 0, 0, 1 },
        over_cd = { &pic_lowres, 0, 143, 255, 255, 0, 0, 1 },
        /* 0x4eaacc, 0x4eaac8, 0x4eaadc, 0x4eaae0 */
        lo[MSGS] = { { &pic_lowres, 91, 40, 176, 47, 0, 0, 1 },
                     { &pic_lowres, 0, 24, 98, 31, 0, 0, 1 },
                     { &pic_lowres, 0, 8, 103, 15, 0, 0, 1 },
                     { &pic_lowres, 0, 32, 124, 39, 0, 0, 1 } },
        /* 0x4eaaa4, 0x4eaa9c, 0x4eaab4, 0x4eaab8 */
        hi[MSGS] = { { &pic_hires, 85, 28, 237, 41, 0, 0, 1 },
                     { &pic_hires, 0, 56, 168, 69, 0, 0, 1 },
                     { &pic_hires, 0, 14, 187, 27, 0, 0, 1 },
                     { &pic_hires, 0, 70, 224, 83, 0, 0, 1 } };
    if (done)
        return;
    done = 1;
    sprite_pic_load(&pic_hires, "Data\\Design\\Texts\\HiRes.RAW");
    sprite_pic_load(&pic_lowres, "Data\\Design\\Texts\\LowRes.RAW");
    sprite_pic_load(&pic_gameover, "Data\\Design\\Texts\\GameOver.RAW");
    spr_over1 = over1;
    spr_over2 = over2;
    /* 0x4dc180 (0x472c40): GOG's LowRes.RAW is 128 high and holds only "Game over" */
    g_msg_pictures = pic_lowres.h != 128;
    spr_over_lr = g_msg_pictures ? over_cd : over_v2;
    memcpy(spr_msg_lo, lo, sizeof lo);
    memcpy(spr_msg_hi, hi, sizeof hi);
    /* The CD's three pictures are 24-bit with a green ground, and its own MOTO.EXE keys them
       green (0x4e77ac). GOG's program left this form's key at black: it never meets these. */
    if (g_msg_pictures) {
        int i;
        spr_over1.key = spr_over2.key = spr_over_lr.key = SPRITE_KEY_GREEN;
        for (i = 0; i < MSGS; i++)
            spr_msg_lo[i].key = spr_msg_hi[i].key = SPRITE_KEY_GREEN;
    }
}

/* 0x481af0, 0x481c50, 0x481db0, 0x482030 on the CD's data: a picture in the middle, blinking,
   going where it stands. */
static void picture_message(int *id, int which, int life, int blink)
{
    TextArgs a;
    if (text_find(*id))
        return;
    memset(&a, 0, sizeof a);
    a.lifetime = life;
    a.align = 0x42;
    a.effect = FX_BLINK;
    a.blink = blink;
    a.id_out = id;
    a.list = 1;         /* 0x53ba68 */
    a.dodge_x = a.dodge_y = 5;
    a.centre_if_full = 1;
    a.parts = 1;
    a.lo[0] = &spr_msg_lo[which];
    a.hi[0] = &spr_msg_hi[which];
    text_add(&a);
}

/* the blink differs between the two forms for TIME OUT only: 1 against 2 */
static void show(int *id, int which, int c, int life, int blink, int picture_blink, int fore)
{
    load_sprites();
    if (g_msg_pictures)
        picture_message(id, which, life, picture_blink);
    else
        message(id, c, life, blink, fore);
}

void hud_message(int which)
{
    switch (which) {
    case HUD_TIME_OUT:
        show(&id_timeout, MSG_TIME_OUT, 'c', 2000, 2, 1, COL_RED);
        break;
    case HUD_FINAL_LAP:
        text_remove(id_check);
        show(&id_final, MSG_FINAL_LAP, 'a', 1000, 4, 4, COL_YELLOW);
        break;
    case HUD_WRONG_WAY:
        show(&id_wrong, MSG_WRONG_WAY, '`', 4000, 4, 4, COL_RED);
        break;
    case HUD_WRONG_WAY_GONE:
        text_remove(id_wrong);
        break;
    }
}

/* 0x482120(0): GAME OVER in the middle for 3 s, then drifting off to the right. It
   also removes TIME OUT's and WRONG WAY's ids from 0x53b8e8, but those live in 0x53ba68,
   so they stay (and GAME OVER, in the list drawn after, covers them). */
void hud_game_over(void)
{
    TextArgs a;
    load_sprites();
    memset(&a, 0, sizeof a);
    a.lifetime = 3000;
    a.align = 0x42;
    a.motion = 4;
    a.id_out = &id_gameover;
    a.dodge_x = a.dodge_y = 5;
    a.centre_if_full = 1;
    a.vx = 2;
    a.s = 2.0;
    a.k = 5.0;
    a.parts = 2;
    a.lo[0] = &spr_over_lr;
    a.hi[0] = &spr_over1;
    a.hi[1] = &spr_over2;
    text_add(&a);
}

/* 0x482220: "3 LAPS", steady, then see-through blinking, then falling away */
static void laps_text(int m)
{
    static char s[16];
    TextArgs a;
    if (m != g_laps_mode) {
        g_laps_mode = m;
        text_remove(id_laps);
    }
    if (text_find(id_laps))
        return;
    snprintf(s, sizeof s, g_laps < 2 ? "%d LAP" : "%d LAPS", g_laps);
    memset(&a, 0, sizeof a);
    a.text = s;
    a.font = F_GAMEMSG;
    a.y = 0x14;
    a.align = 0x22;
    if (m == 1) {
        a.effect = FX_BLINK_TRANSP;
        a.blink = -1000;
    } else if (m == 2) {
        a.effect = FX_FADE_OUT;
        a.motion = 2;
        a.lifetime = 4000;
    }
    a.id_out = &id_laps;
    a.dodge_x = a.dodge_y = 5;
    a.centre_if_full = 1;
    a.vy = 1;
    a.k = 0.2;
    a.own_colours = 1;
    a.fore = named_colour(COL_RED);
    a.back = named_colour(COL_BLACK);
    a.list = 1;         /* 0x53ba68 */
    text_add(&a);
}

/* 0x4082f0: a beep and "3", "2", "1", or "GO" */
static void count_voice(int c)
{
    static const int bip[4] = { 2, 1, 1, 1 }, voice[4] = { 0x33, 0x32, 0x31, 0x30 };
    if (c < 0 || c > 3)
        return;
    sound_play(bip[c], 0);
    sound_play(voice[c], 0);
}

/* ------------------------------------------------------------------ panels */

/* 0x482830: BESTLAP, top right: this race's fastest lap, the track's record, the record
   just made (blinking); mode 1 the record only */
static void best_panel(int mode)
{
    int y = 0x1b, step, header = 0, i;
    Font *f6 = font_raw(F_PLAYER);
    step = (f6 ? f6->height : 0) + 4;
    for (i = 0; i < 3; i++) {
        int c = mode != 0 || !g_solo ? mode : i == 0 ? 0 : i == 1 ? 1 : 4;
        int t = NO_TIME, shift = 0, blink = 0, r[4], tw, th, x, n, j;
        char name[9], bar[24], s[24];
        if (c == 0) {
            int k = g_fastest;
            if (k > 0 && g_racer[k].best != NO_TIME) {
                t = g_racer[k].best;
                rider_name(k, name);
                shift = g_racer[k].human ? 0 : 0x20;
            }
        } else if (c == 1) {
            const unsigned char *rec = rules_record();
            memcpy(&t, rec + 0xf5, 4);
            memcpy(name, rec, 8);
            name[8] = 0;
            shift = 0x40;
        } else if (g_race_best_lap != NO_TIME) {        /* the entry 0x5d25b0 */
            t = g_race_best_lap;
            memcpy(name, g_player_name, 8);
            name[8] = 0;
            shift = 0x40;
            blink = 4;
        }
        if (t != NO_TIME) {
            if (!header) {
                colours(F_SPRITES, COL_WHITE, -1);
                put(F_SPRITES, "C", 0, 5, 8, 0, 0);
            }
            header = 1;
            if (!(blink & g_frames)) {
                n = (int)strlen(name);
                strcpy(bar, "[");
                for (j = n; j < 8; j++)
                    strcat(bar, "@");
                strcat(bar, name);
                for (j = 0; bar[j]; j++)
                    bar[j] = (char)(bar[j] + shift);
                put(F_PLAYER, bar, 0, y - 2, 8, 0, r);
                clock_text(s, sizeof s, t, "%02d:%02d;%02d ");
                font_measure(font_get(F_LOW_CHRONO), s, &tw, &th, 0);
                x = r[0] - tw - 5;
                if (halving())
                    x *= 2;
                put(F_LOW_CHRONO, s, x, y, 0x21, 0, 0);
                y += step;
            }
        }
        if (mode != 0 || !g_solo)
            break;
    }
}

/* 0x483860: LAPTIME, top left: lap n of N, the last three laps' times, the lap numbers */
static void lap_panel(void)
{
    const Racer *r = &g_racer[me()];
    int done = r->laps, finished = done == g_laps, first = 0, last = done, fastest = 0;
    int record = -1, rect[4], i, y;
    char s[24];
    colours(F_SPRITES, COL_WHITE, -1);
    put(F_SPRITES, "B", 2, 5, 0x21, 0, rect);
    if (finished)
        for (i = 0; i < g_laps && i < 8; i++) {
            if (r->lap[i] == g_race_best_lap && g_race_best_lap != NO_TIME)
                record = i;     /* (and the record's name the player's) */
            if (r->lap[i] < r->lap[fastest])
                fastest = i;
        }
    else
        first = done - 2;
    if (!((finished ? 8 : 0) & g_frames)) {
        snprintf(s, sizeof s, "%d<%d", done + 1 < g_laps ? done + 1 : g_laps, g_laps);
        put(F_LOW_CHRONO, s, rect[2] + 10, 5, 0x21, 1, 0);
    }
    if (first < 0)
        first = 0;
    if (last >= g_laps)
        last = g_laps - 1;
    for (i = first, y = 0x19; i <= last && i < 8; i++, y += 0x10) {
        Font *f = font_get(F_LOW_CHRONO);
        unsigned char fore[3];
        int green = g_laps > 1 && finished && i == fastest;
        clock_text(s, sizeof s, r->lap[i], "%02d:%02d;%02d");
        g_seconds = r->lap[i] / 1000 % 60;
        if (green && f) {
            memcpy(fore, f->fore, 3);
            font_set_colors(f, named_colour(COL_GREEN), 0);
        }
        put(F_LOW_CHRONO, s, 0x3c, y, 0x21, 0, 0);
        if (green && f)
            font_set_colors(f, fore, 0);
        if (finished && i == record && !(g_frames & 4))
            put(F_PLAYER, "[]", 0xc0, y, 0x21, 0, 0);
        if (!((i == done ? 4 : 0) & g_frames)) {
            snprintf(s, sizeof s, " %c%c]", (i + 1) / 10 ? '0' + (i + 1) / 10 : ' ',
                     '0' + (i + 1) % 10);
            put(F_PLAYER, s, 0, y, 0x21, 0, 0);
        }
    }
    /* the split against the reference (0x412e50) while the checkpoint's message lasts */
    if (g_solo && g_extended > 0 && g_split[0]) {
        Font *f = font_get(F_LOW_CHRONO);
        unsigned char fore[3];
        const unsigned char *col = named_colour(g_split[0] == ' ' ? COL_ORANGE
                                                : g_split[0] == '+' ? COL_RED : COL_GREEN);
        y += 0x10;
        if (f) {
            memcpy(fore, f->fore, 3);
            font_set_colors(f, col, 0);
        }
        put(F_LOW_CHRONO, g_split + 1, 0x3c, y, 0x21, 0, 0);
        if (f)
            font_set_colors(f, fore, 0);
        snprintf(s, sizeof s, "  %c]", g_split[0] == '+' ? '\\' : ' ');
        put(F_PLAYER, s, 0, y, 0x21, 0, 0);
    }
}

/* 0x482380: TIME left and the place, right side */
static void time_pos(void)
{
    int secs = g_time_left / 1000, mask, label_mask = -1, fore, label = COL_WHITE, place;
    int r1[4], r2[4], x;
    char s[16];
    if (g_time_left < 0)
        strcpy(s, "00");
    else
        snprintf(s, sizeof s, "%02d", secs);
    if (g_extended >= 4000) {           /* a checkpoint just passed */
        fore = label = COL_YELLOW;
        mask = -1;
        label_mask = 2;
    } else if (g_time_left < 0) {
        fore = label = COL_RED;
        mask = 1;
    } else if (secs < 3) {
        fore = label = COL_RED;
        mask = 2;
    } else if (secs < 10) {
        fore = COL_ORANGE;
        mask = 4;
    } else {
        fore = COL_GREEN;
        mask = -1;
    }
    colours(F_TIME, fore, COL_BLACK);
    g_time_blink += g_ticks;
    if (g_time_blink & mask)
        put(F_TIME, s, 8, 0x73, 8, 0, 0);
    if (g_time_blink & label_mask) {
        colours(F_SPRITES, label, -1);
        put(F_SPRITES, "D", 8, 0x5a, 8, 0, 0);
    }
    place = rules_place_of(me()) + 1;
    if (place > 0) {
        colours(F_SPRITES, COL_WHITE, -1);
        put(F_SPRITES, "A", 0, 0x96, 8, 0, r1);
        snprintf(s, sizeof s, "<%d", g_loaded);
        put(F_LOW_CHRONO, s, 8, 0xab, 8, 0, r2);
        x = r1[2] - r1[0] > r2[2] - r2[0] ? r1[2] - r1[0] : r2[2] - r2[0];
        x++;
        if (halving())
            x *= 2;
        snprintf(s, sizeof s, "%d", place);
        put(F_POS, s, x + 8, 0x96, 8, 0, 0);
    }
}

/* 0x482620: practice's place (the large POS) */
static void practice_pos(void)
{
    int place, r[4], x;
    char s[16];
    if (g_racer[me()].state == RS_GRID || g_loaded < 2)
        return;
    place = rules_place_of(me()) + 1;
    if (place <= 0)
        return;
    colours(F_SPRITES, COL_WHITE, -1);
    put(F_SPRITES, "E", 8, 5, 8, 0, 0);
    snprintf(s, sizeof s, "<%d", g_loaded);
    put(F_LOW_CHRONO, s, 8, 0x34, 8, 0, r);
    x = r[2] - r[0];
    if (halving())
        x *= 2;
    snprintf(s, sizeof s, "%d", place);
    put(F_POS, s, x + 9, 0x1e, 8, 0, 0);
}

/* 0x481970: "1ST POSITION" in the middle once the player is over the line */
static void finish_place(void)
{
    static char s[8];
    const Racer *r = &g_racer[me()];
    TextArgs a;
    int p;
    if ((g_mode == MODE_PRACTICE && g_time_attack && !g_racer[2].loaded) || r->state == RS_GRID
        || r->time <= 0 || text_find(id_finish))
        return;
    p = r->state == RS_FINISHED ? r->place : rules_place_of(me());
    if (p < 0)
        return;
    snprintf(s, sizeof s, "%d%c/", p + 1, p == 0 ? ':' : p == 1 ? ';' : p == 2 ? '<' : '=');
    memset(&a, 0, sizeof a);
    a.text = s;
    a.font = F_FINALPOS;
    a.y = 5;
    a.align = 0xc2;
    a.id_out = &id_finish;
    a.dodge_x = a.dodge_y = 5;
    a.centre_if_full = 1;
    a.list = 1;         /* 0x53ba68 */
    text_add(&a);
}

/* ------------------------------------------------------------------ the dials (§3b) */

typedef struct {                /* 0x485be0's: the centre, start angle, radians a unit, radius */
    int cx, cy;
    double a1, k, radius;
} Dial;

static struct {
    int speed, rev, gear;       /* the shown bike's O+0x118, the rev needle's target, O+0x7c */
    int rev_shown;              /* 0x4f0e10 */
    int hint;                   /* the gear-change hint (hud_bike) */
    int built, type, small;     /* 0x4f0e34 / 0x4f0e30 / 0x4f0e40: the layout's settings */
    int bx, by;                 /* 0x5dbc80, 0x5dbc84 */
    int sx[3], sy[3], rx[3], ry[3], dx, dy, gx, gy;
    Dial speed_dial, rev_dial;
    Face needle[6];
} g_dash;

static double wrap_angle(double a)
{
    const double two_pi = 6.283185307179586;            /* 0x4d1a48 */
    if (a < 0)
        a += two_pi;
    if (!(a < two_pi))
        a -= two_pi;
    return a;
}

/* 0x485be0 (dir 0: clockwise from p1 to p2) */
static void dial(Dial *d, int cx, int cy, int p1x, int p1y, int p2x, int p2y, double vmax)
{
    double a1, a2;
    d->cx = cx;
    d->cy = cy;
    d->radius = sqrt((double)(p1x - cx) * (p1x - cx) + (double)(p1y - cy) * (p1y - cy));
    a1 = wrap_angle(atan2((double)(cy - p1y), (double)(p1x - cx)));
    a2 = wrap_angle(atan2((double)(cy - p2y), (double)(p2x - cx)));
    if (a2 > a1)
        a2 -= 6.283185307179586;
    d->a1 = a1;
    d->k = (a2 - a1) / vmax;
}

/* 0x484810's layout: the low-res one with the small fonts or below 640x480 */
static void dash_layout(void)
{
    static const int lo[] = { 0xe3, 0x44, 0xdb, 0x63, 0xf9, 0x2a,  0xb7, 0x4f, 0xb1, 0x63,
                              0xb7, 0x39,  0x9a, 0x1e,  0xe2, 0x4f,  0x1f, 0x33,  0x66, 0x4a };
    static const int hi[] = { 0xcd, 0xbf, 0xbb, 0xfa, 0xf2, 0x8e,  0x77, 0xd3, 0x6c, 0xf7,
                              0x77, 0xaa,  0x3d, 0x74,  0xcf, 0xd2,  0x43, 0x6a,  0xc3, 0x8c };
    const int *t = halving() ? lo : hi;
    int i, ox = 0, oy = 0;
    if (g_counter_type == 1)
        ox = t[12], oy = t[13];         /* the needles' points from the origin */
    for (i = 0; i < 3; i++) {
        g_dash.sx[i] = t[2 * i] - ox, g_dash.sy[i] = t[2 * i + 1] - oy;
        g_dash.rx[i] = t[6 + 2 * i] - ox, g_dash.ry[i] = t[7 + 2 * i] - oy;
    }
    g_dash.dx = t[14] - ox, g_dash.dy = t[15] - oy;
    if (t == lo && behaviour_cd())
        g_dash.dy++;                    /* 0x4a64c5: 0x50 */
    g_dash.gx = t[16], g_dash.gy = t[17];
    g_dash.bx = g_scr_w - t[18];
    g_dash.by = g_scr_h - t[19];
    dial(&g_dash.speed_dial, g_dash.sx[0], g_dash.sy[0], g_dash.sx[1], g_dash.sy[1],
         g_dash.sx[2], g_dash.sy[2], 300);
    dial(&g_dash.rev_dial, g_dash.rx[0], g_dash.ry[0], g_dash.rx[1], g_dash.ry[1],
         g_dash.rx[2], g_dash.ry[2], 16000);
    g_dash.built = 1;
    g_dash.type = g_counter_type;
    g_dash.small = halving();
}

static uint16_t rgb(int r, int g, int b)        /* 0x45a2e0: green shifted by 6 */
{
    return (uint16_t)((r > 31 ? 31 : r) << 11 | (g > 63 ? 63 : g) << 6 | (b > 31 ? 31 : b));
}

/* a needle's corner: the centre plus (along, across) turned by a, then the base point */
static void corner(int xy[2], const Dial *d, double a, int along, int across)
{
    double co = cos(a), si = sin(a);
    xy[0] = d->cx + ftol(co * along - si * across) + g_dash.bx;
    xy[1] = d->cy - ftol(si * along + co * across) + g_dash.by;
}

static void needle(Face *f, const int (*xy)[2], int colour)
{
    memset(f, 0, sizeof *f);
    f->kind = 0;
    f->colour = colour;
    f->n = 3;
    f->idx[0] = 0, f->idx[1] = 1, f->idx[2] = 2;
    r3d_screen_faces(r3d_screen_points(3, xy, 2048.0f), 1, f, 4.0f);
}

/* a needle's black shadow, moved down and right and behind it. Both programs make it; GOG's
   gives its points the depth 1, behind the near plane, so only the 1997 CD's (0x4a6230, 2048)
   shows it */
static void needle_shadow(Face *f, const int (*xy)[2], int off)
{
    int s[3][2], i;
    if (!behaviour_cd())
        return;
    for (i = 0; i < 3; i++)
        s[i][0] = xy[i][0] + off, s[i][1] = xy[i][1] + off;
    memset(f, 0, sizeof *f);
    f->kind = 0;
    f->colour = 0;
    f->n = 3;
    f->idx[0] = 0, f->idx[1] = 1, f->idx[2] = 2;
    r3d_screen_faces(r3d_screen_points(3, (const int (*)[2])s, 2048.0f), 1, f, 8.0f);
}

/* 0x484810's reads: the speed O+0x118, the rev needle's target ftol(O+0x11c / bike+0x70
   x 16000), the gear O+0x7c; and the gear-change hint's state (0x484d97) */
void hud_bike(const BikePhys *b)
{
    float nb = g_bikes[b->bike].phys[(0x70 - 0x14) / 4];        /* NbSpeed */
    float g;
    g_dash.speed = b->kmh;
    g_dash.rev = (int)(b->rpm / (double)nb * 16000.0);
    g_dash.gear = b->gear;
    /* 0 none, 2 sliding or crashed, 3 shifting, 1 in the band, 4 change down, 5 up */
    g_dash.hint = 0;
    if (g_gearbox || g_counting)
        return;
    if (b->slide || b->crashed) {
        g_dash.hint = 2;
        return;
    }
    if (b->shift) {
        g_dash.hint = 3;
        return;
    }
    if ((int32_t)ubits(b->engine) <= 0 || b->gear <= 0)
        return;
    g = (float)b->gear;
    if (g > nb || (ubits(b->table[1]) & 0x7fffffff) == 0)
        return;
    if (nb > g && b->wheel + 2.0 > b->table[b->gear])
        g_dash.hint = 5;
    else if (b->gear > 1 && b->wheel - 2.0 < b->table[b->gear - 1] - 2.0)
        g_dash.hint = 4;
    else
        g_dash.hint = 1;
}

void hud_key(int vk)
{
    if (vk != 0x76)             /* F7 (0x432107; it needs 0x4e8944: the race's drawing) */
        return;
    if (g_show_counter && g_counter_type == 1)
        g_counter_type = 2;
    else if (g_show_counter && g_counter_type)
        g_counter_type = 0, g_show_counter = 0;
    else
        g_show_counter = 1, g_counter_type = 1;
}

/* 0x484810 calls (1) and (2), and 0x486310's slot 0: before the 3D is drawn */
void hud_dials_3d(void)
{
    int hi, xy[3][2], v, R, i;
    double a;
    const Dial *d;
    if (!g_show_counter)
        return;
    if (!g_dash.built || g_dash.type != g_counter_type || g_dash.small != halving())
        dash_layout();
    if (g_counter_type != 1)
        return;
    hi = !halving();
    /* the speed needle */
    d = &g_dash.speed_dial;
    v = g_dash.speed < 0 ? 0 : g_dash.speed > 300 ? 300 : g_dash.speed;
    a = v * d->k + d->a1;
    R = ftol(d->radius);
    {
        int tail[2], tip[2], minus[2], plus[2];
        corner(tail, d, a, 5 * (-1 - hi), 0);
        corner(tip, d, a, R, 0);
        corner(minus, d, a, 0, 3 * (-1 - hi));
        corner(plus, d, a, 0, 3 * hi + 3);
        memcpy(xy[0], tail, sizeof tail), memcpy(xy[1], tip, sizeof tip);
        memcpy(xy[2], minus, sizeof minus);
        needle(&g_dash.needle[0], (const int (*)[2])xy, rgb(0x19, 0x10, 0));
        needle_shadow(&g_dash.needle[3], (const int (*)[2])xy, 2 + hi);
        memcpy(xy[1], plus, sizeof plus), memcpy(xy[2], tip, sizeof tip);
        needle(&g_dash.needle[1], (const int (*)[2])xy, rgb(0x1f, 0x10, 0));
        needle_shadow(&g_dash.needle[4], (const int (*)[2])xy, 2 + hi);
    }
    /* the rev needle: towards its target by 3000 a picture at most */
    v = g_dash.rev;
    if (v > g_dash.rev_shown + 3000)
        v = g_dash.rev_shown + 3000;
    if (v < g_dash.rev_shown - 3000)
        v = g_dash.rev_shown - 3000;
    if (v < 0)
        v = 0;
    g_dash.rev_shown = v;
    d = &g_dash.rev_dial;
    a = (v > 16000 ? 16000 : v) * d->k + d->a1;
    R = ftol(d->radius);
    corner(xy[0], d, a, 2 * (-1 - hi), 2 * hi + 2);
    corner(xy[1], d, a, R, 0);
    corner(xy[2], d, a, 2 * (-1 - hi), 2 * (-1 - hi));
    needle(&g_dash.needle[2], (const int (*)[2])xy, rgb(0x1f, 0x10, 0));
    needle_shadow(&g_dash.needle[5], (const int (*)[2])xy, 2 + hi);
    /* the face: the track's model "counter" from the bottom right corner */
    for (i = 0; i < g_land.nlib; i++)
        if (strcmp(g_land.lib[i].name, "counter") == 0) {
            const Model *m = &g_land.lib[i].m[0];
            static int pts[512][2];
            double sx = hi ? 1.9 : 1.0, sy = hi ? 2.0 : 1.0;
            double ox = (g_scr_w - 1) - 0x5d * sx, oy = g_scr_h - 1;
            int k, n = m->nv < 512 ? m->nv : 512;
            for (k = 0; k < n; k++) {
                pts[k][0] = ftol(m->v[k][0] * sx + ox);
                pts[k][1] = ftol(oy - m->v[k][1] * sy);
            }
            r3d_screen_faces(r3d_screen_points(n, (const int (*)[2])pts, 2048.0f), m->nf,
                             m->f, 15.0f);
            break;
        }
}

/* 0x484810 call (4), after the 3D: the speed and the gear, or the digits alone */
static void dials_digits(void)
{
    int v = g_dash.speed;
    char s[16];
    if (!g_show_counter || !g_dash.built)
        return;
    if (g_speed_unit == 1)
        v = (int)(((long long)v * 0x3e800 / 0x649) >> 8);
    if (g_counter_type == 1) {
        snprintf(s, sizeof s, "%3d", v);
        font_draw(font_get(16), s, g_dash.bx + g_dash.dx, g_dash.by + g_dash.dy, 0x21, 0);
        s[0] = (char)(g_dash.gear + 0x30 + 7);
        s[1] = 0;
        font_draw(font_get(23), s, g_dash.bx + g_dash.gx, g_dash.by + g_dash.gy, 0x21, 0);
    } else if (g_counter_type) {
        /* 0x484d70: with the manual box, a blinking bar of font 11 near the top (change
           up) or the bottom (change down), as wide as an eighth of the screen allows */
        if ((g_dash.hint == 4 || g_dash.hint == 5) && !(g_frames & 4)) {
            Font *f = font_get(F_SPRITES);
            char bar[160];
            int n = f && f->width['W'] ? g_scr_w / 8 / f->width['W'] : 1;
            if (n < 1)
                n = 1;
            if (n > (int)sizeof bar - 1)
                n = (int)sizeof bar - 1;
            memset(bar, 'O' + (g_frames & 3), (size_t)n);
            bar[n] = 0;
            colours(F_SPRITES, COL_RED, -1);
            put(F_SPRITES, bar, 0, g_scr_h / 16, (g_dash.hint == 4 ? 0x80 : 0x20) | 2, 1, 0);
        }
        snprintf(s, sizeof s, "%d\n%c", v, g_speed_unit == 1 ? ';' : ':');
        font_draw(font_get(28), s, 6, 4, 0x90, 0);
    }
}

/* ------------------------------------------------------------------ the frame */

/* 0x413030 */
static void countdown(void)
{
    static char s[8];
    int sec = g_seconds, c;
    g_go = 500;
    laps_text(sec < 2 ? 2 : sec < 4 ? 1 : 0);
    if (g_solo)
        best_panel(1);
    c = sec / 2 + 1;
    if (c != g_last_count) {
        g_last_count = c;
        snprintf(s, sizeof s, "%02d", c);
        say(&id_count, s, 2000, 0x42, 0, 0, FX_FADE_OUT, F_GAMEMSG, 4);
        count_voice(c);
        if (c > 1)
            say(&id_ready, "GET READY", 0, 0x42, 0, 0x1e, FX_BLINK, F_MESSAGES, 4);
    }
}

/* 0x483670: over every other bike in view, its place, its name and an arrow down */
static void labels(void)
{
    Font *f = font_get(5);
    char s[32], name[9];
    int k, sx, sy, w, h;
    if (!f)
        return;
    for (k = 1; k < g_riders; k++) {
        if (k == me() || !g_racer[k].loaded || !race_label_place(k, &sx, &sy))
            continue;
        rider_name(k, name);
        snprintf(s, sizeof s, "%c^%s\\\n]", rules_place_of(k) + 0x29, name);
        font_measure(f, s, &w, &h, 1);
        /* (the colour 0x490a90 gives it does nothing here: fonts 1 and 5 use their own) */
        put(5, s, sx - f->width[']'] / 2, sy - f->height - h, 0x21, 1, NULL);
    }
}

/* 0x413180 */
static void racing(void)
{
    const Racer *r = &g_racer[me()];
    if (g_extended > 0 && r->cp != g_last_cp && r->state == RS_RACING
        && (r->cp != 1 || (!g_reverse ? r->laps != 0 : r->laps - g_laps != -1))) {
        g_last_cp = r->cp;
        sound_queue_voice(6, 500);
        if (g_mode == MODE_PRACTICE && !g_time_attack)
            show(&id_check, MSG_CHECK, 'b', 1000, 2, 2, COL_YELLOW);    /* 0x481f10 */
        else {
            text_remove(id_check);
            sound_queue_voice(0x34, 500);
        }
    }
    if (!g_solo || (g_mode == MODE_PRACTICE && g_time_attack))
        labels();
    if (r->state != RS_GRID && r->state != RS_RACING) {
        best_panel(0);
        return;
    }
    if (!g_solo || g_mode != MODE_PRACTICE || g_time_attack)
        time_pos();
    else
        practice_pos();
    best_panel(0);
    lap_panel();
    if (g_go > 0) {
        if (g_seconds / 2 != g_last_count) {    /* the lap's: 0 on the first picture */
            g_last_count = g_seconds / 2;
            say(&id_ready, "GO", g_go, 0x42, 0, 0xf, FX_FADE_OUT, F_GAMEMSG, 4);
            count_voice(0);
        }
        g_go = 0;
    }
}

/* the controller's name after Ctrl+J / Ctrl+K (0x431922, 0x431a0c), 3 s at the top;
   slot 0 the joystick's (0x4e98d8), 1 the keyboard's (0x4e98dc) */
void hud_sound_notice(const char *s, int trouble)
{
    static int id;
    if (trouble)                /* 0x4dbb20 with texts 134 and 135, blinking lower down */
        say(&id, s, 2000, 0x22, 0, 0x1e, FX_BLINK, 0x20, 4);
    else
        say(&id, s, 2000, 0x22, 0, behaviour_cd() ? 0 : 0x14, FX_FADE_OUT, F_MESSAGES, 4);
}

void hud_say(int *id, const char *s, int life, int align, int x, int y, int effect, int font,
             int blink)
{
    say(id, s, life, align, x, y, effect, font, blink);
}

void hud_notice(int slot, const char *s)
{
    static int id[2];
    say(&id[slot & 1], s, 3000, 0x22, 0, behaviour_cd() ? 0 : 0x14, FX_FADE_OUT, F_MESSAGES, 4);
}

void hud_screen(void)           /* 0x484810: its layout again for a new screen */
{
    g_dash.built = 0;
}

void hud_restart(void)
{
    g_laps_mode = g_last_count = -1;
    g_go = g_last_cp = 0;
    g_frames = g_time_blink = 0;
    g_dash.rev_shown = 0;
    g_dash.built = 0;
}

void hud_draw(int dials)
{
    int k;
    g_frames += g_ticks;
    g_seconds = g_time_left / 1000 % 60;        /* 0x4804e6 */
    if (dials)                  /* 0x47ff90's flags & 8 */
        dials_digits();
    for (g_loaded = 0, k = 1; k < g_riders; k++)        /* 0x4817a0 */
        g_loaded += g_racer[k].loaded;
    if (!g_race_over) {
        if (g_time_over)
            text_remove(id_finish);     /* (Link: the results, GAME OVER) */
        else if (g_counting)
            countdown();
        else
            racing();
    } else {
        if (g_solo)
            text_remove(id_gameover);           /* (and 0x4e9880) */
        best_panel(0);
        finish_place();     /* (unless the race's end shows its podium: not yet) */
        lap_panel();
    }
    /* 0x486660: the mark of a replay, top left */
    if (g_race_kind == 1 && !text_find(id_replay))
        say(&id_replay, "^", 2000, 0x81, 2, 2, FX_BLINK, F_SPRITES, -500);
}
