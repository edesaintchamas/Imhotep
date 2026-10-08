"""Imhotep - differential test: the message windows, the empire map and the mission screens (package msgempire).

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tests/differential/oracle_msgempire_check.py [CASE ...] [-j JOBS]

Each case runs a sequence of the original's window functions (present/ui/messages, present/ui/empire)
on a loaded save or on a booted game with a campaign mission's inputs set, after writing the inputs a
player's actions would (mouse, buttons, the selected city, the editor's mode ...), and compares the
state and the back buffer with the ports (tools/oracle/harness.compare). The city view 0x004DDDC0 and
the sidebar 0x0051E990 drawn behind the windows are replayed (their own checks: oracle_frame_check.py,
oracle_cityview_check.py). The frames, with input through the window procedure, are in
oracle_frame_check.py (cases messages-*, empire-*).
Exit status: 0 if every case is equal, 1 otherwise.
"""
import os
import struct
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))

ACTIUM = 'local/saves/downloaded/1592/39-Actium-Dec30c.sav'
IKEN = 'local/saves/downloaded/1564/CaesarAlan-Iken.sav'
ALEXC = 'local/saves/downloaded/1591/38-AlexC-Feb15.sav'
SAIS = 'local/saves/downloaded/1582/sais 6374.sav'

MSGLIST = [0x004E1870, 0x004E1E10, 0x004E1A50]          # redraw, input, per frame
EMPIRE = [0x0052D4D0, 0x0052F8B0, 0x0052D940]           # redraw, map, title
MOUSE = lambda x, y: {0x00E38E34: (4, x), 0x00E38E30: (4, y)}  # noqa: E731


def sets(*dicts, **kw):
    out = {}
    for d in dicts:
        out.update(d)
    return out


# name: (save or 'mission:N', display, {address: (size, value)}, calls)
CASES = {
    # the message list: rows, scroll bar, delete read, right click (0x004E1E10 reaches core's commands)
    'msglist-actium': (ACTIUM, '1024x768x565', {}, MSGLIST),
    'msglist-row-open-iken': (IKEN, '1024x768x565', sets(MOUSE(300, 260), {0x00E38E04: (1, 1)}), MSGLIST),
    'msglist-row-delete-iken': (IKEN, '800x600x565', sets(MOUSE(250, 210), {0x00E38DF8: (1, 1)}), MSGLIST),
    'msglist-scroll-iken': (IKEN, '1024x768x565', sets(MOUSE(630, 420), {0x00E38E00: (1, 1), 0x00E9511C: (4, 25),
                                                                          0x00E95120: (4, 7)}), MSGLIST),
    'msglist-delete-read-alexc': (ALEXC, '640x480x555', sets(MOUSE(150, 355), {0x00E38E04: (1, 1)}), MSGLIST),
    'msglist-outside-actium': (ACTIUM, '1024x768x555', sets(MOUSE(100, 100), {0x00E38DF8: (1, 1)}), MSGLIST),
    # a message of the list opened (core's 0x004E21D0 on row p1 0x00D3B198 after the scroll 0x00E95120), then the
    # message window 0x004C5E60 (or the video layout 0x004C6C70); the Iken sweep covered every 4th message
    'message-window-iken-0': (IKEN, '1024x768x565', {0x00E95120: (4, 0), 0x00D3B198: (4, 0)}, [0x004E21D0, 0x004C5E60]),
    'message-window-iken-17': (IKEN, '800x600x565', {0x00E95120: (4, 17), 0x00D3B198: (4, 0)}, [0x004E21D0, 0x004C5E60]),
    'message-window-actium-9': (ACTIUM, '640x480x555', {0x00E95120: (4, 9), 0x00D3B198: (4, 0)}, [0x004E21D0, 0x004C5E60]),
    'message-window-alexc-3': (ALEXC, '1024x768x555', {0x00E95120: (4, 3), 0x00D3B198: (4, 0)}, [0x004E21D0, 0x004C5E60]),
    'message-video-iken-2': (IKEN, '1024x768x565', {0x00E95120: (4, 2), 0x00D3B198: (4, 0)}, [0x004E21D0, 0x004C6C70]),
    # the empire map: no selection, cities of each kind, a closed route, scrolled, the editor's panels
    'empire-actium': (ACTIUM, '1024x768x565', {0x00E38DDA: (1, 0x14)}, EMPIRE),
    'empire-open-route-actium': (ACTIUM, '1024x768x565', {0x00E38DDA: (1, 0x14), 0x00784F14: (4, 4), 0x00784F18: (4, 4)}, EMPIRE),
    'empire-trade-city-actium': (ACTIUM, '800x600x565', {0x00E38DDA: (1, 0x14), 0x00784F14: (4, 25), 0x00784F18: (4, 8)}, EMPIRE),
    'empire-other-city-actium': (ACTIUM, '1024x768x565', {0x00E38DDA: (1, 0x14), 0x00784F14: (4, 5), 0x00784F18: (4, 5)}, EMPIRE),
    'empire-closed-route-iken': (IKEN, '1024x768x565', {0x00E38DDA: (1, 0x14), 0x00784F14: (4, 13), 0x00784F18: (4, 12),
                                                         0x0077590C: (4, 700)}, EMPIRE),
    'empire-closed-route-alexc': (ALEXC, '640x480x555', {0x00E38DDA: (1, 0x14), 0x00784F14: (4, 34), 0x00784F18: (4, 14),
                                                          0x0077590C: (4, 300)}, EMPIRE),
    'empire-sais': (SAIS, '1024x768x555', {0x00E38DDA: (1, 0x14), 0x00784F14: (4, 1), 0x00784F18: (4, 1)}, EMPIRE),
    'empire-scroll-keys-iken': (IKEN, '1024x768x565', {0x00E38DDA: (1, 0x14)},
                                [(0x00442730, [3]), (0x00442730, [5]), (0x00442730, [8])] + EMPIRE),
    'editor-picture-actium': (ACTIUM, '1024x768x565', {0x00E38DDA: (1, 0x15), 0x00775928: (4, 1), 0x00775924: (4, 3)},
                              [0x0052D4E0, 0x0052F8B0]),
    'editor-city-actium': (ACTIUM, '1024x768x565', {0x00E38DDA: (1, 0x15), 0x00775928: (4, 2), 0x0077591C: (4, 3)},
                           [0x0052D4E0, 0x0052F8B0]),
    'editor-trade-city-actium': (ACTIUM, '1024x768x565', {0x00E38DDA: (1, 0x15), 0x00775928: (4, 2), 0x0077591C: (4, 24)},
                                 [0x0052D4E0]),
    'editor-route-alexc': (ALEXC, '1024x768x565', {0x00E38DDA: (1, 0x15), 0x00775928: (4, 0x0E), 0x005D2A30: (4, 1)},
                           [0x0052D4E0, 0x0052F8B0]),
    'editor-dialog-actium': (ACTIUM, '1024x768x565', {0x00E38DDA: (1, 0x16), 0x007848D8: (2, -25)}, [0x005305E0]),
    # the mission screens on a started campaign (the briefing starts the mission itself)
    'briefing-start-m0': ('mission:0', '1024x768x565', {0x007027A8: (4, 0), 0x00E38DDA: (1, 0x35), 0x00E38DCA: (1, 1)},
                          [0x0041A180, 0x00418F70]),
    'briefing-start-m7': ('mission:7', '800x600x565', {0x007027A8: (4, 0), 0x00E38DDA: (1, 0x3B), 0x00E38DCA: (1, 1)},
                          [0x0041A180, 0x00418F70]),
    'mission-choice-click': ('mission:0', '1024x768x565', sets(MOUSE(650, 490), {0x00E38EAB: (1, 5), 0x007027A4: (1, 0),
                                                                                  0x00E38E04: (1, 1), 0x00E38DDA: (1, 0x34)}),
                             [0x00419090, 0x0041AAA0, 0x00419140]),
    'mission-choice-hover': ('mission:0', '640x480x555', sets(MOUSE(320, 250), {0x00E38EAB: (1, 34), 0x007027A4: (1, 1),
                                                                                 0x00E38DDA: (1, 0x34)}),
                             [0x00419090, 0x0041AAA0, 0x00419140]),
    'mission-picture-start': ('mission:0', '1024x768x565', {0x006FDCB0: (4, 0), 0x00E38EA8: (1, 3)}, [0x0041A850]),
    'mission-picture-victory': ('mission:0', '1024x768x565', {0x006FDCB0: (4, 2), 0x00E38EA8: (1, 5)}, [0x0041A850]),
    'pictures': (ACTIUM, '1024x768x565', {}, [0x00417E80, 0x00417EB0, 0x00417EE0, 0x00417F10]),
}


def check(name):
    import harness
    from game import Game
    import newgame
    where, display, values, calls = CASES[name]
    harness.FORCED_REPLAY.update({0x004DDDC0, 0x0051E990})
    if where.startswith('mission:'):
        g = Game.booted(display=display)
        newgame.inputs(g, where, 2, campaign=1)
    else:
        g = harness.oracle_state(os.path.join(ROOT, where), 0, display)
    for a, (n, v) in values.items():
        g.write(a, (v & ((1 << (8 * n)) - 1)).to_bytes(n, 'little'))
    calls = [c if isinstance(c, tuple) else (c, []) for c in calls]
    with tempfile.TemporaryDirectory() as tmp:
        try:
            written, ranges, bad = harness.compare(g, calls, tmp)
        except Exception as e:  # noqa: BLE001 - the case's result
            return 'error: %s' % str(e)[-300:]
    if ranges or bad:
        return 'differs: %s; %d pixels %s; returns %s' % (', '.join('0x%08X+%d' % r for r in ranges[:6]),
                                                       harness.compare.screen[0], harness.compare.screen[1][:4], bad[:3])
    return 'equal (the original wrote %d bytes)' % written


def main():
    argv = sys.argv[1:]
    jobs = 1
    if '-j' in argv:
        k = argv.index('-j')
        jobs = min(int(argv[k + 1]), 2)  # CPU budget (local/oracle/agent_brief.md)
        del argv[k:k + 2]
    cases = argv or list(CASES)
    from concurrent.futures import ProcessPoolExecutor
    passed = 0
    with ProcessPoolExecutor(jobs) as pool:
        for case, result in zip(cases, pool.map(check, cases)):
            print('  %-32s %s' % (case, result), flush=True)
            passed += result.startswith('equal')
    print('oracle_msgempire_check: %d / %d cases equal to the original' % (passed, len(cases)))
    return 0 if passed == len(cases) else 1


if __name__ == '__main__':
    sys.exit(main())
