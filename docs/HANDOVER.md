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
- Running the original under wine: tools made, but no picture yet (NOTES §5).

## Waiting for Danik
- Whether Claude may run the original on Danik's own screen for reference pictures (a
  window appears for a minute or two), or whether he prefers to run it himself.
- `xdotool` (`sudo pacman -S xdotool`): to press keys in the original on the hidden display.

## Next, in order
1. A picture from the original (NOTES §5), then pictures of each menu page for reference.
2. A model viewer (`tools/model.py`: 3DF + MAP + texture page; which palette the texture
   pages use: they have none of their own).
3. The port's skeleton (C, SDL2, a 16-bit framebuffer at 640x480/320x240) and its first
   picture: the loading screen and the menu backgrounds from the LEZ files; then the font
   (FNT) and the menu pages in their order (25 → 14 → ...).
4. The track (.trk), the race loop, the renderer.
5. The CD's `MOTO.EXE`: line it up with GOG's `game.EXE` (which tables the port reads).
