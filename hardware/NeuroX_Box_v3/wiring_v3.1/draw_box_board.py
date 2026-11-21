"""NeuroX Box v3.1 - main perfboard map (ESP32-S3 DevKitC-1). Top view, component side."""
from PIL import Image, ImageDraw, ImageFont
SP = "/tmp/claude-0/-home-claude/31d76bfd-844e-5c8a-8c3a-6c0a47579976/scratchpad"
RAJ = lambda s: ImageFont.truetype(f"{SP}/box3/rajdhani700.ttf", s)
ORB = lambda s: ImageFont.truetype(f"{SP}/box3/orbitron900.ttf", s)
MONO = lambda s: ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf", s)

BG = (246, 247, 249); INK = (22, 26, 34); GREY = (150, 156, 168); BOARD = (214, 190, 140); HOLE = (170, 146, 100)
C = dict(scr=(230, 120, 20), btn=(30, 150, 70), mpu=(140, 60, 200), bz=(200, 160, 0), v5=(210, 30, 40),
         gnd=(20, 20, 20), v33=(20, 90, 220), sense=(0, 150, 160), off=(165, 170, 180))
P = 2.54; S = 26                      # mm pitch, px per mm
BW, BH = 68.0, 50.0
W, H = 2800, 2400
OX, OY = 250, 300        # board top-left in px
def px(x, y): return OX + x * S, OY + (BH - y) * S      # perfboard mm (y = 0 front edge) -> px

sh = Image.new("RGB", (W, H), BG); d = ImageDraw.Draw(sh)
d.text((60, 44), "BOX MAIN BOARD  WIRING MAP", font=ORB(60), fill=INK)
d.text((64, 128), "Perfboard 68 x 50 mm, top view (parts side). ESP32-S3 DevKitC-1 on 2 female headers, USB end at the right wall.",
       font=RAJ(36), fill=(110, 116, 128))
d.text((64, 172), "Positions are a suggestion - the connections are what matter. Same colour + same name = one wire.",
       font=RAJ(36), fill=(110, 116, 128))

# board + holes
x0, y0 = px(0, BH); x1, y1 = px(BW, 0)
d.rounded_rectangle([x0, y0, x1, y1], 18, fill=BOARD, outline=(150, 125, 80), width=4)
for i in range(1, 27):
    for j in range(1, 20):
        cx, cy = px(i * P, j * P); d.ellipse([cx - 3, cy - 3, cx + 3, cy + 3], fill=HOLE)

def vtext(x, y, txt, font, fill, up=True):
    """vertical text; (x, y) = the end nearest the pin"""
    tw = int(d.textlength(txt, font=font)) + 6; th = font.size + 8
    im = Image.new("RGBA", (tw, th), (0, 0, 0, 0)); ImageDraw.Draw(im).text((3, 2), txt, font=font, fill=fill)
    im = im.rotate(90 if up else -90, expand=True)
    if up: sh.paste(im, (int(x - im.width / 2), int(y - im.height)), im)
    else: sh.paste(im, (int(x - im.width / 2), int(y)), im)

# ---- DevKit outline (dashed) + headers ----
J1 = ["3V3", "3V3", "RST", "4", "5", "6", "7", "15", "16", "17", "18", "8", "3", "46", "9", "10", "11", "12", "13", "14", "5V", "GND"]
J3 = ["GND", "TX", "RX", "1", "2", "42", "41", "40", "39", "38", "37", "36", "35", "0", "45", "48", "47", "21", "20", "19", "GND", "GND"]
D1 = {"3V3": ("3V3 rail", "v33"), "5": ("OK btn", "btn"), "6": ("LEFT btn", "btn"), "7": ("NEW btn", "btn"), "15": ("STOP btn", "btn"),
      "16": ("T_IRQ", "scr"), "17": ("T_CS", "scr"), "18": ("RESET", "scr"), "8": ("DC", "scr"), "9": ("CS", "scr"),
      "10": ("SDO+T_DO", "scr"), "11": ("SDI+T_DIN", "scr"), "12": ("SCK+T_CLK", "scr"), "13": ("BUZZER +", "bz"),
      "14": ("LED", "scr"), "5V": ("from D1", "v5"), "GND": ("GND rail", "gnd")}
D3 = {"GND": ("GND rail", "gnd"), "2": ("VBAT sense", "sense"), "42": ("MPU SCL", "mpu"), "41": ("MPU SDA", "mpu"), "21": ("USB sense", "sense")}
ROW_J1, ROW_J3 = 2 * P, 11 * P
COL0 = 2
kx0, ky0 = px(COL0 * P - 4, ROW_J3 + 2.5); kx1, ky1 = px(COL0 * P + 21 * P + 7.5, ROW_J1 - 2.5)
for xx in range(int(kx0), int(kx1), 28): d.line([xx, ky0, min(xx + 14, kx1), ky0], fill=(80, 80, 90), width=3); d.line([xx, ky1, min(xx + 14, kx1), ky1], fill=(80, 80, 90), width=3)
for yy in range(int(ky0), int(ky1), 28): d.line([kx0, yy, kx0, min(yy + 14, ky1)], fill=(80, 80, 90), width=3); d.line([kx1, yy, kx1, min(yy + 14, ky1)], fill=(80, 80, 90), width=3)
ax, ay = px(COL0 * P + 2, (ROW_J1 + ROW_J3) / 2)
d.text((ax, ay), "ANTENNA end", font=RAJ(30), fill=(80, 80, 90), anchor="lm")
ux, uy = px(COL0 * P + 21 * P + 7, (ROW_J1 + ROW_J3) / 2)
d.text((ux, uy), "USB end", font=RAJ(34), fill=(80, 80, 90), anchor="rm")
d.text(px(COL0 * P + 10.5 * P, (ROW_J1 + ROW_J3) / 2), "ESP32-S3 DevKitC-1\n(sits on top; wires + resistors may run under it)",
       font=RAJ(30), fill=(80, 80, 90), anchor="mm", align="center")

def header_row(names, dests, row, up):
    for k, n in enumerate(names):
        cx, cy = px((COL0 + k) * P, row)
        dest = dests.get(n)
        col = C[dest[1]] if dest else C["off"]
        d.rectangle([cx - 11, cy - 11, cx + 11, cy + 11], fill=(30, 30, 34))
        d.ellipse([cx - 7, cy - 7, cx + 7, cy + 7], fill=col)
        lab = n if not dest else f"{n}  {dest[0]}"
        if n == "3V3" and k == 1 and row == ROW_J1: lab = "3V3"; col = C["off"]
        f = MONO(19) if dest else MONO(17)
        vtext(cx, cy - 16 if up else cy + 16, lab, f, col if dest else (120, 124, 132), up)
        # pin number outside the board edge side
        nx, ny = cx, (cy + 16 if up else cy - 16)
        d.text((nx, ny + (6 if up else -6)), str(k + 1), font=RAJ(20), fill=(90, 70, 40), anchor="mt" if up else "mb")
header_row(J1, D1, ROW_J1, up=True)
header_row(J3, D3, ROW_J3, up=False)
d.text(px(0.6, ROW_J1 - 1.6), "J1", font=RAJ(34), fill=INK, anchor="lm")
d.text(px(0.6, ROW_J3 + 1.6), "J3", font=RAJ(34), fill=INK, anchor="lm")

# ---- rails ----
RAIL33, RAILG = 13 * P, 14 * P
for y, c, t in ((RAIL33, C["v33"], "3V3 rail"), (RAILG, C["gnd"], "GND rail")):
    a = px(1 * P, y); b = px(26 * P, y); d.line([a, b], fill=c, width=8)
    d.text((px(0, 0)[0] - 16, a[1]), t, font=RAJ(30), fill=c, anchor="rm")

# ---- connector headers (male pins), in the module's own pin order so a ribbon goes straight across ----
ROW_A, ROW_B = 18 * P, 16 * P
def conn(names, cols, row, colmap, tie):
    for n, cc in zip(names, cols):
        cx, cy = px(cc * P, row); col = C[colmap.get(n.strip(), "off")]
        if n in tie:                                            # short link down to its rail
            ry = px(0, RAIL33 if tie[n] == "v33" else RAILG)[1]
            d.line([cx, cy + 11, cx, ry], fill=C[tie[n]], width=5); d.ellipse([cx - 7, ry - 7, cx + 7, ry + 7], fill=C[tie[n]])
        d.rectangle([cx - 11, cy - 11, cx + 11, cy + 11], fill=(250, 250, 250), outline=col, width=4)
        d.ellipse([cx - 5, cy - 5, cx + 5, cy + 5], fill=col)
        vtext(cx, cy - 16, n.strip(), MONO(18), col)

scr = ["VCC", "GND", "CS", "RESET", "DC", "SDI", "SCK", "LED", "SDO", "T_CLK", "T_CS", "T_DIN", "T_DO", "T_IRQ"]
conn(scr, list(range(2, 16)), ROW_A, {n: "scr" for n in scr} | {"VCC": "v33", "GND": "gnd"}, {"VCC": "v33", "GND": "gnd"})
conn(["5V", "GND", "VBAT", "VUSB"], [18, 19, 20, 21], ROW_A, {"5V": "v5", "GND": "gnd", "VBAT": "sense", "VUSB": "sense"}, {"GND": "gnd"})
btn = ["STOP", "G", "NEW", "G ", "LEFT", "G  ", "OK", "G   "]
conn(btn, list(range(2, 10)), ROW_B, {"STOP": "btn", "NEW": "btn", "LEFT": "btn", "OK": "btn", "G": "gnd"}, {g: "gnd" for g in btn if g.strip() == "G"})
conn(["BZ+", "BZ-"], [11, 12], ROW_B, {"BZ+": "bz", "BZ-": "gnd"}, {"BZ-": "gnd"})
conn(["VCC ", "GND ", "SCL", "SDA"], [14, 15, 16, 17], ROW_B, {"VCC": "v33", "GND": "gnd", "SCL": "mpu", "SDA": "mpu"}, {"VCC ": "v33", "GND ": "gnd"})

# group labels above the board edge / beside row B
def over(txt, c0, c1, col):
    a = px(c0 * P - 1, BH); b = px(c1 * P + 1, BH)
    d.line([a[0], a[1] - 14, b[0], b[1] - 14], fill=col, width=5); d.text(((a[0] + b[0]) / 2, a[1] - 22), txt, font=RAJ(30), fill=col, anchor="mb")
over("SCREEN  (14 pins, same order as on the screen)", 2, 15, C["scr"])
over("POWER IN", 18, 21, C["v5"])
for txt, c0, c1, col in (("BUTTONS", 2, 9, C["btn"]), ("BUZZER", 11, 12, C["bz"]), ("MPU-6050", 14, 17, C["mpu"])):
    a = px(c0 * P - 1, ROW_B - 1.3); b = px(c1 * P + 1, ROW_B - 1.3)
    d.text(((a[0] + b[0]) / 2, a[1] + 2), txt, font=RAJ(24), fill=col, anchor="mt")

# ---- D1 + the two dividers (right end of the back strip) ----
def res(x, ya, yb, label, col, right=True):
    a = px(x, ya); b = px(x, yb); d.line([a, b], fill=col, width=5)
    my = (a[1] + b[1]) / 2; d.rectangle([a[0] - 10, my - 26, a[0] + 10, my + 26], fill=(250, 245, 230), outline=col, width=3)
    d.text((a[0] + (16 if right else -16), my), label, font=RAJ(23), fill=INK, anchor="lm" if right else "rm")
def dot(x, y, col):
    q = px(x, y); d.ellipse([q[0] - 8, q[1] - 8, q[0] + 8, q[1] + 8], fill=col)
# D1: POWER IN 5V -> 1N5819 (band away from the header) -> wire to J1 pin 21
res(18 * P, ROW_A - 0.4, 16.2 * P, "", C["v5"])
q = px(18 * P, 16.9 * P); d.rectangle([q[0] - 10, q[1] + 18, q[0] + 10, q[1] + 24], fill=(60, 60, 60))   # cathode band
d.text((px(18 * P, 0)[0] - 16, px(0, 17.2 * P)[1]), "D1", font=RAJ(23), fill=C["v5"], anchor="rm")
d.text((px(BW, 0)[0] + 18, px(0, 18.2 * P)[1]), "D1 1N5819 (band away from 5V pin) -> wire to J1 pin 21 (5V)", font=RAJ(26), fill=C["v5"], anchor="lm")
# VBAT: R1 100k to node, R2 100k + C1 100nF node -> GND rail, node -> J3 pin 5 (GPIO 2)
for seg in ((px(20 * P, ROW_A - 0.4), px(20 * P, 17 * P)), (px(20 * P, 17 * P), px(23 * P, 17 * P))): d.line(seg, fill=C["sense"], width=5)
res(23 * P, 17 * P, 15.4 * P, "R1 100k", C["sense"], right=False)
res(23 * P, 15.4 * P, RAILG, "R2 100k", C["gnd"], right=False)
d.line([px(23 * P, 15.4 * P), px(24 * P, 15.4 * P)], fill=C["sense"], width=5)
res(24 * P, 15.4 * P, RAILG, "", C["gnd"]); dot(24 * P, RAILG, C["gnd"]); dot(23 * P, RAILG, C["gnd"])
dot(23 * P, 15.4 * P, C["sense"])
# VUSB: R3 100k to node, R4 150k node -> GND rail, node -> J3 pin 18 (GPIO 21)
for seg in ((px(21 * P, ROW_A - 0.4), px(21 * P, 17.5 * P)), (px(21 * P, 17.5 * P), px(26 * P, 17.5 * P))): d.line(seg, fill=C["sense"], width=5)
res(26 * P, 17.5 * P, 15.4 * P, "", C["sense"]); res(26 * P, 15.4 * P, RAILG, "", C["gnd"])
dot(26 * P, 15.4 * P, C["sense"]); dot(26 * P, RAILG, C["gnd"])
for yy, t in ((16.45, "R3"), (14.7, "R4")): d.text((px(26 * P, 0)[0] + 14, px(0, yy * P)[1]), t, font=RAJ(23), fill=INK, anchor="lm")
# labels for C1, R3, R4 + node destinations, outside the board's right edge
rx = px(BW, 0)[0] + 18
for y, t, col in ((17.0, "R3 100k  (VUSB -> node)", INK), (15.4, "node -> J3 pin 18 (GPIO 21)", C["sense"]), (14.6, "R4 150k  (node -> GND)", INK)):
    d.text((rx, px(0, y * P)[1]), t, font=RAJ(26), fill=col, anchor="lm")
for y, t, col in ((16.25, "C1 100nF -> GND", INK), (15.4, "node -> J3 pin 5 (GPIO 2)", C["sense"])):
    pass
d.text((px(24 * P, 0)[0] + 14, px(0, 14.9 * P)[1]), "C1", font=RAJ(23), fill=INK, anchor="lm")
d.text((px(23 * P, 0)[0] + 40, px(0, 16.15 * P)[1]), "", font=RAJ(23), fill=INK)
lx2 = px(BW, 0)[0] + 18
d.text((lx2, px(0, 12.2 * P)[1]), "R1/R2 node -> J3 pin 5 (GPIO 2)", font=RAJ(26), fill=C["sense"], anchor="lm")
d.text((lx2, px(0, 11.3 * P)[1]), "C1 100nF: node -> GND", font=RAJ(26), fill=INK, anchor="lm")

# ---- legend ----
ly = OY + BH * S + 90
lx = 70
for k, t in (("v5", "5V (boost, after D1)"), ("v33", "3.3 V"), ("gnd", "GND"), ("scr", "screen"), ("btn", "buttons"),
             ("mpu", "MPU-6050"), ("bz", "buzzer"), ("sense", "battery / USB sense")):
    d.ellipse([lx, ly - 13, lx + 26, ly + 13], fill=C[k]); d.text((lx + 36, ly), t, font=RAJ(32), fill=INK, anchor="lm")
    lx += 70 + int(d.textlength(t, font=RAJ(32)))
notes = ["Off-board wires into the 5V / GND / VBAT / VUSB pins:  5V = MT3608 OUT+ (5.1 V),  GND = TP4056 OUT- / MT3608 IN-,  VBAT = rocker output (also MT3608 IN+),  VUSB = TP4056 IN+ (USB 5V).",
         "Lines that cross WITHOUT a dot are not connected. Grey pins are not used. GPIO 1 and 4 are free now - the pills moved to the HALO.  Do not use 0, 3, 45, 46 (boot pins), 19 / 20 (USB), 35-37 (PSRAM)."]
for i, n in enumerate(notes): d.text((70, ly + 52 + i * 44), n, font=RAJ(30), fill=(80, 86, 98))
sh = sh.crop((0, 0, W, ly + 52 + len(notes) * 44 + 40))
sh.save("NeuroX_Box_main_board_wiring.png"); print(sh.size)
