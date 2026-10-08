"""Imhotep - frame lockstep: the original's frame 0x004D6B60 against the port's, frame by frame.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tools/oracle/framestep.py SAVE | --start map:NAME|mission:N
        [--display WxHxFORMAT] [--window ID] [--script FILE | --frames N [--step MS]] [--chained]
        [--png] [--report] [-v]

The reference game boots with the emulated DirectDraw (--display, default 1024x768x565; docs/oracle.md
"Display"), loads SAVE (or starts a new game), and is put in window ID (default 1, the city view)
with the full-redraw flag set and the cursor at the centre of the screen (a WM_MOUSEMOVE). Then, for
each frame of the script:
  1. the frame's inputs are set: the clock (timeGetTime / GetTickCount return the frame's time), the
     cursor (GetCursorPos), the keys held (GetAsyncKeyState), and the window messages the player's
     actions make are delivered to the original's window procedure 0x00414CD0 (the WndProc), which
     turns them into the input globals (docs/presentation.md 5): WM_MOUSEMOVE when the cursor moves,
     WM_LBUTTONDOWN / UP, WM_RBUTTONDOWN / UP when the buttons change, WM_KEYDOWN / WM_KEYUP and
     WM_CHAR for keys;
  2. the frame 0x004D6B60 runs.
Both run in the original while tools/oracle/replay.py records the unported calls (state and drawing
buffers); then build/state_replay runs the same calls from the same state and buffers: ported
functions natively, the others replayed. A top-level function with no port yet (the frame, the WndProc)
is replayed whole: until the frame is ported the tool checks only its own plumbing ("not ported").
After each frame the state (excluded.py rules) and the back buffer are compared; differing pixels are
reported as rectangles (--png: both screens, the rectangles marked, in local/oracle/screens).
By default each frame starts from the original's state and buffers; --chained continues from the
port's.

Script (--script FILE), one frame per line, '#' comments:
  TIME [mouse=X,Y] [buttons=-|L|R|LR] [key=VK,...] [down=VK,...] [up=VK,...] [char=TEXT]
       [msg=MSG,WPARAM,LPARAM] [window=ID] [redraw] [ADDR=HEXBYTES ...]
TIME in ms (the clock of the frame, absolute); mouse in screen pixels (the window is fullscreen at
0,0); buttons held during the frame (- none); key = pressed and released before the frame; down / up
= held from this frame on / released; VK as a number (0x52) or a letter; char = WM_CHAR per
character; msg = a raw message; window / redraw / ADDR=HEX write the state of both sides first.
Without --script: --frames N (default 10) frames, --step MS apart (default 40), starting at 1000 ms,
no input.
Exit status: 0 if every frame is equal, 1 otherwise, 2 on setup error.
"""
import os
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from game import Game, WNDPROC  # noqa: E402
import diff  # noqa: E402
import excluded  # noqa: E402
import screen  # noqa: E402
from replay import DeepRecorder, buffer_args, capture_buffers, external_maps, mark_inputs, ported_table  # noqa: E402
from where import where  # noqa: E402

ROOT = diff.ROOT
FRAME = 0x004D6B60
WINDOW, REDRAW = 0x00E38DDA, 0x00E38DCA   # u8 current window, u8 full redraw (docs/presentation.md 3.2)
WM_KEYDOWN, WM_KEYUP, WM_CHAR = 0x100, 0x101, 0x102
WM_MOUSEMOVE = 0x200
BUTTON_MSGS = {'L': (0x201, 0x202, 1), 'R': (0x204, 0x205, 2)}   # down, up, MK_ flag


def vk(text):
    return ord(text.upper()) if len(text) == 1 and not text.isdigit() else int(text, 0)


def read_script(path):
    """[{time, mouse, buttons, key, down, up, char, msg, sets}] from a script file"""
    frames = []
    for line in open(path):
        words = line.split('#')[0].split()
        if not words:
            continue
        f = {'time': int(words[0]), 'mouse': None, 'buttons': None, 'key': [], 'down': [], 'up': [], 'char': '',
             'msg': [], 'sets': []}
        for w in words[1:]:
            k, _, v = w.partition('=')
            if k == 'mouse':
                f['mouse'] = tuple(int(x) for x in v.split(','))
            elif k == 'buttons':
                f['buttons'] = '' if v == '-' else v.upper()
            elif k in ('key', 'down', 'up'):
                f[k] = [vk(x) for x in v.split(',') if x]
            elif k == 'char':
                f['char'] = v
            elif k == 'msg':
                f['msg'].append(tuple(int(x, 0) for x in v.split(',')))
            elif k == 'window':
                f['sets'].append((WINDOW, bytes([int(v, 0)])))
            elif k == 'redraw':
                f['sets'].append((REDRAW, b'\x01'))
            else:
                f['sets'].append((int(k, 16), bytes.fromhex(v)))
        frames.append(f)
    return frames


def default_script(n, step):
    return [{'time': 1000 + step * i, 'mouse': None, 'buttons': None, 'key': [], 'down': [], 'up': [], 'char': '',
             'msg': [], 'sets': []} for i in range(n)]


class Inputs:
    """the player's devices between frames: cursor, buttons, held keys"""

    def __init__(self, g):
        self.mouse = g.cursor
        self.buttons = ''

    def messages(self, g, f):
        """the window messages of frame f, in order; updates g's clock, cursor and keys"""
        out = []
        g.clock = f['time']
        if f['mouse'] is not None and f['mouse'] != self.mouse:
            self.mouse = f['mouse']
            g.cursor = self.mouse
            out.append((WM_MOUSEMOVE, self.mk(), (self.mouse[1] & 0xFFFF) << 16 | (self.mouse[0] & 0xFFFF)))
        if f['buttons'] is not None:
            for b, (down, up, _) in BUTTON_MSGS.items():
                was, now = b in self.buttons, b in f['buttons']
                if was != now:
                    self.buttons = f['buttons'] if now else self.buttons.replace(b, '')
                    self.buttons = ''.join(c for c in 'LR' if c in self.buttons or (c == b and now))
                    out.append((down if now else up, self.mk(), (self.mouse[1] & 0xFFFF) << 16 | (self.mouse[0] & 0xFFFF)))
        for k in f['down']:
            g.keys_down.add(k)
            g.keys_pressed.add(k)
            out.append((WM_KEYDOWN, k, 1))
        for k in f['key']:
            g.keys_pressed.add(k)
            out.append((WM_KEYDOWN, k, 1))
            out.append((WM_KEYUP, k, 0xC0000001))
        for k in f['up']:
            g.keys_down.discard(k)
            out.append((WM_KEYUP, k, 0xC0000001))
        for c in f['char']:
            out.append((WM_CHAR, ord(c), 1))
        out += f['msg']
        return out

    def mk(self):
        return sum(BUTTON_MSGS[b][2] for b in self.buttons)


def replayed_ports(o):
    """the ported functions to replay from the original instead of running natively: --replay ADDR,..
    and --replay-owners PKG,.. (the functions those packages claimed in local/oracle/claims.txt) — to
    find which port makes a difference"""
    out = set(o['replay'])
    if o['owners']:
        owners = set(o['owners'])
        for line in open(os.path.join(ROOT, 'local', 'oracle', 'claims.txt')):
            f = line.split()
            if len(f) >= 2 and f[1] in owners:
                out.add(int(f[0], 16))
    out -= {FRAME, WNDPROC}
    return {a for a in out if a in ported_table()}


def options(argv):
    o = {'start': None, 'display': '1024x768x565', 'window': 1, 'script': None, 'frames': 10, 'step': 40,
         'chained': False, 'png': False, 'report': False, 'verbose': False, 'replay': [], 'owners': []}
    rest = []
    i = 0
    while i < len(argv):
        a = argv[i]
        if a in ('--start', '--display', '--script'):
            o[a[2:]] = argv[i + 1]
            i += 1
        elif a in ('--window', '--frames', '--step'):
            o[a[2:]] = int(argv[i + 1], 0)
            i += 1
        elif a == '--replay':
            o['replay'] = [int(x, 16) for x in argv[i + 1].split(',')]
            i += 1
        elif a == '--replay-owners':
            o['owners'] = argv[i + 1].split(',')
            i += 1
        elif a in ('--chained', '--png', '--report'):
            o[a[2:]] = True
        elif a == '-v':
            o['verbose'] = True
        else:
            rest.append(a)
        i += 1
    return o, rest


def setup(o, save):
    """the booted game with the save loaded (or the new game started), in the chosen window"""
    import harness
    if o['start']:
        import newgame
        what, _, difficulty = o['start'].partition('@')

        def start(g):
            addr, args = newgame.inputs(g, what, int(difficulty) if difficulty else None)
            g.call(addr, *args)
        g = Game.cached('start ' + o['start'], start, display=o['display'])  # snapshot (Game.cached)
    else:
        g = harness.oracle_state(save, 0, o['display'])  # boot + load, snapshotted
    g.write(WINDOW, bytes([o['window']]))
    g.write(REDRAW, b'\x01')
    # the cursor at the centre of the screen (at 0,0 it would scroll the map, which also stops ticks)
    _, w, h, _, _ = g.screen_info()
    g.cursor = (w // 2, h // 2)
    g.message(WM_MOUSEMOVE, 0, (h // 2) << 16 | (w // 2))
    return g


def run(g, script, o, out=print):
    """the frame lockstep on game g; returns the number of frames with differences"""
    table = ported_table()
    native = {a for a, t in table.items() if not t[4]}
    forced = replayed_ports(o)  # --replay / --replay-owners: these ports are replayed from the original
    native -= forced
    rec = DeepRecorder(g, native)
    tool = os.path.join(ROOT, os.environ.get('IMHOTEP_BUILD', 'build'), 'state_replay')
    inputs = Inputs(g)
    failed = 0
    ours = None        # the port's state and buffers after the previous frame (--chained)
    ours_buffers = None
    whole_seen = set()
    with tempfile.TemporaryDirectory() as tmp:
        paths = [os.path.join(tmp, n) for n in ('in', 'replay', 'out')]
        for t, f in enumerate(script):
            for a, v in f['sets']:
                g.write(a, v)
                if ours is not None:
                    ours = patch(ours, [(a, v)])
            msgs = inputs.messages(g, f)
            rec.take()
            sound_from = len(g.ail_log)
            pre = g.state_bytes()
            before = capture_buffers(g)
            info_before = g.screen_info()
            mark_inputs(g)
            for m in msgs:
                rec.call(WNDPROC, [Game.HWND, m[0], m[1], m[2]], whole=WNDPROC not in native)
            rec.call(FRAME, [], whole=FRAME not in native)
            spans = list(rec.spans)
            replay = rec.take()
            post = g.state_bytes()
            start = ours if (o['chained'] and ours is not None) else pre
            start_buffers = ours_buffers if (o['chained'] and ours_buffers is not None) else before
            # the input state's back buffer (a mode change in the frame makes a new one: buffer_args
            # gives it to the port zeroed, and it is the one compared)
            opts, outputs = buffer_args(g, tmp, start_buffers, info_before)
            if getattr(g, 'sound', False):  # the original's sound requests (sound.py, docs/oracle.md "Sound")
                import sound
                lines = sound.requests(g.ail_log, g.miles.notes, sound_from, scopes=sound.platform_functions())
                open(os.path.join(tmp, 'sound'), 'w').write(''.join(line + '\n' for line in lines))
                opts += ['--sound', os.path.join(tmp, 'sound')]
            open(paths[0], 'wb').write(start)
            open(paths[1], 'wb').write(replay)
            if forced:
                opts = opts + ['--replay', ','.join('%X' % a for a in sorted(forced))]
            r = subprocess.run([tool] + paths + external_maps(g, tmp) + opts, capture_output=True, text=True)
            whole = [int(line.split()[1], 16) for line in r.stdout.splitlines() if line.startswith('whole ')]
            whole_seen.update(whole)
            label = 'frame %d (t %d ms, window 0x%02X, tick %d)' % (t, f['time'], g.read(WINDOW, 1)[0], g.i32(0x00E94DBC))
            if r.returncode != 0:
                failed += 1
                out('%s: state_replay exit %d' % (label, r.returncode))
                out('    ' + '\n    '.join(line for line in r.stdout.splitlines()
                                             if not line.startswith(('native', 'replay', 'return', 'args', 'whole')))[-2000:])
                out('    ' + r.stderr.strip()[-2000:])
                break
            ours = open(paths[2], 'rb').read()
            ours_buffers = [(a, open(p, 'rb').read()) for a, p in outputs]
            reason = excluded.excluder(post)
            bad = [(a, n) for a, n in diff.state_file_diff(post, ours) if not reason(a)]
            info = g.screen_info()
            pixels, rects = 0, []
            other = []  # (kept for buffers compared besides the screen)
            for (a, mine) in ours_buffers:
                theirs = g.read(a, len(mine))
                if a == info[0]:
                    pixels, rects = screen.diff_rects(theirs, mine, info[1], info[2], info[3])
                # the spare screen is a work buffer (file reads, external images): not compared
            note = ' (not ported, replayed whole: %s)' % ', '.join('0x%08X' % a for a in sorted(set(whole))) if whole else ''
            if not bad and not pixels and not other:
                if o['verbose']:
                    out('%s: equal%s' % (label, note))
                continue
            failed += 1
            out('%s: %d state bytes, %d pixels, %d other buffer bytes differ%s' % (
                label, sum(n for _, n in bad), pixels, sum(n for _, n in other), note))
            for a, n in bad[:10]:
                who = [c for c, lo, ln in spans if lo <= a < lo + ln]
                out('    %s, %d bytes; %s' % (diff.name_address(a), n, 'last written by replayed 0x%08X' % who[-1]
                                             if who else 'written by the ports'))
            if pixels:
                out('    screen: %d rectangles: %s' % (len(rects), ', '.join(
                    '%dx%d at %d,%d' % (w, h, x, y) for x, y, w, h in rects[:8])))
                if o['png']:
                    stem = os.path.join(screen.SCREENS, 'frame%03d' % t)
                    screen.write_png(stem + '_original.png', g.screen(), info[1], info[2], info[3], info[4], rects)
                    port = dict(ours_buffers)[info[0]]
                    screen.write_png(stem + '_port.png', port, info[1], info[2], info[3], info[4], rects)
                    out('    screens: %s_original.png, %s_port.png' % (stem, stem))
    if o['report']:
        out('replayed     calls      bytes')
        for c in sorted(rec.rec_count, key=lambda c: -rec.rec_bytes.get(c, 0)):
            w = where(c)
            out('0x%08X %7d %10d  %s' % (c, rec.rec_count[c], rec.rec_bytes.get(c, 0), w.split()[1] if w else ''))
    run.whole = whole_seen
    return failed


def patch(state, sets):
    b = bytearray(state)
    for a, v in sets:
        if excluded.STATE_BASE <= a < 0x01267AA8:
            b[a - excluded.STATE_BASE:a - excluded.STATE_BASE + len(v)] = v
    return bytes(b)


def main():
    o, rest = options(sys.argv[1:])
    if not rest and not o['start']:
        print(__doc__.split('\n\n')[1])
        return 2
    script = read_script(o['script']) if o['script'] else default_script(o['frames'], o['step'])
    try:
        g = setup(o, rest[0] if rest else None)
    except RuntimeError as e:
        print(e)
        return 2
    failed = run(g, script, o)
    state = ('not ported: %s replayed whole' % ', '.join('0x%08X' % a for a in sorted(run.whole))) if run.whole else 'ported'
    print('framestep: %d frames, %d with differences (%s)' % (len(script), failed, state))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
