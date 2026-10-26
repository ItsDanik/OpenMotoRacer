/* Page 10, CHOOSE BIKE (start 0x44e480, frame 0x44ed30, end 0x44ecf0; NOTES §4.12.4):
   the eight bikes of the track's family, their four bars, the gearbox, Start and Back.
   The bike itself turns on a turntable, the camera going round it once in 10.24 s; on a
   change it whips off to one side and the next comes in from the other. Its drawing
   comes with the 3D bike; the turntable's state is kept here already. */
#include "bikes.h"
#include "choices.h"
#include "clock.h"
#include "menu.h"
#include "r3d.h"
#include "sound.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PIC(id) (&g_pics[id])
enum {
    P_LEFT = 0x1a, P_LEFT_FLASH = 0x1b, P_RIGHT = 0x1c, P_RIGHT_FLASH = 0x1d,
    P_BACKGROUND = 0x2f, P_TITLE = 0x30, P_FEATURES = 0x31, P_BAR = 0x32,
    P_AUTO = 0x36, P_AUTO_HL = 0x37, P_AUTO_DARK = 0x38, P_MANUAL = 0x39,
    P_MANUAL_HL = 0x3a, P_MANUAL_DARK = 0x3b, P_START = 0x4c, P_BACK = 0x51
};

static float g_target[3];       /* 0x53f540: where the camera looks, x y z */
static int g_dir;               /* 0x5420d0: the bike change under way, -1 left, 1 right */
static int g_shown_offset;      /* 0x5420dc: the bike drawn is the chosen one + this */
static int g_shown_bike;        /* 0x584715 / 0x584899 / 0x584a1d: (bike + offset) & 7 */
static float g_turn;            /* the camera's turn about the bike (camera +0x3c) */
static unsigned g_turn_last;    /* 0x4e9118 (not reset by the page) */
static int g_voice_played;      /* 0x53f554 */
static unsigned g_go_at;        /* 0x53f78c: Start waits until its sounds have been heard */

/* the four bars: what they show, from where to where they move */
static int g_bar_from[4];       /* 0x53f4c4, 0x540af8, 0x540990, 0x5420d8 */
static int g_bar_to[4];         /* 0x540cc4, 0x53f670, 0x53f4c8, 0x5420f8 */
static int g_bar_shown[4];      /* 0x53f4e4, 0x540078, 0x53f780, 0x53f54c */
static unsigned g_bar_t0, g_bar_t1;     /* 0x540cc0, 0x540cbc */
static int g_bar_running;       /* 0x53f84c */

/* a bar's length is its value: the part cut off on the right is 100 - value */
static void bar_targets(void)
{
    int i;
    for (i = 0; i < 4; i++)
        g_bar_to[i] = g_bikes[g_bike].gfx[i] * -100 / 100 + 100;
}

static void bars_restart(void)
{
    memcpy(g_bar_from, g_bar_to, sizeof g_bar_from);
    bar_targets();
    g_bar_t0 = g_now;
    g_bar_t1 = g_now + 1000;
    g_bar_running = 1;
}

/* the six buttons; A-T and M-T show the gearbox chosen (lit or dark) and blink when
   highlighted */
static void make_buttons(void)
{
    int a = g_gearbox == 1;
    menu_clear_buttons();
    menu_add_button(PIC(P_LEFT), 0, 0xff0201ffu, VK_LEFT, -1, 0, 0, 0, -1, -1, 0);
    menu_add_button(PIC(P_RIGHT), 0, 0xff02ff00u, VK_RIGHT, -1, 0, 0, 0, -1, -1, 0);
    menu_add_button(PIC(P_START), 0, 0x0103ff00u, VK_RETURN, -1, 0, 0, 0, -1, -1, 0);
    menu_add_button(PIC(P_BACK), 0, 0x02ffffffu, VK_ESCAPE, -1, 0, 0, 0, -1, -1, 0);
    menu_add_button(PIC(a ? P_AUTO : P_AUTO_DARK), PIC(P_AUTO_HL), 0xffffffffu, -1, -1, 0, 0, 1, -1, -1, 0);
    menu_add_button(PIC(a ? P_MANUAL_DARK : P_MANUAL), PIC(P_MANUAL_HL), 0xffffffffu, -1, -1, 0, 0, 1, -1, -1, 0);
}

void page_bike_start(void)
{
    int i, bike;
    if (g_bike == -1)
        g_bike = 0;
    memset(g_target, 0, sizeof g_target);
    g_voice_played = 0;
    g_go_at = 0xffffffffu;
    g_dir = 0;
    g_shown_offset = 0;
    bike = g_bike;
    if (!bikes_load(g_track))
        fprintf(stderr, "openmotoracer: the bikes of %s could not be read\n",
                g_tracks[g_track].name);
    g_bike = bike;
    g_turn = 0.0f;              /* the camera's angles (0, 0, 0), 1500 away */

    bitmap_place(PIC(P_TITLE), 0, -80, 0);
    bitmap_place(PIC(P_LEFT), -100, 148, 0);
    bitmap_place(PIC(P_RIGHT), 740, 146, 0);
    bitmap_place(PIC(P_FEATURES), 27, 530, 0);
    bitmap_place(PIC(P_START), 740, 373, 0);
    bitmap_place(PIC(P_BACK), 740, 417, 0);
    for (i = 0; i < 4; i++)
        bitmap_place(PIC(P_BAR + i), 145, (int[]){ 568, 595, 621, 647 }[i], 0);
    bitmap_place(PIC(P_AUTO), 401, 564, 0);
    bitmap_place(PIC(P_AUTO_DARK), 401, 356, 0);
    bitmap_place(PIC(P_AUTO_HL), 401, 564, 0);
    bitmap_place(PIC(P_MANUAL), 401, 407, 0);
    bitmap_place(PIC(P_MANUAL_DARK), 401, 615, 0);
    bitmap_place(PIC(P_MANUAL_HL), 401, 615, 0);
    bitmap_slide(PIC(P_TITLE), 0, 0, 1.0f, 1, 0);
    bitmap_slide(PIC(P_LEFT), 6, 148, 1.0f, 1, 0);
    bitmap_slide(PIC(P_RIGHT), 578, 146, 1.0f, 1, 0);
    bitmap_slide(PIC(P_FEATURES), 27, 322, 1.0f, 1, 0);
    for (i = 0; i < 4; i++)
        bitmap_slide(PIC(P_BAR + i), 145, (int[]){ 360, 387, 413, 439 }[i], 1.0f, 1, 0);
    /* (the lit A-T and the dark M-T rise with the rest; in manual, the dark A-T and the
       lit M-T are already in their places) */
    bitmap_slide(PIC(P_AUTO), 401, 356, 1.0f, 1, 0);
    bitmap_slide(PIC(P_MANUAL_DARK), 401, 407, 1.0f, 1, 0);
    bitmap_slide(PIC(P_START), 512, 373, 1.0f, 1, 0);
    bitmap_slide(PIC(P_BACK), 512, 417, 1.0f, 1, 0);

    /* the bars start full and shrink to the bike's values in a second */
    bar_targets();
    memset(g_bar_from, 0, sizeof g_bar_from);
    memset(g_bar_shown, 0, sizeof g_bar_shown);
    g_bar_t0 = g_now;
    g_bar_t1 = g_now + 1000;
    g_bar_running = 1;
    make_buttons();
}

/* the page's end frees the bike's model and textures: nothing yet */
void page_bike_end(void)
{
}

static uint32_t bits(float f)
{
    uint32_t u;
    memcpy(&u, &f, 4);
    return u;
}

/* the camera goes round, or, during a change, the point it looks at slides sideways
   (a frame's step, whatever the frame's length: the original's own), the bike goes out
   on one side and the next comes in from the other */
static void turntable(void)
{
    float c = (float)cos(g_turn), s = (float)sin(g_turn);
    float a = (float)(250.0 * c), b = (float)(100.0 * c);
    int cpos = (int32_t)bits(c) > 0, cneg = bits(c) > 0x80000000u;
    float *t = &g_target[0], *z = &g_target[2];
    if (g_dir == 0) {
        int dt = (int)(g_now - g_turn_last);
        if (dt > 1000)
            dt = 1000;
        g_turn_last = g_now;
        g_turn = (float)(dt * 0.0006135923151542565 + g_turn);
    } else if ((g_dir == -1 && cpos) || (g_dir == 1 && cneg)) {
        if (fabsf(a) > *t) {
            *t = (float)((double)*t + fabsf(b));
            *z = (float)((double)fabsf(b) / -c * s + *z);
        } else {
            *t = -fabsf(a);
            *z = (float)((double)-fabsf(a) / -c * s);
            g_shown_offset = 0;
        }
    } else if ((g_dir == 1 && cpos) || (g_dir == -1 && cneg)) {
        if (-fabsf(a) < *t) {
            *t = (float)((double)*t - fabsf(b));
            *z = (float)(*z - (double)fabsf(b) / -c * s);
        } else {
            *t = fabsf(a);
            *z = (float)((double)fabsf(a) / -c * s);
            g_shown_offset = 0;
        }
    }
    g_shown_bike = (g_bike + g_shown_offset) & 7;
    /* (the bike is drawn here, 0x45f290) */
    /* the change is over when the new bike has come back to the middle */
    if (g_shown_offset == 0
        && ((((g_dir == 1 && cpos) || (g_dir == -1 && cneg)) && (int32_t)bits(*t) <= 0)
            || (((g_dir == -1 && cpos) || (g_dir == 1 && cneg)) && bits(*t) <= 0x80000000u))) {
        g_target[0] = g_target[1] = g_target[2] = 0.0f;
        g_dir = 0;
    }
}

static void change_bike(int dir)
{
    sound_play(0x28, 0);
    g_shown_offset = -dir;
    g_bike += dir;
    g_dir = dir;
    if (dir < 0) {
        bitmap_place(PIC(P_LEFT_FLASH), 6, 148, 0);
        bitmap_draw(PIC(P_LEFT_FLASH), BM_OWN, 100, 0);
    } else {
        bitmap_place(PIC(P_RIGHT_FLASH), 578, 146, 0);
        bitmap_draw(PIC(P_RIGHT_FLASH), BM_OWN, 100, 0);
    }
    bars_restart();
}

void page_bike_frame(void)
{
    int i, pressed, leaving = 0;
    menu_begin_frame();
    bitmap_draw(PIC(P_BACKGROUND), BM_OWN, 100, 0);
    turntable();
    bitmap_step(PIC(P_TITLE), 0, 0);
    bitmap_draw(PIC(P_TITLE), BM_OWN, 100, 0);
    bitmap_step(PIC(P_FEATURES), 0, 0);
    bitmap_draw(PIC(P_FEATURES), BM_OWN, 100, 0);
    for (i = 0; i < 4; i++)
        bitmap_step(PIC(P_BAR + i), 0, 0);
    bar_targets();
    if (g_bar_running) {
        unsigned t = g_now < g_bar_t1 ? g_now : g_bar_t1;
        double f = (double)(t - g_bar_t0) / (double)(g_bar_t1 - g_bar_t0);
        double e = (sin(f * 3.141592653589793 - 1.5707963267948966) + 1.0) * 0.5;
        for (i = 0; i < 4; i++)
            g_bar_shown[i] = (int)((g_bar_to[i] - g_bar_from[i]) * e + g_bar_from[i]);
        if (t == g_bar_t1)
            g_bar_running = 0;
    }
    for (i = 0; i < 4; i++)
        bitmap_draw(PIC(P_BAR + i), BM_OWN, 100, g_bar_shown[i]);

    /* Start: the engine (a road or a dirt bike) and then the voice, and on when they
       have been heard; until then nothing else is done */
    pressed = menu_pressed();
    if (pressed == 2 && !g_voice_played) {
        int h = sound_play(g_bikes[g_bike].perso == 0 ? 0x2c : 0x2b, 0);
        if (h != -1)
            g_go_at = sound_length(h);
        h = sound_play(0x58, 0);
        if (h != -1)
            g_go_at = sound_length(h);
        g_go_at += g_now;
        g_voice_played = 1;
    }
    if (pressed != -1 && !(g_go_at != 0xffffffffu && g_now <= g_go_at)) {
        switch (pressed) {
        case 0:
            menu_press_done();
            if (g_bike != 0)
                change_bike(-1);
            break;
        case 1:
            menu_press_done();
            if (g_bike != 7)
                change_bike(1);
            break;
        case 2:
            menu_press_done();
            menu_change_page(1);        /* the loading page, without the glass */
            break;
        case 3:
            menu_press_done();
            leaving = 1;
            menu_change_page(6);
            break;
        case 4:
        case 5:
            menu_press_done();
            g_gearbox = pressed == 4;
            make_buttons();
            g_menu.sel = 2;             /* on to Start */
            break;
        }
    }
    menu_draw_buttons();
    menu_transition();
    r3d_render();
    menu_end_frame();
    if (leaving)
        menu_capture();
}
