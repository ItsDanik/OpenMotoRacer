/* The game's settings (NOTES §3.4): only the player's name is read so far. */
#include "config.h"
#include "script.h"
#include <string.h>

char g_player_name[16];

void config_load(void)
{
    Script s;
    if (!script_open(&s, "Data\\Config.Spt"))
        return;
    script_str(&s, g_player_name, sizeof g_player_name);        /* 0x42af0d: DefName */
    script_close(&s);
}
