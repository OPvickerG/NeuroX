# Close-up parts + views for the motor side (run from export_scene.py via exec: uses its add, m_of, place, ... ).
# M3 gearbox screws + nuts, M4 lock nuts, roller core + nut + screw + sleeve. Views go into views.json as motor_1..4.


def m3_screw(L):
    h = cq.Workplane("XY").circle(2.85).extrude(1.65).edges(">Z").fillet(1.1)
    h = h.cut(cq.Workplane("XY").polygon(6, 2.0 / math.cos(math.pi / 6)).extrude(2).translate((0, 0, 0.6)))
    return m_of(h.union(cq.Workplane("XY").circle(1.45).extrude(L).translate((0, 0, -L))), 0.02)


def nut(af, t, hole):
    return m_of(cq.Workplane("XY").polygon(6, af / math.cos(math.pi / 6)).extrude(t).translate((0, 0, -t / 2))
                .edges("not |Z").chamfer(0.3).cut(cq.Workplane("XY").circle(hole / 2).extrude(t + 2).translate((0, 0, -t / 2 - 1))), 0.02)


def frame(m, xa, za, at):
    """local X -> xa, local Z -> za (unit vectors, chair coords), then move to `at`"""
    xa, za = np.array(xa, float), np.array(za, float); ya = np.cross(za, xa)
    T = np.eye(4); T[:3, 0], T[:3, 1], T[:3, 2] = xa, ya, za; T[:3, 3] = at
    mm = m.copy(); mm.apply_transform(T); return mm


ymid = (C.POD_Y0 + C.POD_Y1) / 2
N3, N4 = nut(5.5, 2.4, 3.0), nut(7.0, 3.2, 4.0)
M3_12, M3_6 = m3_screw(12), m3_screw(6)
GB = {}                                                                     # (ring, angle) -> (d, head seat, nut centre)
for i, (x, gz) in enumerate(zip(C.RX, C.GZ)):
    for a in (-50, 50):
        d = np.array([math.sin(math.radians(a)), 0.0, math.cos(math.radians(a))])
        c = np.array([x, ymid, gz])
        head = c + d * (C.GB_D / 2 + 12.0)                                  # screw tip on the gearbox
        nc = c + d * (C.RING_ID / 2 + 3.0)
        GB[(i, a)] = (d, head, nc)
        add(f"cu_gbscrew{i}_{a}", place(M3_12, head, -d), "black_steel", f"cu_gbscrew{i}_{a}", ("cu",))
        add(f"cu_gbnut{i}_{a}", frame(N3, [0, 1, 0], d, nc), "steel", "cu_gbnut", ("cu",))
for s in (-1, 1):
    add(f"cu_m4nut{s}", frame(N4, [0, 0, 1], [0, 1, 0], [s * C.TAB_X, C.NUT_Y0 + 1.7, C.GZ[0]]), "steel", "cu_m4nut", ("cu",))
core = trimesh.load(os.path.join(os.path.dirname(HERE), "stl", "roller_core.stl"))
for i, (x, z) in enumerate(zip(C.RX, C.RZ)):
    T = np.eye(4); T[:3, :3] = [[0, 1, 0], [0, 0, 1], [1, 0, 0]]; T[:3, 3] = [x, C.ROLL_Y0, z]
    m = core.copy(); m.apply_transform(T)
    add(f"cu_roller{i}", m, "spool", f"cu_roller{i}", ("cu",))
    add(f"cu_sleeve{i}", ring_y(x, z, C.CORE_R + 0.05, C.ROLL_R, C.ROLL_Y0 + 1.25, C.ROLL_Y0 + C.ROLL_W - 1.25), "rubber", f"cu_sleeve{i}", ("cu",))
    add(f"cu_rnut{i}", frame(N3, [0, 1, 0], [0, 0, 1], [x, C.ROLL_Y0 + C.ROLL_W / 2, z + 5.3]), "steel", f"cu_rnut{i}", ("cu",))
    add(f"cu_rscrew{i}", place(M3_6, [x, C.ROLL_Y0 + C.ROLL_W / 2, z + 8.5], [0, 0, -1]), "black_steel", f"cu_rscrew{i}", ("cu",))

V3 = lambda v: [float(t) for t in v]
GBS = [f"cu_gbscrew{i}_{a}" for i in range(2) for a in (-50, 50)]
x1, z1 = C.RX[1], C.RZ[1]
yr = C.ROLL_Y0 + C.ROLL_W / 2
T_ = lambda t, o: [float(t[0] + o[0]), float(t[1] + o[1]), float(t[2] + o[2])]
t1, t2, t3, t4 = [45, 35, 328], [33, 35, 336], [33, -18, 318], [62, 50, 318]
W_ = C.ROLL_W
CU = {
 "motor_1_nuts": {"scene": "cu", "title": "1  Nuts into the pod, before the motors (both rings, both ends)", "only": ["pod", "cu_gbnut", "cu_m4nut"],
   "offsets": {"cu_gbnut": [0, 24, 0], "cu_m4nut": [0, 0, 30]}, "cam": T_(t1, (150, 300, 260)), "tgt": t1, "fov": 15,
   "arrows": [[V3(GB[(1, a)][2] + [0, 22, 0]), V3(GB[(1, a)][2] + [0, 5, 0])] for a in (-50, 50)]
             + [[[C.TAB_X, C.NUT_Y0 + 1.7, C.GZ[0] + 26], [C.TAB_X, C.NUT_Y0 + 1.7, C.TAB_Z1 + 3]]],
   "labels": [[V3(GB[(1, 50)][2]), "M3 nut: push into the slot in the outer face\n(2 per ring, 4 in total)", 170, -170, "cu_gbnut"],
              [[C.TAB_X, C.NUT_Y0 + 1.7, C.GZ[0] + 4], "M4 nut: drop into the slot on top\n(1 each end). It falls in line with the hole.", -40, 200, "cu_m4nut"]]},
 "motor_2_screws": {"scene": "cu", "title": "2  Motor in (shaft at the BOTTOM), 4 x M3 x 12 onto the gearbox", "only": ["pod", "motor", "cu_gbnut", "cu_m4nut"] + GBS,
   "offsets": {g: V3(GB[(int(g[10]), int(g.split("_")[-1]))][0] * 16) for g in GBS}, "cam": T_(t2, (120, 300, 300)), "tgt": t2, "fov": 15,
   "arrows": [[V3(GB[k][1] + GB[k][0] * 27), V3(GB[k][1] + GB[k][0] * 4)] for k in GB if k[0] == 1],
   "labels": [[V3(GB[(1, 50)][1] + GB[(1, 50)][0] * 16), "M3 x 12: turn in until it presses the\ngearbox, then 1/4 turn more", 120, -110, None],
              [V3(GB[(1, -50)][1] + GB[(1, -50)][0] * 16), "2 per motor, into the nuts\nfrom step 1", -150, -110, None],
              [[x1, C.YF + C.GB_T + 25, C.GZ[1] + 18], "both motors the same way round", 130, 70, None]]},
 "motor_3_roller": {"scene": "cu", "title": "3  Rollers onto the shafts (seen from the tyre side)",
   "only": ["pod", "motor", "cu_roller0", "cu_sleeve0", "cu_rnut0", "cu_rscrew0", "cu_roller1", "cu_sleeve1", "cu_rnut1", "cu_rscrew1"] + GBS,
   "offsets": {"cu_roller1": [0, -40, 0], "cu_rnut1": [0, -60, 0], "cu_rscrew1": [0, -40, 26], "cu_sleeve1": [0, -95, 0]},
   "cam": T_(t3, (160, -320, 170)), "tgt": [33, -30, 312], "fov": 19,
   "arrows": [[[x1, C.ROLL_Y0 - 40, z1 - C.ROLL_R - 5], [x1, C.ROLL_Y0 - 4, z1 - C.ROLL_R - 5]],
              [[x1, yr - 60, z1 + 5.3], [x1, yr - 50, z1 + 5.3]],
              [[x1, yr - 40, z1 + 8.5 + 30], [x1, yr - 40, z1 + C.ROLL_R + 3]],
              [[x1, C.ROLL_Y0 - 95 + W_ + 2, z1 + C.ROLL_R + 6], [x1, C.ROLL_Y0 - 44, z1 + C.ROLL_R + 6]]],
   "labels": [[[x1, C.ROLL_Y0 - 40 + W_ / 2, z1 - C.ROLL_R], "a  roller core onto the shaft\n    (shaft tip flush with the end)", 120, 110, None],
              [[x1, yr - 60, z1 + 5.3], "b  M3 nut into the slot in the end", -230, 40, None],
              [[x1, yr - 40, z1 + 8.5 + 26], "c  turn the shaft so its FLAT faces up,\n    M3 x 6 down through the nut, tight", 120, -100, None],
              [[x1, C.ROLL_Y0 - 95 + W_ / 2, z1 - C.ROLL_R], "d  last: stretch the inner-tube sleeve\n    over the core (it covers the screw)", 190, 10, None],
              [[C.RX[0], C.ROLL_Y0 + W_ / 2, C.RZ[0] + C.ROLL_R], "finished roller", -150, -110, None]]},
 "motor_4_lock": {"scene": "cu", "title": "4  Pod into the legs: spool, washer, bolt (both legs)",
   "only": ["pod", "motor", "hanger", "cu_roller0", "cu_roller1", "cu_sleeve0", "cu_sleeve1", "lockhw_spool", "lockhw_washer", "lockhw_screw",
            "leghw_spool", "leghw_washer", "leghw_screw"] + GBS,
   "offsets": {"lockhw_spool": [0, 16, 0], "lockhw_washer": [0, 30, 0], "lockhw_screw": [0, 46, 0],
               "leghw_spool": [0, 16, 0], "leghw_washer": [0, 30, 0], "leghw_screw": [0, 46, 0]},
   "cam": T_(t4, (260, 330, 120)), "tgt": t4, "fov": 17,
   "arrows": [[[C.TAB_X, C.SPOOL_Y1 + 75, z_], [C.TAB_X, C.SPOOL_Y1 + 52, z_]] for z_ in (C.GZ[0], C.FOOT_Z)],
   "labels": [[[C.TAB_X, C.SPOOL_Y1 + 48, C.GZ[0] + 3], "M4 x 25 + washer + spool into the M4 nut\nin the pod: just touching, then back 1/2 turn", 110, -120, None],
              [[C.TAB_X, C.SPOOL_Y1 + 48, C.FOOT_Z - 3], "M4 x 16 + washer + spool into the\nsmall hole in the leg, tight", 110, 100, None],
              [[C.TAB_X, C.LEG_Y1, C.GZ[0] + 10], "pod boss in the leg slot", -200, -130, None]]},
}
_vp = os.path.join(HERE, "views.json"); _v = json.load(open(_vp)); _v.update(CU); json.dump(_v, open(_vp, "w"), indent=1)
