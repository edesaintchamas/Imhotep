"""Imhotep - coverage: the functions the original's tick really runs, and who calls them.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tools/oracle/coverage.py [--ticks N] [--out FILE] SAVE...

The reference game loads each SAVE and runs N ticks (default 816, two months). Every exported
function (local/ghidra-export/index.json) gets a code hook on its first instruction; each entry is
counted, with its caller (the function holding the return address on the stack). Indirect calls
(the figure action table 0x005DC28C, ...) are included, which the static call graph misses. Writes
JSON (default local/oracle/coverage.json):
  {"functions": {"0x...": {"calls": n, "callers": {"0x...": n}}}}
and prints, per function called by the tick skeleton, the number and code size of the distinct
functions reachable from it in that graph.
"""
import json
import os
import sys

from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from game import Game  # noqa: E402
from where import INDEX, function_start  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SIZES = {int(f['address'], 16): f['size'] for f in INDEX}


TICK = 0x004D7090
SKELETON = {0x004D7090, 0x00451C50, 0x00452370}


class Coverage:
    def __init__(self, g, funcs):
        self.g = g
        self.funcs = funcs
        for a in SIZES:
            g.uc.hook_add(UC_HOOK_CODE, self.on_entry, begin=a, end=a)
        g.uc.ctl_flush_tb()  # code translated before (boot, load) would not see the new hooks

    def on_entry(self, uc, addr, size, _):
        f = self.funcs.setdefault('0x%08X' % addr, {'calls': 0, 'callers': {}})
        f['calls'] += 1
        caller = function_start(self.g.u32(uc.reg_read(UC_X86_REG_ESP)))
        c = '0x%08X' % caller if caller else 'other'
        f['callers'][c] = f['callers'].get(c, 0) + 1


def main():
    argv = sys.argv[1:]
    ticks, out = 816, os.path.join(ROOT, 'local', 'oracle', 'coverage.json')
    if '--ticks' in argv:
        k = argv.index('--ticks')
        ticks = int(argv[k + 1])
        del argv[k:k + 2]
    if '--out' in argv:
        k = argv.index('--out')
        out = argv[k + 1]
        del argv[k:k + 2]
    funcs = json.load(open(out))['functions'] if os.path.exists(out) and '--append' in argv else {}
    argv = [a for a in argv if a != '--append']
    for save in argv:
        g = Game()
        g.boot()
        if g.load_save(open(save, 'rb').read()) != 0:
            print('the original cannot load %s' % save)
            return 2
        cov = Coverage(g, funcs)
        for _ in range(ticks):
            g.call(TICK)
        print('%s: %d functions so far' % (save, len(funcs)), flush=True)
    json.dump({'functions': funcs}, open(out, 'w'), indent=0, sort_keys=True)
    callees = {}
    for a, f in funcs.items():
        for c in f['callers']:
            callees.setdefault(c, set()).add(a)
    per_root = {}
    for r in sorted({c for s in SKELETON for c in callees.get('0x%08X' % s, ())} - {'0x%08X' % s for s in SKELETON}):
        seen, todo = set(), [r]
        while todo:
            a = todo.pop()
            if a not in seen:
                seen.add(a)
                todo += callees.get(a, ())
        per_root[r] = {int(a, 16) for a in seen}
    for r, s in sorted(per_root.items(), key=lambda kv: -sum(SIZES.get(a, 0) for a in kv[1])):
        print('%-10s %5d functions %8d bytes' % (r, len(s), sum(SIZES.get(a, 0) for a in s)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
