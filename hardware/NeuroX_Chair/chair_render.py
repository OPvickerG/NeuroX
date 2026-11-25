"""Renders the chair drive on the top of a 24" wheel (matplotlib, flat shading) -> img/"""
import math, os, numpy as np, trimesh, matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
import cadquery as cq
import chair_drive_cq as C
import chair_check as K
import chair_mount_cq as Mt

IMG = os.path.join(C.HERE, "img"); os.makedirs(IMG, exist_ok=True)


def mesh(w, tol=0.4):
    p = "/tmp/_r.stl"; cq.exporters.export(w, p, tolerance=tol, angularTolerance=0.3); return trimesh.load(p)


def tyre_arc():
    a0, a1 = math.radians(-35), math.radians(35)
    ws = cq.Workplane("YZ").center(0, C.R_T - C.TYRE_W / 2).circle(C.TYRE_W / 2)
    sol = cq.Solid.makeTorus(C.R_T - C.TYRE_W / 2, C.TYRE_W / 2, cq.Vector(0, 0, 0), cq.Vector(0, 1, 0))
    box = cq.Workplane("XY").box(2 * C.R_T * math.sin(a1) + 40, 60, 90, centered=(True, True, False)).translate((0, 0, C.R_T - 85))
    return cq.Workplane(obj=sol).intersect(box)


def rim_arc():
    sol = cq.Solid.makeTorus(C.RIM_R, C.RIM_D / 2, cq.Vector(0, C.RIM_Y, 0), cq.Vector(0, 1, 0))
    box = cq.Workplane("XY").box(360, 80, 80, centered=(True, True, False)).translate((0, C.RIM_Y, C.RIM_R - 70))
    return cq.Workplane(obj=sol).intersect(box)


PARTS = [(tyre_arc(), (0.16, 0.17, 0.19)), (rim_arc(), (0.10, 0.10, 0.12)), (K.motors(), (0.72, 0.74, 0.78)),
         (K.rollers(), (0.05, 0.05, 0.05)), (C.pod(), (0.13, 0.36, 0.85)), (C.hanger(), (0.85, 0.40, 0.10)), (K.bolts(), (0.55, 0.55, 0.58))]
_o, _i, _f = Mt.clamp_halves(25)
_wall = Mt.Y_WALL - Mt.WALL_T
PARTS += [(Mt.arm(), (0.20, 0.62, 0.35)), (Mt.place_clamp(_o, 25, -40, _wall, Mt.ZC).union(Mt.place_clamp(_i, 25, -40, _wall, Mt.ZC)), (0.55, 0.30, 0.75)),
          (C.cyl_z(-40, _wall - _f, 12.5, 330, 440), (0.82, 0.83, 0.85))]
MESHES = [(mesh(w), c) for w, c in PARTS]
L = np.array([0.3, 0.6, 0.75]); L /= np.linalg.norm(L)


def draw(ax, elev, azim):
    for m, col in MESHES:
        tri = m.vertices[m.faces]; n = m.face_normals
        s = 0.35 + 0.65 * np.clip(np.abs(n @ L), 0, 1)
        fc = np.clip(np.array(col)[None] * s[:, None] * 1.5, 0, 1)
        ax.add_collection3d(Poly3DCollection(tri, facecolors=fc, edgecolors=fc, linewidths=0.05))
    ax.set_xlim(-120, 120); ax.set_ylim(-110, 130); ax.set_zlim(270, 440); ax.set_box_aspect((240, 240, 170))
    ax.view_init(elev, azim); ax.set_axis_off()


fig = plt.figure(figsize=(16, 7), facecolor="white")
for i, (e, a, t) in enumerate(((22, -55, "from the outside, front"), (6, -90, "side view (wheel plane)"), (30, 215, "from behind, inside: clamp on the backrest tube"))):
    ax = fig.add_subplot(1, 3, i + 1, projection="3d"); draw(ax, e, a); ax.set_title(t, fontsize=13)
fig.suptitle("NeuroX chair drive: blue = motor pod, orange = spring bar, green = arm, purple = clamp on the backrest tube, grey = motors", fontsize=14)
plt.tight_layout(); plt.savefig(os.path.join(IMG, "chair_drive_assembly.png"), dpi=110); print("saved")
