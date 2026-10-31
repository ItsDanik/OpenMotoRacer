#!/bin/bash
# The port for the MiSTer's ARM, static, in the hybrid toolchain's container (~/Hybrid_MiSTer):
#   tools/arm.sh            build/arm/openmotoracer
#   tools/arm.sh check      and every test picture and replay on it under qemu, against the PC's
# SDL2 is a hybrid core's build of hybrid/sdl2 (SDLPREFIX: its mister/prefix). The pictures are
# compared in 5-6-5, the game's own colours: SDL's conversion to a BMP rounds differently on ARM.
# Afterwards build/shots/ holds the ARM's pictures: run tools/shots.sh again for the PC's.
cd "$(dirname "$0")/.."
SDLPREFIX=${SDLPREFIX:-$HOME/MiSTer-ecwolf/build/sdl/mister/prefix}
HYBRID=${HYBRID:-$HOME/Hybrid_MiSTer}
TC="-u $(id -u):$(id -g) -e HOME=/tmp -v $PWD:$PWD -w $PWD"
docker run --rm $TC -v "$SDLPREFIX:$SDLPREFIX:ro" -v "$HYBRID:$HYBRID:ro" mister-hybrid-tc sh -c "
    export PKG_CONFIG_LIBDIR=$SDLPREFIX/lib/pkgconfig
    cmake -S port -B build/arm -DCMAKE_TOOLCHAIN_FILE=$HYBRID/toolchain/mister.cmake \
        -DCMAKE_FIND_ROOT_PATH=$SDLPREFIX -DPKG_CONFIG_EXECUTABLE=/usr/bin/pkg-config \
        -DCMAKE_C_FLAGS=-marm -DCMAKE_EXE_LINKER_FLAGS=-static > build/arm.log 2>&1 &&
    cmake --build build/arm -j8 >> build/arm.log 2>&1" || { tail -20 build/arm.log; exit 1; }
[ "$1" = check ] || exit 0

tools/shots.sh > /dev/null 2>&1
rm -rf build/arm/pc; mkdir -p build/arm/pc
cut -d" " -f1 build/shots.txt | while read -r n; do cp "build/shots/$n.bmp" build/arm/pc/; done
printf '#!/bin/sh\nexec qemu-arm %s/build/arm/openmotoracer "$@"\n' "$PWD" > build/arm/qrun
chmod +x build/arm/qrun
docker run --rm $TC -e SDL_VIDEODRIVER=dummy -e SDL_AUDIODRIVER=dummy mister-dxxrebirth-qemu bash -c \
    'tools/shots.sh build/arm/qrun > build/arm/shots.txt 2>&1; tools/replaycheck.sh build/arm/qrun'
python3 - <<'PY'
import numpy as np
from PIL import Image
def q(f):
    a = np.asarray(Image.open(f).convert("RGB")).astype(int)
    return (a[..., 0] >> 3) * 2048 + (a[..., 1] >> 2) * 32 + (a[..., 2] >> 3)
names = [l.split()[0] for l in open("build/shots.txt")]
bad = [(n, int((q(f"build/shots/{n}.bmp") != q(f"build/arm/pc/{n}.bmp")).sum())) for n in names]
bad = [b for b in bad if b[1]]
print(f"pictures: {len(names) - len(bad)} of {len(names)} the same as the PC's")
for b in bad:
    print(b)
PY
