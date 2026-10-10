#!/usr/bin/env python3
"""Imhotep - replace raw state addresses, record offsets and function addresses in core/ and present/
by the names of the layout headers (roadmap 10.3, docs/naming.md).

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  python3 tools/naming/rename.py [--dry] [--vars f=FIGURE,b=BUILDING] FILE...
                                                   rewrite the files, then list what is left raw;
                                                   --vars: parameters holding a record address
  python3 tools/naming/rename.py --check            names table checks (one name per address, ...)
  python3 tools/naming/rename.py --harvest [--write] candidate names from local #defines (harvest.py)

Names are read from the headers: globals `#define G_X 0x...u` (core/state/layout/g_*.h,
present/layout/g_*.h), record fields (core/state/layout/{building,city,figure,message}.h: BLD_,
CITY_, FIG_, MSG_), functions (core/state/functions.h: FN_). The rewrite is textual and only touches
code (not comments, strings or the port tables' address column):
  - an address literal with a global name or a function name becomes the name;
  - a local `#define NAME 0x...` whose value has a global name is removed, NAME becomes the global;
  - `REC_ADDR(...) + off` and `var + off`, where var holds REC_ADDR(...) / CURRENT_CITY(st) in the same
    function (all its assignments of one record kind), become `... + FIELD` when exactly one field
    name has that offset (an offset with several names is left for review: each is a meaning).
Whether the result compiles to the same code is for tools/check_same_code.py; whether a name fits
its use is for review.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
LAYOUT_DIRS = [os.path.join(ROOT, 'core', 'state', 'layout'), os.path.join(ROOT, 'core', 'state', 'layout', 'pending'),
               os.path.join(ROOT, 'present', 'layout')]
RECORDS = {  # header -> (field prefix, address expressions of a record)
    'building.h': ('BLD_', [r'BUILDING_ADDR\(']),
    'city.h': ('CITY_', [r'CURRENT_CITY\(st\)', r'CITY_ADDR\(', r'\bCITY\(st\)']),
    'figure.h': ('FIG_', [r'FIGURE_ADDR\(']),
    'message.h': ('MSG_', [r'MESSAGE_ADDR\(']),
}
NOT_FIELD = re.compile(r'_(STRIDE|REC_SIZE|DATA_SIZE|TEXT_SIZE|COUNT|BASE)$')
DEFINE = re.compile(r'^[ \t]*#define\s+([A-Za-z_]\w*)\s+(0x[0-9A-Fa-f]+)u?\b', re.M)
HEX = re.compile(r'\b0x([0-9A-Fa-f]+)[uU]?\b')
STATE_BASE = 0x0056F000
CODE_LOW = 0x00401000


def code_mask(s):
    """s with comments and string / char literals blanked (same length, newlines kept)"""
    out, i, n = [], 0, len(s)
    while i < n:
        if s.startswith('/*', i):
            j = s.find('*/', i + 2)
            j = n if j < 0 else j + 2
            out.append(re.sub(r'[^\n]', ' ', s[i:j]))
            i = j
        elif s.startswith('//', i):
            j = s.find('\n', i)
            j = n if j < 0 else j
            out.append(' ' * (j - i))
            i = j
        elif s[i] in '"\'':
            q, j = s[i], i + 1
            while j < n and s[j] != q:
                j += 2 if s[j] == '\\' else 1
            out.append(q + ' ' * (j - i - 1) + q)
            i = j + 1
        else:
            out.append(s[i])
            i += 1
    return ''.join(out)


def load_names():
    glob, fields, fns, where = {}, {}, {}, {}
    problems = []
    for d in LAYOUT_DIRS:
        if not os.path.isdir(d):
            continue
        for f in sorted(os.listdir(d)):
            if not f.endswith('.h'):
                continue
            text = open(os.path.join(d, f)).read()
            for m in DEFINE.finditer(code_mask(text)):
                name, v = m.group(1), int(m.group(2), 16)
                pending = os.path.basename(d) == 'pending'
                if (f.startswith('g_') or pending) and name.startswith('G_') and v >= STATE_BASE:
                    if v in glob and glob[v] != name:
                        problems.append('two names for 0x%08X: %s, %s' % (v, glob[v], name))
                    if name in where and where[name][0] != v:
                        problems.append('one name for two addresses: %s (0x%08X in %s, 0x%08X in %s)'
                                        % (name, where[name][0], where[name][1], v, f))
                    where.setdefault(name, (v, f))
                    glob.setdefault(v, name)
                elif f in RECORDS and name.startswith(RECORDS[f][0]) and not NOT_FIELD.search(name):
                    fields.setdefault(f, {}).setdefault(v, []).append(name)
                elif pending and not NOT_FIELD.search(name):  # a field named in a pending file
                    for rf, (prefix, _) in RECORDS.items():
                        if name.startswith(prefix):
                            fields.setdefault(rf, {}).setdefault(v, []).append(name)
    fh = os.path.join(ROOT, 'core', 'state', 'functions.h')
    if os.path.exists(fh):
        for m in DEFINE.finditer(open(fh).read()):
            fns[int(m.group(2), 16)] = m.group(1)
    return glob, fields, fns, problems


def functions(mask):
    """(start, end) of each top-level brace block"""
    spans, depth, start = [], 0, 0
    for i, c in enumerate(mask):
        if c == '{':
            if depth == 0:
                start = i
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                spans.append((start, i + 1))
    return spans


def balanced_end(s, i):
    """index after the parenthesis group opening at s[i] == '('"""
    depth = 0
    for j in range(i, len(s)):
        if s[j] == '(':
            depth += 1
        elif s[j] == ')':
            depth -= 1
            if depth == 0:
                return j + 1
    return -1


KIND_HEADER = {'BUILDING': 'building.h', 'FIGURE': 'figure.h', 'CITY': 'city.h', 'MESSAGE': 'message.h'}


def rewrite(path, glob, fields, fns, dry, given=None):
    """given: {variable name: record kind} for variables the code does not assign (parameters),
    from --vars (the caller has checked them)"""
    given = given or {}
    s = open(path).read()
    edits = []  # (start, end, text)
    mask = code_mask(s)
    local = {}
    # local defines that duplicate a global: removed, their uses renamed
    for m in DEFINE.finditer(mask):
        v = int(m.group(2), 16)
        if v in glob and m.group(1) != glob[v]:
            local[m.group(1)] = glob[v]
            ls = s.rfind('\n', 0, m.start()) + 1
            le = s.find('\n', m.end())
            le = len(s) if le < 0 else le + 1
            if mask[ls:le].strip().startswith('#define ' + m.group(1)):
                edits.append((ls, le, ''))
    removed = [(a, b) for a, b, t in edits]

    def inside_removed(i):
        return any(a <= i < b for a, b in removed)

    table_rows = [m.span(1) for m in re.finditer(r'\{\s*(0x[0-9A-Fa-f]{8}u?),\s*"', mask)]
    for m in HEX.finditer(mask):
        if inside_removed(m.start()) or any(a == m.start() for a, b in table_rows):
            continue
        v = int(m.group(1), 16)
        if len(m.group(1)) < 6:
            continue
        name = glob.get(v) if v >= STATE_BASE else fns.get(v) if v >= CODE_LOW else None
        if name:
            # not the value of a #define that names it (the layout headers themselves)
            ls = mask.rfind('\n', 0, m.start()) + 1
            if re.match(r'\s*#define\s+' + name + r'\b', mask[ls:]):
                continue
            edits.append((m.start(), m.end(), name))
    for old, new in local.items():
        for m in re.finditer(r'\b%s\b' % re.escape(old), mask):
            if not inside_removed(m.start()):
                edits.append((m.start(), m.end(), new))
    # record fields
    unresolved = []
    # local macros that are a record address: #define B(id) BUILDING_ADDR(id)
    local_rec = {}
    for m in re.finditer(r'^[ \t]*#define[ \t]+(\w+)\(([\w, ]*)\)[ \t]+\(?\s*(\w+)\((.*)\)\s*\)?[ \t]*(?:/\*.*\*/)?$',
                         mask, re.M):
        for f, (prefix, exprs) in RECORDS.items():
            if any(re.fullmatch(e, m.group(3) + '(') for e in exprs) and m.group(4).count('(') == m.group(4).count(')'):
                local_rec.setdefault(f, []).append(r'\b%s\(' % m.group(1))
    for f, (prefix, exprs) in RECORDS.items():
        table = fields.get(f, {})
        if not table:
            continue
        exprs = exprs + local_rec.get(f, [])
        rec = '(?:%s)' % '|'.join(exprs)
        for fs, fe in functions(mask):
            body = mask[fs:fe]
            # header of the function too (parameters are not tracked: review)
            kinds = {}
            for m in re.finditer(r'\b(\w+)\s*(?<![=!<>+\-*/&|^%])=(?!=)\s*([^;]*)', body):
                kinds.setdefault(m.group(1), []).append(bool(re.match(rec, m.group(2).strip())))
            vars_ = [v for v, k in kinds.items() if all(k)]
            vars_ += [v for v, kind in given.items() if KIND_HEADER.get(kind) == f and v not in kinds
                      and re.search(r'\b%s\b' % re.escape(v), body)]
            pats = [r'\b%s\b' % re.escape(v) for v in vars_]
            for m in re.finditer(rec, body):
                st = fs + m.start()
                if mask[fs + m.end() - 1] == '(':
                    e = balanced_end(mask, fs + m.end() - 1)
                else:
                    e = fs + m.end()
                pats.append(re.escape(mask[st:e]))
            if not pats:
                continue
            zero = table.get(0, [])
            for p in set(pats):
                if len(zero) == 1:  # the field at offset 0, accessed without an offset
                    for m in re.finditer(r'\b[SUP](?:8|16|32)\(st, (%s)\)' % p, mask[fs:fe]):
                        edits.append((fs + m.end(1), fs + m.end(1), ' + ' + zero[0]))
                for m in re.finditer(r'(%s)\s*\+\s*(0x[0-9A-Fa-f]+|\d+)[uU]?\b' % p, mask[fs:fe]):
                    a, b = fs + m.start(2), fs + m.end()
                    if mask[b:b + 1] == '*' or re.match(r'\s*\*', mask[b:]):
                        continue  # b + 4 * i: not an offset
                    off = int(m.group(2), 0)
                    names = table.get(off, [])
                    if len(names) == 1:
                        edits.append((a, b, names[0]))
                    else:
                        line = s.count('\n', 0, a) + 1
                        unresolved.append('%s:%d: %s + 0x%X: %s' % (
                            os.path.relpath(path, ROOT), line, f[:-2], off,
                            'several names: ' + ', '.join(names) if names else 'no name'))
    # apply (no overlaps: keep the first edit at a position)
    edits.sort(key=lambda e: (e[0], -e[1]))
    out, pos, last_end = [], 0, -1
    for a, b, t in edits:
        if a < last_end:
            continue
        out.append(s[pos:a])
        out.append(t)
        pos = b
        last_end = b
    out.append(s[pos:])
    new = ''.join(out)
    if not dry and new != s:
        inc = 'present_layout.h' if os.path.relpath(path, ROOT).startswith('present') else 'state/layout.h'
        if '"%s"' % inc not in new:
            new = add_include(new, inc)
        open(path, 'w').write(new)
    return new, unresolved


def add_include(s, inc='state/layout.h'):
    """#include inc in its alphabetical place in the last block of quoted includes"""
    incs = list(re.finditer(r'^#include "([^"]+)"\n', s, re.M))
    if not incs:
        return s
    block = [incs[-1]]
    for m in reversed(incs[:-1]):
        if m.end() != block[0].start():
            break
        block.insert(0, m)
    for m in block:
        if m.group(1) > inc:
            return s[:m.start()] + '#include "%s"\n' % inc + s[m.start():]
    return s[:block[-1].end()] + '#include "%s"\n' % inc + s[block[-1].end():]


def leftovers(path, text):
    mask = code_mask(text)
    rows = [m.start(1) for m in re.finditer(r'\{\s*(0x[0-9A-Fa-f]{8}u?),\s*"', mask)]
    out = []
    for m in HEX.finditer(mask):
        v = int(m.group(1), 16)
        if len(m.group(1)) >= 6 and v >= CODE_LOW and m.start() not in rows:
            line = text.count('\n', 0, m.start()) + 1
            out.append('%s:%d: raw 0x%08X' % (os.path.relpath(path, ROOT), line, v))
    return out


def main():
    args = sys.argv[1:]
    glob, fields, fns, problems = load_names()
    if args and args[0] == '--harvest':
        import harvest
        return harvest.main('--write' in args)
    if args and args[0] == '--check':
        for p in problems:
            print(p)
        print('%d globals, %d record fields, %d functions' % (
            len(glob), sum(len(t) for t in fields.values()), len(fns)))
        return 1 if problems else 0
    dry = '--dry' in args
    given = {}
    if '--vars' in args:  # --vars f=FIGURE,b=BUILDING
        i = args.index('--vars')
        for kv in args[i + 1].split(','):
            k, v = kv.split('=')
            given[k] = v.upper()
        del args[i:i + 2]
    args = [a for a in args if a != '--dry']
    for p in problems:
        print('names: ' + p)
    for path in args:
        new, unresolved = rewrite(os.path.abspath(path), glob, fields, fns, dry, given)
        for u in unresolved:
            print(u)
        for u in leftovers(os.path.abspath(path), new):
            print(u)
    return 0


if __name__ == '__main__':
    sys.exit(main())
