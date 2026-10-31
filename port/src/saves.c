#include "saves.h"
#include "files.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define mkdir_one(p) _mkdir(p)
#define get_cwd _getcwd
#define can_write(p) (_access(p, 2) == 0)
#else
#include <unistd.h>
#define mkdir_one(p) mkdir(p, 0755)
#define get_cwd getcwd
#define can_write(p) (access(p, W_OK) == 0)
#endif

static char g_home[512];
static char g_old[512];         /* the user's folder, read when the home has no such file */

/* the user's folder, where the saves went before they went beside the program's work */
static void user_folder(char *out, size_t size)
{
    const char *base;
    out[0] = 0;
#ifdef _WIN32
    if ((base = getenv("APPDATA")) && *base) {
        snprintf(out, size, "%s/OpenMotoRacer", base);
        return;
    }
#endif
    if ((base = getenv("XDG_CONFIG_HOME")) && *base)
        snprintf(out, size, "%s/openmotoracer", base);
    else if ((base = getenv("HOME")) && *base)
        snprintf(out, size, "%s/.config/openmotoracer", base);
}

/* -home, else the working directory, else (one that can't be written) the user's folder */
void saves_init(const char *home)
{
    char cwd[480];
    g_old[0] = 0;
    if (home && *home) {
        snprintf(g_home, sizeof g_home, "%s", home);
        return;
    }
    user_folder(g_old, sizeof g_old);
    if (get_cwd(cwd, sizeof cwd) && can_write(cwd))
        snprintf(g_home, sizeof g_home, "%s", cwd);
    else if (g_old[0])
        snprintf(g_home, sizeof g_home, "%s", g_old);
    else
        snprintf(g_home, sizeof g_home, "openmotoracer");
    if (!strcmp(g_home, g_old))
        g_old[0] = 0;
}

const char *saves_home(void)
{
    return g_home;
}

/* every folder on the way, as mkdir -p; only the last one's answer counts (Windows answers
   "access denied" for a drive, C:, that is there all the same) */
static int make_dirs(const char *path)
{
    char p[700];
    size_t i;
    snprintf(p, sizeof p, "%s", path);
    for (i = 1; p[i]; i++)
        if (p[i] == '/' || p[i] == '\\') {
            char c = p[i];
            p[i] = 0;
            mkdir_one(p);
            p[i] = c;
        }
    return mkdir_one(p) == 0 || errno == EEXIST;
}

/* the player's folder's path of a save (a track's folder: '\\' to '/') */
static void home_path(char *path, size_t size, const char *name)
{
    char *p;
    if (!g_home[0])
        saves_init(0);
    snprintf(path, size, "%s/%s", g_home, name);
    for (p = path + strlen(g_home); *p; p++)
        if (*p == '\\')
            *p = '/';
}

/* the folders a save's path needs */
static int make_parents(const char *path)
{
    char dir[700];
    char *slash;
    snprintf(dir, sizeof dir, "%s", path);
    slash = strrchr(dir, '/');
    if (!slash)
        return 1;
    *slash = 0;
    return make_dirs(dir);
}

/* the same save in the user's folder */
static void old_path(char *path, size_t size, const char *name)
{
    char *p;
    snprintf(path, size, "%s/%s", g_old, name);
    for (p = path + strlen(g_old); *p; p++)
        if (*p == '\\')
            *p = '/';
}

/* a save read: from the home, else the user's folder */
static FILE *own_read(const char *name, const char *mode)
{
    char path[700];
    FILE *f;
    home_path(path, sizeof path, name);
    if ((f = fopen(path, mode)) || !g_old[0])
        return f;
    old_path(path, sizeof path, name);
    return fopen(path, mode);
}

FILE *save_open_own(const char *name, const char *mode)
{
    char path[700];
    if (mode[0] != 'w')
        return own_read(name, mode);
    home_path(path, sizeof path, name);
    return fopen(path, mode);
}

FILE *save_open(const char *name, const char *mode)
{
    char path[700], rel[300];
    FILE *f;
    home_path(path, sizeof path, name);
    if (mode[0] == 'w')
        return make_parents(path) ? fopen(path, mode) : 0;
    if ((f = own_read(name, mode)))
        return f;
    snprintf(rel, sizeof rel, "data\\%s", name);
    return files_find(rel, path, sizeof path) ? fopen(path, mode) : 0;
}

void save_forget(const char *name)
{
    char path[700], rel[300], game[700];
    FILE *f;
    int old = 0;
    home_path(path, sizeof path, name);
    remove(path);
    snprintf(rel, sizeof rel, "data\\%s", name);
    if (g_old[0]) {
        old_path(game, sizeof game, name);
        if ((f = fopen(game, "rb")))
            fclose(f), old = 1;
    }
    if (old || files_find(rel, game, sizeof game)) {
        f = make_parents(path) ? fopen(path, "wb") : 0;
        if (f)
            fclose(f);
    }
}
