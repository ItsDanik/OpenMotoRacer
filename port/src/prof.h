/* Marks through a picture for the profiler (prof.c), nothing unless built with -DOMR_PROF=ON. */
#ifndef PROF_H
#define PROF_H

enum { MARK_TOP, MARK_STEPS, MARK_SCENE, MARK_RENDER, MARK_2D, MARK_PRESENT, MARK_WAIT, PROF_MARKS };

#ifdef OMR_PROF
void prof_mark(int i);
#else
#define prof_mark(i) ((void)0)
#endif

#endif
