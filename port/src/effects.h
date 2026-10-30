#ifndef EFFECTS_H
#define EFFECTS_H
#include "bikephys.h"

/* What the bikes leave on the track (NOTES §4.15.10): dust and spray, tyre marks */
void effects_restart(void);             /* 0x457320: the marks gone, at a race's start */
/* 0x466d70: at a bike's step, keys its keys of this step */
void effects_bike(BikePhys *b, unsigned keys);
/* 0x467c40: n pieces flying from a crash, sized base + (random & mask), m as a bike's matrix */
void effects_debris(int n, const float pos[3], int base, int mask, const float *m);
void effects_step(float dt);            /* 0x457590 */
/* 0x4575c0, 0x4573e0: at the race's detail, smooth = how far from the step before to the last */
void effects_draw(int detail, float smooth);

#endif
