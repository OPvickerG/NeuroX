// one assembly instance per export (for the STEP assembly): INST = index into INSTS
include <neurox_halo_print.scad>
PART = "none";
INST = -1;
module M(n) import(str("stl_model/", n, ".stl"));
module from_pod0(i) at_pod(i, 0) rotate([0, PODS[0][3], 0]) rotate([0, 0, -PODS[0][2]]) translate(-PODS[0][1]) children();
KNOBS = [for (n = SLIDERS) for (w = STEMS[st_i(n)][8]) [n, w]];
INSTS = concat(
  [["front", "MG"], ["side_L", "MG"], ["side_R", "MG"], ["strap", "MG"], ["crest", "WH"], ["back_hub", "WH"], ["back_cover", "WH"], ["dial", "WH"],
   ["temple_cap_L", "WH"], ["temple_cap_R", "WH"], ["hub", "MG"], ["hub_cap", "WH"], ["hub_ring", "BL"]],
  [for (n = SLIDERS) [str("slider_", n), "MG"]],
  [for (i = [0:5]) [str("cup_", PODS[i][0]), "WH"]],
  [for (k = [0:4]) [str("rib_", k), "MG"]],
  [for (k = [0:len(CLIPS) - 1]) [str("band_clip_", k + 1), "BL"]],
  [for (a = [0:len(ARMS) - 1]) for (j = [0:1]) [str("arm_clip_", ARMS[a][0], "_", j + 1), "BL"]],
  [for (q = KNOBS) [str("lock_knob_", q[0], q[1] > 0 ? "_L" : q[1] < 0 ? "_R" : ""), "BL"]]
);
K_KNOB = 13 + len(SLIDERS) + 11 + len(CLIPS) + 2 * len(ARMS);
module inst(k) {
  n = INSTS[k][0];
  if (k < 8) M(n);
  else if (n == "temple_cap_L") M("temple_cap");
  else if (n == "temple_cap_R") mirror([0, 1, 0]) M("temple_cap");
  else if (n == "hub") hub();
  else if (n == "hub_cap") translate([0, 0, HUB_Z1 - 2]) hub_cap_all();
  else if (n == "hub_ring") translate([0, 0, HUB_Z1 - 2]) hub_ring();
  else if (k < 13 + len(SLIDERS)) M(n);
  else if (k < 13 + len(SLIDERS) + 6) from_pod0(k - 13 - len(SLIDERS)) M("cup");
  else if (k < 13 + len(SLIDERS) + 11) rib(RIBS[k - 13 - len(SLIDERS) - 6]);
  else if (k < 13 + len(SLIDERS) + 11 + len(CLIPS)) let(c = CLIPS[k - 13 - len(SLIDERS) - 11]) at_band(c[0], c[1], 0) rotate([90, 0, 0]) translate([0, 0, -3]) band_clip();
  else if (k >= K_KNOB) let(q = KNOBS[k - K_KNOB], st = STEMS[st_i(q[0])]) in_stem(st) translate([lock_s(st), q[1], T_TG + 8 - KNOB_B]) knob();
  else let(q = k - 13 - len(SLIDERS) - 11 - len(CLIPS), A = ARMS[floor(q / 2)], pts = A[1], nn = len(pts), kk = (q % 2 == 0) ? floor(nn / 3) : floor(2 * nn / 3))
    oriented(pts[kk][0], pts[kk][1], pts[kk][2]) translate([0, 0, -2.5]) arm_clip();
}
if (INST >= 0) inst(INST);
if (INST == -2) for (k = [0:len(INSTS) - 1]) echo(str("INST;", k, ";", INSTS[k][0], ";", INSTS[k][1]));
