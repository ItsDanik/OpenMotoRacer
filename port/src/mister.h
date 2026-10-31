/* The MiSTer hybrid core (branch mister, built with MISTER_HYBRID; the core is
   ~/MiSTer-openmotoracer): the game runs on the MiSTer's ARM, the FPGA core shows the picture
   and has the sound and the input. SDL's "mister" drivers (hybrid/sdl2) do most of it; this is
   the rest. Without the core everything here answers as the PC would. */
#ifndef MISTER_H
#define MISTER_H
#ifdef MISTER_HYBRID

/* the core is there */
int mister_on(void);
/* which copy of the game, by the core's start: the folder to read it from, in out; 0 when
   it isn't there, after the message saying what goes where */
int mister_start(char *out, unsigned size);
/* every frame: the OSD's options and cheats, the HUD margins, the field rate */
void mister_frame(void);
/* every frame: the core's joystick 1 as the game's pretend gamepad */
void mister_pad(void);
/* once a picture, after it was handed over: until the core takes the next */
void mister_wait(void);
/* the game's clock in milliseconds, from the core's fields; 0 without the core */
int mister_clock(unsigned *ms);
/* a message on the core's picture, staying until the player leaves the core */
void mister_fatal(const char *text);
/* what the process ends with: the launcher's contract */
int mister_exit_code(void);
/* the bindings when the player has no settings file yet */
void mister_default_bindings(void);

enum { MISTER_NO_TIME_LIMIT = 1, MISTER_SLOW_RIVALS = 2 };
int mister_cheat(int which);

/* the race's resolution, the OSD's list: 320x240, 640x240 (the 640x480 race with only the
   even rows painted and shown), 640x480 */
enum { MR_320x240, MR_640x240, MR_640x480, MR_COUNT };
extern int g_mister_res;

#endif
#endif
