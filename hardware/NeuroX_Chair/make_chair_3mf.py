"""Print plates for the chair drive (Bambu A1 mini, 180 x 180). Run after chair_drive_cq.py."""
import os, sys, trimesh
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "NeuroX_Box_v3"))
import make_3mf_v3 as M


def ld(n):
    m = trimesh.load(os.path.join(HERE, "stl", n + ".stl")); m.merge_vertices(); return m


pod, hang, rol = ld("pod"), ld("hanger"), ld("roller_core")
M.write_3mf(os.path.join(HERE, "NeuroX_Chair_drive_ONE_WHEEL.3mf"),
            [M.place(("chair_pod", [("MG", pod)]), 90, 48), M.place(("chair_hanger", [("MG", hang)]), 90, 118),
             M.place(("roller_1", [("MG", rol)]), 60, 158), M.place(("roller_2", [("MG", rol)]), 120, 158)],
            "NeuroX chair drive - one wheel: pod, hanger, 2 rollers (print 2x). PETG or PLA, 5 walls, 40% infill")
M.write_3mf(os.path.join(HERE, "NeuroX_Chair_rollers_TPU_x4.3mf"),
            [M.place((f"roller_TPU_{i + 1}", [("MG", ld("roller_TPU_25mm"))]), 50 + i * 30, 90) for i in range(4)],
            "NeuroX chair - 4 solid TPU rollers (only if you print TPU; else use the PLA cores + inner tube)")
print("plates written")
