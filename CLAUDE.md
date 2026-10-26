# OpenMotoRacer — *Moto Racer* (1997, Delphine Software, Windows 95) for Linux and the MiSTer

Reverse engineering and a hand-written reimplementation of `game.EXE`, the Windows program
of *Moto Racer* (the 1998 "polygons version 3.22" patch, as GOG sells it). A native SDL2
port of the **software renderer** (Linux x86, Windows, then ARM), then a MiSTer hybrid core
(game on the ARM, the FPGA for 15 kHz video, audio and input).

**This file is the summary. `docs/NOTES.md` is the full record**: addresses, proofs,
formats, dead ends. New findings go there in detail and here as one line.
**`docs/HANDOVER.md` is where the work stands**: read it first in a new session.

---

## Working agreement — read this first

**Danik does not write code or decompile.** He knows the MiSTer and these ports, and he
does the verification and judges whether it plays like the original. Don't hand him code
to review, patches to apply or compiler errors to sort out.

- **Claude:** all code, tooling, decompilation, builds, debugging. This machine has
  Ghidra 12.1.2 (`/opt/ghidra`), python3 (pefile, PIL), wine/proton, objdump. The `Read`
  tool opens PNGs: look at the pictures you make yourself.
- **Danik:** real hardware, whether it looks and feels like the original, and decisions
  about scope, licences and targets.

"Here is a build and a screenshot" beats "please check this function". Everything builds
with one command. Plain language, not jargon.

**The method is the skill `reverse-engineer-dos-game`** (`~/.claude/skills/`), as on
ReNFS (`~/renfs`) and OpenScreamer (`~/openscreamer`). This game is a **Windows** program
(MSVC, PE), not DOS: Ghidra's own PE loader replaces `unle.py`, and the original is run
under wine instead of DOSBox. Their "Hard-won rules" hold here too, above all: **when a
format question stalls, find the code that reads the field.**

---

## Decisions (Danik, 2026-10-26)

- Engine name **OpenMotoRacer** (`~/openmotoracer`); MiSTer id `MotoRacer`.
- **The player provides the GOG copy as installed** (`Moto Racer/`: `game.EXE`, `data/`,
  the music `02.wav`–`13.wav`), **and the original 1997 CD is to be supported too** (its
  image in `CD/`, its files extracted to `Moto Racer CD/`, neither committed). The port reads the
  game's tables from the player's `game.EXE`, each known build by its hash.
- **Scope: the software renderer, one or two players on one PC.** No Direct3D renderer, no
  network or modem play (DirectPlay).
- **Hand-written from the start.** Decompiled code is read and clean C is written that does
  the same. Machine-converted code only as an unpublished reference under `build/`.
- **Local git only** until Danik says so; commit after each finished step. GPL-3 when it
  goes public.

## Legal

The game is still its publisher's. Never publish `Moto Racer/` or anything made from it:
the flat image, the Ghidra project, `build/decomp`, unpacked banks, pictures, converted code.
All of that stays in the git-ignored `build/`. Decompiled text is never pasted into `port/`.
`git status` before every commit.

## The binary and how to decompile it

`Moto Racer/game.EXE` (SHA-1 `164b4175…`, linked 1998-07-09): PE32, MSVC C++ (sources
`D:\Dsed\mr320\...`), base 0x400000, entry 0x4b6420. DirectDraw, Direct3D (immediate mode),
DirectSound, DirectPlay, WinMM (CD audio through MCI). `MotoRacer.exe` is only GOG's
launcher (Delphi): it writes `motoracer.ini` and starts `game.EXE`.

- `tools/rebuild.sh`: Ghidra imports the PE, analyses, seeds functions from the relocation
  records, renames from `tools/names.txt`, decompiles everything into `build/decomp/`.
  `tools/rebuild.sh --decomp` after new names.
- `tools/pe2flat.py`: the program laid out flat at its addresses (`build/mr_flat.bin`) and
  the layout map with every pointer site (`build/mr_layout.json`) for `strmap.py`,
  `seeds.py`, `strrefs.py`.

## The port (`port/`)
C11, CMake, SDL2 linked dynamically. Built with one command:
`cmake -S port -B port/build && cmake --build port/build` → `port/build/openmotoracer`.
- `-home DIR` / `OMR_HOME`: where saves are written (`saves.c`; default
  `~/.config/openmotoracer`), the game's format; read from there, else the game's data/.
- `-data DIR` / `OMR_DATA`: the player's Moto Racer folder (game.EXE + data/; default
  `./Moto Racer`). Names matched in any case. Files through the banks first (`files.c`).
- **The game's tables are read from the player's game.EXE** at their addresses
  (`exe.c`: the PE placed at 0x400000, the build known by its FNV-1a sum; unknown builds
  are refused with a message).
- The screen is the game's: 640x480 RGB565 (`video.c`), shown 4:3.
- `--shot FILE N` (`OMR_SHOT`): no window, N frames, the screen as a BMP. `--help` lists
  the test switches (`OMR_*`).
- `clock.c`: the game's millisecond clock (timeGetTime): the real time in a window, 1000/60 a
  picture under `--shot`. What the game counts in pictures (tiles, blinking, drifting texts)
  counts 60ths of a second (`g_ticks`), so any refresh rate gives the original's speed.
- `tools/shots.sh`: the test pictures' sums; `tools/shots.sh | diff build/shots.txt -` after
  every change.
- `menu.c`: the menu core (NOTES §4.4): pages by the game's numbers, page change, buttons,
  keys (SDL keys turned into Windows key codes) and mouse, the game's own pointer.
- Pages written: 25 logo and 14 title (`page_intro.c`), 15 name entry (`page_name.c`, its
  3D letters through `obj3d.c`, NOTES §4.11), 3 main menu (`page_main.c`), 4 quit,
  5 SELECT MODE (`page_mode.c`), 6 SELECT RACE (`page_track.c`), 10 CHOOSE BIKE
  (`page_bike.c`, `bikes.c`; no 3D bike yet), 9 TOP 5 (`page_top5.c`, `scores.c`:
  bestscor.dat), 1 loading (`page_load.c`: back to page 3 until the race exists), 20/21
  championship Load/Save (`page_champ.c`); the race
  settings and track lists in `choices.c` (NOTES §4.12). A page not written yet ends the
  program with a message. Starts on 25, as the game.
- 3D (NOTES §4.9): `r3d.c` the software renderer (float transform, depth buckets, s-buffer,
  affine triangles), `model.c` 3DF+MAP, `rand.c` the game's random streams (seeds from
  game.EXE). The glass transition is `menu.c`'s.
- Sound (NOTES §4.10): `sound.c`, the effects by the game's id tables, its queues of
  delayed voices, the music as GOG plays it (NN.wav, looping). `OMR_WAV=FILE` records it.
- Text (NOTES §4.8): `script.c` reads BPT, `font.c` the 35 fonts (FONTS.BPT + .FNT),
  `text.c` the 48 text objects (blink, fades, see-through, drift).
- Test switches: `OMR_PAGE`, `OMR_CHOICES="MODE,TRACK,LAPS[,REV,POC]"`, `OMR_TILE=SX,SY` (the chequered tile's scroll, to match a capture;
  `tools/cmpshot.py` finds it), `OMR_INPUT="F:key F:move:X,Y F:click F:down F:up"` (input at frame F),
  `OMR_TEXT="FONT,X,Y,ALIGN,EFFECT,BLINK,FORE,BACK,TEXT"` (one text object).
- Progress tracker for Danik: https://claude.ai/artifact/CCupuhZmsCRgz7FA5eFMxz

## File formats

| Ext | What | Status |
|-----|------|--------|
| .BKF | bank: u32 count, 0x2c-byte records (name[0x24], offset, size), files | `tools/bkf.py`, all 28 unpack |
| .RAW | Image Alchemy "mhwanh" picture (BE header, palette, 8-bit pixels) | `tools/raw.py` |
| .LEZ | packed 8-bit TGA (`LEZ1`, LZ with 10-bit distances) | `tools/lez.py`, all 755 |
| .FNT | bitmap font: first, count, height, palette size, palette, widths, glyphs | `tools/fnt.py`, all 34; NOTES §4.8 |
| .BPT | the .Spt scripts as tokens, XOR with Park-Miller from 0x1a2b3c4d | `tools/bpt.py`, all |
| .3DF .MAP | model (float vertices x96, faces) + per-face texture mapping | read from 0x4216a0, NOTES §4.5 |
| .ROT .SKL .PIV | animations, skeletons | to do |
| .trk | track (`TRK\0`, floats), `data/ileNN/` | to do |
| .WAV | effects and voices in SFX.BKF: RIFF, enciphered (byte − 4(i+8) per block), mono | `tools/sfx.py`, all 250; NOTES §4.10 |
| .dem | demo list (text: track, time, picture) | read |
| data.z | the CD's InstallShield 3 archive (files stored) | `tools/is3.py` |

## How the game works (the summary; NOTES §4)
- MFC app; `App_OnIdle` 0x415960 is the frame: the menu (page table 0x4dbb38) or the race
  (`Game_Frame` 0x47d660). One step a frame of the measured time (ms x 0.1, at most 200 ms):
  the port runs fixed 1/60 s steps.
- The picture is **16-bit RGB565** (320x240, 512x384, 640x480).
- Running the original: `tools/orig.sh start|shot|key|stop`, unseen (headless Weston +
  Xwayland; the game only moves while its window has focus). `ORIG_DISPLAY=:1` is Danik's
  own screen (allowed). `tools/win/memtool.c` reads its memory (NOTES §5).
