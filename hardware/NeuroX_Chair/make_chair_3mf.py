"""Print plates for the chair drive (Bambu A1 mini, 180 x 180). Run after chair_drive_cq.py."""
import os, sys, trimesh
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "NeuroX_Box_v3"))
import make_3mf_v3 as M


def ld(n):
    m = trimesh.load(os.path.join(HERE, "stl", n + ".stl")); m.merge_vertices(); return m


import chair_drive_cq as C
pod, hang, rol, spl = ld("pod"), ld("hanger"), ld("roller_core"), ld("spool")
HX, HY = 90.0, 135.0                                                     # hanger centre on the plate
small = [("roller_1", rol, 70, 110), ("roller_2", rol, 110, 110)] + [(f"spool_{k + 1}", spl, 50 + 25 * k, 145) for k in range(4)]
# rollers + spools go in the empty space between the hanger legs, under the bar
wx = C.TAB_X - C.LEG_HX; y0 = HY - hang.extents[1] / 2; y1 = y0 + (C.BAR_Z0 - C.LEG_Z0)
for n, m, x, y in small:
    assert HX - wx + 2 < x - m.extents[0] / 2 and x + m.extents[0] / 2 < HX + wx - 2 and y0 + 2 < y - m.extents[1] / 2 and y + m.extents[1] / 2 < y1 - 2, n
M.write_3mf(os.path.join(HERE, "NeuroX_Chair_drive_ONE_WHEEL.3mf"),
            [M.place(("chair_pod", [("MG", pod)]), 90, 27), M.place(("chair_hanger", [("MG", hang)]), HX, HY)]
            + [M.place((n, [("MG", m)]), x, y) for n, m, x, y in small],
            "NeuroX chair drive - one wheel: pod, hanger, 2 rollers, 4 band spools (print 2x). PETG or PLA, 5 walls, 40% infill")
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
