"""Imhotep - differential test: our save writer against the original's.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tests/differential/oracle_save_check.py [FILE.sav ...]
  (default: the downloaded player saves and the m06 corpus saves)

The reference game boots in the emulator and loads each save with its own loader. From that same
state, the original's saver 0x004D4E20 writes a file, and the port's game_save
(core/state/save_game.c, build/state_save) writes another. The two files must be equal byte for
byte (that includes the PKWARE compression of every compressed chunk).
Exit status: 0 if all equal, 1 otherwise, 2 on setup error.
"""
import glob
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))
from game import Game  # noqa: E402


def main():
    files = sys.argv[1:] or sorted(glob.glob(os.path.join(ROOT, 'local', 'saves', 'downloaded', '*', '*.sav')) +
                                   glob.glob(os.path.join(ROOT, 'local', 'saves', 'generated', '*', 'm06.sav')))
    failed = 0
    with tempfile.TemporaryDirectory() as tmp:
        for path in files:
            g = Game()
            g.boot()
            if g.load_save(open(path, 'rb').read()) != 0:
                print('  %s: the original cannot load it' % path)
                return 2
            state = os.path.join(tmp, 'state')
            open(state, 'wb').write(g.state_bytes())
            theirs = g.save('check.sav')
            ours_path = os.path.join(tmp, 'ours.sav')
            r = subprocess.run([os.path.join(ROOT, 'build', 'state_save'), state, ours_path], capture_output=True, text=True)
            if r.returncode != 0:
                print('  %s: state_save failed: %s' % (path, r.stderr.strip()))
                return 2
            ours = open(ours_path, 'rb').read()
            label = os.path.relpath(path, ROOT)
            if ours == theirs:
                print('  %s: equal (%d bytes)' % (label, len(ours)), flush=True)
                continue
            failed += 1
            k = next((i for i in range(min(len(ours), len(theirs))) if ours[i] != theirs[i]), min(len(ours), len(theirs)))
            print('  %s: DIFFERENT, %d vs %d bytes, first difference at byte %d' % (label, len(ours), len(theirs), k),
                  flush=True)
    print('oracle_save_check: %d saves, %d written identically' % (len(files), len(files) - failed))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
