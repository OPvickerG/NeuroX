"""NeuroX box v3 - printed buttons for 12x12 tactile switches (12x12x7.3 mm, the common kind).
One button = GUIDE (pressed + glued up into the 16.3 mm lid hole from below)
           + CAP (slides in the guide, trapped under its lip)
           + CRADLE (holds the switch, 2 x M2x10 screws up through it into the guide's flange).
STOP: same guide + cradle, a longer STOP CAP and a red STOP TOP (mushroom) glued on, inside the red guard.
Button-local frame: z = 0 is the lid's top surface, +z up. mm.
Run: python3 box_buttons_cq.py  -> build/buttons/*.stl, *.step + checks against the lid / base."""
import os, sys, math
import cadquery as cq
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, HERE)
import box_v3_cq as B

# ---- the switch (measure yours: SW_H is from the switch's bottom to the top of its button) ----
SW_W, SW_H = 12.0, 7.3          # body width, total height
SW_PIN_X = 6.25                 # pins stick out at x = +/-6.25 (two per side, 5 mm apart)
# ---- lid ----
HOLE = B.BTN_HOLE               # 16.3
LID_D = B.LIDT + 1.2            # lid 4 mm + the 1.2 mm ring under each hole = 5.2
CLR = 0.25
# ---- guide ----
G_OD, G_ID, LIP_ID = 16.0, 13.2, 12.0
RIB_R = (HOLE + 0.35) / 2       # 4 crush ribs: 0.17 mm interference per side
Z_TOP = -0.4                    # guide top, just under the lid surface
Z_LIP = -1.6                    # lip underside
FL_D, FL_T = 20.0, 3.4          # flange under the lid ring
Z_FL = -LID_D                   # flange top (= -5.2)
Z_FLB = Z_FL - FL_T             # flange bottom (= -8.6)
SCREW_Y = 8.6                   # 2 x M2 at (0, +/-8.6)
# ---- cradle ----
POCKET = (2 * SW_PIN_X + 1.1, SW_W + 2 * CLR)   # 13.6 x 12.5 (pins fit in the long side)
DEPTH = 4.8                     # pocket depth below the guide flange (switch body < 4.8 mm)
FLOOR = 1.6
Z_FLOOR = Z_FLB - DEPTH         # switch bottom sits here (= -13.4)
Z_ACT = Z_FLOOR + SW_H          # switch button top (= -6.1)
CR_W, CR_L = 18.0, 21.0          # 18 wide: clears the lid screw post next to the left button
# ---- caps ----
CAP_D, CAP_FL_D, CAP_FL_T = 11.4, 12.9, 1.2
PL_D = 8.0
GAP = 0.1                       # plunger hovers 0.1 mm over the switch button at rest
CAP_TOP = 4.5                   # normal caps stand 4.5 mm above the lid (was 1.8: too short)
STOP_CAP_TOP = 4.9              # STOP stem top; with the 2.6 mm top = 7.5 mm, 1.5 mm proud of the guard (6.0)
STOP_TOP_D, STOP_TOP_H = 16.5, 2.6
PEG_D, PEG_L = 6.0, 2.6

def cyl(d, z0, h, x=0, y=0): return cq.Workplane("XY").workplane(offset=z0).center(x, y).circle(d / 2).extrude(h)
def box(x0, y0, z0, x1, y1, z1): return cq.Workplane("XY").box(x1 - x0, y1 - y0, z1 - z0, centered=False).translate((x0, y0, z0))

def guide():
    g = cyl(G_OD, Z_FL, Z_TOP - Z_FL)
    for a in (45, 135, 225, 315):                                     # crush ribs (vertical in print)
        g = g.union(box(-0.6, G_OD / 2 - 0.3, Z_FL + 0.6, 0.6, RIB_R, Z_TOP - 0.6).rotate((0, 0, 0), (0, 0, 1), a))
    g = g.union(cyl(FL_D, Z_FLB, FL_T))
    g = g.cut(cyl(G_ID, Z_FLB - 1, Z_LIP - Z_FLB + 1))                 # bore
    g = g.cut(cyl(LIP_ID, Z_LIP - 0.1, Z_TOP - Z_LIP + 1))             # lip opening
    for sy in (-1, 1): g = g.cut(cyl(1.7, Z_FLB - 1, FL_T + 2, 0, sy * SCREW_Y))   # M2 pilots
    return g

def cradle():
    z1 = Z_FLB; z0 = Z_FLOOR - FLOOR
    c = box(-CR_W / 2, -CR_L / 2, z0, CR_W / 2, CR_L / 2, z1).edges("|Z").fillet(2.0)
    px, py = POCKET
    c = c.cut(box(-px / 2, -py / 2, Z_FLOOR, px / 2, py / 2, z1 + 1))                       # switch pocket
    for sx in (-1, 1): c = c.cut(box(sx * SW_PIN_X - 0.9, -3.8, z0 - 1, sx * SW_PIN_X + 0.9, 3.8, Z_FLOOR + 0.1))   # pin slots
    for sy in (-1, 1):
        c = c.cut(cyl(2.3, z0 - 1, z1 - z0 + 2, 0, sy * SCREW_Y))                             # M2 clearance
    return c

def cap(stop=False, sw_h=SW_H, dots=0):
    """sw_h = YOUR switch: bottom of the black body to the top of its button (pins not counted).
    A shorter switch gets a longer plunger, so the cap still rests 0.1 mm above the button.
    dots = 1..5 shallow dots on the cap top (sizer caps only, to tell them apart)."""
    top = STOP_CAP_TOP if stop else CAP_TOP
    zf = Z_LIP                                                         # flange top rests under the lip
    z_act = Z_FLOOR + sw_h                                             # top of your switch's button
    c = cyl(CAP_D, zf, top - zf)
    if not stop: c = c.faces(">Z").edges().chamfer(0.6)
    c = c.union(cyl(CAP_FL_D, zf - CAP_FL_T, CAP_FL_T))
    c = c.union(cyl(PL_D, z_act + GAP, (zf - CAP_FL_T) - (z_act + GAP) + 0.01))
    if stop: c = c.cut(cyl(PEG_D + 0.3, top - PEG_L - 0.2, PEG_L + 1))  # socket for the STOP top
    for i in range(dots):                                              # marks: a row of 1.3 mm dots, 0.4 deep
        c = c.cut(cyl(1.3, top - 0.4, 1, (i - (dots - 1) / 2) * 2.0, 0))
    return c

def stop_top():
    z0 = STOP_CAP_TOP
    t = cyl(STOP_TOP_D, z0, STOP_TOP_H).faces(">Z").edges().chamfer(0.8)
    return t.union(cyl(PEG_D, z0 - PEG_L, PEG_L + 0.01))

# print orientations (flat side on the bed, z = 0)
def on_bed(w, flip=False):
    if flip: w = w.rotate((0, 0, 0), (1, 0, 0), 180)
    bb = w.val().BoundingBox(); return w.translate((0, 0, -bb.zmin))
PRINT = {"guide": lambda: on_bed(guide()), "cradle": lambda: on_bed(cradle()),
         "cap": lambda: on_bed(cap(), flip=True), "stop_cap": lambda: on_bed(cap(True), flip=True),
         "stop_top": lambda: on_bed(stop_top(), flip=True)}

if __name__ == "__main__":
    out = os.path.join(HERE, "build", "buttons"); os.makedirs(out, exist_ok=True)
    parts = {"guide": guide(), "cradle": cradle(), "cap": cap(), "stop_cap": cap(True), "stop_top": stop_top()}
    for n, w in parts.items():
        ok = all(s.isValid() for s in w.solids().vals()); print(f"{n:9s} solids {len(w.solids().vals())} valid {ok}  vol {w.val().Volume() / 1000:.2f} cm3")
        cq.exporters.export(PRINT[n](), os.path.join(out, f"{n}.stl"), tolerance=0.01, angularTolerance=0.1)
        cq.exporters.export(w, os.path.join(out, f"NeuroX_v3_button_{n}.step"))
    # ---- checks: one assembly per button, against the lid, base, guard, clamp tabs, and the switch ----
    def vol(w):
        try: return sum(s.Volume() for s in w.solids().vals())
        except Exception: return 0.0
    LP = B.lid_parts(); lid = B.union_all(list(LP.values())); base = B.union_all(list(B.base_parts().values()))
    CT = B.clamp_tab().val(); tabs = [cq.Workplane().add(CT.moved(cq.Location(cq.Vector(x, y, B.IH - B.SCR_STACK - 2.2), cq.Vector(0, 0, 1), 0 if y < 57 else 180))) for x, y in B.CLAMP_POSTS]
    guard = cq.Workplane().add(B.stop_guard().val().moved(cq.Location(cq.Vector(B.STOP[0], B.STOP[1], B.TOP))))
    sw = box(-SW_W / 2, -SW_W / 2, Z_FLOOR, SW_W / 2, SW_W / 2, Z_FLOOR + 3.6).union(cyl(6.8, Z_FLOOR + 3.6, SW_H - 3.6))
    for sx in (-1, 1):
        for sy in (-1, 1): sw = sw.union(box(sx * SW_PIN_X - 0.4, sy * 2.5 - 0.4, Z_FLOOR - 3.5, sx * SW_PIN_X + 0.4, sy * 2.5 + 0.4, Z_FLOOR + 1.5))
    asm = []
    for n, x, y, d in B.BUTTONS:
        stop = n == "stop"
        P = {"guide": guide(), "cradle": cradle(), "cap": cap(stop), "switch": sw}
        if stop: P["stop_top"] = stop_top()
        P = {k: v.translate((x, y, B.TOP)) for k, v in P.items()}
        asm.append((n, P))
        rows = []
        for k, v in P.items():
            r = [f"lid {vol(v.intersect(lid)):.2f}", f"base {vol(v.intersect(base)):.2f}"]
            r.append("tabs %.2f" % sum(vol(v.intersect(t)) for t in tabs))
            if stop: r.append(f"guard {vol(v.intersect(guard)):.2f}")
            rows.append(f"{k}: " + " ".join(r))
        # parts against each other (should all be ~0, the ribs are the only intended press fit with the lid)
        ks = list(P); inter = []
        for i in range(len(ks)):
            for j in range(i + 1, len(ks)):
                v = vol(P[ks[i]].intersect(P[ks[j]]))
                if v > 0.01: inter.append(f"{ks[i]}x{ks[j]} {v:.2f}")
        print(f"[{n}] " + " | ".join(rows) + "  | parts overlap: " + (", ".join(inter) or "none"))
    # rest gaps
    print(f"plunger over switch button: {GAP} mm, cap travel to the guide lip: free; switch travel ~0.25 mm")
    print(f"cap above lid: {CAP_TOP} mm; STOP top {STOP_CAP_TOP + STOP_TOP_H:.1f} mm (guard top 6.0)")
    print(f"module bottom z {B.TOP + Z_FLOOR - FLOOR:.1f} (absolute)")
    # STEP assembly of the buttons in the box
    a = cq.Assembly(name="NeuroX_Box_v3_buttons")
    col = {"guide": cq.Color(0.17, 0.19, 0.24), "cradle": cq.Color(0.17, 0.19, 0.24), "cap": cq.Color(0.08, 0.08, 0.1),
           "switch": cq.Color(0.6, 0.6, 0.62), "stop_top": cq.Color(0.84, 0.10, 0.12)}
    for n, P in asm:
        for k, v in P.items():
            c = col[k] if not (n == "stop" and k == "cap") else cq.Color(0.84, 0.10, 0.12)
            a.add(v, name=f"{n}_{k}", color=c)
    a.save(os.path.join(out, "NeuroX_Box_v3_buttons_assembly.step"))
    for n, P in asm:
        for k, v in P.items(): cq.exporters.export(v, os.path.join(out, f"asm_{n}_{k}.stl"), tolerance=0.02, angularTolerance=0.1)
    print("done")
