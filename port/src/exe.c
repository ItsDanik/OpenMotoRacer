/* game.EXE: a Windows PE program, loaded at its preferred base 0x400000, so its sections
   only need placing at their addresses (the relocation records only list the places
   that hold an address; tools/pe2flat.py does the same for the tools). */
#include "exe.h"
#include "bitmap.h"
#include "files.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define BASE 0x400000u

/* FNV-1a of the whole file, for the builds known (NOTES §1), and where each keeps the
   tables the port reads. The 1997 CD's MOTO.EXE has the same tables record for record, its
   named colours lacking the grey, and lies inside the CD's installer archive when the game
   was never installed (NOTES §3.5a). */
static const struct {
    unsigned sum;
    unsigned len;
    const char *what;
    unsigned addr[X_COUNT];
} builds[] = {
    { 0xd2534e97u, 1133056, "GOG game.EXE, 3.22, 1998-07-09", { 0x4e3e20, 0x4e8b60, 0x4f0dd4, 0x4f2590,
                                                                0x4db7d8, 0x4db780, 0x4db770,
                                                                0x4f1250, 0x4f1248, 0x4e2920,
                                                                0x4db810, 0x4db8a0, 0x4db920,
                                                                0x4db680, 0x4e9128, 0x4f1640, 0x4f16a0,
                                                                0x4dc050 } },
    { 0x52c7ba57u, 1083392, "the 1997 CD's MOTO.EXE, 1997-07-21", { 0x4d9840, 0x4da8a8, 0x4e7790, 0x4d2bb8,
                                                                0x4de830, 0x4de7d8, 0x4de7c8,
                                                                0x4e7530, 0x4e7528, 0x4d8340,
                                                                0x4de868, 0x4de8f8, 0x4de978,
                                                                0x4de6d8, 0x4e4a68, 0x4e73f0, 0x4e7450,
                                                                0x4d2350 } },
};
static int g_build = -1;

static unsigned char *g_image;
static unsigned g_size;
static unsigned g_rsrc;         /* the resources' place in the image (the PE's directory 2) */

static unsigned u32(const unsigned char *p)
{
    return (unsigned)p[0] | (unsigned)p[1] << 8 | (unsigned)p[2] << 16 | (unsigned)p[3] << 24;
}

static unsigned u16(const unsigned char *p)
{
    return (unsigned)p[0] | (unsigned)p[1] << 8;
}

static unsigned fnv(const unsigned char *p, unsigned long n)
{
    unsigned h = 2166136261u;
    unsigned long i;
    for (i = 0; i < n; i++)
        h = (h ^ p[i]) * 16777619u;
    return h;
}

static int place(const unsigned char *d, unsigned long len)
{
    unsigned pe, nsec, opt, i;
    if (len < 0x40 || d[0] != 'M' || d[1] != 'Z')
        return 0;
    pe = u32(d + 0x3c);
    if (pe > len - 0xf8 || memcmp(d + pe, "PE\0\0", 4) != 0)
        return 0;
    nsec = u16(d + pe + 6);
    opt = pe + 0x18;
    if (u32(d + opt + 0x1c) != BASE)
        return 0;
    g_size = u32(d + opt + 0x38);
    g_rsrc = u32(d + opt + 0x70);
    g_image = calloc(1, g_size);
    if (!g_image)
        return 0;
    for (i = 0; i < nsec; i++) {
        const unsigned char *s = d + opt + u16(d + pe + 0x14) + 40 * i;
        unsigned va = u32(s + 12), raw = u32(s + 16), at = u32(s + 20), vsize = u32(s + 8);
        unsigned n = raw < vsize || !vsize ? raw : vsize;
        if ((unsigned long)at + n > len || va + n > g_size)
            return 0;
        memcpy(g_image + va, d + at, n);
    }
    return 1;
}

/* a stored file out of the CD's InstallShield 3 archive (tools/is3.py, NOTES §3.5): the
   header's counts, the folders' records, then the files' */
static unsigned char *archive_read(const char *rel, const char *want, long *len)
{
    char path[1024];
    unsigned char *d, *out = 0;
    long n;
    unsigned nfiles, ndirs, p, i;
    FILE *f;
    if (!files_find(rel, path, sizeof path) || !(f = fopen(path, "rb")))
        return 0;
    /* the table is at the end of the archive: only it and the one file are read */
    {
        unsigned char h[0x33];
        if (fread(h, 1, sizeof h, f) != sizeof h || u32(h) != 0x8c655d13u) {
            fclose(f);
            return 0;
        }
        nfiles = u16(h + 0x0c), p = u32(h + 0x29), ndirs = u16(h + 0x31);
    }
    if (fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) <= (long)p || fseek(f, (long)p, SEEK_SET) != 0
        || !(d = malloc((size_t)(n - (long)p)))) {
        fclose(f);
        return 0;
    }
    n -= (long)p;
    if (fread(d, 1, (size_t)n, f) == (size_t)n) {
        unsigned at = 0;
        for (i = 0; i < ndirs && at + 6 <= (unsigned)n; i++)
            at += u16(d + at + 2);
        for (i = 0; i < nfiles && at + 30 <= (unsigned)n; i++) {
            unsigned used = u32(d + at + 3), packed = u32(d + at + 7), off = u32(d + at + 11);
            unsigned nl = d[at + 29];
            if (strlen(want) == nl && at + 30 + nl <= (unsigned)n
                && strncasecmp((const char *)d + at + 30, want, nl) == 0) {
                if (used == packed && (out = malloc(used)) && fseek(f, (long)off, SEEK_SET) == 0
                    && fread(out, 1, used, f) == used) {
                    *len = (long)used;
                } else {
                    free(out);
                    out = 0;
                }
                break;
            }
            at += u16(d + at + 23);
        }
    }
    free(d);
    fclose(f);
    return out;
}

int exe_load(char *err, int errsize)
{
    long len = 0;
    unsigned char *d = files_read("game.exe", &len);
    unsigned sum, i;
    if (!d)
        d = files_read("moto.exe", &len);       /* the CD's, installed */
    if (!d)
        d = archive_read("disk1/data.z", "moto.exe", &len);     /* the CD's, as it is */
    if (!d) {
        snprintf(err, errsize, "game.EXE is not in %s.\nPoint -data (or OMR_DATA) at the "
                 "Moto Racer folder that holds game.EXE and data/, or at the CD's files.",
                 files_data());
        return 0;
    }
    sum = fnv(d, (unsigned long)len);
    for (i = 0; i < sizeof builds / sizeof builds[0]; i++)
        if (builds[i].sum == sum && builds[i].len == (unsigned)len)
            break;
    if (i == sizeof builds / sizeof builds[0]) {
        snprintf(err, errsize, "This game.EXE (%ld bytes, sum %08x) is not a build "
                 "OpenMotoRacer knows.", len, sum);
        free(d);
        return 0;
    }
    g_build = (int)i;
    if (!place(d, (unsigned long)len)) {
        snprintf(err, errsize, "game.EXE could not be read.");
        free(d);
        return 0;
    }
    free(d);
    return 1;
}

int exe_is_cd(void)
{
    return g_build == 1;
}

unsigned exe_addr(int what)
{
    return g_build >= 0 && what >= 0 && what < X_COUNT ? builds[g_build].addr[what] : 0;
}

const unsigned char *exe_at(unsigned va, unsigned len)
{
    if (!g_image || va < BASE || va - BASE > g_size || len > g_size - (va - BASE))
        return 0;
    return g_image + (va - BASE);
}

unsigned exe_u32(unsigned va)
{
    const unsigned char *p = exe_at(va, 4);
    return p ? u32(p) : 0;
}

const char *exe_str(unsigned va)
{
    const unsigned char *p = exe_at(va, 1);
    if (!p || !memchr(p, 0, g_size - (va - BASE)))
        return "";
    return (const char *)p;
}

/* one step down the resources' tree: the entry named by this number, or the first */
static unsigned rsrc_entry(unsigned dir, int id)
{
    const unsigned char *d = exe_at(BASE + g_rsrc + dir, 16), *e;
    unsigned n, i;
    if (!d)
        return 0;
    n = u16(d + 12) + u16(d + 14);
    if (!(e = exe_at(BASE + g_rsrc + dir + 16, 8 * n)))
        return 0;
    for (i = 0; i < n; i++, e += 8)
        if (id < 0 || u32(e) == (unsigned)id)
            return u32(e + 4);
    return 0;
}

const unsigned char *exe_resource(int type, int id, unsigned *len)
{
    unsigned a;
    const unsigned char *e;
    if (!g_rsrc || !(a = rsrc_entry(0, type)) || !(a & 0x80000000u)
        || !(a = rsrc_entry(a & 0x7fffffffu, id)) || !(a & 0x80000000u)
        || !(a = rsrc_entry(a & 0x7fffffffu, -1)) || (a & 0x80000000u)
        || !(e = exe_at(BASE + g_rsrc + a, 16)))
        return 0;
    *len = u32(e + 4);
    return exe_at(BASE + u32(e), *len);
}

const char *exe_text(int id)
{
    unsigned a;
    for (a = exe_addr(X_STRINGS); a && exe_at(a, 28) && exe_u32(a) != 0xffffffffu; a += 28)
        if (exe_u32(a) == (unsigned)id) {
            unsigned s = exe_u32(a + 4 + 4 * (unsigned)g_lang);
            return exe_str(s ? s : exe_u32(a + 4));
        }
    return "";
}
