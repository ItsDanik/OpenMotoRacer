/* .BPT scripts (NOTES §3.4): a string is a plain u32 length and its characters, a number
   4 bytes; characters and numbers are XORed with the low byte of Rand_Next(6) (0x4b1160:
   Park-Miller, x16807 in Schrage's form, a result above 0x7fff taken mod 0x7fff), seeded
   with 0x1a2b3c4d when the file opens (0x4b1140). Lengths do not advance the key. */
#include "script.h"
#include "files.h"
#include "rand.h"
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
