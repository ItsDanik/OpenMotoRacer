/* The game's settings (NOTES §3.4, §4.14.5): Data\Config.Spt read in the order of
   0x42ae30, and written in the same order when the program ends, as 0x42c4c0 and the .BPT
   rewrite after it do. */
#include "config.h"
#include "choices.h"
#include "saves.h"
#include "script.h"
#include "sound.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char g_player_name[16];
int g_detail = 4;
int g_race_screen = 2;
int g_mode169;
int g_input_device = 1;
int g_joy_type = 2;
int g_wave16 = 1, g_stereo = 1, g_replay_rate = 22050;
int g_volume[3] = { 32, 32, 32 };
int g_enum_session = 10;
int g_binding[ACTIONS][2];

static int g_cfg[CFG_COUNT];
static int g_cfg_read;          /* 0x4e716c: the file was read, so it may be written */

/* 0x430f20: the bindings before the file's */
static void default_bindings(void)
{
    static const int key[ACTIONS] = { 0x26, 0x28, 0x25, 0x27, 'Z', 'A', 0x20 };
    static const int btn[ACTIONS] = { 0, 1, -2, -3, 2, 3, -1 };
    int i;
    for (i = 0; i < ACTIONS; i++)
        g_binding[i][0] = key[i], g_binding[i][1] = btn[i];
}

void config_load(void)
{
    Script s;
    FILE *f;
    int i, own = 0;
    default_bindings();
    /* the player's own copy first (written by the port), else the game's */
    if ((f = save_open_own("CONFIG.BPT", "rb"))) {
        long len;
        unsigned char *d = 0;
        if (fseek(f, 0, SEEK_END) == 0 && (len = ftell(f)) > 0 && fseek(f, 0, SEEK_SET) == 0
            && (d = malloc((size_t)len)) && fread(d, 1, (size_t)len, f) == (size_t)len) {
            script_open_mem(&s, d, len);
            own = 1;
        } else
            free(d);
        fclose(f);
    }
    if (!own && !script_open(&s, "Data\\Config.Spt"))
        return;
    script_str(&s, g_player_name, sizeof g_player_name);        /* 0x42af0d: DefName */
    g_player_name[8] = 0;                                       /* (8 characters kept) */
    for (i = 0; i < CFG_COUNT; i++)
        g_cfg[i] = script_int(&s);
    if (s.bad) {
        script_close(&s);
        return;
    }
    script_close(&s);
    g_cfg_read = 1;
    g_detail = g_cfg[CFG_DETAIL];
    if (g_detail < 0 || g_detail > 4)
        g_detail = 4;
    g_difficulty = g_cfg[CFG_DIFFICULTY];
    if (g_difficulty < 0 || g_difficulty > 2)
        g_difficulty = 1;
    g_opponents = g_cfg[CFG_COMPUTER_CUT] & 1;
    g_race_screen = g_cfg[CFG_SCREEN_MODE];     /* (anything else: 640x480) */
    if (g_race_screen < 0 || g_race_screen > 2)
        g_race_screen = 2;
    g_mode169 = g_cfg[CFG_MODE_169] & 1;
    g_input_device = g_cfg[CFG_INPUT_DEVICE];
    g_joy_type = g_cfg[CFG_JOY_TYPE];
    g_wave16 = g_cfg[CFG_WAVE16];
    g_stereo = g_cfg[CFG_STEREO];
    g_replay_rate = g_cfg[CFG_REPLAY_RATE];
    for (i = 0; i < 3; i++) {
        int v = g_cfg[CFG_MASTER_VOLUME + i];
        g_volume[i] = v < 0 ? 0 : v > 32 ? 32 : v;
    }
    /* the game's volumes were the Windows mixer's lines, set from these numbers; the
       GOG copy's master is 0, which Windows' mixer never got to (its own stays as it
       is). The port has no system mixer: its master starts full unless it saved one. */
    if (!own)
        g_volume[0] = 32;
    for (i = 0; i < ACTIONS; i++) {
        g_binding[i][0] = g_cfg[CFG_ACTION0_KEY + 2 * i];
        g_binding[i][1] = g_cfg[CFG_ACTION0_BTN + 2 * i];
    }
    g_enum_session = g_cfg[CFG_ENUM_SESSION];
    sound_set_switches(g_cfg[CFG_SOUND], g_cfg[CFG_VOICES], g_cfg[CFG_AUDIO]);
    sound_set_volumes(g_volume);
}

void config_save(void)
{
    ScriptOut o;
    FILE *f;
    int i, on[3];
    if (!g_cfg_read)
        return;
    g_cfg[CFG_SCREEN_MODE] = g_race_screen;
    g_cfg[CFG_MODE_169] = g_mode169;
    g_cfg[CFG_DETAIL] = g_detail;
    g_cfg[CFG_DIFFICULTY] = g_difficulty;
    g_cfg[CFG_COMPUTER_CUT] = g_opponents;
    sound_get_switches(&on[0], &on[1], &on[2]);
    g_cfg[CFG_SOUND] = on[0];
    g_cfg[CFG_VOICES] = on[1];
    g_cfg[CFG_AUDIO] = on[2];
    g_cfg[CFG_WAVE16] = g_wave16;
    g_cfg[CFG_STEREO] = g_stereo;
    g_cfg[CFG_REPLAY_RATE] = g_replay_rate;
    for (i = 0; i < 3; i++)
        g_cfg[CFG_MASTER_VOLUME + i] = g_volume[i];
    g_cfg[CFG_INPUT_DEVICE] = g_input_device;
    g_cfg[CFG_JOY_TYPE] = g_joy_type;
    for (i = 0; i < ACTIONS; i++) {
        g_cfg[CFG_ACTION0_KEY + 2 * i] = g_binding[i][0];
        g_cfg[CFG_ACTION0_BTN + 2 * i] = g_binding[i][1];
    }
    g_cfg[CFG_ENUM_SESSION] = g_enum_session;
    script_out_init(&o);
    script_out_str(&o, g_player_name);
    for (i = 0; i < CFG_COUNT; i++)
        script_out_int(&o, g_cfg[i]);
    if (!o.bad && (f = save_open("CONFIG.BPT", "wb"))) {
        if (fwrite(o.d, 1, (size_t)o.len, f) != (size_t)o.len)
            fprintf(stderr, "openmotoracer: Cannot save config file.\n");
        fclose(f);
    } else
        fprintf(stderr, "openmotoracer: Cannot save config file.\n");
    script_out_free(&o);
}
