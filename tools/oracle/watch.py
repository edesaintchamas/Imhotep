"""Imhotep - who writes an address: python tools/oracle/watch.py SAVE ADDR[:LEN] ... [--ticks N]

Boots the reference game and loads SAVE (then runs N ticks) with a write hook on each range;
prints every write with the writing instruction's function and the value."""
import os
import sys

from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EIP

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from game import Game  # noqa: E402
from where import where  # noqa: E402


def main():
    argv = sys.argv[1:]
    ticks = 0
    if '--ticks' in argv:
        k = argv.index('--ticks')
        ticks = int(argv[k + 1])
        del argv[k:k + 2]
    g = Game()
    g.boot()
    seen = {}

    def on(uc, access, addr, size, value, _):
        eip = uc.reg_read(UC_X86_REG_EIP)
        key = (eip, addr)
        seen[key] = seen.get(key, 0) + 1
        if seen[key] <= 2:
            print('  0x%08X <- %d bytes 0x%X at 0x%08X %s' % (addr, size, value & ((1 << (8 * size)) - 1), eip, where(eip)))

    for spec in argv[1:]:
        a, _, n = spec.partition(':')
        a = int(a, 16)
        n = int(n, 0) if n else 4
        g.uc.hook_add(UC_HOOK_MEM_WRITE, on, begin=a, end=a + n - 1)
    g.load_save(open(argv[0], 'rb').read())
    for _ in range(ticks):
        g.call(0x004D7090)


if __name__ == '__main__':
    main()
