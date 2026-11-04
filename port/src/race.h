#ifndef RACE_H
#define RACE_H
#include "bike3d.h"

/* The race (Game_Frame 0x47d660 and the race object 0x52c478, NOTES §4.16) */
#define RIDERS 25               /* records 0x581218 + k x 0x3209, k = 1..24 riders */

extern Rider g_rider[RIDERS];   /* (their count + 1 is choices.h's g_riders, 0x509bac) */
extern int g_racing;            /* the race has the screen, not the menu */

enum {                          /* the control bits (+0x31e9, NOTES §4.18.2.1) */
    RK_ACCEL = 1, RK_BRAKE = 2, RK_LEFT = 4, RK_RIGHT = 8, RK_GEARUP = 0x10, RK_GEARDOWN = 0x20
};
extern unsigned g_keys[RIDERS];

void race_ask_screen(int mode);
#ifdef MISTER_HYBRID
void race_set_resolution(int r);
#endif
void race_ask_169(void);
int race_is_169(void);
void race_key(int vk, int down);        /* 0x431250 / 0x432700 */
extern int g_race_steps;                /* OMR_SHOT_STEP: steps since the race began */
int race_load(char *err, int errsize);  /* 0x47b420 and the start 0x47bf70 */
void race_frame(void);
void race_unload(void);                 /* 0x47d620 */
/* 0x4e8918's events (0x4587e0), done at the next race frame */
enum {
    EV_RESTART = 2,             /* 1 | 2: the riders back on the grid, the race again */
    EV_UNLOAD = 4,
    EV_NORMAL = 8,              /* the race kind 0 */
    EV_WATCH = 0x10,            /* the race kind 1: the race's replay */
    EV_CONTINUE = 0x40,         /* the continue dialog */
    EV_PAUSE_AFTER = 0x80,      /* the pause menu without CONTINUE (after a single race) */
    EV_TOP5 = 0x200,            /* a championship's advance (0x4580f0), else page 9 */
    EV_PAGE6 = 0x400,
    EV_TITLE = 0x2000,          /* page 14 */
    EV_CLOSE_DEMOS = 0x8000,    /* the demo list closed, the demos over */
    EV_DEMO_LOAD = 0x20000,     /* the demo list's current entry starts */
    EV_DEMO_NEXT = 0x100000     /* the demo list moves on */
};
void race_event(unsigned bits);
int race_label_place(int k, int *sx, int *sy);  /* what 0x483670 asks of the camera */
int race_player(void);                  /* 0x508834: the rider the screen shows */
int race_local(void);                   /* 0x4ea4c0: the local player's record */
extern int g_ctrl_held, g_shift_held;   /* 0x4dc108, 0x4dc10c: the race's and the dialogs' */
extern int g_race_kind;                 /* 0x4e9834: 0 a race, 1 its replay, 2 a demo */
extern int g_playback;                  /* 0x4e9830: the demos run */
extern int g_winner_lap;                /* 0x504ea0: the computer rides the winner's bike on */
/* the demo list (Data\Demo\EA.dem, 0x42e370) and the title page's time-out (0x409a30);
   race_demo_picture: the loading picture of the running demo, or 0 */
void race_demos_load(void);
void race_demos_off(void);              /* 0x53e5f2 = 0: never tried again */
int race_demos_exist(void);             /* 0x53e5f2 */
int race_demo_start(void);
const char *race_demo_picture(void);
void race_release_keys(void);           /* 0x470fc0 on the player, 0x4e8964 = 0 */

#endif
