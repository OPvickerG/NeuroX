"""NeuroX chair wiring: 12 V battery, fuse, E-STOP, switch, 2 x BTS7960 (2 motors each), LM2596, ESP32 DevKit."""
from PIL import Image, ImageDraw, ImageFont
F = "/usr/share/fonts/opentype/inter"
DISP = lambda s: ImageFont.truetype(f"{F}/InterDisplay-Bold.otf", s)
B = lambda s: ImageFont.truetype(f"{F}/Inter-Bold.otf", s)
SB = lambda s: ImageFont.truetype(f"{F}/Inter-SemiBold.otf", s)
R = lambda s: ImageFont.truetype(f"{F}/Inter-Medium.otf", s)
BG = (246, 247, 249); INK = (22, 26, 34); MUT = (104, 110, 124)
RED = (210, 30, 40); BLK = (25, 25, 28); ORG = (230, 120, 20); BLU = (20, 90, 220); GRN = (30, 150, 70); PUR = (140, 60, 200); TEAL = (0, 150, 160)
W, H = 2600, 1700
im = Image.new("RGB", (W, H), BG); d = ImageDraw.Draw(im)
d.text((60, 40), "NeuroX CHAIR  WIRING", font=DISP(60), fill=INK)
d.text((62, 120), "12 V battery -> fuse -> E-STOP -> switch -> 2 x BTS7960 (2 Johnson motors each) + LM2596 5 V -> ESP32. One common GND.", font=R(30), fill=MUT)


def L(pts, col, w=7): d.line(pts, fill=col, width=w, joint="curve")
def dot(x, y, col): d.ellipse([x - 10, y - 10, x + 10, y + 10], fill=col)
def box(x0, y0, x1, y1, title, sub=None, fill=(255, 255, 255), out=(200, 204, 212), tc=INK):
    d.rounded_rectangle([x0, y0, x1, y1], 16, fill=fill, outline=out, width=3)
    d.text(((x0 + x1) / 2, y0 + 16), title, font=B(32), fill=tc, anchor="mt")
    if sub: d.multiline_text(((x0 + x1) / 2, y0 + 58), sub, font=R(23), fill=MUT if tc == INK else (190, 196, 210), anchor="ma", align="center", spacing=6)
def lab(x, y, t, col=INK, a="lm", s=26, f=None): d.text((x, y), t, font=(f or SB)(s), fill=col, anchor=a)
def motor(cx, cy, name):
    d.ellipse([cx - 48, cy - 48, cx + 48, cy + 48], fill=(205, 208, 214), outline=(120, 124, 132), width=3)
    d.text((cx, cy), "M", font=B(40), fill=INK, anchor="mm"); lab(cx, cy + 62, name, MUT, "mt", 22)


# ---- power chain (top) ----
PY = 300
box(60, 220, 300, 430, "BATTERY", "12 V\n7-9 Ah lead-acid\n(or 3S Li-ion + BMS)", fill=(255, 244, 230), out=ORG)
L([(300, PY), (420, PY)], RED); d.rounded_rectangle([420, PY - 26, 560, PY + 26], 10, fill=(255, 230, 230), outline=RED, width=3); lab(490, PY, "FUSE 20 A", RED, "mm", 22, B)
L([(560, PY), (660, PY)], RED); d.ellipse([660, PY - 55, 770, PY + 55], fill=RED); lab(715, PY, "E-STOP", (255, 255, 255), "mm", 21, B)
lab(715, PY + 76, "big red, NC", MUT, "mt", 21)
L([(770, PY), (860, PY)], RED); d.rounded_rectangle([860, PY - 28, 980, PY + 28], 10, fill=(255, 255, 255), outline=INK, width=3); lab(920, PY, "SWITCH", INK, "mm", 21, B)
L([(980, PY), (2480, PY)], RED); lab(1000, PY - 30, "+12 V (switched)", RED, "lm", 24, B)
L([(300, 400), (330, 400), (330, 470), (2480, 470)], BLK); lab(1700, 500, "GND (battery -)", BLK, "lm", 24, B)

# ---- BTS7960 x2 ----
def bts(x0, name, pins, mname):
    y0 = 620
    box(x0, y0, x0 + 520, y0 + 420, f"BTS7960  {name}", "IBT-2 module, heatsink up", fill=(36, 40, 52), out=(36, 40, 52), tc="white")
    xp = x0 + 120
    L([(xp, PY), (xp, y0)], RED); dot(xp, PY, RED); lab(xp + 12, 420, "B+", RED, "lm", 22, B)
    xg = x0 + 200
    L([(xg, 470), (xg, y0)], BLK); dot(xg, 470, BLK); lab(xg + 12, 505, "B-", BLK, "lm", 22, B)
    # motor outputs
    xm = x0 + 360
    L([(xm, y0 + 420), (xm, 1150)], ORG); L([(xm + 70, y0 + 420), (xm + 70, 1130)], ORG)
    lab(xm - 8, y0 + 445, "M+", ORG, "rm", 22, B); lab(xm + 78, y0 + 445, "M-", ORG, "lm", 22, B)
    L([(x0 + 90, 1150), (xm + 70, 1150)], ORG); L([(x0 + 130, 1130), (xm + 110, 1130)], ORG)
    L([(xm + 70, 1130), (xm + 110, 1130)], ORG)
    for i, cx in enumerate((x0 + 110, x0 + 400)):
        L([(cx - 20, 1150), (cx - 20, 1240)], ORG, 5); L([(cx + 20, 1130), (cx + 20, 1240)], ORG, 5)
        motor(cx, 1290, f"{mname} motor {i + 1}")
    lab(x0 + 255, 1405, "2 motors in parallel, same wheel", MUT, "mt", 22)
    # logic pins on the left edge
    for i, (pin, gp, col) in enumerate(pins):
        y = y0 + 150 + i * 52
        lab(x0 + 24, y, pin, (220, 224, 232), "lm", 22, B)
        L([(x0, y), (x0 - 40, y)], col, 5)
    return [(x0 - 40, y0 + 150 + i * 52) for i in range(len(pins))]


lp = bts(1380, "LEFT", [("RPWM", 25, BLU), ("LPWM", 26, BLU), ("R_EN + L_EN", 27, GRN), ("VCC", "3V3", PUR), ("GND", "GND", BLK)], "LEFT")
rp = bts(1980, "RIGHT", [("RPWM", 32, BLU), ("LPWM", 33, BLU), ("R_EN + L_EN", 13, GRN), ("VCC", "3V3", PUR), ("GND", "GND", BLK)], "RIGHT")

# ---- LM2596 + ESP32 ----
box(420, 620, 760, 820, "LM2596", "set to 5.0 V\nBEFORE connecting", fill=(255, 255, 255), out=TEAL)
L([(1040, PY), (1040, 560), (470, 560), (470, 620)], RED); dot(1040, PY, RED); lab(482, 600, "IN+", RED, "lm", 21, B)
lab(1050, 540, "after the switch", RED, "lm", 21)
L([(560, 470), (560, 620)], BLK); dot(560, 470, BLK); lab(572, 600, "IN-", BLK, "lm", 21, B)
box(420, 940, 1100, 1480, "ESP32 DevKit (38-pin)", "NeuroX_Chair_Receiver", fill=(36, 40, 52), out=(36, 40, 52), tc="white")
L([(600, 820), (600, 940)], TEAL); lab(588, 880, "5 V -> 5V", TEAL, "rm", 22, B)
L([(720, 820), (720, 940)], BLK); lab(730, 880, "GND -> GND", BLK, "lm", 22, B)
esp = [("GPIO 25", BLU), ("GPIO 26", BLU), ("GPIO 27", GRN), ("3V3", PUR), ("GND", BLK), ("GPIO 32", BLU), ("GPIO 33", BLU), ("GPIO 13", GRN)]
for i, (t, col) in enumerate(esp):
    y = 1060 + i * 48
    lab(1080, y, t, (220, 224, 232), "rm", 22, B)
    L([(1100, y), (1150 + i * 6, y)], col, 5)
# route ESP -> left driver pins (indices 0-4) and right (5-7 + shared 3V3/GND)
cols = [BLU, BLU, GRN, PUR, BLK]
for i in range(5):                                   # ESP -> LEFT driver
    y = 1060 + i * 48; xm = 1200 + i * 30; tx, ty = lp[i]
    top = 572 + (i - 3) * 13 if i in (3, 4) else ty      # 3V3 and GND also go on to the right driver
    L([(1150 + i * 6, y), (xm, y), (xm, top)], cols[i], 5); L([(xm, ty), (tx, ty)], cols[i], 5)
for k, i in enumerate((5, 6, 7)):                    # ESP -> RIGHT driver, over the top
    y = 1060 + i * 48; col = [BLU, BLU, GRN][k]; tx, ty = rp[k]
    xr = 1170 + k * 8
    yc = 532 + k * 13
    L([(1150 + i * 6, y), (xr + 30, y), (xr + 30, yc), (1900 + k * 14, yc), (1900 + k * 14, ty), (tx, ty)], col, 5)
for k, i in enumerate((3, 4)):                       # 3V3 / GND branch to the right driver
    col = [PUR, BLK][k]; xm = 1200 + i * 30; yc = 572 + k * 13; tx, ty = rp[i]
    dot(xm, yc, col); L([(xm, yc), (1950 + k * 14, yc), (1950 + k * 14, ty), (tx, ty)], col, 5)

# ---- notes ----
notes = [("Motors are 12 V:", " use a 12 V battery, never 24 V."),
         ("Each wheel:", " test both motors alone first; in parallel they must push the same way (swap a motor's 2 wires if not)."),
         ("LM2596:", " set it to 5.0 V with nothing connected, then wire the ESP32."),
         ("E-STOP:", " cuts the motors with no code involved. Press it = everything stops.")]
for i, (a, b) in enumerate(notes):
    y = 1530 + i * 42
    d.text((60, y), a, font=B(26), fill=RED if i in (0, 3) else INK)
    d.text((60 + d.textlength(a, font=B(26)), y), b, font=R(26), fill=(80, 86, 98))
im.save("img/chair_wiring.png"); print(im.size)
