"""HALO v3 assembly guide: 2 sheets (frame, electrodes + finish)."""
from PIL import Image, ImageDraw, ImageFont, ImageChops
import numpy as np
from collections import Counter
SP = "/tmp/claude-0/-home-claude/31d76bfd-844e-5c8a-8c3a-6c0a47579976/scratchpad"
GD = f"{SP}/box3/gd"; R = f"{SP}/box3/gd"
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
    d.text((60, 44), "BOX v3  ASSEMBLY", font=ORB(68), fill=INK)
    d.text((64, 134), sub, font=RAJ(40), fill=GREY)
    d.text((W - 60, 70), page, font=RAJ(40), fill=BL, anchor="rm")
    lx = 64
    for txt, c in (("new parts in colour", BL), ("already built = grey", (190, 194, 202)), ("red arrow = where it goes", RED)):
        d.rounded_rectangle([lx, 196, lx + 30, 226], 6, fill=c); d.text((lx + 42, 211), txt, font=RAJ(30), fill=INK, anchor="lm"); lx += 90 + int(d.textlength(txt, font=RAJ(30))) + 20

def card_dark(d, x, y, w, h, title, rows, lw=290):
    d.rounded_rectangle([x, y, x + w, y + h], 20, fill=MGc)
    d.text((x + 34, y + 30), title, font=RAJ(44), fill="white")
    yy = y + 100
    for a, b in rows:
        if a is None: yy += 18; continue
        d.text((x + 34, yy), a, font=RAJ(31), fill=(130, 175, 255))
        for l in wrap(d, b, RAJ(31), w - lw - 40): d.text((x + lw, yy), l, font=RAJ(31), fill="white"); yy += 38
        yy += 6

PW, PH, GX, GY, X0, Y0 = 1140, 950, 40, 30, 40, 270
def pos(i): return X0 + (i % 2) * (PW + GX), Y0 + (i // 2) * (PH + GY)

# ---------------- sheet 1: base ----------------
sh = Image.new("RGB", (W, H), BG); d = ImageDraw.Draw(sh)
header(d, "1 / 2  -  the base: grommet, power parts, main board.", "SHEET 1 / 2")
S1 = [
    ("s1", 1, "Prepare the base", "grommet, 4 feet, buzzer, MPU-6050", [
        "Push the blue grommet into the round hole in the right wall, from outside, until it clicks.",
        "Stick the 4 rubber feet (10 mm) into the 4 round recesses underneath.",
        "Buzzer into the round ring in the floor, pins up, a drop of glue. It sounds through the holes in the bottom.",
        "MPU-6050 (GY-521) onto the 2 standoffs in the left rear corner: 2 x M2x6, flat board down. SDA/SCL/VCC/GND face up."]),
    ("s2", 2, "Power parts", "TP4056, MT3608, rocker, LiPo", [
        "FIRST set the MT3608 boost to 5.1 V with a multimeter (turn its little screw).",
        "TP4056 onto its shelf at the front left, USB-C into the CHARGE slot in the left wall.",
        "Rocker switch into the POWER cutout in the left wall, from outside, until it snaps.",
        "Battery into its rim with foam tape, boost on tape in front of it. Wire: POWER card."]),
    ("s3", 3, "Main board", "4 x M2x6", [
        "Board wired per the WIRING MAP, ESP32-S3 plugged in, its USB-C at the right-wall slot.",
        "Deepen each of the 4 standoff holes by about 1.5 mm first (1.5-2 mm drill bit, twisted by hand). They are 3 mm deep and an M2x6 needs 4.4 mm. Not more than 1.5 mm: the floor is 2 mm.",
        "4 x M2x6 through the perfboard into the standoffs, snug.",
        "MPU-6050: 4 wires (VCC GND SCL SDA) to its header. Pills are in the HALO now."]),
]
for i, (img, n, title, parts, lines) in enumerate(S1):
    x, y = pos(i); step(sh, d, x, y, PW, PH, crop(f"{GD}/{img}.png"), n, title, lines, parts)
x, y = pos(3)
card_dark(d, x, y, PW, PH, "POWER WIRING", [
    ("LiPo", "+ to TP4056 B+,  - to TP4056 B-"),
    ("TP4056", "OUT+ to the rocker, rocker to boost IN+"), ("", "OUT- to boost IN-"),
    ("Boost", "OUT+ (5.1 V) - D1 1N5819 - ESP32 5V,  OUT- to GND"), ("", "100 uF across the boost output"),
    (None, None),
    ("Battery", "boost IN+ - 100 k - GPIO 2 - 100 k - GND, 100 nF to GND"),
    ("Charger", "TP4056 IN+ - 100 k - GPIO 21 - 150 k - GND"),
    (None, None),
    ("Charging", "rocker OFF + USB-C in the CHARGE slot."),
    ("Check", "before plugging the ESP32: 5.1 V at the boost output, polarity right.")])
x, y = pos(4)
card_dark(d, x, y, PW, PH, "PIN MAP  (ESP32-S3)", [
    ("GPIO 13", "buzzer +"),
    ("GPIO 41 / 42", "MPU-6050 SDA / SCL,  VCC to 3.3 V"),
    ("12 / 11 / 10", "screen SCK / MOSI / MISO"), ("9 / 8 / 18 / 14", "screen CS / DC / RESET / LED"),
    ("17 / 16", "touch T_CS / T_IRQ (T_CLK, T_DIN, T_DO share SCK / MOSI / MISO)"),
    ("15 / 7", "button STOP / NEW"), ("6 / 5", "button LEFT / RIGHT-OK"), ("", "other leg of every button to GND"),
    (None, None),
    ("Free", "GPIO 1 + 4 (the pills moved to the HALO C3)")])
x, y = pos(5)
card_dark(d, x, y, PW, PH, "YOU NEED", [
    ("Screws", "4 x M3x12 countersunk (lid)"), ("", "10 x M2x6 (4 main board, 4 screen tabs, 2 MPU-6050)"), ("", "8 x M2x10 (buttons, 2 each)"),
    (None, None),
    ("Parts", "ESP32-S3 DevKitC-1, 2.8\" ILI9341 screen, 4 x 12x12 tactile switches, KCD1 rocker, TP4056 USB-C, MT3608, LiPo 103450 with protection, buzzer, GY-521 MPU-6050 breakout, perfboard 68 x 50"),
    (None, None),
    ("Also", "4 rubber feet 10 mm, foam tape, 100 k x 3 + 150 k, 100 nF, 100 uF, 1N5819, thin wire, super glue"),
    ("Tools", "Phillips screwdriver, soldering iron, multimeter, 1.5-2 mm drill bit")])
d.text((W - 50, H - 26), "NeuroX Box v3  -  sheet 1: base", font=RAJ(28), fill=GREY, anchor="rs")
sh.save(f"{R}/NeuroX_Box_v3_assembly_1_base.png")

# ---------------- sheet 2: lid + close ----------------
sh = Image.new("RGB", (W, H), BG); d = ImageDraw.Draw(sh)
header(d, "2 / 2  -  the lid: buttons, screen, then close it.", "SHEET 2 / 2")
S2 = [
    ("s4", 4, "Buttons (3 + STOP)", "8 x M2x10", [
        "Lid upside down. STOP first: glue the red guard on top, around the STOP hole.",
        "Guide up into the lid hole from below + a drop of glue. Cap into the guide from below (red for STOP).",
        "Switch into the cradle, button up, pins through the slots, bent flat. Solder 2 wires to DIAGONAL pins.",
        "Cradle onto the guide: 2 x M2x10 up into the flange, snug. Glue the red STOP top on the STOP cap."]),
    ("s5", 5, "Screen", "4 tabs, 4 x M2x6", [
        "Screen glass-first into its pocket, header pins towards the open end of the pocket.",
        "Slide the 4 clamp tabs into the notches under the board edge, 1 x M2x6 each into the post. Snug only.",
        "Solder or plug the screen wires (pin map, sheet 1). Only the lit picture should show in the window."]),
    ("s6", 6, "Close the box", "4 x M3x12 countersunk", [
        "Connect the screen and the 4 button wires to the main board. Button: one wire to its GPIO, one to GND.",
        "Check nothing is pinched, then put the lid on.",
        "4 x M3x12 countersunk into the 4 corner bosses. Stop when the heads sit flush."]),
    ("s7", 7, "Switch on", "battery only", [
        "Rocker ON. It calibrates by itself (~40 s): follow the beeps - blink 5 x, clench 5 x, close your eyes firmly.",
        "Long melody = calibrated and LOCKED. Close your eyes firmly for 3 s to unlock.",
        "STOP stops and locks at any time. NEW calibrates again."]),
]
for i, (img, n, title, parts, lines) in enumerate(S2):
    x, y = pos(i); step(sh, d, x, y, PW, PH, crop(f"{GD}/{img}.png"), n, title, lines, parts)
x, y = pos(4)
card_dark(d, x, y, 2 * PW + GX, 520, "SAFETY + CHECKS", [
    ("Electrodes on", "the HALO runs on its own battery, nothing in its USB-C. The box has no wire to you, so it may charge."),
    ("Charging", "rocker OFF, USB-C in the CHARGE slot (left side)."),
    ("Programming", "rocker OFF, then USB-C on the right side (USB)."),
    (None, None),
    ("Nothing on screen", "check the screen wires against the pin map, and the LED pin (GPIO 14)."),
    ("Button does nothing", "the 2 wires must be on DIAGONAL pins of the switch."),
    ("Resets / BROWNOUT", "set the boost to 5.1 V again, charge the battery."),
    ("CLIPPED / noisy", "re-gel the electrodes, keep the pill wires short, away from chargers.")], lw=380)
H2 = y + 520 + 60
sh = sh.crop((0, 0, W, H2)); d = ImageDraw.Draw(sh)
d.text((W - 50, H2 - 20), "NeuroX Box v3  -  sheet 2: lid + close", font=RAJ(28), fill=GREY, anchor="rs")
sh.save(f"{R}/NeuroX_Box_v3_assembly_2_lid.png")
print("ok")
