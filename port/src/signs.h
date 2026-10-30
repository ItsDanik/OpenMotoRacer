#ifndef SIGNS_H
#define SIGNS_H

/* The turn signs that blink on the screen when a section's event says so (0x486240,
   0x486310) and the 1st, 2nd, 3rd figure in its wreath at the end of a race (0x47b7a0). */

void signs_start(void);                 /* 0x473370: the track must be loaded */
void signs_event(int code, int reverse);        /* 0x486240 */
void signs_step(float dt, int kind);    /* kind: 0x4e9834 */
void signs_draw(void);                  /* before r3d_render */
void signs_key(void);                   /* F8 */

void figure_load(const char *folder);   /* 0x47b600 */
void figure_free(void);
void figure_start(void);
void figure_step(float dt, int place, int leaving);
void figure_draw(void);                 /* before r3d_render */

#endif
