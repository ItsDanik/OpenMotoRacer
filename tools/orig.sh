#!/bin/bash
# Run the original game.EXE under wine, unseen, in a private copy (build/orig, with GOG's
# dgVoodoo DDraw.dll/D3DImm.dll, which give the 16-bit screen the game needs) and a private
# wine prefix (build/wineprefix). The hidden display is a headless Weston with Xwayland: it
# has a window manager, so the game's window can be given focus (the game pauses without it).
# Plain Xvfb stays black (NOTES §5).
#   tools/orig.sh start [switches...]   (default: -NoD3D -windowed -us); waits for the window
#   tools/orig.sh shot FILE.png          picture of the game's window
#   tools/orig.sh key KEYSYM...          keys to the game's window
#   tools/orig.sh stop                   the game and the hidden display
# ORIG_DISPLAY=:1 uses a real display instead (Danik's, with his permission).
cd "$(dirname "$0")/.."
export WINEPREFIX=$PWD/build/wineprefix WINEDEBUG=${WINEDEBUG:--all}
mkdir -p build/logs

# the hidden display: start Weston once, its X display number from its log
hidden() {
    if ! [ -s build/logs/weston.pid ] || ! kill -0 "$(cat build/logs/weston.pid)" 2>/dev/null; then
        rm -f build/logs/weston.log
        weston --backend=headless --xwayland --socket=mrwl --width=1024 --height=768 \
            --idle-time=0 > build/logs/weston.log 2>&1 &
        echo $! > build/logs/weston.pid
        for _ in $(seq 50); do grep -q 'listening on display' build/logs/weston.log && break; sleep 0.2; done
    fi
    grep -o 'listening on display :[0-9]*' build/logs/weston.log | tail -1 | grep -o ':[0-9]*'
}
if [ -n "$ORIG_DISPLAY" ]; then export DISPLAY=$ORIG_DISPLAY
elif [ "$1" = start ]; then export DISPLAY=$(hidden)
else export DISPLAY=$(grep -o 'listening on display :[0-9]*' build/logs/weston.log 2>/dev/null | tail -1 | grep -o ':[0-9]*')
fi

# the game's own window (640x480 drawing area), not wine's helper windows
win() { xdotool search --name "^Moto Racer" 2>/dev/null | tail -1; }

case "$1" in
start)
    shift
    [ -d build/orig ] || { mkdir -p build/orig; cp -r "Moto Racer/data" "Moto Racer/game.EXE" "Moto Racer/winmm.dll" "Moto Racer/DDraw.dll" "Moto Racer/D3DImm.dll" "Moto Racer/dgVoodoo.conf" "Moto Racer"/[01]*.wav build/orig/; }
    (cd build/orig && rm -f data/delphine.err
     WINEDLLOVERRIDES='ddraw,d3dimm=n,b' nohup wine game.EXE ${@:--NoD3D -windowed -us} > ../logs/orig.log 2>&1 &)
    for _ in $(seq 600); do w=$(win); [ -n "$w" ] && break; sleep 0.2; done
    [ -n "$w" ] || { echo "no game window (build/logs/orig.log)"; exit 1; }
    xdotool windowactivate "$w" 2>/dev/null
    echo "running on $DISPLAY, window $w"
    ;;
shot) w=$(win); xdotool windowactivate "$w" 2>/dev/null; import -window "${w:-root}" "$2" ;;
key) shift; w=$(win); xdotool windowactivate "$w" 2>/dev/null
     for k; do xdotool key --delay 100 "$k"; done ;;
stop)
    wineserver -k 2>/dev/null
    if [ -z "$ORIG_DISPLAY" ] && [ -s build/logs/weston.pid ]; then
        kill "$(cat build/logs/weston.pid)" 2>/dev/null; rm -f build/logs/weston.pid
    fi ;;
esac
