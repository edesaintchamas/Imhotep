"""Imhotep - differential test: starting a new game against the original (roadmap phase 6).

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tests/differential/oracle_newgame_check.py [CASE ...] [-j JOBS] [--ticks N]

CASE may also be a group: flow (FLOW_CASES), flow2 (FLOW2_CASES).

A case is map:NAME (a scenario of local/game/app/Maps), mission:N (record N of mission1.pak) or
list:N (mission N from the mission list: campaign kind 3, the start with flag 1, then the briefing's
start with flag 0), optionally with @D for difficulty D (default: the booted game's), +carry (a
carry-over block of a won mission set by hand: CARRY) and +save=PATH (started from a running city:
the save loaded first). The campaign flow (package flow, core/newgame/flow.c): +kind=K (campaign kind
of a mission: 1 Pharaoh, the default, 2 Cleopatra; with +next or +replay the campaign step and period
of the mission are set from the campaign table), +next (then the victory screen's "continue"
0x00418640: the mission won, the progress file written, the next step), +replay (then "replay
mission" 0x00418510), +autosave=PATH (the player's Save\\<family>\\autosave_replay.sav, which the
Pharaoh campaign's replay loads); and the cases menu (the main menu 0x004CB6A0 on the booted game;
+last=PATH: the last saved game 0x00700998) and campaign:NAME (campaign.txt read again by 0x0041AE40,
or with NAME 'test' a file with every kind of line and error, CAMPAIGN_TEST). Package flow2
(core/newgame/flow2.c): family:ROW, select:ROW, newfamily:NAME (the family list, the progress file read,
a new family), menu+family=NAME (saved games listed), maps (the scenario list of the maps directory),
period:P, choice:STEP, continue:SCREEN (FLOW2_CASES). Player files are given to
both sides (the oracle's Game.files, the port's directory IMHOTEP_PLAYER_FILES, tools/oracle/
state_replay.c), and the files the original writes (the progress file Save\\<family>.dat) must be
written by the port with the same bytes. For each case the reference
game boots in the emulator, the start inputs are set (tools/oracle/newgame.py), then 0x005405B0
runs in the original and its port runs on a copy of the same state, the original's unported calls
replayed (tools/oracle/harness.py compare); the whole states must be equal. With --ticks N, a
chained lockstep of N ticks follows from the started game (tools/oracle/lockstep.py --start).
Default cases: every bundled map and every mission1.pak record at the default difficulty, and a
few at difficulties 0 and 4. Exit status: 0 if all equal, 1 otherwise.
"""
import os
import struct
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))
GAME_DIR = os.environ.get('IMHOTEP_GAME_DIR', os.path.join(ROOT, 'local', 'game', 'app'))
MISSIONS = 53  # records 0..52 of mission1.pak hold a city; 53..59 are empty (docs/newgame.md)


def default_cases():
    maps = sorted(f[:-4] for f in os.listdir(os.path.join(GAME_DIR, 'Maps')) if f.lower().endswith('.map'))
    cases = ['map:%s' % m for m in maps] + ['mission:%d' % n for n in range(MISSIONS)]
    cases += ['map:Default@0', 'map:Default@4', 'mission:0@0', 'mission:20@4']
    cases += ['list:5', 'list:39@0', 'mission:49+carry', 'mission:51+carry', 'mission:44+carry',
              'map:Default+save=%s' % SAVES[0], 'mission:12@4+save=%s' % SAVES[1], 'list:21+save=%s' % SAVES[2]]
    cases += FLOW_CASES + FLOW2_CASES
    return cases


SAVES = ['local/saves/downloaded/1592/39-Actium-Dec30c.sav', 'local/saves/downloaded/1564/CaesarAlan-Iken.sav',
         'local/saves/downloaded/1591/38-AlexC-Feb15.sav']
# a won mission's carry-over (docs/newgame.md 3.2, 0x004188F0): kingdom rating, the best formations,
# monument positions (as 0x004188F0 leaves them for 38-AlexC-Feb15.sav, and hand-set 'V' ones)
CARRY = {0x006FDCB8: bytes.fromhex('844e0000' '00000000' '2a000000' '03000000' '02000000' '22000000' '01000000'),
         0x0070278C: bytes.fromhex('30003000' '50005000' '20006000' '36007a00' '47006900' '5e002800')}


# package flow: the next mission after a win (a plain next step, a choice of missions, the end of a period,
# the last Cleopatra mission, a list mission, a custom map), a replay of each campaign kind, the main menu,
# campaign.txt
FLOW_CASES = ['mission:0+next', 'mission:4+next', 'mission:5+next', 'mission:7+next', 'mission:23+next',
              'mission:39+kind=2+next', 'mission:52+kind=2+next', 'mission:45+kind=2+next+carry', 'list:5+next',
              'map:Default+next', 'mission:12+next+save=%s' % SAVES[1],
              'map:Default+replay', 'map:Kyrene@4+replay', 'mission:3+replay', 'mission:3+replay+autosave=%s' % SAVES[1],
              'mission:30@0+replay+autosave=%s' % SAVES[0], 'mission:41+kind=2+replay', 'list:5+replay',
              'menu', 'menu+last=campaign.txt', 'menu+last=nothing.sav', 'campaign:campaign.txt', 'campaign:test']

# package flow2: the family list and the progress file read (family:ROW, families(): ROW the one chosen,
# 0x00EADFCC; none: no family, the new family screen; select:ROW the row clicked on a loaded list, +old
# with a progress file of the older size, +scroll=N), a new family typed in (0x00418220 on screen 0x36),
# the main menu with saved games in the family's directory, the scenario list of the maps directory, the
# period choice (0x0041BB70: PERIOD with +kind=2 for Cleopatra, +closed for a period not open), the
# mission choice (0x00418220 on screen 0x34 at the step STEP, choice +pick=1|2) and the briefing's
# continue (screen 0x35)
FLOW2_CASES = ['family:-1', 'family:0', 'family:1', 'family:2', 'family:7', 'family:0+done', 'family:none',
               'family:none+done', 'newfamily:Ramses', 'newfamily: \tRamses II  ', 'newfamily:nefertiti',
               'newfamily:   ', 'menu+family=Nefertiti', 'menu+family=Zed', 'menu+family=Nobody', 'maps',
               'maps+family=Nefertiti'] + ['period:%d' % p for p in range(5)] + [
               'period:2@0', 'period:3+closed', 'period:4+kind=2', 'period:6+kind=2', 'period:8+kind=2',
               'period:5', 'choice:5+pick=1', 'choice:5+pick=2', 'choice:5', 'continue:53', 'continue:1@0',
               'select:0+old', 'select:1+old', 'select:2+old', 'select:0+old+scroll=2', 'select:1+old+scroll=-1',
               'select:3+old']


def progress_file(g, size, won=(), text=b'', words=b''):
    """a progress file (0x0041B680): the booted campaign table with missions won (records +0x2CD4, 0x4C
    bytes, +0x48), a string at +0x2C90 and the words at +0x4A84, cut to `size` bytes"""
    t = bytearray(g.read(0x006FDD08, 0x4A9C))
    for m in won:
        t[0x2CD4 + 0x4C * m + 0x48:0x2CD4 + 0x4C * m + 0x4C] = bytes([1, 0, 0, 0])
    t[0x2C90:0x2C90 + len(text) + 1] = text + b'\0'
    t[0x4A84:0x4A84 + len(words)] = words
    return bytes(t[:size])


def families(g, tmp, old=False):
    """families: a progress file of the current size, one too short, with `old` one of the older
    0x4A6C bytes; saved games in one's directory. The older file leaves 0x30 bytes of the reader's
    frame in the table (0x0041B680): the stack, cleared in the check, must not hold earlier calls'
    frames there (the select cases)"""
    player_file(g, tmp, 'Save\\Nefertiti.dat', progress_file(g, 0x4A9C, (0, 1, 2, 5, 9), b'Hello',
                                                                bytes(range(1, 25))))
    if old:
        player_file(g, tmp, 'Save\\akhenaten.dat', progress_file(g, 0x4A6C, (0, 3, 40), b'old file'))
    player_file(g, tmp, 'Save\\Zed.dat', progress_file(g, 100))
    player_file(g, tmp, 'Save\\Nefertiti\\one.sav', b'not read')
    player_file(g, tmp, 'Save\\Nefertiti\\Two.SAV', b'not read')


def clear_stack(g):
    """the emulator's stack cleared: the original reads a file into a frame that it does not fill"""
    from emu import STACK_TOP, STACK_SIZE
    g.write(STACK_TOP - STACK_SIZE, bytes(STACK_SIZE - 0x100))


# a campaign file with every kind of line of 0x0041AE40 and each of its errors
CAMPAIGN_TEST = b"""; test campaign\r
\r
[MISSION_NAMES]\r
Alpha Town   \r
B\r
Gamma City With A Very Long Name That Exceeds Fifty Characters\r
Delta\r
Epsilon\r
mission=0,1,2\r
[PREDYNASTIC_PERIOD]\r
mission=0,200,300\r
mission=1,201,301,0\r
choicescreen=5,6\r
choice=1,10,20,700\r
choice=2,30,40,701\r
choice=3,1,1,1\r
mission=2,202,302,1,merge,5,x,7\r
mission=3 , 203 , 303 , 2 ,=, 4\r
garbage line here\r
[archaic_period]\r
mission=9,1,1\r
mission=-1,1,1\r
mission=1,a,1\r
mission=1,1\r
mission=1,1,1,300\r
mission=1,1,1,-\r
mission=1,1,1,2,m,-4\r
mission=1,1,1,2,m,256\r
choice=1,2\r
choice=1,2,x\r
choice=1,2,3\r
choice=x\r
choice=-1\r
choicescreen=1\r
choicescreen=x,1\r
mission=0,1,1,0,,\r
[NEW_KINGDOM]\r
choicescreen=1,2\r
choice=9,1,2,3\r
choice=8,1,2,99999\r
mission=4,4,5,--5\r
\x1a this is after the end"""


def player_file(g, tmp, name, data):
    """a player file for both sides: the oracle's Game.files and the port's IMHOTEP_PLAYER_FILES"""
    g.add_file(name, bytes(data))
    path = os.path.join(tmp, 'player', *name.replace('\\', '/').split('/'))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, 'wb').write(data)


def flow_calls(g, tmp, what, opts):
    """the calls of a flow case (menu, campaign:NAME, +next, +replay) and its setup"""
    from emu import STACK_TOP, STACK_SIZE
    if 'family' in opts:
        families(g, tmp)
        g.write(0x00E38E74, opts['family'].encode()[:31].ljust(32, b'\0'))
    if what == 'menu':
        if 'last' in opts:
            g.write(0x00700998, opts['last'].encode() + b'\0')
        return [(0x004CB6A0, [])]
    if what.startswith('family:'):
        clear_stack(g)
        if what != 'family:none':
            families(g, tmp)
            g.write(0x00EADFCC, struct.pack('<i', int(what[7:])))
        return [(0x00516BB0, [])] + ([(0x00516990, [])] if 'done' in opts else [])
    if what.startswith('select:'):
        # the family list loaded first (0x00516B50), then the row chosen from a clean stack
        families(g, tmp, 'old' in opts)
        p = g.alloc(16)
        g.write(p, b'Save\\*.dat\0')
        g.call(0x00516B50, p)
        clear_stack(g)
        g.write(0x00D3B198, struct.pack('<i', int(what[7:])))
        g.write(0x00EADFC8, struct.pack('<i', int(opts.get('scroll') or 0)))
        return [(0x00516AB0, [])]
    if what.startswith('newfamily:'):
        clear_stack(g)
        families(g, tmp)
        g.write(0x00E38E74, what[10:].replace('\\t', '\t').encode()[:31].ljust(32, b'\0'))
        g.write(0x00E38DDA, b'\x36')
        return [(0x00418220, [])]
    if what == 'maps':
        g.write(0x00704DB0, bytes(4))
        return [(0x0041F870, [])]
    if what.startswith('period:'):
        period = int(what[7:])
        g.write(0x006FDC98, struct.pack('<i', int(opts.get('kind') or 1) - 1))
        g.write(0x007027AC, struct.pack('<i', period))
        g.write(0x007027B0, struct.pack('<i', period + ('closed' in opts)))
        return [(0x0041BB70, [])]
    if what.startswith('choice:'):
        step = int(what[7:])
        g.write(0x00E38DDA, b'\x34')
        g.write(0x00E38EAB, bytes([step]))
        g.write(0x00E38EA9, g.read(0x006FDD08 + 0x40 * step, 1))
        g.write(0x007027A4, bytes([int(opts.get('pick') or 0)]))
        return [(0x00418220, [])]
    if what.startswith('continue:'):
        g.write(0x00E38DDA, bytes([int(what[9:])]))
        g.write(0x00EA1870 + 0x49D8 * g.u32(0x00EAAC28) + 0x47A8, struct.pack('<i', 3))
        return [(0x00418220, [])]
    if what.startswith('campaign:'):
        name = what[9:]
        if name == 'test':
            name = 'campaign_test.txt'
            player_file(g, tmp, name, CAMPAIGN_TEST)
        g.write(0x00E37078, name.encode() + b'\0')
        # the step being read is a local that 0x0041AC00 leaves partly uninitialised: a cleared stack
        g.write(STACK_TOP - STACK_SIZE, bytes(STACK_SIZE - 0x100))
        return [(0x0041AE40, [0x00E37078])]
    return []


def campaign_step(g, mission):
    """(step, period) of the first step of the campaign table 0x006FDD08 that plays `mission`"""
    for i in range(g.i32(0x006FDD08 + 0x1900)):
        if g.i32(0x006FDD08 + 0x40 * i + 0xC) == mission:
            return i, g.read(0x006FDD08 + 0x40 * i, 1)[0]
    return 0xFF, 0


def check(case, ticks):
    """'equal' or a description of the first differences"""
    import harness
    import newgame
    from game import Game
    head, *options = case.split('+')
    what, _, difficulty = head.partition('@')
    opts = dict(o.partition('=')[::2] for o in options)
    g = Game()
    g.boot()
    for o in options:
        if o.startswith('save='):
            if g.load_save(open(os.path.join(ROOT, o[5:]), 'rb').read()) != 0:
                return 'the original could not load ' + o[5:]
    tmp_dir = tempfile.TemporaryDirectory()
    tmp = tmp_dir.name
    os.makedirs(os.path.join(tmp, 'player'))
    os.environ['IMHOTEP_PLAYER_FILES'] = os.path.join(tmp, 'player')
    listed = what.startswith('list:')
    calls = flow_calls(g, tmp, what, opts)
    if difficulty and calls:
        g.write(0x00E39080, struct.pack('<i', int(difficulty)))
    if not calls:
        kind = 3 if listed else int(opts.get('kind') or 1)
        call = newgame.inputs(g, 'mission:' + what[5:] if listed else what, int(difficulty) if difficulty else None,
                              campaign=kind)
        calls = [(call[0], [call[1][0], 1]), call] if listed else [call]
        if ('next' in opts or 'replay' in opts) and what.startswith('mission:'):
            step, period = campaign_step(g, int(what[8:]))
            g.write(0x00E38EAB, bytes([step]))
            g.write(0x00E38EA9, bytes([period]))
        calls += [(0x00418640, [])] if 'next' in opts else []
        calls += [(0x00418510, [])] if 'replay' in opts else []
    if 'autosave' in opts:
        family = g.read(0x00E38E74, 32).split(b'\0')[0].decode('latin-1')
        player_file(g, tmp, 'save\\%s\\autosave_replay.sav' % family, open(os.path.join(ROOT, opts['autosave']), 'rb').read())
    if 'carry' in options:
        for a, v in CARRY.items():
            g.write(a, v)
    files = {k: bytes(v) for k, v in g.files.items()}
    try:
        written, ranges, bad_returns = harness.compare(g, calls, tmp)
    except RuntimeError as e:
        tmp_dir.cleanup()
        return str(e)[:600]
    out = []
    for name, data in g.files.items():  # the files the original wrote, as the port wrote them
        if files.get(name) == bytes(data) or name == 'oracle.sav':
            continue
        path = os.path.join(tmp, 'player', *name.split('/'))
        mine = open(path, 'rb').read() if os.path.exists(path) else None
        if mine is None:
            found = [os.path.join(d, f) for d, _, fs in os.walk(os.path.join(tmp, 'player')) for f in fs
                     if os.path.relpath(os.path.join(d, f), os.path.join(tmp, 'player')).lower() == name]
            mine = open(found[0], 'rb').read() if found else None
        if mine != bytes(data):
            out.append('file %s: %s' % (name, 'not written by the port' if mine is None else
                                         '%d bytes differ' % sum(a != b for a, b in zip(mine, data)) if
                                         len(mine) == len(data) else 'size %d, port %d' % (len(data), len(mine))))
    tmp_dir.cleanup()
    if what.startswith('campaign:'):
        # the CRT strtok's position 0x012663C8 (static of 0x00561DB1): a pointer into the original's
        # stack frame (0x0041AE40's line buffer), read only by strtok(NULL) after a strtok(s); the port
        # keeps its position in C. To be moved to tools/oracle/excluded.py.
        ranges = [(a, n) for a, n in ranges if not (a <= 0x012663C8 < a + n and n <= 4)]
    for (a, args), ro, rp in bad_returns:
        out.append('0x%08X returned %d, port %d' % (a, ro, rp))
    if ranges:
        import diff
        out.append('%d bytes differ in %d runs (the original changed %d): %s' % (
            sum(n for _, n in ranges), len(ranges), written,
            ', '.join(diff.name_address(a) for a, _ in ranges[:6])))
    if out:
        return '; '.join(out)
    if ticks and not options and not listed and ':' in what and not what.startswith('campaign:'):
        r = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'oracle', 'lockstep.py'), '--start', case,
                            '--ticks', str(ticks), '--chained'], capture_output=True, text=True)
        if r.returncode != 0:
            return 'after the start: ' + ' / '.join((r.stdout.strip().splitlines() or [''])[-3:] +
                                                     r.stderr.strip().splitlines()[-2:])
    return 'equal'


def main():
    argv = sys.argv[1:]
    jobs, ticks = 1, 0
    if '-j' in argv:
        k = argv.index('-j')
        jobs = int(argv[k + 1])
        del argv[k:k + 2]
    if '--ticks' in argv:
        k = argv.index('--ticks')
        ticks = int(argv[k + 1])
        del argv[k:k + 2]
    groups = {'flow': FLOW_CASES, 'flow2': FLOW2_CASES}  # a group name stands for its cases
    cases = [c for a in argv for c in groups.get(a, [a])] or default_cases()
    from concurrent.futures import ProcessPoolExecutor
    passed = 0
    with ProcessPoolExecutor(jobs) as pool:
        for case, result in zip(cases, pool.map(check, cases, [ticks] * len(cases))):
            print('  %-28s %s' % (case, result), flush=True)
            passed += result == 'equal'
    print('oracle_newgame_check: %d / %d starts equal to the original' % (passed, len(cases)))
    return 0 if passed == len(cases) else 1


if __name__ == '__main__':
    sys.exit(main())
