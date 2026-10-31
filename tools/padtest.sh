#!/bin/sh
# Shows what a gamepad's triggers and buttons send; a copy goes to build/logs/padtest.txt.
cd "$(dirname "$0")/.."
mkdir -p build/logs
cc -O1 -o build/padtest tools/padtest.c $(pkg-config --cflags --libs sdl2) || exit 1
build/padtest "$@" | tee build/logs/padtest.txt
