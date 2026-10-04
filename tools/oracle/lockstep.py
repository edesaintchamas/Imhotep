"""Imhotep - lockstep: our tick against the original's, tick by tick.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tools/oracle/lockstep.py SAVE [--ticks N] [--chained]
        [--replay-all | --replay ADDR,...] [--report] [-v]

The reference game boots in the emulator and loads SAVE. For each tick, the original runs
0x004D7090 with a code hook at every call made by the tick skeleton (0x004D7090, the day
dispatcher 0x00451C50, the cascade 0x00452370): before and after each call the state is
snapshotted, so the call's changes (data, heap, new heap blocks) and its return value are recorded.
Then our skeleton (core/sim/tick.c, build/state_tick) runs the same tick from the same state: it
must make the same calls in the same order; ported callees run natively (unless --replay), the
others replay the recorded changes. The two states after the tick are compared byte for byte
(scratch and display exclusions: tools/oracle/excluded.py); a difference is attributed to the call
that last wrote it in the original.

By default every tick starts from the original's state (differences do not accumulate);
--chained continues from ours. Prints, per tick, the calls (native / replayed) and the result,
and at the end the share of the original's writes made by native ports; --report lists every
callee with its calls and bytes written (the porting to-do list).
Exit status: 0 if every tick is equal, 1 otherwise, 2 on setup error.
"""
import os
import struct
import subprocess
import sys
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from game import Game  # noqa: E402
from emu import HEAP_BASE  # noqa: E402
import diff  # noqa: E402
import excluded  # noqa: E402
from where import where  # noqa: E402

ROOT = diff.ROOT
TICK = 0x004D7090
SKELETON = {0x004D7090: 0x12F, 0x00451C50: 0x4FF, 0x00452370: 0x27A}  # address: size


def call_sites(g):
    """{call or tail-jump address: (callee, return site or None for the tick's own return)}"""
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    sites = {}
    for f, size in SKELETON.items():
        for ins in md.disasm(g.read(f, size), f):
            if ins.mnemonic not in ('call', 'jmp') or not ins.op_str.startswith('0x'):
                continue
            target = int(ins.op_str, 16)
            if target in SKELETON:
                continue
            if ins.mnemonic == 'call':
                sites[ins.address] = (target, ins.address + ins.size)
            elif not (f <= target < f + size):  # a tail jump: returns where the function would
                sites[ins.address] = (target, 'tail')
    return sites


class Recorder:
    def __init__(self, g):
        self.g = g
        self.sites = call_sites(g)
        self.returns = {r for _, r in self.sites.values() if r != 'tail'}
        self.pending = []
        self.records = []
        # one hook per address: a return site is often the next call site, and the return must be
        # handled first
        for a in set(self.sites) | self.returns | {0x004D716B}:
            g.uc.hook_add(UC_HOOK_CODE, self.on_code, begin=a, end=a)

    def on_code(self, uc, addr, size, _):
        if addr in self.returns or addr == 0x004D716B:
            self.on_return(uc, addr, size, _)
        if addr in self.sites:
            self.on_call(uc, addr, size, _)

    def snapshot(self):
        g = self.g
        return g.state_bytes(), dict((p, n) for p, n in g.sizes.items() if HEAP_BASE <= p < g.heap)

    def on_call(self, uc, addr, size, _):
        callee, ret = self.sites[addr]
        if ret == 'tail':  # returns to the caller of the skeleton function
            ret = self.g.u32(uc.reg_read(UC_X86_REG_ESP))
        self.pending.append((callee, ret, self.snapshot()))

    def on_return(self, uc, addr, size, _):
        if self.pending and self.pending[-1][1] == addr:
            self.finish(uc.reg_read(UC_X86_REG_EAX))

    def finish(self, eax):
        callee, _, (before, blocks_before) = self.pending.pop()
        after, blocks_after = self.snapshot()
        self.records.append((callee, eax, before, after, blocks_before, blocks_after))

    def tick(self):
        self.records = [('tick',)]
        self.g.call(TICK)
        while self.pending:  # the tail jump of 0x004D7090 returns to our sentinel
            self.finish(self.g.uc.reg_read(UC_X86_REG_EAX))
        return self.records


def encode(records):
    """the REPLAY file of build/state_tick"""
    out = [b'']
    n = 0
    for r in records:
        n += 1
        if r[0] == 'tick':
            out.append(struct.pack('<IiIII', 0, 0, 0, 0, 0))
            continue
        callee, eax, before, after, bb, ba = r
        nd = diff.STATE_END - diff.STATE_BASE
        heap_used, = struct.unpack_from('<I', after, nd)
        new_blocks = sorted((p - HEAP_BASE, s) for p, s in ba.items() if p not in bb)
        ranges = diff.state_file_diff(before, after)
        chunk = [struct.pack('<IIII', callee, eax & 0xFFFFFFFF, heap_used, len(new_blocks))]
        chunk += [struct.pack('<II', o, s) for o, s in new_blocks]
        chunk.append(struct.pack('<I', len(ranges)))
        for a, ln in ranges:
            if a >= HEAP_BASE:
                o = nd + 4 + (a - HEAP_BASE)
                data = after[o:o + ln]
                data = data + bytes(ln - len(data))
            else:
                data = after[a - diff.STATE_BASE:a - diff.STATE_BASE + ln]
            chunk.append(struct.pack('<II', a, ln) + data)
        out.append(b''.join(chunk))
    out[0] = struct.pack('<I', n)
    return b''.join(out)


def written_by(records):
    """address -> callee that last wrote it, and bytes written per callee"""
    spans, total = [], {}
    for r in records:
        if r[0] == 'tick':
            continue
        callee, _, before, after = r[:4]
        for a, n in diff.state_file_diff(before, after):
            spans.append((a, a + n, callee))
            total[callee] = total.get(callee, 0) + n
    return spans, total


def main():
    argv = sys.argv[1:]
    ticks, chained, verbose, extra, report = 51, False, False, [], False
    if '--report' in argv:
        report = True
        argv.remove('--report')
    if '--ticks' in argv:
        k = argv.index('--ticks')
        ticks = int(argv[k + 1])
        del argv[k:k + 2]
    if '--chained' in argv:
        chained = True
        argv.remove('--chained')
    if '-v' in argv:
        verbose = True
        argv.remove('-v')
    if '--replay-all' in argv:
        extra = ['--replay-all']
        argv.remove('--replay-all')
    if '--replay' in argv:
        k = argv.index('--replay')
        extra = ['--replay', argv[k + 1]]
        del argv[k:k + 2]
    save = argv[0]
    g = Game()
    g.boot()
    if g.load_save(open(save, 'rb').read()) != 0:
        print('the original cannot load %s' % save)
        return 2
    rec = Recorder(g)
    tool = os.path.join(ROOT, 'build', 'state_tick')
    failed = 0
    native_bytes = replay_bytes = 0
    grand, calls_seen, native_seen = {}, {}, set()
    ours = None
    with tempfile.TemporaryDirectory() as tmp:
        for t in range(ticks):
            pre = g.state_bytes()
            records = rec.tick()
            post = g.state_bytes()
            start = ours if (chained and ours is not None) else pre
            paths = [os.path.join(tmp, n) for n in ('in', 'replay', 'out')]
            open(paths[0], 'wb').write(start)
            open(paths[1], 'wb').write(encode(records))
            r = subprocess.run([tool] + paths + extra, capture_output=True, text=True)
            lines = r.stdout.split()
            natives = {int(lines[i + 1], 16) for i in range(0, len(lines) - 1) if lines[i] == 'native'}
            spans, total = written_by(records)
            for c, n in total.items():
                grand[c] = grand.get(c, 0) + n
            for r0 in records:
                if r0[0] != 'tick':
                    calls_seen[r0[0]] = calls_seen.get(r0[0], 0) + 1
            native_seen |= natives
            native_bytes += sum(n for c, n in total.items() if c in natives)
            replay_bytes += sum(n for c, n in total.items() if c not in natives)
            if r.returncode not in (0,):
                failed += 1
                print('tick %d: state_tick exit %d: %s' % (t, r.returncode, ' '.join(
                    l for l in r.stdout.splitlines() if not l.startswith(('native', 'replay', 'fallback')))))
                break
            ours = open(paths[2], 'rb').read()
            reason = excluded.excluder(post)
            bad = [(a, n) for a, n in diff.state_file_diff(post, ours) if not reason(a)]
            day = (g.i32(0x0078D7BC), g.i32(0x0078D7C0), g.i32(0x0078D7C4), g.i32(0x0078D7CC))
            if not bad:
                if verbose:
                    print('tick %d (y %d m %d d %d t %d): equal, %d calls (%d native)'
                          % ((t,) + day + (len(records) - 1, len(natives))), flush=True)
                continue
            failed += 1
            print('tick %d (y %d m %d d %d t %d): %d bytes differ' % ((t,) + day + (sum(n for _, n in bad),)))
            for a, n in bad[:10]:
                who = [c for lo, hi, c in spans if lo <= a < hi]
                print('    %s, %d bytes; last written by %s' % (
                    diff.name_address(a), n, ('0x%08X %s' % (who[-1], 'native' if who[-1] in natives else 'replayed'))
                    if who else 'the skeleton itself (inline code)'), flush=True)
    if report:
        print('callee       calls      bytes  status')
        for c in sorted(calls_seen, key=lambda c: -grand.get(c, 0)):
            w = where(c)
            print('0x%08X %7d %10d  %-8s %s' % (c, calls_seen[c], grand.get(c, 0),
                                               'native' if c in native_seen else 'replayed', w.split()[1] if w else ''))
    total = native_bytes + replay_bytes
    print('lockstep: %d ticks, %d with differences; native ports wrote %d of %d bytes (%.1f %%)' % (
        ticks, failed, native_bytes, total, 100.0 * native_bytes / total if total else 0))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
