/* .BPT scripts (NOTES §3.4): a string is a plain u32 length and its characters, a number
   4 bytes; characters and numbers are XORed with the low byte of Rand_Next(6) (0x4b1160:
   Park-Miller, x16807 in Schrage's form, a result above 0x7fff taken mod 0x7fff), seeded
   with 0x1a2b3c4d when the file opens (0x4b1140). Lengths do not advance the key. */
#include "script.h"
#include "files.h"
#include "rand.h"
#include "saves.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned key_byte(Script *s)
{
    uint32_t x = s->seed;
    unsigned b = (unsigned)rand_step(&x) & 0xff;
    s->seed = x;
    return b;
}

int script_open(Script *s, const char *spt)
{
    char path[256];
    char *dot;
    memset(s, 0, sizeof *s);
    snprintf(path, sizeof path, "%s", spt);
    dot = strrchr(path, '.');
    if (dot && (size_t)(dot - path) + 4 < sizeof path)
        strcpy(dot, ".BPT");
    s->d = dsi_load(path, &s->len);
    s->seed = 0x1a2b3c4d;
    return s->d != 0;
}

void script_open_mem(Script *s, unsigned char *d, long len)
{
    memset(s, 0, sizeof *s);
    s->d = d;
    s->len = len;
    s->seed = 0x1a2b3c4d;
}

int script_open_own(Script *s, const char *name)
{
    FILE *f = save_open_own(name, "rb");
    unsigned char *d = 0;
    long len;
    int ok = 0;
    if (!f)
        return 0;
    if (fseek(f, 0, SEEK_END) == 0 && (len = ftell(f)) > 0 && fseek(f, 0, SEEK_SET) == 0
        && (d = malloc((size_t)len)) && fread(d, 1, (size_t)len, f) == (size_t)len) {
        script_open_mem(s, d, len);
        ok = 1;
    } else
        free(d);
    fclose(f);
    return ok;
}

void script_close(Script *s)
{
    free(s->d);
    s->d = 0;
}

static uint32_t raw4(Script *s)
{
    unsigned char b[4];
    int i;
    if (s->p + 4 > s->len) {
        s->bad = 1;
        s->p = s->len;
        return 0;
    }
    for (i = 0; i < 4; i++)
        b[i] = s->d[s->p + i] ^ (unsigned char)key_byte(s);
    s->p += 4;
    return (uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
}

int script_int(Script *s)
{
    return (int)raw4(s);
}

float script_float(Script *s)
{
    uint32_t v = raw4(s);
    float f;
    memcpy(&f, &v, 4);
    return f;
}

void script_str(Script *s, char *out, int size)
{
    uint32_t n;
    long i;
    out[0] = 0;
    if (s->p + 4 > s->len) {
        s->bad = 1;
        return;
    }
    n = (uint32_t)s->d[s->p] | (uint32_t)s->d[s->p + 1] << 8 | (uint32_t)s->d[s->p + 2] << 16
        | (uint32_t)s->d[s->p + 3] << 24;
    s->p += 4;
    if (n > (uint32_t)(s->len - s->p)) {
        s->bad = 1;
        s->p = s->len;
        return;
    }
    for (i = 0; i < (long)n; i++) {
        char c = (char)(s->d[s->p + i] ^ key_byte(s));
        if (i < size - 1) {
            out[i] = c;
            out[i + 1] = 0;
        }
    }
    s->p += n;
}

int script_at_end(const Script *s)
{
    return s->p == s->len && !s->bad;
}

/* ---------------------------------------------------------------- writing */

void script_out_init(ScriptOut *o)
{
    memset(o, 0, sizeof *o);
    o->seed = 0x1a2b3c4d;
}

static void put(ScriptOut *o, const unsigned char *b, long n, int cipher)
{
    long i;
    if (o->len + n > o->cap) {
        long cap = o->cap ? o->cap * 2 : 256;
        unsigned char *d;
        while (cap < o->len + n)
            cap *= 2;
        if (!(d = realloc(o->d, (size_t)cap))) {
            o->bad = 1;
            return;
        }
        o->d = d;
        o->cap = cap;
    }
    for (i = 0; i < n; i++) {
        uint32_t x = o->seed;
        unsigned k = 0;
        if (cipher) {
            k = (unsigned)rand_step(&x) & 0xff;
            o->seed = x;
        }
        o->d[o->len++] = (unsigned char)(b[i] ^ k);
    }
}

void script_out_int(ScriptOut *o, int v)
{
    unsigned char b[4];
    b[0] = (unsigned char)v, b[1] = (unsigned char)(v >> 8);
    b[2] = (unsigned char)(v >> 16), b[3] = (unsigned char)((unsigned)v >> 24);
    put(o, b, 4, 1);
}

void script_out_str(ScriptOut *o, const char *str)
{
    unsigned char b[4];
    long n = (long)strlen(str);
    b[0] = (unsigned char)n, b[1] = (unsigned char)(n >> 8);
    b[2] = (unsigned char)(n >> 16), b[3] = (unsigned char)(n >> 24);
    put(o, b, 4, 0);            /* (lengths plain, as read) */
    put(o, (const unsigned char *)str, n, 1);
}

void script_out_free(ScriptOut *o)
{
    free(o->d);
    o->d = 0;
}
