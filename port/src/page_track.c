/* Page 6, SELECT RACE / CHAMPIONSHIP (start 0x447a30, frame 0x448110, end 0x4480d0): the
   track's map and photo, the arrows that change the track, the laps, Practice's laps -/+
   and its time-attack switch. The buttons are made again every frame, as the mode wants
   them. */
#include "video.h"
#include "choices.h"
#include "menu.h"
#include "r3d.h"
#include "sound.h"
#include <stdio.h>
#include <stdlib.h>

#define PIC(id) (&g_pics[id])
enum {
    P_TILE = 0, P_TITLE = 0x19, P_LEFT = 0x1a, P_LEFT_FLASH = 0x1b, P_RIGHT = 0x1c,
    P_RIGHT_FLASH = 0x1d, P_NUM0 = 0x1e, P_MINUS = 0x28, P_MINUS_HL = 0x29,
    P_MINUS_FLASH = 0x2a, P_PLUS = 0x2b, P_PLUS_HL = 0x2c, P_PLUS_FLASH = 0x2d, P_LAPS = 0x2e,
    P_CONTINUE = 0x4d, P_LOAD = 0x4f, P_BACK = 0x51, P_HISCORES = 0x56, P_CHAMP_TITLE = 0x73,
    P_TIMEATTACK = 0xab, P_ATTACK_ON = 0xac, P_ATTACK_OFF = 0xad, P_ATTACK_FLASH = 0xae,
    P_MAPTXT = 0xbb
};

typedef struct {
    Bitmap map, photo;          /* 0x4e90ec + i * 0xe8, + 0x74 */
} TrackPics;

static TrackPics *g_tp;
static int *g_unloaded;         /* the track record's +0: its pictures are not loaded */
static int g_shown;             /* 0x5409ac: the track whose pictures came in last */

/* data\ileNN\map.lez and photo.lez, colour 0 left out */
static void load_track(int i)
{
    char path[0x120];
    snprintf(path, sizeof path, "%smap.lez", g_tracks[i].folder);
    if (!bitmap_load_lez(&g_tp[i].map, path, BM_KEYED))
        fprintf(stderr, "openmotoracer: %s could not be read\n", path);
    snprintf(path, sizeof path, "%sphoto.lez", g_tracks[i].folder);
    if (!bitmap_load_lez(&g_tp[i].photo, path, BM_KEYED))
        fprintf(stderr, "openmotoracer: %s could not be read\n", path);
}

void page_track_start(void)
{
    int i;
    g_tp = calloc((size_t)g_ntracks, sizeof *g_tp);
    g_unloaded = calloc((size_t)g_ntracks, sizeof *g_unloaded);
    if (!g_tp || !g_unloaded) {
        video_error("Aborting game");
        exit(1);
    }
    for (i = 0; i < g_ntracks; i++)
        g_unloaded[i] = 1;
    track_legal(&g_track);
    if (g_mode == MODE_CHAMPIONSHIP) {
        g_reverse = g_champ[g_champ_race].reverse;
        g_pocket = g_champ[g_champ_race].pocket;
    }
    load_track(g_track);
    bitmap_place(&g_tp[g_track].map, -200, 133, 0);
    bitmap_place(&g_tp[g_track].photo, 740, 91, 0);
    g_unloaded[g_track] = 0;
    g_shown = g_track;

    bitmap_place(PIC(P_TITLE), 0, -100, 0);
    bitmap_place(PIC(P_CHAMP_TITLE), 0, -100, 0);
    bitmap_place(PIC(P_MINUS), 266, 580, 0);
    bitmap_place(PIC(P_PLUS), 328, 580, 0);
    bitmap_place(PIC(P_LAPS), 740, 351, 0);
    bitmap_place(PIC(P_LEFT), -100, 148, 0);
    bitmap_place(PIC(P_RIGHT), 740, 146, 0);
    bitmap_place(PIC(P_BACK), 740, 438, 0);
    bitmap_place(PIC(P_CONTINUE), 740, 356, 0);
    bitmap_place(PIC(P_HISCORES), 740, 397, 0);
    bitmap_place(PIC(P_LOAD), 740, 397, 0);
    bitmap_place(PIC(P_TIMEATTACK), 342, 128, 0);
    bitmap_place(PIC(P_ATTACK_ON), 395, 393, 0);
    bitmap_place(PIC(P_ATTACK_OFF), 395, 393, 0);
    bitmap_slide(PIC(P_TITLE), 0, 0, 1.0f, 1, 0);
    bitmap_slide(PIC(P_CHAMP_TITLE), 0, 0, 1.0f, 1, 0);
    bitmap_slide(&g_tp[g_track].map, 78, 133, 1.0f, 1, 0);
    bitmap_slide(&g_tp[g_track].photo, 320, 91, 1.0f, 1, 0);
    bitmap_slide(PIC(P_MINUS), 266, 404, 1.0f, 1, 0);
    bitmap_slide(PIC(P_PLUS), 328, 404, 1.0f, 1, 0);
    bitmap_slide(PIC(P_LAPS), 412, 351, 1.0f, 1, 0);
    bitmap_slide(PIC(P_LEFT), 6, 148, 0.7f, 1, 0);
    bitmap_slide(PIC(P_RIGHT), 578, 146, 0.7f, 1, 0);
    bitmap_slide(PIC(P_BACK), 516, 438, 0.7f, 1, 0);
    bitmap_slide(PIC(P_CONTINUE), 514, 356, 0.7f, 1, 0);
    bitmap_slide(PIC(P_HISCORES), 514, 397, 0.7f, 1, 0);
    bitmap_slide(PIC(P_LOAD), 514, 397, 0.7f, 1, 0);
    music_play(3, 3);
    if (g_mode != MODE_CHAMPIONSHIP)
        sound_queue(1, 600, 0x53);      /* "select race" */
}

void page_track_end(void)
{
    int i;
    for (i = 0; i < g_ntracks && g_tp; i++) {
        bitmap_free(&g_tp[i].map);
        bitmap_free(&g_tp[i].photo);
    }
    free(g_tp);
    free(g_unloaded);
    g_tp = 0;
    g_unloaded = 0;
}

/* a picture drawn for this frame only where another one is (the arrows' and -/+ flashes) */
static void flash(int id, const Bitmap *at)
{
    bitmap_place(PIC(id), at->x, at->y, 0);
    bitmap_draw(PIC(id), BM_OWN, 100, 0);
}

static void flash_xy(int id, int x, int y)
{
    bitmap_place(PIC(id), x, y, 0);
    bitmap_draw(PIC(id), BM_OWN, 100, 0);
}

/* Left and Right: on round the list to the next usable track, its laps */
static void turn(int dir)
{
    int start = g_track;
    do {
        g_track += dir;
        if (g_track < 0)
            g_track = g_ntracks - 1;
        else if (g_track >= g_ntracks)
            g_track = 0;
    } while (!track_usable(g_track) && g_track != start);
    g_laps = g_tracks[g_track].laps;
}

void page_track_frame(void)
{
    int b_back, b_continue, b_left = -2, b_right = -2, b_hiscores = -2, b_load = -2;
    int b_minus = -2, b_plus = -2, b_attack = -2, leaving = 0, still = 1, pressed, i;
    Track *t = &g_tracks[g_track];
    menu_begin_frame();
    if (g_solo && g_mode != MODE_PRACTICE)
        g_laps = t->laps;       /* only Practice chooses its laps */

    menu_clear_buttons();
    b_back = 0;
    b_continue = 1;
    if (g_mode == MODE_CHAMPIONSHIP) {
        menu_add_button(PIC(P_BACK), 0, 0x02ffffffu, VK_ESCAPE, -1, 0, 0, 0, -1, -1, 0);
        menu_add_button(PIC(P_CONTINUE), 0, 0xff02ffffu, -1, -1, 0, 0, 0, -1, -1, 0);
        menu_add_button(PIC(P_LOAD), 0, 0x0100ffffu, -1, -1, 0, 0, 0, -1, -1, 0);
        b_load = 2;
    } else {
        /* Left and Right are the arrows' keys: they turn the tracks */
        menu_add_button(PIC(P_BACK), 0, 0x04ffffffu, VK_ESCAPE, -1, 0, 0, 0, -1, -1, 0);
        menu_add_button(PIC(P_CONTINUE), 0, 0xff04ffffu, VK_RETURN, -1, 0, 0, 0, -1, -1, 0);
        menu_add_button(PIC(P_LEFT), 0, 0xff01ffffu, VK_LEFT, -1, 0, 0, 0, -1, -1, 0);
        menu_add_button(PIC(P_RIGHT), 0, 0xff01ffffu, VK_RIGHT, -1, 0, 0, 0, -1, -1, 0);
        menu_add_button(PIC(P_HISCORES), 0, 0x0100ffffu, -1, -1, 0, 0, 0, -1, -1, 0);
        b_left = 2;
        b_right = 3;
        b_hiscores = 4;
        if (g_mode == MODE_PRACTICE) {
            menu_add_button(PIC(P_MINUS), PIC(P_MINUS_HL), 0x01010101u, VK_SUBTRACT, -1, 0, 0, 0, -1, -1, 0);
            menu_add_button(PIC(P_PLUS), PIC(P_PLUS_HL), 0x01010101u, VK_ADD, -1, 0, 0, 0, -1, -1, 0);
            b_minus = 5;
            b_plus = 6;
            /* time attack only up to the track's own laps */
            if (t->laps < g_laps)
                g_time_attack = 0;
            else {
                Bitmap *p = PIC(g_time_attack ? P_ATTACK_ON : P_ATTACK_OFF);
                menu_add_button(p, p, 0xffffffffu, -1, -1, 0, 0, 0, -1, -1, 0);
                b_attack = 7;
            }
        }
    }

    bitmap_step(PIC(P_TILE), 1, -1);
    bitmap_draw(PIC(P_TILE), BM_OWN, 100, 0);
    {
        Bitmap *title = PIC(g_mode == MODE_CHAMPIONSHIP ? P_CHAMP_TITLE : P_TITLE);
        bitmap_step(title, 0, 0);
        bitmap_draw(title, BM_OWN, 100, 0);
    }

    /* a new track's pictures come in once nothing is moving (a turn made meanwhile waits:
       its map sits at (-200, 90), a few pixels of it on the screen, as in the game) */
    for (i = 0; i < g_ntracks; i++)
        if (!g_unloaded[i] && (g_tp[i].map.sliding || g_tp[i].photo.sliding))
            still = 0;
    if (g_unloaded[g_track]) {
        load_track(g_track);
        bitmap_place(&g_tp[g_track].map, -200, 90, 0);
        bitmap_place(&g_tp[g_track].photo, 740, 91, 0);
        g_unloaded[g_track] = 0;
    }
    if (still && g_shown != g_track) {
        bitmap_place(&g_tp[g_track].map, -200, 133, 0);
        bitmap_slide(&g_tp[g_track].map, 78, 133, 0.5f, 1, 0);
        bitmap_place(&g_tp[g_track].photo, 740, 91, 0);
        bitmap_slide(&g_tp[g_track].photo, 320, 91, 0.5f, 1, 0);
        bitmap_slide(&g_tp[g_shown].map, 78, -400, 0.5f, 1, 0);
        bitmap_slide(&g_tp[g_shown].photo, 320, 580, 0.5f, 1, 0);
        g_shown = g_track;
    }
    for (i = 0; i < g_ntracks; i++) {
        TrackPics *p = &g_tp[i];
        if ((p->map.sliding || i == g_track) && !g_unloaded[i]) {
            bitmap_step(&p->map, 0, 0);
            bitmap_draw(&p->map, BM_OWN, 100, 0);
            flash_xy(P_MAPTXT, p->map.x, p->map.y - 43);        /* "MAP" over it */
        }
        if ((p->photo.sliding || i == g_track) && !g_unloaded[i]) {
            bitmap_step(&p->photo, 0, 0);
            bitmap_draw(&p->photo, BM_OWN, 100, 0);
            if (g_time_attack && g_mode == MODE_PRACTICE)
                flash_xy(P_TIMEATTACK, p->photo.x + 22, p->photo.y + 37);
        }
    }
    /* the laps, two digits */
    flash_xy(P_NUM0 + g_laps % 10, 330, 334);
    flash_xy(P_NUM0 + g_laps / 10 % 10, 275, 334);
    bitmap_step(PIC(P_LAPS), 0, 0);
    bitmap_draw(PIC(P_LAPS), BM_OWN, 100, 0);
    menu_draw_buttons();

    /* what was pressed is done in the middle of the frame: the flashes are drawn over the
       buttons, and a new page starts at the next frame */
    pressed = menu_pressed();
    if (pressed != -1) {
        if (pressed == b_left) {
            menu_press_done();
            turn(-1);
            flash_xy(P_LEFT_FLASH, 6, 148);
            sound_play(0x28, 0);
        } else if (pressed == b_right) {
            menu_press_done();
            turn(1);
            flash_xy(P_RIGHT_FLASH, 578, 146);
            sound_play(0x28, 0);
        } else if (pressed == b_minus) {
            menu_press_done();
            if (g_laps > 1) {
                g_laps--;
                flash(P_MINUS_FLASH, PIC(P_MINUS));
            }
        } else if (pressed == b_plus) {
            menu_press_done();
            if (g_laps < 8) {
                g_laps++;
                flash(P_PLUS_FLASH, PIC(P_PLUS));
            }
        } else if (pressed == b_back) {
            menu_press_done();
            leaving = 1;
            menu_change_page(g_mode == MODE_CHAMPIONSHIP && g_champ_shown ? 9 : 5);
        } else if (pressed == b_continue) {
            menu_press_done();
            if (track_usable(g_track)) {
                leaving = 1;
                menu_change_page(10);
            }
        } else if (pressed == b_hiscores) {
            menu_press_done();
            leaving = 1;
            menu_change_page(9);
        } else if (pressed == b_load) {
            menu_press_done();
            leaving = 1;
            menu_change_page(20);
        } else if (pressed == b_attack) {
            flash(P_ATTACK_FLASH, PIC(P_ATTACK_OFF));
            menu_press_done();
            g_time_attack ^= 1;
        }
    }
    menu_transition();
    r3d_render();
    menu_end_frame();
    if (leaving)
        menu_capture();         /* the glass takes this finished frame */
}
