"""NeuroX box v2 layout - single source of truth for part positions.
Writes layout.scad (included by neurox_box.scad) and checks that no two parts collide.
Inner frame: origin = inner front-left corner on top of the floor. X = length (left->right),
Y = depth (front->back), Z = up. Units mm. Front = the side you face (Y = 0).
"""
import itertools, sys

P = dict(
    T=2.4,          # wall
    F=2.0,          # floor
    IL=158.0,       # inner length
    IW=96.0,        # inner depth
    IH=44.0,        # inner height (floor top -> lid underside)
    LID=3.0,        # lid plate
    R=6.0,          # outer corner radius (walls)
    CH_D=7.0,       # cable channel depth (lid rim + ledge stick out this far)
    CH_H=14.0,      # cable channel height (between ledge and lid rim)
    BOSS_OD=8.0, BOSS_IN=5.0,
)
CH_BOTTOM = P["IH"] - P["CH_H"]          # inner z where the ledge top is (channel bottom)

# ---- parts: name -> [x0, y0, z0, x1, y1, z1] incl. assembly clearance ----
C = {
    # screen module (2.8" ILI9341, 86 x 50, header at the RIGHT short end), glass touching the lid
    "screen":       [2, 32, 37.8, 88, 82, 44],       # PCB + glass stack 6.2 mm
    "screen_wires": [80, 34, 16, 88, 80, 37.8],      # header pins + Dupont housings + wire bend
    # power (left, under the screen / buttons)
    "tp4056":       [0, 12, 2, 29, 30, 7.5],         # TP4056 Type-C module 28 x 17, USB-C at the left wall
    "switch":       [0, 31.5, 5.2, 24, 50.5, 19.5],  # KCD1 2-pin rocker switch body + pins behind the left wall
    "battery":      [25.5, 44, 0, 79.5, 80, 12],     # LiPo pouch 103450: 50 x 34 x 10 (+2 mm)
    "boost":        [32, 10, 0, 70, 29, 11],         # MT3608 boost 36 x 17 (pot on top)
    # main perfboard with ESP32-S3 DevKitC-1 (USB end at the right wall) + both pills
    "mainboard":    [90, 10, 4, 158, 60, 5.6],        # perfboard cut to 68 x 50
    "esp_hdrs":     [90, 14, 5.6, 158, 40, 14.1],     # female headers 8.5 mm
    "esp32":        [90, 14, 14.1, 158, 40, 20.0],    # DevKitC ~69 x 26 incl. module can
    "pill_face":    [100, 44, 5.6, 126, 55, 10],     # BioAmp EXG Pill 25.4 x 10 on short headers
    "jst_face":     [100, 44, 10, 106, 55, 17],      # JST PH plug standing up
    "pill_brain":   [128, 44, 5.6, 154, 55, 10],
    "jst_brain":    [128, 44, 10, 134, 55, 17],
    "buzzer":       [104, 64, 0, 117, 77, 10],       # 12 mm buzzer in a pocket, sound out through the floor
}
# lid buttons: name, x, y, hole dia, body (below lid) dia, body depth
BUTTONS = [("left", 21, 13, 16.3, 18, 24), ("new", 45, 13, 16.3, 18, 24),
           ("ok", 69, 13, 16.3, 18, 24), ("stop", 126, 80, 16.3, 18, 24)]
for n, x, y, d, bd, dep in BUTTONS:
    C["btn_" + n] = [x - bd / 2, y - bd / 2, P["IH"] - dep, x + bd / 2, y + bd / 2, P["IH"]]

# right wall: ESP32 USB window (both USB-C ports) + 2 cable exits into the channel
ESP_USB = dict(y0=15, y1=39, zc=17.3, h=7.5)            # zc = inner height of the port centres
CABLE_EXITS = [50.0, 57.5]                               # y of the two pill cable slots (right wall)
# left wall: TP4056 USB-C + power rocker switch (KCD1, snap-in)
TP_USB = dict(yc=21, zc=5.2, w=10.0, h=4.6)
SWITCH = dict(yc=41.0, zc=12.3, w=19.2, h=13.0, panel=1.6)   # panel cutout; wall thinned to 1.6 mm so the clips snap
# front ledge: 6-lead clip (comb), x range
LEAD_CLIP = dict(x0=100.0, pitch=6.0, n=6, slot=2.0, entry=1.6)

# ---- fixed printed features (AABB) ----
b = P["BOSS_IN"]; r = P["BOSS_OD"] / 2
BOSSES = [(b, b), (P["IL"] - b, b), (b, P["IW"] - b), (P["IL"] - b, P["IW"] - b)]
F = {}
for i, (x, y) in enumerate(BOSSES):
    F[f"boss{i}"] = [x - r, y - r, 0, x + r, y + r, P["IH"]]
MB_HOLES = [(93.5, 13.5), (154.5, 13.5), (93.5, 56.5), (154.5, 56.5)]
for i, (x, y) in enumerate(MB_HOLES):
    F[f"standoff{i}"] = [x - 2.5, y - 2.5, 0, x + 2.5, y + 2.5, 4]
F["scr_frame"] = [0.2, 30.2, 37.0, 89.8, 83.8, P["IH"]]
CLAMP_POSTS = [(14, 28.0), (56, 28.0), (14, 86.0), (56, 86.0)]   # screen clamp strip screws (outside the PCB)
for i, (x, y) in enumerate(CLAMP_POSTS):
    F[f"clamppost{i}"] = [x - 2.6, y - 2.6, 34.0, x + 2.6, y + 2.6, P["IH"]]
F["bat_rim"] = [24.5, 43, 0, 80.5, 81, 3]
F["tp_shelf"] = [0, 12, 0, 29, 30, 2]
F["buzz_pocket"] = [102.5, 62.5, 0, 118.5, 78.5, 4]
for n, x, y, d, bd, dep in BUTTONS:
    F["btnring_" + n] = [x - d / 2 - 2.2, y - d / 2 - 2.2, P["IH"] - 3.0, x + d / 2 + 2.2, y + d / 2 + 2.2, P["IH"]]

ALLOWED = {frozenset(["screen", "scr_frame"]), frozenset(["screen_wires", "scr_frame"]),
           frozenset(["battery", "bat_rim"]), frozenset(["mainboard", "esp_hdrs"]),
           frozenset(["tp4056", "tp_shelf"]), frozenset(["buzzer", "buzz_pocket"])}
for i in range(4): ALLOWED.add(frozenset(["mainboard", f"standoff{i}"]))
for i in range(4): ALLOWED.add(frozenset(["scr_frame", f"clamppost{i}"]))     # posts are part of the frame
for n, *_ in BUTTONS: ALLOWED.add(frozenset(["btn_" + n, "btnring_" + n]))

def overlap(a, b, eps=0.05):
    return all(min(a[i + 3], b[i + 3]) - max(a[i], b[i]) > eps for i in range(3))

def check():
    items = {**C, **F}
    bad = []
    for (na, a), (nb, bb) in itertools.combinations(items.items(), 2):
        if frozenset([na, nb]) in ALLOWED: continue
        if overlap(a, bb): bad.append((na, nb))
    for n, a in items.items():
        if a[0] < -0.01 or a[1] < -0.01 or a[2] < -0.01 or a[3] > P["IL"] + .01 or a[4] > P["IW"] + .01 or a[5] > P["IH"] + .01:
            bad.append((n, "outside the box"))
    lipw = 2.0                                     # lid lip band along the walls (top 2.5 mm)
    for n, a in C.items():
        if a[5] > P["IH"] - 2.5 + 0.05 and (a[0] < lipw or a[1] < lipw or a[3] > P["IL"] - lipw or a[4] > P["IW"] - lipw):
            if n not in ("screen",): bad.append((n, "lid lip"))
    # ports must be below the ledge chamfer (wall is plain there)
    chamfer_bottom = CH_BOTTOM - P["CH_D"] - 0.5
    if ESP_USB["zc"] + ESP_USB["h"] / 2 > chamfer_bottom: bad.append(("esp usb window", "hits the ledge chamfer"))
    if SWITCH["zc"] + SWITCH["h"] / 2 + 3 > chamfer_bottom: bad.append(("switch cutout", "hits the ledge chamfer"))
    return bad

def emit(path="layout.scad"):
    with open(path, "w") as f:
        f.write("// generated by layout.py - edit layout.py, not this file\n")
        for k, v in P.items(): f.write(f"{k} = {v};\n")
        f.write(f"CH_BOTTOM = {CH_BOTTOM};\n")
        f.write("PARTS = [\n" + ",\n".join(f'  ["{k}", {v}]' for k, v in C.items()) + "\n];\n")
        f.write(f"BOSSES = {[list(t) for t in BOSSES]};\n")
        f.write(f"MB_HOLES = {[list(t) for t in MB_HOLES]};\n")
        f.write(f"CLAMP_POSTS = {[list(t) for t in CLAMP_POSTS]};\n")
        f.write("BUTTONS = [\n" + ",\n".join(f'  ["{n}", {x}, {y}, {d}]' for n, x, y, d, bd, dep in BUTTONS) + "\n];\n")
        for k in ("screen", "battery", "esp32", "buzzer", "mainboard", "tp4056", "switch"):
            f.write(f'{k.upper()} = {C[k]};\n')
        f.write(f'ESP_USB = [{ESP_USB["y0"]}, {ESP_USB["y1"]}, {ESP_USB["zc"]}, {ESP_USB["h"]}];\n')
        f.write(f'CABLE_EXITS = {CABLE_EXITS};\n')
        f.write(f'TP_USB = [{TP_USB["yc"]}, {TP_USB["zc"]}, {TP_USB["w"]}, {TP_USB["h"]}];\n')
        f.write(f'SWITCH_H = [{SWITCH["yc"]}, {SWITCH["zc"]}, {SWITCH["w"]}, {SWITCH["h"]}, {SWITCH["panel"]}];\n')
        f.write(f'LEAD_CLIP = [{LEAD_CLIP["x0"]}, {LEAD_CLIP["pitch"]}, {LEAD_CLIP["n"]}, {LEAD_CLIP["slot"]}, {LEAD_CLIP["entry"]}];\n')

if __name__ == "__main__":
    emit()
    bad = check()
    if bad:
        print("COLLISIONS:"); [print("  ", x) for x in bad]; sys.exit(1)
    print(f"layout OK: {len(C)} parts + {len(F)} features, no collisions")
