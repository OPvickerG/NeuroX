// =============================================================================
//  NeuroX HALO – RIBS FIX (v3.3)
//  Ribs only: rib_spine, rib_side (x4), crown hub.
//  Band geometry is UNCHANGED so the tabs still fit the already-printed band pockets.
//
//  What changed vs v3.2:
//    HC  108 → 88   (crown height above the band: real heads, not the CAD model)
//    RC  124 → 94   (rib surface crown: only 6 mm above HC, was 16 mm)
//    PH1  70.3 → 66  (ribs meet the hub a bit lower)
//    HUB_Z0  112.5 → 88   (hub body drops ~25 mm)
//    HUB_Z1  125.5 → 101  (hub top drops ~25 mm)
//    Pilot holes (d2.5) added through every rib tab.
//
//  Result: hub sits ~25 mm closer to the head, ribs follow the skull,
//  no cage effect.
//
//  Print: all midnight gray, no supports needed.
//  PART = rib_spine | rib_side | hub | assembly
// =============================================================================

PART = "assembly";
$fn = 48;

// ---------- head / band (DO NOT change – must match the printed band) ----------
HA = 97; HB = 78;
BIA = 102; BIB = 83; BAND_T = 8; BAND_H = 22;
BOA = BIA + BAND_T; BOB = BIB + BAND_T;
ZT = BAND_H / 2;                        // band top z
POCKET_D = 9;                           // rib tab pocket depth (unchanged)

// ---------- rib surface – FIXED ----------
HC  = 88;                               // was 108: shorter crown model
RC  = HC + 6;                           // was HC+16: only 6 mm clearance above head
RA  = 106.4; RB = 87.4;                 // rib ellipse radii (unchanged)
PH1 = 66;                               // was 70.3: ribs meet hub lower

HUB_A  = RA * cos(PH1); HUB_B = RB * cos(PH1);
HUB_Z0 = 88;                            // was 112.5
HUB_Z1 = 101;                           // was 125.5

RIB_T = 4.5; RIB_W = 12;

// ---------- colours ----------
MG = [0.17, 0.19, 0.24];

// ================= helpers =================
function E(a, b, th)   = [a * cos(th), b * sin(th)];
function rtheta(th)    = sqrt(pow(RA * cos(th), 2) + pow(RB * sin(th), 2));
function rpsi(th)      = atan2(RB * sin(th), RA * cos(th));

RIBS = [0, 38, -38, 142, -142];

// ================= rib 2d profile =================
module rib2d(th) {
  R   = rtheta(th);
  ph0 = asin(ZT / RC);
  n   = 40;
  outer = [for (i = [0:n]) let(p = ph0 + (PH1 - ph0) * i / n,
              c = [R * cos(p), RC * sin(p)],
              t = [-R * sin(p), RC * cos(p)],
              nn = [t[1], -t[0]] / norm(t)) c + nn * RIB_T / 2];
  inner = [for (i = [n:-1:0]) let(p = ph0 + (PH1 - ph0) * i / n,
              c = [R * cos(p), RC * sin(p)],
              t = [-R * sin(p), RC * cos(p)],
              nn = [t[1], -t[0]] / norm(t)) c - nn * RIB_T / 2];
  polygon(concat(outer, inner));

  // bottom tab into the band pocket (unchanged geometry)
  tabl = (th == 0) ? 4.6 : POCKET_D - 0.4;
  translate([R * cos(ph0) - RIB_T / 2, ZT - tabl]) square([RIB_T, tabl + 0.4]);

  // top tab into the hub pocket
  c1 = [R * cos(PH1), RC * sin(PH1)];
  t1 = [-R * sin(PH1), RC * cos(PH1)] / norm([-R * sin(PH1), RC * cos(PH1)]);
  translate(c1) rotate(atan2(t1[1], t1[0])) translate([-1, -RIB_T / 2]) square([10.5, RIB_T]);
}

// rib in assembly orientation
module rib(th) {
  rotate([0, 0, rpsi(th)]) rotate([90, 0, 0]) linear_extrude(RIB_W, center = true)
    rib2d(th);
}

// rib oriented for printing (flat face down) with 2.5 mm pilot hole through the tab
module rib_print(th) {
  R   = rtheta(th);
  ph0 = asin(ZT / RC);
  tabl = (th == 0) ? 4.6 : POCKET_D - 0.4;
  tx  = R * cos(ph0);   // tab centre x in the 2D profile
  difference() {
    linear_extrude(RIB_W) rib2d(th);
    // pilot hole through the tab (M3 self-tap: 2.5 mm drill leaves the right wall)
    translate([tx, ZT - tabl / 2, -1]) cylinder(d = 2.5, h = RIB_W + 2, $fn = 20);
  }
}

// ================= crown hub =================
module hub() {
  difference() {
    translate([0, 0, HUB_Z0])
      linear_extrude(HUB_Z1 - HUB_Z0)
        scale([HUB_A + 1.5, HUB_B + 1.5]) circle(1, $fn = 120);

    // rib pockets
    for (th = RIBS) let(
        R  = rtheta(th),
        c1 = [R * cos(PH1), RC * sin(PH1)],
        t1 = [-R * sin(PH1), RC * cos(PH1)] / norm([-R * sin(PH1), RC * cos(PH1)]))
      rotate([0, 0, rpsi(th)])
        translate([c1[0], 0, c1[1]])
          rotate([0, -atan2(t1[1], t1[0]), 0])
            translate([-3, -(RIB_W + 0.4) / 2, -(RIB_T + 0.3) / 2])
              cube([14.5, RIB_W + 0.4, RIB_T + 0.3]);

    // M3 pilots down into the rib tabs (adjusted for new hub height)
    for (th = RIBS) let(
        R  = rtheta(th),
        c1 = [R * cos(PH1), RC * sin(PH1)])
      rotate([0, 0, rpsi(th)])
        translate([c1[0] - 6, 0, HUB_Z1 - 12])
          cylinder(d = 2.5, h = 13);

    // cap + ring seat at the top
    translate([0, 0, HUB_Z1 - 2])
      linear_extrude(3)
        scale([HUB_A - 4.5, HUB_B - 4.5]) circle(1, $fn = 120);
  }
}

// optional: ring + cap (unchanged geometry, just moved with the hub)
module hub_cap_all() { linear_extrude(2) scale([HUB_A - 8.2, HUB_B - 8.2]) circle(1, $fn = 120); }
module hub_ring() {
  linear_extrude(2) difference() {
    scale([HUB_A - 4.5 - 0.3, HUB_B - 4.5 - 0.3]) circle(1, $fn = 120);
    scale([HUB_A - 8.2 + 0.3, HUB_B - 8.2 + 0.3]) circle(1, $fn = 120);
  }
}

// head surface (reference only, not exported)
module head() {
  color([0.76, 0.71, 0.66])
    scale([HA, HB, HC + ZT])
      sphere(1, $fn = 80);
}

// ================= exports =================
// rib_spine: the front-centre rib (th = 0), printed flat
module export_rib_spine() {
  color(MG) rib_print(0);
}

// rib_side: one of the 4 side ribs; use TH = 38, -38, 142, -142
// (all 4 are identical shape; mirror for L/R)
TH = 38;   // override with -D 'TH=142' etc.
module export_rib_side() {
  color(MG) rib_print(TH);
}

// hub print: rotated so the flat bottom faces the bed
module export_hub() {
  color(MG) translate([0, 0, -(HUB_Z0)]) hub();
}

// ================= assembly preview =================
if (PART == "assembly") {
  head();
  color(MG) for (th = RIBS) rib(th);
  color(MG) hub();
  // show the hub ring + cap position
  color([0.07, 0.34, 0.82]) translate([0, 0, HUB_Z1 - 2]) hub_ring();
  color([0.94, 0.95, 0.96]) translate([0, 0, HUB_Z1])     hub_cap_all();
  // band outline (reference)
  color([0.5, 0.5, 0.5], 0.3)
    linear_extrude(BAND_H, center = true)
      difference() {
        scale([BOA, BOB]) circle(1, $fn = 120);
        scale([BIA, BIB]) circle(1, $fn = 120);
      }
}

if (PART == "rib_spine") export_rib_spine();
if (PART == "rib_side")  export_rib_side();
if (PART == "hub")       export_hub();
