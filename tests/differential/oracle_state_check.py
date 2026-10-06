"""Imhotep - differential test: the whole state after loading a saved game against the original's.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tests/differential/oracle_state_check.py [FILE.sav ...] [-j JOBS]
  (default: the downloaded player saves and the generated corpus saves m00 / m06 / m12)

The reference game boots in the emulator. From that same booted state, with the save's path in the
name buffer 0x00E37078, the original loads it with 0x004D5100 and the port runs its port of
0x004D5100 (core/state/loader.c; the file read through the platform, the original's unported calls
replayed: tools/oracle/harness.py compare). Every byte of the two states is compared (scratch and
display exclusions: tools/oracle/excluded.py). Exit status: 0 if all equal, 1 otherwise.
"""
import glob
import os
import struct
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))
GAME_DIR = os.environ.get('IMHOTEP_GAME_DIR', os.path.join(ROOT, 'local', 'game', 'app'))
LOAD = 0x004D5100   # load a saved game: the name buffer below, relative to the game directory
NAME = 0x00E37078


def check(save):
    import diff
    import harness
    from game import Game
    g = Game()
    g.boot()
    path = os.path.relpath(os.path.abspath(save), GAME_DIR).replace('/', '\\')
    g.write(NAME, path.encode()[:200] + b'\0')
    with tempfile.TemporaryDirectory() as tmp:
        try:
            written, ranges, bad = harness.compare(g, [(LOAD, [NAME])], tmp)
        except RuntimeError as e:
            return str(e)[:400]
    out = ['returned %d, port %d' % (ro, rp) for _, ro, rp in bad]
    if ranges:
        out.append('%d bytes differ in %d runs: %s' % (sum(n for _, n in ranges), len(ranges),
                                                      ', '.join(diff.name_address(a) for a, _ in ranges[:6])))
    return '; '.join(out) or 'equal (%d bytes loaded)' % written


def main():
    argv = sys.argv[1:]
    jobs = 1
    if '-j' in argv:
        k = argv.index('-j')
        jobs = int(argv[k + 1])
        del argv[k:k + 2]
    saves = argv or sorted(glob.glob(os.path.join(ROOT, 'local', 'saves', 'downloaded', '*', '*.sav')) +
                           [p for m in ('m00', 'm06', 'm12')
                            for p in glob.glob(os.path.join(ROOT, 'local', 'saves', 'generated', '*', m + '.sav'))])
    from concurrent.futures import ProcessPoolExecutor
    passed = 0
    with ProcessPoolExecutor(jobs) as pool:
        for save, result in zip(saves, pool.map(check, saves)):
            print('  %-60s %s' % (os.path.relpath(save, ROOT), result), flush=True)
            passed += result.startswith('equal')
    print('oracle_state_check: %d / %d saves loaded equal to the original' % (passed, len(saves)))
    return 0 if passed == len(saves) else 1


if __name__ == '__main__':
    sys.exit(main())
