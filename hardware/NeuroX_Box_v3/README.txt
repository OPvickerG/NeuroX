NeuroX BOX v3 "Reticle" - print + CAD files
=============================================

Same parts inside as box v2 (2.8" screen, 3 buttons + STOP, ESP32-S3, both BioAmp
pills, LiPo, TP4056 USB-C charger, MT3608 boost, rocker switch, buzzer) in the new
sci-fi shell with the Reticle N logo on top.
Outer size 163 x 101 x 50 mm (without the buttons). Fits the A1 mini bed (180 x 180).
All files work the same on Mac and Windows (Bambu Studio and Fusion 360 for macOS).


FOLDERS
-------
1_Bambu_Studio_3MF   open these in Bambu Studio - already flat on the plate, coloured
  plate1_fitcheck_and_small_parts.3mf   PRINT FIRST. Display fit check (only the screen part
                                        of the lid, 9.6 mm) + red STOP guard + blue sleeve grommet
                                        + 4 screen clamp tabs
  plate2_base.3mf                       base, upright (midnight + blue line + white NEUROX)
  plate3_lid.3mf                        lid, top face DOWN (all 4 colours)

2_STEP_Fusion360     real CAD solids (not triangles), each colour a named body
  NeuroX_Box_v3_assembly.step   everything assembled, with the buttons as reference parts
  NeuroX_v3_<part>.step         each part on its own
  Fusion 360: File > Open > Open from my computer > choose the .step (or drag it into
  the Data Panel / upload). It opens as components you can edit.

3_STL
  multicolour_parts/   one STL per part per colour (same positions, so they line up when
                       you load them together as one multi-part object)
  one_colour/          each part as a single STL, for printing without the AMS
  NOTE: STLs are in "assembled" orientation. Flip the lid and the fit check over (top face
  down) before printing. The 3MF files are already flipped.

4_renders            pictures of the final parts and the print plates
5_source             the CadQuery script that builds everything (python3 box_v3_cq.py),
                     with layout.py, the logo geometry and the 2 fonts (Orbitron, Rajdhani -
                     both free OFL fonts)


AMS SLOTS (A1 mini + AMS lite)
------------------------------
  slot 1 midnight gray    slot 2 white    slot 3 blue    slot 4 red
Open a 3MF in Bambu Studio. If it asks how to match the colours, pick the slot above for
each colour. If every part shows the same colour instead: in the object list on the left,
expand the object, click each part (its name ends with "(midnight gray)", "(white)", ...)
and set its filament number. Keep the prime tower on.


PRINT SETTINGS
--------------
PLA, 0.20 mm layers, 3 walls, 15-20 % infill, NO supports, Textured PEI plate (the lid top
gets a matte sci-fi texture from it). Brim not needed.
  - base: prints upright as placed. The front NEUROX letters and the blue line are colour
    inlays in the wall, so the AMS changes colour on every layer there (~90 changes for
    the base, adds time + purge). Want it faster? Set the base's white part to midnight
    in Bambu Studio: the letters disappear but nothing else changes.
  - lid: prints top face down. All colours are in the first 1.6 mm (~20 colour changes).
  - the fit check uses the real lid geometry: print it first, turn it over and drop the
    screen in, glass first. The board must sit in its pocket with no gap at the sides and
    only the lit picture shows in the window (no black glass edge).
    Measure and change the numbers at the top of 5_source/box_v3_cq.py if needed.


BUY (same as box v2 + a few changes)
------------------------------------
  2.8" SPI TFT 240x320 ILI9341 - 4 x 16 mm panel push buttons (R13-507 style): 3 black + 1 red
  KCD1 rocker switch (2 pins, 19.2 x 13 mm cutout) - TP4056 USB-C module - MT3608 boost
  LiPo 103450 (~2000 mAh) with protection board - perfboard 68 x 50 mm - female headers
  4 x M3 x 12 countersunk screws (lid -> corner bosses, self-tapping)
  8 x M2 x 6 self-tapping screws (perfboard 4, screen clamp tabs 4)
  8 mm braided cable sleeve for the 2 pill cables (box -> headset)
  4 rubber feet (10 mm), double-sided foam tape for the battery
  (wiring + pin map: unchanged from the box v2 README)


ASSEMBLY
--------
 1. Push the blue sleeve grommet into the round hole on the right side from outside
    until the barb clicks. Pull both pill cables through it (JST plugs go through).
 2. Base: perfboard (ESP32-S3 + both pills) on the 4 standoffs, battery in its rim,
    TP4056 on the shelf (USB-C in the left-wall slot), rocker into its cutout (snaps),
    buzzer into the floor pocket (it sounds through the holes in the bottom).
 3. Lid: lid upside down, screen glass-first into its pocket (pins towards the open end of
    the pocket), 4 clamp tabs with M2 screws press the board up. Don't overtighten.
    Buttons through the holes; put the red guard ring on the lid first, then the red
    STOP button through the ring, nut underneath.
 4. Close with 4 x M3 x 12 countersunk screws. Stick the rubber feet in the 4 recesses.


v3.1 - SCREEN POCKET FITTED TO YOUR SCREEN (tested)
---------------------------------------------------
  Window: unchanged - it hides the black glass edge, only the picture shows.
  Board pocket: 82.6 x 50.6 mm for your 82 x 50 board (was 86.6 for the standard board).
  Pocket walls: 5.6 mm deep below the lid (the tested 5 mm + 3 layers; lid + walls =
  9.6 mm), 1.6 mm thick. Pin end open for the wires.
  Clamp tabs: 4 notches in the walls so the tabs slide in under the board. The front-right
  post moved 10 mm right (clear of the micro-SD socket) and the tabs are shorter: they
  press only the outer 2.5 mm of the board. Reprint the 4 tabs too.
  Clamp depth 4.6 mm (glass 3 mm above the board + 1.6 mm board).
  To change: BRD_L / BRD_W / POCKET_WALL_H / POCKET_WALL_T / GLASS_H / PCB_T at the top of
  5_source/box_v3_cq.py.

PRINTED BUTTONS (for 12 x 12 x 7.3 mm tactile switches) - see 4_renders/NeuroX_Box_v3_buttons_guide.png
-------------------------------------------------------------------------------------------------
  Instead of 16 mm panel buttons, each of the 4 lid holes gets:
    GUIDE   pressed up into the hole from below + a drop of super glue (4 small ribs grip it)
    CAP     dropped into the guide from below, trapped under its lip (stands 1.8 mm above the lid)
    CRADLE  holds the switch (button up, pins through the slots, bend them flat underneath,
            solder 2 wires to DIAGONAL pins), 2 x M2x10 up into the guide's flange
    STOP    red guard glued on top of the lid, red STOP cap from below, red STOP top glued on
            (0.4 mm under the guard rim)
  Plates (1 colour each, no AMS swaps, PLA 0.2 mm, no supports, no brim):
    plate4_buttons_gray   4 guides, 4 cradles, 3 caps, 4 screen clamp tabs
    plate5_stop_red       STOP guard, STOP cap, STOP top
    plate6_grommet_blue   cable sleeve grommet
  Buy: 4 x tactile switch 12x12x7.3, 8 x M2x10 self-tapping, super glue, thin wire.
  Different switch height? Change SW_H (bottom of the switch to the top of its button) at the
  top of 5_source/box_buttons_cq.py and run it + make_3mf_buttons.py.
  Checked: no part touches the lid (except the ribs' 0.17 mm grip), the base, the screen
  clamp tabs or the STOP guard; the modules end 33 mm above the floor.

WHAT CHANGED FROM v2 (so the old v2 lid will NOT fit this base)
------------------------------------------------------------------
  - new shell: chamfered corners, 45 deg lid bevel, bottom chamfer with blue line,
    2 panel grooves, slanted back vents, NEUROX on the front
  - lid: white top with the Reticle N logo, blue frame + HUD brackets, button rings,
    black legends, red STOP text - all flush colour inlays
  - lid screws moved inwards (off the bevel), one screen clamp post moved 3 mm right
  - the 6-lead comb + cable channel are gone: one blue grommet for the cable sleeve
  - armrest mount: not included yet (as agreed - later)

Checked before export: every solid valid, every STL watertight, no overlap between colours,
lid and base do not collide, no printed part hits any electronic part (layout boxes incl.
clearance), all parts fit the A1 mini plate.
