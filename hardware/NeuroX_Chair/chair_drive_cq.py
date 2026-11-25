"""NeuroX chair - top friction drive for a manual wheelchair, 2 Johnson 12 V 500 RPM motors per wheel.

  POD     holds the 2 motors by their gearboxes (2 x M3 set screws each). Its 2 rollers sit on top of the tyre.
  HANGER  bar above the pod. 2 x M5 bolts + springs push the pod down, one over each roller.
          Tighten the 2 nuts = rollers lift off the tyre (push the chair by hand). Loosen = drive.
  ROLLER  21 mm core + bicycle inner tube sleeve (= ~24 mm), M3 set screw onto the shaft flat.
          ROLLER_TPU = solid 25 mm roller for TPU, if you have it.
  The ARM that bolts the hanger to the chair frame is made after the frame is measured (chair_arm_cq.py).

Same parts on both wheels (the other side is the same part turned over).
Coordinates: wheel centre = origin, X forward, Z up, Y outboard (away from the seat). Units mm.
Run:  python3 chair_drive_cq.py   ->  stl/, NeuroX_Chair_drive_plate_x2.3mf, check report
"""
import math, os, sys, json
import cadquery as cq
import trimesh, numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "stl"); os.makedirs(OUT, exist_ok=True)

# ---------------- chair (24" rear wheel, solid tyre) - check against your chair ----------------
R_T = 305.0          # tyre outer radius (24" wheel ~ 610 mm across)
TYRE_W = 45.0        # tyre width (Vissco Rodeo Veer mag wheel: ~6 cm over the curve = ~45 mm across)
RIM_R, RIM_Y, RIM_D = 272.0, TYRE_W / 2 + 20.0, 19.0     # push rim: radius of its centre line, offset outboard, tube diameter
# ---------------- motor: Johnson 12 V 500 RPM side-shaft geared motor ----------------
GB_D, GB_T, GB_E = 36.0, 22.0, 8.0          # gearbox diameter, thickness, shaft offset from the gearbox centre
MOT_D, MOT_L = 36.0, 57.0                    # motor can
BOSS, SHAFT_D, SHAFT_L = 3.0, 6.0, 29.0      # shaft sticks out SHAFT_L from the boss
YF = 21.0                                    # gearbox shaft-side face, outboard of the tyre centre plane
# ---------------- roller ----------------
CORE_R, ROLL_R, ROLL_W = 10.5, 12.0, 18.0    # printed core, core + inner-tube sleeve, width
ROLL_Y0 = YF - BOSS - SHAFT_L + 1.0          # roller outer face (just past the shaft tip)
PHI = 6.0                                     # rollers at +/- PHI degrees from the top of the tyre
# ---------------- pod / hanger ----------------
RING_ID = GB_D + 1.0; RING_WALL = 6.0; RING_OD = RING_ID + 2 * RING_WALL
POD_Y0, POD_Y1 = YF + 2.0, YF + GB_T - 2.0   # pod slab (on the gearbox only, motor can starts at YF + GB_T)
BOLT_D, SPRING_OD = 5.5, 13.0
BAR_T, BAR_GAP = 12.0, 14.0                  # hanger bar thickness, gap above the rings (= lift travel)

R_C = R_T + ROLL_R                            # roller centre radius
RX = [R_C * math.sin(math.radians(a)) for a in (-PHI, PHI)]
RZ = [R_C * math.cos(math.radians(a)) for a in (-PHI, PHI)]
GZ = [z + GB_E for z in RZ]                   # gearbox centres (shaft at the bottom of the gearbox)
TAB_X = max(abs(x) for x in RX) + RING_OD / 2 + 11.0
TAB_Z0, TAB_Z1 = GZ[0] - 6.0, GZ[0] + 6.0
BAR_Z0 = GZ[0] + RING_OD / 2 + BAR_GAP; BAR_Z1 = BAR_Z0 + BAR_T
BAR_X = TAB_X + 12.0


def prism_y(x0, x1, z0, z1, y0, y1):
    return cq.Workplane("XY").box(x1 - x0, y1 - y0, z1 - z0, centered=False).translate((x0, y0, z0))


def cyl_y(cx, cz, r, y0, y1):
    return cq.Workplane("XZ").center(cx, cz).circle(r).extrude(-(y1 - y0)).translate((0, y0, 0))


def cyl_z(cx, cy, r, z0, z1):
    return cq.Workplane("XY").center(cx, cy).circle(r).extrude(z1 - z0).translate((0, 0, z0))


def cyl_axis(p0, d, r, L):
    """cylinder from point p0 along unit direction d"""
    return cq.Workplane(cq.Plane(origin=p0, xDir=_perp(d), normal=d)).circle(r).extrude(L)


def _perp(d):
    d = np.array(d, float); a = np.array([1, 0, 0]) if abs(d[0]) < 0.9 else np.array([0, 1, 0])
    v = np.cross(d, a); return tuple(v / np.linalg.norm(v))


def hex_y(cx, cz, af, y0, y1, rot=0):
    """hex nut pocket, axis along Y"""
    return (cq.Workplane("XZ").center(cx, cz).polygon(6, af / math.cos(math.pi / 6)).extrude(-(y1 - y0))
            .translate((0, y0, 0)).rotate((cx, 0, cz), (cx, 1, cz), rot))


# ============================================================ POD
def pod():
    ymid = (POD_Y0 + POD_Y1) / 2
    p = None
    for x, gz in zip(RX, GZ):                                                               # 2 rings round the gearboxes
        ring = cyl_y(x, gz, RING_OD / 2, POD_Y0, POD_Y1)
        p = ring if p is None else p.union(ring)
    p = p.union(prism_y(RX[0], RX[1], GZ[0] - 9, GZ[0] + 9, POD_Y0, POD_Y1))              # web between the rings
    for s in (-1, 1):                                                                       # spring tabs
        x0, x1 = sorted((s * (TAB_X - 30), s * (TAB_X + 8)))
        p = p.union(prism_y(x0, x1, TAB_Z0, TAB_Z1, POD_Y0, POD_Y1))
    for x, gz in zip(RX, GZ):
        p = p.cut(cyl_y(x, gz, RING_ID / 2, POD_Y0 - 1, POD_Y1 + 1))                      # gearbox bore
        for a in (-50, 50):                                                                 # 2 x M3 set screws per gearbox
            d = (math.sin(math.radians(a)), 0.0, math.cos(math.radians(a)))
            t = (d[2], 0.0, -d[0])                                                          # tangent; plane y-axis = +Y
            o = (x + d[0] * (RING_OD / 2 + 1), ymid, gz + d[2] * (RING_OD / 2 + 1))
            p = p.cut(cyl_axis(o, tuple(-c for c in d), 1.65, RING_WALL + 3))
            nc = (x + d[0] * (RING_ID / 2 + 3.0), ymid, gz + d[2] * (RING_ID / 2 + 3.0))
            slot_len = POD_Y1 + 1 - ymid + 2.9                                              # M3 nut slides in from the +Y face
            nut = (cq.Workplane(cq.Plane(origin=nc, xDir=t, normal=d)).center(0, slot_len / 2 - 2.9)
                   .rect(5.8, slot_len).extrude(1.4, both=True))
            p = p.cut(nut)
    for s in (-1, 1):                                                                       # M5 spring bolt: slot along X
        p = p.cut(cq.Workplane("XY").center(s * TAB_X, ymid).slot2D(BOLT_D + 3.5, BOLT_D + 0.5).extrude(40).translate((0, 0, TAB_Z0 - 5)))
        p = p.cut(cyl_z(s * TAB_X, ymid, SPRING_OD / 2, TAB_Z1 - 1.5, TAB_Z1 + 1))           # spring seat
    return p


# ============================================================ HANGER
def hanger():
    ymid = (POD_Y0 + POD_Y1) / 2
    h = prism_y(-BAR_X, BAR_X, BAR_Z0, BAR_Z1, POD_Y0, POD_Y1)
    h = h.union(prism_y(-BAR_X, BAR_X, BAR_Z1 - 3, BAR_Z1, POD_Y0 - 6, POD_Y1 + 6))          # top flange = stiffer, wider seat for the arm
    for s in (-1, 1):
        h = h.cut(cyl_z(s * TAB_X, ymid, BOLT_D / 2, BAR_Z0 - 1, BAR_Z1 + 1))                 # spring bolts
        h = h.cut(cyl_z(s * TAB_X, ymid, SPRING_OD / 2, BAR_Z0 - 1, BAR_Z0 + 1.5))           # spring seat
        for x in (12, 24, 36):                                                              # arm bolt holes (pick the pair that fits)
            h = h.cut(cyl_z(s * x, ymid, BOLT_D / 2, BAR_Z0 - 1, BAR_Z1 + 1))
    return h


# ============================================================ ROLLER (made upright: axis = Z, z=0 is the outer face)
def roller(tpu=False):
    R = ROLL_R if tpu else CORE_R
    r = cq.Workplane("XY").circle(R).extrude(ROLL_W)
    if not tpu:
        for z0 in (0, ROLL_W - 1.2):                                                        # lips keep the sleeve on
            r = r.union(cq.Workplane("XY").circle(R + 0.8).extrude(1.2).translate((0, 0, z0)))
    r = r.cut(cq.Workplane("XY").circle(SHAFT_D / 2 + 0.12).extrude(ROLL_W + 2).translate((0, 0, -1)))
    zm = ROLL_W / 2
    r = r.cut(cyl_axis((R + 1, 0, zm), (-1, 0, 0), 1.65, R + 1))                                # M3 set screw
    r = r.cut(cyl_axis((R + 1, 0, zm), (-1, 0, 0), 3.3, R + 1 - 7.0))                           # head pocket (head below the surface)
    r = r.cut(cq.Workplane("XY").box(2.8, 5.9, zm + 3, centered=False).translate((3.9, -2.95, -1)))  # M3 nut slot, from the outer face
    return r


def stl(w, name):
    p = os.path.join(OUT, name + ".stl"); cq.exporters.export(w, p, tolerance=0.02, angularTolerance=0.15)
    m = trimesh.load(p); m.merge_vertices(); return m


if __name__ == "__main__":
    P, Hg, Ro, Rt = pod(), hanger(), roller(), roller(True)
    m_pod, m_hang, m_rol, m_tpu = stl(P, "pod_assembly_pose"), stl(Hg, "hanger_assembly_pose"), stl(Ro, "roller_core"), stl(Rt, "roller_TPU_25mm")
    # ---- print orientation: pod + hanger lie on their +Y face ----
    def lay(m):
        mm = m.copy(); mm.apply_transform(trimesh.transformations.rotation_matrix(math.pi / 2, [1, 0, 0])); mm.apply_translation(-mm.bounds[0]); return mm
    pp = lay(m_pod)
    hp = m_hang.copy(); hp.apply_transform(trimesh.transformations.rotation_matrix(math.pi, [1, 0, 0])); hp.apply_translation(-hp.bounds[0])
    pp.export(os.path.join(OUT, "pod.stl")); hp.export(os.path.join(OUT, "hanger.stl"))
    print("parts:", {n: [round(v, 1) for v in m.extents] for n, m in (("pod", pp), ("hanger", hp), ("roller", m_rol), ("roller_tpu", m_tpu))},
          "watertight:", all(m.is_watertight for m in (pp, hp, m_rol, m_tpu)))
    print("volumes cm3:", {n: round(m.volume / 1000, 1) for n, m in (("pod", pp), ("hanger", hp), ("roller", m_rol))})
    json.dump(dict(R_T=R_T, TYRE_W=TYRE_W, RX=RX, RZ=RZ, GZ=GZ, TAB_X=TAB_X, TAB_Z=[TAB_Z0, TAB_Z1], BAR_Z=[BAR_Z0, BAR_Z1], BAR_X=BAR_X,
                   POD_Y=[POD_Y0, POD_Y1], YF=YF, GB=[GB_D, GB_T, GB_E], MOT=[MOT_D, MOT_L], ROLL=[CORE_R, ROLL_R, ROLL_W, ROLL_Y0],
                   RIM=[RIM_R, RIM_Y, RIM_D], BOSS=BOSS, SHAFT=[SHAFT_D, SHAFT_L]), open(os.path.join(OUT, "geometry.json"), "w"), indent=1)
