"""Motor fit check (about 15 min): a thin slice of the pod's gearbox ring + a thin roller.
  RING:   push it over the motor GEARBOX (the big round part). Should slide on with a little play;
          the 2 M3 screws then hold it. Too tight / too loose -> tell me, the pod bore is one number (GB_D).
  ROLLER: push it on the motor SHAFT. Should slide on snug; the M3 screw locks it on the flat.
Same bore sizes as the real pod and roller in chair_drive_cq.py."""
import math, os, sys, trimesh
import cadquery as cq
import chair_drive_cq as C
sys.path.insert(0, os.path.join(C.HERE, "..", "NeuroX_Box_v3"))
import make_3mf_v3 as M

T = 8.0
ring = cq.Workplane("XY").circle(C.RING_OD / 2).circle(C.RING_ID / 2).extrude(T)
for a in (-50, 50):                                         # same 2 x M3 set screws as the pod
    d = (math.sin(math.radians(a)), math.cos(math.radians(a)))
    ring = ring.cut(cq.Workplane("XY").box(30, 3.3, 3.3).rotate((0, 0, 0), (0, 0, 1), 90 - a)
                    .translate((d[0] * (C.RING_OD / 2), d[1] * (C.RING_OD / 2), T / 2)))
    nc = (d[0] * (C.RING_ID / 2 + 3.0), d[1] * (C.RING_ID / 2 + 3.0))
    ring = ring.cut(cq.Workplane("XY").box(2.8, 5.8, T, centered=(True, True, False)).rotate((0, 0, 0), (0, 0, 1), 90 - a)
                    .translate((nc[0], nc[1], T / 2)))      # nut slot from the top face
ring = ring.union(cq.Workplane("XY").text(f"{C.GB_D:g}", 5, 0.8, halign="center").translate((0, -(C.RING_OD / 2 - 3.5), T)))
roll = C.roller().intersect(cq.Workplane("XY").box(40, 40, 10, centered=(True, True, False)))
out = {}
for n, w in (("motor_fitcheck_ring", ring), ("motor_fitcheck_roller", roll)):
    p = os.path.join(C.OUT, n + ".stl"); cq.exporters.export(w, p, tolerance=0.02, angularTolerance=0.15)
    m = trimesh.load(p); m.merge_vertices(); m.apply_translation(-m.bounds[0]); m.export(p); out[n] = m
    print(n, [round(v, 1) for v in m.extents], "watertight", m.is_watertight)
M.write_3mf(os.path.join(C.HERE, "NeuroX_Chair_MOTOR_fitcheck.3mf"),
            [M.place(("gearbox_ring_test", [("MG", out["motor_fitcheck_ring"])]), 70, 90),
             M.place(("roller_test", [("MG", out["motor_fitcheck_roller"])]), 125, 90)],
            "Motor fit check: ring over the gearbox, roller on the shaft (about 15 min)")
print("bore for gearbox", C.RING_ID, "mm; roller bore", C.SHAFT_D + 0.24, "mm")
