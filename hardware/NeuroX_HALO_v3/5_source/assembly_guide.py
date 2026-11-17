"""HALO v3 assembly guide: 2 sheets (frame, electrodes + finish)."""
from PIL import Image, ImageDraw, ImageFont, ImageChops
import numpy as np
from collections import Counter
SP = "/tmp/claude-0/-home-claude/31d76bfd-844e-5c8a-8c3a-6c0a47579976/scratchpad"
GD = f"{SP}/halo2/gd"; R = f"{SP}/halo2/r"
ORB = lambda s: ImageFont.truetype(f"{SP}/box3/orbitron900.ttf", s)
RAJ = lambda s: ImageFont.truetype(f"{SP}/box3/rajdhani700.ttf", s)
BG = (246, 247, 249); INK = (22, 26, 34); BL = (18, 87, 209); GREY = (110, 116, 128); MGc = (43, 48, 61); RED = (200, 34, 38)
W, H = 2400, 3230

def crop(path, pad=24):
    im = Image.open(path).convert("RGB")
    cc = Counter([im.getpixel(p) for p in ((2, 2), (im.width - 3, 2), (2, im.height - 3), (im.width - 3, im.height - 3))]).most_common(1)[0][0]
    a = np.array(im).astype(int); m = np.abs(a - np.array(cc)).max(axis=2) < 6; a[m] = 255
    im2 = Image.fromarray(a.astype("uint8"))
    b = ImageChops.difference(im2, Image.new("RGB", im.size, (255, 255, 255))).getbbox()
    return im2.crop((max(0, b[0] - pad), max(0, b[1] - pad), min(im.width, b[2] + pad), min(im.height, b[3] + pad)))

def wrap(d, text, font, width):
    out, line = [], ""
    for wd in text.split():
        t = (line + " " + wd).strip()
        if d.textlength(t, font=font) > width and line: out.append(line); line = wd
        else: line = t
    out.append(line); return out

def step(sh, d, x, y, w, h, img, n, title, lines, parts=None):
    d.rounded_rectangle([x, y, x + w, y + h], 20, fill=(255, 255, 255), outline=(222, 225, 231), width=2)
    ih = 560
    t = img.copy(); s = min((w - 40) / t.width, ih / t.height); t = t.resize((int(t.width * s), int(t.height * s)), Image.LANCZOS)
    sh.paste(t, (x + (w - t.width) // 2, y + 20 + (ih - t.height) // 2))
    yy = y + ih + 40
    r = 30; d.ellipse([x + 28, yy - r, x + 28 + 2 * r, yy + r], fill=BL)
    d.text((x + 28 + r, yy + 1), str(n), font=RAJ(40), fill="white", anchor="mm")
    d.text((x + 28 + 2 * r + 20, yy), title, font=RAJ(44), fill=INK, anchor="lm")
    if parts: d.text((x + w - 28, yy + 2), parts, font=RAJ(28), fill=BL, anchor="rm")
    yy += 50
    f = RAJ(32)
    for ln in lines:
        wl = wrap(d, ln, f, w - 100)
        d.ellipse([x + 38, yy + 13, x + 48, yy + 23], fill=BL)
        for k, l in enumerate(wl): d.text((x + 64, yy), l, font=f, fill=INK); yy += 37
        yy += 7

def header(d, sub, page):
    d.text((60, 44), "HALO v3  ASSEMBLY", font=ORB(68), fill=INK)
    d.text((64, 134), sub, font=RAJ(40), fill=GREY)
    d.text((W - 60, 70), page, font=RAJ(40), fill=BL, anchor="rm")
    lx = 64
    for txt, c in (("new parts in colour", BL), ("already built = grey", (190, 194, 202)), ("red arrow = where it goes", RED)):
        d.rounded_rectangle([lx, 196, lx + 30, 226], 6, fill=c); d.text((lx + 42, 211), txt, font=RAJ(30), fill=INK, anchor="lm"); lx += 90 + int(d.textlength(txt, font=RAJ(30))) + 20

def card_dark(d, x, y, w, h, title, rows):
    d.rounded_rectangle([x, y, x + w, y + h], 20, fill=MGc)
    d.text((x + 34, y + 30), title, font=RAJ(44), fill="white")
    yy = y + 100
    for a, b in rows:
        if a is None: yy += 18; continue
        d.text((x + 34, yy), a, font=RAJ(31), fill=(130, 175, 255))
        for l in wrap(d, b, RAJ(31), w - 330): d.text((x + 290, yy), l, font=RAJ(31), fill="white"); yy += 38
        yy += 6

PW, PH, GX, GY, X0, Y0 = 1140, 950, 40, 30, 40, 270
def pos(i): return X0 + (i % 2) * (PW + GX), Y0 + (i // 2) * (PH + GY)

# ---------------- sheet 1: the frame ----------------
sh = Image.new("RGB", (W, H), BG); d = ImageDraw.Draw(sh)
header(d, "1 / 2  -  build the frame.  Print + clean all parts first (supports off the sides + strap).", "SHEET 1 / 2")
S1 = [
    ("s2", 1, "Join the band", "4 x M3x20 countersunk", [
        "Front band + left side + right side.",
        "Slide each side's LOWER tab under the front band's UPPER tab, at the temples.",
        "2 x M3x20 straight down through each joint. Snug, not tight.",
        "Left side = 2 screw holes at the back end (strap). Right side = the ridges + the dial nut pocket."]),
    ("s3", 2, "Ribs + crown hub", "10 x M3x12 pan head", [
        "Push the 5 rib tabs into the band pockets: the short-tab spine at the front centre, the 4 side ribs at the sides.",
        "One M3x12 sideways through each pocket, from the outside.",
        "Put the crown hub on the 5 rib tops, 5 x M3x12 down through the hub.",
        "Too tight in a pocket? Scrape the pocket roof with a knife."]),
    ("s1", 3, "Close the crown hub", "glue", [
        "After the 5 hub screws: press the blue ring into the hub top.",
        "Then the white cap with the blue N inside the ring.",
        "A drop of super glue under each. They hide the screws."]),
    ("s4", 4, "Rear strap + dial", "2 x M3x10 CSK, M4x16 + nut, M3x10", [
        "Thread the strap through the white back hub's tunnel FIRST.",
        "Screw the strap's left end onto the left side: 2 x M3x10.",
        "Lay the right end over the ridges of the right side.",
        "M4 bolt through the dial + the strap slot, nut in the hex pocket inside. Loosen, slide to fit, tighten.",
        "Slide the back hub to the middle, M3x10 set screw on top."]),
    (None, 5, "Lock knobs (x6)", "6 x M3x8 socket head + 6 x M3 nut", [
        "Glue: a drop of CA in the knob's hole, press an M3x8 in head first. Wait 10 min.",
        "Screw the knob into the boss on a sleeve until the tip shows inside.",
        "Hold the nut on the tip, turn the knob: it pulls the nut into its hex pocket.",
        "Back the knob off 2 turns. Do all 6 (the nose sleeve has 2)."]),
]
for i, (img, n, title, parts, lines) in enumerate(S1):
    im = crop(f"{GD}/{img}.png") if img else crop(f"{GD}/s5.png")
    x, y = pos(i); step(sh, d, x, y, PW, PH, im, n, title, lines, parts)
x, y = pos(5)
card_dark(d, x, y, PW, PH, "SHEET 1  -  YOU NEED", [
    ("Screws", "4 x M3x20 countersunk (band joints)"), ("", "10 x M3x12 pan head (ribs + hub)"),
    ("", "2 x M3x10 countersunk (strap) + 1 x M3x10 (set screw)"), ("", "6 x M3x8 socket head + 6 x M3 nut (knobs)"),
    ("", "1 x M4x16 bolt + 1 x M4 nut (dial)"), (None, None),
    ("Tools", "screwdriver, 2.5 mm hex key, super glue, small knife / file"), (None, None),
    ("Tip", "All M3 screws (except the knob screws) cut their own thread in the plastic. Turn slowly, stop when snug."),
    ("", "Each step: fit it dry first, then screw.")])
d.text((W - 50, H - 26), "NeuroX HALO v3  -  sheet 1: frame", font=RAJ(28), fill=GREY, anchor="rs")
sh.save(f"{R}/NeuroX_HALO_v3_assembly_1_frame.png")

# ---------------- sheet 2: electrodes + finish ----------------
sh = Image.new("RGB", (W, H), BG); d = ImageDraw.Draw(sh)
header(d, "2 / 2  -  electrodes, leads, covers, fitting.", "SHEET 2 / 2")
S2 = [
    ("s6", 6, "Cups + stems", "6 cups, 5 sliders", [
        "Snap each lead's clip into a cup, wire out through the cup slot.",
        "Press the cup into its holder from the skin side, slots lined up: forehead on the band, the rest on the sliders.",
        "Knob 2 turns out. Slide each tongue up into its sleeve, grooves towards the skin. Nose: 2 tongues, 2 sleeves.",
        "Start on the long blue mark, finger-tighten the knob."]),
    ("s7", 7, "Leads + clips", "14 band + 10 arm clips, 2 x M3x12", [
        "LEFT side: forehead, nose, left temple, left REF.  RIGHT side: right temple, right REF.",
        "Leave a 2-3 cm loop at every stem so it can still slide.",
        "Lay the leads on the outside of the band + strap, snap the blue clips over. 2 arm clips per arm.",
        "Temple leads go under the white temple caps: M3x12 in the middle of the blue ring."]),
    ("s8", 8, "Crest", "2 x M3x10 countersunk", [
        "Last on the front: the crest covers the forehead holder and the leads behind it.",
        "2 x M3x10 into the two stand-offs on the band.",
        "Check the forehead and nose leads run free under it."]),
    ("s9", 9, "Cables + back cover", "2 zip ties, 4 x M3x10 CSK", [
        "Both pill cables come up into the back hub from below. The split into 3 leads sits inside.",
        "Zip-tie each cable through its slot pair = strain relief.",
        "Back cover on, 4 x M3x10."]),
    ("s10", 10, "Foam + wear it", "EVA foam", [
        "4-5 mm foam strips inside the front + sides (not on the pods).",
        "10-12 mm pad inside the back strap, a 5 mm disc under the crown hub.",
        "Put it on, tighten the dial at the back."]),
]
for i, (img, n, title, parts, lines) in enumerate(S2):
    x, y = pos(i); step(sh, d, x, y, PW, PH, crop(f"{GD}/{img}.png"), n, title, lines, parts)
x, y = pos(5)
card_dark(d, x, y, PW, PH, "FIT THE STEMS  (every pod)", [
    ("1", "Knob 2 turns out."), ("2", "Push or pull until the gel pad sits on its spot."),
    ("", "Temples: eye level, the soft spot beside the eye."), ("", "REF: on the bone behind the ear."), ("", "Nose: just above the nose bridge."),
    ("3", "Wiggle it a little until it drops into the nearest groove."),
    ("4", "Finger-tight. Don't force it."), (None, None),
    ("Marks", "1 groove = 2 mm. Set left + right to the same blue mark and write your marks down: next time it takes 10 s."),
    ("Safety", "Battery only while worn, never while charging, never on the wheelchair battery.")])
d.text((W - 50, H - 26), "NeuroX HALO v3  -  sheet 2: electrodes + finish", font=RAJ(28), fill=GREY, anchor="rs")
sh.save(f"{R}/NeuroX_HALO_v3_assembly_2_finish.png")
print("ok")
