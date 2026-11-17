"""HALO v3 fit check guide: what to test, where."""
from PIL import Image, ImageDraw, ImageFont
import numpy as np
SP = "/tmp/claude-0/-home-claude/31d76bfd-844e-5c8a-8c3a-6c0a47579976/scratchpad"
F = f"{SP}/halo2/fcg"
ORB = lambda s: ImageFont.truetype(f"{SP}/box3/orbitron900.ttf", s)
RAJ = lambda s: ImageFont.truetype(f"{SP}/box3/rajdhani700.ttf", s)
BG = (246, 247, 249); INK = (22, 26, 34); BL = (18, 87, 209); GREY = (110, 116, 128); GRN = (22, 140, 70); RED = (200, 40, 40)
W, H = 2400, 2200
sh = Image.new("RGB", (W, H), BG); d = ImageDraw.Draw(sh)

def load(name, box=None):
    im = Image.open(f"{F}/{name}.png").convert("RGB")
    a = np.array(im).astype(int); c0 = a[2, 2].copy(); m = np.abs(a - c0).max(axis=2) < 4; a[m] = 255
    im = Image.fromarray(a.astype("uint8"))
    if box: im = im.crop(box)
    return im
def place(im, x, y, w, h):
    s = min(w / im.width, h / im.height); t = im.resize((int(im.width * s), int(im.height * s)))
    ox, oy = x + (w - t.width) // 2, y + (h - t.height) // 2; sh.paste(t, (ox, oy)); return ox, oy, s
def badge(x, y, n, r=26):
    d.ellipse([x - r, y - r, x + r, y + r], fill=BL, outline="white", width=3)
    d.text((x, y + 1), str(n), font=RAJ(int(r * 1.3)), fill="white", anchor="mm")
def call(ox, oy, s, px, py, n, tx, ty, text):
    x, y = ox + px * s, oy + py * s
    d.line([x, y, tx, ty], fill=BL, width=4); d.ellipse([x - 8, y - 8, x + 8, y + 8], fill=BL)
    badge(tx, ty, n)
    if text and tx > x: d.text((tx + 38, ty), text, font=RAJ(32), fill=INK, anchor="lm")     # line goes left: text on the right is clear
    elif text: d.text((tx - 30, ty + 34), text, font=RAJ(32), fill=INK, anchor="lt")
def card(x, y, w, h):
    d.rounded_rectangle([x, y, x + w, y + h], 18, fill=(255, 255, 255), outline=(222, 225, 231), width=2)

# header
d.text((50, 40), "HALO v3 FIT CHECK", font=ORB(64), fill=INK)
d.text((54, 122), "plate 1 (~1 h)  -  what to test, and where.  Do 1 to 5 in order, then tell me OK / tight / loose for each.", font=RAJ(40), fill=GREY)

# row 1: outside + inside
y1, h1 = 200, 860
card(40, y1, 1150, h1); card(1210, y1, 1150, h1)
d.text((70, y1 + 20), "OUTSIDE  (as worn, from the front)", font=RAJ(36), fill=INK)
d.text((1240, y1 + 20), "INSIDE  (the side on your skin)", font=RAJ(36), fill=INK)
ox, oy, s = place(load("out", (180, 280, 1020, 1110)), 260, y1 + 80, 730, h1 - 110)
call(ox, oy, s, 360 - 180, 470 - 280, 4, 90, y1 + 200, "band clip")
call(ox, oy, s, 395 - 180, 835 - 280, 5, 90, y1 + 640, "2nd sleeve: test it too")
call(ox, oy, s, 910 - 180, 785 - 280, 5, 1010, y1 + 430, "lock knob")
call(ox, oy, s, 860 - 180, 1000 - 280, 5, 1010, y1 + 720, "test stem")
ox, oy, s = place(load("in", (190, 260, 1000, 1130)), 1430, y1 + 80, 730, h1 - 110)
call(ox, oy, s, 605 - 190, 545 - 260, 2, 2150, y1 + 200, "stud hole")
call(ox, oy, s, 770 - 190, 560 - 260, 1, 2150, y1 + 430, "snap clip")
call(ox, oy, s, 440 - 190, 690 - 260, 3, 1260, y1 + 430, "cup in holder")
call(ox, oy, s, 335 - 190, 880 - 260, 5, 1260, y1 + 720, "grooves")

# row 2: the 5 tests
y2 = y1 + h1 + 30; cw, ch = 448, 1050; gap = 14
tests = [
    ("cb", None, "Snap clip in the cup",
     "Push your lead's snap clip into the round pocket on the cup's back.",
     "Goes in with a light push and stays when you turn the cup upside down.",
     "Won't go in = tight. Falls out = loose."),
    ("cf", None, "Gel pad onto the snap",
     "From the front, push a gel pad's stud through the cup hole into the snap.",
     "It clicks like it does on your skin now.",
     "Stud won't pass the hole = tight."),
    ("in", (380, 360, 860, 870), "Cup into the holder",
     "Press the cup into the forehead holder from the skin side. Lead slots line up.",
     "Presses in firmly, doesn't wobble, doesn't fall out.",
     "Won't go in = tight. Wobbles / falls = loose."),
    ("clip", (300, 150, 1000, 1100), "Clip over the band",
     "Snap the blue test clip over the band piece.",
     "Clicks on and doesn't slide around by itself.",
     "Cracks / won't go = tight. Slides = loose."),
    ("lockx", (60, 0, 760, 1200), "The stem lock (NEW)",
     "Glue the M3x8 into the knob. Screw it in, catch the nut on the tip inside the sleeve, tighten so it pulls the nut into its pocket.",
     "Knob 2 turns out: the stem slides. Finger-tight: pull, push, shake hard - it doesn't move. Loosen, move 1 groove, tighten: locked again.",
     "Stuck with the knob out = tight. Moves when tight = tell me."),
]
for i, (img, box, title, how, ok, bad) in enumerate(tests):
    x = 40 + i * (cw + gap)
    card(x, y2, cw, ch)
    place(load(img, box), x + 20, y2 + 20, cw - 40, 380)
    badge(x + 46, y2 + 440, i + 1, 28)
    d.text((x + 86, y2 + 440), title, font=RAJ(36), fill=INK, anchor="lm")
    yy = y2 + 500
    for lab, txt, col in (("HOW", how, BL), ("OK IF", ok, GRN), ("NOT OK", bad, RED)):
        d.text((x + 24, yy), lab, font=RAJ(28), fill=col); yy += 36
        words, line = txt.split(), ""
        for wd in words:
            t = (line + " " + wd).strip()
            if d.textlength(t, font=RAJ(28)) > cw - 50: d.text((x + 24, yy), line, font=RAJ(28), fill=INK); yy += 32; line = wd
            else: line = t
        d.text((x + 24, yy), line, font=RAJ(28), fill=INK); yy += 50
d.text((W - 50, H - 24), "NeuroX HALO v3  -  every OK = print plates 2 to 8", font=RAJ(28), fill=GREY, anchor="rs")
out = f"{SP}/halo2/r/NeuroX_HALO_v3_fitcheck_guide.png"; sh.save(out); print(out)
