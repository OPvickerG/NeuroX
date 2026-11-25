"""NeuroX chair - frame mount: rubber-lined 2-piece CLAMP on the wheelchair tube + ARM down to the motor hanger.

  CLAMP  2 halves, 4 x M5 x 30. Wrap the tube with 1 layer of bicycle inner tube first: it grips, nothing slides.
         Bore for a 22, 25 or 28 mm tube (print the TUBE SIZER first to find yours).
         Best on the VERTICAL backrest tube just behind the top of the tyre: slide it to any height.
         Also fits a horizontal tube (armrest): turn the clamp 90 degrees, the 4-nut cross pattern allows both.
  ARM    flat deck that bolts on the hanger (2 x M5, slots = 40 mm sideways adjust) + a wall with 3 rows of slots
         that bolts to the clamp (2 x M5, slots = 110 mm front/back adjust). Gussets keep it from flexing.

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
# ---------------- arm ----------------
DECK_T, WALL_T = 8.0, 10.0
DECK_Z0 = C.BAR_Z1                                    # deck sits on top of the hanger
ZC = DECK_Z0 + DECK_T / 2                             # middle slot row height
ROWS = (ZC - CROSS, ZC, ZC + CROSS)
ARM_X0, ARM_X1 = -95.0, 45.0                          # wall length
DECK_X0 = -50.0                                       # deck stops before the rear spring nut (X = -69): you can reach it
SLOT_X0, SLOT_X1 = -85.0, 35.0                        # wall slots (bolt centres can sit anywhere in between)
Y_WALL = -22.0                                        # wall outboard face, nominal (deck slots move it +/-20)
DECK_Y1 = YMID + 30.0
DECK_SLOT_X = 24.0                                    # deck slots line up with the hanger holes at X +/-24
WALL_Z0, WALL_Z1 = ZC - CROSS - 14, ZC + CROSS + 14


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
def arm():
    deck = box(DECK_X0, ARM_X1, Y_WALL - WALL_T + 0.01, DECK_Y1, DECK_Z0, DECK_Z0 + DECK_T)
    wall = box(ARM_X0, ARM_X1, Y_WALL - WALL_T, Y_WALL, WALL_Z0, WALL_Z1)
    a = deck.union(wall)
    for x in (DECK_X0, ARM_X1 - 8, -5.0):                                                        # gussets
        for up in (1, -1):
            if up < 0 and x == -5.0: continue
            z0, z1 = (DECK_Z0 + DECK_T, WALL_Z1) if up > 0 else (WALL_Z0, DECK_Z0)
            tri = (cq.Workplane("YZ").polyline([(Y_WALL, z0 if up > 0 else z1), (Y_WALL, z1 if up > 0 else z0),
                                                (Y_WALL + (z1 - z0), z0 if up > 0 else z1)]).close().extrude(8).translate((x, 0, 0)))
            a = a.union(tri.intersect(box(-999, 999, -999, DECK_Y1, -999, 999)))
    for s in (-1, 1):                                                                            # deck slots over the hanger holes
        a = a.cut(cq.Workplane("XY").center(s * DECK_SLOT_X, YMID).slot2D(40 + M5, M5, 90).extrude(40).translate((0, 0, DECK_Z0 - 10)))
    for z in ROWS:                                                                               # wall slots, 3 rows
        a = a.cut(cq.Workplane("XZ").center((SLOT_X0 + SLOT_X1) / 2, z).slot2D(SLOT_X1 - SLOT_X0 + M5, M5 + 0.6, 0)
                  .extrude(-40).translate((0, Y_WALL - 20, 0)))
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


def place_clamp(part, D, cx, cy_face, cz, horizontal=False):
    """local clamp -> chair: mount face against the wall's inboard face (Y = Y_WALL - WALL_T), centre at (cx, cz)."""
    face = clamp_halves(D)[2]
    w = part.rotate((0, 0, 0), (0, 1, 0), 90) if horizontal else part
    return w.translate((cx, cy_face - face, cz))


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
    res["arm"] = stl(arm(), "arm", rx(90))                                   # clamp side of the wall on the bed
    res["sizer"] = stl(tube_sizer(), "tube_sizer")
    for n, m in res.items():
        print(f"{n:16s} {m.extents[0]:6.1f} x {m.extents[1]:6.1f} x {m.extents[2]:5.1f}  watertight={m.is_watertight}  {m.volume / 1000:5.1f} cm3")
