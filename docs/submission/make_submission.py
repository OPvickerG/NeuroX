"""Builds the 5 submission screenshots (2400 x 1350) from the renders and wiring sheets in this repo.
Run from the repo root:  python3 docs/submission/make_submission.py"""
import os
from PIL import Image, ImageDraw, ImageFont, ImageChops

FONTS = os.environ.get("NEUROX_FONTS", "/tmp/claude-0/-home-claude/31d76bfd-844e-5c8a-8c3a-6c0a47579976/scratchpad/box3")
ORB = lambda s: ImageFont.truetype(f"{FONTS}/orbitron900.ttf", s)
RAJ = lambda s: ImageFont.truetype(f"{FONTS}/rajdhani700.ttf", s)
W, H = 2400, 1350
BG = (246, 247, 249); INK = (22, 26, 34); MUT = (104, 110, 124); BLUE = (20, 90, 220); RED = (210, 30, 40)
LINE = (222, 225, 232)
BOX, HALO = "hardware/NeuroX_Box_v3", "hardware/NeuroX_HALO_v3"
OUT = "docs/submission"


def load(p): return Image.open(p).convert("RGB")


def trim(im, pad=20, tol=10):
    bg = Image.new("RGB", im.size, im.getpixel((2, 2)))
    diff = ImageChops.difference(im, bg).convert("L").point(lambda v: 255 if v > tol else 0)
    b = diff.getbbox()
    if not b: return im
    return im.crop((max(0, b[0] - pad), max(0, b[1] - pad), min(im.width, b[2] + pad), min(im.height, b[3] + pad)))


def light_only(im):
    """crop away a dark frame around a light-background render"""
    b = im.convert("L").point(lambda v: 255 if v > 235 else 0).getbbox()
    return im.crop((b[0] + 4, b[1] + 4, b[2] - 4, b[3] - 4))


def fit(im, w, h):
    r = min(w / im.width, h / im.height)
    return im.resize((max(1, int(im.width * r)), max(1, int(im.height * r))), Image.LANCZOS)


def canvas(title, sub, n):
    c = Image.new("RGB", (W, H), BG); d = ImageDraw.Draw(c)
    d.text((70, 52), title, font=ORB(58), fill=INK)
    d.text((72, 128), sub, font=RAJ(36), fill=MUT)
    d.text((W - 70, 64), f"{n} / 5", font=ORB(30), fill=BLUE, anchor="ra")
    d.text((W - 70, H - 34), "NeuroX v3.1  -  hands-free wheelchair control  -  Gurnihaal Singh", font=RAJ(26), fill=MUT, anchor="rs")
    return c, d


def card(c, d, box, im, cap=None, num=None, fill=(255, 255, 255), pad=24):
    x0, y0, x1, y1 = box
    d.rounded_rectangle(box, 22, fill=fill, outline=LINE if sum(fill) > 200 else fill, width=3)
    capH = 64 if cap else 0
    if im is not None:
        if im.getpixel((2, 2)) != fill:          # match the render's background to the card
            bg = Image.new("RGB", im.size, im.getpixel((2, 2)))
            mask = ImageChops.difference(im, bg).convert("L").point(lambda v: 255 if v > 6 else 0)
            base = Image.new("RGB", im.size, fill); base.paste(im, (0, 0), mask); im = base
        f = fit(im, x1 - x0 - 2 * pad, y1 - y0 - 2 * pad - capH)
        c.paste(f, (x0 + (x1 - x0 - f.width) // 2, y0 + pad + (y1 - y0 - 2 * pad - capH - f.height) // 2))
    if cap:
        cx = x0 + pad
        if num is not None:
            d.ellipse([cx, y1 - pad - 42, cx + 42, y1 - pad], fill=BLUE)
            d.text((cx + 21, y1 - pad - 21), str(num), font=RAJ(28), fill="white", anchor="mm"); cx += 56
        d.text((cx, y1 - pad - 21), cap, font=RAJ(31), fill="white" if sum(fill) < 200 else INK, anchor="lm")


def arrow(d, x0, y0, x1, y1, col=BLUE, w=8, dash=False):
    import math
    a = math.atan2(y1 - y0, x1 - x0); L = math.hypot(x1 - x0, y1 - y0)
    if dash:
        s = 0
        while s < L - 30:
            e = min(s + 26, L - 30)
            d.line([(x0 + math.cos(a) * s, y0 + math.sin(a) * s), (x0 + math.cos(a) * e, y0 + math.sin(a) * e)], fill=col, width=w)
            s += 44
    else:
        d.line([(x0, y0), (x1 - math.cos(a) * 26, y1 - math.sin(a) * 26)], fill=col, width=w)
    h = 30
    d.polygon([(x1, y1), (x1 - h * math.cos(a - 0.45), y1 - h * math.sin(a - 0.45)), (x1 - h * math.cos(a + 0.45), y1 - h * math.sin(a + 0.45))], fill=col)


def info(d, box, head, lines, col=BLUE):
    x0, y0, x1, y1 = box
    d.rounded_rectangle(box, 22, fill=(255, 255, 255), outline=LINE, width=3)
    d.rectangle([x0, y0 + 22, x0 + 8, y1 - 22], fill=col)
    d.text((x0 + 34, y0 + 26), head, font=ORB(30), fill=col)
    for i, t in enumerate(lines): d.text((x0 + 34, y0 + 82 + i * 40), t, font=RAJ(30), fill=INK)


# ---------------- 1. the whole system ----------------
c, d = canvas("NEUROX  -  HALO + BOX", "Eye blinks and jaw clenches drive a wheelchair. The HALO reads them, the box decides, the chair moves.", 1)
card(c, d, (70, 190, 1000, 990), trim(load(f"{HALO}/4_renders/view_hero.png")), "HALO  -  EEG / EMG headset, worn", "A")
card(c, d, (1400, 190, 2330, 760), trim(load(f"{BOX}/4_renders/assembled.png")), "BOX  -  brain of the system, on the armrest", "B")
# radio link
arrow(d, 1030, 470, 1370, 470, dash=True)
d.text((1200, 430), "ESP-NOW wireless", font=ORB(26), fill=BLUE, anchor="mb")
d.text((1200, 512), "raw signals, 256 Hz", font=RAJ(28), fill=MUT, anchor="mt")
d.text((1200, 548), "16 packets / s", font=RAJ(28), fill=MUT, anchor="mt")
arrow(d, 1865, 780, 1865, 860, dash=True)
d.rounded_rectangle([1460, 870, 2270, 990], 22, fill=INK)
d.text((1865, 905), "WHEELCHAIR  (RC-car test rig)", font=ORB(26), fill="white", anchor="mm")
d.text((1865, 952), "forward / reverse / stop, 80 ms heartbeat", font=RAJ(27), fill=(190, 196, 210), anchor="mm")
info(d, (70, 1020, 800, 1290), "HALO", ["ESP32-C3 SuperMini + 2 BioAmp EXG Pills", "6 snap electrodes on screw-lock stems", "1000 mAh LiPo, nothing wired to the user"])
info(d, (835, 1020, 1565, 1290), "BOX", ['ESP32-S3, 2.8" ST7789 touch screen', "4 buttons incl. red STOP, buzzer, MPU-6050", "2000 mAh LiPo, USB-C charging"])
info(d, (1600, 1020, 2330, 1290), "CONTROL", ["clench = forward,  blink = reverse", "close eyes 3 s = unlock,  auto-locks idle", "STOP button always wins"], RED)
c.save(f"{OUT}/1_NeuroX_system.png")

# ---------------- 2. the box ----------------
c, d = canvas("NEUROX BOX  v3", "163 x 101 x 48 mm, 3D printed in 4 colours, no supports. ESP32-S3 + touch screen + 4 buttons on a perfboard.", 2)
card(c, d, (70, 190, 1250, 1290), trim(load(f"{BOX}/4_renders/back_right.png")), "Assembled: STOP, NEW + arrow buttons, screen window", 1)
card(c, d, (1290, 190, 2330, 560), trim(load(f"{BOX}/4_renders/exploded.png")), "Exploded: base, lid, caps, STOP guard", 2)
card(c, d, (1290, 580, 2330, 950), trim(load(f"{BOX}/4_renders/top.png")), "Top: screen, N dial logo, STOP", 3)
card(c, d, (1290, 970, 2330, 1290), trim(load(f"{BOX}/4_renders/buttons_section.png"), tol=40), "Buttons cut through: caps on 12 x 12 switches", 4, pad=16)
c.save(f"{OUT}/2_NeuroX_box.png")

# ---------------- 3. the HALO ----------------
c, d = canvas("NEUROX HALO  v3", "Adjustable printed headset: 6 electrode stems slide in 2 mm steps and lock with an M3 knob. Ratchet dial at the back.", 3)
card(c, d, (70, 190, 830, 960), trim(load(f"{HALO}/4_renders/view_front.png")), "Front: crest, nose + forehead pads", 1)
card(c, d, (860, 190, 1620, 960), light_only(load(f"{HALO}/4_renders/view_lock_exploded.png")), "Close-up: stem, M3 knob, 2 mm grooves", 2)
card(c, d, (1650, 190, 2330, 960), trim(load(f"{HALO}/4_renders/view_back.png")), "Back: hub, dial, cover", 3)
card(c, d, (70, 990, 2330, 1290), trim(load(f"{HALO}/4_renders/how_it_locks_section.png")), None, pad=20)
d.ellipse([100, 1012, 142, 1054], fill=BLUE); d.text((121, 1033), "4", font=RAJ(28), fill="white", anchor="mm")
d.text((100, 1072), "Stem lock, cut through", font=RAJ(31), fill=INK); d.text((100, 1112), "loose: slides", font=RAJ(28), fill=MUT); d.text((100, 1148), "tight: tooth in groove", font=RAJ(28), fill=MUT)
c.save(f"{OUT}/3_NeuroX_HALO.png")

# ---------------- 4. schematics ----------------
c, d = canvas("SCHEMATICS", "Box: ESP32-S3 + screen + buttons + buzzer + MPU-6050 + power.   HALO: ESP32-C3 + 2 BioAmp EXG Pills + power.", 4)
card(c, d, (70, 190, 1290, 1290), load(f"{BOX}/wiring_v3.1/NeuroX_Box_S3_board_wiring.png"), "Box: ESP32-S3 DevKitC-1", 1, fill=BG, pad=16)
card(c, d, (1320, 190, 2330, 1290), load(f"{BOX}/wiring_v3.1/NeuroX_HALO_C3_board_wiring.png"), "HALO: ESP32-C3 SuperMini", 2, fill=BG, pad=16)
c.save(f"{OUT}/4_NeuroX_schematics.png")

# ---------------- 5. perfboard + UI ----------------
c, d = canvas("PERFBOARD + FIRMWARE UI", "Hand-wired 68 x 50 mm perfboard in the box, and the touch UI the box firmware draws (320 x 240).", 5)
card(c, d, (70, 190, 1420, 1290), load(f"{BOX}/wiring_v3.1/NeuroX_Box_main_board_wiring.png"), "Box perfboard map: every wire, colour coded", 1, fill=BG, pad=16)
ui = load("docs/box_ui/NeuroX_Box_UI_screens.png")
cw, ch = ui.width / 4, ui.height / 5


def cell(r, k):
    im = ui.crop((int(k * cw), int(r * ch), int((k + 1) * cw), int((r + 1) * ch)))
    g = im.convert("L").point(lambda v: 255 if v < 60 else 0)   # the dark screen area
    b = g.getbbox(); return im.crop(b)


shots = [(cell(0, 2), "Calibrate: blink x5"), (cell(1, 2), "Locked"), (cell(2, 1), "Clench: FORWARD"), (cell(2, 2), "Blink: REVERSE")]
xs, ys = (1450, 1900), (190, 740)
for i, (im, t) in enumerate(shots):
    x, y = xs[i % 2], ys[i // 2]
    card(c, d, (x, y, x + 430 + (0 if i % 2 else 0), y + 520), im, t, i + 2, fill=(255, 255, 255), pad=18)
c.save(f"{OUT}/5_NeuroX_perfboard_and_UI.png")
print("ok")
