import sys; sys.argv = ['x']
exec(open('/tmp/claude-0/-home-claude/31d76bfd-844e-5c8a-8c3a-6c0a47579976/scratchpad/halo2/tools/concept.py').read().split("# header")[0])
import os; os.chdir('/tmp/claude-0/-home-claude/31d76bfd-844e-5c8a-8c3a-6c0a47579976/scratchpad/halo2')
W2, H2 = 2800, 1300
sh = Image.new("RGB", (W2, H2), BG); dd = ImageDraw.Draw(sh)
dd.text((40, 30), "NeuroX HALO v3 - AMS print files (Bambu A1 mini + AMS lite)", font=ORB(46), fill=INK)
lx = 1960
for name, c in (("1 midnight gray", MGc), ("2 white", WHc), ("3 blue", BLc), ("4 red", RDc)):
    dd.rounded_rectangle([lx, 40, lx + 34, 74], 7, fill=c, outline=(190, 194, 202)); dd.text((lx + 44, 57), name, font=RAJ(26), fill=INK, anchor="lm"); lx += 44 + int(dd.textlength(name, font=RAJ(26))) + 28
views = [("hero", "Front"), ("front", "Face view: crest + nose twin stem"), ("side", "Side: temple + REF stems"), ("back", "Back: hub + side dial")]
for i, (n, t) in enumerate(views):
    im = crop(f"{R}/{n}.png"); im.thumbnail((660, 560)); sh.paste(im, (40 + i * 690 + (660 - im.width) // 2, 110 + (560 - im.height) // 2))
    dd.text((40 + i * 690, 690), t, font=RAJ(30), fill=INK)
plates = [("1_fitcheck", "1 Fit check + lock (first)"), ("2_front", "2 Front band (NEW lock)"), ("3_sides", "3 Sides (NEW lock)"), ("4_strap", "4 Rear strap"),
          ("5_ribs_hub", "5 Ribs + hub"), ("6_white_parts", "6 White parts + crest"), ("7_blue_clips", "7 Clips + 6 knobs (NEW)"), ("8_sliders", "8 Sliders (NEW grooves)")]
for i, (k, t) in enumerate(plates):
    im = Image.open(f"img/amsplate{k}.png").convert("RGB"); im.thumbnail((330, 300)); x = 40 + i * 345
    sh.paste(im, (x, 760)); dd.text((x, 1080), t, font=RAJ(28), fill=BLc if "NEW" in t else INK)
sh.save("r/NeuroX_HALO_v3_print_overview.png"); print("ok")
