#!/usr/bin/env python3
"""Imhotep - check that the working tree compiles core/ and present/ to the same code as a git revision
(roadmap 10.1: a rename that leaves the object code identical leaves the behaviour identical).

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  python3 tools/check_same_code.py [BASE]      (default BASE: HEAD)

Builds the libraries' objects of BASE (in a temporary git worktree) and of the working tree with the
Makefile's own rules and flags, into build-same-base/ and build-same-tree/, and compares every object
of core/ and present/: the contents of all sections and the relocations (llvm-objdump -s -r), so code,
data and constants and what each reference points at. Symbol tables are not compared: a static
function or variable renamed keeps its code. Objects present on one side only are listed (a file
added, removed or split is not a rename: check it with the oracle suites).
Exit status: 0 if every object is the same, 1 otherwise.
"""
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DIRS = ('core', 'present')


def objdump():
    for cand in (['xcrun', 'llvm-objdump'], ['llvm-objdump'], ['objdump']):
        try:
            subprocess.run(cand + ['--version'], capture_output=True, check=True)
            return cand
        except (OSError, subprocess.CalledProcessError):
            continue
    sys.exit('check_same_code: no objdump found')


def build(tree, out):
    """the objects of core/ and present/ built by the Makefile in tree, into out (absolute)"""
    if os.path.exists(out):
        shutil.rmtree(out)
    r = subprocess.run(['make', '-s', '-j4', 'BUILD=' + out, out + '/libimhotep_core.a', out + '/libimhotep_present.a'],
                       cwd=tree, capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit('check_same_code: build failed in %s:\n%s' % (tree, (r.stdout + r.stderr)[-2000:]))
    objs = {}
    for d in DIRS:
        for dirpath, _, files in os.walk(os.path.join(out, d)):
            for f in files:
                if f.endswith('.o'):
                    p = os.path.join(dirpath, f)
                    objs[os.path.relpath(p, out)] = p
    return objs


def contents(tool, path):
    r = subprocess.run(tool + ['-s', '-r', path], capture_output=True, text=True, check=True)
    # the first lines name the file; keep the sections and relocations only
    return [line for line in r.stdout.splitlines() if line.strip() and not line.startswith(path)
            and 'file format' not in line]


def main():
    base = sys.argv[1] if len(sys.argv) > 1 else 'HEAD'
    tool = objdump()
    tmp = tempfile.mkdtemp(prefix='same-code-')
    wt = os.path.join(tmp, 'base')
    try:
        subprocess.run(['git', 'worktree', 'add', '--detach', '-q', wt, base], cwd=ROOT, check=True)
        a = build(wt, os.path.join(ROOT, 'build-same-base'))
        b = build(ROOT, os.path.join(ROOT, 'build-same-tree'))
    finally:
        subprocess.run(['git', 'worktree', 'remove', '--force', wt], cwd=ROOT, capture_output=True)
        shutil.rmtree(tmp, ignore_errors=True)
    differ = [o for o in sorted(set(a) & set(b)) if contents(tool, a[o]) != contents(tool, b[o])]
    only_a = sorted(set(a) - set(b))
    only_b = sorted(set(b) - set(a))
    for o in differ:
        print('  different code: %s' % o)
    for o in only_a:
        print('  only in %s: %s' % (base, o))
    for o in only_b:
        print('  only in the working tree: %s' % o)
    same = len(set(a) & set(b)) - len(differ)
    print('check_same_code: %d / %d objects of core/ and present/ the same as %s%s'
          % (same, len(set(a) | set(b)), base,
             '' if not (only_a or only_b) else ' (%d only on one side)' % (len(only_a) + len(only_b))))
    return 0 if not differ and not only_a and not only_b else 1


if __name__ == '__main__':
    sys.exit(main())
