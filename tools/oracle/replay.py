"""Imhotep - deep replay: record what every unported function did inside an original call.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

A ported function is checked against the original on the same state (harness.py, lockstep.py).
While functions are being ported, a port calls original functions that have no port yet (through
sim_call, core/sim/call.h). So the original's run records, in order, every call it makes to a
function that is not ported, at any depth: its arguments, its return value (eax), and everything
it changed in the state (data-segment bytes it wrote, the simulation heap, new heap blocks). On the
port's side (build/state_replay) a ported function runs natively and each sim_call to an
unported function must be the next recorded call (same address): it gets the recorded changes
and return value. Only the outermost unported call is recorded; what it calls is part of it.

Partial ports (ported for some arguments only, `partial` 1 in build/state_run --list) and platform
requests (`partial` 2: video, sound; core/sim/platform.h) are recorded
too: the port side runs them natively and falls back to the record when they reach unported code.

REPLAY file (build/state_replay):
  u32 count, then records:
    top call:      u32 address, u32 0 (kind), u32 nargs, i32 args[nargs], u32 this
    recorded call: u32 address, u32 1 (kind), i32 eax, u32 nargs, i32 args[nargs], u32 flags,
                   u32 heap_used, u32 new blocks, (u32 offset, u32 size)..., u32 ranges,
                   (u32 address, u32 length, bytes)...
  flags: 1 = the call wrote into its callers' stack frames (an output buffer: not replayable).
"""
import os
import struct
import subprocess

from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP

from emu import HEAP_BASE, RETURN, STACK_TOP, STACK_SIZE
from where import INDEX

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
STATE_BASE, STATE_END = 0x0056F000, 0x01267AA8
NARGS = 8        # stack dwords recorded as arguments
ARG_AREA = 0x40  # writes this far above the return address are the callee's own arguments


def ported_table():
    """{address: (nargs, return mask, name, this, partial)} from build/state_run --list."""
    r = subprocess.run([os.path.join(ROOT, os.environ.get('IMHOTEP_BUILD', 'build'), 'state_run'), '--list'], capture_output=True, text=True,
                       check=True)
    out = {}
    for line in r.stdout.splitlines():
        a, n, bits, this, partial, name = line.split(None, 5)
        out[int(a, 16)] = (int(n), (1 << int(bits)) - 1, name, int(this, 16), partial in ('1', '2'))  # recorded as units
    return out


class DeepRecorder:
    """Records the unported calls made inside top-level calls of the reference game `g`.
    native: functions that run natively on the port's side (not recorded)."""

    def __init__(self, g, native):
        self.g = g
        self.native = set(native)
        self.pending = None   # the recorded call in progress: [address, return, esp, record parts]
        self.records = []
        self.active = False
        self.skip_entry = None
        self.writes = []
        self.ret_hooks = set()
        self.rec_count = {}  # address -> recorded calls (all takes)
        self.rec_bytes = {}  # address -> bytes written by them
        self.spans = []      # (address, first byte, length) of the records since the last take
        for f in INDEX:
            a = int(f['address'], 16)
            if a not in self.native:
                g.uc.hook_add(UC_HOOK_CODE, self.on_entry, begin=a, end=a)
        self.write_hooks = None  # installed only while a call is being recorded (they are costly)
        g.uc.ctl_flush_tb()  # code translated before (boot, load) would not see the new hooks

    # --- hooks ---------------------------------------------------------------------------------
    def on_entry(self, uc, addr, size, _):
        if not self.active or self.pending is not None:
            return
        if self.skip_entry == addr:  # the top call itself
            self.skip_entry = None
            return
        esp = uc.reg_read(UC_X86_REG_ESP)
        ret = self.g.u32(esp)
        args = struct.unpack('<%di' % NARGS, self.g.read(esp + 4, 4 * NARGS))
        g = self.g
        heap_before = g.read(HEAP_BASE, g.heap - HEAP_BASE)
        blocks_before = set(p for p in g.sizes if HEAP_BASE <= p < g.heap)
        self.pending = [addr, ret, esp, args, heap_before, blocks_before, 0]
        self.write_hooks = (uc.hook_add(UC_HOOK_MEM_WRITE, self.on_write, begin=STATE_BASE, end=STATE_END - 1),
                            uc.hook_add(UC_HOOK_MEM_WRITE, self.on_stack_write, begin=STACK_TOP - STACK_SIZE,
                                        end=STACK_TOP))
        self.writes = []
        if ret not in self.ret_hooks:
            self.ret_hooks.add(ret)
            uc.hook_add(UC_HOOK_CODE, self.on_return, begin=ret, end=ret)
            uc.ctl_remove_cache(ret, ret + 1)  # the caller's block may be translated already

    def on_return(self, uc, addr, size, _):
        p = self.pending
        if p is not None and p[1] == addr and uc.reg_read(UC_X86_REG_ESP) > p[2]:
            self.finish(uc.reg_read(UC_X86_REG_EAX))

    def on_write(self, uc, access, addr, size, value, _):
        if self.pending is not None:
            self.writes.append((addr, size))

    def on_stack_write(self, uc, access, addr, size, value, _):
        p = self.pending
        if p is not None and addr >= p[2] + ARG_AREA and addr < STACK_TOP:
            p[6] = 1  # into a caller's frame

    def finish(self, eax):
        g = self.g
        addr, _, _, args, heap_before, blocks_before, flags = self.pending
        self.pending = None
        for h in self.write_hooks:
            g.uc.hook_del(h)
        self.write_hooks = None
        spans = sorted(set(self.writes))
        ranges = []
        for a, n in spans:
            if ranges and a <= ranges[-1][1]:
                ranges[-1][1] = max(ranges[-1][1], a + n)
            else:
                ranges.append([a, a + n])
        heap_now = g.read(HEAP_BASE, g.heap - HEAP_BASE)
        heap_ranges = diff_bytes(heap_before, heap_now, HEAP_BASE)
        new_blocks = sorted((p - HEAP_BASE, s) for p, s in g.sizes.items()
                            if HEAP_BASE <= p < g.heap and p not in blocks_before)
        chunk = [struct.pack('<IIiI', addr, 1, eax_signed(eax), NARGS), struct.pack('<%di' % NARGS, *args),
                 struct.pack('<III', flags, g.heap - HEAP_BASE, len(new_blocks))]
        chunk += [struct.pack('<II', o, s) for o, s in new_blocks]
        all_ranges = [(a, b - a) for a, b in ranges] + heap_ranges
        chunk.append(struct.pack('<I', len(all_ranges)))
        for a, n in all_ranges:
            chunk.append(struct.pack('<II', a, n) + g.read(a, n))
        self.records.append(b''.join(chunk))
        self.rec_count[addr] = self.rec_count.get(addr, 0) + 1
        self.rec_bytes[addr] = self.rec_bytes.get(addr, 0) + sum(n for _, n in all_ranges)
        self.spans += [(addr, a, n) for a, n in all_ranges]

    # --- top-level calls -----------------------------------------------------------------------
    def call(self, addr, args=(), this=None):
        """Run the original addr(args) and record its unported calls; returns eax."""
        args = [eax_signed(a) for a in args]
        self.records.append(struct.pack('<III', addr, 0, len(args)) + struct.pack('<%di' % len(args), *args) +
                            struct.pack('<I', this or 0))
        self.active = True
        self.skip_entry = addr
        try:
            eax = self.g.call(addr, *[a & 0xFFFFFFFF for a in args], ecx=this)
        finally:
            self.active = False
            self.skip_entry = None
        if self.pending is not None:  # a tail jump to an unported function: it returned to our sentinel
            self.finish(self.g.uc.reg_read(UC_X86_REG_EAX))
        return eax

    def take(self):
        """The REPLAY file of the calls since the last take."""
        out = struct.pack('<I', len(self.records)) + b''.join(self.records)
        self.records = []
        self.spans = []
        return out


TEXT_POINTER = 0x00E36C70  # the text buffer of Pharaoh_Text.eng from byte 0x1F5C (loader 0x004CEC40)
TEXT_SKIP = 0x1F5C


def external_maps(g, tmp):
    """state_replay --map options for the game data the original keeps outside its segments"""
    base = g.u32(TEXT_POINTER)
    path = os.path.join(tmp, 'text.bin')
    if base and not os.path.exists(path):
        from game import GAME_DIR
        size = os.path.getsize(os.path.join(GAME_DIR, 'Pharaoh_Text.eng')) - TEXT_SKIP
        open(path, 'wb').write(g.read(base, size))
    return ['--map', '%08X:%s' % (base, path)] if base else []


def eax_signed(v):
    v &= 0xFFFFFFFF
    return v - (1 << 32) if v & 0x80000000 else v


def diff_bytes(a, b, base):
    """[(address, length)] where b differs from a (b may be longer; a missing tail counts as 0)."""
    a = a.ljust(len(b), b'\0')
    out = []
    block = 4096
    for o in range(0, len(b), block):
        x, y = a[o:o + block], b[o:o + block]
        if x == y:
            continue
        k = 0
        while k < len(y):
            if x[k] != y[k]:
                s = k
                while k < len(y) and x[k] != y[k]:
                    k += 1
                out.append((base + o + s, k - s))
            else:
                k += 1
    return out


__all__ = ['DeepRecorder', 'ported_table', 'RETURN']
