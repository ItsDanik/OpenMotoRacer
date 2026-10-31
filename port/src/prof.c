/* The sampling profiler, built with -DOMR_PROF=ON only: OMR_PROF=FILE, a thousand times a second the
   game thread's program counter and return address, from the 300th picture of a race on, written
   as pairs of little-endian words after a minute. tools/prof.py reads them. And each picture's marks
   (prof.h) in FILE.frames, tools/prof.py's second report. */
#define _GNU_SOURCE
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>
#include "prof.h"
#include "race.h"

#define MAX 60000
#define FRAMES 4000

static uint32_t (*g_s)[2];
static volatile int g_n;
static const char *g_file;
/* each picture: the wall clock at each mark, and the thread's own time from the first mark to the
   last but one (a picture that took long without using it was held up by something else) */
static float g_marks[FRAMES][PROF_MARKS + 1];
static int g_frames;
static struct timespec g_start;

static double since(clockid_t c, const struct timespec *from)
{
    struct timespec t;
    clock_gettime(c, &t);
    return (t.tv_sec - from->tv_sec) * 1e3 + (t.tv_nsec - from->tv_nsec) / 1e6;
}

void prof_mark(int i)
{
    static struct timespec cpu;
    if (g_frames >= FRAMES || !g_start.tv_sec)
        return;
    if (i == 0) {
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cpu);
        clock_gettime(CLOCK_MONOTONIC, &g_start);
    }
    g_marks[g_frames][i] = (float)since(CLOCK_MONOTONIC, &g_start);
    if (i == PROF_MARKS - 2)
        g_marks[g_frames][PROF_MARKS] = (float)since(CLOCK_THREAD_CPUTIME_ID, &cpu);
    if (i == PROF_MARKS - 1)
        g_frames++;
}

static void on_tick(int sig, siginfo_t *si, void *uc)
{
    const mcontext_t *m = &((const ucontext_t *)uc)->uc_mcontext;
    (void)sig, (void)si;
    if (g_n < MAX) {
        g_s[g_n][0] = (uint32_t)m->arm_pc;
        g_s[g_n][1] = (uint32_t)m->arm_lr;
        g_n++;
    }
}

static void write_out(void)
{
    FILE *f = fopen(g_file, "wb");
    if (f) {
        fwrite(g_s, sizeof g_s[0], (size_t)g_n, f);
        fclose(f);
    }
    {   /* where the shared libraries lie, for tools/prof.py */
        char cmd[300];
        snprintf(cmd, sizeof cmd, "cp /proc/%d/maps '%s.maps'", (int)getpid(), g_file);
        if (system(cmd)) {}
    }
    {   /* the pictures' marks */
        char name[300];
        FILE *m;
        snprintf(name, sizeof name, "%s.frames", g_file);
        if ((m = fopen(name, "w"))) {
            int i, k;
            for (i = 0; i < g_frames; i++) {
                for (k = 0; k <= PROF_MARKS; k++)
                    fprintf(m, "%.2f ", (double)g_marks[i][k]);
                fprintf(m, "\n");
            }
            fclose(m);
        }
    }
    fprintf(stderr, "prof: %d samples in %s\n", g_n, g_file);
}

void prof_frame(void)
{
    static int racing;
    struct sigaction sa;
    struct sigevent ev;
    struct itimerspec it;
    timer_t t;
    if (racing < 0) {
        if (g_n >= MAX && racing == -1) {
            racing = -2;
            write_out();
        }
        return;
    }
    if (!g_racing || ++racing != 300 || !(g_file = getenv("OMR_PROF")))
        return;
    racing = -1;
    clock_gettime(CLOCK_MONOTONIC, &g_start);
    g_s = malloc(sizeof g_s[0] * MAX);
    memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = on_tick;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGPROF, &sa, 0);
    memset(&ev, 0, sizeof ev);
    ev.sigev_notify = SIGEV_THREAD_ID;
    ev.sigev_signo = SIGPROF;
    ev._sigev_un._tid = (int)syscall(SYS_gettid);
    timer_create(CLOCK_MONOTONIC, &ev, &t);
    memset(&it, 0, sizeof it);
    it.it_interval.tv_nsec = it.it_value.tv_nsec = 1000000;
    timer_settime(t, 0, &it, 0);
}
