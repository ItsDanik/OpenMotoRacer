/* What 0x47ff90 draws over the track besides the dashboard (NOTES §4.13.5): the turn signs
   of 0x486310 and the finishing place's figure of 0x47b7a0. */
#include "signs.h"
#include "config.h"
#include "exe.h"
#include "font.h"
#include "generic.h"
#include "land.h"
#include "r3d.h"
#include "text.h"
#include "video.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ the turn signs */

#define SIGNS 6
#define SIGN_POINTS 64

/* 0x5d2ed0 + i x 0x18: +0 the library entry, +0xc armed, +0x14 its time. The models by the
   event number that arms them (0x486240); 0x473370 finds them by name. */
static const char *const g_sign_name[SIGNS] = {
    "g_45", "d_45", "g_90", "d_90", "g_chica", "d_chica"
};
static struct {
    const Model *model;
    int armed, visible;
    float t;
} g_sign[SIGNS];

/* 0x473370, at the race's start (0x47bf70) */
void signs_start(void)
{
    int i, k;
    memset(g_sign, 0, sizeof g_sign);
    for (k = 0; k < g_land.nlib; k++)
        for (i = 0; i < SIGNS; i++)
            if (strcmp(g_land.lib[k].name, g_sign_name[i]) == 0)
                g_sign[i].model = &g_land.lib[k].m[0];
}

/* 0x486240: a section's event number, 0..5; 16..21 when the race runs backwards. (The
   numbers 6..12 are the ambience's zones.) */
void signs_event(int code, int reverse)
{
    if (reverse)
        code -= 0x10;
    if (code >= 0 && code < SIGNS) {
        g_sign[code].armed = 1;
        g_sign[code].t = 75.0f;
    }
}

/* the clock of 0x486310's loop: the step's dt off the time, showing while the tens of it
   are even (a blink every 100 ms) */
void signs_step(float dt, int kind)
{
    int i;
    for (i = 0; i < SIGNS; i++) {
        g_sign[i].visible = 0;
        if (!g_sign[i].armed)
            continue;
        if (kind == 1 || kind == 2 || !g_display_signal || credits_running()) {
            g_sign[i].t = 0.0f;         /* (still armed: it goes out on the next step) */
            continue;
        }
        g_sign[i].t -= dt;
        if (g_sign[i].t < 0.0f)
            g_sign[i].armed = 0;
        g_sign[i].visible = !((int)(g_sign[i].t * 0.1) & 1);
    }
}

/* 0x486310's slots 1..6: the model flat on the screen around (W - 1 - 320 W/640,
   H - 1 - 400 H/480), its kinds through the detail row (0x4f1a18 stays 0) */
void signs_draw(void)
{
    int i, k;
    float sx = (float)(g_scr_w * 0.0015625), sy = (float)(g_scr_h * (1.0 / 480.0));
    float ox = (float)(g_scr_w - 1) - 320.0f * sx;
    float oy = (float)(g_scr_h - 1) - 400.0f * sy;
    for (i = 0; i < SIGNS; i++) {
        static int pts[SIGN_POINTS][2];
        const Model *m = g_sign[i].model;
        if (!g_sign[i].visible || !m || m->nv > SIGN_POINTS)
            continue;
        for (k = 0; k < m->nv; k++) {
            pts[k][0] = (int)(m->v[k][0] * sx + ox);
            pts[k][1] = (int)(oy - m->v[k][1] * sy);
        }
        r3d_key_faces(r3d_screen_points(m->nv, (const int (*)[2])pts, 2048.0f), m->nf, m->f,
                      15.0f, -1);
    }
}

/* F8 (0x432176): DisplaySignal, and a word about it for two seconds: 0x4188b0 with the id
   0x4e98f0, align 0x22, y 20, fading out, font 12, blinking by 4 pictures */
void signs_key(void)
{
    static int id;
    TextArgs a;
    g_display_signal = !g_display_signal;
    text_remove(id);
    memset(&a, 0, sizeof a);
    a.text = exe_text(g_display_signal ? 181 : 180);
    a.lifetime = 2000;
    a.font = 12;
    a.y = 20;
    a.align = 0x22;
    a.effect = FX_FADE_OUT;
    a.blink = 4;
    a.id_out = &id;
    a.own_colours = 1;
    a.fore = named_colour(COL_RED);
    a.back = named_colour(COL_BLACK);
    a.list = 1;                 /* 0x53ba68 */
    text_add(&a);
}

/* ------------------------------------------------------------------ the place's figure */

/* 0x515ff0 + i x 0x70: 1rst, 2nd, 3rd and the wreath behind them, from the track's FINAL
   folder (0x47b600), each centred */
static Model g_figure[4];
static float g_figure_z = 16000.0f;     /* 0x4f0c3c */
static float g_figure_turn;             /* 0x4f0c40 */
static float g_figure_shown;            /* the wreath's distance this picture */
static int g_figure_place = -1;

void figure_free(void)
{
    int i;
    for (i = 0; i < 4; i++)
        model_free(&g_figure[i]);
}

/* the end of 0x420a50: the faces with the MAP's bit 0x20000 (flag 1) to the front, by its
   exchanges. All of a figure's faces go at one key, so their order is what hides what;
   only the wreaths of ILE03 and ILE06 have such faces. */
static void faces_first(Model *m)
{
    int i, j;
    for (i = 0; i < m->nf - 1; i++)
        if (!(m->f[i].flags & 1))
            for (j = i + 1; j < m->nf; j++)
                if (m->f[j].flags & 1) {
                    Face x = m->f[i];
                    m->f[i] = m->f[j];
                    m->f[j] = x;
                }
}

void figure_load(const char *folder)
{
    static const char *const name[4] = { "1rst", "2nd", "3rd", "Laureat" };
    char f3df[256], fmap[256];
    int i;
    figure_free();
    for (i = 0; i < 4; i++) {
        snprintf(f3df, sizeof f3df, "%sfinal\\%s.3df", folder, name[i]);
        snprintf(fmap, sizeof fmap, "%sfinal\\%s.map", folder, name[i]);
        if (model_load_bike(&g_figure[i], f3df, fmap, 1)) {
            faces_first(&g_figure[i]);
            model_centre(&g_figure[i]);
        } else
            memset(&g_figure[i], 0, sizeof g_figure[i]);
    }
}

/* the end of 0x47bf70 */
void figure_start(void)
{
    g_figure_turn = 0.0f;
    g_figure_z = 16000.0f;
    g_figure_place = -1;
}

/* 0x47b7a0's moves: from far away at 80 a unit to 2500, the wreath then swinging by 128
   about it; going off at 30 a unit once the winner's lap has its credits (0x504eac).
   place: the rider's +0x2f85 when 0x4dc094 is not practice and its state is finished, else
   -1 */
void figure_step(float dt, int place, int leaving)
{
    g_figure_place = place >= 0 && place < 3 ? place : -1;
    if (g_figure_place < 0)
        return;
    g_figure_turn = (float)(dt * 0.015707963267948967 + g_figure_turn);
    if (!leaving) {
        if (g_figure_z > 2500.0f) {
            g_figure_z = dt * -80.0f + g_figure_z;
            if (g_figure_z < 2500.0f)
                g_figure_z = 2500.0f;
        }
        g_figure_shown = g_figure_z;
        if (g_figure_z == 2500.0f)
            g_figure_shown = (float)(sin(g_figure_turn * 4.0f) * 128.0f + g_figure_z);
    } else {
        g_figure_shown = g_figure_z = dt * -30.0f + g_figure_z;
        if (g_figure_z < 0.0f)
            g_figure_z = 0.0f;
    }
}

/* 0x47b7a0's drawing: each model before the camera (the matrices are the view's: angles 0
   from the table, and for the figure the turn about Y), every face at one key through
   detail row 3 */
void figure_draw(void)
{
    float c0, s0, ct, st, n[3][3], t[3];
    const Model *m;
    int i;
    if (g_figure_place < 0 || g_figure_z <= 768.0f || !g_figure[g_figure_place].nf)
        return;
    c0 = g_cos[0], s0 = g_cos[0x3000];
    m = &g_figure[3];
    n[0][0] = c0 * c0;
    n[0][1] = (s0 + 1.0f) * s0 * c0;
    n[0][2] = (s0 - n[0][0]) * s0;
    n[1][0] = -(s0 * c0);
    n[1][1] = n[0][0] - s0 * s0 * s0;
    n[1][2] = n[0][1];
    n[2][0] = s0;
    n[2][1] = n[1][0];
    n[2][2] = n[0][0];
    t[0] = t[1] = 0.0f;
    t[2] = g_figure_shown;
    if (m->nf)
        r3d_key_faces(r3d_points(m->nv, (const float (*)[3])m->v, (const float (*)[3])n, t),
                      m->nf, m->f, 16.0f, 3);
    i = (int)(g_figure_turn * 2607.594587618272);
    ct = g_cos[i & 0x3fff];
    st = g_cos[(i - 0x1000) & 0x3fff];
    m = &g_figure[g_figure_place];
    n[0][0] = c0 * ct;
    n[0][1] = c0 * (st + 1.0f) * s0;
    n[0][2] = s0 * s0 - c0 * c0 * st;
    n[1][0] = -(ct * s0);
    n[1][1] = c0 * c0 - s0 * s0 * st;
    n[1][2] = (st + 1.0f) * s0 * c0;
    n[2][0] = st;
    n[2][1] = n[1][0];
    n[2][2] = n[0][0];
    t[2] = g_figure_z;
    r3d_key_faces(r3d_points(m->nv, (const float (*)[3])m->v, (const float (*)[3])n, t),
                  m->nf, m->f, 13.0f, 3);
}
