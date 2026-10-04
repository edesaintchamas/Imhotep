"""Imhotep - which call of a game function wrote which part of the state.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tools/oracle/attribute.py SAVE [FUNC]

Boots the reference game, then loads SAVE with 0x004D5100 while a code hook sits after every
CALL instruction of FUNC (default: 0x004D5100 and the scenario start 0x004D52A0). At each hook the
state is compared with the previous snapshot, so each call's writes are listed separately
(byte ranges, named after docs/formats/save.md).
"""
import os
import sys

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import UC_HOOK_CODE

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from game import Game  # noqa: E402
import diff  # noqa: E402
from where import where  # noqa: E402

STATE_BASE, STATE_END = 0x0056F000, 0x01267AA8


def call_sites(g, func, size):
    """[(address after the call, call target text)] of the CALL instructions in func."""
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    code = g.read(func, size)
    return [(i.address + i.size, i.op_str) for i in md.disasm(code, func) if i.mnemonic == 'call']


def load_with_attribution(g, data, funcs=(0x004D5100, 0x004D52A0)):
    """Load `data` with 0x004D5100 on the booted game g; returns [((function, call text), ranges)]
    in order, each the state bytes changed since the previous call site of `funcs` returned."""
    import json
    sites = {}
    for f in funcs:
        size = json.load(open(os.path.join(diff.ROOT, 'local', 'ghidra-export', 'functions',
                                           '0x%08X.json' % f)))['size']
        for after, target in call_sites(g, f, size):
            sites[after] = (f, target)
    state = {'snap': g.state_bytes()}
    report = []

    def on(uc, addr, size, _):
        if addr not in sites:
            return
        snap = g.state_bytes()
        ranges = diff.state_file_diff(state['snap'], snap)
        state['snap'] = snap
        if ranges:
            report.append((sites[addr], ranges))

    hooks = [g.uc.hook_add(UC_HOOK_CODE, on, begin=a, end=a) for a in sites]
    result = g.load_save(data)
    snap = g.state_bytes()
    ranges = diff.state_file_diff(state['snap'], snap)
    if ranges:
        report.append(((funcs[0], 'end'), ranges))
    for h in hooks:
        g.uc.hook_del(h)
    if result != 0:
        raise RuntimeError('the game could not load the save')
    return report


def step_label(step):
    f, target = step
    t = int(target, 16) if target.startswith('0x') else None
    return '0x%08X -> %s' % (f, where(t).split()[0] if t and where(t) else target)


def main():
    save = sys.argv[1]
    funcs = [int(a, 16) for a in sys.argv[2:]] or [0x004D5100, 0x004D52A0]
    g = Game()
    g.boot()
    report = load_with_attribution(g, open(save, 'rb').read(), funcs)
    for step, ranges in report:
        total = sum(n for _, n in ranges)
        print('%s: %d bytes in %d runs' % (step_label(step), total, len(ranges)))
        for a, n in ranges[:10]:
            print('    %s: %d' % (diff.name_address(a), n))
        if len(ranges) > 10:
            print('    ... %d more runs' % (len(ranges) - 10))


if __name__ == '__main__':
    main()
