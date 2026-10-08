"""Imhotep - differential test: the sound and music logic (roadmap phase 7.7) against the original.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tests/differential/oracle_sound_check.py [CASE ...] [-j JOBS] [-v]

A case is NAME (from CASES) or SAVE@DISPLAY[:FRAMES]. The reference game boots with sound (Game(sound=True):
the sound start-up 0x005394F0 on tools/oracle/sound.py's stand-in for Miles, every option on) and the
emulated DirectDraw, loads the save and shows the city (window 1, cursor at the centre). Then, for each
frame, the original's frame 0x004D6B60 runs with a scripted clock (40 ms steps, with jumps past the
ambient slots' 2 s and the music's waits; the music and ambient streams end now and then), which draws the city
(its ground pass fills the ambient slots and counters) and plays; after it, on that state, the sound
functions run in the original and in the port (tools/oracle/harness.py compare): the frame's sound
steps (ambient slots 0x0041D560, music 0x00539820, random city sound 0x0053ACF0, ambient stream
0x0053B960, fades 0x0053B6F0), the figure sounds 0x0053B4A0 of figures on and off the screen, the
interface sounds 0x0053B090, the ground pass's helpers (0x0041D410, 0x0053B670, 0x0053B830), a game
tick (its figures' sounds) and the same steps with each sound option off. Then 18 frames run chained through
tools/oracle/framestep.py (the port's frame against the original's), the streams ending and the clock
jumping between runs of 6. Each comparison checks the state and the back buffer, and the sound
requests: the port's (core/sim/platform.h) must be the ones the original made of Miles (state_replay
--sound).
Exit status: 0 if every case is equal, 1 otherwise.
"""
import os
import random
import struct
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))

SAVES = {
    'actium': 'local/saves/downloaded/1592/39-Actium-Dec30c.sav',
    'iken': 'local/saves/downloaded/1564/CaesarAlan-Iken.sav',
    'alexc': 'local/saves/downloaded/1591/38-AlexC-Feb15.sav',
    'sais': 'local/saves/downloaded/1582/sais 6374.sav',
    'maritis': 'local/saves/downloaded/1590/37-Maritis-Dec44c.sav',
    'itjtawy': 'local/saves/downloaded/1562/CaesarAlan-Itjtawy.sav',
    'pak00': 'local/saves/generated/pak00/m00.sav',
}
CASES = {
    'actium-1024': ('actium', '1024x768x565', 24),
    'iken-800': ('iken', '800x600x565', 24),
    'alexc-640': ('alexc', '640x480x555', 24),
    'sais-1024': ('sais', '1024x768x555', 24),
    'maritis-1024': ('maritis', '1024x768x565', 24),
    'itjtawy-800': ('itjtawy', '800x600x555', 24),
    'pak00-1024': ('pak00', '1024x768x565', 24),
}

FRAME = 0x004D6B60
GAME_TICK = 0x004D7090
MUSIC = 0x00E39180
WINDOW, REDRAW = 0x00E38DDA, 0x00E38DCA
OPTIONS = {'sound': (0x00F55B88, 4), 'effects': (0x00E38E63, 1), 'music': (0x00E38E64, 1),
           'city': (0x00E38EC0, 1)}
TAIL = [(0x0041D560, []), (0x00539820, []), (0x0053ACF0, []), (0x0053B960, []), (0x0053B6F0, [2]),
        (0x0053B6F0, [0]), (0x0053B6F0, [1])]


def parse(case):
    if case in CASES:
        save, display, frames = CASES[case]
        return SAVES[save], display, frames
    path, _, rest = case.partition('@')
    parts = rest.split(':')
    return path, parts[0], int(parts[1]) if len(parts) > 1 else 24


def figures(g, n, rng):
    """up to n figures alive (+0x0E = 1) of the figure array 0x00BCD5F8"""
    out = []
    for f in range(1, 2000):
        if g.read(0x00BCD5F8 + 0x184 * f + 0x0E, 1)[0] == 1:
            out.append(f)
    rng.shuffle(out)
    return out[:n]


def figures_near(g, n):
    """up to n figures alive that are on the screen: their tile's screen position (the original's
    0x004F0BA0, into a scratch block) inside the city view"""
    if not hasattr(g, 'scratch8'):
        g.scratch8 = g.alloc(8)
    width = g.i32(0x00F53F60) if g.i32(0x00F53F78) else g.i32(0x00F53F64)
    height = g.i32(0x00E38D98)
    out = []
    for f in range(1, 2000):
        a = 0x00BCD5F8 + 0x184 * f
        if g.read(a + 0x0E, 1)[0] != 1:
            continue
        x, y = struct.unpack('<2h', g.read(a + 0x14, 4))
        g.write(g.scratch8, bytes(8))
        g.call(0x004F0BA0, x & 0xFFFFFFFF, y & 0xFFFFFFFF, g.scratch8, g.scratch8 + 4)
        sx, sy = struct.unpack('<2i', g.read(g.scratch8, 8))
        if 0 <= sx <= width and 0 <= sy <= height:
            out.append(f)
            if len(out) == n:
                break
    return out


def buildings(g, n, rng):
    out = [b for b in range(1, 4000) if g.read(0x007999D8 + 0x108 * b, 1)[0] == 1]
    rng.shuffle(out)
    return out[:n]


def check(case, verbose=False):
    import harness
    from game import Game, WNDPROC  # noqa: F401
    save, display, frames = parse(case)
    rng = random.Random(case)
    g = Game(display=display, sound=True)
    g.boot()
    if g.load_save(open(os.path.join(ROOT, save), 'rb').read()) != 0:
        return 'error: the original cannot load %s' % save
    g.write(WINDOW, b'\x01')
    g.write(REDRAW, b'\x01')
    _, w, h, _, _ = g.screen_info()
    g.cursor = (w // 2, h // 2)
    g.message(0x200, 0, (h // 2) << 16 | (w // 2))
    harness.THIS[0x00539820] = MUSIC
    notes = []
    counts = {'compares': 0, 'requests': 0}
    seen = set()

    def compare(label, calls):
        written, ranges, bad = harness.compare(g, calls, tmp)
        counts['compares'] += 1
        counts['requests'] += len(harness.compare.sound)
        seen.update(line.split()[0] for line in harness.compare.sound)
        if verbose:
            notes.append('%s: %s' % (label, '; '.join(harness.compare.sound) or '-'))
        if ranges or bad:
            raise RuntimeError('%s: %d state runs differ (first %s), returns %s' % (
                label, len(ranges), ['0x%08X+%d' % r for r in ranges[:4]], bad[:2]))

    clock = 1000
    try:
        with tempfile.TemporaryDirectory() as tmp:
            for i in range(frames):
                clock += 2100 if i % 6 == 5 else 120000 if i % 12 == 11 else 40
                g.clock = clock
                g.call(FRAME)
                if i % 4 == 3:
                    g.miles.end_stream(0)  # the tune ends: the music goes on to its next step
                if i % 5 == 4:
                    g.miles.end_stream(2)  # the ambient stream ends: it starts over
                compare('frame %d tail' % i, TAIL)
                if i % 4 == 0:
                    calls = [(0x0053B4A0, [f, rng.randrange(0x21), rng.randrange(5)]) for f in figures(g, 12, rng)]
                    calls += [(0x0053B4A0, [f, rng.randrange(0x21), rng.randrange(5)]) for f in figures_near(g, 12)]
                    calls += [(0x0053B4A0, [f, 0x21, 0]) for f in figures(g, 1, rng)]  # out of range
                    calls += [(0x0053B090, [k]) for k in rng.sample(range(0x14), 5)]
                    compare('frame %d figure / ui sounds' % i, calls)
                if i % 4 == 1:
                    calls = [(0x0053B670, [])] + [(0x0041D410, [b, rng.randrange(5)]) for b in buildings(g, 30, rng)]
                    calls += [(0x0053B830, [t]) for t in rng.sample(range(0xF0), 12)]
                    calls += [(0x0041D560, [])]
                    compare('frame %d ambient helpers' % i, calls)
                    # the ground pass marks its slots through building 0 (type 0x27 written before the
                    # call, 0x00431195), whose record is never in use: put it in use, and make slots
                    # ready (seen long enough, last played long ago), so that one is played
                    old = g.read(0x007999D8, 0x108)
                    g.write(0x007999D8, b'\x01')
                    for t in (0x27, 0x1E, 0x1F, rng.randrange(0xF0)):
                        g.write(0x007999E8, struct.pack('<h', t))
                        g.write(0x00799A18, struct.pack('<h', rng.randrange(-1, 3)))
                        compare('frame %d ambient building 0 type 0x%X' % (i, t),
                                [(0x0041D410, [0, rng.randrange(5)]) for _ in range(3)])
                    g.write(0x007999D8, old)
                    for k in rng.sample(range(1, 70), 4):
                        a = 0x00702900 + 0x80 * k
                        g.write(a, struct.pack('<3i', 1, 200 + rng.randrange(2), 200))
                        g.write(a + 0x0C, struct.pack('<5i', *[rng.choice((0, 5, 11, 20)) for _ in range(5)]))
                        g.write(a + 0x20, struct.pack('<2i', rng.randrange(3), rng.randrange(1, 4)))
                        g.write(a + 0x50, struct.pack('<i', clock - rng.choice((0, 29000, 31000, 60000))))
                    g.write(0x007028FC, struct.pack('<i', clock - rng.choice((1000, 2500))))
                    compare('frame %d ambient slots' % i, [(0x0041D560, [])])
                if i % 4 == 2:
                    compare('frame %d tick' % i, [(GAME_TICK, [])])
                if i % 6 == 3:
                    for name, (a, n) in OPTIONS.items():
                        old = g.read(a, n)
                        g.write(a, bytes(n))
                        try:
                            compare('frame %d %s off' % (i, name), TAIL + [(0x0053B4A0, [f, 3, 2]) for f in figures(g, 3, rng)] +
                                    [(0x0053B090, [0])])
                        finally:
                            g.write(a, old)
        # then the frames themselves, chained (tools/oracle/framestep.py: the port's frame 0x004D6B60 with
        # its sound steps against the original's, state, buffer and sound requests), in runs between which
        # the streams end and the clock jumps
        import framestep
        o, _ = framestep.options(['--display', display, '--chained'])
        lines = []
        for k in range(3):
            if k:
                g.miles.end_stream((0, 2)[k - 1])
            script = []
            for j in range(6):
                clock += 40 if j else 30000 * k + 2100
                script.append({'time': clock, 'mouse': None, 'buttons': None, 'key': [], 'down': [], 'up': [],
                               'char': '', 'msg': [], 'sets': []})
            n = len(g.ail_log)
            if framestep.run(g, script, o, out=lines.append):
                raise RuntimeError('frames: ' + ' / '.join(lines[:6]))
            if framestep.run.whole:
                raise RuntimeError('frames: not ported (replayed whole: %s)' % ', '.join(
                    '0x%08X' % a for a in sorted(framestep.run.whole)))
            import sound
            frame_requests = sound.requests(g.ail_log, g.miles.notes, n, scopes=sound.platform_functions())
            counts['frames'] = counts.get('frames', 0) + len(script)
            counts['requests'] += len(frame_requests)
            seen.update(line.split()[0] for line in frame_requests)
    except Exception as e:  # noqa: BLE001 - reported as the case's result
        return 'differs: %s' % str(e)[:1500] + ('\n      ' + '\n      '.join(notes[-6:]) if notes else '')
    out = 'equal (%d comparisons, %d chained frames, %d sound requests: %s)' % (
        counts['compares'], counts.get('frames', 0), counts['requests'], ', '.join(sorted(seen)))
    if verbose:
        out += '\n      ' + '\n      '.join(notes)
    return out


def main():
    argv = sys.argv[1:]
    jobs = 1
    if '-j' in argv:
        k = argv.index('-j')
        jobs = int(argv[k + 1])
        del argv[k:k + 2]
    verbose = '-v' in argv
    if verbose:
        argv.remove('-v')
    cases = argv or list(CASES)
    if jobs > 1:
        from concurrent.futures import ProcessPoolExecutor
        with ProcessPoolExecutor(jobs) as ex:
            results = list(ex.map(check, cases, [verbose] * len(cases)))
    else:
        results = [check(c, verbose) for c in cases]
    failed = 0
    for c, r in zip(cases, results):
        print('%-16s %s' % (c, r), flush=True)
        failed += not r.startswith('equal')
    print('oracle_sound_check: %d / %d cases equal' % (len(cases) - failed, len(cases)))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
