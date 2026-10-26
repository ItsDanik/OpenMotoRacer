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
extern int g_turbo[RIDERS], g_turbo_hit[RIDERS];

void race_key(int vk, int down);        /* 0x431250 / 0x432700 */
int race_load(char *err, int errsize);  /* 0x47b420 and the start 0x47bf70 */
void race_frame(void);
void race_unload(void);                 /* 0x47d620 */

#endif
