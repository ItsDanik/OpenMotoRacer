#ifndef GENERIC_H
#define GENERIC_H

/* The .GEN scripts: blocks of text shown one after another (NOTES §4.8.4.3), and the victory
   sequence that plays a list of them over the winner's lap (NOTES §4.17). */

void credits_start(int brief);  /* 0x48aca0: the credits, the short list or the long one */
void credits_frame(void);       /* 0x48acd0: once a drawn frame, while it runs */
void credits_stop(void);        /* 0x48b070 */
int credits_running(void);      /* 0x4f15f4 */

#endif
