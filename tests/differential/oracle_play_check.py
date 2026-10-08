"""Imhotep - differential test: a scripted session of the game (build/imhotep) against the original,
frame by frame, state and pixels (roadmap 7.9).

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tests/differential/oracle_play_check.py [SESSION] [--display WxH] [--keep DIR] [-v]
  (default: tests/differential/play/session.txt at 1024x768; needs `make game`)

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
    sides), which the two allocators hand out at different addresses.
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


def run_game(session, display, work):
    from game import RESOLUTIONS
    player = os.path.join(work, 'player')
    dump = os.path.join(work, 'dump')
    os.makedirs(player)
    os.makedirs(dump)
    # the options a player who chose this resolution left (as game.py display_options writes them)
    with open(os.path.join(player, 'Pharaoh.inf'), 'wb') as f:
        f.write(struct.pack('<ii8Bi', 0x10, 1, 0, 0, 0x44, 1, 1, 1, 0, 0, RESOLUTIONS[display]))
    msglog = os.path.join(work, 'messages.txt')
    env = dict(os.environ, IMHOTEP_PLAYER_DIR=player)
    r = subprocess.run([GAME, '--hidden', '--no-intro', '--no-sound', '--no-video',
                        '--det-clock', '%d,%d' % (START, STEP), '--script', session, '--dump', dump,
                        '--msglog', msglog], env=env, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError('build/imhotep failed (%d): %s' % (r.returncode, (r.stdout + r.stderr)[-800:]))
    msgs = {}
    for line in open(msglog):
        k, m, w, l, x, y = line.split()
        msgs.setdefault(int(k), []).append((int(m, 16), int(w, 16), int(l, 16), int(x), int(y)))
    return dump, msgs, r.stderr


def compare(dump, msgs, display, verbose):
    import diff
    import excluded
    from game import Game, WNDPROC
    from oracle_boot_check import known_rules
    g = Game.booted(display='%dx%dx555' % display, sound=False)
    g.write(REDRAW, b'\x01')
    cursor = (0, 0)
    bad = []
    k = 0
    while True:
        k += 1
        path = os.path.join(dump, 'frame_%04d.state' % k)
        if not os.path.exists(path):
            return k - 1, bad
        g.clock = START + (k - 1) * STEP  # main.c delivers the messages before it advances its clock
        for m, w, l, x, y in msgs.get(k, []):
            cursor = (x, y)
            g.cursor = cursor
            g.call(WNDPROC, g.HWND, m, w, l)
            if m == 0x10:  # WM_CLOSE: main.c also sends WM_DESTROY and stops
                g.call(WNDPROC, g.HWND, 2, 0, 0)
                return k - 1, bad
        g.cursor = cursor
        g.clock = START + k * STEP
        g.call(FRAME)
        if g.read(QUIT, 1)[0] == 1:
            g.write(QUIT, b'\0')
            msgs.setdefault(k + 1, []).insert(0, (0x10, 0, 0, cursor[0], cursor[1]))
        mine = open(path, 'rb').read()
        theirs = g.state_bytes()
        reason = excluded.excluder(theirs)
        known = known_rules(theirs)

        def pointers(a, n):
            for d in range(a & ~3, (a + n + 3) & ~3, 4):
                o = d - STATE_BASE
                if d >= 0x60000000 or o + 4 > len(mine):
                    return False
                x, y = struct.unpack_from('<I', theirs, o)[0], struct.unpack_from('<I', mine, o)[0]
                if x != y and not all(OTHER_HEAP[0] <= v < OTHER_HEAP[1] for v in (x, y)):
                    return False
            return True
        ranges = [(a, n) for a, n in diff.state_file_diff(theirs, mine)
                  if not reason(a) and not any(lo <= a < lo + ln for lo, ln in PLATFORM)
                  and not all(known(x) for x in range(a, a + n)) and not pointers(a, n)]
        raw = os.path.join(dump, 'frame_%04d.raw' % k)
        ours, screen = (open(raw, 'rb').read() if os.path.exists(raw) else b''), g.screen()
        if len(ours) != len(screen):
            px = -1
        else:
            px = 0 if ours == screen else sum(ours[i:i + 2] != screen[i:i + 2] for i in range(0, len(screen), 2))
        if ranges or px:
            bad.append(k)
            if verbose or len(bad) <= 5:
                print('  frame %d (window %d): %d state ranges, %s pixels differ%s' % (
                    k, g.read(0x00E38DDA, 1)[0], len(ranges), 'buffer size' if px < 0 else px,
                    ': ' + ', '.join(diff.name_address(a) for a, _ in ranges[:6]) if ranges else ''))


def main():
    argv = sys.argv[1:]
    verbose = '-v' in argv
    argv = [a for a in argv if a != '-v']
    display, keep = (1024, 768), None
    if '--display' in argv:
        k = argv.index('--display')
        display = tuple(int(v) for v in argv[k + 1].lower().split('x')[:2])
        del argv[k:k + 2]
    if '--keep' in argv:
        k = argv.index('--keep')
        keep = argv[k + 1]
        del argv[k:k + 2]
    session = argv[0] if argv else SESSION
    if not os.path.exists(GAME):
        print('oracle_play_check: %s missing (make game)' % os.path.relpath(GAME, ROOT))
        return 2
    with tempfile.TemporaryDirectory() as tmp:
        work = keep or tmp
        try:
            dump, msgs, log = run_game(session, display, work)
        except RuntimeError as e:
            print('oracle_play_check: %s' % e)
            return 2
        if verbose:
            print(log, end='')
        frames, bad = compare(dump, msgs, display, verbose)
    print('oracle_play_check: %d / %d frames equal to the original (state and pixels), %s from the port\'s '
          'boot at %dx%d' % (frames - len(bad), frames, os.path.relpath(session, ROOT), display[0], display[1]))
    return 0 if frames and not bad else 1


if __name__ == '__main__':
    sys.exit(main())
