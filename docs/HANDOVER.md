# Handover — where OpenMotoRacer stands
Read this first in a new session, then CLAUDE.md, then the NOTES sections for the area.

## Done (2026-10-26, session 1)
- Decisions agreed with Danik (CLAUDE.md "Decisions").
- `~/openmotoracer`: git, `.gitignore`, the toolkit copied (the LE-only parts left out),
  `tools/pe2flat.py`, `tools/rebuild.sh` for a PE program, `tools/bkf.py` (all banks
  unpack), `tools/raw.py`.

## Waiting for Danik
- The 1997 retail CD's files (into `~/openmotoracer/Moto Racer CD/`).

## Next, in order
1. The Ghidra run: map the program (strmap, calltree, main, the clock).
2. LEZ and BPT from their readers; the palettes; 3DF/MAP meshes; the track file.
3. The port's first picture: the title screen from the game's own files.
