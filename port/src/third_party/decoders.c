/* the decoders for the CD's music as files (sound.c, the music/ folder): Ogg Vorbis (stb_vorbis 1.22, Sean
   Barrett), MP3 (dr_mp3 0.7.4) and WAV (dr_wav 0.14.6, David Reid) -- each public domain, or
   MIT / MIT-0, as their own texts at their ends say; built here once */
#ifdef OMR_OLD_GLIBC
/* tools/release_linux.sh: pow, log and exp of glibc 2.2.5, not the 2.29 ones Debian 11 binds to,
   so that the program runs on glibc 2.27 as before the decoders (the old ones are in every glibc
   since, the same functions with the old error handling) */
__asm__(".symver pow,pow@GLIBC_2.2.5");
__asm__(".symver log,log@GLIBC_2.2.5");
__asm__(".symver exp,exp@GLIBC_2.2.5");
#endif
#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO_WIDE
#include "dr_mp3.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#include "stb_vorbis.c"
