"""Imhotep - frontier: the unported functions an original call still reaches.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tools/oracle/frontier.py ADDR[@THIS][:ARGS] [--ticks N] [--tick] [SAVE ...]

Runs the original ADDR(ARGS) on each loaded state (default: three downloaded saves) with the deep
recorder (tools/oracle/replay.py) and lists the outermost calls to functions that are not ported:
what the port of ADDR still replays, by calls and bytes written. These are the next functions to
port under ADDR. --tick runs whole ticks instead (ADDR ignored, give 0); --ticks N advances first.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import harness  # noqa: E402
from where import where  # noqa: E402

ROOT = harness.ROOT
DEFAULT = ['local/saves/downloaded/1592/39-Actium-Dec30c.sav', 'local/saves/downloaded/1564/CaesarAlan-Iken.sav',
           'local/saves/downloaded/1591/38-AlexC-Feb15.sav']


def main():
    argv = sys.argv[1:]
    ticks, whole_tick, repeat = 0, False, 1
    if '--ticks' in argv:
        k = argv.index('--ticks')
        ticks = int(argv[k + 1])
        del argv[k:k + 2]
    if '--tick' in argv:
        whole_tick = True
        argv.remove('--tick')
        repeat = 51
    calls = [harness.parse_call(a) for a in argv if not a.endswith('.sav')]
    saves = [a for a in argv if a.endswith('.sav')] or [os.path.join(ROOT, s) for s in DEFAULT]
    count, size = {}, {}
    for save in saves:
        g = harness.oracle_state(save, ticks)
        rec = harness.recorder(g)
        for _ in range(repeat):
            if whole_tick:
                rec.call(harness.GAME_TICK)
            else:
                for addr, args in calls:
                    rec.call(addr, args, harness.THIS.get(addr))
            rec.take()
        for a, n in rec.rec_count.items():
            count[a] = count.get(a, 0) + n
            size[a] = size.get(a, 0) + rec.rec_bytes.get(a, 0)
    print('function       calls      bytes')
    for a in sorted(count, key=lambda a: -size[a]):
        w = where(a)
        print('0x%08X %8d %10d  %s' % (a, count[a], size[a], w.split()[1] if w else ''))
    return 0


if __name__ == '__main__':
    sys.exit(main())
