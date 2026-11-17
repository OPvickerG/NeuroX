"""NeuroX box v3 "Reticle" as real B-rep CAD (CadQuery / OpenCASCADE).
Inside = box v2 (layout.py: screen, buttons, STOP, battery, charger, boost, ESP32-S3, pills,
buzzer). Outside = the v3 sci-fi shell. Every colour is its own solid so the AMS can print it.
Model frame: origin = inner front-left corner on top of the floor; base bottom at z = -F;
lid top at z = IH + LIDT.  +X = length, +Y = depth (front = Y 0), mm.
Run:  python3 box_v3_cq.py   ->  build/*.step + build/stl/*.stl
"""
import math, os, sys, json
import cadquery as cq
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "box", "NeuroX_Box_v2"))
import layout as Lmod

P = Lmod.P; C = Lmod.C
T, F, IL, IW, IH = (P[k] for k in ("T", "F", "IL", "IW", "IH"))
SCREEN, BATTERY, MAINBOARD, BUZZER, TP4056 = C["screen"], C["battery"], C["mainboard"], C["buzzer"], C["tp4056"]
MB_HOLES = Lmod.MB_HOLES
CLAMP_POSTS = [(14, 28.0), (66, 28.0), (17, 86.0), (56, 86.0)]   # v3.1: front-right post 56 -> 66 (clears the micro-SD socket on the board back)
BUTTONS = [(n, x, y, d) for n, x, y, d, bd, dep in Lmod.BUTTONS]
ESP_USB, TP_USB, SWITCH = Lmod.ESP_USB, Lmod.TP_USB, Lmod.SWITCH

# ---- tunables (measure your parts, change here) ----
CLR = 0.30          # general fit clearance
AA_W, AA_H, AA_DX, AA_DY, WIN_MARG = 57.6, 43.2, -1.7, 0.0, 0.5   # screen visible area + offset
GLASS_H, PCB_T = 3.0, 1.6        # MEASURED: glass front 3 mm above the PCB; PCB 1.6 mm
SCR_STACK = GLASS_H + PCB_T      # screen glass front -> PCB back (the clamp tabs press here)
BRD_L, BRD_W = 82.0, 50.0        # MEASURED: screen PCB 82 x 50 (landscape)
BTN_HOLE = 16.3     # 16 mm panel buttons (R13-507 style)
PILOT_M3, PILOT_M2 = 2.5, 1.7
SLEEVE_HOLE = 12.0  # electrode sleeve grommet hole (right wall)
SLEEVE_BORE = 8.5   # inner bore of the grommet = braided sleeve / 2 pill cables
INLAY = 0.6         # colour inlay depth (3 layers at 0.2 mm)
WHITE_T = 1.6       # white top skin thickness on the lid

# ---- v3 shell ----
CF, CB = 7.0, 10.0  # outer corner chamfers: front / back
LIDT = 4.0          # lid thickness (1 mm straight + 3 mm 45 deg bevel)
BEV = 3.0
BOT_CH = 2.2        # 45 deg chamfer at the bottom edge ("floating" base)
GROOVES = (15.0, 28.0)   # panel grooves (z of the groove bottom edge), 1.2 tall, 1.0 deep
BAND_Z = (0.5, 1.7)      # blue light line just above the bottom chamfer
SLEEVE_Y, SLEEVE_Z = 68.0, 26.0
WM_Z = 22.0              # NEUROX wordmark centre height on the front
TOP = IH + LIDT
WREG = BEV + 0.6         # white top region starts this far inside the outer outline
BOSSES = [(7.0, 5.8), (151.0, 5.8), (8.0, 88.0), (150.0, 88.0)]   # lid screws (moved off the bevel)
LOGO_C, LOGO_D = (124.0, 34.0), 44.0                              # Reticle N on the top, ring outer diameter
STOP = [(x, y) for n, x, y, d in BUTTONS if n == "stop"][0]
WIN_C = ((SCREEN[0] + SCREEN[3]) / 2 + AA_DX, (SCREEN[1] + SCREEN[4]) / 2 + AA_DY)
WIN = (AA_W + 2 * WIN_MARG, AA_H + 2 * WIN_MARG)
BRD_C = ((SCREEN[0] + SCREEN[3]) / 2, (SCREEN[1] + SCREEN[4]) / 2)
FONT_ORB = os.path.join(HERE, "orbitron900.ttf")
FONT_RAJ = os.path.join(HERE, "rajdhani700.ttf")
FONT_DV = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
if not os.path.exists(FONT_DV): FONT_DV = FONT_RAJ          # e.g. on a Mac

# ---------------- helpers ----------------
K = 2 - math.sqrt(2)          # chamfer leg change per mm of offset (keeps the wall thickness constant)
def opts(g):
    """outer outline grown by g (g < 0 = inside) as a closed point list"""
    x0, x1, y0, y1 = -T - g, IL + T + g, -T - g, IW + T + g
    cf, cb = CF + K * g, CB + K * g
    return [(x0 + cf, y0), (x1 - cf, y0), (x1, y0 + cf), (x1, y1 - cb), (x1 - cb, y1), (x0 + cb, y1), (x0, y1 - cb), (x0, y0 + cf)]
def slab(g, z0, h): return cq.Workplane("XY").workplane(offset=z0).polyline(opts(g)).close().extrude(h)
def loft(g0, z0, g1, z1):
    w0 = cq.Wire.makePolygon([cq.Vector(x, y, z0) for x, y in opts(g0)], close=True)
    w1 = cq.Wire.makePolygon([cq.Vector(x, y, z1) for x, y in opts(g1)], close=True)
    return cq.Workplane("XY").add(cq.Solid.makeLoft([w0, w1], True))
def box(x0, y0, z0, x1, y1, z1): return cq.Workplane("XY").box(x1 - x0, y1 - y0, z1 - z0, centered=False).translate((x0, y0, z0))
def cyl(x, y, z0, h, d): return cq.Workplane("XY").workplane(offset=z0).center(x, y).circle(d / 2).extrude(h)
def poly(pts, z0, h): return cq.Workplane("XY").workplane(offset=z0).polyline(pts).close().extrude(h)
def stadium_x(x0, length, yc, zc, w, h): return cq.Workplane("YZ").workplane(offset=x0).center(yc, zc).slot2D(w, h, 0).extrude(length)
def text_solid(txt, size, origin, xdir, normal, depth, font, halign="center"):
    pl = cq.Plane(origin=origin, xDir=xdir, normal=normal)
    return cq.Workplane(pl).text(txt, size, depth, combine=False, halign=halign, valign="center", fontPath=font)
def union_all(parts):
    out = parts[0]
    for p in parts[1:]: out = out.union(p)
    return out

# ---------------- Reticle N logo (same geometry as the logo files) ----------------
class SF:                     # Reticle N letter geometry (120-unit logo box, y down) - same as the logo files
    N_LEFT = [(36.0000, 88.0000), (48.0000, 88.0000), (48.0000, 56.5843), (36.0000, 37.9176)]
    N_RIGHT = [(72.0000, 32.0000), (84.0000, 32.0000), (84.0000, 82.0824), (72.0000, 63.4157)]
    N_DIAG = [(36.0000, 32.0000), (50.0000, 32.0000), (84.0000, 88.0000), (70.0000, 88.0000)]
def logo_polys():
    s = LOGO_D / 105.5
    def tr(pts): return [(LOGO_C[0] + s * (x - 60), LOGO_C[1] + s * (60 - y)) for x, y in pts]
    def sector(a0, a1, r0, r1, n=40):
        o = [(60 + r1 * math.cos(math.radians(a0 + (a1 - a0) * i / n)), 60 + r1 * math.sin(math.radians(a0 + (a1 - a0) * i / n))) for i in range(n + 1)]
        ii = [(60 + r0 * math.cos(math.radians(a1 - (a1 - a0) * i / n)), 60 + r0 * math.sin(math.radians(a1 - (a1 - a0) * i / n))) for i in range(n + 1)]
        return o + ii
    blue = [tr(sector(a0, a1, 47.25, 52.75)) for a0, a1 in ((-60, 30), (120, 210))]
    for k in range(36):
        a = math.radians(k * 10); r0 = 40.5 if k % 9 == 0 else 43.0; w = 1.0
        c, sn = math.cos(a), math.sin(a); nx, ny = -sn, c
        blue.append(tr([(60 + r * c + t * nx, 60 + r * sn + t * ny) for r, t in ((r0, -w), (46.5, -w), (46.5, w), (r0, w))]))
    blue.append(tr(SF.N_DIAG))
    stems = [tr(SF.N_LEFT), tr(SF.N_RIGHT)]
    dot = (LOGO_C[0] + s * 30, LOGO_C[1] + s * 32, s * 4.4)
    return blue, stems, dot

# =============================== BASE ===============================
def base_solid():
    b = slab(0, -F + BOT_CH, F + IH - BOT_CH).union(loft(-BOT_CH, -F, 0, -F + BOT_CH))
    b = b.cut(slab(-T, 0, IH + 1))
    for z in GROOVES: b = b.cut(slab(0.5, z, 1.2).cut(slab(-1.0, z - 1, 3.2)))
    # lid screw bosses + ribs to the walls
    for (x, y) in BOSSES:
        b = b.union(cyl(x, y, 0, IH, 8.0))
        fx = 0 if x < IL / 2 else IL; fy = 0 if y < IW / 2 else IW
        b = b.union(box(min(x, fx) - (0.3 if fx == 0 else -0.3), y - 1, 0, max(x, fx) + (0.3 if fx == IL else -0.3), y + 1, IH - 3))
        b = b.union(box(x - 1, min(y, fy) - (0.3 if fy == 0 else -0.3), 0, x + 1, max(y, fy) + (0.3 if fy == IW else -0.3), IH - 3))
    for (x, y) in MB_HOLES: b = b.union(cyl(x, y, 0, MAINBOARD[2], 5))
    bx0, by0 = BATTERY[0] - 1, BATTERY[1] - 1; bl, bw = BATTERY[3] - BATTERY[0], BATTERY[4] - BATTERY[1]
    rim = box(bx0, by0, 0, bx0 + bl + 2, by0 + bw + 2, 3).cut(box(bx0 + 1, by0 + 1, -1, bx0 + 1 + bl, by0 + 1 + bw, 4))
    b = b.union(rim.cut(box(bx0 - 1, by0 + 8, 1, bx0 + 3, by0 + bw - 8, 4)))
    b = b.union(box(0, TP4056[1], 0, TP4056[3], TP4056[4], 2))
    b = b.union(box(3, TP4056[1] - 1.6, 0, TP4056[3], TP4056[1], 5)).union(box(3, TP4056[4], 0, TP4056[3], TP4056[4] + 1.6, 5))
    bzx, bzy = (BUZZER[0] + BUZZER[3]) / 2, (BUZZER[1] + BUZZER[4]) / 2
    b = b.union(cyl(bzx, bzy, 0, 4, 16).cut(cyl(bzx, bzy, -1, 6, 12.6)))
    # ---- cuts ----
    for (x, y) in BOSSES: b = b.cut(cyl(x, y, 3, IH, PILOT_M3))
    for (x, y) in MB_HOLES: b = b.cut(cyl(x, y, 1, 10, PILOT_M2))
    b = b.cut(stadium_x(-T - 1, T + 2, TP_USB["yc"], TP_USB["zc"], TP_USB["w"], TP_USB["h"]))
    sw = SWITCH
    b = b.cut(box(-T - 1, sw["yc"] - sw["w"] / 2, sw["zc"] - sw["h"] / 2, 1, sw["yc"] + sw["w"] / 2, sw["zc"] + sw["h"] / 2))
    b = b.cut(box(-T + sw["panel"], sw["yc"] - sw["w"] / 2 - 3, sw["zc"] - sw["h"] / 2 - 3, sw["panel"], sw["yc"] + sw["w"] / 2 + 3, sw["zc"] + sw["h"] / 2 + 3))
    b = b.cut(stadium_x(IL - 1, T + 2, (ESP_USB["y0"] + ESP_USB["y1"]) / 2, ESP_USB["zc"], ESP_USB["y1"] - ESP_USB["y0"], ESP_USB["h"]))
    b = b.cut(cq.Workplane("YZ").workplane(offset=IL - 1).center(SLEEVE_Y, SLEEVE_Z).circle(SLEEVE_HOLE / 2).extrude(T + 2))
    for i in range(7):                                         # back vents: slanted slits
        x = 46 + i * 9
        b = b.cut(cq.Workplane("XZ").workplane(offset=-(IW - 1)).polyline([(x, 10), (x + 3, 10), (x + 9, 24), (x + 6, 24)]).close().extrude(-(T + 2)))
    for a in range(0, 360, 60):                                # buzzer sound holes
        b = b.cut(cyl(bzx + 3.2 * math.cos(math.radians(a)), bzy + 3.2 * math.sin(math.radians(a)), -F - 1, F + 2, 1.6))
    b = b.cut(cyl(bzx, bzy, -F - 1, F + 2, 1.6))
    for x in (12, IL - 12):                                    # rubber feet recesses
        for y in (12, IW - 12): b = b.cut(cyl(x, y, -F - 0.01, 0.8, 10.5))
    lab = [("POWER", 3.0, (-T + 0.6, SWITCH["yc"] + SWITCH["w"] / 2 + 9, SWITCH["zc"]), (0, -1, 0), (-1, 0, 0)),
           ("CHARGE", 2.8, (-T + 0.6, TP_USB["yc"], TP_USB["zc"] + 5.8), (0, -1, 0), (-1, 0, 0)),
           ("USB", 3.0, (IL + T - 0.6, (ESP_USB["y0"] + ESP_USB["y1"]) / 2, ESP_USB["zc"] - ESP_USB["h"] / 2 - 3.4), (0, 1, 0), (1, 0, 0)),
           ("NEUROX v3", 7, (IL / 2, IW / 2 + 6, -F + 0.6), (-1, 0, 0), (0, 0, -1)),
           ("battery only while worn", 3.6, (IL / 2, IW / 2 - 6, -F + 0.6), (-1, 0, 0), (0, 0, -1))]
    for txt, s, o, xd, nrm in lab: b = b.cut(text_solid(txt, s, o, xd, nrm, 0.62, FONT_DV))
    return b

def base_parts():
    b = base_solid()
    band = slab(0, BAND_Z[0], BAND_Z[1] - BAND_Z[0]).cut(slab(-0.8, BAND_Z[0] - 1, 3)).intersect(b)
    wm = text_solid("NEUROX", 11.0, (IL / 2, -T, WM_Z), (1, 0, 0), (0, -1, 0), -INLAY, FONT_ORB).intersect(b)
    mg = b.cut(band).cut(wm)
    return {"MG": mg, "BL": band, "WH": wm}

# =============================== LID ===============================
POCKET_WALL_H = 5.6      # v3.1: pocket walls 5.6 mm below the lid (tested 5 mm + 3 layers of 0.2; lid 4 + 5.6 = 9.6 mm)
POCKET_WALL_T = 1.6      # v3.1: same thickness as the tested fit check
def screen_pocket(wall_h=POCKET_WALL_H, wall_t=POCKET_WALL_T, for_lid=False):
    """walls around the screen board (board + CLR inside), pin end open. In the lid: notches for the clamp tabs,
    and room around the back-left lid screw boss of the base."""
    bx0, bx1 = BRD_C[0] - BRD_L / 2, BRD_C[0] + BRD_L / 2          # pocket = your board + CLR, same centre as before
    by0, by1 = BRD_C[1] - BRD_W / 2, BRD_C[1] + BRD_W / 2
    fr = box(bx0 - CLR - wall_t, by0 - CLR - wall_t, IH - wall_h, bx1 + CLR + wall_t, by1 + CLR + wall_t, IH + 0.01)
    fr = fr.cut(box(bx0 - CLR, by0 - CLR, IH - wall_h - 1, bx1 + CLR, by1 + CLR, IH + 0.005))
    fr = fr.cut(box(bx1 - 2, by0 + 3, IH - wall_h - 1, bx1 + wall_t + 1, by1 - 3, IH + 0.005))   # pin end open for the wires
    if for_lid:
        zt = IH - SCR_STACK + 0.1                                    # notch top: 0.1 above the board back
        for (x, y) in CLAMP_POSTS:
            ya, yb = (y - 4, by0) if y < BRD_C[1] else (by1, y + 4)
            fr = fr.cut(box(x - 4.5, ya, IH - wall_h - 1, x + 4.5, yb, zt))
        for (x, y) in BOSSES: fr = fr.cut(cyl(x, y, IH - wall_h - 1, wall_h + 2, 8.0 + 1.0))   # 0.5 mm round the base boss
    return fr

def lid_solid():
    l = slab(0, IH, 1.0).union(loft(0, IH + 1.0, -BEV, TOP))
    lip = slab(-T - CLR, IH - 2.5, 2.5).cut(slab(-T - CLR - 2.0, IH - 3.5, 4.5))
    for (x, y) in BOSSES: lip = lip.cut(cyl(x, y, IH - 4, 5, 12.4))
    lip = lip.cut(box(-T - 1, SCREEN[1] - 2, IH - 4, SCREEN[3] + 2, SCREEN[4] + 2, IH + 1))     # screen corner: no lip
    l = l.union(lip)
    l = l.union(screen_pocket(for_lid=True))
    for (x, y) in CLAMP_POSTS: l = l.union(cyl(x, y, IH - SCR_STACK, SCR_STACK + 0.01, 5.2))
    for n, x, y, d in BUTTONS: l = l.union(cyl(x, y, IH - 1.2, 1.21, d + 4.4))
    return l

def lid_cuts(l):
    sx, sy = WIN_C; ww, wh = WIN
    l = l.cut(box(sx - ww / 2, sy - wh / 2, IH - 1, sx + ww / 2, sy + wh / 2, TOP + 1))
    ch = cq.Workplane("XY").workplane(offset=TOP - 1.4).center(sx, sy).rect(ww, wh).workplane(offset=1.41).rect(ww + 2.8, wh + 2.8).loft()
    l = l.cut(ch)
    for n, x, y, d in BUTTONS: l = l.cut(cyl(x, y, IH - 2, LIDT + 4, d))
    for (x, y) in BOSSES:
        l = l.cut(cyl(x, y, IH - 3, LIDT + 4, 3.4))
        l = l.cut(cq.Workplane("XY").add(cq.Solid.makeCone(3.4 / 2, 6.9 / 2, 1.76, pnt=cq.Vector(x, y, TOP - 1.75))))
    for (x, y) in CLAMP_POSTS: l = l.cut(cyl(x, y, IH - SCR_STACK - 1, SCR_STACK, PILOT_M2))
    return l

def inlays():
    z0 = TOP - INLAY
    blue, stems, dot = logo_polys()
    B = [poly(p, z0, INLAY) for p in blue]
    # frame line, open around the buttons and STOP
    fl = slab(-WREG - 1.4, z0, INLAY).cut(slab(-WREG - 2.6, z0 - 1, 3))
    for n, x, y, d in BUTTONS: fl = fl.cut(cyl(x, y, z0 - 1, 3, 25 if n != "stop" else 29))
    for (x, y) in BOSSES: fl = fl.cut(cyl(x, y, z0 - 1, 3, 10))
    B.append(fl)
    for n, x, y, d in BUTTONS:                                 # blue ring around the 3 buttons
        if n != "stop": B.append(cyl(x, y, z0, INLAY, 22).cut(cyl(x, y, z0 - 1, 3, 17.4)))
    sx, sy = WIN_C; hx, hy = WIN[0] / 2 + 3.6, WIN[1] / 2 + 3.6   # HUD corner brackets
    for kx in (-1, 1):
        for ky in (-1, 1):
            cx, cy = sx + kx * hx, sy + ky * hy
            pts = [(0, 0), (-8, 0), (-8, -1.2), (-1.2, -1.2), (-1.2, -8), (0, -8)]
            B.append(poly([(cx + kx * px, cy + ky * py) for px, py in pts], z0, INLAY))
    MGi = [poly(p, z0, INLAY) for p in stems]
    lx, ly = [(x, y) for n, x, y, d in BUTTONS if n == "left"][0]
    ox, oy = [(x, y) for n, x, y, d in BUTTONS if n == "ok"][0]
    nx, ny = [(x, y) for n, x, y, d in BUTTONS if n == "new"][0]
    MGi.append(poly([(lx - 2.6, ly + 14), (lx + 2.2, ly + 16.6), (lx + 2.2, ly + 11.4)], z0, INLAY))
    MGi.append(poly([(ox + 2.6, oy + 14), (ox - 2.2, oy + 16.6), (ox - 2.2, oy + 11.4)], z0, INLAY))
    MGi.append(text_solid("NEW", 5.2, (nx, ny + 14, TOP), (1, 0, 0), (0, 0, 1), -INLAY, FONT_RAJ))
    R = [cyl(dot[0], dot[1], z0, INLAY, 2 * dot[2]),
         text_solid("STOP", 5.2, (STOP[0] - 17.5, STOP[1], TOP), (0, 1, 0), (0, 0, 1), -INLAY, FONT_RAJ)]
    return union_all(B), union_all(MGi), union_all(R)

def lid_parts():
    full = lid_cuts(lid_solid())
    white_reg = slab(-WREG, TOP - WHITE_T, WHITE_T)
    b, m, r = inlays()
    keep = slab(-WREG, TOP - INLAY - 0.01, INLAY + 1)            # inlays only inside the white region
    b, m, r = (lid_cuts(x.intersect(keep)) for x in (b, m, r))
    b = b.cut(r).cut(m); m = m.cut(r)                     # red dot on top of the ring, like the logo
    white = full.intersect(white_reg).cut(b).cut(m).cut(r)
    mg = full.cut(white_reg).union(m)
    return {"MG": mg, "WH": white, "BL": b, "RD": r}

# =============================== small parts ===============================
DFC_WALL_H = POCKET_WALL_H   # display fit check = same walls as the lid (9.6 mm total)
def display_fitcheck(wall_h=DFC_WALL_H):
    """only the screen part of the lid: flat 4 mm plate, window + chamfer, board pocket walls. No holes, no posts."""
    bx0, bx1 = BRD_C[0] - BRD_L / 2, BRD_C[0] + BRD_L / 2
    by0, by1 = BRD_C[1] - BRD_W / 2, BRD_C[1] + BRD_W / 2
    m = CLR + 1.6 + 4.0                                                   # same outside size as the tested one
    p = cq.Workplane("XY").workplane(offset=IH).center((bx0 + bx1) / 2, (by0 + by1) / 2) \
          .rect(bx1 - bx0 + 2 * m, by1 - by0 + 2 * m).extrude(TOP - IH).edges("|Z").fillet(3)
    p = p.union(screen_pocket(wall_h=wall_h))
    sx, sy = WIN_C; ww, wh = WIN
    p = p.cut(box(sx - ww / 2, sy - wh / 2, IH - 1, sx + ww / 2, sy + wh / 2, TOP + 1))
    ch = cq.Workplane("XY").workplane(offset=TOP - 1.4).center(sx, sy).rect(ww, wh).workplane(offset=1.41).rect(ww + 2.8, wh + 2.8).loft()
    return p.cut(ch)

def stop_guard():
    g = cyl(0, 0, 0, 1.4, 26).union(cyl(0, 0, 0, 6.0, 26).cut(cyl(0, 0, -1, 8, 21)))
    g = g.cut(cyl(0, 0, -1, 4, BTN_HOLE))
    for a in (0, 180): g = g.cut(box(9, -3.5, 3.0, 15, 3.5, 7).rotate((0, 0, 0), (0, 0, 1), a))   # finger slots
    return g

def grommet():
    g = cq.Workplane("XY").polygon(6, 18 / math.cos(math.radians(30))).extrude(3.0)
    tube_l = T + 0.4
    g = g.union(cyl(0, 0, 3.0, tube_l, SLEEVE_HOLE - 0.4))
    barb = cq.Workplane("XZ").polyline([(SLEEVE_BORE / 2 + 0.2, 3.0 + tube_l), ((SLEEVE_HOLE - 0.4) / 2, 3.0 + tube_l),
                                        (SLEEVE_HOLE / 2 + 0.5, 3.0 + tube_l + 0.7), (SLEEVE_HOLE / 2 + 0.5, 3.0 + tube_l + 1.2),
                                        ((SLEEVE_HOLE - 0.4) / 2 - 0.3, 3.0 + tube_l + 2.2), (SLEEVE_BORE / 2 + 0.2, 3.0 + tube_l + 2.2)]).close().revolve(360, (0, 0, 0), (0, 1, 0))
    g = g.union(barb)
    g = g.cut(cyl(0, 0, -1, 20, SLEEVE_BORE))
    for a in (0, 90): g = g.cut(box(-0.6, -8, 3.0 + 0.8, 0.6, 8, 20).rotate((0, 0, 0), (0, 0, 1), a))   # 4 slits so the barb clicks in
    g = g.cut(cq.Workplane("XY").workplane(offset=-0.01).circle(SLEEVE_BORE / 2 + 0.8).workplane(offset=0.8).circle(SLEEVE_BORE / 2).loft())  # entry chamfer
    return g

TAB_REACH = 3.0      # v3.1: tab end circle 3 mm from the screw (was 7) -> presses only the outer 2.5 mm of the board
def clamp_tab():
    s = cq.Workplane("XY").sketch().push([(0, 0), (0, TAB_REACH)]).circle(3.5).reset().push([(0, TAB_REACH / 2)]).rect(7, TAB_REACH).finalize().extrude(2.2)
    return s.cut(cyl(0, 0, -1, 5, 2.3))

def push_button():
    LIDZ = LIDT
    bez = cyl(0, 0, 0, 2.5, 19.0); cap = cyl(0, 0, 2.5, 3.5, 12.0).edges(">Z").fillet(1.0)
    thread = cyl(0, 0, -LIDZ - 1.2 - 8.0, LIDZ + 1.2 + 8.0, 15.8)
    nut = cq.Workplane("XY").workplane(offset=-LIDZ - 1.2 - 3.0).polygon(6, 21.0).extrude(3.0).cut(cyl(0, 0, -LIDZ - 1.2 - 4, 5, 16.0))
    body = cyl(0, 0, -LIDZ - 1.2 - 8.0 - 12.0, 12.0, 17.0)
    return bez.union(cap), thread.union(body), nut

if __name__ == "__main__":
    import time
    out = os.path.join(HERE, "build"); os.makedirs(os.path.join(out, "stl"), exist_ok=True)
    t0 = time.time()
    BP = base_parts(); print("base", {k: round(v.val().Volume() / 1000, 2) for k, v in BP.items()}, round(time.time() - t0, 1), "s")
    LP = lid_parts(); print("lid", {k: round(v.val().Volume() / 1000, 2) for k, v in LP.items()}, round(time.time() - t0, 1), "s")
    SG, GR, CT = stop_guard(), grommet(), clamp_tab()
    parts = {"base": BP, "lid": LP, "stop_guard": {"RD": SG}, "sleeve_grommet": {"BL": GR}, "clamp_tab": {"MG": CT}}
    for name, d in parts.items():
        for c, s in d.items():
            ok = all(sol.isValid() for sol in s.solids().vals())
            print(f"  {name}/{c}: {len(s.solids().vals())} solid(s), valid {ok}")
            cq.exporters.export(s, os.path.join(out, "stl", f"{name}__{c}.stl"), tolerance=0.02, angularTolerance=0.1)
    # fit check (v3.1): only the screen part of the lid - window + board pocket, no holes
    FC = display_fitcheck()
    cq.exporters.export(FC, os.path.join(out, "stl", "fitcheck__MG.stl"), tolerance=0.02, angularTolerance=0.1)
    cq.exporters.export(FC, os.path.join(out, "NeuroX_v3_fitcheck.step"))
    # one-colour STL of every part (for a printer without AMS)
    for name, d in parts.items():
        cq.exporters.export(union_all(list(d.values())), os.path.join(out, "stl", f"{name}.stl"), tolerance=0.02, angularTolerance=0.1)
    # STEP: each part (all colours as named bodies) + the coloured assembly
    COL = {"MG": cq.Color(0.17, 0.19, 0.24), "WH": cq.Color(0.95, 0.95, 0.96), "BL": cq.Color(0.07, 0.34, 0.82), "RD": cq.Color(0.84, 0.10, 0.12)}
    CN = {"MG": "midnight", "WH": "white", "BL": "blue", "RD": "red"}
    for name, d in parts.items():
        a = cq.Assembly(name=f"NeuroX_v3_{name}")
        for c, s in d.items(): a.add(s, name=f"{name}_{CN[c]}", color=COL[c])
        a.save(os.path.join(out, f"NeuroX_v3_{name}.step"))
    asm = cq.Assembly(name="NeuroX_Box_v3")
    for name in ("base", "lid"):
        for c, s in parts[name].items(): asm.add(s, name=f"{name}_{CN[c]}", color=COL[c])
    asm.add(SG, name="stop_guard", loc=cq.Location(cq.Vector(STOP[0], STOP[1], TOP)), color=COL["RD"])
    asm.add(GR, name="sleeve_grommet", loc=cq.Location(cq.Vector(IL + T + 3.0, SLEEVE_Y, SLEEVE_Z), cq.Vector(0, 1, 0), -90), color=COL["BL"])
    for i, (x, y) in enumerate(CLAMP_POSTS):
        dy = 1 if y < (SCREEN[1] + SCREEN[4]) / 2 else -1
        asm.add(CT, name=f"clamp_tab_{i + 1}", color=COL["MG"], loc=cq.Location(cq.Vector(x, y, IH - SCR_STACK - 2.2), cq.Vector(0, 0, 1), 0 if dy > 0 else 180))
    top, under, nut = push_button()
    for n, x, y, d in BUTTONS:
        zt = TOP + (1.4 if n == "stop" else 0.0)
        col = cq.Color(0.85, 0.08, 0.06) if n == "stop" else cq.Color(0.08, 0.08, 0.09)
        asm.add(top, name=f"button_{n}_cap", loc=cq.Location(cq.Vector(x, y, zt)), color=col)
        asm.add(under, name=f"button_{n}_body", loc=cq.Location(cq.Vector(x, y, TOP)), color=cq.Color(0.15, 0.15, 0.15))
        asm.add(nut, name=f"button_{n}_nut", loc=cq.Location(cq.Vector(x, y, TOP)), color=cq.Color(0.6, 0.6, 0.62))
    asm.save(os.path.join(out, "NeuroX_Box_v3_assembly.step"))
    print("done", round(time.time() - t0, 1), "s", sorted(os.listdir(out)))
