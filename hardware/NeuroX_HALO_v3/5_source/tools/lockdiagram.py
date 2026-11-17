"""HALO v3: 'how it locks' diagram from real cuts through a sleeve (OpenSCAD projection SVGs).
usage: lockdiagram.py <svg dir> <stem name> <SB> <LS> <out.png>"""
import re, sys, matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import PathPatch
from matplotlib.path import Path
from matplotlib import font_manager as fm

D, NAME, SB, LS, OUT = sys.argv[1], sys.argv[2], float(sys.argv[3]), float(sys.argv[4]), sys.argv[5]
FONT = "/tmp/claude-0/-home-claude/31d76bfd-844e-5c8a-8c3a-6c0a47579976/scratchpad/box3/rajdhani700.ttf"
fm.fontManager.addfont(FONT); plt.rcParams["font.family"] = fm.FontProperties(fname=FONT).get_name()
COL = {"band": "#2B303D", "slider": "#6B7689", "knob": "#1257D1", "screw": "#C3C6CC", "nut": "#959AA3"}
INK, BLUE, GREY = "#161A22", "#1257D1", "#7A808C"

def load(f):
    s = open(f).read(); verts, codes = [], []
    for d in re.findall(r'd="([^"]+)"', s):
        for sub in re.findall(r"M[^M]+", d):
            pts = [tuple(map(float, p.split(","))) for p in re.findall(r"-?[\d.]+(?:e-?\d+)?,-?[\d.]+(?:e-?\d+)?", sub)]
            if len(pts) < 3: continue
            verts += pts + [pts[0]]; codes += [Path.MOVETO] + [Path.LINETO] * (len(pts) - 1) + [Path.CLOSEPOLY]
    return Path(verts, codes) if verts else None

LK = SB + 3.0                       # lock position (LOCK_DS)
fig, axs = plt.subplots(1, 2, figsize=(11.4, 3.3), dpi=200)
for ax, loose in zip(axs, (True, False)):
    L = "true" if loose else "false"
    for w in ("band", "slider", "nut", "screw", "knob"):
        p = load(f"{D}/sec_{NAME}_{L}_{w}.svg")
        if p is not None: ax.add_patch(PathPatch(p, fc=COL[w], ec="#0E1016", lw=0.35))
    ax.set_xlim(SB - 9, SB + LS + 1.5); ax.set_ylim(-3.2, 15.5); ax.set_aspect("equal"); ax.axis("off")
    ax.set_title("LOOSE  -  knob 2 turns out" if loose else "LOCKED  -  knob finger-tight", fontsize=15, color=BLUE if not loose else GREY, loc="left")
    kw = dict(fontsize=10.5, color=INK, arrowprops=dict(arrowstyle="-", color=BLUE, lw=1.1))
    if loose:
        ax.annotate("stem lifts 1 mm and\nslides over the tooth", xy=(LK + 6, 2.2), xytext=(LK + 7.5, 11.0), **kw)
        ax.annotate("grooves every 2 mm", xy=(LK - 7, 1.2), xytext=(SB - 8.5, -2.6), **kw)
    else:
        ax.annotate("screw presses the stem\ndown onto the tooth", xy=(LK, 3.1), xytext=(LK + 7.5, 12.2), **kw)
        ax.annotate("tooth in a groove = locked", xy=(LK, 0.4), xytext=(LK + 2.5, -2.6), **kw)
        ax.annotate("M3 nut", xy=(LK - 1.5, 5.2), xytext=(SB - 8.5, 9.0), **kw)
        ax.annotate("knob + M3x8", xy=(LK - 4.2, 11.5), xytext=(SB - 8.5, 13.2), **kw)
plt.subplots_adjust(left=0.01, right=0.99, top=0.88, bottom=0.02, wspace=0.06)
plt.savefig(OUT, facecolor="white"); print(OUT)
