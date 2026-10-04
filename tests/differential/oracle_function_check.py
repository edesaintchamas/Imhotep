"""Imhotep - differential test: every ported game function against the original, on saved states.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tests/differential/oracle_function_check.py [ADDR ...]

For each test state (player saves loaded by the original, in the emulator) and each entry of
TESTS, the original functions and their ports (core/state/ported.c, build/state_run) run on the
same state; the whole state (data segments, simulation heap) and every return value must be
equal (tools/oracle/harness.py). The state is restored between entries.
Exit status: 0 if all equal, 1 otherwise, 2 on setup error.
"""
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))
import harness  # noqa: E402

STATES = [
    'local/saves/downloaded/1592/39-Actium-Dec30c.sav',      # no floodplain, no type-0x88 buildings
    'local/saves/downloaded/1564/CaesarAlan-Iken.sav',       # floodplain, 4 type-0x88 buildings
    'local/saves/downloaded/1591/38-AlexC-Feb15.sav',        # 1,704 buildings
]

GOODS = list(range(0, 41))

# name: (calls, fills, skip states with type-0x88 buildings). Calls: (address, args).
TESTS = {
    'desirability': ([(0x0046A0C0, []), (0x0046A140, [])], [(0x00AC2D4C, 51984, 0x55), (0x00BC9768, 4, 0xAA)], False),
    'map row ranges': ([(0x0046F7B0, [])], [(0x00BC9804, 0xAB0, 0x77), (0x00BCA2E8, 0x18, 0x77)], False),
    'map tile inside': ([(0x0046F930, [x, y]) for x in range(-2, 230, 7) for y in range(-2, 230, 7)], [], False),
    'routing grids': ([(0x00517A20, []), (0x005178B0, []), (0x005178E0, []), (0x00517660, [])],
                      [(0x00EBAB20, 51984, 0x11), (0x00EC7630, 51984, 0x22), (0x00ED4140, 51984, 0x33)], False),
    'routing ee0c50': ([(0x00517370, [])], [(0x00EE0C50, 51984, 0x44)], True),
    'routing fillers': ([(0x00517690, [3, 5, 40, 60]), (0x005173A0, [-5, -5, 300, 300]),
                         (0x00516F90, [10, 10, 100, 90, 0])],
                        [(0x00ED4140, 51984, 0x5A), (0x00EC7630, 51984, 0x5A), (0x00EE0C50, 51984, 0x5A)], False),
    'goods predicates': ([(a, [r]) for a in (0x004439C0, 0x00443950, 0x00443B90, 0x00443D90, 0x00559480, 0x004C5240)
                          for r in GOODS] + [(0x004C5190, []), (0x00447790, [0, 5]), (0x00447790, [3, 9])], [], False),
    'goods lists': ([(0x005596D0, [])], [(0x00F53DD8, 0x128, 0x66)], False),
    'post-load links': ([(0x0046A320, []), (0x0046F030, []), (0x0046B320, [])],
                        [(0x007900B8, 200 * 0xC4, 0), (0x00EAAF68, 50 * 0xE0, 0), (0x009783A4, 11 * 0x1C, 0)], False),
    'pre-load clear': ([(0x0046FA40, [])], [], False),
}


def main():
    only = {int(a, 16) for a in sys.argv[1:]}
    failed = total = 0
    with tempfile.TemporaryDirectory() as tmp:
        for path in STATES:
            path = os.path.join(ROOT, path)
            g = harness.oracle_state(path, 0)
            snap = g.state_bytes()
            has88 = 'Iken' in path
            for name, (calls, fills, skip88) in TESTS.items():
                if only and not any(a in only for a, _ in calls):
                    continue
                if skip88 and has88:
                    continue
                g.load_state(snap)
                for a, n, v in fills:
                    g.write(a, bytes([v]) * n)
                try:
                    written, ranges, bad = harness.compare(g, calls, tmp)
                except RuntimeError as e:
                    print('  %s / %s: %s' % (os.path.basename(path), name, e))
                    return 2
                total += 1
                ok = not ranges and not bad
                failed += not ok
                print('  %-28s %-16s %s (%d bytes written)' % (os.path.basename(path)[:28], name,
                                                              'equal' if ok else 'DIFFERENT', written), flush=True)
                for addr, n in ranges[:5]:
                    print('      %s: %d bytes' % (harness.diff.name_address(addr), n))
                for (a, args), ro, rp in bad[:5]:
                    print('      0x%08X(%s): original %d, port %d' % (a, ','.join(map(str, args)), ro, rp))
    print('oracle_function_check: %d / %d function tests equal to the original' % (total - failed, total))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
