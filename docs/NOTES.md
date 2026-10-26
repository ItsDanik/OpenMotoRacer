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
A packed TGA (unpacker `Lez_Unpack` 0x43eb00, the file read whole by 0x43e700). Header:
`LEZ1`, u32 unpacked size, u32 offset of the copy words, u32 offset of the literal bytes,
u32 unused, u32 the first flag word; further flag words from 0x18. Flags are taken from
the top bit down with a sentinel 1 (`word*2+1`): 1 = a literal byte, 0 = a u16 copy word,
distance = low 10 bits, length = high 6 bits + 3. The TGA: palette length at +5, width
+12, height +14, palette (B,G,R) at 0x12, 8-bit pixels top row first (whatever the TGA
descriptor says). The palette becomes 16-bit screen colours (shift/position globals
0x4ee9b8..0x4ee9cc set from the DirectDraw pixel format); a 0-entry palette uses a grey
ramp. Mode 4 (`this+0x14 == 4`) makes a 256x128 texture with every row written twice
(0x100 apart). One file is 24-bit (MENU/BITMAP/AUTORUN.LEZ, the CD autorun picture).
`tools/lez.py` (all 755 unpack; checked by eye: ILE01/LOAD, MENU/BITMAP/CHOOSEFO).

### 3.4 BPT binary scripts
The `.Spt` scripts the code names (`Data\Config.Spt`, `%sLevel.Spt`) are opened as
`.BPT` (`Script_Open` 0x4242a0, which swaps the extension). A BPT is the token stream of the
text script without its keywords: a string is a plain u32 length then its characters
(`Script_ReadString` 0x424e60), a number 4 bytes read as a float (0x424c00) or an int
(0x425080). Characters and numbers are enciphered by `Script_Decipher` 0x45a4a0: XOR with
the low byte of `Rand_Next(6)` 0x4b1160 (Park-Miller x16807 in Schrage form, a result
above 0x7fff taken mod 0x7fff), seeded with 0x1a2b3c4d when the file opens (0x4b1140).
Lengths are plain and do not advance the key. `tools/bpt.py` prints the tokens (guessing
string or number). Every BPT in data/ and the banks reads through to its end.
- `LIST.BPT`: 'PLUGIN', 0, 0, 8, then per track: name, folder, ON/OFF, a number
  (SPEED_BAY ile01, SNOW_RIDE ile05, ROCK_FOREST ile03, LOST_RUINS ile04, WEST_WAY ile02,
  GREAT_WALL ile06, RED_CITY ile08, DIRT_ARENA ile09).
- `ILEnn/LEVEL.BPT`: the track's files (`ile01.trk`, `Frames3d.Spt`, `Decor.Spt`,
  `DecorInv.Spt`, `Moto.net`, `Map.raw`, the bike scripts, `balise.spt`, `Map1.raw`, the
  20 texture pages `bank\bnk00_NN.raw` with their Direct3D twins) and numbers (the
  fields named in 0x427e80: StepGlobal, YScaleLevel, StartPosition, AI tuning, NbComputer,
  NbCheckPoint, TimeCheckPoint, AudioTrack ...).
- `CONFIG.BPT`: the settings in the order of 0x42ae30.. (DefName 'DSI', DrawLand,
  ScreenMode, FullScreen, Mode_169, ...).

### 3.5 The CD's installer
`DISK1/data.z` is InstallShield 3 (`tools/is3.py`): 58 files, all stored (packed size =
size). It holds `MOTO.EXE` (the CD's program, §1) and the same data as the CD's `DATA/`.
The CD's `DATA/` against GOG's `data/` (bank by bank, `build/cmpbkf.py`): GENERIC, ILE01-08,
KTMPKT, MENU*, PAGES, PODIUM, SFX, SPEED, SPEEDPKT hold the same files (the bank files
differ only in the junk after the names). GOG changes DEMO (2 demo files fewer),
DESIGN (3 pictures, + TEMP.ACT), FONTES (22 fonts, + DEBUG.RAW, SYMBOL.RAW), ILE09 (2
texture pages, 3 fewer), KTMCROSS (CARAC2/4.BPT, + 2 leftovers), MENU (EALOGO.LEZ), and
adds ILE10, ILE11 (not on the CD; not in LIST.BPT either).

## 4. Map of the program

### 4.1 Start and the frame
- MFC application. `entry` 0x4b6420 → `_WinMain@16` 0x4c39e0 → `AfxWinMain`. The app's
  vtable 0x4d0420 (installed at 0x413aa0): `App_InitInstance` 0x414080 (Ghidra missed it:
  it starts 0x22 bytes before its first block 0x4140a2), `App_PreTranslateMessage`
  0x4159c0, `App_OnIdle` 0x415960, `App_ExitInstance` 0x415500.
- `App_OnIdle` is the frame: `Menu_Frame` 0x409c00 while `*0x4dc0c4` (the menu) is set,
  else `Race_Frame` 0x4583d0 = the race object's (0x52c478, vtable 0x4d04c8, installed
  0x413f30) slot +0x14 = `Game_Frame` 0x47d660.
- Bikes: records of 0x3209 bytes from about 0x583fe0, 24 of them (loops end at
  0x5cf4f9).

### 4.2 The clock: a variable step
`timeGetTime` everywhere. 0x4120e0 each frame: elapsed ms since the last frame
(0x459fa0/0x459fc0) → `0x4e980c` (int) and `0x4e9808` (float); above 200 → 200, below 1 → 30.
0x47df40 then scales `0x4e9808` by 0.1 (float 0x4d1970), at least 1.0, and steps every bike
by it (0x45d540 the player, 0x45e940 the others). So **one step a frame, of the measured
time**: the port runs a fixed step (1/60 s → 1.667) which is the original at a steady 60
frames a second. `-frameratemax N` (0x4dc200) only waits in 0x459fd0 until 1000/N ms have
passed; GOG's launcher passes `-FrameRateMax60`.
