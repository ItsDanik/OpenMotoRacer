/* 3D models: .3DF (the shape) and .MAP (each face's texture), as 0x4216a0 loads them with
   its last argument 1 (NOTES §4.9.3.1, §4.9.4) */
#include "r3d.h"
#include "files.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned u16(const unsigned char *p)
{
    return p[0] | p[1] << 8;
}

static uint32_t u32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static float f32(const unsigned char *p)
{
    uint32_t v = u32(p);
    float f;
    memcpy(&f, &v, 4);
    return f;
}

int model_load(Model *m, const char *base)
{
    char path[256];
    unsigned char *d, *mp;
    long len, mlen, p;
    int i, k, nf = 0;
    memset(m, 0, sizeof *m);
    snprintf(path, sizeof path, "%s.3df", base);
    d = dsi_load(path, &len);
    snprintf(path, sizeof path, "%s.map", base);
    mp = dsi_load(path, &mlen);
    if (!d || !mp || len < 4 || u16(d) != 1 || mlen < 0x200 || !(mp[1] & 0x80))
        goto bad;
    m->nv = (int)u16(d + 2);
    p = 4 + 12L * m->nv;
    for (;;) {                                          /* count the faces */
        unsigned n;
        if (p + 2 > len)
            goto bad;
        if (!(n = u16(d + p)))
            break;
        p += 4 + 2L * n;
        nf++;
    }
    if (mlen < 0x200 + 0x80L * nf)
        goto bad;
    m->v = malloc(sizeof *m->v * (m->nv ? m->nv : 1));
    m->f = calloc(nf ? nf : 1, sizeof *m->f);
    if (!m->v || !m->f)
        goto bad;
    for (i = 0; i < m->nv; i++)
        for (k = 0; k < 3; k++)
            m->v[i][k] = f32(d + 4 + 12 * i + 4 * k) * 96.0f;     /* 0x4d0690 */
    p = 4 + 12L * m->nv;
    for (i = 0; i < nf; i++) {
        const unsigned char *r = mp + 0x200 + 0x80 * i;
        Face *f = &m->f[m->nf];
        unsigned n = u16(d + p), mkind = u16(r);
        uint32_t fl = u32(r + 0x48);
        int idx[5];
        if (n > 4) {            /* (the game takes up to 5 corners; these files have 3, 4) */
            fprintf(stderr, "openmotoracer: %s: a face of %u corners skipped\n", base, n);
            p += 4 + 2L * n;
            continue;
        }
        for (k = 0; k < (int)n; k++) {
            idx[k] = (int)u16(d + p + 4 + 2 * k);
            if (idx[k] >= m->nv)
                goto bad;
        }
        p += 4 + 2L * n;
        f->n = (int)n;
        f->colour = (int)u16(r + 0x4e);
        f->page = (int)(fl & 0xffff);
        f->flags = 4 | (fl & 0x20000 ? 1 : 0) | (fl & 0x100000 ? 8 : 0);
        if (mkind < 3) {                                /* untextured */
            f->kind = fl & 0x80000 ? 0xc : fl & 0x40000 ? 0xd : fl & 0x10000 ? 2 : 0;
        } else {
            if (mkind != n)
                goto bad;
            f->kind = fl & 0x200000 ? (fl & 0x10000 ? 0x27 : 0x15) : (fl & 0x10000 ? 0x26 : 0x10);
            if (fl & 0x40000)
                f->kind = 0xd;
            if (fl & 0x80000)
                f->kind = 8;
            if (fl & 0x400000)
                f->flags |= 2;
        }
        /* the corners in the reverse order, each with its own (u, v) */
        for (k = 0; k < (int)n; k++) {
            int j = (int)n - 1 - k;
            f->idx[k] = idx[j];
            if (mkind >= 3) {
                f->u[k] = (int)u32(r + 0x22 + 4 * j) * 8192;
                f->v[k] = (int)u32(r + 0x32 + 4 * j) * 8192;
            }
        }
        m->nf++;
    }
    for (i = 0; i < m->nv; i++)                         /* 0x422070: the mirror */
        m->v[i][0] = -m->v[i][0];
    free(d);
    free(mp);
    return 1;
bad:
    fprintf(stderr, "openmotoracer: the model %s could not be read\n", base);
    free(d);
    free(mp);
    model_free(m);
    return 0;
}

void model_centre(Model *m)
{
    float lo[3], hi[3], c[3];
    int i, k;
    if (!m->nv)
        return;
    for (k = 0; k < 3; k++)
        lo[k] = hi[k] = m->v[0][k];
    for (i = 1; i < m->nv; i++)
        for (k = 0; k < 3; k++) {
            if (m->v[i][k] < lo[k])
                lo[k] = m->v[i][k];
            if (m->v[i][k] > hi[k])
                hi[k] = m->v[i][k];
        }
    for (k = 0; k < 3; k++)
        c[k] = (lo[k] + hi[k]) * 0.5f;
    for (i = 0; i < m->nv; i++)
        for (k = 0; k < 3; k++)
            m->v[i][k] -= c[k];
}

void model_scale(Model *m, float sx, float sy, float sz)
{
    int i;
    for (i = 0; i < m->nv; i++) {
        m->v[i][0] *= sx;
        m->v[i][1] *= sy;
        m->v[i][2] *= sz;
    }
}

void model_page_add(Model *m, int add)
{
    int i;
    for (i = 0; i < m->nf; i++)
        m->f[i].page += add;
}

void model_free(Model *m)
{
    free(m->v);
    free(m->f);
    memset(m, 0, sizeof *m);
}
