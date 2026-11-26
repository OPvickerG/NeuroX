"""Assembly sheet for the chair drive: 6 numbered steps, cut from the real CAD (cross sections) -> img/chair_assembly_steps.png"""
import math, os, numpy as np, trimesh, matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, Circle, PathPatch, Polygon as MPoly, FancyBboxPatch
from matplotlib.path import Path
import cadquery as cq
import chair_drive_cq as C
import chair_mount_cq as Mt

IMG = os.path.join(C.HERE, "img")
POD, HANG, SPOOL, STEEL, NUT, BAND, ARM, CLAMP, TUBE, TYRE = ("#3a6fd8", "#f08a3c", "#4a4a4a", "#c9ccd1", "#8d939b",
                                                              "#111111", "#3aa35c", "#9b59c9", "#d7d9dc", "#2b2c30")
YM = (C.POD_Y0 + C.POD_Y1) / 2


def mesh(w):
    p = "/tmp/_asm.stl"; cq.exporters.export(w, p, tolerance=0.05, angularTolerance=0.1); return trimesh.load(p)


def section(w, axis, c):
    """shapely polygons of the cut. axis 'x': u = Y, v = Z.  axis 'y': u = X, v = Z."""
    m = mesh(w)
    if axis == "x":
        T = np.array([[0, 1, 0, 0], [0, 0, 1, 0], [1, 0, 0, -c], [0, 0, 0, 1.0]]); n = [1, 0, 0]; o = [c, 0, 0]
    else:
        T = np.array([[1, 0, 0, 0], [0, 0, 1, 0], [0, -1, 0, c], [0, 0, 0, 1.0]]); n = [0, 1, 0]; o = [0, c, 0]
    s = m.section(plane_origin=o, plane_normal=n)
    if s is None: return []
    planar, _ = s.to_2D(to_2D=T, check=False)
    return list(planar.polygons_full)


def fill(ax, polys, col, z=2, alpha=1.0):
    for p in polys:
        verts, codes = [], []
        for ring in [p.exterior] + list(p.interiors):
            xy = np.asarray(ring.coords); verts += list(xy); codes += [Path.MOVETO] + [Path.LINETO] * (len(xy) - 2) + [Path.CLOSEPOLY]
        ax.add_patch(PathPatch(Path(verts, codes), fc=col, ec="k", lw=0.8, zorder=z, alpha=alpha))


def note(ax, txt, xy, xyt, fs=10, **kw):
    ax.annotate(txt, xy, xyt, fontsize=fs, va="center", arrowprops=dict(arrowstyle="->", lw=1), zorder=20, **kw)


def screw_y(ax, z, y_head, length, head_out=+1, col=STEEL, d=4.0, hd=C.M4_HEAD, ht=2.2):
    """screw along Y, drawn in a (Y, Z) view. head sits at y_head on the side head_out points to."""
    y0, y1 = sorted((y_head, y_head - head_out * length))
    ax.add_patch(Rectangle((y0, z - d / 2), y1 - y0, d, fc=col, ec="k", lw=0.8, zorder=8, hatch="////"))
    hy = y_head if head_out > 0 else y_head - ht
    ax.add_patch(FancyBboxPatch((hy, z - hd / 2), ht, hd, boxstyle="round,pad=0,rounding_size=1", fc=col, ec="k", lw=0.8, zorder=8))


def nut_y(ax, z, y0, t=3.2):
    ax.add_patch(Rectangle((y0, z - 4.04), t, 8.08, fc=NUT, ec="k", lw=0.8, zorder=9))


def washer_y(ax, z, y0):
    ax.add_patch(Rectangle((y0, z - 4.5), C.WASHER_T, 9, fc=NUT, ec="k", lw=0.8, zorder=9))


fig, axs = plt.subplots(2, 3, figsize=(21, 14), facecolor="white")
axs = axs.ravel()

# ---------------- 1. POD + MOTORS (seen from outside the wheel, cut through the pod) ----------------
ax = axs[0]
fill(ax, section(C.pod(), "y", YM), POD)
for x, gz in zip(C.RX, C.GZ):
    ax.add_patch(Circle((x, gz), C.GB_D / 2, fc=STEEL, ec="k", zorder=1, alpha=0.9))
    ax.add_patch(Circle((x, gz - C.GB_E), 3, fc="#666", ec="k", zorder=3))
    for a in (-50, 50):
        d = np.array([math.sin(math.radians(a)), math.cos(math.radians(a))])
        p0 = np.array([x, gz]) + d * (C.RING_ID / 2 - 1); p1 = np.array([x, gz]) + d * (C.RING_OD / 2 + 4)
        ax.plot(*zip(p0, p1), color="#555", lw=4, solid_capstyle="butt", zorder=6)
note(ax, "shaft at the BOTTOM", (C.RX[0], C.GZ[0] - C.GB_E - 3), (C.RX[0] - 40, C.GZ[0] - 40))
note(ax, "4 x M3 x 12 set screws\n(M3 nuts slide into the slots\nfrom the outer face first)", (C.RX[1] + 20, C.GZ[1] + 20), (C.RX[1] - 10, C.GZ[1] + 55))
x = C.TAB_X
ax.annotate("", (x, C.GZ[0]), (x, C.TAB_Z1 + 22), arrowprops=dict(arrowstyle="-|>", lw=2, color="#c0392b"), zorder=20)
ax.text(x, C.TAB_Z1 + 25, "drop 1 M4 nut\nin each end", ha="center", fontsize=10, color="#c0392b")
ax.annotate("", (-x, C.GZ[0]), (-x, C.TAB_Z1 + 22), arrowprops=dict(arrowstyle="-|>", lw=2, color="#c0392b"), zorder=20)
ax.set_xlim(-95, 95); ax.set_ylim(285, 395)
ax.set_title("1  Pod: both motors in, same side, shafts at the bottom.\nTighten the 4 M3 x 12. Drop an M4 nut into each end tab.", fontsize=12, loc="left")

# ---------------- 2. CUP ROLLER (cut through the set screw), with the tyre crown under it ----------------
ax = axs[1]
fill(ax, section(C.roller_cup(), "y", 0.0), "#555")
W = C.ROLL_W; rw = (W - C.CUP_N * C.CUP_GROOVE) / (C.CUP_N + 1)
for k in range(C.CUP_N):                                                     # rubber bands in the grooves (both sides of the cut)
    zc = rw * (k + 1) + C.CUP_GROOVE * (k + 0.5); r1 = C.cup_r(zc - W / 2) + 0.1
    for s_ in (-1, 1):
        ax.add_patch(Circle((s_ * (r1 - 0.8), zc), 0.85, fc=BAND, ec="k", lw=0.5, zorder=4))
seat = C.SHAFT_D / 2 - 0.5 + C.ROLL_SCREW_L
ax.add_patch(Rectangle((C.SHAFT_D / 2 - 0.5, W / 2 - 1.5), C.ROLL_SCREW_L, 3.0, fc=STEEL, ec="k", hatch="////", zorder=6))
ax.add_patch(Rectangle((seat, W / 2 - 2.75), 3.0, 5.5, fc=STEEL, ec="k", zorder=6))
ax.add_patch(Rectangle((4.6, W / 2 - 2.75), 2.4, 5.5, fc=NUT, ec="k", zorder=7))
ax.add_patch(Rectangle((-C.SHAFT_D / 2, W - 26), C.SHAFT_D, 30, fc=STEEL, ec="k", zorder=1, alpha=0.7, hatch=".."))
yy = np.linspace(-C.TYRE_W / 2 + 0.5, C.TYRE_W / 2 - 0.5, 80)                   # tyre crown, seen in the same cut (rotated: tyre below = -X)
ax.plot(-(C.ROLL_R + C.TYRE_W / 2 - np.sqrt((C.TYRE_W / 2) ** 2 - yy ** 2)), yy + W / 2, color=TYRE, lw=3)
note(ax, "rubber bands: 1-2 in each\nof the 5 grooves, wrapped tight", (C.cup_r(9) - 0.5, W / 2 + 9), (C.ROLL_R + 10, W + 4))
note(ax, "M3 x 8 onto the FLAT of the shaft\n(head sits under the bands)", (seat + 1.5, W / 2), (C.ROLL_R + 10, W / 2 - 2))
note(ax, "M3 nut, slides in\nfrom this end", (5.8, 1), (C.ROLL_R + 10, -8))
note(ax, "motor shaft\n(from the motor end)", (0, W + 2), (-C.ROLL_R - 26, W + 8), ha="left")
note(ax, "tyre crown:\nthe cup hugs it", (-C.ROLL_R - 0.5, W / 2 - 8), (-C.ROLL_R - 30, -8), ha="left")
ax.set_xlim(-C.ROLL_R - 32, C.ROLL_R + 42); ax.set_ylim(-14, W + 14)
ax.set_title("2  Cup rollers (x2): onto the shaft, M3 nut in the slot, M3 x 8 tight on\nthe shaft flat, then rubber bands in the 5 grooves.", fontsize=12, loc="left")

# ---------------- 3. POD INTO THE LEGS: the lock bolt and the leg spool (cut through one leg) ----------------
ax = axs[2]
fill(ax, section(C.hanger(), "x", C.TAB_X), HANG, z=1)
fill(ax, section(C.pod(), "x", C.TAB_X), POD, z=2)
sp = C.spool().rotate((0, 0, 0), (1, 0, 0), -90)
for zc in (C.GZ[0], C.FOOT_Z):
    fill(ax, section(sp.translate((C.TAB_X, C.LEG_Y1, zc)), "x", C.TAB_X), SPOOL, z=4)
    washer_y(ax, zc, C.SPOOL_Y1)
screw_y(ax, C.GZ[0], C.SPOOL_Y1 + C.WASHER_T, C.LOCK_L)
nut_y(ax, C.GZ[0], C.NUT_Y0 + 0.1)
screw_y(ax, C.FOOT_Z, C.SPOOL_Y1 + C.WASHER_T, C.PIN_L)
note(ax, "M4 x 25 lock bolt + washer", (C.SPOOL_Y1 + 3, C.GZ[0] + 3), (C.SPOOL_Y1 + 6, C.GZ[0] + 22))
note(ax, "spool (flange on the leg)", (C.LEG_Y1 + 4, C.GZ[0] + 7), (C.SPOOL_Y1 + 6, C.GZ[0] + 38))
note(ax, "M4 nut (step 1)", (C.NUT_Y0 + 1.6, C.GZ[0] + 3), (C.POD_Y0 - 22, C.GZ[0] + 28), ha="left")
note(ax, "pod boss in the leg slot", (C.LEG_Y0 + 2, C.GZ[0] + 2), (C.POD_Y0 - 22, C.GZ[0] - 22), ha="left")
note(ax, "M4 x 16 + washer + spool\n(screws into the leg)", (C.SPOOL_Y1 + 3, C.FOOT_Z), (C.SPOOL_Y1 + 6, C.FOOT_Z - 14))
ax.text(C.POD_Y0 - 22, C.BAR_Z1 + 6, "inside (tyre side)", fontsize=10, color="#555")
ax.text(C.SPOOL_Y1 - 4, C.BAR_Z1 + 6, "outside", fontsize=10, color="#555")
ax.set_xlim(C.POD_Y0 - 24, C.SPOOL_Y1 + 48); ax.set_ylim(C.LEG_Z0 - 18, C.BAR_Z1 + 12)
ax.set_title("3  Pod bosses into the leg slots (from inside). Spool, washer, M4 x 25\ninto the nut: just touching, then back HALF a turn (pod slides).", fontsize=12, loc="left")

# ---------------- 4. MOUNT: clamp on the armrest, arm, deck on the hanger bar (cut at X = 8, screws drawn in) ----------------
ax = axs[3]
yt, zt = Mt.Y_T_NOM, C.R_T + 94
o, i, face = Mt.clamp_halves(25)
fill(ax, section(Mt.arm(), "x", 8.0), ARM, z=1)
fill(ax, section(Mt.place_clamp(o, 0, yt, zt), "x", 8.0), CLAMP, z=2)
fill(ax, section(Mt.place_clamp(i, 0, yt, zt), "x", 8.0), CLAMP, z=2, alpha=0.75)
ax.add_patch(Circle((yt, zt), 12.5, fc=TUBE, ec="k", zorder=3)); ax.add_patch(Circle((yt, zt), 13.0, fc="none", ec=BAND, lw=2.5, zorder=3))
fill(ax, section(C.hanger(), "x", 8.0), HANG, z=1)
fill(ax, section(C.pod(), "x", 8.0), POD, z=1, alpha=0.5)
R = (25 + Mt.LINER) / 2
for dz in (-(R + 6), R + 6):                                             # clamp bolts: head in the inner half, nut in the outer half
    screw_y(ax, zt + dz, yt + Mt.CL_HEAD_Y, Mt.CLAMP_L, head_out=-1)
    nut_y(ax, zt + dz, yt + Mt.CL_NUT_Y)
yw1 = yt + face + Mt.WALL_T                                              # arm bolt: head on the wall, nut captive in the clamp
screw_y(ax, zt, yw1, Mt.ARM_L, head_out=+1)
nut_y(ax, zt, yt + face - Mt.NUT_FACE - 3.2)
zd = Mt.DECK_Z0 + Mt.DECK_T - Mt.DECK_CB                                 # deck screw (at X = +/-36, drawn dashed)
ax.add_patch(Rectangle((YM - 2, zd + C.WASHER_T - C.DECK_L), 4, C.DECK_L, fc="none", ec="k", ls="--", zorder=8))
ax.add_patch(Rectangle((YM - 4.5, zd), 9, C.WASHER_T + 2.2, fc="none", ec="k", ls="--", zorder=8))
note(ax, "armrest tube, wrapped in\n1 layer of inner tube", (yt - 9, zt + 9), (yt - 42, zt + 75), ha="left")
note(ax, "4 x M4 x 20: heads in the round half,\nnuts in the flat half", (yt + Mt.CL_HEAD_Y - 2, zt - R - 6), (yt - 42, zt - 75), ha="left")
note(ax, "2 x M4 x 16 through the\narm wall slots into the\nclamp's end nuts", (yw1 + 2.5, zt), (yw1 + 30, zt + 55))
note(ax, "2 x M4 x 12 + washers\nthrough the deck slots\ninto the bar (at X ±36)", (YM, zd + 2), (YM - 15, zd + 75))
note(ax, "arm (wall + deck)", (yw1 - 3, zt - 40), (yw1 + 30, zt - 60))
note(ax, "hanger bar", (YM + 10, C.BAR_Z0 + 4), (YM + 30, C.BAR_Z0 - 25))
ax.set_xlim(yt - 45, C.SPOOL_Y1 + 40); ax.set_ylim(C.GZ[0] - 40, Mt.WALL_Z1 + 15)
ax.set_title("4  Mount (seen from the front of the chair): clamp round the armrest,\narm wall onto the clamp, arm deck onto the hanger bar.", fontsize=12, loc="left")

# ---------------- 5. BANDS (one leg seen from outside) ----------------
ax = axs[4]
x0 = C.TAB_X
ax.add_patch(Rectangle((x0 - C.LEG_HX, C.LEG_Z0), 2 * C.LEG_HX, C.BAR_Z1 - C.LEG_Z0, fc=HANG, ec="k"))
ax.add_patch(Rectangle((x0 - 45, C.BAR_Z0), 45 + C.LEG_HX, C.BAR_T, fc=HANG, ec="k"))
ax.add_patch(FancyBboxPatch((x0 - C.SLOT_W / 2, C.GZ[0] - C.TRAV_DN), C.SLOT_W, C.TRAV_DN + C.TRAV_UP,
                            boxstyle=f"round,pad=0,rounding_size={C.SLOT_W / 2 - 0.01}", fc="white", ec="k"))
ax.add_patch(Rectangle((x0 - 30, C.TAB_Z0), 30 - C.LEG_HX, 12, fc=POD, ec="k"))
for k, off in enumerate((-1.6, 0, 1.6)):                                                  # 3 turns
    for sx in (-1, 1):
        ax.plot([x0 + sx * (C.SPOOL_CORE + 0.4 + k * 0.5)] * 2, [C.FOOT_Z, C.GZ[0]], color=BAND, lw=2.2, zorder=3)
for z in (C.GZ[0], C.FOOT_Z):
    ax.add_patch(Circle((x0, z), C.SPOOL_FL, fc=SPOOL, ec="k", zorder=4, alpha=0.85)); ax.add_patch(Circle((x0, z), C.M4_HEAD / 2, fc=STEEL, ec="k", zorder=5))
note(ax, "6-8 mm strip of inner tube:\ntie on, wrap 3 turns round\nboth spools STRETCHED\n(about twice its length), tie off", (x0 - C.SPOOL_CORE - 1, (C.GZ[0] + C.FOOT_Z) / 2), (x0 - 62, (C.GZ[0] + C.FOOT_Z) / 2 + 30), ha="left")
note(ax, "rollers slip?\nadd a turn", (x0 + C.SPOOL_CORE + 1, (C.GZ[0] + C.FOOT_Z) / 2), (x0 + 22, (C.GZ[0] + C.FOOT_Z) / 2))
ax.set_xlim(x0 - 64, x0 + 50); ax.set_ylim(C.LEG_Z0 - 8, C.BAR_Z1 + 8)
ax.set_title("5  Bands on both legs (after the unit is on the chair, step 6).\nLift the motors by hand: it should take a firm pull (about 3 kg).", fontsize=12, loc="left")

# ---------------- 6. ON THE CHAIR: order ----------------
ax = axs[5]; ax.axis("off")
txt = ("6  On the chair, in this order\n\n"
       "a. Wrap the bare armrest tube (between the foam and\n    the weld) with 1 layer of inner tube.\n"
       "b. Clamp halves round it: 4 x M4 x 20, flat half facing\n    the wheel. Just snug for now so it can slide.\n"
       "c. Arm deck on the hanger bar: 2 x M4 x 12 + washers (snug).\n"
       "d. Arm wall on the clamp: 2 x M4 x 16, loose.\n"
       "e. Lock the pod 6 mm above the bottom of its slots.\n"
       "    Slide the arm sideways: rollers over the middle of the tread.\n"
       "    Slide the clamp along the armrest: rollers on top of the tyre.\n"
       "    Move the arm up/down the wall slots: rollers JUST touch.\n"
       "f. Tighten: clamp HARD (crosswise, halves ~1 mm apart, it must\n    not twist on the tube) > 2 wall bolts > 2 deck screws.\n"
       "    Halves touching and still loose? Add a 2nd layer of tube.\n"
       "g. Bands (step 5), then loosen the 2 lock bolts half a turn.\n\n"
       "DRIVE:  lock bolts half a turn loose.\n"
       "PARK:   lift the motors, tighten both lock bolts\n"
       "            (rollers stay 1 cm up, push by hand).\n\n"
       "Other wheel: the same parts, turned round.\n"
       "First test with the wheels off the ground (README: Test).")
ax.text(0.0, 1.0, txt, va="top", ha="left", fontsize=12.5, family="DejaVu Sans", transform=ax.transAxes, linespacing=1.45)

for a in axs[:5]:
    a.set_aspect(1); a.axis("off")
fig.suptitle("NeuroX chair drive: assembly (one wheel)   blue = pod, orange = hanger, green = arm, purple = clamp, dark = spools, hatched = screws",
             fontsize=15, y=0.995)
plt.tight_layout(rect=(0, 0, 1, 0.97)); plt.savefig(os.path.join(IMG, "chair_assembly_steps.png"), dpi=100); print("saved")
