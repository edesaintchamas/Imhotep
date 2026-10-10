#!/usr/bin/env python3
"""Imhotep - settle names after merging phase 10 packages (docs/naming.md): one name per address.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  python3 tools/naming/resolve.py [--alias OLD=NEW ...] [--repair] [--dry]

1. Aliases: a name a package renamed (OLD, no longer defined) still used by another package's code is
   replaced by NEW everywhere in core/, present/, tests/ and tools/ (C sources and headers).
2. Duplicates: an address defined under several names in the layout headers keeps one: a name in an
   area header (core/state/layout/*.h, present/layout/*.h) over one in a pending fragment, a meaningful
   name over a neutral one (G_U<addr>, *_U<off>), then the one defined first (header order). The
   others' #define lines are removed and their uses renamed.
Prints what it does. Then check: tools/check_same_code.py (identical code) and rename.py --check.
"""
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFINE = re.compile(r'^\s*#define\s+([A-Z][A-Z0-9_]*)\s+(0x[0-9A-Fa-f]+)u?\b')
NEUTRAL = re.compile(r'^(G_U[0-9A-F]+|[A-Z]+_U[0-9A-F]+)$')


def headers():
    out = sorted(glob.glob(os.path.join(ROOT, 'core/state/layout/*.h'))) + \
        sorted(glob.glob(os.path.join(ROOT, 'present/layout/*.h')))
    pending = sorted(glob.glob(os.path.join(ROOT, 'core/state/layout/pending/*.h')))
    return [(p, False) for p in out] + [(p, True) for p in pending]


def sources():
    files = []
    for top in ('core', 'present', 'tests', 'tools'):
        for d, _, fs in os.walk(os.path.join(ROOT, top)):
            files += [os.path.join(d, f) for f in fs if f.endswith(('.c', '.h', '.inc'))]
    return files


def rename_everywhere(old, new, files, dry):
    pat = re.compile(r'\b%s\b' % re.escape(old))
    n = 0
    for f in files:
        s = open(f).read()
        t, k = pat.subn(new, s)
        if k:
            n += k
            if not dry:
                open(f, 'w').write(t)
    return n


def drop_define(lines, name):
    """lines without the #define of name, its comment's continuation lines included"""
    out = []
    skipping = False
    for line in lines:
        if skipping:
            if '*/' in line:
                skipping = False
            continue
        m = DEFINE.match(line)
        if m and m.group(1) == name:
            tail = line[line.find('/*'):] if '/*' in line else ''
            skipping = bool(tail) and '*/' not in tail
            continue
        out.append(line)
    return out


def repair(path):
    """remove comment continuation lines left outside any comment (an earlier drop that kept them)"""
    out, inside, removed = [], False, 0
    for line in open(path).read().splitlines(True):
        stripped = line.lstrip()
        if not inside and stripped.startswith('*') and not stripped.startswith('*/'):
            removed += 1
            continue
        if not inside and stripped.startswith('*/'):
            removed += 1
            continue
        # track comments opened and not closed on this line
        pos = 0
        while True:
            if inside:
                j = line.find('*/', pos)
                if j < 0:
                    break
                inside, pos = False, j + 2
            else:
                j = line.find('/*', pos)
                if j < 0:
                    break
                inside, pos = True, j + 2
        out.append(line)
    if removed:
        open(path, 'w').write(''.join(out))
    return removed


def main():
    argv = sys.argv[1:]
    dry = '--dry' in argv
    aliases = []
    if '--alias' in argv:
        for a in argv[argv.index('--alias') + 1:]:
            if a.startswith('--'):
                break
            old, new = a.split('=')
            aliases.append((old, new))
    files = sources()
    if '--repair' in argv:
        for path, _ in headers():
            n = repair(path)
            if n:
                print('repaired %s: %d stray comment lines removed' % (os.path.relpath(path, ROOT), n))
    for old, new in aliases:
        print('alias %s -> %s: %d uses' % (old, new, rename_everywhere(old, new, files, dry)))
    by_addr = {}
    order = 0
    for path, pending in headers():
        for i, line in enumerate(open(path)):
            m = DEFINE.match(line)
            if m:
                name, addr = m.group(1), int(m.group(2), 16)
                if addr < 0x00400000:  # offsets and sizes, not addresses: names may share values
                    continue
                by_addr.setdefault(addr, []).append((bool(NEUTRAL.match(name)), pending, order, name, path))
                order += 1
    for addr, defs in sorted(by_addr.items()):
        names = {d[3] for d in defs}
        if len(names) < 2:
            continue
        defs.sort()
        keep = defs[0][3]
        for _, _, _, name, path in defs[1:]:
            if name == keep:
                continue
            if not dry:
                open(path, 'w').write(''.join(drop_define(open(path).read().splitlines(True), name)))
            uses = rename_everywhere(name, keep, files, dry)
            print('0x%08X: kept %s, removed %s (%s, %d uses renamed)'
                  % (addr, keep, name, os.path.relpath(path, ROOT), uses))
    return 0


if __name__ == '__main__':
    sys.exit(main())
