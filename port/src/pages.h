#ifndef PAGES_H
#define PAGES_H

/* The front end's pages, by the game's numbers (the page table 0x4dbb38, NOTES §4.4.2) */

void page_load_frame(void);     /* 1: the loading screen, 0x40b3f0 */

void page_main_start(void);     /* 3: the main menu, 0x4422c0 */
void page_main_frame(void);     /* 0x442790 */

void page_quit_frame(void);     /* 4: 0x40bc40 */

void page_mode_start(void);     /* 5 and 26: SELECT MODE, 0x447250 */
void page_mode_frame(void);     /* 0x447710 */

void page_track_start(void);    /* 6: SELECT RACE / CHAMPIONSHIP, 0x447a30 */
void page_track_frame(void);    /* 0x448110 */
void page_track_end(void);      /* 0x4480d0 */

void page_top5_start(void);     /* 9: TOP 5, 0x44cfd0 */
void page_top5_frame(void);     /* 0x44d540 */
void page_top5_end(void);       /* 0x44d510 */

void page_bike_start(void);     /* 10: CHOOSE BIKE, 0x44e480 */
void page_bike_frame(void);     /* 0x44ed30 */
void page_bike_end(void);       /* 0x44ecf0 */

void page_unlock_start(void);   /* 16, 17, 18: 0x44ffd0, 0x450290, 0x450550 */
void page_unlock_end(void);     /* 0x450050 */
void page_reverse_frame(void);  /* 16: the Reverse tracks unlocked, 0x44fdb0 */
void page_pocket_frame(void);   /* 17: the Pocket bikes, 0x450080 */
void page_won_frame(void);      /* 18: the championship won, 0x450340 */
void page_podium_start(void);   /* 24: the podium, 0x452240 */
void page_podium_frame(void);   /* 0x452f80 */
void page_podium_end(void);     /* 0x452eb0 */
void podium_data(void);         /* 0x452090, when the race ends */

void page_load_champ_start(void);   /* 20: championship Load, 0x450600 */
void page_load_champ_frame(void);   /* 0x450800 */
void page_save_champ_start(void);   /* 21: championship Save, 0x450d20 */
void page_save_champ_frame(void);   /* 0x450f40 */

void page_options_start(void);  /* 11: OPTIONS, 0x443000 */
void page_options_frame(void);  /* 0x443200 */
void page_settings_start(void); /* 12: SETTINGS (the controls), 0x444b10 */
void page_settings_frame(void); /* 0x444f30 */
void page_diag_start(void);     /* 13: DIAGNOSTIC, 0x4458a0 */
void page_diag_frame(void);     /* 0x4459d0 */
void page_datadisk_start(void); /* 23: DATA DISK, 0x451d10 */
void page_datadisk_frame(void); /* 0x4514e0 */
void page_datadisk_end(void);   /* 0x451fc0 */

void page_title_start(void);    /* 14: the title, 0x4465f0 */
void page_title_frame(void);    /* 0x446b80 */
void page_title_end(void);      /* 0x446b40 */

void page_name_start(void);     /* 15: the name entry, 0x487070 */
void page_name_frame(void);     /* 0x487bb0 */
void page_name_end(void);       /* 0x488790 */

void page_logo_start(void);     /* 25: the Delphine logo, 0x4537d0 */
void page_logo_frame(void);     /* 0x453c50 */
void page_logo_end(void);       /* 0x453bf0 */

#endif
