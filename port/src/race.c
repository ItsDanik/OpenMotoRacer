/* The race (NOTES §4.16): its load (0x47b420), the riders on the starting grid (0x410a50,
   0x47bf70), and its frame (Game_Frame 0x47d660). Written so far: the track, its sky, the
   riders on the grid, the player's bike (bikephys.c) and the chase camera (camera.c),
   the dashboard, the race's rules, the pause menu (Escape). The computer riders stand on
   the grid until their model (0x46bf80) is written. */
#include "race.h"
#include "bikephys.h"
#include "bikes.h"
#include "camera.h"
#include "choices.h"
#include "clock.h"
#include "config.h"
#include "dialog.h"
#include "horizon.h"
#include "hud.h"
#include "input.h"
#include "land.h"
#include "menu.h"
#include "r3d.h"
#include "rules.h"
#include "scores.h"
#include "sound.h"
#include "text.h"
#include "video.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define FOCAL 394.0f            /* 0x6d20c4 in a race at 640x480 (build/trackcap) */

Rider g_rider[RIDERS];
int g_racing;
static int g_player = 1;        /* 0x4ea4c0: the local player's record */
unsigned g_keys[RIDERS];        /* +0x31e9: the control bits (RK_*) */
static BikePhys g_phys[RIDERS]; /* the riders' bikes */
static unsigned g_events;       /* 0x4e8918 */
static int g_finish_timer, g_timeover_timer;    /* 0x4e9824, 0x4e9828 */

/* 0x431250 (WM_KEYDOWN, the repeats of a held key too) and 0x432700 (WM_KEYUP): the keys
   bound to the seven actions (0x53eae0) set and clear the player's bits; pressing one
   way clears the other, as the original's handlers do */
void race_key(int vk, int down)
{
    unsigned *b = &g_keys[g_player];
    int a;
    if (!g_racing)
        return;
    if (dialog_key(vk, down))
        return;
    if (vk == VK_ESCAPE && down) {
        dialog_pause(1);
        return;
    }
    if (vk == VK_TAB || vk == 0x74) {           /* the look back, F5 the distance */
        camera_key(vk, down);
        return;
    }
    if (vk == 0x76) {                           /* F7: the dials */
        if (down)
            hud_key(vk);
        return;
    }
    for (a = 0; a < ACTIONS; a++)
        if (g_binding[a][0] == vk)
            bikephys_key(&g_phys[g_player], b, a, down, g_control);
}

#define STEP_MS 16
static unsigned g_step_at;      /* the clock's time the steps have reached */

static void race_end(int ms);

/* the drawing's copy of a bike: the frame's matrix +0x2e55, the place, the wheels, the
   section, the lean, pitch, steering, suspension and spins, the crash */
static void to_rider(Rider *r, const BikePhys *b)
{
    memcpy(r->m, b->frame, sizeof r->m);
    memcpy(r->t, b->frame + 9, sizeof r->t);
    memcpy(r->pos, b->pos, sizeof r->pos);
    memcpy(r->rear, b->rear, sizeof r->rear);
    memcpy(r->front, b->front, sizeof r->front);
    r->sec = b->sec;
    r->lean = b->lean, r->pitch = b->pitch, r->steer = b->steer, r->speed = b->speed;
    r->dive = b->dive, r->susp = b->susp;
    r->spin_rear = b->spin_rear, r->spin_front = b->spin_front;
    r->crashed = b->crashed, r->roll_side = b->roll_side;
    if (b->crashed) {
        memcpy(r->body, b->tumble, sizeof r->body);
        memcpy(r->body_pt, b->tumble_pt, sizeof r->body_pt);
    }
}

/* what the drawer leaves for a crash to start from (+0x3085, +0x30b5) */
static void from_rider(BikePhys *b, const Rider *r)
{
    if (b->crashed)
        return;
    memcpy(b->tumble, r->body, sizeof r->body);
    memset(b->tumble + 9, 0, 3 * sizeof(float));
    memcpy(b->tumble_pt, r->body_pt, sizeof r->body_pt);
}

/* one step of Game_Frame (0x47d660) of ms milliseconds: the timers, the countdown and
   the time limit, each driven bike's step (0x45d540) and bike against bike (0x47ecd0),
   the race's rules, the camera */
static void step(int ms)
{
    BikePhys *all[RIDERS];
    int k;
    g_control = 1;              /* Game_Frame: the player's keys are read */
    if (g_extended > 0)
        g_extended -= ms;
    rules_time(ms);
    /* 0x4dc150, the frame time smoothed by an eighth */
    g_bikeworld.frame_ms = (float)((ms - (double)g_bikeworld.frame_ms) * 0.125
                                   + g_bikeworld.frame_ms);
    g_bikeworld.countdown = g_counting;
    for (k = 0; k < RIDERS; k++)
        all[k] = &g_phys[k];
    for (k = 1; k < g_riders; k++)      /* 0x47df40: dt = ms x 0.1 */
        if (g_racer[k].loaded && g_phys[k].driven)
            bikephys_step(&g_phys[k], g_keys[k], 0x8000, 0x8000, (float)(ms * 0.1),
                          g_counting, g_time_left);
    bikephys_collide(all, g_riders, (float)(ms * 0.1));
    for (k = 1; k < g_riders; k++)
        to_rider(&g_rider[k], &g_phys[k]);
    rules_laps();
    rules_places();
    race_end(ms);
    camera_step(&g_phys[g_player], (float)(ms * 0.1), g_counting,
                g_racer[g_player].state == RS_FINISHED);
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
        /* 0x411010: player 1's gearbox is the menu's, the others automatic */
        bikephys_init(&g_phys[k], r->bike, (const float (*)[3])r->m, p, s, k == g_player,
                      k == g_player ? g_gearbox : 1);
        g_phys[k].number = k;
        if (k != g_player)
            g_phys[k].driven = 0;               /* (the computer's model: not yet) */
        rules_progress(k);
    }
}

/* 0x411010 and the start 0x47bf70: the riders back on the grid, every sound stopped, the
   race's sound init 0x4084e0, the track's music */
static void start(void)
{
    int k;
    text_clear();
    /* 0x411010: srand(0), the riders' records for bike against bike */
    memset(&g_bikeworld.rider, 0, sizeof g_bikeworld.rider);
    for (k = 1; k < g_riders; k++)
        g_bikeworld.rider[k] = &g_phys[k];
    g_bikeworld.nriders = g_riders;
    g_bikeworld.crt = 0;
    g_bikeworld.reverse = g_reverse ? -1 : 1;
    g_bikeworld.mode = g_mode;
    g_bikeworld.followed = g_player;
    g_bikeworld.replay = 0;
    g_bikeworld.gear_latch = 0;
    g_bikeworld.countdown = 1;
    g_bikeworld.dt = 1.6f;
    g_bikeworld.frame_ms = STEP_MS;
    memset(g_phys, 0, sizeof g_phys);
    grid();
    camera_reset(&g_phys[g_player]);
    sound_stop_all();
    sound_race_load(g_bikes[g_rider[1].bike].perso != 0);
    music_play(g_land.audio_track, 0);
    memset(g_keys, 0, sizeof g_keys);
    bikephys_key(&g_phys[g_player], &g_keys[g_player], 0, 0, 0);     /* 0x4e8964 = 0 */
    rules_restart();
    hud_restart();
    g_finish_timer = g_timeover_timer = -1;
    g_step_at = g_now;
}

int race_player(void)
{
    return g_player;
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
    g_events = 0;
    start();
    g_racing = 1;
    return 1;
}

void race_release_keys(void)
{
    g_keys[g_player] = 0;
    g_phys[g_player].turbo_held = 0;
    bikephys_key(&g_phys[g_player], &g_keys[g_player], 0, 0, 0);
}

void race_event(unsigned bits)
{
    g_events |= bits;
}

/* 0x4580f0, a championship's advance (no points in Solo): the top three go to the next
   race, unlocking what it unlocks; the page after it (page 9 or the podium, 24; 16 and 17
   for the unlocks, 18 at the end); 0 when the championship is over */
static int champ_advance(int *page)
{
    const Racer *r = &g_racer[g_player];
    *page = r->state != RS_FINISHED ? 9 : 24;
    if (r->place <= 2 && g_champ_race < g_nchamp) {
        const ChampRace *c = &g_champ[g_champ_race];
        int i;
        if (c->unlock_pocket) {
            g_pocket_unlocked = c->unlock_pocket;
            *page = 17;
        }
        if (c->unlock_reverse) {
            g_reverse_unlocked = c->unlock_reverse;
            *page = 16;
        }
        for (i = 0; i < g_ntracks; i++)
            if (strcmp(g_tracks[i].name, c->unlock) == 0)
                g_tracks[i].available = 1;
        g_champ_race++;
    }
    if (g_champ_race >= g_nchamp) {
        *page = 18;
        return 0;
    }
    return 1;
}

/* 0x4587e0's part for the race's events */
static int events(void)
{
    unsigned e = g_events;
    int page = -1;
    g_events = 0;
    if (e & EV_RESTART)
        start();
    if (e & EV_UNLOAD)
        race_unload();
    if (e & EV_CONTINUE)
        dialog_continue();
    else if (e & EV_PAUSE_AFTER)
        dialog_pause(0);
    if (e & EV_TOP5) {
        if (g_mode == MODE_CHAMPIONSHIP)
            g_champ_on = champ_advance(&page);
        else
            page = 9;
    }
    if (e & EV_PAGE6)
        page = 6;
    if (page == 16 || page == 17 || page == 18 || page == 24) {
        fprintf(stderr, "openmotoracer: page %d (the championship's unlocks, end and podium) "
                "is not written yet: TOP 5 instead\n", page);
        page = 9;
    }
    if (page >= 0)
        menu_change_page(page);
    return !(e & EV_UNLOAD);
}

/* 0x412a50, the end of the race once a step: 7 s after the player's finish the results
   (practice: the table and the continue dialog) and a new best lap kept; 3 s after the
   time ran out the continue dialog. (The winner's lap, 0x412960, with the computer
   riders.) */
static void race_end(int ms)
{
    if (g_race_over) {
        if (g_finish_timer == -1) {
            g_finish_timer = 7000;
            sound_queue_voice(0x3e, 500);       /* RACECMPL */
        } else if (g_finish_timer > 1) {
            if ((g_finish_timer -= ms) < 0)
                g_finish_timer = 1;
        } else if (g_finish_timer == 1)
            g_finish_timer = 0;
        else {
            /* (0x452090, the podium's data: with page 24) */
            if (g_mode != MODE_PRACTICE)
                dialog_results();
            else {
                /* (the time attack's ghost saved when beaten: with the ghost) */
                scores_write_race();
                dialog_continue();
            }
            if (g_race_best_lap != NO_TIME && g_race_best_lap != 0) {
                Score s;                        /* 0x5d25b0 */
                size_t n = strlen(g_player_name);
                memset(&s, 0x7f, sizeof s);
                memset(s.b, 0, 9);
                memcpy(s.b, g_player_name, n < 8 ? n : 8);
                memcpy(s.b + 9, "TINI", 4);
                s.b[0xd] = 0;
                memcpy(s.b + 0xf5, &g_race_best_lap, 4);
                best_lap_write(&s);
                g_race_best_lap = NO_TIME;
            }
        }
    } else if (g_time_over) {
        if (g_timeover_timer == -1)
            g_timeover_timer = 3000;
        else if (g_timeover_timer > 0) {
            if ((g_timeover_timer -= ms) < 0)
                g_timeover_timer = 0;
        } else if (g_solo)
            dialog_continue();
    }
}

void race_unload(void)
{
    dialog_free();
    text_clear();
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
    const BikePhys *b = &g_phys[g_player];
    float eye[3], c[3][3];
    static const float zero[3];
    int i, y, sec;
    if (g_events && !events())
        return;
    text_clock(g_now);
    dialog_frame();
    if (dialog_holds())
        g_step_at = g_now;
    /* the original steps once a picture by the time it took, and GOG's -FrameRateMax60
       makes every picture 16 ms (1000 / 60 in integers): 62.5 steps a second of dt 1.6.
       The port runs one such step for every 16 ms of the clock. */
    while (g_now - g_step_at >= STEP_MS) {
        g_step_at += STEP_MS;
        step(STEP_MS);
    }
    camera_apply();
    r3d_get_camera(c, eye);
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
        {
            int lod = detail_of(&g_rider[i], eye);
            bike3d_draw_rider(&g_rider[i], lod);
            if (lod < 2)
                from_rider(&g_phys[i], &g_rider[i]);
        }
    hud_bike(b);
    hud_dials_3d();
    r3d_render();
    hud_draw();
    text_update();
    text_draw();                /* the race's messages */
    dialog_draw();
    sound_update();             /* 0x405f30: the voice queue */
}
