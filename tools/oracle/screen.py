"""Imhotep - the back buffer of the original's drawing: PNG dumps and pixel comparisons.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

The game draws 16-bit pixels (565 or 555, docs/presentation.md 2.3) into its back buffer, rows of
`pitch` bytes (tools/oracle/game.py Game.screen). Images made from it are game output: write them
under local/ (local/oracle/screens/), never into the repository.

  local/venv/bin/python tools/oracle/screen.py SAVE [--display 1024x768x565] [--window ID] [--frames N]
        [--out PNG]

draws N frames (default 1) of the loaded SAVE in window ID (default 1, the city view) with the
frame 0x004D6B60 and writes the back buffer as a PNG (default local/oracle/screens/<save>.png).
"""
import os
import struct
import sys
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SCREENS = os.path.join(ROOT, 'local', 'oracle', 'screens')


def rgb(v, fmt):
    """a 16-bit pixel as 8-bit r, g, b (bits replicated)"""
    if fmt == 565:
        r, g, b = (v >> 11) & 31, (v >> 5) & 63, v & 31
        return (r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)
    r, g, b = (v >> 10) & 31, (v >> 5) & 31, v & 31
    return (r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2)


def write_png(path, data, w, h, pitch, fmt, mark=None):
    """the buffer as an RGB PNG; mark: rectangles (x, y, w, h) outlined in magenta"""
    lut = [bytes(rgb(v, fmt)) for v in range(65536)]
    rows = []
    for y in range(h):
        px = struct.unpack_from('<%dH' % w, data, y * pitch)
        rows.append(bytearray(b''.join(lut[v] for v in px)))
    for x0, y0, rw, rh in mark or ():
        for x in range(x0, x0 + rw):
            for y in (y0, y0 + rh - 1):
                rows[y][3 * x:3 * x + 3] = b'\xff\x00\xff'
        for y in range(y0, y0 + rh):
            for x in (x0, x0 + rw - 1):
                rows[y][3 * x:3 * x + 3] = b'\xff\x00\xff'
    raw = zlib.compress(b''.join(b'\0' + bytes(r) for r in rows), 6)

    def chunk(kind, body):
        return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body) & 0xFFFFFFFF)
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) +
                chunk(b'IDAT', raw) + chunk(b'IEND', b''))


def diff_rects(a, b, w, h, pitch):
    """(differing pixel count, [(x, y, width, height)]) where buffers a and b differ: the row runs
    of differing pixels merged with the runs they overlap (or touch) in the next row"""
    if a == b:
        return 0, []
    count = 0
    open_rects = []   # [x0, x1, y0, y1] still growing (x1, y1 exclusive)
    done = []
    for y in range(h):
        ra, rb = a[y * pitch:y * pitch + 2 * w], b[y * pitch:y * pitch + 2 * w]
        runs = []
        if ra != rb:
            pa, pb = struct.unpack('<%dH' % w, ra), struct.unpack('<%dH' % w, rb)
            x = 0
            while x < w:
                if pa[x] != pb[x]:
                    s = x
                    while x < w and pa[x] != pb[x]:
                        x += 1
                    count += x - s
                    if runs and s - runs[-1][1] <= 8:   # close runs of one row: one rectangle
                        runs[-1][1] = x
                    else:
                        runs.append([s, x])
                else:
                    x += 1
        still = []
        for r in open_rects:
            hit = [u for u in runs if u[0] <= r[1] and u[1] >= r[0]]
            if hit:
                r[0] = min([r[0]] + [u[0] for u in hit])
                r[1] = max([r[1]] + [u[1] for u in hit])
                r[3] = y + 1
                for u in hit:
                    runs.remove(u)
                still.append(r)
            else:
                done.append(r)
        still += [[u[0], u[1], y, y + 1] for u in runs]
        # rectangles that grew into each other
        merged = []
        for r in sorted(still):
            if merged and r[0] <= merged[-1][1] and merged[-1][3] >= r[2]:
                m = merged[-1]
                m[0], m[1], m[2], m[3] = min(m[0], r[0]), max(m[1], r[1]), min(m[2], r[2]), max(m[3], r[3])
            else:
                merged.append(r)
        open_rects = merged
    done += open_rects
    return count, [(x0, y0, x1 - x0, y1 - y0) for x0, x1, y0, y1 in sorted(done, key=lambda r: (r[2], r[0]))]


def main():
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from game import Game
    argv = sys.argv[1:]

    def opt(name, default):
        if name in argv:
            k = argv.index(name)
            v = argv[k + 1]
            del argv[k:k + 2]
            return v
        return default
    display = opt('--display', '1024x768x565')
    window = int(opt('--window', '1'), 0)
    frames = int(opt('--frames', '1'))
    save = argv[0]
    out = opt('--out', None) or os.path.join(SCREENS, '%s_%s_w%02x.png' % (
        os.path.splitext(os.path.basename(save))[0], display, window))
    import harness
    try:
        g = harness.oracle_state(save, 0, display)  # boot + load, snapshotted (Game.cached)
    except RuntimeError:
        print('the game cannot load %s' % save)
        return 2
    g.write(0x00E38DDA, bytes([window]))
    g.write(0x00E38DCA, b'\x01')
    for _ in range(frames):
        g.call(0x004D6B60)
    g.screen_png(out)
    print(out)
    return 0


if __name__ == '__main__':
    sys.exit(main())
