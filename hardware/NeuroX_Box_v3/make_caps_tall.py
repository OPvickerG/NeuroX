"""NeuroX box v3.1 - longer button caps for SHORTER tactile switches.
The v3 caps were made for 12x12x7.3 mm switches. A shorter switch leaves the plunger
hanging above the button, so the press never reaches it.
SW_H = bottom of the black switch body -> top of its button (pins NOT counted).

Outputs (build/caps_fix/):
  NeuroX_Box_v3.1_cap_SIZER" + os.environ.get("CAP_TAG", "") + ".3mf            5 gray test caps, 1-5 dots = 4.6 / 5.0 / 5.4 / 5.8 / 6.2 mm
  NeuroX_Box_v3.1_caps_for_<H>mm_switch.3mf 3 gray caps + 1 red STOP cap for that height
Run: python3 make_caps_fix.py"""
import os, sys, trimesh
import cadquery as cq
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, HERE)
import box_buttons_cq as BB
import make_3mf_v3 as M

import json; HEIGHTS = json.loads(os.environ.get("CAP_HEIGHTS", "[4.6, 5.0, 5.4, 5.8, 6.2]"))
OUT = os.path.join(HERE, "build", "caps_fix"); os.makedirs(OUT, exist_ok=True)

def stl(w, name):
    p = os.path.join(OUT, name + ".stl")
    cq.exporters.export(BB.on_bed(w, flip=True), p, tolerance=0.01, angularTolerance=0.1)
    m = trimesh.load(p); m.merge_vertices(); return m

def item(name, mesh, col): return (name, [(col, mesh)])
def row(items, pitch, x0, y): return [M.place(it, x0 + i * pitch, y) for i, it in enumerate(items)]

# ---- sizer ----
siz = [item(f"sizer_{i+1}dot_{h}mm", stl(BB.cap(sw_h=h, dots=i + 1), f"cap_sizer_{i+1}dot_{h}mm"), "MG") for i, h in enumerate(HEIGHTS)]
p = os.path.join(OUT, "NeuroX_Box_v3.1_cap_SIZER" + os.environ.get("CAP_TAG", "") + ".3mf")
M.write_3mf(p, row(siz, 22, 46, 90), "NeuroX box v3.1 - cap sizer: " + " / ".join(f"{i+1} dot {h} mm" for i, h in enumerate(HEIGHTS)))
print(os.path.basename(p))

# ---- one reprint plate per height ----
for h in HEIGHTS:
    cap = stl(BB.cap(sw_h=h), f"cap_for_{h}mm_switch")
    sc = stl(BB.cap(True, sw_h=h), f"stop_cap_for_{h}mm_switch")
    objs = row([item(f"button_cap_{i+1} ({h} mm switch)", cap, "MG") for i in range(3)] +
               [item(f"STOP_cap ({h} mm switch)", sc, "RD")], 24, 54, 90)
    p = os.path.join(OUT, f"NeuroX_Box_v3.1_caps_for_{h}mm_switch.3mf")
    M.write_3mf(p, objs, f"NeuroX box v3.1 - 3 caps + STOP cap for a {h} mm switch (body bottom to button top)")
    print(os.path.basename(p))

# ---- check: plunger tip vs switch button, per height ----
for h in HEIGHTS:
    c = BB.cap(sw_h=h); bb = c.val().BoundingBox()
    act = BB.Z_FLOOR + h
    print(f"{h} mm switch: plunger tip z {bb.zmin:+.2f}, button top z {act:+.2f}, rest gap {bb.zmin - act:.2f} mm, "
          f"plunger {(-1.6 - 1.2) - bb.zmin:.1f} mm long (v3: {(-1.6 - 1.2) - (BB.Z_ACT + BB.GAP):.1f})")
