"""Fit check for the chair drive: puts pod + hanger + motors + rollers + bolts on a 24" wheel and checks every clearance,
with the rollers ON the tyre (driving) and LIFTED (pushing by hand). Prints PASS / FAIL per check."""
import math
import cadquery as cq
import chair_drive_cq as C

ok_all = True


def check(name, cond, detail=""):
    global ok_all
    ok_all &= bool(cond)
    print(f"  {'PASS' if cond else 'FAIL'}  {name}  {detail}")


def vol(a, b):
    try:
        return a.intersect(b).val().Volume()
    except Exception:
        return 0.0


def torus_y(major, minor, y):
    return cq.Workplane(obj=cq.Solid.makeTorus(major, minor, cq.Vector(0, y, 0), cq.Vector(0, 1, 0)))


tyre = torus_y(C.R_T - C.TYRE_W / 2, C.TYRE_W / 2, 0)
rim = torus_y(C.RIM_R, C.RIM_D / 2, C.RIM_Y)
ymid = (C.POD_Y0 + C.POD_Y1) / 2


def motors(dz=0.0):
    m = None
    for x, gz in zip(C.RX, C.GZ):
        gb = C.cyl_y(x, gz + dz, C.GB_D / 2, C.YF, C.YF + C.GB_T)
        can = C.cyl_y(x, gz + dz, C.MOT_D / 2, C.YF + C.GB_T, C.YF + C.GB_T + C.MOT_L)
        boss = C.cyl_y(x, gz - C.GB_E + dz, 6.0, C.YF - C.BOSS, C.YF)
        m = gb.union(can).union(boss) if m is None else m.union(gb).union(can).union(boss)
    return m


def rollers(dz=0.0, sleeve=True):
    r = None
    for x, z in zip(C.RX, C.RZ):
        c = C.cyl_y(x, z + dz, C.ROLL_R if sleeve else C.CORE_R, C.ROLL_Y0, C.ROLL_Y0 + C.ROLL_W)
        r = c if r is None else r.union(c)
    return r


def bolts(dz=0.0):
    b = None
    for s in (-1, 1):
        shank = C.cyl_z(s * C.TAB_X, ymid, 2.5, C.TAB_Z0 + dz, C.BAR_Z1 + 10)
        head = C.cyl_z(s * C.TAB_X, ymid, 4.3, C.TAB_Z0 - 3.5 + dz, C.TAB_Z0 + dz)
        b = shank.union(head) if b is None else b.union(shank).union(head)
    return b


pod, hang = C.pod(), C.hanger()
print("DRIVING (rollers on the tyre)")
for z_off, label in ((0.0, "driving"), (C.BAR_GAP - 3.0, "lifted")):
    if label == "lifted": print(f"LIFTED (nuts tightened, pod up {z_off:.0f} mm)")
    P = pod.translate((0, 0, z_off)); M = motors(z_off); R = rollers(z_off); B = bolts(z_off)
    for n, part in (("pod", P), ("motors", M), ("hanger", hang), ("bolts", B)):
        check(f"{n} clear of tyre", vol(part, tyre) < 0.5, f"overlap {vol(part, tyre):.1f} mm3")
    for n, part in (("pod", P), ("motors", M), ("hanger", hang), ("bolts", B), ("rollers", R)):
        check(f"{n} clear of push rim", vol(part, rim) < 0.5, f"overlap {vol(part, rim):.1f} mm3")
    check("motors clear of hanger", vol(M, hang) < 0.5, f"overlap {vol(M, hang):.1f}")
    check("pod clear of hanger", vol(P, hang) < 0.5, f"overlap {vol(P, hang):.1f}")
    check("pod holds gearboxes (no clash)", vol(P, M) < 1.0, f"overlap {vol(P, M):.1f}")
    # roller to tyre: radial gap along the line from the wheel centre (tyre crown at Y = roller centre)
    yc = C.ROLL_Y0 + C.ROLL_W / 2
    crown = (C.R_T - C.TYRE_W / 2) + math.sqrt(max(0.0, (C.TYRE_W / 2) ** 2 - yc ** 2))
    gaps = [math.hypot(x, z + z_off) - C.ROLL_R - crown for x, z in zip(C.RX, C.RZ)]
    if label == "driving":
        check("both rollers touch the tyre crown", all(abs(g) < 0.6 for g in gaps), "gaps " + ", ".join(f"{g:+.2f}" for g in gaps) + " mm")
        cover = min(C.ROLL_Y0 + C.ROLL_W, C.TYRE_W / 2) - max(C.ROLL_Y0, -C.TYRE_W / 2)
        check("roller sits on the tyre tread", cover >= C.ROLL_W - 0.5, f"{cover:.1f} of {C.ROLL_W:.0f} mm on the tyre")
        check("shaft reaches into the roller", C.YF - C.BOSS - C.SHAFT_L <= C.ROLL_Y0 + C.ROLL_W / 2 - 2,
              f"shaft tip at Y {C.YF - C.BOSS - C.SHAFT_L:.1f}, roller Y {C.ROLL_Y0:.1f}..{C.ROLL_Y0 + C.ROLL_W:.1f}")
        spring_len = C.BAR_Z0 + 1.5 - (C.TAB_Z1 - 1.5)
        check("spring space 25-40 mm", 25 <= spring_len <= 40, f"{spring_len:.1f} mm between seats")
        bl = (C.BAR_Z1 + 4 + 8) - C.TAB_Z0
        print(f"  info  spring bolt: M5 x {5 * math.ceil(bl / 5):.0f} (needs {bl:.0f} mm under the head)")
    else:
        check("rollers off the tyre when lifted", min(gaps) > 5, "gaps " + ", ".join(f"{g:+.1f}" for g in gaps) + " mm")
print("\nALL PASS" if ok_all else "\nSOME CHECKS FAILED")
