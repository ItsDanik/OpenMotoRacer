#!/bin/sh
# Builds tools/physcheck.c (with port/src/bikephys.c) and replays the recordings
# build/phys/w1..w4.bin through it. Output in build/physcheck/.
set -e
cd "$(dirname "$0")/.."
mkdir -p build/physcheck
python3 -I tools/physprep.py build/bkf SPEED 1 build/physcheck/carac.bin >/dev/null
cc -std=c11 -O2 -Wall -ffp-contract=off -fno-fast-math -Iport/src -o build/physcheck/physcheck \
    tools/physcheck.c -lm
build/physcheck/physcheck build/physcheck/carac.bin build/phys/costab.bin \
    build/phys/sections_speedbay.bin build/phys/w1.bin build/phys/w2.bin build/phys/w3.bin \
    build/phys/w4.bin | tee build/physcheck/report.txt
