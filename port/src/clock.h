#ifndef CLOCK_H
#define CLOCK_H

/* The game's clock: timeGetTime(), milliseconds. The port advances it by 1000/60 a frame
   (16 or 17 ms), so every routine timed by it runs as in the original at 60 pictures a
   second. */
extern unsigned g_now;

void clock_frame(void);     /* one 60th of a second on */

#endif
