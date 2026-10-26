/* The race (NOTES §4.16): its load (0x47b420), the riders on the starting grid (0x410a50,
   0x47bf70), and its frame (Game_Frame 0x47d660). Written so far: the track, its sky, the
   riders standing on the grid, Escape back to the menu. The physics, the chase camera,
   the dashboard and the race's rules are being mapped; until then the camera is a
   stand-in that looks over the player's shoulder. */
#include "race.h"
#include "bikes.h"
#include "choices.h"
#include "clock.h"
#include "config.h"
#include "horizon.h"
#include "input.h"
#include "land.h"
#include "menu.h"
#include "r3d.h"
#include "rules.h"
#include "sound.h"
#include "video.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define FOCAL 394.0f            /* 0x6d20c4 in a race at 640x480 (build/trackcap) */

Rider g_rider[RIDERS];
int g_racing;
static int g_player = 1;        /* 0x4ea4c0: the local player's record */
unsigned g_keys[RIDERS];        /* +0x31e9: the control bits (RK_*) */
int g_turbo[RIDERS], g_turbo_hit[RIDERS];       /* +0x31f9, +0x3201 */
static int g_accel_held;        /* 0x4e8964 */

/* 0x431250 (WM_KEYDOWN, the repeats of a held key too) and 0x432700 (WM_KEYUP): the keys
   bound to the seven actions (0x53eae0) set and clear the player's bits; pressing one
   way clears the other, as the original's handlers do */
void race_key(int vk, int down)
{
    unsigned *b = &g_keys[g_player];
    int a;
    if (!g_racing)
        return;
    for (a = 0; a < ACTIONS; a++) {
        if (g_binding[a][0] != vk)
            continue;
        switch (a) {
        case 0:                                 /* accelerate */
            g_accel_held = down;
            if (down)
                *b = (*b & ~RK_BRAKE) | RK_ACCEL;
            else
                *b &= ~RK_ACCEL;
            break;
        case 1:                                 /* brake */
            if (down) {
                if (g_control)
                    *b = (*b & ~RK_ACCEL) | RK_BRAKE;
            } else {
                *b &= ~RK_BRAKE;
                if (g_accel_held)
                    *b |= RK_ACCEL;
            }
            break;
        case 2:                                 /* left */
        case 3:                                 /* right */
            if (!down)
                *b &= a == 2 ? ~RK_LEFT : ~RK_RIGHT;
            else if (g_control)
                *b = (*b & (a == 2 ? ~RK_RIGHT : ~RK_LEFT)) | (a == 2 ? RK_LEFT : RK_RIGHT);
            break;
        case 4:                                 /* gear up (0x4dc110 = 1) */
        case 5:                                 /* gear down */
            if (!down)
                *b &= a == 4 ? ~RK_GEARUP : ~RK_GEARDOWN;
            else if (!g_counting && g_control)
                *b |= a == 4 ? RK_GEARUP : RK_GEARDOWN;
            break;
        case 6:                                 /* turbo */
            if (!down)
                g_turbo[g_player] = 0;
            else if (g_control)
                g_turbo[g_player] = g_turbo_hit[g_player] = 1;
            break;
        }
    }
}

#define STEP_MS 16
static unsigned g_step_at;      /* the clock's time the steps have reached */

/* one step of Game_Frame (0x47d660) of ms milliseconds: the timers, the countdown and
   the time limit, (the riders' steps and the physics: not yet), the race's rules */
static void step(int ms)
{
    g_control = 1;              /* Game_Frame: the player's keys are read */
    if (g_extended > 0)
        g_extended -= ms;
    rules_time(ms);
    /* 0x47df40: dt = ms x 0.1, each rider's step (0x45d540); 0x47ecd0 */
    rules_laps();
    rules_places();
}

/* 0x40ec30: a section number brought into 0 .. count - 1 */
static int wrap(int s)
{
    while (s < 0)
        s += g_land.nsec;
    while (s >= g_land.nsec)
        s -= g_land.nsec;
    return s;
}

/* 0x46d610: a x b */
static void cross(float o[3], const float a[3], const float b[3])
{
    float r[3];
    r[0] = a[1] * b[2] - a[2] * b[1];
    r[1] = a[2] * b[0] - a[0] * b[2];
    r[2] = a[0] * b[1] - a[1] * b[0];
    memcpy(o, r, sizeof r);
}

static void unit(float v[3])    /* 0x40fcd0 */
{
    float l = (float)sqrt((double)v[0] * v[0] + (double)v[1] * v[1] + (double)v[2] * v[2]);
    if (l != 0.0f) {
        double k = 1.0 / l;     /* kept at the FPU's width */
        v[0] = (float)(v[0] * k), v[1] = (float)(v[1] * k), v[2] = (float)(v[2] * k);
    }
}

/* 0x47bf70, the Solo branch: from the section 3 x (riders + 1) ahead of the start, each
   rider 3 sections further back, the last record first, so the player (record 1) starts
   at the back; 10 along the section, alternately 128 right and left (not in practice,
   where they all share one section) */
static void grid(void)
{
    int dir = g_reverse ? -1 : 1;               /* 0x4dc140 */
    int s = wrap(dir * g_riders * 3), k;
    if (g_reverse)
        s = wrap(s - dir * 3);
    for (k = g_riders - 1; k >= 1; k--) {
        Rider *r = &g_rider[k];
        const Section *sc;
        float p[3];
        int i;
        if (g_mode != 0)
            s = wrap(s - dir * 3);
        sc = &g_land.sec[s];
        for (i = 0; i < 3; i++)                 /* (0, 0, 10) through +0xc, then +0 */
            p[i] = (float)(10.0 * sc->axes[2][i]) + sc->pos[i];
        if (g_mode != 0) {
            double off = k & 1 ? -128.0 : 128.0;
            for (i = 0; i < 3; i++)
                p[i] = (float)(off * sc->axes[0][i] + p[i]);
        }
        /* 0x45bdf0: the rider's state cleared (nothing the drawing reads yet) */
        if (!g_reverse) {
            memcpy(r->m, sc->axes, sizeof r->m);
            memset(r->t, 0, sizeof r->t);
        } else {                /* facing back: z = T - axes[2] (T is 0), x = up x z, y = z x x */
            static const float up[3] = { 0.0f, 1.0f, 0.0f };
            float z[3], x[3], y[3];
            for (i = 0; i < 3; i++)
                z[i] = -sc->axes[2][i];
            unit(z);
            cross(x, up, z);
            cross(y, z, x);
            unit(x);
            unit(y);
            memcpy(r->m[0], x, sizeof x);
            memcpy(r->m[1], y, sizeof y);
            memcpy(r->m[2], z, sizeof z);
            memset(r->t, 0, sizeof r->t);
        }
        bike3d_place(r, p);
        r->sec = s;                             /* +0x31c1 */
        rules_progress(k);
    }
}

int race_load(char *err, int errsize)
{
    int k, opponents;
    if (!land_load(g_track, err, errsize))
        return 0;
    if (!bikes_load(g_track) || !bike3d_load(g_land.bank1_rgb)) {
        snprintf(err, (size_t)errsize, "The bikes of %s could not be read.",
                 g_tracks[g_track].name);
        land_free();
        return 0;
    }
    /* 0x47b420: practice 1, else 23 opponents, or 11 with "OPPONENTS: 12"; 0x410a50 */
    opponents = g_mode == 0 ? 1 : g_opponents ? 11 : 23;
    g_riders = 2 + opponents;
    memset(g_rider, 0, sizeof g_rider);
    g_player = 1;
    g_rider[1].bike = g_bike >= 0 && g_bike < BIKES ? g_bike : 0;
    g_rider[1].skin = g_rider[1].bike & 7;
    for (k = 2; k < g_riders; k++) {
        /* +0x78 = 0; the liveries in turn, the player's own given to the one that would
           have had livery 0 */
        g_rider[k].bike = 0;
        g_rider[k].skin = (k - 1 == g_rider[1].bike ? 0 : k - 1) & 7;
    }
    memset(g_racer, 0, sizeof g_racer);
    for (k = 1; k < g_riders; k++)
        g_racer[k].loaded = 1;
    if (g_mode == MODE_PRACTICE)
        g_racer[2].loaded = 0;  /* the second record: the time attack's ghost [guess] */
    g_racer[g_player].human = 1;
    grid();
    /* 0x410d70 (the race's sounds), then at the start 0x47bf70: every sound stopped, the
       race's sound init 0x4084e0, the track's music */
    sound_stop_all();
    sound_race_load(g_bikes[g_rider[1].bike].perso != 0);
    music_play(g_land.audio_track, 0);
    memset(g_keys, 0, sizeof g_keys);
    memset(g_turbo, 0, sizeof g_turbo);
    memset(g_turbo_hit, 0, sizeof g_turbo_hit);
    g_accel_held = 0;
    rules_restart();
    g_step_at = g_now;
    g_racing = 1;
    return 1;
}

void race_unload(void)
{
    sound_race_free();
    music_stop();
    bike3d_free();
    land_free();
    r3d_race(-1);
    r3d_focal(1911.0f);
    g_racing = 0;
}

/* 0x45f160: the detail by the distance (the zoom 0x6d1f74 at 1) */
static int detail_of(const Rider *r, const float cam[3])
{
    double dx = r->pos[0] - cam[0], dy = r->pos[1] - cam[1], dz = r->pos[2] - cam[2];
    double d = sqrt(dx * dx + dy * dy + dz * dz);
    return d >= 3000.0 ? 4 : d >= 2500.0 ? 3 : d < 1000.0 ? 0 : 2;
}

void race_frame(void)
{
    const Rider *me = &g_rider[g_player];
    float eye[3], at[3], c[3][3], pos[3];
    static const float zero[3];
    int i, y, sec;
    if (input_pressed(VK_ESCAPE)) {
        race_unload();
        menu_change_page(3);
        return;
    }
    /* the original steps once a picture by the time it took, and GOG's -FrameRateMax60
       makes every picture 16 ms (1000 / 60 in integers): 62.5 steps a second of dt 1.6.
       The port runs one such step for every 16 ms of the clock. */
    while (g_now - g_step_at >= STEP_MS) {
        g_step_at += STEP_MS;
        step(STEP_MS);
    }
    /* the stand-in camera: 700 behind the bike, 260 above, looking at its seat */
    for (i = 0; i < 3; i++) {
        eye[i] = me->pos[i] - 700.0f * me->m[2][i] + 260.0f * me->m[1][i];
        at[i] = me->pos[i] + 120.0f * me->m[1][i];
    }
    r3d_camera(eye, at);
    r3d_get_camera(c, pos);
    sec = land_section(eye, me->sec);
    for (y = 0; y < g_scr_h; y++)
        memset(g_screen + y * g_pitch, 0, sizeof(uint16_t) * (size_t)g_scr_w);
    horizon_draw((const float (*)[3])c, zero, eye[1], g_land.horizon_offset,
                 g_land.horizon_scale, FOCAL);
    r3d_race(g_detail);
    r3d_focal(FOCAL);
    land_draw(sec, (const float (*)[3])c, eye);
    for (i = 1; i < g_riders; i++)
        if (g_mode != 0 || i == g_player)       /* (practice's second record: unseen) */
            bike3d_draw_rider(&g_rider[i], detail_of(&g_rider[i], eye));
    r3d_render();
    rules_countdown_sound();    /* (the dashboard's, once a picture) */
    sound_update();             /* 0x405f30: the voice queue */
}
