"""Imhotep - which pages of the game state the original's tick reads.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tools/oracle/readset.py SAVE [--ticks N] [--out FILE]

Boots the reference game, loads SAVE, then makes every page of the state (core/state/state.h)
unreadable and runs N game ticks (0x004D7090; default 816, one month). The first read of a page
faults; the page is recorded with the code address that read it, made readable again, and the
read is retried. The result is the set of 4 KiB pages the simulation reads (a superset of what it
needs, at page granularity), used to tell simulation state from display state
(docs/formats/state.md). Writes are not tracked.
"""
import json
import os
import sys

from unicorn import UC_HOOK_MEM_READ_PROT, UC_PROT_ALL, UC_PROT_WRITE, UC_PROT_EXEC
from unicorn.x86_const import UC_X86_REG_EIP

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from game import Game  # noqa: E402

STATE_BASE, STATE_END = 0x0056F000, 0x01267AA8
PAGE = 0x1000


def pages_read(g, ticks):
    """Run `ticks` game ticks; a read of a protected page stops the emulation (hook returns
    False), the page is made readable outside the hook, and the run resumes at the same
    instruction."""
    from unicorn import UcError
    from emu import RETURN, STACK_TOP
    from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EBP
    first = {}
    pending = []

    def on(uc, access, addr, size, value, _):
        pending.append((addr, size, uc.reg_read(UC_X86_REG_EIP)))
        return False

    from emu import HEAP_BASE
    h = g.uc.hook_add(UC_HOOK_MEM_READ_PROT, on)
    lo, hi = STATE_BASE, (STATE_END + PAGE - 1) & ~(PAGE - 1)
    heap_hi = (g.heap + PAGE - 1) & ~(PAGE - 1)
    g.uc.mem_protect(lo, hi - lo, UC_PROT_WRITE | UC_PROT_EXEC)
    if heap_hi > HEAP_BASE:  # the simulation heap as it is now (later allocations are not tracked)
        g.uc.mem_protect(HEAP_BASE, heap_hi - HEAP_BASE, UC_PROT_WRITE | UC_PROT_EXEC)
    try:
        for _ in range(ticks):
            esp = STACK_TOP - 0x100 - 4
            g.set_u32(esp, RETURN)
            g.uc.reg_write(UC_X86_REG_ESP, esp)
            g.uc.reg_write(UC_X86_REG_EBP, 0)
            eip = 0x004D7090
            while True:
                pending.clear()
                g.fault = None
                try:
                    g.uc.emu_start(eip, RETURN)
                except UcError:
                    if not pending:
                        raise
                if g.fault is not None:
                    raise RuntimeError(str(g.fault))
                if not pending:
                    break
                addr, size, eip = pending[0]
                for page in {addr & ~(PAGE - 1), (addr + size - 1) & ~(PAGE - 1)}:
                    first.setdefault(page, eip)
                    g.uc.mem_protect(page, PAGE, UC_PROT_ALL)
    finally:
        g.uc.mem_protect(lo, hi - lo, UC_PROT_ALL)
        if heap_hi > HEAP_BASE:
            g.uc.mem_protect(HEAP_BASE, heap_hi - HEAP_BASE, UC_PROT_ALL)
        g.uc.hook_del(h)
    return first


def main():
    argv = sys.argv[1:]
    ticks, out = 816, None
    if '--ticks' in argv:
        k = argv.index('--ticks')
        ticks = int(argv[k + 1])
        del argv[k:k + 2]
    if '--out' in argv:
        k = argv.index('--out')
        out = argv[k + 1]
        del argv[k:k + 2]
    g = Game()
    g.boot()
    g.load_save(open(argv[0], 'rb').read())
    first = pages_read(g, ticks)
    total = ((STATE_END + PAGE - 1) & ~(PAGE - 1)) - STATE_BASE
    from emu import HEAP_BASE
    heap_pages = sorted(p for p in first if p >= HEAP_BASE)
    print('%d of %d state pages read in %d ticks; simulation heap pages read: %d of %d' % (
        len(first) - len(heap_pages), total // PAGE, ticks, len(heap_pages), (g.heap - HEAP_BASE + PAGE - 1) // PAGE))
    if out:
        json.dump({'%08X' % p: '%08X' % e for p, e in sorted(first.items())}, open(out, 'w'), indent=0)


if __name__ == '__main__':
    main()
