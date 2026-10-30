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
void sound_race_load(int ktm);          /* 0x4044c0: the race's set (ktm: dirt bikes) */
void sound_race_free(void);
void sound_podium_load(void);           /* 0x404310: ids 4, 5 */
void sound_podium_free(void);           /* 0x404490 */
void sound_queue_reset(void);           /* 0x404d50 */

int sound_play(int id, int flags);      /* 0x4049f0: 1 loop, 2 another copy, 4 not twice;
                                           the copy playing, -1 none */
void sound_stop(int h);                 /* 0x404af0 */
void sound_stop_all(void);              /* 0x424020 */
unsigned sound_length(int h);           /* 0x4240d0: ms */
int sound_handle(int id);               /* 0x519ff8[id] */

/* the sound server's own calls, as the race's sounds make them (0x423fd0..0x424090) */
int sound_server_play(int id, int flags);       /* the id's sample: the copy playing */
void sound_volume(int h, int v);        /* 0..255 */
void sound_pan(int h, int p);           /* -128..128 */
void sound_frequency(int h, int hz);    /* heard only between 1000 and 44100 */
void sound_pause_all(int on);           /* 1: the looping copies stopped; 0: again */
int sound_last_voice(void);             /* 0x502898 */
void sound_set_last_voice(int id);
int sound_last_effect(void);            /* 0x502710 */
void sound_set_last_effect(int id);
int sound_effects_on(void);             /* 0x4db664 */
int sound_voices_on(void);              /* 0x4db668 */
void sound_load_named(int id, const char *name);   /* a track's ambient sample (Divers\) */
int sound_rate(int id);                 /* the id's file's own rate */

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
void music_pause(int on);               /* 0x4096e0: the Pause key and the help */
void sound_set_volumes(const int v[3]); /* master, CD, wave: 0..32 */

#endif
