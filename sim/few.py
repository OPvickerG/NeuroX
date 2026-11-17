import sys, everyone as E
from concurrent.futures import ThreadPoolExecutor
exe = sys.argv[1]; seeds = [int(x) for x in sys.argv[2:]]
def one(s):
    R = E.test(exe, 'auto', s); d = R['drive']
    return f"{s:3d} {R['group'][:14]:14s} drive {d['ok']}/{d['n']} unlfail {d['unl_fail']} relock {d['relock_mid']} {dict(d['wrong'])} | life unl {R['life']['unlocks']} moves {R['life']['moves']}"
with ThreadPoolExecutor(2) as ex:
    for r in ex.map(one, seeds): print(r, flush=True)
