/* Page 9, TOP 5 (start 0x44cfd0, frame 0x44d540, end 0x44d510), the Solo half: the best
   races and the best lap of the chosen track, laps and options, from Data\bestscor.dat
   (NOTES §4.12.5). The race's end marks a new record by the name 'TINI' and the page then
   takes the player's three letters and saves the table: that comes with the race. */
#include "choices.h"
#include "exe.h"
#include "font.h"
#include "menu.h"
#include "pages.h"
#include "r3d.h"
#include "scores.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define PIC(id) (&g_pics[id])
enum {
    P_TILE = 1, P_TITLE = 0x12, P_BEST_LAP = 0x13, P_BEST_RACE = 0x14, P_CONTINUE = 0x4d,
    P_SAVE = 0x4e, P_ABORT = 0x50, P_NEXT = 0x52, P_NUMBER = 0x60, P_LINE = 0x68,
    P_RESET = 0xc9
};

static int g_slid;              /* 0x542380: the second slides have started */
static int g_blink;             /* 0x4e9114: 0..4, the new names' cursor shows on 0..2 */

void page_top5_start(void)
{
    static const unsigned char white[3] = { 255, 255, 255 };
    g_slid = 0;
    memset(&g_best_lap, 0x7f, sizeof g_best_lap);
    font_set_colors(font_get(12), white, white);
    scores_load();
    bitmap_place(PIC(P_TITLE), 0, -80, 0);
    bitmap_slide(PIC(P_TITLE), 0, 0, 1.0f, 1, 0);
    bitmap_place(PIC(P_BEST_RACE), -150, 135, 0);
    bitmap_place(PIC(P_BEST_LAP), 30, 400, 0);
    menu_clear_buttons();
    if (g_mode == MODE_CHAMPIONSHIP) {
        bitmap_place(PIC(P_NEXT), 840, 437, 0);
        bitmap_place(PIC(P_ABORT), 640, 437, 0);
        bitmap_place(PIC(P_SAVE), 640, 437, 0);
        if (g_champ_race < g_nchamp)
            menu_add_button(PIC(P_NEXT), 0, 0xffffff01u, -1, -1, 0, 0, 0, -1, -1, 0);
        menu_add_button(PIC(P_ABORT), 0, 0xffff0002u, VK_ESCAPE, -1, 0, 0, 0, -1, -1, 0);
        if (g_champ_race < g_nchamp)
            menu_add_button(PIC(P_SAVE), 0, 0xffff01ffu, -1, -1, 0, 0, 0, -1, -1, 0);
    } else {
        bitmap_place(PIC(P_CONTINUE), 840, 429, 0);
        bitmap_place(PIC(P_RESET), 640, 429, 0);
        menu_add_button(PIC(P_CONTINUE), 0, 0xffffff01u, -1, -1, 0, 0, 0, -1, -1, 0);
        /* forgetting the time-attack ghost: in practice, at no more than the track's laps */
        if (g_mode == MODE_PRACTICE && g_laps <= g_tracks[g_track].laps)
            menu_add_button(PIC(P_RESET), 0, 0xffff00ffu, -1, -1, 0, 0, 0, -1, -1, 0);
    }
    best_lap_load(&g_best_lap);
    g_champ_shown = 1;
}

void page_top5_end(void)
{
    /* (0x44d510 saves a new record's name as far as it was typed: with the race) */
}

/* "MM'SS''HH" */
static void format_time(char *out, size_t size, unsigned ms)
{
    int t = (int)ms, s = t / 1000;
    snprintf(out, size, "%02d'%02d''%02d", s / 60 % 60, s % 60, t % 1000 / 10);
}

static void draw_text(int font, const char *text, int x, int y)
{
    font_draw(font_get(font), text, x, y, 0, 0);      /* 0x493ee0 */
}

static void draw_table(void)
{
    static const unsigned char yellow[3] = { 255, 255, 0 }, green[3] = { 0, 139, 89 };
    const Track *t = &g_tracks[g_track];
    char s[96], *p;
    int i, y;

    /* the track's name in the title bar, which it follows down */
    snprintf(s, sizeof s, "%s", t->name);
    for (p = s; *p; p++)
        if (*p == '_')
            *p = ' ';
    font_set_colors(font_get(31), named_colour(COL_BLACK), named_colour(COL_YELLOW));
    draw_text(31, s, PIC(P_TITLE)->x + 450, PIC(P_TITLE)->y + 56);

    /* "2 LAPS REVERSE POCKET" */
    snprintf(s, sizeof s, "%d %s ", g_laps, exe_text(g_laps == 1 ? 17 : 18));
    if (g_reverse)
        strncat(s, "Reverse", sizeof s - strlen(s) - 1);
    if (g_reverse && g_pocket)
        strncat(s, " ", sizeof s - strlen(s) - 1);
    if (g_pocket)
        strncat(s, "Pocket", sizeof s - strlen(s) - 1);
    for (p = s; *p; p++)
        *p = (char)toupper((unsigned char)*p);
    font_set_rgb(font_get(31), yellow, green);
    draw_text(31, s, 33, 103);

    if (++g_blink == 5)
        g_blink = 0;
    for (i = 0, y = 182; i < 5; i++, y += 44) {
        const Score *r = &g_scores[i];
        if (score_lap(r) == SCORE_EMPTY)
            continue;
        bitmap_place(PIC(P_LINE), 30, y - 12, 0);
        bitmap_draw(PIC(P_LINE), BM_OWN, 100, 0);
        bitmap_place(PIC(P_NUMBER + i), 28, y - 14, 0);
        bitmap_draw(PIC(P_NUMBER + i), BM_OWN, 100, 0);
        draw_text(12, score_name(r), 110, y);
        format_time(s, sizeof s, score_time(r));
        draw_text(12, s, 480, y);
    }
    if (score_lap(&g_best_lap) != SCORE_EMPTY) {
        draw_text(9, score_name(&g_best_lap), 330, 400);
        format_time(s, sizeof s, score_lap(&g_best_lap));
        draw_text(9, s, 180, 400);
    }
}

void page_top5_frame(void)
{
    int pressed;
    /* once the title bar is down, "Best RACE" and the buttons come in */
    if (!PIC(P_TITLE)->sliding && !g_slid) {
        bitmap_slide(PIC(P_BEST_RACE), 430, 135, 0.7f, 1, 0);
        g_slid = 1;
        if (g_mode == MODE_CHAMPIONSHIP) {
            bitmap_slide(PIC(P_NEXT), 496, 437, 0.7f, 1, 0);
            bitmap_slide(PIC(P_ABORT), 296, 437, 0.7f, 1, 0);
            bitmap_slide(PIC(P_SAVE), 150, 437, 0.7f, 1, 0);
        } else {
            bitmap_slide(PIC(P_CONTINUE), 496, 429, 0.7f, 1, 0);
            bitmap_slide(PIC(P_RESET), 296, 429, 0.7f, 1, 0);
        }
    }
    menu_begin_frame();
    bitmap_step(PIC(P_TILE), -1, 1);
    bitmap_draw(PIC(P_TILE), BM_OWN, 100, 0);
    bitmap_step(PIC(P_TITLE), 0, 0);
    bitmap_draw(PIC(P_TITLE), BM_OWN, 100, 0);
    bitmap_step(PIC(P_BEST_RACE), 0, 0);
    bitmap_draw(PIC(P_BEST_RACE), BM_OWN, 100, 0);
    bitmap_step(PIC(P_BEST_LAP), 0, 0);
    bitmap_draw(PIC(P_BEST_LAP), BM_OWN, 100, 0);
    draw_table();
    menu_draw_buttons();
    menu_transition();
    r3d_render();
    menu_end_frame();

    pressed = menu_pressed();
    if (pressed < 0 || pressed > 2)
        return;
    menu_press_done();
    menu_capture();
    if (pressed == 0) {
        if (g_mode != MODE_CHAMPIONSHIP)
            menu_change_page(6);
        else
            menu_change_page(g_champ_on ? 6 : 5);
    } else if (pressed == 1) {
        if (g_mode == MODE_CHAMPIONSHIP)
            menu_change_page(3);        /* Abort */
        else {
            /* Reset deletes the track's time-attack ghost (<track>\timatak[R_][_]<laps>.bin):
               the ghost comes with the race, and where the port keeps it is not decided */
            menu_change_page(6);
        }
    } else
        menu_change_page(21);           /* Save the championship */
}
