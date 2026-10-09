"""Imhotep - reach: the functions of Pharaoh.exe the game can reach that have no port (roadmap 8.1).

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tools/oracle/reach.py [--md docs/provenance/frontier.md] [--all]

Reachability over the Ghidra export's functions (local/ghidra-export/functions), from the entry points
(WinMain 0x00413ED0, the CRT entry 0x00562FEA and the static constructors of 0x00576000), along:
  - direct calls (the export's callees);
  - function addresses in code: every immediate or memory displacement of a reachable function's
    instructions that is a function's start (callbacks, window procedures, thunks);
  - function tables: aligned dwords in the data sections that are function starts, at most GAP bytes
    apart (arrays of pointers, arrays of structures holding one: window, button, building and figure
    tables); a table is reached when a reachable function's instructions name an address inside it or
    up to MARGIN bytes before it (a structure array's base), and then all its functions are.
This over-approximates (a table entry or callback may never be used) and misses addresses computed at
run time; the frontier is the reachable functions minus the ported ones (docs/provenance/ported.md).
Each frontier function may be classified in tools/oracle/frontier_notes.txt (`0xADDR CLASS reason`;
classes gap, platform, crt, unused, scope: local/oracle/agent_brief.md "Phase 8"); the ones still to
port are the gaps and the unclassified ones.
Prints a summary; --md writes the list with the class and the first path that reaches each function.
"""
import glob
import json
import os
import re
import struct
import sys

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM, X86_OP_MEM

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from disasm import image  # noqa: E402

EXPORT = os.path.join(ROOT, 'local', 'ghidra-export', 'functions')
ROOTS = [0x00413ED0, 0x00562FEA]
CTORS = (0x00576000, 0x0057607C)
# out of scope (docs/roadmap.md): the map editor's entry points; what only they reach is listed apart
OUT_OF_SCOPE = {0x00478440: 'map editor commands', 0x004795E0: 'map editor commands'}
CODE_END = 0x0056F000  # the first data section (.rdata: the import table at 0x0056F000)
GAP = 0x40  # the largest distance between two pointers of a table (a structure's size)
MARGIN = 0x40  # code naming an address this far before a table reaches it (a structure's base)


NOTES = os.path.join(ROOT, 'tools', 'oracle', 'frontier_notes.txt')
CLASSES = ('gap', 'platform', 'crt', 'unused', 'scope')


def notes():
    out = {}
    if os.path.exists(NOTES):
        for line in open(NOTES):
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            a, cls, *why = line.split(None, 2)
            if cls not in CLASSES:
                raise SystemExit('%s: unknown class %s for %s' % (NOTES, cls, a))
            out[int(a, 16)] = (cls, why[0] if why else '')
    return out


def load():
    funcs = {}
    for f in glob.glob(os.path.join(EXPORT, '*.json')):
        d = json.load(open(f))
        a = int(d['address'], 16) if isinstance(d['address'], str) else d['address']
        funcs[a] = {'name': d['name'], 'size': d.get('size', 0),
                    'callees': [int(c, 16) if isinstance(c, str) else c for c in d.get('callees', [])]}
    return funcs


def ported():
    text = open(os.path.join(ROOT, 'docs', 'provenance', 'ported.md')).read()
    return {int(m, 16) for m in re.findall(r'^\| (0x[0-9A-Fa-f]{8})', text, re.M)}


def code_refs(funcs, base, mem):
    """function -> the addresses its instructions name (immediates, displacements)"""
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    refs = {}
    for a, f in funcs.items():
        n = max(f['size'], 1)
        vals = set()
        for ins in md.disasm(mem[a - base:a - base + n], a):
            for op in ins.operands:
                if op.type == X86_OP_IMM:
                    vals.add(op.imm & 0xFFFFFFFF)
                elif op.type == X86_OP_MEM and op.mem.disp:
                    vals.add(op.mem.disp & 0xFFFFFFFF)
        refs[a] = vals
    return refs


def tables(funcs, base, mem, gap=GAP):
    """function starts stored in the data, grouped into tables: aligned dwords that are function starts,
    at most `gap` bytes apart (arrays of pointers, and arrays of structures holding a pointer):
    [(start, end, [funcs])]"""
    out = []
    start, last, run = None, None, []
    for off in range(CODE_END - base, len(mem) - 3, 4):
        v = struct.unpack_from('<I', mem, off)[0]
        if v not in funcs:
            continue
        a = base + off
        if start is not None and a - last > gap:
            out.append((start, last + 4, run))
            start, run = None, []
        if start is None:
            start = a
        last = a
        run.append(v)
    if start is not None:
        out.append((start, last + 4, run))
    return out


def reach(funcs, refs, tabs, roots):
    """reachable -> (parent, how) of the first path found (breadth first)"""
    starts = sorted(t[0] for t in tabs)
    import bisect

    def table_of(v):
        i = bisect.bisect_right(starts, v + MARGIN) - 1
        if i >= 0 and tabs[i][0] - MARGIN <= v < tabs[i][1]:
            return tabs[i]
        return None
    by_start = {t[0]: t for t in tabs}
    seen = {r: (None, 'entry') for r in roots if r in funcs}
    queue = list(seen)
    done_tables = set()
    while queue:
        nxt = []
        for a in queue:
            for c in funcs[a]['callees']:
                if c in funcs and c not in seen:
                    seen[c] = (a, 'call')
                    nxt.append(c)
            for v in refs.get(a, ()):
                if v in funcs and v not in seen:
                    seen[v] = (a, 'address')
                    nxt.append(v)
                t = table_of(v)
                if t and t[0] not in done_tables:
                    done_tables.add(t[0])
                    for c in by_start[t[0]][2]:
                        if c not in seen:
                            seen[c] = (a, 'table 0x%08X' % t[0])
                            nxt.append(c)
        queue = nxt
    return seen


def path(seen, a, limit=6):
    out = []
    while a is not None and len(out) < limit:
        parent, how = seen[a]
        out.append('0x%08X' % a + ('' if parent is None else ' (%s)' % how))
        a = parent
    return ' <- '.join(out)


def main():
    argv = sys.argv[1:]
    md_path = None
    if '--md' in argv:
        md_path = argv[argv.index('--md') + 1]
    funcs = load()
    base, mem = image()
    refs = code_refs(funcs, base, mem)
    tabs = tables(funcs, base, mem)
    roots = ROOTS + [struct.unpack_from('<I', mem, p - base)[0] for p in range(*CTORS, 4)]
    roots = [r for r in roots if r]
    done = ported()
    allr = reach(funcs, refs, tabs, roots)
    # what is reachable without the out-of-scope entry points: the rest is theirs only
    scoped = reach({a: f for a, f in funcs.items() if a not in OUT_OF_SCOPE},
                   {a: v - set(OUT_OF_SCOPE) for a, v in refs.items()},
                   [(s, e, [c for c in r if c not in OUT_OF_SCOPE]) for s, e, r in tabs], roots)
    front = sorted(a for a in scoped if a not in done)
    only_oos = sorted(a for a in allr if a not in scoped and a not in done)
    size = sum(funcs[a]['size'] for a in front)
    note = notes()
    todo = [a for a in front if note.get(a, ('gap',))[0] == 'gap']
    import collections
    by = collections.Counter(note[a][0] if a in note else 'unclassified' for a in front)
    print('reach: %d functions in the export, %d tables in the data; reachable %d (%d ported); '
          'frontier %d functions, %d bytes; out of scope only: %d'
          % (len(funcs), len(tabs), len(allr), len(set(allr) & done), len(front), size, len(only_oos)))
    print('reach: by class: %s; to port (gap + unclassified): %d functions, %d bytes'
          % (', '.join('%s %d' % kv for kv in sorted(by.items())), len(todo), sum(funcs[a]['size'] for a in todo)))
    if md_path:
        with open(md_path, 'w') as f:
            f.write('# Frontier: reachable functions without a port\n\n'
                    'Generated by `tools/oracle/reach.py --md` (roadmap 8.1); do not edit. Reachable from WinMain,\n'
                    'the CRT entry and the static constructors through calls, function addresses in code and\n'
                    'function tables in the data (an over-approximation), minus docs/provenance/ported.md.\n\n'
                    '%d functions, %d bytes of code. Out of scope (reached only through %s): %d functions.\n\n'
                    % (len(front), size, ', '.join('0x%08X %s' % kv for kv in OUT_OF_SCOPE.items()), len(only_oos)))
            f.write('By class (tools/oracle/frontier_notes.txt): %s. To port: %d functions.\n\n'
                    % (', '.join('%s %d' % kv for kv in sorted(by.items())), len(todo)))
            f.write('| Address | Size | Name | Class | First path (callee <- caller) |\n|---|---|---|---|---|\n')
            for a in front:
                cls, why = note.get(a, ('', ''))
                f.write('| 0x%08X | %d | %s | %s | %s |\n' % (a, funcs[a]['size'], funcs[a]['name'],
                                                          (cls + ': ' + why).replace('|', '/') if cls else '',
                                                          path(scoped, a)))
            f.write('\n## Out of scope\n\n| Address | Size | Name |\n|---|---|---|\n')
            for a in only_oos:
                f.write('| 0x%08X | %d | %s |\n' % (a, funcs[a]['size'], funcs[a]['name']))
    return 0


if __name__ == '__main__':
    sys.exit(main())
