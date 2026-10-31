#ifndef TRACKMAP_H
#define TRACKMAP_H

#include "bike3d.h"

/* F9's track map (NOTES §4.20 §1b): the road seen from above, standing in the view's own
   space in a corner or the middle of the picture, and a mark for each rider. */

extern int g_map_mode;          /* 0x4dbdac: MapMode, 0..3 shown, 4 off (F9; CONFIG.BPT) */

void trackmap_build(void);      /* 0x40ed10: from the loaded track; the place starts over */
void trackmap_free(void);
void trackmap_key(void);        /* F9 (0x4322ea) */
/* 0x40f2d0, a picture's: the riders as drawn, the rider followed, the focal length of the
   screen with the zoom at 1 (the caller sets its own back afterwards) */
void trackmap_draw(const Rider *riders, int follow, float focal);

#endif
