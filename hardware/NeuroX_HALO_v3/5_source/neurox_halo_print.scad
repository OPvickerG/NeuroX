// =============================================================================
//  NeuroX HALO headset v2 - PRINT VERSION (passive chassis, concept v5 look)
//  v3: EXTENDABLE STEMS WITH A SCREW LOCK - temple, REF and nose pods sit on sliders in sleeves on
//      the band; an M3 thumb screw presses the grooved tongue onto a tooth = locked in 2 mm steps
//  Bambu Lab A1 mini + AMS lite (midnight gray / white / blue / red), PLA, no supports.
//  Colours print in place: export each colour of a part with COL = MG | WH | BL.
//
//  Head model: average adult, ~55 cm round at the band. The rear strap adjusts
//  about +/-35 mm; for clearly smaller / bigger heads scale ALL parts 95 % / 105 %.
//  Origin = head centre at band height, +X = face, +Y = wearer's left, Z up.
//
//  PART = assembly | front | side_L | side_R | strap | rib_spine | rib_side (x4) |
//         hub | hub_cap | hub_ring | cup (x6) | crest | temple_cap (x2) |
//         dial | back_hub | back_cover | band_clip | arm_clip |
//         slider_temple_L | slider_temple_R | slider_ref_L | slider_ref_R | slider_nose |
//         stem_test | fitcheck
//  Every part except "assembly" is exported already turned for printing (flat side down).
// =============================================================================
include <gen_print.scad>
include <logo_geo.scad>
use <orbitron900.ttf>

PART = "assembly";
COL = "ALL";          // ALL | MG | WH | BL : one colour of a multi-colour part (AMS)
$col = COL;
SHOW_HEAD = true;
$fn = 48;

// ---------- measure + tune ----------
SNAP_D   = 14.5;    // pocket for the lead's snap clip (round body)  <- MEASURE YOUR LEAD END
SNAP_H   = 7.4;     // pocket depth (snap clip height)                <- MEASURE
STUD_D   = 9.5;     // hole the gel pad's stud (+ its base) goes through   v3.1: was 7.0
FRONT_T  = 0.8;     // cup front wall between gel pad and snap clip (4 layers)  v3.1: was 1.6 - the stud didn't reach the snap
WIRE_W   = 3.0;     // lead wire slot
CLR      = 0.30;    // press-fit clearance (cups, covers, inlays)

// ---------- head / band ----------
HA = 97; HB = 78; HC = 108;
BIA = 102; BIB = 83; BAND_T = 8; BAND_H = 22;          // band inner ellipse, thickness, height
BOA = BIA + BAND_T; BOB = BIB + BAND_T;
ZT = BAND_H / 2;                                         // band top z (= print bed side)
JOINT = 66; LAP = 6;                                     // front <-> side joints at +/-66 deg, lap +/-6 deg
SIDE_END = 160;                                          // side pieces end at +/-160 deg
// rib surface (ribs follow it): band mid at the bottom, 16 mm above the head at the top
RA = 106.4; RB = 87.4; RC = HC + 16;
PH1 = 70.3;                                              // where ribs meet the crown hub
HUB_A = RA * cos(PH1); HUB_B = RB * cos(PH1);
HUB_Z0 = 112.5; HUB_Z1 = 125.5;
RIB_T = 4.5; RIB_W = 12;                                 // rib thickness (in its plane) x width
POCKET_D = 9;                                            // rib tab pocket depth in the band top
// pods
CUP_R = 14; CUP_X0 = 1.5; CUP_X1 = 11.5;                 // cup front (on the gel pad) / back, in the pod frame
HOLD_R = 16.6; HOLD_X0 = 3.5; HOLD_X1 = 13.6;            // holder ring
ARM_T = 3; ARM_W = 10;                                   // arms: 3 mm thick = gentle spring onto the skin
// strap (rear, adjustable)
ST_I = BOA + 0.7; ST_T = 3.0;                            // strap inner offset (outside the side pieces), thickness
ST_A0 = 145; ST_A1 = 248;                                // strap from +145 deg (left, screwed) to -112 deg (right)
SLOT_A0 = 202; SLOT_A1 = 242; DIAL_A = 222;              // slot along the strap, dial bolt at -138 deg
SER0 = 197; SER1 = 255;                                  // serrated clamp zone on side_R (in strap angles)

// colours (your filaments)
MG = [0.17, 0.19, 0.24]; WH = [0.94, 0.95, 0.96]; BL = [0.07, 0.34, 0.82];
C_HEAD = [0.76, 0.71, 0.66]; C_GEL = [0.97, 0.96, 0.88];

// ================= helpers =================
// split a part into its colour regions: region() = where colour C2 goes, the rest is C1
module colsplit(c1, c2) {
  if ($col == "ALL") children(0);
  else if ($col == c2) intersection() { children(0); children(1); }
  else if ($col == c1) difference() { children(0); children(1); }
}
function E(a, b, th) = [a * cos(th), b * sin(th)];
function enorm(a, b, th) = atan2(sin(th) / b, cos(th) / a);
module wedge(t0, t1) { n = max(2, ceil(abs(t1 - t0) / 3)); polygon(concat([[0, 0]], [for (i = [0:n]) let(t = t0 + (t1 - t0) * i / n) 600 * [cos(t), sin(t)]])); }
module ering2d(ia, ib, oa, ob) { difference() { scale([oa, ob]) circle(1, $fn = 240); scale([ia, ib]) circle(1, $fn = 240); } }
// ring sector: offsets measured from the band inner ellipse
module sector(o0, o1, t0, t1, z0, z1) { translate([0, 0, z0]) linear_extrude(z1 - z0) intersection() { ering2d(BIA + o0, BIB + o0, BIA + o1, BIB + o1); wedge(t0, t1); } }
module at_band(th, off, z) { p = E(BIA + off, BIB + off, th); translate([p[0], p[1], z]) rotate([0, 0, enorm(BIA + off, BIB + off, th)]) children(); }
module pod_frame(i) { translate(PODS[i][1]) rotate([0, -PODS[i][3], PODS[i][2]]) children(); }
function pod_i(name) = search([name], PODS)[0];
module oriented(q, az, el) { translate(q) rotate([0, -el, az]) children(); }

// ================= pod holder + cup =================
module teardrop2d(r) { union() { circle(r); polygon([[r * cos(-45), r * sin(-45)], [0, -r / sin(45)], [-r * cos(-45), r * sin(-45)]]); } }
// holder in the pod frame (x = outward normal). Pointed top + bottom so it prints upside down
// without supports. slot_a = direction of the lead slot (0 = up towards the arm).
module holder2d() { hull() { circle(HOLD_R, $fn = 72); translate([0, HOLD_R / sin(42) + 0.5]) circle(0.5); translate([0, -(CUP_R / sin(45) + 0.7)]) circle(0.5); } }
module bore2d() { union() { circle(CUP_R + CLR / 2, $fn = 72); polygon([[(CUP_R + CLR / 2) * cos(-45), (CUP_R + CLR / 2) * sin(-45)], [0, -(CUP_R + CLR / 2) / sin(45)], [-(CUP_R + CLR / 2) * cos(-45), (CUP_R + CLR / 2) * sin(-45)]]); } }
module yz(x0, h) { translate([x0, 0, 0]) rotate([0, 90, 0]) rotate([0, 0, 90]) linear_extrude(h) children(); }   // 2D (u,v) -> (y,z)
module holder(slot_a = 0) {
  difference() {
    yz(HOLD_X0, HOLD_X1 - HOLD_X0) holder2d();
    yz(HOLD_X0 - 1, CUP_X1 + 0.1 - HOLD_X0 + 1) bore2d();
    rotate([slot_a, 0, 0]) translate([HOLD_X0 - 1.03, -WIRE_W / 2 - 0.013, 0.017]) cube([CUP_X1 - HOLD_X0 + 1.07, WIRE_W + 0.026, HOLD_R + 10]);   // lead slot
  }
}
module cup() {   // pod frame: x from CUP_X0 (gel pad side) to CUP_X1
  difference() {
    translate([CUP_X0, 0, 0]) rotate([0, 90, 0]) cylinder(r = CUP_R, h = CUP_X1 - CUP_X0, $fn = 72);
    translate([CUP_X0 - 1, 0, 0]) rotate([0, 90, 0]) cylinder(d = STUD_D, h = 5, $fn = 32);                  // stud hole
    translate([CUP_X0 + FRONT_T, 0, 0]) rotate([0, 90, 0]) cylinder(d = SNAP_D, h = SNAP_H + 5, $fn = 64);  // snap clip pocket
    translate([CUP_X0 + FRONT_T, -WIRE_W / 2, 0]) cube([20, WIRE_W, CUP_R + 1]);                           // lead slot
    translate([CUP_X0 - 0.01, 0, 0]) rotate([0, 90, 0]) rotate_extrude($fn = 72) translate([CUP_R - 0.8, 0]) polygon([[0, 0], [1, 0], [1, 0.8]]);   // front edge chamfer
  }
}
module cup_ring() { translate([CUP_X0 - 0.01, -50, -50]) cube([0.81, 100, 100]); }
module cupc() { colsplit("WH", "BL") { cup(); cup_ring(); } }
module arm(name) {
  pts = ARMS[search([name], ARMS)[0]][1];
  for (i = [0:len(pts) - 2]) hull() {
    oriented(pts[i][0], pts[i][1], pts[i][2]) cube([ARM_T, ARM_W, 1], center = true);
    oriented(pts[i + 1][0], pts[i + 1][1], pts[i + 1][2]) cube([ARM_T, ARM_W, 1], center = true);
  }
}

// ================= v3: extendable stems with a SCREW LOCK =================
// stem frame: s = along the stem (0 = pod centre, + towards the band), w = across, t = thickness
// (t = 0 tongue skin-side face, t = T_TG outer face with the blue marks = holder back face)
// v3 lock (v2's spring pawl only overlapped the ridges ~0.3 mm -> it slipped):
//   - V-grooves every 2 mm across the tongue's skin side (0.8 deep)
//   - a V-tooth on the sleeve floor (0.7 high) right under ...
//   - ... an M3 thumb screw in the sleeve's outer wall (M3 nut in a pocket from the channel side)
//   Loose: the tongue lifts 1 mm over the tooth and slides. Tight: the screw presses it onto the tooth
//   = locked in 2 mm steps (it would have to climb the tooth against the screw to move).
T_TG = 3.0; PITCH = 2.0; WALL = 1.6;
GROOVE = 0.8; TOOTH = 0.7; LIFT = 1.0;                  // groove depth, tooth height, free lift of the tongue
CLR_W = 0.5;                                            // channel = tongue width + CLR_W
CH_T0 = -0.05; CH_T1 = T_TG + LIFT;                     // channel in the sleeve (t); floor 0.05 under the arm face (no coplanar faces)
LOCK_DS = 3.0;                                          // lock (tooth + screw) this far inside the sleeve mouth
BOSS_H = 2.4; BOSS_R = 4.6;                             // screw boss on the outer wall
NUT_AF = 5.5 + 0.25; NUT_H = 2.4 + 0.2; SCREW_D = 3.4;  // M3 nut pocket (from the channel side), clearance hole
function st_i(name) = search([name], STEMS)[0];
function chw(st) = st[7] + CLR_W;
function lock_s(st) = st[4] + LOCK_DS;
module in_stem(st) { multmatrix(st[2]) children(); }
// boss outline (s, w): round with a point towards +s (= down when the band pieces print) -> no overhang
module boss2d(r) { hull() { circle(r, $fn = 40); translate([r * 1.414, 0]) circle(0.3, $fn = 12); } }
module sleeve_local(st, w) {
  SB = st[4]; LS = st[5]; CW = chw(st); sl = lock_s(st);
  difference() {
    union() {
      translate([SB, w - CW / 2 - WALL, CH_T0 - WALL]) cube([LS, CW + 2 * WALL, CH_T1 - CH_T0 + 2 * WALL]);
      translate([sl, w, CH_T1 + WALL - 0.01]) linear_extrude(BOSS_H + 0.01) boss2d(BOSS_R);
    }
    sleeve_cuts(st, w);
  }
}
module sleeve_cuts(st, w) {   // channel (open at the pod end, blind at the band end), nut pocket, screw hole
  SB = st[4]; LS = st[5]; CW = chw(st); sl = lock_s(st);
  translate([SB - 1, w - CW / 2, CH_T0]) cube([LS - WALL + 1.07, CW, CH_T1 - CH_T0]);
  translate([sl, w, CH_T1 - 0.01]) rotate([0, 0, 90]) linear_extrude(NUT_H + 0.01) circle(d = NUT_AF / cos(30), $fn = 6);  // flats across s
  translate([sl, w, CH_T1 - 1]) cylinder(d = SCREW_D, h = WALL + BOSS_H + 2, $fn = 24);
}
module lock_tooth(st, w) {   // V-tooth across the channel floor at the lock
  CW = chw(st);
  translate([0, w + CW / 2, 0]) rotate([90, 0, 0]) linear_extrude(CW) polygon([[lock_s(st) - TOOTH - 0.02, -0.3], [lock_s(st) + TOOTH + 0.02, -0.3], [lock_s(st) + TOOTH, 0], [lock_s(st), TOOTH], [lock_s(st) - TOOTH, 0]]);
}
module sleeve(name) { st = STEMS[st_i(name)]; in_stem(st) for (w = st[8]) sleeve_local(st, w); }
module channel_cut(name) { st = STEMS[st_i(name)]; in_stem(st) for (w = st[8]) sleeve_cuts(st, w); }
module teeth(name) { st = STEMS[st_i(name)]; in_stem(st) for (w = st[8]) lock_tooth(st, w); }
// tongue (+ nose fork) in the stem frame; grooves on the skin side, blue marks on the outer face
function tg_end(st) = st[4] + st[6];
function tg_start(st) = st[9] >= 0 ? st[9] : 12.4;
module grooves(st, w) {   // V-grooves 90 deg, 0.8 deep, every 2 mm (phase: one groove sits on the tooth at the design length)
  TW = st[7]; s1 = tg_end(st) - 1.2;
  for (k = [-30:30]) let(sk = lock_s(st) + k * PITCH) if (sk + GROOVE <= s1 && sk - GROOVE >= tg_start(st) + 1)
    translate([0, w + TW / 2 + 1, 0]) rotate([90, 0, 0]) linear_extrude(TW + 2) polygon([[sk - GROOVE - 0.01, -1], [sk + GROOVE + 0.01, -1], [sk + GROOVE, 0], [sk, GROOVE], [sk - GROOVE, 0]]);
}
module marks2d(st, w) {   // every 2 mm, long at the design length (sleeve edge) and every 10 mm
  TW = st[7];
  for (j = [-12:12]) let(sk = st[4] + j * PITCH) if (sk > max(tg_start(st) + 2, HOLD_R + 1.5) && sk < tg_end(st) - 1.5)
    translate([sk - 0.35, w - (j % 5 == 0 ? TW / 2 - 0.8 : 1.6)]) square([0.7, j % 5 == 0 ? TW - 1.6 : 3.2]);
}
module marks(st) { for (w = st[8]) translate([0, 0, T_TG - 0.6]) linear_extrude(0.61) marks2d(st, w); }
module tongue(st) {
  TW = st[7];
  difference() {
    union() {
      for (w = st[8]) translate([tg_start(st), w - TW / 2, 0]) cube([tg_end(st) - tg_start(st), TW, T_TG]);
      if (st[9] < 0) for (w = st[8]) hull() {                                                              // nose: fork arms from the ring to both tongues
        translate([HOLD_R * cos(50) - 3, sign(w) * HOLD_R * sin(50) - 3, 0]) cube([6, 6, T_TG]);
        translate([tg_start(st), w - TW / 2, 0]) cube([6, TW, T_TG]);
      }
    }
    for (w = st[8]) grooves(st, w);
    for (w = st[8]) translate([0, w + TW / 2 + 1, 0]) rotate([90, 0, 0]) linear_extrude(TW + 2) polygon([[tg_end(st) - 3, -1], [tg_end(st) + 1, -1], [tg_end(st) + 1, 1.0], [tg_end(st) - 3, 0]]);   // ramp under the tip: rides over the tooth
  }
}
module holder_round(slot_a) {   // slider holder: round (prints flat, bore vertical)
  difference() {
    yz(HOLD_X0, HOLD_X1 - HOLD_X0) circle(HOLD_R, $fn = 72);
    yz(HOLD_X0 - 1, CUP_X1 + 0.1 - HOLD_X0 + 1) circle(CUP_R + CLR / 2, $fn = 72);
    rotate([slot_a, 0, 0]) translate([HOLD_X0 - 1.03, -WIRE_W / 2 - 0.013, 0.017]) cube([CUP_X1 - HOLD_X0 + 1.07, WIRE_W + 0.026, HOLD_R + 10]);   // lead slot
  }
}
module slider_all(st) { union() { pod_frame(st[1]) holder_round(st[12]); in_stem(st) tongue(st); } }
module slider_marks(st) { in_stem(st) marks(st); }
module slider(name) { st = STEMS[st_i(name)]; colsplit("MG", "BL") { slider_all(st); slider_marks(st); } }
// placed in the assembly, moved along its stem: dl > 0 = longer (pod further from the band)
module slider_at(name, dl = 0) { st = STEMS[st_i(name)]; in_stem(st) translate([-dl, 0, 0]) multmatrix(st[3]) slider(name); }
// print: outer face (holder back + tongue outer face) on the bed
module slider_print(name) { st = STEMS[st_i(name)]; multmatrix([[1, 0, 0, 0], [0, -1, 0, 0], [0, 0, -1, T_TG], [0, 0, 0, 1]]) multmatrix(st[3]) slider(name); }
// stem test: a short nose-size tongue with grooves + marks and a grip tab (fit check for the lock)
module stem_test_local() {
  st = STEMS[st_i("nose")]; TW = st[7];
  stt = [st[0], st[1], st[2], st[3], 14, 21, 14, TW, [0], 2, 3, 8, 0];
  difference() { translate([2, -TW / 2, 0]) cube([26, TW, T_TG]); grooves(stt, 0); }
  translate([-10, -9, 0]) cube([12.5, 18, T_TG]);
}
// thumb knob for the lock screw: M3x8 socket head cap screw glued in head-first (knob bottom 2 mm)
KNOB_D = 11; KNOB_H = 5.4; KNOB_B = 2.0;
module knob() {
  difference() {
    union() {
      cylinder(d = KNOB_D, h = KNOB_H - 0.8, $fn = 48);
      translate([0, 0, KNOB_H - 0.8]) cylinder(d1 = KNOB_D, d2 = KNOB_D - 1.6, h = 0.8, $fn = 48);
    }
    for (a = [0:30:330]) rotate([0, 0, a]) translate([KNOB_D / 2 + 0.35, 0, -1]) cylinder(d = 1.6, h = KNOB_H + 2, $fn = 12);   // grip
    translate([0, 0, KNOB_B]) cylinder(d = 5.75, h = KNOB_H, $fn = 40);                                                      // screw head
    translate([0, 0, -1]) cylinder(d = 3.3, h = KNOB_B + 2, $fn = 24);
  }
}
// knobs in place on every sleeve (assembly / renders); LOCKED = screw down on the tongue
// M3x8 locked: screw tip on the tongue, knob 1 mm above the boss
module knobs_at() { for (n = SLIDERS) let(st = STEMS[st_i(n)]) in_stem(st) for (w = st[8]) translate([lock_s(st), w, T_TG + 8 - KNOB_B]) knob(); }
module stem_test() { colsplit("MG", "BL") { stem_test_local(); let(st = STEMS[st_i("nose")]) marks([st[0], st[1], st[2], st[3], 14, 21, 14, st[7], [0], 2, 3, 8, 0]); } }

// ================= band pieces =================
module rib_pocket(th) {   // pocket for a rib tab at the band top + radial screw hole
  psi = atan2(RB * sin(th), RA * cos(th)); h0 = sqrt(pow(RA * cos(th), 2) + pow(RB * sin(th), 2)) * cos(asin(ZT / RC));
  rotate([0, 0, psi]) {
    translate([h0, 0, ZT - POCKET_D / 2 + 0.01]) cube([RIB_T + 0.25, RIB_W + 0.4, POCKET_D + 0.02], center = true);
    translate([h0, 0, ZT - POCKET_D / 2]) rotate([0, 90, 0]) cylinder(d = 2.5, h = 40, center = true);       // M3 pilot through band + tab
  }
}
module joint_screws(sgn, pilot) {   // 2 vertical M3 at the front<->side lap
  for (a = [JOINT - LAP / 2, JOINT + LAP / 2]) at_band(sgn * a, BAND_T / 2, 0) {
    if (pilot) translate([0, 0, -ZT - 1]) cylinder(d = 2.5, h = ZT + 1.01);                                   // into the lower tab
    else { translate([0, 0, -0.5]) cylinder(d = 3.4, h = ZT + 1); translate([0, 0, ZT - 1.8]) cylinder(d1 = 3.4, d2 = 6.6, h = 1.81); }
  }
}
module front() { union() { front_body(); for (n = ["temple_L", "temple_R", "nose"]) teeth(n); } }
module front_body() {
  difference() {
    union() {
      sector(0, BAND_T, -(JOINT - LAP), JOINT - LAP, -ZT, ZT);
      for (s = [-1, 1]) sector(0, BAND_T, s > 0 ? JOINT - LAP : -(JOINT + LAP), s > 0 ? JOINT + LAP : -(JOINT - LAP), 0, ZT);   // upper lap tabs
      pod_frame(pod_i("forehead")) holder(-135);
      arm("temple_L"); arm("temple_R");                                                                      // v2: upper arms ...
      sleeve("temple_L"); sleeve("temple_R"); sleeve("nose");                                                // ... + sleeves for the sliders
      for (s = [-1, 1]) translate([BOA - 3, s * CR_Y, CR_SZ]) rotate([0, 90, 0]) cylinder(d = 8, h = CR_X0 - BOA + 3, $fn = 4);   // crest stand-offs (up to the flat crest)
    }
    // forehead holder: open the band where the cup goes in
    pod_frame(pod_i("forehead")) translate([HOLD_X0 - 6, 0, 0]) rotate([0, 90, 0]) cylinder(r = CUP_R + CLR / 2, h = CUP_X1 - HOLD_X0 + 6.1);
    pod_frame(pod_i("forehead")) rotate([-135, 0, 0]) translate([HOLD_X0 - 6, -WIRE_W / 2 - 0.1, 0]) cube([CUP_X1 - HOLD_X0 + 6, WIRE_W + 0.2, HOLD_R + 2]);   // v3: stops before the nose sleeve
    for (s = [-1, 1]) at_band(s * 46, BAND_T - 7, 0) rotate([0, 90, 0]) cylinder(d = 2.5, h = 10);                 // temple cap screws (pilot)
    for (s = [-1, 1]) rib_pocket(s * 38);
    // spine: shallow pocket (the forehead cup is right below)
    translate([RA * cos(asin(ZT / RC)), 0, ZT - 2.5 + 0.01]) cube([RIB_T + 0.25, RIB_W + 0.4, 5.02], center = true);
    translate([RA * cos(asin(ZT / RC)), 0, ZT - 2.6]) rotate([0, 90, 0]) cylinder(d = 2.5, h = 30, center = true);
    for (s = [-1, 1]) joint_screws(s, false);
    for (s = [-1, 1]) translate([BOA - 4, s * CR_Y, CR_SZ]) rotate([0, 90, 0]) cylinder(d = 2.5, h = CR_X0 - BOA + 4.1);   // crest screws (pilot)
    // head side stays smooth: nothing sticks inside the band ellipse except the cups
    for (n = ["temple_L", "temple_R", "nose"]) channel_cut(n);
  }
}
module channel_cut(name) { st = STEMS[st_i(name)]; in_stem(st) for (w = st[8]) translate([st[4] - 1, w - chw(st) / 2, CH_T0]) cube([st[5] - WALL + 1, chw(st), CH_T1 - CH_T0]); }
module side(sgn) { union() { side_body(sgn); teeth(sgn > 0 ? "ref_L" : "ref_R"); } }
module side_body(sgn) {
  difference() {
    union() {
      sector(0, BAND_T, sgn > 0 ? JOINT + LAP : -SIDE_END, sgn > 0 ? SIDE_END : -(JOINT + LAP), -ZT, ZT);
      sector(0, BAND_T, sgn > 0 ? JOINT - LAP : -(JOINT + LAP), sgn > 0 ? JOINT + LAP : -(JOINT - LAP), -ZT, 0);   // lower lap tab
      arm(sgn > 0 ? "ref_L" : "ref_R");                                                                   // v2: upper arm + sleeve
      sleeve(sgn > 0 ? "ref_L" : "ref_R");
      if (sgn < 0) for (a = [SER0 - 360 + 2:1.3:min(SER1 - 360, -SIDE_END + 360) - 1]) if (a > -SIDE_END + 0.3 && a < -100)
        sector(BAND_T - 0.2, BAND_T + 0.55, a, a + 0.55, -ZT + 1, ZT - 1);                                         // serrations (clamp grip)
    }
    rib_pocket(sgn * 142);
    channel_cut(sgn > 0 ? "ref_L" : "ref_R");
    joint_screws(sgn, true);
    if (sgn > 0) for (a = [149, 156]) at_band(a, BAND_T - 7, 0) rotate([0, 90, 0]) cylinder(d = 2.5, h = 10);   // strap screws (pilot)
    if (sgn < 0) at_band(DIAL_A - 360, 0, 0) {                                                               // dial bolt: M4 + nut trap on the inside
      translate([-1, 0, 0]) rotate([0, 90, 0]) cylinder(d = 4.4, h = BAND_T + 2);
      translate([-0.01, 0, 0]) rotate([0, 90, 0]) rotate([0, 0, 30]) cylinder(d = 8.1, h = 3.6, $fn = 6);
    }
  }
}
module strap() {
  difference() {
    union() {
      sector(ST_I - BIA + BAND_T - BAND_T, ST_I - BIA + ST_T, ST_A0, ST_A1, -ZT, ZT);                          // (offsets from BIA)
      for (a = [SLOT_A0 - 3:1.3:ST_A1 - 1]) sector(ST_I - BIA - 0.55, ST_I - BIA + 0.2, a + 0.65, a + 1.2, -ZT + 1, ZT - 1);   // inner serrations
    }
    // slot for the dial bolt: diamond section = prints without support
    for (a = [SLOT_A0:2:SLOT_A1 - 2]) hull() for (q = [a, a + 2]) at_band(q, ST_I - BIA - 1, 0) rotate([0, 90, 0]) cylinder(r = 3.1, h = ST_T + 2, $fn = 4);
    for (a = [149, 156]) at_band(a, ST_I - BIA - 1, 0) rotate([0, 90, 0]) { cylinder(d = 3.4, h = ST_T + 2); translate([0, 0, ST_T - 0.6]) cylinder(d1 = 3.4, d2 = 6.4, h = 1.61); }
  }
}

// ================= ribs + crown hub =================
function rtheta(th) = sqrt(pow(RA * cos(th), 2) + pow(RB * sin(th), 2));
function rpsi(th) = atan2(RB * sin(th), RA * cos(th));
module rib2d(th) {   // in its own plane: x = horizontal distance from the head axis, y = height
  R = rtheta(th); ph0 = asin(ZT / RC);
  n = 40;
  outer = [for (i = [0:n]) let(p = ph0 + (PH1 - ph0) * i / n, c = [R * cos(p), RC * sin(p)], t = [-R * sin(p), RC * cos(p)], nn = [t[1], -t[0]] / norm(t)) c + nn * RIB_T / 2];
  inner = [for (i = [n:-1:0]) let(p = ph0 + (PH1 - ph0) * i / n, c = [R * cos(p), RC * sin(p)], t = [-R * sin(p), RC * cos(p)], nn = [t[1], -t[0]] / norm(t)) c - nn * RIB_T / 2];
  polygon(concat(outer, inner));
  // bottom tab into the band pocket
  tabl = (th == 0) ? 4.6 : POCKET_D - 0.4;
  translate([R * cos(ph0) - RIB_T / 2, ZT - tabl]) square([RIB_T, tabl + 0.4]);
  // top tab into the hub pocket (along the rib)
  c1 = [R * cos(PH1), RC * sin(PH1)]; t1 = [-R * sin(PH1), RC * cos(PH1)] / norm([-R * sin(PH1), RC * cos(PH1)]);
  translate(c1) rotate(atan2(t1[1], t1[0])) translate([-1, -RIB_T / 2]) square([10.5, RIB_T]);
}
module rib(th) { rotate([0, 0, rpsi(th)]) rotate([90, 0, 0]) linear_extrude(RIB_W, center = true) rib2d(th); }
module rib_print(th) { linear_extrude(RIB_W) rib2d(th); }
RIBS = [0, 38, -38, 142, -142];
module hub() {
  difference() {
    translate([0, 0, HUB_Z0]) linear_extrude(HUB_Z1 - HUB_Z0) scale([HUB_A + 1.5, HUB_B + 1.5]) circle(1, $fn = 120);
    for (th = RIBS) let(R = rtheta(th), c1 = [R * cos(PH1), RC * sin(PH1)], t1 = [-R * sin(PH1), RC * cos(PH1)] / norm([-R * sin(PH1), RC * cos(PH1)]))
      rotate([0, 0, rpsi(th)]) translate([c1[0], 0, c1[1]]) rotate([0, -atan2(t1[1], t1[0]), 0]) translate([-3, -(RIB_W + 0.4) / 2, -(RIB_T + 0.3) / 2]) cube([14.5, RIB_W + 0.4, RIB_T + 0.3]);
    for (th = RIBS) let(R = rtheta(th), c1 = [R * cos(PH1), RC * sin(PH1)]) rotate([0, 0, rpsi(th)]) translate([c1[0] - 6, 0, HUB_Z1 - 12]) cylinder(d = 2.5, h = 13);   // M3 pilots down into the rib tabs
    translate([0, 0, HUB_Z1 - 2]) linear_extrude(3) scale([HUB_A - 4.5, HUB_B - 4.5]) circle(1, $fn = 120);          // cap + ring seat
  }
}
module hub_cap_all() { linear_extrude(2) scale([HUB_A - 8.2, HUB_B - 8.2]) circle(1, $fn = 120); }
module hub_N() { translate([0, 0, 1.2]) linear_extrude(0.81) rotate(-90) text("N", size = 22, font = "DejaVu Sans:style=Bold", halign = "center", valign = "center"); }
module hub_cap() { colsplit("WH", "BL") { hub_cap_all(); hub_N(); } }
module hub_ring() { linear_extrude(2) difference() { scale([HUB_A - 4.5 - CLR, HUB_B - 4.5 - CLR]) circle(1, $fn = 120); scale([HUB_A - 8.2 + CLR, HUB_B - 8.2 + CLR]) circle(1, $fn = 120); } }

// ================= crest, covers, dial, back hub, clips =================
// v2 crest: a faceplate in the NeuroX look - the Reticle N emblem (same geometry as the
// box lid and the logo files), the wordmark in Orbitron, a blue edge line. Same 2 screws,
// same stand-offs, same place; printed face down, the 4 colours are the first 4 layers.
CR_X0 = BOA + 4.6; CR_Y = 9.5; CR_SZ = 4;          // flat crest: inner face x, screw positions (unchanged)
CR_T = 3; INLAY = 0.81;                            // plate thickness, inlay depth (first 4 layers)
EMB_Z = -8; EMB_R = 11.3; EMB_K = EMB_R / 52.75;   // emblem centre (z), ring radius, logo scale (120-unit logo)
WORD_Z = 16.5; WORD_SIZE = 4.2;                    // wordmark above the band line
module crest_shape2d() { offset(r = 1.5) offset(delta = -1.5) polygon([[-17, 22], [17, 22], [21, 18], [22, 6], [15, -19], [6, -28], [0, -31], [-6, -28], [-15, -19], [-22, 6], [-21, 18]]); }
module crest_word2d() { translate([0, WORD_Z]) text("NEUROX", size = WORD_SIZE, font = "Orbitron Black", halign = "center", valign = "center"); }
module emb(k) { translate([0, EMB_Z]) scale([EMB_K, EMB_K]) children(); }
module crest_red2d() { emb() offset(r = 0.8) logo_dot(); }   // a touch bigger: clean overlap with the ring
module crest_tip2d() { intersection() { offset(delta = -3.2) crest_shape2d(); translate([-30, -40]) square([60, 18.6]); } }   // midnight visor tip
module crest_mg2d() { difference() { union() { emb() logo_stems(); crest_word2d(); crest_tip2d(); } crest_red2d(); } }
module crest_bl2d() {
  difference() {
    union() {
      difference() { offset(delta = -1.4) crest_shape2d(); offset(delta = -2.3) crest_shape2d(); }   // edge line
      translate([-14.5, WORD_Z - 4.6]) square([29, 0.9]);                                              // line under the wordmark
      intersection() { offset(delta = -2.3) crest_shape2d(); translate([-30, -20.3]) square([60, 0.9]); }   // line over the tip
      emb() { logo_ring(); offset(r = 0.9) logo_ticks(); logo_diag(); }                              // ticks thickened for a 0.4 nozzle
    }
    crest_mg2d(); crest_red2d();
  }
}
module crest_face(c) { translate([CR_X0 + CR_T - INLAY, 0, 0]) rotate([90, 0, 90]) linear_extrude(INLAY + 0.2)
  if (c == "BL") crest_bl2d(); else if (c == "MG") crest_mg2d(); else crest_red2d(); }
module crest_all() {   // FLAT plate in front of the forehead holder, printed face down
  difference() {
    translate([CR_X0, 0, 0]) rotate([90, 0, 90]) linear_extrude(CR_T) crest_shape2d();
    for (s = [-1, 1]) translate([CR_X0 - 1, s * CR_Y, CR_SZ]) rotate([0, 90, 0]) { cylinder(d = 3.4, h = 6); translate([0, 0, 2.41]) cylinder(d1 = 3.4, d2 = 6.6, h = 1.6); }
  }
}
module crest() {
  if ($col == "ALL") crest_all();
  else if ($col == "WH") difference() { crest_all(); for (c = ["BL", "MG", "RD"]) crest_face(c); }
  else intersection() { crest_all(); crest_face($col); }
}
module temple_cap_all() {   // white hex cap over the lead turning down to the temple arm (face down on the bed)
  difference() {
    cylinder(r = 12.5, h = 10, $fn = 6);
    translate([0, 0, 2]) cylinder(r = 10.5, h = 10, $fn = 6);
    translate([0, 0, -1]) cylinder(d = 3.4, h = 4);                                   // M3 into the band
    translate([0, 0, -0.01]) cylinder(d = 6.2, h = 1.8);                             // pan head sits flush
    for (s = [-1, 1]) translate([0, s * 10, 8]) cube([9, 6, 6], center = true);        // leads in (sides)
    translate([-10, 0, 8]) cube([6, 5, 6], center = true);                           // lead out (down)
  }
}
module temple_ring() { translate([0, 0, -0.01]) difference() { cylinder(r = 7.6, h = 0.81); translate([0, 0, -1]) cylinder(r = 5.0, h = 3); } }
module temple_cap() { colsplit("WH", "BL") { temple_cap_all(); temple_ring(); } }
module dial_all() {
  difference() {
    cylinder(r = 15, h = 11, $fn = 72);
    for (a = [0:15:359]) rotate([0, 0, a]) translate([15.6, 0, -1]) cylinder(r = 1.5, h = 13, $fn = 12);
    translate([0, 0, -1]) cylinder(d = 4.4, h = 6);
    translate([0, 0, 4]) rotate([0, 0, 30]) cylinder(d = 8.5, h = 8, $fn = 6);     // M4 hex bolt head (7 mm across flats) goes in from the top  v3.2: was 7.4 = too small
  }
}
module dial_ring() { translate([0, 0, 10.2]) difference() { cylinder(r = 12.6, h = 1); translate([0, 0, -1]) cylinder(r = 7.4, h = 3); } }
module dial() { colsplit("WH", "BL") { dial_all(); dial_ring(); } }
HB_A = 16; HB_Z0 = -30; HB_Z1 = 14;     // back hub: +/-16 deg around the back, z range
module back_hub() {
  o0 = ST_I - BIA - 2.4; oc = ST_I - BIA + ST_T + 1.5; o1 = ST_I - BIA + 25;
  difference() {
    intersection() { sector(o0, o1, 180 - HB_A, 180 + HB_A, HB_Z0, HB_Z1); translate([XB, -100, -100]) cube([200, 200, 200]); }
    sector(ST_I - BIA - 0.3, ST_I - BIA + ST_T + 0.3, 170 - HB_A, 190 + HB_A, -ZT - 0.3, ZT + 0.3);          // strap tunnel
    sector(oc, o1 + 1, 180 - HB_A + 2.5, 180 + HB_A - 2.5, HB_Z0 + 4, HB_Z1 - 2.5);                           // cavity (open front)
    for (s = [-1, 1]) sector(ST_I - BIA + ST_T - 0.5, oc + 6, s > 0 ? 180 + HB_A - 3 : 170 - HB_A, s > 0 ? 190 + HB_A : 180 - HB_A + 3, -8, 8);   // lead exits
    for (s = [-1, 1]) at_band(180 + s * 6, oc + 4.5, HB_Z0 - 1) cylinder(d = 4.8, h = 6);                     // cable glands
    for (s = [-1, 1]) for (d = [-1, 1]) at_band(180 + s * 6 + d * 3.6, oc + 4.5, HB_Z0 - 1) translate([0, 0, 0]) cube([2.2, 4.2, 12], center = true);   // zip-tie slots
    at_band(180, ST_I - BIA + ST_T / 2, ZT + 0.2) cylinder(d = 2.5, h = 10);                                   // M3 set screw onto the strap
  }
  for (s = [-1, 1]) for (z = [HB_Z0 + 8, HB_Z1 - 7]) difference() {                                             // screw bosses (along x, to the flat face)
    translate([XB, s * (HB_W - 7), z]) rotate([0, 90, 0]) cylinder(d = 7, h = 6.5);
    translate([XB - 1, s * (HB_W - 7), z]) rotate([0, 90, 0]) cylinder(d = 2.5, h = 9);
  }
}
XB = -(BIA + ST_I - BIA + 15) * cos(HB_A) - 0.0;     // the hub's flat back face (x)
HB_W = 34;                                          // cover half width (y)
module back_cover_all() {
  difference() {
    translate([XB - 3, 0, 0]) rotate([90, 0, 90]) linear_extrude(3) offset(r = 4) offset(delta = -4) translate([-HB_W, HB_Z0]) square([2 * HB_W, HB_Z1 - HB_Z0]);
    for (s = [-1, 1]) for (z = [HB_Z0 + 8, HB_Z1 - 7]) translate([XB - 4, s * (HB_W - 7), z]) rotate([0, 90, 0]) { cylinder(d = 3.4, h = 6); translate([0, 0, 1]) cylinder(d2 = 3.4, d1 = 6.4, h = 1.61); }
  }
}
module back_N_shape() { translate([XB - 3 - 0.01, 0, -8]) rotate([90, 0, -90]) mirror([0, 0, 1]) linear_extrude(0.81) text("N", size = 17, font = "DejaVu Sans:style=Bold", halign = "center", valign = "center"); }
module back_cover() { colsplit("WH", "BL") { back_cover_all(); back_N_shape(); } }
module band_clip() {   // C-clip over the band / strap edges, leads run under it (print 14)
  linear_extrude(6) polygon([[-2.6, ZT + 0.2], [5.4, ZT + 0.2], [5.4, -ZT - 0.2], [-2.6, -ZT - 0.2], [-2.9, -ZT - 1.2], [-2.0, -ZT - 2.0], [7.2, -ZT - 2.0], [7.2, ZT + 2.0], [-2.0, ZT + 2.0], [-2.9, ZT + 1.2]]);
}
module arm_clip() {    // C-clip around an arm, the lead runs on the outside (print 10)
  linear_extrude(5) difference() {
    offset(r = 1.4) translate([-1.7, -5.2]) square([6.9, 10.4]);
    translate([-1.7, -5.2]) square([6.9, 10.4]);
    translate([-4, -3.5]) square([3, 7]);
  }
}
module fitcheck() {    // print first: one cup in a holder + a rib pocket + a band clip on a band piece
  intersection() { front(); translate([60, -30, -40]) cube([60, 60, 60]); }
}

// ================= head, assembly =================
module head() {
  color(C_HEAD) {
    scale([HA, HB, HC]) sphere(1, $fn = 96);
    hull() { translate([18, 0, -55]) scale([76, 64, 62]) sphere(1, $fn = 56); translate([45, 0, -125]) scale([40, 42, 30]) sphere(1, $fn = 40); }
    translate([-12, 0, -230]) cylinder(r = 50, h = 130);
    hull() { translate([92, 0, -50]) sphere(4); translate([104, 0, -82]) sphere(7); translate([93, 0, -88]) sphere(9); }
    for (s = [-1, 1]) translate([-5, s * 76, -40]) scale([12, 6, 26]) sphere(1);
  }
}
CLIPS = [[15, BAND_T], [30, BAND_T], [58, BAND_T], [90, BAND_T], [115, BAND_T], [134, BAND_T], [168, ST_I - BIA + ST_T],
         [-15, BAND_T], [-30, BAND_T], [-58, BAND_T], [-88, BAND_T], [-168, ST_I - BIA + ST_T]];
module asm_col(c) { color(c == "MG" ? MG : c == "WH" ? WH : BL) children(); }
SLIDERS = ["temple_L", "temple_R", "ref_L", "ref_R", "nose"];
STEM_DL = 0;          // assembly: all sliders moved by this (mm, + = longer)
// a pod (cup, gel pad) follows its slider
module at_pod(i, dl) {
  k = [for (j = [0:len(STEMS) - 1]) if (STEMS[j][1] == i) j];
  if (len(k) == 0) pod_frame(i) children();
  else let(st = STEMS[k[0]]) in_stem(st) translate([-dl, 0, 0]) multmatrix(st[3]) pod_frame(i) children();
}
module assembly() {
  if (SHOW_HEAD) head();
  for (c = ["MG", "WH", "BL"]) asm_col(c) let($col = c) {
    bandcol() { front(); side(1); side(-1); strap(); }
    for (i = [0:5]) at_pod(i, STEM_DL) cupc();
    for (n = SLIDERS) slider_at(n, STEM_DL);
    crest(); translate([0, 0, HUB_Z1 - 2]) hub_cap(); back_cover();
    for (s = [-1, 1]) at_band(s * 46, BAND_T + 10, 0) rotate([0, -90, 0]) temple_cap();
    at_band(DIAL_A, ST_I - BIA + ST_T, 0) rotate([0, 90, 0]) dial();
  }
  color(MG) { for (th = RIBS) rib(th); hub(); }
  color(WH) back_hub();
  color(C_GEL) for (i = [0:5]) at_pod(i, STEM_DL) translate([0.2, 0, 0]) rotate([0, 90, 0]) cylinder(r = 18, h = 1.2);
  color(BL) { translate([0, 0, HUB_Z1 - 2]) hub_ring(); knobs_at(); }
  color(BL) for (c = CLIPS) at_band(c[0], c[1], 0) rotate([90, 0, 0]) translate([0, 0, -3]) band_clip();
  color(BL) for (A = ARMS) let(pts = A[1], n = len(pts)) for (k = [floor(n / 3), floor(2 * n / 3)]) oriented(pts[k][0], pts[k][1], pts[k][2]) translate([0, 0, -2.5]) arm_clip();
}
// ================= export switch (print orientation) =================
module under_top() { intersection() { children(); translate([-300, -300, -300]) cube([600, 600, 300 + ZT]); } }   // nothing above the band top (= the bed)
// band pieces: white top edge + blue stripe = whole layers -> only 4 filament swaps per plate
WH_Z0 = ZT - 3.5; BL_Z0 = -5; BL_Z1 = -2;
module wh_slab() { translate([-300, -300, WH_Z0]) cube([600, 600, 20]); }
module bl_slab() { translate([-300, -300, BL_Z0]) cube([600, 600, BL_Z1 - BL_Z0]); }
module bandcol() {
  if ($col == "ALL") children();
  else if ($col == "WH") intersection() { children(); wh_slab(); }
  else if ($col == "BL") intersection() { children(); bl_slab(); }
  else if ($col == "MG") difference() { children(); wh_slab(); bl_slab(); }
}
module flip() { rotate([180, 0, 0]) translate([0, 0, -ZT]) children(); }
ORIENT = "print";     // print = turned for the bed, model = as worn (for checks / renders)
if (PART == "assembly") assembly();
if (ORIENT == "print") {
  if (PART == "front") flip() bandcol() under_top() front();
  if (PART == "side_L") flip() bandcol() under_top() side(1);
  if (PART == "side_R") flip() bandcol() under_top() side(-1);
  if (PART == "strap") flip() bandcol() strap();
  if (PART == "rib_spine") rib_print(0);
  if (PART == "rib_side") rib_print(38);
  if (PART == "hub") translate([0, 0, -HUB_Z0]) hub();
  if (PART == "hub_cap") translate([0, 0, 2]) rotate([180, 0, 0]) hub_cap();   // face down
  if (PART == "hub_ring") hub_ring();
  if (PART == "cup") rotate([0, -90, 0]) translate([-CUP_X0, 0, 0]) cupc();
  if (PART == "crest") rotate([0, 90, 0]) translate([-CR_X0, 0, 0]) crest();   // face down
  if (PART == "temple_cap") temple_cap();
  if (PART == "dial") translate([0, 0, 11]) rotate([180, 0, 0]) dial();   // top (blue ring) down
  if (PART == "back_hub") translate([0, 0, -HB_Z0]) back_hub();
  if (PART == "back_cover") rotate([0, -90, 0]) translate([-(XB - 3), 0, 0]) back_cover();   // face down
  if (PART == "band_clip") band_clip();
  if (PART == "arm_clip") arm_clip();
  if (PART == "fitcheck") flip() bandcol() under_top() fitcheck();
  if (PART == "slider_temple_L") slider_print("temple_L");
  if (PART == "slider_temple_R") slider_print("temple_R");
  if (PART == "slider_ref_L") slider_print("ref_L");
  if (PART == "slider_ref_R") slider_print("ref_R");
  if (PART == "slider_nose") slider_print("nose");
  if (PART == "stem_test") multmatrix([[1, 0, 0, 0], [0, -1, 0, 0], [0, 0, -1, T_TG], [0, 0, 0, 1]]) stem_test();
  if (PART == "knob") knob();
}
if (ORIENT == "model") {
  if (PART == "front") bandcol() under_top() front();
  if (PART == "side_L") bandcol() under_top() side(1);
  if (PART == "side_R") bandcol() under_top() side(-1);
  if (PART == "strap") bandcol() strap();
  if (PART == "rib_spine") rib(0);
  if (PART == "rib_side") rib(38);
  if (PART == "hub") hub();
  if (PART == "hub_cap") translate([0, 0, HUB_Z1 - 2]) hub_cap();
  if (PART == "hub_ring") translate([0, 0, HUB_Z1 - 2]) hub_ring();
  if (PART == "cup") pod_frame(0) cupc();
  if (PART == "crest") crest();
  if (PART == "temple_cap") at_band(46, BAND_T + 10, 0) rotate([0, -90, 0]) temple_cap();
  if (PART == "dial") at_band(DIAL_A, ST_I - BIA + ST_T, 0) rotate([0, 90, 0]) dial();
  if (PART == "back_hub") back_hub();
  if (PART == "back_cover") back_cover();
  if (PART == "band_clip") at_band(15, BAND_T, 0) rotate([90, 0, 0]) translate([0, 0, -3]) band_clip();
  if (PART == "arm_clip") arm_clip();
  if (PART == "fitcheck") bandcol() under_top() fitcheck();
  if (PART == "slider_temple_L") slider_at("temple_L");
  if (PART == "slider_temple_R") slider_at("temple_R");
  if (PART == "slider_ref_L") slider_at("ref_L");
  if (PART == "slider_ref_R") slider_at("ref_R");
  if (PART == "slider_nose") slider_at("nose");
  if (PART == "stem_test") stem_test();
  if (PART == "knob") knob();
}
