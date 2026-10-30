#ifndef GHOST_H
#define GHOST_H

/* The time attack's ghost (NOTES §4.21c): the player's run recorded, a sample a step, into the
   object 0x52c0f0; when it was first over the line it goes into 0x52c0d8 and the file
   <track>\timatak...bin, and rider 2 plays it back in the next run. Up to 10000 samples, as the
   game's (160 s at 62.5 steps a second: the rest of a longer run is not kept). */

void ghost_reset(void);         /* 0x4191a0 on both: nothing recorded, no ghost */
/* 0x4136a0: the current choices' file, record = the 0xf9 bytes of its Score; no file, or one
   for other choices: no ghost, record untouched */
void ghost_load(unsigned char *record);
void ghost_save(const unsigned char *record);   /* 0x4138c0 */
void ghost_forget(void);        /* page 9's Reset: the file deleted */
void ghost_restart(void);       /* 0x4191b0 on the ghost, 0x4191a0 on the recording */
int ghost_count(void);          /* 0x52c0d8: its samples, 0 = no ghost */
float ghost_total(void);        /* 0x52c0e8: its race time, ms */

/* 0x4192a0: dt on; the speed, place, heading point and turbo between the two samples around
   the ghost's clock. crash = 1 with v: a crash happened since. Past the end nothing changes. */
void ghost_play(float dt, float *speed, float at[3], float dir[3], int *turbo, int *crash,
                float v[3]);
/* 0x4191c0: one sample of the run */
void ghost_record(float dt, float speed, const float at[3], const float dir[3], int turbo,
                  int crash, const float v[3]);
/* 0x412d61 and 0x4190d0: the run becomes the ghost, with its race time in ms */
void ghost_take(int ms);

#endif
