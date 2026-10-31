#!/bin/bash
# The port for Windows x86-64, built here with MinGW-w64 (no release, no packaging):
#   tools/windows.sh            build/win/openmotoracer.exe with SDL2.dll beside it
#   tools/windows.sh check      and every test picture, replay and recording on it under wine,
#                               against the PC's (build/shots.txt, the native tools/replaycheck.sh)
# SDL2 is SDL's own Windows package for MinGW (headers, import library, SDL2.dll): the one ReNFS
# fetched when it is there, else downloaded into build/win/dl.
# Wine runs in its own prefix (build/win/prefix), nothing on the screen or the speakers.
set -e
cd "$(dirname "$0")/.."
ROOT=$PWD
W=build/win
SDL=SDL2-2.32.10
TAR=$SDL-mingw.tar.gz
mkdir -p "$W/dl"
if [ ! -f "$W/dl/$TAR" ]; then
    if [ -f "$HOME/renfs/build/release-windows/dl/$TAR" ]; then
        cp "$HOME/renfs/build/release-windows/dl/$TAR" "$W/dl/"
    else
        curl -fsSL -o "$W/dl/$TAR" "https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-devel-2.32.10-mingw.tar.gz"
    fi
fi
[ -d "$W/$SDL" ] || tar -xzf "$W/dl/$TAR" -C "$W"
P=$ROOT/$W/$SDL/x86_64-w64-mingw32
# (its pkg-config file names the place it was built for)
sed "s|^prefix=.*|prefix=$P|" "$P/lib/pkgconfig/sdl2.pc" > "$W/sdl2.pc"
cat > "$W/toolchain.cmake" <<EOT
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32 $P)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
EOT
PKG_CONFIG_LIBDIR=$ROOT/$W PKG_CONFIG_PATH= cmake -S port -B "$W/game" \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/$W/toolchain.cmake" > "$W/cmake.log"
cmake --build "$W/game" -j"$(nproc)" > "$W/build.log" 2>&1 || { grep -E 'error|Error' "$W/build.log"; exit 1; }
cp "$W/game/openmotoracer.exe" "$P/bin/SDL2.dll" "$W/"
echo "$W/openmotoracer.exe: $(x86_64-w64-mingw32-objdump -p "$W/openmotoracer.exe" | awk '/DLL Name/{printf "%s ", $3}')"
[ "$1" = check ] || exit 0

# under wine: no window, no sound, no menu entries or browser installers in the prefix
export WINEPREFIX=$ROOT/$W/prefix WINEDEBUG=-all WINEDLLOVERRIDES="mscoree,mshtml=;winemenubuilder.exe=d"
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
unset DISPLAY WAYLAND_DISPLAY
printf '#!/bin/sh\nexec wine %s/%s/openmotoracer.exe "$@"\n' "$ROOT" "$W" > "$W/wrun"
chmod +x "$W/wrun"
wineboot -i > /dev/null 2>&1
set +e

# the PC's own pictures and replays, to compare with
rm -rf "$W/pc"; mkdir -p "$W/pc"
cmake -S port -B port/build > /dev/null; cmake --build port/build -j"$(nproc)" > /dev/null
tools/shots.sh > "$W/pc/shots.txt" 2>&1
cmp "$W/pc/shots.txt" build/shots.txt || echo "the PC's own pictures differ from build/shots.txt"
cut -d" " -f1 build/shots.txt | while read -r n; do cp "build/shots/$n.bmp" "$W/pc/"; done
tools/replaycheck.sh > "$W/pc/replays.txt"
cp build/replaycheck/*.log "$W/pc/"

tools/shots.sh "$W/wrun" > "$W/shots.txt" 2>&1
python3 tools/shotdiff.py build/shots.txt "$W/shots.txt" build/shots "$W/pc"

tools/replaycheck.sh "$W/wrun"
fail=$?
# the same steps as the PC's, not only the same as its own replay
for n in snow speedbay westway; do
    if tr -d '\r' < "build/replaycheck/$n.log" | cmp -s - "$W/pc/$n.log"; then
        echo "$n: $(wc -l < "$W/pc/$n.log") lines, the same as the PC's"
    else
        echo "$n: its steps DIFFERENT from the PC's"
        fail=1
    fi
done

# the physics and the computer riders against the recordings of the original, on Windows' maths:
# the reports the same as the PC's
tools/physcheck.sh > /dev/null
tools/cpucheck.sh > /dev/null
CC="x86_64-w64-mingw32-gcc -std=c11 -O2 -ffp-contract=off -fno-fast-math -Iport/src"
mkdir -p "$W/check"
$CC -o "$W/check/physcheck.exe" tools/physcheck.c -lm
wine "$W/check/physcheck.exe" build/physcheck/carac.bin build/phys/costab.bin build/phys/sections_speedbay.bin \
    build/phys/w1.bin build/phys/w2.bin build/phys/w3.bin build/phys/w4.bin 2>&1 | tr -d '\r' > "$W/check/phys.txt"
if cmp -s "$W/check/phys.txt" build/physcheck/report.txt; then
    echo "physics: $(wc -l < "$W/check/phys.txt") lines of report, the same as the PC's"
else
    echo "physics: the report DIFFERS from the PC's"; diff "$W/check/phys.txt" build/physcheck/report.txt | head -20
    fail=1
fi
$CC -o "$W/check/cpucheck.exe" tools/cpucheck.c port/src/bikephys.c port/src/cpu.c -lm
for rec in build/cpucap/demo*.bin; do
    case $rec in *_carac.bin|*_sec.bin) continue ;; esac
    b=${rec%.bin}
    wine "$W/check/cpucheck.exe" "${b}_carac.bin" build/phys/costab.bin "${b}_sec.bin" "$rec" 2>&1 | tr -d '\r'
done > "$W/check/cpu.txt"
if cmp -s "$W/check/cpu.txt" build/cpucheck/report.txt; then
    echo "computer riders: $(wc -l < "$W/check/cpu.txt") lines of report, the same as the PC's"
else
    echo "computer riders: the report DIFFERS from the PC's"; diff "$W/check/cpu.txt" build/cpucheck/report.txt | head -20
    fail=1
fi
exit $fail
