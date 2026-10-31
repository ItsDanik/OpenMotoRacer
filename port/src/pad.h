#ifndef PAD_H
#define PAD_H

/* The port's: a gamepad works the menus and the race's dialogs (the original's joystick only
   worked the dialogs, joy_menu). */

extern int g_pad_listen;        /* the settings page waits for a button to bind: hands off */

void pad_frame(void);           /* once a picture, before it is made */
void pad_quiet(void);           /* nothing more until the pad has been let go */

#endif
