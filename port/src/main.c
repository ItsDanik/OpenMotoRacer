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
#include "land.h"
#include "horizon.h"
#include "saves.h"
#include "video.h"
#include "input.h"
#include "joy.h"
#include "pad.h"
#include "dialog.h"
#include "race.h"
#include <SDL.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static void usage(void)
{
    static const char text[] =
         "openmotoracer [options]\n"
         "  -data DIR          the Moto Racer folder, with game.EXE and data/ (OMR_DATA;\n"
         "                     default ./Moto Racer, or beside the program, or the\n"
         "                     program's own folder)\n"
         "  -home DIR          where the hi-scores and championship saves are written\n"
         "                     (OMR_HOME; default ~/.config/openmotoracer, on Windows\n"
         "                     %APPDATA%\\OpenMotoRacer; the game's own data/ is read\n"
         "                     when a file is not there)\n"
         "  -lang N            menus' language: 0 US, 1 French, 2 German, 3 Italian,\n"
         "                     4 Spanish, 5 Swedish (OMR_LANG)\n"
         "  -jN                the joystick's dead zone, 0..0xffff (the game's own switch;\n"
         "                     OMR_JOYDEAD; default 2048)\n"
         "  -windowed          start in a window, whatever the settings say (the game's\n"
         "                     own switch; OMR_WINDOWED=1)\n"
         "  -fullscreen        start on the full screen (the game's own; OMR_FULLSCREEN=1)\n"
         "  -NoSound           no sound effects or voices; the music stays, as the CD's did\n"
         "                     (the game's own; OMR_NOSOUND=1)\n"
         "  --shot FILE N      no window: run N frames, save the screen as a BMP, quit\n"
         "                     (OMR_SHOT=\"FILE N\")\n"
         "test switches (environment):\n"
         "  OMR_PAGE=N         start on the menu page N (the game's numbers; 25 the logo)\n"
         "  OMR_PODIUM=N       page 24: the player's place, 1..4\n"
         "  OMR_INPUT=\"F:WHAT ...\"  at frame F: blur, focus (another program has it, or\n"
         "                     not), a key (return, escape, space, tab,\n"
         "                     backspace, left, right, up, down, shift, ctrl, minus, plus,\n"
         "                     a letter),\n"
         "                     move:X,Y (the mouse), click (left button down and up),\n"
         "                     down, up (the left button alone),\n"
         "                     joy:X,Y,BUTTONS (joystick 0 held so: 0..65535, the button\n"
         "                     bits; with --shot no real joystick is read); or\n"
         "                     pad:X,Y,BUTTONS,LT,RT,CROSS (a gamepad; buttons as joy.h,\n"
         "                     triggers 0..32767, the cross 1 up 2 down 4 left 8 right)\n"
         "  OMR_DEVICE=N       the controller: 1 keyboard, 2 joystick, 4 joypad\n"
         "  OMR_CHOICES=MODE,TRACK,LAPS[,REVERSE,POCKET,MANUAL]  the race chosen (MODE 0\n"
         "                     practice, 1 single race, 2 championship; MANUAL 1: the\n"
         "                     manual gearbox), for OMR_PAGE=9 and on\n"
         "  OMR_LIST=N         the track list: data\\list.bpt (0) or listNN.bpt, as page 23 chooses\n"
         "  OMR_TILE=SX,SY     the menu's chequered tile scrolled so far at the start\n"
         "  OMR_TURN=ANGLE     page 10's turntable held at this angle (radians)\n"
         "  OMR_TEXT=\"FONT,X,Y,ALIGN,EFFECT,BLINK,FORE,BACK,TEXT\"  a text object on the\n"
         "                     first page (FORE, BACK: the game's colour numbers, -1 for\n"
         "                     the font's own)\n"
         "  OMR_WAV=FILE       the sound into FILE instead of the speakers, 1/60 s a frame\n"
         "                     (with --shot: what the run would have sounded like)\n"
         "  OMR_RACEVIEW=\"TRACK,FOCAL,X,Y,Z,C00,C01,...,C22\"  no menu: the track (its number\n"
         "                     in the list) seen from a camera at X,Y,Z with the matrix C\n"
         "                     (the camera block's +0x68), FOCAL 394 as the race's\n"
         "  OMR_AUTOPILOT=1    the computer rides the player's bike; =win: and the others\n"
         "                     crawl, with no time limit (to reach the winner's lap)\n"
         "  OMR_RACELOG=FILE   a sum of every rider's place and speed, a line a step\n"
         "  OMR_WATCH_AT=N     the race's replay is asked for at step N\n"
         "  OMR_SHOT_STEP=N    with --shot: the picture when the race has run N steps\n"
         "  OMR_SCREEN=MODE[,WIDE]  the race's screen: 0 320x240, 1 512x384, 2 640x480; WIDE 1: 16:9\n"
         "  OMR_DIFFICULTY=N   0 easy, 1 medium, 2 hard (and saved so, as the Options page does)\n"
         "  OMR_UNLOCK_ALL=1   every track of the list in use available (and saved so when the\n"
         "                     program ends; tools/unlock_all.sh does it for every list)\n"
         "  OMR_PERF=1         every 5 seconds: pictures a second, the time to make one, the slowest";
#ifdef _WIN32
    /* (a windowed program has no terminal to print on) */
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "OpenMotoRacer", text, 0);
#else
    puts(text);
#endif
}

/* OMR_PERF: what the MiSTer's ARM has to be measured with (the picture's making, without
   waiting for the screen) */
static void perf_frame(Uint64 start, Uint64 drawn)
{
    static Uint64 since, work, worst;
    static int n;
    Uint64 now = SDL_GetPerformanceCounter(), freq = SDL_GetPerformanceFrequency();
    if (!since)
        since = start;
    work += drawn - start;
    if (drawn - start > worst)
        worst = drawn - start;
    n++;
    if (now - since >= 5 * freq) {
        fprintf(stderr, "perf: %.1f pictures a second, %.2f ms to make one, slowest %.2f ms\n",
                (double)n * (double)freq / (double)(now - since),
                1000.0 * (double)work / (double)n / (double)freq, 1000.0 * (double)worst / (double)freq);
        since = now, work = worst = 0, n = 0;
    }
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

/* OMR_RACEVIEW: a track from a still camera, nothing else yet */
static float g_view_c[3][3], g_view_pos[3], g_view_focal;
static int g_view_sec = -1;

static int race_view_start(const char *spec, char *err, int errsize)
{
    float v[14];
    int track, n = 0, k;
    const char *s = spec;
    if (sscanf(s, "%d%n", &track, &n) != 1)
        n = -1;
    for (k = 0; k < 13 && n >= 0; k++) {
        int m = 0;
        s += n;
        if (sscanf(s, " ,%f%n", &v[k], &m) != 1)
            n = -1;
        else
            n = m;
    }
    if (n < 0) {
        snprintf(err, (size_t)errsize, "OMR_RACEVIEW wants TRACK,FOCAL,X,Y,Z and nine numbers.");
        return 0;
    }
    if (!land_load(track, err, errsize))
        return 0;
    g_view_focal = v[0];
    for (k = 0; k < 3; k++)
        g_view_pos[k] = v[1 + k];
    for (k = 0; k < 9; k++)
        g_view_c[k / 3][k % 3] = v[4 + k];
    g_view_sec = land_section(g_view_pos, -1);
    fprintf(stderr, "openmotoracer: section %d\n", g_view_sec);
    return 1;
}

static void race_view_frame(void)
{
    int y;
    static const float t[3] = { 0, 0, 0 };
    for (y = 0; y < g_scr_h; y++)
        memset(g_screen + y * g_pitch, 0, sizeof(uint16_t) * (size_t)g_scr_w);
    horizon_draw((const float (*)[3])g_view_c, t, g_view_pos[1], g_land.horizon_offset,
                 g_land.horizon_scale, g_view_focal);
    r3d_race(g_detail);
    r3d_focal(g_view_focal);
    land_draw(g_view_sec, (const float (*)[3])g_view_c, g_view_pos);
    r3d_render();
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
    case SDLK_PAUSE: return 0x13;
    case SDLK_KP_0: return 0x60;
    case SDLK_KP_PLUS: return VK_ADD;
    case SDLK_KP_MINUS: return VK_SUBTRACT;
    case SDLK_KP_MULTIPLY: return 0x6a;
    case SDLK_KP_DIVIDE: return 0x6f;
    }
    return 0;
}

/* WM_ACTIVATEAPP (0x43336b): the music holds, the bike's keys go, the texts and pictures are
   not drawn and the full screen gives way. Nothing stops the race (NOTES §4.4) */
static void app_focus(int gained)
{
    music_focus(gained);
    video_focus(gained);
    if (!gained && g_racing)
        race_release_keys();
}

/* OMR_INPUT: what happens at this frame */
static void scripted_input(const char *script, int frame)
{
    static const struct { const char *name; int vk; } keys[] = {
        { "return", VK_RETURN }, { "escape", VK_ESCAPE }, { "space", VK_SPACE },
        { "tab", VK_TAB }, { "backspace", VK_BACK }, { "left", VK_LEFT },
        { "right", VK_RIGHT }, { "up", VK_UP }, { "down", VK_DOWN }, { "shift", VK_SHIFT },
        { "ctrl", VK_CONTROL },
        { "minus", VK_SUBTRACT }, { "plus", VK_ADD }, { "f2", 0x71 }, { "f3", 0x72 }, { "f4", 0x73 },
        { "f5", 0x74 }, { "f6", 0x75 }, { "f7", 0x76 }, { "f8", 0x77 }, { "f9", 0x78 },
        { "f10", 0x79 }, { "pause", 0x13 }, { "f1", 0x70 }, { "f11", 0x7a },
        { "f12", 0x7b }, { "pgup", VK_PRIOR }, { "pgdn", VK_NEXT }, { "home", VK_HOME },
        { "end", VK_END }, { "star", 0x6a }, { "slash", 0x6f },
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
        unsigned jb, cross;
        int lt, rt;
        if (sscanf(what, "move:%d,%d", &x, &y) == 2) {
            menu_mouse_move(x, y);
            dialog_mouse_move(x, y);
        }
        else if (sscanf(what, "pad:%d,%d,%u,%d,%d,%u", &x, &y, &jb, &lt, &rt, &cross) == 6)
            joy_fake_pad(x, y, jb, lt, rt, cross);
        else if (sscanf(what, "joy:%d,%d,%u", &x, &y, &jb) == 3)
            joy_fake(x, y, jb);
        else if (strcmp(what, "blur") == 0 || strcmp(what, "focus") == 0)
            app_focus(what[0] == 'f');
        else if (strcmp(what, "click") == 0) {
            menu_mouse_button(1, 1);
            dialog_mouse_button(1, 1);
            menu_mouse_button(1, 0);
            dialog_mouse_button(1, 0);
        } else if (strcmp(what, "down") == 0 || strcmp(what, "up") == 0) {
            menu_mouse_button(1, what[0] == 'd');
            dialog_mouse_button(1, what[0] == 'd');
        }
        else {
            /* KEY: pressed and let go; hold:KEY, release:KEY: for the race's held keys */
            int hold = strncmp(what, "hold:", 5) == 0, release = strncmp(what, "release:", 8) == 0;
            const char *k = hold ? what + 5 : release ? what + 8 : what;
            int vk = 0;
            if (strlen(k) == 1 && isalnum((unsigned char)k[0]))
                vk = toupper((unsigned char)k[0]);
            for (i = 0; i < sizeof keys / sizeof keys[0]; i++)
                if (strcmp(k, keys[i].name) == 0)
                    vk = keys[i].vk;
            if (!vk)
                continue;
            if (!release) {
                input_key(vk, 1);
                race_key(vk, 1);
                menu_key(vk);
                g_menu_char = vk == VK_BACK ? 8 : vk == VK_RETURN ? 13
                    : strlen(k) == 1 ? tolower((unsigned char)k[0]) : g_menu_char;
            }
            if (!hold) {
                input_key(vk, 0);
                race_key(vk, 0);
                menu_key_up(vk);
            }
        }
    }
}

int main(int argc, char **argv)
{
    int perf;
    const char *data = getenv("OMR_DATA"), *home = getenv("OMR_HOME"), *shot = 0, *env, *script = getenv("OMR_INPUT");
    char err[512], shotbuf[512];
    int shot_frames = 0, frame, i, page = 25, forced_screen = -1, no_sound;

    perf = getenv("OMR_PERF") != 0;
    no_sound = getenv("OMR_NOSOUND") != 0;
    if (getenv("OMR_FULLSCREEN"))
        forced_screen = 1;
    if (getenv("OMR_WINDOWED"))
        forced_screen = 0;
    if ((env = getenv("OMR_LANG")))
        g_lang = atoi(env);
    if ((env = getenv("OMR_SHOT")) && sscanf(env, "%511s %d", shotbuf, &shot_frames) == 2)
        shot = shotbuf;
    if ((env = getenv("OMR_PAGE")))
        page = atoi(env);
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-data") == 0 && i + 1 < argc)
            data = argv[++i];
        else if (strcmp(argv[i], "-home") == 0 && i + 1 < argc)
            home = argv[++i];
        else if (strcmp(argv[i], "-lang") == 0 && i + 1 < argc)
            g_lang = atoi(argv[++i]);
        else if (strncmp(argv[i], "-j", 2) == 0 && argv[i][2])  /* the game's own */
            g_joy_dead = atoi(argv[i] + 2);
        else if (strcasecmp(argv[i], "-windowed") == 0)         /* 0x414080: any case */
            forced_screen = 0;
        else if (strcasecmp(argv[i], "-fullscreen") == 0)
            forced_screen = 1;
        else if (strcasecmp(argv[i], "-NoSound") == 0)
            no_sound = 1;
        else if (strcmp(argv[i], "--shot") == 0 && i + 2 < argc) {
            shot = argv[++i];
            shot_frames = atoi(argv[++i]);
        } else {
            usage();
            return strcmp(argv[i], "--help") == 0 ? 0 : 1;
        }
    }
    if ((env = getenv("OMR_JOYDEAD")))
        g_joy_dead = atoi(env);
    if (g_lang < 0 || g_lang > 5)
        g_lang = 0;
    if (page < 0 || page >= MENU_PAGES)
        page = 25;
    if (shot)
        SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
        fail(SDL_GetError(), 1);
    if (!shot)                          /* (tests read no real joystick) */
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);

    if (!files_init(data)) {
        snprintf(err, sizeof err, "No data folder in %s.\nPoint -data (or OMR_DATA) at the "
                 "Moto Racer folder that holds game.EXE and data/.", files_data());
        fail(err, shot != 0);
    }
    saves_init(home);
    if (!exe_load(err, sizeof err))
        fail(err, shot != 0);
    config_load();
    if ((env = getenv("OMR_DIFFICULTY")))
        g_difficulty = atoi(env) % 3;
    if ((env = getenv("OMR_SCREEN"))) {
        int m = 2, wide = 0;
        sscanf(env, "%d,%d", &m, &wide);
        g_race_screen = m >= 0 && m <= 2 ? m : 2;
        g_mode169 = wide != 0;
    }
    if ((env = getenv("OMR_DEVICE")))
        g_input_device = atoi(env);
    if (script && strstr(script, ":pad:"))      /* (a pretend pad is there from the start) */
        joy_fake_pad(0x8000, 0x8000, 0, 0, 0, 0);
    joy_detect();                       /* 0x4189b0, at the start */
    if ((env = getenv("OMR_LIST")))
        g_list_number = atoi(env);
    /* the track list and the championship (0x472c40: the game stops without them) */
    if (!tracks_load() || !champ_load())
        fail("The track lists (data\\LIST.BPT, CHAMPA.BPT...) could not be read.", shot != 0);
    if (getenv("OMR_UNLOCK_ALL"))
        for (i = 0; i < g_ntracks; i++)
            g_tracks[i].available = 1;
    if (!font_init(err, sizeof err))
        fail(err, shot != 0);
    r3d_init();
    rand_init();
    race_demos_load();
    if (!video_init(shot != 0, err, sizeof err))
        fail(err, shot != 0);
    /* FullScreen from the settings, unless a switch forced it (0x42b0d9) */
    video_set_fullscreen(forced_screen >= 0 ? forced_screen : config_fullscreen());
    SDL_ShowCursor(SDL_DISABLE);        /* the game draws its own pointer */
    if (!shot || getenv("OMR_WAV"))     /* (--shot is silent unless asked for a file) */
        sound_init(getenv("OMR_WAV"), no_sound);

    clock_start(!shot);
    if ((env = getenv("OMR_RACEVIEW"))) {
        if (!race_view_start(env, err, sizeof err))
            fail(err, shot != 0);
    } else if (!menu_start(page, err, sizeof err))  /* from the logo (25) */
        fail(err, shot != 0);
    if ((env = getenv("OMR_TEXT")))
        test_text(env);
    if ((env = getenv("OMR_CHOICES"))) {
        int manual = 0;
        sscanf(env, "%d,%d,%d,%d,%d,%d", &g_mode, &g_track, &g_laps, &g_reverse, &g_pocket,
               &manual);
        g_gearbox = !manual;
        if (g_track < 0 || g_track >= g_ntracks)
            g_track = 0;
    }
    if ((env = getenv("OMR_TILE")))
        if (sscanf(env, "%d,%d", &g_pics[0].sx, &g_pics[0].sy) == 2)
            for (i = 1; i < MENU_PICS; i++)     /* every page's tile */
                if (g_pics[i].mode == BM_TILE) {
                    g_pics[i].sx = g_pics[0].sx;
                    g_pics[i].sy = g_pics[0].sy;
                }

    for (frame = 0; !g_quit; frame++) {
        Uint64 perf_start = perf ? SDL_GetPerformanceCounter() : 0;
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                g_quit = 1;
                break;
            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST
                    || e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
                    app_focus(e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED);
                break;
            case SDL_KEYDOWN:
                if (e.key.keysym.mod & KMOD_ALT) {
                    /* WM_SYSKEYDOWN, not a key to the game: only Return, the full screen
                       (0x43355f); not on the podium (0x4e8934) */
                    if ((e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER)
                        && !e.key.repeat && g_alt_return)
                        video_toggle_fullscreen();
                    break;
                }
                if (vk_of(e.key.keysym.sym)) {
                    input_key(vk_of(e.key.keysym.sym), 1);
                    race_key(vk_of(e.key.keysym.sym), 1);
                    menu_key(vk_of(e.key.keysym.sym));
                }
                /* WM_CHAR: Backspace and Return give one too */
                if (e.key.keysym.sym == SDLK_BACKSPACE)
                    g_menu_char = 8;
                else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER)
                    g_menu_char = 13;
                break;
            case SDL_TEXTINPUT:
                if ((unsigned char)e.text.text[0] >= 0x20 && (unsigned char)e.text.text[0] < 0x7f)
                    g_menu_char = e.text.text[0];
                break;
            case SDL_KEYUP:
                if (vk_of(e.key.keysym.sym)) {
                    input_key(vk_of(e.key.keysym.sym), 0);
                    race_key(vk_of(e.key.keysym.sym), 0);
                    menu_key_up(vk_of(e.key.keysym.sym));
                }
                break;
            case SDL_JOYDEVICEADDED:
            case SDL_JOYDEVICEREMOVED:
                joy_detect();
                break;
            case SDL_MOUSEMOTION: {
                int mx = e.motion.x, my = e.motion.y;
                video_mouse(&mx, &my);
                menu_mouse_move(mx, my);
                dialog_mouse_move(mx, my);
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                if (e.button.button == SDL_BUTTON_LEFT || e.button.button == SDL_BUTTON_RIGHT) {
                    menu_mouse_button(e.button.button == SDL_BUTTON_LEFT,
                                      e.type == SDL_MOUSEBUTTONDOWN);
                    dialog_mouse_button(e.button.button == SDL_BUTTON_LEFT,
                                        e.type == SDL_MOUSEBUTTONDOWN);
                }
                break;
            }
        }
        scripted_input(script, frame);
        pad_frame();
        if (g_view_sec >= 0)
            race_view_frame();
        else if (g_racing)
            race_frame();
        else
            menu_frame();
        if (perf)
            perf_frame(perf_start, SDL_GetPerformanceCounter());
        if (shot && (frame + 1 >= shot_frames
                     || (getenv("OMR_SHOT_STEP") && g_race_steps >= atoi(getenv("OMR_SHOT_STEP"))))) {
            if (!video_shot(shot))
                fail(SDL_GetError(), 1);
            break;
        }
        video_present();
        video_frame_end();
        sound_frame();
        clock_frame();
    }
    config_save();              /* (the game: on WM_DESTROY) */
    sound_quit();
    video_quit();
    SDL_Quit();
    return 0;
}
