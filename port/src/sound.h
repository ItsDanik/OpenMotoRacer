#ifndef SOUND_H
#define SOUND_H

/* Sound (NOTES §4.10): the effects and voices of SFX.BKF played as the game's DirectSound
   layer plays them ("CfxServer"), its queues of delayed sounds, and the music, the CD's
   audio tracks, which GOG's copy has as 02.wav..13.wav beside game.EXE. */

/* wav: 0 plays through the sound card; a path writes what would be heard to that WAV file
   instead, 1/60 s a frame (the headless checks) */
int sound_init(const char *wav);
void sound_quit(void);
void sound_frame(void);                 /* once a picture: the music read ahead, the file */

void sound_menu_load(void);             /* 0x403f90: the menu's set (+ its voices) */
void sound_queue_reset(void);           /* 0x404d50 */

int sound_play(int id, int flags);      /* 0x4049f0: 1 loop, 2 another copy, 4 not twice;
                                           the copy playing, -1 none */
void sound_stop(int h);                 /* 0x404af0 */
void sound_stop_all(void);              /* 0x424020 */
unsigned sound_length(int h);           /* 0x4240d0: ms */
int sound_handle(int id);               /* 0x519ff8[id] */

void sound_queue(int n, ...);           /* 0x404e10(n, delay, id, delay, id, ...) */
void sound_queue_voice(int id, int delay);  /* 0x404da0 */
void sound_update(void);                /* 0x404eb0: the queues, once a menu frame */

void music_play(int track, int mode);   /* 0x404990: 0 once, 1 once unless playing,
                                           2 repeat, 3 repeat unless playing */
void music_stop(void);                  /* 0x4049e0 */
int sound_running(void);                /* the sound came up (the server 0x516aa0) */
/* CONFIG's SoundState, VoicesState, AudioState, and the options page's toggles */
void sound_set_switches(int effects, int voices, int music);
void sound_get_switches(int *effects, int *voices, int *music);
void sound_toggle_effects(void);        /* 0x408b30 */
void sound_toggle_voices(void);         /* 0x408f30 */
void music_toggle(void);                /* 0x409410 */
void sound_set_volumes(const int v[3]); /* master, CD, wave: 0..32 */

#endif
