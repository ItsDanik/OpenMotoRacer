#!/bin/bash
# Run the original game.EXE under wine on a hidden 24-bit X display (full screen: wine emulates the 16-bit mode) (:57), in a private
# copy (build/orig) and wine prefix (build/wineprefix). The game needs a 16-bit screen.
#   tools/orig.sh start [switches...]   (default: -NoD3D -windowed -us)
#   tools/orig.sh shot FILE.png          picture of the hidden display
#   tools/orig.sh key KEYSYM...          (needs xdotool)
#   tools/orig.sh stop
cd "$(dirname "$0")/.."
export DISPLAY=:57 WINEPREFIX=$PWD/build/wineprefix WINEDEBUG=${WINEDEBUG:--all}
case "$1" in
start)
    shift
    [ -d build/orig ] || { mkdir -p build/orig; cp -r "Moto Racer/data" "Moto Racer/game.EXE" "Moto Racer/winmm.dll" "Moto Racer/DDraw.dll" "Moto Racer/D3DImm.dll" "Moto Racer/dgVoodoo.conf" "Moto Racer"/[01]*.wav build/orig/; }
    xdpyinfo >/dev/null 2>&1 || { Xvfb :57 -screen 0 1024x768x24 >/dev/null 2>&1 & sleep 1; }
    cd build/orig && rm -f data/delphine.err; WINEDLLOVERRIDES='ddraw,d3dimm=n,b' nohup wine game.EXE ${@:--NoD3D -windowed -us} > ../logs/orig.log 2>&1 &
    ;;
shot) import -window root "$2" ;;
key) shift; for k; do xdotool key --delay 100 "$k"; done ;;
stop) wineserver -k; kill $(cat /tmp/.X57-lock 2>/dev/null) 2>/dev/null ;;
esac
