"""Attribute false unlocks / locked moves / wrong driving moves to causes.
usage: python3 why.py <exe> <mode> <json from everyone.py> [seed ...]"""
import sys, json, re, random
from collections import Counter
import everyone as E

def everyday_events(p, seed, t0, minutes=4):
    r = random.Random(seed + 999); ev = []; t = t0; t1 = t0 + minutes * 60
    while t < t1:
        k = r.choice(['B', 'b', 'BB', 'E', 'h', 'T', 'S', 'W', 'q', 'C', 'C', 'Qs', 'Y', 'M'])
        d = 0.4
        if k == 'B': ev.append(f"B@{t:.2f}")
        if k == 'b': ev.append(f"b@{t:.2f}")
        if k == 'BB': ev += [f"B@{t:.2f}", f"B@{t + 0.4:.2f}"]
        if k == 'E': ev.append(f"E{r.choice((-300, -150, 150, 300))}@{t:.2f}"); d = 1.3
        if k == 'h': ev.append(f"h{r.choice((-400, -200, 200, 400))}@{t:.2f}"); d = 1.3
        if k == 'T': d = r.uniform(1, 3); ev.append(f"T{d:.1f}@{t:.2f}")
        if k == 'S': d = r.uniform(2, 6); ev.append(f"S{d:.1f}@{t:.2f}")
        if k == 'W': d = r.uniform(3, 8); ev.append(f"W{d:.1f}@{t:.2f}")
        if k == 'q': d = r.uniform(0.3, 2.5); ev.append(f"q{d:.1f}@{t:.2f}")
        if k == 'C': ev.append(f"C@{t:.2f}")
        if k == 'Qs': d = r.uniform(0.3, 1.5); ev.append(f"Q{d:.2f}@{t:.2f}")
        if k == 'Y': d = r.uniform(3, 6); ev.append(f"Y{d:.1f}@{t:.2f}")
        if k == 'M': d = r.uniform(1, 3); ev.append(f"M{d:.1f}@{t:.2f}")
        t += d + r.uniform(1.0, 4.0)
    ev += E.background(p, r, t0, t1)
    return ev, t1

def kind(e):
    k = e.split('@')[0]; k = re.sub(r'[-\d.:]', '', k); return k

def life_why(exe, seed, prefix=(), t0=8.0, nv=None):
    p = E.person(seed)
    ev, t1 = everyday_events(p, seed, t0)
    out, pk = E.run(exe, list(prefix) + ev + [f"Kh@{t1 + 2:.1f}"], p, seed, nv_in=nv)
    evs = sorted(ev, key=lambda x: float(x.split('@')[1]))
    c = Counter()
    for l in out:
        if 'UNLOCKED' in l and E.ts(l) > t0:
            tu = E.ts(l)
            # the event active / started in the 4.5 s before the unlock (longest-lasting wins)
            near = [e for e in evs if tu - 4.5 <= float(e.split('@')[1]) <= tu]
            dur = lambda e: float(re.sub(r'^[A-Za-z]', '', e.split('@')[0]) or 0) if kind(e) in 'TSWqQYMX' and re.sub(r'^[A-Za-z]', '', e.split('@')[0]) else 0
            c[max((kind(e) for e in near), key=lambda k: {'Y': 9, 'W': 8, 'S': 7, 'q': 6, 'M': 5, 'Q': 4, 'X': 3, 'T': 2}.get(k, 0), default='?')] += 1
    return c

if __name__ == "__main__":
    exe, mode, js = sys.argv[1], sys.argv[2], sys.argv[3]
    RES = json.load(open(js))
    tot = Counter(); bygroup = {}
    for x in RES:
        if x['life']['unlocks'] == 0: continue
        c = life_why(exe, x['seed'])
        tot += c; bygroup.setdefault(x['group'], Counter()).update(c)
    print("false unlock causes:", dict(tot.most_common()))
    for g, c in bygroup.items(): print(f"  {g:34s} {dict(c.most_common())}")
    w = Counter(); wg = {}
    for x in RES:
        for k, v in x['drive']['wrong'].items(): w[k] += v; wg.setdefault(x['group'], Counter())[k] += v
    print("driving errors:", dict(w.most_common()))
    for g, c in wg.items(): print(f"  {g:34s} {dict(c.most_common(6))}")
