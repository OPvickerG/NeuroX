"""NeuroX HALO - ESP32-C3 SuperMini + 2 BioAmp EXG Pills + battery. Wiring schematic."""
from PIL import Image, ImageDraw, ImageFont
SP = "/tmp/claude-0/-home-claude/31d76bfd-844e-5c8a-8c3a-6c0a47579976/scratchpad"
RAJ = lambda s: ImageFont.truetype(f"{SP}/box3/rajdhani700.ttf", s)
ORB = lambda s: ImageFont.truetype(f"{SP}/box3/orbitron900.ttf", s)
MONO = lambda s: ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf", s)
BG = (246, 247, 249); INK = (22, 26, 34); MUT = (110, 116, 128)
C = dict(v5=(210, 30, 40), bat=(230, 120, 20), gnd=(20, 20, 20), v33=(20, 90, 220), sig=(140, 60, 200), sense=(0, 150, 160), off=(165, 170, 180), el=(30, 150, 70))
W, H = 2400, 1760
sh = Image.new("RGB", (W, H), BG); d = ImageDraw.Draw(sh)
d.text((60, 44), "HALO  C3 BOARD  WIRING", font=ORB(60), fill=INK)
d.text((64, 128), "ESP32-C3 SuperMini + both BioAmp EXG Pills + its own LiPo, on a small perfboard in the HALO.", font=RAJ(36), fill=MUT)
d.text((64, 172), "3V3 / GND tags = connect to that rail. Go by the labels printed on your C3 board, not by pin position.", font=RAJ(36), fill=MUT)

def L(pts, col, w=6): d.line(pts, fill=col, width=w, joint="curve")
def dot(x, y, col): d.ellipse([x - 9, y - 9, x + 9, y + 9], fill=col)
def tag(x, y, net, side="r"):           # net flag at the end of a stub
    col = C["v33"] if net == "3V3" else C["sense"] if net.startswith("VBAT") else C["gnd"]
    f = RAJ(30); tw = d.textlength(net, font=f) + 24
    x0 = x if side == "r" else x - tw
    d.polygon([(x0, y - 18), (x0 + tw, y - 18), (x0 + tw + (14 if side == "r" else 0), y), (x0 + tw, y + 18), (x0, y + 18), (x0 - (0 if side == "r" else 14), y)], fill=col)
    d.text((x0 + tw / 2, y), net, font=f, fill="white", anchor="mm")
def block(x0, y0, x1, y1, title, sub=None, fill=(255, 255, 255), out=(200, 204, 212)):
    d.rounded_rectangle([x0, y0, x1, y1], 16, fill=fill, outline=out, width=3)
    d.text(((x0 + x1) / 2, y0 + 16), title, font=RAJ(34), fill=INK, anchor="mt")
    if sub: d.multiline_text(((x0 + x1) / 2, y0 + 54), sub, font=RAJ(26), fill=MUT, anchor="ma", align="center")
def res(x, y0, y1, label, col):
    L([(x, y0), (x, y1)], col, 5); m = (y0 + y1) / 2
    d.rectangle([x - 14, m - 34, x + 14, m + 34], fill=(250, 245, 230), outline=col, width=3)
    d.text((x - 24, m), label, font=RAJ(26), fill=INK, anchor="rm")
def pinlab(x, y, txt, col, side):
    d.text((x + (14 if side == "r" else -14), y), txt, font=RAJ(28), fill=col, anchor="lm" if side == "r" else "rm")

# ---------------- C3 SuperMini ----------------
CX0, CX1, CY0 = 1180, 1500, 470
LEFT = ["5V", "G", "3.3", "4", "3", "2", "1", "0"]; RIGHT = ["5", "6", "7", "8", "9", "10", "20", "21"]
PY = lambda i: CY0 + 110 + i * 78
block(CX0, CY0, CX1, CY0 + 110 + 8 * 78, "ESP32-C3", "SuperMini  (USB-C up)", fill=(36, 40, 52), out=(36, 40, 52))
d.text(((CX0 + CX1) / 2, CY0 + 16), "ESP32-C3", font=RAJ(34), fill="white", anchor="mt")
d.rounded_rectangle([(CX0 + CX1) / 2 - 50, CY0 - 30, (CX0 + CX1) / 2 + 50, CY0 + 6], 8, fill=(180, 184, 192))
d.text(((CX0 + CX1) / 2, CY0 - 40), "USB-C (programming)", font=RAJ(26), fill=MUT, anchor="mb")
used = {"5V": C["v5"], "G": C["gnd"], "3.3": C["v33"], "4": C["sig"], "3": C["sense"], "1": C["sig"]}
NAME = {"5V": "5V", "G": "GND", "3.3": "3.3V"}
for i, n in enumerate(LEFT):
    y = PY(i); col = used.get(n, C["off"])
    d.rectangle([CX0 - 12, y - 12, CX0 + 12, y + 12], fill=col); d.text((CX0 + 22, y), NAME.get(n, f"GPIO {n}"), font=RAJ(28), fill="white", anchor="lm")
for i, n in enumerate(RIGHT):
    y = PY(i); d.rectangle([CX1 - 12, y - 12, CX1 + 12, y + 12], fill=C["off"]); d.text((CX1 - 22, y), f"GPIO {n}", font=RAJ(28), fill=(150, 156, 168), anchor="rm")
d.multiline_text((CX1 + 26, PY(2)), "right side: nothing\n(GPIO 8 = on-board LED,\n 9 = BOOT button)", font=RAJ(28), fill=MUT, anchor="la")
L([(CX0 - 12, PY(1)), (CX0 - 70, PY(1))], C["gnd"]); tag(CX0 - 70, PY(1), "GND", "l")
L([(CX0 - 12, PY(2)), (CX0 - 70, PY(2))], C["v33"]); tag(CX0 - 70, PY(2), "3V3", "l")
L([(CX0 - 12, PY(4)), (CX0 - 70, PY(4))], C["sense"]); tag(CX0 - 70, PY(4), "VBAT/2", "l")

# ---------------- power chain (top) ----------------
PR = 360
block(60, PR - 110, 250, PR + 130, "LiPo", "1S 3.7 V\n500-1000 mAh", fill=(255, 244, 230), out=C["bat"])
block(360, PR - 130, 640, PR + 130, "TP4056", "USB-C (charge)\nwith protection")
L([(250, PR), (360, PR)], C["bat"]); pinlab(250, PR - 22, "+", C["bat"], "r")
L([(250, PR + 80), (360, PR + 80)], C["gnd"]); pinlab(250, PR + 102, "-", C["gnd"], "r")
d.text((372, PR), "B+", font=RAJ(26), fill=INK, anchor="lm"); d.text((372, PR + 80), "B-", font=RAJ(26), fill=INK, anchor="lm")
d.text((628, PR), "OUT+", font=RAJ(26), fill=INK, anchor="rm"); d.text((628, PR + 80), "OUT-", font=RAJ(26), fill=INK, anchor="rm")
L([(640, PR + 80), (690, PR + 80)], C["gnd"]); tag(690, PR + 80, "GND")
# slide switch
L([(640, PR), (700, PR)], C["bat"]); d.ellipse([700 - 8, PR - 8, 700 + 8, PR + 8], outline=C["bat"], width=4)
L([(706, PR - 4), (790, PR - 40)], C["bat"], 6); d.ellipse([790 - 8, PR - 8, 790 + 8, PR + 8], outline=C["bat"], width=4)
d.text((745, PR - 56), "SW1 slide switch", font=RAJ(26), fill=INK, anchor="mb")
L([(798, PR), (900, PR)], C["bat"]); dot(860, PR, C["bat"])
# D2 -> 5V pin
L([(900, PR), (960, PR)], C["bat"]); d.polygon([(960, PR - 22), (960, PR + 22), (1004, PR)], fill=C["v5"]); L([(1006, PR - 22), (1006, PR + 22)], C["v5"], 7)
d.text((982, PR - 34), "D2 1N5819", font=RAJ(26), fill=C["v5"], anchor="mb"); d.text((982, PR + 34), "band towards C3", font=RAJ(22), fill=C["v5"], anchor="mt")
L([(1006, PR), (1100, PR), (1100, PY(0)), (CX0 - 12, PY(0))], C["v5"])
# battery divider (after SW1, before D2) -> VBAT/2 tag -> GPIO 3
DX = 860
res(DX, PR, PR + 170, "", C["sense"]); d.text((DX + 24, PR + 100), "R5 100k", font=RAJ(26), fill=INK, anchor="lm")
NY = PR + 200; L([(DX, PR + 170), (DX, NY)], C["sense"], 5); dot(DX, NY, C["sense"])
res(DX, NY, NY + 160, "R6 100k", C["gnd"]); L([(DX, NY + 160), (DX, NY + 190)], C["gnd"], 5); tag(DX - 46, NY + 214, "GND")
L([(DX, NY), (DX + 80, NY), (DX + 80, NY + 50)], C["sense"], 5)
d.rectangle([DX + 54, NY + 50, DX + 106, NY + 58], fill=C["gnd"]); d.rectangle([DX + 54, NY + 70, DX + 106, NY + 78], fill=C["gnd"])
L([(DX + 80, NY + 78), (DX + 80, NY + 120)], C["gnd"], 5); tag(DX + 80, NY + 144, "GND")
d.text((DX + 116, NY + 64), "C2 100nF", font=RAJ(24), fill=INK, anchor="lm")
L([(DX, NY), (DX - 70, NY)], C["sense"], 5); tag(DX - 70, NY, "VBAT/2", "l")

# ---------------- pills ----------------
def pill(y0, name, sub, out_pin_y, route_x, inputs):
    x0, x1 = 420, 760
    block(x0, y0, x1, y0 + 200, name, sub, fill=(244, 236, 250), out=C["sig"])
    for k, (pn, col, net) in enumerate((("VCC", C["v33"], "3V3"), ("GND", C["gnd"], "GND"))):
        y = y0 + 110 + k * 40
        d.text((x1 - 12, y), pn, font=RAJ(26), fill=INK, anchor="rm"); L([(x1, y), (x1 + 60, y)], col); tag(x1 + 60, y, net)
    yo = y0 + 190 - 10
    d.text((x1 - 12, yo), "OUT", font=RAJ(26), fill=INK, anchor="rm")
    L([(x1, yo), (route_x, yo), (route_x, out_pin_y), (CX0 - 12, out_pin_y)], C["sig"])
    for k, (pn, el) in enumerate(inputs):
        y = y0 + 100 + k * 40
        d.text((x0 + 12, y), pn, font=RAJ(26), fill=INK, anchor="lm"); L([(x0, y), (x0 - 60, y)], C["el"], 5)
        d.text((x0 - 70, y), el, font=RAJ(28), fill=C["el"], anchor="rm")
pill(1000, "PILL 1  HORIZONTAL", "temples", PY(3), 1120, [("IN+", "right temple"), ("IN-", "left temple"), ("REF", "REF behind right ear")])
pill(1250, "PILL 2  VERTICAL", "forehead / nose", PY(6), 1150, [("IN+", "forehead"), ("IN-", "above the nose bridge"), ("REF", "REF behind left ear")])

# ---------------- notes ----------------
ny = 1510
notes = [("Pills on 3.3 V ONLY.", " A pill on 5 V swings its OUT up to 5 V and kills the C3's ADC pin. Both VCC -> C3 3.3."),
         ("Next to each pill:", " 100 nF + 10 uF from VCC to GND, as close to the pill pins as you can."),
         ("Electrode leads:", " plug in exactly as they are now - only the pills moved. If your leads already work, don't swap anything."),
         ("Worn = battery only:", " SW1 on, NOTHING in either USB-C (C3 or TP4056). Charge and program with the HALO off the head.")]
for i, (a, b) in enumerate(notes):
    d.text((64, ny + i * 50), a, font=RAJ(32), fill=C["v5"] if i == 0 or i == 3 else INK)
    d.text((64 + d.textlength(a, font=RAJ(32)), ny + i * 50), b, font=RAJ(32), fill=(80, 86, 98))
sh.save(f"{SP}/wiring/NeuroX_HALO_C3_board_wiring.png"); print(sh.size)
