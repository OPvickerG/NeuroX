"""Fit check, arm v2 on the Vissco Rodeo Veer armrest: clamp on the horizontal 25 mm armrest tube, arm down to the hanger bar.
Checks 9 tube positions (3 sideways x 3 heights) across the adjustment range, driving and lifted."""
import cadquery as cq
import chair_drive_cq as C
import chair_mount_cq as Mt
import chair_check as K
from chair_check import tyre, rim, vol, check

pod, hang = C.pod(), C.hanger()
o, i, face = Mt.clamp_halves(25)
top = C.R_T
zlo, zhi = Mt.WALL_Z0 + 10, Mt.WALL_Z1 - 10                      # clamp bolt range on the wall slots
print("ARM ON THE HANGER BAR")
check("deck sits on the hanger bar, no overlap", vol(Mt.arm(), hang) < 0.5 and abs(Mt.DECK_Z0 - C.BAR_Z1) < 0.01)
check("deck clear of the legs and spools", Mt.DECK_X < C.TAB_X - C.LEG_HX - 3, f"deck to X +/-{Mt.DECK_X:.0f}, legs from X +/-{C.TAB_X - C.LEG_HX:.0f}")
for y_t in (Mt.Y_T_NOM - 40, Mt.Y_T_NOM, Mt.Y_T_NOM + 40):
    arm = Mt.arm(Mt.Y_T_NOM, face).translate((0, y_t - Mt.Y_T_NOM, 0))
    hole_ok = abs((y_t - Mt.Y_T_NOM) - 0) <= Mt.DECK_SLOT_L / 2 + 0.01
    for z_t in (zlo, (zlo + zhi) / 2, zhi):
        cl = Mt.place_clamp(o, 0, y_t, z_t).union(Mt.place_clamp(i, 0, y_t, z_t))
        tube = cq.Workplane("YZ").center(y_t, z_t).circle(12.5).extrude(300).translate((-150, 0, 0))
        tag = f"tube {abs(y_t):.0f} mm inboard, {z_t - top:.0f} mm above the tyre top"
        bad = []
        for z_off in (0.0, K.LIFT):
            P = pod.translate((0, 0, z_off)); M = K.motors(z_off); B = K.bolts(z_off)
            for n, part in (("arm", arm), ("clamp", cl), ("tube", tube)):
                for m, other in (("tyre", tyre), ("pod", P), ("motors", M), ("spools/bands", B), ("push rim", rim)):
                    if vol(part, other) > 0.5: bad.append(f"{n}/{m}")
        if vol(cl, arm) > 1.0: bad.append("clamp/arm")
        if vol(cl, tube) > 1.0: bad.append("clamp/tube")
        check(tag, not bad and hole_ok, ", ".join(sorted(set(bad))))
print("\nSCREWS (M4 kit)")
KIT = (8, 10, 12, 16, 20, 25)
for D in (22, 25, 28):
    R = (D + Mt.LINER) / 2
    tip = Mt.CL_HEAD_Y + Mt.CLAMP_L
    check(f"{D} mm clamp: M4 x {Mt.CLAMP_L} through its nut, head sunk in", Mt.CLAMP_L in KIT and tip >= Mt.CL_NUT_Y + 3.2 + 0.3
          and -(R + 6) < Mt.CL_HEAD_Y < -Mt.GAP / 2 - 6, f"tip at {tip:.1f}, nut {Mt.CL_NUT_Y:.1f}..{Mt.CL_NUT_Y + 3.2:.1f}, {Mt.CL_HEAD_Y + Mt.GAP / 2:+.1f} mm under the head")
tip = Mt.WALL_T - Mt.ARM_L                                                      # from the mount face
check(f"arm to clamp: M4 x {Mt.ARM_L} through the captive nut", Mt.ARM_L in KIT and tip <= -(Mt.NUT_FACE + 3.2 + 0.3), f"tip {-tip:.1f} mm behind the face, nut ends {Mt.NUT_FACE + 3.2:.1f}")
into_bar = C.DECK_L - (Mt.DECK_T - Mt.DECK_CB + C.WASHER_T)
check(f"deck to bar: M4 x {C.DECK_L} + washer, {into_bar:.1f} mm into the bar", C.DECK_L in KIT and 5 <= into_bar <= C.BAR_T - 2)
print("\nRANGES")
print(f"  sideways: armrest tube centre {abs(Mt.Y_T_NOM + 40):.0f}-{abs(Mt.Y_T_NOM - 40):.0f} mm inboard of the tyre centre")
print(f"  height:   armrest tube centre {zlo - top:.0f}-{zhi - top:.0f} mm above the top of the tyre")
print(f"  front/back: slide the clamp along the 14 cm bare tube; the rollers work anywhere within +/-30 mm of the tyre top")
print("\nALL PASS" if K.ok_all else "\nSOME CHECKS FAILED")
