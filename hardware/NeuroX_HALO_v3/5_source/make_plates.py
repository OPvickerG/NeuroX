"""NeuroX HALO -> Bambu Lab A1 mini + AMS lite plates (180 x 180).
Standard 3MF with colours (materials extension): Bambu Studio reads the colours and shows its
colour -> filament mapping when the file is opened. Multi-colour parts are ONE object made of
coloured parts that print in place."""
import io, os, zipfile, datetime, math
import numpy as np, trimesh
from shapely import affinity
from shapely.geometry import MultiPoint
from shapely.ops import unary_union
HERE = os.path.dirname(os.path.abspath(__file__))
BED, MARGIN, GAP = 180.0, 3.0, 4.0
OUT = os.path.join(HERE, "bambu_A1mini_AMS"); os.makedirs(OUT, exist_ok=True)
COLOURS = [("midnight gray", "#2B303DFF"), ("white", "#F0F2F5FF"), ("blue", "#1257D1FF"), ("red", "#D61A1FFF")]
CI = {"MG": 0, "WH": 1, "BL": 2, "RD": 3}

def mesh(n):
    m = trimesh.load(os.path.join(HERE, "stl", n + ".stl")); m.merge_vertices(); return m
def obj(name, part, cols, single=None):
    """an object = list of (colour, mesh) that keep their relative positions"""
    if single: return (name, [(single, mesh(part))])
    return (name, [(c, mesh(f"{part}__{c}")) for c in cols])
def footprint(ms):
    polys = []
    for _, m in ms:
        try: polys.append(trimesh.path.polygons.projected(m, normal=[0, 0, 1], ignore_sign=True).buffer(0))
        except Exception: polys.append(MultiPoint(m.vertices[:, :2]).convex_hull)
    return unary_union(polys)

def pack(items, title):
    placed, shapes = [], []
    items = sorted(items, key=lambda it: -footprint(it[1]).area)
    for name, ms in items:
        fp0 = footprint(ms); best = None
        rots = range(0, 360, 15) if len(items) <= 2 else (0, 90, 180, 270)
        for rot in rots:
            fp = affinity.rotate(fp0, rot, origin=(0, 0))
            minx, miny, maxx, maxy = fp.bounds
            for y in np.arange(MARGIN - miny, BED - MARGIN - maxy + 0.01, 2.0):
                for x in np.arange(MARGIN - minx, BED - MARGIN - maxx + 0.01, 2.0):
                    cand = affinity.translate(fp, x, y)
                    if any(cand.distance(s) < GAP for s in shapes): continue
                    key = (y + maxy, x + maxx)
                    if best is None or key < best[0]: best = (key, rot, x, y, cand)
                    break
                if best is not None and best[0][0] <= y + maxy: break
        assert best is not None, f"{name} does not fit on '{title}'"
        _, rot, x, y, cand = best
        zmin = min(m.bounds[0][2] for _, m in ms)
        T = trimesh.transformations.rotation_matrix(math.radians(rot), [0, 0, 1]); T[:3, 3] = [x, y, -zmin]
        out = []
        for c, m in ms:
            mm = m.copy(); mm.apply_transform(T); out.append((c, mm))
        placed.append((name, out)); shapes.append(cand)
    return placed

def model_xml(objects, title):
    now = datetime.date.today().isoformat(); o = io.StringIO()
    o.write('<?xml version="1.0" encoding="UTF-8"?>\n<model unit="millimeter" xml:lang="en-US" '
            'xmlns="http://schemas.microsoft.com/3dmanufacturing/core/2015/02" '
            'xmlns:m="http://schemas.microsoft.com/3dmanufacturing/material/2015/02">\n')
    for k, v in (("Title", title), ("Designer", "NeuroX"), ("Description", "NeuroX HALO v3 headset (extendable stems with screw lock) - Bambu Lab A1 mini + AMS lite, PLA 0.20 mm, no supports"),
                 ("CreationDate", now), ("ModificationDate", now)):
        o.write(f' <metadata name="{k}">{v}</metadata>\n')
    o.write(' <resources>\n  <m:colorgroup id="1">\n')
    for _, hexc in COLOURS: o.write(f'   <m:color color="{hexc}"/>\n')
    o.write('  </m:colorgroup>\n')
    nid = 2; build = []
    for name, parts in objects:
        ids = []
        for c, m in parts:
            o.write(f'  <object id="{nid}" name="{name} ({COLOURS[CI[c]][0]})" type="model" pid="1" pindex="{CI[c]}">\n   <mesh>\n    <vertices>\n')
            for x, y, z in m.vertices: o.write(f'     <vertex x="{x:.4f}" y="{y:.4f}" z="{z:.4f}"/>\n')
            o.write('    </vertices>\n    <triangles>\n')
            for a, b, cc in m.faces: o.write(f'     <triangle v1="{a}" v2="{b}" v3="{cc}"/>\n')
            o.write('    </triangles>\n   </mesh>\n  </object>\n')
            ids.append(nid); nid += 1
        if len(ids) == 1: build.append(ids[0])
        else:
            o.write(f'  <object id="{nid}" name="{name}" type="model">\n   <components>\n')
            for i in ids: o.write(f'    <component objectid="{i}"/>\n')
            o.write('   </components>\n  </object>\n'); build.append(nid); nid += 1
    o.write(' </resources>\n <build>\n')
    for b in build: o.write(f'  <item objectid="{b}"/>\n')
    o.write(' </build>\n</model>\n'); return o.getvalue()
CT = ('<?xml version="1.0" encoding="UTF-8"?>\n<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
      '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
      '<Default Extension="model" ContentType="application/vnd.ms-package.3dmanufacturing-3dmodel+xml"/>'
      '<Default Extension="png" ContentType="image/png"/></Types>\n')
def rels(thumb):
    s = ('<?xml version="1.0" encoding="UTF-8"?>\n<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
         '<Relationship Target="/3D/3dmodel.model" Id="rel0" Type="http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel"/>')
    if thumb: s += '<Relationship Target="/Metadata/thumbnail.png" Id="rel1" Type="http://schemas.openxmlformats.org/package/2006/relationships/metadata/thumbnail"/>'
    return s + '</Relationships>\n'
def write_3mf(path, objects, title, thumb=None):
    for name, parts in objects:
        lo = np.min([m.bounds[0] for _, m in parts], axis=0); hi = np.max([m.bounds[1] for _, m in parts], axis=0)
        assert lo[0] >= 0.5 and lo[1] >= 0.5 and hi[0] <= BED - 0.5 and hi[1] <= BED - 0.5, (name, lo, hi)
        assert abs(lo[2]) < 1e-6, (name, "not on the plate")
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("[Content_Types].xml", CT); z.writestr("_rels/.rels", rels(thumb is not None))
        z.writestr("3D/3dmodel.model", model_xml(objects, title))
        if thumb: z.write(thumb, "Metadata/thumbnail.png")

BAND = ["MG", "WH", "BL"]; WB = ["WH", "BL"]
def many(name, part, k, cols=None, single=None): return [obj(f"{name}_{i + 1}", part, cols, single) for i in range(k)]
PLATES = [
    ("1_fitcheck", "PRINT FIRST - fit check", [obj("fitcheck", "fitcheck", BAND), obj("cup_test", "cup", None, "MG"), obj("band_clip_test", "band_clip", None, "MG"),
                                               obj("stem_test", "stem_test", ["MG", "BL"]), obj("knob_test", "knob", None, "BL")]),
    ("2_front", "front band", [obj("front_band", "front", BAND)]),
    ("3_sides", "left + right side", [obj("side_L", "side_L", BAND), obj("side_R", "side_R", BAND)]),
    ("4_strap", "rear strap", [obj("rear_strap", "strap", BAND)]),
    ("5_ribs_hub", "5 ribs + crown hub (midnight gray)", [obj("rib_spine", "rib_spine", None, "MG"), obj("crown_hub", "hub", None, "MG")] + many("rib_side", "rib_side", 4, None, "MG")),
    ("6_white_parts", "white parts with blue details", many("cup", "cup", 6, WB) + [obj("crest", "crest", ["WH", "BL", "MG", "RD"]), obj("back_cover", "back_cover", WB), obj("back_hub", "back_hub", None, "WH"),
                                                         obj("dial", "dial", WB), obj("hub_cap", "hub_cap", WB)] + many("temple_cap", "temple_cap", 2, WB)),
    ("8_sliders", "5 extendable stems (sliders) - midnight gray + blue length marks",
     [obj(f"slider_{n}", f"slider_{n}", ["MG", "BL"]) for n in ("temple_L", "temple_R", "ref_L", "ref_R", "nose")]),
    ("7_blue_clips", "blue clips + hub ring + 6 lock knobs", [obj("hub_ring", "hub_ring", None, "BL")] + many("band_clip", "band_clip", 14, None, "BL") + many("arm_clip", "arm_clip", 10, None, "BL") + many("knob", "knob", 6, None, "BL")),
]
if __name__ == "__main__":
    import json
    summary = []
    for key, title, items in PLATES:
        placed = pack(items, title)
        path = os.path.join(OUT, f"NeuroX_HALO_v3_plate{key}.3mf")
        thumb = os.path.join(HERE, "img", f"amsplate{key}.png")
        write_3mf(path, placed, "NeuroX HALO v3 - " + title, thumb if os.path.exists(thumb) else None)
        # per-colour preview meshes of the plate
        for c in ("MG", "WH", "BL", "RD"):
            ms = [m for _, parts in placed for cc, m in parts if cc == c]
            f = os.path.join(HERE, "img", f"amsplate{key}_{c}.stl")
            if ms: trimesh.util.concatenate(ms).export(f)
            elif os.path.exists(f): os.remove(f)
        vol = {c: sum(abs(m.volume) for _, parts in placed for cc, m in parts if cc == c) / 1000 for c in ("MG", "WH", "BL", "RD")}
        print(f"{os.path.basename(path):46s} {len(placed):2d} objects  {os.path.getsize(path) // 1024:5d} KB  solid cm3 " + " ".join(f"{c}:{v:.1f}" for c, v in vol.items() if v))
