#!/bin/sh
# Builds tools/physcheck_cd.c (with port/src/bikephys.c, the CD's behaviour on) and replays
# MOTO.EXE's recordings through it (build/phys_cd/*.bin, made with tools/win/stepwatch.c).
#   tools/physcheck_cd.sh [REC...]      Output in build/physcheck_cd/.
set -e
cd "$(dirname "$0")/.."
mkdir -p build/physcheck_cd
cc -std=c11 -O2 -Wall -ffp-contract=off -fno-fast-math -Iport/src -o build/physcheck_cd/physcheck_cd \
    tools/physcheck_cd.c -lm
build/physcheck_cd/physcheck_cd ${@:-build/phys_cd/*.bin} | tee build/physcheck_cd/report.txt
