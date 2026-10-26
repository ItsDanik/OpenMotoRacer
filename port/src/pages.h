#ifndef PAGES_H
#define PAGES_H

/* The front end's pages, by the game's numbers (the page table 0x4dbb38, NOTES §4.4.2) */

void page_main_start(void);     /* 3: the main menu, 0x4422c0 */
void page_main_frame(void);     /* 0x442790 */

void page_quit_frame(void);     /* 4: 0x40bc40 */

void page_title_start(void);    /* 14: the title, 0x4465f0 */
void page_title_frame(void);    /* 0x446b80 */
void page_title_end(void);      /* 0x446b40 */

void page_logo_start(void);     /* 25: the Delphine logo, 0x4537d0 */
void page_logo_frame(void);     /* 0x453c50 */
void page_logo_end(void);       /* 0x453bf0 */

#endif
