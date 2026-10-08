"""Imhotep - differential test: the front end's functions (package frontend, present/ui/frontend).

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tests/differential/oracle_frontend_check.py [CASE ...] [-j JOBS] [--display WxHxF]

Each case prepares the reference game (a booted game with a display, or a save loaded by the original),
the player files (progress files Save\\<family>.dat and saved games: the oracle's Game.files and the
port's IMHOTEP_PLAYER_FILES), runs some setup calls in the original only (open a window, fill a list),
writes some globals, then runs the case's calls in the original and in the ports
(tools/oracle/harness.compare): the state, the back buffer, the return values and the player files
written must be equal. The windows and their ids: docs/presentation.md 3.2, docs/newgame.md 2.
Exit status: 0 if every case is equal, 1 otherwise.
"""
import os
import struct
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))
sys.path.insert(0, os.path.join(ROOT, 'tests', 'differential'))

ACTIUM = 'local/saves/downloaded/1592/39-Actium-Dec30c.sav'
IKEN = 'local/saves/downloaded/1564/CaesarAlan-Iken.sav'
FAMILIES = ['Nefertiti', 'Zed', 'Amun', 'bastet', 'Hathor', 'Isis', 'Khufu', 'Menes', 'Narmer', 'Osiris', 'Ptah',
            'Ra', 'Seth', 'Thoth', 'Userkaf']
NEF = FAMILIES[0].encode().ljust(32, b'\0').hex()


def i32(v):
    return struct.pack('<i', v).hex()


def case(calls, save=None, families=0, saves=(), steps=(), clear_stack=False, files=()):
    """calls: 'ADDR[:args]' strings compared; steps: ('set', ADDR, HEX) or ('call', 'ADDR[:args]') in order;
    files: other player files (path, bytes)"""
    return {'calls': calls, 'save': save, 'families': families, 'saves': saves, 'steps': steps,
            'clear_stack': clear_stack, 'files': files}


def highscores(records):
    """save\\highscore.jas: 100 records of 0x4C bytes (0x004C5A40): (mission, score, name, months)"""
    data = bytearray(0x1DB0)
    for k, score, name, months in records:
        r = struct.pack('<ii', score, k) + name.encode().ljust(32, b'\0') + struct.pack('<9i', 50, 60, 40, 1234, 5678,
                                                                                         months, 2, 1, 1)
        data[k * 0x4C:(k + 1) * 0x4C] = r
    return bytes(data)


HS = [(k, 1000 + 37 * k, FAMILIES[k % 3], 20 + k) for k in range(0, 50, 2)]


S = lambda a, v: ('set', a, v)  # noqa: E731
C = lambda c: ('call', c)  # noqa: E731
LIST = [C('0x00516BB0')]                      # the family list (0x2F)
MENU = [C('0x00516BB0'), C('0x00516990')]     # the family's menu (0x30)

CASES = {
    # window 0, the main menu
    'menu-draw': case(['0x00520AB0:0', '0x004180C0']),
    'menu-hover': case(['0x00520AB0:0', '0x004180C0'], steps=[S(0x007056C8, i32(3))]),
    'menu-play': case(['0x00417FF0'], families=3, steps=[S(0x00D3B198, i32(0))]),
    'menu-greatest': case(['0x00417FF0'], steps=[S(0x00D3B198, i32(3))]),
    # the family list (0x2F), the name (0x36), the family's menu (0x30)
    'families-draw': case(['0x00516630'], families=15, steps=LIST + [S(0x00E38DCA, '01'), S(0x007056C8, i32(3))]),
    'families-scrolled': case(['0x00516630', '0x00516720'], families=15,
                              steps=LIST + [S(0x00EADFC8, i32(3)), S(0x007056C8, i32(14)), S(0x00EADFCC, i32(4))]),
    'families-empty': case(['0x00516630'], steps=LIST + [S(0x00E38DCA, '01')]),
    'families-drag': case(['0x005168E0'], families=15,
                          steps=LIST + [S(0x00E38E00, '01'), S(0x00E38E34, i32(664)), S(0x00E38E30, i32(349))]),
    'families-down': case(['0x00516890'], families=15, steps=LIST + [S(0x00EADFC8, i32(3)), S(0x00D3B198, i32(1))]),
    'families-up': case(['0x00516890'], families=15, steps=LIST + [S(0x00EADFC8, i32(3)), S(0x00D3B198, i32(0))]),
    'families-delete': case(['0x00516C60'], families=2, steps=LIST),
    'back': case(['0x00402BB0', '0x0040C890']),
    'name-draw': case(['0x00520B60', '0x00520B90'], steps=LIST + [S(0x00E38E74, '52616d00'), S(0x007056C8, i32(1))]),
    'family-menu-draw': case(['0x004CB4D0'], families=3, steps=MENU + [S(0x00E38DCA, '01'), S(0x007056C8, i32(2))]),
    'family-begin': case(['0x004CB800'], families=3, steps=MENU),
    'family-cleopatra': case(['0x004CB860'], families=3, steps=MENU + [S(0x00D3B188, '0100')]),
    'family-cleopatra-periods': case(['0x004CB860'], families=3, steps=MENU + [S(0x00D3B188, '0101')]),
    'family-load': case(['0x004CB910'], families=3, steps=MENU + [S(0x00D3B18A, '01')]),
    'family-custom': case(['0x004CB940'], families=3, steps=MENU),
    'family-change': case(['0x004CB970'], families=3, steps=MENU),
    'family-create': case(['0x00418220'], steps=[S(0x00E38DDA, '36'), S(0x00E38E74, '52616d73657300')], clear_stack=True),
    # the period choice (0x17)
    'periods-pharaoh': case(['0x0041C2D0', '0x0041BE10', '0x0041BB30'], families=3,
                            steps=MENU + [S(0x00E38DDA, '17'), S(0x006FDC98, i32(0)), S(0x007027AC, i32(3)),
                                          S(0x007027B0, i32(3)), S(0x007056C8, i32(4))]),
    'periods-cleopatra': case(['0x0041C2D0', '0x0041BE10', '0x0041BB30'], families=3,
                              steps=MENU + [S(0x00E38DDA, '17'), S(0x006FDC98, i32(1)), S(0x007027AC, i32(7)),
                                            S(0x007056C8, i32(10))]),
    'period-click': case(['0x0041BB10'], steps=[S(0x00D3B198, i32(5))]),
    'period-tabs': case(['0x0041C310', '0x0041C350', '0x0041F650'], families=3,
                        steps=MENU + [S(0x00E38DDA, '17'), S(0x006FDC98, i32(1))]),
    # the scenario lists (0x3D)
    'maps-draw': case(['0x0041E5C0'], families=1, steps=[S(0x00E38E74, NEF), C('0x004CB940'), S(0x00E38DCA, '01')]),
    'maps-row': case(['0x0041E5C0', '0x0041F220', '0x0041E170', '0x0041E140', '0x0041E3B0', '0x0041E470'], families=1,
                     steps=[S(0x00E38E74, NEF), C('0x004CB940'), S(0x00D3B198, i32(5)), C('0x0041F670'),
                            S(0x007056C8, i32(3))]),
    'maps-drag': case(['0x0041F4D0'], families=1,
                      steps=[S(0x00E38E74, NEF), C('0x004CB940'), S(0x00E38E00, '01'), S(0x00E38E34, i32(480)),
                             S(0x00E38E30, i32(450))]),
    'missions-draw': case(['0x0041E5C0'], families=3,
                          steps=MENU + [S(0x00E38DDA, '17'), S(0x006FDC98, i32(1)), C('0x0041C310'),
                                        S(0x00D3B198, i32(3)), C('0x0041F670'), S(0x00E38DCA, '01')]),
    # the front end's other screens
    'title': case(['0x00520E00'], steps=[S(0x00E38DDA, '3C')]),
    'resolution': case(['0x00534540', '0x005345A0'], steps=[S(0x00E38DDA, '3E'), S(0x007056C8, i32(1))]),
    'resolution-keep': case(['0x00402E10'], steps=[S(0x0070570C, '05')]),
    # the options dialogs over a city (drawn twice: the buttons appear on the second frame)
    'display-options': case(['0x00531190'], save=ACTIUM,
                            steps=[S(0x00E38DDA, '0A'), C('0x00531190'), S(0x007056C8, i32(3))]),
    'sound-options': case(['0x00531EF0'], save=ACTIUM, steps=[S(0x00E38DDA, '0B'), C('0x00531EF0'), S(0x007056C8, i32(6))]),
    'speed-options': case(['0x00531CC0'], save=ACTIUM, steps=[S(0x00E38DDA, '0C'), C('0x00531CC0'), S(0x007056C8, i32(2))]),
    'difficulty': case(['0x00531BE0'], save=ACTIUM, steps=[S(0x00E38DDA, '45')]),
    'hotkeys': case(['0x00537260'], save=ACTIUM, steps=[S(0x00E38DDA, '46'), S(0x007056C8, i32(12))]),
    'sound-switch-music': case(['0x00403060'], save=ACTIUM, steps=[S(0x00D3B198, i32(1))]),
    'sound-switch-speech': case(['0x00403060'], save=ACTIUM, steps=[S(0x00D3B198, i32(2)), S(0x00E38E65, '00')]),
    'sound-switch-effects': case(['0x00403060'], save=ACTIUM, steps=[S(0x00D3B198, i32(3))]),
    'sound-switch-city': case(['0x00403060'], save=ACTIUM, steps=[S(0x00D3B198, i32(4))]),
    'volumes': case(['0x00403130', '0x004031A0', '0x00403210', '0x00403270', '0x00403390'], save=ACTIUM,
                    steps=[S(0x00D3B198, i32(1)), S(0x00E38EC8, i32(0))]),
    'sound-cancel': case(['0x00402F70', '0x004032F0', '0x004EA760'], save=ACTIUM),
    # the file dialogs (0x0D)
    'load-open': case(['0x00402450'], families=1, saves=(ACTIUM, IKEN), steps=[S(0x00E38E74, NEF)]),
    'load-draw': case(['0x00532340'], families=1, saves=(ACTIUM, IKEN), steps=[S(0x00E38E74, NEF), C('0x00402450')]),
    'save-open-draw': case(['0x00402540', '0x00532340'], families=1, saves=(ACTIUM,), steps=[S(0x00E38E74, NEF)]),
    'delete-open-draw': case(['0x00402630', '0x00532340'], families=1, saves=(ACTIUM,), steps=[S(0x00E38E74, NEF)]),
    'file-row': case(['0x00402A80'], families=1, saves=(ACTIUM, IKEN),
                     steps=[S(0x00E38E74, NEF), C('0x00402450'), S(0x00D3B198, i32(1))]),
    'file-load': case(['0x00402720'], families=1, saves=(ACTIUM, IKEN),
                      steps=[S(0x00E38E74, NEF), C('0x00402450'), S(0x00D3B198, i32(0)), C('0x00402A80'),
                             S(0x00D3B198, i32(1))]),
    'file-not-found': case(['0x00402720'], families=1, saves=(ACTIUM,),
                           steps=[S(0x00E38E74, NEF), C('0x00402450'), S(0x00D3B198, i32(1))]),
    'file-cancel': case(['0x00402720'], families=1, steps=[S(0x00E38E74, NEF), C('0x00402450'), S(0x00D3B198, i32(0))]),
    'file-save': case(['0x00402720'], save=IKEN, families=1, saves=(ACTIUM,),
                      steps=[S(0x00E38E74, NEF), S(0x007027B4, '01'), C('0x00402540'), S(0x00D3B198, i32(1))]),
    # the number pad (0x0E) and the pop-up list (0x0F, 0x48)
    'numpad-draw': case(['0x005309B0:300,200'], save=ACTIUM,
                        steps=[S(0x00605978, i32(2)), S(0x0060597C, i32(42)), S(0x007056C8, i32(11))]),
    'numpad-digit': case(['0x004035A0'], save=ACTIUM,
                         steps=[S(0x0060596C, i32(2)), S(0x00605978, i32(2)), S(0x0060597C, i32(7)), S(0x00D3B198, i32(3))]),
    'numpad-ok': case(['0x004035F0', '0x00403770'], save=ACTIUM,
                      steps=[S(0x00605960, i32(0x00E37080)), S(0x0060596C, i32(2)), S(0x00E37080, 'fbff'),
                             S(0x0060597C, i32(12345)), S(0x00605968, i32(0)), S(0x00605A4C, i32(0)), S(0x00605980, '01')]),
    # the window-system requests (platform_open_url, platform_post_message, platform_file_delete)
    'menu-website': case(['0x00417FF0'], steps=[S(0x00D3B198, i32(1))]),
    'menu-quit': case(['0x00417FF0'], steps=[S(0x00D3B198, i32(4))]),
    'display-fullscreen': case(['0x00402C10'], save=ACTIUM, steps=[S(0x00E38DDA, '0A')]),
    'display-640': case(['0x00402C40'], save=ACTIUM, steps=[S(0x00E38DDA, '0A'), S(0x00D3B198, i32(1))]),
    'display-800': case(['0x00402C40'], save=ACTIUM, steps=[S(0x00E38DDA, '0A'), S(0x00D3B198, i32(2))]),
    'display-1024': case(['0x00402C40'], save=ACTIUM, steps=[S(0x00E38DDA, '0A'), S(0x00D3B198, i32(3))]),
    'display-back': case(['0x00402C40'], save=ACTIUM, steps=[S(0x00E38DDA, '0A'), S(0x00D3B198, i32(5))]),
    'resolution-revert': case(['0x00402DE0'], steps=[S(0x00E38DDA, '3E'), S(0x0070570C, '05')]),
    'file-delete': case(['0x00402720'], families=1, saves=(ACTIUM, IKEN),
                        steps=[S(0x00E38E74, NEF), C('0x00402630'), S(0x00D3B198, i32(0)), C('0x00402A80'),
                               S(0x00D3B198, i32(1))]),
    'saved-game-delete': case(['0x004D4D80:0x00E37100'], families=1, saves=(ACTIUM, IKEN),
                              steps=[S(0x00E38E74, NEF), C('0x00402630'),
                                     S(0x00E37100, (b'Save\\Nefertiti\\' + os.path.basename(IKEN).encode() + b'\0').hex())]),
    'saved-game-delete-none': case(['0x004D4D80:0x00E37100'], families=1, saves=(ACTIUM,),
                                   steps=[S(0x00E38E74, NEF), C('0x00402630'),
                                          S(0x00E37100, b'Save\\Nefertiti\\none.sav\0'.hex())]),
    'family-delete': case(['0x00516DD0'], families=3, saves=(ACTIUM, IKEN),
                          steps=LIST + [S(0x00E38E74, NEF), S(0x00D3B439, '01')]),
    'family-delete-empty': case(['0x00516DD0'], families=3, steps=LIST + [S(0x00E38E74, NEF), S(0x00D3B439, '01')]),
    'family-delete-no': case(['0x00516DD0'], families=3, saves=(ACTIUM,),
                             steps=LIST + [S(0x00E38E74, NEF), S(0x00D3B439, '00')]),
    # the high scores (package frontier7, present/ui/frontend/highscore.c): window 0x53, the record of a won
    # mission (window 0x42, 0x004C5A40), the end of a won mission (0x33), window 0x51
    'highscore-new': case(['0x004C5A40'], save=ACTIUM, families=1, steps=[S(0x00E38E74, NEF), S(0x00E38EA8, '03')]),
    'highscore-better': case(['0x004C5A40'], save=ACTIUM, families=1, files=[('save\\highscore.jas', highscores(HS))],
                             steps=[S(0x00E38E74, NEF), S(0x00E38EA8, '00')]),
    'highscore-other': case(['0x004C5A40'], save=IKEN, families=1, files=[('save\\highscore.jas', highscores(HS))],
                            steps=[S(0x00E38E74, NEF), S(0x00E38EA8, '02')]),
    'highscore-worse': case(['0x004C5A40'], save=IKEN, families=1,
                            files=[('save\\highscore.jas', highscores([(4, 0x7FFFFFFF, 'Zed', 3)]))],
                            steps=[S(0x00E38E74, NEF), S(0x00E38EA8, '04')]),
    'highscore-cheated': case(['0x004C5A40'], save=ACTIUM, families=1, steps=[S(0x00E38E74, NEF), S(0x00E38DD7, '01')]),
    'highscore-draw': case(['0x004C5920', '0x004C5660'], files=[('save\\highscore.jas', highscores(HS))],
                           steps=[S(0x00E38DDA, '53')]),
    'highscore-scrolled': case(['0x004C5920', '0x004C5660'], files=[('save\\highscore.jas', highscores(HS))],
                               steps=[S(0x00E38DDA, '53'), S(0x00D3A4B8, i32(4))]),
    'highscore-empty': case(['0x004C5920', '0x004C5660'], steps=[S(0x00E38DDA, '53')]),
    'highscore-drag': case(['0x004C57D0', '0x004C5660'], files=[('save\\highscore.jas', highscores(HS))],
                           steps=[S(0x00E38DDA, '53'), C('0x004C5920'), S(0x00E38E00, '01'), S(0x00E38E34, i32(0x248 + 192)),
                                  S(0x00E38E30, i32(0x100 + 144))]),
    'choice-draw': case(['0x00520D00'], save=ACTIUM, steps=[S(0x00E38DDA, '51'), S(0x007056C8, i32(5)), S(0x006059FC, i32(7))]),
    'choice-button': case(['0x00520D00'], save=ACTIUM, steps=[S(0x00E38DDA, '51'), S(0x007056C8, i32(0x2B))]),
    # the end of a mission (0x33): lost
    'mission-lost': case(['0x004194C0', '0x00419DB0', '0x004EA880', '0x004EA8D0'], save=ACTIUM,
                         steps=[S(0x00EA1380, '01'), S(0x00E38DD9, '00'), S(0x007056C8, i32(2))]),
}
for _r in range(7):  # the end of a won mission: the high score outcomes 0..6 (0x00D38700)
    CASES['mission-won-%d' % _r] = case(['0x004194C0', '0x00419DB0'], save=IKEN if _r % 2 else ACTIUM, families=1,
                                        files=[('save\\highscore.jas', highscores(HS))],
                                        steps=[S(0x00E38E74, NEF), C('0x004C5920'), S(0x00E38DD9, '01'),
                                               S(0x00E38EA8, '02'), S(0x00D386E0, b'Amenhotep\0'.hex()),
                                               S(0x00D38700, i32(_r))])
POPUP_RECTS = b''.join(struct.pack('<hhhh', 10, 10 + 20 * k, 190, 28 + 20 * k) + bytes(20) for k in range(4)).hex()
CASES['popup'] = case(['0x00530D70:300,200,2'], save=ACTIUM,
                      steps=[S(0x00605948, i32(4)), S(0x006057EC, i32(1)), S(0x00605944, i32(0x2C)),
                             S(0x00605940, i32(0x10)), S(0x006057F4, '00000000010000000200000003000000'),
                             S(0x00704DC8, POPUP_RECTS), S(0x007056C8, i32(3)), S(0x006057F0, '01')])


def player_files(tmp):
    """the port's player files, by their lower-case names ('/')"""
    top = os.path.join(tmp, 'player')
    return {os.path.relpath(os.path.join(d, f), top).replace(os.sep, '/').lower() for d, _, fs in os.walk(top) for f in fs}


def run(name, display):
    import harness
    import oracle_newgame_check as nc
    from game import Game
    c = CASES[name]
    g = harness.oracle_state(os.path.join(ROOT, c['save']), 0, display) if c['save'] else Game.booted(display=display)
    tmp = tempfile.mkdtemp()
    os.makedirs(os.path.join(tmp, 'player'))
    os.environ['IMHOTEP_PLAYER_FILES'] = os.path.join(tmp, 'player')
    for k in range(c['families']):
        nc.player_file(g, tmp, 'Save\\%s.dat' % FAMILIES[k], nc.progress_file(g, 0x4A9C, (0, 1, 2) if k % 2 == 0 else ()))
    for path, data in c.get('files', ()):
        nc.player_file(g, tmp, path, data)
    for path in c['saves']:
        nc.player_file(g, tmp, 'Save\\%s\\%s' % (FAMILIES[0], os.path.basename(path)),
                       open(os.path.join(ROOT, path), 'rb').read())
    for step in c['steps']:
        if step[0] == 'set':
            g.write(step[1], bytes.fromhex(step[2]))
        else:
            addr, args = harness.parse_call(step[1])
            g.call(addr, *args)
    if c['clear_stack']:
        nc.clear_stack(g)  # the original copies records with padding its frame leaves (core/newgame/flow.c)
    before = {k: bytes(v) for k, v in g.files.items()}
    player_before = player_files(tmp)
    posted_from = len(getattr(g, 'posted', []))
    requests = os.path.join(tmp, 'requests')
    os.environ['IMHOTEP_REQUEST_LOG'] = requests
    calls = [harness.parse_call(x) for x in c['calls']]
    try:
        written, ranges, bad = harness.compare(g, calls, tmp)
    except RuntimeError as e:
        return 'error: %s' % str(e)[:300]
    out = []
    # the window-system requests: the original's (Game.posted) made by the port, in order
    names = {'post': lambda m, a, b: 'post_message 0x%X 0x%X 0x%X' % (m, a, b), 'url': 'open_url %s'.__mod__,
             'quit': 'quit %d'.__mod__}
    original = [names[p[0]](*p[1:]) for p in getattr(g, 'posted', [])[posted_from:]]
    port = [line.strip() for line in (open(requests).readlines() if os.path.exists(requests) else [])
            if line.split()[0] in ('open_url', 'post_message', 'quit')]
    if original != port:
        out.append('requests %s, port %s' % (original, port))
    for fname in before:  # the player files the original deleted, deleted by the port (and only those)
        if fname not in player_before:
            continue
        found = fname in player_files(tmp)
        if (fname in g.files) != found:
            out.append('file %s %s' % (fname, 'deleted by the port only' if fname in g.files else 'not deleted'))
    for fname, data in g.files.items():  # the player files the original wrote, as the port wrote them
        if before.get(fname) == bytes(data) or fname == 'oracle.sav':
            continue
        found = [os.path.join(d, f) for d, _, fs in os.walk(os.path.join(tmp, 'player')) for f in fs
                 if os.path.relpath(os.path.join(d, f), os.path.join(tmp, 'player')).lower() == fname]
        if not found or open(found[0], 'rb').read() != bytes(data):
            out.append('file %s differs' % fname)
    out += ['%s: %d bytes' % (harness.diff.name_address(a), n) for a, n in ranges[:6]]
    out += ['0x%08X returned %d, port %d' % (a, ro, rp) for (a, _), ro, rp in bad[:4]]
    return ('differs: ' + '; '.join(out)) if out else 'equal (%d bytes written)' % written


def main():
    argv = sys.argv[1:]
    jobs, displays = 1, []
    if '-j' in argv:
        k = argv.index('-j')
        jobs = min(2, int(argv[k + 1]))  # the CPU budget: at most 2 workers
        del argv[k:k + 2]
    while '--display' in argv:
        k = argv.index('--display')
        displays.append(argv[k + 1])
        del argv[k:k + 2]
    names = argv or list(CASES)
    work = [(n, d) for d in (displays or ['1024x768x565', '640x480x555']) for n in names]
    from concurrent.futures import ProcessPoolExecutor
    passed = 0
    with ProcessPoolExecutor(jobs) as pool:
        for (n, d), r in zip(work, pool.map(run, [w[0] for w in work], [w[1] for w in work])):
            print('  %-24s %-14s %s' % (n, d, r), flush=True)
            passed += r.startswith('equal')
    print('oracle_frontend_check: %d / %d cases equal to the original' % (passed, len(work)))
    return 0 if passed == len(work) else 1


if __name__ == '__main__':
    sys.exit(main())
