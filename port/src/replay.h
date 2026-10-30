#ifndef REPLAY_H
#define REPLAY_H

/* The race's recording (NOTES §4.21b): the local rider's inputs, a record a step, at
   0x558aa8. Everything else plays the same way from the same start. */

void replay_clear(void);        /* 0x4577c0: a normal race starts */
void replay_rewind(void);       /* 0x4577e0: a replay starts */
/* 0x4577f0: keys with bit 7 = +0x31f9 and bit 6 = +0x3201; full: dropped */
void replay_record(int ms, unsigned keys, int held, int pressed, int x, int y);
/* 0x4578a0: 0 past the end */
int replay_play(int *ms, unsigned *keys, int *held, int *pressed, int *x, int *y);
int replay_count(void);         /* 0x4e9838 */

#endif
