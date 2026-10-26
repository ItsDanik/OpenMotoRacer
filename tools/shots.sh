#!/bin/bash
# The port's test pictures: a fixed set of --shot runs, one sum each.
#   tools/shots.sh [BINARY]          print "name sum" for every run
#   tools/shots.sh [BINARY] > build/shots.txt, later: tools/shots.sh | diff build/shots.txt -
# The pictures themselves land in build/shots/ (they are the game's, never committed).
cd "$(dirname "$0")/.."
bin=${1:-port/build/openmotoracer}
mkdir -p build/shots
run() {     # name page frames [input]
    OMR_PAGE=$2 OMR_INPUT=$4 "$bin" --shot "build/shots/$1.bmp" "$3" > /dev/null 2>&1
    echo "$1 $(md5sum < "build/shots/$1.bmp" | cut -c1-12)"
}
run logo_120 25 120
run logo_400 25 400
run title_300 14 300
run title_900 14 900
run name_300 15 300
run name_typed 15 500 "320:backspace 380:q"
run main_200 3 200
run main_500 3 500
run main_hover 3 300 "200:move:150,250"
OMR_TILE=36,32 run mode_champ 5 120 "100:move:600,400"
# page 6 through page 5, each matching a capture of the original in build/ref/ (all but
# the pointer, tools/cmpshot.py): Single, Practice, Practice at 5 laps, Snow Ride, Championship
OMR_TILE=69,127 run track_single 5 260 "80:move:320,250 90:click 200:move:630,10"
OMR_TILE=98,8 run track_practice 5 260 "80:move:110,250 90:click 200:move:630,10"
OMR_TILE=86,20 run track_laps5 5 300 "80:move:110,250 90:click 200:move:630,10 210:plus 220:plus"
OMR_TILE=83,113 run track_snow 5 330 "80:move:320,250 90:click 200:move:630,10 210:right"
OMR_TILE=94,12 run track_champ 5 260 "80:move:530,250 90:click 200:move:630,10"
