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

# with a display (Game(display=...)) these run as the original: the present blits the back buffer to the
# emulated primary surface (Game.blits) and writes its counters; the loading screen's text goes to
# Game.text (TextOutA)
DISPLAY_RUNS = {0x004CF7B0, 0x004D0280}

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
    0x004D4E20: 'the saver: its route buffer rebuild after writing (autosave, last.sav; core/frame/autosave.c)',
    0x004D8310: 'the input dispatcher: window 0x15 (empire editor) grows a route path buffer (chunk 143)',
    0x004DD930: 'window 0x15: a route waypoint deleted, the route path rebuilt (chunk 143)',
    0x004F6A50: 'a ring that grows during a tick (0x004F6900 push -> 0x004F6A50 -> 0x004F6700): its slots are '
                'simulation data (core/sim/ring.c)',
}

# ... except while one of these is on the call stack (nearer than the simulation allocator): their blocks
# hold no simulation data, whoever calls them (package finish)
NOT_SIM_ALLOCATORS = {
    0x00414BD0: 'the spare screen made again after a display mode change (0x00413690: the 640 x 480 switch around '
                'a period video, inside a mission start); the port\'s is platform memory (core/frame/display.c)',
}

# The modal message box 0x00538CD0 runs a Win32 message loop until the player presses a button.
# Headless, the first button is pressed at once: the same writes as core/sim/platform.c
# platform_message_box (button count, pressed flag and index, redraw flag; the window id is
# restored), without the drawing (0x00538C80) and the button rectangles (excluded.py).
MESSAGE_BOX = 0x00538CD0
MESSAGE_BOX_RESULTS = {0: 1, 1: 1, 2: 3, 3: 6, 4: 6, 5: 4}  # first button's code (0x00538940)
MESSAGE_BOX_BUTTONS = {0: 1, 1: 2, 2: 3, 3: 3, 4: 2, 5: 2}
MESSAGE_BOX_CODES = {0: (1,), 1: (1, 2), 2: (3, 4, 5), 3: (6, 7, 2), 4: (6, 7), 5: (4, 2)}  # each button's (0x00538940)

SPARE_SCREEN = 0x00E92E1C  # 100,000-byte work buffer the model loader reads into


# --- display (roadmap phase 7, docs/presentation.md 4.1) -------------------------------------------
# Game(display=(width, height, format)) boots with WinMain's DirectDraw set-up 0x004CF960 run on
# emulated DirectDraw objects: the back buffer is then a block of the other heap whose address the game
# keeps in 0x00E92E18 (pitch 0x00E38D94), and the drawing code (the frame 0x004D6B60) runs. Without it
# (display=None, the default) the boot skips 0x004CF960 as before: the resolution globals, the sidebar
# x 0x00F53F64 and the city viewport 0x0070E430.. stay 0 (docs/oracle.md "Display").
# IMHOTEP_ORACLE_DISPLAY=WxHxFORMAT (e.g. 1024x768x565) makes it the default of Game().
RESOLUTIONS = {(640, 480): 1, (800, 600): 2, (1024, 768): 3}  # 0x00E38E68 (@bin 0x004CF960)
FORMAT_MASKS = {565: (0xF800, 0x07E0, 0x001F), 555: (0x7C00, 0x03E0, 0x001F)}
BACK_BUFFER = 0x00E92E18   # u32: lpSurface of the back buffer (@bin 0x00427257)
PITCH = 0x00E38D94         # i32: bytes per row (@bin 0x004CF960)
SCREEN_W, SCREEN_H = 0x00E38DB0, 0x00E38DAC  # i32 width, height (@bin 0x004CF960)
PIXEL_FORMAT = 0x00E92DFC  # 0: 555, 2: 565 (@bin 0x004D0060)
WNDPROC = 0x00414CD0       # the window procedure (stdcall hwnd, msg, wparam, lparam)
WM_MOVE, WM_SIZE = 0x0003, 0x0005
DISPLAY_ENV = 'IMHOTEP_ORACLE_DISPLAY'
_DEFAULT = object()

# --- sound (roadmap phase 7, docs/oracle.md "Sound") ------------------------------------------------
# Game(sound=True) boots with WinMain's sound start-up 0x005394F0 (after 0x0044EFA0, dis 0x00414167) on
# a working stand-in for Miles (tools/oracle/sound.py FakeMiles): the sound flag 0x00F55B88 is 1 and the
# sound tables are built from the game's files. Every AIL_* call is in Game.ail_log. Without it (the
# default) the start-up is skipped and the game plays nothing, as before. IMHOTEP_ORACLE_SOUND=1 makes
# it the default of Game().
SOUND_ENV = 'IMHOTEP_ORACLE_SOUND'
SOUND_START = 0x005394F0
_FILE_CACHE = {}  # game files read (path -> bytes): the sound start-up opens ~1300 audio files


def parse_display(text):
    """'1024x768x565' -> (1024, 768, 565); '', 'none', None -> None"""
    if not text or text.lower() in ('none', '0', 'off'):
        return None
    w, h, f = (int(x) for x in text.lower().split('x'))
    if (w, h) not in RESOLUTIONS or f not in FORMAT_MASKS:
        raise ValueError('display %s: one of 640x480, 800x600, 1024x768 and 555 / 565' % text)
    return w, h, f


def display_name(display):
    return 'nodisplay' if display is None else '%dx%dx%d' % display


class Game(Pharaoh):
    def __init__(self, exe=None, display=_DEFAULT, sound=_DEFAULT):
        super().__init__(exe)
        if display is _DEFAULT:
            display = parse_display(os.environ.get(DISPLAY_ENV))
        elif isinstance(display, str):
            display = parse_display(display)
        self.display = display
        if sound is _DEFAULT:
            sound = os.environ.get(SOUND_ENV, '') not in ('', '0', 'off', 'none')
        self.sound = bool(sound)
        self.miles = None
        if self.sound:
            from sound import FakeMiles
            self.miles = FakeMiles(self)
            # the sound start-up 0x0055E6C4 asks the Windows version (NT 4 and older: no sound)
            self.handlers['GetVersionExA'] = (1, self._version)
        # frame inputs (tools/oracle/framestep.py): the clock of timeGetTime / GetTickCount (0 by
        # default, as before), the cursor of GetCursorPos, the keys of GetAsyncKeyState
        self.clock = 0
        self.cursor = (0, 0)
        self.keys_down = set()
        self.keys_pressed = set()
        self.handlers['timeGetTime'] = (0, lambda e: self.clock)
        self.handlers['GetTickCount'] = (0, lambda e: self.clock)
        # the Bink player (BINKW32): BinkOpen fails (0) unless a test sets self.bink = (width, height):
        # it then opens a video of that size with no frame, so that 0x00413690 skips its frame loop and
        # makes only its state effects (the 640 x 480 switch in fullscreen, the window id, the redraw)
        self.bink = None
        self.handlers['_BinkOpen@8'] = (2, self._bink_open)
        self.files = {}   # virtual files: lower-case name -> bytes
        self.names = {}   # lower-case name -> the name as the game (or a test) wrote it, for listings
        self.finds = {}   # FindFirstFileA handle -> [names, next index]
        self.handlers['FindFirstFileA'] = (2, self._find_first)
        self.handlers['FindNextFileA'] = (2, self._find_next)
        self.handlers['FindClose'] = (1, lambda e, h: 1 if self.finds.pop(h, None) is not None else 0)
        # deleting player files (CRT remove 0x00562BB5 -> DeleteFileA, the family deletion 0x00516DD0 ->
        # RemoveDirectoryA): a file of self.files is removed (logged in self.deleted); any other fails
        # with ERROR_FILE_NOT_FOUND (2, for GetLastError); a directory is removed when no file of
        # self.files is left in it (directories are implied by the files), else ERROR_DIR_NOT_EMPTY
        self.deleted = []
        self.last_error = 0
        self.handlers['DeleteFileA'] = (1, self._delete_file)
        self.handlers['RemoveDirectoryA'] = (1, self._remove_directory)
        self.handlers['GetLastError'] = (0, lambda e: self.last_error)
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
        self.hook(CRT_FCLOSE, self._fclose)
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
            if self.display is None or addr not in DISPLAY_RUNS:
                self.hook(addr, lambda e: 0)
        self.hook(MESSAGE_BOX, self._message_box)
        self.set_u32(SPARE_SCREEN, self.alloc(100000))
        if self.display is not None:
            self._display_stubs()

    # --- display: window calls and DirectDraw ---------------------------------------------------
    HWND, HDC = 0x4000, 0x1234

    def _display_stubs(self):
        w, h, _ = self.display
        self.surfaces = {}   # surface object -> [buffer, width, height, pitch]
        self.blits = []      # (kind, destination, source) of Blt / BltFast
        self.posted = []     # posted messages and quit requests (PostMessageA, PostQuitMessage)
        self.text = []       # (x, y, text) of TextOutA (the loading screen 0x004CF7B0, when it runs)
        self.window_calls = []

        def ok(n, r=1):
            return (n, lambda e, *a: r)

        def rect(r, a, b, c, d):
            self.write(r, struct.pack('<4i', a, b, c, d))
            return 1
        stubs = {
            'GetDC': ok(1, self.HDC), 'ReleaseDC': ok(2),
            'GetDeviceCaps': (2, lambda e, dc, i: {0x0C: 16, 0x0E: 1, 0x18: 16}.get(i, 0)),  # BITSPIXEL, PLANES
            'CoInitialize': ok(1, 0), 'FindWindowA': ok(2, 0), 'LoadIconA': ok(2, 0x50),
            'GetStockObject': ok(1, 0x60), 'RegisterClassA': ok(1, 0xC001), 'LoadCursorA': ok(2, 0x70),
            'CreateWindowExA': ok(12, self.HWND), 'UpdateWindow': ok(1), 'SetFocus': ok(1, 0),
            'GetWindowLongA': ok(2, 0), 'SetWindowLongA': ok(3, 0),
            'SetRect': (5, lambda e, r, a, b, c, d: rect(r, a, b, c, d)),
            'AdjustWindowRectEx': ok(4), 'GetMenu': ok(1, 0), 'SetWindowPos': ok(7),
            'SystemParametersInfoA': ok(4), 'GetWindowRect': (2, lambda e, wnd, r: rect(r, 0, 0, w, h)),
            # the client area: the mode's size (0x00E38D9C x 0x00E38D98, the window sized for it by
            # AdjustWindowRectEx; fullscreen the screen in that mode), the screen's before the set-up;
            # ClientToScreen leaves it at 0, 0 (cursor positions are given in client pixels). The
            # game reads it only windowed (fullscreen: GetSystemMetrics)
            'GetClientRect': (2, lambda e, wnd, r: rect(r, 0, 0, self.u32(0x00E38D9C) or w, self.u32(0x00E38D98) or h)),
            'ClientToScreen': ok(2),
            'GetPaletteEntries': ok(4, 0), 'ShowCursor': ok(1, 0), 'SetCursor': ok(1, 0),
            'PeekMessageA': ok(5, 0), 'InvalidateRect': ok(3), 'GetForegroundWindow': ok(0, self.HWND),
            # WM_PAINT, Alt+X (a posted Esc) and quitting: logged in self.posted, nothing else happens
            'BeginPaint': (2, lambda e, w, ps: self.write(ps, bytes(64)) or 0x0DC0),
            'EndPaint': ok(2),
            'PostMessageA': (4, lambda e, w, m, a, b: self.posted.append(('post', m, a, b)) or 1),
            'PostQuitMessage': (1, lambda e, c: self.posted.append(('quit', c)) or 0),
            # ShellExecuteA(window, verb, file, parameters, directory, show): the main menu's web site
            'ShellExecuteA': (6, lambda e, w, verb, f, a, d, show: self.posted.append(('url', e.cstr(f))) or 42),
            'SetActiveWindow': ok(1, 0), 'ShowWindow': ok(2), 'SetForegroundWindow': ok(1),
            'SetBkColor': ok(2, 0), 'SetTextColor': ok(2, 0), 'SetBkMode': ok(2, 0),
            'DefWindowProcA': ok(4, 0), 'IsIconic': ok(1, 0),
            'GetTextExtentPointA': (4, lambda e, dc, p, n, sz: self.write(sz, struct.pack('<ii', 8 * n, 16)) or 1),
            'TextOutA': (5, self._text_out),
            'DirectDrawCreate': (3, self._dd_create),
            'GetCursorPos': (1, lambda e, p: self.write(p, struct.pack('<ii', *self.cursor)) or 1),
            'GetAsyncKeyState': (1, self._async_key),
            'GetSystemMetrics': (1, lambda e, i: {0: w, 1: h}.get(i, 0)),  # fullscreen: the display mode
        }
        self.handlers.update(stubs)
        # IDirectDraw, IDirectDrawSurface, IDirectDrawClipper: the methods 0x004CF960 and the present
        # call (vtable offsets of the DirectDraw 1 interfaces)
        self.dd = self.com_object({0x08: (1, 0), 0x10: (4, self._create_clipper), 0x18: (4, self._create_surface),
                                   0x50: (3, 0), 0x54: (4, 0)}, 'IDirectDraw')
        self.clipper_methods = {0x08: (1, 0), 0x20: (3, 0)}
        self.surface_methods = {0x08: (1, 0), 0x14: (6, self._blt), 0x1C: (6, self._blt_fast), 0x2C: (3, 0),
                                0x54: (2, self._pixel_format), 0x58: (2, self._surface_desc), 0x60: (1, 0),
                                0x64: (5, self._lock), 0x6C: (1, 0), 0x70: (2, 0), 0x80: (2, 0)}

    def _text_out(self, e, dc, x, y, p, n):
        self.text.append((x, y, self.read(p, n).decode('latin-1')))
        return 1

    def _async_key(self, e, vk):
        """bit 15: the key is down; bit 0: pressed since the last call for it"""
        r = 0x8000 if vk in self.keys_down else 0
        if vk in self.keys_pressed:
            self.keys_pressed.discard(vk)
            r |= 1
        return r

    def _dd_create(self, e, guid, out, outer):
        self.set_u32(out, self.dd)
        return 0

    def _create_clipper(self, e, this, flags, out, outer):
        self.set_u32(out, self.com_object(self.clipper_methods, 'IDirectDrawClipper'))
        return 0

    def _create_surface(self, e, this, desc, out, outer):
        """DDSURFACEDESC: flags +4, height +8, width +0xC; no DDSD_HEIGHT (2): the primary surface, of
        the screen's size. The memory is a block of the other heap, rows of 2 * width bytes."""
        flags, h, w = struct.unpack('<3I', self.read(desc + 4, 12))
        if not flags & 2:
            w, h = self.display[0], self.display[1]
        pitch = 2 * w
        buf = self.alloc(pitch * h)
        s = self.com_object(self.surface_methods, 'IDirectDrawSurface')
        self.surfaces[s] = [buf, w, h, pitch]
        self.set_u32(out, s)
        return 0

    def _pf(self, p):
        """DDPIXELFORMAT: size, DDPF_RGB, fourCC, 16 bits, the masks of the chosen format"""
        self.write(p, struct.pack('<8I', 32, 0x40, 0, 16, *FORMAT_MASKS[self.display[2]], 0))

    def _pixel_format(self, e, this, p):
        self._pf(p)
        return 0

    def _surface_desc(self, e, this, d):
        """DDSURFACEDESC: flags, height, width, pitch +0x10, lpSurface +0x24, pixel format +0x48"""
        buf, w, h, pitch = self.surfaces[this]
        self.write(d + 4, struct.pack('<4I', 0x100F, h, w, pitch))
        self.set_u32(d + 0x24, buf)
        self._pf(d + 0x48)
        return 0

    def _lock(self, e, this, r, d, flags, event):
        return self._surface_desc(e, this, d)

    def _blt(self, e, this, dst_rect, src, src_rect, flags, fx):
        """the whole source copied (the present's copies are of the whole screen); DDBLT_COLORFILL
        (0x400): the whole surface filled with the 16-bit colour DDBLTFX.dwFillColor (+0x50; the
        clearing 0x00426F60, whole surfaces, colour 0)"""
        self.blits.append(('Blt', this, src))
        if flags & 0x400 and this in self.surfaces and fx:
            a = self.surfaces[this]
            self.write(a[0], struct.pack('<H', self.u32(fx + 0x50) & 0xFFFF) * (a[3] * a[2] // 2))
            return 0
        if src in self.surfaces and this in self.surfaces:
            a, b = self.surfaces[this], self.surfaces[src]
            self.write(a[0], self.read(b[0], min(a[3] * a[2], b[3] * b[2])))
        return 0

    def _blt_fast(self, e, this, x, y, src, r, trans):
        self.blits.append(('BltFast', this, src))
        return self._blt(e, this, 0, src, r, 0, 0)

    # --- display: the back buffer -----------------------------------------------------------------
    def screen_info(self):
        """(address, width, height, pitch, format) of the back buffer, or None without a display"""
        if self.display is None or not self.u32(BACK_BUFFER):
            return None
        return (self.u32(BACK_BUFFER), self.i32(SCREEN_W), self.i32(SCREEN_H), self.i32(PITCH), self.display[2])

    def screen(self):
        """the back buffer's bytes (pitch * height; the drawing writes through 0x00E92E18 with the
        pitch 0x00E38D94), or None without a display"""
        info = self.screen_info()
        return None if info is None else self.read(info[0], info[3] * info[2])

    def set_screen(self, data):
        info = self.screen_info()
        self.write(info[0], data)

    def screen_png(self, path):
        """the back buffer as a PNG, for debugging (game output: write it under local/, never into the
        repository)"""
        import screen
        _, w, h, pitch, fmt = self.screen_info()
        screen.write_png(path, self.screen(), w, h, pitch, fmt)

    def message(self, msg, wparam=0, lparam=0):
        """a window message delivered to the original's window procedure 0x00414CD0 (its input
        globals: docs/presentation.md 5); returns its result"""
        return self.call(WNDPROC, self.HWND, msg, wparam, lparam)

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
        if path in _FILE_CACHE:
            return _FILE_CACHE[path]
        if os.path.isfile(path):
            data = open(path, 'rb').read()
            if len(_FILE_CACHE) < 4096:
                _FILE_CACHE[path] = data
            return data
        return None

    def _version(self, e, p):
        # OSVERSIONINFOA: Windows XP (5.1.2600, VER_PLATFORM_WIN32_NT)
        e.write(p + 4, struct.pack('<4I', 5, 1, 2600, 2))
        return 1

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
            self._stream_fd(fp, data)
            return fp
        if 'r' not in mode or '+' in mode:
            return 0
        data = self._file(e.cstr(e.arg(0)))
        if data is None:
            return 0
        raw = data
        if 'b' not in mode:  # text mode: CR LF -> LF, ^Z ends the file
            data = data.split(b'\x1a')[0].replace(b'\r\n', b'\n')
        fp = e.alloc(32)
        self.streams[fp] = [data, 0]
        self._stream_fd(fp, raw)
        return fp

    def _fclose(self, e):
        fp = e.arg(0)
        if self.streams.pop(fp, None) is None:
            return -1
        self.fds.pop(self.u32(fp + 0x10), None)
        return 0

    def _stream_fd(self, fp, raw):
        """FILE._file (+0x10): a descriptor of the open file, as the CRT's fopen leaves it, so that
        _filelength(fp->_file) gives the file's size (0x004CBA60 sizes EVENTMSG.TXT that way)"""
        fd = 3
        while fd in self.fds:
            fd += 1
        self.fds[fd] = [raw, 0]
        self.set_u32(fp + 0x10, fd)

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

    def _delete_file(self, e, p):
        name = e.cstr(p)
        key = name.replace('\\', '/').lower()
        self.deleted.append(('file', name))
        if key in self.files:
            del self.files[key]
            self.names.pop(key, None)
            return 1
        self.last_error = 2
        return 0

    def _remove_directory(self, e, p):
        name = e.cstr(p)
        key = name.replace('\\', '/').lower().rstrip('/')
        self.deleted.append(('directory', name))
        if any(k.startswith(key + '/') for k in self.files):
            self.last_error = 145
            return 0
        return 1

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
            if 0x00401000 <= v < 0x0056F000:
                f = function_start(v)
                if f in NOT_SIM_ALLOCATORS:
                    return False
                if f in SIM_ALLOCATORS:
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
        """the box answered at once: with the next of self.box_answers (a test's: the button's index,
        0 the first), else the first button"""
        k = e.arg(2) & 0xF
        k = k if k <= 5 else 0
        answers = getattr(self, 'box_answers', None)
        pressed = answers.pop(0) if answers else 0
        if not 0 <= pressed < MESSAGE_BOX_BUTTONS[k]:
            pressed = 0
        self.write(0x00F535C8, struct.pack('<i', MESSAGE_BOX_BUTTONS[k]))
        self.write(0x00F535C0, b'\x01')
        self.write(0x00F535C4, struct.pack('<i', pressed))
        self.write(0x00E38DCA, b'\x01')
        return MESSAGE_BOX_CODES[k][pressed]

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

    def boot_steps(self):
        """BOOT, or with a display: 0x004CF960 after the settings read, and no 'video' step; with sound:
        the sound start-up 0x005394F0 after 0x0044EFA0"""
        steps = []
        for step in self.BOOT:
            if step[0] == 'video' and self.display is not None:
                continue
            steps.append(step)
            if step[0] == 0x004149D0 and self.display is not None:
                steps.append(('display', (), None))
            if step[0] == 0x0044EFA0 and self.sound:
                steps.append((SOUND_START, (), None))
        return steps

    def _bink_open(self, e, name, flags):
        """BinkOpen(name, flags): the BINK structure (+0 width, +4 height, +0x10 frames, +0x14 the current
        frame, 1-based in Bink; 0 frames here) of the video self.bink describes, or 0"""
        if not self.bink:
            return 0
        p = e.alloc(0x40)
        e.write(p, struct.pack('<6I', self.bink[0], self.bink[1], 0, 0, 0, 0) + bytes(0x28))
        return p

    def display_options(self):
        """With a display: the options file the settings read at boot (0x004147F0 reads Pharaoh.inf over
        0x00E38E58, core/boot/settings.c), as a player who chose this resolution left it: its first 0x14
        bytes (the original reads whatever length the file has), the settings' defaults (@bin
        0x00414802..0x004148B1: depth 0x10, 0x00E38E60..0x00E38E65 0, 0, 'D', 1, 1, 1; 0x00E38E66/67 are 0,
        nothing sets them before) with fullscreen 0x00E38E5C 1 and the resolution 0x00E38E68 of the
        display (1 640x480, 2 800x600, 3 1024x768). With Game.windowed: fullscreen 0 (the game's F6) and
        the resolution below the display's (a window that fits the screen; 640x480 on a 640x480
        display). A test's own Pharaoh.inf (Game.files) is kept."""
        if self.display is None or 'pharaoh.inf' in self.files:
            return
        res = RESOLUTIONS[self.display[:2]]
        windowed = getattr(self, 'windowed', False)
        self.add_file('Pharaoh.inf', struct.pack('<ii8Bi', 0x10, 0 if windowed else 1, 0, 0, 0x44, 1, 1, 1, 0, 0,
                                                 max(1, res - 1) if windowed else res))

    def boot(self, trace=False):
        self.display_options()
        for addr, args, this in self.boot_steps():
            if addr == 'initializers':
                # CRT _cinit (0x005646C0): the C table (0x00576080, CRT internals) is replaced by
                # its one effect on the simulation, the x87 precision (0x005640EC: 53-bit); the C++
                # table (0x00576000) holds the game's static constructors and runs as is.
                self.fpu_control(0x027F)
                for p in range(0x00576000, 0x0057607C, 4):
                    if self.u32(p):
                        self.call(self.u32(p))
                # WinMain's first stores (dis 0x00413F02..0x00413F1F): 0x44 dwords at 0x006FDB38
                # cleared, 0x00E38DD6 := 0
                self.write(0x006FDB38, bytes(0x44 * 4))
                self.write(0x00E38DD6, b'\0')
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
            if addr == 'display':
                # WinMain's DirectDraw set-up 0x004CF960 (after the settings read 0x004149D0), at the
                # resolution (0x00E38E68) and fullscreen (0x00E38E5C) the settings read from Pharaoh.inf
                # (display_options); it also makes the Bink player's object (the 'video' step)
                result = self.call(0x004CF960, 0x00400000)
                if trace:
                    print('boot 0x004CF960 -> %d' % result)
                # the window is shown: Windows sends WM_SIZE and WM_MOVE (assumed: the order of the
                # messages of ShowWindow is not observed), whose handler (dis 0x00414E8C..0x00414EF9)
                # sets the client rectangle 0x00E39130 (in fullscreen: 0, 0 and the screen size), which
                # scales the cursor (0x004CF210), and presents
                self.message(WM_SIZE, 0, self.display[1] << 16 | self.display[0])
                self.message(WM_MOVE, 0, 0)
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
                if addr == 0x004CF8E0:  # dis 0x0041424B: WinMain's store after it
                    self.write(0x00E38DD6, b'\x01')
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

    @classmethod
    def booted(cls, **kw):
        """A booted game (Game(**kw).boot()), from a snapshot when one is cached
        (local/oracle/cache/boot_*.snap, keyed by the arguments and the oracle's code)."""
        return cls.cached('boot', lambda g: None, **kw)

    @classmethod
    def cached(cls, name, setup, **kw):
        """Game(**kw), booted, then setup(g); the result is snapshotted under `name` (with the
        arguments and a hash of the oracle's code in the key) and restored next time (0.3 s
        instead of the boot and the setup). setup must be deterministic for a given name."""
        import hashlib
        here = os.path.dirname(os.path.abspath(__file__))
        code = hashlib.sha1()
        for f in ('emu.py', 'game.py', 'sound.py'):
            if os.path.exists(os.path.join(here, f)):
                code.update(open(os.path.join(here, f), 'rb').read())
        g = cls(**kw)  # the key holds the settings in effect (defaults come from the environment)
        settings = (g.display, bool(getattr(g, 'sound', False)), sorted(k for k in kw if k not in ('display', 'sound')))
        key = hashlib.sha1(repr((name, settings, code.hexdigest())).encode()).hexdigest()[:20]
        cache = os.path.join(os.path.dirname(os.path.dirname(here)), 'local', 'oracle', 'cache')
        path = os.path.join(cache, 'snap_%s.snap' % key)
        if os.path.exists(path):
            try:
                g.restore(path)
                return g
            except Exception:  # an unreadable or old snapshot: build it again
                g = cls(**kw)
        g.boot()
        setup(g)
        try:
            os.makedirs(cache, exist_ok=True)
            g.snapshot(path)
        except Exception:  # something cannot be saved (e.g. a sound driver object): no cache
            pass
        return g

    # --- snapshots: the whole emulated game, to skip a boot and a load next time ----------------
    # Not saved: what the constructor rebuilds the same way (the Unicorn engine and its hooks, the
    # Python handlers of the imports); the COM handlers the boot created are rebuilt from their names.
    SNAPSHOT_SKIP = {'uc', 'hooks', 'handlers', 'surface_methods', 'deep_recorder'}
    SNAPSHOT_VERSION = 1

    def snapshot(self, path):
        """Write the whole emulated game to path: every mapped region (the heaps up to their use),
        the CPU context (x87 included), the Python-side state. Raises if something cannot be saved."""
        import pickle
        from emu import HEAP_BASE, OTHER_HEAP_BASE
        regions = []
        for lo, hi, _ in self.uc.mem_regions():
            end = hi + 1
            if lo == HEAP_BASE:
                end = self.heap
            elif lo == OTHER_HEAP_BASE:
                end = self.other_heap
            regions.append((lo, hi + 1, bytes(self.uc.mem_read(lo, end - lo))))
        attrs = {k: v for k, v in self.__dict__.items() if k not in self.SNAPSHOT_SKIP}
        blob = pickle.dumps({'version': self.SNAPSHOT_VERSION, 'regions': regions,
                             'context': self.uc.context_save(), 'attrs': attrs}, protocol=5)
        tmp = path + '.%d' % os.getpid()
        with open(tmp, 'wb') as f:
            f.write(blob)
        os.replace(tmp, path)

    def restore(self, path):
        """Replace this game (constructed with the same arguments, not booted) by a snapshot."""
        import pickle
        with open(path, 'rb') as f:
            snap = pickle.load(f)
        if snap.get('version') != self.SNAPSHOT_VERSION:
            raise ValueError('old snapshot format')
        mapped = {(lo, hi + 1) for lo, hi, _ in self.uc.mem_regions()}
        for lo, end, data in snap['regions']:
            if (lo, end) not in mapped:
                raise ValueError('snapshot region 0x%08X not mapped here' % lo)
            self.uc.mem_write(lo, data)
            if lo + len(data) < end:  # the unused rest of a heap: zero, as never used
                self.uc.mem_write(lo + len(data), bytes(end - lo - len(data)))
        self.uc.context_restore(snap['context'])
        self.__dict__.update(snap['attrs'])
        tables = {'IDirectDrawClipper': getattr(self, 'clipper_methods', {}),
                  'IDirectDrawSurface': getattr(self, 'surface_methods', {})}
        for name in self.imports.values():
            if name.startswith('COM!') and name[4:] not in self.handlers:
                label, _, off = name[4:].rpartition('+')
                n, fn = self.com_method(tables.get(label, {}), label, int(off, 16))
                if not callable(fn):
                    value = fn
                    fn = lambda e, *a, value=value: value  # noqa: E731
                self.handlers[name[4:]] = (n, fn)
        self.uc.ctl_flush_tb()

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
