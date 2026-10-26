#ifndef CONFIG_H
#define CONFIG_H

/* The game's settings, Data\Config.Spt (CONFIG.BPT): read in the order of 0x42ae30 at the
   start, written back in that order when the program ends (0x42c4c0, on WM_DESTROY). The
   port writes it to the player's folder (saves.h) and reads it from there first. */

enum {                          /* the file's numbers, in their order */
    CFG_DRAW_LAND, CFG_SCREEN_MODE, CFG_FULLSCREEN, CFG_MODE_169, CFG_MAP_MODE,
    CFG_DISPLAY_SIGNAL, CFG_DETAIL, CFG_DIFFICULTY, CFG_COMPUTER_CUT, CFG_AMBIANT,
    CFG_MOTO, CFG_SOUND, CFG_VOICES, CFG_AUDIO, CFG_WAVE16, CFG_STEREO, CFG_REPLAY_RATE,
    CFG_MASTER_VOLUME, CFG_CD_VOLUME, CFG_WAVE_VOLUME, CFG_INPUT_DEVICE, CFG_JOY_TYPE,
    CFG_COUNTER_TYPE, CFG_SHOW_COUNTER, CFG_D3D_EMULATION, CFG_DETECT_D3D,
    CFG_LAST_LEVEL, CFG_LAST_MAX_TOUR, CFG_LAST_CUR_MODE, CFG_LAST_NB_COMP, CFG_LAST_REV,
    CFG_LAST_POKT, CFG_LAST_MOTO,
    CFG_ACTION0_KEY, CFG_ACTION0_BTN,   /* ... Action6Btn: 14 numbers */
    CFG_SMALL_FONTS = CFG_ACTION0_KEY + 14, CFG_ENUM_SESSION, CFG_SPEED_UNIT, CFG_COUNT
};

#define ACTIONS 7               /* accelerate, brake, left, right, gear up, gear down, turbo */

extern char g_player_name[16];  /* 0x5161b0: DefName, then the name typed on page 15 */
extern int g_detail;            /* 0x4dc098: CurrentDetail, the row of Level.Spt's detail
                                   table (0..4; 4 in the GOG copy) */
extern int g_race_screen;       /* 0x504bc4: the race's screen (0 320x240, 1 512x384,
                                   2 640x480); the menus are always 640x480 */
extern int g_mode169;           /* 0x504e80: the race in 16:9 */
extern int g_input_device;      /* 0x4dc11c: 1 keyboard, 4 joypad, 2 joystick */
extern int g_joy_type;          /* 0x4dc120 */
extern int g_wave16, g_stereo, g_replay_rate;   /* 0x4db670, 0x4db674, 0x504568 */
extern int g_volume[3];         /* the mixer's lines 0 master, 1 CD, 2 wave: 0..32 */
extern int g_show_counter, g_counter_type;   /* 0x4f0dc8, 0x4f0dcc: the dials (1 dials, 2 digits) */
extern int g_speed_unit;        /* 0x4f0dd0: 1 mph */
extern int g_enum_session;      /* 0x4dc044: the Link sessions' refresh, s (0 manual) */
extern int g_binding[ACTIONS][2];   /* 0x53eae0: key (Windows code, -1 none), button
                                       (n, -1 none, -2..-5 left, right, up, down) */

void config_load(void);
void config_save(void);         /* when the program ends */
/* the port's own settings (openmotoracer.ini in the home folder, never the game's) */
extern int g_smooth;            /* pictures between the race's steps (F10) */

#endif
