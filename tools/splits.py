#!/usr/bin/env python3
"""Routines Ghidra cut in two: MSVC's exception-handling routines set up their frame, end
the prologue with a `nop`, and something points at the body after it, so a second function
starts there (no callers) and the first is only the prologue. Prints `entry body size`
triples for JoinFunc.java.

    python3 -I tools/splits.py build/mr_flat.bin build/decomp/INDEX.txt > build/joins.txt
"""
import sys
img = open(sys.argv[1], "rb").read()
rows = []
for l in open(sys.argv[2]):
    p = l.split()
    rows.append((int(p[0], 16), int(p[1]), l.split("by [")[1].split("]")[0].strip() == ""))
starts = {a: (n, nc) for a, n, nc in rows}
for a, n, _ in rows:
    e = a + n
    if a < 0x4b0000 and e in starts and starts[e][1] and img[e - 1] == 0x90 and n < 0x60:
        print("0x%x 0x%x %d" % (a, e, starts[e][0]))
