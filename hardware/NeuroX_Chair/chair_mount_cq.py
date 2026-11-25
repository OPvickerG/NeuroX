"""NeuroX chair - frame mount: rubber-lined 2-piece CLAMP on the wheelchair tube + ARM down to the motor hanger.

  CLAMP  2 halves, 4 x M5 x 30. Wrap the tube with 1 layer of bicycle inner tube first: it grips, nothing slides.
         Bore for a 22, 25 or 28 mm tube (print the TUBE SIZER first to find yours).
         Best on the VERTICAL backrest tube just behind the top of the tyre: slide it to any height.
         Also fits a horizontal tube (armrest): turn the clamp 90 degrees, the 4-nut cross pattern allows both.
  ARM    v2 for the Vissco Rodeo Veer: the clamp goes on the BARE ARMREST TUBE (14 cm between the foam and the weld).
         Deck bolts on the spring bar (2 x M5, 80 mm sideways slots). Tall wall with 2 full-height slots: the clamp bolts
         anywhere along 15 cm of height. A centre web + 6 gussets keep it stiff.

Coordinates as chair_drive_cq.py (wheel centre = origin, X forward, Z up, Y outboard). Units mm.
Run: python3 chair_mount_cq.py
"""
import math, os, sys
import cadquery as cq
import trimesh
import chair_drive_cq as C

OUT = C.OUT
YMID = (C.POD_Y0 + C.POD_Y1) / 2
M5 = 5.6; M5_HEAD = 9.6; M5_NUT_AF = 8.2; M5_NUT_T = 4.4

# ---------------- clamp ----------------
LINER = 1.0                       # inner tube wrap (0.8-1 mm) -> bore = tube + 2 x LINER/2
GAP = 2.0                         # gap between the halves = how much the clamp can squeeze
CL_H = 60.0                       # clamp length along the tube
FACE = 14.0                       # material from the bore to the mount face
SIDE = 12.0                       # material beside the bore (bolt bosses)
CROSS = 20.0                      # mount nuts at (0, +/-20) and (+/-20, 0) on the face
# ---------------- arm (v2: Vissco Rodeo Veer armrest tube, horizontal, about 12 cm inboard of the tyre) ----------------
DECK_T, WALL_T, WEB_H = 10.0, 8.0, 16.0
DECK_Z0 = C.BAR_Z1                                    # deck sits on top of the hanger (spring bar)
DECK_X = 45.0                                         # deck is X -45..45: stays clear of the spring nuts at X +/-69
DECK_SLOT_X = 36.0                                    # deck slots over the hanger holes at X +/-36 (80 mm sideways travel)
DECK_SLOT_L = 80.0
CLAMP_X = 20.0                                        # clamp bolts at X +/-20 (the clamp's nut pair along the tube)
Y_T_NOM = -120.0                                      # armrest tube centre, nominal (sideways from the tyre centre)
WALL_Z0, WALL_Z1 = DECK_Z0 - 50.0, DECK_Z0 + 100.0   # tall wall = height range for the clamp
DECK_Y1 = YMID + DECK_SLOT_L / 2 + 8.0
def box(x0, x1, y0, y1, z0, z1):
    return cq.Workplane("XY").box(x1 - x0, y1 - y0, z1 - z0, centered=False).translate((x0, y0, z0))


# ============================================================ CLAMP (local: tube along Z, mount face = +Y, centre at origin)
def clamp_halves(D):
    B = D + LINER
    R = B / 2
    W = R + SIDE                                       # half width (X)
    bx = R + 6.0                                       # clamp bolt X
    outer = (box(-W, W, GAP / 2, R + FACE, -CL_H / 2, CL_H / 2)
             .union(cq.Workplane("XY").circle(R + 6).extrude(CL_H).translate((0, 0, -CL_H / 2)).intersect(box(-W, W, GAP / 2, 99, -99, 99))))
    inner = (box(-W, W, -(R + 6), -GAP / 2, -CL_H / 2, CL_H / 2)
             .edges("|Z and <Y").fillet(8.0))
    bore = cq.Workplane("XY").circle(R).extrude(CL_H + 2).translate((0, 0, -CL_H / 2 - 1))
    outer, inner = outer.cut(bore), inner.cut(bore)
    for x in (-bx, bx):
        for z in (-18, 18):
            hole = cq.Workplane("XZ").center(x, z).circle(M5 / 2).extrude(-80).translate((0, -40, 0))
            outer, inner = outer.cut(hole), inner.cut(hole)
            inner = inner.cut(cq.Workplane("XZ").center(x, z).circle(M5_HEAD / 2 + 0.3).extrude(-20).translate((0, -(R + 6) - 1, 0))
                              .translate((0, 0, 0)).intersect(box(-99, 99, -99, -(R + 6) + 4.5, -99, 99)))      # head counterbore
            nut = (cq.Workplane("XZ").center(x, z).polygon(6, M5_NUT_AF / math.cos(math.pi / 6)).extrude(-(R + FACE)).translate((0, 6.5, 0)))
            outer = outer.cut(nut)                                                                                # nut drops in from the face side
    # 4 captive M5 nuts for the arm, cross pattern; slid in from the top/bottom or the side
    ny = R + FACE - 5.0 - M5_NUT_T / 2                                                                            # 5 mm of face in front of the nut
    for (x, z, ax) in ((0, CROSS, "z"), (0, -CROSS, "z"), (CROSS, 0, "x"), (-CROSS, 0, "x")):
        outer = outer.cut(cq.Workplane("XZ").center(x, z).circle(M5 / 2).extrude(-(FACE + 2)).translate((0, R + FACE - FACE + 2, 0)).translate((0, 0, 0))
                          .intersect(box(-99, 99, R + 1.5, R + FACE + 1, -99, 99)))
        pocket = (cq.Workplane("XZ").center(x, z).polygon(6, M5_NUT_AF / math.cos(math.pi / 6)).extrude(-M5_NUT_T).translate((0, ny + M5_NUT_T / 2, 0)))
        if ax == "z":
            chan = box(x - M5_NUT_AF / 2, x + M5_NUT_AF / 2, ny - M5_NUT_T / 2, ny + M5_NUT_T / 2, min(z, math.copysign(CL_H, z)), max(z, math.copysign(CL_H, z)))
        else:
            chan = box(min(x, math.copysign(99, x)), max(x, math.copysign(99, x)), ny - M5_NUT_T / 2, ny + M5_NUT_T / 2, z - M5_NUT_AF / 2, z + M5_NUT_AF / 2)
        outer = outer.cut(pocket).cut(chan)
    return outer, inner, R + FACE                         # last: mount face Y


# ============================================================ ARM
def arm(y_t=Y_T_NOM, face=None):
    """wall (XZ plane) on the clamp's mount face + deck that sits on the spring bar.
    y_t = tube centre Y it is drawn for (the real arm slides +/-40 on the deck slots)."""
    if face is None: face = clamp_halves(25)[2]
    yw0 = y_t + face; yw1 = yw0 + WALL_T                                                # wall: inboard face touches the clamp
    deck_y0 = yw1 - 0.01
    a = box(-DECK_X, DECK_X, yw0, yw1, WALL_Z0, WALL_Z1)                                 # wall
    a = a.union(box(-DECK_X, DECK_X, deck_y0, DECK_Y1, DECK_Z0, DECK_Z0 + DECK_T))       # deck
    a = a.union(box(-3, 3, deck_y0, DECK_Y1 - 6, DECK_Z0 + DECK_T - 0.01, DECK_Z0 + DECK_T + WEB_H))   # stiffening web on top
    for x in (-DECK_X, DECK_X - 6, -3):                                                  # gussets wall <-> deck, above and below
        for z0, z1, up in ((DECK_Z0 + DECK_T, WALL_Z1, True), (WALL_Z0, DECK_Z0, False)):
            h = min(z1 - z0, 60.0)
            pts = [(yw1, z0), (yw1, z0 + h), (yw1 + h, z0)] if up else [(yw1, z1), (yw1, z1 - h), (yw1 + h, z1)]
            a = a.union(cq.Workplane("YZ").polyline(pts).close().extrude(6).translate((x, 0, 0)))
    for s_ in (-1, 1):
        x = s_ * CLAMP_X                                                                  # clamp bolt slots, full height
        a = a.cut(cq.Workplane("XZ").center(x, (WALL_Z0 + WALL_Z1) / 2).slot2D(WALL_Z1 - WALL_Z0 - 20, M5, 90)
                  .extrude(-(WALL_T + 2)).translate((0, yw0 - 1, 0)))
        a = a.cut(box(x - 6.5, x + 6.5, yw1 - 0.5, DECK_Y1 + 1, DECK_Z0 - 1, DECK_Z0 + DECK_T + WEB_H + 1))   # head + key channel
        for z0, z1 in ((DECK_Z0 + DECK_T, WALL_Z1), (WALL_Z0, DECK_Z0)):                  # ... and through the gussets
            a = a.cut(box(x - 6.5, x + 6.5, yw1 - 0.5, yw1 + 70, z0 - 1, z1 + 1))
        xs = s_ * DECK_SLOT_X                                                             # deck slots over the hanger holes
        a = a.cut(cq.Workplane("XY").center(xs, YMID).slot2D(DECK_SLOT_L + M5, M5, 90).extrude(DECK_T + 2).translate((0, 0, DECK_Z0 - 1)))
    return a


def tube_sizer():
    """push it over the tube: the smallest hole that slides on = your tube size.
    Notches above each hole: 1 = 19, 2 = 20, 3 = 22, 4 = 25, 5 = 28, 6 = 30, 7 = 32 mm."""
    rows = ((19, 20, 22, 25), (28, 30, 32))
    p = cq.Workplane("XY").box(126, 100, 3.2, centered=False)
    for r, sizes in enumerate(rows):
        x, y = 6, 74 - r * 50
        for s in sizes:
            p = p.cut(cq.Workplane("XY").center(x + s / 2, y - 4).circle(s / 2).extrude(10).translate((0, 0, -1)))
            n = (19, 20, 22, 25, 28, 30, 32).index(s) + 1                    # 1-7 notches above the hole = size (see README)
            for k in range(n):
                p = p.cut(cq.Workplane("XY").center(x + s / 2 + (k - (n - 1) / 2) * 3.2, y + 17).rect(1.6, 6).extrude(2).translate((0, 0, 2.2)))
            x += s + 6
    return p


def place_clamp(part, cx, y_t, cz, horizontal=True):
    """local clamp (tube along Z) -> chair: tube centre at (Y = y_t, Z = cz), clamp centre at X = cx.
    horizontal = tube along X (armrest): the clamp's (0, +/-20) nut pair then lines up with the wall slots at X +/-20."""
    w = part.rotate((0, 0, 0), (0, 1, 0), 90) if horizontal else part
    return w.translate((cx, y_t, cz))


if __name__ == "__main__":
    def stl(w, name, rot=None):
        p = os.path.join(OUT, name + ".stl"); cq.exporters.export(w, p, tolerance=0.02, angularTolerance=0.15)
        m = trimesh.load(p); m.merge_vertices()
        if rot is not None: m.apply_transform(rot)
        m.apply_translation(-m.bounds[0]); m.export(p); return m
    rx = lambda a: trimesh.transformations.rotation_matrix(math.radians(a), [1, 0, 0])
    res = {}
    for D in (22, 25, 28):
        o, i, _ = clamp_halves(D)
        res[f"clamp_{D}_outer"] = stl(o, f"clamp_{D}mm_outer", rx(-90))     # mount face down on the bed
        res[f"clamp_{D}_inner"] = stl(i, f"clamp_{D}mm_inner", rx(-90))     # split face down
    res["arm"] = stl(arm(), "arm", rx(90))                                   # clamp side of the wall flat on the bed
    res["sizer"] = stl(tube_sizer(), "tube_sizer")
    for n, m in res.items():
        print(f"{n:16s} {m.extents[0]:6.1f} x {m.extents[1]:6.1f} x {m.extents[2]:5.1f}  watertight={m.is_watertight}  {m.volume / 1000:5.1f} cm3")
