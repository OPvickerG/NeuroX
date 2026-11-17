"""Virtual YOU: the real NeuroX_Final firmware driven by a model built from your measured signals (YOU in everyone.py).
usage: python3 me.py ./sim <scenario> <N sessions> [out.json]
scenarios: exact | days | tired | sweaty | rushed | all"""
import sys, os, json, random
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
import everyone as E

def you(seed, scen):
    r = random.Random(seed * 6151 + 17)
    p = dict(E.YOU)                                    # your measured face, unchanged
    p['CD'] = r.uniform(0.6, 0.8)                      # your bite ~0.7 s
    p['_react'] = r.uniform(0.3, 0.5); p['_ready'] = r.uniform(0.4, 0.8)
    p['_pause'] = (r.uniform(0.7, 1.0), r.uniform(1.5, 2.0)); p['_sqz'] = r.uniform(3.4, 3.9)
    p['_dgap'] = r.uniform(0.35, 0.45); p['_cgap'] = r.uniform(0.35, 0.55)
    p['_blinkrate'] = r.uniform(12, 18); p['_inv'] = []
    if scen == 'days':      # same you, different day: electrodes placed a bit differently, gel fresher/older
        for k in ('VN', 'HN', 'BA', 'BE', 'CE', 'CDC', 'SQE', 'SQDC', 'SQLOBE', 'FQE', 'CHE', 'TKE'):
            p[k] *= r.uniform(0.8, 1.2)
    if scen == 'tired':     # end of a long day: slower blinks, softer squeeze, slower reactions, more blinking
        p['BT'] = r.uniform(1.2, 1.5); p['BA'] *= r.uniform(0.75, 0.9); p['SQE'] *= r.uniform(0.7, 0.85)
        p['_react'] = r.uniform(0.5, 0.9); p['_blinkrate'] = r.uniform(18, 28); p['CD'] = r.uniform(0.8, 1.1)
    if scen == 'sweaty':    # hot day / old gel: more noise, mains hum, drifting electrodes
        p['VN'] *= r.uniform(1.4, 2.0); p['HN'] *= r.uniform(1.4, 2.0); p['MAINS'] = r.uniform(5, 15)
        p['VDR'] = r.uniform(15, 50); p['HDR'] = r.uniform(15, 50)
    if scen == 'rushed':    # driving fast: short pauses, quick double gestures
        p['_pause'] = (r.uniform(0.4, 0.6), r.uniform(0.8, 1.1)); p['_dgap'] = r.uniform(0.3, 0.35); p['_cgap'] = r.uniform(0.3, 0.4)
    p['_group'] = scen
    return p

SCEN = os.environ.get('SCEN', 'exact')
E.person = lambda seed: you(seed, SCEN)

if __name__ == "__main__":
    exe, scen, N = sys.argv[1], sys.argv[2], int(sys.argv[3]); outf = sys.argv[4] if len(sys.argv) > 4 else None
    SCEN = scen; E.person = lambda seed: you(seed, scen)
    with ThreadPoolExecutor(max_workers=2) as ex:
        RES = list(ex.map(lambda s: E.test(exe, 'auto', s), range(int(os.environ.get('SEED0', '1000')), int(os.environ.get('SEED0', '1000')) + N)))
    ok = sum(x['drive']['ok'] for x in RES); n = sum(x['drive']['n'] for x in RES)
    wrong = Counter(); [wrong.update(x['drive']['wrong']) for x in RES]
    uf = sum(x['drive']['unl_fail'] for x in RES); tr = sum(x['drive']['tries'] for x in RES)
    rm = sum(x['drive']['relock_mid'] for x in RES); wm = sum(x['drive']['wrongmove'] for x in RES)
    fu = sum(x['life']['unlocks'] for x in RES); lm = sum(x['life']['moves'] for x in RES)
    bite = sum(1 for x in RES if any('BITE' in c for c in x.get('cal', [])))
    S = dict(scen=scen, sessions=N, ok=ok, n=n, pct=100 * ok / n, wrongmove=wm, wrong=dict(wrong), unlock_fail=uf, unlock_tries=tr,
             relock_mid=rm, false_unlocks=fu, locked_moves=lm, life_min=4 * N, cal=[x.get('cal', []) for x in RES][:3])
    print(json.dumps(S))
    if outf: json.dump(dict(summary=S, runs=RES), open(outf, 'w'), default=lambda o: dict(o))
