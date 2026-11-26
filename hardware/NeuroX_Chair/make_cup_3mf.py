"""Print plate: the 2 CUP rollers for one wheel (print it twice). Run after chair_drive_cq.py."""
import os, sys, trimesh
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "NeuroX_Box_v3"))
import make_3mf_v3 as M

cup = trimesh.load(os.path.join(HERE, "stl", "roller_cup.stl")); cup.merge_vertices()
M.write_3mf(os.path.join(HERE, "NeuroX_Chair_CUP_rollers_ONE_WHEEL.3mf"),
            [M.place(("cup_roller_1", [("MG", cup)]), 65, 90), M.place(("cup_roller_2", [("MG", cup)]), 115, 90)],
            "NeuroX chair - 2 CUP rollers for one wheel (print 2x). Upright as placed, no supports. PETG or PLA, 4 walls, 40% infill. "
            "M3 nut + M3 x 8 set screw each; rubber bands in the 5 grooves.")
print("cup plate written")
