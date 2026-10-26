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

void page_load_champ_start(void);   /* 20: championship Load, 0x450600 */
void page_load_champ_frame(void);   /* 0x450800 */
void page_save_champ_start(void);   /* 21: championship Save, 0x450d20 */
void page_save_champ_frame(void);   /* 0x450f40 */

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
