#ifndef DIALOG_H
#define DIALOG_H

/* The race's dialogs (NOTES §4.19): the pause menu, the results and the continue boxes, in
   the slot 0x4e8904. Built from text objects; the keyboard drives them (the mouse and the
   F1 / F11 / F12 dialogs: not yet). */

int dialog_open(void);          /* there is one */
int dialog_holds(void);         /* +0x10: the race does not step */
int dialog_key(int vk, int down);       /* 0x433ec0: 1 if it took the key */
void dialog_frame(void);        /* 0x434960 (from 0x4126a0), once a race frame */
void dialog_draw(void);         /* 0x434ae0, after the dashboard */
void dialog_free(void);         /* the slot emptied without the closing effects */

void dialog_pause(int arg);     /* 0x436420: arg 1 from Escape, 0 after a single race */
void dialog_results(void);      /* 0x437b70: [REPLAY] YES / NO */
void dialog_continue(void);     /* 0x4358b0: [CONTINUE] YES / NO */

#endif
