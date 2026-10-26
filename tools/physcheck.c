/* Checks port/src/bikephys.c against states recorded from the running original
   (tools/win/memwatch.c, build/phys/w*.bin): one step from recorded state k, compared bit
   for bit with recorded state k+1. Build and run: tools/physcheck.sh.

   physcheck CARAC COSTAB SECTIONS REC...
     CARAC     one carac row, 0x124 bytes (tools/physprep.py)
     COSTAB    the game's cosine table, 16384 floats (a dump of 0x5fcb80)
     SECTIONS  the track's sections as the game keeps them, 0xfc bytes each (a dump)
   It includes bikephys.c to reach its pieces; the rest of the port is stubbed. */
#include "../port/src/bikephys.c"

#include <stdio.h>

Bike g_bikes[BIKES];
Land g_land;
float g_cos[16384];
static int s_draws;

int rand_next(int stream)
{
    (void)stream;
    s_draws++;
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

static int same(float a, float b)
{
    return memcmp(&a, &b, 4) == 0;
}

/* ------------------------------------------------------------------ the recordings */

#define OBJ 0x3209
typedef struct {
    float dt;
    int ms, countdown, running;
    const unsigned char *o;
} Rec;

static int load_rec(const unsigned char *d, long size, Rec *out)
{
    long rs = 24 + OBJ, k;
    unsigned last = 0;
    int n = 0;
    for (k = 0; k < size / rs; k++) {
        const unsigned char *r = d + k * rs;
        unsigned stamp;
        Rec x;
        memcpy(&stamp, r + 12, 4);
        x.dt = gf(r, 4), x.ms = gi(r, 8), x.countdown = gi(r, 16), x.running = gi(r, 20);
        x.o = r + 24;
        if (n && stamp == last)
            out[n - 1] = x;             /* the last sample of a frame */
        else
            out[n++] = x;
        last = stamp;
    }
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
    LF(push, 0x180); LV(skid_w, 0x184, 2); LV(vdir, 0x18c, 3); LF(cpu_speed, 0x1a4);
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
    LF(hit_timer, 0x31d1); LI(lane, 0x31d9); LI(turbo_held, 0x31f9);
    LI(turbo_pressed, 0x3201); LI(turbo_pressed1, 0x3205);
}

/* ------------------------------------------------------------------ the checks */

typedef struct {
    const char *name;
    int off, n, isint;
    long ok, all;
    long firstbad;
} Field;

static Field s_fields[] = {
    { "speed +0x124", 0x124, 1, 0, 0, 0, -1 }, { "wheel +0x12c", 0x12c, 1, 0, 0, 0, -1 },
    { "engine +0xec", 0xec, 1, 0, 0, 0, -1 }, { "gear +0x7c", 0x7c, 1, 1, 0, 0, -1 },
    { "steer +0x110", 0x110, 1, 0, 0, 0, -1 }, { "steer rate +0x10c", 0x10c, 1, 0, 0, 0, -1 },
    { "pos x,z +0x30/38", 0x30, -1, 0, 0, 0, -1 }, { "pos y +0x34", 0x34, 1, 0, 0, 0, -1 },
    { "matrix +0xbc", 0xbc, 9, 0, 0, 0, -1 }, { "vdir +0x18c", 0x18c, 3, 0, 0, 0, -1 },
    { "move +0xfc", 0xfc, 3, 0, 0, 0, -1 }, { "rear +0x338", 0x338, 3, 0, 0, 0, -1 },
    { "front +0x1b4", 0x1b4, 3, 0, 0, 0, -1 }, { "section +0x31c1", 0x31c1, 1, 1, 0, 0, -1 },
    { "on ground +0x2db8", 0x2db8, 1, 1, 0, 0, -1 }, { "radius +0x18", 0x18, 1, 0, 0, 0, -1 },
    { "half +0xf4", 0xf4, 1, 0, 0, 0, -1 }, { "accel +0xf8", 0xf8, 1, 0, 0, 0, -1 },
    { "dive +0x2e39", 0x2e39, 1, 0, 0, 0, -1 }, { "wheel spins +0x2e3d/49", 0x2e3d, -2, 0, 0, 0, -1 },
    { "spring +0x2e95..9d", 0x2e95, 3, 0, 0, 0, -1 }, { "susp +0x2e91", 0x2e91, 1, 0, 0, 0, -1 },
    { "pitch_s +0x2ea1", 0x2ea1, 1, 0, 0, 0, -1 }, { "pitch +0x2e8d", 0x2e8d, 1, 0, 0, 0, -1 },
    { "brake dive +0x2ea5", 0x2ea5, 1, 0, 0, 0, -1 }, { "frame +0x2e55", 0x2e55, 9, 0, 0, 0, -1 },
    { "lean +0x2e1d", 0x2e1d, 1, 0, 0, 0, -1 }, { "km/h +0x114", 0x114, 1, 1, 0, 0, -1 },
    { "rpm +0x11c", 0x11c, 1, 1, 0, 0, -1 }, { "lane +0x31d9", 0x31d9, 1, 1, 0, 0, -1 },
    { "wheelie +0x307d", 0x307d, 1, 0, 0, 0, -1 }, { "turbo time +0x3081", 0x3081, 1, 0, 0, 0, -1 },
    { "wall spin +0x2e31", 0x2e31, 1, 0, 0, 0, -1 }, { "push +0x180", 0x180, 1, 0, 0, 0, -1 },
    { "air +0xf0", 0xf0, 1, 0, 0, 0, -1 },
};
#define NFIELDS ((int)(sizeof s_fields / sizeof s_fields[0]))

/* the field's value in a BikePhys, by its offset, through a temporary record */
static void store_obj(unsigned char *o, const BikePhys *b)
{
#define SF(f, off) memcpy(o + (off), &b->f, 4)
#define SV(f, off, n) memcpy(o + (off), b->f, 4 * (n))
    SF(speed, 0x124); SF(wheel, 0x12c); SF(engine, 0xec); SF(gear, 0x7c); SF(steer, 0x110);
    SF(steer_rate, 0x10c); SV(pos, 0x30, 3); SV(m, 0xbc, 12); SV(vdir, 0x18c, 3);
    SV(move, 0xfc, 3); SV(rear, 0x338, 3); SV(front, 0x1b4, 3); SF(sec, 0x31c1);
    SF(ground, 0x2db8); SF(radius, 0x18); SF(half, 0xf4); SF(accel, 0xf8); SF(dive, 0x2e39);
    SF(spin_rear, 0x2e3d); SF(spin_front, 0x2e49); SF(kick, 0x2e95); SF(phase, 0x2e99);
    SF(amp, 0x2e9d); SF(susp, 0x2e91); SF(pitch_s, 0x2ea1); SF(pitch, 0x2e8d);
    SF(brake_dive, 0x2ea5); SV(frame, 0x2e55, 12); SF(lean, 0x2e1d); SF(kmh_shown, 0x114);
    SF(rpm, 0x11c); SF(lane, 0x31d9); SF(wheelie, 0x307d); SF(turbo_time, 0x3081);
    SF(wall_spin, 0x2e31); SF(push, 0x180); SF(air, 0xf0);
}

static int field_same(const Field *f, const unsigned char *a, const unsigned char *b)
{
    if (f->n == -1)                     /* x and z */
        return !memcmp(a + f->off, b + f->off, 4) && !memcmp(a + f->off + 8, b + f->off + 8, 4);
    if (f->n == -2)                     /* +0x2e3d and +0x2e49 */
        return !memcmp(a + f->off, b + f->off, 4) && !memcmp(a + f->off + 12, b + f->off + 12, 4);
    return !memcmp(a + f->off, b + f->off, 4 * (size_t)f->n);
}

static unsigned char s_mine[OBJ];

/* the state between steps the recordings do not hold */
static void world_for(const Rec *r0, const Rec *r1, BikePhys *b, float frame_ms)
{
    W.dt = (float)((R)(float)r1->ms * 0.1f);
    W.countdown = r1->countdown;
    W.gear_latch = (b->keys1 & 0x30) != 0;
    W.frame_ms = frame_ms;
    W.mode = 0, W.followed = 1, W.replay = 0, W.reverse = 1;
    memset(W.rider, 0, sizeof W.rider);
    W.rider[1] = b;
    b->turbo_held = gi(r1->o, 0x31f9) | gi(r1->o, 0x2e85);
    (void)r0;
}

static int run(const char *path)
{
    long size;
    unsigned char *d = slurp(path, &size);
    Rec *rec;
    int n, k;
    long eng_ok = 0, eng_n = 0, st_ok = 0, st_n = 0,
         full_n = 0, all_ok = 0, but_front = 0, missed = 0,
         crash_n = 0, crash_ok = 0, crash_m = 0;
    float frame_ms;
    if (!d) {
        printf("%s: cannot read\n", path);
        return 1;
    }
    rec = malloc(sizeof *rec * (size_t)(size / (24 + OBJ) + 1));
    n = load_rec(d, size, rec);
    for (k = 0; k < NFIELDS; k++)
        s_fields[k].ok = s_fields[k].all = 0, s_fields[k].firstbad = -1;
    frame_ms = 0.0f;                    /* 0x4dc150 converges from below (from 0) */
    for (k = 0; k < 200; k++)
        frame_ms = (float)(((R)rec[0].ms - frame_ms) * 0.125 + frame_ms);
    for (k = 0; k + 1 < n; k++) {
        const unsigned char *o0 = rec[k].o, *o1 = rec[k + 1].o;
        unsigned keys = (unsigned)gi(o1, 0x2dc8);
        BikePhys bb, *b = &bb;
        float dt = (float)((R)(float)rec[k + 1].ms * 0.1f);
        int i, good;
        frame_ms = (float)(((R)rec[k + 1].ms - frame_ms) * 0.125 + frame_ms);     /* 0x4120e0 */
        if (memcmp(o1 + 0x3c, o0 + 0x30, 12)) {
            missed++;                   /* the recorder missed a frame: not one step */
            continue;
        }
        if (gi(o0, 0x30c1) || gi(o1, 0x30c1)) {     /* the crash (0x467e30) and tumble (0x4683e0) */
            int c;
            load_obj(b, o0);
            world_for(&rec[k], &rec[k + 1], b, frame_ms);
            bikephys_step(b, keys, 0x8000, 0x8000, dt, rec[k + 1].countdown, 0);
            crash_n++;
            good = b->crashed == gi(o1, 0x30c1) && same(b->crash_timer, gf(o1, 0x30c5))
                && same(b->tumble_speed, gf(o1, 0x30d5)) && same(b->speed, gf(o1, 0x124));
            for (c = 0; c < 3; c++)     /* once up, the drawing keeps +0x30b5 and +0x3085 */
                good &= same(b->pos[c], gf(o1, 0x30 + 4 * c)) && (!gi(o1, 0x30c1)
                    || (same(b->tumble_pt[c], gf(o1, 0x30b5 + 4 * c))
                        && same(b->tumble_dir[c], gf(o1, 0x30c9 + 4 * c))));
            crash_ok += good;
            for (c = 0; c < 9 && gi(o1, 0x30c1); c++)
                good &= same(b->tumble[c], gf(o1, 0x3085 + 4 * c));
            crash_m += good;
            continue;
        }
        if (keys == 0 && gf(o0, 0x124) == 0.0f && gf(o1, 0x124) == 0.0f)
            continue;                   /* standing still */

        /* 1. the engine and the gearbox (0x465da0 -> 0x48ef30, 0x466660's box part) */
        load_obj(b, o0);
        world_for(&rec[k], &rec[k + 1], b, frame_ms);
        throttle(b, keys, 0x8000, dt);
        gearbox(b, keys);
        eng_n++;
        if (same(b->speed, gf(o1, 0x124)) && same(b->wheel, gf(o1, 0x12c))
            && same(b->engine, gf(o1, 0xec)) && b->gear == gi(o1, 0x7c))
            eng_ok++;

        /* 2. the steering (0x466240) */
        if (!rec[k + 1].countdown && ((keys & 0xc) || gf(o0, 0x110) != 0.0f)) {
            load_obj(b, o0);
            world_for(&rec[k], &rec[k + 1], b, frame_ms);
            b->sliding = gi(o1, 0x2da0);
            throttle(b, keys, 0x8000, dt);
            steering(b, keys, 0x8000, dt);
            st_n++;
            st_ok += same(b->steer, gf(o1, 0x110)) && same(b->steer_rate, gf(o1, 0x10c));
        }

        /* 3. the whole step (0x45d580), field by field */
        load_obj(b, o0);
        world_for(&rec[k], &rec[k + 1], b, frame_ms);
        bikephys_step(b, keys, 0x8000, 0x8000, dt, rec[k + 1].countdown, 0);
        memcpy(s_mine, o1, OBJ);
        store_obj(s_mine, b);
        full_n++;
        good = 1;
        {
            int nf = 1;
            for (i = 0; i < NFIELDS; i++) {
                Field *f = &s_fields[i];
                int ok = field_same(f, s_mine, o1);
                f->all++;
                f->ok += ok;
                if (!ok && f->firstbad < 0)
                    f->firstbad = k;
                good &= ok;
                if (f->off != 0x1b4)
                    nf &= ok;
            }
            but_front += nf;
        }
        all_ok += good;
        if (getenv("PHYSDBG") && atoi(getenv("PHYSDBG")) == k) {
            printf("step %d keys %x dt %g frame_ms %g\n", k, keys, dt, frame_ms);
            for (i = 0; i < 3; i++)
                printf("  front %d: %.9g %.9g   rear %.9g %.9g\n", i, b->front[i],
                       gf(o1, 0x1b4 + 4 * i), b->rear[i], gf(o1, 0x338 + 4 * i));
            printf("  lean %.9g %.9g  lean1 %.9g %.9g wheelie %g speed %g\n", b->lean,
                   gf(o1, 0x2e1d), b->lean1, gf(o1, 0x2e21), b->wheelie, b->speed);
            printf("  speed %.9g %.9g push %.9g %.9g events %x %x wall_spin %.9g %.9g\n",
                   b->speed, gf(o1, 0x124), b->push, gf(o1, 0x180), b->events, gi(o1, 0x2dac),
                   b->wall_spin, gf(o1, 0x2e31));
            printf("  pos %.9g %.9g %.9g | %.9g %.9g %.9g ground %d %d sec %d %d\n", b->pos[0],
                   b->pos[1], b->pos[2], gf(o1, 0x30), gf(o1, 0x34), gf(o1, 0x38), b->ground,
                   gi(o1, 0x2db8), b->sec, gi(o1, 0x31c1));
            for (i = 0; i < 9; i++)
                printf("  frame[%d] %.9g %.9g\n", i, b->frame[i], gf(o1, 0x2e55 + 4 * i));
            printf("  brake_dive %.9g %.9g pitch_s %.9g %.9g dive %.9g %.9g crashed %d %d\n",
                   b->brake_dive, gf(o1, 0x2ea5), b->pitch_s, gf(o1, 0x2ea1), b->dive,
                   gf(o1, 0x2e39), b->crashed, gi(o1, 0x30c1));
        }
    }
    printf("%s: %d frames, %ld pairs skipped (two steps or none between the samples)\n", path,
           n, missed);
    printf("  engine + gearbox alone (speed, wheel, engine, gear): %ld of %ld steps\n",
           eng_ok, eng_n);
    printf("  steering alone (angle and rate): %ld of %ld steps\n", st_ok, st_n);
    printf("  whole step 0x45d580, all the fields below at once: %ld of %ld steps\n", all_ok,
           full_n);
    printf("  the same but the front wheel point (rewritten after the step): %ld of %ld\n",
           but_front, full_n);
    if (crash_n)
        printf("  crashed steps (tumble point, direction, speed, timer, the body's speed and\n"
               "  position): %ld of %ld; with the tumble matrix too: %ld\n", crash_ok, crash_n,
               crash_m);
    for (k = 0; k < NFIELDS; k++)
        printf("    %-24s %5ld of %5ld%s", s_fields[k].name, s_fields[k].ok, s_fields[k].all,
               s_fields[k].firstbad >= 0 ? "" : "\n"),
        s_fields[k].firstbad >= 0 ? printf("   (first differs at step %ld)\n", s_fields[k].firstbad) : 0;
    free(rec);
    free(d);
    return 0;
}

int main(int argc, char **argv)
{
    long size;
    unsigned char *d;
    int i, k, same_cos = 0, ok = 0;
    if (argc < 5) {
        fprintf(stderr, "usage: physcheck CARAC COSTAB SECTIONS REC...\n");
        return 2;
    }
    if (!(d = slurp(argv[1], &size)) || size < 0x124) {
        fprintf(stderr, "%s: no carac row\n", argv[1]);
        return 1;
    }
    memcpy(&g_bikes[0].type, d, 4);
    memcpy(g_bikes[0].gfx, d + 4, 16);
    memcpy(g_bikes[0].phys, d + 0x14, sizeof g_bikes[0].phys);
    memcpy(&g_bikes[0].perso, d + 0x120, 4);
    free(d);
    if (!(d = slurp(argv[2], &size)) || size < 0x10000) {
        fprintf(stderr, "%s: no cosine table\n", argv[2]);
        return 1;
    }
    memcpy(g_cos, d, 0x10000);
    free(d);
    for (i = 0; i < 16384; i++) {       /* 0x4901e0's formula */
        float c = (float)cos(i * 6.283185307178 * (double)6.103515625e-05f);
        same_cos += same(c, g_cos[i]);
    }
    printf("cosine table: the game's formula gives %d of 16384 entries\n", same_cos);
    if (!(d = slurp(argv[3], &size)) || size < 0xfc) {
        fprintf(stderr, "%s: no sections\n", argv[3]);
        return 1;
    }
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
    for (i = 0; i < s_nsec; i++) {      /* the planes 0x40e5c0 builds */
        const unsigned char *s = d + i * 0xfc;
        const PSec *p = &s_sec[i];
        int good = 1;
        for (k = 0; k < 4; k++)
            good &= same(p->p40[k], gf(s, 0x40 + 4 * k)) && same(p->p80[k], gf(s, 0x80 + 4 * k))
                 && same(p->p90[k], gf(s, 0x90 + 4 * k)) && same(p->pa0[k], gf(s, 0xa0 + 4 * k));
        ok += good;
    }
    printf("section planes (+0x40, +0x80, +0x90, +0xa0): %d of %d sections bit-exact\n", ok,
           s_nsec);
    free(d);
    for (i = 4; i < argc; i++)
        run(argv[i]);
    printf("stream 1 draws (effects stub): %d\n", s_draws);
    return 0;
}
