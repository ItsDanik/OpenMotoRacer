/* The game's settings (NOTES §3.4), Data\Config.Spt read in the order of 0x42ae30: so far
   as far as the opponents' count. */
#include "config.h"
#include "choices.h"
#include "script.h"
#include <string.h>

char g_player_name[16];

void config_load(void)
{
    Script s;
    int i;
    if (!script_open(&s, "Data\\Config.Spt"))
        return;
    script_str(&s, g_player_name, sizeof g_player_name);        /* 0x42af0d: DefName */
    g_player_name[8] = 0;                                       /* (8 characters kept) */
    /* DrawLand, ScreenMode, FullScreen, Mode_169, MapMode, DisplaySignal, CurrentDetail */
    for (i = 0; i < 7; i++)
        script_int(&s);
    g_difficulty = script_int(&s);      /* CurrentDifficulty */
    g_opponents = script_int(&s);       /* NbComputerCut */
    script_close(&s);
}
