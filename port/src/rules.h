#ifndef RULES_H
#define RULES_H

/* The race's rules (NOTES §4.17): the start countdown and the time limit (0x47dae0), laps,
   checkpoints, the wrong way and the finish (0x47e3d0), the places (0x47e1e0). */

#define NO_TIME 0x7f7f7f7f      /* an empty time (best lap, records) */
#define SPLITS 0x31             /* +0x2f89: 0x30 cleared, index 0x30 still written */

enum { RS_GRID = 0, RS_RACING = 1, RS_FINISHED = 2, RS_OUT = 3 };

typedef struct {                /* a rider record's race fields (offsets in it) */
    int loaded;                 /* +0x68 */
    int human;                  /* +0x08: the player */
    int time;                   /* +0x2ddd: race time, ms */
    int best;                   /* +0x2de1: best lap, ms */
    int state;                  /* +0x2de5: RS_* */
    int cp;                     /* +0x2f79 (i16): the next checkpoint's number */
    int laps;                   /* +0x2f7b (i16): laps done */
    int place;                  /* +0x2f85: 0 the winner */
    int split[SPLITS];          /* +0x2f89: race time at each checkpoint passed */
    float dist;                 /* +0x31b1: from the section walk */
    int progress, furthest;     /* +0x31b5, +0x31b9: sections since the start */
    int prev;                   /* +0x31bd: last frame's section */
    int lap[8];                 /* 0x5db290 + k x 0x20: each lap's time, ms */
} Racer;

extern Racer g_racer[25];       /* by rider record, as g_rider (RIDERS) */

extern int g_counting;          /* 0x4dc0b4: the start countdown runs */
extern int g_time_left;         /* 0x4dc0a8: the countdown, then the time left, ms */
extern int g_time_over;         /* 0x52c238 */
extern int g_race_over;         /* 0x509ba0: the player has finished */
extern int g_control;           /* 0x5429b0: the player has control */
extern int g_extended;          /* 0x519fec: ms the "extended time" message has left */
extern int g_order[32];         /* 0x516ab8: riders by place */
extern int g_norder;            /* 0x4dc154 */
extern int g_fastest;           /* 0x4f0db8: who holds the fastest lap */
extern int g_race_best_lap;     /* 0x5d26a5 */

extern char g_split[12];        /* 0x5dbc70: the sign, then the difference to the record's */
void rules_save_results(void);          /* 0x463c30: before a replay is watched */
void rules_restore_results(void);       /* 0x463ca0 */
void rules_progress(int k);     /* 0x47cff4: progress from the section, at the grid */
void rules_restart(void);       /* 0x411010's part */
void rules_time(int ms);        /* 0x47dae0 */
void rules_laps(void);          /* 0x47e3d0 */
void rules_places(void);        /* 0x47e1e0 */
int rules_place_of(int k);      /* 0x459f50 */
/* 0x4b3100, MSVC's qsort (its order of equal keys is the game's): over 32-bit values */
typedef int (*QsortCmp)(unsigned a, unsigned b);
void msvc_qsort(unsigned *base, int num, QsortCmp cmp);
/* the time attack's ghost: its record (0x581018, the splits it is measured against), and the
   run of rider k taken as the new ghost's (0x412d86) */
unsigned char *rules_reference(void);
void rules_take_ghost(int k);
const unsigned char *rules_record(void);        /* 0x581118: the track's best lap record */

#endif
