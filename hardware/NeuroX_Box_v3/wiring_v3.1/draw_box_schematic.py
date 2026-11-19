"""NeuroX Box v3.1 - ESP32-S3 DevKitC-1 wiring schematic (same style as the HALO C3 sheet)."""
from PIL import Image, ImageDraw, ImageFont
SP = "/tmp/claude-0/-home-claude/31d76bfd-844e-5c8a-8c3a-6c0a47579976/scratchpad"
RAJ = lambda s: ImageFont.truetype(f"{SP}/box3/rajdhani700.ttf", s)
ORB = lambda s: ImageFont.truetype(f"{SP}/box3/orbitron900.ttf", s)
BG = (246, 247, 249); INK = (22, 26, 34); MUT = (110, 116, 128)
C = dict(v5=(210, 30, 40), bat=(230, 120, 20), gnd=(20, 20, 20), v33=(20, 90, 220), scr=(225, 115, 10), btn=(30, 150, 70),
         mpu=(140, 60, 200), bz=(190, 150, 0), sense=(0, 150, 160), off=(165, 170, 180))
NETCOL = {"3V3": C["v33"], "GND": C["gnd"], "+5V": C["v5"], "VBAT": C["bat"], "VUSB": C["bat"], "VBAT/2": C["sense"], "USB_SENSE": C["sense"]}
W, H = 2600, 2900
sh = Image.new("RGB", (W, H), BG); d = ImageDraw.Draw(sh)
d.text((60, 44), "BOX  S3 BOARD  WIRING", font=ORB(60), fill=INK)
d.text((64, 128), "ESP32-S3 DevKitC-1 + screen + 4 buttons + buzzer + MPU-6050 + power, on the 68 x 50 mm perfboard in the box.", font=RAJ(36), fill=MUT)
d.text((64, 172), "Same tag name = same wire (3V3, GND, +5V, VBAT ...). Pin numbers in ( ) = the module's own header pin.", font=RAJ(36), fill=MUT)

def L(pts, col, w=6): d.line(pts, fill=col, width=w, joint="curve")
def dot(x, y, col): d.ellipse([x - 9, y - 9, x + 9, y + 9], fill=col)
def tag(x, y, net, side="r", size=28):
    col = NETCOL.get(net, C["gnd"]); f = RAJ(size); tw = d.textlength(net, font=f) + 22
    x0 = x if side == "r" else x - tw
    if side == "r": pts = [(x0, y - 17), (x0 + tw, y - 17), (x0 + tw + 13, y), (x0 + tw, y + 17), (x0, y + 17)]
    else: pts = [(x0 - 13, y), (x0, y - 17), (x0 + tw, y - 17), (x0 + tw, y + 17), (x0, y + 17)]
    d.polygon(pts, fill=col); d.text((x0 + tw / 2, y), net, font=f, fill="white", anchor="mm")
def block(x0, y0, x1, y1, title, sub=None, fill=(255, 255, 255), out=(200, 204, 212), tcol=INK):
    d.rounded_rectangle([x0, y0, x1, y1], 16, fill=fill, outline=out, width=3)
    d.text(((x0 + x1) / 2, y0 + 14), title, font=RAJ(32), fill=tcol, anchor="mt")
    if sub: d.multiline_text(((x0 + x1) / 2, y0 + 50), sub, font=RAJ(24), fill=MUT, anchor="ma", align="center")
def resv(x, y0, y1, label, col, right=True):
    L([(x, y0), (x, y1)], col, 5); m = (y0 + y1) / 2
    d.rectangle([x - 13, m - 30, x + 13, m + 30], fill=(250, 245, 230), outline=col, width=3)
    d.text((x + (22 if right else -22), m), label, font=RAJ(26), fill=INK, anchor="lm" if right else "rm")
def capv(x, y, label, col=C["gnd"], right=True):     # plates centred on y
    d.rectangle([x - 24, y - 12, x + 24, y - 5], fill=col); d.rectangle([x - 24, y + 5, x + 24, y + 12], fill=col)
    d.text((x + (32 if right else -32), y), label, font=RAJ(24), fill=INK, anchor="lm" if right else "rm")
def lab(x, y, t, col=INK, a="lm", s=26): d.text((x, y), t, font=RAJ(s), fill=col, anchor=a)

# ======================= POWER (top band) =======================
PR = 480
block(50, PR - 90, 230, PR + 150, "LiPo", "103450\n1S 3.7 V", fill=(255, 244, 230), out=C["bat"])
block(320, PR - 110, 590, PR + 150, "TP4056", "USB-C in the\nCHARGE slot")
L([(230, PR), (320, PR)], C["bat"]); lab(240, PR - 20, "+", C["bat"])
L([(230, PR + 90), (320, PR + 90)], C["gnd"]); lab(240, PR + 112, "-", C["gnd"])
lab(332, PR, "B+"); lab(332, PR + 90, "B-"); lab(578, PR, "OUT+", a="rm"); lab(578, PR + 90, "OUT-", a="rm")
# USB 5V pad -> VUSB tag (top of the TP4056)
L([(455, PR - 110), (455, PR - 150)], C["bat"], 5); lab(468, PR - 128, "IN+ (USB 5 V pad)", MUT, s=22); tag(455 - 20, PR - 172, "VUSB", "r", 24)
L([(590, PR + 90), (640, PR + 90)], C["gnd"]); tag(640, PR + 90, "GND")
# rocker
L([(590, PR), (660, PR)], C["bat"]); d.ellipse([652, PR - 8, 668, PR + 8], outline=C["bat"], width=4)
L([(666, PR - 4), (744, PR - 38)], C["bat"]); d.ellipse([742, PR - 8, 758, PR + 8], outline=C["bat"], width=4)
lab(700, PR - 52, "ROCKER", a="mb", s=24)
L([(758, PR), (860, PR)], C["bat"]); dot(810, PR, C["bat"])
L([(810, PR), (810, PR - 70)], C["bat"], 5); tag(810 - 20, PR - 92, "VBAT", "r", 24)
# no boost: rocker -> D1 -> 5V pin (the S3's own regulator makes 3.3 V)
L([(860, PR), (1290, PR)], C["bat"]); dot(1190, PR, C["bat"])
lab(1000, PR - 30, "no boost converter", MUT, "mb", 22)
L([(1190, PR), (1190, PR + 30)], C["bat"], 5)
capv(1190, PR + 45, "", C["bat"]); L([(1190, PR + 57), (1190, PR + 130)], C["gnd"], 5)
tag(1190 - 20, PR + 152, "GND", "r", 24)
lab(1222, PR + 45, "100 uF", s=24); lab(1222, PR + 70, "+ to VBAT", MUT, s=20)
# D1
d.polygon([(1290, PR - 22), (1290, PR + 22), (1334, PR)], fill=C["v5"]); L([(1336, PR - 22), (1336, PR + 22)], C["v5"], 7)
lab(1312, PR - 34, "D1 1N5819  (band -> ESP)", C["v5"], "mb", 24)
L([(1336, PR), (1400, PR)], C["v5"]); tag(1400, PR, "+5V")

# ---- the two dividers (top right) ----
DX1, DX2 = 1760, 2160
for x, top, mid, bot, rt, rb, out, cap in ((DX1, "VBAT", "VBAT/2", "GND", "R1 100k", "R2 100k", "to GPIO 2", True),
                                             (DX2, "VUSB", "USB_SENSE", "GND", "R3 100k", "R4 150k", "to GPIO 21", False)):
    tag(x - 20, PR - 150, top, "r", 24); L([(x, PR - 132), (x, PR - 110)], NETCOL[top], 5)
    resv(x, PR - 110, PR + 20, rt, C["sense"], right=False)
    L([(x, PR + 20), (x, PR + 50)], C["sense"], 5); dot(x, PR + 50, C["sense"])
    L([(x, PR + 50), (x + 70, PR + 50)], C["sense"], 5); tag(x + 70, PR + 50, mid, "r", 24)
    resv(x, PR + 50, PR + 180, rb, C["gnd"], right=False); L([(x, PR + 180), (x, PR + 200)], C["gnd"], 5); tag(x - 20, PR + 222, "GND", "r", 22)
    if cap:
        L([(x + 40, PR + 50), (x + 40, PR + 100)], C["sense"], 5); capv(x + 40, PR + 112, "C1 100nF")
        L([(x + 40, PR + 124), (x + 40, PR + 185), (x, PR + 185)], C["gnd"], 5); dot(x, PR + 185, C["gnd"])
lab(DX1 - 60, PR - 200, "BATTERY SENSE  4.2 V -> 2.1 V", C["sense"], "lm", 24)
lab(DX2 - 60, PR - 200, "USB SENSE  5 V -> 3.0 V", C["sense"], "lm", 24)
L([(40, PR + 280), (W - 40, PR + 280)], (215, 218, 224), 2)

# ======================= ESP32-S3 DevKitC-1 =======================
J1 = ["3V3", "3V3", "RST", "4", "5", "6", "7", "15", "16", "17", "18", "8", "3", "46", "9", "10", "11", "12", "13", "14", "5V", "GND"]
J3 = ["GND", "TX", "RX", "1", "2", "42", "41", "40", "39", "38", "37", "36", "35", "0", "45", "48", "47", "21", "20", "19", "GND", "GND"]
DK0, DK1, DY0, PP = 1180, 1520, 880, 62
PY = lambda i: DY0 + 100 + i * PP
dy1 = PY(21) + 70
block(DK0, DY0, DK1, dy1, "ESP32-S3", "DevKitC-1  (seen from the top)", fill=(36, 40, 52), out=(36, 40, 52), tcol="white")
d.text(((DK0 + DK1) / 2, DY0 - 12), "antenna end", font=RAJ(24), fill=MUT, anchor="mb")
d.rounded_rectangle([(DK0 + DK1) / 2 - 90, dy1 - 6, (DK0 + DK1) / 2 - 20, dy1 + 26], 6, fill=(180, 184, 192))
d.rounded_rectangle([(DK0 + DK1) / 2 + 20, dy1 - 6, (DK0 + DK1) / 2 + 90, dy1 + 26], 6, fill=(180, 184, 192))
d.text(((DK0 + DK1) / 2, dy1 + 34), "2 x USB-C  (to the USB slot)", font=RAJ(24), fill=MUT, anchor="mt")
d.text((DK0, DY0 - 12), "J1", font=RAJ(30), fill=INK, anchor="mb"); d.text((DK1, DY0 - 12), "J3", font=RAJ(30), fill=INK, anchor="mb")
U1 = {"3V3": C["v33"], "5": C["btn"], "6": C["btn"], "7": C["btn"], "15": C["btn"], "16": C["scr"], "17": C["scr"], "18": C["scr"], "8": C["scr"],
      "9": C["scr"], "10": C["scr"], "11": C["scr"], "12": C["scr"], "13": C["bz"], "14": C["scr"], "5V": C["v5"], "GND": C["gnd"]}
U3 = {"GND": C["gnd"], "2": C["sense"], "42": C["mpu"], "41": C["mpu"], "21": C["sense"]}
def pname(n): return n if n in ("3V3", "5V", "GND", "RST", "TX", "RX") else f"GPIO {n}"
for i, n in enumerate(J1):
    y = PY(i); col = U1.get(n, C["off"]) if not (i == 1) else C["off"]
    d.rectangle([DK0 - 12, y - 12, DK0 + 12, y + 12], fill=col)
    d.text((DK0 + 22, y), f"{i + 1}  {pname(n)}", font=RAJ(26), fill="white" if col != C["off"] else (150, 156, 168), anchor="lm")
for i, n in enumerate(J3):
    y = PY(i); col = U3.get(n, C["off"])
    d.rectangle([DK1 - 12, y - 12, DK1 + 12, y + 12], fill=col)
    d.text((DK1 - 22, y), f"{pname(n)}  {i + 1}", font=RAJ(26), fill="white" if col != C["off"] else (150, 156, 168), anchor="rm")

# ---- J1 power pins ----
L([(DK0 - 12, PY(0)), (DK0 - 60, PY(0))], C["v33"]); tag(DK0 - 60, PY(0), "3V3", "l")
lab(DK0 - 175, PY(0), "3V3 rail source", MUT, "rm", 22)
L([(DK0 - 12, PY(20)), (DK0 - 60, PY(20))], C["v5"]); tag(DK0 - 60, PY(20), "+5V", "l")
L([(DK0 - 12, PY(21)), (DK0 - 60, PY(21))], C["gnd"]); tag(DK0 - 60, PY(21), "GND", "l")

# ---- buttons (J1 5..8) ----
SWX = 900
for i, name in ((4, "OK  (RIGHT-OK)"), (5, "LEFT"), (6, "NEW"), (7, "STOP  (red)")):
    y = PY(i)
    L([(DK0 - 12, y), (SWX + 40, y)], C["btn"]); d.ellipse([SWX + 32, y - 8, SWX + 48, y + 8], outline=C["btn"], width=4)
    L([(SWX + 34, y - 4), (SWX - 26, y - 30)], C["btn"]); d.ellipse([SWX - 48, y - 8, SWX - 32, y + 8], outline=C["btn"], width=4)
    L([(SWX - 48, y), (SWX - 100, y)], C["gnd"]); tag(SWX - 100, y, "GND", "l", 24)
    lab(SWX + 70, y - 16, name, C["btn"], "lm", 24)
lab(SWX - 220, PY(4) - 44, "BUTTONS  (12 x 12 switch, 2 DIAGONAL pins)", C["btn"], "lm", 26)

# ---- screen (J1 9..12, 15..18, 20) + buzzer (J1 19) ----
SX0, SX1 = 420, 860
srow = {8: "T_IRQ  (14)", 9: "T_CS  (11)", 10: "RESET  (4)", 11: "DC  (5)", 14: "CS  (3)", 15: "T_DO (13)   SDO open",
        16: "SDI (6) + T_DIN (12)", 17: "SCK (7) + T_CLK (10)", 19: "LED  (8)"}
d.rounded_rectangle([SX0, PY(8) - 36, SX1, PY(19) + 120], 16, fill=(255, 244, 232), outline=C["scr"], width=3)
d.text(((SX0 + SX1) / 2, PY(19) + 50), "SCREEN  2.8\" ST7789", font=RAJ(32), fill=INK, anchor="mt")
d.text(((SX0 + SX1) / 2, PY(19) + 86), "14-pin header, SPI + touch", font=RAJ(24), fill=MUT, anchor="mt")
for i, t in srow.items():
    y = PY(i); L([(SX1, y), (DK0 - 12, y)], C["scr"])
    d.text((SX1 - 14, y), t, font=RAJ(25), fill=INK, anchor="rm")
for y, pn, net in ((PY(8) - 150 + 98, "VCC (1)", "3V3"), (PY(8) - 150 + 98 + 0, None, None)):
    pass
# VCC / GND on the left side of the screen
for k, (pn, net) in enumerate((("VCC (1)", "3V3"), ("GND (2)", "GND"))):
    y = PY(12) + k * 50
    d.text((SX0 + 14, y), pn, font=RAJ(25), fill=INK, anchor="lm"); L([(SX0, y), (SX0 - 50, y)], NETCOL[net]); tag(SX0 - 50, y, net, "l")
d.multiline_text((SX0 + 14, PY(14) - 14), "join SDI+T_DIN\nand SCK+T_CLK\nat the header;\nleave SDO open", font=RAJ(22), fill=MUT, anchor="la")
# buzzer on row 19 (between SCK and LED, no screen pin there)
yb = PY(18)
L([(DK0 - 12, yb), (1080, yb)], C["bz"]); d.ellipse([1000, yb - 24, 1060, yb + 24], fill=(40, 40, 44)); d.text((1030, yb), "BZ", font=RAJ(22), fill="white", anchor="mm")
lab(1074, yb - 22, "+", C["bz"], "lm", 24); L([(1000, yb), (980, yb)], C["gnd"]); tag(980, yb, "GND", "l", 22)
lab(1030, yb - 30, "BUZZER", C["bz"], "mb", 22)

# ======================= J3 side =======================
R0 = DK1 + 12
L([(R0, PY(0)), (R0 + 50, PY(0))], C["gnd"]); tag(R0 + 50, PY(0), "GND")
L([(R0, PY(4)), (R0 + 50, PY(4))], C["sense"]); tag(R0 + 50, PY(4), "VBAT/2")
L([(R0, PY(17)), (R0 + 50, PY(17))], C["sense"]); tag(R0 + 50, PY(17), "USB_SENSE")
for i in (20, 21): L([(R0, PY(i)), (R0 + 50, PY(i))], C["gnd"]); tag(R0 + 50, PY(i), "GND")
# MPU-6050
MX0, MX1 = 1880, 2220
block(MX0, PY(5) - 110, MX1, PY(7) + 70, "MPU-6050", "GY-521 breakout", fill=(246, 238, 252), out=C["mpu"])
for i, t in ((5, "SCL  (3)"), (6, "SDA  (4)")):
    y = PY(i); L([(R0, y), (MX0, y)], C["mpu"]); d.text((MX0 + 14, y), t, font=RAJ(25), fill=INK, anchor="lm")
for k, (pn, net) in enumerate((("VCC (1)", "3V3"), ("GND (2)", "GND"))):
    y = PY(5) + k * 62
    d.text((MX1 - 14, y), pn, font=RAJ(25), fill=INK, anchor="rm"); L([(MX1, y), (MX1 + 50, y)], NETCOL[net]); tag(MX1 + 50, y, net)
d.text((MX1 - 14, PY(7)), "AD0, INT, XDA, XCL:", font=RAJ(22), fill=MUT, anchor="rm"); d.text((MX1 - 14, PY(7) + 28), "leave open", font=RAJ(22), fill=MUT, anchor="rm")
lab(R0 + 70, PY(10), "GPIO 1 + 4: free now", MUT, "lm", 24); lab(R0 + 70, PY(11), "(pills are in the HALO)", MUT, "lm", 24)

# ======================= notes =======================
ny = dy1 + 110
notes = [("Screen VCC on 3.3 V:", " safe whether or not the J1 jumper on the screen board is bridged."),
         ("LED (8) on GPIO 14:", " fine if your screen has a small 3-leg transistor by the LED pin. No transistor? LED to 3V3 instead."),
         ("Before plugging the ESP32 in:", " ~3.9 V after D1 on a full cell, 3V3 to GND not shorted, VBAT/2 about half the battery."),
         ("USB into the ESP32:", " rocker OFF first. The box has no wire to the person, so charging while the HALO is worn is fine.")]
for i, (a, b) in enumerate(notes):
    d.text((64, ny + i * 48), a, font=RAJ(31), fill=C["v5"] if i in (2, 3) else INK)
    d.text((64 + d.textlength(a, font=RAJ(31)), ny + i * 48), b, font=RAJ(31), fill=(80, 86, 98))
sh = sh.crop((0, 0, W, ny + len(notes) * 48 + 40))
sh.save("NeuroX_Box_S3_board_wiring.png"); print(sh.size)
