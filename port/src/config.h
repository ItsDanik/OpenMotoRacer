#ifndef CONFIG_H
#define CONFIG_H

/* The game's settings, Data\Config.Spt (CONFIG.BPT), read in the order of 0x42ae30 */
extern char g_player_name[16];  /* 0x5161b0: DefName, then the name typed on page 15 */

void config_load(void);

#endif
