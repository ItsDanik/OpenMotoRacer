#!/usr/bin/env python3
"""Proves that tools/physcheck_cd exercises each of the CD's differences: builds the check with
one difference turned back to GOG's way at a time and counts the steps of MOTO.EXE's recordings
that then differ. A difference that makes steps fail is seen in the recordings; one that makes
none is verified by reading only.

    tools/physcheck_cd_off.py [REC...]      (default build/phys_cd/*.bin; run from the top)
"""
import glob
import os
import re
import shutil
import subprocess
import sys

OFF = [
    ("1  lean: dirt, the computer's rule", "b->driven && b->human && d && !g_phys_cd", "b->driven && b->human && d"),
    ("1  lean: road, the computer's rule", "W.replay == 2 || g_phys_cd)", "W.replay == 2)"),
    ("2  steering rate limited", "R rmax = g_phys_cd ? (R)carac(b, C_RMAA) : HUGE_VAL;", "R rmax = HUGE_VAL;"),
    ("3  dirt steering not clamped", "k->type == 1 && !g_phys_cd", "k->type == 1"),
    ("4  ground covered in a turn", "if (g_phys_cd)                  /* /0x41cda5", "if (0)                  /* /0x41cda5"),
    ("5  slide costs speed", "if (g_phys_cd && fabs((R)nw - lat) > 0.1)", "if (0)"),
    ("15 engine brake a gear off", "(g_phys_cd ? k_engine_brake_cd : k_engine_brake)", "(k_engine_brake)"),
    ("16 no box under the brake", "if (g_phys_cd && (keys & 2) && bits(b->speed) > 0x40400000)", "if (0)"),
    ("17 crash bounce", "(g_phys_cd ? 0.1 : 0.01)", "0.01"),
    ("18 slope always, by length", "if (g_phys_cd) {                /* /0x41f44c", "if (0) {                /* /0x41f44c"),
    ("19 spring: no floor on dt", "bits(d) < 0x40400000 && !g_phys_cd", "bits(d) < 0x40400000"),
    ("+  turn by the extra angle", "if (g_phys_cd) {                    /* /0x487135", "if (0) {                    /* /0x487135"),
    ("+  brake+steer only when sliding", "(b->sliding || !g_phys_cd)", "1"),
    ("+  radius: none on the CD", "if (!d || g_phys_cd)", "if (!d)"),
    ("+  mulvec's order", "float y = g_phys_cd ? (float)", "float y = 0 ? (float)"),
    ("+  lateral's sums", "if (g_phys_cd) {                            /* /0x41c650", "if (0) {                            /* /0x41c650"),
    ("+  forward's sums", "if (g_phys_cd) {                            /* /0x41c0f0", "if (0) {                            /* /0x41c0f0"),
    ("+  set_lateral's sums", "if (g_phys_cd) {                            /* /0x41c740", "if (0) {                            /* /0x41c740"),
    ("+  pitch's stored products", "if (g_phys_cd) {                    /* /0x47e073", "if (0) {                    /* /0x47e073"),
    ("+  tumble's matrix sums", "if (g_phys_cd) {                    /* /0x489a25", "if (0) {                    /* /0x489a25"),
    ("+  rear point through a float", "if (g_phys_cd) {                /* /0x48b03f", "if (0) {                /* /0x48b03f"),
    ("+  grip's stored lateral", "if (g_phys_cd) {                /* /0x41e2d5", "if (0) {                /* /0x41e2d5"),
]


def run(bin_, recs):
    out = subprocess.run([bin_] + recs, capture_output=True, text=True, errors="replace").stdout
    total = good = 0
    for m in re.finditer(r"(\d+) steps, \d+ moving, all the fields below at once: (\d+)", out):
        total += int(m.group(1))
        good += int(m.group(2))
    return total, good


def main():
    recs = sys.argv[1:] or sorted(glob.glob("build/phys_cd/*.bin"))
    mut = "build/physcheck_cd/off"
    shutil.rmtree(mut, ignore_errors=True)
    shutil.copytree("port/src", mut + "/port/src")
    os.makedirs(mut + "/tools")
    shutil.copy("tools/physcheck_cd.c", mut + "/tools/")
    src = open("port/src/bikephys.c").read()

    def build(text):
        open(mut + "/port/src/bikephys.c", "w").write(text)
        subprocess.run(["cc", "-std=c11", "-O2", "-w", "-ffp-contract=off", "-fno-fast-math",
                        "-I" + mut + "/port/src", "-o", mut + "/check",
                        mut + "/tools/physcheck_cd.c", "-lm"], check=True)
        return mut + "/check"

    total, base = run(build(src), recs)
    print("%d steps; with everything on, all the fields right on %d" % (total, base))
    for name, old, new in OFF:
        if old not in src:
            print("%-36s (no such text in bikephys.c: %s)" % (name, old))
            continue
        _, good = run(build(src.replace(old, new)), recs)
        print("%-36s turned off: %6d steps fail" % (name, base - good))


main()
