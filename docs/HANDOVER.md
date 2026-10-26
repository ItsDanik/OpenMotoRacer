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

## Waiting for Danik
- Nothing. (He allowed the original on his screen, `ORIG_DISPLAY=:1`, and installed xdotool.)

## Next, in order
1. The menu's machinery: the page table (0x4dbb38) and Menu_Frame, the buttons (0x40c1f0,
   0x40c500), the mouse cursor, the fonts (FNT) and the texts; then the pages in their
   order (25 logo → 14 title → 15 name entry → main menu). Reference pictures of each from
   the original as the port reaches it (`tools/orig.sh key`).
2. A model viewer (`tools/model.py`: 3DF + MAP + texture page; which palette the texture
   pages use: they have none of their own).
3. The port's skeleton (C, SDL2, a 16-bit framebuffer at 640x480/320x240) and its first
   picture: the loading screen and the menu backgrounds from the LEZ files; then the font
   (FNT) and the menu pages in their order (25 → 14 → ...).
4. The track (.trk), the race loop, the renderer.
5. The CD's `MOTO.EXE`: line it up with GOG's `game.EXE` (which tables the port reads).
