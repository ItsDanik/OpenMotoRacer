#!/usr/bin/env python3
"""Moto Racer's bitmap fonts: the .FNT files of FONTES.BKF and the script FONTS.BPT.

    python3 -I tools/fnt.py list                           the 35 fonts the script declares
    python3 -I tools/fnt.py text FONT "TEXT" out.png [options]   a string drawn as the game does
    python3 -I tools/fnt.py sheets DIR                     every font's characters, one PNG each
    python3 -I tools/fnt.py rawcheck                       the .RAW frame scan against the .FNT
  options: -data DIR (the Moto Racer folder, default "Moto Racer"), --fore R,G,B --back R,G,B
  (override the script's colours, as the text objects do), --level PCT (0..100, the effect
  level: transparency for AntiAlias8Transp), --bg R,G,B or --bgpic MENU\\BITMAP\\X.LEZ,
  --at X,Y, --size W,H, --scale N. FONT is a number or a name ("MenuLink").

FONTS.BPT (read by 0x426460, opened by 0x426350 as Data\\Fonts.Spt) holds, in this order:
  FontNumber n (read twice: the BPT replays the reads, and the game reads it twice);
  FontTypes: index, name, type, condition ... until index -1 (the type picks the class:
    Direct, DirectTrame, CustomColor, CustomColorTrame, AntiAlias8, AntiAlias8Transp, Pal,
    Moto; the names GameMsgLowRes/GameMsgHiRes are forced to Direct);
  FontSpacings: index, letter spacing (+0xa20), line spacing (+0xa1c) ... -1;
  FontForeColors / FontBackColors: index, R, G, B ... -1 (+0x5494 / +0x5497);
  FontData: index, first char, last char, frame colour, file ... -1 (up to 4 files a font).
Defaults (0x490840): spacings 1 and 1, fore (255,50,20), back (100,100,10).

The script names a .Raw; the loader (0x491380, slot 3 of the font classes) first tries the
same name with .FNT (0x490c90 swaps the extension, 0x490d80 reads it). FNT: u32 first
char, u32 count (both replaced by the script's), u32 height, u32 palette entries (0 or
256), the palette (3 bytes R,G,B an entry), count u32 widths, then the glyphs one after
the other, each width*height bytes, rows top first. Glyph pointer = file block + height *
(sum of the widths before it) (0x492170). Pixel 0 is never drawn. The .RAW fallback is
an mhwanh picture where each glyph sits in a box drawn in the frame colour (0x491210).

Drawing (0x493820 -> 0x493490 -> slot 6 of the class, one call a character, into the
16-bit RGB565 screen): Direct/AntiAlias: colour table[v] = ramp[v & 31], the ramp going in
32 steps from the back colour (0) toward the fore colour (0x490b90); Pal: table[v] = the
font's palette[v % entries] (0x495bd0); CustomColor: (fore*v + back*(31-v))/31, v = b&31
(0x494170). AntiAlias8Transp (0x495280): t = level*224/100 + (b & 0xe0); drawn only if
t <= 224, mixed with the screen by alpha = t*256/224 (0 = all glyph colour).
"""
import argparse, struct, sys
from pathlib import Path
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bkf, bpt, lez  # noqa: E402

TYPES = {"Direct": 1, "DirectTrame": 2, "CustomColor": 3, "CustomColorTrame": 4,
         "AntiAlias8": 5, "AntiAlias8Transp": 6, "Pal": 7, "Moto": 8}


class Reader:
    """The token stream of FONTS.BPT, read in the order the game reads it."""

    def __init__(self, data):
        self.d, self.p, self.k = data, 0, bpt.Key()

    def int(self):
        raw = bytes(c ^ self.k.byte() for c in self.d[self.p:self.p + 4])
        self.p += 4
        return struct.unpack("<i", raw)[0]

    def str(self):
        n, = struct.unpack_from("<I", self.d, self.p)
        s = bytes(c ^ self.k.byte() for c in self.d[self.p + 4:self.p + 4 + n])
        self.p += 4 + n
        return s.split(b"\0")[0].decode("latin-1")


class Font:
    def __init__(self, idx, name, kind, cond):
        self.idx, self.name, self.kind, self.cond = idx, name, kind, cond
        if name in ("GameMsgLowRes", "GameMsgHiRes"):
            self.kind = "Direct"                            # 0x426911
        self.type = TYPES[self.kind]
        self.letter, self.line = 1, 1
        self.fore, self.back = (255, 50, 20), (100, 100, 10)
        self.files = []                                     # (first, count, frame, name)
        self.height = 0
        self.width = [0] * 256
        self.glyph = [None] * 256
        self.pal = None


def read_script(data):
    r = Reader(data)
    n = r.int()
    assert r.int() == n
    fonts = [None] * n
    while (i := r.int()) != -1:
        fonts[i] = Font(i, r.str(), r.str(), r.str())
    while (i := r.int()) != -1:
        if fonts[i]:
            fonts[i].letter, fonts[i].line = r.int(), r.int()
    while (i := r.int()) != -1:
        fonts[i].fore = (r.int(), r.int(), r.int())
    while (i := r.int()) != -1:
        fonts[i].back = (r.int(), r.int(), r.int())
    while (i := r.int()) != -1:
        first, last, frame = r.int(), r.int(), r.int()
        fonts[i].files.append((first, last - first + 1, frame, r.str()))
    assert r.p == len(data), (r.p, len(data))
    return fonts


def read_fnt(f, d, first, count):
    """Adds one .FNT file to the font (0x490d80)."""
    _, _, h, npal = struct.unpack_from("<4I", d, 0)
    p = 16
    pal = [tuple(d[p + 3 * i:p + 3 * i + 3]) for i in range(npal)]
    p += 3 * npal
    if npal and f.pal is None and f.type in (7, 8):         # only Pal/Moto keep one
        f.pal = pal
    widths = struct.unpack_from("<%dI" % count, d, p)
    p += 4 * count
    f.height = h                                            # the last file's wins
    for c, w in zip(range(first, first + count), widths):
        f.width[c] = w
        f.glyph[c] = d[p:p + w * h]
        p += w * h
    assert p == len(d)


class Data:
    def __init__(self, folder):
        folder = Path(folder)
        files = {p.name.upper(): p for p in (folder / "data").iterdir()} \
            if (folder / "data").is_dir() else {p.name.upper(): p for p in folder.iterdir()}
        self.fonts = read_script(files["FONTS.BPT"].read_bytes())
        self.bank = {n.upper(): b for n, _, _, b in bkf.read_bkf(files["FONTES.BKF"])}
        self.menu = files.get("MENU.BKF")
        for f in self.fonts:
            for first, count, frame, name in f.files:
                key = name.upper().replace("DATA\\FONTES\\", "").rsplit(".", 1)[0] + ".FNT"
                read_fnt(f, self.bank[key], first, count)

    def font(self, s):
        if s.lstrip("-").isdigit():
            return self.fonts[int(s)]
        return next(f for f in self.fonts if f.name.lower() == s.lower())

    def picture(self, name):
        for n, _, _, b in bkf.read_bkf(self.menu):
            if n.upper() == name.upper():
                return b
        raise KeyError(name)


# ---- 16-bit colours, as the game builds them (shift globals 0x4ee9b8.. for RGB565) ----

def rgb565(r, g, b):                                        # 0x473a90
    return ((r & 0xFF) >> 3) << 11 | ((g & 0xFF) >> 2) << 5 | ((b & 0xFF) >> 3)


def to888(c):
    r, g, b = c >> 11, (c >> 5) & 63, c & 31
    return (r << 3 | r >> 2, g << 2 | g >> 4, b << 3 | b >> 2)


def ramp(fore, back):
    """0x490b90: 32 colours from back (0) toward fore, 8.16 fixed point, 16-bit steps."""
    acc = [b << 16 for b in back]
    step = [((f - b) & 0xFFFF) * 0x800 for f, b in zip(fore, back)]
    out = []
    for _ in range(32):
        out.append(rgb565(*((a >> 16) & 0xFF for a in acc)))
        acc = [(a + s) & 0xFFFFFFFF for a, s in zip(acc, step)]
    return out


def table(f):
    """The 256 colours at font+0x418 (slot 1, called with 1, 0xff)."""
    t = [0] * 256
    if f.type in (7, 8):
        if f.pal:
            for v in range(1, 256):
                t[v] = rgb565(*f.pal[v % len(f.pal)])
    elif f.type in (1, 2, 5, 6):
        rp = ramp(f.fore, f.back)
        for v in range(1, 256):
            t[v] = rp[v & 31]
    return t


MASKS = (0xF800, 0x07E0, 0x001F)


def mix(src, dst, a):
    """0x495280: per channel mask m, (S&m) - (((S&m) - (D&m)) * a >> 8), masked."""
    out = 0
    for m in MASKS:
        s, d = src & m, dst & m
        out |= (s - (((s - d) * a) >> 8)) & m
    return out


class Screen:
    def __init__(self, w, h, fill=0):
        self.w, self.h, self.px = w, h, [fill] * (w * h)

    def png(self, path, scale=1):
        im = Image.new("RGB", (self.w, self.h))
        im.putdata([to888(c) for c in self.px])
        if scale > 1:
            im = im.resize((self.w * scale, self.h * scale), Image.NEAREST)
        Path(path).parent.mkdir(parents=True, exist_ok=True)
        im.save(path)


def measure(f, text, one_line=False):
    """0x492b10: width = max over lines of sum(width + letter spacing) for chars of width >
    0; height = font height + (line spacing + height) a further line."""
    w = best = 0
    h = f.height
    for ch in text.encode("latin-1"):
        if ch == 10:
            if one_line:
                break
            best, w = max(best, w), 0
            h += f.line + f.height
        elif f.width[ch] > 0:
            w += f.letter + f.width[ch]
    return max(best, w), h


def draw_char(scr, f, tab, ch, x, y, level):
    w, h, g = f.width[ch], f.height, f.glyph[ch]
    if not ch or not g or not w:
        return
    for row in range(h):
        sy = y + row
        if not 0 <= sy < scr.h:
            continue
        base = sy * scr.w
        for col in range(w):
            sx = x + col
            b = g[row * w + col]
            if not b or not 0 <= sx < scr.w:
                continue
            if f.type == 6:                                 # AntiAlias8Transp
                t = level + (b & 0xE0 if b >= 0x20 else 0)
                if t <= 0xE0:
                    scr.px[base + sx] = mix(tab[b], scr.px[base + sx], (t << 8) // 0xE0)
            elif f.type == 5:                               # AntiAlias8 (unused; approx.)
                k = level + (b >> 5)
                if k == 0:
                    scr.px[base + sx] = tab[b]
                elif k < 8:
                    scr.px[base + sx] = mix(tab[b], scr.px[base + sx], k * 32)
            elif f.type in (3, 4):                          # CustomColor
                v = b & 31
                scr.px[base + sx] = rgb565(*((fc * v + bc * (31 - v)) // 31
                                             for fc, bc in zip(f.fore, f.back)))
            else:                                           # Direct, Pal, Moto
                scr.px[base + sx] = tab[b]


def draw_text(scr, f, text, x, y, flags=0x21, pct=0):
    """0x493820 + 0x492c20 + 0x493490: flags & 0x1f: 2/4 centred on the screen, 8/0x10
    from the right edge (4/0x10 also align each line in the block); flags & 0xe0: 0x40
    centred vertically, 0x80 from the bottom. pct is the effect level 0..100."""
    if f.type == 5:
        level = (pct << 11) // 100 >> 8
    elif f.type == 6:
        level = (pct * 0xE000) // 100 >> 8
    else:
        level = pct
    bw, bh = measure(f, text)
    if flags & 0x1F in (2, 4):
        x = (scr.w - bw) // 2
    elif flags & 0x1F in (8, 0x10):
        x = scr.w - x - bw
    if flags & 0xE0 == 0x40:
        y = (scr.h - bh) // 2
    elif flags & 0xE0 == 0x80:
        y = scr.h - y - bh
    tab = table(f)
    lines = text.encode("latin-1").split(b"\n")
    for line in lines:
        lw, _ = measure(f, line.decode("latin-1"))
        cx = x + ((bw - lw) // 2 if flags & 0x1F == 4 else bw - lw if flags & 0x1F == 0x10 else 0)
        for ch in line:
            if f.width[ch] and f.glyph[ch]:
                draw_char(scr, f, tab, ch, cx, y, level)
                cx += f.width[ch] + f.letter
        y += f.height + f.line


# ---- the .RAW fallback: glyphs boxed in the frame colour (0x491380, 0x491210) ----

def raw_glyphs(d, first, count, frame):
    assert d[:6] == b"mhwanh"
    W, H, npal = struct.unpack_from(">HHH", d, 8)
    px = d[32 + 3 * npal:]
    x = y = 0
    height = None
    out = []

    def find(first_call):
        nonlocal x, y
        newrow = True
        while True:
            row = px[y * W:(y + 1) * W]
            i = row.find(bytes([frame]), x)
            if i >= 0:
                break
            x = 0
            if not first_call and newrow:
                newrow = False
                y += height + 2
            else:
                y += 1
            if y >= H:
                return None
        x0 = i
        while i < W and row[i] == frame:
            i += 1
        w = i - x0 - 2
        j = y
        while j < H and px[j * W + x0] == frame:
            j += 1
        x = i
        return x0 + 1, y + 1, w, j - y - 2

    for n in range(count):
        r = find(n == 0)
        if r is None:
            break
        gx, gy, w, h = r
        if height is None:
            height = h
        assert h == height, (n, h, height)
        out.append(bytes(px[(gy + j) * W + gx:(gy + j) * W + gx + w][k]
                         for j in range(h) for k in range(w)))
    return height, out


def parse_rgb(s):
    return tuple(int(v) for v in s.split(","))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-data", default="Moto Racer")
    ap.add_argument("cmd")
    ap.add_argument("args", nargs="*")
    ap.add_argument("--fore"); ap.add_argument("--back"); ap.add_argument("--bg", default="60,60,60")
    ap.add_argument("--bgpic"); ap.add_argument("--level", type=int, default=0)
    ap.add_argument("--at", default="8,8"); ap.add_argument("--size", default="640,120")
    ap.add_argument("--flags", type=lambda s: int(s, 0), default=0x21)
    ap.add_argument("--scale", type=int, default=1)
    a = ap.parse_args()
    data = Data(a.data)
    if a.cmd == "list":
        for f in data.fonts:
            print(f"{f.idx:2} {f.name:15} {f.kind:16} {f.cond:16} h={f.height:2} sp={f.letter},{f.line}"
                  f" fore={f.fore} back={f.back} " +
                  " ".join(f"{n.split(chr(92))[-1]}[{c0}..{c0 + c - 1}]" for c0, c, _, n in f.files))
    elif a.cmd == "text":
        f = data.font(a.args[0])
        if a.fore:
            f.fore = parse_rgb(a.fore)
        if a.back:
            f.back = parse_rgb(a.back)
        w, h = parse_rgb(a.size)
        scr = Screen(w, h, rgb565(*parse_rgb(a.bg)))
        if a.bgpic:
            pw, ph, _, _, pal, px = _lez(data.picture(a.bgpic))
            scr = Screen(pw, ph)
            scr.px = [rgb565(*pal[3 * v:3 * v + 3]) for v in px]
        x, y = parse_rgb(a.at)
        draw_text(scr, f, a.args[1].replace("\\n", "\n"), x, y, a.flags, a.level)
        scr.png(a.args[2], a.scale)
    elif a.cmd == "sheets":
        out = Path(a.args[0])
        for f in data.fonts:
            chars = [c for c in range(256) if f.width[c]]
            if not chars:
                continue
            rows = [bytes(chars[i:i + 16]).decode("latin-1") for i in range(0, len(chars), 16)]
            text = "\n".join(rows)
            bw, bh = measure(f, text)
            scr = Screen(bw + 16, bh + 16, rgb565(60, 60, 60))
            draw_text(scr, f, text, 8, 8)
            scr.png(out / f"{f.idx:02}_{f.name}.png", 2 if f.height < 20 else 1)
            print(f.idx, f.name, len(chars), "chars")
    elif a.cmd == "rawcheck":
        for f in data.fonts:
            for first, count, frame, name in f.files:
                key = name.upper().replace("DATA\\FONTES\\", "")
                if key not in data.bank:
                    continue
                h, gl = raw_glyphs(data.bank[key], first, count, frame)
                same = h == f.height and all(g == f.glyph[first + i] for i, g in enumerate(gl))
                print(key, "height", h, "glyphs", len(gl), "same as .FNT" if same else "DIFFERENT")


def _lez(d):
    t = lez.unpack(d)
    npal, = struct.unpack_from("<h", t, 5)
    w, h = struct.unpack_from("<hh", t, 12)
    pal = t[0x12:0x12 + 3 * npal]
    rgb = bytes(c for i in range(npal) for c in pal[3 * i:3 * i + 3][::-1])
    return w, h, None, None, rgb, t[0x12 + 3 * npal:0x12 + 3 * npal + w * h]


if __name__ == "__main__":
    main()
