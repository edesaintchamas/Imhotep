"""Imhotep - differential test: a scripted session of the game (build/imhotep) against the original,
frame by frame, state and pixels (roadmap 7.9).

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tests/differential/oracle_play_check.py [SESSION] [--display WxH] [--windowed]
        [--keep DIR] [--player DIR] [--stop] [--png] [-v]
  (default: tests/differential/play/session.txt at 1024x768; needs `make game`)
  --windowed: the game windowed (as F6 leaves Pharaoh.inf), in a window of the screen's size
  --player DIR: the player directory both sides start from (saved games, families: tools/oracle/fuzz.py);
        its files are copied into the game's player directory and given to the original (Game.add_file)
        before a full boot
  --stop: stop at the first differing frame and print it in detail (every range named, both sides'
        bytes); the states before and after it are kept in the work directory (--keep), with --png
        both screens (local/oracle/screens)

The game runs alongside the comparison, at most WINDOW frames ahead (build/imhotep --dump-window: the
compared frames' dumps are deleted), so a session's length is not bounded by the disk. The unported
functions the game reached (its gaps.log) and a crash (exit status) are reported.

The game runs the session (platform/main.c --script: clicks, keys, text) from its own boot (core/boot, as
a player starts it), hidden, with a scripted clock (--det-clock 0,25: frame k reads 25 k ms, the boot 0,
as the oracle's boot), no sound and no video (in the oracle BinkOpen fails: the original skips the
videos), and dumps its state and back buffer after each frame and the window messages it delivered
(--dump, --msglog). The original boots in the oracle (tools/oracle/game.py) with the same display
(Pharaoh.inf: the same options in both player directories), and each frame gets the same clock, the
same window messages (to its window procedure 0x00414CD0, before the frame they precede) and the same
cursor, then runs its frame 0x004D6B60. Each frame's state and back buffer must be equal.

Set aside, with their reasons:
  - the booted state's known differences (tests/differential/oracle_boot_check.py known_rules: the
    import table, the stack bytes the original leaves in the monument objects and campaign steps,
    the oracle's omissions) and tools/oracle/excluded.py;
  - platform values: the window handle 0x00E39140, the cursor handles (platform/cursors.c), the
    DirectDraw handles and lock object;
  - pointers into the memory outside the state (pixel data, buffers: 0x68000000..0x98000000 on both
    sides), which the two allocators hand out at different addresses;
  - the original's uninitialised stack bytes the port writes otherwise, as docs/unverified.md documents,
    are made the port's in the original as they appear (normalise_uninitialised(): UNINIT-417B40).
Exit status: 0 if every frame is equal, 1 otherwise, 2 on setup errors.
"""
import os
import struct
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))
sys.path.insert(0, os.path.join(ROOT, 'tests', 'differential'))
GAME = os.path.join(ROOT, os.environ.get('IMHOTEP_BUILD', 'build'), 'imhotep')
SESSION = os.path.join(ROOT, 'tests', 'differential', 'play', 'session.txt')
FRAME = 0x004D6B60
QUIT = 0x00D3B43D    # a quit request: main.c turns it into WM_CLOSE before the next frame
REDRAW = 0x00E38DCA  # main.c sets it before its loop
START, STEP = 0, 25
PLATFORM = [(0x00E39140, 4), (0x00E39090, 0x4C), (0x00E390DC, 4), (0x00E39148, 4), (0x00E39170, 0xC),
            (0x006DE534, 0x10)]
OTHER_HEAP = (0x68000000, 0x98000000)
STATE_BASE = 0x0056F000


def options(display, windowed):
    """the Pharaoh.inf both sides boot with (as game.py display_options writes it): the display's
    resolution, fullscreen or windowed (a window of the screen's size: the session's coordinates are
    those of that resolution's layout; tests/differential/oracle_boot_check.py checks a smaller window)"""
    from game import RESOLUTIONS
    return struct.pack('<ii8Bi', 0x10, 0 if windowed else 1, 0, 0, 0x44, 1, 1, 1, 0, 0, RESOLUTIONS[display])


STOP_ORPHAN = False  # a pool worker's check (tools/oracle/fuzz.py -j): stops when its parent is gone
WINDOW = 48  # frames the game may run ahead of the comparison (each dump: about 15 MB)


def player_files(src):
    """(relative name with '/', bytes) of a player directory's files"""
    out = []
    for d, _, files in os.walk(src):
        for f in sorted(files):
            full = os.path.join(d, f)
            out.append((os.path.relpath(full, src).replace(os.sep, '/'), open(full, 'rb').read()))
    return sorted(out)


def start_game(session, display, windowed, work, player_src=None):
    import shutil
    player = os.path.join(work, 'player')
    dump = os.path.join(work, 'dump')
    if player_src:
        shutil.copytree(player_src, player)
    else:
        os.makedirs(player)
    os.makedirs(dump)
    with open(os.path.join(player, 'Pharaoh.inf'), 'wb') as f:
        f.write(options(display, windowed))
    msglog = os.path.join(work, 'messages.txt')
    open(msglog, 'w').close()
    env = dict(os.environ, IMHOTEP_PLAYER_DIR=player)
    err = open(os.path.join(work, 'game.log'), 'w')
    proc = subprocess.Popen([GAME, '--hidden', '--no-intro', '--no-sound', '--no-video',
                             '--det-clock', '%d,%d' % (START, STEP), '--script', session, '--dump', dump,
                             '--dump-window', str(WINDOW), '--dump-delta', '--msglog', msglog], env=env, stdout=subprocess.DEVNULL,
                            stderr=err)
    return proc, err, player, dump, msglog


class Messages:
    """the game's window messages (its --msglog), read as the game writes them"""

    def __init__(self, path):
        self.f = open(path)
        self.rest = ''
        self.by_frame = {}

    def poll(self):
        data = self.rest + self.f.read()
        lines = data.split('\n')
        self.rest = lines.pop()
        for line in lines:
            if line.strip():
                k, m, w, l, x, y = line.split()
                self.by_frame.setdefault(int(k), []).append((int(m, 16), int(w, 16), int(l, 16), int(x), int(y)))

    def get(self, k, default):
        return self.by_frame.get(k, default)

    def setdefault(self, k, v):
        return self.by_frame.setdefault(k, v)


def frames(proc, dump):
    """frame numbers as their dumps are complete (main.c --dump-delta writes each file whole, by a rename;
    the back buffer's before the state's)"""
    import time
    k = 1
    while True:
        if os.path.exists(os.path.join(dump, 'frame_%04d.sdelta' % k)):
            yield k
            k += 1
        elif proc.poll() is not None and not os.path.exists(os.path.join(dump, 'frame_%04d.sdelta' % k)):
            return
        else:
            time.sleep(0.002)


class Delta:
    """a file rebuilt from main.c's --dump-delta files ("IMDL", u32 size, (u32 offset, u32 length,
    bytes)..., u32 0xFFFFFFFF), applied in frame order"""

    def __init__(self):
        self.buf = bytearray()

    def apply(self, path):
        data = open(path, 'rb').read()
        if data[:4] != b'IMDL':
            raise RuntimeError('%s: not a delta dump' % path)
        size = struct.unpack_from('<I', data, 4)[0]
        if len(self.buf) != size:
            self.buf = bytearray(size)
        o = 8
        while True:
            off = struct.unpack_from('<I', data, o)[0]
            if off == 0xFFFFFFFF:
                break
            n = struct.unpack_from('<I', data, o + 4)[0]
            self.buf[off:off + n] = data[o + 8:o + 8 + n]
            o += 8 + n
        return bytes(self.buf)


class FrameDiff:
    """The differing ranges of a frame's states, less the pairs of pointers into the memory outside the
    state (OTHER_HEAP on both sides, in the data segments), for a sequence of frames: a 4 KB block whose
    bytes are the same as at the previous frame on both sides keeps its result (most of the state does not
    change from frame to frame, and the pointer pairs are the same ones). Runs are cut at block bounds."""
    BLOCK = 4096

    def __init__(self):
        import re
        self.runs = re.compile(rb'[^\x00]+')
        self.prev = (b'', b'')
        self.cache = {}

    def block(self, t, m, o, n, seg):
        x, y = t[o:o + n], m[o:o + n]
        if x == y:
            return []
        z = (int.from_bytes(x, 'little') ^ int.from_bytes(y, 'little')).to_bytes(n, 'little')
        out = []
        for r in self.runs.finditer(z):
            a, e = o + r.start(), o + r.end()
            if a >= seg:  # the heap: no pointer pairs set aside
                out.append((a, e - a))
                continue
            for d in range(a & ~3, e, 4):
                u, v = struct.unpack_from('<I', t, d)[0], struct.unpack_from('<I', m, d)[0]
                if u != v and not (OTHER_HEAP[0] <= u < OTHER_HEAP[1] and OTHER_HEAP[0] <= v < OTHER_HEAP[1]):
                    out.append((a, e - a))
                    break
        return out

    def __call__(self, theirs, mine):
        import diff
        n = diff.STATE_END - diff.STATE_BASE + 4  # the segments and the heap's size, then the heap
        seg = n - 4
        if len(theirs) < n or len(mine) < n or theirs[seg:n] != mine[seg:n]:
            # heaps of different sizes: the plain comparison (no pointer pairs set aside in the heap)
            self.cache, self.prev = {}, (b'', b'')
            return diff.state_file_diff(theirs, mine)
        end = n + struct.unpack_from('<I', theirs, seg)[0]  # the block list after the heap is not compared
        pt, pm = self.prev
        if len(pt) != len(theirs) or len(pm) != len(mine):
            self.cache = {}
        mt, mm, mpt, mpm = theirs, mine, pt, pm  # bytes: slices compare with memcmp (memoryviews do not)
        out = []
        cache = {}
        for o in range(0, end, self.BLOCK):
            k = min(self.BLOCK, end - o)
            r = self.cache.get((o, k))
            if r is None or mt[o:o + k] != mpt[o:o + k] or mm[o:o + k] != mpm[o:o + k]:
                r = self.block(mt, mm, o, k, seg)
            cache[(o, k)] = r
            out += r
        self.cache = cache
        self.prev = (theirs, mine)
        res = []
        for o, k in out:
            if o < seg:
                res.append((diff.STATE_BASE + o, min(k, seg - o)))
            elif o >= n:
                res.append((diff.HEAP_BASE + o - n, k))
        return res


def detail(ranges, theirs, mine, limit=40):
    import diff
    out = []
    for a, n in ranges[:limit]:
        if a >= 0x60000000:
            out.append('    %s (%d bytes)' % (diff.name_address(a), n))
            continue
        o = a - STATE_BASE
        out.append('    %s (%d bytes): original %s, port %s' % (diff.name_address(a), n, theirs[o:o + min(n, 16)].hex(),
                                                                 mine[o:o + min(n, 16)].hex()))
    if len(ranges) > limit:
        out.append('    ... %d more ranges' % (len(ranges) - limit))
    return '\n'.join(out)


def pixel_box(ours, screen, pitch):
    xs, ys = [], []
    for i in range(0, len(screen), 2):
        if ours[i:i + 2] != screen[i:i + 2]:
            ys.append(i // pitch)
            xs.append(i % pitch // 2)
    return (min(xs), min(ys), max(xs), max(ys)) if xs else None


SCREEN = (1024, 768)  # the screen build/imhotep boots on (platform/main.c port_boot), before the game's mode


def follow_display_mode(g, display, screen=None):
    """GetSystemMetrics(SM_CXSCREEN / SM_CYSCREEN) answers the display's current mode, as Windows does
    once DirectDraw's SetDisplayMode (IDirectDraw +0x54) has changed it, and the desktop's again after
    SetCooperativeLevel (+0x50) with DDSCL_NORMAL (8, windowed; assumed: the desktop's mode restored).
    tools/oracle/game.py answers the boot's display size always, so a second DirectDraw set-up
    (0x00426B40, which stores GetSystemMetrics into 0x00E38DB8 / 0x00E38DB4) in another mode differs
    from the port's (core/frame/display.c dd_create: the platform's current mode, WM_DISPLAYCHANGE).
    Here only, for the play sessions (game.py's code is in every snapshot's key). `screen`: the screen's
    mode before the game sets its own (default: the display's, as game.py boots)."""
    mode = list(screen or display)
    g.screen_mode = mode
    g.booting = False

    def set_mode(e, this, w, h, bpp):
        if g.booting and [w, h] != mode:
            # during the boot, Windows sends WM_DISPLAYCHANGE within SetDisplayMode, as the port's boot
            # does (core/boot/display.c display_changed); its handler (0x00414CD0, core/frame/wndproc.c)
            # writes the bits per pixel 0x00E38DBC and the screen's size 0x00E38DB8 / 0x00E38DB4. In play,
            # build/imhotep delivers it after the call (its message log gives it to the original too)
            g.write(0x00E38DBC, struct.pack('<i', 16))
            g.write(0x00E38DB8, struct.pack('<i', w))
            g.write(0x00E38DB4, struct.pack('<i', h))
        mode[:] = [w, h]
        return 0

    def cooperative(e, this, hwnd, flags):
        if flags & 8:
            mode[:] = list(screen or display)
        return 0
    def overrides():
        g.handlers['IDirectDraw+54'] = (4, set_mode)
        g.handlers['IDirectDraw+50'] = (3, cooperative)
        g.handlers['GetSystemMetrics'] = (1, lambda e, i: {0: mode[0], 1: mode[1]}.get(i, 0))
    overrides()
    # Each set-up makes new COM objects (surfaces, a clipper), and emu.py's com_object new stubs for their
    # vtables: a game that changes the mode every frame (window 62 reverting to itself) runs out of stub
    # slots. The stubs of one label call the same handlers (named label+offset, the last registered),
    # so objects of a label can share one vtable: here they do (the handlers registered again each time).
    tables = {}
    make = g.com_object

    def com_object(methods, label):
        if label not in tables:
            obj = make(methods, label)
            tables[label] = g.u32(obj)
        else:
            for off in range(0, 0x100, 4):
                n, fn = g.com_method(methods, label, off)
                if not callable(fn):
                    fn = (lambda value: lambda e, *a: value)(fn)
                g.handlers['%s+%X' % (label, off)] = (n, fn)
            obj = g.alloc(16)
            g.set_u32(obj, tables[label])
        overrides()
        return obj
    g.com_object = com_object


MESSAGES, MESSAGE_CURRENT = 0x00E95228, 0x00E951E4


def normalise_uninitialised(g, mine):
    """The original's uninitialised stack bytes that the port writes otherwise, as docs/unverified.md
    documents, are made the port's in the original's state as soon as they appear (an input neither side
    can reproduce: both sides then go on from the same values). UNINIT-417B40: the tutorial message 0x83 of
    0x00417B40 (its block's phrases 0x249, 0x248 at record +0x10, +0x12): the block's +4..+0x0F and
    +0x12..+0x1D (record +0x14..+0x1F, +0x22..+0x2D) are the original's stack, -1 in the port; the replies
    0x00D3B186, which the message window copies from the current record's +0x26 (core/command/
    orders_messages.c), follow. Only where the port's record holds the -1s (a message posted in the session:
    a record loaded from a saved game holds what the file has on both sides). Returns the records changed."""
    raw = g.read(MESSAGES, 0x30 * 1000)
    cur = struct.unpack('<i', g.read(MESSAGE_CURRENT, 4))[0]
    changed = []
    for k in range(1000):
        o = 0x30 * k
        if struct.unpack_from('<h', raw, o + 0x0A)[0] != 0x83 or struct.unpack_from('<hh', raw, o + 0x10) != (0x249, 0x248):
            continue
        if raw[o + 0x14:o + 0x20] == b'\xff' * 12 and raw[o + 0x22:o + 0x2E] == b'\xff' * 12:
            continue
        p = MESSAGES - STATE_BASE + o
        if mine[p + 0x14:p + 0x20] != b'\xff' * 12 or mine[p + 0x22:p + 0x2E] != b'\xff' * 12:
            continue
        old = raw[o + 0x26:o + 0x28]
        g.write(MESSAGES + o + 0x14, b'\xff' * 12)
        g.write(MESSAGES + o + 0x22, b'\xff' * 12)
        if k == cur and g.read(0x00D3B186, 2) == old:
            g.write(0x00D3B186, b'\xff\xff')
        changed.append(k)
    return changed


def compare(proc, dump, msgs, display, windowed, verbose, player_src=None, stop=False, png=False, work=None,
            report=print):
    import diff
    import excluded
    from game import Game, WNDPROC
    from oracle_boot_check import known_rules
    files = [(n, d) for n, d in player_files(player_src) if n.lower() != 'pharaoh.inf'] if player_src else []
    if windowed or files or display != SCREEN:
        # a boot of its own: the player's files, and the port's screen (1024 x 768: a smaller display is a
        # mode change, which Windows reports with WM_DISPLAYCHANGE, as core/boot/display.c assumes)
        g = Game(display=display + (555,), sound=False)
        follow_display_mode(g, display, SCREEN)
        g.add_file('Pharaoh.inf', options(display, windowed))
        for name, data in files:
            g.add_file(name, data)
        g.booting = True
        g.boot()
        g.booting = False
    else:
        g = Game.booted(display='%dx%dx555' % display, sound=False)
        follow_display_mode(g, display)
    g.write(REDRAW, b'\x01')
    cursor = (0, 0)
    bad = []
    k = 0
    before = None  # (the port's state, the original's) before the frame
    frame_diff = FrameDiff()
    state_file, raw_file = Delta(), Delta()
    for k in frames(proc, dump):
        if STOP_ORPHAN and k % 100 == 0 and os.getppid() == 1:
            raise SystemExit('oracle_play_check: the parent process is gone')
        path = os.path.join(dump, 'frame_%04d.sdelta' % k)
        raw = os.path.join(dump, 'frame_%04d.rdelta' % k)
        mine = state_file.apply(path)
        ours = raw_file.apply(raw) if os.path.exists(raw) else b''
        msgs.poll()
        g.clock = START + (k - 1) * STEP  # main.c delivers the messages before it advances its clock
        try:
            for m, w, l, x, y in msgs.get(k, []):
                if m == 0xFFFFFFFF:  # the script's answer to the next message box (main.c: answer N)
                    g.box_answers = getattr(g, 'box_answers', None) or []
                    g.box_answers.append(w)
                    continue
                cursor = (x, y)
                g.cursor = cursor
                g.call(WNDPROC, g.HWND, m, w, l)
                if m == 0x10:  # WM_CLOSE: main.c also sends WM_DESTROY and stops
                    g.call(WNDPROC, g.HWND, 2, 0, 0)
                    return k - 1, bad
            g.cursor = cursor
            g.clock = START + k * STEP
            g.call(FRAME)
        except Exception as e:  # the original stopped (an import the oracle does not emulate, a fault)
            report('  frame %d (window %d): the original stopped: %s' % (k, g.read(0x00E38DDA, 1)[0], e))
            bad.append(k)
            return k, bad
        for rec in normalise_uninitialised(g, mine):
            report('  frame %d: message %d (0x83 of 0x00417B40): the original\'s uninitialised bytes made the '
                   'port\'s (UNINIT-417B40)' % (k, rec))
        if g.read(QUIT, 1)[0] == 1:
            g.write(QUIT, b'\0')
            msgs.setdefault(k + 1, []).insert(0, (0x10, 0, 0, cursor[0], cursor[1]))
        theirs = g.state_bytes()
        reason = excluded.excluder(theirs)
        known = known_rules(theirs)

        ranges = [(a, n) for a, n in frame_diff(theirs, mine)
                  if not reason(a) and not any(lo <= a < lo + ln for lo, ln in PLATFORM)
                  and not all(known(x) for x in range(a, a + n))]
        screen = g.screen()
        if len(ours) != len(screen):
            px = -1
        else:
            px = 0 if ours == screen else sum(ours[i:i + 2] != screen[i:i + 2] for i in range(0, len(screen), 2))
        if ranges or px:
            bad.append(k)
            if verbose or stop or len(bad) <= 5:
                report('  frame %d (window %d): %d state ranges, %s pixels differ%s' % (
                    k, g.read(0x00E38DDA, 1)[0], len(ranges), 'buffer size' if px < 0 else px,
                    ': ' + ', '.join(diff.name_address(a) for a, _ in ranges[:6]) if ranges else ''))
            if stop:
                if ranges:
                    report(detail(ranges, theirs, mine))
                if px > 0:
                    info = g.screen_info()
                    report('    pixels: box %s (x0, y0, x1, y1)' % (pixel_box(ours, screen, info[3]),))
                if work:
                    kept = [('diff_original.state', theirs), ('diff_port.state', mine)]
                    if before:
                        kept += [('before_port.state', before[0]), ('before_original.state', before[1])]
                    for name, data in kept:
                        with open(os.path.join(work, name), 'wb') as f:
                            f.write(data)
                    report('    kept in %s: %s' % (os.path.relpath(work, ROOT), ', '.join(n for n, _ in kept)))
                if png:
                    import screen as scr
                    out = os.path.join(ROOT, 'local', 'oracle', 'screens')
                    os.makedirs(out, exist_ok=True)
                    g.screen_png(os.path.join(out, 'play_original_%04d.png' % k))
                    _, w, h, pitch, fmt = g.screen_info()
                    if len(ours) == pitch * h:
                        scr.write_png(os.path.join(out, 'play_port_%04d.png' % k), ours, w, h, pitch, fmt)
                    report('    screens: local/oracle/screens/play_{original,port}_%04d.png' % k)
                return k, bad
        # the frame is compared: its dump goes (the game waits for it, --dump-window); the states before
        # the next frame are kept for a diagnosis
        before = (mine, theirs) if work else None
        os.remove(path)
        if os.path.exists(raw):
            os.remove(raw)
    return k, bad


def gaps(player):
    path = os.path.join(player, 'gaps.log')
    return [line.strip() for line in open(path)] if os.path.exists(path) else []


def check(session, display=(1024, 768), windowed=False, keep=None, player_src=None, stop=False, png=False,
          verbose=False, report=print):
    """runs the game and the comparison; returns a dict: frames, bad (frame numbers), gaps (gaps.log lines),
    status (the game's exit status), log (its stderr)"""
    import shutil
    work = keep or tempfile.mkdtemp(prefix='play_')
    if keep and os.path.exists(keep):
        shutil.rmtree(keep)
    os.makedirs(work, exist_ok=True)
    proc, err, player, dump, msglog = start_game(session, display, windowed, work, player_src)
    try:
        n, bad = compare(proc, dump, Messages(msglog), display, windowed, verbose, player_src, stop, png,
                         work if keep else None, report)
    finally:
        if proc.poll() is None:
            proc.kill()
        proc.wait()
        err.close()
    log = open(os.path.join(work, 'game.log')).read()
    result = {'frames': n, 'bad': bad, 'gaps': gaps(player), 'status': proc.returncode, 'log': log}
    if not keep:
        shutil.rmtree(work, ignore_errors=True)
    return result


def main():
    argv = sys.argv[1:]
    flags = {f: f in argv for f in ('-v', '--windowed', '--stop', '--png')}
    argv = [a for a in argv if a not in flags]
    opts = {'--display': None, '--keep': None, '--player': None}
    for o in opts:
        if o in argv:
            k = argv.index(o)
            opts[o] = argv[k + 1]
            del argv[k:k + 2]
    display = tuple(int(v) for v in opts['--display'].lower().split('x')[:2]) if opts['--display'] else (1024, 768)
    session = argv[0] if argv else SESSION
    if not os.path.exists(GAME):
        print('oracle_play_check: %s missing (make game)' % os.path.relpath(GAME, ROOT))
        return 2
    r = check(session, display, flags['--windowed'], opts['--keep'], opts['--player'], flags['--stop'],
              flags['--png'], flags['-v'])
    if flags['-v']:
        print(r['log'], end='')
    frames, bad = r['frames'], r['bad']
    for line in r['gaps']:
        print('  gap: %s' % line)
    if r['status'] not in (0, -9):
        print('  the game exited with status %s: %s' % (r['status'], r['log'][-600:]))
    print('oracle_play_check: %d / %d frames equal to the original (state and pixels), %s from the port\'s '
          'boot at %dx%d%s' % (frames - len(bad), frames, os.path.relpath(session, ROOT), display[0], display[1],
                                ', windowed' if flags['--windowed'] else ''))
    return 0 if frames and not bad else 1


if __name__ == '__main__':
    sys.exit(main())
