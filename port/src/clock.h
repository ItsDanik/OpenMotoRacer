#ifndef CLOCK_H
#define CLOCK_H

/* The game's clock: timeGetTime(), milliseconds. In a window it is the real time, so every
   routine timed by it runs at the original's speed at any refresh rate. Under --shot it
   moves 1000/60 ms a picture, so test pictures do not depend on the machine.

   What the original counts in pictures (the scrolling tiles, the blinking, the drifting
   texts) depended on the screen it ran on (1997's were 60-75 Hz; it has no limiter of
   its own and ran at ~95 pictures a second on an unsynchronised display). The port counts
   those in 60ths of a second: g_ticks is how many have passed since the last picture
   (0 or more on a fast screen, 1 a picture under --shot). */
extern unsigned g_now;
extern int g_ticks;

void clock_start(int realtime);
void clock_frame(void);     /* after a picture */

#endif
