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
# page 10 (2D; the bike is not drawn yet, the rest matches orig_p10_bike_a and _manual)
run bike_auto 5 420 "80:move:320,250 90:click 200:move:630,10 210:right 260:return 330:move:630,10"
run bike_manual 5 560 "80:move:320,250 90:click 200:move:630,10 210:right 260:return 330:move:630,10 400:right 430:move:448,425 440:click 450:move:630,10"
# page 9, from page 6's Top 5 (Snow Ride, 2 laps) and straight away in a championship
# (Speed Bay, 4 laps): orig_p9_top5, orig_p9_top5_champ
OMR_TILE=13,115 run top5_snow 5 450 "80:move:320,250 90:click 200:move:630,10 210:right 260:move:574,417 270:click 300:move:630,10"
OMR_CHOICES=2,0,4 OMR_TILE=109,19 run top5_champ 9 200 "1:move:630,10"
# page 1 from page 10's Start: orig_p1_loading_snowride
run loading_snow 5 520 "80:move:320,250 90:click 200:move:630,10 210:right 260:return 330:move:630,10 420:return"
