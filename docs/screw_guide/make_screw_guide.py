"""NeuroX - screw picture guide (Box v3 + HALO v3). Every screw drawn to scale, with count, where it goes and a render.
Counts = docs/NeuroX_Screw_List.xlsx ('Where it goes').  Run from the repo root:  python3 docs/screw_guide/make_screw_guide.py"""
import os
from PIL import Image, ImageDraw, ImageFont, ImageChops

F = "/usr/share/fonts/opentype/inter"
DISP = lambda s: ImageFont.truetype(f"{F}/InterDisplay-Bold.otf", s)
B = lambda s: ImageFont.truetype(f"{F}/Inter-Bold.otf", s)
SB = lambda s: ImageFont.truetype(f"{F}/Inter-SemiBold.otf", s)
R = lambda s: ImageFont.truetype(f"{F}/Inter-Medium.otf", s)
BG = (246, 247, 249); INK = (22, 26, 34); MUT = (104, 110, 124); LINE = (220, 223, 230); CARD = (255, 255, 255)
STEEL = (150, 156, 168); STEEL_D = (92, 98, 110); STEEL_L = (196, 201, 210); BLUE = (20, 90, 220); RED = (210, 30, 40); AMBER = (190, 120, 0)
BOX, HALO = "hardware/NeuroX_Box_v3/4_renders", "hardware/NeuroX_HALO_v3/4_renders"
OUT = "docs/screw_guide"
S = 16   # px per mm for the screw drawings


# ------------------------------------------------------------------ screw drawings (side view, head on the left)
def thread(d, x0, x1, yc, dia, tip):
    r = dia * S / 2
    body_end = x1 - (dia * S * 0.9 if tip else 0)
    d.rectangle([x0, yc - r, body_end, yc + r], fill=STEEL)
    if tip: d.polygon([(body_end, yc - r), (x1, yc), (body_end, yc + r)], fill=STEEL)
    pitch = (0.5 if dia <= 2.2 else 0.7 if dia <= 3.2 else 0.8) * S * (1.6 if tip else 1.0)
    x = x0 + pitch / 2
    while x < x1 - 4:
        rr = r if x < body_end else r * max(0.15, (x1 - x) / (x1 - body_end))
        d.line([(x - pitch * 0.35, yc - rr), (x + pitch * 0.35, yc + rr)], fill=STEEL_D, width=2)
        x += pitch
    d.line([(x0, yc - r), (body_end, yc - r)], fill=STEEL_D, width=2); d.line([(x0, yc + r), (body_end, yc + r)], fill=STEEL_D, width=2)


def screw(d, x, yc, head, dia, length, tip=True):
    """x = left edge. length = overall for countersunk, under the head for the rest (how screws are sold)."""
    r = dia * S / 2
    if head == "socket":
        hd, hh = 1.75 * dia * S, dia * S
        d.rounded_rectangle([x, yc - hd / 2, x + hh, yc + hd / 2], 4, fill=STEEL_L, outline=STEEL_D, width=2)
        d.rectangle([x, yc - hd * 0.22, x + hh * 0.45, yc + hd * 0.22], fill=STEEL_D)              # hex socket
        thread(d, x + hh, x + hh + length * S, yc, dia, tip); return x + hh + length * S
    if head == "csk":
        hd = 2.0 * dia * S; hh = (hd - 2 * r) / 2                                                 # 90 deg head
        d.polygon([(x, yc - hd / 2), (x + hh, yc - r), (x + hh, yc + r), (x, yc + hd / 2)], fill=STEEL_L, outline=STEEL_D)
        d.line([(x + 1, yc - hd * 0.18), (x + 1, yc + hd * 0.18)], fill=STEEL_D, width=5)        # Phillips slot edge-on
        thread(d, x + hh, x + length * S, yc, dia, tip); return x + length * S
    if head == "pan":
        hd, hh = 2.0 * dia * S, 0.7 * dia * S
        d.chord([x - hh, yc - hd / 2, x + hh, yc + hd / 2], 90, 270, fill=STEEL_L, outline=STEEL_D, width=2)
        d.rectangle([x, yc - hd / 2, x + 3, yc + hd / 2], fill=STEEL_L)
        d.line([(x - hh + 2, yc - hd * 0.15), (x - hh + 2, yc + hd * 0.15)], fill=STEEL_D, width=5)
        d.line([(x, yc - hd / 2), (x, yc + hd / 2)], fill=STEEL_D, width=2)
        thread(d, x, x + length * S, yc, dia, tip); return x + length * S
    if head == "hex":
        hd, hh = 1.75 * dia * S, 0.7 * dia * S
        d.rectangle([x, yc - hd / 2, x + hh, yc + hd / 2], fill=STEEL_L, outline=STEEL_D, width=2)
        d.line([(x, yc - hd / 4), (x + hh, yc - hd / 4)], fill=STEEL_D, width=2); d.line([(x, yc + hd / 4), (x + hh, yc + hd / 4)], fill=STEEL_D, width=2)
        thread(d, x + hh, x + hh + length * S, yc, dia, tip); return x + hh + length * S


def nut(d, cx, yc, dia):
    af, t = (5.5 if dia == 3 else 7.0) * S, (2.4 if dia == 3 else 3.2) * S
    w = af * 1.155
    d.rectangle([cx - t / 2, yc - w / 2, cx + t / 2, yc + w / 2], fill=STEEL_L, outline=STEEL_D, width=2)
    d.line([(cx - t / 2, yc - w / 4), (cx + t / 2, yc - w / 4)], fill=STEEL_D, width=2)
    d.line([(cx - t / 2, yc + w / 4), (cx + t / 2, yc + w / 4)], fill=STEEL_D, width=2)


def dim(d, x0, x1, y, label):
    d.line([(x0, y), (x1, y)], fill=MUT, width=2)
    for x in (x0, x1): d.line([(x, y - 9), (x, y + 9)], fill=MUT, width=2)
    d.text(((x0 + x1) / 2, y + 12), label, font=SB(22), fill=MUT, anchor="mt")


# ------------------------------------------------------------------ renders
def trim(im, pad=12, tol=12):
    bg = Image.new("RGB", im.size, im.getpixel((2, 2)))
    b = ImageChops.difference(im, bg).convert("L").point(lambda v: 255 if v > tol else 0).getbbox()
    return im.crop((max(0, b[0] - pad), max(0, b[1] - pad), min(im.width, b[2] + pad), min(im.height, b[3] + pad))) if b else im


def thumb(path, w, h, crop=None):
    im = Image.open(path).convert("RGB")
    if im.getpixel((2, 2))[0] < 60:                                     # dark-framed render: keep the light inside
        b = im.convert("L").point(lambda v: 255 if v > 235 else 0).getbbox(); im = im.crop(b)
    if crop: im = im.crop([int(c * s) for c, s in zip(crop, (im.width, im.height, im.width, im.height))])
    im = trim(im); r = min(w / im.width, h / im.height)
    im = im.resize((int(im.width * r), int(im.height * r)), Image.LANCZOS)
    out = Image.new("RGB", (w, h), (248, 248, 248)); out.paste(im, ((w - im.width) // 2, (h - im.height) // 2)); return out


# ------------------------------------------------------------------ sheet
W = 2400; ROW_H = 330; DRAW_W = 560; THUMB = (620, 290)


def sheet(title, sub, rows, name, tools):
    H = 470 + (ROW_H + 30) * len(rows)
    im = Image.new("RGB", (W, H), BG); d = ImageDraw.Draw(im)
    d.text((70, 56), title, font=DISP(66), fill=INK)
    d.text((72, 150), sub, font=R(30), fill=MUT)
    y = 230
    for r in rows:
        d.rounded_rectangle([60, y, W - 60, y + ROW_H], 22, fill=CARD, outline=LINE, width=3)
        # drawing
        cy = y + ROW_H / 2 - 18; x0 = 110
        x1 = screw(d, x0, cy, r["head"], r["dia"], r["len"], r.get("tip", True))
        hstart = x0 if r["head"] in ("csk",) else x0 + (r["dia"] * S if r["head"] == "socket" else 0.7 * r["dia"] * S if r["head"] == "hex" else 0)
        dim(d, hstart, x1, cy + r["dia"] * S * 1.05 + 26, f'{r["len"]} mm')
        if r.get("nut"): nut(d, x1 + 90, cy, r["dia"]); d.text((x1 + 90, cy + r["dia"] * S * 1.05 + 38), "+ nut", font=SB(22), fill=MUT, anchor="mt")
        # count badge
        bx = 110 + DRAW_W - 20
        d.rounded_rectangle([bx, y + 40, bx + 150, y + 130], 18, fill=r.get("col", BLUE))
        d.text((bx + 75, y + 85), f'x{r["qty"]}', font=DISP(52), fill="white", anchor="mm")
        d.text((bx + 75, y + 152), "pieces", font=SB(22), fill=MUT, anchor="mt")
        # text
        tx = bx + 200; tw = W - 60 - THUMB[0] - 60 - tx
        d.text((tx, y + 36), r["name"], font=B(40), fill=INK)
        d.text((tx, y + 92), r["kind"], font=SB(25), fill=r.get("col", BLUE))
        ly = y + 140
        for line in r["where"]:
            d.ellipse([tx, ly + 10, tx + 10, ly + 20], fill=INK)
            d.text((tx + 24, ly), line, font=R(28), fill=INK); ly += 42
        if r.get("tip_text"): d.text((tx, y + ROW_H - 46), r["tip_text"], font=R(24), fill=AMBER)
        # render
        im.paste(thumb(*r["img"][:1], *THUMB, r["img"][1] if len(r["img"]) > 1 else None), (W - 60 - THUMB[0] - 20, y + 20))
        y += ROW_H + 30
    # tools + tips
    d.rounded_rectangle([60, y, W - 60, y + 160], 22, fill=(30, 34, 44))
    d.text((100, y + 30), "TOOLS", font=B(26), fill=(150, 190, 255))
    d.text((100, y + 72), tools, font=R(28), fill="white")
    d.text((100, y + 112), "Into plastic: drive the screw in once to cut the thread, back it out, then screw it in for real. Stop as soon as it is snug.",
           font=R(26), fill=(200, 205, 215))
    im = im.crop((0, 0, W, y + 200)); im.save(os.path.join(OUT, name)); print(name, im.size)


os.makedirs(OUT, exist_ok=True)
sheet("BOX v3  SCREWS", "20 screws, all small Phillips. Drawings are to scale (the length shown is what the shop sells).", [
    dict(head="csk", dia=3, len=12, qty=4, name="M3 x 12 mm  countersunk", kind="flat head  ·  self-tapping 3 mm (or machine)",
         where=["Lid  ->  the 4 corner bosses of the base", "Last step: after the wiring is tested"],
         tip_text="Flat head sits flush in the lid's corner holes", img=(f"{BOX}/top.png",)),
    dict(head="pan", dia=2, len=10, qty=8, name="M2 x 10 mm  pan head", kind="round head  ·  self-tapping 2 mm (or machine)",
         where=["4 printed buttons, 2 each", "switch cradle  ->  button guide (from under the lid)"],
         tip_text="Switch legs straight + body flat before you screw", img=(f"{BOX}/exploded.png", (0.0, 0.0, 1.0, 0.45))),
    dict(head="pan", dia=2, len=6, qty=8, name="M2 x 6 mm  pan head", kind="round head  ·  self-tapping 2 mm (or machine)",
         where=["4 x perfboard  ->  the 4 standoffs in the base", "4 x screen clamp tabs  ->  lid underside"],
         tip_text="Short ones: they must not poke through the lid", img=(f"{BOX}/base_inside.png",)),
], "NeuroX_Box_screw_guide.png", "Small Phillips screwdriver (PH0 / PH1) for every box screw.")

sheet("HALO v3  SCREWS", "32 screws + 7 nuts. Drawings are to scale (the length shown is what the shop sells).", [
    dict(head="socket", dia=3, len=8, qty=6, nut=True, tip=False, name="M3 x 8 mm  socket head  + M3 nut", kind="Allen 2.5 mm  ·  machine thread (into the nut)",
         where=["6 stem lock knobs: screw glued into the printed knob", "nut sits in the hex pocket of each sleeve"],
         tip_text="Finger-tight only: it presses the stem onto the teeth", img=(f"{HALO}/view_lock_exploded.png",), col=BLUE),
    dict(head="csk", dia=3, len=20, qty=4, name="M3 x 20 mm", kind="flat head  ·  self-tapping 3 mm (or machine)",
         where=["4 band joints: front piece  <->  left + right sides", "not longer than 20 mm"],
         img=(f"{HALO}/view_hero.png", (0.0, 0.35, 1.0, 0.85))),
    dict(head="csk", dia=3, len=10, qty=8, name="M3 x 10 mm  countersunk", kind="flat head  ·  self-tapping 3 mm (or machine)",
         where=["2 x strap  ->  left side", "2 x crest (front badge)", "4 x back cover"],
         img=(f"{HALO}/view_front.png", (0.05, 0.0, 0.95, 0.75))),
    dict(head="pan", dia=3, len=12, qty=12, name="M3 x 12 mm  pan head", kind="round head  ·  self-tapping 3 mm (or machine)",
         where=["5 x rib tabs  ->  band", "5 x ribs  ->  crown hub (top)", "2 x temple caps"],
         img=(f"{HALO}/view_hero.png", (0.1, 0.0, 0.9, 0.5))),
    dict(head="pan", dia=3, len=10, qty=1, name="M3 x 10 mm  pan head", kind="round head  ·  self-tapping 3 mm (or machine)",
         where=["back hub set screw"], img=(f"{HALO}/view_back.png", (0.15, 0.35, 0.85, 0.85))),
    dict(head="hex", dia=4, len=16, qty=1, nut=True, tip=False, name="M4 x 16 mm  hex bolt  + M4 nut", kind="spanner 7 mm  ·  machine thread (into the nut)",
         where=["back dial (strap adjust)", "nut goes in the hex pocket of the right side"],
         tip_text="Tight enough to hold, loose enough for the dial to click", img=(f"{HALO}/view_back.png", (0.45, 0.25, 1.0, 0.8)), col=RED),
], "NeuroX_HALO_screw_guide.png", "2.5 mm Allen key (lock knobs), small Phillips screwdriver, 7 mm spanner or pliers (M4 nut).")
