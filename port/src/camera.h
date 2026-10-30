#ifndef CAMERA_H
#define CAMERA_H
#include "bikephys.h"

/* The race's camera (the block 0x52c0c4, NOTES §4.20 §2): the chase behind the followed
   bike (0x46f840), its distances (F5), the look back (Tab), the finish orbit, the TV
   director of the replays. */

void camera_reset(const BikePhys *b);           /* the race's start (0x46da40) */
/* 0x4583e0 + 0x458620 once a step: countdown 0x4dc0b4, finished = the followed rider
   has finished (the orbit to the front), tv = a replay, demo or the winner's lap: the TV
   director 0x46e9f0 where the track has trackside cameras */
void camera_step(const BikePhys *b, float dt, int countdown, int finished, int tv);
float camera_zoom(void);                        /* 0x6d1f74, this step's */
void camera_key(int vk, int down);              /* Tab, F5 (0x431250, 0x432700) */
/* the block's C and position to r3d, a of the way from the step before (1: as it is) */
void camera_apply(float a);
/* 0x4583e0: the on-board view (F6 0x4dc00c and the bike not crashed) or the chase */
void camera_onboard(int on);
int camera_preset(void);                        /* +0x21c: 1 while Tab looks back */
void camera_keep(void);                         /* before a step: the state to blend from */
const float *camera_eye(void);                  /* +0x98 */
const float *camera_target(void);               /* +0xa4: the point looked at */

#endif
