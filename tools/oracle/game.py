"""Imhotep - the reference game in the emulator: file access, loading a saved game, game calls.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

The game reads files through the statically linked CRT. Its low-level functions are replaced by
Python ones over a small virtual file system (game files from IMHOTEP_GAME_DIR, plus in-memory
files such as a campaign record), and its heap by the emulator's bump allocator. Everything else
runs as the original code.
"""
import os
import struct

from unicorn.x86_const import UC_X86_REG_ESP

from emu import Pharaoh, GAME_DIR

# CRT and game helpers replaced by Python (addresses in Pharaoh.exe GOG 2.1)
CRT_OPEN = 0x00568EAB      # _open(name, flags[, mode]) -> fd
CRT_READ = 0x005660ED      # _read(fd, buf, n) -> n read
CRT_WRITE = 0x005664D5     # _write(fd, buf, n) -> n written
CRT_CLOSE = 0x005647EB     # _close(fd)
CRT_CHDIR = 0x0056B2DD     # _chdir(path) -> 0
CRT_FILELENGTH = 0x0056B1B6  # _filelength(fd)
CRT_FOPEN = 0x00561A0A     # fopen(name, mode) -> FILE * (campaign.txt 0x0041AE40, Pharaoh.inf, ...)
CRT_FGETS = 0x00561E4D     # fgets(buf, n, FILE *)
CRT_FCLOSE = 0x00561994    # fclose(FILE *)
CRT_FWRITE = 0x00561F5F    # fwrite(buf, size, count, FILE *) (the campaign progress file 0x0041B8A0)
CRT_FREAD = 0x00561CA1     # fread(buf, size, count, FILE *) (the campaign progress file 0x0041B680)
CRT_FSEEK = 0x00561EA4     # fseek(FILE *, offset, whence) (0x0041B680, an older progress file)
CRT_LSEEK = 0x005662E3     # _lseek(fd, offset, whence) -> new position (mission1.pak records, 0x004CE480)
CRT_NEW = 0x00561A1D       # operator new(n)
CRT_DELETE = 0x00561A2B    # operator delete(p)
CRT_MALLOC = 0x00561B68    # malloc(n)
CRT_NH_MALLOC = 0x00561B7A  # _nh_malloc(n, new_handler_flag) (malloc's body)
CRT_FREE = 0x00561BDC      # free(p)
CRT_ATEXIT = 0x00561953    # atexit(fn): the emulated game never exits
CRT_CALLOC = 0x00563129    # calloc(n, size)
CRT_REALLOC = 0x005645A0   # realloc(p, n)
GAME_LOG = 0x004CE6E0      # log line (message, a, b)
FILE_READ_AT = 0x004CEB60  # file_read_at(name, buf, offset) -> 1 / 0
FILE_SIZE = 0x004CED50     # file_size(name)

# display-only game functions, replaced by no-ops (read: they write no simulation state)
PRESENTATION = {
    0x004CF7B0: 'loading-screen text (GDI calls only)',
    0x004D0280: 'screen fade / present (writes only its counters 0x00E92E08, 0x00E38DC8)',
}

# Allocations made while one of these functions is on the call stack hold simulation records:
# they go to the simulation heap, which is part of the state (core/state/state.h). Found by
# logging the callers of new / malloc during loads and ticks; every other allocation (graphics,
# UI) goes to the other heap. The stack is scanned for return addresses (32 frames' worth).
SIM_ALLOCATORS = {
    0x004BD740: 'floodplain object',
    0x004D4720: 'empire route path (route rebuild at load)',
    0x004D0A40: 'type-0x4A queue records: their arrays (0x004F6700), restored at load',
    0x00451A70: 'static constructor: arrays of 20 for shipwright queues 0-4 (kept by a load that fits)',
    0x0041C740: 'file list buffers (names, offsets): the scenario / mission list 0x00704C00 and the family list '
                '0x00EADFA8, whose names choose the city to start (core/newgame/flow2.c)',
}

# The modal message box 0x00538CD0 runs a Win32 message loop until the player presses a button.
# Headless, the first button is pressed at once: the same writes as core/sim/platform.c
# platform_message_box (button count, pressed flag and index, redraw flag; the window id is
# restored), without the drawing (0x00538C80) and the button rectangles (excluded.py).
MESSAGE_BOX = 0x00538CD0
MESSAGE_BOX_RESULTS = {0: 1, 1: 1, 2: 3, 3: 6, 4: 6, 5: 4}  # first button's code (0x00538940)
MESSAGE_BOX_BUTTONS = {0: 1, 1: 2, 2: 3, 3: 3, 4: 2, 5: 2}

SPARE_SCREEN = 0x00E92E1C  # 100,000-byte work buffer the model loader reads into


class Game(Pharaoh):
    def __init__(self, exe=None):
        super().__init__(exe)
        self.files = {}   # virtual files: lower-case name -> bytes
        self.names = {}   # lower-case name -> the name as the game (or a test) wrote it, for listings
        self.finds = {}   # FindFirstFileA handle -> [names, next index]
        self.handlers['FindFirstFileA'] = (2, self._find_first)
        self.handlers['FindNextFileA'] = (2, self._find_next)
        self.handlers['FindClose'] = (1, lambda e, h: 1 if self.finds.pop(h, None) is not None else 0)
        self.fds = {}     # fd -> [data, position]
        self.log = []
        self.hook(CRT_OPEN, self._open)
        self.hook(CRT_READ, self._read)
        self.hook(CRT_WRITE, self._write)
        self.hook(CRT_CLOSE, lambda e: 0 if self.fds.pop(e.arg(0), None) is not None else -1)
        self.hook(CRT_CHDIR, lambda e: 0)
        self.hook(CRT_LSEEK, self._lseek)
        self.streams = {}  # FILE * -> [data, position]
        self.hook(CRT_FOPEN, self._fopen)
        self.hook(CRT_FGETS, self._fgets)
        self.hook(CRT_FCLOSE, lambda e: 0 if self.streams.pop(e.arg(0), None) is not None else -1)
        self.hook(CRT_FWRITE, self._fwrite)
        self.hook(CRT_FREAD, self._fread)
        self.hook(CRT_FSEEK, self._fseek)
        self.hook(CRT_FILELENGTH, lambda e: len(self.fds[e.arg(0)][0]) if e.arg(0) in self.fds else -1)
        self.hook(CRT_NEW, lambda e: e.alloc(e.arg(0), self._sim_caller()))
        self.hook(CRT_DELETE, self._free)
        self.hook(CRT_MALLOC, lambda e: e.alloc(e.arg(0), self._sim_caller()))
        self.hook(CRT_NH_MALLOC, lambda e: e.alloc(e.arg(0)))
        self.hook(CRT_FREE, self._free)
        self.hook(CRT_ATEXIT, lambda e: 0)
        self.hook(CRT_CALLOC, self._calloc)
        self.hook(CRT_REALLOC, self._realloc)
        self.hook(GAME_LOG, self._log)
        self.hook(FILE_READ_AT, self._file_read_at)
        self.hook(FILE_SIZE, lambda e: len(self._file(e.cstr(e.arg(0))) or b''))
        for addr in PRESENTATION:
            self.hook(addr, lambda e: 0)
        self.hook(MESSAGE_BOX, self._message_box)
        self.set_u32(SPARE_SCREEN, self.alloc(100000))

    # --- files ---------------------------------------------------------------------------------
    def _file(self, name):
        key = name.replace('\\', '/').lower()
        if key in self.files:
            return self.files[key]
        path = os.path.join(GAME_DIR, name.replace('\\', '/'))
        if not os.path.exists(path):  # the game's names are case-insensitive
            d, base = os.path.split(path)
            if os.path.isdir(d):
                match = [f for f in os.listdir(d) if f.lower() == base.lower()]
                path = os.path.join(d, match[0]) if match else path
        if os.path.isfile(path):
            return open(path, 'rb').read()
        return None

    def _open(self, e):
        name, flags = e.cstr(e.arg(0)), e.arg(1)
        if flags & 0x0300:  # _O_CREAT | _O_TRUNC: a new file, kept in self.files when closed
            data = bytearray()
            self.add_file(name, data)
        else:
            data = self._file(name)
            if data is None:
                return -1
        fd = 3
        while fd in self.fds:
            fd += 1
        self.fds[fd] = [data, 0]
        return fd

    def _write(self, e):
        f = self.fds.get(e.arg(0))
        if f is None or not isinstance(f[0], bytearray):
            return -1
        chunk = e.read(e.arg(1), e.arg(2))
        f[0][f[1]:f[1] + len(chunk)] = chunk
        f[1] += len(chunk)
        return len(chunk)

    def _read(self, e):
        f = self.fds.get(e.arg(0))
        if f is None:
            return -1
        data, pos = f
        chunk = data[pos:pos + e.arg(2)]
        e.write(e.arg(1), chunk)
        f[1] = pos + len(chunk)
        return len(chunk)

    def _file_read_at(self, e):
        data = self._file(e.cstr(e.arg(0)))
        if data is None:
            return 0
        e.write(e.arg(1), data[e.arg(2):])
        return 1

    def _fopen(self, e):
        """read streams and binary write streams (the CRT's stdio tables are set up by the
        initialiser table, which is not run); a FILE * is a 32-byte block of the other heap, used
        only as a key. A "wb" stream is a new file of self.files (lower-case name, '/')."""
        mode = e.cstr(e.arg(1))
        if mode == 'wb':
            data = bytearray()
            self.add_file(e.cstr(e.arg(0)), data)
            fp = e.alloc(32)
            self.streams[fp] = [data, 0]
            return fp
        if 'r' not in mode or '+' in mode:
            return 0
        data = self._file(e.cstr(e.arg(0)))
        if data is None:
            return 0
        if 'b' not in mode:  # text mode: CR LF -> LF, ^Z ends the file
            data = data.split(b'\x1a')[0].replace(b'\r\n', b'\n')
        fp = e.alloc(32)
        self.streams[fp] = [data, 0]
        return fp

    def _fgets(self, e):
        f = self.streams.get(e.arg(2))
        n = struct.unpack('<i', struct.pack('<I', e.arg(1)))[0]
        if f is None or n < 1:
            return 0
        data, pos = f
        if pos >= len(data):
            return 0
        end = data.find(b'\n', pos, pos + n - 1)
        end = pos + n - 1 if end < 0 else end + 1
        chunk = data[pos:min(end, len(data))]
        e.write(e.arg(0), chunk + b'\0')
        f[1] = pos + len(chunk)
        return e.arg(0)

    def _fwrite(self, e):
        """fwrite on a write stream: the bytes appended; the number of whole items written"""
        f = self.streams.get(e.arg(3))
        if f is None or not isinstance(f[0], bytearray) or e.arg(1) == 0:
            return 0
        chunk = e.read(e.arg(0), e.arg(1) * e.arg(2))
        f[0][f[1]:f[1] + len(chunk)] = chunk
        f[1] += len(chunk)
        return e.arg(2)

    def _fread(self, e):
        """fread on a read stream: the bytes up to the end of the file copied (a short file fills the
        buffer as far as it goes); the number of whole items read"""
        f = self.streams.get(e.arg(3))
        size, count = e.arg(1), e.arg(2)
        if f is None or size == 0 or count == 0:
            return 0
        data, pos = f
        chunk = bytes(data[pos:pos + size * count])
        e.write(e.arg(0), chunk)
        f[1] = pos + len(chunk)
        return len(chunk) // size

    def _fseek(self, e):
        f = self.streams.get(e.arg(0))
        if f is None:
            return -1
        offset = struct.unpack('<i', struct.pack('<I', e.arg(1)))[0]
        base = {0: 0, 1: f[1], 2: len(f[0])}.get(e.arg(2))
        if base is None or base + offset < 0:
            return -1
        f[1] = base + offset
        return 0

    def add_file(self, name, data):
        """a player file (a test's, or one the game creates): kept in self.files, its name as given"""
        key = name.replace('\\', '/').lower()
        self.files[key] = data
        self.names[key] = name.replace('\\', '/')

    # --- directory listings (FindFirstFileA / FindNextFileA / FindClose) ------------------------
    # The game lists maps\*.map, Save\*.dat and save\<family>\*.sav (0x0041C8F0). The entries: the
    # files of self.files in that directory (and the directories they imply), then those of the game
    # directory, names compared case-insensitively; in the order of NTFS (by name, upper-cased: the
    # order of Windows' listings on the GOG install's file system, assumed), as build/state_replay's
    # platform_file_list lists them.
    def listing(self, pattern):
        import fnmatch
        path = pattern.replace('\\', '/')
        folder, _, mask = path.rpartition('/')
        folder = folder.lower().strip('/')
        found = {}
        for key, data in self.files.items():
            name = self.names.get(key, key)
            parts = name.split('/')
            for depth in range(len(parts)):
                if '/'.join(parts[:depth]).lower() == folder:
                    found.setdefault(parts[depth].lower(), parts[depth])
        d = GAME_DIR
        for part in [p for p in folder.split('/') if p]:
            match = [f for f in os.listdir(d) if f.lower() == part] if os.path.isdir(d) else []
            d = os.path.join(d, match[0] if match else part)
        if os.path.isdir(d):
            for f in os.listdir(d):
                found.setdefault(f.lower(), f)
        upper = mask.upper()
        names = [n for n in found.values() if fnmatch.fnmatchcase(n.upper(), upper)]
        return sorted(names, key=lambda n: n.encode('latin-1').upper())

    def _find_data(self, p, name):
        """WIN32_FIND_DATAA: attributes, times, size, reserved, cFileName[260], cAlternateFileName[14]"""
        self.write(p, struct.pack('<I', 0x80) + bytes(0x28) + name.encode('latin-1')[:259] + b'\0')

    def _find_first(self, e, pattern, data):
        names = self.listing(e.cstr(pattern))
        if not names:
            return 0xFFFFFFFF
        h = 0x100
        while h in self.finds:
            h += 1
        self.finds[h] = [names, 1]
        self._find_data(data, names[0])
        return h

    def _find_next(self, e, h, data):
        f = self.finds.get(h)
        if f is None or f[1] >= len(f[0]):
            return 0
        self._find_data(data, f[0][f[1]])
        f[1] += 1
        return 1

    def _lseek(self, e):
        f = self.fds.get(e.arg(0))
        if f is None:
            return -1
        offset = struct.unpack('<i', struct.pack('<I', e.arg(1)))[0]
        base = {0: 0, 1: f[1], 2: len(f[0])}.get(e.arg(2))
        if base is None or base + offset < 0:
            return -1
        f[1] = base + offset
        return f[1]

    def _sim_caller(self):
        """A function of SIM_ALLOCATORS is on the call stack of this new / malloc."""
        from where import function_start
        esp = self.uc.reg_read(UC_X86_REG_ESP)
        top = min(0x800, 0x7E000000 - esp)
        for v in struct.unpack('<%dI' % (top // 4), self.read(esp, top - top % 4)):
            if 0x00401000 <= v < 0x0056F000 and function_start(v) in SIM_ALLOCATORS:
                return True
        return False

    def _free(self, e):
        """delete / free: a simulation-heap block is zero-filled (core/state/state.c state_dealloc
        does the same); nothing is reused."""
        from emu import HEAP_BASE
        p = e.arg(0)
        if HEAP_BASE <= p < self.heap and p in self.sizes:
            e.write(p, bytes(self.sizes[p]))
        return 0

    def _calloc(self, e):
        n = e.arg(0) * e.arg(1)
        p = e.alloc(n, self._sim_caller())
        e.write(p, bytes(n))
        return p

    def _realloc(self, e):
        old, n = e.arg(0), e.arg(1)
        p = e.alloc(n, self._sim_caller())
        if old:
            keep = min(n, self.sizes.get(old, n))
            e.write(p, e.read(old, keep))
            from emu import HEAP_BASE
            if HEAP_BASE <= old < self.heap and old in self.sizes:
                e.write(old, bytes(self.sizes[old]))
        return p

    def _message_box(self, e):
        k = e.arg(2) & 0xF
        k = k if k <= 5 else 0
        self.write(0x00F535C8, struct.pack('<i', MESSAGE_BOX_BUTTONS[k]))
        self.write(0x00F535C0, b'\x01')
        self.write(0x00F535C4, struct.pack('<i', 0))
        self.write(0x00E38DCA, b'\x01')
        return MESSAGE_BOX_RESULTS[k]

    def _log(self, e):
        self.log.append(e.cstr(e.arg(0)) if e.arg(0) else '')
        return 0

    # --- start-up ------------------------------------------------------------------------------
    # The CRT's static initialisers, then WinMain (0x00413ED0) initialisation, in its order, without the CD drive search (0x004268C0),
    # DirectDraw (0x004CF960), the
    # sound start (0x005394F0), the intro video and the message loop.
    BOOT = [  # (address, string arguments, this pointer)
        ('initializers', (), None), (0x004147F0, (), None),
        (0x004CEC40, ('Pharaoh_Text.eng', 'Pharaoh_MM.eng'), None), ('cd', (), None),
        (0x004149D0, (), None), (0x00414420, (), None), (0x00540C90, (), None), (0x00540ED0, (), None),
        (0x00541160, (), None), (0x0041AE40, ('campaign.txt',), 0x006FDD08), (0x00541310, (), None),
        (0x0044EFA0, (), None), (0x0051F780, (), None), ('globals', (), None), (0x00540B50, (), None),
        (0x004013F0, (), 0x006DE650), (0x0040D7E0, (), 0x006FD5D8), (0x00416F60, (), 0x006FD368),
        (0x004E6590, (), 0x006DE548), (0x005456A0, (), 0x006F9320), (0x00558B40, (), 0x006F5648),
        (0x00555DE0, (), 0x006DE9E0), (0x0054F3D0, (), 0x006E9188), (0x00541D50, (), None),
        (0x004DEE10, (), None), ('globals2', (), None), (0x0051E930, (), None), ('globals3', (), None),
        (0x004CF8E0, (), None), ('video', (), None),
    ]

    def boot(self, trace=False):
        for addr, args, this in self.BOOT:
            if addr == 'initializers':
                # CRT _cinit (0x005646C0): the C table (0x00576080, CRT internals) is replaced by
                # its one effect on the simulation, the x87 precision (0x005640EC: 53-bit); the C++
                # table (0x00576000) holds the game's static constructors and runs as is.
                self.fpu_control(0x027F)
                for p in range(0x00576000, 0x0057607C, 4):
                    if self.u32(p):
                        self.call(self.u32(p))
                continue
            if addr == 'globals':
                for a, v in ((0x00605A3C, 0), (0x00605A38, 0), (0x00605A34, 2), (0x00605A30, 0),
                             (0x00605A0C, 0), (0x00605A08, 0), (0x00605A04, 1), (0x00605A00, 0),
                             (0x006059FC, 0xFFFFFFFF)):
                    self.set_u32(a, v)
                self.write(0x006DE514, b'\0\0')
                continue
            if addr == 'globals2':
                self.set_u32(0x006FDCB8, 0)
                continue
            if addr == 'globals3':
                self.write(0x00E38DDA, b'\x44')
                self.write(0x00E38DCA, b'\x01')
                continue
            if addr == 'cd':  # 0x004268C0 looks for the CD drive; WinMain stores its letter
                self.write(0x00E38E62, b'D')
                continue
            if addr == 'video':  # the Bink player's object, made by the DirectDraw set-up 0x004CF960
                p = self.alloc(0x1C)
                self.call(0x004133D0, self.u32(0x00E39140), ecx=p)
                self.set_u32(0x006FDC54, p)
                continue
            if True:
                ptrs = []
                for a in args:
                    p = self.alloc(len(a) + 1)
                    self.write(p, a.encode() + b'\0')
                    ptrs.append(p)
                result = self.call(addr, *ptrs, ecx=this)
            if trace:
                print('boot 0x%08X -> %d' % (addr, result))

    # --- saved games ---------------------------------------------------------------------------
    def load_save(self, data, name='oracle.sav'):
        """Load a saved game (or a mission1.pak record) with the game's own loader 0x004D5100:
        chunk reader, version upgrades, route rebuild, then the scenario start 0x004D52A0 and the
        post-load rebuilds. Returns the loader's result (0 = loaded)."""
        self.files[name] = bytes(data)
        p = self.alloc(len(name) + 1)
        self.write(p, name.encode() + b'\0')
        return self.call(0x004D5100, p)

    def save(self, name='oracle_out.sav'):
        """Write the current state with the game's own saver 0x004D4E20 (the autosave path) and
        return the file's bytes."""
        p = self.alloc(len(name) + 1)
        self.write(p, name.encode() + b'\0')
        self.call(0x004D4E20, p)
        return bytes(self.files[name.lower()])

    # --- state files (core/state/state.h) -------------------------------------------------------
    STATE_BASE, STATE_END = 0x0056F000, 0x01267AA8

    def state_bytes(self):
        """The game state as a state file: data segments, u32 heap size, heap, u32 block count,
        (u32 offset, u32 size) per block of the simulation heap."""
        from emu import HEAP_BASE
        used = self.heap - HEAP_BASE
        blocks = sorted((p - HEAP_BASE, n) for p, n in self.sizes.items() if HEAP_BASE <= p < self.heap)
        return (self.read(self.STATE_BASE, self.STATE_END - self.STATE_BASE) + struct.pack('<I', used) +
                self.read(HEAP_BASE, used) + struct.pack('<I', len(blocks)) +
                b''.join(struct.pack('<II', o, n) for o, n in blocks))

    def load_state(self, data):
        """Replace the game state by a state file's (data segments, simulation heap, blocks)."""
        from emu import HEAP_BASE
        n = self.STATE_END - self.STATE_BASE
        self.write(self.STATE_BASE, data[:n])
        used, = struct.unpack_from('<I', data, n)
        self.write(HEAP_BASE, data[n + 4:n + 4 + used])
        if self.heap > HEAP_BASE + used:  # blocks allocated after this state: zero, as never used
            self.write(HEAP_BASE + used, bytes(self.heap - HEAP_BASE - used))
        self.heap = HEAP_BASE + used
        count, = struct.unpack_from('<I', data, n + 4 + used)
        self.sizes = {p: s for p, s in self.sizes.items() if not HEAP_BASE <= p < HEAP_BASE + 0x08000000}
        for i in range(count):
            o, sz = struct.unpack_from('<II', data, n + 8 + used + 8 * i)
            self.sizes[HEAP_BASE + o] = sz
