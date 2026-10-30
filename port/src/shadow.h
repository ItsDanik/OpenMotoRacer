#ifndef SHADOW_H
#define SHADOW_H
#include "bike3d.h"
#include "bikephys.h"

/* A bike's shadow on the road (NOTES §4.15.9): the top view of the bike on page 1 */
void shadow_step(BikePhys *b);          /* 0x461aa0: its points, once a step */
void shadow_draw(const Rider *r);       /* 0x461f70 */

#endif
