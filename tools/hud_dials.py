#!/usr/bin/env python3
"""The race's dashboard (rev counter + speedometer, bottom right) as game.EXE draws it:
0x484810 (layout, needles, digits), 0x485be0 (dial scale), 0x486310 slot 0 (the dial face,
decor model "counter" of the track's library), through tools/track_raster's renderer and
tools/fnt's text. Notes: build/notes/race_view.md §3b.

    python3 -I tools/hud_dials.py draw ILE SPEED RPM GEAR out.png [--bg CAPTURE.png] [--hires]
    python3 -I tools/hud_dials.py fit CAPTURE.png SPEED GEAR [ILE]   best rpm (and track) for a capture

ILE is the track's bank number (1..11). RPM is the dial's value 0..16000 (0x4f0e10). The
layout is the GOG default (SmallFonts 1: the low-res layout at every screen size) unless
--hires (SmallFonts 0 at 640x480). CounterType 1 (dials) only.

Checked against build/trackcap/q17.png (ile02, 125 km/h, gear 3, dial 13250): the whole
120x80 box (face, both needles, digits, gear) is pixel-exact; q14 (ile02, 0, gear 1):
45 pixels (the countdown's blank digits).
"""
import math, sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import model                                          # noqa: E402
import track                                          # noqa: E402
import fnt                                            # noqa: E402
from model import W, H, ftol                          # noqa: E402

DATA = Path("Moto Racer")
TWO_PI = 6.283185307179586                            # double 0x4d1a48

# 0x484810 [asm 0x4849df..0x484b48]: (hi-res, low-res) = (SmallFonts 0 at 640x480, others)
LAYOUT = {
    True: dict(digits=(0xcf, 0xd2), gear=(0x43, 0x6a), shadow=-3, origin=(0x3d, 0x74),
               speed=((0xcd, 0xbf), (0xbb, 0xfa), (0xf2, 0x8e)),
               rev=((0x77, 0xd3), (0x6c, 0xf7), (0x77, 0xaa)), base=(0xc3, 0x8c)),
    False: dict(digits=(0xe2, 0x4f), gear=(0x1f, 0x33), shadow=-2, origin=(0x9a, 0x1e),
                speed=((0xe3, 0x44), (0xdb, 0x63), (0xf9, 0x2a)),
                rev=((0xb7, 0x4f), (0xb1, 0x63), (0xb7, 0x39)), base=(0x66, 0x4a)),
}
SPEED_MAX, REV_MAX = 0x12c, 0x3e80                    # 300, 16000


def dial(c, p1, p2, vmin, vmax, ccw=0):
    """0x485be0: (start angle, radians per unit, radius), all doubles."""
    cx, cy = c
    radius = math.sqrt((p1[0] - cx) ** 2 + (p1[1] - cy) ** 2)
    def wrap(a):
        if a < 0:
            a += TWO_PI
        if not a < TWO_PI:
            a -= TWO_PI
        return a
    a1 = wrap(math.atan2(cy - p1[1], p1[0] - cx))     # screen y down: angles counter-clockwise
    a2 = wrap(math.atan2(cy - p2[1], p2[0] - cx))
    if ccw == 0:
        if a2 > a1:
            a2 -= TWO_PI
    elif a2 < a1:
        a1 -= TWO_PI
    return a1, (a2 - a1) / (vmax - vmin), radius


def rot(c, a, along, across):
    """A needle corner: centre + (along, across) turned by a, screen y down, each part
    truncated (0x4b3048)."""
    co, si = math.cos(a), math.sin(a)
    return (c[0] + ftol(co * along - si * across), c[1] - ftol(si * along + co * across))


def layout(hires, w=W, bottom=H):
    L = LAYOUT[hires]
    ox, oy = L["origin"]
    base = (w - L["base"][0], bottom - L["base"][1])  # 0x5dbc80 / 0x5dbc84
    rel = lambda p: (p[0] - ox, p[1] - oy)            # CounterType 1: points from the origin
    sp = [rel(p) for p in L["speed"]]
    rv = [rel(p) for p in L["rev"]]
    return dict(base=base, speed=sp, rev=rv, digits=rel(L["digits"]), gear=L["gear"],
                shadow=L["shadow"], hires=hires,
                sdial=dial(sp[0], sp[1], sp[2], 0, SPEED_MAX),
                rdial=dial(rv[0], rv[1], rv[2], 0, REV_MAX))


def needles(lay, speed, rpm):
    """0x484810 param 2: the faces (corners, RGB565 colour, sort key, vertex Z) in bucket
    order. The shadows' vertex records get Z = 1.0 (the loop at 0x48553d), behind the race's
    near plane 64: the software renderer never draws them."""
    hi = 1 if lay["hires"] else 0
    bx, by = lay["base"]
    out = []
    a1, k, radius = lay["sdial"]
    c = lay["speed"][0]
    v = min(max(speed, 0), SPEED_MAX)
    a = v * k + a1
    R = ftol(radius)
    p = [rot(c, a, 5 * (-1 - hi), 0), rot(c, a, R, 0),
         rot(c, a, 0, 3 * (-1 - hi)), rot(c, a, 0, 3 * hi + 3)]
    p = [(x + bx, y + by) for x, y in p]
    s = [(x - lay["shadow"], y - lay["shadow"]) for x, y in p]
    out += [([p[0], p[1], p[2]], model_rgb(0x19, 0x10, 0), 4.0, 2048.0),
            ([p[0], p[3], p[1]], model_rgb(0x1f, 0x10, 0), 4.0, 2048.0),
            ([s[0], s[3], s[1]], 0, 8.0, 1.0),
            ([s[0], s[1], s[2]], 0, 8.0, 1.0)]
    a1, k, radius = lay["rdial"]
    c = lay["rev"][0]
    a = min(max(rpm, 0), REV_MAX) * k + a1
    R = ftol(radius)
    p = [rot(c, a, 2 * (-1 - hi), 2 * hi + 2), rot(c, a, R, 0),
         rot(c, a, 2 * (-1 - hi), 2 * (-1 - hi))]
    p = [(x + bx, y + by) for x, y in p]
    s = [(x - lay["shadow"], y - lay["shadow"]) for x, y in p]
    out += [([p[0], p[1], p[2]], model_rgb(0x1f, 0x10, 0), 4.0, 2048.0),
            ([s[0], s[1], s[2]], 0, 8.0, 1.0)]
    return out


def model_rgb(r, g, b):
    """0x45a2e0: r, g, b clamped to 0..31, 0..63, 0..31, then r << 11 | g << 6 | b (green
    shifted by 6, not 5: these 5-bit greens land on RGB565's upper five green bits)."""
    return (min(r, 31) << 11 | min(g, 63) << 6 | min(b, 31)) & 0xFFFF


def counter_face(trk, hires, w=W, bottom=H):
    """0x486310 slot 0 (0x473370: the library model named "counter", at (0x5d, 0) from the
    bottom-right corner, scale 1.9 x 2.0 in the hi-res layout, else 1)."""
    t = track.read_track(trk)
    lib = next(L for L in t["lib"] if L["name"] == "counter")
    verts, faces = lib["models"][0]
    sx, sy = (1.9, 2.0) if hires else (1.0, 1.0)      # 0x3ff33333, 0x40000000
    ox = (w - 1) - 0x5d * sx
    oy = (bottom - 1) - 0 * sy
    pts = [(ftol(x * sx + ox), ftol(oy - y * sy)) for x, y, _ in verts]
    return pts, faces


def draw(ile, speed, rpm, gear, out=None, bg=None, hires=False, pages=None):
    trk = next((DATA / "data" / ("ile%02d" % ile)).glob("*.trk"))
    if pages is None:
        pages, _ = track.bank_pages(DATA / "data" / ("ILE%02d.BKF" % ile))
    from track_raster import RaceRenderer
    model.NEAR = 64.0
    r = RaceRenderer(bg)
    lay = layout(hires)
    Z = 2048.0                                        # vertex Z 0x45000000, 1/Z 0x3a000000
    for pts, colour, key, z in needles(lay, speed, rpm):
        tv = [(0.0, 0.0, z, x, y) for x, y in pts]
        f = dict(kind=0, colour=colour, idx=[0, 1, 2], uv=[(0, 0)] * 3, flags=0, page=0)
        r.buckets.setdefault(ftol(key * 0.25), []).append((f, tv, None))
    pts, faces = counter_face(trk, hires)
    tv = [(0.0, 0.0, Z, x, y) for x, y in pts]
    for f in faces:
        if f["kind"] == 0xd:
            continue
        g = dict(f, uv=f["uv"][:len(f["idx"])])      # 0x4f1a18 = 1: no detail remap (0x49c64b)
        r.buckets.setdefault(ftol(15.0 * 0.25), []).append((g, tv, pages.get(f["page"])))
    r.render()
    # 0x484810 param 4: the digits (fonts 16/23, or 15/22 in the hi-res layout), noscale
    data = fnt.Data(DATA)
    scr = fnt.Screen(W, H)
    scr.px = r.px
    bx, by = lay["base"]
    fs = data.fonts[15 if hires else 16]
    fg = data.fonts[22 if hires else 23]
    fnt.draw_text(scr, fs, "%3d" % speed, bx + lay["digits"][0], by + lay["digits"][1], 0x21)
    fnt.draw_text(scr, fg, chr(gear + 0x30 + 7), bx + lay["gear"][0], by + lay["gear"][1], 0x21)
    if out:
        scr.png(out)
    return scr.px


def load565(path):
    from PIL import Image
    im = Image.open(path).convert("RGB")
    return [fnt.rgb565(*p) for p in im.getdata()]


def diff(a, b, box=(520, 400, 640, 480)):
    x0, y0, x1, y1 = box
    return sum(a[y * W + x] != b[y * W + x] for y in range(y0, y1) for x in range(x0, x1))


def main():
    a = sys.argv[1:]
    hires = "--hires" in a
    a = [x for x in a if x != "--hires"]
    if a and a[0] == "draw":
        bg = None
        if "--bg" in a:
            i = a.index("--bg")
            bg = load565(a[i + 1])
            del a[i:i + 2]
        ile, speed, rpm, gear = map(int, a[1:5])
        px = draw(ile, speed, rpm, gear, a[5], bg, hires)
        if bg:
            print("pixels different in the dial box:", diff(px, bg))
    elif a and a[0] == "fit":
        cap = load565(a[1])
        speed, gear = int(a[2]), int(a[3])
        iles = [int(a[4])] if len(a) > 4 else [1, 2, 3, 4, 5, 6, 8, 9, 10, 11]
        best = None
        for ile in iles:
            if not (DATA / "data" / ("ILE%02d.BKF" % ile)).exists():
                continue
            pages, _ = track.bank_pages(DATA / "data" / ("ILE%02d.BKF" % ile))
            for rpm in range(0, REV_MAX + 1, 250):
                d = diff(draw(ile, speed, rpm, gear, None, cap, hires, pages), cap)
                if best is None or d < best[0]:
                    best = (d, ile, rpm)
        d, ile, rpm = best
        for r_ in range(max(0, rpm - 250), min(REV_MAX, rpm + 250) + 1, 10):
            pages, _ = track.bank_pages(DATA / "data" / ("ILE%02d.BKF" % ile))
            dd = diff(draw(ile, speed, r_, gear, None, cap, hires, pages), cap)
            if dd < best[0]:
                best = (dd, ile, r_)
        print("best: ile%02d rpm %d, %d pixels different in the dial box" % (best[1], best[2], best[0]))
    else:
        print(__doc__)


if __name__ == "__main__":
    main()
