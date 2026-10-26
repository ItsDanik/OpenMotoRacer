#ifndef HUD_H
#define HUD_H

/* The race's dashboard (NOTES §4.20 §3): the countdown, the lap and best-lap panels, the
   time left and the place, the finishing place and the messages. (The dials: not yet.) */

enum { HUD_TIME_OUT, HUD_FINAL_LAP, HUD_WRONG_WAY, HUD_WRONG_WAY_GONE };

void hud_restart(void);         /* a race (re)started */
void hud_message(int which);    /* 0x481af0, 0x481c50, 0x481db0 */
void hud_draw(void);            /* 0x4806e0, after the 3D, before the text lists */

#endif
