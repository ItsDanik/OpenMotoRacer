/* Sound (NOTES §4.10). The effects: SFX.BKF's enciphered WAV files, loaded by id from the
   game's tables, each with 4 more copies (0x4415a0), played, stopped and turned as
   DirectSound buffers (0x441a40..0x441fa0) and mixed here. The queues of delayed sounds
   (0x404b30). The music: the CD's audio tracks as GOG's winmm.dll plays them, NN.wav
   beside game.EXE, read ahead a piece a picture. */
#include "sound.h"
#include "clock.h"
#include "exe.h"
#include "files.h"
#include <SDL.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RATE 44100
#define SLOTS 1024              /* CfxServer +0x30 */
#define MUSIC_RING 65536        /* frames: about 1.5 s */
#define MUSIC_AHEAD 22050       /* kept queued: half a second */

typedef struct {
    int16_t *pcm;               /* mono */
    unsigned n;                 /* samples */
    int rate;
} Sample;

typedef struct {                /* a DirectSound buffer: a sample object (0x30 bytes) */
    Sample *s;                  /* 0: a free slot */
    int next;                   /* +4: the next copy, -1 none */
    int copy;                   /* +0x28: shares its data with the head */
    int playing, loop;
    uint64_t pos;               /* in samples, 32.32 */
    int freq;                   /* what is heard */
    int hz;                     /* +0x14: as last set (the length's) */
    float gl, gr;               /* volume and pan as gains */
    int vol, pan;
    int paused;                 /* +0x24: stopped by the pause-all, to play again */
} Buffer;

static SDL_AudioDeviceID g_dev;
static FILE *g_wav;
static unsigned long g_wav_frames, g_frames;
static int g_ok;
static int g_fx_silent;             /* another program has the focus */
static int g_no_effects;            /* -NoSound (0x4dc1a8): the mixing goes on for the music */

static Buffer g_slot[SLOTS];
static int g_top;               /* +0x1030 */
static int g_handle[256];       /* 0x519ff8 */
static int g_menu_loaded;       /* 0x503554 */
static int g_race_set;          /* the race's set is loaded (0x504874: its voice table) */

/* the switches (NOTES §4.10.6): effects 0x4db664, voices 0x4db668, CD audio 0x4db66c */
static int g_effects_on = 1, g_voices_on = 1, g_cd_on = 1;
static int g_last_voice = -1, g_last_effect = -1;   /* 0x502898, 0x502710 */
static float g_fx_gain = 1.0f, g_mus_gain = 1.0f;   /* the mixer's lines (config.c) */

/* the music */
static FILE *g_mus;
static long g_mus_data, g_mus_len, g_mus_pos;        /* bytes: where the samples start */
static int g_mus_track = -1, g_mus_playing;
static int16_t g_ring[MUSIC_RING][2];
static unsigned g_ring_r, g_ring_w;                 /* frames, read and written */
static int g_mus_paused;                            /* the music holds its place (0x4b1660) */

static void lock(void)
{
    if (g_dev)
        SDL_LockAudioDevice(g_dev);
}

static void unlock(void)
{
    if (g_dev)
        SDL_UnlockAudioDevice(g_dev);
}

/* ------------------------------------------------------------------ the mixer */

static void mix(int16_t *out, int frames)
{
    int i, k;
    for (i = 0; i < frames; i++) {
        float l = 0, r = 0;
        for (k = 0; k < g_top; k++) {
            Buffer *b = &g_slot[k];
            unsigned at;
            float a, f;
            if (!b->s || !b->playing)
                continue;
            at = (unsigned)(b->pos >> 32);
            if (at >= b->s->n) {
                if (!b->loop || !b->s->n) {
                    b->playing = 0;
                    continue;
                }
                b->pos %= (uint64_t)b->s->n << 32;
                at = (unsigned)(b->pos >> 32);
            }
            f = (float)(uint32_t)b->pos * (1.0f / 4294967296.0f);
            a = b->s->pcm[at];
            if (at + 1 < b->s->n)
                a += (b->s->pcm[at + 1] - a) * f;
            else if (b->loop)
                a += (b->s->pcm[0] - a) * f;
            l += a * b->gl;
            r += a * b->gr;
            b->pos += ((uint64_t)b->freq << 32) / RATE;
        }
        l *= g_fx_silent ? 0.0f : g_fx_gain;
        r *= g_fx_silent ? 0.0f : g_fx_gain;
        if (!g_mus_paused && g_ring_r != g_ring_w) {
            l += g_ring[g_ring_r % MUSIC_RING][0] * g_mus_gain;
            r += g_ring[g_ring_r % MUSIC_RING][1] * g_mus_gain;
            g_ring_r++;
        }
        out[2 * i] = (int16_t)(l > 32767 ? 32767 : l < -32768 ? -32768 : l);
        out[2 * i + 1] = (int16_t)(r > 32767 ? 32767 : r < -32768 ? -32768 : r);
    }
}

static void callback(void *user, Uint8 *stream, int len)
{
    (void)user;
    mix((int16_t *)stream, len / 4);
}

/* ------------------------------------------------------------------ the buffers */

static void set_gains(Buffer *b)
{
    /* 0x441d50: hundredths of a dB, (v*5 - 0x4fb) * 800 / 256; 0x441e10: pan +-4000,
       the other side turned down by it */
    int att = ((b->vol * 5 - 0x4fb) * 800) / 256, pan = (b->pan * 4000) / 128;
    float g = powf(10.0f, (float)att / 2000.0f);
    b->gl = g * (pan > 0 ? powf(10.0f, (float)-pan / 2000.0f) : 1.0f);
    b->gr = g * (pan < 0 ? powf(10.0f, (float)pan / 2000.0f) : 1.0f);
}

static int new_slot(Sample *s, int copy)
{
    int k;
    for (k = 0; k < SLOTS && g_slot[k].s; k++)
        ;
    if (k == SLOTS)
        return -1;
    memset(&g_slot[k], 0, sizeof g_slot[k]);
    g_slot[k].s = s;
    g_slot[k].next = -1;
    g_slot[k].copy = copy;
    g_slot[k].freq = g_slot[k].hz = s->rate;
    g_slot[k].vol = 255;
    set_gains(&g_slot[k]);
    if (k >= g_top)
        g_top = k + 1;
    return k;
}

static unsigned u32le(const unsigned char *p)
{
    return (unsigned)p[0] | (unsigned)p[1] << 8 | (unsigned)p[2] << 16 | (unsigned)p[3] << 24;
}

/* 0x440d00 on one block read: each byte less 4(i + 8), i from the block's start */
static void decipher(unsigned char *p, unsigned long n)
{
    unsigned long i;
    for (i = 0; i < n; i++)
        p[i] = (unsigned char)(p[i] - 4 * (i + 8));
}

/* 0x440d30: the header (0x24 bytes), the data chunk's (8) and the samples, each block
   deciphered when the file starts with 'r' */
static Sample *read_wav(const char *path)
{
    long len;
    unsigned char *d = dsi_load(path, &len), h[0x24], c[8];
    unsigned fsize, size, bits, ch, rate, i;
    long p;
    Sample *s = 0;
    if (!d || len < 0x24)
        goto out;
    memcpy(h, d, 0x24);
    if (d[0] == 'r')
        decipher(h, 0x24);
    fsize = u32le(h + 0x10);
    ch = h[0x16] | h[0x17] << 8;
    rate = u32le(h + 0x18);
    bits = h[0x22] | h[0x23] << 8;
    p = 0x24 + (long)fsize - 16;
    if (memcmp(h, "RIFF", 4) || memcmp(h + 8, "WAVEfmt ", 8) || fsize < 16 || p + 8 > len)
        goto out;
    memcpy(c, d + p, 8);
    if (d[0] == 'r')
        decipher(c, 8);
    size = u32le(c + 4);
    p += 8;
    if (memcmp(c, "data", 4) || size > (unsigned long)(len - p) || ch < 1 ||
        (bits != 8 && bits != 16) || rate < 1000)
        goto out;
    if (d[0] == 'r')
        decipher(d + p, size);
    s = calloc(1, sizeof *s);
    if (!s)
        goto out;
    s->rate = (int)rate;
    s->n = size / (bits / 8 * ch);
    s->pcm = malloc(sizeof *s->pcm * (s->n ? s->n : 1));
    if (!s->pcm) {
        free(s);
        s = 0;
        goto out;
    }
    for (i = 0; i < s->n; i++) {        /* the first channel (all the files are mono) */
        const unsigned char *q = d + p + (unsigned long)i * (bits / 8 * ch);
        s->pcm[i] = bits == 16 ? (int16_t)(q[0] | q[1] << 8) : (int16_t)((q[0] - 128) << 8);
    }
out:
    free(d);
    return s;
}

/* 0x4415a0: a sample and 4 copies of it, chained */
static int load(const char *path)
{
    Sample *s = read_wav(path);
    int h, k, prev;
    if (!s) {
        fprintf(stderr, "openmotoracer: the sound %s could not be read\n", path);
        return -1;
    }
    h = prev = new_slot(s, 0);
    for (k = 0; k < 4 && prev >= 0; k++) {
        int c = new_slot(s, 1);
        g_slot[prev].next = c;
        prev = c;
    }
    return h;
}

static void free_chain(int h)
{
    Sample *s;
    if (h < 0 || !g_slot[h].s)
        return;
    s = g_slot[h].s;
    while (h >= 0) {
        int next = g_slot[h].next;
        memset(&g_slot[h], 0, sizeof g_slot[h]);
        h = next;
    }
    free(s->pcm);
    free(s);
}

/* 0x441a40 + 0x440ec0 */
static int play_buffer(int h, int flags)
{
    int c = h;
    if (h < 0 || !g_slot[h].s)
        return -1;
    if (flags & 2) {
        int last = h;
        for (c = h; c >= 0 && g_slot[c].playing; c = g_slot[c].next)
            last = c;
        if (c < 0) {
            c = new_slot(g_slot[h].s, 1);
            if (c < 0)
                return -1;
            g_slot[last].next = c;
        }
    }
    g_slot[c].pos = 0;
    g_slot[c].loop = flags & 1;
    g_slot[c].playing = 1;
    return c;
}

static void set_volume(int h, int v)
{
    if (h < 0 || !g_slot[h].s)
        return;
    g_slot[h].vol = v < 0 ? 0 : v > 255 ? 255 : v;
    set_gains(&g_slot[h]);
}

/* the sound tables: {char *name, int id} (NOTES §4.10.2) */
static void load_table(int which, int count, const char *folder)
{
    unsigned a = exe_addr(which);
    int k;
    for (k = 0; k < count; k++) {
        char path[256];
        unsigned id = exe_u32(a + 8 * k + 4);
        int h;
        if (id > 255)
            continue;
        snprintf(path, sizeof path, "data\\sfx\\16_Bits\\%s%s.wav", folder,
                 exe_str(exe_u32(a + 8 * k)));
        h = load(path);
        g_handle[id] = h;
        set_volume(h, 0xff);    /* 0x424030, 0x424070: full, in the middle */
    }
}

/* ------------------------------------------------------------------ the game's calls */

int sound_init(const char *wav, int no_effects)
{
    int k;
    for (k = 0; k < 256; k++)
        g_handle[k] = -1;
    g_no_effects = no_effects;
    g_ok = 1;
    if (wav) {
        static const unsigned char head[44] = {
            'R','I','F','F', 0,0,0,0, 'W','A','V','E','f','m','t',' ', 16,0,0,0, 1,0, 2,0,
            0x44,0xac,0,0, 0x10,0xb1,2,0, 4,0, 16,0, 'd','a','t','a', 0,0,0,0
        };
        g_wav = fopen(wav, "wb");
        if (!g_wav || fwrite(head, 1, 44, g_wav) != 44) {
            fprintf(stderr, "openmotoracer: %s could not be written\n", wav);
            g_ok = 0;
        }
        return g_ok;
    }
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) == 0) {
        SDL_AudioSpec want, got;
        memset(&want, 0, sizeof want);
        want.freq = RATE;
        want.format = AUDIO_S16SYS;
        want.channels = 2;
        want.samples = 1024;
        want.callback = callback;
        g_dev = SDL_OpenAudioDevice(0, 0, &want, &got, 0);
        if (g_dev)
            SDL_PauseAudioDevice(g_dev, 0);
    }
    if (!g_dev)
        fprintf(stderr, "openmotoracer: no sound (%s)\n", SDL_GetError());
    return 1;
}

static void wav_close(void)
{
    unsigned char n[4];
    unsigned long bytes = g_wav_frames * 4;
    int k;
    for (k = 0; k < 4; k++)
        n[k] = (unsigned char)((bytes + 36) >> 8 * k);
    fseek(g_wav, 4, SEEK_SET);
    fwrite(n, 1, 4, g_wav);
    for (k = 0; k < 4; k++)
        n[k] = (unsigned char)(bytes >> 8 * k);
    fseek(g_wav, 40, SEEK_SET);
    fwrite(n, 1, 4, g_wav);
    fclose(g_wav);
    g_wav = 0;
}

void sound_quit(void)
{
    int k;
    if (g_dev)
        SDL_CloseAudioDevice(g_dev);
    g_dev = 0;
    if (g_wav)
        wav_close();
    music_stop();
    for (k = 0; k < g_top; k++)
        if (g_slot[k].s && !g_slot[k].copy)
            free_chain(k);
}

void sound_menu_load(void)
{
    if (!sound_running())
        return;
    lock();
    if (g_handle[0x29] < 0)     /* (the game frees them before a race) */
        load_table(X_SFX_MENU, 7, "Menu\\");
    if (g_voices_on && !g_menu_loaded) {
        load_table(X_SFX_MENU_VOICES, 11, "Voices\\Menu\\");
        g_menu_loaded = 1;
    }
    unlock();
}

/* every sound of a set freed and its ids forgotten (0x4042c0 for the menu's, the race's
   at its end); 0x4044c0 starts by forgetting every id */
static void free_all(void)
{
    int k;
    sound_stop_all();
    lock();
    for (k = 0; k < g_top; k++)
        if (g_slot[k].s && !g_slot[k].copy)
            free_chain(k);
    for (k = 0; k < 256; k++)
        g_handle[k] = -1;
    g_menu_loaded = 0;
    unlock();
}

/* 0x4044c0 (from 0x410d70 at the race's load): the bikes' engines (KTM for a dirt family,
   else Ninja), the effects, (the track's ambience from Sound.Spt: not yet, 0x4059a0) and
   the voices if they are on */
void sound_race_load(int ktm)
{
    if (!sound_running())
        return;
    free_all();
    lock();
    if (ktm)
        load_table(X_SFX_KTM, 16, "Ktm\\");
    else
        load_table(X_SFX_NINJA, 11, "Ninja\\");
    load_table(X_SFX_DIVERS, 18, "Divers\\");
    if (g_voices_on)
        load_table(X_SFX_VOICES, 30, "Voices\\");
    g_race_set = 1;
    unlock();
}

/* the race's end: its set freed; the menu loads its own as it comes back */
void sound_podium_load(void)
{
    if (!sound_running())
        return;
    lock();
    if (g_handle[4] < 0)
        load_table(X_SFX_PODIUM, 2, "Menu\\");
    unlock();
}

void sound_podium_free(void)
{
    int i;
    if (!sound_running())
        return;
    lock();
    for (i = 4; i <= 5; i++)
        if (g_handle[i] >= 0) {
            free_chain(g_handle[i]);
            g_handle[i] = -1;
        }
    unlock();
}

void sound_race_free(void)
{
    if (!sound_running())
        return;
    free_all();
    g_race_set = 0;
}

int sound_running(void)
{
    return g_ok && !g_no_effects;
}

int sound_mixing(void)
{
    return g_ok;
}

void sound_set_switches(int effects, int voices, int music)
{
    g_effects_on = effects != 0;
    g_voices_on = voices != 0;
    g_cd_on = music != 0;
}

void sound_get_switches(int *effects, int *voices, int *music)
{
    *effects = g_effects_on;
    *voices = g_voices_on;
    *music = g_cd_on;
}

/* 0x408b30: off stops every sound */
void sound_toggle_effects(void)
{
    g_effects_on ^= 1;
    if (!g_effects_on)
        sound_stop_all();
}

/* 0x408f30: on loads the voices of the set in use if they were left out */
void sound_toggle_voices(void)
{
    g_voices_on ^= 1;
    if (!g_voices_on)
        return;
    if (!g_race_set)
        sound_menu_load();
    else if (sound_running() && g_handle[0x30] < 0) {
        lock();
        load_table(X_SFX_VOICES, 30, "Voices\\");
        unlock();
    }
}

/* 0x409410: off stops the music, on plays the track again */
void music_toggle(void)
{
    g_cd_on ^= 1;
    if (!g_cd_on)
        music_stop();
    else if (g_mus_track >= 0)
        music_play(g_mus_track, 2);
}

/* the mixer's lines as gains: master x wave for the effects, master x CD for the music
   (the original set Windows' own volumes) */
void sound_set_volumes(const int v[3])
{
    float m = v[0] / 32.0f;
    lock();
    g_fx_gain = m * v[2] / 32.0f;
    g_mus_gain = m * v[1] / 32.0f;
    unlock();
}

int sound_handle(int id)
{
    return id >= 0 && id < 256 ? g_handle[id] : -1;
}

/* 0x4049f0: 0x2f..0x5c are voices, the rest effects, each class with its switch, its
   volume (0x4db654 = 0xff, 0x4db658 = 0xee) and the last one played */
static int play(int id, int flags, int not_twice)
{
    int h;
    if (id >= 0x2f && id <= 0x5c) {
        if (!g_voices_on)
            return -1;
        if (not_twice && (flags & 4) && g_last_voice == id)
            return -1;
        g_last_voice = id;
        lock();
        h = play_buffer(g_handle[id], flags);
        set_volume(h, 0xff);
        unlock();
        return h;
    }
    if (!g_effects_on)
        return -1;
    if (not_twice && (flags & 4) && g_last_effect == id)
        return -1;
    if (id < 0 || id >= 256)
        return -1;
    g_last_effect = id;
    lock();
    h = play_buffer(g_handle[id], flags);
    set_volume(h, 0xee);
    unlock();
    return h;
}

int sound_play(int id, int flags)
{
    return play(id, flags, 1);
}

void sound_stop(int h)
{
    lock();
    if (h >= 0 && h < SLOTS && g_slot[h].s)
        g_slot[h].playing = g_slot[h].paused = 0;
    unlock();
}

void sound_stop_all(void)
{
    int k;
    lock();
    for (k = 0; k < g_top; k++)
        g_slot[k].playing = g_slot[k].paused = 0;
    unlock();
}

/* ------------------------------------------------------------------ the server's own calls */

/* 0x423fd0 on the id's sample (0x519ff8[id]): no switch, no volume set, no "last" kept;
   the copy playing, -1 none */
int sound_server_play(int id, int flags)
{
    int h;
    if (id < 0 || id > 255)
        return -1;
    lock();
    h = play_buffer(g_handle[id], flags);
    unlock();
    return h;
}

void sound_volume(int h, int v)         /* 0x424030 */
{
    lock();
    set_volume(h, v);
    unlock();
}

void sound_pan(int h, int p)            /* 0x424070 → 0x441e10 */
{
    if (h < 0 || h >= SLOTS || !g_slot[h].s)
        return;
    lock();
    g_slot[h].pan = p < -128 ? -128 : p > 128 ? 128 : p;
    set_gains(&g_slot[h]);
    unlock();
}

/* 0x424050 → 0x441dc0: kept always, heard only between 1000 and 44100 Hz */
void sound_frequency(int h, int hz)
{
    if (h < 0 || h >= SLOTS || !g_slot[h].s)
        return;
    lock();
    if (hz > 1000 && hz < 44100)
        g_slot[h].freq = hz;
    g_slot[h].hz = hz;
    unlock();
}

/* 0x424090 → 0x441e80: 1 stops every looping copy that plays and marks it, 0 plays the
   marked ones again from their start */
void sound_pause_all(int on)
{
    int k;
    lock();
    for (k = 0; k < g_top; k++) {
        Buffer *b = &g_slot[k];
        if (!b->s)
            continue;
        if (on && b->playing && b->loop) {
            b->playing = 0;
            b->paused = 1;
        } else if (!on && b->paused) {
            b->paused = 0;
            b->pos = 0;
            b->playing = 1;
        }
    }
    unlock();
}

int sound_last_voice(void)              /* 0x502898 */
{
    return g_last_voice;
}

void sound_set_last_voice(int id)
{
    g_last_voice = id;
}

int sound_last_effect(void)             /* 0x502710 */
{
    return g_last_effect;
}

void sound_set_last_effect(int id)
{
    g_last_effect = id;
}

/* 0x441fa0: in whole samples, then ms at the buffer's frequency */
unsigned sound_length(int h)
{
    if (h < 0 || h >= SLOTS || !g_slot[h].s || g_slot[h].hz <= 0)
        return 0;
    return g_slot[h].s->n * 1000u / (unsigned)g_slot[h].hz;
}

/* ------------------------------------------------------------------ the queues */

typedef struct {                /* 0x64 bytes */
    int mode, idle, state, i, count, h;
    unsigned start, played, len;
    int ids[8], delays[8];
} Queue;

static Queue g_queue[8];        /* 0x504240 */
static Queue g_vqueue = { .mode = 1, .idle = 1 };   /* 0x5028c8: one after the other */

void sound_queue_reset(void)
{
    int k;
    for (k = 0; k <= 8; k++) {
        Queue *q = k < 8 ? &g_queue[k] : &g_vqueue;
        q->idle = 1;
        q->state = q->i = q->count = 0;
    }
}

void sound_queue(int n, ...)
{
    va_list ap;
    int k;
    Queue *q = 0;
    for (k = 0; k < 8 && !q; k++)
        if (g_queue[k].idle)
            q = &g_queue[k];
    if (!q)
        return;
    memset(q, 0, sizeof *q);
    va_start(ap, n);
    for (k = 0; k < n && k < 8; k++) {
        q->delays[k] = va_arg(ap, int);
        q->ids[k] = va_arg(ap, int);
    }
    va_end(ap);
    q->count = k;
}

void sound_queue_voice(int id, int delay)
{
    Queue *q = &g_vqueue;
    if (!q->idle && ((q->count + 1) & 7) == q->i)
        return;                 /* full */
    q->ids[q->count] = id;
    q->delays[q->count] = delay;
    q->count = (q->count + 1) & 7;
    q->idle = 0;
}

/* 0x404b30: one step of a queue a call */
static void queue_step(Queue *q)
{
    if (q->idle)
        return;
    switch (q->state) {
    case 0:
        q->start = g_now - sound_length(sound_handle(q->ids[q->i]));
        q->state = 1;
        break;
    case 1:
        if ((int)(g_now - q->start) >= q->delays[q->i] || q->mode) {
            q->h = play(q->ids[q->i], 0, 0);
            q->played = g_now;
            q->len = q->h >= 0 ? sound_length(q->h) : 0;
            q->state = 2;
        }
        break;
    case 2:
        if (q->mode && (int)(g_now - q->played) <= q->delays[q->i] + (int)q->len)
            break;
        q->i = (q->i + 1) & 7;
        if (q->i == q->count)
            q->idle = 1;
        else
            q->state = 0;
        break;
    }
}

void sound_update(void)
{
    int k;
    for (k = 0; k < 8; k++)
        queue_step(&g_queue[k]);
    queue_step(&g_vqueue);
}

/* ------------------------------------------------------------------ the music */

/* GOG's winmm.dll: MCI_PLAY of track n is PlaySound("0n.wav" or "n.wav", SND_LOOP), so
   every track goes round and round (NOTES §4.10.5) */
static int music_open(int track)
{
    char name[16], path[1024];
    unsigned char h[12], c[8];
    if (g_mus)
        fclose(g_mus);
    g_mus = 0;
    snprintf(name, sizeof name, track < 10 ? "0%d.wav" : "%d.wav", track);
    if (!files_find(name, path, sizeof path) || !(g_mus = fopen(path, "rb")))
        return 0;
    if (fread(h, 1, 12, g_mus) != 12 || memcmp(h, "RIFF", 4) || memcmp(h + 8, "WAVE", 4))
        goto bad;
    for (;;) {                  /* the chunks, to "data"; "fmt " is 44.1 kHz 16-bit stereo */
        unsigned n;
        if (fread(c, 1, 8, g_mus) != 8)
            goto bad;
        n = u32le(c + 4);
        if (!memcmp(c, "fmt ", 4)) {
            unsigned char f[16];
            if (n < 16 || fread(f, 1, 16, g_mus) != 16)
                goto bad;
            if ((f[0] | f[1] << 8) != 1 || (f[2] | f[3] << 8) != 2 || u32le(f + 4) != RATE ||
                (f[14] | f[15] << 8) != 16) {
                fprintf(stderr, "openmotoracer: %s is not 44.1 kHz 16-bit stereo\n", name);
                goto bad;
            }
            n -= 16;
        } else if (!memcmp(c, "data", 4)) {
            g_mus_data = ftell(g_mus);
            g_mus_len = (long)(n & ~3u);
            g_mus_pos = 0;
            return 1;
        }
        if (fseek(g_mus, (long)(n + (n & 1)), SEEK_CUR))
            goto bad;
    }
bad:
    fclose(g_mus);
    g_mus = 0;
    return 0;
}

static void music_fill(unsigned want)
{
    unsigned char buf[4096];
    while (g_mus && g_mus_len > 0 && g_ring_w - g_ring_r < want) {
        unsigned n = want - (g_ring_w - g_ring_r), i;
        size_t got;
        if (n > sizeof buf / 4)
            n = sizeof buf / 4;
        if (g_mus_pos >= g_mus_len) {           /* SND_LOOP */
            fseek(g_mus, g_mus_data, SEEK_SET);
            g_mus_pos = 0;
        }
        if ((long)n * 4 > g_mus_len - g_mus_pos)
            n = (unsigned)((g_mus_len - g_mus_pos) / 4);
        got = fread(buf, 4, n, g_mus);
        if (!got) {
            fclose(g_mus);
            g_mus = 0;
            break;
        }
        g_mus_pos += (long)got * 4;
        lock();
        for (i = 0; i < got; i++) {
            g_ring[g_ring_w % MUSIC_RING][0] = (int16_t)(buf[4 * i] | buf[4 * i + 1] << 8);
            g_ring[g_ring_w % MUSIC_RING][1] = (int16_t)(buf[4 * i + 2] | buf[4 * i + 3] << 8);
            g_ring_w++;
        }
        unlock();
    }
}

/* 0x404990 → 0x4b18e0 (choose) and 0x4b19f0 (play): modes 1 and 3 leave a track going
   when it is the one asked for. (Track 0, a random one, is asked for by no level.) */
void music_play(int track, int mode)
{
    if (!g_cd_on || track < 2 || track > 13)
        return;
    if ((mode == 1 || mode == 3) && g_mus_playing && track == g_mus_track)
        return;
    lock();
    g_ring_r = g_ring_w = 0;
    g_mus_paused = 0;
    unlock();
    g_mus_track = track;
    g_mus_playing = music_open(track);
    music_fill(MUSIC_AHEAD);
}

void music_stop(void)
{
    if (g_mus)
        fclose(g_mus);
    g_mus = 0;
    g_mus_playing = 0;
    lock();
    g_ring_r = g_ring_w = 0;
    g_mus_paused = 0;
    unlock();
}

/* 0x4096e0 -> 0x4b1660: the music holds, and goes on from there */
void music_pause(int on)
{
    if (!g_cd_on)
        return;
    lock();
    g_mus_paused = on;
    unlock();
}

/* 0x409670, on WM_ACTIVATEAPP: the music that plays holds while another program has the focus
   (0x4db678). A pause of its own stays as it was. */
void music_focus(int gained)
{
    static int held;
    lock();
    g_fx_silent = !gained;      /* DirectSound: buffers with no GLOBALFOCUS are not heard */
    unlock();
    if (!gained && g_mus_playing && !g_mus_paused) {
        music_pause(1);
        held = 1;
    } else if (gained && held) {
        music_pause(0);
        held = 0;
    }
}

void sound_frame(void)
{
    if (!g_ok)
        return;
    music_fill(g_wav ? RATE / 60 + 2 : MUSIC_AHEAD);
    if (g_wav) {                /* exactly 44100 frames every 60 pictures */
        int16_t out[2 * (RATE / 60 + 2)];
        unsigned long n;
        g_frames++;
        n = g_frames * RATE / 60 - g_wav_frames;
        mix(out, (int)n);
        fwrite(out, 4, n, g_wav);
        g_wav_frames += n;
    }
}

int sound_effects_on(void)
{
    return g_effects_on;
}

int sound_voices_on(void)
{
    return g_voices_on;
}

/* 0x4059a0's load of one of the track's ambient samples (DIVERS\name.wav) as id, if that
   id has none yet: volume full, pan in the middle */
void sound_load_named(int id, const char *name)
{
    char path[256];
    if (!sound_running() || id < 0 || id > 255 || g_handle[id] >= 0)
        return;
    snprintf(path, sizeof path, "data\\sfx\\16_Bits\\Divers\\%s.wav", name);
    lock();
    g_handle[id] = load(path);
    set_volume(g_handle[id], 0xff);
    unlock();
}

int sound_rate(int id)                  /* 0x442010: the file's own rate, 0 none */
{
    int h = sound_handle(id);
    return h >= 0 && g_slot[h].s ? g_slot[h].s->rate : 0;
}
