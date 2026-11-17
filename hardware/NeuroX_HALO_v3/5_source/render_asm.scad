// exact assembly render from the exported (CGAL) parts, coloured like the AMS print
// DL = how far every slider is pulled out (mm, + = longer stems, - = shorter)
include <neurox_halo_print.scad>
PART = "none";
SHOW_HEAD = true;
DL = 0;
ONLY = "all";          // all | stem_closeup (band piece + one slider, exploded)
module I(n, c) { color(c == "MG" ? MG : c == "WH" ? WH : c == "RD" ? [0.84, 0.10, 0.12] : BL) import(str("stl_model/", n, ".stl")); }
// a part exported at pod 0 (forehead) moved to pod i, following its slider
module from_pod0(i, dl) at_pod(i, dl) rotate([0, PODS[0][3], 0]) rotate([0, 0, -PODS[0][2]]) translate(-PODS[0][1]) children();
module slider_I(name, dl) { st = STEMS[st_i(name)]; in_stem(st) translate([-dl, 0, 0]) multmatrix(st[3]) for (c = ["MG", "BL"]) I(str("slider_", name, "__", c), c); }
if (ONLY == "all") {
  if (SHOW_HEAD) head();
  for (p = ["front", "side_L", "side_R", "strap"]) for (c = ["MG", "WH", "BL"]) I(str(p, "__", c), c);
  for (c = ["WH", "BL", "MG", "RD"]) I(str("crest__", c), c);
  for (c = ["WH", "BL"]) { I(str("back_cover__", c), c); I(str("dial__", c), c); I(str("temple_cap__", c), c); mirror([0, 1, 0]) I(str("temple_cap__", c), c);
    for (i = [0:5]) from_pod0(i, DL) I(str("cup__", c), c); }
  for (n = SLIDERS) slider_I(n, DL);
  for (n = SLIDERS) let(st = STEMS[st_i(n)]) in_stem(st) for (w = st[8]) translate([lock_s(st), w, T_TG + 8 - KNOB_B]) I("knob", "BL");   // v3 lock knobs
  I("back_hub", "WH");
  color(MG) { for (th = RIBS) rib(th); hub(); }
  color(WH) translate([0, 0, HUB_Z1 - 2]) hub_cap_all();
  color(BL) { translate([0, 0, HUB_Z1 - 2]) hub_ring(); translate([0, 0, HUB_Z1 - 2 + 0.01]) hub_N(); }
  color(C_GEL) for (i = [0:5]) at_pod(i, DL) translate([0.2, 0, 0]) rotate([0, 90, 0]) cylinder(r = 18, h = 1.2);
  color(BL) for (c = CLIPS) at_band(c[0], c[1], 0) rotate([90, 0, 0]) translate([0, 0, -3]) band_clip();
  color(BL) for (A = ARMS) let(pts = A[1], n = len(pts)) for (k = [floor(n / 3), floor(2 * n / 3)]) oriented(pts[k][0], pts[k][1], pts[k][2]) translate([0, 0, -2.5]) arm_clip();
}
