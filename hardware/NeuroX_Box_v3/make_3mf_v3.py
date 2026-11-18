"""NeuroX box v3 -> Bambu Lab A1 mini + AMS lite plates (180 x 180 bed).
Standard 3MF with colours (materials extension): Bambu Studio reads the colours and asks
which AMS slot each colour uses. Multi-colour parts are ONE object made of coloured parts."""
import io, os, zipfile, datetime, math
import numpy as np, trimesh
HERE = os.path.dirname(os.path.abspath(__file__))
STL = os.path.join(HERE, "build", "stl")
OUT = os.path.join(HERE, "build", "bambu_A1mini_AMS"); os.makedirs(OUT, exist_ok=True)
BED = 180.0
COLOURS = [("midnight gray", "#2B303DFF"), ("white", "#F0F2F5FF"), ("blue", "#1257D1FF"), ("red", "#D61A1FFF")]
CI = {"MG": 0, "WH": 1, "BL": 2, "RD": 3}

def load(name, flip=False):
    m = trimesh.load(os.path.join(STL, name + ".stl")); m.merge_vertices()
    if flip: m.apply_transform(trimesh.transformations.rotation_matrix(math.pi, [1, 0, 0]))
    return m

def obj(name, part, cols, flip=False):
    return (name, [(c, load(f"{part}__{c}", flip)) for c in cols])

def place(item, cx, cy, rot=0):
    name, parts = item
    lo = np.min([m.bounds[0] for _, m in parts], axis=0); hi = np.max([m.bounds[1] for _, m in parts], axis=0)
    T = trimesh.transformations.rotation_matrix(math.radians(rot), [0, 0, 1], point=[(lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, 0])
    out = []
    for c, m in parts:
        mm = m.copy(); mm.apply_transform(T); out.append((c, mm))
    lo = np.min([m.bounds[0] for _, m in out], axis=0); hi = np.max([m.bounds[1] for _, m in out], axis=0)
    for _, m in out: m.apply_translation([cx - (lo[0] + hi[0]) / 2, cy - (lo[1] + hi[1]) / 2, -lo[2]])
    return (name, out)

def model_xml(objects, title):
    now = datetime.date.today().isoformat(); o = io.StringIO()
    o.write('<?xml version="1.0" encoding="UTF-8"?>\n<model unit="millimeter" xml:lang="en-US" '
            'xmlns="http://schemas.microsoft.com/3dmanufacturing/core/2015/02" '
            'xmlns:m="http://schemas.microsoft.com/3dmanufacturing/material/2015/02">\n')
    for k, v in (("Title", title), ("Designer", "NeuroX"), ("Description", "NeuroX box v3 Reticle - Bambu Lab A1 mini + AMS lite, PLA 0.20 mm, no supports"),
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
        for _, m in parts: assert m.is_watertight, name
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("[Content_Types].xml", CT); z.writestr("_rels/.rels", rels(thumb is not None))
        z.writestr("3D/3dmodel.model", model_xml(objects, title))
        if thumb: z.write(thumb, "Metadata/thumbnail.png")

if __name__ == "__main__":
    img = os.path.join(HERE, "img")
    p1 = [place(obj("display_fitcheck", "fitcheck", ["MG"], flip=True), 56, 60),
          place(obj("stop_guard", "stop_guard", ["RD"]), 128, 40),
          place(obj("sleeve_grommet", "sleeve_grommet", ["BL"]), 160, 40)]
    for i, (x, y) in enumerate([(118, 75), (130, 75), (142, 75), (154, 75)]):
        p1.append(place(obj(f"clamp_tab_{i + 1}", "clamp_tab", ["MG"]), x, y))
    p2 = [place(obj("NeuroX_v3_base", "base", ["MG", "BL", "WH"]), 90, 90)]
    p3 = [place(obj("NeuroX_v3_lid", "lid", ["MG", "WH", "BL", "RD"], flip=True), 90, 90)]
    PL = [("1_fitcheck_and_small_parts", "PRINT FIRST - fit check + STOP guard + grommet + clamp tabs", p1, "p_expl.png"),
          ("2_base", "base (midnight + blue line + white NEUROX)", p2, "p_hero.png"),
          ("3_lid", "lid, top face down (4 colours)", p3, "p_top.png")]
    for key, title, objs, th in PL:
        path = os.path.join(OUT, f"NeuroX_Box_v3_plate{key}.3mf")
        th = os.path.join(img, th); write_3mf(path, objs, "NeuroX box v3 - " + title, th if os.path.exists(th) else None)
        vol = {c: sum(abs(m.volume) for _, parts in objs for cc, m in parts if cc == c) / 1000 for c in CI}
        print(f"{os.path.basename(path):52s} {len(objs)} objects {os.path.getsize(path) // 1024:6d} KB  cm3 " + " ".join(f"{c}:{v:.2f}" for c, v in vol.items() if v))
        # per-colour preview of the plate
        for c in CI:
            ms = [m for _, parts in objs for cc, m in parts if cc == c]
            if ms and os.path.isdir(img): trimesh.util.concatenate(ms).export(os.path.join(img, f"plate{key[0]}_{c}.stl"))
