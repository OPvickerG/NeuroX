"""NeuroX HALO print geometry: pod frames on the head + arm paths -> gen_print.scad
Head model = the one used in the concepts (average adult, ~55 cm round at the band).
Pod frame: origin = skin point under the pad centre, local +x = outward skin normal."""
import numpy as np, math, json, sys
HA, HB, HC = 97, 78, 108
SH = [((0, 0, 0), (HA, HB, HC)), ((18, 0, -55), (76, 64, 62))]

def field(p, sh=SH): return min(sum(((p[i]-c[i])/r[i])**2 for i in range(3)) for c, r in sh)
def surf(th, z):
    d = np.array([math.cos(math.radians(th)), math.sin(math.radians(th)), 0.0])
    lo, hi = 0.0, 200.0
    for _ in range(60):
        m = (lo+hi)/2
        if field(d*m + [0, 0, z]) <= 1: lo = m
        else: hi = m
    p = d*lo + [0, 0, z]
    c, rr = min(SH, key=lambda s: sum(((p[i]-s[0][i])/s[1][i])**2 for i in range(3)))
    n = np.array([(p[i]-c[i])/rr[i]**2 for i in range(3)]); n /= np.linalg.norm(n)
    return p, n
def ang(n): return math.degrees(math.atan2(n[1], n[0])), math.degrees(math.asin(max(-1, min(1, n[2]))))
def frame(n):
    az, el = map(math.radians, ang(n))
    x = np.array([math.cos(el)*math.cos(az), math.cos(el)*math.sin(az), math.sin(el)])
    y = np.array([-math.sin(az), math.cos(az), 0.0])
    z = np.cross(x, y)
    return x, y, z

# pods: name, theta, z (skin point), lead/arm
NOSE_Z = -52.0      # v2: nose pad 4 mm lower than v1 (room for the sliding stem to go up to -49)
PODS = [("forehead", 0, -10), ("nose", 0, NOSE_Z), ("temple_L", 49, -60), ("temple_R", -49, -60),
        ("ref_L", 116, -44), ("ref_R", -116, -44)]
# holder geometry in the pod frame (must match helmet_print.scad)
ATTACH_X, ATTACH_Z = 12.6, 13.0       # where an arm meets the holder (back, top)

out = {}
for name, th, z in PODS:
    p, n = surf(th, z); a, e = ang(n)
    out[name] = dict(p=p, n=n, az=a, el=e)

BIA, BIB, BAND_T = 102, 83, 8
def band_mid(th):
    t = math.radians(th); return np.array([(BIA+BAND_T/2)*math.cos(t), (BIB+BAND_T/2)*math.sin(t)])

def arm_path(start, start_dir, pod, nseg=10):
    """start: 3D point at the band bottom (or another holder), pod: name. Waypoints follow the head."""
    P = out[pod]; x, y, zz = frame(P["n"])
    A = P["p"] + ATTACH_X*x + ATTACH_Z*zz
    ths, zs = math.degrees(math.atan2(start[1], start[0])), start[2]
    tha, za = math.degrees(math.atan2(A[1], A[0])), A[2]
    s0, _ = surf(ths, zs); off0 = np.linalg.norm(start[:2]) - np.linalg.norm(s0[:2])
    sA, _ = surf(tha, za); offA = np.linalg.norm(A[:2]) - np.linalg.norm(sA[:2])
    pts = []
    for k in range(nseg+1):
        t = k/nseg; th = ths + (tha-ths)*t; zk = zs + (za-zs)*t
        s, n = surf(th, zk)
        q = s + n*(off0 + (offA-off0)*t)
        a, e = ang(n); pts.append((q, a, e))
    pts[-1] = (A, P["az"], P["el"])
    return pts


# ======================= v2: EXTENDABLE RIDGED STEMS =======================
# Each lower pod (temples, REF, nose) sits on a SLIDER: holder ring + flat tongue with
# ridges. The tongue slides in a SLEEVE that is part of the band piece; a spring pawl in
# the sleeve clicks into the ridges (every 2 mm). Stem frame (local coords):
#   s = along the stem (0 = holder centre, + towards the band)
#   w = across the stem (width)
#   t = thickness, t = 0 tongue skin-side face, t = T tongue outer face (= holder back face)
T_TG, RIDGE, CLR_T = 3.0, 0.6, 0.2
HOLD_R, HOLD_X1, X_TG0 = 16.6, 13.6, 13.6 - 3.0
WALL = 1.6
def unit(v): return v / np.linalg.norm(v)
def arm_path2(start, end, end_n, nseg=8):
    """surface-following path from start (band) to end (sleeve top), offsets interpolated"""
    ths, zs = math.degrees(math.atan2(start[1], start[0])), start[2]
    tha, za = math.degrees(math.atan2(end[1], end[0])), end[2]
    s0, _ = surf(ths, zs); off0 = np.linalg.norm(start[:2]) - np.linalg.norm(s0[:2])
    sA, _ = surf(tha, za); offA = np.linalg.norm(end[:2]) - np.linalg.norm(sA[:2])
    pts = []
    for k in range(nseg + 1):
        t = k / nseg; th = ths + (tha - ths) * t; zk = zs + (za - zs) * t
        s, n = surf(th, zk)
        q = s + n * (off0 + (offA - off0) * t)
        a, e = ang(n); pts.append((q, a, e))
    a, e = ang(end_n); pts[-1] = (end, a, e)
    return pts
def head_clear(p):
    """distance (approx, mm) of a point outside the head model"""
    lo, hi = 0.0, 1.0
    c = np.array([0, 0, p[2]]) if True else None
    d = p - np.array([0, 0, 0.0]); r = np.linalg.norm(d[:2]); th = math.degrees(math.atan2(d[1], d[0]))
    s, n = surf(th, p[2]); return float(np.dot(p - s, n))

STEMS = []; ARMS = {}
LS, G, I0, TW = 20.0, 10.0, 11.0, 8.0        # sleeve length, exposed tongue (nominal), inserted (nominal), tongue width
for side, sg in (("L", 1), ("R", -1)):
    for kind, th in (("temple", 46), ("ref", 128)):
        name = f"{kind}_{side}"; P = out[name]; x, y, z = frame(P["n"])
        LS, G, I0 = (18.0, 9.0, 10.0) if kind == "temple" else (20.0, 10.0, 11.0)
        b = band_mid(sg * th); B = np.array([b[0], b[1], -9.0])
        O = P["p"] + X_TG0 * x
        v = B - (O + 1.5 * x); d = unit(v - np.dot(v, x) * x)
        e1, e3 = d, x; e2 = np.cross(e3, e1)
        SB = HOLD_R + G; top = SB + LS
        S = O + (top + 2.0) * e1 + (T_TG / 2 + 0.7) * e3              # upper arm ends inside the sleeve top wall (v3: +0.7 = clear of the channel floor plane)
        ARMS[name] = arm_path2(B, S, x)
        STEMS.append(dict(name=name, pod=name, e=(e1, e2, e3), O=O, SB=SB, LS=LS, I0=I0, TW=TW, W=[0.0],
                          s0=HOLD_R - 3.0, push=LS - WALL - I0 - 0.4, pull=I0 - 5.0, upper=float(np.linalg.norm(B - S))))
# nose: twin tongues left + right of the forehead holder, sleeves hang from the band bottom
P = out["nose"]; x, y, z = frame(P["n"])
O = P["p"] + X_TG0 * x; e1, e2, e3 = z, y, x
s_top = (-9.0 - O[2]) / e1[2]                                       # sleeve top merges into the band bottom (z = -9)
LSn = 21.0; SBn = s_top - LSn
pushn = 2.5                                                          # up until the ring nearly touches the forehead holder
I0n = LSn - WALL - 0.4 - pushn
STEMS.append(dict(name="nose", pod="nose", e=(e1, e2, e3), O=O, SB=SBn, LS=LSn, I0=I0n, TW=7.0, W=[-22.0, 22.0],
                  s0=None, push=pushn, pull=I0n - 5.0, upper=0.0))

L = ["// generated by gen_print.py - do not edit", "// POD = [name, skin point, azimuth, elevation]", "PODS = ["]
for name, *_ in PODS:
    P = out[name]; p = P["p"]
    L.append(f'  ["{name}", [{p[0]:.3f}, {p[1]:.3f}, {p[2]:.3f}], {P["az"]:.3f}, {P["el"]:.3f}],')
L.append("];")
L.append("// ARMS (v2 = upper arms only): [name, [[point, az, el], ...]] band -> top of the sleeve")
L.append("ARMS = [")
for k, pts in ARMS.items():
    L.append(f'  ["{k}", [' + ", ".join(f"[[{q[0]:.3f}, {q[1]:.3f}, {q[2]:.3f}], {a:.3f}, {e:.3f}]" for q, a, e in pts) + "]],")
L.append("];")
L.append("// STEMS: [name, pod index, M (stem -> world), Minv, SB (sleeve bottom s), LS, I0 (nominal insertion), TW, [w offsets], s0 (tongue start, -1 = nose fork), push, pull, lead slot angle]")
L.append("STEMS = [")
pod_names = [p[0] for p in PODS]
for st in STEMS:
    e1, e2, e3 = st["e"]; O = st["O"]
    M = np.eye(4); M[:3, 0], M[:3, 1], M[:3, 2], M[:3, 3] = e1, e2, e3, O
    Mi = np.linalg.inv(M)
    Px, Py, Pz = frame(out[st["pod"]]["n"])
    dy, dz = float(np.dot(e1, Py)), float(np.dot(e1, Pz))
    st["slot"] = math.degrees(math.atan2(-dy, dz)) + (40.0 if st["name"] != "nose" else 0.0)
    f = lambda A: "[" + ", ".join("[" + ", ".join(f"{v:.6f}" for v in row) + "]" for row in A) + "]"
    L.append(f'  ["{st["name"]}", {pod_names.index(st["pod"])}, {f(M)}, {f(Mi)}, {st["SB"]:.3f}, {st["LS"]:.3f}, {st["I0"]:.3f}, {st["TW"]:.2f}, '
             f'[{", ".join(f"{w:.2f}" for w in st["W"])}], {(-1 if st["s0"] is None else st["s0"]):.3f}, {st["push"]:.2f}, {st["pull"]:.2f}, {st["slot"]:.2f}],')
L.append("];")
open(sys.argv[1] if len(sys.argv) > 1 else "gen_print.scad", "w").write("\n".join(L) + "\n")
for st in STEMS:
    e1, e2, e3 = st["e"]; O = st["O"]
    # clearance of the sleeve's skin-side wall (t = -0.8 - WALL) along its length
    cl = [head_clear(O + s * e1 + w * e2 + (-0.8 - WALL) * e3) for s in np.linspace(st["SB"], st["SB"] + st["LS"], 6) for w in st["W"]]
    print(f'{st["name"]:9s} SB {st["SB"]:5.1f} LS {st["LS"]:4.1f} I0 {st["I0"]:4.1f} travel -{st["push"]:.1f}/+{st["pull"]:.1f} mm  upper arm {st["upper"]:5.1f} mm  '
          f'sleeve wall clearance to skin min {min(cl):5.1f} mm  stem dir el {math.degrees(math.asin(e1[2])):5.1f}')
F = out["forehead"]; xf, yf, zf = frame(F["n"]); N = out["nose"]; xn, yn, zn = frame(N["n"])
fh_bottom = [F["p"] + xx * xf - (14.15 / math.sin(math.radians(45)) + 0.7) * zf for xx in (3.5, 13.6)]
ns = STEMS[-1]
nose_top = [N["p"] + xx * xn + (HOLD_R + ns["push"]) * zn for xx in (3.5, 13.6)]
print("forehead holder bottom tip z", [round(float(p[2]), 1) for p in fh_bottom], " x", [round(float(p[0]), 1) for p in fh_bottom])
print("nose ring top at full push z", [round(float(p[2]), 1) for p in nose_top], " x", [round(float(p[0]), 1) for p in nose_top])
