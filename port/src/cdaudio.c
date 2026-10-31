/* The 1997 CD's music (NOTES §4.10.5.5). Its tracks aren't files: the port reads them from the
   CD's image, a .cue and the .bin beside it: OMR_CD (the .cue, or its folder), else the game's
   folder, the folder above it, or a CD folder there. */
#include "cdaudio.h"
#include "files.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define MAX_TRACKS 100
#define SECTOR 2352L

typedef struct {
    int file;                   /* which FILE line of the .cue */
    long start;                 /* INDEX 01, in sectors of that file */
    int seen;
} Track;

static int g_read;
static Track g_tr[MAX_TRACKS];
static char g_files[MAX_TRACKS][700];
static int g_nfiles;

static int has_suffix(const char *name, const char *suffix)
{
    size_t n = strlen(name), m = strlen(suffix);
    return n > m && strcasecmp(name + n - m, suffix) == 0;
}

/* a name in a folder, in any case */
static int find_in(const char *dir, const char *name, char *out, size_t size)
{
    DIR *d = opendir(dir);
    struct dirent *e;
    int ok = 0;
    if (!d)
        return 0;
    while (!ok && (e = readdir(d)))
        if (strcasecmp(e->d_name, name) == 0) {
            snprintf(out, size, "%s/%s", dir, e->d_name);
            ok = 1;
        }
    closedir(d);
    return ok;
}

/* the image a .cue means: the name it gives, else (a copy renamed, as they often are) the
   .bin of the .cue's own name, else the only .bin there */
static int image_of(const char *dir, const char *cue_name, const char *given, char *out, size_t size)
{
    char stem[300];
    const char *dot = strrchr(cue_name, '.');
    DIR *d;
    struct dirent *e;
    int bins = 0;
    if (find_in(dir, given, out, size))
        return 1;
    snprintf(stem, sizeof stem, "%.*s.bin", (int)(dot ? dot - cue_name : (long)strlen(cue_name)), cue_name);
    if (find_in(dir, stem, out, size))
        return 1;
    if (!(d = opendir(dir)))
        return 0;
    while ((e = readdir(d)))
        if (has_suffix(e->d_name, ".bin")) {
            if (!bins++)
                snprintf(out, size, "%s/%s", dir, e->d_name);
        }
    closedir(d);
    return bins == 1;
}

static int read_cue(const char *dir, const char *cue_name)
{
    char path[1024], line[600];
    FILE *f;
    int track = 0, any = 0;
    snprintf(path, sizeof path, "%s/%s", dir, cue_name);
    if (!(f = fopen(path, "r")))
        return 0;
    g_nfiles = 0;
    memset(g_tr, 0, sizeof g_tr);
    while (fgets(line, sizeof line, f)) {
        char *p = line, *q;
        int n, m, s, fr;
        while (*p == ' ' || *p == '\t')
            p++;
        if (strncasecmp(p, "FILE ", 5) == 0) {
            char given[500];
            p += 5;
            while (*p == ' ')
                p++;
            if (*p == '"') {
                q = strchr(++p, '"');
            } else
                q = strchr(p, ' ');
            if (!q || g_nfiles >= MAX_TRACKS)
                break;
            snprintf(given, sizeof given, "%.*s", (int)(q - p), p);
            if (!image_of(dir, cue_name, given, g_files[g_nfiles], sizeof g_files[0]))
                g_files[g_nfiles][0] = 0;
            g_nfiles++;
        } else if (sscanf(p, "TRACK %d", &n) == 1)
            track = n > 0 && n < MAX_TRACKS && strstr(p, "AUDIO") ? n : 0;
        else if (track && g_nfiles && sscanf(p, "INDEX 01 %d:%d:%d", &m, &s, &fr) == 3) {
            g_tr[track].file = g_nfiles - 1;
            g_tr[track].start = (m * 60L + s) * 75L + fr;
            g_tr[track].seen = 1;
            any = 1;
        }
    }
    fclose(f);
    return any;
}

/* the first .cue in a folder with a track list that reads */
static int cue_in(const char *dir)
{
    DIR *d = opendir(dir);
    struct dirent *e;
    int ok = 0;
    if (!d)
        return 0;
    while (!ok && (e = readdir(d)))
        if (has_suffix(e->d_name, ".cue"))
            ok = read_cue(dir, e->d_name);
    closedir(d);
    return ok;
}

static void find_image(void)
{
    const char *env = getenv("OMR_CD");
    char dir[1100];
    if (env && *env) {
        const char *slash;
        if (!has_suffix(env, ".cue")) {
            cue_in(env);
            return;
        }
        slash = strrchr(env, '/');
        snprintf(dir, sizeof dir, "%.*s", slash ? (int)(slash - env) : 1, slash ? env : ".");
        read_cue(dir, slash ? slash + 1 : env);
        return;
    }
    if (cue_in(files_data()))
        return;
    snprintf(dir, sizeof dir, "%s/..", files_data());
    if (cue_in(dir))
        return;
    snprintf(dir, sizeof dir, "%s/../CD", files_data());
    cue_in(dir);
}

int cdaudio_track(int track, char *path, size_t size, long *offset, long *length)
{
    FILE *f;
    long end;
    int t;
    if (!g_read) {
        g_read = 1;
        find_image();
    }
    if (track < 1 || track >= MAX_TRACKS || !g_tr[track].seen || !g_files[g_tr[track].file][0])
        return 0;
    snprintf(path, size, "%s", g_files[g_tr[track].file]);
    if (!(f = fopen(path, "rb")))
        return 0;
    fseek(f, 0, SEEK_END);
    end = ftell(f);
    fclose(f);
    *offset = g_tr[track].start * SECTOR;
    /* to the next track's start, which is what the CD's table of contents says of a track's
       length: 0x442740's copy check wants 4:12:62 of track 2 */
    for (t = track + 1; t < MAX_TRACKS; t++)
        if (g_tr[t].seen && g_tr[t].file == g_tr[track].file) {
            end = g_tr[t].start * SECTOR;
            break;
        }
    *length = end - *offset;
    return *length > 0;
}
