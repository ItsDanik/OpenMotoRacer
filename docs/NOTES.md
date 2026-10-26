# OpenMotoRacer — the full record

Every claim carries its address and its proof. Dead ends are written down too.

## 1. The program

- `Moto Racer/game.EXE`, 1,133,056 bytes, SHA-1 `164b417553a8df4284b908405b66c269cc6d94e5`,
  PE timestamp 1998-07-09 14:15:27. GOG's copy; the readme calls it "Polygons version 3.22"
  (the patch that fixed Voodoo 2 freezes). MSVC C++ (`Microsoft Visual C++ Runtime
  Library`), source paths `D:\Dsed\mr320\` (`screen3d\bankw.Cpp`, `menu\menu.cpp`,
  `Dsi_d3d.cpp`, `frame3d.cpp`, `event.cpp`, `player.cpp`, `animDecor.cpp`, ...).
- Sections: `.text` 0x401000 (0xce600), `.rdata` 0x4d0000, `.data` 0x4db000 (0x1a400 in the
  file, 0x231568 in memory: a 2.2 MB BSS), `.idata` 0x70d000, `.rsrc`, `.reloc` (35,560
  pointer sites). Entry 0x4b6420.
- Imports: WINMM, DDRAW, DPLAYX, DSOUND, KERNEL32, USER32, GDI32, comdlg32, WINSPOOL,
  ADVAPI32, SHELL32, COMCTL32.
- Command-line switches (strings): `-NoD3D`, `-windowed`, `-fullscreen`, `-NoSound`,
  `-Pal16`, `-ZBuffer`, `-Alpha`, `-SnapShot`, `-saverecord`, `-savpath`, `-frameratemax`,
  `-nbcomp`, `-Modem`, `-fr -gr -it -sp -sw -us` (language), and many Direct3D tuning ones.
- GOG adds: `MotoRacer.exe` (Delphi launcher: `motoracer.ini` → switches for `game.EXE`),
  `DDraw.dll`/`D3DImm.dll`/`dgVoodooCpl.exe` (dgVoodoo 2), `winmm.dll` (CD audio from
  `NN.wav`), `goggame-*.dll`.
- Other builds: the 1997 retail CD (to be added; Danik will provide it).

## 2. The data (`Moto Racer/data/`)

- 28 `.BKF` banks: `MENU` (+ `MENUFR/GR/IT/SP/SW/US` per language), `PAGES`, `PODIUM`,
  `DEMO`, `DESIGN`, `FONTES`, `GENERIC`, `SFX`, the bikes (`SPEED`, `SPEEDPKT`, `KTMCROSS`,
  `KTMPKT`), and the tracks `ILE01`–`ILE11` (no 07).
- `.BPT` loose: `CONFIG`, `LIST`/`LIST01`, `CHAMPA/B/C` (+`01`), `FONTS`, `MOTOKTM`,
  `MOTOSPD`, `PKTKTM`, `PKTSPEED`. Enciphered (CONFIG.BPT and LIST.BPT share a long run of
  identical bytes after their counts, so a fixed key stream, §3.4).
- `data/ileNN/*.trk` (one a track, `TRK\0` + floats), `data/demo/EA.dem`, `BMG.dem`
  (text lists of two demo runs: track name, 90000, a picture), `bestscor.dat`,
  `config.txt` (`-us`), `delphine.err` (the game's error log).
- Music: `02.wav`–`13.wav` beside the program (the CD's audio tracks, through GOG's winmm).

## 3. Formats

### 3.1 BKF banks
u32 count; count records of 0x2c bytes: name (0x24 bytes, zero-ended; the rest is old
memory), u32 offset from the file's start, u32 size. Files follow in order without gaps
(MENU.BKF has 0x4419 unreferenced bytes after its records, and two names, EALOGO and
LODANIM, share one file). Every bank ends exactly where its last file does.
`tools/bkf.py` lists and unpacks (`-x build/bkf`). 2,200 files: 755 LEZ, 418 RAW, 250 WAV,
132 MAP, 128 3DF, 106 ROT, 52 BPT, 34 FNT, 25 GEN, 10 SKL, 9 PIV, ...

### 3.2 RAW pictures
Image Alchemy's "mhwanh": 32-byte header, big-endian: magic, u16 version (4), u16 width,
u16 height, u16 palette entries; then the palette (3 bytes an entry), then 8-bit pixels.
402 of 418 have no palette (the track and bike texture pages, 256x256 and 128x128: the
palette comes from elsewhere). `tools/raw.py`.

### 3.3 LEZ pictures
Magic `LEZ1`, then u32s (e.g. NUM0.LEZ: 0x1126, 0xc4, 0x482, then u16 0x3e, 0x35 = 62x53?).
Compressed. To do: find the decoder.

### 3.4 BPT
To do.

## 4. Map of the program
To do (Ghidra run under way).
