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
  files not here yet: they go in `Moto Racer CD/`, never committed). The port reads the
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

## File formats

| Ext | What | Status |
|-----|------|--------|
| .BKF | bank: u32 count, 0x2c-byte records (name[0x24], offset, size), files | `tools/bkf.py`, all 28 unpack |
| .RAW | Image Alchemy "mhwanh" picture (BE header, palette, 8-bit pixels) | `tools/raw.py` |
| .LEZ | compressed picture, magic `LEZ1` | to do (decoder in the EXE) |
| .BPT | settings/lists; enciphered in `data/`, plain inside banks | to do |
| .3DF .MAP .ROT .SKL .PIV | meshes, mappings, animations, skeletons | to do |
| .trk | track (`TRK\0`, floats), `data/ileNN/` | to do |
| .WAV | sound effects in SFX.BKF (headerless?) | to do |
| .dem | demo list (text: track, time, picture) | read |
