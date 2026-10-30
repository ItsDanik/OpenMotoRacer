/* The race's rules (NOTES §4.17): the countdown and the time limit (0x47dae0), laps,
   checkpoints, the wrong way and the finish (0x47e3d0), the places (0x47e1e0), the
   fastest lap (0x481800) and the lap record (0x458310). The dashboard's messages
   these routines put up are hud.c's. */
#include "rules.h"
#include "choices.h"
#include "ghost.h"
#include "hud.h"
#include "land.h"
#include "menu.h"
#include "race.h"
#include "rand.h"
#include "scores.h"
#include "signs.h"
#include "sound.h"
#include <string.h>

Racer g_racer[RIDERS];
int g_counting, g_time_left, g_time_over, g_race_over, g_control, g_extended;
int g_order[32], g_norder, g_fastest, g_race_best_lap;

static int g_ncheck;            /* 0x4e9844: the track's checkpoint sections */
static int g_wrong_said;        /* 0x4f0c58: WRONGWAY said, until a section forward */
static unsigned g_clock;        /* 0x50bbc8 for the rules: the race's own ms */
static unsigned g_hurry_at;     /* 0x4f0c54: the last TIMEOUT voice */
static int g_hurry_said;
static Score g_record;          /* 0x581118: the track's records (bestscor.dat) */
static Score g_reference;       /* 0x581018: the race the splits are measured against */
char g_split[12];

static int shown(void)          /* 0x4119a0: the player's record (1 in Solo) */
{
    int k;
    for (k = 1; k < g_riders; k++)
        if (g_racer[k].loaded && g_racer[k].human)
            return k;
    return g_race_kind == 2 ? race_player() : -1;
}

/* every rider counts in Solo (0x4ea4c4 = 1); Link's remote players would not */
#define COUNTS(k) 1

static int wrap(int s)          /* 0x40ec30 */
{
    while (s < 0)
        s += g_land.nsec;
    while (s >= g_land.nsec)
        s -= g_land.nsec;
    return s;
}

/* ------------------------------------------------------------------ MSVC's qsort */

/* 0x4b3100, the C library's quicksort as the game has it (the order it leaves equal keys
   in decides the places of riders with equal times) */
typedef QsortCmp Cmp;

static void swap(unsigned *a, unsigned *b)
{
    unsigned t = *a;
    *a = *b;
    *b = t;
}

static void shortsort(unsigned *lo, unsigned *hi, Cmp cmp)     /* 0x4b3270 */
{
    while (hi > lo) {
        unsigned *max = lo, *p;
        for (p = lo + 1; p <= hi; p++)
            if (cmp(*p, *max) > 0)
                max = p;
        swap(max, hi);
        hi--;
    }
}

void msvc_qsort(unsigned *base, int num, Cmp cmp)
{
    unsigned *lostk[30], *histk[30], *lo, *hi;
    int sp = 0;
    if (num < 2)
        return;
    lo = base;
    hi = base + num - 1;
    for (;;) {
        int size = (int)(hi - lo) + 1;
        if (size <= 8)
            shortsort(lo, hi, cmp);
        else {
            unsigned *loguy = lo, *higuy = hi + 1;
            swap(lo + size / 2, lo);
            for (;;) {
                do
                    loguy++;
                while (loguy <= hi && cmp(*loguy, *lo) <= 0);
                do
                    higuy--;
                while (higuy > lo && cmp(*higuy, *lo) >= 0);
                if (higuy < loguy)
                    break;
                swap(loguy, higuy);
            }
            swap(lo, higuy);
            if (higuy - 1 - lo >= hi - loguy) {
                if (lo + 1 < higuy) {
                    lostk[sp] = lo;
                    histk[sp++] = higuy - 1;
                }
                if (loguy < hi) {
                    lo = loguy;
                    continue;
                }
            } else {
                if (loguy < hi) {
                    lostk[sp] = loguy;
                    histk[sp++] = hi;
                }
                if (lo + 1 < higuy) {
                    hi = higuy - 1;
                    continue;
                }
            }
        }
        if (--sp < 0)
            return;
        lo = lostk[sp];
        hi = histk[sp];
    }
}

static int descending(unsigned a, unsigned b)   /* 0x457730, on the low 27 bits */
{
    int x = (int)(a & 0x7ffffff), y = (int)(b & 0x7ffffff);
    return y > x ? 1 : y >= x ? 0 : -1;
}

static int ascending(unsigned a, unsigned b)    /* 0x4817d0 */
{
    a &= 0x7ffffff, b &= 0x7ffffff;
    return a > b ? 1 : a < b ? -1 : 0;
}

/* ------------------------------------------------------------------ the start */

/* 0x47bf70 at 0x47cff4 (0x45bd40 / 0x45bb30 the same): the progress of a rider on the
   grid, negative behind the line */
void rules_progress(int k)
{
    Racer *r = &g_racer[k];
    int s = g_rider[k].sec, n = g_land.nsec, half = n / 2;
    r->prev = s;
    if (!g_reverse)
        r->progress = s >= half ? s - n : s;
    else
        r->progress = s >= half ? n - s : -s;
    r->furthest = r->progress;
}

/* 0x411010's rules part (and the race sound init 0x4084e0's countdown reset) */
void rules_restart(void)
{
    int k, i;
    g_control = 0;
    g_counting = 1;
    g_time_left = 6000;
    for (k = 1; k < g_riders; k++) {
        Racer *r = &g_racer[k];
        r->cp = 1;
        r->laps = 0;
        r->time = 0;
        r->best = NO_TIME;                      /* 0x47bf70 at 0x47cf82 */
        r->state = RS_GRID;
        r->place = 7;
        for (i = 0; i < SPLITS - 1; i++)
            r->split[i] = 0;
        memset(r->lap, 0, sizeof r->lap);
    }
    g_time_over = g_race_over = 0;
    g_race_best_lap = NO_TIME;
    g_extended = 0;
    g_wrong_said = 0;
    g_clock = 0;
    g_hurry_said = 0;
    g_fastest = 0;
    /* 0x4656c0: this track's records, none when the file has no block for it */
    if (!best_lap_load(&g_record))
        memset(&g_record, 0x7f, sizeof g_record);
    /* 0x411205: the player's own best here, else the table's slowest; in the time attack the
       ghost's, and none without a ghost */
    if (g_mode != MODE_PRACTICE || !g_time_attack)
        scores_split_reference(&g_reference);
    else if (!ghost_count())
        memset(&g_reference, 0x7f, sizeof g_reference);
    for (g_ncheck = k = 0; k < g_land.nsec; k++)          /* 0x47b4ff */
        if (g_land.sec[k].checkpoint >= 0)
            g_ncheck++;
    rules_places();
}

/* ------------------------------------------------------------------ the clock */

/* 0x47dae0: the countdown, then the time left; GO; the time running out */
void rules_time(int ms)
{
    int k, me = shown();
    g_clock += (unsigned)ms;
    g_time_left -= ms;
    if (!g_counting)
        for (k = 0; k < g_riders; k++) {
            Racer *r = &g_racer[k];
            if (r->loaded && (r->state == RS_GRID || r->state == RS_RACING)) {
                r->time += ms;
                if (r->laps < 8)
                    r->lap[r->laps] += ms;
            }
        }
    if (g_time_left >= 0)
        return;
    if (g_counting) {                           /* GO */
        int n = g_land.nsec, half = n / 2;
        g_counting = 0;
        if (n > 0 && g_land.sec[0].checkpoint >= 0)
            g_time_left = g_land.sec[0].time + g_land.time_skill[g_difficulty % 3];
        for (k = 0; k < g_riders; k++)
            if (g_racer[k].loaded) {
                g_racer[k].time = 0;
                memset(g_racer[k].lap, 0, sizeof g_racer[k].lap);
            }
        /* a rider already past the line races at once; one behind it starts at its
           first crossing */
        for (k = 0; k < g_riders; k++) {
            int s = g_rider[k].sec;
            if (!g_reverse ? s >= 0 && s < half : s < 0 || s >= half)
                g_racer[k].state = RS_RACING;
        }
        g_control = 1;
        return;
    }
    if (g_playback || me < 0 || (g_racer[me].state != RS_GRID && g_racer[me].state != RS_RACING)
        || (g_mode == MODE_PRACTICE && !g_time_attack))
        return;
    if (g_time_left + 500 < 0) {                /* time over */
        g_time_over = 1;
        sound_queue_voice(0x39, 500);           /* GAMEOVER */
        hud_game_over();                        /* 0x482120(0) */
        g_racer[me].state = RS_OUT;
    } else if (!g_hurry_said || g_hurry_at + 2000 < g_clock) {
        hud_message(HUD_TIME_OUT);              /* 0x481af0 */
        g_hurry_at = g_clock;
        g_hurry_said = 1;
        sound_queue_voice(0x42, 500);           /* TIMEOUT */
    }
}

const unsigned char *rules_record(void)
{
    return g_record.b;
}

unsigned char *rules_reference(void)
{
    return g_reference.b;
}

void rules_take_ghost(int k)
{
    memcpy(g_reference.b + 0xd, g_racer[k].split, 0xc0);
    memcpy(g_reference.b, "SHADOW", 7);         /* 0x4dbea8 */
    g_record = g_reference;
}

/* ------------------------------------------------------------------ laps */

/* 0x458310: a lap better than the track's record and this race's best */
static void lap_record(int k)
{
    Racer *r = &g_racer[k];
    int b = r->best, rec = (int)score_lap(&g_record);
    if (!g_solo || !r->human || b == NO_TIME || b == 0)
        return;
    if ((rec == NO_TIME || rec > b) && (g_race_best_lap > b || g_race_best_lap == NO_TIME)) {
        /* (0x482dc0: the record message; the holder's name kept) */
        g_race_best_lap = b;
        sound_queue_voice(0x3b, 500);           /* NEWRECOR */
    }
}

/* 0x481800 */
static void fastest_lap(void)
{
    unsigned key[RIDERS];
    int k, n = 0;
    for (k = 0; k < RIDERS && k < g_riders; k++)
        if (g_racer[k].loaded && g_racer[k].best != NO_TIME)
            key[n++] = ((unsigned)g_racer[k].best & 0x7ffffff) | (unsigned)k << 27;
    if (n > 1)
        msvc_qsort(key, n, ascending);
    if (n > 0)
        g_fastest = (int)(key[0] >> 27);
}

/* 0x412e50 (with 0x46d4b0): the race time at the checkpoint passed, and for the rider
   shown the difference to the reference's time there, for the dashboard */
static void split(int k, int is_shown)
{
    Racer *r = &g_racer[k];
    int i = r->cp == 0 ? (r->laps + 1) * g_ncheck - 1 : r->laps * g_ncheck - 1 + r->cp;
    int d, ref, ms, sign;
    if (i > 0x30)
        i = -1;
    if (i >= 0)
        r->split[i] = r->time;
    if (!is_shown)
        return;
    g_split[0] = 0;
    if (i < 0 || g_reference.b[0] == 0x7f)
        return;
    ref = (int)((unsigned)g_reference.b[0xd + 4 * i] | (unsigned)g_reference.b[0xe + 4 * i] << 8
                | (unsigned)g_reference.b[0xf + 4 * i] << 16
                | (unsigned)g_reference.b[0x10 + 4 * i] << 24);
    if (ref == 0 || ref == NO_TIME)
        return;
    d = r->split[i] - ref;
    sign = d == 0 ? ' ' : d < 0 ? '-' : '+';
    ms = d < 0 ? -d : d;
    /* (cut at 10 characters: a difference of an hour does not fit anyway) */
    g_split[0] = (char)sign;
    g_split[1] = (char)('0' + ms / 600000 % 6), g_split[2] = (char)('0' + ms / 60000 % 10);
    g_split[3] = ':';
    g_split[4] = (char)('0' + ms / 10000 % 6), g_split[5] = (char)('0' + ms / 1000 % 10);
    g_split[6] = ';';
    g_split[7] = (char)('0' + ms / 100 % 10), g_split[8] = (char)('0' + ms / 10 % 10);
    g_split[9] = 0;
}

/* 0x463c30, 0x463ca0: every rider's result, around watching a replay */
static struct {
    int time, best, state, cp, laps, place;
} g_results[RIDERS];

void rules_save_results(void)
{
    int k;
    for (k = 1; k < RIDERS; k++) {
        const Racer *r = &g_racer[k];
        g_results[k].time = r->time;
        g_results[k].best = r->best;
        g_results[k].state = r->state;
        g_results[k].cp = r->cp;
        g_results[k].laps = r->laps;
        g_results[k].place = r->place;
    }
}

void rules_restore_results(void)
{
    int k;
    for (k = 1; k < RIDERS; k++) {
        Racer *r = &g_racer[k];
        r->time = g_results[k].time;
        r->best = g_results[k].best;
        r->state = g_results[k].state;
        r->cp = g_results[k].cp;
        r->laps = g_results[k].laps;
        r->place = g_results[k].place;
    }
}

/* 0x47e3d0, every frame after the step */
void rules_laps(void)
{
    int k, me = shown(), anyline = 0;
    int n = g_land.nsec, half = n / 2, dir = g_reverse ? -1 : 1;
    if (n <= 0)
        return;
    for (k = 1; k < g_riders; k++) {
        Racer *r = &g_racer[k];
        Rider *rd = &g_rider[k];
        int cp = 0, line = 0, cur;
        if (!r->loaded)
            continue;
        cur = rd->sec = land_walk(rd->pos, rd->sec, &r->dist);
        if (cur != r->prev) {
            int d;
            if (!g_reverse) {
                d = cur - r->prev;
                if (d > half)
                    d -= n;
                if (d < -half) {
                    d += n;
                    if (COUNTS(k) && r->progress == r->furthest)
                        line = 1;
                }
            } else {
                d = r->prev - cur;
                if (r->prev >= 0 && r->prev < half && cur > half && COUNTS(k)
                    && r->progress == r->furthest)
                    line = 1;
                if (d > half)
                    d -= n;
                if (d < -half)
                    d += n;
            }
            r->progress += d;
            if (d > 0 && k == me)
            {
                g_wrong_said = 0;
                hud_message(HUD_WRONG_WAY_GONE);
            }
            if (d > 0 && r->progress > r->furthest) {
                int s = r->prev, i;
                r->furthest = r->progress;
                for (i = 0; i < d; i++) {
                    s = wrap(s + dir);
                    if (g_land.sec[s].checkpoint >= 0) {
                        if (COUNTS(k))
                            cp = 1;
                        if (k == me) {
                            g_time_left += g_land.sec[s].time;
                            g_extended = 5000;
                        }
                    }
                    if (k == race_player())
                        signs_event(g_land.sec[s].event, g_reverse);    /* 0x486240 */
                }
            }
            if (d <= 0 && k == me) {
                /* going backwards: the bike's forward axis against the section's start
                   plane, (n2 + n0) + n1 in the FPU's width */
                const float *pl = g_land.sec[cur].plane[0];
                double f = dir;
                double v = (f * rd->m[2][2] + rd->t[2]) * pl[2]
                           + (f * rd->m[2][0] + rd->t[0]) * pl[0]
                           + (f * rd->m[2][1] + rd->t[1]) * pl[1];
                if (v < 0.0 && !g_wrong_said) {
                    g_wrong_said = 1;
                    hud_message(HUD_WRONG_WAY);         /* 0x481db0 */
                    sound_queue_voice(0x46, 500);       /* WRONGWAY */
                }
            }
            r->prev = cur;
        }
        if (cp && r->state == RS_RACING) {
            if (!g_playback && !g_winner_lap)
                split(k, k == me);
            if (COUNTS(k) && ++r->cp >= g_ncheck)
                r->cp = 0;
        }
        if (!line)
            continue;
        if (COUNTS(k)) {
            if (r->state == RS_GRID)
                r->state = RS_RACING;
            else if (r->state == RS_RACING) {
                r->laps++;
                if (k == me && r->laps == g_laps - 1) {
                    hud_message(HUD_FINAL_LAP);         /* 0x481c50 */
                    sound_queue_voice(0x37, 500);       /* FINLAP */
                }
            }
        }
        if (r->state != RS_RACING)
            continue;
        /* 0x47ea0f: the ghost over the last line has the time its run was saved with */
        if (k == 2 && r->laps == g_laps && g_mode == MODE_PRACTICE && g_time_attack) {
            r->time = (int)ghost_total();
            if (g_laps == 1)
                r->lap[0] = r->best = r->time;
        }
        if (COUNTS(k)) {
            int i;
            for (i = 0; i < r->laps && i < 8; i++)
                if (r->best > r->lap[i]) {
                    r->best = r->lap[i];
                    if (g_solo)
                        lap_record(k);
                }
        }
        if (k == me && r->laps == g_laps) {
            g_control = 0;
            g_race_over = 1;
        }
        anyline = 1;
        if (r->laps == g_laps && COUNTS(k)) {   /* the finish */
            r->state = RS_FINISHED;
            rules_places();
            r->place = rules_place_of(k);
            if (k == me) {
                int p = r->place;
                if (p == 0)
                    sound_queue_voice(rand_next(1) > 0x3fff ? 0x4f : 0x50, 500);
                else if (p == g_riders - 1)
                    sound_queue_voice(0x48, 500);       /* YURALOSR (never in Solo) */
                else if (p > 2)
                    sound_queue_voice(0x47, 500);       /* YOULOST */
            }
        }
    }
    if (anyline)
        fastest_lap();
}

/* ------------------------------------------------------------------ places */

/* 0x47e1e0: finishers by race time, the riders racing by how far they are, then the
   rest by race time; each group sorted with the game's quicksort */
void rules_places(void)
{
    unsigned a[RIDERS], b[RIDERS], c[RIDERS];
    int na = 0, nb = 0, nc = 0, k, i;
    for (k = 0; k < RIDERS; k++) {
        const Racer *r = &g_racer[k];
        unsigned id = (unsigned)k << 27;
        if (!r->loaded)
            continue;
        if (r->state == RS_OUT || r->state == RS_GRID)
            c[nc++] = ((unsigned)r->time & 0x7ffffff) | id;
        else if (r->state == RS_FINISHED)
            a[na++] = (~(unsigned)r->time & 0x7ffffff) | id;
        else {
            int t = g_reverse ? (int)-(double)r->dist : (int)((double)r->dist + 1024.0);
            if (t < 0)
                t = 0;
            if (t > 0xfff)
                t = 0xfff;
            b[nb++] = ((unsigned)t & 0xfff) | (unsigned)r->progress << 12 | id;
        }
    }
    msvc_qsort(b, nb, descending);
    msvc_qsort(a, na, descending);
    msvc_qsort(c, nc, descending);
    g_norder = 0;
    for (i = 0; i < na; i++)
        g_order[g_norder++] = (int)(a[i] >> 27);
    for (i = 0; i < nb; i++)
        g_order[g_norder++] = (int)(b[i] >> 27);
    for (i = 0; i < nc; i++)
        g_order[g_norder++] = (int)(c[i] >> 27);
}

int rules_place_of(int k)
{
    int i;
    for (i = 0; i < g_norder; i++)
        if (g_order[i] == k)
            return i;
    return -1;
}
