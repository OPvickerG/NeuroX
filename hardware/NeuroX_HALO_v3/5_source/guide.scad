// HALO v3 assembly guide scenes. STEP = 1..10. Done parts = light grey, new parts = real colour (exploded), red arrows.
include <neurox_halo_print.scad>
PART = "none";
STEP = 1;
GH = [0.80, 0.82, 0.86];      // already built
SC = [0.72, 0.73, 0.76];      // screws
AR = [0.86, 0.12, 0.14];      // arrows
RDc = [0.84, 0.10, 0.12];
module I(n, c) { color(c == "MG" ? MG : c == "WH" ? WH : c == "RD" ? RDc : BL) import(str("stl_model/", n, ".stl")); }
module G(n) { color(GH) import(str("stl_model/", n, ".stl")); }
module part(n, cols, ghost) { if (ghost) G(n); else for (c = cols) I(str(n, "__", c), c); }
module from_pod0(i, dl) at_pod(i, dl) rotate([0, PODS[0][3], 0]) rotate([0, 0, -PODS[0][2]]) translate(-PODS[0][1]) children();
module arrow(a, b, r = 1.4) {
  v = b - a; L = norm(v); hl = min(L * 0.45, r * 6);
  color(AR) translate(a) rotate([0, acos(v[2] / L), atan2(v[1], v[0])]) { cylinder(r = r, h = L - hl, $fn = 16); translate([0, 0, L - hl]) cylinder(r1 = r * 2.6, r2 = 0, h = hl, $fn = 24); }
}
module screw(L, d = 3, head = 5.5) { color(SC) { cylinder(d = d, h = L, $fn = 16); translate([0, 0, L]) cylinder(d = head, h = 2.2, $fn = 24); } }

// ---------- groups ----------
module band(g, ex = 0) {
  part("front", ["MG", "WH", "BL"], g);
  translate([-ex, ex, -ex * 0.9]) part("side_L", ["MG", "WH", "BL"], g);
  translate([-ex, -ex, -ex * 0.9]) part("side_R", ["MG", "WH", "BL"], g);
}
module hubset(g, ex = 0) {
  color(g ? GH : MG) hub();
  color(g ? GH : BL) translate([0, 0, HUB_Z1 - 2 + ex]) hub_ring();
  translate([0, 0, HUB_Z1 - 2 + 2 * ex]) { color(g ? GH : WH) hub_cap_all(); if (!g) color(BL) translate([0, 0, 0.01]) hub_N(); }
}
module ribs(g) color(g ? GH : MG) for (th = RIBS) rib(th);
module rear(g, ex = 0) {
  translate([-ex, 0, 0]) part("strap", ["MG", "WH", "BL"], g);
  translate([-2 * ex, 0, ex * 0.8]) { if (g) G("back_hub"); else I("back_hub", "WH"); }
  let(p = E(ST_I + ST_T + 2.5 * ex, ST_I * BIB / BIA + ST_T + 2.5 * ex, DIAL_A) - E(ST_I + ST_T, ST_I * BIB / BIA + ST_T, DIAL_A)) translate([p[0], p[1], 0]) part("dial", ["WH", "BL"], g);
}
module sliders(g, dl = 0) for (n = SLIDERS) let(st = STEMS[st_i(n)]) in_stem(st) translate([-dl, 0, 0]) multmatrix(st[3]) for (c = ["MG", "BL"]) if (g) G(str("slider_", n, "__", c)); else I(str("slider_", n, "__", c), c);
module knobs(g, ex = 0) for (n = SLIDERS) let(st = STEMS[st_i(n)]) in_stem(st) for (w = st[8]) translate([lock_s(st), w, T_TG + 8 - KNOB_B + ex]) { if (g) G("knob"); else I("knob", "BL"); }
module cups(g, ex = 0, dl = 0) for (i = [0:5]) at_pod(i, dl) translate([-ex, 0, 0]) rotate([0, PODS[0][3], 0]) rotate([0, 0, -PODS[0][2]]) translate(-PODS[0][1]) for (c = ["WH", "BL"]) if (g) G(str("cup__", c)); else I(str("cup__", c), c);
module clips(g, ex = 0) {
  color(g ? GH : BL) for (c = CLIPS) at_band(c[0], c[1] + ex, 0) rotate([90, 0, 0]) translate([0, 0, -3]) band_clip();
  color(g ? GH : BL) for (A = ARMS) let(pts = A[1], n = len(pts)) for (k = [floor(n / 3), floor(2 * n / 3)]) oriented(pts[k][0], pts[k][1], pts[k][2]) translate([ex * 0.6, 0, -2.5]) arm_clip();
  for (s = [0, 1]) let(p = E(BIA + BAND_T + ex, BIB + BAND_T + ex, 46) - E(BIA + BAND_T, BIB + BAND_T, 46)) mirror([0, s, 0]) translate([p[0], p[1], 0]) part("temple_cap", ["WH", "BL"], g);
}
module crest_p(g, ex = 0) translate([ex, 0, 0]) part("crest", ["WH", "BL", "MG", "RD"], g);
module cover_p(g, ex = 0) translate([-ex, 0, 0]) part("back_cover", ["WH", "BL"], g);
module gels() color(C_GEL) for (i = [0:5]) at_pod(i, 0) translate([0.2, 0, 0]) rotate([0, 90, 0]) cylinder(r = 18, h = 1.2);

// ---------- steps ----------
if (STEP == 1) {                      // crown hub top
  hubset(false, 14);
  arrow([0, 0, HUB_Z1 + 75], [0, 0, HUB_Z1 + 34]);
}
if (STEP == 2) {                      // band: front + 2 sides, 4 x M3x20
  band(false, 16);
  for (s = [-1, 1]) for (a = [JOINT - LAP / 2, JOINT + LAP / 2]) at_band(s * a, BAND_T / 2, ZT + 14) screw(20);
  for (s = [-1, 1]) { p = E(BIA + BAND_T / 2, BIB + BAND_T / 2, s * JOINT); arrow([p[0] - 18, p[1] + s * 18, -16], [p[0] - 3, p[1] + s * 3, -3]); arrow([p[0], p[1], ZT + 52], [p[0], p[1], ZT + 40]); }
}
if (STEP == 3) {                      // ribs into the band, hub onto the ribs
  band(true);
  translate([0, 0, 22]) ribs(false);
  translate([0, 0, 50]) color(MG) hub();
  for (th = [38, -38, 142, -142]) let(R = rtheta(th), ps = rpsi(th)) arrow([R * cos(ps) * 0.93, R * sin(ps) * 0.93, ZT + 40], [R * cos(ps) * 0.93, R * sin(ps) * 0.93, ZT + 12]);
  arrow([0, 0, HUB_Z1 + 75], [0, 0, HUB_Z1 + 56]);
}
if (STEP == 4) {                      // strap, back hub, dial
  band(true); ribs(true); hubset(true);
  rear(false, 18);
  arrow([-BIA - 70, 0, 26], [-BIA - 42, 0, 16]);
  let(p = E(ST_I + ST_T + 40, ST_I * BIB / BIA + ST_T + 40, DIAL_A), q = E(ST_I + ST_T + 18, ST_I * BIB / BIA + ST_T + 18, DIAL_A)) arrow([p[0], p[1], 0], [q[0], q[1], 0]);
}
if (STEP == 6) {                      // cups into holders, sliders into sleeves
  band(true); ribs(true); hubset(true); rear(true);
  sliders(false, 26); knobs(true);
  cups(false, 16, 26);
  for (i = [0:5]) at_pod(i, 26) arrow([-44, 0, 0], [-20, 0, 0], 1.2);
  for (n = SLIDERS) let(st = STEMS[st_i(n)]) in_stem(st) arrow([st[4] - 24, 0, T_TG + 14], [st[4] - 4, 0, T_TG + 14], 1.2);
}
if (STEP == 7) {                      // leads, clips, temple caps
  band(true); ribs(true); hubset(true); rear(true); sliders(true); knobs(true); cups(true);
  clips(false, 14);
  for (c = [CLIPS[1], CLIPS[3], CLIPS[8]]) let(p = E(BIA + c[1] + 34, BIB + c[1] + 34, c[0]), q = E(BIA + c[1] + 19, BIB + c[1] + 19, c[0])) arrow([p[0], p[1], 0], [q[0], q[1], 0]);
}
if (STEP == 8) {                      // crest
  band(true); ribs(true); hubset(true); rear(true); sliders(true); knobs(true); cups(true); clips(true);
  crest_p(false, 26);
  arrow([CR_X0 + 80, -48, CR_SZ - 8], [CR_X0 + 42, -40, CR_SZ - 8]);
}
if (STEP == 9) {                      // cables + back cover
  band(true); ribs(true); hubset(true); rear(true); sliders(true); knobs(true); cups(true); clips(true); crest_p(true);
  cover_p(false, 24);
  arrow([XB - 60, 0, (HB_Z0 + HB_Z1) / 2], [XB - 32, 0, (HB_Z0 + HB_Z1) / 2]);
  color([0.15, 0.15, 0.16]) for (s = [-1, 1]) let(p = E(ST_I + ST_T + 4.5, ST_I * BIB / BIA + ST_T + 4.5, 180 + s * 6)) hull() { translate([p[0], p[1], HB_Z0 - 40]) sphere(2.1, $fn = 16); translate([p[0], p[1], HB_Z0 + 6]) sphere(2.1, $fn = 16); }
}
if (STEP == 10) {                     // done, on the head
  head();
  band(false); ribs(false); hubset(false); rear(false); sliders(false); knobs(false); cups(false); clips(false); crest_p(false); cover_p(false); gels();
}
if (STEP == 5) {                      // lock knob: knob + M3x8 from outside, nut into its pocket
  st = STEMS[st_i("temple_L")]; w = st[8][0]; ls = lock_s(st);
  color(MG) import("gd/crop/front5__MG.stl");
  in_stem(st) translate([ls, w, T_TG + 8 - KNOB_B + 20]) { I("knob", "BL"); translate([0, 0, KNOB_B]) color(SC) { translate([0, 0, -8]) cylinder(d = 3, h = 8, $fn = 20); cylinder(d = 5.5, h = 3, $fn = 24); } }
  in_stem(st) translate([ls - 30, w, CH_T1 - 2]) color([0.62, 0.63, 0.66]) difference() { rotate([0, 0, 90]) cylinder(d = 5.5 / cos(30), h = 2.4, $fn = 6); translate([0, 0, -1]) cylinder(d = 3, h = 5, $fn = 20); }
  in_stem(st) arrow([ls - 11, w, T_TG + 8 + 26], [ls - 11, w, T_TG + 8 + 6], 1.1);
  in_stem(st) arrow([ls - 26, w, CH_T1 - 0.8], [ls - 9, w, CH_T1 - 0.8], 1.0);
}
