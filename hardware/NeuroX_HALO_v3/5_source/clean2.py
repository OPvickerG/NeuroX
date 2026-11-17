import trimesh, glob, numpy as np
def clean(f):
    m=trimesh.load(f); parts=m.split(only_watertight=False)
    keep=[p for p in parts if abs(p.volume)>0.5]
    if len(keep)<len(parts):
        out=trimesh.util.concatenate(keep); out.export(f); return len(parts)-len(keep)
    return 0
for d in ('stl','stl_model'):
    for f in sorted(glob.glob(f'{d}/*.stl')):
        n=clean(f)
        m=trimesh.load(f)
        if n or not m.is_watertight: print(f, 'dropped', n, 'watertight', m.is_watertight)
