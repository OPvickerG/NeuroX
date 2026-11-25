"""NeuroX chair - top friction drive for a manual wheelchair, 2 Johnson 12 V 500 RPM motors per wheel.

  POD     holds the 2 motors by their gearboxes (2 x M3 set screws each). Its 2 rollers sit on top of the tyre.
          A boss on each end slides up and down in a slot in the hanger leg.
  HANGER  bar above the pod + 2 legs. No springs: bicycle inner-tube bands, from a SPOOL on each pod boss
          down to a spool on the leg below, pull the rollers onto the tyre.
          LOCK BOLT (M4 x 25 through the boss spool): loose = the pod slides, rollers drive.
          Lift the pod and tighten it = the pod is clamped to the leg, rollers off the tyre (push by hand).
  SPOOL   4 per wheel: 2 on the lock bolts (move with the pod), 2 on M4 x 16 screws in the legs (fixed).
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
BAR_T, BAR_GAP = 12.0, 14.0                  # hanger bar thickness, gap above the rings
# ---------------- M4 kit hardware (button head Allen, hex nuts, washers) ----------------
M4, M4_PILOT, M4_HEAD = 4.4, 3.6, 7.6       # clearance hole, hole a screw cuts its own thread in, head diameter
M4_NUT_AF, M4_NUT_T = 7.2, 3.4              # nut pocket (nut 7.0 x 3.2)
WASHER_T = 0.8
# ---------------- pod guide, legs, bands ----------------
BOSS_D, SLOT_W = 8.0, 8.6                   # boss on the pod tab, slot in the leg
LEG_T, LEG_W = 6.0, 5.0                     # leg thickness (Y), material beside the slot
TRAV_DN, TRAV_UP = 6.0, 12.0                # pod travel below / above the "rollers just touching" pose
SPOOL_L, SPOOL_CORE, SPOOL_FL, SPOOL_FLT = 9.0, 5.5, 9.0, 1.2   # spool length, core radius, flange radius, flange thickness
FOOT_DZ = 27.0                              # band span (boss to leg spool) when driving
ARM_HOLE_X = 36.0                           # arm deck screws (M4 x 12) go into the bar here

R_C = R_T + ROLL_R                            # roller centre radius
RX = [R_C * math.sin(math.radians(a)) for a in (-PHI, PHI)]
RZ = [R_C * math.cos(math.radians(a)) for a in (-PHI, PHI)]
GZ = [z + GB_E for z in RZ]                   # gearbox centres (shaft at the bottom of the gearbox)
TAB_X = max(abs(x) for x in RX) + RING_OD / 2 + 11.0
TAB_Z0, TAB_Z1 = GZ[0] - 6.0, GZ[0] + 6.0
BAR_Z0 = GZ[0] + RING_OD / 2 + BAR_GAP; BAR_Z1 = BAR_Z0 + BAR_T
LEG_Y0 = POD_Y1 + 0.6; LEG_Y1 = LEG_Y0 + LEG_T   # leg, just outboard of the pod
LEG_HX = SLOT_W / 2 + LEG_W                    # leg half width (X) around the slot
BAR_X = TAB_X + LEG_HX
FOOT_Z = GZ[0] - FOOT_DZ                        # leg spool (band anchor) centre
LEG_Z0 = FOOT_Z - SPOOL_FL + 1.5                # leg bottom
BOSS_Y1 = LEG_Y1 - 0.5                          # boss 0.5 shorter than the leg: the lock bolt clamps the leg
SPOOL_Y1 = LEG_Y1 + SPOOL_L                     # spool outer face (both spools sit on the leg face)
LOCK_L, PIN_L, DECK_L = 25, 16, 12              # screw lengths
LOCK_TIP = SPOOL_Y1 + WASHER_T - LOCK_L         # lock bolt tip (Y)
NUT_Y0 = LOCK_TIP + 0.6                         # lock nut pocket in the pod tab, Y NUT_Y0 .. + M4_NUT_T


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
    for s in (-1, 1):                                                                       # end tabs (guide boss + lock nut)
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
    for s in (-1, 1):                                                                       # guide boss + lock bolt
        x = s * TAB_X
        p = p.union(cyl_y(x, GZ[0], BOSS_D / 2, POD_Y1 - 0.01, BOSS_Y1))
        p = p.cut(cyl_y(x, GZ[0], M4 / 2, POD_Y0 - 1, BOSS_Y1 + 1))
        p = p.cut(prism_y(x - M4_NUT_AF / 2, x + M4_NUT_AF / 2, GZ[0] - 4.1, TAB_Z1 + 1, NUT_Y0, NUT_Y0 + M4_NUT_T))  # nut drops in from the top
    return p


# ============================================================ HANGER
def hanger():
    ymid = (POD_Y0 + POD_Y1) / 2
    h = prism_y(-BAR_X, BAR_X, BAR_Z0, BAR_Z1, POD_Y0, LEG_Y1)
    h = h.union(prism_y(-BAR_X, BAR_X, BAR_Z1 - 3, BAR_Z1, POD_Y0 - 6, LEG_Y1))            # top flange = stiffer, wider seat for the arm
    for s in (-1, 1):
        x = s * TAB_X
        h = h.union(prism_y(x - LEG_HX, x + LEG_HX, LEG_Z0, BAR_Z0 + 0.01, LEG_Y0, LEG_Y1))  # leg
        xo = s * BAR_X - (6 if s > 0 else 0)                                               # gusset leg <-> bar, outer edge
        g = cq.Workplane("YZ").polyline([(LEG_Y0 + 0.01, BAR_Z0 + 0.01), (POD_Y0 + 7, BAR_Z0 + 0.01), (LEG_Y0 + 0.01, BAR_Z0 - 18)]).close().extrude(6)
        h = h.union(g.translate((xo, 0, 0)))
        h = h.cut(prism_y(x - SLOT_W / 2, x + SLOT_W / 2, GZ[0] - TRAV_DN, GZ[0] + TRAV_UP, LEG_Y0 - 1, LEG_Y1 + 1))   # guide slot
        for z in (GZ[0] - TRAV_DN, GZ[0] + TRAV_UP):
            h = h.cut(cyl_y(x, z, SLOT_W / 2, LEG_Y0 - 1, LEG_Y1 + 1))
        h = h.cut(cyl_y(x, FOOT_Z, M4_PILOT / 2, LEG_Y0 - 1, LEG_Y1 + 1))                  # leg spool screw (cuts its own thread)
        h = h.cut(cyl_z(s * ARM_HOLE_X, ymid, M4_PILOT / 2, BAR_Z0 - 1, BAR_Z1 + 1))       # arm deck screw (cuts its own thread)
    return h


def spool():
    """band spool, made upright: z = 0 is the flange that sits on the leg"""
    sp = cq.Workplane("XY").circle(SPOOL_FL).extrude(SPOOL_FLT)
    sp = sp.union(cq.Workplane("XY").circle(SPOOL_CORE).extrude(SPOOL_L))
    cone_h = SPOOL_FL - SPOOL_CORE                                                          # 45 degree outer lip, prints without support
    sp = sp.union(cq.Workplane("XY").circle(SPOOL_CORE).workplane(offset=cone_h).circle(SPOOL_FL).loft().translate((0, 0, SPOOL_L - cone_h - 0.6)))
    sp = sp.union(cq.Workplane("XY").circle(SPOOL_FL).extrude(0.6).translate((0, 0, SPOOL_L - 0.6)))
    return sp.cut(cq.Workplane("XY").circle(M4 / 2 + 0.05).extrude(SPOOL_L + 2).translate((0, 0, -1)))


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
    m_sp = stl(spool(), "spool")
    # ---- print orientation: pod + hanger lie on their +Y face ----
    def lay(m):
        mm = m.copy(); mm.apply_transform(trimesh.transformations.rotation_matrix(math.pi / 2, [1, 0, 0])); mm.apply_translation(-mm.bounds[0]); return mm
    pp = lay(m_pod)
    hp = m_hang.copy(); hp.apply_transform(trimesh.transformations.rotation_matrix(-math.pi / 2, [1, 0, 0])); hp.apply_translation(-hp.bounds[0])  # outer face on the bed
    pp.export(os.path.join(OUT, "pod.stl")); hp.export(os.path.join(OUT, "hanger.stl"))
    print("parts:", {n: [round(v, 1) for v in m.extents] for n, m in (("pod", pp), ("hanger", hp), ("roller", m_rol), ("roller_tpu", m_tpu), ("spool", m_sp))},
          "watertight:", all(m.is_watertight for m in (pp, hp, m_rol, m_tpu, m_sp)))
    print("volumes cm3:", {n: round(m.volume / 1000, 1) for n, m in (("pod", pp), ("hanger", hp), ("roller", m_rol))})
    json.dump(dict(FOOT_Z=FOOT_Z, LEG_Y=[LEG_Y0, LEG_Y1], LEG_Z0=LEG_Z0, R_T=R_T, TYRE_W=TYRE_W, RX=RX, RZ=RZ, GZ=GZ, TAB_X=TAB_X, TAB_Z=[TAB_Z0, TAB_Z1], BAR_Z=[BAR_Z0, BAR_Z1], BAR_X=BAR_X,
                   POD_Y=[POD_Y0, POD_Y1], YF=YF, GB=[GB_D, GB_T, GB_E], MOT=[MOT_D, MOT_L], ROLL=[CORE_R, ROLL_R, ROLL_W, ROLL_Y0],
                   RIM=[RIM_R, RIM_Y, RIM_D], BOSS=BOSS, SHAFT=[SHAFT_D, SHAFT_L]), open(os.path.join(OUT, "geometry.json"), "w"), indent=1)
