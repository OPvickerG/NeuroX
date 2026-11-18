"""NeuroX box v3: button + small-part plates, ONE colour per plate (no AMS swaps)."""
import os, math, trimesh, numpy as np
import make_3mf_v3 as M
HERE = os.path.dirname(os.path.abspath(__file__))
BT = os.path.join(HERE, "build", "buttons"); OUT = os.path.join(HERE, "build", "bambu_A1mini_AMS")
def ld(path):
    m = trimesh.load(path); m.merge_vertices(); return m
def item(name, path, col): return (name, [(col, ld(path))])
def grid(items, cols, pitch, x0, y0):
    return [M.place(it, x0 + (i % cols) * pitch[0], y0 + (i // cols) * pitch[1]) for i, it in enumerate(items)]
S = os.path.join(HERE, "build", "stl")
gray = ([item(f"button_guide_{i+1}", f"{BT}/guide.stl", "MG") for i in range(4)] +
        [item(f"switch_cradle_{i+1}", f"{BT}/cradle.stl", "MG") for i in range(4)] +
        [item(f"button_cap_{i+1}", f"{BT}/cap.stl", "MG") for i in range(3)] +
        [item(f"screen_clamp_tab_{i+1}", f"{S}/clamp_tab__MG.stl", "MG") for i in range(4)])
red = [item("stop_guard", f"{S}/stop_guard__RD.stl", "RD"), item("stop_cap", f"{BT}/stop_cap.stl", "RD"), item("stop_top", f"{BT}/stop_top.stl", "RD")]
blue = [item("sleeve_grommet", f"{S}/sleeve_grommet__BL.stl", "BL")]
P = [("4_buttons_gray", "4 button guides + 4 switch cradles + 3 caps + 4 screen clamp tabs (midnight gray)", grid(gray, 4, (34, 30), 30, 30)),
     ("5_stop_red", "STOP guard + STOP cap + STOP top (red)", grid(red, 3, (40, 40), 50, 90)),
     ("6_grommet_blue", "cable sleeve grommet (blue)", grid(blue, 1, (40, 40), 90, 90))]
for key, title, objs in P:
    path = os.path.join(OUT, f"NeuroX_Box_v3_plate{key}.3mf"); M.write_3mf(path, objs, "NeuroX box v3 - " + title)
    ms = [m for _, parts in objs for _, m in parts]; trimesh.util.concatenate(ms).export(os.path.join(HERE, "img", f"bplate{key}.stl"))
    vol = sum(abs(m.volume) for m in ms) / 1000
    print(os.path.basename(path), len(objs), "objects", round(vol, 2), "cm3")
