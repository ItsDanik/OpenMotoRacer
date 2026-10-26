#!/bin/bash
# The port's test pictures: a fixed set of --shot runs, one sum each.
#   tools/shots.sh [BINARY]          print "name sum" for every run
#   tools/shots.sh [BINARY] > build/shots.txt, later: tools/shots.sh | diff build/shots.txt -
# The pictures themselves land in build/shots/ (they are the game's, never committed).
cd "$(dirname "$0")/.."
bin=${1:-port/build/openmotoracer}
mkdir -p build/shots
# the saves a run writes go to a scratch folder, emptied first (never the player's)
export OMR_HOME=$PWD/build/shots/home
rm -rf "$OMR_HOME"
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
# page 10 with its turning bike; the turntable held (OMR_TURN) where the original's captures
# had it: orig_p10_bike_a (the dirt bike, 8 pixels at a boot's edge differ) and
# orig_p10_champ_roadbike (the road bike, all equal), the pointer apart
run bike_auto 5 420 "80:move:320,250 90:click 200:move:630,10 210:right 260:return 330:move:630,10"
run bike_manual 5 560 "80:move:320,250 90:click 200:move:630,10 210:right 260:return 330:move:630,10 400:right 430:move:448,425 440:click 450:move:630,10"
run bike_change 5 406 "80:move:320,250 90:click 200:move:630,10 210:right 260:return 330:move:630,10 400:right"
OMR_TURN=1.302 run bike_dirt 5 420 "80:move:320,250 90:click 200:move:630,10 210:right 260:return 330:move:630,10"
OMR_TURN=2.4985 run bike_road 5 480 "80:move:530,250 90:click 200:move:630,10 260:move:572,372 270:click 330:move:630,10"
# page 9, from page 6's Top 5 (Snow Ride, 2 laps) and straight away in a championship
# (Speed Bay, 4 laps): orig_p9_top5, orig_p9_top5_champ
OMR_TILE=13,115 run top5_snow 5 450 "80:move:320,250 90:click 200:move:630,10 210:right 260:move:574,417 270:click 300:move:630,10"
OMR_CHOICES=2,0,4 OMR_TILE=109,19 run top5_champ 9 200 "1:move:630,10"
# page 1 from page 10's Start: orig_p1_loading_snowride
run loading_snow 5 423 "80:move:320,250 90:click 200:move:630,10 210:right 260:return 330:move:630,10 420:return"
# pages 21 and 20 (in this order: the save made is then listed and loaded):
# orig_p20_load_empty, orig_p20_load_hover2, orig_p20_load_saved (the original listing the
# port's own Champ1.sav, which it loads); champ_save has no capture (the original reaches
# page 21 only after a championship race)
IN20="80:move:530,250 90:click 200:move:630,10 260:move:572,411 270:click 300:move:630,10"
OMR_TILE=105,105 run champ_load_empty 5 450 "$IN20"
OMR_TILE=108,108 run champ_load_hover 5 450 "$IN20 400:move:300,220"
OMR_CHOICES=2,0,4 run champ_save 9 360 "150:move:200,452 160:click 260:move:300,230 270:click 300:move:630,10"
OMR_TILE=27,27 run champ_load_saved 5 450 "$IN20"
run champ_loaded 5 560 "$IN20 400:move:300,230 410:click 430:move:630,10"
# the race's track alone (OMR_RACEVIEW), from cameras the original had: West Way's canyon
# (build/trackcap q20), Snow Ride (p3), West Way through a TV camera at x3 zoom (q15).
# Each the same to the pixel as tools/track.py's view with build/notes' race rasterizer
OMR_RACEVIEW=4,394.0,56277.69140625,12255.3740234375,51147.0625,-0.5763577222824097,0.048714954406023026,0.8157442212104797,0.0,0.9982215762138367,-0.0596122108399868,-0.8171975612640381,-0.034357957541942596,-0.5753327012062073 run race_westway 3 1
OMR_RACEVIEW=1,394.0,-14334.8232421875,275.4504089355469,-1427.6490478515625,0.9294362664222717,0.055745311081409454,0.3647473454475403,0.0,0.9885218143463135,-0.15107843279838562,-0.36898258328437805,0.14041776955127716,0.918768048286438 run race_snow 3 1
OMR_RACEVIEW=4,1182.0,-217.19285583496094,509.0,6831.60986328125,-0.9877492189407349,0.0238176379352808,0.15422138571739197,0.0,0.988283634185791,-0.15262851119041443,-0.1560497283935547,-0.15075869858264923,-0.976176381111145 run race_tv 3 1
# pages 11, 12 and 23 (Options, its keyboard settings, the data disk), sound on (OMR_WAV) as
# the original had it: orig_p11_options (all but the CD volume, which wine's MCI never
# offered, and the pointer), orig_p12_settings_keyboard, orig_p23_datadisk (reached through
# the title, which gives font 31 the colours the list name is drawn with). Then Q bound to
# Higher Gear, and the next track list. Page 13 shows the machine it runs on: not here.
# Their own home folder: these runs save changed settings.
export OMR_HOME=$PWD/build/shots/home_opt
rm -rf "$OMR_HOME"
INO="200:move:320,440 210:click 300:move:630,10"
OMR_WAV=build/shots/o.wav OMR_TILE=59,59 run options 3 400 "$INO"
OMR_WAV=build/shots/o.wav OMR_TILE=86,62 run settings 3 600 "$INO 400:move:500,132 410:click 500:move:630,10"
OMR_WAV=build/shots/o.wav run settings_bind 3 600 "$INO 400:move:500,132 410:click 500:move:400,320 520:q"
OMR_WAV=build/shots/o.wav OMR_TILE=120,120 run datadisk 14 900 "100:return 250:return 500:move:320,440 510:click 600:move:630,10 700:move:330,435 710:click 800:move:630,10"
OMR_WAV=build/shots/o.wav run datadisk_next 3 800 "$INO 400:move:330,435 410:click 500:move:630,10 600:right"
# the race (Snow Ride) on its first frames: the riders on the grid, seen from the port's
# stand-in camera (the game's chase camera is not written yet)
export OMR_HOME=$PWD/build/shots/home
run race_grid 5 440 "80:move:320,250 90:click 200:move:630,10 210:right 260:return 330:move:630,10 420:return"
