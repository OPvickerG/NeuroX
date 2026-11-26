"""Exports the chair drive as coloured meshes for the realistic three.js renders (render3d/render.js).
Writes render3d/out/*.stl + scenes.json. Coordinates: chair (wheel centre = origin, X forward, Y outboard, Z up), mm."""
import json, math, os, sys
import numpy as np, trimesh, cadquery as cq
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, os.path.dirname(HERE))
import chair_drive_cq as C
import chair_mount_cq as Mt
import chair_check as K

OUT = os.path.join(HERE, "out"); os.makedirs(OUT, exist_ok=True)
MAT = {  # colour, metalness, roughness
    "pod": ("#2f6fe0", 0.0, 0.55), "hanger": ("#f07d2a", 0.0, 0.55), "arm": ("#2fa35a", 0.0, 0.55), "clamp": ("#8e4fd0", 0.0, 0.55),
    "spool": ("#45474d", 0.0, 0.6), "rubber": ("#1d1d20", 0.0, 0.9), "band": ("#202023", 0.0, 0.85), "steel": ("#d5d9de", 0.55, 0.35),
    "black_steel": ("#3a3c41", 0.45, 0.4), "gearbox": ("#c8b37a", 0.55, 0.35), "can": ("#d7dade", 0.55, 0.3), "cap": ("#26272a", 0.0, 0.6),
    "chrome": ("#dfe3e8", 0.5, 0.25), "mag": ("#80858d", 0.35, 0.45), "foam": ("#2e2f33", 0.0, 0.95), "tyre": ("#222225", 0.0, 0.88),
}


def m_of(w, tol=0.08):
    p = os.path.join(OUT, "_tmp.stl"); cq.exporters.export(w, p, tolerance=tol, angularTolerance=0.12); return trimesh.load(p)


def screw_mesh(L, head=True):
    """M4 button head screw: head on +Z side of z = 0, shank down to z = -L"""
    h = cq.Workplane("XY").circle(3.8).extrude(2.2).edges(">Z").fillet(1.5)
    h = h.cut(cq.Workplane("XY").polygon(6, 2.5 / math.cos(math.pi / 6)).extrude(3).translate((0, 0, 0.8)))
    s = cq.Workplane("XY").circle(1.95).extrude(L).translate((0, 0, -L))
    return m_of(h.union(s), 0.03)


def place(mesh, at, direction):
    """screw/washer made along -Z (shank) -> shank points along `direction`, head seat at `at`"""
    m = mesh.copy(); R = trimesh.geometry.align_vectors([0, 0, -1], direction); m.apply_transform(R); m.apply_translation(at); return m


def washer():
    return m_of(cq.Workplane("XY").circle(4.5).circle(2.2).extrude(C.WASHER_T).translate((0, 0, -C.WASHER_T)), 0.03)


def ring_y(cx, cz, r0, r1, y0, y1):
    return m_of(cq.Workplane("XZ").center(cx, cz).circle(r1).circle(r0).extrude(-(y1 - y0)).translate((0, y0, 0)), 0.05)


parts = {}   # name -> (mesh, material, group)


def add(name, mesh, mat, group, scenes=("main", "cu")):
    parts[name] = (mesh, mat, group, scenes)


# ---------------- wheel + chair frame (context) ----------------
T = C.R_T
add("tyre", m_of(cq.Workplane(obj=cq.Solid.makeTorus(T - C.TYRE_W / 2, C.TYRE_W / 2, cq.Vector(0, 0, 0), cq.Vector(0, 1, 0))), 0.3), "tyre", "wheel")
rim = cq.Workplane("XZ").circle(T - C.TYRE_W / 2 - 2).circle(T - C.TYRE_W - 12).extrude(15, both=True)
hub = cq.Workplane("XZ").circle(42).extrude(28, both=True)
for k in range(5):                                                     # 5-spoke mag wheel
    a = math.radians(90 + 72 * k)
    sp = (cq.Workplane("XY").box(26, 16, T - C.TYRE_W - 40).translate((0, 0, (T - C.TYRE_W - 40) / 2 + 30))
          .rotate((0, 0, 0), (0, 1, 0), math.degrees(a) - 90))
    hub = hub.union(sp)
add("mag", m_of(rim.union(hub), 0.3), "mag", "wheel")
add("pushrim", m_of(cq.Workplane(obj=cq.Solid.makeTorus(C.RIM_R, C.RIM_D / 2, cq.Vector(0, C.RIM_Y, 0), cq.Vector(0, 1, 0))), 0.3), "chrome", "wheel")
tabs = None
for k in range(6):
    a = math.radians(30 + 60 * k)
    t = cq.Workplane("XZ").center(C.RIM_R * math.cos(a), C.RIM_R * math.sin(a)).circle(4).extrude(-(C.RIM_Y - 12)).translate((0, 12, 0))
    tabs = t if tabs is None else tabs.union(t)
add("pushrim_tabs", m_of(tabs, 0.3), "chrome", "wheel")
yt, zt = Mt.Y_T_NOM, C.R_T + 94
add("armrest", m_of(cq.Workplane("YZ").center(yt, zt).circle(12.5).extrude(470).translate((-170, 0, 0)), 0.1), "chrome", "chair")
add("backrest", m_of(cq.Workplane("XY").center(-170, yt).circle(12.5).extrude(560).translate((0, 0, zt - 260)), 0.1), "chrome", "chair")
add("seatrail", m_of(cq.Workplane("YZ").center(yt, zt - 210).circle(12.5).extrude(470).translate((-170, 0, 0)), 0.1), "chrome", "chair")
add("foam", m_of(cq.Workplane("XY").box(190, 58, 34).edges("|X").fillet(14).translate((205, yt, zt + 26)), 0.2), "foam", "chair")
add("liner", m_of(cq.Workplane("YZ").center(yt, zt).circle(13.4).circle(12.5).extrude(70).translate((-35, 0, 0)), 0.05), "rubber", "liner")

# ---------------- drive ----------------
add("pod", m_of(C.pod()), "pod", "pod")
add("hanger", m_of(C.hanger()), "hanger", "hanger")
for i, (x, gz) in enumerate(zip(C.RX, C.GZ)):
    add(f"gearbox{i}", m_of(C.cyl_y(x, gz, C.GB_D / 2, C.YF, C.YF + C.GB_T).edges().chamfer(1.0)), "gearbox", "motor")
    add(f"can{i}", m_of(C.cyl_y(x, gz, C.MOT_D / 2 - 0.5, C.YF + C.GB_T, C.YF + C.GB_T + C.MOT_L - 6)), "can", "motor")
    add(f"cap{i}", m_of(C.cyl_y(x, gz, C.MOT_D / 2 - 2, C.YF + C.GB_T + C.MOT_L - 6, C.YF + C.GB_T + C.MOT_L)
                        .union(C.prism_y(x - 9, x + 9, gz - 1.5, gz + 1.5, C.YF + C.GB_T + C.MOT_L, C.YF + C.GB_T + C.MOT_L + 6))), "cap", "motor")
    add(f"boss{i}", m_of(C.cyl_y(x, gz - C.GB_E, 6.0, C.YF - C.BOSS, C.YF)), "gearbox", "motor")
    add(f"shaft{i}", m_of(C.cyl_y(x, gz - C.GB_E, C.SHAFT_D / 2, C.YF - C.BOSS - C.SHAFT_L, C.YF - C.BOSS)), "steel", "motor")
for i, (x, z) in enumerate(zip(C.RX, C.RZ)):
    add(f"roller{i}", m_of(C.cyl_y(x, z, C.ROLL_R, C.ROLL_Y0 + 1.2, C.ROLL_Y0 + C.ROLL_W - 1.2)), "rubber", "pod", ("main",))
    add(f"rollercore{i}", m_of(C.cyl_y(x, z, C.CORE_R + 0.8, C.ROLL_Y0, C.ROLL_Y0 + 1.2).union(C.cyl_y(x, z, C.CORE_R + 0.8, C.ROLL_Y0 + C.ROLL_W - 1.2, C.ROLL_Y0 + C.ROLL_W))
                                  .cut(C.cyl_y(x, z, C.SHAFT_D / 2, C.ROLL_Y0 - 1, C.ROLL_Y0 + C.ROLL_W + 1))), "spool", "pod", ("main",))
sp = trimesh.load(os.path.join(os.path.dirname(HERE), "stl", "spool.stl")); sp.apply_translation(-sp.bounds[0] - [sp.extents[0] / 2, sp.extents[1] / 2, 0])
S25, S16, S20, S12 = screw_mesh(C.LOCK_L), screw_mesh(C.PIN_L), screw_mesh(Mt.CLAMP_L), screw_mesh(C.DECK_L)
W = washer()
for s in (-1, 1):
    x = s * C.TAB_X
    for tag, z, scr, grp in (("lock", C.GZ[0], S25, "lockhw"), ("leg", C.FOOT_Z, S16, "leghw")):
        m = sp.copy(); m.apply_transform(trimesh.geometry.align_vectors([0, 0, 1], [0, 1, 0])); m.apply_translation([x, C.LEG_Y1, z])
        add(f"{tag}spool{s}", m, "spool", grp + "_spool")
        add(f"{tag}washer{s}", place(W, [x, C.SPOOL_Y1 + C.WASHER_T, z], [0, -1, 0]), "steel", grp + "_washer")
        add(f"{tag}screw{s}", place(scr, [x, C.SPOOL_Y1 + C.WASHER_T, z], [0, -1, 0]), "black_steel", grp + "_screw")
    # band: 3 turns = 3 shells on each spool + 6 straight strands
    for k in range(3):
        r0 = C.SPOOL_CORE + 0.05 + k * 0.55; y0, y1 = C.LEG_Y1 + C.SPOOL_FLT + 0.6 + k * 1.3, C.LEG_Y1 + C.SPOOL_FLT + 1.8 + k * 1.3
        for z in (C.GZ[0], C.FOOT_Z):
            add(f"bandring{s}{k}{z:.0f}", ring_y(x, z, r0, r0 + 0.6, y0, y1), "band", "band")
        for sx in (-1, 1):
            add(f"bandstrand{s}{k}{sx}", m_of(C.prism_y(x + sx * r0 - (0.6 if sx < 0 else 0), x + sx * r0 + (0.6 if sx > 0 else 0),
                                                         C.FOOT_Z, C.GZ[0], y0, y1)), "band", "band")

# ---------------- mount ----------------
o, i_, face = Mt.clamp_halves(25)
add("arm", m_of(Mt.arm()), "arm", "arm")
add("clamp_outer", m_of(Mt.place_clamp(o, 0, yt, zt)), "clamp", "clamp_outer")
add("clamp_inner", m_of(Mt.place_clamp(i_, 0, yt, zt)), "clamp", "clamp_inner")
R = (25 + Mt.LINER) / 2
for zx in (-18, 18):                                                    # clamp screws: local (+/-(R+6), y, +/-18) -> chair X = z_local, Z = -x_local
    for dz in (-(R + 6), R + 6):
        add(f"clampscrew{zx}{dz:.0f}", place(S20, [zx, yt + Mt.CL_HEAD_Y, zt + dz], [0, 1, 0]), "black_steel", "clamp_screw")
for ax_ in (-Mt.CLAMP_X, Mt.CLAMP_X):                                   # arm wall -> clamp end nuts
    add(f"armscrew{ax_:.0f}", place(screw_mesh(Mt.ARM_L), [ax_, yt + face + Mt.WALL_T, zt], [0, -1, 0]), "black_steel", "arm_screw")
for s in (-1, 1):                                                       # deck -> bar
    at = [s * C.ARM_HOLE_X, (C.POD_Y0 + C.POD_Y1) / 2, Mt.DECK_Z0 + Mt.DECK_T - Mt.DECK_CB + C.WASHER_T]
    add(f"deckwasher{s}", place(W, at, [0, 0, -1]), "steel", "deck_hw")
    add(f"deckscrew{s}", place(S12, at, [0, 0, -1]), "black_steel", "deck_hw")

exec(open(os.path.join(HERE, "closeup.py")).read())        # close-up parts + views for the motor side

for n, (m, mat, g, sc) in parts.items():
    m.export(os.path.join(OUT, n + ".stl"))

# exploded offsets per group (chair coords)
EXP = {"pod": [0, -70, -60], "motor": [0, -70, -60], "lockhw_spool": [0, 22, 0], "lockhw_washer": [0, 38, 0], "lockhw_screw": [0, 55, 0],
       "leghw_spool": [0, 22, 0], "leghw_washer": [0, 38, 0], "leghw_screw": [0, 55, 0], "band": [0, 22, 0],
       "arm": [0, 0, 90], "deck_hw": [0, 0, 160], "clamp_outer": [0, -45, 90], "clamp_inner": [0, -110, 90], "liner": [0, -45, 90],
       "clamp_screw": [0, -160, 90], "arm_screw": [0, 25, 90], "chair": [0, -45, 90]}
json.dump({"materials": MAT, "parts": [{"name": n, "mat": mat, "group": g, "scenes": list(sc)} for n, (m, mat, g, sc) in parts.items()], "exploded": EXP,
           "labels": {
               "pod": [0, C.POD_Y0, C.GZ[0] + 26], "hanger": [-C.BAR_X + 10, C.LEG_Y1, C.BAR_Z1], "lock": [C.TAB_X, C.SPOOL_Y1 + 3, C.GZ[0]],
               "leg": [C.TAB_X, C.SPOOL_Y1 + 3, C.FOOT_Z], "band": [C.TAB_X + C.SPOOL_CORE, C.LEG_Y1 + 3, (C.GZ[0] + C.FOOT_Z) / 2],
               "arm": [-30, -20, Mt.DECK_Z0 + Mt.DECK_T + 8], "clamp": [0, yt, zt + R + 6], "deck": [C.ARM_HOLE_X, (C.POD_Y0 + C.POD_Y1) / 2, Mt.DECK_Z0 + Mt.DECK_T],
               "roller": [C.RX[1], C.ROLL_Y0, C.RZ[1] - C.ROLL_R], "motor": [C.RX[1], C.YF + C.GB_T + 30, C.GZ[1] + 18],
               "armscrew": [Mt.CLAMP_X, yt + face + Mt.WALL_T + 2, zt], "clampscrew": [18, yt + Mt.CL_HEAD_Y, zt - R - 6]}},
          open(os.path.join(OUT, "scenes.json"), "w"), indent=1)
print(len(parts), "parts exported")
