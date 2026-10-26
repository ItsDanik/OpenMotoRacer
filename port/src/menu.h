#ifndef MENU_H
#define MENU_H
#include "bitmap.h"

/* The front end's machinery (NOTES §4.4): the pages, their buttons, keys and mouse. */

#define MENU_PAGES 27
#define MENU_BUTTONS 32
#define MENU_PICS 0xd1          /* pictures by id (the table 0x4e3e20 has 209) */

/* Windows' key codes, which the pages compare against */
enum {
    VK_BACK = 8, VK_TAB = 9, VK_RETURN = 0xd, VK_SHIFT = 0x10, VK_CONTROL = 0x11,
    VK_MENU = 0x12, VK_ESCAPE = 0x1b, VK_SPACE = 0x20, VK_PRIOR = 0x21, VK_NEXT = 0x22,
    VK_END = 0x23, VK_HOME = 0x24, VK_LEFT = 0x25, VK_UP = 0x26, VK_RIGHT = 0x27,
    VK_DOWN = 0x28, VK_INSERT = 0x2d
};

typedef struct {                /* 0x58 bytes in the game, at menu + 0x44 + i * 0x58 */
    int hotkey;                 /* +0x10: a key code, -1 none */
    int kind;                   /* +0x14: 1 text, 2 picture */
    Bitmap *pic, *hl;           /* +0x20, +0x24: normal and highlighted picture (0 none) */
    int px, py;                 /* +0x34, +0x38: where the picture goes, -1 = where it is */
    unsigned nb;                /* +0x40: the button the arrows go to: bytes Left, Right,
                                   Down, Up; 0xff none */
    int group;                  /* +0x44: -1 none */
    int *var, value;            /* +0x48, +0x4c: shown "on" while *var == value */
    int blink;                  /* +0x50 */
    int flags;                  /* +0x54: bit 0 = pressed again every frame while held */
} Button;

typedef struct {                /* the menu object, 0xd4c bytes at *0x4dc0c4 */
    int active;                 /* +0 */
    int mouse_mode;             /* +4: the selection follows the pointer */
    unsigned prev_mouse;        /* +8: last frame's mouse buttons */
    int key;                    /* +0xc: the last key pressed */
    int sel, prev_sel;          /* +0x10, +0x14 */
    int pressed;                /* +0x18: the button pressed, -1 none */
    unsigned tr_start, tr_end;  /* +0x1c, +0x20: the glass transition */
    unsigned last_input;        /* +0x24: for the title's 30 s demo time-out */
    float tr_t;                 /* +0x2c: its progress, 0..1 */
    int page;                   /* +0x3c */
    int nbuttons;               /* +0x40 */
    Button b[MENU_BUTTONS];     /* +0x44 */
    int tr_running;             /* +0xd48 */
    int started[MENU_PAGES];    /* the page table's first words (0x4dbb38) */
} Menu;

extern Menu g_menu;
extern Bitmap g_pics[MENU_PICS];    /* 0x504880: the menu's pictures by id */
extern unsigned g_mouse;            /* 0x5083b8: bit 1 left button, bit 0 right */
extern int g_mx, g_my;              /* 0x519fe8, 0x519fea */

/* game flags the menu shows (the name-entry cheats set them, NOTES §4.4.5.2) */
extern int g_pocket_unlocked, g_pocket_unlocked2;  /* 0x4dc03c, 0x4dc054 */
extern int g_reverse_unlocked, g_reverse_unlocked2; /* 0x4dc040, 0x4dc050 */
extern int g_pocket, g_reverse;                     /* 0x4dc078, 0x4dc068 */
extern int g_quit;                                  /* page 4 asked the program to end */

int menu_start(int page, char *err, int errsize);   /* 0x472c40's part: the first page */
void menu_frame(void);                              /* Menu_Frame 0x409c00 */
void menu_change_page(int page);                    /* 0x409eb0 */
void menu_capture(void);                            /* 0x40c4a0: the glass transition */

void menu_clear_buttons(void);                      /* 0x40bd40 */
void menu_add_button(Bitmap *pic, Bitmap *hl, unsigned nb, int hotkey, int group,
                     int value, int *var, int blink, int x, int y, int flags); /* 0x40bef0 */
int menu_hit(void);                                 /* 0x40c0c0 */
void menu_draw_buttons(void);                       /* 0x40c1f0 */
void menu_transition(void);                         /* 0x40c500 */
int menu_pressed(void);                             /* 0x40c1b0 */
void menu_press_done(void);                         /* 0x40c1e0 */

/* what a page does around its drawing: 0x49b1c0 + 0x49a160, then the pointer + 0x49b0e0 */
void menu_begin_frame(void);
void menu_end_frame(void);

/* input, as the window procedure hands it on (0x4329a5) */
void menu_key(int vk);                              /* WM_KEYDOWN → 0x40a0f0 */
void menu_key_up(int vk);                           /* WM_KEYUP → 0x432700 */
void menu_mouse_move(int x, int y);                 /* WM_MOUSEMOVE */
void menu_mouse_button(int left, int down);         /* WM_[LR]BUTTON(DOWN|UP) */


#endif
