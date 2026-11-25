"""Fit check: pod + hanger + motors + arm + clamp on a 24" wheel, clamp on a vertical 25 mm backrest tube.
Checks the nominal position and both ends of the sideways adjustment, driving and lifted."""
import math
import cadquery as cq
import chair_drive_cq as C
import chair_mount_cq as Mt
from chair_check import tyre, rim, motors, bolts, vol, check, ymid
import chair_check as K

pod, hang = C.pod(), C.hanger()
D = 25
o, i, face = Mt.clamp_halves(D)
print("ARM ON THE HANGER")
check("deck sits on the hanger (touching, no overlap)", vol(Mt.arm(), hang) < 0.5 and abs(Mt.DECK_Z0 - C.BAR_Z1) < 0.01)
check("deck slots line up with hanger holes X +/-24", 24 in (12, 24, 36))
check("rear spring nut reachable (deck ends before it)", Mt.DECK_X0 > -C.TAB_X + 12, f"deck from X {Mt.DECK_X0:.0f}, nut at X {-C.TAB_X:.0f}")
for dy, label in ((0, "nominal"), (-20, "arm slid 20 mm inboard"), (20, "arm slid 20 mm outboard")):
    arm = Mt.arm().translate((0, dy, 0))
    wall_in = Mt.Y_WALL - Mt.WALL_T + dy
    post_y = wall_in - face
    cl = Mt.place_clamp(o, D, -40, wall_in, Mt.ZC).union(Mt.place_clamp(i, D, -40, wall_in, Mt.ZC))
    post = C.cyl_z(-40, post_y, D / 2, 200, 520)
    print(f"{label.upper()}: tube centre at Y {post_y:.0f} mm (tyre centre = 0, tyre side at -{C.TYRE_W / 2:.1f})")
    for z_off, st in ((0.0, "driving"), (C.BAR_GAP - 3.0, "lifted")):
        P = pod.translate((0, 0, z_off)); M = K.motors(z_off); B = K.bolts(z_off)
        for n, part in (("arm", arm), ("clamp", cl)):
            check(f"  {st}: {n} clear of tyre", vol(part, tyre) < 0.5)
            check(f"  {st}: {n} clear of pod", vol(part, P) < 0.5)
            check(f"  {st}: {n} clear of motors", vol(part, M) < 0.5)
            check(f"  {st}: {n} clear of spring bolts", vol(part, B) < 0.5)
            check(f"  {st}: {n} clear of push rim", vol(part, rim) < 0.5)
    check("  tube clear of the tyre", vol(post, tyre) < 0.5, f"gap {abs(post_y) - D / 2 - C.TYRE_W / 2:.0f} mm")
    check("  clamp grips the tube (bore fits, no overlap)", vol(cl, post) < 1.0)
    check("  arm clear of the clamp halves", vol(arm, cl) < 1.0)
print("\nRANGES")
print(f"  tube centre sideways: {Mt.Y_WALL - Mt.WALL_T - 20 - face:.0f} .. {Mt.Y_WALL - Mt.WALL_T + 20 - face:.0f} mm inboard of the tyre centre (25 mm tube)")
print(f"  tube front/back: vertical tube anywhere from X {Mt.SLOT_X0:.0f} to {Mt.SLOT_X1:.0f} mm (0 = top of the tyre, + = forward)")
print(f"  height: vertical tube = slide the clamp; horizontal tube centre {Mt.ROWS[0] - C.R_T:.0f} / {Mt.ROWS[1] - C.R_T:.0f} / {Mt.ROWS[2] - C.R_T:.0f} mm above the tyre top")
print("\nALL PASS" if K.ok_all else "\nSOME CHECKS FAILED")
