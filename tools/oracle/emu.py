"""Imhotep - run functions of the reference Pharaoh.exe in a CPU emulator (the test oracle).

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

The original cannot be run as a program here, but its simulation functions are plain x86 code that
reads and writes global variables. This module maps Pharaoh.exe (GOG 2.1) at its image base, seeds
the globals from a saved game (the memory image written by `build/save_dump --image`), calls a
function by address and lets the caller read memory back. Calls into DLLs land on stubs: the ones
listed in STUBS are emulated, any other one stops the run with its name, so nothing is ignored
silently.

Needs Unicorn (`make oracle-env`, installed into local/venv). Nothing from the game leaves local/.
"""
import os
import struct

from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESP,
                               UC_X86_REG_EBP, UC_X86_REG_EIP, UC_X86_REG_FS, UC_X86_REG_GDTR,
                               UC_X86_REG_SS, UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_GS)

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GAME_DIR = os.environ.get('IMHOTEP_GAME_DIR', os.path.join(ROOT, 'local', 'game', 'app'))

PAGE = 0x1000
STUB_BASE = 0x7F000000   # one 16-byte slot per import
STACK_TOP = 0x7E000000
STACK_SIZE = 0x100000
HEAP_BASE = 0x60000000   # simulation heap (core/state/state.h STATE_HEAP_BASE): see game.py
HEAP_SIZE = 0x08000000
OTHER_HEAP_BASE = 0x68000000  # every other allocation (graphics, UI, CRT): not part of the state
OTHER_HEAP_SIZE = 0x10000000
GDT = 0x7EE00000
TIB = 0x7EF00000         # thread information block (fs:[0] is the SEH chain head)
HOOK_BASE = 0x7F100000  # one 16-byte trap slot per replaced game function
RETURN = 0x7EFFFFF0      # sentinel return address of a call


class StubCall(Exception):
    """A call into a DLL function that has no emulation."""


def _align(n, a=PAGE):
    return (n + a - 1) & ~(a - 1)


class Pharaoh:
    def __init__(self, exe=None):
        data = open(exe or os.path.join(GAME_DIR, 'Pharaoh.exe'), 'rb').read()
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        pe = struct.unpack_from('<I', data, 0x3C)[0]
        nsec = struct.unpack_from('<H', data, pe + 6)[0]
        opt = struct.unpack_from('<H', data, pe + 20)[0]
        self.base = struct.unpack_from('<I', data, pe + 24 + 28)[0]
        image_size = struct.unpack_from('<I', data, pe + 24 + 56)[0]
        self.uc.mem_map(self.base, _align(image_size))
        self.uc.mem_write(self.base, data[:PAGE])
        self.sections = []
        sec = pe + 24 + opt
        for i in range(nsec):
            name, vsize, va, rsize, raw = struct.unpack_from('<8sIIII', data, sec + 40 * i)
            self.uc.mem_write(self.base + va, data[raw:raw + min(rsize, vsize or rsize)])
            self.sections.append((name.rstrip(b'\0').decode(), self.base + va, vsize))
        self.uc.mem_map(STACK_TOP - STACK_SIZE, STACK_SIZE + PAGE)
        self.uc.mem_map(HEAP_BASE, HEAP_SIZE)
        self.uc.mem_map(OTHER_HEAP_BASE, OTHER_HEAP_SIZE)
        self.heap = HEAP_BASE
        self.other_heap = OTHER_HEAP_BASE
        self.sizes = {}  # allocation sizes (realloc)
        self.uc.mem_map(TIB, PAGE)
        self.uc.mem_write(TIB, struct.pack('<III', 0xFFFFFFFF, STACK_TOP, STACK_TOP - STACK_SIZE))
        self.uc.mem_write(TIB + 0x18, struct.pack('<I', TIB))
        self._segments()
        self.imports = {}  # stub address -> 'DLL!name'
        self._bind_imports(data, pe, opt, nsec)
        self.uc.hook_add(UC_HOOK_CODE, self._on_stub, begin=STUB_BASE, end=STUB_BASE + 0x10000)
        self.uc.hook_add(UC_HOOK_MEM_UNMAPPED, self._on_unmapped)
        self.uc.mem_map(HOOK_BASE, 0x10000)
        self.uc.hook_add(UC_HOOK_CODE, self._on_hook, begin=HOOK_BASE, end=HOOK_BASE + 0x10000)
        self.handlers = dict(STUBS)
        self.stub_log = []
        self.ail_log = []  # MSS32 calls: (name, args, result, return address, caller's frame args, recorded call)
        self.fault = None
        self.hooks = {}

    def hook(self, addr, fn):
        """Replace the game function at addr (cdecl) by fn(emu) -> eax; fn reads its arguments
        with emu.arg(i). The function's first bytes become a jump to a trap slot, so only calls to
        it reach Python."""
        slot = HOOK_BASE + 16 * len(self.hooks)
        self.uc.mem_write(slot, b'\xC3')
        self.uc.mem_write(addr, b'\xE9' + struct.pack('<i', slot - (addr + 5)))
        self.hooks[slot] = fn

    def arg(self, i):
        return self.u32(self.uc.reg_read(UC_X86_REG_ESP) + 4 + 4 * i)

    def _on_hook(self, uc, addr, size, _):
        fn = self.hooks.get(addr)
        if fn is None:
            return
        try:
            result = fn(self)
        except Exception as e:  # noqa: BLE001 - reported as the run's fault
            self.fault = e
            uc.emu_stop()
            return
        esp = uc.reg_read(UC_X86_REG_ESP)
        uc.reg_write(UC_X86_REG_EAX, (result or 0) & 0xFFFFFFFF)
        uc.reg_write(UC_X86_REG_ESP, esp + 4)  # cdecl: caller pops the arguments
        uc.reg_write(UC_X86_REG_EIP, self.u32(esp))

    def _segments(self):
        """Flat 32-bit data segments for ss, ds, es, gs, and a segment based at the TIB for fs."""
        def entry(base, limit, access):
            return struct.pack('<HHBBBB', limit & 0xFFFF, base & 0xFFFF, (base >> 16) & 0xFF, access,
                               ((limit >> 16) & 0xF) | 0xC0, (base >> 24) & 0xFF)  # 4 KiB units, 32-bit

        self.uc.mem_map(GDT, PAGE)
        self.uc.mem_write(GDT + 8 * 2, entry(0, 0xFFFFF, 0x93))  # data, ring 0, read/write
        self.uc.mem_write(GDT + 8 * 3, entry(TIB, 0, 0x93))      # one page at the TIB
        self.uc.reg_write(UC_X86_REG_GDTR, (0, GDT, 8 * 4 - 1, 0))
        for reg in (UC_X86_REG_SS, UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_GS):
            self.uc.reg_write(reg, 2 << 3)
        self.uc.reg_write(UC_X86_REG_FS, 3 << 3)

    # --- imports -------------------------------------------------------------------------------
    def _bind_imports(self, data, pe, opt, nsec):
        sec = pe + 24 + opt

        def off(rva):
            for i in range(nsec):
                _, vsize, va, rsize, raw = struct.unpack_from('<8sIIII', data, sec + 40 * i)
                if va <= rva < va + max(vsize, rsize):
                    return rva - va + raw
            raise ValueError(hex(rva))

        self.uc.mem_map(STUB_BASE, 0x10000)
        o = off(struct.unpack_from('<I', data, pe + 24 + 104)[0])
        slot = 0
        while True:
            ilt, _, _, name_rva, iat = struct.unpack_from('<5I', data, o)
            if name_rva == 0:
                break
            no = off(name_rva)
            dll = data[no:data.index(b'\0', no)].decode().upper()
            t = off(ilt or iat)
            k = 0
            while True:
                entry = struct.unpack_from('<I', data, t + 4 * k)[0]
                if entry == 0:
                    break
                if entry & 0x80000000:
                    fname = '#%d' % (entry & 0xFFFF)
                else:
                    fo = off(entry) + 2
                    fname = data[fo:data.index(b'\0', fo)].decode()
                addr = STUB_BASE + 16 * slot
                self.uc.mem_write(addr, b'\xC3')
                self.uc.mem_write(self.base + iat + 4 * k, struct.pack('<I', addr))
                self.imports[addr] = '%s!%s' % (dll, fname)
                slot += 1
                k += 1
            o += 20

    def _on_stub(self, uc, addr, size, _):
        name = self.imports.get(addr)
        if name is None:
            return
        fname = name.split('!', 1)[1]
        handler = self.handlers.get(fname)
        if handler is None and name.split('!')[0] in PRESENTATION_DLLS and '@' in fname:
            # audio / video libraries: stdcall names carry the argument size; they only present
            handler = (int(fname.rsplit('@', 1)[1]) // 4, lambda e, *a: 0)
        if handler is None:
            self.fault = StubCall('%s called from 0x%08X' % (name, self.u32(uc.reg_read(UC_X86_REG_ESP))))
            uc.emu_stop()
            return
        nargs, fn = handler
        esp = uc.reg_read(UC_X86_REG_ESP)
        ret = self.u32(esp)
        args = [self.u32(esp + 4 + 4 * i) for i in range(nargs)]
        self.stub_log.append(fname)
        try:
            result = fn(self, *args)
        except Exception as e:  # noqa: BLE001 - reported as the run's fault
            self.fault = e
            uc.emu_stop()
            return
        if name.startswith('MSS32.DLL!'):
            # the sound driver's calls (docs/oracle.md "Sound"): name, arguments, result, the caller's
            # return address and the first three stack arguments of the calling function's frame (the
            # Miles wrappers 0x0055F32F..0x0055F639 keep ebp frames: their channel is there)
            ebp = uc.reg_read(UC_X86_REG_EBP)
            try:
                frame = tuple(self.u32(ebp + 8 + 4 * i) for i in range(3))
            except UcError:
                frame = (0, 0, 0)
            rec = getattr(self, 'deep_recorder', None)  # replay.py: the recorded call it is made in
            scope = rec.pending[0] if rec is not None and rec.pending is not None else None
            self.ail_log.append((fname.split('@')[0].lstrip('_'), tuple(args), (result or 0) & 0xFFFFFFFF, ret, frame,
                                 scope))
        uc.reg_write(UC_X86_REG_EAX, (result or 0) & 0xFFFFFFFF)
        uc.reg_write(UC_X86_REG_ESP, esp + 4 + 4 * nargs)  # stdcall: callee pops
        uc.reg_write(UC_X86_REG_EIP, ret)

    def _on_unmapped(self, uc, access, addr, size, value, _):
        self.fault = MemoryError('unmapped access at 0x%08X (eip 0x%08X)' % (addr, uc.reg_read(UC_X86_REG_EIP)))
        return False

    def stub(self, nargs, fn, name):
        """A new stdcall stub past the imports (a COM method, ...): its address. fn(emu, *args) ->
        eax, or a constant. Calls are logged in stub_log under `name`."""
        slot = len(self.imports)
        addr = STUB_BASE + 16 * slot
        if slot >= 0x10000 // 16:
            raise MemoryError('no stub slot left')
        self.uc.mem_write(addr, b'\xC3')
        self.imports[addr] = 'COM!' + name
        if not callable(fn):
            value = fn
            fn = lambda e, *a: value  # noqa: E731
        self.handlers[name] = (nargs, fn)
        return addr

    @staticmethod
    def com_method(methods, label, off):
        """(argument count, handler) of the vtable entry `off` of a COM object (com_object)"""
        if off in methods:
            return methods[off]

        def fn(e, *a):
            raise RuntimeError('%s: method +0x%X is not emulated' % (label, off))
        return 1, fn

    def com_object(self, methods, label):
        """An emulated COM object: a pointer to a vtable of 64 stdcall stubs. methods: {vtable
        offset: (argument count with `this`, handler or constant)}; any other method stops the run."""
        table = self.alloc(0x100)
        for off in range(0, 0x100, 4):
            n, fn = self.com_method(methods, label, off)
            self.set_u32(table + off, self.stub(n, fn, '%s+%X' % (label, off)))
        obj = self.alloc(16)
        self.set_u32(obj, table)
        return obj

    # --- memory --------------------------------------------------------------------------------
    def read(self, addr, n):
        return bytes(self.uc.mem_read(addr, n))

    def write(self, addr, data):
        self.uc.mem_write(addr, bytes(data))

    def u32(self, addr):
        return struct.unpack('<I', self.read(addr, 4))[0]

    def i32(self, addr):
        return struct.unpack('<i', self.read(addr, 4))[0]

    def set_u32(self, addr, v):
        self.write(addr, struct.pack('<I', v & 0xFFFFFFFF))

    def cstr(self, addr, limit=4096):
        b = self.read(addr, limit)
        return b[:b.index(0)].decode('latin-1')

    def alloc(self, n, sim=False):
        """Bump allocation, 16-byte steps, zero-filled memory never reused. sim=True: in the
        simulation heap (the port's arena, core/state/state.c state_alloc, does the same)."""
        if sim:
            p = self.heap
            self.heap += _align(max(n, 1), 16)
            limit = HEAP_BASE + HEAP_SIZE
            top = self.heap
        else:
            p = self.other_heap
            self.other_heap += _align(max(n, 1), 16)
            limit = OTHER_HEAP_BASE + OTHER_HEAP_SIZE
            top = self.other_heap
        self.sizes[p] = n
        if top > limit:
            raise MemoryError('emulated heap exhausted')
        return p

    def load_image(self, path):
        """Write the chunks of a `save_dump --image` file to their variables."""
        data = open(path, 'rb').read()
        o = 0
        regions = []
        while o < len(data):
            addr, n = struct.unpack_from('<II', data, o)
            self.write(addr, data[o + 8:o + 8 + n])
            regions.append((addr, n))
            o += 8 + n
        return regions

    def fpu_control(self, word):
        """Load the x87 control word (fldcw), as the CRT start-up does."""
        code = self.alloc(16)
        data = self.alloc(4)
        self.write(data, struct.pack('<H', word))
        self.write(code, b'\xD9\x2D' + struct.pack('<I', data) + b'\xC3')  # fldcw [data]; ret
        self.call(code)

    # --- calls ---------------------------------------------------------------------------------
    def call(self, addr, *args, ecx=None, timeout=0):
        """Call a cdecl (or thiscall, with ecx) function; returns eax. timeout in seconds, 0 = none
        (an instruction count limit would instrument every instruction)."""
        esp = STACK_TOP - 0x100
        for a in reversed(args):
            esp -= 4
            self.set_u32(esp, a)
        esp -= 4
        self.set_u32(esp, RETURN)
        self.uc.reg_write(UC_X86_REG_ESP, esp)
        self.uc.reg_write(UC_X86_REG_EBP, 0)
        if ecx is not None:
            self.uc.reg_write(UC_X86_REG_ECX, ecx)
        self.fault = None
        try:
            self.uc.emu_start(addr, RETURN, timeout=int(timeout * 1_000_000))
        except UcError as e:
            if self.fault is None:
                raise RuntimeError('emulation error %s at eip 0x%08X' % (e, self.uc.reg_read(UC_X86_REG_EIP)))
        if self.fault is not None:
            raise RuntimeError('stopped in 0x%08X: %s' % (addr, self.fault))
        if self.uc.reg_read(UC_X86_REG_EIP) != RETURN:
            raise RuntimeError('0x%08X did not return (timeout?)' % addr)
        return self.uc.reg_read(UC_X86_REG_EAX)


PRESENTATION_DLLS = {'MSS32.DLL', 'BINKW32.DLL', 'SMACKW32.DLL'}


# --- emulated DLL functions: name -> (stdcall argument count, handler) ----------------------------
def _heap_alloc(emu, heap, flags, n):
    p = emu.alloc(n)
    if flags & 8:
        emu.write(p, bytes(n))
    return p


def _message_box(emu, hwnd, text, caption, kind):
    raise RuntimeError('MessageBox: %s / %s' % (emu.cstr(caption), emu.cstr(text)))


def _memory_status(emu, p):
    # MEMORYSTATUS: length, load, total/avail physical, page file, virtual (256 MiB each)
    emu.write(p, struct.pack('<8I', 32, 0, 1 << 28, 1 << 28, 1 << 28, 1 << 28, 1 << 28, 1 << 28))


STUBS = {
    'GlobalMemoryStatus': (1, _memory_status),
    'MessageBoxA': (4, _message_box),
    'GetProcessHeap': (0, lambda e: 0x1),
    'HeapAlloc': (3, _heap_alloc),
    'HeapFree': (3, lambda e, h, f, p: 1),
    'GlobalAlloc': (2, lambda e, flags, n: _heap_alloc(e, 0, 8 if flags & 0x40 else 0, n)),
    'GlobalFree': (1, lambda e, p: 0),
    'GlobalLock': (1, lambda e, p: p),
    'GlobalUnlock': (1, lambda e, p: 1),
    'VirtualAlloc': (4, lambda e, a, n, t, p: _heap_alloc(e, 0, 8, n)),
    'VirtualFree': (3, lambda e, a, n, t: 1),
    'GetTickCount': (0, lambda e: 0),
    'SetCurrentDirectoryA': (1, lambda e, p: 1),
    'CreateDirectoryA': (2, lambda e, p, a: 0),
    'FindFirstFileA': (2, lambda e, p, d: 0xFFFFFFFF),  # nothing found; game.py Game lists its files
    'FindClose': (1, lambda e, h: 1),
    'GetLastError': (0, lambda e: 0),
    'SetLastError': (1, lambda e, v: 0),
    'Sleep': (1, lambda e, ms: 0),
    'OutputDebugStringA': (1, lambda e, p: 0),  # debugger output (figure actions 0x004A98B0, ...)
    'GetSystemMetrics': (1, lambda e, i: {0: 1024, 1: 768}.get(i, 0)),
    'GetCurrentDirectoryA': (2, lambda e, n, p: (e.write(p, b'C:\\\0'), 3)[1]),
    'timeGetTime': (0, lambda e: 0),
    # the profiler's timer (0x00489B40): no performance counter, as on hardware without one
    'CoUninitialize': (0, lambda e: 0),  # COM shut down at quit (0x004CFF50)
    'QueryPerformanceFrequency': (1, lambda e, p: (e.write(p, bytes(8)), 0)[1]),
    'QueryPerformanceCounter': (1, lambda e, p: (e.write(p, bytes(8)), 0)[1]),
}
