NeuroX logo pack - "Reticle N" (sci-fi)
A stencil N inside a HUD targeting ring. The glowing diagonal is the signal: your blink/clench
cutting through the N. The broken ring with tick marks is the headset halo seen as a scanner
reticle, and the red dot is the "live" light - the system is reading you.

svg/   vector files, all text already converted to shapes. The *_neon SVGs carry a glow filter
       that browsers render; some editors drop it - use the PNGs if the glow matters.
png/   transparent PNGs with the neon glow baked in, several sizes, + favicon.ico
       *_neon            -> on black / midnight / dark photos / dark slides (main version)
       *_on_dark_bg      -> same, already on the #0B0E16 background (for places that need a filled image)
       *_on_light        -> on white / paper (flat, no glow: blue #1257D1 + midnight #1E222D)
       mono_black / mono_white -> one colour: laser engraving, stamp, single-filament print
       app_icon          -> profile picture, YouTube / Instagram / GitHub avatar
       favicon.ico + neurox_favicon_16/32/48px.png -> simplified bold version for tiny sizes
screen/neurox_boot_320x240.png + neurox_boot.h -> boot screen for the box's 2.8" TFT
       #include "neurox_boot.h"
       tft.setRotation(1); tft.setSwapBytes(true); tft.pushImage(0, 0, 320, 240, neurox_boot);

Colours  void #0B0E16 (background)   midnight #1E222D   neon cyan #3FB6FF -> blue #1257D1 (gradient)
         white #FFFFFF   red #FF2E3A only for the live dot (and STOP on the wheelchair UI)
Type     wordmark: Orbitron Black, +14 % letter spacing, X in the cyan->blue gradient
         labels/UI: Rajdhani SemiBold, wide tracking, uppercase
Rules    use the neon version on dark backgrounds, the flat version on light ones.
         Under 48 px use the favicon version (no tick marks). Keep clear space of a quarter of
         the ring around it. Don't stretch it, don't put the glow on a white background.
