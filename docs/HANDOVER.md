# Handover — where OpenMotoRacer stands
Read this first in a new session, then CLAUDE.md, then the NOTES sections for the area.

## Done (2026-10-26, session 1)
- Decisions agreed with Danik (CLAUDE.md "Decisions").
- `~/openmotoracer`: git, `.gitignore` (game folders, `CD/`, `build/` kept out), the toolkit
  (the LE-only parts left out), and for this Windows program: `tools/pe2flat.py`,
  `tools/rebuild.sh` (Ghidra's PE loader, seeds, the MSVC split-function repair),
  `tools/imprefs.py` (who calls which Windows function), `tools/vtables.py`.
- The CD (`CD/*.BIN/CUE`, Danik's): `tools/cdbin.py` → `build/cd/data.iso` + audio tracks;
  files extracted to `Moto Racer CD/`; `tools/is3.py` unpacks the installer's `data.z`
  (the CD's own program `MOTO.EXE`, 1997-07-21, a different build from GOG's). NOTES §1, §3.5.
- Formats read, each tool checked on every file: BKF banks (`tools/bkf.py`), RAW pictures
  (`tools/raw.py`), LEZ pictures (`tools/lez.py`, unpacker 0x43eb00), BPT scripts
  (`tools/bpt.py`, the cipher 0x45a4a0). 3DF/MAP models read from the loader (NOTES §4.5),
  no tool yet.
- Map: the MFC app, `OnIdle` = the frame, the menu's page table, the race object, the
  variable-step clock (NOTES §4). The picture is 16-bit RGB565.
- The original runs unseen (`tools/orig.sh`, headless Weston + Xwayland, its window given
  focus with xdotool, keys reach it): logo, title and name entry pictured (NOTES §5).

- The port's skeleton (`port/`, CLAUDE.md "The port"): C/SDL2, reads the player's
  game.EXE for the game's tables, the banks, the LEZ pictures and the game's own drawing
  routine; its first picture, the settled title page, equals the original's to the pixel
  (5-6-5) apart from the Start button (NOTES §4.7).

- The menu core (NOTES §4.4, mapped from the machine code: pages, buttons, keys, mouse,
  slides, the glass transition's timing) and the main menu (page 3) in the port: equal
  to the original's main menu apart from the pointer; arrows, Tab, Return, Escape and the
  mouse work as in the game. The text system mapped (NOTES §4.8, `tools/fnt.py`).

- The text system in the port (fonts, text objects): "FULL D3D VERSION" equal to the
  original's pixel for pixel, blinking at its 500 ms (NOTES §4.8.3.7).
- A progress tracker for Danik: https://claude.ai/artifact/CCupuhZmsCRgz7FA5eFMxz (its
  numbers are a JSON file uploaded to it; update it after each step).

- The 3D renderer and pages 25 (logo) and 14 (title) with the breaking-glass transition
  between them (NOTES §4.9): the logo within ~1100 px of the original, the glass matching
  its captures, the settled title equal to the original. The port now starts on 25.

- Sound (NOTES §4.10, mapped by a helper into build/notes/sound.md, then into NOTES):
  `tools/sfx.py` decodes SFX.BKF; `port/src/sound.c` plays the effects, the voice queues
  and the music as GOG does; a recorded run equals 13.wav sample for sample.
- Page 15, the name entry, in the port (NOTES §4.11): equal to the original's picture
  (through model.py) apart from the pointer; the chain logo → title → name → main menu runs.
- The Solo pages mapped (NOTES §4.12, from build/notes/solo_pages.md; captures
  `build/ref/orig_p*`); pages 5 (SELECT MODE) and 6 (SELECT RACE / CHAMPIONSHIP) in the
  port with the race settings and track lists (`choices.c`): six screens equal to the
  original's captures but for the pointer. The track format is mapped part-way
  (`build/notes/track.md`).
- The speed (Danik, 2026-10-26: "runs a bit too fast"): the clock is the real time in a
  window, per-picture counts go by 60ths (NOTES §4.2). `tools/shots.sh` sums the test
  pictures (`tools/shots.sh | diff build/shots.txt -` after every change);
  `tools/cmpshot.py` compares a picture with a capture and finds the tile's scroll.
- Pages 10 CHOOSE BIKE (2D; the 3D bike not yet), 9 TOP 5 (bestscor.dat read) and 1
  loading in the port (NOTES §4.12.12): TOP 5 single race and championship, and the loading
  screen, equal to the original's captures but the pointer; page 10 equal outside the bike.
  The whole Solo chain runs logo → … → Start → loading, then back to the main menu (no
  race yet). Correction in NOTES §4.12.10: the loading logo and badges are drawn by the
  load's steps (0x4470d0).
- Pages 20/21, championship Load/Save: equal to the original's captures; the original
  loads a save the port wrote. Saves go to ~/.config/openmotoracer for now (saves.c),
  read from there or else from the game's data/.
- The race's track in the port (NOTES §4.13, the track notes folded in there): the .trk,
  the level's pages, the section walk, what a camera sees, the race's perspective-correct
  spans and see-through kinds, the horizon. `OMR_RACEVIEW` draws a track from a still
  camera: the same to the pixel as tools/track.py on eight captures of the original;
  only the bikes, riders, shadows and HUD differ from the original's screens.
- Pages 11 Options, 12 controls, 13 diagnostic, 23 data disk (NOTES §4.14): equal to the
  original's captures but for the pointer (and the CD volume, which wine never offered).
  The settings (CONFIG.BPT) are saved to the home folder when the program ends; sound
  switches and the three volumes work. No joystick yet. Page 13 shows the port's machine.
- The 3D bike on page 10 (NOTES §4.15, mapped by a helper with tools/bikeparts.py as the
  reference): skeleton, pose, wheels, the liveries' palettes, the drawer's menu part.
  Equal to the original's captures (dirt: 8 edge pixels; road: all). `OMR_TURN` holds the
  turntable for such checks.
- The race starts: page 1 loads it (track, bikes, 24 riders on the grid as 0x47bf70
  places them). The rules, laps, places, countdown, time limit, the dashboard and dials,
  the pause menu, the end of the race (results, continue, bestscor.dat) are written.
- The player's bike rides (bikephys.c, bit-exact on the original's recorded steps) under
  the chase camera (camera.c). The computer riders stand on their grid places.
- The tracker shows Claude's messages at its bottom ("From Claude", the `messages` dataset)
  and Danik writes from it (comments sent to Claude): keep it watched.

## Done (2026-10-26, session 2: the race made whole)
- The bike's drawing leans, pitches and steers; the chase camera turns with it; F7 the
  dashboard's three states and the gear hint; smooth pictures between the 62.5 Hz steps
  (F10; Danik asked for "unlocked framerate, correct speed").
- TOP 5: a new record's name typed and saved.
- The computer riders drive (`cpu.c`, NOTES §4.21, bit-exact on three recordings of the
  original: `tools/cpucheck.sh`, the recorder `tools/win/cpuwatch.c`).
- The race's sounds (`race_sound.c`, NOTES §4.10.6): engines, skids, crowd, voices.
- GAME OVER (a sprite object, `sprite.c`, NOTES §4.20 §3.5); Pause.
- Joysticks (`joy.c`, NOTES §4.22): detection, dead zone, page 12 binding, page 13, the
  race's bits through the bindings, the dialogs' arrows, Ctrl+J / Ctrl+K.
- The on-board view (F6) with the cockpit (NOTES §4.20 mode 6): looks the same as the
  original's capture (`build/ref_onboard_orig.png`, Speed Bay; retake it as below). The fix that made the cockpit textured also gave the
  player's rear wheel its tyre texture (it was black near the camera).

### Driving the original to the on-board view (Speed Bay, Practice)
Keep its sound off Danik's speakers: `pactl load-module module-null-sink sink_name=mrnull`
and `export PULSE_SINK=mrnull` before `tools/orig.sh start`. Keys with `tools/orig.sh key`:
Escape (retry until the title shows, not the logo), Return (name), Return (main menu),
Left + Return (Solo), Left Left + Return (Practice), Return (track page), Return (bike page:
Start), then F6 once the race runs. `tools/orig.sh shot FILE`; afterwards
`tools/orig.sh stop` and `pactl unload-module module-null-sink`.

## Done (2026-10-30, session 3: replays, the TV, the winner's lap)
- Replays (`replay.c`, NOTES §4.21b): YES on the results watches the race again under the TV's
  cameras, to the bit (`tools/replaycheck.sh`). The demos start after 30 s on the title.
- The TV director (`camera.c`), the record split line on the dashboard (`rules.c`).
- The winner's lap with the credits (`generic.c`, the .GEN scripts): compared with a capture of
  the original (`build/ref/winner_*_orig.png`; how to force it in the original: NOTES §4.21b).
- The championship's end: pages 16, 17, 18 and the podium (`page_podium.c`, NOTES §4.12.14): the
  stand the same as the original's capture to the pixel. The player's 4th place not captured.
- The riders' bodies (`pose.c`, NOTES §4.18 8.16a): every pose by speed, lean, jump, stunt, rival,
  place, the salute and the fall, with the hand skeleton. Same as the original's captures by eye.

## Waiting for Danik
- The CD's MOTO.EXE (NOTES §1): its text code differs from GOG's; run GOG's logic on the
  CD's data, or ask for the official 3.22 patch?
- Music: loop for ever (GOG, now) or the CD's "play a repeating track twice"?
- Where saves go (bestscor.dat, championship saves, ghosts): the game's data folder, as
  the original, or a user folder? (For now a user folder, the game's read as a fallback.)

## Next, in order
3. The CD sprite messages (0x482030), the mouse in the race's dialogs, F1 (the help: the same
   .GEN parser, `generic.c`) / F11 / F12, the music's pause.
4. The track's billboards (+0x5c) and AnimDecor (land.c, "not read yet"); the bikes'
   shadows and lighting; the glass capture's page 2 against the level's pages 2..19.
6. The CD's `MOTO.EXE`: line it up with GOG's `game.EXE` (which tables the port reads).
7. Then speed on the MiSTer's ARM, releases, the MiSTer core (the skill's steps 8-10).
8. **Last: the split screen** for two players on one PC (the original has none). Danik,
   2026-10-30: leave it for last and **stop before starting work on it**.
