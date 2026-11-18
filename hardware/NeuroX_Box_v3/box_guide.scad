// NeuroX Box v3 assembly guide scenes. STEP = 1..8. Done parts grey, new parts in colour, red arrows.
include <gd_const.scad>
STEP = 1;
S = "build/stl/"; BT = "build/buttons/";
MG = [0.17, 0.19, 0.24]; WH = [0.94, 0.95, 0.96]; BL = [0.07, 0.34, 0.82]; RD = [0.84, 0.10, 0.12];
GH = [0.80, 0.82, 0.86]; SC = [0.72, 0.73, 0.76]; AR = [0.86, 0.12, 0.14];
PCB = [0.10, 0.45, 0.26]; CHIP = [0.12, 0.12, 0.14]; METAL = [0.78, 0.79, 0.82]; BAT = [0.20, 0.35, 0.65];
$fn = 32;
module part(n, cols, g) { if (g) color(GH) import(str(S, n, ".stl")); else for (c = cols) color(c == "MG" ? MG : c == "WH" ? WH : c == "BL" ? BL : RD) import(str(S, n, "__", c, ".stl")); }
module blk(b, c) { color(c) translate([b[0], b[1], b[2]]) cube([b[3] - b[0], b[4] - b[1], b[5] - b[2]]); }
module arrow(a, b, r = 1.6) {
  v = b - a; L = norm(v); hl = min(L * 0.45, r * 6);
  color(AR) translate(a) rotate([0, acos(v[2] / L), atan2(v[1], v[0])]) { cylinder(r = r, h = L - hl, $fn = 16); translate([0, 0, L - hl]) cylinder(r1 = r * 2.6, r2 = 0, h = hl, $fn = 24); }
}
module screw(L, d = 3, head = 5.5, csk = false) { color(SC) { cylinder(d = d, h = L); translate([0, 0, L]) if (csk) cylinder(d1 = d, d2 = head, h = 1.7); else cylinder(d = head, h = 2.0); } }

module base(g) part("base", ["MG", "BL", "WH"], g);
module lid(g, dz = 0) translate([0, 0, dz]) part("lid", ["MG", "WH", "BL", "RD"], g);
module grommet(g, ex = 0) translate([IL + T + ex, SLEEVE[0], SLEEVE[1]]) rotate([0, -90, 0]) part("sleeve_grommet", ["BL"], g);
module feet(ex = 0) color([0.1, 0.1, 0.1]) for (x = [12, IL - 12]) for (y = [12, IW - 12]) translate([x, y, -F - 2.4 - ex]) cylinder(d = 10, h = 2.4);
module buzzer(ex = 0) { b = C_buzzer; color(CHIP) translate([(b[0] + b[3]) / 2, (b[1] + b[4]) / 2, ex]) cylinder(d = 12, h = 9.5); }
module mpu6050(g, ex = 0) {
  b = C_mpu6050;
  color(g ? GH : PCB) translate([b[0], b[1], ex]) cube([b[3]-b[0], b[4]-b[1], 1.6]);    // PCB
  color(g ? GH : CHIP) translate([b[0]+3, b[1]+3, ex+1.6]) cube([14, 14, 2.2]);          // chip on top
  for (h = MPU_HOLES) color(g ? GH : METAL) translate([h[0], h[1], ex]) cylinder(d=4, h=3, $fn=12); // standoffs
}
module power(g, ex = 0) {
  translate([0, 0, ex]) { blk(C_tp4056, g ? GH : [0.15, 0.30, 0.70]); blk(C_boost, g ? GH : PCB); blk(C_battery, g ? GH : BAT); }
  translate([-ex * 0.8, 0, 0]) blk(C_switch, g ? GH : CHIP);
}
module mainboard(g, ex = 0) translate([0, 0, ex]) {
  blk(C_mainboard, g ? GH : [0.75, 0.62, 0.38]); blk(C_esp_hdrs, g ? GH : CHIP); blk(C_esp32, g ? GH : [0.15, 0.15, 0.20]);
  // v3.1: the pills moved to the HALO (ESP32-C3) - nothing else on the board
}
module screen(g, ex = 0) translate([0, 0, -ex]) {
  bx = BRD_C[0] - BRD_L / 2; by = BRD_C[1] - BRD_W / 2; zb = IH - SCR_STACK;
  color(g ? GH : [0.80, 0.12, 0.12]) translate([bx, by, zb]) cube([BRD_L, BRD_W, 1.6]);
  color(g ? GH : [0.05, 0.05, 0.07]) translate([bx + 1, by, zb + 1.6]) cube([70, BRD_W, 3.0]);
  color(g ? GH : CHIP) translate([bx + BRD_L - 4, by + 8, zb - 8]) cube([2.5, 34, 8]);     // header pins (to the wires)
}
module tabs(g, ex = 0) for (p = CLAMP_POSTS) translate([p[0], p[1], IH - SCR_STACK - 2.2 - ex]) rotate([0, 0, p[1] < 57 ? 0 : 180]) part("clamp_tab", ["MG"], g);
module tab_screws(ex) for (p = CLAMP_POSTS) translate([p[0], p[1], IH - SCR_STACK - 2.2 - ex]) rotate([180, 0, 0]) screw(6, 2, 3.8);
module btn(n, k, g) { c = k == "switch" ? METAL : k == "cap" ? (n == "stop" ? RD : CHIP) : k == "stop_top" ? RD : MG; color(g ? GH : c) import(str(BT, "asm_", n, "_", k, ".stl")); }
module buttons(g, ex = 0) for (b = BUTTONS) let(n = b[0]) {
  btn(n, "guide", g);
  translate([0, 0, -ex * 0.6]) btn(n, "cap", g);
  translate([0, 0, -ex * 1.6]) btn(n, "switch", g);
  translate([0, 0, -ex * 2.3]) btn(n, "cradle", g);
  if (n == "stop") btn(n, "stop_top", g);
}
module guard(g, dz = 0) translate([STOP[0], STOP[1], TOP + dz]) part("stop_guard", ["RD"], g);

// ---------------- steps ----------------
if (STEP == 1) {     // base prep: grommet, feet, buzzer, MPU-6050
  base(false); grommet(false, 22); feet(10); buzzer(16); mpu6050(false, 20);
  arrow([IL + T + 52, SLEEVE[0], SLEEVE[1]], [IL + T + 30, SLEEVE[0], SLEEVE[1]]);
  b = C_buzzer; arrow([(b[0] + b[3]) / 2, (b[1] + b[4]) / 2, 50], [(b[0] + b[3]) / 2, (b[1] + b[4]) / 2, 30]);
  b2 = C_mpu6050; arrow([(b2[0]+b2[3])/2, (b2[1]+b2[4])/2, 50], [(b2[0]+b2[3])/2, (b2[1]+b2[4])/2, 25]);
}
if (STEP == 2) {     // power: charger, boost, battery, rocker
  base(true); grommet(true); buzzer(0); mpu6050(true);
  power(false, 22);
  arrow([(C_battery[0] + C_battery[3]) / 2, 62, 62], [(C_battery[0] + C_battery[3]) / 2, 62, 40]);
  arrow([-40, 41, 12], [-24, 41, 12]);
}
if (STEP == 3) {     // perfboard on the standoffs, 4 x M2x6
  base(true); grommet(true); buzzer(0); power(true); mpu6050(true);
  mainboard(false, 26);
  for (h = MB_HOLES) translate([h[0], h[1], 5.6 + 26 + 18]) rotate([180, 0, 0]) screw(6, 2, 3.8);
  for (h = [MB_HOLES[0], MB_HOLES[3]]) arrow([h[0], h[1], 5.6 + 26 + 40], [h[0], h[1], 5.6 + 26 + 22]);
}
if (STEP == 4) {     // buttons into the lid (lid seen from below) — base now complete
  lid(true); guard(false, 10);
  buttons(false, 12);
  for (b = BUTTONS) arrow([b[1] + 14, b[2], IH - 52], [b[1] + 14, b[2], IH - 30]);
}
if (STEP == 5) {     // screen into the pocket + 4 clamp tabs (lid seen from below)
  lid(true); guard(true); buttons(true);
  screen(false, 22); tabs(false, 30); tab_screws(30 + 12);
  arrow([BRD_C[0], BRD_C[1], IH - 60], [BRD_C[0], BRD_C[1], IH - 34]);
}
if (STEP == 6) {     // close: lid onto the base, 4 x M3x12 countersunk
  base(true); grommet(true); buzzer(0); mpu6050(true); power(true); mainboard(true);
  lid(false, 34); guard(false, 34); translate([0, 0, 34]) { buttons(false); screen(false); tabs(false); }
  for (p = BOSSES) translate([p[0], p[1], TOP + 34 + 22]) rotate([180, 0, 0]) screw(12, 3, 6.0, true);
  for (p = [BOSSES[0], BOSSES[1]]) arrow([p[0], p[1], TOP + 34 + 52], [p[0], p[1], TOP + 34 + 26]);
}
if (STEP == 7) {     // done
  base(false); grommet(false); mpu6050(false); lid(false); guard(false); for (b = BUTTONS) { btn(b[0], "cap", false); if (b[0] == "stop") btn(b[0], "stop_top", false); }
  feet(0);
}
