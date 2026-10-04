"""Imhotep - differential test: the whole state after our load against the original's.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tests/differential/oracle_state_check.py [-v] [--readset F.json ...] [FILE.sav ...]
  (default: the downloaded player saves)

The reference game boots in the emulator. From that same booted state (Pharaoh.exe's data
segments, core/state/state.h), the original loads the save with 0x004D5100 and the port loads it
with game_load_save (build/state_load). Every byte of the two results is compared. Each
difference is listed under the step of 0x004D5100 / scenario start 0x004D52A0 that last wrote it
in the original (tools/oracle/attribute.py), so the report reads as a to-do list of steps.
Each step's bytes are split into those on pages the original's tick reads ("sim", from the read
sets of tools/oracle/readset.py: local/oracle/readset_*.json, or --readset files) and the others.
A save passes when no "sim" byte differs; the other bytes are display state (docs/formats/state.md). The heap is part of the state
(core/state/state.h): the port allocates as the oracle does, so stored pointers compare equal.
Exit status: 0 if equal, 1 otherwise, 2 on setup error.
"""
import bisect
import glob
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))
from game import Game  # noqa: E402
import attribute  # noqa: E402
import excluded  # noqa: E402
import diff  # noqa: E402

STATE_BASE, STATE_END = 0x0056F000, 0x01267AA8


def last_writer(report):
    """address -> step label of the last step that wrote it (interval lookup)."""
    spans = []  # (start, end, order, label)
    for order, (step, ranges) in enumerate(report):
        label = attribute.step_label(step)
        spans.extend((a, a + n, order, label) for a, n in ranges)
    spans.sort()
    starts = [s[0] for s in spans]

    def lookup(addr):
        best = None
        i = bisect.bisect_right(starts, addr)
        for s in spans[max(0, i - 2000):i]:
            if s[0] <= addr < s[1] and (best is None or s[2] > best[0]):
                best = (s[2], s[3])
        return best[1] if best else 'not written by the original load'
    return lookup


READ_PAGES = set()


def check(path, tmp, verbose):
    g = Game()
    g.boot()
    pre = os.path.join(tmp, 'pre.state')
    open(pre, 'wb').write(g.state_bytes())
    report = attribute.load_with_attribution(g, open(path, 'rb').read())
    theirs = g.state_bytes()
    out = os.path.join(tmp, 'ours.state')
    r = subprocess.run([os.path.join(ROOT, 'build', 'state_load'), pre, path, out], capture_output=True, text=True)
    if r.returncode != 0:
        print('  %s: state_load failed: %s' % (path, r.stderr.strip()))
        return None
    ours = open(out, 'rb').read()
    lookup = last_writer(report)
    reason = excluded.excluder(theirs)
    by_step, skipped = {}, 0
    for a, n in diff.state_file_diff(theirs, ours):
        for k in range(a, a + n):
            if reason(k):
                skipped += 1
                continue
            by_step.setdefault(lookup(k), []).append(k)
    total = sum(len(v) for v in by_step.values())
    sim_total = sum(1 for v in by_step.values() for k in v if k & ~0xFFF in READ_PAGES)
    print('  %s: %d bytes differ, %d on simulation pages (%d more in excluded state, tools/oracle/excluded.py)'
          % (os.path.relpath(path, ROOT), total, sim_total, skipped))
    for step, addrs in sorted(by_step.items(), key=lambda kv: -len(kv[1])):
        sim = sum(1 for k in addrs if k & ~0xFFF in READ_PAGES)
        if not verbose and sim == 0:
            continue
        print('    %-40s %8d bytes (%7d sim)   e.g. %s' % (step, len(addrs), sim, diff.name_address(addrs[0])))
        if verbose:
            runs = []
            for k in addrs:
                if runs and runs[-1][1] == k:
                    runs[-1][1] = k + 1
                else:
                    runs.append([k, k + 1])
            for s, e in runs[:20]:
                print('        %s: %d bytes' % (diff.name_address(s), e - s))
    return sim_total


def main():
    import json
    args = sys.argv[1:]
    while '--readset' in args:
        k = args.index('--readset')
        READ_PAGES.update(int(p, 16) for p in json.load(open(args[k + 1])))
        del args[k:k + 2]
    if not READ_PAGES:
        for f in glob.glob(os.path.join(ROOT, 'local', 'oracle', 'readset_*.json')):
            READ_PAGES.update(int(p, 16) for p in json.load(open(f)))
    if not READ_PAGES:
        print('oracle_state_check: no read set (tools/oracle/readset.py)')
        return 2
    verbose = '-v' in args
    files = [a for a in args if a != '-v'] or sorted(glob.glob(os.path.join(ROOT, 'local', 'saves', 'downloaded', '*', '*.sav')))
    failed = 0
    with tempfile.TemporaryDirectory() as tmp:
        for path in files:
            total = check(path, tmp, verbose)
            if total is None:
                return 2
            failed += total > 0
    print('oracle_state_check: %d saves, %d with the simulation state equal to the original load'
          % (len(files), len(files) - failed))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
