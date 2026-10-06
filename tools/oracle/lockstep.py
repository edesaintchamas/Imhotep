"""Imhotep - lockstep: our tick against the original's, tick by tick.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tools/oracle/lockstep.py SAVE [--ticks N] [--chained]
        [--replay-all | --replay ADDR,...] [--commands FILE] [--report] [-v]

The reference game boots in the emulator and loads SAVE. For each tick, the original runs
0x004D7090 while tools/oracle/replay.py records every call it makes to a function that is not
ported (at any depth): what the call changed in the state and what it returned. Then our tick
(core/sim/tick.c, build/state_replay) runs from the same state: ported functions run natively,
and each call to an unported one must be the next recorded call, which is replayed. The two states
after the tick are compared byte for byte (scratch and display exclusions: tools/oracle/excluded.py);
a difference is attributed to the recorded call that last wrote it, or else to the ports.

By default every tick starts from the original's state (differences do not accumulate);
--chained continues from ours. --replay replays these ported functions too (--replay-all: every
function but the tick). Prints, per tick with differences, where they are, and at the end the
share of the original's writes that ran natively; --report lists the replayed functions with their
calls and bytes written (the porting to-do list).
--commands FILE applies player commands between ticks (phase 5, docs/commands.md). One per line:
  TICK ADDR[@THIS][:ARG,...] [SET ...]      SET = ADDR=HEXBYTES, written into both states first;
                                            hover=X,Y sets the hovered tile (0x00BCA300 offset,
                                            0x00BCA304 x, 0x00BCA308 y), mouse=X,Y the tile under
                                            the mouse (0x00BCA2C4 .. 0x00BCA2CC); X,Y = -1,-1: off map
before tick TICK (0-based); several lines may share a tick and run in file order. A ported command
function runs in the original and in the port like the tick (its unported callees replayed) and the
states are compared after it; an unported one runs in the original only and its result is taken (the states are
equal before it, or the previous tick reported a difference).
Exit status: 0 if every tick is equal, 1 otherwise, 2 on setup error.
"""
import os
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from game import Game  # noqa: E402
import diff  # noqa: E402
import excluded  # noqa: E402
from replay import DeepRecorder, external_maps, ported_table  # noqa: E402
from where import where  # noqa: E402

ROOT = diff.ROOT
TICK = 0x004D7090


def covered(changed, spans):
    """bytes of the changed ranges [(address, n)] that lie in recorded spans [(callee, address, n)]"""
    marks = sorted((a, a + n) for _, a, n in spans)
    merged = []
    for lo, hi in marks:
        if merged and lo <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], hi)
        else:
            merged.append([lo, hi])
    import bisect
    starts = [m[0] for m in merged]
    total = 0
    for a, n in changed:
        i = max(bisect.bisect_right(starts, a) - 1, 0)
        while i < len(merged) and merged[i][0] < a + n:
            total += max(0, min(merged[i][1], a + n) - max(merged[i][0], a))
            i += 1
    return total


def read_commands(path):
    """{tick: [(address, args, this, [(address, bytes)])]} from a --commands file"""
    out = {}
    for line in open(path):
        words = line.split('#')[0].split()
        if not words:
            continue
        head, _, args = words[1].partition(':')
        addr, _, this = head.partition('@')
        sets = []
        for w in words[2:]:
            a, _, v = w.partition('=')
            if a in TILE_SETS:
                x, y = (int(c) for c in v.split(','))
                sets.append((a, (x, y)))
            else:
                sets.append((int(a, 16), bytes.fromhex(v)))
        out.setdefault(int(words[0]), []).append((int(addr, 16), [int(x, 0) for x in args.split(',') if x],
                                                  int(this, 16) if this else None, sets))
    return out


TILE_SETS = {'hover': 0x00BCA300, 'mouse': 0x00BCA2C4}  # grid offset, x, y (0x00405C20)
MAP_START = 0x00BCA2D8


def resolve(g, sets):
    """hover= / mouse= sets turned into bytes: offset = map start + 228 y + x (@bin 0x00405C4F)"""
    out = []
    for a, v in sets:
        if a in TILE_SETS:
            x, y = v
            off = 0 if x < 0 else g.i32(MAP_START) + 228 * y + x
            v = struct.pack('<iii', off, x, y)
            a = TILE_SETS[a]
        out.append((a, v))
    return out


def patch(state, sets):
    """state bytes with SET writes applied (addresses inside the segments)"""
    b = bytearray(state)
    for a, v in sets:
        b[a - excluded.STATE_BASE:a - excluded.STATE_BASE + len(v)] = v
    return bytes(b)


def run_port(tool, paths, start, replay, extra):
    """state_replay on start with replay; (process, resulting state or None)"""
    open(paths[0], 'wb').write(start)
    open(paths[1], 'wb').write(replay)
    r = subprocess.run([tool] + paths + extra, capture_output=True, text=True)
    return r, (open(paths[2], 'rb').read() if r.returncode == 0 else None)


def options(argv):
    o = {'commands': {}, 'ticks': 51, 'chained': False, 'verbose': False, 'replay': None, 'all': False, 'report': False}
    i = 0
    rest = []
    while i < len(argv):
        a = argv[i]
        if a == '--ticks':
            o['ticks'] = int(argv[i + 1])
            i += 1
        elif a == '--replay':
            o['replay'] = [int(x, 16) for x in argv[i + 1].split(',')]
            i += 1
        elif a == '--replay-all':
            o['all'] = True
        elif a == '--commands':
            o['commands'] = read_commands(argv[i + 1])
            i += 1
        elif a in ('--chained', '--report'):
            o[a[2:]] = True
        elif a == '-v':
            o['verbose'] = True
        else:
            rest.append(a)
        i += 1
    return o, rest


def main():
    o, rest = options(sys.argv[1:])
    save = rest[0]
    table = ported_table()
    native = {a for a, t in table.items() if not t[4]}
    extra = []
    if o['all']:
        native = {TICK}
        extra = ['--replay-all']
    elif o['replay']:
        native -= set(o['replay'])
        extra = ['--replay', ','.join('%X' % a for a in o['replay'])]
    g = Game()
    g.boot()
    if g.load_save(open(save, 'rb').read()) != 0:
        print('the original cannot load %s' % save)
        return 2
    rec = DeepRecorder(g, native)
    tool = os.path.join(ROOT, os.environ.get('IMHOTEP_BUILD', 'build'), 'state_replay')
    failed = 0
    written_total = 0
    replayed_total = 0
    ours = None
    with tempfile.TemporaryDirectory() as tmp:
        paths = [os.path.join(tmp, n) for n in ('in', 'replay', 'out')]
        for t in range(o['ticks']):
            for addr, args, this, sets in o['commands'].get(t, []):
                sets = resolve(g, sets)
                for a, v in sets:
                    g.write(a, v)
                if ours is not None:
                    ours = patch(ours, sets)
                pre = g.state_bytes()
                start = ours if (o['chained'] and ours is not None) else pre
                if addr not in native:  # unported: the original's result is taken (ours is equal to pre)
                    g.call(addr, *[x & 0xFFFFFFFF for x in args], ecx=this)
                    ours = g.state_bytes()
                    print('tick %d: command 0x%08X(%s) not ported, the original\'s result taken' % (
                        t, addr, ','.join(map(str, args))))
                    continue
                rec.call(addr, args, this)
                r, ours = run_port(tool, paths, start, rec.take(), extra + external_maps(g, tmp))
                post = g.state_bytes()
                if ours is None:
                    failed += 1
                    print('tick %d: command 0x%08X: state_replay exit %d\n    %s' % (
                        t, addr, r.returncode, (r.stdout.strip()[-1500:] + r.stderr.strip())))
                    break
                reason = excluded.excluder(post)
                bad = [(a, n) for a, n in diff.state_file_diff(post, ours) if not reason(a)]
                print('tick %d: command 0x%08X(%s): %s' % (t, addr, ','.join(map(str, args)), (
                    '%d bytes differ: ' % sum(n for _, n in bad) +
                    ', '.join(diff.name_address(a) for a, _ in bad[:6])) if bad else 'equal'), flush=True)
                failed += bool(bad)
            pre = g.state_bytes()
            rec.call(TICK)
            spans = list(rec.spans)
            replay = rec.take()
            post = g.state_bytes()
            start = ours if (o['chained'] and ours is not None) else pre
            open(paths[0], 'wb').write(start)
            open(paths[1], 'wb').write(replay)
            r = subprocess.run([tool] + paths + extra + external_maps(g, tmp), capture_output=True, text=True)
            changed = diff.state_file_diff(pre, post)
            written_total += sum(n for _, n in changed)
            replayed_total += covered(changed, spans)
            day = (g.i32(0x0078D7BC), g.i32(0x0078D7C0), g.i32(0x0078D7C4), g.i32(0x0078D7CC))
            if r.returncode not in (0,):
                failed += 1
                print('tick %d (y %d m %d d %d t %d): state_replay exit %d' % ((t,) + day + (r.returncode,)))
                print('    ' + '\n    '.join(l for l in r.stdout.splitlines()
                                             if not l.startswith(('native', 'replay', 'return', 'args')))[-2000:])
                print('    ' + r.stderr.strip()[-2000:])
                break
            ours = open(paths[2], 'rb').read()
            reason = excluded.excluder(post)
            bad = [(a, n) for a, n in diff.state_file_diff(post, ours) if not reason(a)]
            if not bad:
                if o['verbose']:
                    print('tick %d (y %d m %d d %d t %d): equal, %d replayed calls' % (
                        (t,) + day + (sum(1 for l in r.stdout.splitlines() if l.startswith('replay')),)),
                        flush=True)
                continue
            failed += 1
            print('tick %d (y %d m %d d %d t %d): %d bytes differ' % ((t,) + day + (sum(n for _, n in bad),)))
            for a, n in bad[:10]:
                who = [c for c, lo, ln in spans if lo <= a < lo + ln]
                print('    %s, %d bytes; %s' % (diff.name_address(a), n, 'last written by replayed 0x%08X' % who[-1]
                                                if who else 'written by the ports'), flush=True)
            args_lines = [l for l in r.stdout.splitlines() if l.startswith('args')]
            for l in args_lines[:5]:
                print('    ' + l)
    if o['report']:
        print('replayed     calls      bytes')
        for c in sorted(rec.rec_count, key=lambda c: -rec.rec_bytes.get(c, 0)):
            w = where(c)
            print('0x%08X %7d %10d  %s' % (c, rec.rec_count[c], rec.rec_bytes.get(c, 0), w.split()[1] if w else ''))
    native_share = 100.0 * (written_total - replayed_total) / written_total if written_total else 0
    print('lockstep: %d ticks, %d with differences; the ports wrote %.1f %% of the changed bytes' % (
        o['ticks'], failed, max(native_share, 0.0)))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
