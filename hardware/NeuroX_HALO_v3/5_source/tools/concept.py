"""NeuroX HALO v3 concept sheet"""
from PIL import Image, ImageDraw, ImageFont, ImageChops
import os
SP = "/tmp/claude-0/-home-claude/31d76bfd-844e-5c8a-8c3a-6c0a47579976/scratchpad"
R = f"{SP}/halo2/r"; FONT = f"{SP}/box3"
ORB = lambda s: ImageFont.truetype(f"{FONT}/orbitron900.ttf", s)
RAJ = lambda s: ImageFont.truetype(f"{FONT}/rajdhani700.ttf", s)
BG = (246, 247, 249); INK = (22, 26, 34); MGc = (43, 48, 61); BLc = (18, 87, 209); WHc = (240, 242, 245); RDc = (214, 26, 31); GREY = (120, 126, 138)
W, H = 2400, 2400
sheet = Image.new("RGB", (W, H), BG); d = ImageDraw.Draw(sheet)

def crop(path, pad=20):
    im = Image.open(path).convert("RGB")
    import numpy as np
    from collections import Counter
    cc = Counter([im.getpixel(p) for p in ((2, 2), (im.width - 3, 2), (2, im.height - 3), (im.width - 3, im.height - 3))]).most_common(1)[0][0]
    bg = Image.new("RGB", im.size, cc)
    a = np.array(im).astype(int); c0 = np.array(cc)
    mask = (np.abs(a - c0).max(axis=2) < 4); a[mask] = 255
    b = ImageChops.difference(im, bg).getbbox(); im = Image.fromarray(a.astype("uint8")); b = (max(0, b[0] - pad), max(0, b[1] - pad), min(im.width, b[2] + pad), min(im.height, b[3] + pad))
    global LAST_BOX; LAST_BOX = b
    return im.crop(b)
LOCK_LABELS = [(1000, 770, 330, 470, "thumb knob + M3x8"), (545, 565, 0, 120, "M3 nut"), (800, 520, 360, 200, "boss + tooth inside"), (440, 720, 0, 340, "grooves every 2 mm")]
def panel(x, y, w, h, img, title, num, sub=None):
    d.rounded_rectangle([x, y, x + w, y + h], 18, fill=(255, 255, 255), outline=(222, 225, 231), width=2)
    im = img.copy(); im.thumbnail((w - 30, h - 110))
    bgc = im.getpixel((1, 1))                    # render background -> white
    im = Image.eval(im, lambda v: v)
    px = im.load()
    for yy in range(im.height):
        for xx in range(im.width):
            p = px[xx, yy]
            if abs(p[0] - bgc[0]) < 3 and abs(p[1] - bgc[1]) < 3 and abs(p[2] - bgc[2]) < 3: px[xx, yy] = (255, 255, 255)
    sheet.paste(im, (x + (w - im.width) // 2, y + 18 + (h - 110 - im.height) // 2))
    d.ellipse([x + 20, y + h - 78, x + 62, y + h - 36], fill=BLc)
    d.text((x + 41, y + h - 57), str(num), font=RAJ(28), fill="white", anchor="mm")
    d.text((x + 76, y + h - 74), title, font=RAJ(34), fill=INK)
    if sub: d.text((x + 76, y + h - 40), sub, font=RAJ(24), fill=GREY)
    return im

# header
d.text((60, 46), "NeuroX HALO v3", font=ORB(70), fill=INK)
d.text((64, 136), "extendable stems with a screw lock  -  loosen, slide, tighten: locked in 2 mm steps", font=RAJ(40), fill=GREY)
lx = 1500
for name, c in (("midnight gray", MGc), ("white", WHc), ("blue", BLc), ("red", RDc)):
    d.rounded_rectangle([lx, 66, lx + 40, 106], 8, fill=c, outline=(190, 194, 202)); d.text((lx + 52, 86), name, font=RAJ(30), fill=INK, anchor="lm"); lx += 52 + int(d.textlength(name, font=RAJ(30))) + 36

y1 = 210
panel(40, y1, 1060, 980, crop(f"{R}/hero.png"), "HALO v3 on the head", 1, "passive chassis, 6 snap electrodes, 6 lock knobs")
panel(1120, y1, 600, 980, crop(f"{R}/front.png"), "Nose: twin tongues", 2, "one lock knob per tongue")
panel(1740, y1, 620, 980, crop(f"{R}/side.png"), "Temple + REF stems", 3, "knob on the sleeve, stem below")

y2 = y1 + 1000
def lab(x, y, tx, ty, text):
    d.line([x, y, tx, ty], fill=BLc, width=4); d.ellipse([x - 7, y - 7, x + 7, y + 7], fill=BLc)
    d.rounded_rectangle([tx - 6, ty - 22, tx + int(d.textlength(text, font=RAJ(28))) + 8, ty + 20], 8, fill=(255, 255, 255), outline=BLc, width=2)
    d.text((tx + 1, ty), text, font=RAJ(28), fill=INK, anchor="lm")
lock_img = crop(f"{R}/lock.png"); im = panel(40, y2, 760, 640, lock_img, "The lock, taken apart", 4, "knob + M3x8 - M3 nut - tooth inside the sleeve")
ox, oy = 40 + (760 - im.width) // 2, y2 + 18 + (640 - 110 - im.height) // 2
P = lambda fx, fy: (ox + int(fx * im.width), oy + int(fy * im.height))
sc = im.width / (LAST_BOX[2] - LAST_BOX[0])
for (px_, py_, tx, ty, t) in LOCK_LABELS: lab(ox + int((px_ - LAST_BOX[0]) * sc), oy + int((py_ - LAST_BOX[1]) * sc), ox + tx, oy + ty, t)
panel(820, y2, 520, 640, crop(f"{R}/skin.png"), "Grooves under every stem", 5, "V-grooves every 2 mm + blue marks on top")
sh = crop(f"{R}/short.png"); lo = crop(f"{R}/long.png")
pair = Image.new("RGB", (2 * 560 + 30, 600), (255, 255, 255))
for i, (img, txt) in enumerate(((sh, "SHORTEST  -6 mm"), (lo, "LONGEST  +4 mm"))):
    c = img.copy(); c.thumbnail((540, 540)); pair.paste(c, (i * 590 + (560 - c.width) // 2, 50))
    ImageDraw.Draw(pair).text((i * 590 + 280, 18), txt, font=RAJ(34), fill=BLc, anchor="mm")
panel(1360, y2, 1000, 640, pair, "Loosen, push / pull, tighten", 6, "temples -6/+4  REF -6/+6  nose -2/+10 mm")

y3 = y2 + 660
d.rounded_rectangle([40, y3, 1500, y3 + 480], 18, fill=(255, 255, 255), outline=(222, 225, 231), width=2)
d.text((70, y3 + 18), "HOW IT LOCKS  (real cut through a sleeve)", font=RAJ(32), fill=INK)
dg = Image.open(f"{R}/lockdiagram.png").convert("RGB"); dg.thumbnail((1420, 410)); sheet.paste(dg, (40 + (1460 - dg.width) // 2, y3 + 62))
d.rounded_rectangle([1520, y3, 2360, y3 + 480], 18, fill=MGc)
specs = [("Stems", "5 sliders, 6 sleeves (nose = twin)"),
         ("Lock", "M3x8 + nut presses stem onto a tooth"),
         ("Steps", "2 mm grooves, blue marks every 2 mm"),
         ("Print", "8 AMS plates, PLA, NO supports"),
         ("Files", "3MF  STEP  STL  OpenSCAD")]
for i, (a, b) in enumerate(specs):
    d.text((1550, y3 + 50 + i * 82), a, font=RAJ(32), fill=(120, 170, 255))
    d.text((1690, y3 + 50 + i * 82), b, font=RAJ(31), fill="white")
d.text((W - 60, H - 30), "NeuroX  -  hands-free mobility", font=RAJ(26), fill=GREY, anchor="rs")
out = f"{SP}/halo2/r/NeuroX_HALO_v3_concept.png"; sheet.save(out); print(out, sheet.size)
