/* Checks port/src/bikephys.c with g_phys_cd on against the running 1997 MOTO.EXE: a recording
   of tools/win/stepwatch.c holds, for every step of the driven bike, the object before and
   after it. One step of the port from "before", compared bit for bit with "after", field by
   field. Build and run: tools/physcheck_cd.sh.

     physcheck_cd REC...      (REC.carac, REC.cos, REC.sec beside each)

   It includes bikephys.c to reach its pieces; the rest of the port is stubbed. The object's
   layout is MOTO.EXE's (GOG's offset in the comments of bikephys.h, the map below). */
#include "../port/src/bikephys.c"

#include <stdio.h>

Bike g_bikes[BIKES];
Land g_land;
float g_cos[16384];
int rand_next(int stream)
{
    (void)stream;
    return 0;
}

#define OBJ 0x31d5
#define HEAD 96

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

#define LF(f, off) b->f = gf(o, off)
#define LI(f, off) b->f = gi(o, off)
#define LV(f, off, n) for (i = 0; i < n; i++) b->f[i] = gf(o, (off) + 4 * i)

/* MOTO.EXE's object: the body one word lower to +0x138 (a new float there), the same to the
   front point's record, rear +4, third +8, +0xc from +0x2dac, +4 and +0x10 past the lean,
   -8 past the shadow (4 points, not 6) */
static void load_obj(BikePhys *b, const unsigned char *o)
{
    int i;
    memset(b, 0, sizeof *b);
    LI(driven, 0x08); LI(human, 0x04); LI(forced, 0x0c); LI(forced_keys, 0x10);
    LF(radius, 0x18); LI(bike, 0x74); LI(number, 0x2ddc); LI(state, 0x2df1);
    LV(pos, 0x2c, 3); LV(pos1, 0x38, 3); LV(pos2, 0x44, 3); LF(pos_dt, 0x50);
    LI(gear, 0x78); LI(shift_timer, 0x7c); LI(shift, 0x80); LI(automatic, 0x84);
    LI(pending, 0x88); LV(table, 0x8c, 7); LI(brakes_grip, 0xa8); LI(brake_ms, 0xac);
    LF(top, 0xb0); LF(power, 0xb4); LV(m, 0xb8, 12); LF(engine, 0xe8); LF(air, 0xec);
    LF(half, 0xf0); LF(accel, 0xf4); LV(move, 0xf8, 3); LF(steer_rate, 0x108);
    LF(steer, 0x10c); LI(kmh_shown, 0x110); LI(kmh, 0x114); LI(rpm, 0x118);
    LF(cpu_accel, 0x11c); LF(speed, 0x120); LF(speed_in, 0x124); LF(wheel, 0x128);
    LV(dir, 0x12c, 3); LI(slide, 0x13c); LV(m1, 0x140, 12); LV(push_dir, 0x174, 3);
    LF(push, 0x180); LV(skid_w, 0x184, 2); LV(vdir, 0x18c, 3); LF(cpu_speed, 0x1a4);
    LV(front, 0x1b4, 3); LV(front1, 0x1c0, 3); LV(front2, 0x1cc, 3); LF(front_air, 0x274);
    LV(rear, 0x33c, 3); LV(rear1, 0x348, 3); LV(rear2, 0x354, 3); LF(rear_air, 0x3fc);
    LV(third, 0x4c4, 3); LV(third1, 0x4d0, 3); LV(third2, 0x4dc, 3);
    LI(sliding, 0x2dac); LI(sliding1, 0x2db0); LF(lon0, 0x2db4); LI(events, 0x2db8);
    LF(rear_h, 0x2dbc); LI(ground, 0x2dc4); LI(ground1, 0x2dc8); LI(rear_touch, 0x2dcc);
    LI(front_touch, 0x2dd0); LI(keys1, 0x2dd4); LF(lean, 0x2e29);
    LF(wall_spin, 0x2e35); LF(dive, 0x2e3d); LF(spin_rear, 0x2e4d);
    LF(spin_front, 0x2e59); LV(frame, 0x2e65, 12); LI(turbo_req, 0x2e95); LF(pitch, 0x2e9d);
    LF(susp, 0x2ea1); LF(kick, 0x2ea5); LF(phase, 0x2ea9); LF(amp, 0x2ead);
    LF(pitch_s, 0x2eb1); LF(brake_dive, 0x2eb5); LF(wheelie, 0x3075); LF(turbo_time, 0x3079);
    LV(tumble, 0x307d, 12); LV(tumble_pt, 0x30ad, 3); LI(crashed, 0x30b9);
    LF(crash_timer, 0x30bd); LV(tumble_dir, 0x30c1, 3); LF(tumble_speed, 0x30cd);
    LF(tumble_speed0, 0x30d1); LF(roll_side, 0x30d5); LV(roll, 0x30d9, 3); LI(detail, 0x3195);
    LV(crash_v, 0x3199, 3); LI(crash_flag, 0x31a5); LI(sec, 0x31b9); LV(wall_n, 0x31bd, 3);
    LF(hit_timer, 0x31c9); LI(lane, 0x31d1);
}

typedef struct {
    const char *name;
    int off, n, isint;
    long ok, near, all, firstbad, firstfar;
} Field;

/* offsets in MOTO.EXE's object; through s_put (the same layout, written from a BikePhys) */
static Field s_fields[] = {
    { "lon0 +0x2db4", 0x2db4, 1, 0, 0, 0, 0, -1, -1 }, { "keys1 +0x2dd4", 0x2dd4, 1, 1, 0, 0, 0, -1, -1 },
    { "ground1 +0x2dc8", 0x2dc8, 1, 1, 0, 0, 0, -1, -1 }, { "sliding1 +0x2db0", 0x2db0, 1, 1, 0, 0, 0, -1, -1 },
    { "rear h +0x2dbc", 0x2dbc, 1, 0, 0, 0, 0, -1, -1 }, { "speed in +0x124", 0x124, 1, 0, 0, 0, 0, -1, -1 },
    { "skid +0x184", 0x184, 2, 0, 0, 0, 0, -1, -1 }, { "bike's km/h +0x114", 0x114, 1, 1, 0, 0, 0, -1, -1 },
    { "cpu speed +0x1a4", 0x1a4, 1, 0, 0, 0, 0, -1, -1 }, { "crash flag +0x31a5", 0x31a5, 1, 1, 0, 0, 0, -1, -1 },
    { "hit timer +0x31c9", 0x31c9, 1, 0, 0, 0, 0, -1, -1 }, { "wall normal +0x31bd", 0x31bd, 3, 0, 0, 0, 0, -1, -1 },
    { "crash v +0x3199", 0x3199, 3, 0, 0, 0, 0, -1, -1 }, { "roll +0x30d9", 0x30d9, 3, 0, 0, 0, 0, -1, -1 },
    { "roll side +0x30d5", 0x30d5, 1, 0, 0, 0, 0, -1, -1 }, { "speed0 +0x30d1", 0x30d1, 1, 0, 0, 0, 0, -1, -1 },
    { "detail +0x3195", 0x3195, 1, 1, 0, 0, 0, -1, -1 }, { "tumble +0x307d", 0x307d, 12, 0, 0, 0, 0, -1, -1 },
    { "turbo req +0x2e95", 0x2e95, 1, 1, 0, 0, 0, -1, -1 }, { "state +0x2df1", 0x2df1, 1, 1, 0, 0, 0, -1, -1 },
    { "gear table +0x8c", 0x8c, 7, 0, 0, 0, 0, -1, -1 },
    { "speed +0x120", 0x120, 1, 0, 0, 0, -1, -1 }, { "wheel +0x128", 0x128, 1, 0, 0, 0, -1, -1 },
    { "engine +0xe8", 0xe8, 1, 0, 0, 0, -1, -1 }, { "gear +0x78", 0x78, 1, 1, 0, 0, -1 },
    { "shift +0x80", 0x80, 1, 1, 0, 0, -1 }, { "shift timer +0x7c", 0x7c, 1, 1, 0, 0, -1 },
    { "pending +0x88", 0x88, 1, 1, 0, 0, -1 }, { "brake ms +0xac", 0xac, 1, 1, 0, 0, -1 },
    { "steer +0x10c", 0x10c, 1, 0, 0, 0, -1, -1 }, { "steer rate +0x108", 0x108, 1, 0, 0, 0, -1, -1 },
    { "pos +0x2c", 0x2c, 3, 0, 0, 0, -1, -1 }, { "matrix +0xb8", 0xb8, 9, 0, 0, 0, -1, -1 },
    { "dir +0x12c", 0x12c, 3, 0, 0, 0, -1, -1 }, { "vdir +0x18c", 0x18c, 3, 0, 0, 0, -1, -1 },
    { "move +0xf8", 0xf8, 3, 0, 0, 0, -1, -1 }, { "rear +0x33c", 0x33c, 3, 0, 0, 0, -1, -1 },
    { "front +0x1b4", 0x1b4, 3, 0, 0, 0, -1, -1 }, { "section +0x31b9", 0x31b9, 1, 1, 0, 0, -1 },
    { "on ground +0x2dc4", 0x2dc4, 1, 1, 0, 0, -1 }, { "radius +0x18", 0x18, 1, 0, 0, 0, -1, -1 },
    { "half +0xf0", 0xf0, 1, 0, 0, 0, -1, -1 }, { "accel +0xf4", 0xf4, 1, 0, 0, 0, -1, -1 },
    { "sliding +0x2dac", 0x2dac, 1, 1, 0, 0, -1 }, { "slide +0x13c", 0x13c, 1, 1, 0, 0, -1 },
    { "brakes grip +0xa8", 0xa8, 1, 1, 0, 0, -1 }, { "events +0x2db8", 0x2db8, 1, 1, 0, 0, -1 },
    { "dive +0x2e3d", 0x2e3d, 1, 0, 0, 0, -1, -1 }, { "wheel spin R +0x2e4d", 0x2e4d, 1, 0, 0, 0, -1, -1 },
    { "wheel spin F +0x2e59", 0x2e59, 1, 0, 0, 0, -1, -1 }, { "spring +0x2ea5..ad", 0x2ea5, 3, 0, 0, 0, -1, -1 },
    { "susp +0x2ea1", 0x2ea1, 1, 0, 0, 0, -1, -1 }, { "pitch_s +0x2eb1", 0x2eb1, 1, 0, 0, 0, -1, -1 },
    { "pitch +0x2e9d", 0x2e9d, 1, 0, 0, 0, -1, -1 }, { "brake dive +0x2eb5", 0x2eb5, 1, 0, 0, 0, -1, -1 },
    { "frame +0x2e65", 0x2e65, 9, 0, 0, 0, -1, -1 }, { "lean +0x2e29", 0x2e29, 1, 0, 0, 0, -1, -1 },
    { "km/h +0x110", 0x110, 1, 1, 0, 0, -1 }, { "rpm +0x118", 0x118, 1, 1, 0, 0, -1 },
    { "lane +0x31d1", 0x31d1, 1, 1, 0, 0, -1 }, { "wheelie +0x3075", 0x3075, 1, 0, 0, 0, -1, -1 },
    { "turbo time +0x3079", 0x3079, 1, 0, 0, 0, -1, -1 }, { "wall spin +0x2e35", 0x2e35, 1, 0, 0, 0, -1, -1 },
    { "push +0x180", 0x180, 1, 0, 0, 0, -1, -1 }, { "air +0xec", 0xec, 1, 0, 0, 0, -1, -1 },
    { "crashed +0x30b9", 0x30b9, 1, 1, 0, 0, -1 }, { "crash timer +0x30bd", 0x30bd, 1, 0, 0, 0, -1, -1 },
    { "tumble speed +0x30cd", 0x30cd, 1, 0, 0, 0, -1, -1 }, { "tumble dir +0x30c1", 0x30c1, 3, 0, 0, 0, -1, -1 },
    { "tumble point +0x30ad", 0x30ad, 3, 0, 0, 0, -1, -1 },
};
#define NFIELDS ((int)(sizeof s_fields / sizeof s_fields[0]))

static void store_obj(unsigned char *o, const BikePhys *b)
{
#define SF(f, off) memcpy(o + (off), &b->f, 4)
#define SV(f, off, n) memcpy(o + (off), b->f, 4 * (n))
    SF(speed, 0x120); SF(wheel, 0x128); SF(engine, 0xe8); SF(gear, 0x78); SF(shift, 0x80);
    SF(shift_timer, 0x7c); SF(pending, 0x88); SF(brake_ms, 0xac); SF(steer, 0x10c);
    SF(steer_rate, 0x108); SV(pos, 0x2c, 3); SV(m, 0xb8, 12); SV(dir, 0x12c, 3);
    SV(vdir, 0x18c, 3); SV(move, 0xf8, 3); SV(rear, 0x33c, 3); SV(front, 0x1b4, 3);
    SF(sec, 0x31b9); SF(ground, 0x2dc4); SF(radius, 0x18); SF(half, 0xf0); SF(accel, 0xf4);
    SF(sliding, 0x2dac); SF(slide, 0x13c); SF(brakes_grip, 0xa8); SF(events, 0x2db8);
    SF(dive, 0x2e3d); SF(spin_rear, 0x2e4d); SF(spin_front, 0x2e59); SF(kick, 0x2ea5);
    SF(phase, 0x2ea9); SF(amp, 0x2ead); SF(susp, 0x2ea1); SF(pitch_s, 0x2eb1); SF(pitch, 0x2e9d);
    SF(brake_dive, 0x2eb5); SV(frame, 0x2e65, 12); SF(lean, 0x2e29); SF(kmh_shown, 0x110);
    SF(rpm, 0x118); SF(lane, 0x31d1); SF(wheelie, 0x3075); SF(turbo_time, 0x3079);
    SF(wall_spin, 0x2e35); SF(push, 0x180); SF(air, 0xec); SF(crashed, 0x30b9);
    SF(crash_timer, 0x30bd); SF(tumble_speed, 0x30cd); SV(tumble_dir, 0x30c1, 3);
    SV(tumble_pt, 0x30ad, 3); SF(lon0, 0x2db4); SF(keys1, 0x2dd4); SF(ground1, 0x2dc8);
    SF(sliding1, 0x2db0); SF(rear_h, 0x2dbc); SF(speed_in, 0x124); SV(skid_w, 0x184, 2);
    SF(kmh, 0x114); SF(cpu_speed, 0x1a4); SF(crash_flag, 0x31a5); SF(hit_timer, 0x31c9);
    SV(wall_n, 0x31bd, 3); SV(crash_v, 0x3199, 3); SV(roll, 0x30d9, 3); SF(roll_side, 0x30d5);
    SF(tumble_speed0, 0x30d1); SF(detail, 0x3195); SV(tumble, 0x307d, 12); SF(turbo_req, 0x2e95);
    SF(state, 0x2df1); SV(table, 0x8c, 7);
}

static unsigned char s_mine[OBJ];
static int s_loaded_track;

/* the carac rows, cosines and sections beside a recording */
static int load_world(const char *rec)
{
    char name[600];
    long size;
    unsigned char *d;
    int i;
    snprintf(name, sizeof name, "%s.carac", rec);
    if (!(d = slurp(name, &size)) || size < 8 * 0x124) {
        printf("%s: no carac rows\n", name);
        return 0;
    }
    for (i = 0; i < 8 && i < BIKES; i++) {
        const unsigned char *r = d + i * 0x124;
        memcpy(&g_bikes[i].type, r, 4);
        memcpy(g_bikes[i].gfx, r + 4, 16);
        memcpy(g_bikes[i].phys, r + 0x14, sizeof g_bikes[i].phys);
        memcpy(&g_bikes[i].perso, r + 0x120, 4);
    }
    free(d);
    snprintf(name, sizeof name, "%s.cos", rec);
    if (!(d = slurp(name, &size)) || size < 0x10000) {
        printf("%s: no cosines\n", name);
        return 0;
    }
    memcpy(g_cos, d, 0x10000);
    free(d);
    snprintf(name, sizeof name, "%s.sec", rec);
    if (!(d = slurp(name, &size)) || size < 0xfc) {
        printf("%s: no sections\n", name);
        return 0;
    }
    if (s_loaded_track)
        free(g_land.sec);
    g_land.nsec = (int)(size / 0xfc);
    g_land.sec = calloc((size_t)g_land.nsec, sizeof *g_land.sec);
    s_loaded_track = 1;
    for (i = 0; i < g_land.nsec; i++) {
        const unsigned char *s = d + i * 0xfc;
        Section *l = &g_land.sec[i];
        memcpy(l->pos, s, 12);
        memcpy(l->axes, s + 0x0c, 36);
        memcpy(l->plane, s + 0x40, 64);
        l->line = gf(s, 0xb0);
        l->edge[0] = gf(s, 0xf0), l->edge[1] = gf(s, 0xf4);
    }
    free(d);
    bikephys_track();
    return 1;
}

static void run(const char *path)
{
    long size, rs = HEAD + 2 * OBJ, n, k, full = 0, good = 0, moved = 0;
    long sdbg = getenv("PHYSDBG") ? atol(getenv("PHYSDBG")) : -1;
    unsigned char *d = slurp(path, &size);
    int i;
    if (!d || !load_world(path)) {
        printf("%s: cannot read\n", path);
        return;
    }
    for (i = 0; i < NFIELDS; i++)
        s_fields[i].ok = s_fields[i].near = s_fields[i].all = 0, s_fields[i].firstbad = -1,
        s_fields[i].firstfar = -1;
    n = size / rs;
    for (k = 0; k < n; k++) {
        const unsigned char *r = d + k * rs;
        const unsigned char *o0 = r + HEAD, *o1 = r + HEAD + OBJ;
        unsigned keys = (unsigned)gi(r, 8);
        float dt;
        BikePhys bb, *b = &bb;
        int nf, ok = 1;
        memcpy(&dt, r + 20, 4);
        load_obj(b, o0);
        W.dt = gf(r, 24);
        W.countdown = gi(r, 28);
        W.gear_latch = gi(r, 24 + 16);
        W.mode = 0, W.followed = 1, W.replay = 0, W.reverse = 1;
        memset(W.rider, 0, sizeof W.rider);
        W.rider[1] = b;
        W.crt = (unsigned)gi(r, 24 + 64);
        b->turbo_held = gi(r, 24 + 12);
        bikephys_step(b, keys, gi(r, 16), gi(r, 12), dt, W.countdown, 0);
        memcpy(s_mine, o1, OBJ);
        store_obj(s_mine, b);
        full++;
        moved += gf(o0, 0x120) != 0.0f;
        /* the tumble's matrix means something only in a crash (the drawer keeps it till then);
           +0x184 is the effects' own, in the drawing */
        if (!gi(o1, 0x30b9))
            memcpy(s_mine + 0x307d, o1 + 0x307d, 48);
        memcpy(s_mine + 0x184, o1 + 0x184, 8);
        for (nf = 0; nf < NFIELDS; nf++) {
            Field *f = &s_fields[nf];
            int same = !memcmp(s_mine + f->off, o1 + f->off, 4 * (size_t)f->n);
            f->all++;
            f->ok += same;
            if (!same && f->firstbad < 0)
                f->firstbad = k;
            if (!same && sdbg == -2)    /* PHYSDBG=-2: every step and field that differs */
                printf("  %ld %s\n", k, f->name);
            if (!same) {                /* rounding, or something else? */
                int c, close = !f->isint;
                for (c = 0; c < f->n && close; c++) {
                    int a = gi(s_mine, f->off + 4 * c), g = gi(o1, f->off + 4 * c);
                    float fa = gf(s_mine, f->off + 4 * c), fg = gf(o1, f->off + 4 * c);
                    if (a == g || ((a ^ g) >= 0 && abs(a - g) <= 64))
                        continue;
                    if (fabsf(fa) < 1e-6f && fabsf(fg) < 1e-6f)
                        continue;
                    close = 0;
                }
                if (close)
                    f->near++;
                else if (f->firstfar < 0)
                    f->firstfar = k;
            }
            ok &= same;
            if (!same && sdbg == k) {
                int c;
                for (c = 0; c < f->n; c++) {
                    if (f->isint)
                        printf("  %s[%d]: port %d, game %d\n", f->name, c,
                               gi(s_mine, f->off + 4 * c), gi(o1, f->off + 4 * c));
                    else
                        printf("  %s[%d]: port %.9g, game %.9g (before %.9g)\n", f->name, c,
                               gf(s_mine, f->off + 4 * c), gf(o1, f->off + 4 * c),
                               gf(o0, f->off + 4 * c));
                }
            }
        }
        good += ok;
        if (sdbg == k)
            printf("step %ld keys %x dt %.9g W.dt %.9g countdown %d k(+0x138) %.9g -> %.9g\n", k,
                   keys, dt, W.dt, W.countdown, gf(o0, 0x138), gf(o1, 0x138));
    }
    printf("%s: %ld steps, %ld moving, all the fields below at once: %ld\n", path, full, moved, good);
    for (i = 0; i < NFIELDS; i++) {
        Field *f = &s_fields[i];
        printf("    %-24s %6ld of %6ld", f->name, f->ok, f->all);
        if (f->firstbad >= 0)
            printf("   first at %ld; %ld by rounding only", f->firstbad, f->near);
        if (f->firstfar >= 0)
            printf(", first real at %ld", f->firstfar);
        printf("\n");
    }
    free(d);
}

int main(int argc, char **argv)
{
    int i;
    if (argc < 2) {
        fprintf(stderr, "usage: physcheck_cd REC...\n");
        return 2;
    }
    g_phys_cd = 1;
    for (i = 1; i < argc; i++)
        run(argv[i]);
    return 0;
}
