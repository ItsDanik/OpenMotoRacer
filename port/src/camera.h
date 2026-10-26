#ifndef CAMERA_H
#define CAMERA_H
#include "bikephys.h"

/* The race's camera (the block 0x52c0c4, NOTES §4.20 §2): the chase behind the followed
   bike (0x46f840), its distances (F5), the look back (Tab) and the finish orbit. */

void camera_reset(const BikePhys *b);           /* the race's start (0x46da40) */
/* 0x4583e0 + 0x458620 once a step: countdown 0x4dc0b4, finished = the followed rider
   has finished (the orbit to the front) */
void camera_step(const BikePhys *b, float dt, int countdown, int finished);
void camera_key(int vk, int down);              /* Tab, F5 (0x431250, 0x432700) */
void camera_apply(void);                        /* the block's C and position to r3d */
const float *camera_eye(void);                  /* +0x98 */

#endif
