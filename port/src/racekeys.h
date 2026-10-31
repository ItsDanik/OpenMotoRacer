#ifndef RACEKEYS_H
#define RACEKEYS_H

/* The race's small keys from 0x431250 that have something of their own to show (NOTES
   §4.20 §2.6, §5b). The screen's own switches and the column's are elsewhere. */

void racekeys_volume(int up, int shift, int ctrl);      /* 0x409720: * and / */
void racekeys_info(void);                               /* 0x484380: Shift+F1 */
void racekeys_small_fonts(void);                        /* 0x4866d0: Ctrl+F */
void racekeys_new_screen(void);                         /* 0x4592a0's part for the race */

#endif
