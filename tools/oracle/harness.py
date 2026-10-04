"""Imhotep - the function oracle: an original game function against its port, on the same state.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tools/oracle/harness.py ADDR[:ARG,...] [ADDR ...] [--ticks N]
        [--fill ADDR:LEN:BYTE ...] [SAVE ...]

For each save (default: the downloaded player saves and the m06 corpus saves), the reference game
boots in the emulator, loads the save with its own loader, and optionally runs N game ticks. The
whole game state (Pharaoh.exe's data segments, core/state/state.h) is then taken as the input of
both sides:
  - the original functions run in the emulator, in order;
  - their ports run on a copy of the same bytes (build/state_run, core/state/ported.c).
--fill overwrites a region of the input first (e.g. an output grid, so that a recomputation is
seen in full). Every byte of the two results is compared; differences are named after docs/formats/save.md.
Exit status: 0 when every result is equal, 1 otherwise, 2 on setup error.
"""
import glob
import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from game import Game  # noqa: E402
import diff  # noqa: E402
import excluded  # noqa: E402

ROOT = diff.ROOT
STATE_BASE, STATE_END = 0x0056F000, 0x01267AA8  # core/state/state.h
GAME_TICK = 0x004D7090


def parse_call(text):
    addr, _, args = text.partition(':')
    return int(addr, 16), [int(a, 0) & 0xFFFFFFFF for a in args.split(',') if a]


def oracle_state(save, ticks):
    g = Game()
    g.boot()
    if g.load_save(open(save, 'rb').read()) != 0:
        raise RuntimeError('the game could not load %s' % save)
    for _ in range(ticks):
        g.call(GAME_TICK)
    return g


def ported():
    """{address: (nargs, return mask, name)} from build/state_run --list."""
    r = subprocess.run([os.path.join(ROOT, 'build', 'state_run'), '--list'], capture_output=True, text=True, check=True)
    out = {}
    for line in r.stdout.splitlines():
        a, n, bits, this, name = line.split()
        out[int(a, 16)] = (int(n), (1 << int(bits)) - 1, name, int(this, 16))
    return out


def compare(g, calls, tmp, limit=12):
    """Run calls in the oracle and the port from g's current state; returns the differences."""
    table = ported()
    RET_MASK = [table[a][1] if a in table else 0 for a, _ in calls]
    pre = g.state_bytes()
    returns_o = []
    for addr, args in calls:
        this = table[addr][3] if addr in table and table[addr][3] else None
        returns_o.append(g.call(addr, *args, ecx=this))
    post_o = g.state_bytes()
    src, dst = os.path.join(tmp, 'pre.state'), os.path.join(tmp, 'post.state')
    open(src, 'wb').write(pre)
    spec = ['%08X' % a + (':' + ','.join(str(x) for x in args) if args else '') for a, args in calls]
    r = subprocess.run([os.path.join(ROOT, 'build', 'state_run'), src, dst] + spec, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError('state_run exit %d: %s' % (r.returncode, r.stderr.strip()))
    post_p = open(dst, 'rb').read()
    returns_p = [int(x) & 0xFFFFFFFF for x in r.stdout.split()]
    written = sum(n for _, n in diff.state_file_diff(pre, post_o))
    bad_returns = [(calls[i], returns_o[i], returns_p[i]) for i in range(len(calls))
                   if (returns_o[i] & RET_MASK[i]) != (returns_p[i] & RET_MASK[i])]
    compare.last = (pre, post_o, post_p)
    reason = excluded.excluder(post_o)
    ranges = [(a, n) for a, n in diff.state_file_diff(post_o, post_p) if not reason(a)]
    return written, ranges, bad_returns


def main():
    argv = sys.argv[1:]
    ticks = 0
    if '--ticks' in argv:
        k = argv.index('--ticks')
        ticks = int(argv[k + 1])
        del argv[k:k + 2]
    fills = []
    while '--fill' in argv:
        k = argv.index('--fill')
        a, n, v = argv[k + 1].split(':')
        fills.append((int(a, 16), int(n, 0), int(v, 0)))
        del argv[k:k + 2]
    calls = [parse_call(a) for a in argv if not a.endswith('.sav')]
    saves = [a for a in argv if a.endswith('.sav')] or sorted(
        glob.glob(os.path.join(ROOT, 'local', 'saves', 'downloaded', '*', '*.sav')) +
        glob.glob(os.path.join(ROOT, 'local', 'saves', 'generated', '*', 'm06.sav')))
    if not calls:
        print(__doc__.split('\n\n')[1])
        return 2
    failed = 0
    with tempfile.TemporaryDirectory() as tmp:
        for save in saves:
            g = oracle_state(save, ticks)
            for a, n, v in fills:
                g.write(a, bytes([v]) * n)
            written, ranges, bad_returns = compare(g, calls, tmp)
            label = os.path.relpath(save, ROOT)
            for (a, args), ro, rp in bad_returns[:8]:
                print('  %s: 0x%08X(%s) returned %d, port %d' % (label, a, ','.join(map(str, args)), ro, rp))
            if bad_returns and not ranges:
                failed += 1
                continue
            if not ranges:
                print('  %s: equal (the original changed %d bytes)' % (label, written), flush=True)
                continue
            failed += 1
            total = sum(n for _, n in ranges)
            print('  %s: %d bytes differ in %d runs (the original changed %d bytes)' % (label, total, len(ranges), written))
            pre, post_o, post_p = compare.last
            for addr, n in ranges[:12]:
                k = addr - STATE_BASE if addr < 0x60000000 else None
                if k is None:
                    print('    %s, %d bytes' % (diff.name_address(addr), n))
                    continue
                k4 = k & ~3  # the aligned dword around it
                print('    %s, %d bytes: before %s original %s port %s' % (
                    diff.name_address(addr), n, pre[k4:k4 + 4][::-1].hex(), post_o[k4:k4 + 4][::-1].hex(),
                    post_p[k4:k4 + 4][::-1].hex()), flush=True)
    print('harness: %s: %d / %d states equal' % (' '.join('0x%08X' % a for a, _ in calls), len(saves) - failed, len(saves)))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
