#ifndef SCRIPT_H
#define SCRIPT_H

/* The game's scripts (.Spt), as the player has them: .BPT, the tokens of the text script
   enciphered (NOTES §3.4). The reader knows what comes next; so does the caller. */
typedef struct {
    unsigned char *d;
    long len, p;
    unsigned seed;
    int bad;            /* read past the end */
} Script;

/* the game's name ("Data\\Fonts.Spt"): opened as .BPT (Script_Open 0x4242a0) */
int script_open(Script *s, const char *spt);
void script_close(Script *s);
int script_int(Script *s);                          /* 0x425080 */
float script_float(Script *s);                      /* 0x424c00 */
void script_str(Script *s, char *out, int size);    /* 0x424e60 */
int script_at_end(const Script *s);

#endif
