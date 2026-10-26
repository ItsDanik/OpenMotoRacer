#ifndef EXE_H
#define EXE_H

/* The player's own program, laid out at its addresses: the game's fixed tables are read
   from it, never copied into the port. 0 and a message if it is missing or unknown. */
int exe_load(char *err, int errsize);

/* the bytes at the program's address va (len of them inside the image), else 0 */
const unsigned char *exe_at(unsigned va, unsigned len);
unsigned exe_u32(unsigned va);
const char *exe_str(unsigned va);       /* a zero-ended string there, else "" */

#endif
