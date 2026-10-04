"""Imhotep - is a global scratch, i.e. always written before it is read within a tick?

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tools/oracle/scratch.py SAVE [--ticks N] ADDR[:LEN] ...

Boots the reference game, loads SAVE, then runs N game ticks (default 816, one month) with
read and write hooks on the given byte ranges. At the start of each tick every byte counts as
unwritten; a read of an unwritten byte is a "read before write" and is reported with the reading
function. A range with none over the run is scratch: its value between ticks cannot influence the
simulation, so state comparisons at tick boundaries may ignore it (tools/oracle/excluded.py).
The load itself (0x004D5100) is also checked, as one more "tick".
"""
import os
import sys

from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EIP

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from game import Game  # noqa: E402
from where import where  # noqa: E402


def main():
    argv = sys.argv[1:]
    ticks = 816
    if '--ticks' in argv:
        k = argv.index('--ticks')
        ticks = int(argv[k + 1])
        del argv[k:k + 2]
    save, specs = argv[0], argv[1:]
    ranges = []
    for spec in specs:
        a, _, n = spec.partition(':')
        ranges.append((int(a, 16), int(n, 0) if n else 4))
    g = Game()
    g.boot()
    written = set()
    first_bad = {}

    def on_write(uc, access, addr, size, value, _):
        for k in range(size):
            written.add(addr + k)

    def on_read(uc, access, addr, size, value, _):
        for k in range(size):
            if addr + k not in written:
                for lo, n in ranges:
                    if lo <= addr + k < lo + n and lo not in first_bad:
                        eip = uc.reg_read(UC_X86_REG_EIP)
                        first_bad[lo] = '0x%08X %s' % (eip, where(eip))

    for lo, n in ranges:
        g.uc.hook_add(UC_HOOK_MEM_WRITE, on_write, begin=lo, end=lo + n - 1)
        g.uc.hook_add(UC_HOOK_MEM_READ, on_read, begin=lo, end=lo + n - 1)
    g.load_save(open(save, 'rb').read())
    for _ in range(ticks):
        written.clear()
        g.call(0x004D7090)
    for lo, n in ranges:
        print('0x%08X:%d %s' % (lo, n, 'scratch' if lo not in first_bad else 'read before write by ' + first_bad[lo]))


if __name__ == '__main__':
    main()
