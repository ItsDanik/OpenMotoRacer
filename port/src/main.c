/* OpenMotoRacer: Moto Racer (Delphine Software, 1997), its software renderer, written
   again in C from what the original program does (docs/NOTES.md). */
#include "bitmap.h"
#include "exe.h"
#include "files.h"
#include "video.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(void)
{
    puts("openmotoracer [options]\n"
         "  -data DIR          the Moto Racer folder, with game.EXE and data/ (OMR_DATA;\n"
         "                     default ./Moto Racer)\n"
         "  -lang N            menus' language: 0 US, 1 French, 2 German, 3 Italian,\n"
         "                     4 Spanish, 5 Swedish (OMR_LANG)\n"
         "  --shot FILE N      no window: run N frames, save the screen as a BMP, quit\n"
         "                     (OMR_SHOT=\"FILE N\")\n"
         "test switches (environment):\n"
         "  OMR_PIC=ID         show the menu picture ID of the game's table instead");
}

static void fail(const char *msg, int headless)
{
    fprintf(stderr, "openmotoracer: %s\n", msg);
    if (!headless)
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OpenMotoRacer", msg, 0);
    exit(1);
}

int main(int argc, char **argv)
{
    const char *data = getenv("OMR_DATA"), *shot = 0, *env;
    char err[512], shotbuf[512];
    int shot_frames = 0, frame, i, quit = 0;
    Bitmap title;

    if ((env = getenv("OMR_LANG")))
        g_lang = atoi(env);
    if ((env = getenv("OMR_SHOT")) && sscanf(env, "%511s %d", shotbuf, &shot_frames) == 2)
        shot = shotbuf;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-data") == 0 && i + 1 < argc)
            data = argv[++i];
        else if (strcmp(argv[i], "-lang") == 0 && i + 1 < argc)
            g_lang = atoi(argv[++i]);
        else if (strcmp(argv[i], "--shot") == 0 && i + 2 < argc) {
            shot = argv[++i];
            shot_frames = atoi(argv[++i]);
        } else {
            usage();
            return strcmp(argv[i], "--help") == 0 ? 0 : 1;
        }
    }
    if (g_lang < 0 || g_lang > 5)
        g_lang = 0;
    if (shot)
        SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
        fail(SDL_GetError(), 1);

    if (!files_init(data ? data : "Moto Racer")) {
        snprintf(err, sizeof err, "No data folder in %s.\nPoint -data (or OMR_DATA) at the "
                 "Moto Racer folder that holds game.EXE and data/.", files_data());
        fail(err, shot != 0);
    }
    if (!exe_load(err, sizeof err))
        fail(err, shot != 0);
    if (!video_init(shot != 0, err, sizeof err))
        fail(err, shot != 0);

    /* the first picture: the title page once its three logo pieces have arrived (page 14,
       0x446b80, draws the table's picture 0xbf, title4.lez, whole in its own mode);
       OMR_PIC=<id> shows another picture of the table */
    env = getenv("OMR_PIC");
    if (!bitmap_load_id(&title, env ? (int)strtol(env, 0, 0) : 0xbf))
        fail("The title picture (data\\menu\\bitmap\\title4.lez) could not be read.", shot != 0);

    for (frame = 0; !quit; frame++) {
        SDL_Event e;
        while (SDL_PollEvent(&e))
            if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
                quit = 1;
        bitmap_draw(&title, BM_OWN, 100, 0);
        if (shot && frame + 1 >= shot_frames) {
            if (!video_shot(shot))
                fail(SDL_GetError(), 1);
            break;
        }
        video_present();
    }
    bitmap_free(&title);
    video_quit();
    SDL_Quit();
    return 0;
}
