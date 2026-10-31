#ifndef DIALOG_H
#define DIALOG_H

/* The race's dialogs (NOTES §4.19): the pause menu, the results and the continue boxes, in
   the slot 0x4e8904. Built from text objects; the keyboard drives them (the mouse and the
   F1 / F11 / F12 dialogs: not yet). */

int dialog_open(void);          /* there is one */
int dialog_holds(void);         /* +0x10: the race does not step */
int dialog_flags(void);         /* 0x4126a0: what of the race is drawn (0x47ff90's flags) */
enum {                          /* the flags' bits and values */
    RACE_DRAW_DIALS = 8,        /* the dials */
    RACE_DRAW_LIST1 = 0x20,     /* the texts of 0x53ba68 */
    RACE_DRAW_HUD = 0x40,       /* the dashboard and all the texts */
    RACE_DRAW_ALL = -2,         /* a race running */
    RACE_DRAW_DIALOG = -10,     /* the race's dialogs: no dials */
    RACE_DRAW_HELP = -0x4a      /* the help: no dials, no dashboard */
};
int dialog_key(int vk, int down);       /* 0x433ec0: 1 if it took the key */
void dialog_mouse_move(int x, int y);           /* 0x433ec0's mouse messages */
void dialog_mouse_button(int left, int down);
void dialog_frame(void);        /* 0x434960 (from 0x4126a0), once a race frame */
void dialog_draw(void);         /* 0x434ae0, after the dashboard */
void dialog_shade(int x0, int y0, int x1, int y1);      /* 0x49a5a0 */
void dialog_modes(void);        /* Shift+F12 0x43c2b0: opened, or closed again */
void dialog_screen(void);       /* a new screen: the items placed afresh */
void dialog_free(void);         /* the slot emptied without the closing effects */

void dialog_pause(int arg);     /* 0x436420: arg 1 from Escape, 0 after a single race */
void dialog_results(void);      /* 0x437b70: [REPLAY] YES / NO */
int dialog_help(void);          /* 0x43cf90: 0 if Help.gen does not load */
void dialog_side(int kind);     /* F12 1 (0x438f70), F11 3 (0x43a400): opened, or closed again */
void dialog_continue(void);     /* 0x4358b0: [CONTINUE] YES / NO */

#endif
