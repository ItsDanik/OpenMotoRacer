/* Pages 25 and 14 (NOTES §4.9.1, §4.9.2): the Delphine Software logo, its letters
   spiralling in for 20 s, then the Moto Racer title, its three pieces flying in over a
   picture that gains each one as it lands. */
#include "clock.h"
#include "font.h"
#include "menu.h"
#include "pages.h"
#include "r3d.h"
#include "sound.h"
#include "text.h"
#include "video.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define PI 3.14159265358979323846
#define PIC(id) (&g_pics[id])
enum { P_START = 0x4c, P_CONTINUE = 0x4d, P_TITLE = 0xbc };

static const float g_campos[3] = { 0.0f, 0.0f, 4500.0f }, g_origin[3];

/* time since the last frame (0x453c50, 0x446b80): a stall of over 500 ms counts 100 */
static float frame_dt(unsigned *last)
{
    float dt = (float)(g_now - *last);
    *last = g_now;
    return dt > 500.0f ? 100.0f : dt;
}

/* ---------------------------------------------------------------- 25, the logo */

static Model g_dsi[5];          /* 0x542150: camera, D, S, I, DsInt */
static float g_dsi_ang[5][3], g_dsi_pos[5][3];          /* the objects 0x53f850 */
static float g_dsi_prog[6];     /* 0x540188 */
static int g_dsi_done;          /* 0x53f788 */
static unsigned g_dsi_last;     /* 0x5409a8 */

/* 0x453610's tables */
static const float g_dsi_dur[6] = { 15.0f, 14.5f, 14.0f, 13.5f, 16.0f, 20.0f };
static const float g_dsi_end[6][3] = {
    { 50, 0, 7000 }, { 200, 0, 200 }, { -50, 0, 200 }, { -250, 0, 200 }, { 20, -400, -600 },
    { 50, 0, 7000 }
};
static const float g_dsi_start4[3] = { 20, 0, 60000 }, g_dsi_start5[3] = { 50, 80, -1000 };

void page_logo_start(void)
{
    static const char *const names[5] = { "camera", "D", "S", "I", "DsInt" };
    char path[64];
    int i;
    r3d_load_page(0, "data\\pages\\titleDsi\\bnk00_00.raw");
    r3d_load_page(1, "data\\pages\\titleDsi\\bnk00_01.raw");
    for (i = 0; i < 5; i++) {
        snprintf(path, sizeof path, "data\\pages\\titleDsi\\%s", names[i]);
        model_free(&g_dsi[i]);
        if (model_load(&g_dsi[i], path)) {
            model_centre(&g_dsi[i]);
            if (i == 0)
                model_scale(&g_dsi[i], 0.055f, 0.055f, 0.06f);
            else
                model_scale(&g_dsi[i], 0.04f, 0.04f, 0.04f);
        }
    }
    memset(g_dsi_ang, 0, sizeof g_dsi_ang);
    memset(g_dsi_pos, 0, sizeof g_dsi_pos);
    g_dsi_ang[0][1] = -1.701696f;
    g_dsi_pos[0][0] = -30.0f, g_dsi_pos[0][2] = -900.0f;
    for (i = 1; i < 5; i++)
        g_dsi_ang[i][1] = (float)(-PI / 2);
    g_dsi_ang[3][1] = g_dsi_ang[4][1] = (float)(PI / 2);
    r3d_camera(g_campos, g_origin);
    memset(g_dsi_prog, 0, sizeof g_dsi_prog);
    g_dsi_done = 0;
    music_play(13, 0);          /* (logodsi.lez is loaded too, but never drawn) */
    menu_clear_buttons();
    bitmap_place(PIC(P_CONTINUE), -300, -300, 0);       /* off the screen: Escape skips */
    menu_add_button(PIC(P_CONTINUE), 0, 0xffffffffu, VK_ESCAPE, -1, 0, 0, 0, -1, -1, 0);
    g_dsi_last = g_now;
}

void page_logo_frame(void)
{
    float dt = frame_dt(&g_dsi_last);
    int i, done = 0, pressed;
    for (i = 0; i < 480; i++)                           /* black (0x476680) */
        memset(g_screen + i * g_pitch, 0, sizeof(uint16_t) * 640);
    menu_begin_frame();
    for (i = 0; i < 6; i++) {
        double t;
        g_dsi_prog[i] = (float)(g_dsi_prog[i] + dt / (g_dsi_dur[i] * 1000.0f));
        t = g_dsi_prog[i] < 0 ? 0 : g_dsi_prog[i] > 1 ? 1 : g_dsi_prog[i];
        if (g_dsi_prog[i] > 1.0f)
            done++;
        if (i == 0) {           /* the camera model swings round */
            g_dsi_ang[0][1] = (float)((1 - t) * (PI / 2) - 1.701696021);
            g_dsi_ang[0][2] = (float)((1 - t * t) * (PI / 2));
        } else if (i <= 3) {    /* D, S, I spiral in, four turns */
            const float *e = g_dsi_end[i];
            double l = sqrt((double)e[0] * e[0] + (double)e[1] * e[1] + (double)e[2] * e[2]);
            double r = t * t * -2500.0 + l + 2500.0, x = e[0] / l * r, y = e[1] / l * r;
            double z = e[2] / l * r;
            int n = (int)(t * (-2 * PI) * 2607.5945876176133);
            double c = g_cos[n & 0x3fff], s = g_cos[(n - 0x1000) & 0x3fff];
            g_dsi_ang[i][1] = (float)((1 - fabs(t)) * 8 * PI + (i == 3 ? PI / 2 : -PI / 2));
            g_dsi_pos[i][0] = (float)(x * c + z * s);
            g_dsi_pos[i][1] = (float)y;
            g_dsi_pos[i][2] = (float)(-x * s + z * c - 800.0);
        } else if (i == 4) {    /* the banner comes from far away */
            int k;
            for (k = 0; k < 3; k++)
                g_dsi_pos[4][k] = (float)((g_dsi_end[4][k] - g_dsi_start4[k]) * t
                                          + g_dsi_start4[k]);
        } else {                /* the camera moves back, still looking along z, after
                                   this frame's objects have used where it was */
            float p[3];
            int k;
            for (k = 0; k < 3; k++)
                p[k] = (float)((g_dsi_end[5][k] - g_dsi_start5[k]) * t + g_dsi_start5[k]);
            r3d_camera_pos(p);
        }
        if (i < 5 && g_dsi[i].nf)
            r3d_object(&g_dsi[i], g_dsi_ang[i], g_dsi_pos[i]);
    }
    if (done > g_dsi_done)
        g_dsi_done = done;
    r3d_render();
    menu_draw_buttons();
    menu_transition();
    menu_end_frame();
    pressed = menu_pressed();
    if (pressed == 0)
        menu_press_done();
    if (pressed == 0 || g_dsi_done == 6) {
        menu_change_page(14);
        menu_capture();         /* after the frame: the glass takes it, pointer and all */
    }
}

void page_logo_end(void)
{
    int i;
    music_stop();
    r3d_free_page(0);
    r3d_free_page(1);
    for (i = 0; i < 5; i++)
        model_free(&g_dsi[i]);
}

/* ---------------------------------------------------------------- 14, the title */

static Model g_mr[3];           /* 0x540338: ellipse, moto, racer */
static float g_mr_prog[3];      /* 0x5420e8 */
static int g_mr_arrived;        /* 0x5409bc */
static unsigned g_mr_last;      /* 0x53f4e0 */
static int g_mr_frames;         /* 0x4e90e0 */
static int g_text_id;           /* 0x4e98ac */

/* 0x446460's tables (the Moto Racer edition's; the GP edition's are not used) */
static const float g_mr_dur[3] = { 1.0f, 1.2f, 1.4f };
static const float g_mr_start[3][3] = { { 60, 42, -40000 }, { -12000, 44, -798 },
                                        { 12000, 25, -798 } };
static const float g_mr_end[3][3] = { { 60, 42, -800 }, { 42, 44, -798 }, { 40, 25, -798 } };

void page_title_start(void)
{
    static const char *const names[3] = { "ellipse", "moto", "racer" };
    static const float scale[3][3] = { { 0.04f, 0.03875f, 0.04f }, { 0.04f, 0.036f, 0.0375f },
                                       { 0.04f, 0.033f, 0.0395f } };
    char path[64];
    int i;
    r3d_load_page(3, "data\\pages\\titleMr\\bnk00_00.raw");
    for (i = 0; i < 3; i++) {
        snprintf(path, sizeof path, "data\\pages\\titleMr\\%s", names[i]);
        model_free(&g_mr[i]);
        if (model_load(&g_mr[i], path)) {
            model_page_add(&g_mr[i], 3);
            model_scale(&g_mr[i], scale[i][0], scale[i][1], scale[i][2]);
        }
    }
    r3d_camera(g_campos, g_origin);
    memset(g_mr_prog, 0, sizeof g_mr_prog);
    g_mr_arrived = 0;
    menu_clear_buttons();
    bitmap_place(PIC(P_START), 0, 430, 1);
    menu_add_button(PIC(P_START), 0, 0xffffffffu, VK_RETURN, -1, 0, 0, 0, -1, -1, 0);
    g_mr_last = g_now;
    /* (the start draws a first picture here, 0x49b0e0, not yet) */
    music_play(3, 2);
    sound_play(0x2d, 0);
}

void page_title_frame(void)
{
    float dt = frame_dt(&g_mr_last);
    int i, arrived = 0;
    r3d_camera(g_campos, g_origin);
    menu_begin_frame();
    if (g_mr_arrived > 2) {
        bitmap_draw(PIC(P_TITLE + 3), BM_OWN, 100, 0);
        /* "FULL D3D VERSION" is added on frames 11 to 20 of every 21: each copy blinks
           500 ms on, 500 off from its own start, and the copies pile up (48 at most) */
        if (++g_mr_frames > 10) {
            Font *f = font_get(31);
            TextArgs a;
            if (f)
                font_set_colors(f, named_colour(COL_YELLOW), named_colour(COL_BLACK));
            memset(&a, 0, sizeof a);
            a.text = "FULL D3D VERSION";
            a.font = 31;
            a.y = 60;
            a.align = 0x22;
            a.effect = FX_BLINK;
            a.blink = -500;
            a.id_out = &g_text_id;
            a.vx = -1, a.vy = 1;
            a.tx = 0, a.ty = 40;
            a.s = 1.5, a.k = 1.0;
            a.own_colours = 1;
            a.fore = named_colour(COL_YELLOW);
            a.back = named_colour(COL_BLACK);
            text_add(&a);
        }
        if (g_mr_frames > 20)
            g_mr_frames = 0;
    } else {
        bitmap_draw(PIC(P_TITLE + g_mr_arrived), BM_OWN, 100, 0);
        for (i = 0; i < 3; i++) {
            float ang[3] = { 0.0f, (float)(PI / 2), 0.0f }, pos[3];
            double t;
            int k;
            g_mr_prog[i] = (float)(g_mr_prog[i] + dt / (g_mr_dur[i] * 1000.0f));
            t = g_mr_prog[i] < 0 ? 0 : g_mr_prog[i] > 1 ? 1 : g_mr_prog[i];
            if (i == 0) {       /* the ellipse: two half-turns about two axes */
                if (t < 0.5) {
                    ang[0] = (float)(2 * PI * t);
                    ang[1] = (float)(-PI / 2);
                    ang[2] = (float)PI;
                } else {
                    ang[1] = (float)(2 * PI * t + PI / 2);
                }
            }
            if (g_mr_prog[i] <= 1.0f)
                for (k = 0; k < 3; k++)
                    pos[k] = (float)((g_mr_end[i][k] - g_mr_start[i][k]) * t + g_mr_start[i][k]);
            else {
                memcpy(pos, g_mr_end[i], sizeof pos);
                arrived++;
            }
            if (g_mr_arrived < i + 1 && g_mr[i].nf)
                r3d_object(&g_mr[i], ang, pos);
        }
        if (arrived > g_mr_arrived)
            g_mr_arrived = arrived;
    }
    menu_draw_buttons();
    menu_transition();
    r3d_render();
    menu_end_frame();
    if (menu_pressed() == 0) {
        menu_press_done();
        menu_change_page(15);   /* the name entry, without the glass */
    }
}

void page_title_end(void)
{
    int i;
    r3d_free_page(3);
    for (i = 0; i < 3; i++)
        model_free(&g_mr[i]);
}
