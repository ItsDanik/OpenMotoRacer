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
  They are the CD's tracks 2–13 sample for sample, the same length, shifted by 532 samples
  (2128 bytes: a ripping offset). `tools/cdbin.py` cuts the CD image into `build/cd/NN.wav`
  and `data.iso`.

- **The 1997 CD's `DATA/` against GOG's** (2026-10-26, file by file inside the banks): the
  same files apart from: `FONTES` (22 of the CD's 34 fonts differ, GOG has 2 more),
  `DESIGN` (`TEXTS\LOWRES/HIRES/GAMEOVER.RAW` differ, GOG has one more), `KTMCROSS`
  (`CARAC2/4.BPT` differ, GOG 2 more), `ILE09` (`BNK00_02.RAW`, `BNKD3D02.RAW` differ; the
  CD has 3 more), `DEMO` (the CD has 2 more), `MENU` (`BITMAP\EALOGO.LEZ`), `CONFIG.BPT`,
  `FONTS.BPT`. GOG alone has `ILE10`, `ILE11` (the patch's two tracks?) and the `*01.BPT`
  lists. Banks of equal size still differ as files: the junk after each name's zero
  differs. So the CD's program reads data the GOG program also reads, nearly all of it
  the same; its own tables (MOTO.EXE, 1997-07-21) are at other addresses (to line up).

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

### 4.3 The screen
- **16-bit colour.** The software renderer draws RGB565 (wine's trace: the 640x480 surface,
  pitch 1280, R 0xf800 G 0x07e0 B 0x1f); the game refuses a desktop that is not 16-bit
  (0x45a7c2: `GetDisplayMode` bits != 16 → "16 bit screen mode required !"). LEZ palettes
  and texture palettes become 16-bit colours through the shift globals 0x4ee9b8..0x4ee9cc.
  So the port's framebuffer is 16-bit, not palette numbers.
- Resolutions offered: 320*240, 512*384, 640*480 (0x443200).
- Cdd (screen3d/dd.cpp): the DirectDraw object 0x5d922c, width 0x5d93bc, height 0x5d9c20,
  bits 0x5d8944; the back surface 0x5d4320 locked by 0x476700 (address → 0x4eea14).

### 4.4 The menu
`Menu_Frame` 0x409c00 runs page `menu+0x3c` from the table at 0x4dbb38, 16 bytes a page:
started flag, frame routine, start routine, end routine. Pages (from their strings):
1 0x40b3f0 (loading, `%sload.lez`), 2 0x486a60 (network, DirectPlay), 3 0x442790
(championship), 5/26 0x447710 ("PRESENTS"), 6 0x448110 (track choice: `%smap.lez`,
`%sphoto.lez`), 7 0x449680, 8 0x44aee0 (`%ssmallmap.lez`), 9 0x44d540 (times, Reverse,
Pocket), 10 0x44ed30, 11 0x443200 (screen sizes), 12 0x444f30, 13 0x4459d0 (3D card
info), 14 0x446b80 (the title: `pages\titleMr` / `titleGP` 3D models), 15 0x487bb0
(name entry, `menu\font3d`), 16-18 0x44fdb0/0x450080/0x450340, 19 0x488970, 20 0x450800
and 21 0x450f40 (championship saves), 22 0x44a4f0, 23 0x4514e0, 24 0x452f80 (podium),
25 0x453c50 (the Delphine logo intro, `pages\titleDsi`).

### 4.5 3D models: 3DF + MAP
Loader 0x4216a0 (and 0x420a50, its twin for the race). **3DF**: u16 version (1), u16
vertex count, vertices as 3 floats each, multiplied by 96.0 (float 0x4d0690; Ghidra shows
an int-to-float conversion, the machine code is `fld dword` — a decompiler trap), then
faces: u16 point count n, u16 attribute, n u16 vertex numbers; a count of 0 ends the list.
In memory a face is 0x40 bytes. **MAP**: 0x200-byte header (byte 1 bit 7 must be set:
"version trop ancienne" otherwise), then 0x80 bytes a face in the 3DF's order: u16 kind
(<3 untextured; 3 triangle, 4 quad: must equal the face's point count), texture (u,v) as
ints at (+0x22,+0x32) (+0x26,+0x36) (+0x2a,+0x3a) (+0x2e,+0x3e), flags u32 at +0x48 (low
16 bits: the texture page; 0x10000, 0x20000, 0x40000 ... 0x400000 set the face's drawing
kind), u16 colour at +0x4e (RGB565). After the faces an `ANI!` block (animation).

### 4.6 Ghidra and MSVC
Routines with C++ exception handling set up their frame, end the prologue with a `nop`,
and Ghidra starts a second function after it: 81 such pairs, found by `tools/splits.py`
and joined by `JoinFunc.java` (now a step of `rebuild.sh`). Catch blocks stay separate
small functions (they call 0x4c4f71). No RTTI for the game's own classes:
`tools/vtables.py` lists the virtual tables and who installs them.

### 4.7 Menu pictures
- **The table** at 0x4e3e20: 209 records of 20 bytes, `u32 ?, u32 language, u32 id,
  char *name, u32 mode`, ended by id -1. 0x415ad2 copies it out by id into 0x53a8d8 (16
  bytes an id: ?, language, name, mode). The loaded pictures are pointers by id at
  0x504880 (0x504b70 = id 0xbc).
- **Loading** (0x43e8e0): language 0 → `data\menu\bitmap\<name>`; language 1 → the menu
  language's folder from 0x4e8b60 (`data\MenuUs\`, `MenuFr`, `MenuGr`, `MenuIt`, `MenuSp`,
  `MenuSw`; index 0x4dbce8, set by `-us`/`-fr`/...), and if that file is missing,
  `data\MenuUs\`. A `.lez` is kept packed and unpacked at its first drawing; anything else
  is a 24-bit TGA (0x43e280, rows flipped). All 209 names are `.lez`.
- **The object** (0x40 bytes): +0 x, +4 y, +8 w, +0xc h, +0x14 mode, +0x18/+0x1c the tile's
  scroll, +0x2c the 16-bit pixels, +0x30 the packed file.
- **Drawing** (0x43ede0, `this, ?, mode, pct_h, pct_w`): mode 5 = the picture's own.
  Only the bottom `pct_h`% of its rows are drawn, at y + the rows skipped; `pct_w`% of its
  columns are cut off on the right; clipped to the screen (0x655d10 x 0x624b18, pitch
  0x62ebec, surface 0x624f5c). Modes: 0 copied, 1 colour 0 left out, 2 every other pixel
  (the next row starting one pixel later, then one earlier), 3 the same with colour 0 left
  out, 4 (decided by the picture's own mode, whatever is asked) the 128x128 tile filling
  640x480 from (sx & 0x7f, sy), 0x43f0f0. Its second path (0x4dbcf4 set, aligned 8-byte
  copies) is not read yet; it looks like the same picture copied faster.
- **The title page** (14, 0x446b80) draws `title.lez` (0xbc; `titleGP.lez` 0xc0 for the GP
  edition, 0x4dc1c4) while its three 3D logo pieces fly in (`titleMr\moto`, `racer`,
  `ellipse`, each a textured quad, drawn by 0x48c030), then `title4.lez` (0xbf), which has
  the logo in it, once all three have arrived (0x5409bc = 3). "FULL D3D VERSION" blinks
  above (0x4e90e0 counts frames: drawn from 11 to 20).
- **Checked**: the port's `title4.lez` against the original's settled title page (build/ref/
  orig_p14_title.png): every pixel the same in 5-6-5 terms outside the "Start" button
  (261..378 x 431..469), not drawn yet. All 209 pictures load through the port.

## 5. Running the original (for checking)
`tools/orig.sh start|shot|key|stop`, unseen, nobody's screen needed (2026-10-26):
- A private copy in `build/orig` (with GOG's dgVoodoo `DDraw.dll`, `D3DImm.dll` and
  `dgVoodoo.conf`, which sets DesktopBitDepth=16: the game demands a 16-bit screen) and a
  private wine prefix `build/wineprefix`, run with `WINEDLLOVERRIDES=ddraw,d3dimm=n,b`.
- The hidden display is a headless **Weston with Xwayland** (`weston --backend=headless
  --xwayland --socket=mrwl`, 1024x768; its X display number read from
  `build/logs/weston.log`, its pid in `build/logs/weston.pid`). Weston is a window manager,
  so `xdotool windowactivate` can give the game's window focus: **the game stops moving
  whenever its window lacks focus**, and that is the whole reason the plain Xvfb stayed black.
- `start` waits for the window "Moto Racer - © 1997" (640x480 at -windowed) and focuses it;
  `shot FILE` takes that window (`import`); `key KEYSYM...` focuses it and types with
  xdotool (real key events: `key --window` would send synthetic ones); `stop` ends wine and
  Weston. `ORIG_DISPLAY=:1` uses Danik's own screen instead (he agreed, 2026-10-26).
- Checked: Escape skips the Delphine logo (page 25) to the title (page 14, which shows
  "FULL D3D VERSION" even with -NoD3D), Return goes to the name entry (page 15). Left alone,
  the logo leads to the attract-mode demo race (West Way). Reference pictures in `build/ref/`.

`tools/win/memtool.c` (MinGW, run under wine) reads and writes the running game's memory
(`/proc/<pid>/mem` is closed by the kernel's ptrace rule; wine's ReadProcessMemory works).
`tools/origshot.sh` (the back surface from memory, 0x4eea14) and `tools/patch_orig.py` (the
16-bit test 0x45a7c9 jumped over) are left from the attempts below; neither is needed now.

Dead ends on the hidden display (2026-10-26; never resurrect), every one black:
- Xvfb at 24 bits, windowed: "16 bit screen mode required !".
- Xvfb at 16 bits, windowed or full screen, wine's ddraw (OpenGL or GDI renderer).
- The depth test patched out, full screen on 24 bits: "Cannot initialise screen mode"
  (Xvfb cannot change mode); the same inside `explorer /desktop`.
- dgVoodoo on Xvfb; reading the back surface (0x4eea14) out of memory: 0 or black.
