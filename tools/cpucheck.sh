#!/bin/sh
# Builds tools/cpucheck.c (with port/src/bikephys.c and cpu.c) and replays the recordings of
# the computer riders (build/cpucap/NAME.bin with NAME_carac.bin and NAME_sec.bin, made with
# tools/win/cpuwatch.c) through it. Output in build/cpucheck/.
set -e
cd "$(dirname "$0")/.."
mkdir -p build/cpucheck
cc -std=c11 -O2 -Wall -ffp-contract=off -fno-fast-math -Iport/src -o build/cpucheck/cpucheck \
    tools/cpucheck.c port/src/bikephys.c port/src/cpu.c -lm
for rec in ${@:-build/cpucap/demo*.bin}; do
    case $rec in *_carac.bin|*_sec.bin) continue ;; esac
    b=${rec%.bin}
    build/cpucheck/cpucheck ${b}_carac.bin build/phys/costab.bin ${b}_sec.bin $rec
done | tee build/cpucheck/report.txt
