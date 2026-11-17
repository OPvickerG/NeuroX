"""NeuroX 'everyone' test: 100 virtual people across disability groups, real firmware, closed-loop.
usage: python3 everyone.py <exe> <mode: factory|train|auto> <seed0> <N> [out.json]
"""
import os, sys, re, json, math, random, subprocess, uuid
from collections import Counter, defaultdict
from concurrent.futures import ThreadPoolExecutor

YOU = dict(VN=20, HN=45, BA=320, BR=0.15, BE=110, BW=0.025, BH=0.3, CE=290, CV=0.3, CDC=1.1, CD=0.7,
           SQLOBE=320, SQDC=100, SQE=300, FQE=70, CHE=250, TKE=250, JIT=0.2)
EXP = {'B': 'R', 'BB': '<', 'C': 'F', 'CC': '>'}
GROUPS = [("typical", 24), ("weak blink / ptosis / myasthenia", 8), ("facial palsy (stroke, Bell's)", 8),
          ("weak jaw / TMJ / dental", 8), ("tremor / Parkinson's", 8), ("spasms / cerebral palsy", 8),
          ("frequent involuntary blinks", 8), ("slow movers / fatigue / elderly", 8), ("noisy electrodes", 8),
          ("nystagmus", 6), ("very strong signals", 6)]

def group_of(i):
    k = i % 100
    for g, n in GROUPS:
        if k < n: return g
        k -= n

def person_you(seed):
    """the 50-people 'like YOU' crowd of the Pro-Safe test (+-20 % around your face)"""
    r = random.Random(seed * 7919 + 101)
    p = {}
    for k, v in YOU.items():
        if k == 'JIT': p[k] = v
        elif k == 'CD': p[k] = r.uniform(0.45, 0.95)
        else: p[k] = v * r.uniform(0.8, 1.2)
    p['_react'] = r.uniform(0.25, 0.6); p['_ready'] = r.uniform(0.4, 0.9)
    p['_pause'] = (r.uniform(0.6, 1.0), r.uniform(1.4, 2.2)); p['_sqz'] = r.uniform(3.3, 4.0)
    p['_dgap'] = r.uniform(0.3, 0.5); p['_cgap'] = r.uniform(0.3, 0.6)
    p['_blinkrate'] = r.uniform(8, 22); p['_inv'] = []; p['_group'] = 'like you (+-20 %)'
    return p

def person(seed):
    if os.environ.get('POP') == 'you': return person_you(seed)
    r = random.Random(seed * 104729 + 7)
    lu = lambda a, b: math.exp(r.uniform(math.log(a), math.log(b)))
    p = {k: (v * lu(0.7, 1.4) if k not in ('JIT', 'CD', 'BR', 'BW', 'CV') else v) for k, v in YOU.items()}
    p['BR'] = r.uniform(0.08, 0.5); p['BW'] = r.uniform(0.018, 0.04); p['CV'] = r.uniform(0.15, 0.45)
    p['CD'] = r.uniform(0.45, 0.95); p['BT'] = r.uniform(0.9, 1.15)
    p['_react'] = r.uniform(0.25, 0.6); p['_ready'] = r.uniform(0.4, 0.9); p['_pause'] = (r.uniform(0.6, 1.0), r.uniform(1.4, 2.2))
    p['_sqz'] = r.uniform(3.3, 4.0); p['_dgap'] = r.uniform(0.3, 0.55); p['_cgap'] = r.uniform(0.3, 0.6)
    p['_blinkrate'] = r.uniform(8, 22)     # natural blinks per minute
    p['_inv'] = []                          # involuntary event kinds during life / driving
    g = group_of(seed); p['_group'] = g
    if g.startswith("weak blink"):
        p['BA'] *= r.uniform(0.35, 0.6); p['BE'] *= r.uniform(0.4, 0.7); p['BT'] = r.uniform(1.2, 1.6); p['SQE'] *= r.uniform(0.5, 0.8); p['SQLOBE'] *= 0.6
    elif g.startswith("facial palsy"):
        p['CE'] *= r.uniform(0.4, 0.6); p['BA'] *= r.uniform(0.5, 0.8); p['SQE'] *= r.uniform(0.3, 0.6); p['SQLOBE'] *= 0.5; p['BE'] *= 0.7
    elif g.startswith("weak jaw"):
        p['CE'] *= r.uniform(0.35, 0.55); p['CD'] = r.uniform(0.4, 0.65); p['CDC'] *= r.uniform(0.4, 0.7)
    elif g.startswith("tremor"):
        p['TRA'] = r.uniform(20, 60); p['TRF'] = r.uniform(4, 6); p['TRE'] = r.uniform(15, 40); p['BA'] *= r.uniform(0.7, 1.0)
        p['BT'] = r.uniform(1.1, 1.4); p['_blinkrate'] = r.uniform(4, 10)
    elif g.startswith("spasms"):
        p['SPE'] = r.uniform(200, 450); p['SPD'] = r.uniform(40, 150); p['MA'] = r.uniform(100, 250)
        p['CE'] *= r.uniform(0.8, 1.5); p['CD'] = r.uniform(0.6, 1.2); p['JIT'] = 0.35; p['_inv'] = ['X', 'M']
        p['_react'] = r.uniform(0.4, 1.0)
    elif g.startswith("frequent"):
        p['_blinkrate'] = r.uniform(30, 60); p['_inv'] = ['B']; p['FQE'] *= 1.5
    elif g.startswith("slow"):
        p['BT'] = r.uniform(1.4, 2.0); p['CD'] = r.uniform(0.9, 1.5); p['_react'] = r.uniform(0.6, 1.5); p['JIT'] = 0.3
        p['_pause'] = (r.uniform(1.0, 1.4), r.uniform(1.8, 2.4)); p['_sqz'] = r.uniform(3.6, 4.6)
    elif g.startswith("noisy"):
        p['VN'] *= r.uniform(1.5, 2.5); p['HN'] *= r.uniform(1.5, 2.5); p['MAINS'] = r.uniform(5, 25)
        p['VDR'] = r.uniform(20, 80); p['HDR'] = r.uniform(20, 80)
    elif g.startswith("nystagmus"):
        p['NYS'] = r.uniform(20, 60); p['NYF'] = r.uniform(2, 5)
    elif g.startswith("very strong"):
        p['BA'] *= r.uniform(1.4, 1.9); p['CE'] *= r.uniform(1.4, 2.0); p['BE'] *= r.uniform(1.3, 1.8)
    return p

# ---------------------------------------------------------------- sim plumbing
def run(exe, sched, p, seed, nv_in=None, nv_out=None):
    pf = f'pk_{uuid.uuid4().hex}.txt'
    env = dict(os.environ)
    env.update({k: f"{v:.4f}" for k, v in p.items() if not k.startswith('_')})
    env.update(SCHED=','.join(sched), SEED=str(seed), TSLOG='1', PKT_OUT=pf)
    if nv_in: env['NVS_IN'] = nv_in
    if nv_out: env['NVS_OUT'] = nv_out
    out = subprocess.run([exe], env=env, capture_output=True, text=True, errors='replace').stdout.splitlines()
    pk = [(int(l.split()[0]) / 1e6, l.split()[1]) for l in open(pf)]
    os.remove(pf)
    return out, pk
def ts(l):
    try: return float(l.split('|')[0])
    except Exception: return -1
def moves(pk, a=0.0, b=1e9):
    m = []; last = 'S'
    for t, c in pk:
        if c != last and c != 'S' and a <= t < b: m.append((t, c))
        last = c
    return m
def gesture_items(k, t, p):
    if k == 'B': return [f"B@{t:.2f}"], 1.05
    if k == 'BB': return [f"B@{t:.2f}", f"B@{t + p['_dgap'] * p.get('BT', 1):.2f}"], p['_dgap'] + 0.3
    cd = p['CD']
    if k == 'C': return [f"C@{t:.2f}"], 1.6 + max(0, cd - 0.7)
    t2 = t + cd + p['_cgap']
    return [f"C@{t:.2f}", f"C@{t2:.2f}"], (t2 - t) + 0.95 + cd * 0.3

def background(p, r, t0, t1, rate_scale=1.0, everyday=False):
    """natural blinks + involuntary events between t0 and t1"""
    ev = []; t = t0
    while True:
        t += r.expovariate(p['_blinkrate'] * rate_scale / 60.0)
        if t >= t1: break
        ev.append(f"b@{t:.2f}")
    for k in p['_inv']:
        rate = {'X': 2.0, 'M': 2.0, 'B': 4.0}[k]                       # per minute
        t = t0
        while True:
            t += r.expovariate(rate / 60.0)
            if t >= t1: break
            if k == 'X': ev.append(f"X{r.uniform(0.2, 1.0):.2f}@{t:.2f}")
            if k == 'M': ev.append(f"M{r.uniform(0.8, 2.5):.1f}@{t:.2f}")
            if k == 'B': ev.append(f"B@{t:.2f}")
    return ev

# ---------------------------------------------------------------- calibration followers
def follow(exe, p, seed, sched, rules, t_end, nv_out=None, max_iter=40):
    """closed loop: rules = list of (regex, action(line_time) -> list of schedule items). Each prompt answered once."""
    done = set()
    for _ in range(max_iter):
        out, _ = run(exe, sched + [f"Kh@{t_end:.1f}"], p, seed)
        new = None
        for l in out:
            t = ts(l)
            for i, (rx, act) in enumerate(rules):
                if (i, round(t, 3)) in done: continue
                if re.search(rx, l):
                    new = (i, round(t, 3), act(t)); break
            if new: break
        if not new: break
        done.add((new[0], new[1])); sched += new[2]
        t_end = max(t_end, max(float(x.split('@')[1]) for x in new[2]) + 20 if new[2] else t_end)
    if nv_out: run(exe, sched + [f"Kh@{t_end:.1f}"], p, seed, nv_out=nv_out)
    return sched, t_end

def calibrate_train(exe, p, seed, nv):
    """Pro-Safe keyboard training 't' (needs a keyboard - upper bound for the old firmware)"""
    rc = p['_react']; bt = p.get('BT', 1)
    rules = [(r'Step 1/3', lambda t: [f"B@{t + 2.0 + rc + i * 2.8:.2f}" for i in range(5)]),
             (r'Step 2/3', lambda t: [f"C@{t + 2.0 + rc + i * (3.0 + max(0, p['CD'] - 0.7)):.2f}" for i in range(5)]),
             (r'Step 3/3', lambda t: [f"Q2.80@{t + 2.5 + 3.0 + rc:.2f}", f"Q2.80@{t + 2.5 + 9.0 + rc:.2f}"])]
    follow(exe, p, seed, ["Kt@8"], rules, 40, nv_out=nv)

def calibrate_auto(exe, p, seed, nv):
    """new firmware: power-on calibration guided by beeps / [AUTO] prompts"""
    rc = p['_react']
    rules = [(r'\[AUTO\] BLINK', lambda t: [f"B@{t + rc:.2f}"]),
             (r'\[AUTO\] CLENCH', lambda t: [f"C@{t + rc:.2f}"]),
             (r'\[AUTO\] CLOSE', lambda t: [f"Q{3.0 + 0.5 * rc:.2f}@{t + rc:.2f}"])]
    return follow(exe, p, seed, [], rules, 15, nv_out=nv)

# ---------------------------------------------------------------- tests
def lock_state(out, t):
    st, since = 'locked', 0.0
    for l in out:
        x = ts(l)
        if x > t: break
        if '[LOCK] LOCKED' in l: st, since = 'locked', x
        elif 'READY - drive' in l: st, since = 'ready', x
        elif '[LOCK] ACTIVE' in l: st, since = 'wait', x
    return st, since

def drive(exe, p, seed, nv, t_start, prefix, rounds=3, per=6, life=True):
    r = random.Random(seed + 4242)
    sched = list(prefix); plan = []; sqz = []; brk = []; relock_mid = 0; unl_fail = 0
    def sim(tend): return run(exe, sched + [f"Kh@{tend:.2f}"], p, seed, nv_in=nv)[0]
    def unlock(t):
        nonlocal unl_fail
        for attempt in range(3):
            sqz.append(t)
            if not p.get('_biteUnlock', False): sched.append(f"Q{p['_sqz']:.2f}@{t:.2f}")
            else: sched.append(f"H{p['_sqz']:.2f}@{t:.2f}")       # the firmware said: bite and hold
            out = sim(t + 14)
            rdy = [ts(l) for l in out if 'READY - drive' in l and ts(l) > t]
            if rdy: return rdy[0] + p['_ready']
            unl_fail += 1; t += 9.0
        return t + 8.0
    t = t_start
    for rd in range(rounds):
        tg = unlock(t)
        for i in range(per):
            st, since = lock_state(sim(tg), tg - p['_react'])
            if st == 'locked':
                relock_mid += 1; tg = unlock(max(tg, since + p['_react'] + 0.4))
            k = r.choice(['B', 'BB', 'C', 'CC'])
            items, lat = gesture_items(k, tg, p)
            sched += items; plan.append((tg, k, rd))
            t_prev = tg
            tg += lat + r.uniform(*p['_pause']) + 0.42
            if life: sched += background(p, r, t_prev + lat + 0.3, tg - 0.1)
        b = r.uniform(5.0, 7.0); brk.append((tg, tg + b)); t = tg + b
    out, pk = run(exe, sched + [f"Kh@{t + 2:.1f}"], p, seed, nv_in=nv)
    mv = moves(pk, t_start)
    res = dict(ok=0, n=len(plan), wrong=Counter(), tries=len(sqz), unl_fail=unl_fail, relock_mid=relock_mid, extra=0, wrongmove=0)
    for i, (ta, k, rd) in enumerate(plan):
        tb = plan[i + 1][0] if i + 1 < len(plan) and plan[i + 1][2] == rd else ta + 4.0
        g = [c for tm, c in mv if ta <= tm < tb + 0.2]
        if g[:1] == [EXP[k]]: res['ok'] += 1
        else: res['wrong'][f"{k}->{g[0] if g else 'none'}"] += 1
        res['extra'] += max(0, len(g) - 1)
        res['wrongmove'] += sum(1 for c in g[:1] if c != EXP[k]) + max(0, len(g) - 1)
    res['drive_s'] = t - t_start; res['plan'] = [(round(a, 2), k) for a, k, _ in plan]
    return res

def everyday(exe, p, seed, nv, t0, prefix, minutes=4):
    r = random.Random(seed + 999); ev = list(prefix); t = t0; t1 = t0 + minutes * 60
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
    ev += background(p, r, t0, t1)
    out, pk = run(exe, ev + [f"Kh@{t1 + 2:.1f}"], p, seed, nv_in=nv)
    return dict(unlocks=sum(1 for l in out if 'UNLOCKED' in l and ts(l) > t0), moves=len(moves(pk, t0)))

def test(exe, mode, seed):
    p = person(seed)
    nv = f'nv_e_{seed}_{uuid.uuid4().hex[:6]}.bin'; prefix = []; t0 = 8.0; nv_use = None
    R = dict(seed=seed, group=p['_group'])
    if mode == 'train':
        calibrate_train(exe, p, seed, nv); nv_use = nv if os.path.exists(nv) else None
    elif mode == 'auto':
        prefix, tcal = calibrate_auto(exe, p, seed, None)
        out, _ = run(exe, prefix + [f"Kh@{tcal:.1f}"], p, seed)
        R['cal'] = [l.split('| ', 1)[1][:120] for l in out if '[AUTO]' in l and ('result' in l.lower() or 'fail' in l.lower() or 'using' in l.lower())]
        done = [ts(l) for l in out if '[AUTO] done' in l]
        p['_biteUnlock'] = any('[AUTO] done' in l and 'BITE AND HOLD' in l for l in out)
        t0 = (done[0] if done else tcal) + 3.0
    R['drive'] = drive(exe, p, seed, nv_use, t0, prefix, life=os.environ.get('LIFE', '1') == '1')
    R['life'] = everyday(exe, p, seed, nv_use, t0, prefix)
    if os.path.exists(nv): os.remove(nv)
    return R

def summary(RES, label=""):
    by = defaultdict(list)
    for x in RES: by[x['group']].append(x)
    lines = [f"=== {label}  ({len(RES)} people)"]
    hdr = f"  {'group':34s} {'n':>3s} {'gestures ok':>13s} {'wrong moves':>11s} {'unlock fails':>12s} {'relock mid':>10s} {'false unl':>9s} {'locked moves':>12s}"
    lines.append(hdr)
    def row(name, xs):
        ok = sum(x['drive']['ok'] for x in xs); n = sum(x['drive']['n'] for x in xs)
        wm = sum(x['drive']['wrongmove'] for x in xs); uf = sum(x['drive']['unl_fail'] for x in xs); tr = sum(x['drive']['tries'] for x in xs)
        rm = sum(x['drive']['relock_mid'] for x in xs); fu = sum(x['life']['unlocks'] for x in xs); lm = sum(x['life']['moves'] for x in xs)
        return f"  {name:34s} {len(xs):3d} {ok:5d}/{n:<4d}{100 * ok / max(1, n):3.0f}% {wm:11d} {uf:6d}/{tr:<5d} {rm:10d} {fu:9d} {lm:12d}"
    for g in [g for g, _ in GROUPS] + ['like you (+-20 %)']:
        if by[g]: lines.append(row(g, by[g]))
    lines.append(row("ALL", RES))
    return "\n".join(lines)

if __name__ == "__main__":
    exe, mode, s0, N = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
    outf = sys.argv[5] if len(sys.argv) > 5 else None
    with ThreadPoolExecutor(max_workers=2) as ex:
        RES = list(ex.map(lambda s: test(exe, mode, s), range(s0, s0 + N)))
    print(summary(RES, f"{exe} {mode} seeds {s0}-{s0 + N - 1}"))
    if outf: json.dump(RES, open(outf, 'w'), default=lambda o: dict(o))
