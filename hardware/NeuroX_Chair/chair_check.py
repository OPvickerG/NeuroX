"""Fit check for the chair drive: puts pod + hanger + motors + rollers + spools, screws and bands on a 24" wheel and checks
every clearance, with the rollers ON the tyre (driving), DROPPED (worn / out-of-round tyre) and LIFTED (locked, pushing by hand).
Also checks every screw length against the M4 kit (8/10/12/16/20/25). Prints PASS / FAIL per check."""
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


_CUP = C.roller_cup().rotate((0, 0, 0), (1, 0, 0), -90)                         # axis along +Y, z = 0 end at Y = 0


def rollers(dz=0.0, sleeve=True):
    """the 2 CUP rollers (printed part; the rubber bands add CUP_DEEP to the groove floor)"""
    r = None
    for x, z in zip(C.RX, C.RZ):
        c = _CUP.translate((x, C.ROLL_Y0, z + dz))
        r = c if r is None else r.union(c)
    return r


def bolts(dz=0.0):
    """screws, spools and the rubber bands. Lock bolt + its spool move with the pod (dz); leg spool is fixed."""
    b = None
    for s in (-1, 1):
        x = s * C.TAB_X
        up = C.cyl_y(x, C.GZ[0] + dz, C.SPOOL_FL, C.LEG_Y1, C.SPOOL_Y1)                         # lock spool
        up = up.union(C.cyl_y(x, C.GZ[0] + dz, C.M4_HEAD / 2 + 1, C.SPOOL_Y1, C.SPOOL_Y1 + C.WASHER_T + 2.2))   # washer + head
        lo = C.cyl_y(x, C.FOOT_Z, C.SPOOL_FL, C.LEG_Y1, C.SPOOL_Y1)                              # leg spool
        lo = lo.union(C.cyl_y(x, C.FOOT_Z, C.M4_HEAD / 2 + 1, C.SPOOL_Y1, C.SPOOL_Y1 + C.WASHER_T + 2.2))
        band = C.prism_y(x - C.SPOOL_CORE - 2, x + C.SPOOL_CORE + 2, C.FOOT_Z - C.SPOOL_CORE - 2, C.GZ[0] + dz + C.SPOOL_CORE + 2,
                         C.LEG_Y1 + C.SPOOL_FLT, C.SPOOL_Y1 - 1)                                  # band wraps
        part = up.union(lo).union(band)
        b = part if b is None else b.union(part)
    return b


pod, hang = C.pod(), C.hanger()
LIFT = C.TRAV_UP - 1.0
for z_off, label in ((0.0, "driving"), (-(C.TRAV_DN - 1.0), "dropped"), (LIFT, "lifted")):
    print({"driving": "DRIVING (rollers on the tyre)", "dropped": f"DROPPED (pod {C.TRAV_DN - 1:.0f} mm lower: worn or out-of-round tyre)",
           "lifted": f"LIFTED (pod up {LIFT:.0f} mm, lock bolts tight)"}[label])
    P = pod.translate((0, 0, z_off)); M = motors(z_off); R = rollers(z_off); B = bolts(z_off)
    for n, part in (("pod", P), ("motors", M), ("hanger", hang), ("spools/bands", B)):
        check(f"{n} clear of tyre", vol(part, tyre) < 0.5, f"overlap {vol(part, tyre):.1f} mm3")
    for n, part in (("pod", P), ("motors", M), ("hanger", hang), ("spools/bands", B), ("rollers", R)):
        check(f"{n} clear of push rim", vol(part, rim) < 0.5, f"overlap {vol(part, rim):.1f} mm3")
    check("motors clear of hanger", vol(M, hang) < 0.5, f"overlap {vol(M, hang):.1f}")
    check("motors clear of spools/bands", vol(M, B) < 0.5, f"overlap {vol(M, B):.1f}")
    check("pod slides in the leg slots (no clash)", vol(P, hang) < 0.5, f"overlap {vol(P, hang):.1f}")
    check("pod holds gearboxes (no clash)", vol(P, M) < 1.0, f"overlap {vol(P, M):.1f}")
    # roller to tyre: radial gap along the line from the wheel centre (tyre crown at Y = roller centre)
    yc = C.ROLL_Y0 + C.ROLL_W / 2
    crown = (C.R_T - C.TYRE_W / 2) + math.sqrt(max(0.0, (C.TYRE_W / 2) ** 2 - yc ** 2))
    gaps = [math.hypot(x, z + z_off) - C.ROLL_R - crown for x, z in zip(C.RX, C.RZ)]
    span = C.GZ[0] + z_off - C.FOOT_Z
    if label == "driving":
        check("both rollers touch the tyre crown", all(abs(g) < 0.6 for g in gaps), "gaps " + ", ".join(f"{g:+.2f}" for g in gaps) + " mm")
        cover = min(C.ROLL_Y0 + C.ROLL_W, C.TYRE_W / 2) - max(C.ROLL_Y0, -C.TYRE_W / 2)
        check("roller sits on the tyre tread", cover >= C.ROLL_W - 0.5, f"{cover:.1f} of {C.ROLL_W:.0f} mm on the tyre")
        edge = [math.hypot(x, z) - C.cup_r(C.ROLL_W / 2) - ((C.R_T - C.TYRE_W / 2) + math.sqrt((C.TYRE_W / 2) ** 2 - (C.ROLL_W / 2) ** 2)) for x, z in zip(C.RX, C.RZ)]
        check("cup hugs the crown, edges just clear", 0 < min(edge) < 1.5, f"band surface {min(edge):.2f} mm off the tyre at the cup edges")
        check("printed cup clear of the tyre (bands do the touching)", vol(R, tyre) < 0.5, f"overlap {vol(R, tyre):.1f} mm3")
        seat = C.SHAFT_D / 2 - 0.5 + C.ROLL_SCREW_L
        check("M3 x 8 head sunk under the rubber bands", seat + 3.0 < C.ROLL_R - C.CUP_DEEP, f"socket head top r {seat + 3:.1f}, band seat r {C.ROLL_R - C.CUP_DEEP:.1f}")
        check("cup clear of the gearbox boss", C.ROLL_Y0 + C.ROLL_W < C.YF - C.BOSS - 2, f"cup ends at Y {C.ROLL_Y0 + C.ROLL_W:.1f}, boss at {C.YF - C.BOSS:.1f}")
        check("the two cups don't touch", 2 * C.X_ROLL > 2 * C.cup_r(C.ROLL_W / 2) + 5)
        check("shaft reaches into the roller", C.YF - C.BOSS - C.SHAFT_L <= C.ROLL_Y0 + C.ROLL_W / 2 - 2,
              f"shaft tip at Y {C.YF - C.BOSS - C.SHAFT_L:.1f}, roller Y {C.ROLL_Y0:.1f}..{C.ROLL_Y0 + C.ROLL_W:.1f}")
        check("band span 20-35 mm (band stretched, pulls down)", 20 <= span <= 35, f"{span:.1f} mm spool to spool")
    elif label == "dropped":
        check("bands still pull when the pod has dropped", span - 2 * C.SPOOL_FL > 2, f"spools {span - 2 * C.SPOOL_FL:.1f} mm apart")
    else:
        check("rollers off the tyre when lifted", min(gaps) > 5, "gaps " + ", ".join(f"{g:+.1f}" for g in gaps) + " mm")
        check("pod top still under the bar", C.GZ[0] + C.RING_OD / 2 + z_off < C.BAR_Z0 - 0.5)

print("SCREWS (M4 kit: 8 10 12 16 20 25, button head)")
KIT = (8, 10, 12, 16, 20, 25)
lock_in_nut = (C.NUT_Y0 + C.M4_NUT_T - 0.2) - C.LOCK_TIP                        # thread length through the nut
check(f"lock bolt M4 x {C.LOCK_L} goes right through its nut", C.LOCK_L in KIT and C.LOCK_TIP <= C.NUT_Y0 - 0.3,
      f"tip at Y {C.LOCK_TIP:.1f}, nut Y {C.NUT_Y0:.1f}..{C.NUT_Y0 + C.M4_NUT_T - 0.2:.1f}")
check("plastic in front of the lock nut >= 4 mm", C.POD_Y1 - (C.NUT_Y0 + C.M4_NUT_T) >= 4, f"{C.POD_Y1 - (C.NUT_Y0 + C.M4_NUT_T):.1f} mm")
check("lock bolt clamps the leg (boss shorter than the leg)", C.BOSS_Y1 < C.LEG_Y1 and C.BOSS_Y1 > C.LEG_Y0 + 3)
pin_grip = C.PIN_L - (C.SPOOL_L + C.WASHER_T)
check(f"leg spool screw M4 x {C.PIN_L}: {pin_grip:.1f} mm into the leg", C.PIN_L in KIT and C.LEG_T - 0.5 <= pin_grip <= C.LEG_T + 1)
print("\nALL PASS" if ok_all else "\nSOME CHECKS FAILED")
