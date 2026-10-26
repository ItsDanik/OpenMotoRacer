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
- **MOTO.EXE's tables against game.EXE's** (2026-10-26, pefile images, `build/moto.asm`):
  the menu pictures 0x4d9840 (209 records, as GOG's 0x4e3e20), the language folders
  0x4da8a8 (GOG 0x4e8b60), the random seeds 0x4d2bb8 (GOG 0x4f2590; the first 7 equal,
  **the 8th is 0** where GOG has 0x73616c43). **No table of named colours** (GOG 0x4f0dd4):
  the 1997 build pushes R, G, B as three immediates before its colour call (e.g. 0x46723c:
  255,255,255 then 0x423620), a different font interface. So the CD's program is not
  GOG's with tables moved: its text code differs (and 22 of its 34 fonts). Open: run the
  CD build as GOG's logic with the CD's data (colours from GOG's values?), or ask CD
  players for the official 3.22 patch (whose game.EXE may be GOG's own, to check).

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

### 4.4 The menu (menu\menu.cpp, menuTools.cpp; mapped 2026-10-26)


Everything here was read from build/decomp and checked against build/mr.asm where the
arithmetic or the layout mattered. Confidence marks: **[asm]** = checked in the machine
code, **[decomp]** = read from Ghidra's text only, **[guess]** = an interpretation.
"now" always means `timeGetTime()` (import 0x70da04, confirmed with pefile: WINMM
timeGetTime). Other imports confirmed the same way: 0x70d99c ClientToScreen, 0x70d9a0
SetCapture, 0x70d9a4 ReleaseCapture, 0x70d9a8 DefWindowProcA, 0x70d9cc PostMessageA.

#### 4.4.1 The menu object

One object, 0xd4c bytes, `operator new(0xd4c)` in 0x472c40 (start-up), pointer kept in
**0x4dc0c4**. Constructor 0x4098e0. App_OnIdle 0x415960 calls `Menu_Frame` 0x409c00 while
`menu+0x00` is non-zero, otherwise `Race_Frame`.

| Off | Meaning | Who writes / reads |
|-----|---------|--------------------|
| +0x000 | **active** (1 = the menu runs, 0 = the race runs) | set 1 by 0x409f70 on first entry; 0 by ctor, by 0x409990, and by the loading page 0x40b3f0 when the race starts. Menu_Frame aborts with "MENU-Actived error" (menu.cpp line 0x141) if it is 0 |
| +0x004 | **mouse mode**: 1 = selection follows the pointer, 0 = keyboard selection kept | 1: page change (0x409eb0), WM_MOUSEMOVE (0x4336c8). 0: first entry (0x409f70), any WM_KEYDOWN except Shift (0x10) and Ctrl (0x11) (0x40a0f0) [asm/decomp] |
| +0x008 | mouse button bits of the previous Menu_Frame (copy of 0x5083b8) | end of Menu_Frame; 0 on page change |
| +0x00c | **last key**: Windows virtual-key code of the last WM_KEYDOWN | 0x40a0f0 stores it; consumed (0) by a hotkey match, by 0x40c1e0, by page change, by Alt+Enter; pages read it directly (name entry 0x487bb0 does its own typing from it) |
| +0x010 | **selected** (highlighted) button index, -1 none | hover (0x40c0c0), Tab/arrows, hotkeys; -1 on page change and in ctor |
| +0x014 | previous selected (to play the hover sound once) | 0x4099a0 |
| +0x018 | **pressed** button index, -1 none (the "action" the page reads) | click (0x40a460), held repeat (Menu_Frame), Return/Space, hotkey; read via 0x40c1b0, cleared by 0x40c1e0 |
| +0x01c | transition start time (ms) | Menu_Frame |
| +0x020 | transition end time = start + 0x4b0 (1200 ms) | Menu_Frame |
| +0x024 | **last activity time** (ms) for the 30 s demo timeout | page change (0x409f70), mouse move, left click (0x40a460), left button held (Menu_Frame), any key (0x40a0f0) |
| +0x028 | set 0 at transition start; no reader found in the menu routines | |
| +0x02c | transition progress, float 0..1 | 0x40c500 |
| +0x030 | redraw/flip request | set 1 by many; 0x49b1c0 flips only if set, then clears it. 0x4099a0 sets it to 1 unconditionally every frame [asm 0x4099e7], so in practice **every frame is flipped** |
| +0x034 | "force redraw" | 0x409bf0 sets +0x34 and +0x30; 0x4099a0 clears +0x34 |
| +0x03c | **current page** (index into 0x4dbb38) | 0x409f70 |
| +0x040 | **button count** (0..32) | 0x40bd40 clears; 0x40bd60/0x40bef0 add |
| +0x044 | 32 button records of 0x58 bytes (+0x044 .. +0xb44) | see §4.4.3 |
| +0xb44 | set 0 at every page start; no reader found [asm: only write 0x409cc7] | |
| +0xd48 | **transition running** | Menu_Frame sets, 0x40c500 clears after 1200 ms |

Constructor 0x4098e0 [decomp]: constructs the 32 records (0x409980 is an empty ctor),
active=0, +4=0, page=0; fills the 2048 bytes 0x5da298..0x5daa98 with `i>>3` (each value
0..255 eight times: a divide-by-8 table); allocates 0x2108 bytes into 0x504e70
(0x43e1c0); loads the language strings/pictures table (0x415a70 with the language index
0x4dbce8); +0x30=1, +0xd48=0, +0x34=0, +0x40=0, +0x10=-1.

Globals of the menu (outside the object):
- **0x5083b8** mouse buttons: bit 1 (value 2) = left button down, bit 0 (value 1) = right
  button down (§4.4.4) [asm].
- **0x519fe8 / 0x519fea** mouse x / y, signed 16-bit (all readers use `movsx WORD`) [asm].
- **0x4dbfc8** modifier bits: 1 = Alt, 2 = Shift, 4 = Ctrl (set on key down in
  0x40a0f0, cleared on key up in 0x432700; WM_SYSKEYUP clears the whole word). Alt never
  arrives as WM_KEYDOWN (it is a WM_SYSKEYDOWN, and App_PreTranslateMessage 0x4159c0
  swallows WM_SYSKEYDOWN with wParam 0x12), so bit 1 is probably never set [guess].
- **0x504e78** "the keyboard navigated on this page" (Tab/arrows set 1; page change and a
  hotkey match clear it).
- **0x504e74** "a page change is in progress": 1 from 0x409eb0 until the new page's start
  routine has run. While 1, page frames throw the 3D list away (0x49ad70) instead of
  drawing it (0x49ad90).
- **0x4dbd00** "capture taken, do the transition on the next page start" (set by
  0x40c4a0, consumed by Menu_Frame).
- **0x504e84**, **0x504e7c**: every write found stores 0 (ctor, 0x40bd40, Menu_Frame,
  0x40a0f0, 0x40a460) [asm grep], so the tests `0x504e84 != 0 → no input` never fire in
  this build. **0x504878** = ftol(now + 300.0) (double 0x4d02a0) is written at every
  press and never read [asm grep]. The port can drop all three.
- **0x504880..0x504bc4**: the 209 menu picture pointers by id (§4.7). Picture
  object fields used here: +0 x, +4 y, +8 w, +0xc h, +0x10 "slide animation running",
  +0x2c unpacked pixels, +0x30 packed file, +0x44..+0x60 the slide (§4.4.3.6).

#### 4.4.2 Pages

##### 4.4.2.1 The page table 0x4dbb38 (dumped from mr_flat.bin)

16 bytes a page: `u32 started, frame routine, start routine, end routine`
(Menu_Frame reads +4 as the per-frame routine, +8 as start, +0xc as end) [asm/decomp].
Entry 27 would start at 0x4dbce8, which is the language index: 27 pages, 0..26.

| # | addr | frame | start | end | what (strings / exits) |
|---|------|-------|-------|-----|------|
| 0 | 4dbb38 | 0 | 0 | 0 | idle (page 0 = nothing) |
| 1 | 4dbb48 | 40b3f0 | 0 | 0 | loading (`%sload.lez`); ends the menu: page 0, `menu+0 = 0` → race |
| 2 | 4dbb58 | 486a60 | 486860 | 486a40 | network (DirectPlay) |
| 3 | 4dbb68 | 442790 | 4422c0 | 442730 | **main menu**: Solo / Link / Options / Exit (see §4.4.5) |
| 4 | 4dbb78 | 40bc40 | 0 | 0 | quit: tears the menu down, PostMessage(hwnd, WM_CLOSE) |
| 5 | 4dbb88 | 447710 | 447250 | 0 | ("PRESENTS"; reached by Solo) |
| 6 | 4dbb98 | 448110 | 447a30 | 4480d0 | track choice (`map.lez`, `photo.lez`) |
| 7 | 4dbba8 | 449680 | 449100 | 44a370 | |
| 8 | 4dbbb8 | 44aee0 | 44a930 | 44aeb0 | (`smallmap.lez`) |
| 9 | 4dbbc8 | 44d540 | 44cfd0 | 44d510 | times, Reverse, Pocket |
| 10 | 4dbbd8 | 44ed30 | 44e480 | 44ecf0 | |
| 11 | 4dbbe8 | 443200 | 443000 | 0 | options (screen sizes, `List*.bpt`); main menu "Options" goes here |
| 12 | 4dbbf8 | 444f30 | 444b10 | 0 | |
| 13 | 4dbc08 | 4459d0 | 4458a0 | 0 | 3D card info |
| 14 | 4dbc18 | 446b80 | 4465f0 | 446b40 | **title** |
| 15 | 4dbc28 | 487bb0 | 487070 | 488790 | **name entry** |
| 16 | 4dbc38 | 44fdb0 | 44ffd0 | 450050 | |
| 17 | 4dbc48 | 450080 | 450290 | 450310 | |
| 18 | 4dbc58 | 450340 | 450550 | 4505d0 | |
| 19 | 4dbc68 | 488970 | 488850 | 488840 | |
| 20 | 4dbc78 | 450800 | 450600 | 0 | championship saves |
| 21 | 4dbc88 | 450f40 | 450d20 | 0 | championship saves |
| 22 | 4dbc98 | 44a4f0 | 44a390 | 44a4e0 | |
| 23 | 4dbca8 | 4514e0 | 451d10 | 451fc0 | |
| 24 | 4dbcb8 | 452f80 | 452240 | 452eb0 | podium |
| 25 | 4dbcc8 | 453c50 | 4537d0 | 453bf0 | **Delphine logo intro** (`pages\titleDsi`) |
| 26 | 4dbcd8 | 447710 | 447250 | 0 | same routines as 5 (main menu Solo when 0x442740 fails) |

The old page list called page 3 "championship": page 3 is the main menu (its buttons are SOLO_D/SOLO_OK,
LINK_D/LINK_OK, options.lez, exit.lez), not "championship"; its start routine only reads
`Data\champ%c.spt` [decomp, picture names from the table 0x4e3e20].

##### 4.4.2.2 Changing page: 0x409eb0 (menu, page) [decomp, matches asm]

`ChangePage` = 0x409eb0, called by every page that leaves (thiscall, menu in ECX):
1. 0x45a190(0x4dbf84): sets the Windows cursor back to the arrow (port: nothing).
2. 0x417750(0x53b8e8): deletes every object of the 48-slot text/sprite list (§4.4.6).
3. For every picture pointer in 0x504880..0x504bc4: if it has a packed file (+0x30) and
   unpacked pixels (+0x2c), the pixels are freed and +0x2c = 0 (pictures are re-unpacked
   on demand: memory only, invisible).
4. selected (+0x10) = -1; 0x504e78 = 0; mouse mode (+4) = 1; 0x504e74 = 1.
5. If the *current* page's started flag is set: call its end routine (if any), clear
   the flag.
6. If 0x4ee954 (Direct3D only): 0x4dc070 = 1, 0x4e9cf8 = 1.
7. 0x409f70(menu, page).

0x409f70 (menu, page) — "SetPage" [asm checked for the order]:
- **If the menu is not active** (first entry, or coming back from a race): if a frame is
  locked (0x4f1a20) end it (0x49b0e0(0,0)); save 0x4dc0d4 → 0x504e80, 0x4dc0c8 → 0x504bc4,
  0x4dc028 → 0x50487c and set 0x4dc0d4 = 0, **0x4dc0c8 = 2** (the 640x480 mode index),
  0x4dc028 = 0; viewport 0x499ba0(0,0,640,480,0,0); far plane 0x6d1f70 = 40000.0
  (0x471c4000); 0x62e444 = 1; focal lengths 0x49a0e0(1911.0), 0x49a100(1911.0); 0x625774 =
  256.0; re-lock if it was locked; **0x40a9a0 loads the menu's resources** (pictures, the
  vitre mesh 0x504c70, textures); active = 1; selected = previous = pressed = -1; mouse
  mode = 0; 0x4961d0(0); 0x404d50().
- Else, in Direct3D mode only, may set 0x4dbcec (display reset) [decomp].
- Always: +0x24 = now; started[page] = 0; **+0x3c = page**; +0xc = 0; **0x5083b8 = 0**;
  +8 = 0. (So the mouse button state is forgotten at a page change: holding the button
  across a change needs a new click.)

The new page's start routine does not run here: it runs at the next Menu_Frame.

0x409a30 (demo start, §4.4.5.2) and 0x40bc40 (page 4, quit) contain inlined copies of
0x409eb0. Quit = the same teardown, SetPage(0), then the end routine of **every** page
whose started flag is set, then `PostMessageA(hwnd 0x68b028, WM_CLOSE 0x10, 0, 0)`.

##### 4.4.2.3 Menu_Frame 0x409c00 (fastcall, ECX = menu) [asm]

1. If active == 0 → fatal error, exit.
2. 0x50bbc8 = now (the sounds' clock); 0x4178c0(now): 0x4e4ed0 = now (the animation
   clock of the text/sprite objects).
3. If 0x4e8918 != 0: 0x4587e0(0x4e8918) (network/session event flags; may change page:
   0x18, 0xe, 5) — out of scope.
4. **Page start** (if started[page] == 0): button count = 0; 0x504e84 = 0; +0x30 = 1;
   every picture's +0x10 (slide running) = 0; +0xb44 = 0; call the start routine if any,
   and then 0x504e74 = 0 (only when there is a start routine: pages 1 and 4 leave it 1);
   started = 1; **if 0x4dbd00**: start the transition (§4.4.2.4) and clear 0x4dbd00.
5. Call the page's frame routine (if any).
6. **Demo timer**: if page == 14 and the demo list has entries (0x53e5f2, a field of the
   demo list object 0x53e4d0) and (now − menu+0x24 > 30000 (unsigned) or 0x4dbf80, the
   `-d` switch): 0x409a30; if that fails 0x53e5f2 = 0 (never tried again).
7. If 0x4e9ce4 (Alt+Enter was pressed): clear it, 0x499ed0(0x4dbff0, 0) (re-set the
   display mode, windowed/fullscreen), +0xc = 0.
8. **Held left button repeat**: if (0x5083b8 & 2) and (+8 & 2) (down now *and* last frame):
   +0x24 = now; and if mouse mode and selected != -1 and that button's flags (+0x54 in
   the record, menu+0x98+i*0x58) bit 0: pressed = selected (every frame while held).
9. +8 = 0x5083b8; 0x404eb0 (sound channels update).

##### 4.4.2.4 The transition: a pane of glass shattering towards you

Taken in two halves:
- **Leaving**: a page calls 0x40c4a0 just after its last frame is drawn (and before or
  after 0x409eb0). 0x40c4a0: 0x4dbd00 = 1; 0x479260(0x4dc0c0, 2) copies the back buffer
  into texture slot 2, **640x480 → 256x192**: from every 5 pixels of a row it keeps the 1st
  and 3rd (x = 0,2,5,7,10,...), from every 5 rows the 1st and 3rd (rows 0,2,5,7,...) [asm
  loop 0x479260: 10-byte step, two rows per 5]; then 0x41b5b0(2, {0,0,256,256}, 0x4f9c00)
  hands it to the texture system. Pages that change *without* calling 0x40c4a0 get no
  transition (title 14 → 15 has none; 25 → 14 and 15 → 3 have one).
- **New page start** (Menu_Frame step 4): +0xd48 = 1; +0x1c = now; +0x20 = now + 1200;
  0x47b1d0(0x504bc8, (0,0,0)); 0x47a340(*0x504c70, (0,0,0), (0,0,1911)); +0x28 = 0;
  sound 0x2a (0x4049f0(0x2a, 0)).
- **Each frame**, the page calls 0x40c500 (after drawing its buttons, before the 3D list
  is drawn): if +0xd48: if now < +0x20: `t = (now − start) × 0.000833333` (float
  0x4d02b4 = 1/1200) → +0x2c, and 0x479c10 adds the shards to the 3D list; else +0xd48 =
  0. While +0xd48 is set no button can be pressed (0x40c1b0 returns -1, hotkeys are
  ignored).

The shards (0x504c70, a 0x94-byte object, created in 0x40a9a0 from
`data\menu\meshes\vitre.3df`, prepared by 0x479630 in menuTools.cpp):
- vitre.3df: 59 vertices, **99 triangles**, x −320.4..320.4, y −240.2..240.2, z −0.69
  (loader 0x420590 does *not* multiply by 96 [asm: no 0x4d0690]). It is the 640x480
  screen cut into triangles.
- For each triangle: centre `(cx, cy) = (sum of the 3 x, sum of the 3 y) × 1/3`, z = 0
  → obj+0x8c (12 bytes a piece); the vertices become relative to the centre (z = 0).
  Spin speeds (obj+0x90, 3 floats) from three `Rand_Next(1)` draws r1, r2, r3 (0x458cd0;
  stream 1, seeded from the program's table 0x4f2590 = 0x89696798, **0xbadebeaf**,
  0x39859835, 0x33243094, 0x1a3d5e7f, 0x45f8a6c1, 0x12345678, 0x73616c43; before these the
  mesh loader 0x420590 has drawn one number per face (99, at 0x42089f) and nothing else
  draws from stream 1 before the first transition; the port with that sequence matches the
  original's captures of 25 -> 14, the pieces each turning their own way):
  `spin[2] = (r1 − 32767) × (1/32767) × 5.340707511` (1.7π),
  `spin[1] = (r2 − 32767) × (1/32767) × 4.712388980` (1.5π),
  `spin[0] = (r3 − 32767) × (1/32767) × 5.654866776` (1.8π) [asm 0x479931..0x479985].
- Texture coordinates, per vertex, truncated to integers: `u = (x + 320) × 256 × 0.0015625`
  (= 0..256), `v = (y + 240) × 192 × (−0.00208333) + 192` (= 192..0, y up) [asm,
  floats 0x4d1850..0x4d1864]. Each triangle becomes two faces (both windings, so it is
  seen from both sides), texture slot 2.
- Drawing at progress t (0x479c10) [asm, FPU stack traced]: angles `a_k = t × spin[k]`;
  `idx_k = ftol(a_k × 2607.594587618272)` (16384/2π); the table 0x5fcb80 is a 16384-entry
  **cosine** table (built at 0x4901e0 with `fcos`), so `c_k = T[idx_k & 0x3fff]`,
  `s_k = T[(idx_k − 0x1000) & 0x3fff]`. Matrix written to 0x5daa98 (row-major as stored):
  `[c1c2, s0s1c2 + c0s2, s0s2 − c0s1c2;  −c1s2, c0c2 − s0s1s2, s0c2 + c0s1s2;  s1, −s0c1,
  c0c1]`, translation `(cx, cy, 1911 − 1850 t)` (floats 0x4d1870 = 1911, 0x4d186c =
  −1850). 0x49b2c0(matrix, the 3 vertices, 3, 0x1f) transforms, 0x49b7f0 queues the face
  (into the same z-sorted list as the page's 3D objects). With the focal length 1911 a
  piece at z = 1911 covers exactly its screen pixels; over 1.2 s the pieces come towards
  the viewer (z 1911 → 61) while tumbling. ftol = truncation towards zero.
- The matrix goes straight to 0x49b2c0 with no camera (the pieces are in the view's frame)
  and is the same as 0x48c030's rotation, here with the constant 2607.594587618272
  (0x4d1878); translation (cx, cy, 1911 - 1850 t). The faces (0x422f80): kind 16 (opaque),
  page 2, corners (0,1,2) and (0,2,1). The loader does not x96 nor mirror. [asm, px:
  captures of the original fitted to the port's frames, 3-13 % of pixels differ, all
  sub-frame timing of pieces moving fast]

#### 4.4.3 Buttons

##### 4.4.3.1 The record (0x58 bytes, record i at menu + 0x44 + i×0x58, at most 32)

Offsets relative to the record (absolute menu offsets for i = 0 in brackets) [decomp of
both creators, the drawing and the hit test, asm for the drawing]:

| Rel | (abs) | Field |
|-----|-------|-------|
| +0x00..+0x0c | (+0x44..+0x50) | text button rectangle x0, y0, x1, y1 (filled by 0x492a90 when created) |
| +0x10 | (+0x54) | **hotkey**: virtual-key code, -1 = none |
| +0x14 | (+0x58) | **kind**: 1 = text, 2 = picture |
| +0x18 | (+0x5c) | text (char*) — text buttons |
| +0x1c | (+0x60) | font object — text buttons |
| +0x20 | (+0x64) | **normal picture** — picture buttons |
| +0x24 | (+0x68) | **highlighted picture**, 0 = none |
| +0x28 | (+0x6c) | text style flags (passed to the measuring and the drawing) |
| +0x2c, +0x30 | (+0x70, +0x74) | text x, y (0 for picture buttons) |
| +0x34, +0x38 | (+0x78, +0x7c) | picture position x, y; -1 = leave the picture where it is (not written by the text creator) |
| +0x3c | (+0x80) | always -1; no reader found |
| +0x40 | (+0x84) | **neighbours** for the arrows: byte 0 = Left, byte 1 = Right, byte 2 = Down, byte 3 = Up; 0xff = none; -1 = no arrows |
| +0x44 | (+0x88) | **group**, -1 = none |
| +0x48 | (+0x8c) | pointer to an int option variable, 0 = none |
| +0x4c | (+0x90) | value: the button shows "on" when `*var == value` (radio buttons) |
| +0x50 | (+0x94) | **blink** flag (not written by the text creator) |
| +0x54 | (+0x98) | **flags**: bit 0 = repeats while the left mouse button is held |

##### 4.4.3.2 Creating buttons

- 0x40bd40 (menu) = clear the page's buttons: count = 0, 0x504e84 = 0, +0x30 = 1. Every
  start routine calls it first.
- **0x40bef0** (menu, pic, hlpic, neighbours, hotkey, group, value, varptr, blink, x, y,
  flags) — picture button (kind 2). Fatal "MAX BUTTON reached" (line 0x682) if count > 31.
  Unpacks the LEZ of both pictures now if needed. Writes rect/text/font/style/textpos 0,
  +0x3c = -1.
- **0x40bd60** (menu, int pos[2], text, font, neighbours, style, hotkey, flags) — text
  button (kind 1): 0x492a90(text, &rect, x, y, style) measures it into the rectangle;
  pictures 0, group -1, varptr 0, value 0; +0x34/+0x38/+0x50 are not written (stale).
  Used by options (11) and network (2) only.

Examples [decomp]: title (0x4465f0): start.lez (id 0x4c, *0x5049b0) placed at (0, 430)
with alignment 1 (x centred: (640 − w) >> 1), one button, no highlight, hotkey Return
(0xd). Delphine intro (0x4537d0): continue.lez (id 0x4d, *0x5049b4) placed at (−300, −300)
(off screen, never drawn visibly, never hovered), hotkey Escape (0x1b). Main menu
(0x4422c0): Solo (SOLO_D id 4 / SOLO_OK id 5, neighbours 0xff0201ff), Link (LINK_D 6 /
LINK_OK 7, 0xff03ff00), Options (id 0x4b, no highlight, 0x00ff03ff), Exit (id 0x49, no
highlight, 0x01ffff02, hotkey Escape): a 2x2 grid Solo|Link over Options|Exit; plus up to
two optional extra buttons without neighbours: Pocket (poc_D id 0x59 / poc_L id 0x58,
when 0x4dc03c or 0x4dc054) and Reverse (rev_D 0x5b / rev_L 0x5a, when 0x4dc040 or
0x4dc050) — probably unlocked by the name-entry cheats [guess].

Picture placing helpers: 0x43fa40(pic, int pt[2], align) sets pic x,y: align 0 = as
given, 1 = x = (640 − w) >> 1 (y as given), 2 = centred on the point (x − w>>1, y − h>>1);
it also writes the adjusted point back. 0x43faa0(pic) returns pic itself (= a pointer to
its x,y).

##### 4.4.3.3 Selecting with the mouse (every frame)

0x49a160(0,0) (frame begin, called by every page) ends with 0x4099a0(menu) when the menu
is active [asm]:
- +0x30 = 1 (always); +0x34 = 0.
- `sel = 0x40c0c0(menu)`; +0x10 = sel; if sel != +0x14: +0x14 = sel, and if sel != -1
  play sound **0x29** (the hover tick).

0x40c0c0 [decomp, coordinates asm]: in keyboard mode (+4 == 0) returns +0x10 unchanged.
In mouse mode: for i = 0.. count−1, the first button under the pointer wins:
- picture button: first moves its normal picture to (+0x34, +0x38) if both != −1; hit if
  `pic.x <= mx <= pic.x + pic.w` and `pic.y <= my <= pic.y + pic.h` (inclusive both ends);
- other kinds: hit if `x0 <= mx <= x1` and `y0 <= my <= y1` (the text rectangle).
No hit → selected = −1. So in mouse mode the highlight disappears when the pointer is
off every button.

##### 4.4.3.4 Pressing

Pressing = writing the index into **pressed (+0x18)**. Ways [decomp; asm where marked]:
- **Left button down** (WM_LBUTTONDOWN → 0x40a460, called from the window procedure only
  when the menu is active): +0x24 = now; if mouse mode and selected != −1: pressed =
  selected, sound **3**. The press happens on the button going *down*. [asm 0x4336fa]
- **Held**: Menu_Frame step 8, only for buttons with flags bit 0, every frame from the
  second frame of holding.
- **Keys** (0x40a0f0, §4.4.4.2): a button's hotkey; Return or Space on the selected button.

Pages read the press with **0x40c1b0** (menu): returns −1 while the transition runs
(+0xd48) or if 0x504e84, else +0x18. After acting they call **0x40c1e0** (menu): last key
(+0xc) = 0 and pressed = −1. A press stays pending until the page clears it.

##### 4.4.3.5 Drawing: 0x40c1f0 (menu), once a frame by every page [asm for the state table]

1. For every picture button: 0x43fab0(normal picture, 0,0,0,0) = step its slide (§4.4.3.6).
2. `g` = group of the selected button (−1 if none selected).
3. Only if +0x30 (always, see §4.4.1): toggle the blink phase 0x4dbd08 (^= 1, so blinking
   buttons blink at half the frame rate), then for each button:
   - **text** (kind 1): selected → font colours (255,255,0) (yellow, two calls 0x490a60 and
     0x490a30), else (255,255,255) white; font vtable slot 1 (1, 0xff); 0x493ee0(font,
     text, x, y, style, 0,0,0,0) draws it (the text system is described elsewhere).
   - **picture** (kind 2): move the normal picture to (+0x34,+0x38) if both != −1. State
     bits: 4 if g != −1 and this button's group == g (includes the selected one itself);
     2 if this is the selected one; 1 if varptr and *varptr == value. If the blink flag is
     set and the blink phase is 1: state = 0 (`and ecx,0xfffffff0`). Then (jump table
     0x40c474, mode numbers of 0x43ede0, §4.7):

| state | drawn |
|-------|-------|
| 0 | normal picture: mode 5 (its own) if the button has a highlight picture, else mode **3** (every other pixel, colour 0 transparent = dimmed) |
| 1, 6, 7 | highlight picture moved to the normal picture's x,y, mode 5 |
| 2 | highlight picture at the normal's x,y, mode 5; without a highlight picture: normal, mode 5 |
| 3 | **nothing** (selected and on, no group: never drawn — literal reading of the table) |
| 4 | normal picture, mode **2** (every other pixel, colour 0 drawn) |
| 5 | highlight picture at the normal's x,y, mode 2 |

All drawn on the menu surface 0x4dc0c0 with pct_h = 100, pct_w = 0 (whole picture).
So with highlight pictures (Solo/Link) the plain one is shown until hovered; without
(Options/Exit/Start) the button is shown half-dissolved and becomes solid when selected.

##### 4.4.3.6 Picture slides (buttons sliding in)

0x43f9a0(pic, int target[2], float seconds, kind, align): start = now (+0x54), duration
= ftol(seconds × 1000.0) (+0x58), end (+0x5c); target adjusted by align like 0x43fa40;
delta (+0x4c, +0x50) = target − current; from (+0x44, +0x48) = current; +0x10 = 1 (running);
+0x60 = kind. 0x43fab0 each frame (for mode-4 tile pictures it scrolls the tile instead):
`n = min(now, end)`, `f = (n − start) / duration` (float); kind 0:
`x = ftol(from + delta × f)`; kind 1: `x = ftol(from + delta × (sin(f ×
π − π/2) + 1) × 0.5)` (ease in-out); same for y; when n == end, +0x10 = 0 [asm 0x43fb04..].
The main menu starts slides for its pictures with 1.0 s, kind 1 (0x4422c0).

#### 4.4.4 Input from Windows

Window procedure at 0x4329a5 (the message switch from 0x432ecc; jump tables 0x433880 for
0x101..0x105 and 0x433894 for 0x200..0x205) [asm]:

##### 4.4.4.1 Mouse
- **WM_MOUSEMOVE** (0x433637): 0x519fe8 (dword) = lParam (x low word, y high word);
  ClientToScreen into 0x51be94/0x51be96 (screen x,y; previous kept in 0x509220); deltas
  0x519ff4/0x519ff6; reset of the cursor timers 0x4dbfb4 = 3000, 0x4dbfb0 = 0. If the menu
  is active: mouse mode (+4) = 1, +0x24 = now. If fullscreen (0x4dbff0): 0x519fe8 =
  0x51be94 (screen coordinates = game pixels).
- **WM_LBUTTONDOWN** (0x4336fa): 0x516aa4 = old bits; 0x5083b8 |= 2; 0x53eb18 = 1,
  0x53ead8 = 0; SetCapture; if the menu is active: 0x40a460 (press, §4.4.3.4).
- **WM_LBUTTONUP** (0x43375a): 0x53eb18 = 0; 0x5083b8 &= ~2; ReleaseCapture.
- **WM_RBUTTONDOWN** (0x43377e): 0x5083b8 |= 1; 0x53ead8 = 1; SetCapture.
  **WM_RBUTTONUP** (0x4337bd): 0x5083b8 &= ~1; ReleaseCapture. The menu core ignores the
  right button.

##### 4.4.4.2 Keyboard
- **WM_KEYDOWN** (0x43351f) → 0x431250(hwnd, wParam, lParam): if the menu is active it
  only calls **0x40a0f0(menu, vk, 0)** and returns [asm 0x431250..0x431275]; otherwise the
  race's key table. Windows' auto-repeat makes held keys repeat.
- **WM_KEYUP** (0x433535) → 0x432700: menu active → clear Shift/Ctrl/Alt bits of 0x4dbfc8.
- **WM_CHAR** (0x433547): 0x51be90 = the character, 0x50bbb0 = scan code (lParam >> 16).
- **WM_SYSKEYDOWN** (0x43355f): only Return, and only if 0x4e8934 == 1: toggles 0x4dbff0
  (fullscreen ↔ window) and sets 0x4e9ce4 (Menu_Frame step 7 re-sets the display).
  **WM_SYSKEYUP**: 0x4dbfc8 = 0.

0x40a0f0 (menu, vk) [decomp, branch order matches the asm]:
1. +0x24 = now; +0xc = vk; unless vk is Shift (0x10) or Ctrl (0x11): mouse mode = 0.
2. (0x504e84 test: never true.)
3. **Hotkeys**, only if no transition and nothing pressed yet (+0x18 == −1): for every
   button whose hotkey == vk: if 0x504e78 (arrows/Tab used) and vk == Return and Alt not
   held → stop looking (Return then acts on the selection, step 4); else selected =
   pressed = i, +0xc = 0, 0x504e78 = 0. The loop does not stop at a match (the last
   matching button wins). No sound.
4. Then on +0xc (0 after a hotkey match, so nothing more happens):
   - **Tab (9)**: Shift held → selected − 1 (below 0 → count − 1), else selected + 1
     (== count → 0); 0x504e78 = 1.
   - **Return (0xd) / Space (0x20)**: if no transition, Alt not held and selected != −1:
     pressed = selected (no sound here).
   - **Shift (0x10)** / **Ctrl (0x11)** / **Alt (0x12)**: set bit 2 / 4 / 1 of 0x4dbfc8.
   - **Left (0x25) / Right (0x27) / Down (0x28) / Up (0x26)**: if selected == −1: selected
     = (count == 0) (i.e. 0 when there are buttons); else n = byte 0 / 1 / 2 / 3 of the
     selected button's neighbours; n == 0xff → unchanged, else selected = n. 0x504e78 = 1.
   - **Insert (0x2d)**: if 0x4e8928 (a debug switch) 0x471110(0x5d2dd8,1,0): screenshot
     to TGA files.
   - **Escape** has no code of its own: pages give a button the hotkey 0x1b.

#### 4.4.5 Start-up and the first pages

##### 4.4.5.1 How the menu starts [decomp]
App_InitInstance 0x414080 → 0x473520 → 0x472c40: after the screen and sound set-up it
creates the menu object (new 0xd4c, ctor 0x4098e0) into 0x4dc0c4, 0x4311c0, 0x4e9834 = 0,
loads the demo list `Data\Demo\EA.dem` (`Data\Demo\BMG.dem` for the GP edition 0x4dc1c4)
into 0x53e4d0 (0x42e370; 0x42e6d0 if it has entries), then **ChangePage(menu, 25)**. As the
menu is not active yet, 0x409f70 does the first-entry set-up (§4.4.2.2). App_OnIdle then runs
Menu_Frame every idle loop. (0x415b20, also called from InitInstance, builds a temporary
menu object only to load the language tables, then deletes it.)

##### 4.4.5.2 The chain 25 → 14 → 15 → 3 (every exit found by scanning all 0x409eb0 calls)

- **25, Delphine logo** (0x453c50): six 3D pieces (`titleDsi` camera, D, S, I, DsInt)
  animate; CD track 13 (0x404990(0xd, 0)) starts in its start routine. Each frame
  `dt = now − last`, if dt > 500 → 100 (float 0x4d0bf4 = 500.0); each piece's progress
  += dt / (its duration × 1000.0). Ends when **all 6 have arrived** (0x53f788 == 6) or its
  only button is pressed: the invisible "continue" button, hotkey **Escape** (also
  Tab/arrow then Return/Space; it cannot be clicked). → `0x409eb0(14)`, then after the
  frame is finished `0x40c4a0` (so 25 → 14 **has the glass transition**).
- **14, title** (0x446b80): the three logo pieces fly in, then title4 with blinking
  "FULL D3D VERSION" (§4.7); CD track 3 (0x404990(3,2)) and sound 0x2d at its start.
  One button "Start" (Return hotkey, clickable). Pressed → `0x40c1e0`, `0x409eb0(15)`
  with **no transition** (no 0x40c4a0). Idle 30 s (no mouse move, click or key; the
  clock restarts at each page change) with demos available, or `-d`: **demo** (0x409a30):
  opens the demo list, takes the next entry (0x42e580 → track/time/picture into 0x508430,
  name into 0x542891), sets up the race (0x457bf0), 0x4dbffc = 1, and does ChangePage to
  **page 1** (loading) inline. On failure: list closed, 0x4e8944 = 1, and with `-d` the
  box "Error in demo script". Escape does nothing on the title.
- **15, name entry** (0x487bb0): a 3D grid of 28 cells, 7 a row (26 letters, 26 = delete,
  27 = OK; cursor 0x5f647c — an int that Ghidra shows as a float: 3.64338e-44 = 26,
  3.78351e-44 = 27); name in 0x5f6518, length 0x5f1a44 (max 8). It reads menu+0xc itself
  (not buttons): first PageUp/PageDown/End/Home (0x21–0x24), numpad and F-keys
  (0x60–0x6b, 0x6f–0x7b) are discarded; **Backspace (8)** deletes the last letter;
  **Return** (no Alt) confirms if the name is not empty; **Space** = act on the cell under
  the cursor (letter, delete, OK); **arrows** move the cursor (Left/Right ±1 wrapping
  0..27, Up/Down ±7 wrapping by ±21); **A–Z** (0x41–0x5a, also 0x61–0x7a) type the letter.
  On confirm the name goes to 0x5161b0 and is compared (case-insensitive) with the cheat
  table 0x4f1250 (0x4f1248 entries of {name, int *var, value}, first "CESREVER"): a cheat
  sets its variable and the page restarts (`0x409eb0(15)`, no transition); a real name
  → `0x40c4a0` then `0x409eb0(3)` (with transition). Sound 3 on each accepted key.
- **3, main menu** (0x442790): Solo (button 0): voice 0x55 (once, 0x53f554 = 1), then
  nothing happens until now > 0x53f78c = ftol(now + 1000.0) (double 0x4d0b68) [asm]
  (the press stays pending meanwhile); then 0x4dbffc = 1, glass capture, a CD-audio check loop (0x4723c0 asks for
  the CD, may drop out of fullscreen), then page **5** if 0x442740 succeeds, else **26**.
  Link (1): voice 0x56, the same 1000 ms wait, then 0x4dbffc = 0, transition, page **2**. Options (2):
  transition, page **11**. Exit (3, also **Escape**): page **4** (quit, no transition).
  Arrows move in the 2x2 grid; Tab cycles. The two optional buttons toggle 0x4dc078
  (Pocket) / 0x4dc068 (Reverse) in place (0x40c1e0 + 0x409bf0, no page change).

##### 4.4.5.3 The main menu's layout (0x4422c0, 0x442790) [asm for the press handling]
- Start: title bar (id 3) at (0,-80) slides to (0,0); Solo (4/5) from (-140,116) and Link
  (6/7) from (780,116) slide to (56,177) and (363,177) — y 116 when Pocket or Reverse is
  unlocked; all 1.0 s eased. Options (0x4b) at (240,500), Exit (0x49) at (580,500); the
  frame after Solo's slide has ended they slide to (240,420) and (510,430) in 0.7 s
  (= 699 ms: the float 0.7 times the double 1000.0, truncated). Pocket button (0x59/0x58)
  at (227,288) or (352,288) with Reverse; Reverse (0x5b/0x5a) at (202,293) or (72,293)
  with Pocket. Their switch labels (0x5c+0x5f off, 0x5d+0x5e on) each frame at
  (248|373, 362) + (328|453, 362) for Pocket and (250|120, 363) + (330|200, 363) for
  Reverse.
- Frame: hover; Options/Exit rise; tile (id 0) scrolled (1,1) a frame and drawn; title
  bar; buttons; switches; glass; pointer. Presses (esi = 0x40c1b0): if the voice's wait
  0x53f78c is set (not -1) and now <= it, nothing more. Else 0 Solo → done, 0x4dbffc = 1,
  then (after the toggles' tests) glass capture, the CD check loop, 0x442740 (the copy
  protection: the CD's track 2 must be 0x3e0c04 long) → page 5, else 26; if the CD check
  loop gives up, no page change and the capture is dropped. 1 Link → 0x4dbffc = 0, glass,
  page 2. 2 Options → glass, page 11. 3 Exit → page 4. Then pressed == 0x53f848 (Pocket's
  index, -2 none) toggles 0x4dc078, == 0x53f4e8 toggles 0x4dc068. The end routine
  0x442730 is an empty `ret`.
- The tile picture's step (0x43f840) adds (dx, dy) to its scroll (& 0x7f) and moves a
  cosine "wobble" (+0x20/+0x24 from counters +0x64..+0x70) that only the MMX copy path
  reads (0x43f292); the plain path (0x4dbcf4 = 0) ignores it. The scroll is per frame: its
  speed follows the frame rate.
- Checked: the port's settled main menu (frame 120) against the original's
  (build/ref/orig_mainmenu.png): 99.94 % of pixels equal, the rest the two mouse pointers.

#### 4.4.6 The common calls of a page frame (what each is, briefly)

The usual order of a page's frame routine (title 0x446b80, intro 0x453c50, main menu
0x442790 all follow it) [decomp]:

1. **0x49b1c0** — present the previous frame: if the menu is active and +0x30 is 0 do
   nothing, else clear +0x30 and flip (0x419900 → 0x474860); then display-mode
   housekeeping (0x4dbcec → 0x499ed0; 0x4e8940 → 0x4f1824). Since +0x30 is always 1,
   every frame is presented here, one frame late (the port presents at the end of the
   frame: same picture sequence).
2. **0x49a160(0,0)** — begin the frame ("BeginDisplay", screen3d.cpp): lock the back
   surface (0x476700; fatal "BeginDisplay failed"), 0x624f5c = pixels + (pitch × 0x6d1040 +
   0x624374) × 2 (the viewport origin), 0x4f1a20 = 1 (locked); reset the 3D renderer's per-
   row span lists (0x623bd8, 0x68b008 rows); then **0x4099a0** (menu hover, §4.4.3.3).
3. The page draws its pictures (0x43ede0) and queues 3D objects (0x48c030 etc.).
4. **0x40c1f0** buttons, **0x40c500** transition shards (the intro calls 0x40c500 after
   the sprites; order between them does not matter for 2D since the shards go to the 3D
   list).
5. **0x49ad90** — draw the queued 3D polygons (z-sorted, 8000 buckets at 0x65da34 down,
   each emptied); only when 0x504e74 == 0. **0x49ad70** just empties the 8000 buckets
   (used instead when 0x504e74 == 1). In Direct3D mode (0x4dc06c && !0x4dc070) 0x49ad90 is
   called after the unlock instead.
6. Text/sprite list **0x53b8e8** (48 object pointers with a vtable: slot 0 destroy, +8
   update, +0xc "still alive", +0x10 draw): **0x418790** = time-outs (objects whose
   timer +0x8c has expired change state 3→1 or 4→2) and removal of dead ones; **0x417840**
   = 1 when the list is empty/all dead; **0x418860** = update + draw every live object
   (called only if 0x417840 says something is alive). **0x417750** = delete them all
   (page change). The text system itself is documented elsewhere.
7. **Mouse pointer**: if fullscreen (0x4dbff0) and DirectDraw is up (0x53c524): picture
   **0x5049d4** (id 0x55 `Pointeur.lez`, language 0) placed at (mx, my) = (0x519fe8,
   0x519fea) with 0x43fa40 align 0 (its top-left corner on the hot spot), drawn mode 5.
   In a window the Windows cursor is used instead. 0x4dbff0 is forced to 1 whenever
   DirectDraw was created (0x42b0cb), so normally the game draws its own pointer.
8. **0x49b0e0(0,0)** — end the frame: if 0x625748 (a clear is pending) fill each row's
   remaining span list with the colour 0x65f0d8; unlock (0x4767b0); 0x624f5c = 0,
   0x4f1a20 = 0 [decomp; what the spans mean is the renderer's business].
9. Read the press (0x40c1b0), act, 0x40c1e0, maybe 0x40c4a0 + 0x409eb0.

Menu_Frame itself then does the demo timer, Alt+Enter, the held-button repeat and
**0x404eb0** = update the sound channels (0x404b30 on the 8 records of 100 bytes at
0x504240..0x504560 and on 0x5028c8: queued sounds started when their delay has passed).

Sounds used by the core: **0x4049f0(id, flags)** plays sound effect `id`: ids < 0x2f or
> 0x5c on the effects channel (if effects are on, 0x4db664), 0x2f..0x5c (voices) on the
second channel (0x4db668); flag 4 = don't restart if it is the last one played. Menu ids:
0x29 hover, 3 click/accept, 0x2a transition start, 0x2d title start, 0x55/0x56 the "Solo"/
"Link" voices. **0x404990(track, x)** plays a CD audio track through MCI (0x503528).

#### 4.4.7 Notes for the port

- Fixed 60 Hz steps: everything here runs on real milliseconds (now), so the port can
  keep a millisecond clock advanced by 16.667 per step and feed the same formulas
  (1200 ms transition, 30000 ms demo timeout, slide durations, per-frame dt capped 500 →
  100 in the intro/title).
- The selection model to copy: mouse mode follows the pointer each frame (−1 off the
  buttons); any key (other than Shift/Ctrl) switches to keyboard mode where the selection
  stays put; a mouse move switches back.
- Press on left button *down*, Return/Space on the selection, or a hotkey; read once by
  the page, then cleared. No presses during the 1.2 s transition.
- Open questions: the row/column convention of 0x49b2c0 for the shard matrix; Rand_Next's
  range (assumed 0..65534 from the (r − 32767)/32767 scaling); what 0x4d0bf4/0x540ae0
  durations of the intro are (page 25 is described only as far as its exit).

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

### 4.8 Text: fonts, the text objects, the words (mapped 2026-10-26)


Scope: bitmap fonts (FNT/RAW), FONTS.BPT, the string drawing into the 16-bit screen, the
moving "text objects" list (0x417d20 and friends), where the words come from, the 3D
letters of the name page. Addresses are virtual addresses in game.EXE. Confidence:
**high** = read in the asm or checked by running the tool on the data; **medium** = read
in Ghidra's text, consistent, not checked against the running game; **low** = inferred.

Tool: `tools/fnt.py` (list / text / sheets / rawcheck). Pictures: `build/png/fonts/`
(`sheets/NN_Name.png` every font's characters in its script colours; `sample_*.png`).
Strings: `build/notes/menu_strings.txt` (the whole language table).


#### 4.8.1 The FNT font files

##### 4.8.1.1 Where they are
`FONTES.BKF`: `LOWRES\*.FNT` (11), `HIRES\*.FNT` (21), `DELPHINE\DEBUG.FNT` + `DEBUG.RAW`,
`SYMBOL.FNT` + `SYMBOL.RAW`. The script names `.Raw` files; only DEBUG.RAW and SYMBOL.RAW
are in the bank (GOG), every other font exists only as .FNT. (high)

##### 4.8.1.2 Loading chain (high, asm + decomp)
- `0x472c40` (game start, after Config/List scripts): `0x426350` opens `Data\Fonts.Spt`
  (Script_Open swaps to .BPT), `0x426460(0, &0x4eaaec, 1)` reads only FontNumber (throws
  to stop), `0x426460(0, &0x4f180c, 0)` reads the whole script, creates the font objects,
  and (FontData section, 0x427c04) loads each font with `0x492170`.
- Then for every font: slot 1 `(1, 0xff)` (build the colour table) and `0x490960`
  (Font_Unload: frees glyph blocks, glyph pointers, palette; +0x5248 = 0). Fonts 10 and 11
  (Sprites) get +0x5480 = 1.
- Later `0x4961d0` (Fonts_Refresh; called from 0x409f70 menu start, 0x4592a0, 0x47d660
  Game_Frame, 0x43cf90 help, 0x48ae40/0x48b070 generic, ...) loads the fonts whose
  condition predicate (+0x5478) says yes and unloads the others. (medium: predicates not
  fully read)
- `0x492170` Font_Load: for each of up to 4 files (+0x524c count): extension of the name
  "raw" -> vtable slot 3 (+0xc) = `0x491380`; "iff" -> slot 5 (+0x14) = `0x492130` (empty
  stub). Then computes glyph pointers: for c in each file's range, glyph[c] (+0x18+c*4) =
  block[file] + height * (sum of widths of the chars before c in that file). Also a
  texture atlas layout for Direct3D (+0xa40 x, +0xe40 y, +0x1248 quads) - not needed.
  Pal/Moto fonts: +0x54a2 = largest pixel value. Ends with slot 1 (1, 0xff) and +0x5248 = 1.
- `0x491380` (slot 3): `0x490c90` makes "<name without extension>.FNT" (into 0x508430),
  `0x490d80` reads it; if that fails, reads the .RAW and scans it (1.4).

##### 4.8.1.3 FNT layout (high: 0x490d80 read in the asm; all 34 files parse to their exact
size, see `tools/fnt.py`)
| offset | size | meaning |
|---|---|---|
| 0 | u32 | first character code (ignored: the script's value is used) |
| 4 | u32 | character count (ignored: the script's last-first+1 is used; must agree) |
| 8 | u32 | height in pixels (stored at font+0xa18; the last file loaded wins) |
| 0xc | u32 | palette entries, 0 or 256 (checked 0..256) |
| 0x10 | 3*n | palette, bytes R,G,B (kept at +0x549a/+0x549e only by Pal/Moto fonts (+0x547c), only from the first file that has one) |
| .. | 4*count | widths, u32, one per character (font+0x618+c*4) |
| .. | sum(w)*h | glyphs one after the other, each w*h bytes, rows top first |
Checks in the loader: count > 0, first+count <= 256, palette entries 0..256.
A width of 0 means "no glyph": not drawn and no advance.

Pixel bytes mean different things per font type (2.2): 0 is always see-through.
Observed: Direct fonts use 1..31 (and 128..159 / 128 = colour index 0 = back colour:
CHRONO, DEBUG's opaque box); AntiAlias8Transp fonts use low 5 bits = colour, high 3 bits
= transparency (MENULINK 31,63,..,223; MSG up to 255; MENULAB 1 and 32..160, i.e. the
back colour, translucent edges); Pal fonts use 1..15 into their own palette.

##### 4.8.1.4 The RAW fallback (high: tools/fnt.py rawcheck reproduces DEBUG.FNT and SYMBOL.FNT
byte for byte)
RAW = mhwanh picture (32-byte BE header: magic, u16 version, u16 width @8, u16 height
@10, u16 palette entries @12; palette; pixels). The loader checks
`w*h + 3*pal + 32 == file size`. Each glyph sits in a box drawn in the **frame colour**
(FontData's 4th number). Scan (`0x491210`): from (x, y) search the row rightwards for a
frame pixel; none -> next row, except that the first miss after a glyph row jumps
`height + 2` rows (y of the top line + height + 2); found at (x0, y0): the run of frame
pixels to the right is w+2 long, the run downward from (x0, y0) is h+2 long; the glyph
is the w x h block at (x0+1, y0+1); the scan continues on the same row after the run.
The first glyph fixes the height ("The frame color is incorrect or the first found char
is too small" if < 2); every other glyph must have the same height ("Reading char at
%dx%d"). Glyphs are taken in character order first..last.

##### 4.8.1.5 The font object (C++ classes, vtables) (high for offsets used above)
Size 0x54a6 (AntiAlias8/8Transp: 0x70a6, the extra 0x1c00 = seven 256-entry int tables
for AntiAlias8's blend). Base init `0x490840`.
| off | meaning |
|---|---|
| +0 | vtable |
| +4 | type: 1 Direct (also DirectTrame), 3 CustomColor (also ..Trame), 5 AntiAlias8, 6 AntiAlias8Transp, 7 Pal, 8 Moto |
| +8 | name (script's first string, strncpy 15) |
| +0x18 | glyph pointer [256] |
| +0x418 | colour table u16 [256] (RGB565; index 0 unused) |
| +0x618 | width [256] |
| +0xa18 | height |
| +0xa1c | line spacing (extra pixels between lines), default 1 |
| +0xa20 | letter spacing (extra pixels after each glyph, may be negative), default 1 |
| +0xa38/+0xa3c/+0x1244/+0x1248.. | Direct3D texture and per-glyph quads (ignore) |
| +0x5248 | loaded flag |
| +0x524c | number of files (max 4); +0x5250 names [4][0x81]; +0x5454 first char [4] (bytes); +0x5458 counts [4]; +0x5468 frame colours [4] |
| +0x5478 | condition predicate (function pointer, 0 = none) |
| +0x547c | uses own palette (Pal, Moto) |
| +0x5484 | glyph blocks [4] |
| +0x5494 | fore colour R,G,B (default 255,50,20) |
| +0x5497 | back colour R,G,B (default 100,100,10) |
| +0x549a/+0x549e | palette pointer / entries |
| +0x54a2 | largest pixel value (Pal/Moto) |

Vtables (slot 0 SetColors(fore*, back*), 1 BuildColours(from, to), 2 ?, 3 LoadRaw, 4 ?,
5 LoadIff stub, 6 DrawChar):
| type (script name) | ctor | vtable | slot 0 | slot 1 | slot 6 DrawChar |
|---|---|---|---|---|---|
| Direct | 0x490700 | 0x4d1e68 | 0x490b00 | 0x490b90 | 0x493100 |
| DirectTrame | 0x490700 | 0x4d0710 | 0x490b00 | 0x490b90 | 0x492d20 |
| CustomColor | 0x494020 | 0x4d1e88 | 0x494160 (nothing) | 0x494150 (nothing) | 0x494170 |
| CustomColorTrame | 0x494020 | 0x4d06f0 | nothing | nothing | 0x494590 |
| AntiAlias8 | 0x4949f0 | 0x4d1ea8 | 0x490b00 | 0x494b90 (= 0x490b90) | 0x494bb0 |
| AntiAlias8Transp | 0x495110 | 0x4d1ec8 | 0x490b00 | 0x494b90 | 0x495280 |
| Pal | 0x495700 | 0x4d1ee8 | 0x490b00 | 0x495bd0 | 0x495840 |
| Moto | 0x495c30 | 0x4d1f08 | 0x490b00 | 0x4960e0 | 0x495d50 |
FONTS.BPT only uses Direct, AntiAlias8Transp, Pal and CustomColor (font 18 Symbol,
"Obsolete", no files). So a port needs Direct, AntiAlias8Transp and Pal.
Font array: `*(Font***)0x4f1810`, count `0x4f180c` (35).


#### 4.8.2 FONTS.BPT

##### 4.8.2.1 What it holds (high: tools/fnt.py reads it token by token to its exact end)
Reader `0x426460` (+ its pieces 0x42664e..0x427d90). Order of the tokens (the BPT is the
replay of the reads, so FontNumber appears twice because the game reads it twice):
1. FontNumber = 35 (twice). Must be 0..48. Allocates the pointer array.
2. FontTypes: `index, name, type, condition` until index -1. Type -> class (1.5);
   names "GameMsgLowRes"/"GameMsgHiRes" are forced to Direct whatever the script says
   (0x426911). Condition -> +0x5478: OutGame 0x45a8e0, InGameLowRes 0x45a930,
   InGameHiRes 0x45a970, InGameMenu 0x45a9c0, InGameMenuLowRes 0x45aa00,
   InGameMenuHiRes 0x45aa70, InGameGeneric 0x45aaf0, InGameHelpMenu 0x45ab20,
   Obsolete 0x45ab50 (returns 0, never loaded), NoCondition 0 (always). The predicates test
   the menu object (0x4dc0c4), SmallFonts (0x4f0e08), ScreenMode (0x4dc0c8 == 2) and
   in-game state flags 0x4e8904..0x4e8910. (medium)
3. FontSpacings: `index, letter spacing (+0xa20), line spacing (+0xa1c)` until -1
   (the two numbers are read only if the font object exists).
4. FontForeColors: `index, R, G, B` until -1 (0x490a30 -> +0x5494).
5. FontBackColors: `index, R, G, B` until -1 (0x490a60 -> +0x5497).
6. FontData: `index, first char, last char, frame colour, file` until -1
   (0x4929f0 adds a file: max 4, frame colour must be 1..255).

##### 4.8.2.2 The 35 fonts (from `tools/fnt.py list`; h = height of the .FNT, sp = letter,line)
| # | name | type | condition | h | sp | fore | back | files [chars] |
|---|---|---|---|---|---|---|---|---|
| 0 | ChronoLowRes | Direct | InGameLowRes | 6 | 0,0 | 255,255,255 | 0,0,0 | LowRes\Chrono [48..60] |
| 1 | MotoLowRes | Pal | InGameLowRes | 9 | 0,0 | (255,50,20) | 255,255,255 | LowRes\Moto [41..94] |
| 2 | PlayerLowRes | Pal | InGameLowRes | 8 | 0,0 | 255,0,0 | 255,255,255 | LowRes\Name [32..159] |
| 3 | TimeLowRes | Direct | InGameLowRes | 17 | -3,0 | 0,200,0 | 0,0,0 | LowRes\PosTime [32..57] |
| 4 | ChronoHiRes | Direct | InGameHiRes | 12 | -1,0 | 255,255,255 | 0,0,0 | HiRes\Chrono [48..60] |
| 5 | MotoHiRes | Pal | InGameHiRes | 16 | 0,0 | | 255,255,255 | HiRes\Moto [41..94] |
| 6 | PlayerHiRes | Pal | InGameHiRes | 16 | 0,0 | 255,0,0 | 255,255,255 | HiRes\Name [32..159] |
| 7 | TimeHiRes | Direct | InGameHiRes | 34 | -4,0 | 0,200,0 | 0,0,0 | HiRes\PosTime [32..57] |
| 8 | InfosDebug | Direct | NoCondition | 12 | 0,1 | 0,0,250 | 255,255,200 | Delphine\Debug [32..125] |
| 9 | MenuLink | AntiAlias8Transp | NoCondition | 29 | 2,4 | 200,200,0 | 150,150,0 | HiRes\MenuLink [32..95], HiRes\MenuLin2 [192..211] |
| 10 | SpritesLowRes | Pal | InGameLowRes | 19 | 0,0 | 255,255,255 | 0,0,0 | LowRes\Sprites [65..99] |
| 11 | SpritesHiRes | Pal | InGameHiRes | 33 | 0,0 | 255,255,255 | 0,0,0 | HiRes\Sprites [65..99] |
| 12 | Messages | AntiAlias8Transp | NoCondition | 21 | 1,1 | 255,127,10 | 200,100,0 | HiRes\Msg [32..255] |
| 13 | GameMsgLowRes | Direct (forced) | InGameLowRes | 17 | -3,0 | 255,10,10 | 127,5,5 | LowRes\PosTime [32..57], LowRes\PosTimeL [65..90] |
| 14 | GameMsgHiRes | Direct (forced) | InGameHiRes | 34 | -4,0 | 255,10,10 | 127,5,5 | HiRes\PosTime, HiRes\PosTimeL |
| 15 | SpeedLcdHiRes | Pal | InGameHiRes | 18 | -5,0 | | | HiRes\SpeedLcd [32..57] |
| 16 | SpeedLcdLowRes | Pal | InGameLowRes | 10 | 0,0 | | | LowRes\SpeedLcd [32..57] |
| 17 | MenuHiRes | Direct | OutGame | 22 | 1,1 | 255,0,0 | 0,0,0 | HiRes\Generic [32..255] |
| 18 | Symbol | CustomColor | Obsolete | - | 1,1 | | | (none) |
| 19 | PosLowRes | Direct | InGameLowRes | 17 | -3,0 | 0,200,0 | 0,0,0 | LowRes\PosTime [32..57] |
| 20 | PosHiRes | Direct | InGameHiRes | 34 | -4,0 | 0,200,0 | 0,0,0 | HiRes\PosTime [32..57] |
| 21 | MenuLabel | AntiAlias8Transp | OutGame | 16 | 0,6 | (255,50,20) | 255,255,0 | HiRes\MenuLab [32..90] |
| 22 | MotorLcdHiRes | Pal | InGameHiRes | 21 | 1,1 | | | HiRes\MotorLcd [48..61] |
| 23 | MotorLcdLowRes | Pal | InGameLowRes | 11 | 1,1 | | | LowRes\MotorLcd [48..61] |
| 24 | Keyboard | Pal | OutGame | 31 | 1,1 | | | HiRes\Keyb [27..174], HiRes\Keyb2 [205..245] |
| 25 | FinalPosHiRes | Pal | InGameHiRes | 39 | 1,1 | | | HiRes\FinalPos [47..61] |
| 26 | FinalPosLowRes | Pal | InGameLowRes | 19 | 1,1 | | | LowRes\FinalPos [47..61] |
| 27 | Generic | Direct | InGameGeneric | 22 | 1,1 | (255,50,20) | 0,0,0 | HiRes\Generic [32..255] |
| 28 | CounterHiRes | Pal | InGameHiRes | 28 | -5,0 | | | HiRes\Speed [48..59] |
| 29 | CounterLowRes | Pal | InGameLowRes | 14 | -3,0 | | | LowRes\Speed [48..59] |
| 30 | KeyboardHelp | Pal | InGameHelpMenu | 17 | 1,1 | | | HiRes\KeybHelp [26..173] |
| 31 | DataDiskMenu | Direct | OutGame | 21 | 0,0 | (255,50,20) | (100,100,10) | HiRes\DataDisk [32..90] |
| 32 | GameMenuHiRes | Pal | InGameHiRes | 18 | 0,2 | | | HiRes\GameMenu [32..220] |
| 33 | GameMenuLowRes | Pal | InGameLowRes | 9 | 0,1 | | | LowRes\GameMenu [32..220] |
| 34 | Help | Pal | InGameHelpMenu | 9 | 0,1 | 0,0,0 | | HiRes\Help [32..220] |
(blank / bracketed = the defaults 255,50,20 and 100,100,10; Pal fonts ignore both.)
The 4th FontData number (frame colour) only matters for the RAW scan.

Per language? **No.** FONTS.BPT has no language field; one set of fonts for all six
languages. Accented capitals are in Msg/Generic/GameMenu (chars 192..255; MenuLink adds
192..211 from MenuLin2). Per **resolution**, yes: LowRes/HiRes pairs (2.3). (high)

##### 4.8.2.3 Which font a number gives: `0x4903d0` Font_Get(index, int *substituted) (high)
- If `0x4f0e08` (SmallFonts, from Config) == 0 and `0x4dc0c8` (ScreenMode) == 2
  (640x480): low-res numbers become their hi-res partner: 0->4, 1->5, 2->6, 3->7, 10->11,
  13->14, 16->15, 19->20, 23->22, 26->25, 29->28, 33->32. Otherwise the reverse
  (4->0 ... 32->33). Other numbers stay.
- Returns the mapped font if loaded (+0x5248); else the asked one if loaded; else font 8
  (InfosDebug) and *substituted = 1. Index >= count -> font 0. No fonts at all -> one
  message box "No font loaded", returns 0.
- Direct3D mode (0x4f1814 set, 0x4f1828 clear) replaces AntiAlias8/8Transp fonts by
  font 27 (Generic). Not relevant to the software port.
ScreenMode 0x4dc0c8: 0 = 320x240, 1 = 512x384, 2 = 640x480 (Menu_Frame forces 2 at
0x409fc3 and restores it after). (medium for 0/1, high for 2)
The menu pages use (calls to 0x4903d0 with a constant): font 12 Messages 87 times,
21 MenuLabel 10, 31 DataDiskMenu 9, 9 MenuLink 6, 32 GameMenuHiRes 4, and a few others.


#### 4.8.3 Drawing text

##### 4.8.3.1 Colour tables (slot 1, called as (1, 0xff) after loading and by SetColors) (high)
`0x473a90(r, g, b)` makes a screen colour: `(r&0xff)>>3 << 11 | (g&0xff)>>2 << 5 |
(b&0xff)>>3` for RGB565 (shift globals 0x4ee9b8/bc/c0 = losses 3,2,3, 0x4ee9c4/c8/cc =
positions 11,5,0, masks 0x4ee9d0/d4/d8 = R,G,B masks, set at 0x473c43 from
DDPIXELFORMAT dwRBitMask/dwGBitMask/dwBBitMask - so the 3-byte colours are R,G,B).
- **Direct / AntiAlias8 / AntiAlias8Transp (`0x490b90`)**: a 32-entry ramp at 0x60d1a0:
  per channel `acc = back << 16`, `step = ((fore - back) & 0xffff) * 0x800`; entry i =
  `0x473a90(acc_r>>16, acc_g>>16, acc_b>>16)`, then `acc += step` (32-bit). Entry 0 =
  back colour, entry 31 = back + 31/32 of the way to fore (the &0xffff makes negative
  steps work modulo 256). Then table[v] = ramp[v & 31] for v = from..to (1..255).
- **Pal / Moto (`0x495bd0`)**: table[v] = 0x473a90(palette[v % entries]) (R,G,B bytes).
- **CustomColor**: no table; per pixel (3.4).
- `0x490b00` SetColors(fore*, back*): copies the 3-byte colours into +0x5494/+0x5497 if
  different and then calls slot 1 (1, 0xff). CustomColor's slot 0 does nothing.
- NB: `0x493490` (3.3) also copies its fore/back arguments into +0x5494/+0x5497 but does
  **not** rebuild the table, so for Direct/AntiAlias/Pal they change nothing in software
  (they matter for CustomColor and Direct3D). To recolour a Direct or AntiAlias font,
  callers call slot 0 first (e.g. 0x446f0e, 0x4471f3) and the text objects do it around
  each draw (3.6).

##### 4.8.3.2 `0x493820` DrawString(font, int noscale, char *text, x, y, flags, level, int
*rect_out, u8 *fore, u8 *back) (high)
Wrappers: `0x493e20`, `0x493ee0` (noscale 0, char*), `0x493e60` (noscale 0, CString by
value), `0x493f20`, `0x493fe0` (noscale 1), `0x493f60` (noscale 1, CString).
- Nothing if no screen object (0x4dc0c0), empty text, or Direct3D lost surfaces.
- Target: the back surface (address of (x,y) by `0x49d0f0`, pitch 0x62ebec pixels, clip
  width 0x624f58, height 0x68b008), or an alternative buffer (0x4e7058, pitch 0x4e7054,
  0x4e704c x 0x4e7050) when 0x4ee940 and 0x5d8940 are set. Locks the surface with
  0x49a160 if needed and unlocks with 0x49b0e0.
- Halves x and y when `noscale == 0`, the menu object is null or its first field is 0
  (in-game), and (SmallFonts or ScreenMode != 2). In the 640x480 menu: no change.
- flags & 0x100: word wrap - a copy of the text with '\n' inserted where a line would
  pass the clip width.
- `0x492c20` places the block (w, h from 0x492b10): flags & 0x1f: 2 or 4 -> x = (W-w)/2;
  8 or 0x10 -> x = W - x - w; others (0, 1) leave x. flags & 0xe0: 0x40 -> y = (H-h)/2,
  0x80 -> y = H - y - h; 0x20 or 0 leave y. (Flags 0x21 skip even the measuring.)
- Level: type 5 -> `(level << 11)/100 >> 8` (0..8); type 6 -> `level*0xe000/100 >> 8`
  (0..224); others unchanged (Direct/Pal ignore it in software). Direct3D: 0..63.
- Then `0x493490` (if 0x4f1824 is set, i.e. the 2D drawing is on).

`0x492b10` Measure(font, text, *w, *h, one_line): w = longest line of sum(width[c] +
letter spacing) over chars with width > 0 (so it includes the spacing after the last
char); h = height + (line spacing + height) per '\n' (rounded up to even when 0x624b14).
`0x417d20` and 0x416300 add the line spacing once more to h for layout.

##### 4.8.3.3 `0x493490` the string loop (high)
- Optional fore/back (3-byte) copied into the font (see 3.1 NB).
- 0x624b14 (in-game "every other line" mode): y forced even (moved up one row if it
  would go off the bottom).
- flags & 0x1f == 4 or 0x10: each line is centred / right-aligned inside the block
  width (measured per line, one_line=1).
- For each byte: '\n' -> x back to the block's left (+ that line's alignment), y +=
  height + line spacing; else slot 6 DrawChar(&screen_ptr, pitch, ?, clip_w, clip_h,
  char, &x, &y, level). Characters are unsigned bytes; 0 and width-0 chars are skipped
  without advancing.
- rect_out (if given): x, y of the start, then right = max line end - letter spacing,
  bottom = y + line spacing.

##### 4.8.3.4 DrawChar: one glyph into RGB565 (high: inner loops read in the asm)
Common: glyph w = width[c], h = height; clipped to 0..clip_w-1, 0..clip_h-1; pixel 0
never drawn; afterwards x += w + letter spacing (screen pointer likewise). If 0x624b14
and in-game, rows with odd screen y are skipped. No scaling, no separate shadow pass:
outlines and shadows are pixels of the glyph (e.g. DataDisk's dark edges are ramp
entries near 0 = back colour; MenuLab's 32..160 are the back colour made translucent;
Debug's 128 is an opaque back-colour box).
- **Direct `0x493100` / Pal `0x495840`**: `screen = table[b]` for b != 0.
- **AntiAlias8Transp `0x495280`**: `t = level + (b >= 32 ? b & 0xe0 : 0)`; if t > 224
  skip; `a = (t << 8) / 224` (unsigned, 0..256); S = table[b] (= ramp[b & 31]), D = the
  screen pixel; for each mask m in (0xf800, 0x07e0, 0x001f):
  `out |= ((S&m) - ((((S&m) - (D&m)) * a) >> 8)) & m` - the machine does imul and a
  logical shr on 32 bits; because of the final mask this equals an arithmetic shift
  (floor), which is what tools/fnt.py does. a = 0 -> glyph colour, a = 256 -> screen.
- **AntiAlias8 `0x494bb0`** (unused): k = level + (b >> 5); k >= 8 skip; k == 0 ->
  table[b]; else per channel sum of two lookups in the 1 KB tables at +0x54a6 + k*0x400
  (glyph) and +0x74a6 - k*0x400 (screen) - an 8-level mix. Table fill not read. (low)
- **CustomColor `0x494170`**: v = b & 31; colour = 0x473a90((fore*v + back*(31-v))/31
  per channel). Only font 18 (never loaded).
- **DirectTrame `0x492d20`** (unused): like Direct but a pixel is drawn only if level == 0
  or `level*2 < ((rows_left & 15)^2 + (cols_left & 15)^2) & 0x7f` (a dissolve pattern).
- Every DrawChar also has a Direct3D branch (textured quad via 0x485e50 with colour
  0x4f1804) taken when the font has a texture (+0xa38/+0xa3c) and the menu object's first
  field is 0. Not needed.

##### 4.8.3.5 The named colours at 0x4f0dd4.. (high: bytes read from the image)
3-byte R,G,B colours on a 4-byte stride (passed as `u8*`, not RGB565):
0x4f0dd4 white (255,255,255), **0x4f0dd8 black (0,0,0)**, 0x4f0ddc grey (127,127,127),
0x4f0de0 (0,255,255) cyan, 0x4f0de4 (255,0,255) magenta, **0x4f0de8 (255,255,0) yellow**,
0x4f0dec (255,0,0) red, 0x4f0df0 (0,0,255) blue, 0x4f0df4 (0,255,0) green,
0x4f0df8 (19,188,5) green, 0x4f0dfc (255,160,0) orange.
(0x4903d0 does not make colours: it is Font_Get. The colours are turned into RGB565 by
the font's slot 1 when SetColors is called with them.)

##### 4.8.3.6 Text objects: the list at 0x53b8e8 (high for the list functions, medium for the
motion maths)
Three lists of 48 pointers (0xc0 bytes each): **0x53b8e8** (menu and game messages),
0x53b9a8, 0x53ba68. Object = 0xc2 bytes, vtable 0x4d0560 (base class ctor 0x415cd0,
vtable 0x4d0540). Slots of 0x4d0560: 0 0x489c50 destructor, 1 0x4160a0, **2 0x416da0
Draw**, **3 0x4169b0 IsAlive(force)**, **4 0x4173d0 debug frame**, 5 0x4168b0,
**6 0x4165c0 Relayout**, 7 0x417210, 8 0x415f50, 9 0x416100, 10 0x417080, 11 0x416a60.

Object fields: +4 drawn this frame; +8 align flags; +0xc = 1; +0x10 effect; +0x14 motion
mode; +0x18/+0x20 velocity (doubles); +0x28/+0x30 position (doubles, x and screenH - y);
+0x38 double k; +0x40 double s; +0x48/+0x50 doubles = 1.0; +0x58/+0x5c vx, vy (int);
+0x60/+0x64 target x, y; +0x68/+0x6c authored target; +0x70/+0x74 current x, y;
+0x78/+0x7c authored x, y; +0x80/+0x84 text size; +0x88 start time (ms); +0x8c lifetime
(ms, 0 = forever); +0x90 blink period (ms); +0x94 frame counter; +0x98 blink frame
mask; +0x9c id (from the global counter 0x4e4ecc); +0xa0 int* that receives the id;
+0xa4 no off-screen check; +0xac text (own copy); +0xb0 font; +0xb4 font number;
+0xb8 use own colours; +0xbc fore R,G,B (default 255,255,255); +0xbf back R,G,B
(default 127,127,127).

**`0x417d20` TextList_Add(list, ...) - 26 stack arguments** (this = list in ecx;
checked: args 14 and 15 are never read):
| # | name | FULL D3D VERSION (0x446f8a) |
|---|---|---|
| 1 | text (char*) | "FULL D3D VERSION" (0x4e94e0) |
| 2 | lifetime ms (+0x8c), 0 = until removed | 0 |
| 3 | font number (through 0x4903d0) | 0x1f DataDiskMenu |
| 4 | x | 0 |
| 5 | y | 0x3c = 60 |
| 6 | align flags (+8), applied by 0x417900 | 0x22 = centred horizontally |
| 7 | effect (+0x10): 0 none, 1 FadeIn, 2 FadeOut, 3 Blink, 4 BlinkTransp, 5 Transp75, 6 Transp50, 7 Transp25 | 3 Blink |
| 8 | motion mode (+0x14): 0 static, 1 fly to target, 2 drift, 3 wait lifetime then 1, 4 wait lifetime then 2 | 0 |
| 9 | blink: < 0 -> period -n ms (+0x90); > 0 -> frame mask (+0x98); 0 -> 2 for effects 3 and 4 | -500 |
| 10 | int* receiving the object's id | &0x4e98ac |
| 11 | x step to dodge other texts | 0 |
| 12 | y step to dodge other texts | 0 |
| 13 | if no free place: re-align centred both ways (0x42) | 0 |
| 14, 15 | unused | 0, 0 |
| 16, 17 | vx, vy (+0x58/+0x5c; initial velocity for mode 1, direction for mode 2) | -1, 1 |
| 18, 19 | target x, y (+0x60/+0x64), aligned like x, y | 0, 40 |
| 20+21 | double s (+0x40), mode 2's starting speed | 1.5 (0, 0x3ff80000) |
| 22+23 | double k (+0x38), mode 1 gain / mode 2 acceleration | 1.0 (0, 0x3ff00000) |
| 24 | use own colours (+0xb8) | 1 |
| 25 | fore colour, u8[3] R,G,B, copied to +0xbc | 0x4f0de8 yellow |
| 26 | back colour, u8[3], copied to +0xbf | 0x4f0dd8 black |
Returns the object or 0. Steps: Font_Get; find a free slot (empty or not alive; none ->
0); measure the text (h += line spacing); unless mode 1: align with 0x417900 and, if the
box overlaps another live text (0x417ac0), try y += step12 / y -= step12, then x +=/-=
step11 within the screen, then (arg 13) centre; new object, 0x416170 stores the motion
fields (start time = clock 0x4e4ed0, or 0x4e9814 in demo playback), 0x416300 copies text
+ font and measures; mode 1 objects get slot 6 Relayout at once.
FULL D3D VERSION: the title page adds it on each frame whose counter 0x4e90e0 is 11..20
(the counter wraps after 20), centred at y = 60, yellow ramp over black edges, blinking
500 ms on / 500 ms off. The other arguments are inert for mode 0. Sample:
`build/png/fonts/sample_fulld3d_title.png` (drawn by tools/fnt.py, not a capture).

`0x417900` Align(w, h, *x, *y, flags) on the screen size (W = 0x5d93c8, H = 0x5d8948):
flags & 6: with flags & 0x18: x = W/2 - x - w; else flags & 1 ? x += W/2 : x += (W-w)/2.
Without 6: flags & 0x18 -> x = W - x - w. Vertical: 0x40 with 0x80 -> y = H/2 - y - h;
0x40 with 0x20 -> y += H/2; 0x40 alone -> y += (H-h)/2; 0x80 alone -> y = H - y - h.

`0x416da0` Draw: refresh the font (Font_Get(+0xb4)); motion step `0x416b40`; then by
effect, visible and a level (percent):
- 0: level 0. 5/6/7: level 75/50/25.
- 1 FadeIn: lifetime > 0 -> level = 100 - elapsed*100/lifetime; else if period > 0:
  level = (1 - elapsed % period)*100 (a quirk; period is in ms).
- 2 FadeOut: level = elapsed*100/lifetime (or (elapsed % period)*100).
- 3 Blink: period 0 -> visible when ((++frames) & mask) != mask; else visible when
  elapsed % (2*period) < period. Level 0.
- 4 BlinkTransp: p = period or 500; e = elapsed % 2p, folded (e > p -> 2p - e);
  level = ((e << 4)/p * 100) >> 4.
(elapsed = clock 0x4e4ed0 - start.) If visible: when +0xb8, save the font's colours,
SetColors(+0xbc, +0xbf), draw with 0x493e20(font, text, x, y, 0x21, level) (x, y
doubled when in-game and not 640x480 - undone by 0x493820's halving), restore colours.
Only AntiAlias8Transp (and Direct3D) fonts show the level; Direct and Pal fonts ignore it,
so FadeIn/Out are instant for them.

`0x416b40` motion (each Draw):
- mode 1: if not at the target: d = target - pos (y measured upward), dist = |d|; dist
  <= 1.5 -> snap to target. Else heading = atan2(vy, vx), aim = atan2(dy, dx); the
  difference wrapped by +-2*pi (constants 0x4d0510 pi/2, 0x4d0518 pi, 0x4d0520 2pi,
  0x4d0528 -pi/2); new heading = heading + k*diff*0.05 (0x4d0530); new speed =
  (dist - |v|)*k*0.05; v = speed*(cos, sin); pos += v; x, y = truncated pos.
- mode 2: s += k; x += trunc(vx*s); y += trunc(vy*s) (ints, each frame).
Note: one step per drawn frame, not per ms (frame-rate dependent in the original).

`0x4169b0` IsAlive(force): id != 0, (force or lifetime 0 or elapsed <= lifetime), the
box intersects the screen rectangle (0x624374, 0x6d1040, size 0x624f58 x 0x5d8948)
unless +0xa4, text and font set.
`0x418790` TextList_Update (all 48): lifetime over and mode 3 -> mode 1, start =
timeGetTime(); mode 4 -> mode 2, lifetime 0, vy = 1 if vx = vy = 0, s = 1.0 if 0; then
delete the object if not alive.
`0x418860` TextList_Draw: for each live object Draw (slot 2) then slot 4 (0x4173d0 ->
0x4174f0: only with the debug flag 0x4e4ec8 - frame rectangles and the id in font 8).
`0x417840` TextList_IsEmpty: 1 when no object is alive.
`0x417750` TextList_Clear: deletes all 48.
`0x4178c0` SetTextClock(ms): 0x4e4ed0 = ms if > 0, else 0x4e9814 in demo playback, else
0x50bbc8. Menu_Frame (0x409c63) passes timeGetTime() (also stored in 0x50bbc8); 0x4120e0
passes -1.
`0x4165c0` Relayout (slot 6): positions are authored for 640x480; at 512x384 (ScreenMode
1, when the font was substituted or hi-res) x = ((x<<8)/640 << 9) >> 8, y = ((y<<8)/480
* 384) >> 8; in other low modes x/2, y/2; then re-align start and target with 0x417900.

##### 4.8.3.7 Port recipe (summary)
Load FONTS.BPT and the .FNT files once; keep per font: height, spacings, widths, glyphs,
fore/back, palette. Colour table per font (3.1). Draw strings with 3.2-3.4 into the
RGB565 framebuffer at 640x480; implement Direct, AntiAlias8Transp, Pal. Implement the
48-slot text list with effects 0..7 and modes 0..4 driven by a millisecond clock.
`tools/fnt.py` is the reference for the pixel maths.

**In the port** (`port/src/script.c`, `font.c`, `text.c`): FONTS.BPT and every .FNT
loaded at start; the clock set at the top of Menu_Frame (0x409c70, timeGetTime), the list
updated and drawn before the pointer, cleared on a page change. **Checked**: "FULL D3D
VERSION" (`OMR_TEXT="31,0,60,0x22,3,-500,5,1,FULL D3D VERSION"`) covers the same 2171
pixels as in the original's title capture (orig_p14_title_fulld3d.png against
orig_p14_title.png), every one equal in 5-6-5; shown 0-499 ms, hidden 500-999 ms.
Not written yet: motion 1 (0x416b40's fly-to-target) and the dodging (0x417ac0), each
prints a message when a page asks for it.


#### 4.8.4 Where the words come from

##### 4.8.4.1 The language string table (high)
`0x4e2920`: records of 28 bytes `u32 id, char *en, *fr, *de, *it, *sp, *sw` (null = not
translated), 191 records, ended by id -1 at 0x4e3e04 (just before the picture table
0x4e3e20). `0x415a70` SetLanguage(n) (n = 0x4dbce8: 0 US, 1 FR, 2 GR, 3 IT, 4 SP, 5 SW,
the order of the MenuXX folders 0x4e8b60): fills `char *text[192]` at **0x53b5e8** with
"INVALID STRING" (0x4e4eb8), then text[id] = record[n] or the English one. Code reads
it directly as `*(char**)(0x53b5e8 + 4*id)`. Ids 0..191 (not in order): control names
(0-13), "Lap(s)", "ONE RACE"/"CHAMPIONSHIP"/"GRAND PRIX" (19-21), the options texts
(25-48, 93, 171-174, 188-191), network messages, in-game menu items in brackets
("[YES]", "[QUIT RACE]", 101-114), sound/replay messages (115-135), error messages
(175-182). Full dump: `build/notes/menu_strings.txt`. The brackets are characters of the
GameMenu fonts (button ends, presumably; not checked).

##### 4.8.4.2 Pictures, not text (high)
Most menu words (page titles such as "MAIN MENU", buttons "Options", "Exit", "Play Solo")
are LEZ pictures from `data\MenuXX\` (NOTES §4.7, language-dependent folder). Checked:
MenuLabel drawn as "MAIN MENU" does not match the original screen
(`build/png/fonts/sample_mainmenu_label.png` vs `ref_mainmenu_label.png`: the original
is a bolder italic face, i.e. part of the picture).

##### 4.8.4.3 .GEN scripts (medium)
Plain-text "generic" sequences: `GENERIC.BKF` (START, END, DSI1-3, EA1-3, BMG1-3,
SHORT1-8 = credits/intro texts) and `MenuXX\HELP.GEN` (the in-game key help, per
language, opened by 0x43cf90 "openHelpMenu", "Help.gen"). Parser `0x488a30` (CloseGeneric
0x48b070, checkGeneric 0x48ae40); it makes text objects (calls 0x416170/0x416300).
Syntax: `{ ... }` one text item; `[X...]` tags (letter upper-cased): `B n`, `C r,g,b`
fore colour (0x4f1604..), `G r,g,b` back colour (0x4f1608..), `D n` (0x4f162c), `E name`
effect None/FadeIn/FadeOut/Blink/BlinkTransp/Transp75/Transp50/Transp25 -> 0..7
(0x4f1600, high), `F name` font by name (0x490690 Font_FindByName: compares the 15-char name at font+8, default 8; e.g. FMessages, FKeyboardHelp, FHelp; -> 0x4f15fc), `H C|L|R`
horizontal align -> flags 2/1/8, `V C|D|U` vertical -> 0x40/0x80/0x20 (0x4f1610), `I n`,
`L n` lifetime ms (0x4f1614), `MH n` `MV n` `MX n` `MY n` (0x4f1618..0x4f1624, motion),
`S name` (up to 16 chars, 0x5f6760: the key/section, e.g. SF1, SESC), `W n` (a column
position, W-1 = back to default), `Y n`; `\NNN` = character code NNN (key-cap glyphs of
KeyboardHelp, e.g. \131 = F1), `\(` `\)` literal brackets; newline = new line.


#### 4.8.5 The 3D font (`data\menu\font3d`, page 15 name entry) (medium)
Not a bitmap font: 26 3D models `LA.3DF/.MAP` .. `LZ.3DF/.MAP` (3DF + MAP as in NOTES
§4.5, textured, purple), plus `BACKSPAC`, `VALID`, `CADRE` (the selection frame; also
CADRE1-3.3DF), `CURSEUR` (+CURSEUR2). Loaded by page 15's start routine `0x487070`
(loop of 0x1a letters with "data\menu\font3d\l%c.3df/.map"; it first calls 0x49a0e0 and
0x49a100 with 1024.0, meaning not read) and drawn by `0x487a70` from the frame routine `0x487bb0` through
the 3D model renderer (0x47a3a0, 0x47a820, 0x47a8b0). The page calls no bitmap-font routine (no 0x4903d0/0x493xxx/0x417d20 in 0x487bb0); judging
by build/ref/orig_p15_name.png the typed name ("DSI") is made of the same 3D letters.


#### 4.8.6 Open points
- 0x4471d8 (page 5/26 "PRESENTS") draws with MenuLabel at (550, 800) with SetColors
  (white, white): only fits if 0x493820's halving is active at that moment (menu object's
  first field 0). Check under wine.
- The exact meaning of the menu object's first field (0x4dc0c4 -> +0): "drawing in 2D in
  the menu" judging by its uses; not read where it is set.
- AntiAlias8's tables (unused), the `B`, `D`, `I`, `Y`, `W` GEN tags in detail.
- Not compared against a capture of the original yet: the AntiAlias8Transp blend and the
  ramp are from the asm; a capture of a Messages text over a picture would confirm.

### 4.9 The 3D intro pages (25 logo, 14 title) and the software 3D path (mapped 2026-10-26)

Inside 4.9, "section 3.1", "(2.2)" and the like mean 4.9.3.1, 4.9.2.2.
Everything here is from `game.EXE` 1998 (SHA-1 164b4175...), addresses in the program at 0x400000.
Confidence marks: **[asm]** read in the disassembly (build/mr.asm), **[px]** confirmed by
pixel comparison with the running original (tools/orig.sh), **[decomp]** read only in the
Ghidra output, **[guess]** inferred.

The reference renderer is `tools/model.py` (modes `dsi`, `title`, `model`). It follows every
rule below and reproduces the original's frames to within a few dozen pixels (4.9.6).

---------------------------------------------------------------------------------------------

#### 4.9.1 Page 25: the Delphine Software International logo

Page table entry (0x4dbb38 + 25*16): start 0x4537d0, frame 0x453c50, end 0x453bf0.

##### 4.9.1.1 What the start routine 0x4537d0 loads

- **Texture pages** through 0x401160 (thiscall on a texture page object; the objects are an
  array at 0x4f92f8 with stride 0x338):
  - page 0 (object 0x4f92f8) = `data\pages\titleDsi\bnk00_00.raw`
  - page 1 (object 0x4f9630) = `data\pages\titleDsi\bnk00_01.raw`
  - The success flag goes to 0x54098c. Afterwards the u16 at page +0x254 is set to 0x180
    (bit 3 clear, so no half-texel UV tweak in software, see 3.6). +0x150 keeps a copy of the
    path (used only by the Direct3D reload path 0x41b0f0/0x41aee0 when 0x4dc06c is set). [asm]
- **Models** through 0x4216a0(model, name.3df, name.map, -1, 1) into 0x70-byte model
  structures (section 3.1):
  - 0x542150 `camera` (the camera body), 0x5421c0 `D`, 0x542230 `S`, 0x5422a0 `I`,
    0x542310 `DsInt` (the "Delphine Software International" banner). All from
    `data\pages\titleDsi\`. [asm]
  - Each is then recentred on its bounding-box centre by 0x420250 (centre = (min+max)*0.5,
    constant 0x4d0680 = 0.5; normals recomputed by 0x41f920) and scaled by 0x4204b0 with
    per-axis factors:
    camera (0.055, 0.055, 0.06); D, S, I and DsInt each (0.04, 0.04, 0.04). [asm]
- **A picture** 0xb2 `logodsi.lez` is loaded into a fresh 0x74-byte picture object
  (pointer 0x5409c0) but is **never drawn** by page 25; the end routine frees it. [asm]
- **Objects**: an array of 5 object structures of 0x184 bytes at 0x53f850 (built by static
  initialiser 0x4535b0 through ctor 0x48bbc0, which sets +0x14c = -1). Each is initialised by
  0x48bce0(obj, position, angles, model): +0x00..+0x08 angles a,b,c (floats, radians),
  +0x0c..+0x14 position x,y,z, +0x34 model pointer, +0x94 = 1.0. [asm]
  - object 0 = camera model, position (-30, 0, -900), angles (0, -1.701696 [bits 0xbfd9d12d], 0)
  - objects 1..3 = D, S, I; object 4 = DsInt; all position 0, angles (0, -pi/2, 0);
    then object 3 (I, +4 at 0x53fce0) and object 4 (DsInt, +4 at 0x53fe64) get b = +pi/2.
- **Camera** (the view, not the model): camera block = 0x5161c0 + index*0x238 where index
  = 0x4dc0f8. 0x46da40(cam, (0,0,0), (0,0,1500)) sets +0x38 and +0x50; the camera position
  (+0x98..+0xa0) ends at (0, 0, 4500); 0x46e140(cam, target (0,0,0)) builds the look-at
  matrix, which for this position is C = diag(-1, 1, -1) (see 3.3). 0x49a120(1.0) sets the
  screen scale factors to 1. [asm]
- **Tables** filled by 0x453610 (section 1.2). Music: 0x404990(13, 0) = CD/WAV track 13.
- **Button**: 0x40bd40 resets the menu buttons; picture 0x5049b4 (`continue.lez`) placed at
  (-300,-300) i.e. off screen; 0x40bef0 binds key 0x1b (Escape) to it. So Escape skips. [asm]
- 0x5409a8 = timeGetTime() at start (the frame's time base). [asm]

##### 4.9.1.2 Tables (0x453610) [asm]

Six animation slots i = 0..5. Floats.

| table | meaning | values |
|---|---|---|
| 0x540188[6] | progress (sum of dt/duration) | all 0 |
| 0x540ae0[6] | duration in seconds | 15, 14.5, 14, 13.5, 16, 20 |
| 0x5401d0 (stride 0xc) | start position | i0 (50,80,-1000) unused; i1-3 (0,0,20000) unused; i4 (20,0,60000); i5 (50,80,-1000) |
| 0x542100 (stride 0xc) | end position | i0 (50,0,7000) unused; D (200,0,200); S (-50,0,200); I (-250,0,200); DsInt (20,-400,-600); i5 (50,0,7000) |

0x53f788 (count of finished slots, kept as a maximum) = 0.

##### 4.9.1.3 Frame 0x453c50 [asm, px]

1. dt = timeGetTime() - 0x5409a8, as unsigned to float (ms); 0x5409a8 = now. If dt > 500
   (0x4d0bf4) then dt = 100. (So a stall advances only 100 ms.)
2. 0x49b1c0 (begin frame); 0x45a2e0(0,0,0) sets the clear colour black; 0x499ff0 ->
   0x476680 clears the back buffer with a DirectDraw COLORFILL; 0x49a160(0,0) locks and
   resets the 3D lists (section 3).
3. For i = 0..5: progress[i] += dt / (duration[i] * 1000) (constant 0x4d0bfc = 1000);
   t = progress clamped to [0,1] (the clamp is done on the float's integer bits);
   n = number of slots with progress > 1.0.
   - **i = 0 (camera model, object 0)**: angles
     a = 0, b = (1 - t)*pi/2 - 1.701696021 (0x4d0d28), c = (1 - t*t)*pi/2. Position fixed.
   - **i = 1..3 (D, S, I)**: e = end position, L = |e|,
     R = L + 2500 - 2500*t*t (0x4d0d40/0x4d0d44 = 2500), p' = e * R / L;
     theta = -2*pi*t (0x4d0d30), evaluated through the cosine table (section 3.2);
     position = ( p'x*cos(theta) + p'z*sin(theta),  p'y,  -p'x*sin(theta) + p'z*cos(theta) - 800 )
     (0x4d0d48 = 800). Yaw b = (1 - |t|)*8*pi (0x4d0d38) - pi/2 for D and S, + pi/2 for I;
     a = c = 0. So each letter spirals in from 2500 further out, turning four times, and lands
     on its end position shifted 800 towards the camera.
   - **i = 4 (DsInt)**: position = start + (end - start)*t (from z 60000 to -600).
   - **i = 5**: position lerp written into the camera block +0x98..+0xa0 (0x516258..60).
     The camera is not looked again (its matrix stays diag(-1,1,-1)) but 0x48c030 uses the
     position, so the whole logo recedes from z 4500-ish: (50,80,-1000) -> (50,0,7000) over
     20 s. **Visible** (correction: the port without it differed from the original by a
     third of the screen). Slot 5 comes last in the loop, so each frame's objects use the
     position of the frame before. [asm, px: port vs original 1108 px at the end]
4. Objects 0..4 drawn by 0x48c030 (section 3). 0x53f788 = max(0x53f788, n).
5. 0x49ad90 renders the collected faces (software path when 0x504e74 == 0 and
   (0x4dc06c == 0 or 0x4dc070 != 0)). Then 0x40c1f0 (menu buttons), 0x418790 / 0x417840 /
   0x418860 (effect list 0x53b8e8), 0x40c500 (page-transition timer), mouse cursor if
   0x4dbff0 && 0x53c524, 0x49b0e0 (unlock, flip).
6. **Leaving**: r = 0x40c1b0() (returns -1 while a transition runs (menu+0xd48) or
   0x504e84, else the activated button index). r == 0 (Escape) -> 0x40c1e0, then page 14.
   Else if 0x53f788 == 6 (every slot past 1.0, i.e. after the longest, 20 s) -> page 14 via
   0x409eb0(menu, 14), then 0x40c4a0 which sets 0x4dbd00 = 1. [asm, px: the original moves on
   at about 20 s]
7. 0x4dbd00 = 1 makes the next page start with the **1.2 s screen-tile transition**: the
   menu frame sets menu+0xd48, menu+0x20 = now + 1200 (0x4b0), camera 0x504c70 at z 1911;
   0x40c500 advances progress = (now - start)*0.000833333 (0x4d02b4 = 1/1200); the old
   screen (captured by 0x479260) is cut into tiles that fly apart, drawn by 0x479c10 through
   the same 3D path (0x49b2c0 / 0x49b7f0). The new page animates underneath at the same
   time. [asm, px: the shards are visible in captures]

End routine 0x453bf0: 0x4049e0 stops the music, 0x401ab0 frees the texture pages,
0x41f7d0 frees the models, the logodsi picture is freed. [asm]

---------------------------------------------------------------------------------------------

#### 4.9.2 Page 14: the Moto Racer title

Page entry: start 0x4465f0, frame 0x446b80, end 0x446b40.

##### 4.9.2.1 Start 0x4465f0 [asm]

- 0x4dc1c4 = GP edition flag (selects `titleGP` and the GP tables/pictures).
- Texture page 3 (object 0x4f9ca0) = `data\pages\titleMr\bnk00_00.raw` (GP: `titleGP`);
  result flag 0x53f66c.
- Models (0x4216a0 with last argument 1): 0x540338 `ellipse`, 0x5403a8 `moto`,
  0x540418 `racer`. 0x422250(model, 3) adds 3 to every face's page (+0x36), so their page-0
  textures come from page 3. **No recentre** (unlike page 25).
- Scales (0x4204b0):
  - MR: ellipse (0.04, 0.03875, 0.04), moto (0.04, 0.036, 0.0375), racer (0.04, 0.033, 0.0395)
  - GP: ellipse (0.04, 0.0325, 0.0455), moto (0.04, 0.053, 0.04825), racer (0.04, 0.0315, 0.0315)
- Objects at 0x540500 / 0x540684 / 0x540808 (static init 0x446400), angles (0, pi/2, 0),
  position 0.
- Camera: same as page 25 (position (0,0,4500), looking at the origin, C = diag(-1,1,-1));
  0x49a120(1.0).
- Tables by 0x446460 (2.2). Start button: picture 0x5049b0 (`start.lez`) at (0, 430), key
  0x0d (Return).
- 0x53f4e0 = timeGetTime(). The title pictures 0xbf, 0xbe, 0xbd, 0xbc (GP 0xc3..0xc0) are
  drawn once (pre-load), 0x49b0e0, then 0x404990(3, 2) = music track 3, 0x4049f0(0x2d, 0)
  a sound effect.

##### 4.9.2.2 Tables (0x446460) [asm]

| table | meaning | MR | GP |
|---|---|---|---|
| 0x5420e8[3] | progress | 0 | 0 |
| 0x540220[3] | duration (s) | 1.0, 1.2, 1.4 | same |
| 0x53fff8 (stride 0xc) | start | ellipse (60,42,-40000), moto (-12000,44,-798), racer (12000,25,-798) | (69,26,-40000), (-12000,-90,-798), (12000,40,-798) |
| 0x5401a0 (stride 0xc) | end | (60,42,-800), (42,44,-798), (40,25,-798) | (69,26,-800), (252,-90,-798), (-265,40,-798) |

0x5409bc (number of objects that have arrived, kept as maximum) = 0; 0x542148 = 0.

Note the camera matrix flips x: world x = -12000 appears at the **right** of the screen, so
MOTO comes in from the right and RACER from the left. [px]

##### 4.9.2.3 Frame 0x446b80 [asm, px]

1. dt from 0x53f4e0 exactly as page 25 (>500 -> 100). 0x46e140(cam, (0,0,0)) re-looks at
   the origin every frame (no change, the camera is fixed). 0x49b1c0; 0x49a160 (**no clear**:
   the background picture covers the screen).
2. If 0x5409bc > 2 (everything arrived): 0x409bf0 (menu +0x30 = +0x34 = 1), draw picture
   `title4` (0xbf; GP 0xc3) as the whole screen. Frame counter 0x4e90e0: ++, above 10 font
   31 gets yellow/black and "FULL D3D VERSION" is **added** through 0x417d20 (lifetime 0 =
   for ever, Blink with period 500 ms, centred at y 60); above 20 it resets to 0. 0x417d20
   creates the object even when it overlaps a live one (with dodge steps 0 and no
   re-centre every path reaches the creation, asm 0x417e00..0x417f9b), so copies pile up,
   ten every 21 frames, until the 48 slots are full; each blinks 500/500 ms from its own
   start. What is seen depends on the frame rate: under wine (very fast frames) the
   original shows about 600 ms on, 360 ms off (40 captures, build/ref/blink/); at a steady
   60 frames a second the copies' phases cover each other and the text stays on. [asm, px]
3. Otherwise draw the background by 0x5409bc: 0 -> 0xbc `title` (GP 0xc0), 1 -> 0xbd
   `title2`, 2 -> 0xbe `title3`. Each later picture has the arrived objects painted into it,
   so an object stops being drawn in 3D once it has arrived.
   Then for each object k = 0..2: progress += dt/(duration*1000); t clamped to [0,1];
   - k = 0 (ellipse) angles: t < 0.5 -> (a, b, c) = (2*pi*t (0x4d0c08/0x4d0c10), -pi/2, pi);
     t >= 0.5 -> (0, 2*pi*t + pi/2, 0). (Two half-turns about different axes; it ends
     facing the camera.)
   - moto and racer keep angles (0, pi/2, 0).
   - position: progress <= 1 -> lerp start -> end by t; else end, and the arrived count
     increments.
   - the object is drawn (0x48c030) only while 0x5409bc < k + 1.
   0x5409bc = max(0x5409bc, count). 0x4e90e4 accumulates dt.
4. 0x40c1f0, 0x40c500 (the tile transition from page 25 runs over this), 0x49ad90 render,
   effects, cursor, 0x49b0e0.
5. **Leaving**: 0x40c1b0() == 0 (Return / the Start button) -> page 15 (0xf, the name entry).
   No timeout in this routine; the attract mode lives in the menu frame 0x409c00: on page 14,
   when 0x53e5f2 is set and more than 30000 ms passed since menu+0x24 (last input), or when
   0x4dbf80 is set, 0x409a30 starts a demo. [asm; the demo start was not exercised here]

---------------------------------------------------------------------------------------------

#### 4.9.3 The software 3D path

##### 4.9.3.1 Model loader 0x4216a0 and the model structure [asm]

Model structure (0x70 bytes): +0x00 faces (0x40 each), +0x04 vertices (3 floats each),
+0x08 face normals, +0x0c plane d per face, +0x2c face count, +0x30 vertex count,
+0x48.. half extents, +0x58 radius.

**3DF**: u16 version (1), u16 vertex count, vertices as 3 floats each, multiplied by 96.0
(0x4d0690); then faces: u16 n (0 ends the list), u16 colour/attribute (equals n in these
files), n u16 vertex indices.

**MAP**: a 0x200-byte header (bit 7 of byte 1 is a flag), then 0x80 bytes per face:
kind at +0 (< 3 = untextured; 3 or 4 must equal the face's corner count), u values at
+0x22.. and v values at +0x32.. (integers, texels), flags (u32) at +0x48, colour at +0x4e.
Another set of coordinates at +0x0a..+0x1e is not used. An `ANI!` block of 0x80 bytes may
follow (not used by these pages).

Face structure (0x40 bytes): +0x00 draw kind (u16), +0x02 colour RGB565, +0x04..+0x2b five
(u, v) float pairs, +0x2c..+0x34 five u16 vertex indices (+0x34 != 0xffff means a quad),
+0x36 texture page, +0x38 flag bits, +0x3a copy of the draw kind, +0x3c colour 8:8:8
(0x423110).

Corner order (this is the easy thing to get wrong): the builder 0x422c80/0x422be0 stores
the indices reversed, 0x423170 reverses them back and closes the polygon (slot n = slot 0);
the MAP builder 0x422d20/0x422ed0 pairs MAP corner k with **file index k**; then 0x422070
mirrors every vertex x -> -x and 0x423170 reverses every face again (so the winding stays
front-facing after the mirror). Net effect: x negated, corner order reversed, each corner
keeps its own MAP (u,v). [asm, px]

0x4232e0 multiplies u, v by 1/256 (0.00390625). Half-texel adjustments only when page +0x254
bit 3 or Direct3D - never in these pages. [asm]

##### 4.9.3.2 Angles and the cosine table [asm]

0x5fcb80: 16384 floats, COS[k] = cos(k*2*pi/16384), filled by 0x4901e0. Angle to index:
trunc(angle * 2607.594588) & 0x3fff (double constants 0x4d1bf8 / 0x4d0ca8 / 0x4d0ed0;
2607.59 = 16384/(2*pi)); sin(x) = COS[(n - 0x1000) & 0x3fff]. Truncation, not rounding
(_ftol 0x4b3048). The port must use the table to be exact (COS[0x3000] is ~0, not 0).

##### 4.9.3.3 Camera and object transform: 0x46e140 and 0x48c030 [asm, px]

- Camera block (0x238 bytes, 0x5161c0 + 0x4dc0f8*0x238): look-at matrix C at +0x68..+0x88
  (3x3), translation +0x8c..+0x94, position +0x98..+0xa0. 0x46e140(cam, target) builds C
  from the direction position->target; for (0,0,4500) -> (0,0,0) it gives diag(-1, 1, -1).
- 0x48c030(obj, renderer *0x4dc0c0, camera): row-vector rotation
  M = Rz(c)*Ry(b)*Rx(a), entries (cX = cos, sX = sin of the angle, from the table):
  M0 = cc*cb, M1 = sc*ca + cc*sb*sa, M2 = sc*sa - cc*sb*ca,
  M3 = -sc*cb, M4 = cc*ca - sc*sb*sa, M5 = cc*sa + sc*sb*ca,
  M6 = sb, M7 = -cb*sa, M8 = cb*ca.
  View matrix N = M * C; translation T = (pos - campos) * C + cam(+0x8c..0x94).
  0x4039c0 copies; then 0x49b2c0(matrix, vertices, count, 0x1f) transforms.
- The object scale field +0x94 = 1.0 is not used in these pages (scale was baked in by
  0x4204b0).

##### 4.9.3.4 Vertex transform and projection 0x49b2c0 [asm, px]

Sets the x87 precision to 24 bits (PC=single) with round-to-nearest; the port must compute
in float32 for exact pixels. Output records of 0x1c bytes at 0x6d20d8:
+0x00 X*fx, +0x04 Y*fy, +0x08 Z, +0x0c light (0x1f << 13), +0x10 sx, +0x14 sy, +0x18 1/Z.
A vertex is projected only if Z >= near:
sx = round(cx + X*fx/Z), sy = round(cy - Y*fy/Z) (round to nearest through fistp).

Constants for the menu (set by the menu at 0x40a003 via 0x49a0e0/0x49a100, and 0x40a01d,
0x40a037):
- fx = fy = 1911.0 (bits 0x44eee000). (The generic setup 0x4995a0 would use 307200/W and
  230400/H; the menu overrides it.)
- centre cx = 320, cy = 240 (0x60d3c4, 0x624f88, from 0x499ba0(0,0,640,480)).
- near = 256 (0x625774; races use 64). far = 40000 (0x6d1f70).
- 0x62e444 = 1: s-buffer mode.

##### 4.9.3.5 Face setup and sorting 0x49b7f0 / 0x49c590 [asm, px]

0x48c030 calls 0x49b7f0(vertices, faces, count, 1) because object +0x14c == -1 (otherwise
0x49c110). Per face:
- kind 0xd is skipped.
- key = Z of the farthest of the four corners. Skip if that farthest Z <= near.
- if any corner has Z <= near: 3D back-face test (triple product of the corners in view
  space); else 2D: trivial reject if all corners are left of 0, right of 639, above 0 or
  below 479, and the screen area (y2-y1)(x1-x0) + (y0-y1)(x2-x1) must be > 1.0 (so only
  clockwise-on-screen faces survive).
- 0x49c590(face, key (+1024 if flag bit 1, constant 0x4d1f94), vertices, flags, 0):
  bucket = trunc(key * 0.25) (0x4d1f9c); < 0 dropped; >= 7999 clamped to 7998. Buckets are
  LIFO lists (heads 0x655d38, entries 0x18 bytes at 0x65f0e0, at most 7499). The menu skips
  the detail-level kind table.

0x49a160 at frame start resets: vertex counter 0x68b014, face count 0x655d14, every screen
row's free list to one interval [0, 640), 0x6d20c0 = 1, deferred counters 0x6d1048 /
0x6d1f78 = 0.

##### 4.9.3.6 Rendering 0x49ad90 (software, s-buffer) [asm, px]

Front to back: buckets ascending (nearest first); within a bucket from the list head (last
inserted first); at most 2000 faces (0x62e438). Each face goes through 0x4aef20, which looks
up the draw kind in the kind table 0x4f1a70 (16-byte entries {id, sub, flag, span routine},
re-indexed by id at startup by 0x4a2ea0 / 0x4a2e00) and the sub table 0x4f1ee0 ({id,
polygon routine, edge routine, edge routine when flag}).

For the kinds used here (16, 21, 38, 39) sub = 4: polygon routine 0x49e510, edge routine
0x49d9e0 (opaque, flag 0) or 0x49dbf0 (see-through, flag 1).

- **0x49e510 polygon**: fan (0,1,2), (0,2,3). Texture coordinates become integers
  ftol(u * 2^21) (0x4d1fa8; u is texels/256, so this is texels * 8192, 13 fraction bits).
  Near clipping per triangle at Z < near: one corner in front -> one triangle (F, P1, P2);
  two in front -> (F1, F2, P1) and (F2, P2, P1). A clip point P is projected by 0x49d8f0 as
  sx = cx + trunc(X/near), sy = cy - trunc(Y/near) (X, Y already carry fx, fy), and its uv =
  uB + trunc(near - zB) * ((uF - uB) / trunc(zF - zB)).
- **0x49dde0 triangle** (x0,y0,x1,y1,x2,y2, page, u0,v0,u1,v1,u2,v2): rotate the corners so
  corner 0 is topmost; D = (x0-x2)(y1-y2) - (x1-x2)(y0-y2) truncated to int, must be >= 1
  (back faces and slivers dropped). Constant gradients (affine, **no perspective
  correction**): du/dx = trunc((u0-u2)/D*(y1-y2) - (u1-u2)/D*(y0-y2)) (0x655d20), dv/dx the
  same (0x655d24). Edges in 13-bit fixed point (x << 13, slope = (dx << 13)/dy as C integer
  division, no sub-pixel prestep); u and v interpolated down the edge 0 -> 2 (0x624b0c,
  0x68b018 / 0x68b01c, per-row steps (u2-u0)/h as integers); the other side at 0x624b44.
  Rows above 0 are skipped (advancing the edges), rows beyond 479 cut. Per row, if the row's
  free list is not empty, the span [xl >> 13, xr >> 13) is passed to the edge routine.
- **0x49d9e0 (opaque, s-buffer)**: draws the parts of the span that are still free (u,v
  advanced by (a - start)*du for each piece) and removes them from the row's free list (new
  list nodes from 0x60d3d0). Nearer faces were drawn first, so this is a z-buffer-free hidden
  surface removal.
- **0x49dbf0 (see-through)**: records the visible pieces (0x68b040, 0x1c-byte records, at
  most 0x2800) without covering them; 0x49dde0 also records the triangle batch (0x625790,
  0x24 bytes, at most 1000). 0x4af130 replays the batches **last first** after all buckets.
- **Span routines** (kind table): 16 -> 0x4a5ab0 opaque (self-modifying code, texel at
  [v & 255][u & 255]); 21 -> 0x4a5e10 opaque, colour (c >> 1) & 0x7bef (mask at 0x50270c,
  set by 0x401fc0; i.e. half brightness); 38 -> 0x4a5330 skips texel value 0 (see-through);
  39 -> 0x4a5580 skips 0 and halves. For 21/38/39 the texel index is
  ((v >> 13)*256 + (u >> 13)) & 0xffff. No lighting: the light value in the vertex is not
  used by these span routines.
- 0x49b0e0 fills the spans still free with the colour 0x65f0d8 only when 0x625748 is set
  (not in these pages; page 25 clears with COLORFILL, page 14 draws its background first).

An equivalent painter's algorithm (what tools/model.py does): buckets from far to near,
within a bucket in insertion order; opaque spans overwrite, see-through spans skip 0; see-
through batches are drawn in the same order as recorded but only over pixels no opaque
face covered. Pixel-exact results need the float32 transform, the cosine table, the
truncations above and the s-buffer span boundaries.

##### 4.9.3.7 Texture pages 0x401160 [asm, px]

RAW (Image Alchemy) header big-endian: width at +8, height +0xa, palette count +0xc; width
must be >= 256. When the palette count is 0 the image is 24-bit: each row's first 256
texels are read as R, G, B and the rest of the row (w*3 - 0x300 bytes) skipped. A texel is
stored as RGB565 built from the DirectDraw pixel format: (R >> 3) << 11 | (G >> 3) << 6 |
(B >> 3). **Green uses only 5 bits** (its lowest bit is always 0) because the shifts come
from per-channel bit counts 5/6/5 and positions 11/5/0 (globals 0x4ee9ac.., 0x4ee9c4..,
masks 0x4ee9d0..0x4ee9d8 set by 0x473c3c) applied as "top 5 bits then shift". (LEZ menu
pictures use G >> 2, a full 6 bits.) The paletted RAW path converts the palette the same
way then maps indices. 0x43e280 is **not** involved (that is the LEZ/TGA picture path).
Stored as 256 u16 per row at page object +0x330. Texel value 0 is the see-through colour
for kinds 38/39 (pure black RGB < 8,8,8 becomes 0). [asm, px]

---------------------------------------------------------------------------------------------

#### 4.9.4 MAP face flags (u32 at MAP face +0x48) [asm]

For textured faces (MAP kind 3/4) and the loader's last argument = 1 (all these pages):

| flag | effect |
|---|---|
| (none) | draw kind 0x10 (16): opaque textured. (Argument 0 -> kind 1.) |
| 0x10000 | kind 0x26 (38): see-through texture (texel 0 skipped), **two-sided** in practice because the models carry both windings (argument 0 -> kind 2) |
| 0x200000 | kind 0x15 (21): half-brightness texture; with 0x10000 -> 0x27 (39) see-through and half brightness |
| 0x20000 | face flag bit 0 (+0x38) |
| 0x400000 | flag bit 1: sort key + 1024 (drawn as if farther: decals/backgrounds) |
| always | flag bit 2 |
| 0x100000 | flag bit 3 |
| 0x40000 | kind 0x0d: never drawn (skipped in 0x49b7f0) |
| 0x80000 | kind 8: span routine 0x4a5a90 does nothing |
| low 16 bits | texture page number (plus the 0x422250 offset) |

Untextured faces (MAP kind < 3): kind 0 (solid colour, 0x423150), 0x10000 -> 2,
0x40000 -> 0xd, 0x80000 -> 0xc. If the page is loaded and all four (u, v) are equal, the
face is turned into a flat face of that texel's colour.

Bits 0 and 3 of +0x38 had no visible effect in these pages ([guess]: lighting / collision
uses in races).

Flags used by these models: camera 0x200001 / 0x1 (page 1; kinds 21 / 16); D, S, I
0x200000 / 0 (kinds 21 / 16); DsInt, moto, racer, ellipse 0x110000 (kind 38) and 0x310000
(kind 39): flat two-sided billboards in the model's YZ plane (they face the camera after the
b = +-pi/2 yaw).

---------------------------------------------------------------------------------------------

#### 4.9.4b In the port (`r3d.c`, `model.c`, `page_intro.c`, `rand.c`)

Written from 4.9 and the asm, every step a float as the game's (x87 at 24 bits; built with
`-ffp-contract=off`). Checked: the logo against the original 1108 px (end) and 3798 px
(14.6 s) at the best 60 Hz frame; against tools/model.py (given the same camera lag) 306 px
at 14.6 s; the glass as above; the settled title page equal to the original apart from
the pointer (with the pointer on Start, which the original's capture had selected).

#### 4.9.5 What the port needs, in order

1. RAW texture page loader (24-bit and paletted, green 5 bits, 256 texels per row).
2. 3DF + MAP loader with the corner order and mirror of 3.1, the scales and (page 25) the
   recentre.
3. The cosine table, the float32 transform, projection and the sort/clip rules (3.2-3.5).
4. The affine triangle with 13-bit fixed point and the four span kinds; the s-buffer (or the
   painter's equivalent).
5. The two page scripts (1.3, 2.3) with the dt rule (>500 ms -> 100 ms) and progress sums,
   the 1.2 s tile transition between them, Escape on page 25, Return on page 14, and the
   blinking FULL D3D VERSION text (frame counted: 10 on, 10 off at the game's frame rate).

---------------------------------------------------------------------------------------------

#### 4.9.6 Verification against the running original

- Page 25: tools/model.py `dsi` at 19923.5 ms vs build/ref/orig_p25_logo_end.png: 11
  differing pixels of 307200; at 14592 ms vs build/ref/orig_p25_logo_a.png: 28 pixels.
  16 further captures 1.25 s apart fitted with consistent time offsets; residues 14-6776
  pixels, all texel noise on slivers / big camera faces from sub-millisecond timing; the
  orbit, spin, camera-model swing and DsInt approach all match. [px]
- Page 14: captures after the automatic end of page 25 (about 20 s). The fitted model times
  advance by the same 140 ms as the captures (3.02 s -> 1124 ms, 3.16 s -> 1266 ms);
  differences are the Start button (not drawn by model.py) and a few edge pixels; the MOTO
  texture is itself motion-blurred (it is in the page, not an effect). Final state matches
  apart from the blinking text. See build/png/models/title_vs_orig.png (left original,
  right model.py). [px]
- Not verified: the GP edition tables (no GP copy run), the Escape skip on page 25 (xdotool
  key events did not reach the game in the headless session), the 30 s attract-demo
  timeout.

### 4.10 Sound: effects, voices, DirectSound, CD music (mapped 2026-10-26)


game.EXE 1998 GOG build (SHA-1 164b4175..). Addresses are the program's. Confidence:
[asm] read in build/mr.asm; [decomp] Ghidra only; [test] checked on the data; [guess].

#### 4.10.0 Summary for the port

- All sound effects and voices are **125 RIFF WAVE files in SFX.BKF, twice** (`16_BITS\...`
  and `8_BITS\...`), **enciphered with a trivial byte-add** (below). Mono PCM, 5000–22050 Hz,
  16-bit signed or 8-bit unsigned. No other sound files exist in any bank (language banks
  MENUxx hold only pictures and a .GEN; no speech per language: voices are English only).
  `tools/sfx.py` decodes them; `build/sfx/` holds all 250 as plain .wav [test].
- A sound **id (0..255)** maps to a file through **7 tables in the program** of
  `{char *name, int id}` (8 bytes): see §2. The path is
  `data\sfx\` + (`16_Bits\` if Wave16Bits else `8_Bits\`) + folder + name + `.wav`.
- **0x4049f0 play(id, flags)**: ids 0x2f..0x5c are *voices* (switch 0x4db668, volume 0xff),
  everything else *effects* (switch 0x4db664, volume 0xee). Flags: bit0 loop, bit1 use a free
  copy (overlap) instead of restarting, bit2 don't play if this id was the last one played
  in its class. Menu calls all use flags 0 (restart the sound from its start).
- **Volume** v 0..255 → DirectSound attenuation (v−255)×15.625 hundredths of dB (v=0 →
  −39.8 dB). **Pan** p −128..128 → ±p×31.25 hundredths of dB on the other side.
  **Frequency** in Hz, accepted only 1000 < f < 44100.
- **Queued/delayed sounds**: 8 records of 100 bytes at 0x504240 (menu voice prompts after
  600 ms) + 1 sequential voice queue at 0x5028c8 (race announcements); updated once a frame
  by 0x404eb0 (Menu_Frame) [asm].
- **Music** = CD audio tracks 2..13 through MCI. Real CD: plays the track once, mode 2/3
  repeats it **once** (a 1 s timer replays it when MCI says "stopped"). GOG's winmm.dll
  replaces MCI by `PlaySoundA("NN.wav", SND_LOOP|SND_ASYNC)`: **every track loops forever**
  [asm, both]. Track per page/track in §5.

#### 4.10.1 The files

##### 4.10.1.1 Where they are
- `data\SFX.BKF` (28 banks total; the others hold no sound): 250 .WAV =
  2 × (MENU 9, VOICES 31, VOICES\MENU 12, DIVERS 46, KTM 16, NINJA 11). The bank names are
  `16_BITS\MENU\SELECT.WAV` etc.; the game asks for `data\sfx\16_Bits\Menu\select.wav` and
  the bank lookup is case-insensitive (dsi_fopen through the banks, NOTES §3.1).
- `02.wav`..`13.wav` beside game.EXE: the CD's audio tracks (§5). 44100 Hz, 16-bit,
  stereo, plain PCM RIFF [test: python wave]. `build/cd/NN.wav` (cut from the CD image) have
  identical format and the same frame counts (e.g. 03: 6325704 frames both).
- No `data\*.wav` loose files; no per-bike sound bank (the bike sounds are SFX\KTM and
  SFX\NINJA); no per-language speech.

##### 4.10.1.2 Format and cipher (loader 0x440d30, decipher 0x440d00) [asm-checked decomp + test]
`0x440d30(path, WAVEFORMATEX *out, void **data, int *size)`:
1. open (0x42ee60/dsi_fopen mode "rb"), read **0x24 bytes**. If the first byte is `'r'`
   (0x72), the file is enciphered and **each block read is deciphered from i = 0**:
   `plain[i] = stored[i] - 4*(i + 8)  (mod 256)`  (0x440d00: `b += (-8 - i) * 4`).
2. seek `fmtSize - 16` forward (fmtSize = dword at 0x10 of the header), read **8 bytes**
   (decipher again from i = 0): must be `'data'`, size.
3. Require `RIFF`/`WAVE`/`fmt `/`data` at their places (else error 2). GlobalAlloc(size),
   read size bytes, decipher (from i = 0).
4. WAVEFORMATEX built by hand: tag 1 (PCM), channels and rate and bits from the file,
   blockAlign = bits/8 × channels, avgBytes = blockAlign × rate, cbSize 0.
All 250 files are enciphered ('r'), tag 1, mono. 16_BITS = signed 16-bit LE; 8_BITS =
unsigned 8-bit. Many 8-bit files have the RIFF pad byte (+1 after data); BIRD3 and FETE have
32/92 junk bytes after the data (ignored).

Test [test]: every 16-bit file correlates with its 8-bit twin (8-bit minus 128) at median
0.99988 (worst CONTINUE.WAV 0.80: 5 kHz, very coarse 8-bit version); lengths agree to the
sample; zero-crossing rates are those of sounds, not noise (e.g. SELECT 22050 Hz: 3364/s;
the voices 11025 Hz: 1300..4000/s). Rates found: 5000, 8000, 11025, 14000, 16000, 19000,
22050 Hz.

`tools/sfx.py SFX.BKF` lists every file (format, length, peak, zc/s); `-x DIR` writes
plain .wav. It also takes loose files.

##### 4.10.1.3 8 or 16 bits
`0x4db670` = Wave16Bits (CONFIG.BPT; 1 on both the GOG and the CD config). Forced to 0 at
start (0x408720) if the primary buffer did not come out 16-bit. 0x408f30 toggles it and
reloads everything. The port should simply use 16_BITS.

#### 4.10.2 Ids → files (the tables in game.EXE)

Handles: `int 0x519ff8[256]` = sample handle by id (−1 none). Every loader first fills it
with −1, then for each record: path as above, `h = 0x423f80(path)` (load), store,
`0x424030(h, 0xff)` (volume full), `0x424070(h, 0)` (pan centre) [asm].

Two sound sets, never both loaded:
- **Menu set** — `0x403f90` called from 0x40a9a0 (the menu's resource loader, when the menu
  becomes active): menu table (7) and, if voices are on (0x4db668) and not loaded yet
  (0x503554 == 0), the voices-menu table (11); 0x503554 = 1. Freed by 0x4042c0 (from 0x40b1f0,
  the menu's resource release). Page 24 (podium) adds the podium table in its start routine
  (0x404310 → ids 4, 5), freed in its end routine (0x404490).
- **Race set** — `0x4044c0(bikeType)` from 0x410d70 ("Init WAV samples...") at race
  load: divers (18), then ktm (16) if bikeType != 0 else ninja (11), then the track's
  ambient samples (0x4059a0, from `Sound.Spt`, ids from 0x5d up: 0x519ff0 = 0x5d), then
  voices (30) if voices are on. bikeType = `[0x509398 + 0x124*bike]` (bike 0x584499):
  0 = the "Ninja" road bikes, else KTM (the Go sound on page 10 agrees: type 0 → goninja).

0x504874/0x5028a8/0x5028a4 remember which voice table/count/folder is loaded (for the
voices toggle 0x408f30 to reload).

**menu (0x403f90, entering the menu)** table 0x4db7d8, 7 records, folder `MENU\`

| id | rec addr | name | 16-bit file | rate | ms |
|---|---|---|---|---|---|
| 0x28 | 0x4db7d8 | deplett | MENU/DEPLETT.WAV | 16000 | 671 |
| 0x29 | 0x4db7e0 | select | MENU/SELECT.WAV | 22050 | 436 |
| 0x2a | 0x4db7e8 | valide | MENU/VALIDE.WAV | 16000 | 1379 |
| 0x2b | 0x4db7f0 | goktm | MENU/GOKTM.WAV | 16000 | 839 |
| 0x2c | 0x4db7f8 | goninja | MENU/GONINJA.WAV | 16000 | 697 |
| 0x2d | 0x4db800 | title | MENU/TITLE.WAV | 22050 | 1696 |
| 0x03 | 0x4db808 | continue | MENU/CONTINUE.WAV | 5000 | 592 |

**voices-menu (0x403f90 if voices on)** table 0x4db780, 11 records, folder `Voices\Menu\`

| id | rec addr | name | 16-bit file | rate | ms |
|---|---|---|---|---|---|
| 0x36 | 0x4db780 | entrname | VOICES/MENU/ENTRNAME.WAV | 11025 | 1009 |
| 0x52 | 0x4db788 | slctrans | VOICES/MENU/SLCTRANS.WAV | 11025 | 1273 |
| 0x53 | 0x4db790 | selerace | VOICES/MENU/SELERACE.WAV | 11025 | 1363 |
| 0x54 | 0x4db798 | selectmd | VOICES/MENU/SELECTMD.WAV | 11025 | 1229 |
| 0x55 | 0x4db7a0 | playsolo | VOICES/MENU/PLAYSOLO.WAV | 11025 | 859 |
| 0x56 | 0x4db7a8 | playlink | VOICES/MENU/PLAYLINK.WAV | 11025 | 855 |
| 0x57 | 0x4db7b0 | makurchc | VOICES/MENU/MAKURCHC.WAV | 11025 | 1061 |
| 0x58 | 0x4db7b8 | letsgo | VOICES/MENU/LETSGO.WAV | 11025 | 1322 |
| 0x59 | 0x4db7c0 | cngrecor | VOICES/MENU/CNGRECOR.WAV | 11025 | 3209 |
| 0x5a | 0x4db7c8 | cngratim | VOICES/MENU/CNGRATIM.WAV | 11025 | 3064 |
| 0x5b | 0x4db7d0 | yutrygan | VOICES/MENU/YUTRYGAN.WAV | 11025 | 2818 |

**podium (0x404310, page 24 start)** table 0x4db770, 2 records, folder `Menu\`

| id | rec addr | name | 16-bit file | rate | ms |
|---|---|---|---|---|---|
| 0x04 | 0x4db770 | podium1 | MENU/PODIUM1.WAV | 22050 | 3552 |
| 0x05 | 0x4db778 | podium2 | MENU/PODIUM2.WAV | 22050 | 4502 |

**divers (0x4044c0, race)** table 0x4db810, 18 records, folder `Divers\`

| id | rec addr | name | 16-bit file | rate | ms |
|---|---|---|---|---|---|
| 0x01 | 0x4db810 | bip1 | DIVERS/BIP1.WAV | 8000 | 582 |
| 0x02 | 0x4db818 | bip2 | DIVERS/BIP2.WAV | 8000 | 1360 |
| 0x06 | 0x4db820 | chckpvox | DIVERS/CHCKPVOX.WAV | 8000 | 628 |
| 0x03 | 0x4db828 | continue | DIVERS/CONTINUE.WAV | 5000 | 592 |
| 0x28 | 0x4db830 | deplett | DIVERS/DEPLETT.WAV | 16000 | 671 |
| 0x29 | 0x4db838 | select | DIVERS/SELECT.WAV | 22050 | 436 |
| 0x2e | 0x4db840 | pause | DIVERS/PAUSE.WAV | 11025 | 1417 |
| 0x0a | 0x4db848 | derap | DIVERS/DERAP.WAV | 16000 | 496 |
| 0x1a | 0x4db850 | brake1 | DIVERS/BRAKE1.WAV | 16000 | 408 |
| 0x1b | 0x4db858 | brake2 | DIVERS/BRAKE2.WAV | 16000 | 620 |
| 0x12 | 0x4db860 | crash | DIVERS/CRASH.WAV | 16000 | 870 |
| 0x11 | 0x4db868 | rouleman | DIVERS/ROULEMAN.WAV | 11025 | 1987 |
| 0x14 | 0x4db870 | wterdeb | DIVERS/WTERDEB.WAV | 8000 | 1064 |
| 0x15 | 0x4db878 | wterpas | DIVERS/WTERPAS.WAV | 8000 | 944 |
| 0x16 | 0x4db880 | wterfin | DIVERS/WTERFIN.WAV | 8000 | 1130 |
| 0x17 | 0x4db888 | gravier | DIVERS/GRAVIER.WAV | 16000 | 1762 |
| 0x18 | 0x4db890 | glace | DIVERS/GLACE.WAV | 16000 | 1306 |
| 0x19 | 0x4db898 | herbe | DIVERS/HERBE.WAV | 22050 | 896 |

**ktm (0x4044c0 arg != 0)** table 0x4db8a0, 16 records, folder `Ktm\`

| id | rec addr | name | 16-bit file | rate | ms |
|---|---|---|---|---|---|
| 0x21 | 0x4db8a0 | motostp | KTM/MOTOSTP.WAV | 16000 | 2532 |
| 0x26 | 0x4db8a8 | Gear5 | KTM/GEAR5.WAV | 16000 | 1517 |
| 0x25 | 0x4db8b0 | Gear4 | KTM/GEAR4.WAV | 16000 | 1513 |
| 0x24 | 0x4db8b8 | Gear3 | KTM/GEAR3.WAV | 16000 | 1412 |
| 0x23 | 0x4db8c0 | Gear2 | KTM/GEAR2.WAV | 16000 | 1607 |
| 0x22 | 0x4db8c8 | Gear1 | KTM/GEAR1.WAV | 16000 | 1122 |
| 0x1d | 0x4db8d0 | depart1 | KTM/DEPART1.WAV | 16000 | 710 |
| 0x1e | 0x4db8d8 | depart2 | KTM/DEPART2.WAV | 16000 | 945 |
| 0x1f | 0x4db8e0 | depart3 | KTM/DEPART3.WAV | 16000 | 692 |
| 0x1c | 0x4db8e8 | grince | KTM/GRINCE.WAV | 16000 | 771 |
| 0x0e | 0x4db8f0 | choc1 | KTM/CHOC1.WAV | 16000 | 480 |
| 0x0f | 0x4db8f8 | choc2 | KTM/CHOC2.WAV | 16000 | 577 |
| 0x0b | 0x4db900 | land1 | KTM/LAND1.WAV | 16000 | 349 |
| 0x0c | 0x4db908 | land2 | KTM/LAND2.WAV | 19000 | 343 |
| 0x0d | 0x4db910 | land3 | KTM/LAND3.WAV | 16000 | 456 |
| 0x13 | 0x4db918 | wheel | KTM/WHEEL.WAV | 16000 | 368 |

**ninja (0x4044c0 arg == 0)** table 0x4db920, 11 records, folder `Ninja\`

| id | rec addr | name | 16-bit file | rate | ms |
|---|---|---|---|---|---|
| 0x21 | 0x4db920 | motostp | NINJA/MOTOSTP.WAV | 16000 | 2460 |
| 0x26 | 0x4db928 | Gear5 | NINJA/GEAR5.WAV | 14000 | 9751 |
| 0x22 | 0x4db930 | Gear1 | NINJA/GEAR1.WAV | 22050 | 666 |
| 0x1d | 0x4db938 | depart1 | NINJA/DEPART1.WAV | 16000 | 312 |
| 0x1e | 0x4db940 | depart2 | NINJA/DEPART2.WAV | 16000 | 614 |
| 0x1f | 0x4db948 | depart3 | NINJA/DEPART3.WAV | 16000 | 687 |
| 0x1c | 0x4db950 | suspens1 | NINJA/SUSPENS1.WAV | 16000 | 608 |
| 0x0e | 0x4db958 | choc1 | NINJA/CHOC1.WAV | 16000 | 430 |
| 0x0f | 0x4db960 | choc2 | NINJA/CHOC2.WAV | 16000 | 661 |
| 0x0b | 0x4db968 | land | NINJA/LAND.WAV | 16000 | 390 |
| 0x13 | 0x4db970 | wheel | NINJA/WHEEL.WAV | 14000 | 290 |

**voices (0x4044c0 if voices on)** table 0x4db680, 30 records, folder `Voices\`

| id | rec addr | name | 16-bit file | rate | ms |
|---|---|---|---|---|---|
| 0x30 | 0x4db680 | 3 | VOICES/3.WAV | 11025 | 917 |
| 0x31 | 0x4db688 | 2 | VOICES/2.WAV | 11025 | 896 |
| 0x32 | 0x4db690 | 1 | VOICES/1.WAV | 11025 | 914 |
| 0x33 | 0x4db698 | go | VOICES/GO.WAV | 11025 | 895 |
| 0x34 | 0x4db6a0 | ckckpnt | VOICES/CKCKPNT.WAV | 11025 | 973 |
| 0x37 | 0x4db6a8 | finlap | VOICES/FINLAP.WAV | 11025 | 1147 |
| 0x38 | 0x4db6b0 | fuel | VOICES/FUEL.WAV | 11025 | 1067 |
| 0x39 | 0x4db6b8 | gameover | VOICES/GAMEOVER.WAV | 11025 | 1087 |
| 0x3a | 0x4db6c0 | lechef | VOICES/LECHEF.WAV | 11025 | 4250 |
| 0x3b | 0x4db6c8 | newrecor | VOICES/NEWRECOR.WAV | 11025 | 1750 |
| 0x3c | 0x4db6d0 | onegear | VOICES/ONEGEAR.WAV | 11025 | 2354 |
| 0x3d | 0x4db6d8 | oouups | VOICES/OOUUPS.WAV | 11025 | 654 |
| 0x3e | 0x4db6e0 | racecmpl | VOICES/RACECMPL.WAV | 11025 | 1316 |
| 0x3f | 0x4db6e8 | runlast | VOICES/RUNLAST.WAV | 11025 | 2798 |
| 0x40 | 0x4db6f0 | running4 | VOICES/RUNNING4.WAV | 11025 | 1853 |
| 0x41 | 0x4db6f8 | stillrun | VOICES/STILLRUN.WAV | 11025 | 1914 |
| 0x42 | 0x4db700 | timeout | VOICES/TIMEOUT.WAV | 11025 | 865 |
| 0x44 | 0x4db708 | tryagain | VOICES/TRYAGAIN.WAV | 11025 | 1105 |
| 0x45 | 0x4db710 | watarokt | VOICES/WATAROKT.WAV | 11025 | 1754 |
| 0x46 | 0x4db718 | wrongway | VOICES/WRONGWAY.WAV | 11025 | 1058 |
| 0x47 | 0x4db720 | youlost | VOICES/YOULOST.WAV | 11025 | 1976 |
| 0x48 | 0x4db728 | yuralosr | VOICES/YURALOSR.WAV | 11025 | 1701 |
| 0x49 | 0x4db730 | yurdalst | VOICES/YURDALST.WAV | 11025 | 1752 |
| 0x4a | 0x4db738 | opopadow | VOICES/OPOPADOW.WAV | 11025 | 2378 |
| 0x4b | 0x4db740 | ouch | VOICES/OUCH.WAV | 11025 | 796 |
| 0x4c | 0x4db748 | yeehaaa | VOICES/YEEHAAA.WAV | 11025 | 2689 |
| 0x4e | 0x4db750 | yeees | VOICES/YEEES.WAV | 11025 | 1465 |
| 0x4f | 0x4db758 | yeaah | VOICES/YEAAH.WAV | 11025 | 1161 |
| 0x50 | 0x4db760 | WellDone | VOICES/WELLDONE.WAV | 11025 | 1147 |
| 0x51 | 0x4db768 | wouaou | VOICES/WOUAOU.WAV | 11025 | 858 |

Files in the bank that no table names: `VOICES\BESTLAP`, `VOICES\MENU\CHSEMSTR`, and most of
DIVERS (AIGLE1-3, AIRPLANE, BERCY*, BIRD1-3, CORSAIRE, FETE, FOULE, GODZILLA, HELICO, HURRY,
METRO, MOUET-1..3, RESSAC, SABLE, SAUCER, TERRE, VENT): the latter are the tracks' ambient
sounds, loaded by name from each track's Sound.Spt (§6.2). No id 0x35, 0x43, 0x4d. No caller
of 0x52 (slctrans) was found (unused?) [asm: not among the 21 calls of 0x4049f0, 4 of 0x404e10,
10 of 0x404da0].

For the port: the tables are in game.EXE (.data, initialised): read them at the addresses
above like the other tables (name pointer → string in .data at 0x4db9xx..). The folder
strings: 0x4db9b8 `data\sfx\`, 0x4db9ac `16_Bits\`, 0x4db9a4 `8_Bits\`, 0x4db9d4 `Menu\`,
0x4db9c4 `Voices\Menu\`, 0x4db9e4 `Divers\`, 0x4db9ec `Ktm\`, 0x4db9f4 `Ninja\`,
0x4db9dc `Voices\`, 0x4db99c `.wav`.

#### 4.10.3 The DirectSound layer ("CfxServer", fx\*.cpp; object 0x103c bytes)

Wrapper object at `[0x516aa0]`: +0 → CfxServer, +4 = "server alive". Thin methods
(all skip a handle of −1 or a dead server) [decomp, simple]:

| wrapper | server | what |
|---|---|---|
| 0x423eb0(hwnd) | 0x441050 ctor | create (new 0x103c) |
| 0x423f50 | 0x440f30 | destroy |
| 0x423f80(path) | 0x4415a0 | load → handle |
| 0x423fb0(h) | 0x441960 | free (h and its copies) |
| 0x423fd0(h, flags) | 0x441a40 | play → handle of the copy used |
| 0x424000(h) | 0x441ca0 | stop |
| 0x424020 | 0x441cf0 | stop everything |
| 0x424030(h, vol) | 0x441d50 | volume 0..255 |
| 0x424050(h, hz) | 0x441dc0 | frequency |
| 0x424070(h, pan) | 0x441e10 | pan −128..128 |
| 0x424090(on) | 0x441e80 | pause all (1) / resume (0) |
| 0x4240b0 | 0x441f90 | "paused?" |
| 0x4240d0(h) | 0x441fa0 | length in ms |

##### 4.10.3.1 Creation 0x441050 [decomp; numbers asm-checked]
DirectSoundCreate; SetCooperativeLevel PRIORITY (2), falling back to NORMAL (1) on
DSERR_... 0x88780000a (log "Warning normal cooperative level used"). GetCaps: +0xc = caps
PRIMARY16BIT, +0x14 = PRIMARYSTEREO, +0x18 = !CERTIFIED?(0x20 bit), +0x10 = has hw mixing.
Primary buffer (DSBCAPS_PRIMARYBUFFER), and with PRIORITY: SetFormat **22050 Hz (0x5622),
stereo if the card can, 16-bit if the card can**. Then 0x4413b0 reads the actual primary
format back: +0x24 rate, +0x28 bits, +0x2c stereo. `-NoSound` (0x4dc1a8) disables it all.
At start 0x408720 then calls **0x4414c0(stereo=Stereo setting 0x503530, bits16=Wave16Bits,
rate=ReplayRate 0x504568)** = 22050 in both known configs. 0x409570 (options) toggles
quality: 22050 ↔ 11025 (0x2b11), 16/8-bit, stereo/mono.
For the port: a 44100/48000 Hz stereo float mix is fine; the original's software mixer ran at
22050 Hz (so its output had no content above 11 kHz) [guess on whether this is audible].

##### 4.10.3.2 Samples 0x4415a0 / 0x440af0 [decomp]
Slot table: CfxServer+0x30 = 1024 pointers to 0x30-byte sample objects; +0x1030 = high
water mark. A handle is a slot index. Load: new sample object, 0x440af0 creates a secondary
buffer: DSBUFFERDESC size 0x14, **flags = 0xe0 (CTRLFREQUENCY|CTRLPAN|CTRLVOLUME) | 2
(STATIC) if the card has more than 4 free hardware static buffers**, size = data size,
format from the file; Lock/copy/Unlock. Then **4 duplicates** (DuplicateSoundBuffer) are
made at once, chained through +4 ("next copy"), so every sample has 5 voices ready.
Sample object: +0x14 frequency (starts at the file's rate), +0x18 last play flags,
+0x1c IDirectSoundBuffer, +0x20 "has buffer", +0x24 "paused by pause-all", +0x28 "is a
duplicate" (shares data), +0xc data size, +0x10 WAVEFORMATEX copy.

##### 4.10.3.3 Play 0x441a40(h, flags) [decomp; flags path asm-checked at 0x440ec0]
- **flags & 2**: walk the chain h → next... for the first copy whose GetStatus has no
  PLAYING bit; if all play, make one more duplicate (no limit but the 1024 slots) and append
  it. That copy is played and its handle returned.
- otherwise: the head buffer itself is used (a sound already playing is cut and restarted).
- 0x440ec0(copy, flags): SetCurrentPosition(0); +0x18 = flags; **Play(0, 0, flags & 1 ?
  DSBPLAY_LOOPING : 0)** [asm].
Stop 0x441ca0: IDirectSoundBuffer::Stop, +0x24 = 0 (position not reset; the next play resets
it). Free 0x441960 releases the whole chain.

##### 4.10.3.4 Volume, pan, frequency, length [asm-checked decomp]
- 0x441d50(h, v): clamp v to 0..255; `SetVolume(((v*5 - 0x4fb) * 800) / 256)` (C division,
  truncation towards zero) = (v−255)×15.625 hundredths of dB: 255 → 0 dB, 238 (0xee) →
  −265 (−2.65 dB, ×0.737), 128 → −1984, 0 → −3984 (−39.8 dB). Gain = 10^(vol/2000).
- 0x441e10(h, p): clamp −128..128; `SetPan((p*4000)/128)` (trunc.) → ±4000 at the ends
  (±40 dB on the opposite channel). DirectSound pan: negative attenuates the right channel
  by |pan|, positive the left.
- 0x441dc0(h, hz): stores +0x14 = hz always, but calls SetFrequency only if
  1000 < hz < 44100 (0xac44).
- 0x441fa0(h): `ms = (size / (bits/8 * channels)) * 1000 / (+0x14)` (unsigned integer
  arithmetic, using the current frequency).
- 0x441e80(1) pause-all: every copy that is playing *and was started looping* is stopped
  and marked +0x24; 0x441e80(0) replays the marked ones (from position 0, as 0x440ec0 resets).
  Used by the race pause (0x4121e0 / 0x412299) and app (de)activation (0x409700 from
  0x431250).

##### 4.10.3.5 0x4049f0 play(id, flags) — the game's play call [asm]
```
if 0x2f <= id <= 0x5c:                         # voices
    if !voicesOn(0x4db668) or id == -1: return -1
    if (flags & 4) and lastVoice(0x502898) == id: return -1
    lastVoice = id; h = Play(handle[id], flags); Volume(h, 0x4db654 = 0xff); return h
else:                                           # effects
    if !effectsOn(0x4db664): return -1
    if (flags & 4) and lastEffect(0x502710) == id: return -1
    if id < 0 or id >= 256: log "Invalid sound handle", return -1
    lastEffect = id; h = Play(handle[id], flags); Volume(h, 0x4db658 = 0xee); return h
```
The flags go straight to the server (bit 0 loop, bit 1 overlap; bit 2 only matters here).
0x4db654/0x4db658 are initialised data (0xff, 0xee) and never written [asm grep]. Stop:
`0x404af0(h)` = 0x424000. Other race code calls 0x423fd0 directly with the same pattern.

##### 4.10.3.6 Queued and delayed sounds [asm]
Record (0x64 bytes): +0 mode, +4 idle, +8 state, +0xc current index, +0x10 count (write
index for the ring), +0x14 handle playing, +0x18 start time, +0x1c play time, +0x20 length of
the playing copy (ms), +0x24 ids[8], +0x44 delays[8] (ms). Clock = 0x50bbc8 (ms, the menu's
`now`).
- **8 records at 0x504240..0x504560** (ctor 0x404b10, reset 0x404d50 at every menu entry:
  idle, state 0, index 0, count 0; mode stays 0). `0x404e10(n, delay0, id0, delay1, id1, ...)`
  (cdecl, varargs) takes the first idle record, clears it and appends up to 8 (id, delay)
  pairs.
- **Voice queue record 0x5028c8** (mode 1 = sequential; idle; reset by 0x404d50/0x4084e0).
  `0x404da0(id, delay)` appends one pair to its ring of 8 (dropped if full) and makes it
  busy.
- **0x404b30 (update, one record)**, called for the 8 records + the queue by **0x404eb0**
  from Menu_Frame each frame (after the page's frame), and for the queue only by the race
  (0x405f30):
  - idle → nothing.
  - state 0: start = now − length(handle[ids[i]]); state 1.
  - state 1: if now − start ≥ delays[i], or mode ≠ 0: play ids[i] (voice/effect rules of
    0x4049f0, flags 0, no flag-4 test, volumes 0xff/0xee), +0x14 = handle, +0x1c = now,
    +0x20 = its length (0 if not played); state 2.
  - state 2: mode 1 waits until now − playTime > delays[i] + length; mode 0 goes on at
    once. Then i = (i+1) & 7; if i == count → idle, else state 0.
  So in mode 0 (the menu prompts) the delay counts from (state-0 time − the sound's own
  length): a 600 ms prompt longer than 600 ms plays on the second update after it was queued
  (≈ 2 frames). In mode 1 (race announcements) each voice starts `delay` ms after the
  previous one ended.

#### 4.10.4 The menu's sounds, call by call

All with flags 0 unless noted. "voice" = needs VoicesState; "effect" = needs SoundState.

| where | when | id → file | how |
|---|---|---|---|
| 0x4099a0 hover (every menu frame) | the selected button changes to a new one (mouse or arrow keys; not when it becomes −1) | 0x29 select (effect) | play [asm] |
| 0x40a460 WM_LBUTTONDOWN | a click on a button (mouse mode, selected != −1) | 0x03 continue (effect) | play [asm] |
| Menu_Frame 0x409d65 | a page starts with the glass transition (0x4dbd00 set by 0x40c4a0) | 0x2a valide (effect) | play [asm] |
| page 14 title start 0x4465f0 | after its first frame | 0x2d title (effect) | play; music (3, 2) [asm] |
| page 3 main start 0x4422c0 | — | 0x57 makurchc "make your choice" (voice) | 0x404e10(1, 600, 0x57); music (3, 3) [asm] |
| page 3 frame 0x442790 | Solo pressed | 0x55 playsolo (voice) | play once (0x53f554), wait 1000 ms [asm] |
| page 3 frame | Link pressed | 0x56 playlink (voice) | same [asm] |
| page 5/26 start 0x447250 | — | 0x54 selectmd "select mode" (voice) | 0x404e10(1, 600, 0x54); music (3, 3) [asm] |
| page 6 start 0x447a30 | unless 0x4dc094 == 2 | 0x53 selerace "select race" (voice) | 0x404e10(1, 600, 0x53); music (3, 3) [asm] |
| page 6 frame 0x448110 | left / right track arrow | 0x28 deplett (effect) | play [asm] |
| page 7 start 0x449100, page 8 start 0x44a930 | — | — | music (3, 3) [asm] |
| page 9 start 0x44cfd0 | if 0x5401c4 != 0 (new record) | bit 0 set: 0x5a cngratim, else 0x59 cngrecor (voice) | play [asm] |
| page 10 frame 0x44ed30 | bike left / right | 0x28 deplett (effect) | play [asm] |
| page 10 frame | Go (once, 0x53f554) | 0x2c goninja if bike type 0, else 0x2b goktm (effect), then 0x58 letsgo (voice) | presses held until now > 0x53f78c = now + length(letsgo) (or of goXXX if voices off) [asm] |
| page 15 start 0x487070 (call at 0x487a32) | — | 0x36 entrname (voice) | 0x404e10(1, 600, 0x36) [asm] |
| page 15 frame 0x487bb0 | each accepted key | 0x03 continue (effect) | play [asm] |
| page 24 podium start 0x452240 | player not in the first 3 | 0x5b yutrygan (voice) | play; music (11, 0); else music (12, 0) [asm] |
| page 24 frame 0x452f80 | — | podium1/podium2 crowd loops | 4 "sound objects" (§6.2) [decomp] |
| page 25 logo start 0x4537d0 | — | — | music (13, 0) [asm] |

Silent: Return/Space/hotkey presses (only the mouse click sounds), Escape, page changes
without the glass transition, the quit page. Race-set ids used by the in-race menus
(0x4354e0 0x28, 0x4358b0 0x44 tryagain after stopping everything, 0x436150 0x29, 0x436420
and 0x437b70 0x03) and the race pause 0x4121e0 (0x424090(1) pause-all, CD pause, then
**0x2e pause.wav with flags 1 = looping**, handle in 0x4dbe40; 0x412299 stops it and resumes).

#### 4.10.5 Music: CD audio through MCI

##### 4.10.5.1 The calls
- **0x404990(track, mode)** [asm]: if the CD object `[0x503528]` exists: 0x4b18e0(track)
  (choose), obj+0x1c = mode, and if CD audio is on (0x4db66c): 0x4b19f0(−1, mode) (play).
- **0x4049e0()** = 0x4b1720 stop (only caller: the logo's end routine 0x453bf0).
- 0x4096e0(p) / 0x409670(p): pause (1) / resume (0) through 0x4b1660 (MCI_PAUSE; resume =
  MCI_PLAY from the current position to the saved end, no notify). 0x409670 is called on app
  (de)activation (0x4329a5); 0x4096e0 by the race pause and 0x457bf0.
- 0x409410: toggle CD audio (0x4db66c; on: replay obj+0x14, or the level's AudioTrack if
  none). 0x4db66c starts as AudioState (0x504870) if the CD device opened.

##### 4.10.5.2 The CD object (0x503528; ctor 0x4b11f0, open 0x4b1460/0x4b13b0) [decomp, asm where noted]
+0 MCI device id, +4 timer id, +8 open, +0xc repeat, +0x10 MCI "to" (TMSF end of track),
+0x14 current track (−1 none), +0x18 cached mode (0x20c not ready, 0x20d stopped, 0x20e
playing, 0x211 paused, 0x212 door open), +0x1c play mode, +0x20 CD labelled `moto_rac`
found, +0x24 hwnd. Open: MCI_OPEN "cdaudio" on each CD-ROM drive (GetDriveType 5... in the
loop the test is type 3? [decomp says 3; not checked]), volume label compared with
`moto_rac` (8 chars). A **1000 ms Windows timer** (SetTimer, callback 0x4b1330 → 0x4b12e0)
is the only looping mechanism: MM_MCINOTIFY reaches 0x4b1340, which does nothing [asm].

- **0x4b18e0(track)** choose [asm]: −1 → nothing. Not ready / door open → nothing. Query
  MCI_STATUS_NUMBER_OF_TRACKS = n. **track 0 → Rand_Next(1) % n + 1** (0x458cd0 =
  Rand_Next stream 1); else track must be 1..n. Then MCI_CDA_STATUS_TYPE_TRACK: audio
  (0x440) → +0x14 = track; else +0x14 = −1 and error 0x128 (so a random pick of the data
  track 1 gives silence: 1 chance in 13).
- **0x4b19f0(track, mode)** play [asm]: +0x1c = mode; read status. Skip if not ready / door
  open. **Skip if mode is 1 or 3, the drive is playing, and (track == −1 or track ==
  current)** — "keep the music going across pages". Else choose(track) (−1 keeps +0x14),
  get the track's length (MSF), MCI_PLAY with NOTIFY|FROM|TO from (track,0:0:0) to
  (track, length) (TMSF), +0x18 = playing, **+0xc = 1 if mode is 2 or 3** (repeat).
- Timer 0x4b12e0 [asm]: if repeat && cached mode == playing && track != −1: status; if not
  ready / door open → cached mode = stopped; **if stopped → 0x4b19f0(−1, 0)**: the same track
  again, but with mode 0, which clears +0xc. So on a real CD a "repeating" track is played
  **twice** and then stops; a later page start with mode 3 finds it stopped and starts it
  again.
- Mode meanings: 0 once; 1 once, don't restart if playing; 2 repeat (once), always restart;
  3 repeat (once), don't restart if playing.

##### 4.10.5.3 GOG's winmm.dll (proxy, 73728 bytes, 2010) [asm of the DLL]
Its mciSendCommandA (0x100011f0) handles: MCI_PLAY → builds `"0%d.wav"` / `"%d.wav"` from
dwFrom (TMSF from = the track number) and calls **PlaySoundA(name, 0, 0x2000b =
SND_FILENAME|SND_LOOP|SND_NODEFAULT|SND_ASYNC)**; MCI_STOP (and similar) → PlaySoundA(NULL,
0, 1). MCI_STATUS: number of tracks → 13; track type → audio (0x440) for 2..13, other
(0x441) for the rest; mode → playing/stopped/paused/not ready from its own flags; length
items go to the real winmm. So under GOG **every music loops forever** and the timer never
sees "stopped"; the music volume is the wave volume; MCI pause stops the sound (resume
restarts the file? not checked).

##### 4.10.5.4 Track use (both the CD and the GOG files: 02..13.wav, 44.1 kHz stereo 16-bit)

| track | length | used by | mode |
|---|---|---|---|
| 1 | data | — (random pick → silence) | |
| 2 | 252.8 s | race ROCK_FOREST (ile03); also the copy check: 0x442740 wants track 2's length = MSF 0x3e0c04 = 4:12:62 = 252.83 s [asm + test] | 0 |
| 3 | 143.4 s | **menu**: title (mode 2), main 3, 5/26, 6, 7, 8 (mode 3) | 2 / 3 |
| 4 | 187.4 s | race SPEED_BAY ile01, ile10 | 0 |
| 5 | 185.9 s | race SNOW_RIDE ile05 | 0 |
| 6 | 263.2 s | race RED_CITY ile08 | 0 |
| 7 | 187.9 s | race WEST_WAY ile02 | 0 |
| 8 | 194.6 s | race LOST_RUINS ile04 | 0 |
| 9 | 207.9 s | race DIRT_ARENA ile09, ile11 | 0 |
| 10 | 190.7 s | race GREAT_WALL ile06 | 0 |
| 11 | 57.7 s | podium, player not in the top 3 (with voice 0x5b) | 0 |
| 12 | 159.5 s | podium, player in the top 3; also at the race's end (0x412a50 → 0x48aca0) | 0 |
| 13 | 20.3 s | Delphine logo (page 25); stopped by its end routine | 0 |

Race music: 0x47bf70 at race start (stops all sounds 0x424020, race sound init 0x4084e0,
then **0x404990(AudioTrack 0x4db67c, 0)**). AudioTrack is read from the level's LEVEL.BPT
(field "AudioTrack", 0x428b4c; missing → 0 = random) [asm]; values from the banks: ile01 4,
ile02 7, ile03 2, ile04 8, ile05 5, ile06 10, ile08 6, ile09 9, ile10 4, ile11 9 [test].

##### 4.10.5.5 Volume settings
MasterVolume / CDVolume / WaveVolume (CONFIG.BPT) go to the **Windows mixer** (object
0x5d2958, 0x470cf0(line 0/1/2, value)): value 0..32 (0x5d2dbc[line] = 32 steps) scaled
onto the mixer control's range with MulDiv; restored at exit (0x470970). In-game hotkeys
(0x409720 from 0x431250) and the options page 11 step them (0x470d90 up / 0x470e90 down) and
draw a gauge. CD config: 19/19/19; GOG config: Master 0, CD 32, Wave 32. For the port: a
music gain = CDVolume/32 and an effects gain = WaveVolume/32 (ignore Master, or treat it
as overall) [guess on what feels the same; the original changed system volumes].

#### 4.10.6 Settings and race sounds (for later)

##### 4.10.6.1 Switches [asm]
CONFIG.BPT order (0x42ae30..): ... AmbiantState 0x50354c, MotoState 0x5028a0,
SoundState 0x504208, VoicesState 0x50355c, AudioState 0x504870, Wave16Bits 0x4db670,
Stereo 0x503530, ReplayRate 0x504568, MasterVolume, CDVolume, WaveVolume (mixer).
GOG CONFIG.BPT: 0 0 1 1 1 1 1 22050 0 32 32; CD: 1 1 1 1 1 1 1 22050 19 19 19.
Live switches: **effects 0x4db664** (= SoundState at start), **voices 0x4db668** (=
VoicesState), **engine 0x4db65c**, **ambience 0x4db660**, **CD audio 0x4db66c** (=
AudioState if the CD opened). 0x408720 (start) clears AmbiantState/MotoState and both live
flags; 0x4084e0 (race sound init) sets engine and ambience to 1 when effects are on and both
are off — so AmbiantState/MotoState are effectively ignored and follow SoundState.
Toggles: 0x408b30 effects (also creates the server if there was none; sets engine = ambience
= effects), 0x408f30 voices (loads/unloads voice sets), 0x408900 engine, 0x408800
ambience, 0x409410 CD audio, 0x409570 quality. Messages via 0x4188b0 with strings
0x53b7c8.. (language).

##### 4.10.6.2 Sound objects: ambience and the podium (0x405030, 0x404ee0) [decomp, partial asm]
A "soundDesc" (0x50 bytes; from `data\ileNN\Sound.Spt` = SOUND.BPT in the track bank,
parsed by 0x42e850: `soundDesc id count name... "FLAGS" 15 numbers`, plus `ambiance n ids`
and `signal`; static ones for the podium at 0x4e9128/0x4e9178/0x4e91c8/0x4e9218):
+0 id, +4 first sample id, +8 number of samples (random pick), +0xc flags (LOOPED 1, PAN 2,
RANDPAN 4, VOL 8 = distance attenuation, FADEIN 0x10, FADEOUT 0x20, FADEFREQ 0x40,
INFPLAY 0x80, SINVOL 0x100, SINFREQ 0x200), +0x10/+0x14 start delay base/random ms,
+0x18/+0x1c play time base/random (also SINVOL period and depth), +0x20 fade-in ms, +0x24
fade-out ms, +0x28/+0x2c repeat count base/random, +0x30/+0x34 frequency base/random
(base < 1 → added to the sample's own rate), +0x38 FADEFREQ / SINFREQ depth, +0x3c volume,
+0x40 fixed pan, +0x44/+0x48 floats near/far distance.
Object (0x40 bytes) state machine (+0x2c): 0 wait delay → 1 play (LOOPED → flags 3, else
2), set frequency, volume 0 if FADEIN (state 2) else full (state 5), pan fixed or RANDPAN
(rand&0x7f)−64 → 2 fade-in: `vol = ftol(sin(min(v, t·v/fadeIn)/v · π/2) · v · att) >> 8`
→ 5 hold (until play time unless INFPLAY; SINVOL/SINFREQ: sin(t/period · 2π)) → 4 fade-out
(mirror of 2) → 3 stop; repeat−−; at 0 re-randomise (0x404ee0); back to 0. All gated by
effects 0x4db664.
Podium (page 24): objects 0x540080/0x5400c0 = podium1 (id 4) looped forever with fade-in
1000 ms, pans +20 and −20, the second at the file's rate − 1000 Hz (a stereo chorus);
0x540100/0x540140 = podium2 (id 5) the same after 7000 ms [decomp; field meanings above].

##### 4.10.6.3 Engine (0x406290, per bike, from 0x4079e0 in the race) [asm for the formula, decomp
for the rest]
Gear sound id by gear (bike+0x7c): KTM table 0x4db638 {0, 0x22, 0x23, 0x24, 0x25, 0x26, 0x26},
Ninja 0x4db618 {0, 0x22, 0x22, 0x26, 0x26, 0x26, 0x26}; played looping (flags 3) on a gear
change, the old one stopped. Every frame
`hz = lo[g] + (hi[g] − lo[g]) · r` with (lo, hi) per gear from 0x4db5e0 (KTM: (9000,29000)
(1000,26000) (1000,26000) (1500,16000) (1000,16000) (1000,16000)) or 0x4db5a8 (Ninja:
(8000,48000) (10000,48000) (2000,40000) ×4), r = bike+0x11c / maxRpm(type) clamped
[0.05, 1]; then: sometimes + 2000 + rand%3000 (a misfire, 1 in 4 when flag bit 0), − a
0..3000 ramp (+0x4c, ±1000 a frame in state 3), + bike[+0x307d]·4000, − ftol((bike+0x1b8 −
bike+0x33c)/(X+0x38 · 48) · 1800), + obj+0x70, all × the pitch factor (arg 5, clamped
0.5..1.5) → SetFrequency (ignored above 44099 Hz). Volume = arg 2 · k >> 8, pan = arg 3.
Also skid/surface loops (ids 0x17 gravier, 0x18 glace, 0x19 herbe, 0x0a derap with set
frequencies 18000/20000/8000/5000), landings 0x0b..0x0d, brakes 0x1a/0x1b, start 0x1d..0x1f,
random "oouups"/"ouch" voices on crashes. Not finished: what the bike fields are.

##### 4.10.6.4 Start countdown (0x4082f0) [decomp]
state 0: bip2 (2) + "go" (0x33); 1/2/3: bip1 (1) + "1"/"2"/"3" (0x32/0x31/0x30); once per
state change.

#### 4.10.7 Open questions
- 0x4b1460: the drive-type test (decomp shows 3 = fixed disk; the cdaudio device is opened
  per letter anyway) — irrelevant for the port.
- What GOG's proxy does on MCI_PAUSE/resume (stop + restart from 0, or nothing).
- Page 6's 0x4dc094 == 2 (which game mode skips "select race").
- slctrans (0x52) has no caller; chsemstr / bestlap have no table entry.
- 1997 CD MOTO.EXE: same scheme expected, tables at other addresses (not looked up).
- Whether the original sounded duller (22050 Hz primary buffer): could be checked by
  recording the original under wine (tools/orig.sh) — not done.

#### 4.10.8 What the port needs, in order
1. **sfx.c**: read SFX.BKF entries through files.c, decipher (`b − 4(i+8)` per block, three
   blocks), keep mono s16 + rate. Read the 7 id tables from game.EXE (addresses in §2; the
   CD build needs its own). Load the menu set on menu entry, podium set on page 24, race set
   at race load (§2).
2. **mixer (SDL audio callback)**: N voices, each {sample, pos (fixed point), step =
   rate/outRate, loop, gain = 10^((vol−255)·15.625/2000) as truncated hundredths, pan as
   DirectSound}. Per sample: a head voice + copies (5 at load, grow on demand). Lock around
   the game-thread calls.
3. **play(id, flags)** exactly as 0x4049f0 (voice/effect classes, last ids, volumes 0xff /
   0xee, flags 1/2/4), stop, stop-all, pause-all/resume, length(ms) as 0x441fa0.
4. **Queues**: the 8 records + voice queue, `queue_n(...)` = 0x404e10, `queue_voice` =
   0x404da0, update all from the menu frame after the page (0x404eb0) with the menu clock.
5. **Menu hooks** (§4): hover 0x29, click 0x03, transition 0x2a, title 0x2d, page start
   prompts via the queue, Solo/Link voices + the 1000 ms wait, page 6/10 arrows 0x28, Go.
6. **music.c**: stream `NN.wav` (or the CD image's tracks for the CD version) with
   0x404990 semantics (modes 0..3, skip-if-playing). Decide looping: GOG loops forever
   (what Danik hears today); the real CD plays repeat tracks twice. Pause on focus loss as
   0x409670.
7. Settings: SoundState / VoicesState / AudioState / CDVolume / WaveVolume from CONFIG.BPT.
8. Race: engine (§6.3), ambience objects (§6.2), countdown, voice queue (later).


#### 4.10.9 In the port (2026-10-26)
`sound.c`: the effects decoded as above (16-bit set), slots of 1024 buffers with 4 copies
each, play/stop/volume/pan/length as §4.10.3, 0x4049f0's classes, the 8 queues and the
voice queue stepped once a menu frame after the page; the menu set loaded with the menu's
resources, the queues reset at the menu's first entry. Mixed at 44100 Hz stereo with linear
interpolation (the original's primary buffer ran at 22050). Music as GOG's winmm.dll plays
it: NN.wav looping, read ahead half a second, 0x404990's modes 1/3 keep a track going.
Hooked: logo music 13 and its stop, title music (3, 2) + 0x2d, main menu (3, 3) + the
queued 0x57 when `Data\champA.spt` opens, hover 0x29, click 0x03, glass 0x2a, Solo/Link
voices. Not yet: CONFIG.BPT's switches and volumes (all on, full), pause on losing focus,
the CD version's tables and audio tracks, the race's sounds.
Checks (`OMR_WAV=FILE` with `--shot`: what the run sounds like, 735 or 736 frames of
sound a picture): a 30 s run from the logo equals 13.wav sample for sample for 19 s, and
03.wav starts at frame 1201 (the title's start); on page 3 "make your choice" (1061 ms)
starts on the second frame, as the queue's quirk says; the down arrow plays 0x29.

### 4.11 Page 15, the name entry: the 3D letter grid (mapped 2026-10-26)


Start 0x487070, frame 0x487bb0, type-a-letter 0x487a70, end 0x488790.
Confidence marks: [asm] read from the instructions, [px] confirmed against the running
original, [decomp] read from Ghidra's C only, [guess] not proven.
Reference renderer: `tools/model.py name ...` (see the end of this file).

Result: **model.py reproduces the settled page with 0 differing pixels in 4 captures**
(16-bit comparison, 640x480), and to within 30 and 8 edge pixels in the 2 s ripple-start
after the intro (sub-millisecond timing). See "Verification".


#### 4.11.1 Start 0x487070 [asm]

##### View
- `0x49a0e0(1024.0)` and `0x49a100(1024.0)`: argument bits **0x44800000**. The focal
  in x is 1024 × [0x6d1f74] (screen scale, 1.0 at 640x480), in y 1024. Elsewhere in
  the menu it is 1911 (0x44eee000); the confirm path puts 1911 back.
- Near plane 256, centre (320,240) unchanged (the menu's), no x flip.
- `0x5f675c = 0` (the "confirm now" flag, see §4).

##### Models (only when [0x4f1284] == 0; afterwards it is set to 1)
Each through `0x4794b0(obj, "…3df", "…map", 0, 0, -1)` → `0x420a50(model, 3df, map, -1, 0)`
then `0x420250` (recentre on the bounding box centre).

| Object | File | Stored at |
|---|---|---|
| letters A..Z | `data\menu\font3d\l%c.3df` / `.map` (%c = 'a'..'z') | 0x5f1880[0..25] |
| delete | `data\menu\font3d\backspac.3df/.map` | 0x5f18e8 (= index 26) |
| confirm | `data\menu\font3d\valid.3df/.map` | 0x5f18ec (= index 27) |
| frame | `data\menu\font3d\cadre.3df/.map` | 0x4f1288 |
| cursor | `data\menu\font3d\curseur.3df/.map` | 0x4f128c |

**Not loaded**: CADRE1..3, CURSEUR2, no texture pages (0x4794b0 with page argument 0).

- `0x420a50` is the same reader as `0x4216a0` (NOTES §4.5) with a last argument. With
  last argument 0: textured faces get draw kind 1/2 (0x15/0x27 with 0x200000), faces
  flagged 0x20000 are moved to the end, and the model is mirrored by `0x422070`
  (x sign + corner order reversed). **Every font3d face is MAP kind 0, flags 0**, so it
  becomes draw kind 0 = flat colour, colour = the u16 at **MAP + 2** of the face record
  (not +0x4e as for textured faces). The shading is baked into those colours
  (e.g. LI: 0x829f, 0x935f, 0x48d8, 0xe55f). [asm, px]
- vertices are file floats × 96, as elsewhere. [asm]

##### Scales [asm]
- Cadre: `0x47a960(cadre, 78.0)` (0x429c0000) — fit to 78.
- `s = 0x47a960(letter A, 62.0)` (0x42780000) — letter A fitted to 62, s returned.
- Letters 1..27 (B..Z, BACKSPAC, VALID): `0x47abd0(model, s)`.
- `0x47ad40` (mirror again, same as 0x422070) on all 28 letter models → they end the
  right way round. The cadre and curseur are **not** mirrored back.
- Curseur: `0x47abd0(curseur, s)`.

`0x47a960 fit(model, size)`: per axis min/max over the vertices; half = f32((max-min)·0.5),
0 becomes 1; s = min(size/half) over the three axes; c = f32((max+min)·-0.5)
(0x4d18c8 = -0.5); every vertex v = f32((v + c)·s). Returns s. `0x47abd0(model, s)` does
the same with s given. [asm]

##### Objects and their start state [asm]
Menu object (0x94 bytes, §3). Camera `0x47af50(0x504bc8, angles (0,0,0), pos (0,0,-2400))`
(-2400 = 0xc5160000) — the identity view apart from 1e-16 terms.

- Wave phase `0x5f6480`: `0x47a340(obj, 0, 0)`, then `0x47a820(obj, (2π,0,0), 1.0 s, mode 2)`
  (2π = 0x40c90fdb) — the phase turns once a second, unbounded.
- Wave amplitude `0x5f63e8`: `0x47a340` to zero (x of its angles is the amplitude).
- Selection spinner `0x5f1690` (static, **not** reset here):
  `0x47a820((0,2π,0), 1.0, 2)` — its yaw (0x5f1694) is the selected letter's yaw.
- Cadre: `0x47a820(cadre, (0,2π,0), 1.0, 2)` — spins once a second.
- Each letter i (0..27): random start
  angles (A·4π, B·4π, C·4π), position (A·2048, B·512+2250, C·1024+4096),
  A,B,C = r/32767·2-1 in that call order, r = `0x458cd0()` (= `0x4b1160(1)`, menu random
  stream 1, 0..0x7ffe). Then rotate to (0,0,0) in 2.0 s mode 1, move to grid[i] in
  2.0 s mode 1.
- Grid table 0x5f18f0[28] (x,y,z floats), cell i: c = i % 7, r = i / 7:
  x = f32(f32(c·(1/6))·1100 - 550), y = f32(f32((3-r)·(1/3))·500 - 250), z = 0.
  Constants: 0x4d1b18 = 1/6 (double), 0x4d1b20 = 1100, 0x4d1b28 = 550, 0x4d1b00 = 1/3,
  0x4d1b08 = 500, 0x4d1b10 = 250. So 7 columns × 4 rows, A..G top row, VALID (27)
  bottom-right, BACKSPAC (26) left of it.
- Name slots 0x5f6530[8] (new objects, `0x479470`): model = letter k's, position
  (0,800,500), random angles r·2π/32767·3 (0x4d1b50 = 3.0).
- Slot positions 0x5f6378[9]: x = f32(k·140 - 490) (0x4d1b30 = 140, 0x4d1b38 = 490),
  y = -325 (0xc3a28000), z = -600 (0xc4160000). k = 0..8; [8] is where the curseur
  would sit after the 8th letter.
- Cursor (selected cell) `0x5f647c = 0`; menu+0xc = 0; `0x5f6528 = 1` (intro running);
  `0x4dbfc8 = 0` (Alt state).
- **Prefill** from the stored name 0x5161b0: each char `& 0xdf`, non-letters become 'A',
  at most 8. Slot k gets that letter's model, position (0,800,500), angles 0. If the
  length > 0 the cursor = 27 (VALID).
- Curseur: position (0,800,500), `0x47a820((0,2π,0), 1.0, 2)`.
- `0x43fa40(picture 0x69 enternam, (0,0))`; `0x5f6524 = 0`, `0x5f652c = 1` (roles
  not traced [guess]: page-local flags); mouse mode menu+4 = 0.
- No buttons (`0x40bd40` with an empty list). No music change.
- Sound: `0x404e10(1, 0x258, 0x36)` — voice sample 0x36 ("enter your name") queued with a
  600 ms delay. [asm; delay semantics guess]


#### 4.11.2 Frame 0x487bb0 [asm, px]

1. `0x49b1c0(1)`, `0x49a160(0,0)`, menu+0x10 = -1.
2. Background: picture id 2 = `paralax3.lez`, mode 4 (tile 128x128): `0x43fab0(0,1,0,0)`
   scrolls it **one row a frame**, pixel (x,y) = tile[(scroll+y) & 0x7f][x & 0x7f].
   Then picture id 0x69 = `enternam.lez` (per language, MENUUS etc.), mode 1 (colour-key)
   at (0,0). [px]
3. Update `0x5f1690` (selection spinner).
4. **Intro** (`0x5f6528` == 1): for i in 0..27: if i == cursor: place the cadre at the
   letter's *previous-frame* position, update + draw the cadre; update letter i; if
   i == cursor its yaw = [0x5f1694]; draw letter i. After the loop the cadre is
   updated and drawn again(**). When letter A's rotation is finished (+0x24 == 0):
   - amplitude object → (120.0, 0, 0) (0x42f00000) in 2.0 s mode 0 (linear);
   - every slot → its slot position (1 s mode 1) and → angles 0 (1 s mode 1);
   - if len < 8: curseur → slot[len] (1 s mode 1);
   - `0x5f6528 = 0`.
   (**) draws the cadre a second time at the same place — same pixels, harmless.
5. **Steady** (`0x5f6528` == 0): update 0x5f6480 (phase) and 0x5f63e8 (amplitude).
   phase p = [0x5f6480 +0] at the start of each row, for r in 0..3, c in 0..6:
   - z = f32(cos(p)·amp), amp = [0x5f63f4]; written to grid[i].z;
   - letter i updated, then its z forced to grid z;
   - i == cursor: update cadre, cadre position = letter's, draw cadre,
     letter yaw = [0x5f1694]; else letter angles = (0,0,0);
   - draw letter i;
   - p += 0.5235987755982988 per column (π/6), and per row the row's start phase
     += 0.4487989505128276 (π/7), each step rounded to float. [asm, px]
   The cosine is the x87 fcos on the float phase (not the COS table). [asm]
6. The 8 slots: update and draw (only those with a model). Then the curseur.
7. `0x40c500`, `0x49ad90` (flush the s-buffer; when [0x504e74]==0 and not D3D),
   pointer `0x442260(0x5049d4)` only if [0x4dbff0] && [0x53c524] (never drawn in the
   windowed captures), `0x49b0e0` (present).
8. No buttons, no text list.

So the draw order is: background, title, (per cell: cadre if selected, letter), typed
letters in slot order, curseur. Each draw is s-buffered and flushed at 0x49ad90, so the
order only matters for equal depth.

##### How the typed name is shown
As 3D letters in the slots 0x5f6378[k] along the bottom (y -325, z -600, x -490..+490
step 140), each object's model being the typed letter's. Typing: the slot object takes the
letter's model *and* its current angles and grid position, then flies to slot[len] and
rotates to 0 in 1 s (mode 1 = sine ease). Deleting: the last slot flies to (0,800,500)
(above the top edge) in 1 s mode 1 while given random angles. The curseur (the small
spinning marker) moves to slot[len] in 0.2 s linear.


#### 4.11.3 The menu 3D object and its routines [asm]

##### Layout (0x94 bytes)
| Off | What |
|---|---|
| +0x00 | angles a,b,c (float) |
| +0x0c | position x,y,z |
| +0x18 | model pointer |
| +0x1c, +0x8c, +0x90 | buffers used only by the glass-shard transition (0x479630/0x479c10) |
| +0x20 | move active |
| +0x24 | rotation active |
| +0x28 | "restart spinner" flag (mode 2: start time reset) |
| +0x2c | rotation start angles |
| +0x38, +0x3c | 0 → draw through 0x49b7f0; else 0x49c110 |
| +0x40 | angle delta |
| +0x4c | rotation end time; +0x54 duration (ms); +0x58 start time; +0x5c mode |
| +0x60 | move start position; +0x6c delta; +0x78 end; +0x80 duration; +0x84 start; +0x88 mode |

##### Setters
- `0x47a340(obj, angles, pos)`: both, clears +0x20 and +0x24.
- `0x47a380(obj, angles)`: angles, clears +0x24.
- `0x47a3a0(obj, pos)`: position, clears +0x20.
- `0x47a820(obj, angles*, float seconds, mode)` / `0x47a8b0(obj, pos*, seconds, mode)`:
  start = current, duration = ftol(seconds × 1000.0) (0x4d18a8 = 1000.0 double),
  start time = timeGetTime, end = start + duration. Mode 2: delta = the given vector
  (a rate per duration); otherwise delta = f32(target - current). Sets the active flag.

##### Update 0x47a3d0 (now = timeGetTime)
- ratio = f32((min(now,end) - start) / f32(duration)).
- mode 0: linear, value = start + delta·ratio.
- mode 1: e = (sin(ratio·π - π/2) + 1)·0.5; constants 0x4d1880 (π), 0x4d1888 (π/2),
  0x4d1890 (1.0), 0x4d1898 (0.5), all doubles; value = start + delta·e.
- mode 2: value = start + delta·(now - start)/duration, **not** clamped; each angle then
  wrapped by subtracting 2π_f (0x4d18a0) while > 2π (negatives through the sign bit test).
  Never ends. With +0x28 set the start time is reset.
- Otherwise the animation ends when min(now,end) == end (flag cleared, value = target).
- Bug: the mode-2 **position** branch uses now - duration instead of now - start (not
  used on this page).
Each result is stored as float. Times are integer ms.

##### Draw 0x479eb0
M = Rz(c)·Ry(b)·Rx(a), the same formula and product order as 0x48c030 (page 25/14).
With the camera C (0x47af50: V = Rᵀ, Vt = -(R·p)): N = M·C, T = pos·C + Ct, rounded to
float at every stored partial (T = f32(cc·sb), T2 = f32(sc·sb) in the matrix).
Then `0x49b2c0(model, N, T, 0x1f)` → `0x49b7f0(…, 1)` (when +0x38 == 0) → faces into the
s-buffer → `0x49ad90` at the end of the frame. **The same path as 0x48c030 → 0x49b2c0 →
0x49b7f0 → 0x49ad90**; only the object/matrix building differs. 0x49b2c0 sets the x87 to
24-bit precision and restores it; outside it the game runs at 53-bit.

##### Flat faces (draw kind 0)
Kind table 0x4f1a70: kind 0 = {0, sub 0, flag 0, 0x4a3950}; with no sub-kind 0x4aef20
calls `0x4a3950(vertices, face)` directly.
- `0x4a3950`: colour through 0x402020 = the 5:5:5 form, i.e. the 565 colour with bit 5
  cleared (`colour & 0xFFDF`); fan of triangles, near clip as in 0x49e510; only
  triangles entirely left of / above the screen are rejected.
- `0x4a34f0`: flat triangle, no area/back-face test (back faces are culled earlier),
  13-bit fixed-point edges.
- `0x4a2f10`: span insert into the s-buffer free list (splits a node on reversed spans);
  `0x4a31d0` is the MMX copy used when [0x4dbcf4].
No textures, no lighting on this page.


#### 4.11.4 Input

##### Mouse (menu+4 set when the mouse moved) [asm, px]
- The 28 letters' (x, y, 0) are copied to 0x5f1728 and projected through the camera
  0x504bc8 (focal 1024).
- The cursor = the cell nearest to the pointer by squared pixel distance, strict `<`,
  so the lowest index wins a tie. **Clicking anywhere acts on the nearest cell** — there
  is no rectangle. If the cursor changed, 0x5f16b8 = 1 (spinner restart).
- [0x5083b8] == 2 (a level: set on button down, cleared on up — a press shorter than a
  frame is lost): letter → type it (0x487a70); 26 → delete; 27 → confirm if len > 0.
  Then 0x5083b8 = 0 and sound 3.

##### Keys (menu+0xc) [asm, px]
- Cleared first (ignored): 0x21-0x24, 0x60-0x6b, 0x6f-0x7b. 0x6c-0x6e are not cleared and
  reach the letter test as 'l','m','n' (numpad +, separator, - type L, M, N).
- 0x08 Backspace: delete (no sound, no spinner restart): cursor = 26; the last slot flies
  to (0,800,500) in 1 s mode 1, angles random r·2π/32767·3; len--; the curseur moves to
  slot[len] in 0.2 s (0x3e4ccccd) mode 0.
- 0x0d Return: if not Alt ([0x4dbfc8] & 1) and len > 0: confirm, with sound.
- 0x20 Space: on a letter type it (sound only if len < 8); on 26 delete (always sound);
  on 27 confirm if len > 0 (sound).
- Arrows (set 0x5f16b8 = 1): Left -1 (below 0 → 27); Up -7 (negative → +28);
  Right +1 (≥28 → 0); Down +7 (≥28 → -28).
- 0x41-0x5a, 0x61-0x7a: 0x487a70((k|0x20) - 0x61), no sound.
- Escape: nothing (no way back except confirm).
- If [0x5f675c] is set the frame goes straight to confirm.

##### 0x487a70 type(i) [asm, px]
If len < 8: slot[len].model = letter i's model; 0x47a340(slot, letter i's angles, grid[i]);
slot → slot pos[len] (1 s mode 1) and → angles 0 (1 s mode 1); name[len] = 'A'+i.
The grid letter is put at (0,800,500) and moves back to grid[i] in 1.5 s (0x3fc00000)
mode 1. len++; cursor = i (no spinner restart); curseur → slot[len] in 0.2 s mode 0.
Always: 0x5f675c = 0.
(i is 0..25 here; 26/27 are handled by the callers.)


#### 4.11.5 Exits [asm]

Confirm:
1. strcpy name → 0x5161b0 (the player name).
2. `_strcmpi` (0x4bf8f0) against the cheat table 0x4f1250, count [0x4f1248] = 3
   (entries: string pointer, variable pointer, value):

   | Name | Variable | Value | Effect |
   |---|---|---|---|
   | CESREVER | 0x4dc050 | 1 | "reverse" tracks; read by the main menu with 0x4dc040 |
   | CTEKCOP | 0x4dc054 | 1 | "pocket" bikes; with 0x4dc03c |
   | CDNALSI | 0x4dc058 | 1 | read by 0x447920 / 0x448110 / 0x44aee0 (track pages) |

3. Focal back to 1911 (0x44eee000) in x and y; `0x40c1e0`.
4. Cheat matched: `0x409eb0(15)` — page 15 again, no transition; the end routine frees
   everything and the start reloads, so the intro plays again, prefilled from 0x5161b0
   (now the cheat word).
   No match: `0x40c4a0` (capture the screen for the glass transition), `0x409eb0(3)`
   → main menu.

End 0x488790: frees the slot objects (model pointer cleared first so the shared letter
models are not freed twice), the 28 letters, the cadre, the curseur; [0x4f1284] = 0.


#### 4.11.6 Verification [px]

Captures in `build/ref/name15/` with `tools/orig.sh` (headless). Comparison by
`model.py name` against each capture, 640x480 16-bit; side-by-side pictures (original |
model | difference) in `build/png/models/name_vs_orig_*.png`.

| Capture | State | Fitted parameters | Differing px |
|---|---|---|---|
| c_idle | DSI typed, cursor 27 | t 904, scroll 76, cadre 905, curseur 899 | **0** |
| orig_p15_name | DSI, cursor 6 | t 957, t_sel 614, scroll 100 | **0** |
| d2_bs_settled | DS after Backspace, cursor 26 | t 496, cadre 497, curseur 491 | **0** |
| d5_q_settled | DSQ after Q, cursor 16 | t 652, t_sel 890, cadre 653, curseur 647, scroll 66 | **0** |
| b_intro_6 | ripple starting | t 990, t_end 986, scroll 65 | 30 (one column x=472) |
| b_intro_7 | ripple growing | t 1442, t_end 1438, scroll 123 | 8 |

The fitted times are consistent: the cadre spinner always started 1 ms before the wave
spinner and the curseur 5 ms after (they are started at those points of the start
routine); the end of the intro was seen 4 ms after the 2000 ms mark. The residues are
single edge pixels from sub-millisecond timing. The intro fly-in itself is random and was
not fitted (needs the random stream's state, see open questions).
Mouse: nearest-cell selection and click-anywhere confirmed (e1/e2/e3/e4). Keys, Backspace,
arrows, typing, the Q fly-in confirmed (d0..d5).


#### 4.11.7 Open questions
- Where 0x5161b0's first "DSI" comes from: bestscor.dat holds "DSI"; the load was not traced.
- Exact semantics of the delay argument of 0x404e10 (voice 0x36, 600).
- The state of random stream 1 at page entry (needed for an exactly equal intro; the port
  can use any seed — the fly-in is random by design).
- Why the pointer is not drawn in the windowed captures (0x4dbff0 / 0x53c524 = fullscreen?).


#### 4.11.8 What the port needs, in order
1. Focal 1024 on entry, 1911 back on exit (menu view setters already needed elsewhere).
2. Model loading with the "last argument 0" path: MAP kind 0 → flat face with colour at
   MAP+2; mirror; recentre; then `fit` / `scale` / mirror-back as in §1.
3. Flat face drawing (kind 0): 0x4a3950 → 0x4a34f0 → 0x4a2f10 into the existing s-buffer.
4. The menu object (0x94) with set/rotate-to/move-to/update (modes 0,1,2 with the float
   roundings of §3) and its draw (M·C, pos·C + Ct) into 0x49b2c0's path; camera 0x47af50.
5. Page 15 start, frame and end as §1/§2/§5, with the grid, wave and slot tables.
6. Input §4 (nearest-cell mouse, keys, type/delete) and the confirm with the cheat table
   (three flags) → page 3 through the glass transition, or page 15 again on a cheat.
7. Voice 0x36 with its delay; sound 3 on clicks/space.
8. The stored player name 0x5161b0 (load from bestscor.dat — to be traced).

#### 4.11.x model.py
`python3 tools/model.py name BKF MS out.png [sel=N] [tsel=MS] [name=ABC] [scroll=N]
[tcadre=MS] [tcur=MS] [tend=MS]` — MS = ms since the wave spinner started; tsel = since
the selection spinner (re)started; tend = since the intro ended (ripple amplitude and
name fly-in). Functions: `load_menu_model`, `fit`, `scale`, `mirror`,
`menu_object_matrix`, `menu_camera`, `menu_draw`, `spin`, `ease`, `grid_xy`, `slot_xyz`,
`wave`, `name_models`, `name_background`, `name`; the Renderer gained `focal=` and flat
faces (`face_flat`, `flat_triangle`, `flat_row`).


#### 4.11.9 In the port (2026-10-26)
`page_name.c` (the page), `obj3d.c` (the menu object 0x47a3d0 and its setters), in `r3d.c`
a settable focal length, flat faces (0x4a3950/0x4a34f0/0x4a2f10) and the menu object's
matrix, camera and projection, in `model.c` the loader with the last argument 0, the mirror
0x422070 with its cosine-table constants, fit and rescale. The first name is CONFIG.BPT's
DefName (0x42af0d reads it into 0x5161b0; `config.c`). The cheat table is read from
game.EXE; its three variables map to the port's flags by address.
Checks: against `tools/model.py name` (0 px from the original's captures), the port's
settled page with DSI (frame 301: t 5000 ms, scroll 45) and after Backspace + Q (DSQ, frame
501) differ only in the mouse pointer (179 px; the original's windowed captures have none).
The logo and title pages are unchanged by the renderer's additions (0 px). The chain
25 → 14 → 15 → glass → 3 runs; CTEKCOP restarts the page and the main menu then shows the
Pocket switch; a confirmed cheat word restarts the page again each time (as the code does);
the mouse picks and types the nearest cell. The fly-in is random (stream 1) and not compared.

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
