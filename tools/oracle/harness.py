"""Imhotep - the function oracle: an original game function against its port, on the same state.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tools/oracle/harness.py ADDR[:ARG,...] [ADDR ...] [--ticks N]
        [--fill ADDR:LEN:BYTE ...] [--display WxHxFORMAT [--png]] [SAVE ...]

For each save (default: the downloaded player saves and the m06 corpus saves), the reference game
boots in the emulator, loads the save with its own loader, and optionally runs N game ticks. The
whole game state (Pharaoh.exe's data segments, core/state/state.h) is then taken as the input of
both sides:
  - the original functions run in the emulator, in order;
  - their ports run on a copy of the same bytes (build/state_replay, core/state/ported.c); the
    original functions they call that are not ported yet are recorded in the original's run and
    replayed on the port's side (tools/oracle/replay.py).
--fill overwrites a region of the input first (e.g. an output grid, so that a recomputation is
seen in full). Every byte of the two results is compared; differences are named after docs/formats/save.md.
--display (e.g. 1024x768x565) boots with the emulated DirectDraw (docs/oracle.md "Display"): the back
buffer and the spare screen are compared too, the differing pixels reported as rectangles (--png
writes both screens with the rectangles marked into local/oracle/screens).
--replay A,B replays these ported functions too; --actions A,B runs only these figure actions
(table 0x005DC28C) natively, the other ported actions are replayed (to check one's own ports while
others are being written).
Exit status: 0 when every result is equal, 1 otherwise, 2 on setup error.
"""
import glob
import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from game import Game  # noqa: E402
import diff  # noqa: E402
import excluded  # noqa: E402
from replay import DeepRecorder, buffer_args, capture_buffers, external_maps, mark_inputs, ported_table  # noqa: E402
import screen  # noqa: E402

ROOT = diff.ROOT
STATE_BASE, STATE_END = 0x0056F000, 0x01267AA8  # core/state/state.h
GAME_TICK = 0x004D7090


THIS = {}  # address -> `this` given on the command line (ADDR@THIS:ARGS)


def parse_call(text):
    """ADDR[@THIS][:ARG,...] -> (address, [args]); THIS is kept in THIS[address]"""
    head, _, args = text.partition(':')
    addr, _, this = head.partition('@')
    a = int(addr, 16)
    if this:
        THIS[a] = int(this, 16)
    return a, [int(x, 0) & 0xFFFFFFFF for x in args.split(',') if x]


CACHE = os.path.join(ROOT, 'local', 'oracle', 'cache')


def oracle_state(save, ticks, display=None):
    """The original game with SAVE loaded and advanced `ticks` ticks. The whole emulated game is
    snapshotted after the boot, the load and the ticks (local/oracle/cache, Game.cached): a later
    run restores it in a fraction of a second. display: as Game(display=...) (None: Game()'s
    default, IMHOTEP_ORACLE_DISPLAY or no display)."""
    import hashlib
    data = open(save, 'rb').read()

    def setup(g):
        if g.load_save(data) != 0:
            raise RuntimeError('the game could not load %s' % save)
        for _ in range(ticks):
            g.call(GAME_TICK)
    kw = {} if display is None else {'display': display}
    return Game.cached('save %s %d' % (hashlib.sha1(data).hexdigest(), ticks), setup, **kw)


def ported():
    """{address: (nargs, return mask, name)} from build/state_run --list."""
    r = subprocess.run([os.path.join(ROOT, os.environ.get('IMHOTEP_BUILD', 'build'), 'state_run'), '--list'], capture_output=True, text=True, check=True)
    out = {}
    for line in r.stdout.splitlines():
        a, n, bits, this, _, name = line.split(None, 5)
        out[int(a, 16)] = (int(n), (1 << int(bits)) - 1, name, int(this, 16))
    return out


FORCED_REPLAY = set()  # ported functions replayed anyway (--replay, --actions)
ACTION_TABLE, ACTION_TYPES = 0x005DC28C, 110


def action_functions():
    """the figure action functions (table 0x005DC28C)"""
    from disasm import image
    import struct
    base, mem = image()
    return {struct.unpack_from('<I', mem, ACTION_TABLE - base + 4 * i)[0] for i in range(ACTION_TYPES)}


def recorder(g):
    """The deep recorder of g (tools/oracle/replay.py), created once per game."""
    if not hasattr(g, 'deep_recorder'):
        table = ported_table()
        g.deep_recorder = DeepRecorder(g, {a for a, t in table.items() if not t[4]} - FORCED_REPLAY)
    return g.deep_recorder


def compare(g, calls, tmp, limit=12, whole=False):
    """Run calls in the oracle and the port from g's current state; returns the differences.
    Unported functions the originals call are recorded and replayed on the port's side.
    With a display (Game(display=...)), the port gets the drawing buffers as they were before the calls
    (the back buffer, and the spare screen, a work buffer that is not compared: the ports of the file
    readers use their own buffers); the back buffers are compared, their differences added to the
    returned ranges, and compare.screen holds (differing pixels, [(x, y, w, h)]) of the back
    buffer. whole: a call to a function that is not ported is replayed whole (compare.whole lists them)
    instead of failing."""
    table = ported()
    RET_MASK = [table[a][1] if a in table else 0 for a, _ in calls]
    rec = recorder(g)
    table_kind = {a: t[5] for a, t in ported_table().items()}
    rec.take()
    sound_from = len(g.ail_log)
    pre = g.state_bytes()
    buffers_before = capture_buffers(g)
    info_before = g.screen_info()
    mark_inputs(g)
    returns_o = []
    for addr, args in calls:
        this = THIS.get(addr) or (table[addr][3] if addr in table and table[addr][3] else None)
        # a partial port is recorded whole: state_replay falls back to the record when it reaches
        # unported code (its nested records are the unported calls it makes when it runs natively)
        partial = addr in table_kind and table_kind[addr] == 1
        returns_o.append(rec.call(addr, args, this, whole=(whole or partial) and addr not in rec.native))
    replay = rec.take()
    post_o = g.state_bytes()
    src, rpl, dst = (os.path.join(tmp, n) for n in ('pre.state', 'replay', 'post.state'))
    open(src, 'wb').write(pre)
    open(rpl, 'wb').write(replay)
    extra = ['--replay', ','.join('%X' % a for a in sorted(FORCED_REPLAY))] if FORCED_REPLAY else []
    extra += external_maps(g, tmp)
    buffer_opts, buffer_outputs = buffer_args(g, tmp, buffers_before, info_before)
    extra += buffer_opts
    if getattr(g, 'sound', False):  # the original's sound requests: the port must make the same (sound.py)
        import sound
        lines = sound.requests(g.ail_log, g.miles.notes, sound_from,
                               scopes=sound.platform_functions())
        path = os.path.join(tmp, 'sound')
        open(path, 'w').write(''.join(line + '\n' for line in lines))
        extra += ['--sound', path]
        compare.sound = lines
    r = subprocess.run([os.path.join(ROOT, os.environ.get('IMHOTEP_BUILD', 'build'), 'state_replay'), src, rpl, dst] + extra,
                       capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError('state_replay exit %d: %s %s' % (r.returncode, r.stdout.strip()[-1500:], r.stderr.strip()))
    post_p = open(dst, 'rb').read()
    returns_p = [int(l.split()[1]) & 0xFFFFFFFF for l in r.stdout.splitlines() if l.startswith('return ')]
    compare.replayed = sum(1 for l in r.stdout.splitlines() if l.startswith('replay '))
    compare.whole = [int(l.split()[1], 16) for l in r.stdout.splitlines() if l.startswith('whole ')]
    # partial ports that reached unported code and were replayed from the original's record instead
    compare.fallback = [l for l in r.stdout.splitlines() if l.startswith('fallback ')]
    compare.stdout = r.stdout  # state_replay's report (native / replay / fallback / skipped lines)
    written = sum(n for _, n in diff.state_file_diff(pre, post_o))
    bad_returns = [(calls[i], returns_o[i], returns_p[i]) for i in range(len(calls))
                   if (returns_o[i] & RET_MASK[i]) != (returns_p[i] & RET_MASK[i])]
    compare.last = (pre, post_o, post_p)
    reason = excluded.excluder(post_o)
    ranges = [(a, n) for a, n in diff.state_file_diff(post_o, post_p) if not reason(a)]
    ranges += compare_buffers(g, buffer_outputs)
    return written, ranges, bad_returns


def compare_buffers(g, outputs):
    """the drawing buffers of the original (now) against the port's (state_replay's output files):
    [(address, length)] of the differences; compare.screen = (pixels, rectangles) of the back buffer"""
    compare.screen = (0, [])
    out = []
    info = g.screen_info()
    for a, path in outputs:
        if info is None or a != info[0]:
            continue  # the spare screen: a work buffer (file reads, external images), not compared
        mine = open(path, 'rb').read()
        theirs = g.read(a, len(mine))
        if mine == theirs:
            continue
        _, w, h, pitch, _ = info
        compare.screen = screen.diff_rects(theirs, mine, w, h, pitch)
        diff.SCREEN = info[:4]
        out += [(a + y * pitch + 2 * x, 2 * rw) for x, y, rw, rh in compare.screen[1]]
    compare.port_screen = None
    for a, path in outputs:
        if info is not None and a == info[0]:
            compare.port_screen = open(path, 'rb').read()
    return out


compare.screen = (0, [])
compare.port_screen = None


def main():
    argv = sys.argv[1:]
    ticks = 0
    if '--ticks' in argv:
        k = argv.index('--ticks')
        ticks = int(argv[k + 1])
        del argv[k:k + 2]
    if '--replay' in argv:
        k = argv.index('--replay')
        FORCED_REPLAY.update(int(a, 16) for a in argv[k + 1].split(','))
        del argv[k:k + 2]
    if '--actions' in argv:  # only these figure actions run natively; the other ported ones replay
        k = argv.index('--actions')
        keep = {int(a, 16) for a in argv[k + 1].split(',')}
        FORCED_REPLAY.update((action_functions() & set(ported_table())) - keep - {0x0051F780})  # the empty function
        del argv[k:k + 2]
    display = None
    if '--display' in argv:  # e.g. 1024x768x565: the back buffer is compared too
        k = argv.index('--display')
        display = argv[k + 1]
        del argv[k:k + 2]
    png = '--png' in argv  # the two back buffers of a differing state, as PNGs in local/oracle/screens
    if png:
        argv.remove('--png')
    fills = []
    while '--fill' in argv:
        k = argv.index('--fill')
        a, n, v = argv[k + 1].split(':')
        fills.append((int(a, 16), int(n, 0), int(v, 0)))
        del argv[k:k + 2]
    calls = [parse_call(a) for a in argv if not a.endswith('.sav')]
    saves = [a for a in argv if a.endswith('.sav')] or sorted(
        glob.glob(os.path.join(ROOT, 'local', 'saves', 'downloaded', '*', '*.sav')) +
        glob.glob(os.path.join(ROOT, 'local', 'saves', 'generated', '*', 'm06.sav')))
    if not calls:
        print(__doc__.split('\n\n')[1])
        return 2
    failed = 0
    with tempfile.TemporaryDirectory() as tmp:
        for save in saves:
            g = oracle_state(save, ticks, display)
            for a, n, v in fills:
                g.write(a, bytes([v]) * n)
            written, ranges, bad_returns = compare(g, calls, tmp)
            label = os.path.relpath(save, ROOT)
            for (a, args), ro, rp in bad_returns[:8]:
                print('  %s: 0x%08X(%s) returned %d, port %d' % (label, a, ','.join(map(str, args)), ro, rp))
            if bad_returns and not ranges:
                failed += 1
                continue
            fell = sorted(set(compare.fallback))
            note = ' (replayed, not run natively: %s)' % '; '.join(fell[:4]) if fell else ''
            if not ranges:
                print('  %s: equal (the original changed %d bytes)%s' % (label, written, note), flush=True)
                continue
            failed += 1
            total = sum(n for _, n in ranges)
            print('  %s: %d bytes differ in %d runs (the original changed %d bytes)' % (label, total, len(ranges), written))
            pixels, rects = compare.screen
            if pixels:
                print('    screen: %d pixels differ in %d rectangles: %s' % (pixels, len(rects), ', '.join(
                    '%dx%d at %d,%d' % (w, h, x, y) for x, y, w, h in rects[:8])))
                if png:
                    _, w, h, pitch, fmt = g.screen_info()
                    stem = os.path.join(screen.SCREENS, os.path.splitext(os.path.basename(save))[0])
                    screen.write_png(stem + '_original.png', g.screen(), w, h, pitch, fmt, rects)
                    screen.write_png(stem + '_port.png', compare.port_screen, w, h, pitch, fmt, rects)
                    print('    screens: %s_original.png, %s_port.png' % (stem, stem))
            pre, post_o, post_p = compare.last
            for addr, n in ranges[:12]:
                k = addr - STATE_BASE if addr < 0x60000000 else None
                if k is None:
                    print('    %s, %d bytes' % (diff.name_address(addr), n))
                    continue
                k4 = k & ~3  # the aligned dword around it
                print('    %s, %d bytes: before %s original %s port %s' % (
                    diff.name_address(addr), n, pre[k4:k4 + 4][::-1].hex(), post_o[k4:k4 + 4][::-1].hex(),
                    post_p[k4:k4 + 4][::-1].hex()), flush=True)
    print('harness: %s: %d / %d states equal' % (' '.join('0x%08X' % a for a, _ in calls), len(saves) - failed, len(saves)))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
