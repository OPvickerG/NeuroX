import sys, everyone as E
exe, seed = sys.argv[1], int(sys.argv[2]); mode = sys.argv[3] if len(sys.argv)>3 else 'auto'
p = E.person(seed); prefix=[]; t0=8.0
if mode=='auto':
    prefix, tcal = E.calibrate_auto(exe, p, seed, None)
    out,_ = E.run(exe, prefix+[f"Kh@{tcal:.1f}"], p, seed)
    p['_biteUnlock'] = any('[AUTO] done' in l and 'BITE AND HOLD' in l for l in out)
    t0 = [E.ts(l) for l in out if '[AUTO] done' in l][0] + 3
cap = {}
orig = E.run
def run(exe_, sched, p_, seed_, nv_in=None, nv_out=None):
    r = orig(exe_, sched, p_, seed_, nv_in, nv_out); cap['s']=list(sched); cap['out']=r[0]; cap['pk']=r[1]; return r
E.run = run
res = E.drive(exe, p, seed, None, t0, prefix)
print(res['ok'], res['n'], dict(res['wrong'])); print(res['plan'])
sched = cap['s']; out = cap['out']
evs = sorted([x for x in sched if not x.startswith('K')], key=lambda x: float(x.split('@')[1]))
mv = E.moves(cap['pk'], t0)
# replay plan
import random
show = [l for l in out if any(k in l for k in ('[GEST','[SEQ','[EYE]','[TALK]','LOCK] LOCKED','READY','UNLOCKED')) and E.ts(l) > t0]
lo = float(sys.argv[4]) if len(sys.argv)>4 else t0; hi = float(sys.argv[5]) if len(sys.argv)>5 else t0+40
for l in show:
    if lo <= E.ts(l) <= hi: print(l[:175])
print([e for e in evs if lo <= float(e.split('@')[1]) <= hi])
print([m for m in mv if lo <= m[0] <= hi])
