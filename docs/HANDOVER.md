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
- Mapped by helpers, not yet in NOTES: the Solo pages 5, 6, 9, 10, 1 and the race settings
  (`build/notes/solo_pages.md`, captures `build/ref/orig_p*`). The track format
  (`build/notes/track.md`, part-way).
- The tracker shows Claude's messages at its bottom ("From Claude", the `messages` dataset)
  and Danik writes from it (comments sent to Claude): keep it watched.

## Waiting for Danik
- The CD's MOTO.EXE (NOTES §1): its text code differs from GOG's; run GOG's logic on the
  CD's data, or ask for the official 3.22 patch?
- Music: loop for ever (GOG, now) or the CD's "play a repeating track twice"?
- Where saves go (bestscor.dat, championship saves, ghosts): the game's data folder, as
  the original, or a user folder?

## Next, in order
1. The Solo pages from build/notes/solo_pages.md (5, 6, 10, 9, then 1), then 11 options.
2. Sound: CONFIG.BPT's switches and volumes, pause on losing focus, the race's sounds.
3. The track (.trk), the race loop, the race renderer (the same r3d with the race's
   near plane 64 and its face kinds): build/notes/track.md, track_raster.md/.py,
   track_horizon.md/.py and tools/track.py (a helper was stopped part-way).
4. The CD's `MOTO.EXE`: line it up with GOG's `game.EXE` (which tables the port reads).
