#ifndef CDAUDIO_H
#define CDAUDIO_H
#include <stddef.h>

/* The 1997 CD's music: its audio tracks, read from the CD's image (a .cue and its .bin). */
/* the file a track is in and where, in bytes of 44.1 kHz 16-bit stereo; 0 if there's no image */
int cdaudio_track(int track, char *path, size_t size, long *offset, long *length);

#endif
