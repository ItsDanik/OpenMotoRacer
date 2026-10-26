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
  Spin speeds (obj+0x90, 3 floats) from three `Rand_Next(1)` draws r1, r2, r3 (0x458cd0):
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
- Not checked: whether 0x49b2c0 multiplies by rows or columns (so the matrix might be
  the transpose); visually it is a random tumble either way.

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
