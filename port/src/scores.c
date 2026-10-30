/* Data\bestscor.dat read as 0x464e80 and 0x4656c0 do, and written as 0x464680 (a race's
   finishers into the table), 0x464b70 (the table after page 9's names) and 0x4658b0 (a new
   best lap) do: the game rewrites its block in place; the port writes the whole file into
   the player's folder (saves.h), the same bytes. */
#include "scores.h"
#include "choices.h"
#include "saves.h"
#include "menu.h"
#include "race.h"
#include "rules.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

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

/* the track's table: the row with the player's name; without one the last row filled;
   0x7f all over when the table has none */
void scores_split_reference(Score *out)
{
    long len = 0;
    unsigned char *d = read_table(&len);
    const unsigned char *b = find_block(d, len);
    int i, last = -1;
    memset(out, 0x7f, sizeof *out);
    if (b) {
        const Score *row = (const Score *)(b + 0x20 + SCORE_SIZE);
        for (i = 0; i < SCORE_ROWS; i++) {
            const char *n = (const char *)row[i].b;
            if (n[0] != 0x7f && n[0] != 0)
                last = i;
            if (strncasecmp(n, g_player_name, 9) == 0) {
                *out = row[i];
                free(d);
                return;
            }
        }
        if (last >= 0)
            *out = row[last];
    }
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

/* ------------------------------------------------------------------ writing */

static void put32(unsigned char *p, unsigned v)
{
    p[0] = (unsigned char)v, p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16), p[3] = (unsigned char)(v >> 24);
}

/* the file in memory with the current key's block in it (appended when missing: the key,
   an empty best lap, 8 empty races), *at its place; a file without "00.1" starts again */
static unsigned char *open_block(long *len, long *at)
{
    unsigned char *d = read_table(len), *n;
    char key[0x40];
    long p;
    if (!d || *len < 4 || memcmp(d, "00.1", 4) != 0) {
        free(d);
        if (!(d = malloc(4)))
            return 0;
        memcpy(d, "00.1", 4);
        *len = 4;
    }
    scores_key(key, sizeof key);
    for (p = 4; p + BLOCK <= *len; p += BLOCK)
        if (strncmp((const char *)d + p, key, 0x20) == 0) {
            *at = p;
            return d;
        }
    if (!(n = realloc(d, (size_t)(*len + BLOCK)))) {
        free(d);
        return 0;
    }
    d = n;
    *at = *len;
    memset(d + *at, 0, 0x20);
    memcpy(d + *at, key, strlen(key) < 0x20 ? strlen(key) : 0x20);
    memset(d + *at + 0x20, 0x7f, BLOCK - 0x20);
    d[*at + 0x20 + 9] = 0;                      /* the best lap's name empty */
    put32(d + *at + 0x20 + 0xf5, SCORE_EMPTY);
    *len += BLOCK;
    return d;
}

static void close_block(unsigned char *d, long len)
{
    FILE *f;
    if (!d)
        return;
    if ((f = save_open("bestscor.dat", "wb"))) {
        if (fwrite(d, 1, (size_t)len, f) != (size_t)len)
            fprintf(stderr, "openmotoracer: bestscor.dat could not be written.\n");
        fclose(f);
    } else
        fprintf(stderr, "openmotoracer: bestscor.dat could not be written.\n");
    free(d);
}

static Score g_sort[25];                /* 0x464680's frame array */

static int by_time(unsigned a, unsigned b)      /* 0x4644a0: a - b on the race times */
{
    return (int)(score_time(&g_sort[a]) - score_time(&g_sort[b]));
}

/* 0x464680: every finished human's race into the track's table, kept to the best 6 */
void scores_write_race(void)
{
    Score sorted[25];
    unsigned idx[25];
    long len, at;
    unsigned char *d = open_block(&len, &at);
    int i, j, k, n;
    if (!d)
        return;
    memset(g_sort, 0x7f, sizeof g_sort);
    memcpy(g_sort, d + at + 0x20 + SCORE_SIZE, SCORE_ROWS * SCORE_SIZE);
    for (k = 1; k < RIDERS; k++) {
        const Racer *r = &g_racer[k];
        unsigned char *s = g_sort[6].b;
        if (!r->loaded || r->state != RS_FINISHED || !r->human)
            continue;
        memset(s, 0, 9);
        memcpy(s, g_player_name, strlen(g_player_name) < 8 ? strlen(g_player_name) : 8);
                                                    /* the rider's name +0x2dd4 */
        memcpy(s + 9, "TINI", 4);                   /* page 9's "new row" */
        for (i = 0; i < 0x30; i++)
            put32(s + 0xd + 4 * i, (unsigned)r->split[i]);
        s[0xcd] = 0;
        put32(s + 0xf1, (unsigned)r->time);
        put32(s + 0xf5, (unsigned)r->best);
    }
    for (i = 0, n = 25; i < n; i++)             /* the same record twice: the later goes */
        for (j = i + 1; j < n;)
            if (memcmp(&g_sort[i], &g_sort[j], SCORE_SIZE) == 0) {
                memmove(&g_sort[j], &g_sort[j + 1], (size_t)(24 - j) * SCORE_SIZE);
                memset(&g_sort[24], 0x7f, SCORE_SIZE);
                n--;
            } else
                j++;
    for (i = 0; i < 25; i++)
        idx[i] = (unsigned)i;
    msvc_qsort(idx, 25, by_time);
    for (i = 0; i < 25; i++)
        sorted[i] = g_sort[idx[i]];
    for (i = 6; i < 25; i++)
        memset(&sorted[i], 0x7f, SCORE_SIZE);
    memcpy(d + at + 0x20 + SCORE_SIZE, sorted, SCORE_ROWS * SCORE_SIZE);
    memcpy(g_scores, sorted, sizeof g_scores);
    close_block(d, len);
}

/* 0x464b70: the table as page 9 left it (the names typed) */
void scores_write_rows(void)
{
    long len, at;
    unsigned char *d = open_block(&len, &at);
    if (!d)
        return;
    memcpy(d + at + 0x20 + SCORE_SIZE, g_scores, sizeof g_scores);
    close_block(d, len);
}

/* 0x4658b0: the track's best lap */
void best_lap_write(const Score *s)
{
    long len, at;
    unsigned char *d = open_block(&len, &at);
    if (!d)
        return;
    memcpy(d + at + 0x20, s, SCORE_SIZE);
    close_block(d, len);
}
