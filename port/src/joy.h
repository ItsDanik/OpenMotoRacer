#ifndef JOY_H
#define JOY_H

/* The joysticks (the block 0x51ae88, NOTES §4.22): what the game read through WinMM's
   joyGetPosEx, from SDL's joysticks. X and Y are 0..0xffff, 0x8000 in the middle, as
   WinMM gives them; a hat (a joypad's cross) pushes them to the ends, as a joypad of the
   time did with its axes. Buttons are bits, button 0 first. */

#define JOY_MAX 8

typedef struct {
    int present;                /* +0xe48: answered, or only unplugged */
    int working;                /* +0xe4c: answered */
    int centre[3], dead[3];     /* +0xe54, +0xe60 */
    int x, y;                   /* +0xe6c, +0xe70: rescaled past the dead zone */
    unsigned buttons;           /* +0xe78 */
    unsigned dir;               /* +0xe7c: 1 up, 2 down, 4 left, 8 right */
    int nbuttons;               /* the caps' wNumButtons (+0x1e4) */
    /* the port's, for a gamepad */
    int pad;                    /* SDL knows its layout: the buttons as PAD_*, the triggers */
    int stick[2];               /* the left stick alone, rescaled, without the cross */
    unsigned cross;             /* the cross: as dir */
    int trigger[2];             /* LT, RT, 0..0x7fff */
} Joy;

/* a gamepad's buttons, numbered as most of them come raw, which keeps saved bindings; the
   triggers count as two more */
enum {
    PAD_A, PAD_B, PAD_X, PAD_Y, PAD_LB, PAD_RB, PAD_BACK, PAD_START, PAD_GUIDE, PAD_L3, PAD_R3,
    PAD_LT, PAD_RT, PAD_BUTTONS
};

extern Joy g_joy[JOY_MAX];
extern int g_njoy;              /* +0: joyGetNumDevs, at most 8 */
extern int g_joy_dead;          /* 0x4dbf7c: -jN on the command line, -1 = 0x800 */

void joy_detect(void);          /* 0x4189c0: (re)open them all, the dead zones */
void joy_close(void);
int joy_known(int j);           /* 0x418b60: working, detecting again if not */
int joy_read(int j);            /* 0x418da0: x, y, buttons, dir; 0 if it did not answer */
/* 0x418c60 for page 12: the first button held (n >= 0), else a direction from the raw
   position (-2 left, -3 right, -4 up, -5 down), else -1 */
int joy_code(int j);
/* 0x411b30: rider's keys from joystick j through the bindings (bits as bikephys.h), *x
   and *y for the step; *turbo (the rider's +0x31fd) 1 while a turbo binding is held: the
   caller sets turbo_held / turbo_pressed when it changes.
   countdown 0x4dc0b4, running 0x5429b0; device 0x4dc11c (2 joystick, 4 joypad) */
unsigned joy_race(int j, int *x, int *y, int *turbo, int countdown, int running, int device);
/* 0x411a10: joystick 0 as the arrow keys and Return (button 0) for the race's dialogs */
void joy_menu(void (*key)(int vk, int down));

/* tests: joystick 0 replaced by these values (OMR_INPUT's joy:X,Y,BUTTONS) */
void joy_fake(int x, int y, unsigned buttons);
/* the same as a gamepad (pad:X,Y,BUTTONS,LT,RT,CROSS: buttons as PAD_*, cross as dir) */
void joy_fake_pad(int x, int y, unsigned buttons, int lt, int rt, unsigned cross);

int joy_is_pad(int j);
int joy_spare_trigger(int j, int action);        /* RT, LT there without being set, or -1 */
const char *joy_button_name(int code);          /* a gamepad's button by its number */
void joy_rumble(int low, int high, int ms);     /* joystick 0 shakes, if it can */

#endif
