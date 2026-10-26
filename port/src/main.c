/* OpenMotoRacer: Moto Racer (Delphine Software, 1997), its software renderer, written
   again in C from what the original program does (docs/NOTES.md). */
#include "choices.h"
#include "clock.h"
#include "exe.h"
#include "files.h"
#include "menu.h"
#include "font.h"
#include "text.h"
#include "r3d.h"
#include "rand.h"
#include "sound.h"
#include "config.h"
#include "video.h"
#include <SDL.h>
#include <ctype.h>
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
         "  OMR_PAGE=N         start on the menu page N (the game's numbers; 25 the logo)\n"
         "  OMR_INPUT=\"F:WHAT ...\"  at frame F: a key (return, escape, space, tab,\n"
         "                     backspace, left, right, up, down, shift, minus, plus,\n"
         "                     a letter),\n"
         "                     move:X,Y (the mouse), click (left button down and up),\n"
         "                     down, up (the left button alone)\n"
         "  OMR_TILE=SX,SY     the menu's chequered tile scrolled so far at the start\n"
         "  OMR_TEXT=\"FONT,X,Y,ALIGN,EFFECT,BLINK,FORE,BACK,TEXT\"  a text object on the\n"
         "                     first page (FORE, BACK: the game's colour numbers, -1 for\n"
         "                     the font's own)\n"
         "  OMR_WAV=FILE       the sound into FILE instead of the speakers, 1/60 s a frame\n"
         "                     (with --shot: what the run would have sounded like)");
}

static void fail(const char *msg, int headless)
{
    fprintf(stderr, "openmotoracer: %s\n", msg);
    if (!headless)
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OpenMotoRacer", msg, 0);
    exit(1);
}

/* OMR_TEXT: one still text object, as 0x417d20 gets it */
static void test_text(const char *spec)
{
    static char text[200];
    int font, x, y, align, effect, blink, fore, back, n = 0;
    TextArgs a;
    if (sscanf(spec, "%d,%d,%d,%i,%d,%d,%d,%d,%n", &font, &x, &y, &align, &effect, &blink,
               &fore, &back, &n) < 8 || !n) {
        fprintf(stderr, "openmotoracer: OMR_TEXT wants FONT,X,Y,ALIGN,EFFECT,BLINK,FORE,BACK,"
                "TEXT\n");
        return;
    }
    snprintf(text, sizeof text, "%s", spec + n);
    memset(&a, 0, sizeof a);
    a.text = text;
    a.font = font;
    a.x = x;
    a.y = y;
    a.align = align;
    a.effect = effect;
    a.blink = blink;
    a.own_colours = fore >= 0 && back >= 0;
    if (a.own_colours) {
        a.fore = named_colour(fore);
        a.back = named_colour(back);
    }
    if (!text_add(&a))
        fprintf(stderr, "openmotoracer: OMR_TEXT: the text was not added\n");
}

/* SDL's keys as the Windows key codes the game compares against */
static int vk_of(SDL_Keycode k)
{
    if (k >= 'a' && k <= 'z')
        return k - 'a' + 'A';
    if (k >= '0' && k <= '9')
        return k;
    if (k >= SDLK_F1 && k <= SDLK_F12)
        return 0x70 + (k - SDLK_F1);
    if (k >= SDLK_KP_1 && k <= SDLK_KP_9)
        return 0x61 + (k - SDLK_KP_1);
    switch (k) {
    case SDLK_BACKSPACE: return VK_BACK;
    case SDLK_TAB: return VK_TAB;
    case SDLK_RETURN: case SDLK_KP_ENTER: return VK_RETURN;
    case SDLK_LSHIFT: case SDLK_RSHIFT: return VK_SHIFT;
    case SDLK_LCTRL: case SDLK_RCTRL: return VK_CONTROL;
    case SDLK_LALT: case SDLK_RALT: return VK_MENU;
    case SDLK_ESCAPE: return VK_ESCAPE;
    case SDLK_SPACE: return VK_SPACE;
    case SDLK_PAGEUP: return VK_PRIOR;
    case SDLK_PAGEDOWN: return VK_NEXT;
    case SDLK_END: return VK_END;
    case SDLK_HOME: return VK_HOME;
    case SDLK_LEFT: return VK_LEFT;
    case SDLK_UP: return VK_UP;
    case SDLK_RIGHT: return VK_RIGHT;
    case SDLK_DOWN: return VK_DOWN;
    case SDLK_INSERT: return VK_INSERT;
    case SDLK_KP_0: return 0x60;
    case SDLK_KP_PLUS: return VK_ADD;
    case SDLK_KP_MINUS: return VK_SUBTRACT;
    }
    return 0;
}

/* OMR_INPUT: what happens at this frame */
static void scripted_input(const char *script, int frame)
{
    static const struct { const char *name; int vk; } keys[] = {
        { "return", VK_RETURN }, { "escape", VK_ESCAPE }, { "space", VK_SPACE },
        { "tab", VK_TAB }, { "backspace", VK_BACK }, { "left", VK_LEFT },
        { "right", VK_RIGHT }, { "up", VK_UP }, { "down", VK_DOWN }, { "shift", VK_SHIFT },
        { "minus", VK_SUBTRACT }, { "plus", VK_ADD },
    };
    const char *s = script;
    while (s && *s) {
        int f, n = 0, x, y;
        char what[32];
        unsigned i;
        if (sscanf(s, " %d:%31s%n", &f, what, &n) != 2)
            break;
        s += n;
        if (f != frame)
            continue;
        if (sscanf(what, "move:%d,%d", &x, &y) == 2)
            menu_mouse_move(x, y);
        else if (strcmp(what, "click") == 0) {
            menu_mouse_button(1, 1);
            menu_mouse_button(1, 0);
        } else if (strcmp(what, "down") == 0 || strcmp(what, "up") == 0)
            menu_mouse_button(1, what[0] == 'd'); else if (strlen(what) == 1 && isalpha((unsigned char)what[0]))
            menu_key(toupper((unsigned char)what[0]));
        else
            for (i = 0; i < sizeof keys / sizeof keys[0]; i++)
                if (strcmp(what, keys[i].name) == 0) {
                    menu_key(keys[i].vk);
                    menu_key_up(keys[i].vk);
                }
    }
}

int main(int argc, char **argv)
{
    const char *data = getenv("OMR_DATA"), *shot = 0, *env, *script = getenv("OMR_INPUT");
    char err[512], shotbuf[512];
    int shot_frames = 0, frame, i, page = 25;

    if ((env = getenv("OMR_LANG")))
        g_lang = atoi(env);
    if ((env = getenv("OMR_SHOT")) && sscanf(env, "%511s %d", shotbuf, &shot_frames) == 2)
        shot = shotbuf;
    if ((env = getenv("OMR_PAGE")))
        page = atoi(env);
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
    if (page < 0 || page >= MENU_PAGES)
        page = 25;
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
    config_load();
    /* the track list and the championship (0x472c40: the game stops without them) */
    if (!tracks_load() || !champ_load())
        fail("The track lists (data\\LIST.BPT, CHAMPA.BPT...) could not be read.", shot != 0);
    if (!font_init(err, sizeof err))
        fail(err, shot != 0);
    r3d_init();
    rand_init();
    if (!video_init(shot != 0, err, sizeof err))
        fail(err, shot != 0);
    SDL_ShowCursor(SDL_DISABLE);        /* the game draws its own pointer */
    if (!shot || getenv("OMR_WAV"))     /* (--shot is silent unless asked for a file) */
        sound_init(getenv("OMR_WAV"));

    clock_start(!shot);
    /* the front end, from its first page: the Delphine logo (25) */
    if (!menu_start(page, err, sizeof err))
        fail(err, shot != 0);
    if ((env = getenv("OMR_TEXT")))
        test_text(env);
    if ((env = getenv("OMR_TILE")))
        sscanf(env, "%d,%d", &g_pics[0].sx, &g_pics[0].sy);

    for (frame = 0; !g_quit; frame++) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                g_quit = 1;
                break;
            case SDL_KEYDOWN:
                if (vk_of(e.key.keysym.sym))
                    menu_key(vk_of(e.key.keysym.sym));
                break;
            case SDL_KEYUP:
                if (vk_of(e.key.keysym.sym))
                    menu_key_up(vk_of(e.key.keysym.sym));
                break;
            case SDL_MOUSEMOTION:
                menu_mouse_move(e.motion.x, e.motion.y);
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                if (e.button.button == SDL_BUTTON_LEFT || e.button.button == SDL_BUTTON_RIGHT)
                    menu_mouse_button(e.button.button == SDL_BUTTON_LEFT,
                                      e.type == SDL_MOUSEBUTTONDOWN);
                break;
            }
        }
        scripted_input(script, frame);
        menu_frame();
        if (shot && frame + 1 >= shot_frames) {
            if (!video_shot(shot))
                fail(SDL_GetError(), 1);
            break;
        }
        video_present();
        sound_frame();
        clock_frame();
    }
    sound_quit();
    video_quit();
    SDL_Quit();
    return 0;
}
