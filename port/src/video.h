#ifndef VIDEO_H
#define VIDEO_H
#include <stdint.h>

/* The game's screen: 16-bit RGB565 (NOTES §4.3). The menus are 640x480. */
#define SCREEN_MAX_W 640
#define SCREEN_MAX_H 480

extern uint16_t *g_screen;     /* the back surface the game draws on (0x624f5c) */
extern int g_scr_w, g_scr_h;   /* its size (0x655d10, 0x624b18) */
extern int g_pitch;            /* pixels a row (0x62ebec) */

int video_init(int headless, char *err, int errsize);
void video_present(void);
int video_shot(const char *file);   /* the screen as a BMP */
void video_quit(void);

#endif
