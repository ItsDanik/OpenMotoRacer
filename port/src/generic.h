#ifndef GENERIC_H
#define GENERIC_H

/* The .GEN scripts: blocks of text shown one after another (NOTES §4.8.4.3), and the victory
   sequence that plays a list of them over the winner's lap (NOTES §4.17). */

void credits_start(int brief);  /* 0x48aca0: the credits, the short list or the long one */
void credits_frame(void);       /* 0x48acd0: once a drawn frame, while it runs */
void credits_stop(void);        /* 0x48b070 */
int credits_running(void);      /* 0x4f15f4 */

/* F1 (0x48b120, 0x48b180): Help.gen, its blocks one under the other as a long list, moved so
   that the block asked for is at the top of the screen */
int help_open(void);            /* 0 if the file does not load */
void help_close(void);
void help_page(int page);       /* 0x48b190 */
int help_pages(void);           /* +0x18 */
int help_at(void);              /* +0x20 */
void help_draw(void);           /* 0x43deb0 */

#endif
