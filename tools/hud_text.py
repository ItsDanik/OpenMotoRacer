#!/usr/bin/env python3
"""The race HUD's texts drawn as the game draws them, laid over a capture and compared.

    python3 -I tools/hud_text.py CAPTURE.png SCENE out.png [-data DIR] [--small] [--set K=V ...]

SCENE is "racing" (0x413180: lap panel 0x483860, best lap 0x482830, TIME and POS 0x482380)
or "countdown" (0x413030: "%d LAPS" 0x482220, the number and GET READY by 0x4188b0, the best
lap panel in mode 1). --small: the "small fonts" setting (0x4f0e08), as the build/trackcap
captures were taken: the low-res fonts at 640x480 and every noscale-0 position halved.
--set gives the values (lap=1 laps=3 laptime=4060 best=43320 bestname=DSI time=31
pos=22 bikes=23 clock=5434 ...). Each text is drawn alone onto a blank screen; the pixels
it sets are compared with the capture: exact RGB565 matches at the game's position, and the
offset (within +-4) where the shape agrees best with the capture's non-background pixels.
out.png: the capture with the tool's pixels on top (magenta where they differ).
"""
import argparse, sys
from pathlib import Path
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fnt

W, H = 640, 480
LOW = {4: 0, 5: 1, 6: 2, 7: 3, 11: 10, 14: 13, 20: 19, 25: 26}   # Font_Get 0x4903d0
WHITE, BLACK = (255, 255, 255), (0, 0, 0)
RED, GREEN, ORANGE, YELLOW = (255, 0, 0), (0, 255, 0), (255, 160, 0), (255, 255, 0)
SENT = 0x0821                                               # a colour no font draws


class Hud:
    def __init__(self, data, small):
        self.data, self.small, self.items = data, small, []

    def font(self, n, fore=None, back=None):
        f = self.data.fonts[LOW.get(n, n) if self.small else n]
        if fore:
            f.fore = fore
        if back:
            f.back = back
        return f

    def draw(self, name, n, text, x, y, flags, noscale=0, fore=None, back=None, key=None):
        """DrawString 0x493820: noscale 0 halves x and y with small fonts or below 640."""
        f = self.font(n, fore, back)
        if not noscale and self.small:
            x, y = x // 2, y // 2
        scr = fnt.Screen(W, H, SENT)
        fnt.draw_text(scr, f, text, x, y, flags)
        bw, bh = fnt.measure(f, text)
        if flags & 0x1F in (8, 0x10):
            x = W - x - bw
        elif flags & 0x1F in (2, 4):
            x = (W - bw) // 2
        if flags & 0xE0 == 0x40:
            y = (H - bh) // 2
        self.items.append((name, scr, key))
        return x, y, x + bw, y + bh                         # the rect the game hands back

    def obj(self, name, n, text, x, y, align, fore=RED, back=BLACK, key=None):
        """A text object (TextList_Add 0x417d20) placed by its align bits, drawn unblinked."""
        f = self.font(n, fore, back)
        bw, bh = fnt.measure(f, text)
        if align & 6:
            x = (W - bw) // 2
        if align & 0x40:
            y += (H - bh) // 2
        elif align & 0x80:
            y = H - y - bh
        scr = fnt.Screen(W, H, SENT)
        fnt.draw_text(scr, f, text, x, y, 0x21)
        self.items.append((name, scr, key))


def split(ms):
    s = ms // 1000
    return (s // 60) % 60, s % 60, (ms % 1000) // 10


def racing(h, v):
    # 0x483860: the lap panel, top left
    r = h.draw("LAPTIME sprite", 11, "B", 2, 5, 0x21)
    lap, laps = int(v["lap"]), int(v["laps"])
    h.draw("lap n<total", 4, "%d<%d" % (min(lap, laps), laps), r[2] + 10, 5, 0x21, noscale=1)
    m, s, c = split(int(v["laptime"]))
    h.draw("lap time", 4, "%02d:%02d;%02d" % (m, s, c), 60, 25, 0x21)
    n = lap
    h.draw("lap number bar", 6, " %c%c]" % (chr(48 + n // 10) if n >= 10 else " ", chr(48 + n % 10)),
           0, 25, 0x21)
    # 0x482830 mode 0, the track record row (case 1): name bar + time to its left
    h.draw("BESTLAP sprite", 11, "C", 0, 5, 8)
    name = v["bestname"]
    bar = "[" + "@" * max(0, 8 - len(name)) + name
    bar = "".join(chr(ord(ch) + 0x40) for ch in bar)       # a record: every char + '@'
    r = h.draw("best lap name bar", 6, bar, 0, 27 - 2, 8)
    m, s, c = split(int(v["best"]))
    t = "%02d:%02d;%02d " % (m, s, c)
    f = h.font(4)
    tw, _ = fnt.measure(f, t)
    x = r[0] - tw - 5
    if h.small:
        x *= 2
    h.draw("best lap time", 4, t, x, 27, 0x21)
    # 0x482380: TIME and POS, right side
    sec = int(v["time"])
    col = GREEN if sec >= 10 else ORANGE if sec >= 3 else RED
    h.draw("TIME number", 7, "%02d" % sec, 8, 0x73, 8, fore=col, back=BLACK)
    h.draw("TIME sprite", 11, "D", 8, 0x5A, 8)
    r1 = h.draw("POS sprite", 11, "A", 0, 0x96, 8)
    r2 = h.draw("/bikes", 4, "<%d" % int(v["bikes"]), 8, 0xAB, 8, fore=WHITE, back=BLACK)
    x = max(r1[2] - r1[0], r2[2] - r2[0]) + 1
    if h.small:
        x *= 2
    h.draw("position", 20, "%d" % int(v["pos"]), x + 8, 0x96, 8)


def countdown(h, v):
    clock = int(v["clock"])
    s = clock // 1000
    laps = int(v["laps"])
    h.obj("N LAPS", 14, "%d LAP%s" % (laps, "S" if laps > 1 else ""), 0, 0x14, 0x22, key="red")
    h.obj("countdown number", 14, "%02d" % (s // 2 + 1), 0, 0, 0x42, key="red")
    if s // 2 + 1 > 1:
        h.obj("GET READY", 12, "GET READY", 0, 0x1E, 0x42, key="red")
    h.draw("BESTLAP sprite", 11, "C", 0, 5, 8)


def compare(cap, items, out):
    b = cap.tobytes()
    cpx = [fnt.rgb565(b[i], b[i + 1], b[i + 2]) for i in range(0, len(b), 3)]
    over = list(cpx)
    rows = []
    for name, scr, key in items:
        pts = [(i % W, i // W, c) for i, c in enumerate(scr.px) if c != SENT]
        if not pts:
            rows.append((name, 0, 0, None))
            continue
        same = sum(cpx[y * W + x] == c for x, y, c in pts)

        def score(dx, dy):
            k = 0
            for x, y, c in pts:
                X, Y = x + dx, y + dy
                if 0 <= X < W and 0 <= Y < H:
                    p = cpx[Y * W + X]
                    if key == "red":
                        r, g, b = fnt.to888(p)
                        k += r > g + 60 and r > b + 60
                    else:
                        k += p == c
            return k
        best = max(((score(dx, dy), -abs(dx) - abs(dy), dx, dy)
                    for dx in range(-4, 5) for dy in range(-4, 5)))
        rows.append((name, same, len(pts), (best[2], best[3], best[0])))
        for x, y, c in pts:
            over[y * W + x] = c if cpx[y * W + x] == c else 0xF81F
    im = Image.new("RGB", (W, H))
    im.putdata([fnt.to888(c) for c in over])
    im.save(out)
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("capture"); ap.add_argument("scene"); ap.add_argument("out")
    ap.add_argument("-data", default="Moto Racer")
    ap.add_argument("--small", action="store_true")
    ap.add_argument("--set", nargs="*", default=[])
    a = ap.parse_args()
    v = dict(lap=1, laps=3, laptime=0, best=0, bestname="DSI", time=0, pos=1, bikes=1, clock=0)
    v.update(kv.split("=", 1) for kv in a.set)
    h = Hud(fnt.Data(a.data), a.small)
    {"racing": racing, "countdown": countdown}[a.scene](h, v)
    cap = Image.open(a.capture).convert("RGB")
    for name, same, n, best in compare(cap, h.items, a.out):
        if best is None:
            print(f"{name:20} nothing drawn")
            continue
        dx, dy, k = best
        print(f"{name:20} {same:5}/{n:<5} exact at the game's place {100 * same / n:5.1f}%"
              f"   best shift ({dx:+d},{dy:+d}) {100 * k / n:5.1f}%")


if __name__ == "__main__":
    main()
