/* The software 3D renderer (NOTES §4.9.3), as the menus set it up: focal length 1911,
   centre (320, 240), near plane 256. Every step of the transform is a float, as the game
   runs it with the x87 at 24-bit precision (0x49b2c0). */
#include "r3d.h"
#include "files.h"
#include "video.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 640
#define H 480
#define CX 320.0f               /* 0x60d3c4, 0x624f88: half the 640x480 view (0x499ba0) */
#define CY 240.0f
#define FOCAL 1911.0f           /* set by the menu (0x40a003) */
#define NEAR 256.0f             /* 0x625774, the menu's */
#define TO_INDEX 2607.5945876176133     /* 0x4d1bf8: 16384 / 2pi */

#define VERTS 16384             /* transformed points (0x6d20d8, 0x1c bytes each) */
#define BUCKETS 7999            /* 0x655d38: 4 units of depth each */
#define ENTRIES 7499            /* 0x65f0e0 */
#define FREE_MAX 256            /* free pieces a screen row can hold */
#define SPANS 0x2800            /* see-through pieces put off till last (0x68b040) */
#define BATCHES 1000            /* their triangles (0x625790) */

float g_cos[16384];
uint16_t *g_texpage[R3D_PAGES];

typedef struct {                /* 0x49b2c0's output record */
    float x, y, z;              /* in the view, x and y times the focal length */
    int sx, sy;                 /* on the screen when z >= NEAR */
} Point;

typedef struct {
    const Face *f;
    int base;                   /* its object's first point */
    int next;
} Entry;

typedef struct {
    int16_t y, x0, x1;
    uint8_t kind;
    int u, v, du, dv;
    const uint16_t *tex;
} Span;

static Point g_pt[VERTS];
static int g_npt;
static int g_head[BUCKETS];
static Entry g_entry[ENTRIES];
static int g_nentry;
static int16_t g_free[H][2 * FREE_MAX];   /* 0x623bd8: [start, end) pieces a row */
static int g_nfree[H];
static Span g_span[SPANS];
static int g_nspan;
static int g_batch[BATCHES + 1];          /* each batch's first span */
static int g_nbatch;
static float g_cam[3][3], g_campos[3];    /* the camera block 0x5161c0 */

void r3d_init(void)
{
    int k;
    for (k = 0; k < 16384; k++)                         /* 0x4901e0 */
        g_cos[k] = (float)cos(k * 6.283185307179586 / 16384.0);
    r3d_begin();
}

uint16_t *r3d_page(int n)
{
    if (n < 0 || n >= R3D_PAGES)
        return 0;
    if (!g_texpage[n])
        g_texpage[n] = calloc(65536, sizeof(uint16_t));
    return g_texpage[n];
}

void r3d_free_page(int n)
{
    if (n >= 0 && n < R3D_PAGES) {
        free(g_texpage[n]);
        g_texpage[n] = 0;
    }
}

/* the DirectDraw format's shifts applied as "top 5 bits": green keeps only 5 */
static uint16_t texel(const unsigned char *rgb)
{
    return (uint16_t)((rgb[0] >> 3) << 11 | (rgb[1] >> 3) << 6 | rgb[2] >> 3);
}

/* 0x401160: an Image Alchemy RAW, the first 256 texels of each row */
int r3d_load_page(int n, const char *path)
{
    long len;
    unsigned char *d = dsi_load(path, &len);
    uint16_t *t;
    int w, h, npal, x, y;
    if (!d)
        return 0;
    w = d[8] << 8 | d[9];
    h = d[10] << 8 | d[11];
    npal = d[12] << 8 | d[13];
    if (len < 32 || memcmp(d, "mhwanh", 6) || w < 256
        || len < 32 + 3L * npal + (long)w * h * (npal ? 1 : 3) || !(t = r3d_page(n))) {
        free(d);
        return 0;
    }
    if (h > 256)
        h = 256;
    for (y = 0; y < h; y++)
        for (x = 0; x < 256; x++)
            t[y * 256 + x] = npal ? texel(d + 32 + 3 * d[32 + 3 * npal + (long)y * w + x])
                                  : texel(d + 32 + 3 * ((long)y * w + x));
    free(d);
    return 1;
}

/* ---------------------------------------------------------------- the camera, objects */

void r3d_camera(const float pos[3], const float target[3])
{
    double d[3], l, nx, ny, nz, hz;
    int k;
    for (k = 0; k < 3; k++) {
        d[k] = (double)target[k] - pos[k];
        g_campos[k] = pos[k];
    }
    l = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    nx = d[0] / l, ny = d[1] / l, nz = d[2] / l;
    hz = sqrt(nx * nx + nz * nz);
    /* a yaw (nz/h, nx/h) then a pitch (h, ny), for row vectors */
    g_cam[0][0] = (float)(nz / hz);
    g_cam[0][1] = (float)(-nx / hz * ny);
    g_cam[0][2] = (float)(nx / hz * hz);
    g_cam[1][0] = 0.0f;
    g_cam[1][1] = (float)hz;
    g_cam[1][2] = (float)ny;
    g_cam[2][0] = (float)(-nx / hz);
    g_cam[2][1] = (float)(-nz / hz * ny);
    g_cam[2][2] = (float)(nz / hz * hz);
}

void r3d_camera_pos(const float pos[3])
{
    memcpy(g_campos, pos, sizeof g_campos);
}

void r3d_rotation(float m[3][3], const float ang[3], double k)
{
    int i[3], j;
    double c[3], s[3];
    for (j = 0; j < 3; j++) {
        i[j] = (int)(ang[j] * k);
        c[j] = g_cos[i[j] & 0x3fff];
        s[j] = g_cos[(i[j] - 0x1000) & 0x3fff];
    }
    m[0][0] = (float)(c[2] * c[1]);
    m[0][1] = (float)(s[2] * c[0] + c[2] * s[1] * s[0]);
    m[0][2] = (float)(s[2] * s[0] - c[2] * s[1] * c[0]);
    m[1][0] = (float)(-s[2] * c[1]);
    m[1][1] = (float)(c[2] * c[0] - s[2] * s[1] * s[0]);
    m[1][2] = (float)(c[2] * s[0] + s[2] * s[1] * c[0]);
    m[2][0] = (float)s[1];
    m[2][1] = (float)(-c[1] * s[0]);
    m[2][2] = (float)(c[1] * c[0]);
}

void r3d_object(const Model *m, const float ang[3], const float pos[3])
{
    float r[3][3], n[3][3], d[3], t[3];
    int i, j;
    r3d_rotation(r, ang, TO_INDEX);
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            n[i][j] = (float)((double)r[i][0] * g_cam[0][j] + (double)r[i][1] * g_cam[1][j]
                              + (double)r[i][2] * g_cam[2][j]);
    for (i = 0; i < 3; i++)
        d[i] = pos[i] - g_campos[i];
    for (j = 0; j < 3; j++)
        t[j] = (float)((double)d[0] * g_cam[0][j] + (double)d[1] * g_cam[1][j]
                       + (double)d[2] * g_cam[2][j]);
    r3d_view(m->nv, (const float (*)[3])m->v, m->nf, m->f, (const float (*)[3])n, t);
}

/* ---------------------------------------------------------------- transform and sort */

void r3d_begin(void)
{
    int y;
    g_npt = 0;
    g_nentry = 0;
    g_nspan = 0;
    g_nbatch = 0;
    for (y = 0; y < BUCKETS; y++)
        g_head[y] = -1;
    for (y = 0; y < H; y++) {
        g_free[y][0] = 0;
        g_free[y][1] = W;
        g_nfree[y] = 1;
    }
}

/* 0x49c590: into the bucket of its depth, the newest first */
static void queue(const Face *f, int base, float key)
{
    int b = (int)(key * 0.25f);
    if (b < 0 || g_nentry >= ENTRIES)
        return;
    if (b > BUCKETS - 1)
        b = BUCKETS - 1;
    g_entry[g_nentry].f = f;
    g_entry[g_nentry].base = base;
    g_entry[g_nentry].next = g_head[b];
    g_head[b] = g_nentry++;
}

void r3d_view(int nv, const float (*v)[3], int nf, const Face *f, const float n[3][3],
              const float t[3])
{
    float nx[4], ny[4], nz[4];
    int i, k, base = g_npt;
    if (g_npt + nv > VERTS)
        return;
    for (k = 0; k < 3; k++) {
        nx[k] = n[k][0] * FOCAL;
        ny[k] = n[k][1] * FOCAL;
        nz[k] = n[k][2];
    }
    nx[3] = t[0] * FOCAL;
    ny[3] = t[1] * FOCAL;
    nz[3] = t[2];
    for (i = 0; i < nv; i++) {                          /* 0x49b2c0 */
        Point *p = &g_pt[g_npt++];
        p->x = v[i][0] * nx[0] + v[i][1] * nx[1] + v[i][2] * nx[2] + nx[3];
        p->y = v[i][0] * ny[0] + v[i][1] * ny[1] + v[i][2] * ny[2] + ny[3];
        p->z = v[i][0] * nz[0] + v[i][1] * nz[1] + v[i][2] * nz[2] + nz[3];
        if (p->z >= NEAR) {
            float rz = 1.0f / p->z;
            p->sx = (int)nearbyintf(p->x * rz + CX);
            p->sy = (int)nearbyintf(CY - p->y * rz);
        }
    }
    for (i = 0; i < nf; i++) {                          /* 0x49b7f0, last argument 1 */
        const Face *fc = &f[i];
        const Point *p[4], *far;
        double area;
        int near = 0;
        if (fc->kind == 0xd)
            continue;
        for (k = 0; k < 4; k++)
            p[k] = &g_pt[base + fc->idx[k < fc->n ? k : 0]];
        far = p[0];
        for (k = 1; k < 4; k++)
            if (far->z < p[k]->z)
                far = p[k];
        if (!(far->z > NEAR))
            continue;
        for (k = 0; k < 4; k++)
            if (p[k]->z <= NEAR)
                near = 1;
        if (near) {             /* the back-face test in the view's space */
            const Point *a = p[1], *b = p[2], *c = p[0];
            double f6 = (double)a->y - b->y, f2 = (double)a->z - b->z;
            double f3 = (double)a->y - c->y, f4 = (double)a->z - c->z;
            area = far->z * (f6 * ((double)a->x - c->x) - ((double)a->x - b->x) * f3)
                   + (((double)a->x - b->x) * f4 - f2 * ((double)a->x - c->x)) * far->y
                   + far->x * (f2 * f3 - f6 * f4);
        } else {                /* wholly beyond a side of the screen, then the winding */
            int l = 1, r = 1, u = 1, d = 1;
            for (k = 0; k < 4; k++) {
                l &= p[k]->sx < 0;
                r &= p[k]->sx > W - 1;
                u &= p[k]->sy < 0;
                d &= p[k]->sy > H - 1;
            }
            if (l || r || u || d)
                continue;
            area = (double)(p[2]->sy - p[1]->sy) * (p[1]->sx - p[0]->sx)
                   + (double)(p[0]->sy - p[1]->sy) * (p[2]->sx - p[1]->sx);
        }
        if (!(area > 1.0))
            continue;
        queue(fc, base, far->z + (fc->flags & 2 ? 1024.0f : 0.0f));
    }
}

/* ---------------------------------------------------------------- drawing */

static int cdiv(int a, int b)   /* C's division, as the game's idiv */
{
    return a / b;
}

static void span(int y, int x0, int x1, int u, int v, int du, int dv, int kind,
                 const uint16_t *tex)
{
    uint16_t *o = g_screen + y * g_pitch;
    int x;
    for (x = x0; x < x1; x++, u += du, v += dv) {
        unsigned c;
        if (kind == 0x10) {                             /* 0x4a5ab0 */
            o[x] = tex[((v >> 13) & 0xff) << 8 | ((u >> 13) & 0xff)];
            continue;
        }
        c = tex[((v >> 13) * 256 + (u >> 13)) & 0xffff];  /* 0x4a5e10, 0x4a5330, 0x4a5580 */
        if ((kind == 0x26 || kind == 0x27) && c == 0)
            continue;
        if (kind == 0x15 || kind == 0x27)
            c = (c >> 1) & 0x7bef;
        o[x] = (uint16_t)c;
    }
}

/* 0x49d9e0 (opaque: draw the free pieces and close them) and 0x49dbf0 (see-through:
   keep the free pieces for the end) */
static void row(int y, int x0, int x1, int u, int v, int du, int dv, int kind,
                const uint16_t *tex)
{
    int16_t out[2 * FREE_MAX];
    int i, n = 0, deferred = kind == 0x26 || kind == 0x27;
    for (i = 0; i < g_nfree[y]; i++) {
        int a = g_free[y][2 * i], b = g_free[y][2 * i + 1];
        int s = a > x0 ? a : x0, e = b < x1 ? b : x1;
        if (s < e) {
            int us = u + (s - x0) * du, vs = v + (s - x0) * dv;
            if (deferred) {
                if (g_nspan < SPANS) {
                    Span *p = &g_span[g_nspan++];
                    p->y = (int16_t)y, p->x0 = (int16_t)s, p->x1 = (int16_t)e;
                    p->kind = (uint8_t)kind;
                    p->u = us, p->v = vs, p->du = du, p->dv = dv;
                    p->tex = tex;
                }
            } else {
                span(y, s, e, us, vs, du, dv, kind, tex);
                if (n + 2 <= FREE_MAX) {
                    if (a < s)
                        out[2 * n] = (int16_t)a, out[2 * n + 1] = (int16_t)s, n++;
                    if (e < b)
                        out[2 * n] = (int16_t)e, out[2 * n + 1] = (int16_t)b, n++;
                }
                continue;
            }
        }
        if (n < FREE_MAX)
            out[2 * n] = (int16_t)a, out[2 * n + 1] = (int16_t)b, n++;
    }
    memcpy(g_free[y], out, sizeof(int16_t) * 2 * n);
    g_nfree[y] = n;
}

typedef struct {
    int sx, sy, u, v;
} Corner;

/* 0x49dde0: an affine triangle, 13-bit fixed point, one du/dx and dv/dx */
static void triangle(Corner c0, Corner c1, Corner c2, int kind, const uint16_t *tex)
{
    int x0, y0, u0, v0, x1, y1, u1, v1, x2, y2, u2, v2, A, B, D, du, dv, r, nrow = 0;
    int deferred = kind == 0x26 || kind == 0x27, first = g_nspan;
    struct { int y, cnt, xl, sl, xr, sr, ul, dul, vl, dvl; } rows[2];
    while (c1.sy < c0.sy || c2.sy < c0.sy) {            /* the top corner first */
        Corner t = c0;
        c0 = c1, c1 = c2, c2 = t;
    }
    x0 = c0.sx, y0 = c0.sy, u0 = c0.u, v0 = c0.v;
    x1 = c1.sx, y1 = c1.sy, u1 = c1.u, v1 = c1.v;
    x2 = c2.sx, y2 = c2.sy, u2 = c2.u, v2 = c2.v;
    A = y1 - y2, B = y0 - y2;
    D = (x0 - x2) * A - (x1 - x2) * B;
    if (D < 1)
        return;
    du = (int)((double)(u0 - u2) / D * A - (double)(u1 - u2) / D * B);
    dv = (int)((double)(v0 - v2) / D * A - (double)(v1 - v2) / D * B);
    if (y2 < y1) {                                      /* the bend on corner 2's side */
        int sr, h;
        if (!(y0 < y1 && y1 >= 0))
            return;
        sr = cdiv((x1 - x0) * 0x2000, y1 - y0);
        h = y2 - y0;
        if (h >= 1 && y2 >= 0) {
            rows[nrow].y = y0, rows[nrow].cnt = h;
            rows[nrow].xl = x0 * 0x2000, rows[nrow].sl = cdiv((x2 - x0) * 0x2000, h);
            rows[nrow].xr = x0 * 0x2000, rows[nrow].sr = sr;
            rows[nrow].ul = u0, rows[nrow].dul = cdiv(u2 - u0, h);
            rows[nrow].vl = v0, rows[nrow].dvl = cdiv(v2 - v0, h);
            nrow++;
        }
        h = y1 - y2;
        if (h >= 1) {
            rows[nrow].y = y2, rows[nrow].cnt = h;
            rows[nrow].xl = x2 * 0x2000, rows[nrow].sl = cdiv((x1 - x2) * 0x2000, h);
            rows[nrow].xr = x0 * 0x2000 + (y2 - y0) * sr, rows[nrow].sr = sr;
            rows[nrow].ul = u2, rows[nrow].dul = cdiv(u1 - u2, h);
            rows[nrow].vl = v2, rows[nrow].dvl = cdiv(v1 - v2, h);
            nrow++;
        }
    } else {
        int sl, dul, dvl, h, h1, h2;
        if (!(y0 < y2 && y2 >= 0))
            return;
        h = y2 - y0;
        sl = cdiv((x2 - x0) * 0x2000, h);
        dul = cdiv(u2 - u0, h), dvl = cdiv(v2 - v0, h);
        h1 = y1 - y0;
        if (h1 >= 1 && y1 >= 0) {
            rows[nrow].y = y0, rows[nrow].cnt = h1;
            rows[nrow].xl = x0 * 0x2000, rows[nrow].sl = sl;
            rows[nrow].xr = x0 * 0x2000, rows[nrow].sr = cdiv((x1 - x0) * 0x2000, h1);
            rows[nrow].ul = u0, rows[nrow].dul = dul;
            rows[nrow].vl = v0, rows[nrow].dvl = dvl;
            nrow++;
        }
        h2 = y2 - y1;
        if (h2 >= 1) {
            rows[nrow].y = y1, rows[nrow].cnt = h2;
            rows[nrow].xl = x0 * 0x2000 + (y1 - y0) * sl, rows[nrow].sl = sl;
            rows[nrow].xr = x1 * 0x2000, rows[nrow].sr = cdiv((x2 - x1) * 0x2000, h2);
            rows[nrow].ul = u0 + (y1 - y0) * dul, rows[nrow].dul = dul;
            rows[nrow].vl = v0 + (y1 - y0) * dvl, rows[nrow].dvl = dvl;
            nrow++;
        }
    }
    for (r = 0; r < nrow; r++) {
        int y = rows[r].y, cnt = rows[r].cnt, xl = rows[r].xl, xr = rows[r].xr;
        int ul = rows[r].ul, vl = rows[r].vl;
        if (y < 0) {                                    /* rows above the screen skipped */
            xl -= y * rows[r].sl, xr -= y * rows[r].sr;
            ul -= y * rows[r].dul, vl -= y * rows[r].dvl;
            cnt += y, y = 0;
        }
        if (cnt > H - y)
            cnt = H - y;
        for (; cnt > 0; cnt--, y++) {
            row(y, xl >> 13, xr >> 13, ul, vl, du, dv, kind, tex);
            xl += rows[r].sl, xr += rows[r].sr, ul += rows[r].dul, vl += rows[r].dvl;
        }
    }
    if (deferred && g_nspan > first && g_nbatch < BATCHES)
        g_batch[g_nbatch++] = first;
}

/* 0x49d8f0 + 0x49e510: where the edge from B (behind) to F (in front) meets z = NEAR */
static Corner clip(const Point *b, int ub, int vb, const Point *f, int uf, int vf)
{
    Corner c;
    float x = (NEAR - b->z) * ((f->x - b->x) / (f->z - b->z)) + b->x;
    float y = (NEAR - b->z) * ((f->y - b->y) / (f->z - b->z)) + b->y;
    int dz = (int)(f->z - b->z), t = (int)(NEAR - b->z);
    c.sx = (int)CX + (int)(x / NEAR);
    c.sy = (int)CY - (int)(y / NEAR);
    c.u = ub + t * (dz ? cdiv(uf - ub, dz) : uf - ub);
    c.v = vb + t * (dz ? cdiv(vf - vb, dz) : vf - vb);
    return c;
}

/* 0x49e510: a fan of triangles, each cut at the near plane */
static void face(const Face *f, int base)
{
    const uint16_t *tex;
    int k;
    if (f->kind != 0x10 && f->kind != 0x15 && f->kind != 0x26 && f->kind != 0x27) {
        static int told;
        if (!told++)
            fprintf(stderr, "openmotoracer: 3D face kind %d is not written yet\n", f->kind);
        return;
    }
    tex = f->page >= 0 && f->page < R3D_PAGES ? g_texpage[f->page] : 0;
    if (!tex)
        return;
    for (k = 1; k < f->n - 1; k++) {
        int j[3] = { 0, k, k + 1 }, behind[3], nb = 0, i;
        const Point *p[3];
        Corner c[3];
        for (i = 0; i < 3; i++) {
            p[i] = &g_pt[base + f->idx[j[i]]];
            c[i].sx = p[i]->sx, c[i].sy = p[i]->sy;
            c[i].u = f->u[j[i]], c[i].v = f->v[j[i]];
            behind[i] = p[i]->z < NEAR;
            nb += behind[i];
        }
        if (nb == 0)
            triangle(c[0], c[1], c[2], f->kind, tex);
        else if (nb == 2) {                             /* one corner in front */
            int r = !behind[0] ? 0 : !behind[1] ? 1 : 2, a = (r + 1) % 3, b = (r + 2) % 3;
            triangle(c[r], clip(p[a], c[a].u, c[a].v, p[r], c[r].u, c[r].v),
                     clip(p[b], c[b].u, c[b].v, p[r], c[r].u, c[r].v), f->kind, tex);
        } else if (nb == 1) {                           /* two in front */
            int r = behind[0] ? 0 : behind[1] ? 1 : 2, a = (r + 1) % 3, b = (r + 2) % 3;
            Corner p1 = clip(p[r], c[r].u, c[r].v, p[a], c[a].u, c[a].v);
            Corner p2 = clip(p[r], c[r].u, c[r].v, p[b], c[b].u, c[b].v);
            triangle(c[a], c[b], p1, f->kind, tex);
            triangle(c[b], p2, p1, f->kind, tex);
        }
    }
}

void r3d_render(void)
{
    int b, e, i;
    for (b = 0; b < BUCKETS; b++)                       /* nearest first */
        for (e = g_head[b]; e >= 0; e = g_entry[e].next)
            face(g_entry[e].f, g_entry[e].base);
    g_batch[g_nbatch] = g_nspan;
    for (b = g_nbatch - 1; b >= 0; b--)                 /* 0x4af130: the last first */
        for (i = g_batch[b + 1] - 1; i >= g_batch[b]; i--) {
            const Span *p = &g_span[i];
            span(p->y, p->x0, p->x1, p->u, p->v, p->du, p->dv, p->kind, p->tex);
        }
    r3d_begin();
}
