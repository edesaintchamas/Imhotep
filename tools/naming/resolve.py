#!/usr/bin/env python3
"""Imhotep - settle names after merging phase 10 packages (docs/naming.md): one name per address.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  python3 tools/naming/resolve.py [--alias OLD=NEW ...] [--from REF] [--repair] [--dry]

--from REF: names the build reports undeclared (a package branched before a merge renamed them) are
looked up in REF's headers and replaced by this tree's name for the same address.

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
    """old -> new in files, except a file that defines old itself (a local macro of that name, not the
    layout's: e.g. present/city/ghost.h's own G_EDGE)"""
    pat = re.compile(r'\b%s\b' % re.escape(old))
    local = re.compile(r'^\s*#define\s+%s\b' % re.escape(old), re.M)
    n = 0
    for f in files:
        s = open(f).read()
        if local.search(s) and '/layout/' not in f.replace(os.sep, '/'):
            continue
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


def define_block(lines, name):
    """(start, end) of the #define of name with its comment's continuation lines, or None"""
    for i, line in enumerate(lines):
        m = DEFINE.match(line)
        if m and m.group(1) == name:
            j = i
            tail = line[line.find('/*'):] if '/*' in line else ''
            if tail and '*/' not in tail:
                while j + 1 < len(lines) and '*/' not in lines[j]:
                    j += 1
            return i, j + 1
    return None


def move_define(name, src, dst):
    """the #define of name moved from header src to header dst (before its include guard's #endif)"""
    a = open(src).read().splitlines(True)
    span = define_block(a, name)
    if not span:
        return
    block = a[span[0]:span[1]]
    del a[span[0]:span[1]]
    with open(src, 'w') as f:
        f.write(''.join(a))
    b = open(dst).read().splitlines(True)
    end = max([i for i, l in enumerate(b) if l.startswith('#endif')] or [len(b)])
    b[end:end] = block
    with open(dst, 'w') as f:
        f.write(''.join(b))


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


def undeclared():
    """identifiers the libraries' build reports as undeclared"""
    import subprocess
    out = os.path.join(ROOT, 'build-resolve')
    r = subprocess.run(['make', '-s', '-k', '-j2', 'BUILD=' + out, out + '/libimhotep_core.a',
                        out + '/libimhotep_present.a'], cwd=ROOT, capture_output=True, text=True)
    import shutil
    shutil.rmtree(out, ignore_errors=True)
    return sorted(set(re.findall(r"undeclared identifier '([A-Z][A-Z0-9_]*)'", r.stdout + r.stderr)))


def names_at(ref):
    """name -> address in the layout headers of a git revision"""
    import subprocess
    files = subprocess.run(['git', 'ls-tree', '-r', '--name-only', ref, 'core/state/layout', 'present/layout'],
                           cwd=ROOT, capture_output=True, text=True).stdout.split()
    out = {}
    for f in files:
        text = subprocess.run(['git', 'show', '%s:%s' % (ref, f)], cwd=ROOT, capture_output=True, text=True).stdout
        for line in text.splitlines():
            m = DEFINE.match(line)
            if m:
                out[m.group(1)] = int(m.group(2), 16)
    return out


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
    table = os.path.join(ROOT, 'tools', 'naming', 'aliases.txt')
    if os.path.exists(table):
        for line in open(table):
            line = line.split('#')[0].split()
            if len(line) == 2:
                aliases.append(tuple(line))
    files = sources()
    if '--from' in argv:
        ref = argv[argv.index('--from') + 1]
        theirs = names_at(ref)
        ours = {}
        for path, _ in headers():
            for line in open(path):
                m = DEFINE.match(line)
                if m:
                    ours.setdefault(int(m.group(2), 16), m.group(1))
        for name in undeclared():
            addr = theirs.get(name)
            new_name = ours.get(addr) if addr is not None else None
            if new_name:
                aliases.append((name, new_name))
                with open(os.path.join(ROOT, 'tools', 'naming', 'aliases.txt'), 'a') as f:
                    f.write('%s %s\n' % (name, new_name))
            else:
                print('undeclared %s: no address in %s or no name for it here' % (name, ref))
    if '--repair' in argv:
        for path, _ in headers():
            n = repair(path)
            if n:
                print('repaired %s: %d stray comment lines removed' % (os.path.relpath(path, ROOT), n))
    for old, new in aliases:
        # a local macro of the same name somewhere (not the layout's): the alias is ambiguous there;
        # leave it (a branch that still needs it gets an undeclared name, which --from settles by address)
        local = re.compile(r'^\s*#define\s+%s\b' % re.escape(old), re.M)
        where = [f for f in files if '/layout/' not in f.replace(os.sep, '/') and local.search(open(f).read())]
        if where:
            print('alias %s -> %s skipped: a local macro %s in %s' % (old, new, old, ', '.join(os.path.relpath(f, ROOT) for f in where)))
            continue
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
    # a name defined for two addresses is ambiguous: renaming by name would rewrite both meanings.
    # Separate them first (rename one inside the package that introduced it), then run again
    addrs_of = {}
    for addr, defs in by_addr.items():
        for d in defs:
            addrs_of.setdefault(d[3], set()).add(addr)
    clash = {n: a for n, a in addrs_of.items() if len(a) > 1}
    if clash:
        for n, a in sorted(clash.items()):
            print('one name for several addresses: %s (%s): separate them first' % (n, ', '.join('0x%08X' % x for x in sorted(a))))
        return 1
    for addr, defs in sorted(by_addr.items()):
        names = {d[3] for d in defs}
        if len(names) < 2:
            continue
        defs.sort()
        keep = defs[0][3]
        keep_path = defs[0][4]
        core_loser = next((d[4] for d in defs[1:] if '/core/state/layout/' in d[4].replace(os.sep, '/')), None)
        if not dry and core_loser and '/present/layout/' in keep_path.replace(os.sep, '/'):
            # core cannot include present/ headers: the kept name's definition moves to the core header
            move_define(keep, keep_path, core_loser)
            print('0x%08X: %s moved to %s (core uses the address)' % (addr, keep, os.path.relpath(core_loser, ROOT)))
        for _, _, _, name, path in defs[1:]:
            if name == keep:
                continue
            if not dry:
                kept = drop_define(open(path).read().splitlines(True), name)  # read before opening to write
                with open(path, 'w') as f:
                    f.write(''.join(kept))
            uses = rename_everywhere(name, keep, files, dry)
            if not dry:
                with open(os.path.join(ROOT, 'tools', 'naming', 'aliases.txt'), 'a') as f:
                    f.write('%s %s\n' % (name, keep))
            print('0x%08X: kept %s, removed %s (%s, %d uses renamed)'
                  % (addr, keep, name, os.path.relpath(path, ROOT), uses))
    return 0


if __name__ == '__main__':
    sys.exit(main())
