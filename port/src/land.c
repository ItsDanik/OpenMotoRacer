/* A race track (build/notes/track.md §2, §5): the .trk file read as 0x40d650 reads it,
   the level's texture pages (0x410590), the section a point is in (0x40c740, 0x40c7e0)
   and the pieces a camera sees (0x40ce60), drawn through the race's renderer. */
#include "land.h"
#include "choices.h"
#include "config.h"
#include "files.h"
#include "horizon.h"
#include "script.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCKS_AHEAD 10         /* track +0x24, from the constructor 0x40c560 */
#define LEVEL_BANKS 20          /* Level.Spt's texture banks: pages 0..19 */

Land g_land;

typedef struct {
    const unsigned char *d;
    long len, p;
    int bad;
} Reader;

static const unsigned char *take(Reader *r, long n)
{
    const unsigned char *q = r->d + r->p;
    if (r->bad || n < 0 || n > r->len - r->p) {
        r->bad = 1;
        return 0;
    }
    r->p += n;
    return q;
}

static int rd_i32(Reader *r)
{
    const unsigned char *q = take(r, 4);
    return q ? (int)((uint32_t)q[0] | (uint32_t)q[1] << 8 | (uint32_t)q[2] << 16
                     | (uint32_t)q[3] << 24) : 0;
}

static int rd_u16(Reader *r)
{
    const unsigned char *q = take(r, 2);
    return q ? q[0] | q[1] << 8 : 0;
}

static float rd_f32(Reader *r)
{
    union { uint32_t u; float f; } x;
    x.u = (uint32_t)rd_i32(r);
    return x.f;
}

/* a count that the rest of the file can hold, at least min bytes each */
static int rd_count(Reader *r, long min)
{
    int n = rd_i32(r);
    if (n < 0 || (long)n > (r->len - r->p) / min)
        r->bad = 1;
    return r->bad ? 0 : n;
}

/* 0x422a20 with its faces (last argument 1) and 0x423240 for a face before version 6:
   the points as they are, (x, y, z) and a word dropped; per face kind, flags, colour,
   page, five corners (the fifth 0xffff: a triangle) and five (u, v) in texels */
static void read_model(Reader *r, Model *m)
{
    int i, k;
    memset(m, 0, sizeof *m);
    m->nv = rd_count(r, 16);
    if (m->nv && !(m->v = calloc((size_t)m->nv, sizeof *m->v))) {
        r->bad = 1;
        return;
    }
    for (i = 0; i < m->nv; i++) {
        for (k = 0; k < 3; k++)
            m->v[i][k] = rd_f32(r);
        rd_i32(r);
    }
    m->nf = rd_count(r, 58);
    if (m->nf && !(m->f = calloc((size_t)m->nf, sizeof *m->f))) {
        r->bad = 1;
        return;
    }
    for (i = 0; i < m->nf; i++) {
        Face *f = &m->f[i];
        int idx[5];
        f->kind = rd_u16(r);
        f->flags = rd_u16(r);
        f->colour = rd_u16(r);
        f->page = rd_u16(r);
        for (k = 0; k < 5; k++)
            idx[k] = rd_u16(r);
        f->n = idx[4] != 0xffff ? 4 : 3;
        for (k = 0; k < 5; k++) {
            float u = rd_f32(r), v = rd_f32(r);
            if (k < 4) {
                f->ut[k] = u, f->vt[k] = v;
                f->u[k] = (int)(u * 8192.0), f->v[k] = (int)(v * 8192.0);
            }
        }
        for (k = 0; k < f->n; k++) {
            f->idx[k] = idx[k];
            if (idx[k] >= m->nv)
                f->kind = 0xd;          /* (a corner out of the model: not drawn) */
        }
    }
}

static void read_name(Reader *r, char *out, int size)
{
    int n = rd_count(r, 1);
    const unsigned char *q = take(r, n);
    int i;
    for (i = 0; q && i < n && i < size - 1 && q[i]; i++)
        out[i] = (char)q[i];
    out[i] = 0;
}

/* 0x40d650 */
static int read_trk(const unsigned char *d, long len)
{
    Reader r = { d, len, 0, 0 };
    Land *l = &g_land;
    int i, k, v, n;
    if (len < 12 || memcmp(d, "TRK", 4))
        return 0;
    r.p = 4;
    v = l->version = rd_i32(&r);
    l->nsec = rd_count(&r, 0xb4);
    if (!(l->sec = calloc((size_t)l->nsec + 1, sizeof *l->sec)))
        return 0;
    for (i = 0; i < l->nsec; i++) {
        Section *s = &l->sec[i];
        for (k = 0; k < 3; k++)
            s->pos[k] = rd_f32(&r);
        for (k = 0; k < 9; k++)
            s->axes[k / 3][k % 3] = rd_f32(&r);
        for (k = 0; k < 3; k++)
            rd_f32(&r);                         /* +0x30, always 0 */
        s->block = rd_i32(&r);
        for (k = 0; k < 16; k++)                /* (0x40e5c0 makes them again, the same) */
            s->plane[k / 4][k % 4] = rd_f32(&r);
        s->line = rd_f32(&r);
        if (v > 0) {
            n = rd_count(&r, 2);
            for (k = 0; k < n; k++) {
                int di = rd_u16(&r);
                if (s->ndecor < LAND_SECDECOR)
                    s->decor[s->ndecor++] = (uint16_t)di;
            }
        }
        s->edge[0] = -1024.0f, s->edge[1] = 1024.0f;
        if (v > 3) {
            s->edge[0] = rd_f32(&r);
            s->edge[1] = rd_f32(&r);
        }
        s->checkpoint = -1;
    }
    l->nblock = rd_count(&r, 8);
    if (!(l->block = calloc((size_t)l->nblock + 1, sizeof *l->block)))
        return 0;
    for (i = 0; i < l->nblock; i++)
        read_model(&r, &l->block[i]);
    if (v > 0) {
        l->nlib = rd_count(&r, 28);
        if (!(l->lib = calloc((size_t)l->nlib + 1, sizeof *l->lib)))
            return 0;
        for (i = 0; i < l->nlib; i++) {
            read_name(&r, l->lib[i].name, sizeof l->lib[i].name);
            for (k = 0; k < 3; k++)
                read_model(&r, &l->lib[i].m[k]);
        }
        l->ndecor = rd_count(&r, 32);
        if (!(l->decor = calloc((size_t)l->ndecor + 1, sizeof *l->decor)))
            return 0;
        for (i = 0; i < l->ndecor; i++) {
            Decor *o = &l->decor[i];
            char name[64];
            read_name(&r, name, sizeof name);
            for (k = 0; k < 3; k++)
                o->pos[k] = rd_f32(&r);
            for (k = 0; k < 3; k++)
                o->ang[k] = rd_f32(&r);
            o->lib = rd_i32(&r);
            o->anim = v > 4 ? rd_i32(&r) : -1;
            if (o->lib < 0 || o->lib >= l->nlib)
                o->lib = -1;
        }
    }
    if (v > 1) {
        n = rd_count(&r, 6);
        for (i = 0; i < n; i++) {
            Section dummy, *s = i < l->nsec ? &l->sec[i] : &dummy;
            s->checkpoint = (int16_t)rd_u16(&r);
            s->time = rd_u16(&r);
            rd_u16(&r);                         /* +0xde */
            if (v > 3) {
                s->event = rd_i32(&r);
                s->ground_off = rd_i32(&r);
                s->ground_on = rd_i32(&r);
                s->flag = rd_i32(&r);
            }
        }
        if (v > 2) {
            l->horizon_offset = rd_i32(&r);
            l->horizon_scale = rd_f32(&r);
        }
        /* then "AnimDecor" (0x48b7e0): the moving decor, not read yet */
    }
    for (i = 0; i < l->nsec; i++)
        if (l->sec[i].block < 0 || l->sec[i].block >= l->nblock)
            l->sec[i].block = 0;
    return !r.bad && l->nsec > 0 && l->nblock > 0;
}

static void free_model(Model *m)
{
    free(m->v);
    free(m->f);
    memset(m, 0, sizeof *m);
}

void land_free(void)
{
    int i, k;
    for (i = 0; i < g_land.nblock; i++)
        free_model(&g_land.block[i]);
    for (i = 0; i < g_land.nlib; i++)
        for (k = 0; k < 3; k++)
            free_model(&g_land.lib[i].m[k]);
    free(g_land.sec);
    free(g_land.block);
    free(g_land.lib);
    free(g_land.decor);
    memset(&g_land, 0, sizeof g_land);
    for (i = 2; i < LEVEL_BANKS; i++)
        r3d_free_page(i);
    horizon_free();
}

/* Level.Spt (0x427e80): NewMotorEngine, FileLand and nine more names, then the texture
   banks, each a name, its Direct3D twin and a number */
int land_load(int track, char *err, int errsize)
{
    Script s;
    char spt[0x120], land[64], name[64], path[0x180];
    unsigned char *d;
    long len;
    int i, k, ok;
    land_free();
    if (track < 0 || track >= g_ntracks) {
        snprintf(err, (size_t)errsize, "No track %d.", track);
        return 0;
    }
    snprintf(spt, sizeof spt, "%sLevel.Spt", g_tracks[track].folder);
    if (!script_open(&s, spt)) {
        snprintf(err, (size_t)errsize, "%s could not be read.", spt);
        return 0;
    }
    script_int(&s);
    script_str(&s, land, sizeof land);
    for (i = 0; i < 9; i++)
        script_str(&s, name, sizeof name);
    for (i = 0; i < LEVEL_BANKS && !s.bad; i++) {
        script_str(&s, name, sizeof name);
        script_str(&s, path, sizeof path);
        k = script_int(&s);                     /* 0x51d168 + i x 4 */
        if (i == 1)
            g_land.bank1_rgb = (k & 4) != 0;
        if (i >= 2 && !s.bad) {                 /* 0, 1: the bikes' (0x518b20, 0x518b60) */
            snprintf(path, sizeof path, "%s%s", g_tracks[track].folder, name);
            if (!r3d_load_page(i, path))
                fprintf(stderr, "openmotoracer: %s could not be read\n", path);
        }
    }
    ok = !s.bad;
    script_close(&s);
    if (!ok) {
        snprintf(err, (size_t)errsize, "%s is not as expected.", spt);
        return 0;
    }
    snprintf(path, sizeof path, "%s%s", g_tracks[track].folder, land);
    if (!(d = dsi_load(path, &len))) {
        snprintf(err, (size_t)errsize, "%s could not be read.", path);
        return 0;
    }
    ok = read_trk(d, len);
    free(d);
    if (ok && !horizon_load(g_tracks[track].folder))
        fprintf(stderr, "openmotoracer: %sBank\\HorizHi.raw could not be read\n",
                g_tracks[track].folder);
    if (!ok) {
        land_free();
        snprintf(err, (size_t)errsize, "%s is not a track this game knows.", path);
        return 0;
    }
    return 1;
}

static double plane_dist(const float pl[4], const float p[3])   /* 0x4b10c0 */
{
    return (double)pl[0] * p[0] + (double)pl[1] * p[1] + (double)pl[2] * p[2] + pl[3];
}

int land_section(const float pos[3], int hint)
{
    int n = g_land.nsec, i, k;
    if (n <= 0)
        return 0;
    if (hint < 0 || hint >= n) {                /* 0x40c740: the nearest by distance */
        double best = 0.0;
        hint = 0;
        for (i = 0; i < n; i++) {
            double dx = (double)g_land.sec[i].pos[0] - pos[0];
            double dy = (double)g_land.sec[i].pos[1] - pos[1];
            double dz = (double)g_land.sec[i].pos[2] - pos[2];
            double dd = dx * dx + dy * dy + dz * dz;
            if (i == 0 || dd < best)
                best = dd, hint = i;
        }
    }
    /* 0x40c7e0: forward while in front of the next one's start plane, back while behind
       its own */
    i = hint;
    if (plane_dist(g_land.sec[i].plane[0], pos) >= 0.0) {
        for (k = 0; k < n && plane_dist(g_land.sec[(i + 1) % n].plane[0], pos) >= 0.0; k++)
            i = (i + 1) % n;
    } else
        for (k = 0; k < n; k++) {
            i = (i + n - 1) % n;
            if (plane_dist(g_land.sec[i].plane[0], pos) >= 0.0)
                break;
        }
    return i;
}

/* 0x403880: the decor's rotation from its angles (a, b, c), every entry a float, two
   products kept in floats on the way */
static void decor_matrix(float m[3][3], const float ang[3])
{
    double c[3], s[3];
    float scsb, ccsb;
    int j;
    for (j = 0; j < 3; j++) {
        int i = (int)(ang[j] * 2607.594587618272);        /* 0x4d00d0 */
        c[j] = g_cos[i & 0x3fff];
        s[j] = g_cos[(i - 0x1000) & 0x3fff];
    }
    scsb = (float)(s[2] * s[1]);
    ccsb = (float)(c[2] * s[1]);
    m[0][0] = (float)(c[2] * c[1] + scsb * s[0]);
    m[0][1] = (float)(s[2] * c[0]);
    m[0][2] = (float)(s[2] * s[0] * c[1] - ccsb);
    m[1][0] = (float)(ccsb * s[0] - s[2] * c[1]);
    m[1][1] = (float)(c[2] * c[0]);
    m[1][2] = (float)(c[2] * s[0] * c[1] + scsb);
    m[2][0] = (float)(s[1] * c[0]);
    m[2][1] = (float)-s[0];
    m[2][2] = (float)(c[0] * c[1]);
}

static unsigned char *g_drawn;  /* each decor object once a frame */
static int g_ndrawn;

static void section_decor(int si)
{
    int i;
    for (i = 0; i < g_land.sec[si].ndecor; i++) {
        int di = g_land.sec[si].decor[i];
        const Decor *o;
        float m[3][3];
        if (di >= g_ndrawn || g_drawn[di])
            continue;
        g_drawn[di] = 1;
        o = &g_land.decor[di];
        if (o->lib < 0 || strncmp(g_land.lib[o->lib].name, "CAMERA", 6) == 0)
            continue;                           /* 0x473240 hides the cameras */
        decor_matrix(m, o->ang);
        r3d_object_m(&g_land.lib[o->lib].m[0], (const float (*)[3])m, o->pos);
    }
}

void land_draw(int sec, const float cm[3][3], const float pos[3])
{
    static const float one[3][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
    static const float zero[3] = { 0, 0, 0 };
    const Section *s;
    double d;
    int ahead, behind, nb = g_land.nblock, n = g_land.nsec, b0, k;
    if (n <= 0 || sec < 0 || sec >= n)
        return;
    r3d_set_camera(cm, pos);
    s = &g_land.sec[sec];
    d = (double)cm[0][2] * s->plane[0][0] + (double)cm[1][2] * s->plane[0][1]
        + (double)cm[2][2] * s->plane[0][2];
    if (d >= 0.25)                              /* looking along the road (0x4d02f0) */
        ahead = BLOCKS_AHEAD, behind = 1;
    else if (d > -0.25)                         /* sideways (0x4d02f8) */
        ahead = behind = BLOCKS_AHEAD / 2;
    else
        ahead = 1, behind = BLOCKS_AHEAD;
    b0 = s->block;
    r3d_object_m(&g_land.block[b0], one, zero);
    for (k = 1; k <= ahead; k++)
        r3d_object_m(&g_land.block[(b0 + k) % nb], one, zero);
    for (k = 1; k <= behind; k++)
        r3d_object_m(&g_land.block[((b0 - k) % nb + nb) % nb], one, zero);
    if (g_detail == 0)                          /* no decor at the lowest detail */
        return;
    if (g_ndrawn < g_land.ndecor) {
        free(g_drawn);
        g_ndrawn = 0;
        if (!(g_drawn = calloc((size_t)g_land.ndecor, 1)))
            return;
        g_ndrawn = g_land.ndecor;
    }
    memset(g_drawn, 0, (size_t)g_ndrawn);
    /* the decor of the sections sec .. sec + 4 ahead + 4, then sec - 1 .. sec - 4 behind - 4 */
    for (k = 0; k <= 4 * ahead + 4; k++)
        section_decor((sec + k) % n);
    for (k = 1; k <= 4 * behind + 4; k++)
        section_decor(((sec - k) % n + n) % n);
}
