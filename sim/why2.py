"""false unlocks in the everyday test: cause + lock numbers. usage: why2.py <exe> <json> [maxpeople]"""
import sys, json, re
from collections import Counter
import everyone as E, why as W
exe, js = sys.argv[1], sys.argv[2]; mx = int(sys.argv[3]) if len(sys.argv) > 3 else 999
R = json.load(open(js)); tot = Counter(); n = 0
for x in R:
    if x['life']['unlocks'] == 0: continue
    n += 1
    if n > mx: break
    seed = x['seed']; p = E.person(seed)
    prefix, tcal = E.calibrate_auto(exe, p, seed, None)
    out, _ = E.run(exe, prefix + [f"Kh@{tcal:.1f}"], p, seed)
    t0 = [E.ts(l) for l in out if '[AUTO] done' in l][0] + 3
    info = [l.split('| ', 1)[1][:110] for l in out if 'unlock:' in l or 'BITE AND HOLD' in l]
    ev, t1 = W.everyday_events(p, seed, t0)
    out2, pk = E.run(exe, prefix + ev + [f"Kh@{t1 + 2:.1f}"], p, seed)
    evs = sorted(ev, key=lambda e: float(e.split('@')[1]))
    print(f"--- seed {seed} {x['group']}"); [print('    ', i) for i in info]
    for i, l in enumerate(out2):
        if 'UNLOCKED' in l and E.ts(l) > t0:
            tu = E.ts(l)
            near = [e for e in evs if tu - 5 <= float(e.split('@')[1]) <= tu]
            hold = [m.split('| ', 1)[1][:100] for m in out2[max(0, i - 12):i] if 'holding 0.0' in m or 'waiting' in m]
            print(f"  UNLOCK {tu:.2f} near {near} | {hold[-1:] }")
            k = max((W.kind(e) for e in near), key=lambda k: {'Y': 9, 'W': 8, 'S': 7, 'q': 6, 'M': 5, 'Q': 4, 'X': 3, 'T': 2}.get(k, 0), default='?')
            tot[k] += 1
print(dict(tot))
