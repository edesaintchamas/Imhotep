"""Imhotep - compare two game states field by field.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  python3 tools/oracle/diff.py A B [--max N]

A and B are saved games (read with `build/save_dump --stored --image`, i.e. the version-160 layout
as stored) or memory images written by `save_dump --image`. Every one of the 181 saved variables
is compared. Differences are named after docs/formats/save.md: the chunk's role (§3), and for the
record chunks (buildings, figures, city data, §5) the record index and the field at that offset.

Also importable: `diff_images(a, b)` returns the differences as tuples for other tools.
"""
import os
import re
import struct
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SAVE_MD = os.path.join(ROOT, 'docs', 'formats', 'save.md')

# record chunks: chunk -> (record size, record noun, §5 heading prefix)
RECORDS = {32: (264, 'building', '### Buildings'), 21: (388, 'figure', '### Figures'),
           28: (18904, 'city', '### City data')}


def _doc():
    text = open(SAVE_MD).read()
    roles = {}
    for m in re.finditer(r'^\| (\d+) \| 0x[0-9A-F]{8} \| [^|]+\| [^|]+\| ([^|]*)\|', text, re.M):
        role = re.sub(r'\*\*@bin[^*]*\*\*', '', m.group(2)).strip(' ,')
        roles[int(m.group(1))] = role[:60]
    fields = {}
    for chunk, (_, _, heading) in RECORDS.items():
        start = text.find(heading)
        end = text.find('\n### ', start + 1)
        table = {}
        for m in re.finditer(r'^\| \+0x([0-9A-F]+)(?:[–-]0x([0-9A-F]+))?(?:, \+0x[0-9A-F]+)* \| ([^|]+) \| ([^|]+) \|',
                             text[start:end], re.M):
            lo = int(m.group(1), 16)
            hi = int(m.group(2), 16) if m.group(2) else lo
            name = '%s %s' % (m.group(4).strip(), m.group(3).strip())
            for o in range(lo, hi + 1):
                table.setdefault(o, (lo, name[:50]))
        fields[chunk] = table
    return roles, fields


def read_image(path):
    """{chunk: (address, bytes)} from a save (stored layout) or a save_dump --image file."""
    data = open(path, 'rb').read()
    if not path.endswith('.img'):
        with tempfile.TemporaryDirectory() as tmp:
            img = os.path.join(tmp, 'state.img')
            r = subprocess.run([os.path.join(ROOT, os.environ.get('IMHOTEP_BUILD', 'build'), 'save_dump'), path, '--stored', '--image', img],
                               capture_output=True, text=True)
            if r.returncode != 0:
                raise RuntimeError('save_dump %s: %s' % (path, r.stderr.strip()))
            data = open(img, 'rb').read()
    out, o, i = {}, 0, 0
    while o < len(data):
        addr, n = struct.unpack_from('<II', data, o)
        out[i] = (addr, data[o + 8:o + 8 + n])
        o += 8 + n
        i += 1
    return out


def field_of(fields, chunk, offset):
    size, noun, _ = RECORDS[chunk]
    rec, off = divmod(offset, size)
    known = fields.get(chunk, {}).get(off)
    # nearest documented field at or below this offset, when the byte is inside a wider field
    if known is None:
        lower = [o for o in fields.get(chunk, {}) if o <= off and off - o < 4]
        known = fields[chunk][max(lower)] if lower else None
    name = known[1] if known else '?'
    base = known[0] if known else off
    return '%s %d +0x%X (%s)' % (noun, rec, base, name), rec, base


def diff_images(a, b):
    """[(chunk, address, offset, byte_a, byte_b)] for every differing byte."""
    out = []
    for i in sorted(set(a) & set(b)):
        (addr, x), (_, y) = a[i], b[i]
        if x != y:
            out.extend((i, addr, k, x[k], y[k]) for k in range(min(len(x), len(y))) if x[k] != y[k])
    return out


def main():
    argv = sys.argv[1:]
    limit = 12
    if '--max' in argv:
        k = argv.index('--max')
        limit = int(argv[k + 1])
        del argv[k:k + 2]
    if len(argv) != 2:
        print(__doc__.split('\n\n')[2])
        return 2
    args = argv
    roles, fields = _doc()
    a, b = read_image(args[0]), read_image(args[1])
    diffs = diff_images(a, b)
    by_chunk = {}
    for d in diffs:
        by_chunk.setdefault(d[0], []).append(d)
    for chunk, ds in sorted(by_chunk.items()):
        addr = ds[0][1]
        print('chunk %d (0x%08X, %s): %d bytes' % (chunk, addr, roles.get(chunk, '?'), len(ds)))
        if chunk in RECORDS:
            seen = {}
            for _, _, k, x, y in ds:
                label, rec, base = field_of(fields, chunk, k)
                seen.setdefault((rec, base), (label, []))[1].append((k, x, y))
            for (rec, base), (label, bytes_) in list(seen.items())[:limit]:
                print('  %s: %s' % (label, ' '.join('+0x%X %02X→%02X' % (k % RECORDS[chunk][0], x, y)
                                                     for k, x, y in bytes_[:4])))
            if len(seen) > limit:
                print('  ... %d more fields' % (len(seen) - limit))
        else:
            for _, _, k, x, y in ds[:limit]:
                print('  +0x%X %02X→%02X' % (k, x, y))
            if len(ds) > limit:
                print('  ... %d more bytes' % (len(ds) - limit))
    print('%d chunks differ, %d bytes' % (len(by_chunk), len(diffs)))
    return 1 if diffs else 0


if __name__ == '__main__':
    sys.exit(main())


# --- naming any address of the game state --------------------------------------------------------
_SCHEMA = None
_DOC = None


def schema():
    """[(chunk, address, size)] from core/io/save_schema.c."""
    global _SCHEMA
    if _SCHEMA is None:
        text = open(os.path.join(ROOT, 'core', 'io', 'save_schema.c')).read()
        _SCHEMA = [(int(m.group(5)), int(m.group(1), 16), int(m.group(2)) * int(m.group(3)))
                   for m in re.finditer(r'\{\d, 0x([0-9A-F]+)u, (\d+), (\d+), (\d+)\}, /\* (\d+) \*/', text)]
    return _SCHEMA


def name_address(addr):
    """'chunk 32 buildings: building 412 +0x18 (...)' or 'global 0x00BCA2D0'."""
    global _DOC
    if _DOC is None:
        _DOC = _doc()
    roles, fields = _DOC
    for chunk, base, size in schema():
        if base <= addr < base + size:
            if chunk in RECORDS:
                return 'chunk %d: %s' % (chunk, field_of(fields, chunk, addr - base)[0])
            return 'chunk %d (%s) +0x%X' % (chunk, roles.get(chunk, '?')[:40], addr - base)
    if addr >= 0x60000000:
        return 'heap 0x%08X' % addr
    return 'global 0x%08X' % addr


def diff_ranges(a, b, base, block=4096):
    """[(address, length)] of the differing byte runs of two equal-length buffers."""
    out = []
    for o in range(0, len(a), block):
        x, y = a[o:o + block], b[o:o + block]
        if x == y:
            continue
        k = 0
        while k < len(x):
            if x[k] != y[k]:
                s = k
                while k < len(x) and x[k] != y[k]:
                    k += 1
                if out and out[-1][0] + out[-1][1] == base + o + s:
                    out[-1] = (out[-1][0], out[-1][1] + k - s)
                else:
                    out.append((base + o + s, k - s))
            else:
                k += 1
    return out


STATE_BASE, STATE_END, HEAP_BASE = 0x0056F000, 0x01267AA8, 0x60000000


def state_file_diff(a, b):
    """[(address, length)] differing between two state files (data segments, then the heap; a
    heap present in one file only counts as different)."""
    import struct as _s
    n = STATE_END - STATE_BASE
    out = diff_ranges(a[:n], b[:n], STATE_BASE)
    ua, = _s.unpack_from('<I', a, n)
    ub, = _s.unpack_from('<I', b, n)
    ha, hb = a[n + 4:n + 4 + ua], b[n + 4:n + 4 + ub]
    m = max(len(ha), len(hb))
    out += diff_ranges(ha.ljust(m, b'\0'), hb.ljust(m, b'\0'), HEAP_BASE)
    if len(ha) != len(hb):
        out.append((HEAP_BASE + min(len(ha), len(hb)), abs(len(ha) - len(hb))))
    return out
