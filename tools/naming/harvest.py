#!/usr/bin/env python3
"""Imhotep - candidate names for the state's globals from the ports' own local #defines (roadmap 10.3,
docs/naming.md "Harvested names").

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  python3 tools/naming/rename.py --harvest            write tools/naming/candidates.tsv
  python3 tools/naming/rename.py --harvest --write    also seed the area headers (wave 0, once)

Every `#define NAME 0x00XXXXXX` of core/ and present/ (one literal, an address of the data segments)
is a name a port gave that address, with its comment as evidence. Per address:
  - names already in the layout headers win (status `named`);
  - addresses inside a record array (buildings, figures, cities, messages) are not globals: listed
    with the record expression to write instead (status `record`);
  - else the local names are compared after dropping each file's own prefix (a token most of the
    file's names start with): one name left (`single`), or one name defined in more files than any
    other (`majority`); a tie stays unresolved with a neutral name G_U<address> (`unresolved`);
  - a generic name (TABLE, BASE, ...) or one of three letters or fewer is qualified by the defining
    file's name (`generic`); two addresses with the same name: the less used one is qualified
    (`clash`).
The address goes to the area header of the package that uses it most (core before present: a core
global must be visible to core). Status `review` marks names that need a package agent's look.
"""
import collections
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from rename import ROOT, code_mask, load_names  # noqa: E402

STATE_BASE, STATE_END = 0x0056F000, 0x01267AA8
DEF = re.compile(r'^[ \t]*#define[ \t]+([A-Za-z_]\w*)[ \t]+\(?(0x[0-9A-Fa-f]{6,8})[uU]?\)?[ \t]*'
                 r'(?:/\*(.*?)\*/)?[ \t]*$', re.M)
HEX = re.compile(r'\b0x([0-9A-Fa-f]{6,8})[uU]?\b')
SKIP_FILES = {'core/state/state.h', 'core/state/records.h', 'core/state/functions.h'}

# package -> (directories, area header). "present/ui" alone: the files directly in it.
CL = 'core/state/layout/'
PL = 'present/layout/'
AREAS = [
    ('P1', ['core/figure'], CL + 'g_figure.h'),
    ('P2', ['core/building'], CL + 'g_building.h'),
    ('P3', ['core/city'], CL + 'g_city.h'),
    ('P4', ['core/command'], CL + 'g_command.h'),
    ('P5', ['core/map'], CL + 'g_map.h'),
    ('P5', ['core/sim', 'core/state', 'core/random', 'core/model'], CL + 'g_sim.h'),
    ('P5', ['core/graphics'], CL + 'g_graphics.h'),
    ('P6', ['core/boot'], CL + 'g_boot.h'),
    ('P6', ['core/io', 'core/newgame'], CL + 'g_scenario.h'),
    ('P7', ['core/frame'], CL + 'g_frame.h'),
    ('P7', ['present/play', 'present/boot'], PL + 'g_play.h'),
    ('P8a', ['present/ui', 'present/ui/messages', 'present/ui/empire'], PL + 'g_ui_main.h'),
    ('P8b', ['present/ui/info'], PL + 'g_ui_info.h'),
    ('P8c', ['present/ui/advisors', 'present/ui/frontend'], PL + 'g_ui_advisors.h'),
    ('P9', ['present/city', 'present/sidebar', 'present/draw'], PL + 'g_city_view.h'),
]
RECORD_ARRAYS = [  # name, base, record size, count, field prefix
    ('BUILDING', 0x007999D8, 0x108, 4000, 'BLD_'),
    ('FIGURE', 0x00BCD5F8, 0x184, 2000, 'FIG_'),
    ('CITY', 0x00EA1870, 0x49D8, 2, 'CITY_'),
    ('MESSAGE', 0x00E95228, 0x30, 1000, 'MSG_'),
]
GENERIC = {'TABLE', 'BASE', 'COUNT', 'DATA', 'LIST', 'LIST_COUNT', 'BUF', 'BUFFER', 'PTR', 'FLAG',
           'FLAGS', 'STATE', 'MODE', 'INDEX', 'VALUE', 'TEMP', 'TMP', 'QUEUE', 'START', 'END', 'HEAD',
           'TAIL', 'SIZE', 'RESULT', 'NAME', 'NAMES', 'ITEMS', 'ENTRIES', 'SLOTS', 'BACK', 'CUR', 'OUT',
           'TOTAL', 'NUM', 'LIMIT', 'ARGS', 'PARAM', 'PARAMS', 'OBJ', 'OBJECT', 'THIS', 'SELF', 'GRID',
           'BITS', 'TEXT', 'STR', 'STRING', 'POS', 'X', 'Y', 'W', 'H', 'ID', 'KIND', 'TYPE', 'TYPES',
           'COST', 'PERIOD', 'IMAGES', 'GROUPS', 'BLOCK', 'RECORD', 'RECORDS', 'ROWS', 'COLS', 'MAX',
           'MIN', 'OFFSET', 'OFFSETS', 'POINTER', 'COUNTER', 'TIMER', 'SPEED', 'LEVEL', 'TARGET'}

# ties settled by reading the comments of the defining ports (wave 0); the others stay neutral
RESOLVED = {
    0x0057BE14: 'MENU_LABELS', 0x005BE9B8: 'PART_DESC_SETS', 0x005CC8C0: 'FOOTPRINT_OFFSETS',
    0x005CE3BC: 'MENU_TYPES_1', 0x005D048C: 'DIE_FRAMES', 0x005D19C8: 'MONUMENT_MODELS',
    0x005D2A30: 'ROUTE_SELECTED', 0x005E047C: 'GLYPH_MAP', 0x005E049C: 'GLYPH_DRAW_MAP',
    0x006059E4: 'CONSTRUCTION_DRAG', 0x006FDC88: 'ADVISOR_AVAILABLE', 0x007027D4: 'GOAL_SEPARATOR',
    0x00704DB0: 'SCENARIO_LIST_MODE', 0x00704DBC: 'SCENARIO_LIST_ROW', 0x007056C0: 'HOVER_3',
    0x007056C4: 'HOVER_2', 0x0070CDCC: 'HOUSE_LEVELS', 0x0077646C: 'TRADE_ROUTE_LIMITS',
    0x0077F748: 'EMPIRE_OBJECTS', 0x00784DE0: 'MONUMENT_SLOTS', 0x00784F18: 'EMPIRE_SELECTED',
    0x0078D83C: 'HOUSE_LIST_COUNT', 0x0078D840: 'HOUSE_LIST_INDEX', 0x0078E178: 'HOUSE_LIST',
    0x007900C2: 'STORAGE_ORDERS', 0x007900EC: 'STORAGE_FILL', 0x00BC96F8: 'BRIDGE_LENGTH',
    0x00BC9714: 'BUILD_ORIENTATION', 0x00BC9738: 'FOOTPRINT_ROWS', 0x00BC973C: 'FOOTPRINT_COLS',
    0x00BCA300: 'HOVER_TILE', 0x00BCD5EC: 'BEST_DISTANCE',
    0x00D3B134: 'MESSAGE_WINDOW_SCROLL_MAX', 0x00D3B138: 'TEXT_LINES', 0x00D3B13C: 'MESSAGE_WINDOW_VISIBLE',
    0x00D3B148: 'MESSAGE_WINDOW_BOX_H', 0x00D3B14C: 'MESSAGE_WINDOW_BOX_W', 0x00D3B150: 'MESSAGE_WINDOW_BOX_Y',
    0x00D3B154: 'MESSAGE_WINDOW_BOX_X', 0x00D3B15C: 'MESSAGE_WINDOW_Y', 0x00D3B160: 'MESSAGE_WINDOW_X',
    0x00D3B164: 'MESSAGE_WINDOW_FONT_LINK', 0x00D3B168: 'MESSAGE_WINDOW_FONT_TEXT',
    0x00D3B180: 'MESSAGE_WINDOW_VIDEO', 0x00D3B186: 'MESSAGE_WINDOW_REPLIES', 0x00D3B439: 'CONFIRM_ANSWER',
    0x00D3B5DC: 'BLIT_Y', 0x00D3B5E0: 'BLIT_X', 0x00E207EC: 'CARET_POS', 0x00E20990: 'CARET_FIELD',
    0x00E34238: 'NUMBER_TEXT', 0x00E38D94: 'SCREEN_PITCH', 0x00E38E63: 'SOUND_EFFECTS_ON',
    0x00E38E64: 'MUSIC_ON', 0x00E38EC0: 'CITY_SOUNDS_ON', 0x00E38EC4: 'EFFECTS_VOLUME',
    0x00E38EC8: 'MUSIC_VOLUME', 0x00E38ECC: 'SPEECH_VOLUME', 0x00E38ED0: 'CITY_VOLUME',
    0x00E39140: 'HWND', 0x00E39148: 'DDRAW', 0x00E39170: 'DD_CLIPPER', 0x00E39174: 'DD_BACK',
    0x00E39178: 'DD_PRIMARY', 0x00E92E18: 'BACK_BUFFER', 0x00E92E50: 'FILE_SCHEMA',
    0x00E945C4: 'USE_FILE_SCHEMA', 0x00EADCC4: 'INFO_WARSHIP', 0x00EADCC8: 'INFO_TRANSPORT',
    0x00EADCCC: 'INFO_FORMATION', 0x00EADD6C: 'INFO_BUILDING', 0x00EADFC8: 'FAMILY_SCROLL',
    0x00ED4140: 'GRID_ROUTING_WATER', 0x00F53F08: 'ADVISOR_Y', 0x00F53F0C: 'ADVISOR_X',
    0x00F53F48: 'ADVISOR_SLIDE', 0x00F55C5C: 'GOD_IMAGES', 0x010D6A00: 'IMAGE_ENTRIES',
}


def area_of(path):
    d = os.path.dirname(path)
    best = None
    for pkg, dirs, header in AREAS:
        for x in dirs:
            if (x == 'present/ui' and d == x) or (x != 'present/ui' and (d == x or d.startswith(x + '/'))):
                if best is None or len(x) > len(best[0]):
                    best = (x, pkg, header)
    return (best[1], best[2]) if best else (None, None)


def scan():
    defs = collections.defaultdict(list)    # address -> [(name, path, comment)]
    uses = collections.defaultdict(collections.Counter)  # address -> header -> uses
    file_names = {}
    for top in ('core', 'present'):
        for d, _, files in os.walk(os.path.join(ROOT, top)):
            rel = os.path.relpath(d, ROOT)
            if rel.startswith(CL.rstrip('/')) or rel.startswith(PL.rstrip('/')):
                continue
            for f in sorted(files):
                path = os.path.join(rel, f)
                if not f.endswith(('.c', '.h')) or path in SKIP_FILES:
                    continue
                s = open(os.path.join(ROOT, path)).read()
                mask = code_mask(s)
                pkg, header = area_of(path)
                local = {}
                for m in DEF.finditer(s):
                    v = int(m.group(2), 16)
                    if STATE_BASE <= v < STATE_END:
                        defs[v].append((m.group(1), path, (m.group(3) or '').strip()))
                        local[m.group(1)] = v
                file_names[path] = list(local)
                for line in mask.split('\n'):
                    if re.search(r'\{\s*0x[0-9A-Fa-f]{8}u?,\s*"', line) or re.match(r'\s*#define', line):
                        continue
                    for m in HEX.finditer(line):
                        v = int(m.group(1), 16)
                        if STATE_BASE <= v < STATE_END and header:
                            uses[v][header] += 1
                for name, v in local.items():
                    n = len(re.findall(r'\b%s\b' % name, mask)) - 1
                    if header and n > 0:
                        uses[v][header] += n
    return defs, uses, file_names


def file_prefix(names):
    toks = collections.Counter(n.split('_')[0] for n in names if '_' in n)
    if len(names) >= 3 and toks:
        t, c = toks.most_common(1)[0]
        if c >= 0.8 * len(names):
            return t + '_'
    return ''


def stem(path):
    return re.sub(r'[^A-Za-z0-9]', '_', os.path.splitext(os.path.basename(path))[0]).upper()


def record_of(v):
    for name, base, size, count, prefix in RECORD_ARRAYS:
        if base <= v < base + size * count:
            k, off = divmod(v - base, size)
            return '%s_ADDR(%d) + 0x%X' % (name, k, off) if name != 'CITY' else 'CITY_ADDR(%d) + 0x%X' % (k, off)
    return None


def main(write):
    glob, _, _, _ = load_names()
    defs, uses, file_names = scan()
    prefixes = {p: file_prefix(n) for p, n in file_names.items()}
    rows = []
    for v in sorted(set(defs) | set()):
        cands = defs[v]
        hdr_uses = uses.get(v, collections.Counter())
        core_h = [(c, h) for h, c in hdr_uses.items() if h.startswith('core/')]
        pick = core_h or [(c, h) for h, c in hdr_uses.items()]
        if not pick:  # only defined: the defining files' area
            pick = [(1, area_of(p)[1]) for _, p, _ in cands if area_of(p)[1]]
        pick.sort(key=lambda x: (-x[0], [a[2] for a in AREAS].index(x[1])))
        header = pick[0][1] if pick else CL + 'g_sim.h'
        total = sum(hdr_uses.values())
        norm = collections.defaultdict(set)
        for n, p, _ in cands:
            pre = prefixes.get(p, '')
            k = n[len(pre):] if pre and n.startswith(pre) and len(n) > len(pre) + 1 else n
            if k.startswith('G_') and len(k) > 4:
                k = k[2:]
            norm[k].add(p)
        ranked = sorted(norm.items(), key=lambda x: (-len(x[1]), x[0]))
        alts = ', '.join('%s:%d' % (k, len(ps)) for k, ps in ranked)
        evidence = next(('%s: %s' % (p, c) for n, p, c in cands if c), cands[0][1])
        rec = record_of(v)
        if v in glob:
            status, name = 'named', glob[v]
        elif rec:
            status, name = 'record', rec
        elif len(ranked) == 1:
            status, name = 'single', ranked[0][0]
        elif len(ranked[0][1]) > len(ranked[1][1]):
            status, name = 'majority', ranked[0][0]
        elif v in RESOLVED:
            status, name = 'resolved', RESOLVED[v]
        else:
            status, name = 'unresolved', 'U%06X' % (v & 0xFFFFFF)
        if status in ('single', 'majority'):
            if name in GENERIC or len(name) <= 3:
                p = sorted(ranked[0][1])[0]
                name = '%s_%s' % (stem(p), name)
                status = 'generic'
            name = 'G_' + name
        elif status in ('unresolved', 'resolved'):
            name = 'G_' + name
        rows.append({'addr': v, 'name': name, 'header': header, 'status': status, 'uses': total,
                     'alts': alts, 'evidence': evidence, 'files': sorted(set(p for _, p, _ in cands))})
    # clashes: one name for two addresses (among the new names and the layout's)
    taken = {n: a for a, n in glob.items()}
    by_name = collections.defaultdict(list)
    for r in rows:
        if r['status'] not in ('named', 'record'):
            by_name[r['name']].append(r)
    for n, rs in by_name.items():
        rs.sort(key=lambda r: -r['uses'])
        keep = 0 if n not in taken else None
        for i, r in enumerate(rs):
            if i == keep:
                continue
            q = 'G_%s_%s' % (stem(r['files'][0]), n[2:])
            if q in by_name or q in taken or any(x['name'] == q for x in rows):
                q = '%s_%06X' % (n, r['addr'] & 0xFFFFFF)
            r['name'], r['status'] = q, r['status'] + '+clash'
    out = os.path.join(ROOT, 'tools', 'naming', 'candidates.tsv')
    with open(out, 'w') as f:
        f.write('# address\tname\theader\tstatus\tuses\tlocal names (defining files)\tevidence\n')
        for r in rows:
            f.write('0x%08X\t%s\t%s\t%s\t%d\t%s\t%s\n' % (r['addr'], r['name'], r['header'], r['status'],
                                                    r['uses'], r['alts'], r['evidence'].replace('\t', ' ')))
    counts = collections.Counter(r['status'] for r in rows)
    print('%d addresses: %s' % (len(rows), ', '.join('%s %d' % kv for kv in sorted(counts.items()))))
    if write:
        seed(rows)
    return 0


BEGIN = '/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).\n' \
        ' * @bin evidence: the defining port (file) and its comment. A package agent checks each name it\n' \
        ' * uses, and moves it above this block once checked. --- */\n'
END = '/* --- end harvested --- */\n'


def seed(rows):
    per = collections.defaultdict(list)
    for r in rows:
        if r['status'] not in ('named', 'record'):
            per[r['header']].append(r)
    for _, _, header in AREAS:
        path = os.path.join(ROOT, header)
        if os.path.exists(path):
            s = open(path).read()
        else:
            g = os.path.splitext(os.path.basename(header))[0]
            guard = 'IMHOTEP_%s_%s_H' % ('STATE_LAYOUT' if header.startswith('core') else 'PRESENT_LAYOUT',
                                        g.upper())
            s = ('/* Imhotep - named globals: %s.\n * Copyright (C) 2026 Imhotep contributors. Licensed under '
                 'the GNU AGPL-3.0.\n * Conventions: docs/naming.md (owner: package %s). */\n#ifndef %s\n#define %s\n\n'
                 '#endif\n') % (header, [a[0] for a in AREAS if a[2] == header][0], guard, guard)
        if BEGIN in s:
            sys.exit('harvest: %s already seeded' % header)
        lines = [BEGIN]
        for r in sorted(per.get(header, []), key=lambda r: r['addr']):
            note = r['evidence'].replace('*/', '* /')
            if len(note) > 140:
                note = note[:137] + '...'
            tag = '' if r['status'] in ('single', 'majority', 'resolved') else ' [%s]' % r['status']
            lines.append('#define %s 0x%08Xu /* %s%s */\n' % (r['name'], r['addr'], note, tag))
        lines.append(END)
        i = s.rindex('#endif')
        s = s[:i] + ''.join(lines) + '\n' + s[i:]
        open(path, 'w').write(s)
    print('seeded %d headers' % len(AREAS))


if __name__ == '__main__':
    sys.exit(main('--write' in sys.argv))
