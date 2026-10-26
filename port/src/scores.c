/* Data\bestscor.dat read as 0x464e80 and 0x4656c0 do. (Writing it, 0x464b70 and 0x4658b0,
   comes with the race, which makes the new records.) */
#include "scores.h"
#include "choices.h"
#include "saves.h"
#include "menu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK (0x20 + SCORE_SIZE * (1 + SCORE_ROWS))

Score g_scores[SCORE_ROWS];
Score g_best_lap;

static unsigned u32(const unsigned char *p)
{
    return (unsigned)p[0] | (unsigned)p[1] << 8 | (unsigned)p[2] << 16 | (unsigned)p[3] << 24;
}

const char *score_name(const Score *s)
{
    return (const char *)s->b + 9;
}

unsigned score_time(const Score *s)
{
    return u32(s->b + 0xf1);
}

unsigned score_lap(const Score *s)
{
    return u32(s->b + 0xf5);
}

void scores_key(char *key, int size)
{
    snprintf(key, (size_t)size, "%s%s%s", g_tracks[g_track].name, g_pocket ? "POC" : "",
             g_reverse ? "REV" : "");
    if (g_mode == MODE_PRACTICE) {
        size_t n = strlen(key);
        snprintf(key + n, (size_t)size - n, "TRA%d", g_laps);
    }
}

/* the block of the current key: its best lap and its races; 0 when not there */
static const unsigned char *find_block(unsigned char *d, long len)
{
    char key[0x40], have[0x21];
    long p;
    if (!d || len < 4 || memcmp(d, "00.1", 4) != 0)
        return 0;
    scores_key(key, sizeof key);
    for (p = 4; p + BLOCK <= len; p += BLOCK) {
        memcpy(have, d + p, 0x20);
        have[0x20] = 0;
        if (strcmp(have, key) == 0)
            return d + p;
    }
    return 0;
}

/* the whole of bestscor.dat (the player's folder first, saves.h) */
static unsigned char *read_table(long *len)
{
    FILE *f = save_open("bestscor.dat", "rb");
    unsigned char *d = 0;
    long n;
    *len = 0;
    if (!f)
        return 0;
    if (fseek(f, 0, SEEK_END) == 0 && (n = ftell(f)) > 0 && fseek(f, 0, SEEK_SET) == 0
        && (d = malloc((size_t)n)) && fread(d, 1, (size_t)n, f) == (size_t)n)
        *len = n;
    else {
        free(d);
        d = 0;
    }
    fclose(f);
    return d;
}

void scores_load(void)
{
    long len = 0;
    unsigned char *d = read_table(&len);
    const unsigned char *b = find_block(d, len);
    if (b)
        memcpy(g_scores, b + 0x20 + SCORE_SIZE, sizeof g_scores);
    else
        memset(g_scores, 0x7f, sizeof g_scores);
    free(d);
}

int best_lap_load(Score *out)
{
    long len = 0;
    unsigned char *d = read_table(&len);
    const unsigned char *b = find_block(d, len);
    if (b)
        memcpy(out, b + 0x20, sizeof *out);
    free(d);
    return b != 0;
}
