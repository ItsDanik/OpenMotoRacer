#ifndef POINTER_H
#define POINTER_H

/* The mouse pointer in a race (NOTES §4.19 §2.6): the game's Windows cursors, which dissolve
   and go 3.63 s after the mouse last did something. (The menus draw their own, menu.c.) */

enum { CUR_NONE = 0, CUR_ARROW = 116, CUR_ROW = 228, CUR_NO = 251 };

void pointer_touched(int x, int y);     /* a mouse message: 0x4dbfb4 = 3000 */
void pointer_step(int ms);              /* 0x4120e0: once a race frame */
void pointer_shape(int cursor);         /* 0x4dbfac */
void pointer_draw(void);                /* 0x45a190: last on the race's picture */
void pointer_undraw(void);              /* what it covered put back (the race paused) */

#endif
