#!/bin/bash
# A picture of the running original's back surface (640x480 RGB565 at the address held in
# 0x4eea14, the software renderer's locked surface, Cdd lock 0x476700), as PNG.
#   tools/origshot.sh OUT.png
cd "$(dirname "$0")/.."
export DISPLAY=:57 WINEPREFIX=$PWD/build/wineprefix WINEDEBUG=-all
[ -f build/memtool.exe ] || i686-w64-mingw32-gcc -O2 -o build/memtool.exe tools/win/memtool.c
wine build/memtool.exe deref 0x4eea14 ${LEN:-532480} 'Z:\tmp\origshot.raw' || exit 1
python3 -I -c "
import sys, numpy as np
from PIL import Image
a = np.frombuffer(open('/tmp/origshot.raw','rb').read(), '<u2').reshape(-1, 640)
r = ((a >> 11) & 31) * 255 // 31; g = ((a >> 5) & 63) * 255 // 63; b = (a & 31) * 255 // 31
Image.fromarray(np.dstack([r, g, b]).astype('uint8')).save(sys.argv[1])" "$1"
rm -f /tmp/origshot.raw
