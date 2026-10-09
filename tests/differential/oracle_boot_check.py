"""Imhotep - differential test: the game's start-up (core/boot) against the original's.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tests/differential/oracle_boot_check.py [CASE ...] [--steps] [--downstream]
        [--keep DIR]

CASE: none (no display), WxHxF (a display: 640x480x555, 800x600x565, 1024x768x565, ...), with +sound
for the sound start-up, +windowed for a windowed game (Pharaoh.inf as the game's F6 leaves it: the
resolution below the display's, game.py display_options) (default: none, none+sound, the four display
cases of the frame checks, a windowed one).

The original boots in the emulator (tools/oracle/game.py Game.boot: the static constructors, then WinMain's
steps; its own run, step by step, with every allocation it makes outside the state logged). The port
(core/boot game_boot, run by build/oracle_boot_run: tests/differential/oracle_boot_run.c) boots a state
made from Pharaoh.exe's sections (platform/exe_image.c), with the game files from the platform. The two
booted states (data segments and simulation heap) must be equal byte for byte, and with a display the
back buffers too.

Memory outside the state (the oracle's "other heap", 0x68000000: the text files, the spare screen, the
pixel data, the DirectDraw surfaces, the sound registry ...) is the host's in the port (core/boot/boot.h
boot_host): the state holds only addresses into it. The run gives the port the original's addresses: each
allocation the port makes (with the original's call site) takes the original's next allocation of that
site, which must have the same size (oracle_boot_run --allocs). The original's allocations that are not
the game's are left out: the oracle's own blocks (the boot's string arguments, the x87 control word's
code, the FILE blocks of its fopen, the spare screen of its constructor, the DirectDraw COM objects);
allocations of the game that the port does not make are listed by site ("skipped": the graphics heap's
list nodes of 0x0053DFF0, which hold no state).

Known differences (KNOWN, KNOWN_BLOCKS, FREE_SITES and the rules of known_rules; docs/formats/state.md
"The booted state") are listed with their reasons and are not failures: bytes nothing initialises in the
original (stack contents), the import table, the oracle's omissions (0x00E38DD6, EVENTMSG.TXT), the
tools/oracle/excluded.py ranges, and blocks outside the state that are the platform's (DirectDraw objects,
the primary surface).

--steps also runs each step alone from the original's state before it (the arena: the original's other
heap) and compares it with the original's state after it: where a difference comes from.
--downstream then starts from the port's booted state on both sides (its known bytes taken from the
original's: they point into the oracle's memory): a save loaded (0x004D5100) and ticks, two new games
started (0x005405B0) and ticks, each the original against the port (tools/oracle/harness.py compare,
unported callees replayed); with a display also frames of the city view (0x004D6B60), whose outcome must
be the same as from the original's own boot.
Exit status: 0 if all equal, 1 otherwise, 2 on setup errors.
"""
import hashlib
import os
import pickle
import struct
import subprocess
import sys
import tempfile
import traceback

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))
GAME_DIR = os.environ.get('IMHOTEP_GAME_DIR', os.path.join(ROOT, 'local', 'game', 'app'))
BUILD = os.environ.get('IMHOTEP_BUILD', 'build')
RUNNER = os.path.join(ROOT, BUILD, 'oracle_boot_run')
CACHE = os.path.join(ROOT, 'local', 'oracle', 'cache')
STATE_BASE, STATE_END = 0x0056F000, 0x01267AA8
DEFAULT = ['none', 'none+sound', '640x480x555', '800x600x565', '1024x768x565', '1024x768x555', '1024x768x565+sound',
           '1024x768x555+windowed']
WINDOWED = False  # the current case's +windowed (oracle_trace, player_dir)

# the oracle's own allocations, by the Python function that makes them (not the game's)
ARTIFACTS = {'_fopen': 'FILE block of the CRT fopen hook (the real CRT uses its _iob table)',
             'fpu_control': 'code and data of the x87 control word load',
             'com_object': 'DirectDraw COM object and its vtable (the platform\'s)',
             '__init__': 'the spare screen set by Game() (the game allocates its own, 0x00414420)'}
VIDEO_SITE = 0x004CFD13  # operator new(0x1C) of the Bink object in 0x004CF960 (game.py 'video' makes it)

# allocations of the port whose size differs from the oracle's for a known reason (site: reason)
FREE_SITES = {
    0x004CBB2B: 'EVENTMSG.TXT: in the oracle the CRT _fileno of its fopen FILE block is 0, so _filelength fails '
                '(-1) and 0x004CBA60 reads nothing (blocks of 0 bytes); the port reads the file as the game does',
    0x004CBB7B: 'EVENTMSG.TXT strings: the same (the oracle\'s block is empty)',
}

# blocks outside the state whose bytes differ for a known reason (site: reason)
KNOWN_BLOCKS = {
    0x004CBAB3: 'the event phrases\' pointers (0x004CBA60): the port\'s point into the strings it read; the '
                'oracle\'s stay 0 (EVENTMSG.TXT not read: FREE_SITES)',
    0x43000000: 'DirectDraw\'s objects: in the oracle COM objects with their vtables (emu.py com_object); for the '
                'port opaque handles of the platform, no memory',
}
PRIMARY_SURFACE = ('the primary surface (the screen): the oracle\'s present (0x004D0280, Blt) copies the back '
                   'buffer into it; the port\'s present is a platform request')

# bytes of the booted state that differ for a known reason (address, length, reason)
KNOWN = [
    (0x00E38DD6, 1, 'WinMain sets it to 1 after 0x004CF8E0 (dis 0x0041424B); game.py Game.boot does not'),
    (0x00784F28, 4, 'the event phrases\' string block (0x004CBA60): EVENTMSG.TXT is not read in the oracle '
                    '(FREE_SITES), so the block is another one'),
]


def runner():
    """build the port's side (core + present + the driver) into BUILD"""
    r = subprocess.run(['make', '-s', 'BUILD=' + BUILD, BUILD + '/libimhotep_core.a', BUILD + '/libimhotep_present.a'],
                       cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError(r.stdout + r.stderr)
    src = os.path.join(ROOT, 'tests', 'differential', 'oracle_boot_run.c')
    libs = [os.path.join(ROOT, BUILD, 'libimhotep_present.a'), os.path.join(ROOT, BUILD, 'libimhotep_core.a')]
    deps = [src, os.path.join(ROOT, 'platform', 'exe_image.c'), os.path.join(ROOT, 'platform', 'game_files.c')] + libs
    if os.path.exists(RUNNER) and all(os.path.getmtime(RUNNER) >= os.path.getmtime(d) for d in deps):
        return
    cmd = [os.environ.get('CC', 'cc'), '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-I' + os.path.join(ROOT, 'core'),
           '-I' + ROOT, src, os.path.join(ROOT, 'platform', 'exe_image.c'), os.path.join(ROOT, 'platform', 'game_files.c')]
    r = subprocess.run(cmd + libs + ['-o', RUNNER], capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError(r.stdout + r.stderr)


def parse_case(case):
    """(display, sound); sets WINDOWED"""
    global WINDOWED
    head, *tail = case.split('+')
    from game import parse_display
    WINDOWED = 'windowed' in tail
    return (None if head == 'none' else parse_display(head)), 'sound' in tail


def code_hash():
    h = hashlib.sha1()
    for f in ('emu.py', 'game.py', 'sound.py'):
        p = os.path.join(ROOT, 'tools', 'oracle', f)
        if os.path.exists(p):
            h.update(open(p, 'rb').read())
    h.update(open(__file__, 'rb').read())
    return h.hexdigest()[:16]


def oracle_trace(display, sound):
    """The original's boot, step by step: [(step name, state before, state after, other-heap top
    before, [(site, size, address, kind)])], the final other heap, the final game. Cached in
    local/oracle/cache (keyed by the display, sound and the oracle's code)."""
    from game import Game, display_name
    from emu import OTHER_HEAP_BASE
    from unicorn.x86_const import UC_X86_REG_ESP
    key = hashlib.sha1(repr((display_name(display), sound, code_hash()) + (('windowed',) if WINDOWED else ())).encode()).hexdigest()[:20]
    path = os.path.join(CACHE, 'boottrace_%s.pkl' % key)
    if os.path.exists(path):
        with open(path, 'rb') as f:
            return pickle.load(f)
    g = Game(display=display, sound=sound)
    g.windowed = WINDOWED
    log = []
    plain = g.alloc
    current = ['ctor']

    def alloc(n, sim=False):
        p = plain(n, sim)
        names = [fr.name for fr in traceback.extract_stack()[:-1]]
        kind = 'game'
        for name in names:
            if name in ARTIFACTS:
                kind = 'oracle:' + name
        if names[-1] == 'boot':  # the boot's own: string arguments, or the Bink object of 'video'
            kind = 'game' if current[0] == 'video' else 'oracle:boot'
        if '_create_surface' in names:
            kind = 'surface'
        if 'com_object' in names:  # a COM object (DirectDraw's): its vtable (256 bytes) and the object
            kind = 'com' if n == 16 else 'oracle:vtable'
        try:
            site = g.u32(g.uc.reg_read(UC_X86_REG_ESP))
        except Exception:  # noqa: BLE001 - no emulated stack (Python-side allocation)
            site = 0
        if current[0] == 'video' and kind == 'game':
            site = VIDEO_SITE
        if kind == 'surface':
            site = 0x53000000
        if kind == 'com':
            site = 0x43000000
        if not sim:
            log.append((site, n, p, kind, current[0]))
        return p
    g.alloc = alloc
    steps = []
    for step in g.boot_steps():
        name = step[0] if isinstance(step[0], str) else '0x%08X' % step[0]
        current[0] = name
        pre = g.state_bytes()
        top = g.other_heap
        start = len(log)
        saved = g.boot_steps
        g.boot_steps = lambda step=step: [step]
        g.boot()
        g.boot_steps = saved
        allocs = log[start:]
        if name == 'display':  # DirectDrawCreate gives the object Game() made (game.py _display_stubs)
            allocs = [(0x43000000, 16, g.dd, 'com', name)] + allocs
        steps.append((name, pre, g.state_bytes(), top, allocs))
    heap = g.read(OTHER_HEAP_BASE, g.other_heap - OTHER_HEAP_BASE)
    trace = {'steps': steps, 'heap': heap, 'screen': g.screen_info(), 'buffer': g.screen()}
    os.makedirs(CACHE, exist_ok=True)
    with open(path + '.tmp', 'wb') as f:
        pickle.dump(trace, f, protocol=5)
    os.replace(path + '.tmp', path)
    return trace


def schedule(allocs):
    return ''.join('%s %d %08X\n' % ({0x53000000: 'S', 0x43000000: 'C'}.get(site, '%08X' % site), n, p)
                   for site, n, p, kind, _ in allocs if kind in ('game', 'surface', 'com'))


def display_arg(display):
    return [] if display is None else ['--display', '%d,%d,%d' % display]


def player_dir(tmp, display):
    """the player's files of the oracle's boot (Game.files: the Pharaoh.inf of a display boot,
    game.py display_options) in a directory for the port's runner (--player)"""
    from game import Game
    path = os.path.join(tmp, 'player')
    os.makedirs(path, exist_ok=True)
    g = Game.__new__(Game)  # only the options file: no emulator
    g.display, g.files, g.names, g.windowed = display, {}, {}, WINDOWED
    g.display_options()
    for key, data in g.files.items():
        open(os.path.join(path, g.names[key].split('/')[-1]), 'wb').write(data)
    return path


def run_port(tmp, src, steps, display, sound, allocs, arena=None):
    """the port's steps on src (a state file's bytes or 'exe'); (exit, output, state bytes, arena bytes)"""
    inp = os.path.join(tmp, 'in.state')
    if src != 'exe':
        open(inp, 'wb').write(src)
    out, sched, arena_in, arena_out = (os.path.join(tmp, n) for n in ('out.state', 'allocs', 'arena.in', 'arena.out'))
    open(sched, 'w').write(allocs)
    cmd = [RUNNER, 'exe' if src == 'exe' else inp, out, '--game', GAME_DIR, '--player', player_dir(tmp, display), '--steps', '%d-%d' % steps,
           '--allocs', sched, '--arena-out', arena_out, '--free-sites', ','.join('%08X' % a for a in FREE_SITES)] + display_arg(display) + (['--sound'] if sound else [])
    if arena is not None:
        open(arena_in, 'wb').write(arena)
        cmd += ['--arena', arena_in]
    r = subprocess.run(cmd, capture_output=True, text=True)
    state = open(out, 'rb').read() if os.path.exists(out) else None
    buf = open(arena_out, 'rb').read() if os.path.exists(arena_out) else b''
    return r.returncode, r.stdout + r.stderr, state, buf


def differences(a, b):
    """[(address, length)] where the state files a and b differ (segments, then the heap by address)"""
    import diff
    return diff.state_file_diff(a, b)


CAMPAIGN, CAMPAIGN_STEPS = 0x006FDD08, 0x006FDD08 + 0x1900
# @bin 0x0041AE40: campaign.txt's steps are built in a 64-byte record of its stack frame, initialised by
# 0x0041AC00 except these bytes, and copied whole into the table (0x0041AC90): what the stack held there
CAMPAIGN_GAPS = (2, 3, 4, 5, 6, 0x2B, 0x37, 0x3D, 0x3E, 0x3F)


# the monument objects (core/boot/monuments.c): 0x004E5140 copies 13 dwords of 0x004E5210's stack temporary
# into the base part at +0xC, whose last 9 (+0x1C .. +0x3F of the object) nothing initialised: the stack's
# contents (in the oracle return addresses, SEH frames, stack pointers of its emulated stack); the parts
# WinMain lays out later (0x004E52B0) are rewritten, this one is not
MONUMENT_OBJECTS = (0x006FD5D8, 0x006FD368, 0x006F9320, 0x006F5648, 0x006E9188, 0x006DE9E0, 0x006DE650, 0x006DE548)


def import_table():
    """(address, size) of Pharaoh.exe's import address table (PE data directory 12)"""
    data = open(os.path.join(GAME_DIR, 'Pharaoh.exe'), 'rb').read(0x1000)
    pe = struct.unpack_from('<I', data, 0x3C)[0]
    base = struct.unpack_from('<I', data, pe + 24 + 28)[0]
    rva, size = struct.unpack_from('<II', data, pe + 24 + 96 + 8 * 12)
    return base + rva, size


def known_rules(state):
    """addr -> reason or None: the KNOWN bytes, the stack bytes of the campaign table's steps, and the
    state tools/oracle/excluded.py excludes from every comparison"""
    import excluded
    steps = struct.unpack_from('<i', state, CAMPAIGN_STEPS - STATE_BASE)[0]
    gaps = {CAMPAIGN + 0x40 * k + o for k in range(max(0, min(steps, 100))) for o in CAMPAIGN_GAPS}
    rule = excluded.excluder(state)
    iat, iat_size = import_table()

    def known(addr):
        for a, n, why in KNOWN:
            if a <= addr < a + n:
                return why
        if iat <= addr < iat + iat_size:
            return ('the import address table: the Windows loader writes the DLLs\' entry points there (the '
                    'oracle its stubs, tools/oracle/emu.py); core calls no DLL')
        for o in MONUMENT_OBJECTS:
            if o + 0x1C <= addr < o + 0x40:
                return ('a monument object\'s base part: 9 dwords copied from an uninitialised stack temporary '
                        '(0x004E5210, 0x004E5140)')
        if addr in gaps:
            return 'campaign step bytes 0x0041AC00 leaves: the stack of 0x0041AE40 (uninitialised)'
        r = rule(addr)
        return 'excluded.py: ' + r if r else None
    return known


# the parts of the monument objects (offset of the parts, count): after the constructors their dwords
# +0x10 .. +0x33 hold the stack temporary's contents too, until WinMain lays them out (0x004E52B0)
MONUMENT_PARTS = ((0x006DE650, 0x4C, 0x10), (0x006FD5D8, 0x4C, 0x19), (0x006FD368, 0x4C, 9), (0x006DE548, 0x4C, 2),
                  (0x006F9320, 0x1E5C, 0xA7), (0x006F5648, 0x1CBC, 0x9E), (0x006DE9E0, 0x4EE0, 0x1B5),
                  (0x006E9188, 0x5C88, 0x201))


def with_parts(known):
    """known, and the parts' stack bytes before their layout (the constructors' step alone)"""
    def rule(addr):
        r = known(addr)
        if r:
            return r
        for o, parts, n in MONUMENT_PARTS:
            k, off = divmod(addr - o - parts, 0x34)
            if 0 <= k < n and 0x10 <= off < 0x34:
                return 'a monument part before its layout: the stack temporary\'s dwords (0x004E5140)'
        return None
    return rule


def split_known(ranges, known):
    """ranges [(address, length)] -> (unexplained ranges, {reason: bytes})"""
    bad, why = [], {}
    for a, n in ranges:
        run = None
        for x in range(a, a + n):
            r = known(x)
            if r:
                why[r] = why.get(r, 0) + 1
                run = None
            elif run is not None and run[0] + run[1] == x:
                run[1] += 1
            else:
                run = [x, 1]
                bad.append(run)
    return [tuple(r) for r in bad], why


def describe(ranges, a, b, limit=int(os.environ.get("BOOT_CHECK_LIMIT", "12"))):
    import diff
    out = []
    for addr, n in ranges[:limit]:
        k = addr - STATE_BASE if addr < 0x60000000 else None
        if k is None:
            out.append('    %s, %d bytes' % (diff.name_address(addr), n))
            continue
        k4 = k & ~3
        out.append('    %s, %d bytes: original %s port %s' % (
            diff.name_address(addr), n, a[k4:k4 + 4][::-1].hex(), b[k4:k4 + 4][::-1].hex()))
    return '\n'.join(out)


def check_steps(trace, display, sound, tmp):
    failed = 0
    from emu import OTHER_HEAP_BASE
    for k, (name, pre, post, top, allocs) in enumerate(trace['steps']):
        arena = trace['heap'][:top - OTHER_HEAP_BASE]
        code, out, state, _ = run_port(tmp, pre, (k, k), display, sound, schedule(allocs), arena)
        if state is None or code not in (0,):
            failed += 1
            lines = [l for l in out.splitlines() if not l.startswith(('alloc ', 'skipped'))]
            print('  step %2d %-12s: exit %d %s' % (k, name, code, ' | '.join(lines)[-300:]))
            continue
        rules = known_rules(post)
        if name == 'initializers':
            rules = with_parts(rules)
        bad, _ = split_known(differences(post, state), rules)
        if bad:
            failed += 1
            print('  step %2d %-12s: %d bytes differ in %d runs\n%s' % (k, name, sum(n for _, n in bad), len(bad),
                                                                     describe(bad, post, state)))
        else:
            print('  step %2d %-12s: equal' % (k, name))
    return failed


def skipped_summary(out):
    sites = {}
    for line in out.splitlines():
        if line.startswith('skipped '):
            _, site, n, _ = line.split()
            c = sites.setdefault(site, [0, 0])
            c[0] += 1
            c[1] += int(n)
    return ', '.join('%s x%d (%d bytes)' % (s, c[0], c[1]) for s, c in sorted(sites.items()))


def check_full(trace, display, sound, tmp, keep=None):
    allocs = [a for step in trace['steps'] for a in step[4]]
    n = len(trace['steps'])
    code, out, state, arena = run_port(tmp, 'exe', (0, n - 1), display, sound, schedule(allocs))
    final = trace['steps'][-1][2]
    if keep:
        os.makedirs(keep, exist_ok=True)
        if state:
            open(os.path.join(keep, 'port.state'), 'wb').write(state)
        open(os.path.join(keep, 'oracle.state'), 'wb').write(final)
        open(os.path.join(keep, 'run.txt'), 'w').write(out)
    if state is None or code != 0:
        lines = [l for l in out.splitlines() if not l.startswith(('alloc ', 'skipped'))]
        print('  boot: exit %d: %s' % (code, ' | '.join(lines)[-600:]))
        return 1, None
    bad, why = split_known(differences(final, state), known_rules(final))
    print('  boot: %d steps; skipped original allocations: %s' % (n, skipped_summary(out) or 'none'))
    for r, count in sorted(why.items()):
        print('    known: %d bytes: %s' % (count, r))
    failed = 0
    if bad:
        failed = 1
        print('  boot state: %d bytes differ in %d runs\n%s' % (sum(n for _, n in bad), len(bad),
                                                             describe(bad, final, state)))
    else:
        print('  boot state: equal (%d bytes of segments and heap)' % len(final))
    # the memory outside the state that the port allocated at the original's addresses: the same bytes
    # (text files, spare screen, pixel data, registry, voices, Bink object ...; the free sites excepted)
    blocks, unequal, known_blocks = 0, {}, {}
    heap = trace['heap']
    surfaces = 0
    for line in out.splitlines():
        w = line.split()
        if w[0] != 'alloc' or len(w) != 4:
            continue
        site, size, addr = int(w[1], 16), int(w[2]), int(w[3], 16)
        o = addr - 0x68000000
        blocks += 1
        primary = site == 0x53000000 and surfaces == 0
        surfaces += site == 0x53000000
        if heap[o:o + size] != arena[o:o + size].ljust(min(size, len(heap) - o), b'\0')[:len(heap[o:o + size])]:
            why = PRIMARY_SURFACE if primary else KNOWN_BLOCKS.get(site)
            if why:
                known_blocks[why] = known_blocks.get(why, 0) + 1
            else:
                unequal.setdefault('%08X' % site, []).append(addr)
    for why, n in sorted(known_blocks.items()):
        print('    known: %d blocks outside the state: %s' % (n, why))
    if unequal:
        failed = 1
        print('  outside memory: %d of %d blocks differ: %s' % (sum(map(len, unequal.values())), blocks, ', '.join(
            '%s x%d' % (k, len(v)) for k, v in sorted(unequal.items()))))
    else:
        print('  outside memory: the %d blocks the port allocated hold the original\'s bytes' % blocks)
    info = trace['screen']
    if info is not None:
        addr, w, h, pitch, fmt = info
        mine = arena[addr - 0x68000000:addr - 0x68000000 + pitch * h]
        if mine != trace['buffer']:
            import screen
            px, rects = screen.diff_rects(trace['buffer'], mine.ljust(pitch * h, b'\0'), w, h, pitch)
            failed = 1
            print('  back buffer: %d pixels differ in %d rectangles' % (px, len(rects)))
        else:
            print('  back buffer: equal (%dx%d)' % (w, h))
    return failed, state


def main():
    argv = sys.argv[1:]
    steps = '--steps' in argv
    downstream = '--downstream' in argv
    keep = None
    if '--keep' in argv:
        k = argv.index('--keep')
        keep = argv[k + 1]
        del argv[k:k + 2]
    cases = [a for a in argv if not a.startswith('--')] or DEFAULT
    try:
        runner()
    except RuntimeError as e:
        print('cannot build the port\'s side:\n%s' % e)
        return 2
    failed = 0
    with tempfile.TemporaryDirectory() as tmp:
        for case in cases:
            display, sound = parse_case(case)
            print('%s:' % case, flush=True)
            trace = oracle_trace(display, sound)
            if steps:
                failed += check_steps(trace, display, sound, tmp)
            f, state = check_full(trace, display, sound, tmp, keep and os.path.join(keep, case))
            failed += f
            if downstream and state is not None:
                final = trace['steps'][-1][2]
                failed += check_downstream(with_known(state, final), display, sound, tmp)
    print('oracle_boot_check: %d cases, %s' % (len(cases), 'all equal' if not failed else '%d failures' % failed))
    return 1 if failed else 0


def with_known(state, original):
    """the port's state with the bytes of the known differences taken from the original's (the import
    table, the oracle's pointers): the state both sides of the downstream runs start from"""
    known = known_rules(original)
    out = bytearray(state)
    for a, n in differences(original, state):
        for x in range(a, a + n):
            if known(x):
                k = x - STATE_BASE if x < 0x60000000 else None
                if k is not None:
                    out[k] = original[k]
    return bytes(out)


def frames_outcome(g, save, display, tmp):
    """the result of 6 frames of the city view after loading save in g: a short text"""
    import harness
    data = open(save, 'rb').read()
    g.files['oracle.sav'] = data
    if g.load_save(data) != 0:
        return 'the save does not load'
    g.write(0x00E38DDA, b'\x01')
    g.write(0x00E38DCA, b'\x01')
    g.cursor = (display[0] // 2, display[1] // 2)
    bad = 0
    for t in range(6):
        g.clock = 1000 + 40 * t
        try:
            _, ranges, bad_returns = harness.compare(g, [(0x004D6B60, [])], tmp)
        except RuntimeError as e:
            return 'frame %d: %s' % (t, ' '.join(str(e).split())[-160:])
        bad += bool(ranges or bad_returns)
    return 'equal (state and back buffer)' if not bad else '%d frames differ' % bad


def check_downstream(state, display, sound, tmp):
    """from the port's booted state on both sides: a save loaded and ticks, a new game started"""
    from game import Game
    import harness
    import newgame
    failed = 0
    save = os.path.join(ROOT, 'local', 'saves', 'downloaded', '1592', '39-Actium-Dec30c.sav')
    kw = {'display': display, 'sound': sound}
    whats = ('load', 'map:Default', 'mission:3') + (('frames',) if display is not None else ())
    for what in whats:
        g = Game.booted(**kw) if not sound else Game(**kw)
        if sound:
            g.boot()
        g.load_state(state)
        if what == 'frames':
            # frames of the city view of the save loaded from the booted state (as framestep.py: window 1, the
            # redraw flag, the cursor at the centre), clock 1000 ms then 40 ms apart, state and back buffer
            # compared after each; the same from the original's own booted state: the boot must not change
            # the outcome (the frame's port is another package's work: where it stops, it must stop the same)
            mine = frames_outcome(g, save, display, tmp)
            ref = Game.booted(**kw) if not sound else Game(**kw)
            if sound:
                ref.boot()
            theirs = frames_outcome(ref, save, display, tmp)
            same = mine == theirs
            print('  downstream frames: 6 frames of the city view after the load: %s; from the original\'s boot: '
                  '%s' % (mine, 'the same' if same else theirs))
            failed += not same
            continue
        if what == 'load':
            data = open(save, 'rb').read()
            g.files['oracle.sav'] = data
            player = os.path.join(tmp, 'player')  # the port reads the save through its platform
            os.makedirs(player, exist_ok=True)
            open(os.path.join(player, 'oracle.sav'), 'wb').write(data)
            os.environ['IMHOTEP_PLAYER_FILES'] = player
            g.write(0x00E37078, b'oracle.sav\0')  # the name, in the state (the scenario name buffer)
            calls = [(0x004D5100, [0x00E37078])]
        else:
            calls = [newgame.inputs(g, what)]
        written, ranges, bad_returns = harness.compare(g, calls, tmp)
        label = 'downstream %s' % what
        if ranges or bad_returns:
            failed += 1
            pre, post_o, post_p = harness.compare.last
            print('  %s: %d bytes differ\n%s' % (label, sum(n for _, n in ranges), describe(ranges, post_o, post_p)))
            continue
        ticks_bad = 0
        for t in range(3 if what == 'load' else 2):
            _, r2, b2 = harness.compare(g, [(0x004D7090, [])], tmp)
            if r2 or b2:
                ticks_bad += 1
        print('  %s: equal (the original changed %d bytes); ticks after it: %s' % (
            label, written, 'equal' if not ticks_bad else '%d differ' % ticks_bad))
        failed += bool(ticks_bad)
    return failed


if __name__ == '__main__':
    sys.exit(main())
