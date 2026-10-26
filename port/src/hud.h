#ifndef HUD_H
#define HUD_H

/* The race's dashboard (NOTES §4.20 §3): the countdown, the lap and best-lap panels, the
   time left and the place, the finishing place, the messages, the dials. */

enum { HUD_TIME_OUT, HUD_FINAL_LAP, HUD_WRONG_WAY, HUD_WRONG_WAY_GONE };

void hud_restart(void);         /* a race (re)started */
void hud_message(int which);    /* 0x481af0, 0x481c50, 0x481db0 */
struct BikePhys;
void hud_bike(const struct BikePhys *b);        /* the shown bike (0x508834) */
void hud_key(int vk);                           /* F7: dials, digits, none (0x432107) */
void hud_dials_3d(void);        /* the dial and needles, before the 3D is drawn */
void hud_draw(void);            /* 0x4806e0, after the 3D, before the text lists */

#endif
