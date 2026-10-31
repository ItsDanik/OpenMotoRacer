/* mister.c - see mister.h */
#include "mister.h"
#include "pad.h"
#include "config.h"
#include "dialog.h"
#include "input.h"
#include "joy.h"
#include "menu.h"
#include "race.h"
#include "video.h"
#include <SDL.h>
#include <mister_hybrid.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define EXIT_TO_MENU  0
#define EXIT_RESTART  42    /* danik_hybrid_launch.sh starts the game again: the core's list */
#define EXIT_RELOADED 43    /* the same, once the core has settled after being loaded again */

/* 1 if a picture is drawn a field ahead (MH_PresentAtField): measured, core's CLAUDE.md */
#define DRAW_AHEAD 1

int g_mister_res = MR_320x240;
int g_half_rows;
int g_hud_top, g_hud_bottom;

static int g_listed;
static int g_limit30;
static int g_cheats;

int mister_on(void)
{
    static int tried;
    if (!tried) {                       /* (once: without the core it would look each time) */
        tried = 1;
        return MH_Open();
    }
    return MH_IsOpen();
}

/* ------------------------------------------------------------------ the game's files */

/* every file the game opens that both copies have under data/; the player copies folders whole
   (hybrid/CLAUDE.md), so one missing means a copy cut short */
#define DATA_FILES(p) \
    p "CHAMPA.BPT", p "CHAMPB.BPT", p "CHAMPC.BPT", p "CONFIG.BPT", p "DEMO.BKF", p "DESIGN.BKF", \
    p "FONTES.BKF", p "FONTS.BPT", p "GENERIC.BKF", p "ILE01.BKF", p "ILE02.BKF", p "ILE03.BKF", \
    p "ILE04.BKF", p "ILE05.BKF", p "ILE06.BKF", p "ILE08.BKF", p "ILE09.BKF", p "KTMCROSS.BKF", \
    p "KTMPKT.BKF", p "LIST.BPT", p "MENU.BKF", p "MOTOKTM.BPT", p "MOTOSPD.BPT", p "PAGES.BKF", \
    p "PKTKTM.BPT", p "PKTSPEED.BPT", p "PODIUM.BKF", p "SFX.BKF", p "SPEED.BKF", p "SPEEDPKT.BKF"

static const char *const g_gog_files[] = { "game.EXE", DATA_FILES("data/"), NULL };
static const char *const g_cd_files[] = { "DISK1/data.z", DATA_FILES("DATA/"), NULL };

#define NO_GAME "Moto Racer was not found. Copy the game's whole folder as it is, GOG's " \
    "(game.EXE, data, 02.wav...) or the CD's (DISK1, DATA, its .cue and .bin), to %s/Moto Racer/"

static int has(const char *dir, const char *file)
{
    mh_game g;
    const char *const files[2] = { file, NULL };
    memset(&g, 0, sizeof g);
    g.files = files;
    return MH_GameFound(&g, dir);
}

int mister_start(char *out, unsigned size)
{
    static const char *const dirs[] = { "Moto Racer", "Moto Racer CD", "." };
    static mh_game game = { "MotoRacer", "Moto Racer", g_gog_files, NULL, NO_GAME };
    const char *dir = "Moto Racer";
    unsigned i;
    if (!mister_on())
        return 0;
    /* GOG's first when both are there; the copy is known by what is in its folder */
    for (i = 0; i < sizeof dirs / sizeof dirs[0]; i++)
        if (has(dirs[i], "game.EXE") || has(dirs[i], "DISK1/data.z")) {
            dir = dirs[i];
            break;
        }
    game.files = has(dir, "DISK1/data.z") && !has(dir, "game.EXE") ? g_cd_files : g_gog_files;
    if (MH_PickGame(&game, 1, dir, NO_GAME, &g_listed) < 0)
        return 0;
    snprintf(out, size, "%s", dir);
    return 1;
}

void mister_fatal(const char *text)
{
    if (mister_on())
        MH_UI_Message("OpenMotoRacer", text, MH_UI_WAIT | MH_UI_ERROR);
}

int mister_exit_code(void)
{
    if (MH_CoreReloaded())
        return EXIT_RELOADED;
    if (g_listed && MH_Open())
        return EXIT_RESTART;
    return EXIT_TO_MENU;
}

/* ------------------------------------------------------------------ the OSD */

/* an option takes effect when it changes, and at the start: the keyboard's own keys for the
   same thing (F2, F3, F10, Options) hold until the OSD's line is next moved */
#define FOLLOW(name, value, then) do { \
        static int was_##name = -1; \
        int v_##name = (value); \
        if (v_##name != was_##name) { \
            was_##name = v_##name; \
            then; \
        } \
    } while (0)

void mister_frame(void)
{
    uint64_t s;
    int top, bottom, k;
    if (!MH_IsOpen())
        return;
    s = MH_OSDStatus();
    FOLLOW(res, MH_OSD_GAME_BITS(s, 24, 2) % MR_COUNT, race_set_resolution(v_res));
    g_limit30 = MH_OSD_GAME_BITS(s, 26, 1);
    FOLLOW(smooth, !MH_OSD_GAME_BITS(s, 27, 1), g_smooth = v_smooth);
    FOLLOW(analog, !MH_OSD_GAME_BITS(s, 28, 1), g_analog = v_analog);
    FOLLOW(rumble, !MH_OSD_GAME_BITS(s, 29, 1), g_rumble = v_rumble);
    /* the game's three cheats, as if typed for its name (they hold after one was typed too,
       until the line is next moved) */
    FOLLOW(all, MH_OSD_GAME_BITS(s, 32, 1), if (v_all || was_all != -1) g_island_unlocked = v_all);
    FOLLOW(rev, MH_OSD_GAME_BITS(s, 33, 1), if (v_rev || was_rev != -1) g_reverse_unlocked2 = v_rev);
    FOLLOW(poc, MH_OSD_GAME_BITS(s, 34, 1), if (v_poc || was_poc != -1) g_pocket_unlocked2 = v_poc);
    k = MH_OSD_GAME_BITS(s, 35, 1) ? MISTER_NO_TIME_LIMIT : 0;
    g_cheats = k | (MH_OSD_GAME_BITS(s, 36, 1) ? MISTER_SLOW_RIVALS : 0);
    /* the HUD margins are a 240-line screen's; 640x240 draws in 480 rows */
    MH_HudMargins(&top, &bottom);
    k = g_scr_h >= 480 ? 2 : 1;
    g_hud_top = top * k;
    g_hud_bottom = bottom * k;
    /* the menus (640x480, interlaced on a 15 kHz screen) take a picture at every field, not a
       race: hybrid/CLAUDE.md */
    MH_SetFieldRate(!g_racing && MH_Height() >= 400);
}

int mister_cheat(int which)
{
    return (g_cheats & which) != 0;
}

/* ------------------------------------------------------------------ the pictures' time */

static uint32_t g_field;
static int g_field_ok;

void mister_wait(void)
{
    if (!MH_IsOpen())
        return;
    if (!g_field_ok) {
        g_field = MH_FieldCounter();
        g_field_ok = 1;
    }
#if DRAW_AHEAD
    if (!MH_Interlaced() || MH_FieldRate()) {
        uint32_t target = g_field + (g_limit30 && g_racing ? 2 : 1), now;
        MH_WaitField(target - 3);
        now = MH_FieldCounter();
        if ((int32_t)(now + 1 - target) > 0)
            target = now + 1;
        MH_PresentAtField(target);
        g_field = target;
        return;
    }
#endif
    MH_WaitFrame(g_field);
    /* Limit to 30 FPS: a race's picture lasts two fields (an interlaced one does anyway) */
    if (g_limit30 && g_racing && !MH_Interlaced()
        && MH_FieldCounter() - g_field < 2 && MH_IsOpen())
        MH_WaitField(g_field + 1);
    g_field = MH_FieldCounter();
}

int mister_clock(unsigned *ms)
{
    static uint32_t first;
    static int started;
    if (!MH_IsOpen())
        return 0;
    if (!started) {
        first = MH_FieldCounter();
        started = 1;
    }
    /* 59.64 fields a second (hybrid_host.sv) */
    *ms = (unsigned)((uint64_t)(MH_FieldCounter() - first) * 100000u / 5964u);
    return 1;
}

/* ------------------------------------------------------------------ the pad */

/* The core's J1 list, as SDL's buttons */
enum { B_ACCEL, B_BRAKE, B_TURBO, B_CAMERA, B_GEAR_DOWN, B_GEAR_UP, B_LOOK, B_PAUSE, B_OK, B_BACK,
       B_MENU_OK = 28, B_MENU_BACK = 29, B_L3 = 30, B_R3 = 31 };

void mister_default_bindings(void)
{
    /* accelerate, brake, the stick, gear up, gear down, turbo: PAD_* as mister_pad gives them */
    static const int btn[ACTIONS] = { PAD_A, PAD_B, -2, -3, PAD_RB, PAD_LB, PAD_X };
    int i;
    for (i = 0; i < ACTIONS; i++)
        g_binding[i][1] = btn[i];
}

static void tap(int vk)
{
    input_key(vk, 1);
    race_key(vk, 1);
    input_key(vk, 0);
    race_key(vk, 0);
}

void mister_pad(void)
{
    static SDL_Joystick *d;
    static unsigned last;
    static int looking;
    unsigned b = 0, pressed;
    int x, y, i, cross = 0, lt, rt, driving;
    Uint8 hat;
    uint32_t drop;
    if (!MH_IsOpen())
        return;
    if (!d && !(d = SDL_JoystickOpen(0)))
        return;
    SDL_JoystickUpdate();
    for (i = 0; i < 32; i++)
        if (SDL_JoystickGetButton(d, i))
            b |= 1u << i;
    pressed = b & ~last;
    last = b;
    x = SDL_JoystickGetAxis(d, 0);
    y = SDL_JoystickGetAxis(d, 1);
    lt = SDL_JoystickGetAxis(d, 4);
    rt = SDL_JoystickGetAxis(d, 5);
    /* Main_MiSTer presses the d-pad as well when the stick is pushed far: the stick is the
       stick here (it steers by degrees, and moves the pointer) */
    hat = SDL_JoystickGetHat(d, 0);
    drop = MH_StickDpad(x >> 8, y >> 8);
    if ((hat & SDL_HAT_UP) && !(drop & MH_JOY_UP))
        cross |= 1;
    if ((hat & SDL_HAT_DOWN) && !(drop & MH_JOY_DOWN))
        cross |= 2;
    if ((hat & SDL_HAT_LEFT) && !(drop & MH_JOY_LEFT))
        cross |= 4;
    if ((hat & SDL_HAT_RIGHT) && !(drop & MH_JOY_RIGHT))
        cross |= 8;
    driving = g_racing && !dialog_holds();
    {
        unsigned out = 0;
        if (b & 1u << B_PAUSE)
            out |= 1u << PAD_START;
        /* the settings page waiting for a button hears them as a race does */
        if (driving || g_pad_listen) {
            static const struct { int from, to; } map[] = {
                { B_ACCEL, PAD_A }, { B_BRAKE, PAD_B }, { B_TURBO, PAD_X }, { B_CAMERA, PAD_Y },
                { B_GEAR_DOWN, PAD_LB }, { B_GEAR_UP, PAD_RB },
            };
            for (i = 0; i < (int)(sizeof map / sizeof map[0]); i++)
                if (b & 1u << map[i].from)
                    out |= 1u << map[i].to;
            if (g_playback && (b & (1u << B_OK | 1u << B_MENU_OK)))
                out |= 1u << PAD_A;     /* a demo gives way */
        } else {
            /* the menus: Menu OK and Menu Back, whatever those buttons do in a race */
            if (b & (1u << B_OK | 1u << B_MENU_OK))
                out |= 1u << PAD_A;
            if (b & (1u << B_BACK | 1u << B_MENU_BACK))
                out |= 1u << PAD_B;
            if (b & 1u << B_LOOK)
                out |= 1u << PAD_BACK;
            /* a game action does not confirm: the triggers are no menu's */
            lt = rt = 0;
        }
        joy_fake_pad(x + 0x8000, y + 0x8000, out, lt < 0 ? 0 : lt, rt < 0 ? 0 : rt, (unsigned)cross);
    }
    if (driving) {
        int y_bound = 0;
        for (i = 0; i < ACTIONS; i++)
            y_bound |= g_binding[i][1] == PAD_Y;
        if ((pressed & 1u << B_CAMERA) && !y_bound)
            tap(0x75);                  /* F6: the chase camera or on board */
        if (pressed & 1u << B_L3)
            tap(0x76);                  /* F7: the dashboard */
        if (pressed & 1u << B_R3)
            tap(0x78);                  /* F9: the map */
    }
    /* the look back, as long as it is held */
    {
        int want = driving && (b & 1u << B_LOOK);
        if (want != looking) {
            looking = want;
            input_key(VK_TAB, want);
            race_key(VK_TAB, want);
        }
    }
}
