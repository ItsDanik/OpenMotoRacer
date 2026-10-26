/* Checks port/src/cpu.c (the computer riders, 0x46bf80) against frames recorded from the
   running original by tools/win/cpuwatch.c: from the recorded state of frame k, one step of
   each rider, compared bit for bit with the recorded frame k+1.
     cpucheck CARAC COSTAB SECTIONS REC [RIDER]
       CARAC     the carac table, 8 rows of 0x124 bytes (a dump of 0x509278)
       COSTAB    the game's cosine table (a dump of 0x5fcb80)
       SECTIONS  the track's sections, 0xfc bytes each (a dump of *0x52c108)
       REC       cpuwatch's recording
   Each rider is checked alone: the riders before it in the frame's order at their frame
   k+1 state, the others at frame k (the step reads the others' lanes, sections and speeds),
   the C library's rand() seed as the riders before it left it in the chained pass.
   Then the whole frame: every rider stepped in order, bike against bike (0x47ecd0) and the
   section walk of 0x47e3d0, against frame k+1 (all but the front wheel point, which the
   drawing rewrites for the riders drawn in detail).
   Build and run: tools/cpucheck.sh. */
#include "bikephys.h"
#include "bikephys_int.h"
#include "bikes.h"
#include "cpu.h"
#include "land.h"
#include "r3d.h"
#include "rand.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W g_bikeworld

Bike g_bikes[BIKES];
Land g_land;
float g_cos[16384];

int rand_next(int stream)
{
    (void)stream;
    return 0;
}

static unsigned char *slurp(const char *path, long *size)
{
    FILE *f = fopen(path, "rb");
    unsigned char *d;
    long n;
    if (!f)
        return 0;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    d = malloc((size_t)n + 1);
    if (d && fread(d, 1, (size_t)n, f) != (size_t)n) {
        free(d);
        d = 0;
    }
    fclose(f);
    *size = n;
    return d;
}

static float gf(const unsigned char *o, int off)
{
    float f;
    memcpy(&f, o + off, 4);
    return f;
}

static int gi(const unsigned char *o, int off)
{
    int i;
    memcpy(&i, o + off, 4);
    return i;
}

/* ------------------------------------------------------------------ the recording */

#define OBJ 0x3209
#define LO 0x4e0
#define HI0 0x2d90
#define RIDER (LO + OBJ - HI0)
#define FSIZE (4 + 4 + 64 + 4 + 24 * RIDER)

typedef struct {
    unsigned stamp, seed;
    int ms, countdown, running, nriders, dir, rev, mode, followed, replay, nsec, opp, diff;
    float frame_ms, dt;
    unsigned char *o[25];               /* rider k's record (the bytes recorded) */
} Frame;

static int load_frames(const unsigned char *d, long size, Frame **out)
{
    int n = (int)(size / FSIZE), k, i;
    Frame *f = calloc((size_t)n, sizeof *f);
    for (k = 0; k < n; k++) {
        const unsigned char *r = d + (long)k * FSIZE, *g = r + 8;
        Frame *x = &f[k];
        memcpy(&x->stamp, r + 4, 4);
        x->ms = gi(g, 4), x->countdown = gi(g, 8), x->running = gi(g, 12);
        x->nriders = gi(g, 16), x->dir = gi(g, 20), x->rev = gi(g, 24), x->mode = gi(g, 28);
        x->followed = gi(g, 32), x->replay = gi(g, 36), x->frame_ms = gf(g, 40);
        x->nsec = gi(g, 44), x->dt = gf(g, 52), x->opp = gi(g, 56), x->diff = gi(g, 60);
        memcpy(&x->seed, r + 72, 4);
        for (i = 1; i <= 24; i++) {
            const unsigned char *s = r + 76 + (i - 1) * RIDER;
            x->o[i] = calloc(1, OBJ);
            memcpy(x->o[i], s, LO);
            memcpy(x->o[i] + HI0, s + LO, OBJ - HI0);
        }
    }
    *out = f;
    return n;
}

#define LF(f, off) b->f = gf(o, off)
#define LI(f, off) b->f = gi(o, off)
#define LV(f, off, n) for (i = 0; i < n; i++) b->f[i] = gf(o, (off) + 4 * i)

static void load_obj(BikePhys *b, const unsigned char *o)
{
    int i;
    memset(b, 0, sizeof *b);
    LI(driven, 0x08); LI(human, 0x04); LI(forced, 0x0c); LI(forced_keys, 0x10);
    LF(radius, 0x18); LI(bike, 0x78); LI(number, 0x2dd0); LI(state, 0x2de5);
    LV(pos, 0x30, 3); LV(pos1, 0x3c, 3); LV(pos2, 0x48, 3); LF(pos_dt, 0x54);
    LI(gear, 0x7c); LI(shift_timer, 0x80); LI(shift, 0x84); LI(automatic, 0x88);
    LI(pending, 0x8c); LV(table, 0x90, 7); LI(brakes_grip, 0xac); LI(brake_ms, 0xb0);
    LF(top, 0xb4); LF(power, 0xb8); LV(m, 0xbc, 12); LF(engine, 0xec); LF(air, 0xf0);
    LF(half, 0xf4); LF(accel, 0xf8); LV(move, 0xfc, 3); LF(steer_rate, 0x10c);
    LF(steer, 0x110); LI(kmh_shown, 0x114); LI(kmh, 0x118); LI(rpm, 0x11c);
    LF(cpu_accel, 0x120); LF(speed, 0x124); LF(speed_in, 0x128); LF(wheel, 0x12c);
    LV(dir, 0x130, 3); LI(slide, 0x13c); LV(m1, 0x140, 12); LV(push_dir, 0x174, 3);
    LF(push, 0x180); LV(skid_w, 0x184, 2); LV(vdir, 0x18c, 3); LV(fwd, 0x198, 3);
    LF(cpu_speed, 0x1a4);
    LV(front, 0x1b4, 3); LV(front1, 0x1c0, 3); LV(front2, 0x1cc, 3); LF(front_air, 0x274);
    LV(rear, 0x338, 3); LV(rear1, 0x344, 3); LV(rear2, 0x350, 3); LF(rear_air, 0x3f8);
    LV(third, 0x4bc, 3); LV(third1, 0x4c8, 3); LV(third2, 0x4d4, 3);
    LI(sliding, 0x2da0); LI(sliding1, 0x2da4); LF(lon0, 0x2da8); LI(events, 0x2dac);
    LF(rear_h, 0x2db0); LI(ground, 0x2db8); LI(ground1, 0x2dbc); LI(rear_touch, 0x2dc0);
    LI(front_touch, 0x2dc4); LI(keys1, 0x2dc8); LF(lean, 0x2e1d); LF(lean1, 0x2e21);
    LI(analog, 0x2e29); LF(wall_spin, 0x2e31); LF(dive, 0x2e39); LF(spin_rear, 0x2e3d);
    LF(spin_front, 0x2e49); LV(frame, 0x2e55, 12); LI(turbo_req, 0x2e85); LF(pitch, 0x2e8d);
    LF(susp, 0x2e91); LF(kick, 0x2e95); LF(phase, 0x2e99); LF(amp, 0x2e9d);
    LF(pitch_s, 0x2ea1); LF(brake_dive, 0x2ea5); LF(wheelie, 0x307d); LF(turbo_time, 0x3081);
    LV(tumble, 0x3085, 12); LV(tumble_pt, 0x30b5, 3); LI(crashed, 0x30c1);
    LF(crash_timer, 0x30c5); LV(tumble_dir, 0x30c9, 3); LF(tumble_speed, 0x30d5);
    LF(tumble_speed0, 0x30d9); LF(roll_side, 0x30dd); LV(roll, 0x30e1, 3); LI(detail, 0x319d);
    LV(crash_v, 0x31a1, 3); LI(crash_flag, 0x31ad); LI(sec, 0x31c1); LV(wall_n, 0x31c5, 3);
    LF(hit_timer, 0x31d1); LF(lane_x, 0x31d5); LI(lane, 0x31d9); LI(turbo_held, 0x31f9);
    LI(turbo_pressed, 0x3201); LI(turbo_pressed1, 0x3205);
}

/* the fields compared: name, offset, count; the BikePhys member through store() */
typedef struct {
    const char *name;
    int off, n;
    long ok, all, firstbad;
} Field;

static Field s_f[] = {
    { "pos +0x30", 0x30, 3 }, { "matrix +0xbc", 0xbc, 9 }, { "vdir +0x18c", 0x18c, 3 },
    { "fwd +0x198", 0x198, 3 }, { "engine (speed) +0xec", 0xec, 1 },
    { "cpu_speed +0x1a4", 0x1a4, 1 }, { "speed +0x124", 0x124, 1 },
    { "section +0x31c1", 0x31c1, 1 }, { "lane +0x31d9", 0x31d9, 1 },
    { "lane_x +0x31d5", 0x31d5, 1 }, { "steer +0x110", 0x110, 1 },
    { "rear +0x338", 0x338, 3 }, { "front +0x1b4", 0x1b4, 3 },
    { "frame +0x2e55", 0x2e55, 9 }, { "lean +0x2e1d/21", 0x2e1d, 2 },
    { "spins +0x2e3d", 0x2e3d, 1 }, { "spins +0x2e49", 0x2e49, 1 },
    { "spring +0x2e8d..a1", 0x2e8d, 6 }, { "hit timer +0x31d1", 0x31d1, 1 },
    { "km/h +0x114/118", 0x114, 2 }, { "rpm +0x11c", 0x11c, 1 }, { "accel +0x120", 0x120, 1 },
    { "gear +0x7c", 0x7c, 1 }, { "turbo +0x2e85", 0x2e85, 1 }, { "wheelie +0x307d/81", 0x307d, 2 },
    { "ground +0x2db8", 0x2db8, 1 }, { "events +0x2dac", 0x2dac, 1 }, { "keys +0x2dc8", 0x2dc8, 1 },
    { "crash +0x30c1", 0x30c1, 1 },
};
#define NF ((int)(sizeof s_f / sizeof s_f[0]))

static void store(unsigned char *o, const BikePhys *b)
{
#define SF(f, off) memcpy(o + (off), &b->f, 4)
#define SV(f, off, n) memcpy(o + (off), b->f, 4 * (n))
    SV(pos, 0x30, 3); SV(m, 0xbc, 9); SV(vdir, 0x18c, 3); SV(fwd, 0x198, 3); SF(engine, 0xec);
    SF(cpu_speed, 0x1a4); SF(speed, 0x124); SF(sec, 0x31c1); SF(lane, 0x31d9);
    SF(lane_x, 0x31d5); SF(steer, 0x110); SV(rear, 0x338, 3); SV(front, 0x1b4, 3);
    SV(frame, 0x2e55, 9); SF(lean, 0x2e1d); SF(lean1, 0x2e21); SF(spin_rear, 0x2e3d);
    SF(spin_front, 0x2e49); SF(pitch, 0x2e8d); SF(susp, 0x2e91); SF(kick, 0x2e95);
    SF(phase, 0x2e99); SF(amp, 0x2e9d); SF(pitch_s, 0x2ea1); SF(hit_timer, 0x31d1);
    SF(kmh_shown, 0x114); SF(kmh, 0x118); SF(rpm, 0x11c); SF(cpu_accel, 0x120); SF(gear, 0x7c);
    SF(turbo_req, 0x2e85); SF(wheelie, 0x307d); SF(turbo_time, 0x3081); SF(ground, 0x2db8);
    SF(events, 0x2dac); SF(keys1, 0x2dc8); SF(crashed, 0x30c1);
}

static BikePhys s_b[25];
static unsigned char s_mine[OBJ];

static void world(const Frame *f1)
{
    int i;
    W.dt = f1->dt;
    W.countdown = f1->countdown;
    W.reverse = f1->dir;
    W.frame_ms = f1->frame_ms;
    W.mode = f1->mode, W.followed = f1->followed, W.replay = f1->replay;
    W.nriders = f1->nriders;
    W.snap = 0;
    memset(W.rider, 0, sizeof W.rider);
    for (i = 1; i < f1->nriders && i < 25; i++)
        W.rider[i] = &s_b[i];
}

int main(int argc, char **argv)
{
    long size;
    unsigned char *d;
    Frame *fr;
    int n, i, k, only = argc > 5 ? atoi(argv[5]) : 0;
    long pairs = 0, skipped = 0, steps = 0, exact = 0, seed_ok = 0, seed_n = 0, crashes = 0;
    long chain_ok = 0, frames_ok = 0, frame_steps = 0, frame_steps_ok = 0, nofront = 0;
    if (argc < 5) {
        fprintf(stderr, "usage: cpucheck CARAC COSTAB SECTIONS REC [RIDER]\n");
        return 2;
    }
    if (!(d = slurp(argv[1], &size)) || size < 8 * 0x124)
        return fprintf(stderr, "no carac table\n"), 1;
    for (i = 0; i < 8; i++) {
        const unsigned char *c = d + i * 0x124;
        memcpy(&g_bikes[i].type, c, 4);
        memcpy(g_bikes[i].gfx, c + 4, 16);
        memcpy(g_bikes[i].phys, c + 0x14, sizeof g_bikes[i].phys);
        memcpy(&g_bikes[i].perso, c + 0x120, 4);
    }
    free(d);
    if (!(d = slurp(argv[2], &size)) || size < 0x10000)
        return fprintf(stderr, "no cosine table\n"), 1;
    memcpy(g_cos, d, 0x10000);
    free(d);
    if (!(d = slurp(argv[3], &size)) || size < 0xfc)
        return fprintf(stderr, "no sections\n"), 1;
    g_land.nsec = (int)(size / 0xfc);
    g_land.sec = calloc((size_t)g_land.nsec, sizeof *g_land.sec);
    for (i = 0; i < g_land.nsec; i++) {
        const unsigned char *s = d + i * 0xfc;
        Section *l = &g_land.sec[i];
        memcpy(l->pos, s, 12);
        memcpy(l->axes, s + 0x0c, 36);
        memcpy(l->plane, s + 0x40, 64);
        l->line = gf(s, 0xb0);
        l->edge[0] = gf(s, 0xf0), l->edge[1] = gf(s, 0xf4);
    }
    bikephys_track();
    free(d);
    if (!(d = slurp(argv[4], &size)))
        return fprintf(stderr, "no recording\n"), 1;
    n = load_frames(d, size, &fr);
    printf("%s: %d frames, %d riders, track %d sections, demo %d\n", argv[4], n,
           n ? fr[0].nriders - 1 : 0, g_land.nsec, n ? fr[0].replay : 0);
    for (k = 0; k + 1 < n; k++) {
        Frame *f0 = &fr[k], *f1 = &fr[k + 1];
        int r, nr = f1->nriders, ok_all = 1;
        unsigned crt;
        for (r = 1; r < nr; r++)        /* one step between the two? */
            if (memcmp(f1->o[r] + 0x3c, f0->o[r] + 0x30, 12))
                break;
        if (r < nr) {
            skipped++;
            continue;
        }
        pairs++;
        /* the chained pass: every rider stepped in order from frame k, for the seed */
        world(f1);
        for (r = 1; r < nr; r++)
            load_obj(&s_b[r], f0->o[r]);
        W.crt = f0->seed;
        for (r = 1; r < nr; r++) {
            unsigned before = W.crt;
            BikePhys keep[25];
            /* alone: riders before r at frame k+1, the rest at frame k */
            memcpy(keep, s_b, sizeof keep);
            for (i = 1; i < nr; i++)
                load_obj(&s_b[i], i < r ? f1->o[i] : f0->o[i]);
            W.crt = before;
            crt = before;
            cpu_step(&s_b[r], f1->dt, f1->countdown);
            crt = W.crt;
            if (gi(f0->o[r], 0x30c1) || gi(f1->o[r], 0x30c1))
                crashes++;
            if (!only || only == r) {
                int good = 1;
                memcpy(s_mine, f1->o[r], OBJ);
                store(s_mine, &s_b[r]);
                steps++;
                for (i = 0; i < NF; i++) {
                    Field *F = &s_f[i];
                    int ok = !memcmp(s_mine + F->off, f1->o[r] + F->off, 4 * (size_t)F->n);
                    F->all++;
                    F->ok += ok;
                    if (!ok && F->firstbad == 0)
                        F->firstbad = k * 100 + r + 1;
                    good &= ok;
                    if (!ok && getenv("CPULIST") && strstr(F->name, getenv("CPULIST")))
                        printf("differs: frame %d rider %d %s\n", k, r, F->name);
                    if (!ok && getenv("CPUDBG") && atoi(getenv("CPUDBG")) == k * 100 + r) {
                        int j;
                        printf("frame %d rider %d %s:", k, r, F->name);
                        for (j = 0; j < F->n; j++)
                            printf(" %.9g/%.9g", gf(s_mine, F->off + 4 * j),
                                   gf(f1->o[r], F->off + 4 * j));
                        printf("  (int %d/%d)\n", gi(s_mine, F->off), gi(f1->o[r], F->off));
                    }
                }
                exact += good;
                {
                    int nf = 1;
                    for (i = 0; i < NF; i++)
                        if (s_f[i].off != 0x1b4)
                            nf &= !memcmp(s_mine + s_f[i].off, f1->o[r] + s_f[i].off,
                                          4 * (size_t)s_f[i].n);
                    nofront += nf;
                }
                ok_all &= good;
            }
            /* back to the chain: this rider's own result, the rest as before */
            {
                BikePhys mine = s_b[r];
                memcpy(s_b, keep, sizeof keep);
                s_b[r] = mine;
            }
            W.crt = crt;
        }
        chain_ok += ok_all;
        /* the whole frame: every rider stepped in order from frame k, then bike against
           bike (0x47ecd0), compared with frame k+1 (all but the front point) */
        {
            BikePhys *all[25];
            int good = 1;
            world(f1);
            for (r = 1; r < nr; r++)
                load_obj(&s_b[r], f0->o[r]);
            W.crt = f0->seed;
            for (r = 1; r < nr; r++)
                cpu_step(&s_b[r], f1->dt, f1->countdown);
            for (r = 0; r < 25; r++)
                all[r] = &s_b[r];
            bikephys_collide(all, nr, f1->dt);
            for (r = 1; r < nr; r++)        /* 0x47e3d0 walks the section again */
                s_b[r].sec = bp_walk(s_b[r].sec, s_b[r].pos);
            for (r = 1; r < nr; r++) {
                int rg = 1;
                memcpy(s_mine, f1->o[r], OBJ);
                store(s_mine, &s_b[r]);
                for (i = 0; i < NF; i++)
                    if (s_f[i].off != 0x1b4) {
                        int ok = !memcmp(s_mine + s_f[i].off, f1->o[r] + s_f[i].off,
                                         4 * (size_t)s_f[i].n), j;
                        rg &= ok;
                        if (!ok && getenv("CPUFRAME") && atoi(getenv("CPUFRAME")) == k) {
                            printf("  frame %d rider %d %s:", k, r, s_f[i].name);
                            for (j = 0; j < s_f[i].n; j++)
                                printf(" %.9g/%.9g", gf(s_mine, s_f[i].off + 4 * j),
                                       gf(f1->o[r], s_f[i].off + 4 * j));
                            printf("  (int %d/%d)\n", gi(s_mine, s_f[i].off),
                                   gi(f1->o[r], s_f[i].off));
                        }
                    }
                if (!rg && getenv("CPUFRAME"))
                    printf("frame %d rider %d differs after the whole frame\n", k, r);
                good &= rg;
                frame_steps_ok += rg;
                frame_steps++;
            }
            frames_ok += good;
            seed_n++;
            seed_ok += W.crt == f1->seed;
            if (getenv("CPUSEED") && W.crt != f1->seed)
                printf("frame %d: seed %08x, recorded %08x\n", k, W.crt, f1->seed);
        }
    }
    printf("  %ld pairs of frames one step apart, %ld skipped (two steps or none)\n", pairs,
           skipped);
    printf("  rider steps checked: %ld, every field below bit-exact: %ld (crashed: %ld)\n",
           steps, exact, crashes);
    printf("  the same but the front wheel point (rewritten after the step): %ld of %ld\n",
           nofront, steps);
    printf("  frames with every rider's step exact (front point too): %ld of %ld\n",
           chain_ok, pairs);
    printf("  whole frames (all riders in order, then bike against bike 0x47ecd0), all but\n"
           "  the front point: %ld of %ld frames, %ld of %ld rider states\n", frames_ok,
           pairs, frame_steps_ok, frame_steps);
    printf("  rand() seed after the whole frame = the recorded one: %ld of %ld frames\n",
           seed_ok, seed_n);
    for (i = 0; i < NF; i++)
        printf("    %-22s %6ld of %6ld%s", s_f[i].name, s_f[i].ok, s_f[i].all,
               s_f[i].firstbad ? "" : "\n"),
        s_f[i].firstbad ? printf("   (first: frame %ld rider %ld)\n", (s_f[i].firstbad - 1) / 100,
                                 (s_f[i].firstbad - 1) % 100) : 0;
    return 0;
}
