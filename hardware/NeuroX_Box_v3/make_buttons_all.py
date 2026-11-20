"""NeuroX box v3.1 - ALL button parts in one Bambu Studio file (A1 mini + AMS lite, 180 x 180 bed).
  gray: 4 button guides, 4 switch cradles, 3 caps (9.0 mm switch, caps 7 mm above the lid)
  red : STOP cap (9.0 mm switch), STOP top, STOP guard
Run: python3 make_buttons_all.py   ->  caps_fix_v3.1/NeuroX_Box_v3.1_buttons_ALL_9.0mm_switch.3mf"""
import os, sys, trimesh
import cadquery as cq
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, HERE)
import box_buttons_cq as BB
import make_3mf_v3 as M

CAPS = os.path.join(HERE, "caps_fix_v3.1"); TMP = os.path.join(HERE, "build", "buttons_all"); os.makedirs(TMP, exist_ok=True)
SW = "9.0mm_switch_7mm_high"


def ld(p):
    m = trimesh.load(p); m.merge_vertices(); return m


def gen(name):
    p = os.path.join(TMP, name + ".stl")
    cq.exporters.export(BB.PRINT[name](), p, tolerance=0.01, angularTolerance=0.1)
    return ld(p)


guide, cradle, stop_top = gen("guide"), gen("cradle"), gen("stop_top")
cap = ld(f"{CAPS}/cap_for_{SW}.stl")
stop_cap = ld(f"{CAPS}/stop_cap_for_{SW}.stl")
stop_guard = ld(os.path.join(HERE, "3_STL", "stop_guard__RD.stl"))

for n, m in (("guide", guide), ("cradle", cradle), ("cap", cap), ("stop_cap", stop_cap), ("stop_top", stop_top), ("stop_guard", stop_guard)):
    e = m.extents; print(f"{n:10s} {e[0]:5.1f} x {e[1]:5.1f} x {e[2]:5.1f} mm  watertight={m.is_watertight}")


def it(name, mesh, col): return (name, [(col, mesh.copy())])


P = 36   # pitch
objs = []
objs += [M.place(it(f"button_guide_{i+1}", guide, "MG"), 36 + i * P, 36) for i in range(4)]
objs += [M.place(it(f"switch_cradle_{i+1}", cradle, "MG"), 36 + i * P, 74) for i in range(4)]
objs += [M.place(it(f"button_cap_{i+1} (9.0 mm switch, 7 mm high)", cap, "MG"), 36 + i * P, 108) for i in range(3)]
objs += [M.place(it("STOP_cap (9.0 mm switch)", stop_cap, "RD"), 36 + 3 * P, 108)]
objs += [M.place(it("STOP_top", stop_top, "RD"), 54, 146), M.place(it("STOP_guard", stop_guard, "RD"), 108, 146)]

ms = [m for _, parts in objs for _, m in parts]
lo = min(m.bounds[0][0] for m in ms), min(m.bounds[0][1] for m in ms)
hi = max(m.bounds[1][0] for m in ms), max(m.bounds[1][1] for m in ms)
print(f"layout {lo[0]:.1f}..{hi[0]:.1f} x {lo[1]:.1f}..{hi[1]:.1f} mm on a 180 x 180 bed")
assert lo[0] > 5 and lo[1] > 5 and hi[0] < 175 and hi[1] < 175
# no two parts overlapping
bx = [(m.bounds[0][:2], m.bounds[1][:2]) for m in ms]
for i in range(len(bx)):
    for j in range(i + 1, len(bx)):
        (a0, a1), (b0, b1) = bx[i], bx[j]
        assert a1[0] + 2 < b0[0] or b1[0] + 2 < a0[0] or a1[1] + 2 < b0[1] or b1[1] + 2 < a0[1], (i, j)
out = os.path.join(CAPS, "NeuroX_Box_v3.1_buttons_ALL_9.0mm_switch.3mf")
M.write_3mf(out, objs, "NeuroX box v3.1 - all button parts: 4 guides, 4 cradles, 3 caps (gray) + STOP cap, top, guard (red), 9.0 mm switches")
trimesh.util.concatenate(ms).export(os.path.join(TMP, "plate_preview.stl"))
print(os.path.basename(out), len(objs), "objects", round(sum(abs(m.volume) for m in ms) / 1000, 2), "cm3")
