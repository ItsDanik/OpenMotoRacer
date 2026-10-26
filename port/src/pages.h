#ifndef PAGES_H
#define PAGES_H

/* The front end's pages, by the game's numbers (the page table 0x4dbb38, NOTES §4.4.2) */

void page_main_start(void);     /* 3: the main menu, 0x4422c0 */
void page_main_frame(void);     /* 0x442790 */

void page_quit_frame(void);     /* 4: 0x40bc40 */

#endif
