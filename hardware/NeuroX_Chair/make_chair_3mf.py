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

# ---- frame mount (arm v2, armrest tube): arm + clamp halves, one file per tube size ----
arm = ld("arm")
for D in (22, 25, 28):
    M.write_3mf(os.path.join(HERE, f"NeuroX_Chair_mount_ONE_WHEEL_{D}mm_armrest.3mf"),
                [M.place(("chair_arm", [("MG", arm)]), 52, 90),
                 M.place((f"clamp_{D}mm_outer", [("MG", ld(f"clamp_{D}mm_outer"))]), 135, 50),
                 M.place((f"clamp_{D}mm_inner", [("MG", ld(f"clamp_{D}mm_inner"))]), 135, 130)],
                f"NeuroX chair mount - one wheel, {D} mm armrest tube: arm + 2 clamp halves (print 2x). PETG or PLA, 5 walls, 40% infill")
M.write_3mf(os.path.join(HERE, "NeuroX_Chair_tube_sizer.3mf"), [M.place(("tube_sizer", [("MG", ld("tube_sizer"))]), 90, 90)],
            "Tube sizer: smallest hole that slides over the frame tube. Notches: 1=19 2=20 3=22 4=25 5=28 6=30 7=32 mm")
print("mount plates written")
