#include "saves.h"
#include "files.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define mkdir_one(p) _mkdir(p)
#else
#define mkdir_one(p) mkdir(p, 0755)
#endif

static char g_home[512];

void saves_init(const char *home)
{
    const char *base;
    if (home && *home)
        snprintf(g_home, sizeof g_home, "%s", home);
    else if ((base = getenv("XDG_CONFIG_HOME")) && *base)
        snprintf(g_home, sizeof g_home, "%s/openmotoracer", base);
    else if ((base = getenv("HOME")) && *base)
        snprintf(g_home, sizeof g_home, "%s/.config/openmotoracer", base);
    else
        snprintf(g_home, sizeof g_home, "openmotoracer");
}

const char *saves_home(void)
{
    return g_home;
}

/* every folder on the way, as mkdir -p */
static int make_dirs(const char *path)
{
    char p[512];
    size_t i;
    snprintf(p, sizeof p, "%s", path);
    for (i = 1; p[i]; i++)
        if (p[i] == '/' || p[i] == '\\') {
            char c = p[i];
            p[i] = 0;
            if (mkdir_one(p) != 0 && errno != EEXIST)
                return 0;
            p[i] = c;
        }
    return mkdir_one(p) == 0 || errno == EEXIST;
}

FILE *save_open(const char *name, const char *mode)
{
    char path[700], rel[300];
    FILE *f;
    if (!g_home[0])
        saves_init(0);
    snprintf(path, sizeof path, "%s/%s", g_home, name);
    if (mode[0] == 'w')
        return make_dirs(g_home) ? fopen(path, mode) : 0;
    if ((f = fopen(path, mode)))
        return f;
    snprintf(rel, sizeof rel, "data\\%s", name);
    return files_find(rel, path, sizeof path) ? fopen(path, mode) : 0;
}
