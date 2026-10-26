#ifndef RAND_H
#define RAND_H
#include <stdint.h>

/* The game's random numbers (0x4b1160): Park-Miller (x16807 mod 2^31-1, in Schrage's
   form) in 8 streams, whose seeds are the program's own (0x4f2590). 1 is the menu's
   (0x458cd0), 6 the scripts' cipher. */

void rand_init(void);                   /* the seeds from game.EXE */
void rand_seed(int stream, uint32_t seed);      /* 0x4b1140 */
int rand_next(int stream);              /* 0..0x7ffe */
int rand_step(uint32_t *state);         /* the same on a state of one's own */

#endif
