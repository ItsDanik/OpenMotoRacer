/* game.EXE: a Windows PE program, loaded at its preferred base 0x400000, so its sections
   only need placing at their addresses (the relocation records only list the places
   that hold an address; tools/pe2flat.py does the same for the tools). */
#include "exe.h"
#include "files.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE 0x400000u

/* FNV-1a of the whole file, for the builds known (NOTES §1) */
static const struct {
    unsigned sum;
    unsigned len;
    const char *what;
} builds[] = {
    { 0xd2534e97u, 1133056, "GOG game.EXE, 3.22, 1998-07-09" },
};

static unsigned char *g_image;
static unsigned g_size;

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

int exe_load(char *err, int errsize)
{
    long len = 0;
    unsigned char *d = files_read("game.exe", &len);
    unsigned sum, i;
    if (!d) {
        snprintf(err, errsize, "game.EXE is not in %s.\nPoint -data (or OMR_DATA) at the "
                 "Moto Racer folder that holds game.EXE and data/.", files_data());
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
    if (!place(d, (unsigned long)len)) {
        snprintf(err, errsize, "game.EXE could not be read.");
        free(d);
        return 0;
    }
    free(d);
    return 1;
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
