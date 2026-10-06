"""Imhotep - differential test: the hover functions against the original's city-view drawing.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  IMHOTEP_BUILD=build local/venv/bin/python tests/differential/oracle_hover_check.py [-v]

core/command/hover.c computes, for the hovered tile, what the original's drawing code computes
each frame and the place step reads (docs/commands.md 2.1): the ferry reach 0x0047F3E0, the bridge
measure 0x0047F200 (plain functions, also checked by tools/oracle/harness.py) and ferry_hover, the
rule part of the ferry ghost draw 0x0043D300 (the landing validity 0x00BCA314). 0x0043D300 runs
headless in the oracle; its port is registered at its address with PORT_PLATFORM (the drawing is
left out), so it cannot go through harness.py (the original's draw calls are not made by the
port): here the original runs 0x0043D300 and the port runs ferry_hover (build/state_run) from the
same state, for shore tiles of several saves, first and second landings, and the whole states are
compared except DRAW_GLOBALS. Exit status: 0 if all equal, 1 otherwise.
"""
import os
import random
import struct
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))

from game import Game  # noqa: E402
import diff  # noqa: E402

STATE_BASE = 0x0056F000
TERRAIN, MAP_START, MAP_W, MAP_H = 0x00AF598C, 0x00BCA2D8, 0x00BCA2D0, 0x00BCA2D4
HOVER = 0x00BCA300
# the sprite-drawing globals written by 0x0053E590 / 0x0053F250 / 0x0053F4F0 (thiscall on the
# graphics object 0x00F55BB0) and the 0x0040F9A0.. blitters; read only by drawing functions
# (Ghidra export: their readers are 0x0042C740 .. 0x0042E680, 0x004CDAD0, 0x004CDB80, 0x0040F9A0 ..
# 0x004114A0, 0x0053E590 .. 0x0053FA40)
DRAW_GLOBALS = (0x00D3B530, 0x00D3B5E4)

IKEN = 'local/saves/downloaded/1564/CaesarAlan-Iken.sav'
ALEXC = 'local/saves/downloaded/1591/38-AlexC-Feb15.sav'
SAIS = 'local/saves/downloaded/1582/sais 6374.sav'
ACTIUM = 'local/saves/downloaded/1592/39-Actium-Dec30c.sav'

# (save, first landing placed before (second-landing cases) or None, tiles, random shore tiles)
CASES = [
    (IKEN, None, [(96, 25), (96, 26), (96, 27), (95, 25), (106, 51), (108, 59), (100, 40)], 30),
    (IKEN, (96, 25), [(x, y) for x in range(119, 124) for y in range(22, 29)] + [(96, 27), (96, 38)], 0),
    (ALEXC, None, [(101, 55), (31, 102), (100, 40)], 30),
    (ALEXC, (101, 55), [(70, y) for y in range(52, 59)] + [(71, 55)], 0),
    (SAIS, None, [(43, 81), (42, 69), (44, 81)], 30),
    (SAIS, (43, 81), [(32, y) for y in range(78, 85)], 0),
    (ACTIUM, None, [], 20),
]


def tile_bytes(g, x, y):
    return struct.pack('<iii', g.i32(MAP_START) + 228 * y + x, x, y)


def first_landing(g, x, y):
    """a ferry's first landing at (x, y) through the original's UI: choose the ferry (menu 34,
    item 2), its draw (0x0043D300) while hovering, mouse down, mouse up"""
    g.set_u32(0x007056EC, 34)
    g.set_u32(0x00D3B198, 2)
    g.call(0x00405200, timeout=10 ** 8)
    g.write(HOVER, tile_bytes(g, x, y))
    g.write(0x00BCA2C4, tile_bytes(g, x, y))
    g.call(0x0043D300, timeout=10 ** 9)
    g.call(0x00405C20, timeout=10 ** 8)
    g.call(0x00405E90, timeout=10 ** 8)


def shore_tiles(state, n, seed):
    def u32(a):
        return struct.unpack_from('<I', state, a - STATE_BASE)[0]
    ms, w, h = u32(MAP_START), u32(MAP_W), u32(MAP_H)
    out = []
    for y in range(1, h - 1):
        for x in range(1, w - 1):
            o = ms + 228 * y + x
            water = u32(TERRAIN + 4 * o) & 4
            if any((u32(TERRAIN + 4 * (o + d)) & 4) != water for d in (-228, 1, 228, -1)):
                out.append((x, y))
    random.Random(seed).shuffle(out)
    return out[:n]


def main():
    verbose = '-v' in sys.argv
    tool = os.path.join(ROOT, os.environ.get('IMHOTEP_BUILD', 'build'), 'state_run')
    failed = total = valid = 0
    with tempfile.TemporaryDirectory() as tmp:
        pin, pout = os.path.join(tmp, 'in'), os.path.join(tmp, 'out')
        for k, (save, first, tiles, n) in enumerate(CASES):
            g = Game()
            g.boot()
            g.load_save(open(os.path.join(ROOT, save), 'rb').read())
            if first:
                first_landing(g, *first)
            clean = g.state_bytes()
            for x, y in tiles + shore_tiles(clean, n, k):
                g.load_state(clean)
                g.write(HOVER, tile_bytes(g, x, y))
                pre = g.state_bytes()
                g.call(0x0043D300, timeout=10 ** 9)
                post = g.state_bytes()
                open(pin, 'wb').write(pre)
                r = subprocess.run([tool, pin, pout, '0x0043D300'], capture_output=True, text=True)
                total += 1
                if r.returncode != 0:
                    failed += 1
                    print('  %s %s (%d, %d): state_run exit %d %s' % (save, first, x, y, r.returncode, r.stderr[-200:]))
                    continue
                ours = open(pout, 'rb').read()
                bad = [(a, l) for a, l in diff.state_file_diff(post, ours)
                       if not DRAW_GLOBALS[0] <= a < DRAW_GLOBALS[1]]
                v = post[0x00BCA314 - STATE_BASE]
                valid += v
                if bad:
                    failed += 1
                    print('  %s %s (%d, %d): %d bytes differ: %s' % (save, first, x, y, sum(l for _, l in bad), ', '.join(
                        diff.name_address(a) for a, _ in bad[:6])))
                elif verbose:
                    print('  %s %s (%d, %d): equal, valid %d' % (save, first, x, y, v))
    print('oracle_hover_check: %d / %d hovered tiles equal to the original (%d valid landings)' % (
        total - failed, total, valid))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
