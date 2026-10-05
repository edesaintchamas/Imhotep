"""Imhotep - differential test: our save loader against the original's, run by the oracle.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tests/differential/oracle_load_check.py [--pak] [FILE.sav ...]
  (default: local/saves/downloaded/*/*.sav and local/saves/generated/*/*.sav;
   --pak: every record of mission1.pak, older file versions that go through the version upgrades)

For each save, the reference Pharaoh.exe (in the emulator, tools/oracle/) reads it with its chunk
reader and loader 0x004D41E0, and `build/save_dump` reads it with save_file_read + save_file_load
(core/io/), starting from the same CRT rand() state. Every one of the 181 saved variables must be
equal byte for byte, except the route records' path-buffer pointers (chunk 143, +0x04 of each
0x144-byte record), which are heap addresses.
Exit status: 0 if all equal, 1 otherwise, 2 on setup error.
"""
import glob
import os
import struct
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))
from game import Game  # noqa: E402

CRT_RAND_STATE = 0x00604950  # @bin 0x00562348 rand(): holdrand
LOADER = 0x004D41E0
ROUTES = 0x00776FB0          # chunk 143: 0x144-byte route records, path buffer pointer at +0x04
ROUTE_SIZE = 0x144


def ignored(addr, k):
    return addr == ROUTES and k % ROUTE_SIZE in (4, 5, 6, 7)


def check(g, path, tmp):
    name = 'check.sav'
    g.files[name] = open(path, 'rb').read()
    p = g.alloc(len(name) + 1)
    g.write(p, name.encode() + b'\0')
    g.call(0x0046FA40)  # as 0x004D5100 does before the loader
    seed = g.u32(CRT_RAND_STATE)
    fd = g.call(0x00568EAB, p, 0x8000)
    g.call(LOADER, fd)
    g.call(0x005647EB, fd)
    img = os.path.join(tmp, 'ours.img')
    r = subprocess.run([os.path.join(ROOT, os.environ.get('IMHOTEP_BUILD', 'build'), 'save_dump'), path, '--rand', str(seed), '--image', img],
                       capture_output=True, text=True)
    if r.returncode != 0:
        print('  %s: save_dump failed: %s' % (path, r.stderr.strip()))
        return None
    data = open(img, 'rb').read()
    o, bad = 0, []
    for i in range(181):
        addr, n = struct.unpack_from('<II', data, o)
        ours = data[o + 8:o + 8 + n]
        o += 8 + n
        emu = g.read(addr, n)
        if emu != ours:
            diffs = [k for k in range(n) if emu[k] != ours[k] and not ignored(addr, k)]
            if diffs:
                bad.append('chunk %d (0x%08X): %d bytes, first +0x%X ours %02X original %02X'
                           % (i, addr, len(diffs), diffs[0], ours[diffs[0]], emu[diffs[0]]))
    return bad


def pak_records(tmp):
    """Write each record of mission1.pak to tmp/recordN.sav (via build/save_dump --raw)."""
    out = []
    for n in range(100):
        path = os.path.join(tmp, 'record%d.sav' % n)
        r = subprocess.run([os.path.join(ROOT, os.environ.get('IMHOTEP_BUILD', 'build'), 'save_dump'), '--pak', str(n), '--stored', '--raw', path],
                           capture_output=True, text=True)
        if r.returncode == 0:
            out.append(path)
    return out


def main():
    with tempfile.TemporaryDirectory() as tmp_pak:
        return run(tmp_pak)


def run(tmp_pak):
    if sys.argv[1:] == ['--pak']:
        files = pak_records(tmp_pak)
    else:
        files = sys.argv[1:] or sorted(glob.glob(os.path.join(ROOT, 'local', 'saves', 'downloaded', '*', '*.sav')) +
                                   glob.glob(os.path.join(ROOT, 'local', 'saves', 'generated', '*', '*.sav')))
    if not files:
        print('oracle_load_check: no saves found')
        return 2
    failed = 0
    with tempfile.TemporaryDirectory() as tmp:
        for path in files:
            g = Game()  # a fresh boot (0.4 s) for each save: no state carried over
            g.boot()
            bad = check(g, path, tmp)
            if bad is None:
                return 2
            failed += bool(bad)
            label = os.path.basename(path) if path.startswith(tempfile.gettempdir()) else os.path.relpath(path, ROOT)
            print('  %s: %s' % (label, 'equal' if not bad else '; '.join(bad[:4])), flush=True)
    print('oracle_load_check: %d saves, %d equal to the original loader' % (len(files), len(files) - failed))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
