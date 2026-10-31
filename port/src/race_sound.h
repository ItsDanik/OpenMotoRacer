#ifndef RACE_SOUND_H
#define RACE_SOUND_H
#include "bikephys.h"
#include "race.h"

/* The race's sounds (NOTES §4.10.6): the engines near the camera with their Doppler and
   pan (0x4079e0, 0x406290), the rider's shouts (0x407600), the track's ambience
   (0x405f30). */

/* 0x4084e0, at the race's start (after the grid and the camera): cam the camera's eye,
   player the local player's record (0x4ea4c0) */
void race_sound_start(const float cam[3], int player);
/* Game_Frame's calls 0x4079e0 and 0x405f30's ambience, once a step after the camera:
   all[k] the riders' records (all[0] unused), followed the rider the camera follows
   (0x508834), cam / look the camera's eye and the point it looks at, dt the step
   (ms x 0.1) */
void race_sound_commentary(int place, int count);   /* 0x407420 */
void race_sound_step(BikePhys *const *all, int followed, const float cam[3],
                     const float look[3], float dt);
int race_sound_player(void);

/* the race's switches, with their messages (F11; Ctrl+S, V, A) */
void race_sound_toggle_engine(void);    /* 0x408900 */
void race_sound_toggle_ambience(void);  /* 0x408800 */
void race_sound_toggle_effects(void);   /* 0x408b30 */
void race_sound_toggle_voices(void);    /* 0x408f30 */
void race_sound_toggle_music(void);     /* 0x409410 */
int race_sound_engine_on(void);         /* 0x4db65c */
int race_sound_ambience_on(void);       /* 0x4db660 */
int race_sound_music_on(void);          /* 0x4db66c */

void podium_sound_start(void);
void podium_sound_frame(void);
void podium_sound_stop(void);

#endif
