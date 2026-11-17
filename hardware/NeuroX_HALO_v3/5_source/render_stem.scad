// close-up scenes of the extendable stem (from the exported parts)
include <neurox_halo_print.scad>
PART = "none";
SCENE = "explode";     // explode | slider_skin | slider_out | lock | lock_cut
UP = 14;              // lock: knob + screw lifted this far
module I(n, c) { color(c == "MG" ? MG : c == "WH" ? WH : c == "RD" ? [0.84, 0.10, 0.12] : BL) import(str("stl_model/", n, ".stl")); }
module slider_I(name, dl) { st = STEMS[st_i(name)]; in_stem(st) translate([-dl, 0, 0]) multmatrix(st[3]) for (c = ["MG", "BL"]) I(str("slider_", name, "__", c), c); }
module from_pod0(i, dl) at_pod(i, dl) rotate([0, PODS[0][3], 0]) rotate([0, 0, -PODS[0][2]]) translate(-PODS[0][1]) children();
if (SCENE == "explode") {          // left side piece + REF slider pulled 14 mm out of its sleeve
  for (c = ["MG", "WH", "BL"]) I(str("side_L__", c), c);
  slider_I("ref_L", 14);
  for (c = ["WH", "BL"]) from_pod0(4, 14) I(str("cup__", c), c);
}
if (SCENE == "slider_skin") {      // a slider seen from the skin side: ridges
  st = STEMS[st_i("temple_L")];
  rotate([0, 0, 90]) multmatrix([[1, 0, 0, 0], [0, -1, 0, 0], [0, 0, -1, T_TG], [0, 0, 0, 1]]) multmatrix(st[3]) {
    for (c = ["MG", "BL"]) I(str("slider_temple_L__", c), c);
    for (c = ["WH", "BL"]) from_pod0(2, 0) I(str("cup__", c), c);
  }
}
if (SCENE == "slider_out") {       // a slider from the outside: blue length marks
  st = STEMS[st_i("temple_L")];
  rotate([0, 0, 90]) multmatrix([[1, 0, 0, 0], [0, -1, 0, 0], [0, 0, -1, T_TG], [0, 0, 0, 1]]) multmatrix(st[3])
    for (c = ["MG", "BL"]) I(str("slider_temple_L__", c), c);
}

// v3 lock: REF sleeve on the left side piece, knob + screw lifted, slider pulled out
module screw_v(len) { color([0.72, 0.73, 0.76]) { translate([0, 0, -len]) cylinder(d = 3, h = len, $fn = 24); difference() { cylinder(d = 5.5, h = 3, $fn = 32); translate([0, 0, 1]) cylinder(d = 2.6, h = 3, $fn = 6); } } }
module nut_v() { color([0.62, 0.63, 0.66]) difference() { rotate([0, 0, 90]) cylinder(d = 5.5 / cos(30), h = 2.4, $fn = 6); translate([0, 0, -1]) cylinder(d = 3, h = 5, $fn = 20); } }
if (SCENE == "lock") {
  st = STEMS[st_i("ref_L")];
  for (c = ["MG", "WH", "BL"]) I(str("side_L__", c), c);
  slider_I("ref_L", 12);
  for (c = ["WH", "BL"]) from_pod0(4, 12) I(str("cup__", c), c);
  in_stem(st) translate([lock_s(st), 0, T_TG + 8 - KNOB_B + UP]) { I("knob", "BL"); translate([0, 0, KNOB_B]) screw_v(8); }
  in_stem(st) translate([lock_s(st) - 9, 0, CH_T1 - 7]) nut_v();
}
// v3: a slider as it prints (outer face down): the grooves are on top
if (SCENE == "grooves") {
  color(MG) import("stl/slider_temple_L__MG.stl");
  color(BL) import("stl/slider_temple_L__BL.stl");
}
