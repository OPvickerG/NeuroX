NeuroX HALO headset v3 - print + build guide
============================================
v3 = STEMS WITH A SCREW LOCK (v2's click slipped).

Why v2 slipped: the spring bump in the sleeve overlapped the ridges by only about 0.3 mm,
and printing rounds tiny bumps and ridges off. So the stems slid freely and fell out.

How v3 locks:
  - every stem (tongue) has V-grooves across its skin side, every 2 mm, 0.8 mm deep
  - inside each sleeve, on the floor, there is one V-tooth (0.7 mm)
  - right above the tooth, a small M3 thumb screw in the sleeve wall (M3 nut in a pocket)
  LOOSE  (knob 2 turns out): the stem lifts 1 mm over the tooth and slides
  LOCKED (knob finger-tight): the screw presses the stem down, the tooth sits in a groove.
         To move, the stem would have to climb the tooth against the screw -> it can't slip.
  It still sets in 2 mm steps, and the blue length marks are unchanged.

  push the pod towards the band  = shorter stem
  pull it away                   = longer stem
  temples  -6 / +4 mm
  REF      -6 / +6 mm
  nose     -2 / +10 mm

v3.1 CUP: the gel pad's stud didn't reach the lead's snap. The cup's front wall is now 0.8 mm
(was 1.6) and the stud hole 9.5 mm (was 7), so the stud's base fits in and the stud reaches
deeper into the snap. Reprint the 6 cups (plate 6) and the test cup (plate 1).
  Still not clicking? FRONT_T 0.6 / STUD_D 10.5 at the top of neurox_halo_print.scad.
v3.2 DIAL: the hex pocket for the M4 bolt head was too small (6.4 mm across flats, an M4 hex
head is 7 mm). Fixed to 7.4 mm. Only the dial changed: reprint it from plate 6.
  The M4 nut pocket inside the right side is a snug 7.0 mm: if the nut won't push in, put it on
  the bolt tip and tighten the dial - the bolt pulls the nut into its pocket.
Unchanged from v2: band, ribs, hub, strap, crest, caps, clips. Changed: front band,
both side pieces (sleeves), the 5 sliders, the stem test and the fit check. New: 6 lock knobs.

The headset is passive (no electronics). Your 6 snap leads run in the headset, the gel pads
click on, and the 2 pill cables go to your NeuroX box.
Printer: Bambu Lab A1 mini + AMS lite, PLA, NO supports.

FOLDERS
-------
1_Bambu_Studio_3MF/   8 plates with the colours inside (AMS). Open ONE file at a time.
2_STEP_Fusion360/     every part as a STEP solid, as worn, plus a coloured assembly
                      (NeuroX_HALO_v3_assembly.step). Use these in Fusion 360, Onshape,
                      SolidWorks or FreeCAD.
3_STL/print/          every part turned for printing. part__MG / __WH / __BL / __RD = colour
                      pieces for the AMS; part.stl = the whole part in one colour.
3_STL/as_worn/        the same parts in their place on the head (for checks and other CAD).
4_renders/            concept sheet, lock close-ups, plate previews.
5_source/             OpenSCAD source + scripts. Measurements are at the top of
                      neurox_halo_print.scad.

1. LOAD THE AMS
---------------
   slot 1 midnight gray   slot 2 white   slot 3 blue   slot 4 red (only the small dot on the crest)

2. PRINT (1_Bambu_Studio_3MF)
-----------------------------
   Use File > Open Project, or drag the file onto an EMPTY project. Bambu Studio shows a
   colour -> filament window: check gray -> 1, white -> 2, blue -> 3, red -> 4, then OK.
   No AMS? Open the plates anyway and print every part in one colour.

   plate1_fitcheck   PRINT FIRST (~1 h). It holds:
                     - a piece of the front band with the forehead holder and the two nose
                       sleeves (with the new lock: tooth, nut pocket, screw boss)
                     - a test cup, a test clip, a STEM TEST tongue (grooved, nose size)
                     - 1 lock knob
                     Check:
                     (a) your lead's snap clip sits snugly in the cup pocket
                     (b) a gel pad's stud goes through the cup hole into the snap
                     (c) the cup presses into the holder
                     (d) the clip snaps over the band
                     (e) THE LOCK (see 4e for how to put the nut + knob in):
                         - knob 2 turns out: the stem test slides in and out
                         - knob finger-tight: pull and push hard, shake it - it must not move
                         - loosen, move 1 groove, tighten: it locks again 2 mm further
                     Wrong? Change the values at the top of the stem section of
                     neurox_halo_print.scad, then run export.sh and make_plates.py:
                       stem stuck even with the knob out  -> CLR_W 0.7 (width) or LIFT 1.2
                       nut won't go in / spins            -> NUT_AF 5.9 / 5.6
                       screw tight in the boss            -> SCREW_D 3.6
                     Also check the cup values: SNAP_D / SNAP_H / STUD_D / FRONT_T / CLR.
   plate2_front      front band: forehead holder, 2 temple sleeves, twin nose sleeves    brim ON
   plate3_sides      left + right side with the REF sleeves                               brim ON
                     + SUPPORTS ON for this plate only: tree(auto), "On build plate only".
                     The joint tab at the end of each side (the one with the M3 hole) hangs
                     in the air when printed - that is the "floating regions" warning.
   plate4_strap      rear strap
                     + SUPPORTS ON: tree(auto), "On build plate only" OFF. The long dial slot's
                     ceiling is a 53 mm bridge; the supports stand inside the slot. Push them
                     out with a small screwdriver so the M4 bolt slides the whole length.
   plate5_ribs_hub   5 ribs + crown hub (gray only)
   plate6_white_parts 6 cups, crest, back hub + cover, dial, top cap, 2 temple caps
                     The crest uses all 4 colours. It prints FACE DOWN, so the wordmark,
                     emblem and lines are inlaid in the first layers (crisp, no supports).
   plate7_blue_clips hub ring, 14 band clips, 10 arm clips, 6 lock knobs
   plate8_sliders    the 5 sliders (temple L/R, REF L/R, nose). They print flat on their
                     outer face, grooves up. The blue marks are in the first layers.

   Settings: 0.20 mm Standard @BBL A1M, Bambu PLA (Basic / Matte), wall loops 4,
   infill 25 % gyroid, supports OFF.
   Brim ON (5 mm) on plates 2 + 3: the band pieces stand on their top edge and the sleeves
   point up. The screw bosses have a pointed underside so they print without supports.
   Bambu adds a prime tower. If it doesn't fit, drag it into a free spot (inside the curve of
   the band works).
   About 250-290 g PLA in total incl. purge.

3. BUY
------
   NEW for the stem lock:
     6 x M3x8 socket head cap screws (DIN 912)    one per sleeve (nose has 2)
     6 x M3 hex nuts (DIN 934, 5.5 mm across flats, 2.4 mm thick)
   M3 self-tapping screws (same as v2):
     4 x M3x20 countersunk   front-side joints
     2 x M3x10 countersunk   strap to left side
     2 x M3x10 countersunk   crest
     4 x M3x10 countersunk   back cover
     12 x M3x12 pan head     5 rib tabs, 5 ribs into the hub, 2 temple caps
     1 x M3x10               hub set screw
     An M3 assortment box (screws + nuts) covers all of it.
   1 x M4x16 hex bolt + 1 x M4 nut (dial)
   EVA foam with adhesive:
     4-5 mm strips for the inside of the front + sides
     a 10-12 mm pad for the back strap
     a 5 mm disc (about 50 mm) under the crown hub
   2 small zip ties (2.5 mm), super glue (CA)
   Gel pads: the same snap ECG pads you use now.

4. BUILD
--------
   Picture guide: 4_renders/NeuroX_HALO_v3_assembly_1_frame.png + _2_finish.png
   (same steps as below, in the order to do them: close the crown hub AFTER its 5 screws).
   a. Crown hub top: the blue hub ring and the white cap (blue N) go on LAST, after the
      5 hub screws in step c. Press them in with a drop of glue.
   b. Band: slide each side piece's lower tab under the front band's upper tab at +/-66 deg,
      then 2 x M3x20 down through each joint.
   c. Ribs: push the 5 rib tabs into the band pockets (spine at the front centre, the 4 side
      ribs at +/-38 and +/-142 deg), one M3x12 sideways through each pocket.
      Then put the crown hub onto the 5 rib tops, 5 x M3x12 down through the hub. Ring + cap
      go on top and hide the screws.
   d. Strap: thread it through the back hub's tunnel first.
      - Screw its left end onto the left side (2 x M3x10).
      - Lay its right end over the right side's ridges.
      - M4 bolt through the dial + slot, nut in the hex pocket inside.
      - Turn the dial to loosen, slide to fit, tighten. Range about +/-35 mm.
      - Slide the back hub to the middle, M3 set screw on top.
   e. Lock knobs (NEW in v3), 6 of them:
      - Put a drop of super glue in the knob's hole and press an M3x8 socket head screw in,
        head first, thread sticking out of the knob's bottom. Let it set 10 minutes.
      - Screw the knob into the boss on the sleeve from the outside until the thread tip
        shows inside the sleeve (look into the sleeve's open end).
      - Hold the nut on the tip with a fingertip and turn the knob: the screw pulls the nut up
        into its hex pocket, then the pocket stops it turning. Back the knob off 2 turns.
        (No knob? An M3x8 screw and a 2.5 mm hex key work the same.)
   f. Pods + stems:
      - Put each lead's snap clip into a cup, wire through the cup slot.
      - Press the cup into its holder: the forehead holder on the band, or the ring of its
        slider. The holder's lead slot faces the cup slot.
      - Knob 2 turns out. Slide each slider's tongue into its sleeve, grooves towards your skin,
        (the ramp at the tip rides over the tooth). The nose slider's two tongues go into the
        two sleeves left and right of the forehead holder.
      - Start every stem on its long blue mark (the design length), finger-tighten the knob.
      - Leads: forehead, nose, left temple and left REF run along the LEFT side; right temple
        and right REF along the RIGHT side.
      - Leave a 2-3 cm loop of lead at every stem so it can still slide.
   g. Leads: lay them on the outside of the band/strap and snap the blue band clips over. On
      the upper arms use the arm clips (2 per arm).
      Temple leads go under the white temple caps (M3x12 pan head in the middle of the blue
      ring).
      Crest last (2 x M3x10 into the two stand-offs on the band): it covers the forehead
      holder and the leads behind it.
   h. Cables: both pill cables come up into the back hub from below. Zip-tie each one through
      its slot pair (strain relief), then the cover (4 x M3x10).
   i. Foam: strips inside the front + sides (not on the pods), thick pad inside the back strap,
      disc under the crown hub.

5. FIT THE STEMS
----------------
   Put the headset on and tighten the dial.
   Then, for each pod:
     1. turn its knob 2 turns out
     2. push or pull until the gel pad sits on its spot
          temples: at eye level, in the soft spot beside the eye
          REF:     on the bone behind the ear
          nose:    just above the nose bridge
     3. let go and wiggle it a little until it drops into the nearest groove
     4. finger-tighten the knob. Don't force it - finger-tight is plenty.
   Every groove is 2 mm. Read the blue mark at the sleeve edge and set the left and right
   stems to the same mark. Write your marks down, so next time it takes 10 seconds.

6. LEAD LENGTH
--------------
   Measured from where each cable splits into its 3 leads (the split is inside the back hub):
   forehead ~33 cm, nose ~37 cm, temples ~29 cm, REF ~14 cm, +3 cm for the stem loops.
   If your leads are shorter, let the split sit lower behind the head and run the cables up
   into the hub.

7. SIZES
--------
   Built around an average head, about 55 cm round at the band. The strap covers roughly
   52-58 cm, and the stems cover different face lengths.
   For a clearly smaller or bigger head, print ALL parts at 95 % or 105 % (then use M3
   nuts/screws as they are - check the lock on the fit check first).
   Everything was checked on a model head only. Try it on, then adjust.

8. SAFETY
---------
   Same rules as the box: battery only while the headset is on, never charging, never
   connected to the wheelchair battery.

SOURCE (5_source)
-----------------
   neurox_halo_print.scad   all parts (PART = ...), measurements at the top
   gen_print.py             pod positions + stem frames on the head -> gen_print.scad
   export.sh                export all STLs (every colour of a part separately)
   make_plates.py           build the A1 mini + AMS plates
   logo_geo.scad            the Reticle N emblem geometry (crest)
   orbitron900.ttf          the wordmark font: install it, or keep it next to the .scad
   render_asm.scad          assembly render from the exported parts (DL = stem extension)
   render_stem.scad         close-ups of the stem + lock
   stl2step.py              STL -> STEP solids + coloured assembly
