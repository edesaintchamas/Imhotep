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
Their own unported calls (the outermost ones inside them) are recorded as nested records, which the
native run consumes and a fallback skips.

REPLAY file (build/state_replay):
  u32 count, then records:
    top call:      u32 address, u32 0 (kind), u32 nargs, i32 args[nargs], u32 this
    recorded call: u32 address, u32 1 (kind), i32 eax, u32 nargs, i32 args[nargs], u32 flags,
                   u32 nested, u32 heap_used, u32 other_top, u32 new blocks, (u32 offset, u32 size)...,
                   u32 ranges, (u32 address, u32 length, bytes)...
  flags: 1 = the call wrote into its callers' stack frames (an output buffer: not replayable).
  nested: the number of records that follow and belong to this one (a partial port's or platform
  request's outermost unported calls, each with its own nested records): state_replay consumes them
  when the port runs natively, and skips them when it replays the record.
  other_top: the top of the oracle's other heap after the call (its bump allocator, emu.py alloc):
  state_replay's platform memory (--other-heap) allocates where the original does.

With a display (tools/oracle/game.py Game(display=...)), the recorded changes also cover the
drawing buffers outside the state (display_buffers: the back buffer [0x00E92E18] and the spare
screen [0x00E92E1C]): while a recorded call runs, their pages are write-protected; the first write to
a page saves the page and unprotects it (the write itself is made by the hook, as Unicorn drops a
write it reports on a protected page), and at the end of the call the saved pages are compared with
their new bytes. The ranges are recorded like state writes, at their addresses; state_replay applies
them to its copies of the buffers (--screen, --rw).

A top-level call to a function that is not ported, made with call(..., whole=True), is recorded as a
whole: its own record follows the top call's, and state_replay replays it
(lockstep --frames: the frame 0x004D6B60 and the window procedure before their ports exist).
"""
import os
import struct
import subprocess

from unicorn import (UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_WRITE_PROT, UC_PROT_ALL, UC_PROT_READ,
                     UC_PROT_EXEC)
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP

from emu import HEAP_BASE, OTHER_HEAP_BASE, RETURN, STACK_TOP, STACK_SIZE
from where import INDEX

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
STATE_BASE, STATE_END = 0x0056F000, 0x01267AA8
NARGS = 8        # stack dwords recorded as arguments
ARG_AREA = 0x40  # writes this far above the return address are the callee's own arguments
PAGE = 0x1000
SPARE_SCREEN = 0x00E92E1C  # u32 pointer: the spare screen, a work buffer the game allocates (1.2 MB,
SPARE_SCREEN_SIZE = 100000  # a screen's size with a display); its size is the block's (g.sizes), this the fallback


def display_buffers(g):
    """[(address, size)] of the drawing buffers outside the state that recorded calls may write: the
    back buffer (0x00E92E18, pitch * height) and the spare screen (0x00E92E1C, the whole block the
    game allocated: external images up to a screen's size are read into it, 0x004CE480); [] without
    a display"""
    info = g.screen_info() if getattr(g, 'display', None) is not None else None
    if info is None:
        return []
    out = [(info[0], info[3] * info[2])]
    if g.u32(SPARE_SCREEN):
        out.append((g.u32(SPARE_SCREEN), g.sizes.get(g.u32(SPARE_SCREEN), SPARE_SCREEN_SIZE)))
    return out


def ported_table():
    """{address: (nargs, return mask, name, this, partial, kind)} from build/state_run --list (partial:
    recorded as a unit; kind 1 a partial port, 2 a platform request, 0 neither)."""
    r = subprocess.run([os.path.join(ROOT, os.environ.get('IMHOTEP_BUILD', 'build'), 'state_run'), '--list'], capture_output=True, text=True,
                       check=True)
    out = {}
    for line in r.stdout.splitlines():
        a, n, bits, this, partial, name = line.split(None, 5)
        out[int(a, 16)] = (int(n), (1 << int(bits)) - 1, name, int(this, 16), partial in ('1', '2'),  # recorded as units
                           int(partial))
    return out


class Level:
    """a recorded call in progress (DeepRecorder): an unported function entered inside a top-level call,
    or, inside a partial port's record, one of the outermost unported calls it makes"""
    __slots__ = ('addr', 'ret', 'esp', 'args', 'heap_before', 'blocks_before', 'flags', 'wstart', 'dirty',
                 'buffers', 'slot')

    def __getitem__(self, k):  # rec.pending[0]: the recorded function (emu.py's sound log)
        return self.addr if k == 0 else None


class DeepRecorder:
    """Records the unported calls made inside top-level calls of the reference game `g`.
    native: functions that run natively on the port's side (not recorded). Inside the record of a
    partial port or a platform request (partial 1 or 2 in build/state_run --list), the outermost
    unported calls it makes are recorded too, as its nested records: state_replay consumes them when
    the partial port runs natively and skips them when it falls back to the record."""

    def __init__(self, g, native, partial=None):
        self.g = g
        g.deep_recorder = self  # the sound driver's log (emu.py ail_log) notes the recorded call in progress
        self.native = set(native)
        if partial is None:
            partial = {a for a, t in ported_table().items() if t[4]}
        self.partial = set(partial) - self.native
        self.levels = []      # the recorded calls in progress, outermost first (Level)
        self.records = []
        self.active = False
        self.skip_entry = None
        self.writes = []
        self.ret_hooks = set()
        self.rec_count = {}  # address -> recorded calls (all takes; outermost records only)
        self.rec_bytes = {}  # address -> bytes written by them
        self.spans = []      # (address, first byte, length) of the outermost records since the last take
        self.entry_hooks = set()
        for f in INDEX:
            a = int(f['address'], 16)
            if a not in self.native:
                g.uc.hook_add(UC_HOOK_CODE, self.on_entry, begin=a, end=a)
                self.entry_hooks.add(a)
        self.buffers = []    # drawing buffers (display_buffers), recorded through page protection
        self.buffer_pages = []
        g.uc.hook_add(UC_HOOK_MEM_WRITE_PROT, self.on_write_prot)
        self.write_hooks = None  # installed only while a call is being recorded (they are costly)
        g.uc.ctl_flush_tb()  # code translated before (boot, load) would not see the new hooks
        # writes made by Python hooks (CRT _read, file_read_at, ... in game.py) go through
        # uc.mem_write, which UC_HOOK_MEM_WRITE does not see: record them here
        plain_write = g.write

        def write(addr, data):
            if self.levels and data and self.buffer_pages:
                self.save_pages(addr, len(data))
            plain_write(addr, data)
            if not self.levels or not data:
                return
            if STATE_BASE <= addr < STATE_END:
                self.writes.append((addr, len(data)))
            else:
                self.stack_write(addr)
        g.write = write

    @property
    def pending(self):
        """the innermost recorded call in progress (None: none)"""
        return self.levels[-1] if self.levels else None

    # --- hooks ---------------------------------------------------------------------------------
    def on_entry(self, uc, addr, size, _):
        if not self.active:
            return
        if self.levels and self.levels[-1].addr not in self.partial:
            return  # inside a recorded call: part of it
        if not self.levels and self.skip_entry == addr:  # the top call itself
            self.skip_entry = None
            return
        esp = uc.reg_read(UC_X86_REG_ESP)
        ret = self.g.u32(esp)
        g = self.g
        lv = Level()
        lv.addr, lv.ret, lv.esp = addr, ret, esp
        lv.args = struct.unpack('<%di' % NARGS, g.read(esp + 4, 4 * NARGS))
        lv.heap_before = g.read(HEAP_BASE, g.heap - HEAP_BASE)
        lv.blocks_before = set(p for p in g.sizes if HEAP_BASE <= p < g.heap)
        lv.flags = 0
        lv.dirty = {}
        lv.buffers = [a for a, _ in self.buffers]
        lv.slot = len(self.records)
        self.records.append(None)  # filled when the call returns (its nested records follow it)
        if not self.levels:
            self.writes = []
            self.write_hooks = (uc.hook_add(UC_HOOK_MEM_WRITE, self.on_write, begin=STATE_BASE, end=STATE_END - 1),
                                uc.hook_add(UC_HOOK_MEM_WRITE, self.on_stack_write, begin=STACK_TOP - STACK_SIZE,
                                            end=STACK_TOP))
        lv.wstart = len(self.writes)
        self.levels.append(lv)
        self.protect()
        if ret not in self.ret_hooks:
            self.ret_hooks.add(ret)
            uc.hook_add(UC_HOOK_CODE, self.on_return, begin=ret, end=ret)
            uc.ctl_remove_cache(ret, ret + 1)  # the caller's block may be translated already

    def on_return(self, uc, addr, size, _):
        p = self.pending
        if p is not None and p.ret == addr and uc.reg_read(UC_X86_REG_ESP) > p.esp:
            self.finish(uc.reg_read(UC_X86_REG_EAX))

    def on_write(self, uc, access, addr, size, value, _):
        if self.levels:
            self.writes.append((addr, size))

    def stack_write(self, addr):
        for lv in self.levels:
            if lv.esp + ARG_AREA <= addr < STACK_TOP:
                lv.flags = 1  # into a caller's frame

    def on_stack_write(self, uc, access, addr, size, value, _):
        self.stack_write(addr)

    # --- drawing buffers (page protection) -----------------------------------------------------
    def set_buffers(self, buffers):
        self.buffers = list(buffers)
        pages = []
        for a, n in sorted(self.buffers):
            lo, hi = a & ~(PAGE - 1), (a + n + PAGE - 1) & ~(PAGE - 1)
            if pages and lo <= pages[-1][1]:
                pages[-1][1] = max(pages[-1][1], hi)
            else:
                pages.append([lo, hi])
        self.buffer_pages = [(lo, hi) for lo, hi in sorted(pages)]

    def protect(self):
        """every buffer page write-protected: the first write of the innermost recorded call saves it"""
        for lo, hi in self.buffer_pages:
            self.g.uc.mem_protect(lo, hi - lo, UC_PROT_READ | UC_PROT_EXEC)

    def unprotect(self):
        for lo, hi in self.buffer_pages:
            self.g.uc.mem_protect(lo, hi - lo, UC_PROT_ALL)

    def save_pages(self, addr, n):
        """the pages of [addr, addr + n) inside the buffers' pages: saved for every recorded call in
        progress that has not saved them yet, and unprotected"""
        for lo, hi in self.buffer_pages:
            a, b = max(lo, addr & ~(PAGE - 1)), min(hi, addr + n)
            for page in range(a, b, PAGE):
                if any(page not in lv.dirty for lv in self.levels):
                    before = self.g.read(page, PAGE)
                    for lv in self.levels:
                        lv.dirty.setdefault(page, before)
                self.g.uc.mem_protect(page, PAGE, UC_PROT_ALL)

    def on_write_prot(self, uc, access, addr, size, value, _):
        if not self.levels or not self.buffer_pages:
            return False  # a protection fault of the game's own (none expected): stop
        self.save_pages(addr, size)
        uc.mem_write(addr, (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little'))
        return True

    def buffer_ranges(self, lv):
        """[(address, length)] of the buffers' bytes the recorded call lv changed. A buffer that did not
        exist when it started (the back buffer and spare screen made again by a mode change, 0x004D0060,
        0x00414BD0) is recorded whole where it is not zero (state_replay gives the port such a buffer
        zeroed: tools/oracle/harness.py, framestep.py)"""
        out = []
        for page, before in sorted(lv.dirty.items()):
            now = self.g.read(page, PAGE)
            if now == before:
                continue
            for a, n in diff_bytes(before, now, page):
                for lo, size in self.buffers:   # only inside the buffers (a page may hold other data)
                    if lo not in lv.buffers:
                        continue  # recorded whole below
                    x, y = max(a, lo), min(a + n, lo + size)
                    if x < y:
                        if out and out[-1][0] + out[-1][1] == x:
                            out[-1] = (out[-1][0], y - out[-1][0])
                        else:
                            out.append((x, y - x))
        for lo, size in display_buffers(self.g):
            if lo not in lv.buffers:
                out += diff_bytes(b'', self.g.read(lo, size), lo)
        return out

    def finish(self, eax):
        g = self.g
        lv = self.levels.pop()
        if not self.levels:
            for h in self.write_hooks:
                g.uc.hook_del(h)
            self.write_hooks = None
            self.unprotect()
        current = display_buffers(g)
        buffer_ranges = self.buffer_ranges(lv)
        if [a for a, _ in current] != [a for a, _ in self.buffers]:  # new buffers: tracked from now on
            if self.levels:
                self.unprotect()
            self.set_buffers(current)
            if self.levels:  # the outer calls' next writes save the pages they have not saved yet
                self.protect()
        spans = sorted(set(self.writes[lv.wstart:]))
        if not self.levels:
            self.writes = []
        ranges = []
        for a, n in spans:
            if ranges and a <= ranges[-1][1]:
                ranges[-1][1] = max(ranges[-1][1], a + n)
            else:
                ranges.append([a, a + n])
        heap_now = g.read(HEAP_BASE, g.heap - HEAP_BASE)
        heap_ranges = diff_bytes(lv.heap_before, heap_now, HEAP_BASE)
        new_blocks = sorted((p - HEAP_BASE, s) for p, s in g.sizes.items()
                            if HEAP_BASE <= p < g.heap and p not in lv.blocks_before)
        nested = len(self.records) - lv.slot - 1
        chunk = [struct.pack('<IIiI', lv.addr, 1, eax_signed(eax), NARGS), struct.pack('<%di' % NARGS, *lv.args),
                 struct.pack('<IIIII', lv.flags, nested, g.heap - HEAP_BASE, g.other_heap, len(new_blocks))]
        chunk += [struct.pack('<II', o, s) for o, s in new_blocks]
        all_ranges = [(a, b - a) for a, b in ranges] + heap_ranges + buffer_ranges
        chunk.append(struct.pack('<I', len(all_ranges)))
        for a, n in all_ranges:
            chunk.append(struct.pack('<II', a, n) + g.read(a, n))
        self.records[lv.slot] = b''.join(chunk)
        if not self.levels:  # statistics: the outermost records (nested ones are part of them)
            self.rec_count[lv.addr] = self.rec_count.get(lv.addr, 0) + 1
            self.rec_bytes[lv.addr] = self.rec_bytes.get(lv.addr, 0) + sum(n for _, n in all_ranges)
            self.spans += [(lv.addr, a, n) for a, n in all_ranges]

    def abort(self):
        """after an exception in the original's run: no call in progress"""
        if self.write_hooks:
            for h in self.write_hooks:
                self.g.uc.hook_del(h)
        self.write_hooks = None
        del self.records[self.levels[0].slot if self.levels else len(self.records):]
        self.levels = []
        self.unprotect()

    # --- top-level calls -----------------------------------------------------------------------
    def call(self, addr, args=(), this=None, whole=False):
        """Run the original addr(args) and record its unported calls; returns eax. whole: addr itself
        is recorded as one call (for a function that is not ported, or a partial port: it then falls
        back to it), replayed whole by state_replay."""
        args = [eax_signed(a) for a in args]
        self.records.append(struct.pack('<III', addr, 0, len(args)) + struct.pack('<%di' % len(args), *args) +
                            struct.pack('<I', this or 0))
        self.set_buffers(display_buffers(self.g))
        if whole and addr not in self.entry_hooks:  # e.g. the window procedure (not a Ghidra function)
            self.g.uc.hook_add(UC_HOOK_CODE, self.on_entry, begin=addr, end=addr)
            self.entry_hooks.add(addr)
            self.g.uc.ctl_remove_cache(addr, addr + 1)
        self.active = True
        self.skip_entry = None if whole else addr
        try:
            eax = self.g.call(addr, *[a & 0xFFFFFFFF for a in args], ecx=this)
        except BaseException:
            self.abort()
            raise
        finally:
            self.active = False
            self.skip_entry = None
        while self.levels:  # a tail jump to an unported function: it returned to our sentinel
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
MM_POINTER = 0x00E36C74    # the text buffer of Pharaoh_MM.eng (messages, their video names) from byte 0x13898
MM_SKIP = 0x13898


def mark_inputs(g):
    """remember the scripted keys and the other heap's top before a run of the original
    (external_maps passes these)"""
    g.input_mark = (set(getattr(g, 'keys_down', ())), set(getattr(g, 'keys_pressed', ())))
    g.other_heap_mark = g.other_heap


def external_maps(g, tmp):
    """state_replay --map options for the game data the original keeps outside its segments, and
    the game directory for the files the ports read through the platform (platform_file_read:
    the .sg3 indexes of 0x0053DFF0), the same files the original reads here (game.py). With a
    display, also the oracle's other heap as it is now (after the original's run), read-only: the
    pixel data the image entries point to (the .555 buffers of 0x0053F660, the fonts, the external
    images) at the original's addresses (docs/formats/state.md "Game data outside the segments")."""
    from game import GAME_DIR
    out = []
    if getattr(g, 'display', None) is not None:
        path = os.path.join(tmp, 'other_heap.bin')
        open(path, 'wb').write(g.read(OTHER_HEAP_BASE, g.other_heap - OTHER_HEAP_BASE))
        out += ['--map', '%08X:%s' % (OTHER_HEAP_BASE, path)]
    for pointer, name, skip, file in ((TEXT_POINTER, 'Pharaoh_Text.eng', TEXT_SKIP, 'text.bin'),
                                      (MM_POINTER, 'Pharaoh_MM.eng', MM_SKIP, 'mm.bin')):
        base = g.u32(pointer)
        path = os.path.join(tmp, file)
        if base and not os.path.exists(path):
            open(path, 'wb').write(g.read(base, os.path.getsize(os.path.join(GAME_DIR, name)) - skip))
        if base:
            out += ['--map', '%08X:%s' % (base, path)]
    out += ['--map', 'files=' + GAME_DIR]
    if getattr(g, 'display', None) is not None and getattr(g, 'other_heap_mark', None):
        # the platform's memory and display modes (sim_platform.memory, set_display) at the addresses
        # the original's allocations take: its other heap's top before the run (mark_inputs)
        out += ['--other-heap', '0x%08X' % g.other_heap_mark]
        g.other_heap_mark = None
    if getattr(g, 'bink', None):  # BinkOpen opens a video of this size (game.py Game.bink): video_open too
        out += ['--video', '%d,%d' % tuple(g.bink)]
    if getattr(g, 'clock', 0):  # the wall clock the original reads (Game.clock): the port's platform_clock_ms
        out += ['--clock', str(g.clock & 0xFFFFFFFF)]
    if getattr(g, 'display', None) is not None:  # the scripted cursor and keys (GetCursorPos, GetAsyncKeyState)
        out += ['--cursor', '%d,%d' % tuple(g.cursor)]
        # the keys as they were before the original's run (mark_inputs): its GetAsyncKeyState calls
        # consume the "pressed since the last call" bits, which the port must see too
        down, pressed = getattr(g, 'input_mark', None) or (g.keys_down, g.keys_pressed)
        if down or pressed:
            out += ['--keys', '%s/%s' % (','.join(map(str, sorted(down))), ','.join(map(str, sorted(pressed))))]
    return out


def capture_buffers(g):
    """[(address, bytes)] of the drawing buffers (display_buffers) as they are now"""
    return [(a, g.read(a, n)) for a, n in display_buffers(g)]


def buffer_args(g, tmp, before, info=None):
    """state_replay --screen / --rw options giving the port the buffers `before` (capture_buffers,
    taken before the original's run; info: g.screen_info() then, the back buffer's address, size and
    format in the input state); returns (options, [(address, output path)]). A drawing buffer the
    original made during its run (a mode change, 0x004D0060: a new back buffer; 0x00414BD0: a new spare
    screen) is given to the port zeroed at its address, where the recorded calls' writes into it are
    applied (replay.py records such a buffer whole) and the port draws: the buffer the state points to
    after the run is compared on both sides (harness.compare_buffers, framestep)"""
    out, outputs = [], []
    if info is None:
        info = g.screen_info() if getattr(g, 'display', None) is not None else None
    known = {a for a, _ in before}
    for k, (a, n) in enumerate(display_buffers(g)):
        if a not in known:
            src, dst = os.path.join(tmp, 'new%d.in' % k), os.path.join(tmp, 'new%d.out' % k)
            open(src, 'wb').write(bytes(n))
            out += ['--rw', '%08X,%s,%s' % (a, src, dst)]
            outputs.append((a, dst))
    for k, (a, data) in enumerate(before):
        src, dst = os.path.join(tmp, 'buffer%d.in' % k), os.path.join(tmp, 'buffer%d.out' % k)
        open(src, 'wb').write(data)
        if info is not None and a == info[0]:
            out += ['--screen', '%d,%d,%d,%d,%s,%s' % (info[1], info[2], info[3], info[4], src, dst)]
        else:
            out += ['--rw', '%08X,%s,%s' % (a, src, dst)]
        outputs.append((a, dst))
    return out, outputs


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


__all__ = ['DeepRecorder', 'ported_table', 'RETURN', 'display_buffers', 'capture_buffers', 'buffer_args']
