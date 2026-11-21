"""NeuroX box v3.1 - VBAT + VUSB sense dividers, drawn as real parts (bands, 104 cap, TP4056 IN+ pad, J3 pins)."""
from PIL import Image, ImageDraw, ImageFont
SP = "/tmp/claude-0/-home-claude/31d76bfd-844e-5c8a-8c3a-6c0a47579976/scratchpad"
RAJ = lambda s: ImageFont.truetype(f"{SP}/box3/rajdhani700.ttf", s)
ORB = lambda s: ImageFont.truetype(f"{SP}/box3/orbitron900.ttf", s)
BG = (246, 247, 249); INK = (22, 26, 34); MUT = (110, 116, 128); LINE = (215, 218, 224)
BAT = (230, 120, 20); GND = (20, 20, 20); SEN = (0, 150, 160); RED = (210, 30, 40); BLUE = (20, 90, 220)
BAND = {"brown": (125, 70, 30), "black": (25, 25, 25), "yellow": (240, 195, 0), "green": (30, 150, 70), "gold": (196, 156, 60)}
W, H = 2500, 1720
im = Image.new("RGB", (W, H), BG); d = ImageDraw.Draw(im)
d.text((60, 40), "VBAT + VUSB  SENSE  WIRING", font=ORB(56), fill=INK)
d.text((64, 118), "Two resistor dividers so the ESP32 can read the battery and see the charger. Each one halves (or so) the voltage, so the GPIO never sees more than ~2.1 / 3.0 V.", font=RAJ(32), fill=MUT)


def L(pts, col, w=8): d.line(pts, fill=col, width=w, joint="curve")
def dot(x, y, col, r=12): d.ellipse([x - r, y - r, x + r, y + r], fill=col)
def lab(x, y, t, col=INK, a="lm", s=30): d.text((x, y), t, font=RAJ(s), fill=col, anchor=a)


def resistor(x, y0, y1, bands, name, val, side="r"):
    """vertical through-hole resistor between y0 and y1 (lead ends)."""
    m = (y0 + y1) / 2; bh = 150
    L([(x, y0), (x, m - bh / 2)], (150, 150, 150), 5); L([(x, m + bh / 2), (x, y1)], (150, 150, 150), 5)
    d.rounded_rectangle([x - 26, m - bh / 2, x + 26, m + bh / 2], 20, fill=(226, 206, 168), outline=(170, 150, 115), width=3)
    for i, b in enumerate(bands):
        yy = m - bh / 2 + 24 + i * 28 if i < 3 else m + bh / 2 - 34
        d.rectangle([x - 26, yy, x + 26, yy + 13], fill=BAND[b])
    tx = x + 50 if side == "r" else x - 50; a = "lm" if side == "r" else "rm"
    lab(tx, m - 22, f"{name}  {val}", INK, a, 34)
    lab(tx, m + 18, " - ".join(bands[:3]), MUT, a, 26)


def cap104(x, y0, y1, name):
    m = (y0 + y1) / 2
    L([(x - 14, y0), (x - 14, m)], (150, 150, 150), 5); L([(x + 14, y0), (x + 14, m)], (150, 150, 150), 5)
    L([(x - 14, m), (x - 14, y1)], (150, 150, 150), 5); L([(x + 14, m), (x + 14, y1)], (150, 150, 150), 5)
    d.ellipse([x - 44, m - 44, x + 44, m + 44], fill=(222, 140, 40), outline=(170, 100, 20), width=3)
    d.text((x, m), "104", font=RAJ(30), fill="white", anchor="mm")
    lab(x + 62, m - 16, name, INK, "lm", 32); lab(x + 62, m + 18, "100 nF, either way round", MUT, "lm", 24)


def pin(x, y, text, col, sub=None):
    d.rectangle([x - 22, y - 22, x + 22, y + 22], fill=(30, 32, 38)); d.rectangle([x - 8, y - 8, x + 8, y + 8], fill=(205, 175, 90))
    lab(x, y - 40, text, col, "mb", 32)
    if sub: lab(x, y + 40, sub, MUT, "mt", 24)


def rail(x0, x1, y):
    L([(x0, y), (x1, y)], GND, 12); lab(x1 + 18, y, "GND rail", GND, "lm", 30)


# ---------------- ESP32-S3 J3 header (right) ----------------
HX = 2130; PY = lambda i: 300 + i * 58
J3 = ["GND", "TX", "RX", "1", "2", "42", "41", "40", "39", "38", "37", "36", "35", "0", "45", "48", "47", "21", "20", "19", "GND", "GND"]
d.rounded_rectangle([HX - 40, PY(0) - 50, HX + 40, PY(21) + 50], 14, fill=(36, 40, 52))
lab(HX, PY(0) - 66, "ESP32-S3  J3", INK, "mb", 32)
lab(HX, PY(21) + 64, "antenna end at the top", MUT, "mt", 22)
for i, n in enumerate(J3):
    y = PY(i); hot = n in ("2", "21")
    col = SEN if hot else (GND if n == "GND" else (95, 100, 112))
    d.rectangle([HX - 14, y - 14, HX + 14, y + 14], fill=col if (hot or n == "GND") else (70, 74, 86))
    name = n if n in ("GND", "TX", "RX") else f"GPIO {n}"
    d.text((HX + 58, y), f"{i + 1}", font=RAJ(26 if hot else 22), fill=SEN if hot else MUT, anchor="lm")
    d.text((HX + 96, y), name, font=RAJ(30 if hot else 22), fill=SEN if hot else (MUT if n != "GND" else INK), anchor="lm")

# ---------------- 1. battery sense ----------------
d.rounded_rectangle([40, 200, 1990, 930], 20, outline=LINE, width=3)
lab(70, 240, "1  BATTERY SENSE   VBAT  ->  GPIO 2", SEN, "lm", 38)
vx, vy = 260, 400
pin(vx, vy, "VBAT pin", BAT, "rocker out = MT3608 IN+")
RX1 = 760
L([(vx + 22, vy), (RX1, vy)], BAT); L([(RX1, vy), (RX1, 420)], BAT)
lab(480, vy - 22, "3.0 - 4.2 V", BAT, "mb", 26)
resistor(RX1, 420, 620, ["brown", "black", "yellow", "gold"], "R1", "100k", "l")
NY = 650
L([(RX1, 620), (RX1, NY)], SEN); dot(RX1, NY, SEN, 15)
resistor(RX1, NY, 880, ["brown", "black", "yellow", "gold"], "R2", "100k", "l")
CX = 1060
L([(RX1, NY), (1700, NY)], SEN); dot(CX, NY, SEN, 13)
cap104(CX, NY, 880, "C1")
rail(RX1 - 40, CX + 60, 880)
L([(1700, NY), (1700, PY(4)), (HX - 14, PY(4))], SEN)
lab(1410, NY - 22, "middle point  ~2.1 V", SEN, "mb", 28)
lab(1720, (NY + PY(4)) / 2 + 10, "J3 pin 5", SEN, "lm", 28)
d.multiline_text((1440, 730), "full battery 4.2 V  ->  2.1 V on GPIO 2\nempty 3.3 V  ->  1.65 V\nfirmware doubles it for the battery %", font=RAJ(26), fill=MUT, spacing=8)

# ---------------- 2. USB sense ----------------
d.rounded_rectangle([40, 970, 1990, 1680], 20, outline=LINE, width=3)
lab(70, 1010, "2  USB SENSE   TP4056 IN+  ->  GPIO 21", SEN, "lm", 38)
# TP4056 board
bx0, by0, bx1, by1 = 110, 1090, 560, 1300
d.rounded_rectangle([bx0, by0, bx1, by1], 10, fill=(28, 70, 150))
d.rounded_rectangle([bx0 - 26, by0 + 70, bx0 + 50, by0 + 140], 10, fill=(200, 204, 212), outline=(150, 154, 160), width=2)
lab(bx0 + 12, by0 + 105, "USB-C", INK, "mm", 18)
d.text(((bx0 + bx1) / 2 + 40, by0 + 108), "TP4056", font=RAJ(34), fill="white", anchor="mm")
pads = [("IN+", bx0 + 90, by0 + 26), ("IN-", bx0 + 90, by1 - 26), ("B+", (bx0 + bx1) / 2 + 40, by0 + 26), ("B-", (bx0 + bx1) / 2 + 40, by1 - 26),
        ("OUT+", bx1 - 50, by0 + 26), ("OUT-", bx1 - 50, by1 - 26)]
for n, x, y in pads:
    d.rectangle([x - 18, y - 12, x + 18, y + 12], fill=(205, 175, 90))
    d.text((x, y + (-34 if y > by0 + 100 else 34)), n, font=RAJ(22), fill="white", anchor="mm")
ipx, ipy = pads[0][1], pads[0][2]
d.ellipse([ipx - 34, ipy - 30, ipx + 34, ipy + 30], outline=BAT, width=5)
lab(bx0, by1 + 34, "IN+ = the pad next to the USB-C port (5 V only while charging)", BAT, "lm", 24)
# wire IN+ -> VUSB pin -> R3
ux, uy = 860, 1160
L([(ipx, ipy - 30), (ipx, 1050), (ux, 1050), (ux, uy - 22)], BAT)
pin(ux + 0, uy, "", BAT)
lab(ux, uy + 40, "VUSB pin", BAT, "mt", 30)
RX3 = 1100
L([(ux + 22, uy), (RX3, uy), (RX3, 1190)], BAT)
resistor(RX3, 1190, 1390, ["brown", "black", "yellow", "gold"], "R3", "100k", "l")
NY2 = 1420
L([(RX3, 1390), (RX3, NY2)], SEN); dot(RX3, NY2, SEN, 15)
resistor(RX3, NY2, 1640, ["brown", "green", "yellow", "gold"], "R4", "150k", "r")
rail(RX3 - 60, RX3 + 60, 1640)
L([(RX3, NY2), (1800, NY2), (1800, PY(17)), (HX - 14, PY(17))], SEN)
lab(1500, NY2 - 22, "middle point  ~3.0 V", SEN, "mb", 28)
lab(1820, (NY2 + PY(17)) / 2 + 30, "J3 pin 18", SEN, "lm", 28)
d.multiline_text((1440, 1500), "charger plugged in  ->  3.0 V on GPIO 21\nno charger  ->  0 V", font=RAJ(26), fill=MUT, spacing=8)

# footer
d.text((60, H - 30), "Check with a meter before plugging the ESP32 in: GPIO 2 = about half the battery, GPIO 21 = 0 V / 3.0 V.  Never above 3.3 V on either.",
       font=RAJ(28), fill=RED, anchor="ls")
im.save("NeuroX_Box_VBAT_VUSB_sense.png"); print(im.size)
