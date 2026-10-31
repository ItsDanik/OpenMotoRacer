/* F9's track map: 0x40ed10 makes it from the track's sections, 0x40f2d0 draws it each
   picture (NOTES §4.20 §1b). */
#include "trackmap.h"
#include "bikephys.h"
#include "choices.h"
#include "clock.h"
#include "config.h"
#include "land.h"
#include "r3d.h"
#include "race.h"
#include "rules.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define ANGLE_K 2607.594587618272       /* 0x4d0300: radians to the cosine table's index */
#define MARK_POINTS 75                  /* +0xd4: 3 for each of the 25 rider records */

int g_map_mode = 4;

static int g_nfaces, g_npoints;         /* +0x54, +0x58 */
static float (*g_points)[3];            /* +0x2c */
static Face *g_faces;                   /* +0x28: kind 12, on the half of what is behind */
static float g_centre[3];               /* +0x98 */
static float g_scale;                   /* +0x114 */
static float g_mark_points[MARK_POINTS][3];     /* +0xa8 */
static Face g_mark[RIDERS];             /* +0xa4 */
static float g_place[3];                /* 0x504e88: eases to where the mode wants it */

static uint16_t rgb(int r, int g, int b)        /* 0x45a2e0 */
{
    return (uint16_t)(r << 11 | g << 6 | b);
}

static void place_start(void)
{
    g_place[0] = -2000.0f;
    g_place[1] = -1200.0f;
    g_place[2] = 4000.0f;
}

void trackmap_free(void)
{
    free(g_points);
    free(g_faces);
    g_points = 0;
    g_faces = 0;
    g_nfaces = g_npoints = 0;
}

/* a side of the road's ribbon, 1024 across from the section's middle, flat: the map's Y is
   the world's Z */
static void edge(float *v, const Section *s, float across)
{
    v[0] = s->axes[0][0] * across + s->pos[0];
    v[1] = s->axes[0][2] * across + s->pos[2];
    v[2] = 0.0f;
}

void trackmap_build(void)
{
    float lo[3] = { 999999.0f, 999999.0f, 999999.0f };
    float hi[3] = { -999999.0f, -999999.0f, -999999.0f };
    float half[3];
    int i, k, n = g_land.nsec / 4;
    trackmap_free();
    if (n <= 0)
        return;
    /* (the software path: every 4th section; Direct3D's is a line through every 8th) */
    g_nfaces = n;
    g_npoints = n * 2 + 2;
    g_points = calloc((size_t)g_npoints, sizeof *g_points);
    g_faces = calloc((size_t)n, sizeof *g_faces);
    if (!g_points || !g_faces) {
        trackmap_free();
        return;
    }
    for (i = 0; i < n; i++) {
        const Section *s = &g_land.sec[i * 4];
        Face *f = &g_faces[i];
        edge(g_points[i * 2], s, -1024.0f);
        edge(g_points[i * 2 + 1], s, 1024.0f);
        f->kind = 12;                   /* 0x422c80 */
        f->n = 4;
        f->idx[0] = i * 2;
        f->idx[1] = i * 2 + 2;
        f->idx[2] = i * 2 + 3;
        f->idx[3] = i * 2 + 1;
    }
    memcpy(g_points[n * 2], g_points[0], sizeof g_points[0]);
    memcpy(g_points[n * 2 + 1], g_points[1], sizeof g_points[0]);
    /* 0x420250, then 0x4204b0 by 1280 / the box's half diagonal */
    for (i = 0; i < g_npoints; i++)
        for (k = 0; k < 3; k++) {
            if (g_points[i][k] > hi[k])
                hi[k] = g_points[i][k];
            if (g_points[i][k] < lo[k])
                lo[k] = g_points[i][k];
        }
    for (k = 0; k < 3; k++) {
        g_centre[k] = (lo[k] + hi[k]) * 0.5f;
        half[k] = (hi[k] - lo[k]) * 0.5f;
    }
    g_scale = 1280.0f / sqrtf(half[1] * half[1] + half[2] * half[2] + half[0] * half[0]);
    for (i = 0; i < g_npoints; i++)
        for (k = 0; k < 3; k++)
            g_points[i][k] = (g_points[i][k] - g_centre[k]) * g_scale;
    for (i = 0; i < RIDERS; i++) {
        memset(&g_mark[i], 0, sizeof g_mark[i]);
        g_mark[i].kind = 0;             /* 0x422be0: a plain triangle */
        g_mark[i].n = 3;
        for (k = 0; k < 3; k++)
            g_mark[i].idx[k] = i * 3 + k;
        g_mark[i].colour = rgb(0x1f, 0, 0);
    }
    place_start();
}

void trackmap_key(void)
{
    g_map_mode = (g_map_mode + 1) % 5;
}

/* a rider's triangle (0x40f7cd): where he is on the map, pointing along his heading */
static void mark_points(int k, const Rider *r, float size)
{
    float ang[3], m[3][3], x, y;
    float *v = g_mark_points[k * 3];
    int i;
    ang[0] = ang[2] = 0.0f;
    ang[1] = (float)-bp_heading(&r->m[0][0]);
    r3d_rotation(m, ang, ANGLE_K);      /* 0x40fd40 */
    x = (r->pos[0] - g_centre[0]) * g_scale;
    y = (r->pos[2] - g_centre[1]) * g_scale;
    for (i = 0; i < 3; i++, v += 3) {
        float ox, oy;
        if (i == 0) {
            ox = (m[2][0] + m[0][0]) * -size;
            oy = (m[2][2] + m[0][2]) * -size;
        } else if (i == 1) {
            ox = size * 2.0f * m[2][0];
            oy = size * 2.0f * m[2][2];
        } else {
            ox = -size * m[2][0] + m[0][0] * size;
            oy = -size * m[2][2] + m[0][2] * size;
        }
        v[0] = x + ox;
        v[1] = y + oy;
        v[2] = 0.0f - g_centre[2];
    }
}

void trackmap_draw(const Rider *riders, int follow, float focal)
{
    float n[3][3], t[3], want[3], c, s, size;
    int i, base, turns, side = g_map_mode == 0 || g_map_mode == 2;
    static const float low169 = -500.0f;
    if (g_map_mode == 4) {
        /* (off: it comes back from its corner) */
        place_start();
        if (g_mode169)
            g_place[1] = low169;
        return;
    }
    if (g_nfaces <= 0)
        return;
    r3d_focal(focal);                   /* 0x49a120(1.0) */
    c = g_cos[0], s = g_cos[0x3000];
    turns = g_map_mode == 0 || g_map_mode == 1;
    if (turns) {
        /* the map turns so that the rider's way is up: 0x40f490 */
        int a = (int)(-bp_heading(&riders[follow].m[0][0]) * ANGLE_K);
        float ca = g_cos[a & 0x3fff], sa = g_cos[(a - 0x1000) & 0x3fff];
        n[0][0] = c * ca;
        n[1][0] = c * sa;
        n[0][2] = (sa - n[0][0]) * s;
        n[0][1] = s * ca * s + n[1][0];
        n[1][1] = n[0][0] - sa * s * s;
        n[1][2] = (n[1][0] + ca) * s;
    } else {
        n[0][0] = c * c;
        n[1][0] = s * c;
        n[0][2] = (s - n[0][0]) * s;
        n[0][1] = (s + 1.0f) * s * c;
        n[1][1] = n[0][0] - s * s * s;
        n[1][2] = n[0][1];
    }
    n[1][0] = -n[1][0];
    n[2][0] = s;
    n[2][1] = -(c * s);
    n[2][2] = c * c;
    if (side) {
        want[0] = -2000.0f;
        want[1] = g_mode169 ? low169 : -1200.0f;
        want[2] = 4000.0f;
    } else {
        want[0] = want[1] = 0.0f;
        want[2] = g_mode169 ? 2500.0f : 2000.0f;
    }
    /* half way there each picture (each 60th here) */
    for (i = 0; i < g_ticks; i++) {
        g_place[0] = (want[0] - g_place[0]) * 0.5f + g_place[0];
        g_place[1] = (want[1] - g_place[1]) * 0.5f + g_place[1];
        g_place[2] = (want[2] - g_place[2]) * 0.5f + g_place[2];
    }
    memcpy(t, g_place, sizeof t);
    /* the road: its first piece (the start line) white, the others grey; a brightness
       added to the half of the picture behind */
    {
        uint16_t grey = side ? rgb(8, 8, 8) : rgb(16, 16, 16);
        for (i = 0; i < g_nfaces; i++)
            g_faces[i].colour = i ? grey : rgb(0x1f, 0x1f, 0x1f);
    }
    base = r3d_points(g_npoints, (const float (*)[3])g_points, (const float (*)[3])n, t);
    r3d_key_faces(base, g_nfaces, g_faces, 15.0f, -1);
    /* the riders: red and orange by twos, the followed one yellow at the very front */
    size = g_map_mode == 1 || (g_map_mode == 3 && !behaviour_cd()) ? 30.0f : 50.0f;
    for (i = 0; i < g_riders; i++)
        if (g_racer[i].loaded)
            mark_points(i, &riders[i], size);
    base = r3d_points(MARK_POINTS, (const float (*)[3])g_mark_points, (const float (*)[3])n, t);
    for (i = 0; i < g_riders; i++)
        if (g_racer[i].loaded && i != follow) {
            g_mark[i].colour = i & 2 ? rgb(0x1f, 0, 0) : rgb(0x1f, 8, 8);
            r3d_key_faces(base, 1, &g_mark[i], 10.0f, -1);
        }
    g_mark[follow].colour = rgb(0x1f, 0x1f, 0);
    r3d_key_faces(base, 1, &g_mark[follow], 0.0f, -1);
}
