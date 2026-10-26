#ifndef SCORES_H
#define SCORES_H

/* The hi-score table, Data\bestscor.dat (NOTES §4.12.5): "00.1", then a block for every
   track and set of options: a 0x20-byte key (the track's name, + "POC" with Pocket, + "REV"
   reversed, + "TRA<laps>" in practice), the best lap's record and the 8 races' records. */
#define SCORE_SIZE 0xf9
#define SCORE_ROWS 8

typedef struct {
    unsigned char b[SCORE_SIZE];
} Score;

/* a record's fields: the name (+9), the race's time (+0xf1) and the row's own lap time
   (+0xf5), ms; 0x7f7f7f7f = empty */
const char *score_name(const Score *s);
unsigned score_time(const Score *s);
unsigned score_lap(const Score *s);
#define SCORE_EMPTY 0x7f7f7f7fu

extern Score g_scores[SCORE_ROWS];  /* 0x5cf500 */
extern Score g_best_lap;            /* 0x540230 */

void scores_key(char *key, int size);   /* the current choices' key */
void scores_load(void);                 /* 0x464e80: the races (all 0x7f when none) */
int best_lap_load(Score *out);          /* 0x4656c0: 1 if the key's block was found */

#endif
