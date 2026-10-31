/* The Options pages (NOTES §4.14): 11 OPTIONS (start 0x443000, frame 0x443200), 12
   SETTINGS, the controls (0x444b10, 0x444f30), 13 DIAGNOSTIC (0x4458a0, 0x4459d0) and 23
   DATA DISK, the plug-in track lists (0x451d10, 0x4514e0, end 0x451fc0). What the game
   took from Windows (Direct3D, the mixer, DirectSound's format, the system's facts) the
   port answers for itself. */
#include "choices.h"
#include "config.h"
#include "mister.h"
#include "exe.h"
#include "files.h"
#include "font.h"
#include "joy.h"
#include "menu.h"
#include "pad.h"
#include "pages.h"
#include "r3d.h"
#include "race.h"
#include "sound.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <cpuid.h>
#include <x86intrin.h>
#endif

#define PIC(id) (&g_pics[id])
#define T(n) exe_text(n)

static const unsigned char white[3] = { 255, 255, 255 }, yellow[3] = { 255, 255, 0 };

/* ================================================================ page 11, OPTIONS */

enum {
    P_TILE2 = 0x01, P_GAUGE = 0x32, P_OPTITRE = 0x3c, P_ACCEPT = 0x46, P_CANCEL = 0x47,
    P_CONFIG = 0x57, P_INFO = 0xaa, P_DATADISK = 0xb7
};

static int g_ctrl;              /* 0x4e90d0: 0 keyboard, 1 joypad, 2 joystick */
static int g_nlists;            /* 0x4dc04c: Data\List*.bpt files */

/* a joystick or joypad the game could use (0x418b60) */
static int joystick_known(void)
{
    return joy_known(0);
}

/* 0x4311c0: the controller chosen; back to the keyboard when there is no joystick */
static void controller_setup(void)
{
    static const int device[3] = { 1, 4, 2 };
    g_input_device = device[g_ctrl];
    if (g_input_device == 2 || g_input_device == 4) {
        g_joy_type = g_input_device;
        joy_detect();
        if (!g_joy[0].working)
            g_input_device = 1;
    }
}

void page_options_start(void)
{
    bitmap_place(PIC(P_OPTITRE), 0, 0, 0);
    bitmap_place(PIC(P_ACCEPT), 500, 420, 0);
    bitmap_place(PIC(P_INFO), 25, 420, 0);
    bitmap_place(PIC(P_DATADISK), 275, 420, 0);
    if (g_input_device == 1)
        g_ctrl = 0;
    else if (g_input_device == 4)
        g_ctrl = 1;
    else if (g_input_device == 2)
        g_ctrl = 2;
    font_set_colors(font_get(9), white, white);
    if (font_get(12))
        font_set_colors(font_get(12), white, white);
    g_nlists = files_count("data", "list", ".bpt");
}

/* a row's label in white, or yellow while one of its buttons is selected */
static void label(int y, const char *text, int lit)
{
    Font *f = font_get(12);
    font_set_colors(f, lit ? yellow : white, lit ? yellow : white);
    font_draw(f, text, 50, y, 0, 0);
}

static const char *on_off(int on)
{
    return T(on ? 40 : 39);
}

/* a mixer line's row: the gauge (featacc cut on the right by what is missing), "-", "+" */
static void volume_row(int y, int line, unsigned nb_minus, unsigned nb_plus)
{
    Bitmap *g = PIC(P_GAUGE);
    bitmap_place(g, 312, y + 8, 0);
    bitmap_draw(g, BM_OWN, 100, -100 * g_volume[line] / 32 + 100);
    menu_add_text_button(300, y, "-", 12, nb_minus, 0, -1, 1);
    menu_add_text_button(400, y, "+", 12, nb_plus, 0, -1, 1);
}

void page_options_frame(void)
{
#ifdef MISTER_HYBRID
    static const char *const screen[3] = { "320*240", "640*240", "640*480" };
#else
    static const char *const screen[3] = { "320*240", "512*384", "640*480" };
#endif
    char buf[64];
    int sel, p, page = -1, on[3];
    Font *f12 = font_get(12);
    menu_begin_frame();
    bitmap_step(PIC(P_TILE2), -1, -1);
    bitmap_draw(PIC(P_TILE2), BM_OWN, 100, 0);
    sel = g_menu.sel;
    bitmap_draw(PIC(P_OPTITRE), BM_OWN, 100, 0);
    menu_clear_buttons();
    sound_get_switches(&on[0], &on[1], &on[2]);

    label(100, T(25), sel == 0);
    if (g_difficulty >= 0 && g_difficulty <= 2)
        menu_add_text_button(300, 100, T(26 + g_difficulty), 12, 0xff01ffffu, 0, -1, 0);
    label(120, T(29), sel == 1);
    menu_add_text_button(300, 120, T(30 + g_ctrl), 12, g_ctrl == 2 ? 0x0003ffffu : 0x000302ffu,
                         0, -1, 0);
    menu_add_button(PIC(P_CONFIG), 0, 0xffffff01u, -1, -1, 0, 0, 0, 475, 120, 0);
    label(140, T(33), sel == 3);
    snprintf(buf, sizeof buf, "%d", g_detail + 1);
    menu_add_text_button(300, 140, buf, 12, 0x0104ffffu, 0, -1, 0);
    label(160, T(34), sel == 4);
    menu_add_text_button(300, 160, g_opponents ? "12" : "24", 12, 0x0305ffffu, 0, -1, 0);
    label(180, T(35), sel == 5);
    menu_add_text_button(300, 180, T(41), 12, 0x0406ffffu, 0, -1, 0);  /* no Direct3D */
    label(200, T(36), sel == 6);
    menu_add_text_button(300, 200, sound_running() ? on_off(on[1]) : T(132), 12, 0x0507ffffu,
                         0, -1, 0);
    label(220, T(37), sel == 7);
    menu_add_text_button(300, 220, sound_running() ? on_off(on[0]) : T(132), 12, 0x0608ffffu,
                         0, -1, 0);
    label(240, T(38), sel == 8);
    menu_add_text_button(300, 240, sound_running() ? on_off(on[2]) : T(133), 12, 0x0709ffffu,
                         0, -1, 0);
    label(260, T(43), sel == 9);
#ifdef MISTER_HYBRID
    menu_add_text_button(300, 260, screen[g_mister_res], 12, 0x080affffu, 0, -1, 0);
#else
    menu_add_text_button(300, 260, screen[g_race_screen], 12, 0x080affffu, 0, -1, 0);
#endif
    label(280, T(44), sel == 10);
    menu_add_text_button(300, 280, on_off(g_mode169), 12, 0x090bffffu, 0, -1, 0);
    label(300, T(93), sel == 11);
    if (sound_running())
        snprintf(buf, sizeof buf, "%d %s / %d %s /%s", g_wave16 ? 16 : 8, T(47),
                 g_replay_rate / 1000, T(48), T(g_stereo ? 61 : 62));
    else
        snprintf(buf, sizeof buf, "%s", T(71));
    menu_add_text_button(300, 300, buf, 12, 0x0a0cffffu, 0, -1, 0);
    label(320, T(82), sel == 12 || sel == 13);
    volume_row(320, 0, 0x0b0e0dffu, 0x0b0eff0cu);
    label(340, T(83), sel == 14 || sel == 15);
    volume_row(340, 2, 0x0c100fffu, 0x0c10ff0eu);
    label(360, T(84), sel == 16 || sel == 17);
    volume_row(360, 1, 0x0e1211ffu, 0x0e12ff10u);
    label(380, T(189), sel == 18 || sel == 19);
    menu_add_text_button(300, 380, "-", 12, 0x101413ffu, 0, -1, 1);
    menu_add_text_button(400, 380, "+", 12, 0x1014ff12u, 0, -1, 1);
    if (g_enum_session == 0)
        snprintf(buf, sizeof buf, "%s", T(190));
    else
        snprintf(buf, sizeof buf, "%d %s", g_enum_session, T(191));
    font_draw(f12, buf, 312, 380, 0, 0);
    /* the port's own row: which program the game acts like */
    label(400, "GAME BEHAVIOUR :", sel == 20);
    menu_add_text_button(300, 400, behaviour_cd() ? "1997 CD" : "GOG RELEASE", 12, 0x1215ffffu,
                         0, -1, 0);

    menu_add_button(PIC(P_ACCEPT), 0, 0x14ffff16u, -1, -1, 0, 0, 0, -1, -1, 0);
    if (g_nlists > 1) {
        menu_add_button(PIC(P_INFO), 0, 0x14ff1517u, -1, -1, 0, 0, 0, -1, -1, 0);
        menu_add_button(PIC(P_DATADISK), 0, 0x14ff15ffu, -1, -1, 0, 0, 0, -1, -1, 0);
    } else
        menu_add_button(PIC(P_INFO), 0, 0x14ff15ffu, -1, -1, 0, 0, 0, -1, -1, 0);
    /* a joypad or joystick asked for and none there: the keyboard again (after the button
       was made, so the other name shows for one picture) */
    if ((g_ctrl == 1 || g_ctrl == 2) && !joystick_known())
        g_ctrl = 0;
    font_set_colors(f12, white, white);
    menu_draw_buttons();

    p = menu_pressed();
    if (p != -1)
        menu_press_done();
    switch (p) {
    case 0:
        g_difficulty = (g_difficulty + 1) % 3;
        break;
    case 1:
        g_ctrl = (g_ctrl + 1) % 3;
        break;
    case 2:
        controller_setup();
        page = 12;
        break;
    case 3:
        g_detail = (g_detail + 1) % 5;
        break;
    case 4:
        g_opponents ^= 1;
        break;
    case 6:
        sound_toggle_voices();
        break;
    case 7:
        sound_toggle_effects();
        break;
    case 8:
        music_toggle();
        break;
    case 9:
#ifdef MISTER_HYBRID
        race_set_resolution((g_mister_res + 1) % MR_COUNT);
#else
        g_race_screen = (g_race_screen + 1) % 3;
#endif
        break;
    case 10:
#ifndef MISTER_HYBRID
        g_mode169 ^= 1;
#endif
        break;
    case 11:
        /* 0x409570 on a card that does 16 bits and stereo: 16 bits off and on, then the
           rate, and the stereo with it going up */
        if (sound_running() && !(g_wave16 ^= 1)) {
            int old = g_replay_rate;
            g_replay_rate = old <= 0x4099 ? 22050 : 11025;
            if (old < 0x4099)
                g_stereo ^= 1;
        }
        break;
    case 12: case 13: case 14: case 15: case 16: case 17: {
        static const int line[3] = { 0, 2, 1 };
        int *v = &g_volume[line[(p - 12) / 2]], n = *v + (p & 1 ? 1 : -1);
        if (n >= 0 && n <= 32)
            *v = n;
        sound_set_volumes(g_volume);
        break;
    }
    case 18:
        if (--g_enum_session < 0)
            g_enum_session = 0;
        break;
    case 19:
        if (++g_enum_session > 30)
            g_enum_session = 30;
        break;
    case 20:
        behaviour_toggle();
        break;
    case 21:
        controller_setup();
        page = 3;
        break;
    case 22:
        page = 13;
        break;
    case 23:
        page = 23;
        break;
    }
    menu_transition();
    r3d_render();
    menu_end_frame();
    if (page >= 0) {            /* the glass, of the picture just shown */
        menu_capture();
        menu_change_page(page);
    }
}

/* ================================================================ page 12, SETTINGS */

enum {
    P_TILE3 = 0x02, P_PADCARD = 0x6a, P_CLAVCARD = 0x6b, P_JOYCARD = 0x6c, P_CHOSET = 0x6d,
    P_SETTING = 0x6e, P_SET = 0x6f, P_SET2 = 0x70, P_JTOP = 0xcc, P_JLEFT = 0xcd,
    P_JRIGHT = 0xce, P_JBAS = 0xcf
};

static int g_row;               /* 0x53f790: the action being set */
static int g_let_go;            /* a gamepad's row chosen: the press must end before the next */

void page_settings_start(void)
{
    bitmap_place(PIC(P_SETTING), 0, 0, 0);
    bitmap_place(PIC(P_ACCEPT), 0, 420, 1);
    bitmap_place(PIC(P_PADCARD), 36, 101, 0);
    bitmap_place(PIC(P_CLAVCARD), 36, 101, 0);
    bitmap_place(PIC(P_JOYCARD), 36, 101, 0);
    bitmap_place(PIC(P_CHOSET), 251, 101, 0);
    g_row = 0;
    g_pad_listen = 0;
}

/* the keys an action can have (0x44580c): digits, letters, Return, Shift, Ctrl, Caps
   Lock, Space, the page and arrow keys, Insert, Delete, the keypad's digits, separator
   and point */
static int key_allowed(int k)
{
    return (k >= '0' && k <= '9') || (k >= 'A' && k <= 'Z') || k == 0x0d || k == 0x10
           || k == 0x11 || k == 0x14 || (k >= 0x20 && k <= 0x28) || k == 0x2d || k == 0x2e
           || (k >= 0x60 && k <= 0x69) || k == 0x6c || k == 0x6e;
}

/* 0x4310f0 (key) and 0x431140 (button): taken from any other action first */
static void bind(int row, int col, int v)
{
    int i;
    if (row < 0 || row >= ACTIONS)
        return;
    for (i = 0; i < ACTIONS; i++)
        if (g_binding[i][col] == v)
            g_binding[i][col] = -1;
    g_binding[row][col] = v;
}

/* what is held on the joystick goes to the row being set, whatever the pointer is on; a
   joystick's direction takes its opposite to the paired action */
static int joy_bind(int v)
{
    int pair = -1, w = 0;
    if (v == -1)
        return 0;
    if (g_ctrl == 1) {          /* a joypad: anything to any row */
        bind(g_row, 1, v);
        return 1;
    }
    switch (g_row) {
    case 0: case 1:             /* accelerate, brake: a button or up / down */
        if (v < 0 && v > -4)
            return 0;
        pair = 1 - g_row, w = v == -4 ? -5 : -4;
        break;
    case 2: case 3:             /* left, right: a button or left / right */
        if (v < -3)
            return 0;
        pair = 5 - g_row, w = v == -2 ? -3 : -2;
        break;
    case 4: case 5:             /* the gears: a button */
        if (v < 0)
            return 0;
        break;
    }
    bind(g_row, 1, v);
    if (pair >= 0 && v < 0)
        bind(pair, 1, w);
    return 1;
}

/* The port's: a gamepad's cross and stick work this page too, so what they do can't be taken
   for a binding as it was: a row is chosen with A, then the next thing pressed goes to it. */
static void pad_listen(void)
{
    int v = joy_code(0);
    if (g_let_go) {
        g_let_go = v != -1;
        return;
    }
    if (joy_bind(v)) {
        g_pad_listen = 0;
        pad_quiet();
    }
}

void page_settings_frame(void)
{
    char buf[96];
    int sel, i, p, pad = g_ctrl != 0 && joy_is_pad(0);
    Font *f12 = font_get(12), *f24 = font_get(24);
    menu_begin_frame();
    sel = g_menu.sel;
    bitmap_step(PIC(P_TILE3), -1, -1);
    bitmap_draw(PIC(P_TILE3), BM_OWN, 100, 0);
    bitmap_step(PIC(P_SETTING), 0, 0);
    bitmap_draw(PIC(P_SETTING), BM_OWN, 100, 0);
    if (g_ctrl == 0) {
        bitmap_step(PIC(P_CLAVCARD), 0, 0);
        bitmap_draw(PIC(P_CLAVCARD), BM_OWN, 100, 0);
        if (sel >= 1 && sel <= 7) {
            if (!key_allowed(g_menu.key))
                g_menu.key = 0;
            if (!g_menu.mouse_mode && g_menu.key)
                bind(g_row, 0, g_menu.key);
        }
    } else {
        Bitmap *card = PIC(g_ctrl == 1 ? P_PADCARD : P_JOYCARD);
        bitmap_step(card, 0, 0);
        bitmap_draw(card, BM_OWN, 100, 0);
        if (!pad)
            joy_bind(joy_code(0));
        else if (g_pad_listen)
            pad_listen();
    }
    bitmap_step(PIC(P_CHOSET), 0, 0);
    bitmap_draw(PIC(P_CHOSET), BM_OWN, 100, 0);
    menu_clear_buttons();
    /* (a gamepad's cross: up and down the rows, and Accept between the two options) */
    menu_add_button(PIC(P_ACCEPT), 0, pad ? 0x07ff0908u : 0x03ff0506u, VK_ESCAPE, -1, 0, 0, 0,
                    -1, -1, 0);
    for (i = 0; i < ACTIONS; i++)
        menu_add_button(PIC(i == g_row ? P_SET : P_SET2), 0,
                        !pad ? 0xffffffffu
                        : (unsigned)(i ? i : 0xff) << 24 | (unsigned)(i < ACTIONS - 1 ? i + 2 : 0) << 16
                          | 0xffffu, -1, -1, 0, 0, 0, 251, 148 + 37 * i, 0);
    if (g_ctrl != 0) {          /* the port's two options */
        font_set_colors(f12, white, white);
        snprintf(buf, sizeof buf, "Analog   %s", on_off(g_analog));
        menu_add_text_button(70, 432, buf, 12, 0x07ff00ffu, 0, -1, 0);
        snprintf(buf, sizeof buf, "Rumble   %s", on_off(g_rumble));
        menu_add_text_button(420, 432, buf, 12, 0x07ffff00u, 0, -1, 0);
    }
    menu_draw_buttons();
    for (i = 0; i < ACTIONS; i++) {
        int y = 156 + 37 * i;
        const char *name = T(i);
        font_set_colors(f12, white, white);
        if (g_ctrl == 0) {
            int k = g_binding[i][0];
            if (k == -1) {
                snprintf(buf, sizeof buf, T(8), name);
                font_draw(f12, buf, 255, y, 0, 0);
            } else {
                snprintf(buf, sizeof buf, T(7), name);
                font_draw(f12, buf, 255, y, 0, 0);
                if (k >= 0 && k + 0x13 < 256) {     /* the key's cap, right on 590 */
                    buf[0] = (char)(k + 0x13);
                    buf[1] = 0;
                    font_draw(f24, buf, 590 - f24->width[k + 0x13], y - 5, 0, 0);
                }
            }
        } else {
            int b = g_binding[i][1], spare = joy_spare_trigger(0, i);
            static const int dir[4] = { P_JLEFT, P_JRIGHT, P_JTOP, P_JBAS };
            if (g_pad_listen && i == g_row) {
                snprintf(buf, sizeof buf, T(7), name);
                font_draw(f12, buf, 255, y, 0, 0);
                font_draw(f12, "Press...", 510, y, 0, 0);
            } else if (b >= 0 && pad) {
                snprintf(buf, sizeof buf, T(7), name);
                font_draw(f12, buf, 255, y, 0, 0);
                if (spare >= 0) {
                    snprintf(buf, sizeof buf, "%s + %s", joy_button_name(b), joy_button_name(spare));
                    font_draw(f12, buf, 510, y, 0, 0);
                } else
                    font_draw(f12, joy_button_name(b), 550, y, 0, 0);
            } else if (b >= 0) {
                snprintf(buf, sizeof buf, T(9), name, b);
                font_draw(f12, buf, 255, y, 0, 0);
            } else if (b >= -5 && b <= -2) {
                snprintf(buf, sizeof buf, T(7), name);
                font_draw(f12, buf, 255, y, 0, 0);
                bitmap_place(PIC(dir[-2 - b]), 550, y - 7, 0);
                bitmap_draw(PIC(dir[-2 - b]), BM_OWN, 100, 0);
                if (spare >= 0)
                    font_draw(f12, joy_button_name(spare), 510, y, 0, 0);
            } else if (spare >= 0) {
                snprintf(buf, sizeof buf, T(7), name);
                font_draw(f12, buf, 255, y, 0, 0);
                font_draw(f12, joy_button_name(spare), 550, y, 0, 0);
            } else {
                snprintf(buf, sizeof buf, T(8), name);
                font_draw(f12, buf, 255, y, 0, 0);
            }
        }
    }
    menu_transition();
    r3d_render();
    menu_end_frame();
    p = menu_pressed();
    if (p != -1)
        menu_press_done();
    if (p == 0) {
        g_pad_listen = 0;
        menu_capture();
        menu_change_page(11);
        return;
    }
    if (g_pad_listen) {         /* a click elsewhere: not this time */
        if (p != -1)
            g_pad_listen = 0;
        return;
    }
    if (p >= 1 && p <= 7) {
        g_row = p - 1;
        if (pad)
            g_pad_listen = g_let_go = 1;
    }
    if (p == 8)
        g_analog ^= 1;
    if (p == 9)
        g_rumble ^= 1;
    if (sel >= 1 && sel <= 7)   /* the row under the pointer is the one set next */
        g_row = sel - 1;
}

/* ================================================================ page 13, DIAGNOSTIC */

enum {
    P_TILE1 = 0x00, P_BACK = 0x51, P_PROC = 0xb3, P_VIDEO = 0xb4, P_SON = 0xb5, P_JOY = 0xb6,
    P_DIAGNOS = 0xc5
};

/* 0x440540's facts, as the port finds them */
static char g_os[64], g_cpu[64], g_speed[32], g_mem[32];
static int g_mmx;

static void sysinfo(void)
{
    FILE *f;
    char line[256], vendor[32] = "", *v;
    int family = -1, mhz = 0;
    int ram;
    g_os[0] = g_cpu[0] = g_speed[0] = g_mem[0] = 0;
    g_mmx = 0;
#if defined(_WIN32)
    snprintf(g_os, sizeof g_os, "Windows(R)");
#elif defined(__linux__)
    snprintf(g_os, sizeof g_os, "Linux");
#endif
#ifdef _WIN32
    {   /* no /proc/cpuinfo: the same facts from the CPU itself, its speed by the clock */
        unsigned a, b, c, d;
        unsigned long long t;
        if (__get_cpuid(0, &a, &b, &c, &d)) {
            memcpy(vendor, &b, 4), memcpy(vendor + 4, &d, 4), memcpy(vendor + 8, &c, 4);
            vendor[12] = 0;
        }
        if (__get_cpuid(1, &a, &b, &c, &d)) {
            family = (a >> 8 & 15) + (a >> 20 & 255);
            g_mmx = d >> 23 & 1;
        }
        t = __rdtsc();
        SDL_Delay(50);
        mhz = (int)((__rdtsc() - t) / 50000);
    }
#endif
    if ((f = fopen("/proc/cpuinfo", "r"))) {
        while (fgets(line, sizeof line, f)) {
            if (!(v = strchr(line, ':')))
                continue;
            v += 2;
            v[strcspn(v, "\n")] = 0;
            if (!strncmp(line, "vendor_id", 9) && !vendor[0])
                snprintf(vendor, sizeof vendor, "%s", v);
            else if (!strncmp(line, "cpu family", 10) && family < 0)
                family = atoi(v);
            else if (!strncmp(line, "cpu MHz", 7) && !mhz)
                mhz = atoi(v);
            else if (!strncmp(line, "flags", 5) && strstr(v, " mmx"))
                g_mmx = 1;
        }
        fclose(f);
    }
    if (vendor[0]) {            /* named the way 0x440540 names a CPUID */
        const char *name = !strcmp(vendor, "AuthenticAMD") ? "AMD"
                         : !strcmp(vendor, "GenuineIntel") ? "Intel"
                         : !strcmp(vendor, "CyrixInstead") ? "Cyrix" : "Unknow";
        if (family == 4)
            snprintf(g_cpu, sizeof g_cpu, "%s 486", name);
        else if (family == 5)
            snprintf(g_cpu, sizeof g_cpu, "%s Pentium(R)", name);
        else if (family == 6)
            snprintf(g_cpu, sizeof g_cpu, "%s Pentium(R) Pro", name);
        else
            snprintf(g_cpu, sizeof g_cpu, "%s CPU Family:$%x", name, family);
        if (mhz)
            snprintf(g_speed, sizeof g_speed, "%d Mhz", mhz);
    }
    ram = SDL_GetSystemRAM();
    if (ram > 0)
        snprintf(g_mem, sizeof g_mem, T(188), ram);
}

void page_diag_start(void)
{
    bitmap_place(PIC(P_DIAGNOS), 0, 0, 0);
    bitmap_place(PIC(P_BACK), 500, 440, 0);
    menu_clear_buttons();
    menu_add_button(PIC(P_BACK), 0, 0x0bffffffu, VK_ESCAPE, -1, 0, 0, 0, -1, -1, 0);
    font_set_colors(font_get(12), white, white);
    sysinfo();
    joy_detect();
    bitmap_place(PIC(P_PROC), 32, 85, 0);
    bitmap_place(PIC(P_VIDEO), 32, 185, 0);
    bitmap_place(PIC(P_SON), 32, 285, 0);
    bitmap_place(PIC(P_JOY), 32, 385, 0);
}

static void line(int y, const char *a, const char *b)
{
    char buf[200];
    snprintf(buf, sizeof buf, "%s%s", a, b);
    font_draw(font_get(12), buf, 128, y, 0, 0);
}

void page_diag_frame(void)
{
    static const int icon[5] = { P_DIAGNOS, P_PROC, P_VIDEO, P_SON, P_JOY };
    char buf[200];
    int i, y;
    menu_begin_frame();
    bitmap_step(PIC(P_TILE1), 1, -1);
    bitmap_draw(PIC(P_TILE1), BM_OWN, 100, 0);
    for (i = 0; i < 5; i++)
        bitmap_draw(PIC(icon[i]), BM_OWN, 100, 0);
    r3d_render();
    line(80, T(172), g_os[0] ? g_os : T(71));
    if (!g_cpu[0])
        line(100, T(49), T(71));
    else if (!g_speed[0])
        line(100, T(49), g_cpu);
    else {
        snprintf(buf, sizeof buf, "%s%s (%s)", T(49), g_cpu, g_speed);
        font_draw(font_get(12), buf, 128, 100, 0, 0);
    }
    line(120, T(50), g_mmx ? T(51) : T(53));
    line(140, T(171), g_mem[0] ? g_mem : T(71));
    line(200, T(89), T(73));
    line(220, T(92), T(71));            /* no Direct3D */
    if (!behaviour_cd())                /* (0x46a020 has no such line) */
        line(240, "", T(71));           /* (the video board's memory: not known) */
    if (sound_running()) {              /* SDL's mixing: 16 bits, stereo, no hardware's */
        y = 290;
        snprintf(buf, sizeof buf, "%s%s%s", T(68), T(45), T(61));
        font_draw(font_get(12), buf, 128, y, 0, 0);
        y += 20;
        line(y, T(67), T(69));
        y += 20;
    } else {
        line(300, "", T(63));
        y = 320;
    }
    snprintf(buf, sizeof buf, T(74), T(73));
    font_draw(font_get(12), buf, 128, y, 0, 0);
    line(400, "", T(g_joy[0].present ? 58 : 57));
    line(420, "", T(g_joy[0].working ? 60 : 59));
    menu_draw_buttons();
    menu_transition();
    r3d_render();
    menu_end_frame();
    if (menu_pressed() == 0) {
        menu_press_done();
        menu_capture();
        menu_change_page(11);
    }
}

/* ================================================================ page 23, DATA DISK */

enum { P_LEFT = 0x1a, P_RIGHT = 0x1c, P_DATTITLE = 0xb8 };
#define SHOWN 8                 /* pictures on the page */

static int g_old_list;          /* 0x540180 */
static int g_loaded;            /* 0x542380: the list is read, its pictures wanted */
static int g_leaving;           /* 0x540994: the pictures slide out, a new list is due */
static int g_none_loaded;       /* 0x53f554: no picture was loaded this picture */
static Bitmap *g_photo;         /* 0x4e90ec: a picture a track */
static unsigned char *g_want;   /* the track's +0 here: its picture is still to load */
static int g_nphoto;

static void photos_free(void)
{
    int i;
    for (i = 0; i < g_nphoto; i++)
        bitmap_free(&g_photo[i]);
    free(g_photo);
    free(g_want);
    g_photo = 0;
    g_want = 0;
    g_nphoto = 0;
}

static int photos_new(void)
{
    photos_free();
    g_photo = calloc((size_t)g_ntracks + 1, sizeof *g_photo);
    g_want = malloc((size_t)g_ntracks + 1);
    if (!g_photo || !g_want) {
        photos_free();
        return 0;
    }
    memset(g_want, 1, (size_t)g_ntracks);
    g_nphoto = g_ntracks;
    return 1;
}

/* the list g_list_number and its championship (0x428d60, 0x429a00) */
static int read_list(void)
{
    if (!tracks_load() || !champ_load())
        return 0;
    if (g_track >= g_ntracks)
        g_track = 0;
    return 1;
}

void page_datadisk_start(void)
{
    g_old_list = g_list_number;
    bitmap_place(PIC(P_DATTITLE), 0, 0, 0);
    bitmap_place(PIC(P_ACCEPT), 442, 425, 0);
    bitmap_place(PIC(P_CANCEL), 85, 425, 0);
    bitmap_place(PIC(P_LEFT), -100, 181, 0);
    bitmap_place(PIC(P_RIGHT), 740, 181, 0);
    bitmap_slide(PIC(P_LEFT), 7, 181, 0.7f, 1, 0);
    bitmap_slide(PIC(P_RIGHT), 581, 181, 0.7f, 1, 0);
    menu_clear_buttons();
    menu_add_button(PIC(P_ACCEPT), 0, 0xffffffffu, VK_RETURN, -1, 0, 0, 0, -1, -1, 0);
    menu_add_button(PIC(P_LEFT), 0, 0xff02ffffu, VK_LEFT, -1, 0, 0, 0, -1, -1, 0);
    menu_add_button(PIC(P_RIGHT), 0, 0xff01ffffu, VK_RIGHT, -1, 0, 0, 0, -1, -1, 0);
    menu_add_button(PIC(P_CANCEL), 0, 0xffffffffu, VK_ESCAPE, -1, 0, 0, 0, -1, -1, 0);
    photos_new();
    g_loaded = 1;
    g_leaving = 0;
}

void page_datadisk_end(void)
{
    photos_free();
}

void page_datadisk_frame(void)
{
    int i, n, p;
    menu_begin_frame();
    bitmap_step(PIC(P_TILE2), 1, 1);
    bitmap_draw(PIC(P_TILE2), BM_OWN, 100, 0);
    bitmap_step(PIC(P_DATTITLE), 0, 0);
    bitmap_draw(PIC(P_DATTITLE), BM_OWN, 100, 0);
    font_set_colors(font_get(31), yellow, 0);
    font_draw(font_get(31), g_list_name, 350, 27, 0, 0);
    /* the pictures have left: the next list (a list that cannot be read leaves the
       picture unfinished, as the game does) */
    if (g_nphoto && !g_photo[0].sliding && !g_loaded)
        g_leaving = 0;
    if (!g_leaving && !g_loaded) {
        if (!read_list())
            return;
        g_loaded = 1;
        photos_new();
    }
    /* one picture loaded a frame, sliding in: the top row from above, the bottom row from
       below; the others drawn */
    g_none_loaded = 1;
    n = g_nphoto < SHOWN ? g_nphoto : SHOWN;
    for (i = 0; i < n; i++) {
        if (g_want[i] && !g_leaving) {
            char path[0x140];
            int x = 85 + 126 * (i % 4), r = i / 4;
            snprintf(path, sizeof path, "%ssmalphot.lez", g_tracks[i].folder);
            bitmap_load_lez(&g_photo[i], path, BM_KEYED);
            g_want[i] = 0;
            bitmap_place(&g_photo[i], x, 840 * r - 200, 0);
            bitmap_slide(&g_photo[i], x, 95 + 166 * r, 0.7f, 1, 0);
            g_none_loaded = 0;
            break;
        }
        bitmap_step(&g_photo[i], 0, 0);
        bitmap_draw(&g_photo[i], BM_OWN, 100, 0);
    }
    menu_draw_buttons();
    p = menu_pressed();
    if (p == 0) {
        menu_press_done();
        menu_change_page(11);
    } else if (p == 1 || p == 2) {
        menu_press_done();
        if (g_loaded && !g_leaving && g_none_loaded) {
            g_loaded = 0;
            g_leaving = 1;
            for (i = 0; i < n; i++)     /* the left two columns out left, the others right */
                bitmap_slide(&g_photo[i], (i / 2) & 1 ? 640 : -100, g_photo[i].y, 0.4f, 1, 0);
            if (p == 1)
                g_list_number = (g_list_number <= 0 ? g_nlists : g_list_number) - 1;
            else
                g_list_number = g_list_number < g_nlists - 1 ? g_list_number + 1 : 0;
        }
    } else if (p == 3) {
        menu_press_done();
        g_list_number = g_old_list;
        if (!read_list())
            return;
        menu_change_page(11);
    }
    menu_transition();
    r3d_render();
    menu_end_frame();
}
