"""Imhotep - differential test: frames (roadmap phase 7), state and back buffer against the original.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tests/differential/oracle_frame_check.py [CASE ...] [-j JOBS] [--chained]

A case is NAME (from CASES) or SAVE@DISPLAY:WINDOW[:FRAMES] (DISPLAY as 1024x768x565, WINDOW the
window id 0x00E38DDA). For each case the reference game boots with the emulated DirectDraw at that
resolution and pixel format, loads the save, enters the window (full redraw) and runs FRAMES frames
(default 3, 40 ms apart, cursor at the centre; a case may give its own frames as framestep script
lines instead, e.g. the sidebar clicks) through tools/oracle/framestep.py: after every frame
the state and the back buffer of the original's frame 0x004D6B60 must equal the port's. Until the
frame (and the window procedure, for scripted input) is ported, framestep replays it whole: the case
is then reported "not ported" and fails; the run still checks the oracle side end to end.
Exit status: 0 if every case is equal with the frame ported, 1 otherwise.
"""
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))

SAVES = {
    'actium': 'local/saves/downloaded/1592/39-Actium-Dec30c.sav',
    'iken': 'local/saves/downloaded/1564/CaesarAlan-Iken.sav',
    'alexc': 'local/saves/downloaded/1591/38-AlexC-Feb15.sav',
    'sais': 'local/saves/downloaded/1582/sais 6374.sav',
    'pak00': 'local/saves/generated/pak00/m00.sav',
}
DISPLAYS = ['640x480x555', '800x600x565', '1024x768x565', '1024x768x555']

# the city view (window 1) on several saves at each resolution and format; windows drawn over the city
# (ids from the sweep, docs/presentation.md 3.2, 4.2): 5 info box, 6 build menu, 7 advisors, 8 overlays
# menu, 0x11 message list, 0x14 empire map, 0x17 mission briefing, 0x3F yes / no popup
CASES = {}
for _save in ('actium', 'iken', 'alexc', 'pak00'):
    for _d in DISPLAYS:
        CASES['city-%s-%s' % (_save, _d)] = (_save, _d, 1, 3)
CASES['city-sais-1024x768x565'] = ('sais', '1024x768x565', 1, 3)
for _w in (0x05, 0x06, 0x07, 0x08, 0x11, 0x14, 0x17, 0x3F):
    CASES['window%02x-actium-1024x768x565' % _w] = ('actium', '1024x768x565', _w, 2)
CASES['window07-iken-800x600x565'] = ('iken', '800x600x565', 0x07, 2)
CASES['window07-alexc-640x480x555'] = ('alexc', '640x480x555', 0x07, 2)

# sidebar clicks (package 7.5, present/sidebar): the cursor moves onto a sidebar control, the left
# (or right) button is pressed and released over the next frames, then the game runs on; positions
# from the button tables (0x005C7300, 0x005C7370, 0x005C7980, the minimap 0x0043E810), relative to
# the sidebar's x 0x00F53F64 (0x51E930: 0x1DE / 0x256 / 0x346 for widths 640 / 800 / 1024), y 0x1E
SIDEBAR_X = {640: 0x1DE, 800: 0x256, 1024: 0x346}
SIDEBAR_CLICKS = {
    'collapse': (128 + 5, 5, 'L'),       # 0x005C7300 entry 0: slide the sidebar (window 2)
    'overlays': (8 + 20, -5 + 10, 'L'),  # 0x005C7300 entry 1: the overlays menu (window 8)
    'housing': (9 + 10, 251 + 10, 'L'),  # 0x005C7370 entry 0: build menu 1 (a single building)
    'industry': (46 + 10, 300 + 10, 'L'),  # 0x005C7370 entry 4: build menu 10 (window 6)
    'undo': (9 + 10, 404 + 10, 'L'),      # 0x005C7370 entry 12: undo 0x00406010
    'messages': (46 + 10, 404 + 10, 'L'),  # 0x005C7370 entry 13: the message list (window 0x11)
    'advisors': (16 + 10, 143 + 10, 'L'),  # 0x005C7980 entry 0: the advisors (window 7)
    'help': (125 + 10, 251 + 10, 'R'),    # 0x005C7370 entry 9: right click, the help message
    'minimap': (0x0D + 40, 0x3B + 50 - 0x1E, 'L'),  # the minimap 0x0043E810: the camera moves
}


def sidebar_click_script(display, what):
    """frames: move onto the control, press, release, two more frames (40 ms apart)"""
    w = int(display.split('x')[0])
    dx, dy, button = SIDEBAR_CLICKS[what]
    x, y = SIDEBAR_X[w] + dx, 0x1E + dy
    return ['1000 mouse=%d,%d buttons=-' % (x, y), '1040 buttons=%s' % button, '1080 buttons=-', '1120', '1160']




def sidebar_tooltip_script(display, what):
    """frames: tooltips on (0x00E38EAC = 2), the cursor rests on a control past the 150 ms delay
    (0x004C8750), the tooltip is drawn (0x004CABD0)"""
    w = int(display.split('x')[0])
    dx, dy, _ = SIDEBAR_CLICKS[what]
    x, y = SIDEBAR_X[w] + dx, 0x1E + dy
    return ['1000 mouse=%d,%d E38EAC=02000000' % (x, y), '1040', '1300', '1340', '1380']


for _what in ('housing', 'overlays', 'advisors', 'undo'):
    CASES['tooltip-%s-actium-1024x768x565' % _what] = ('actium', '1024x768x565', 1, 5,
                                                       sidebar_tooltip_script('1024x768x565', _what))
CASES['tooltip-housing-iken-640x480x555'] = ('iken', '640x480x555', 1, 5, sidebar_tooltip_script('640x480x555', 'housing'))
for _what in SIDEBAR_CLICKS:
    CASES['sidebar-%s-actium-1024x768x565' % _what] = ('actium', '1024x768x565', 1, 5,
                                                       sidebar_click_script('1024x768x565', _what))
for _what in ('housing', 'collapse', 'minimap', 'overlays'):
    for _save, _d in (('iken', '800x600x565'), ('alexc', '640x480x555'), ('sais', '1024x768x555'), ('pak00', '800x600x565')):
        CASES['sidebar-%s-%s-%s' % (_what, _save, _d)] = (_save, _d, 1, 5, sidebar_click_script(_d, _what))


# advisors (package "advisors", present/ui/advisors): from the city view, the sidebar's advisors
# button opens window 7, then clicks on the hall's page tabs (0x005F8320, at the layout origin
# + (x, 0x1B0 + y)) and on the pages' controls (the pages put their panel at the layout origin
# (0x00E38DA8, 0x00E38DA4): rectangles of the page tables, docs/commands.md 3), each a move, a
# rest, a press, a release and a rest frame. Positions are in the 640 x 480 layout.
ADVISOR_TABS = {1: (12, 33), 2: (52, 39), 4: (96, 34), 5: (135, 38), 6: (178, 46), 7: (229, 48), 8: (282, 35),
                9: (321, 38), 10: (363, 39), 11: (406, 35), 12: (445, 40), 13: (490, 46), 14: (542, 40), 0: (588, 42)}
ADVISOR_CLICKS = {  # name: [page tab, then (x, y[, button]) in the layout], from the tables named
    'tabs': [1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 0],
    'labour': [1, (170, 366), (194, 366), (300, 138), (191, 234)],  # 0x005C8D08 wages -1 / +1, row 2 (0x005C4AB8) -> 0x2A, priority 1 (0x005C4BB8)
    'tax': [12, (190, 82), (190, 82), (214, 82), (214, 82)],          # 0x005C8D38: tax -1, -1, +1, +1
    'military': [2, (455, 98), (515, 142), (575, 98)],                 # 0x005C4CD0: go to, return, kingdom service
    'fleet': [2, 3, (455, 98)],                                           # 0x005C4F00
    'kingdom-gift': [4, (445, 342), (368, 263), (368, 223), (248, 346)],  # 0x005C5368 gift -> 0x2E, sizes 3 / 1 (0x005C5680), send
    'kingdom-donation': [4, (445, 363), (240, 226), (224, 293)],       # donation -> 0x2D, 500 (0x005C55B8), donate
    'kingdom-salary': [4, (320, 403), (288, 135), (320, 405)],         # salary -> 0x2C, rank 2 (0x005C5468), close
    'kingdom-request': [4, (318, 116)],                                # 0x0040AE10 row 0
    'ratings': [5, (140, 319), (260, 319)],                            # 0x005F7650 (0x0040ACE0)
    'trade': [6, (266, 66), (171, 227), (466, 227), (314, 313), (40, 345)],  # row 0 -> 0x28, import, export, stockpile, help
    'population': [7, (555, 88), (555, 188)],                          # 0x005C5130 graphs
    'festival': [11, (252, 358), (110, 141), (317, 259), (417, 334)],  # 0x0040AA50 -> 0x2B, god 0, size 2, hold
    'health': [8],
    'religion': [9],
    'education': [10],
    'monuments': [14, (318, 79)],                                      # 0x005C5798 rows (0x0040CF20 for 5-7)
    'chief': [13, (324, 76)],                                          # 0x005C5878 (0x0040D130)
}


def advisor_click_script(display, what):
    """frames: the sidebar's advisors button, then each click of ADVISOR_CLICKS[what] (move, rest, press,
    release, rest)"""
    w, h = (int(v) for v in display.split('x')[:2])
    dx, dy = (w - 640) // 2, (h - 480) // 2
    sx, sy, _ = SIDEBAR_CLICKS['advisors']
    clicks = [(SIDEBAR_X[w] + sx, 0x1E + sy)]
    for c in ADVISOR_CLICKS[what]:
        if isinstance(c, int):
            if c == 3:  # page 3 has no tab: shown by the military page's row 18 (0x0040A0E0 p1 3)
                clicks.append((dx + 572, dy + 371))
                continue
            x, tw = ADVISOR_TABS[c]
            clicks.append((dx + x + tw // 2, dy + 0x1B0 + 9 + 16))
        else:
            clicks.append((dx + c[0], dy + c[1]))
    lines, t = [], 1000
    for x, y in clicks:
        # the rectangle tables see the cursor one frame after it moves: a frame at rest before the press
        lines += ['%d mouse=%d,%d buttons=-' % (t, x, y), '%d' % (t + 40), '%d buttons=L' % (t + 80),
                  '%d buttons=-' % (t + 120), '%d' % (t + 160)]
        t += 200
    return lines


for _what in ADVISOR_CLICKS:
    CASES['advisors-%s-actium-1024x768x565' % _what] = ('actium', '1024x768x565', 1, 0,
                                                        advisor_click_script('1024x768x565', _what))
for _what in ('tabs', 'labour', 'kingdom-gift', 'trade', 'festival'):
    for _save, _d in (('iken', '800x600x565'), ('alexc', '640x480x555'), ('sais', '1024x768x555')):
        CASES['advisors-%s-%s-%s' % (_what, _save, _d)] = (_save, _d, 1, 0, advisor_click_script(_d, _what))


# info panels (package "info", present/ui/info): a right click on a building, figure or terrain tile of the
# city view opens window 9 (0x004D8310: right release, 0x00405BA0 cancels no construction -> 0x004F7E50), then clicks on the panel's buttons (input
# 0x004F96E0: tables of docs/commands.md 3.5, 3.7) and a right click that closes it. The tile's screen
# point and the panel's place (x 0x00EADD90, y 0x00EADD8C, height 0x00EADD84 in 16-pixel blocks) were
# found by running the original's hovered tile 0x00405F80 and open 0x004F7E50 on the save's view.
INFO_PANELS = [  # (save, display, what, tile's screen point, (panel x, y, height))
    ('actium', '1024x768x565', 'house', (282, 40), (188, 408, 22)),
    ('actium', '1024x768x565', 'bazaar', (570, 136), (188, 504, 16)),
    ('actium', '1024x768x565', 'figures', (442, 48), (188, 408, 22)),
    ('actium', '1024x768x565', 'temple', (778, 240), (188, 504, 16)),
    ('actium', '1024x768x565', 'legion', (690, 496), (188, 32, 22)),
    ('alexc', '640x480x555', 'bazaar', (10, 160), (8, 32, 16)),
    ('alexc', '640x480x555', 'farm', (98, 40), (8, 32, 16)),
    ('alexc', '640x480x555', 'figures', (290, 64), (8, 32, 22)),
    ('sais', '1024x768x555', 'granary', (10, 400), (188, 32, 18)),
    ('sais', '1024x768x555', 'house', (626, 136), (188, 408, 22)),
    ('iken', '800x600x565', 'terrain', (10, 40), (68, 240, 22)),
]


def info_click_script(what, point, panel, help_button=False):
    """frames: the click on the tile, the panel's own buttons, then a right click (or the help button)"""
    x, y, h = panel
    bottom = y + 16 * h
    clicks = [point + ('R',)]
    clicks += {
        'house': [(x + 0x70 + 20, bottom - 0x20 + 5, 'L')],                     # 0x005C5A78 -> 0x004062D0
        'bazaar': [(x + 0x64 + 20, bottom - 0x22 + 5, 'L'), (x + 0xFA + 20, 0x4E + 5, 'L')],  # orders, row 1
        'granary': [(x + 0x64 + 20, bottom - 0x22 + 5, 'L'), (x + 0xFA + 20, 0x4E + 25, 'L')],
        'figures': [(x + 90, y + 60, 'L'), (x + 0x11D + 20, y + 0xC3 + 5, 'L')],   # 0x005C5BE8 row 1, 0x005C5CB0
        'temple': [(x + 60, y + 70, 'L')],                                        # 0x005C5448 -> 0x0040AD30
        'legion': [(x + 19 + 85 + 30, y + 180, 'L')],                             # 0x005C66D8 -> 0x00406720
    }.get(what, [])
    clicks.append((x + 19, bottom - 0x28 + 5, 'L') if help_button else (x + 200, y + 100, 'R'))
    lines, t = [], 1000
    for cx, cy, b in clicks:
        lines += ['%d mouse=%d,%d buttons=-' % (t, cx, cy), '%d buttons=%s' % (t + 40, b), '%d buttons=-' % (t + 80),
                  '%d' % (t + 120)]
        t += 160
    return lines


for _save, _d, _what, _pt, _panel in INFO_PANELS:
    CASES['info-%s-%s-%s' % (_what, _save, _d)] = (_save, _d, 1, 0, info_click_script(_what, _pt, _panel))
    CASES['info-%s-%s-%s-help' % (_what, _save, _d)] = (_save, _d, 1, 0, info_click_script('', _pt, _panel, True))


# messages and the empire map (package "msgempire", present/ui/messages, present/ui/empire): the sidebar's
# messages button opens the list (window 0x11, 0x004E1870 / 0x004E1A50 / 0x004E1E10); a left click on a row
# opens the message (window 5, 0x004C5E60 / 0x004C6C70), a right click on a row deletes it, the "delete read"
# button (0x005C4810), the scroll bar, a right click back to the city. Positions in the 640 x 480 layout:
# rows (0x005C4830, 20 pixels apart) at (16, 0x6A), delete read at (100, 344), close at (442, 348).
# The empire map (window 0x14, 0x0052D4D0 / 0x0052F8B0, set by the script: its opener is a menu) with
# clicks on cities (map origin 0x005D2A18 = 16, 16 minus the scroll 0x00775910 / 0x0077590C) and on the
# open-route frame of a closed route's panel (0x0052DB10: 400 x 20 at ((W - 500) / 2 + 0x32, H - 0x28)).
MESSAGE_CLICKS = {  # name: [(x, y[, button]) in the layout, or 'list' for the sidebar button]
    'open-row0': ['list', (60, 0x6A + 8), (60, 0x6A + 8), (300, 200, 'R')],
    'open-row3': ['list', (200, 0x6A + 68), (300, 200, 'R')],
    'delete-row1': ['list', (60, 0x6A + 28, 'R'), (60, 0x6A + 28, 'R')],
    'delete-read': ['list', (60, 0x6A + 8), (300, 200, 'R'), 'list', (150, 354)],
    'close': ['list', (452, 358), (300, 200, 'R')],
}


def message_click_script(display, what):
    w, h = (int(v) for v in display.split('x')[:2])
    dx, dy = (w - 640) // 2, (h - 480) // 2
    sx, sy, _ = SIDEBAR_CLICKS['messages']
    lines, t = [], 1000
    for c in MESSAGE_CLICKS[what]:
        if c == 'list':
            x, y, b = SIDEBAR_X[w] + sx, 0x1E + sy, 'L'
        else:
            x, y, b = dx + c[0], dy + c[1], c[2] if len(c) > 2 else 'L'
        lines += ['%d mouse=%d,%d buttons=-' % (t, x, y), '%d buttons=%s' % (t + 40, b), '%d buttons=-' % (t + 80),
                  '%d' % (t + 120)]
        t += 160
    return lines


for _what in MESSAGE_CLICKS:
    CASES['messages-%s-iken-1024x768x565' % _what] = ('iken', '1024x768x565', 1, 0, message_click_script('1024x768x565', _what))
for _what in ('open-row0', 'delete-row1'):
    for _save, _d in (('actium', '800x600x565'), ('alexc', '640x480x555'), ('sais', '1024x768x555')):
        CASES['messages-%s-%s-%s' % (_what, _save, _d)] = (_save, _d, 1, 0, message_click_script(_d, _what))


def empire_click_script(display, scroll, clicks):
    """frames: window 0x14 with the map scrolled to `scroll` (x, y), then each (x, y[, button]) on the screen"""
    sx, sy = scroll
    lines = ['1000 window=20 redraw 00775910=%s 0077590C=%s' % (struct.pack('<i', sx).hex(), struct.pack('<i', sy).hex()),
             '1040']
    t = 1080
    for c in clicks:
        b = c[2] if len(c) > 2 else 'L'
        lines += ['%d mouse=%d,%d buttons=-' % (t, c[0], c[1]), '%d buttons=%s' % (t + 40, b), '%d buttons=-' % (t + 80),
                  '%d' % (t + 120)]
        t += 160
    return lines


EMPIRE_CASES = {  # name: (save, display, scroll, clicks)
    'cities-actium': ('actium', '1024x768x565', (0, 0), [(440, 420), (60, 380), (580, 520), (712, 80), (300, 300, 'R')]),
    'closed-route-iken': ('iken', '1024x768x565', (0, 700), [(840, 300), (512, 738), (546, 336), (400, 500)]),
    'closed-route-cancel-iken': ('iken', '1024x768x565', (0, 700), [(840, 300), (512, 738), (482, 336)]),
    'closed-route-alexc': ('alexc', '640x480x555', (300, 400), [(300, 210), (240, 450)]),
    'open-route-sais': ('sais', '1024x768x555', (0, 0), [(580, 530), (300, 300, 'R')]),
    'scroll-iken': ('iken', '800x600x565', (0, 0), [(400, 300), (400, 300)]),
}
for _name, (_save, _d, _scroll, _clicks) in EMPIRE_CASES.items():
    CASES['empire-%s-%s' % (_name, _d)] = (_save, _d, 1, 0, empire_click_script(_d, _scroll, _clicks))


# the front end (package frontend, present/ui/frontend): from a booted game ('boot': no save loaded) in
# the main menu (window 0) through the family list, a new family typed in, the family's menu and the
# period choice; the options dialogs (0x0A display, 0x0B sound, 0x0C speed) and the file dialogs (0x0D
# load / save) on a loaded city. Positions from the button tables (0x0057BD88, 0x005F7F28, 0x005F7EE0,
# 0x005E0408, 0x0057BE78, 0x005C3070, 0x005C8DF8, 0x005C3038, 0x005C8D98, 0x005C3118, 0x005C79F0) at
# the layout origin ((width - 640) / 2, (height - 480) / 2). A case's 6th element prepares the reference
# game and the player files (IMHOTEP_PLAYER_FILES) before the frames.
SAVES['boot'] = None
FAMILY_NAMES = ['Nefertiti', 'Zed', 'Amun', 'bastet', 'Hathor', 'Isis', 'Khufu', 'Menes', 'Narmer', 'Osiris',
                'Ptah', 'Ra', 'Seth', 'Thoth', 'Userkaf']


def frontend_origin(display):
    w, h = (int(v) for v in display.split('x')[:2])
    return (w - 640) // 2, (h - 480) // 2


def frontend_files(families=0, saves=()):
    """a setup: `families` progress files (Save\\<name>.dat), the saved games `saves` in the first
    family's directory, that family chosen (0x00E38E74)"""
    def setup(g, tmp):
        sys.path.insert(0, os.path.join(ROOT, 'tests', 'differential'))
        import oracle_newgame_check as nc
        for k in range(families):
            nc.player_file(g, tmp, 'Save\\%s.dat' % FAMILY_NAMES[k], nc.progress_file(g, 0x4A9C, (0, 1, 2) if k % 2 == 0 else ()))
        for path in saves:
            nc.player_file(g, tmp, 'Save\\%s\\%s' % (FAMILY_NAMES[0], os.path.basename(path)),
                           open(os.path.join(ROOT, path), 'rb').read())
        if families:
            g.write(0x00E38E74, FAMILY_NAMES[0].encode().ljust(32, b'\0'))
    return setup


def frontend_click(t, x, y, button='L'):
    return ['%d mouse=%d,%d buttons=-' % (t, x, y), '%d buttons=%s' % (t + 40, button), '%d buttons=-' % (t + 80)]


def frontend_menu_script(display):
    """the five buttons hovered, then the first ("play"): the family list, or the new family screen"""
    dx, dy = frontend_origin(display)
    lines = ['%d mouse=%d,%d' % (1000 + 40 * k, dx + 0x140, dy + 0x64 + 12 + 40 * k) for k in range(5)]
    return lines + frontend_click(1200, dx + 0x140, dy + 0x6A) + ['1320', '1360']


def frontend_families_script(display):
    """the family list: arrow down twice, the scroll bar dragged, a row chosen, proceed (the family's
    menu 0x30), its buttons hovered, "begin" (the period choice 0x17), a period clicked, back"""
    dx, dy = frontend_origin(display)
    s = frontend_click(1000, dx + 0x140, dy + 0x6A)                       # play
    s += frontend_click(1120, dx + 0x1D0 + 17, dy + 0x12C + 17)           # arrow down (0x005F7EE0 entry 1)
    s += frontend_click(1240, dx + 0x1D0 + 17, dy + 0x12C + 17)
    s += ['1360 mouse=%d,%d buttons=L' % (dx + 0x1E0, dy + 0xD0), '1400 mouse=%d,%d' % (dx + 0x1E0, dy + 0xF0),
          '1440 buttons=-']                                               # the scroll bar (0x005168E0)
    s += frontend_click(1480, dx + 0x100, dy + 0x98)                      # row 2 (0x00516AB0)
    s += frontend_click(1600, dx + 0x1C0, dy + 0x158)                     # proceed (0x00516990)
    s += ['%d mouse=%d,%d' % (1720 + 40 * k, dx + 0x140, dy + 0x8C + 0x30 * k) for k in range(5)]
    s += frontend_click(1920, dx + 0x140, dy + 0x8C)                      # begin (0x004CB800)
    s += frontend_click(2040, dx + 0x5C, dy + 0x12F)                      # period 0 (0x0041BB10)
    s += frontend_click(2160, dx + 0xF4, dy + 0x111)                      # period 2
    return s + ['2280 buttons=R', '2320 buttons=-', '2360', '2400']       # back to the family's menu


def frontend_newfamily_script(display):
    """no family: play opens the name screen (0x36); a name typed, Enter: the family is created and its
    menu (0x30) opens"""
    dx, dy = frontend_origin(display)
    s = frontend_click(1000, dx + 0x140, dy + 0x6A)
    return s + ['1120 char=Ramses', '1160 key=0x0D', '1200', '1240', '1280 mouse=%d,%d' % (dx + 0x140, dy + 0x8C), '1320']


def frontend_options_script(display, window):
    """window 0x0A, 0x0B or 0x0C over the city: buttons hovered and clicked, the arrows of the volumes
    and speeds; the closing buttons only hovered (their callbacks redraw the city, checked by the city
    cases; 0x00402C10 / 0x00402C40 post Win32 messages, not ported)"""
    dx, dy = frontend_origin(display)
    if window == 0x0B:
        s = ['1000 mouse=%d,%d' % (dx + 0x80, dy + 0xC8)]
        s += frontend_click(1040, dx + 0x80, dy + 0xAC)                   # music off (0x00403060)
        s += frontend_click(1160, dx + 0x80, dy + 0xE8)                   # effects (0x00403060)
        s += frontend_click(1280, dx + 0xD0 + 0x88 + 8, dy + 0x3C + 0x64 + 12)   # music volume down (0x00403130)
        s += frontend_click(1400, dx + 0xD0 + 0x70 + 8, dy + 0x3C + 0x82 + 12)   # speech volume up (0x004031A0)
        return s + ['1520 mouse=%d,%d' % (dx + 0xF0, dy + 0x150), '1560']  # cancel hovered
    if window == 0x0C:
        s = frontend_click(1000, dx + 0x80 + 0x88 + 8, dy + 0x28 + 0x64 + 12)    # game speed down (0x00403320)
        s += frontend_click(1120, dx + 0x80 + 0x70 + 8, dy + 0x28 + 0x88 + 12)   # scroll speed up (0x00403390)
        return s + ['1240 mouse=%d,%d' % (dx + 0xF0, dy + 0xF2), '1280']    # OK hovered
    return ['%d mouse=%d,%d' % (1000 + 40 * k, dx + 0xF0, dy + 0x92 + 0x18 * k) for k in range(5)] + ['1200']


def frontend_file_script(display, ok=True):
    """the file dialog: the second row clicked (0x00402A80), then OK (0x00402720, parameter 1)"""
    dx, dy = frontend_origin(display)
    s = frontend_click(1000, dx + 0x100, dy + 0x98)
    return s + (frontend_click(1120, dx + 0x188 + 17, dy + 0x14F + 17) if ok else []) + ['1240']


def frontend_dialog(addr, families, saves):
    files = frontend_files(families, saves)

    def setup(g, tmp):
        files(g, tmp)
        g.call(addr)
    return setup


for _d in DISPLAYS:
    CASES['frontend-menu-%s' % _d] = ('boot', _d, 0, 0, frontend_menu_script(_d), frontend_files(3))
for _d in ('1024x768x565', '640x480x555', '800x600x565'):
    CASES['frontend-families-%s' % _d] = ('boot', _d, 0, 0, frontend_families_script(_d), frontend_files(15))
CASES['frontend-newfamily-1024x768x565'] = ('boot', '1024x768x565', 0, 0, frontend_newfamily_script('1024x768x565'),
                                               frontend_files(0))
for _w in (0x0A, 0x0B, 0x0C):
    for _save, _d in (('actium', '1024x768x565'), ('iken', '800x600x565'), ('alexc', '640x480x555')):
        CASES['frontend-options%02x-%s-%s' % (_w, _save, _d)] = (_save, _d, _w, 0, frontend_options_script(_d, _w))
_SAVED = ('local/saves/downloaded/1592/39-Actium-Dec30c.sav', 'local/saves/downloaded/1564/CaesarAlan-Iken.sav')
CASES['frontend-load-actium-1024x768x565'] = ('actium', '1024x768x565', 1, 0, frontend_file_script('1024x768x565'),
                                               frontend_dialog(0x00402450, 1, _SAVED))
CASES['frontend-load-boot-800x600x565'] = ('boot', '800x600x565', 0, 0, frontend_file_script('800x600x565'),
                                            frontend_dialog(0x00402450, 1, _SAVED))
CASES['frontend-save-iken-1024x768x565'] = ('iken', '1024x768x565', 1, 0, frontend_file_script('1024x768x565'),
                                             frontend_dialog(0x00402540, 1, _SAVED[:1]))
CASES['frontend-savedialog-alexc-640x480x555'] = ('alexc', '640x480x555', 1, 0,
                                                   frontend_file_script('640x480x555', ok=False),
                                                   frontend_dialog(0x00402540, 1, _SAVED))


# the window machinery and the small windows (package wincore, present/ui/dispatch.c, windows.c,
# msgwin.c): the top menu (window 3: a title of the menu bar 0x005C91F0 pressed, an item chosen, or a
# right click closing it), the yes / no popup (window 0x3F, opened by the script as 0x004264D0 does:
# action 0x0051F780, 2 buttons 0x005C8168 at (x + 192 / 256, y + 100), x, y = layout origin + 0x50),
# a build menu's rows (window 6, 0x00403E00), the overlays menu's groups and items (window 8,
# 0x00409E40), and the help window (window 5, opened by a right click on a sidebar button: its close
# button and scroll arrows).
def wincore_origin(display):
    w, h = (int(v) for v in display.split('x')[:2])
    return (w - 640) // 2, (h - 480) // 2


def wincore_popup_script(display, button):
    dx, dy = wincore_origin(display)
    x, y = dx + 0x50, dy + 0x50
    sets = ('window=0x3F redraw 705704=80F75100 705708=02000000 70570C=01000000 705710=02000000 '
            '705714=%s 705718=%s' % (y.to_bytes(4, 'little').hex(), x.to_bytes(4, 'little').hex()))
    bx = x + (256 if button == 'yes' else 192) + 10
    return ['1000 %s mouse=%d,%d buttons=-' % (sets, bx, y + 110), '1040 buttons=L', '1080 buttons=-', '1120', '1160']


def wincore_topmenu_script(title_x, item_y, close=False):
    lines = ['1000 mouse=%d,8 buttons=-' % title_x, '1040 buttons=L', '1080 buttons=-', '1120 mouse=%d,%d' % (title_x + 4, item_y)]
    lines += ['1160 buttons=R', '1200 buttons=-'] if close else ['1160 buttons=L', '1200 buttons=-']
    return lines + ['1240']


def wincore_buildmenu_script(display):
    w = int(display.split('x')[0])
    x = SIDEBAR_X[w]
    lines = ['1000 mouse=%d,%d buttons=-' % (x + 56, 0x1E + 310), '1040 buttons=L', '1080 buttons=-']
    t = 1120
    for y in (0xA0, 0xB8, 0xD0):
        lines += ['%d mouse=%d,%d' % (t, x - 0x60, y), '%d buttons=L' % (t + 40), '%d buttons=-' % (t + 80)]
        t += 120
    return lines


def wincore_overlays_script(display):
    w = int(display.split('x')[0])
    x = SIDEBAR_X[w]
    return ['1000 mouse=%d,%d buttons=-' % (x + 28, 0x1E + 5), '1040 buttons=L', '1080 buttons=-',
            '1120 mouse=%d,%d' % (x - 0xAA + 0x10, 0x48 + 24 + 5), '1160', '1200 mouse=%d,%d' % (x - 0x15C + 0x10, 0x48 + 24 + 5),
            '1240 buttons=L', '1280 buttons=-', '1320']


def wincore_help_script(display):
    w = int(display.split('x')[0])
    x = SIDEBAR_X[w]
    return ['1000 mouse=%d,%d buttons=-' % (x + 135, 0x1E + 261), '1040 buttons=R', '1080 buttons=-',
            '1120 mouse=630,220', '1160 buttons=L', '1200 buttons=-', '1240 mouse=630,520', '1280 buttons=L',
            '1320 buttons=-', '1360 mouse=640,300 buttons=L', '1400 mouse=640,340', '1440 buttons=-',
            '1480 mouse=500,300 buttons=R', '1520 buttons=-', '1560']


CASES['wincore-topmenu-file-actium-1024x768x565'] = ('actium', '1024x768x565', 1, 7, wincore_topmenu_script(20, 58))
CASES['wincore-topmenu-options-actium-1024x768x565'] = ('actium', '1024x768x565', 1, 7, wincore_topmenu_script(110, 38))
CASES['wincore-topmenu-close-iken-800x600x565'] = ('iken', '800x600x565', 1, 7, wincore_topmenu_script(20, 78, True))
CASES['wincore-topmenu-help-alexc-640x480x555'] = ('alexc', '640x480x555', 1, 7, wincore_topmenu_script(240, 38))
for _d, _save in (('1024x768x565', 'actium'), ('640x480x555', 'iken')):
    for _b in ('yes', 'no'):
        CASES['wincore-popup-%s-%s-%s' % (_b, _save, _d)] = (_save, _d, 1, 5, wincore_popup_script(_d, _b))
for _d, _save in (('1024x768x565', 'actium'), ('800x600x565', 'pak00')):
    CASES['wincore-buildmenu-%s-%s' % (_save, _d)] = (_save, _d, 1, 12, wincore_buildmenu_script(_d))
    CASES['wincore-overlays-%s-%s' % (_save, _d)] = (_save, _d, 1, 9, wincore_overlays_script(_d))
    CASES['wincore-help-%s-%s' % (_save, _d)] = (_save, _d, 1, 15, wincore_help_script(_d))


# package frontier7 (present/ui/cityframe.c, debug_panels.c): the debug panels of window 1 (their
# switches poked: the debug keys that set them are inactive), the construction cost at the hovered
# tile, scrolling by the screen's edges and the arrow keys, the cheat input window 0x55
FRONTIER7_DEBUG = ['1000 mouse=280,40 buttons=- 78D7D8=01000000 78D7D4=01000000', '1040', '1080 mouse=568,136',
                   '1120 78D7D8=00000000 78D7D4=00000000 78D7DC=01000000 78D7E0=03000000', '1160 EA181C=01000000',
                   '1200 BC9724=E8030000 EA181C=00000000 78D7DC=00000000', '1240 BC9724=40420F00', '1280']
FRONTIER7_SCROLL = ['1000 mouse=0,300 buttons=-', '1040', '1080 mouse=799,0', '1120', '1160 mouse=400,599',
                    '1200 mouse=400,300 down=0x25', '1240 up=0x25 down=0x26', '1280 up=0x26', '1290', '1340']
FRONTIER7_CHEAT = ['1000 window=0x55 redraw mouse=320,240 buttons=- 7027E8=01', '1040 char=Meow', '1080',
                   '1120 key=0x0D', '1160', '1200']
CASES['frontier7-debug-actium-1024x768x565'] = ('actium', '1024x768x565', 1, 8, FRONTIER7_DEBUG)
CASES['frontier7-debug-alexc-640x480x555'] = ('alexc', '640x480x555', 1, 8, FRONTIER7_DEBUG)
CASES['frontier7-scroll-iken-800x600x565'] = ('iken', '800x600x565', 1, 10, FRONTIER7_SCROLL)
CASES['frontier7-cheat-actium-1024x768x565'] = ('actium', '1024x768x565', 1, 6, FRONTIER7_CHEAT)
# windows no other package owned (frontier7): 0x31 and 0x32 (no redraw, frame or input of their own), 0x51,
# 0x53 (the high scores, without the file: none recorded), 0x57, 0x42 (a won mission's high score, then 0x33)
for _w in (0x31, 0x32, 0x51, 0x53, 0x57, 0x42):
    CASES['frontier7-window%02x-actium-1024x768x565' % _w] = ('actium', '1024x768x565', _w, 3)
CASES['frontier7-window51-iken-640x480x555'] = ('iken', '640x480x555', 0x51, 3)
CASES['frontier7-window53-alexc-800x600x565'] = ('alexc', '800x600x565', 0x53, 3)
CASES['frontier7-window42-iken-800x600x565'] = ('iken', '800x600x565', 0x42, 3,
                                                 ['1000 E38DD9=01', '1040', '1080'])


# the scenario editor's empire map (window 0x15, present/ui/empire_editor.c): a new route (mode 0x0E: each
# release adds a waypoint, the route's points recomputed on integers, frontier7), then edited (mode 0x0F:
# a waypoint dragged), with route spacing 0x00776FB0 + 0x144 k set
def editor_route_script(points, drag=None, spacing=8, route=1):
    r = 0x00776FB0 + 0x144 * route
    lines = ['1000 window=0x15 redraw mouse=%d,%d buttons=- 775928=0E000000 5D2A30=%s %X=%02X' %
             (points[0][0], points[0][1], route.to_bytes(4, 'little').hex(), r, spacing)]
    t = 1040
    for x, y in points:
        lines += ['%d mouse=%d,%d' % (t, x, y), '%d buttons=L' % (t + 40), '%d buttons=-' % (t + 80)]
        t += 120
    if drag:
        (x0, y0), (x1, y1) = drag
        lines += ['%d 775928=0F000000 mouse=%d,%d' % (t, x0, y0), '%d buttons=L' % (t + 40),
                  '%d mouse=%d,%d' % (t + 80, x1, y1), '%d buttons=-' % (t + 120), '%d' % (t + 160)]
    return lines


_ROUTE = [(300, 200), (517, 263), (650, 407), (433, 521)]
CASES['frontier7-editor-route-actium-1024x768x565'] = ('actium', '1024x768x565', 0x15, 14, editor_route_script(_ROUTE))
CASES['frontier7-editor-route-drag-alexc-1024x768x565'] = ('alexc', '1024x768x565', 0x15, 18,
                                                           editor_route_script(_ROUTE, ((517, 263), (561, 199)), 5, 2))
CASES['frontier7-editor-route-iken-640x480x555'] = ('iken', '640x480x555', 0x15, 11,
                                                    editor_route_script([(100, 90), (333, 177), (211, 400)], spacing=3))


# package play (present/play/play.c, core/frame/display.c, core/city/route_points.c): the on-screen
# messages (0x004E2720: a slot of 0x00E94E2A written, its button pulsing past 0x28 frames, shown 15 s of
# the frame clock, lower while paused), the window procedure's WM_ACTIVATEAPP / WM_SIZE / WM_MOVE, the
# resolution keys F7..F9 (0x004CFDC0, 0x00414BD0, 0x004D0220: DirectDraw's set-up 0x004D0060 is replayed,
# a platform request) and the confirmation window 0x3E (0x00402DA0), the briefing's difficulty arrows
# (0x00418FE0, window 0x35)
PLAY_MESSAGE = ('E94E2A=2C017800 E94E2E=0100 E94E32=0401 E94E34=E8030000 E94E3A=48656C6C6F00 '
                'E94EA2=58028C00 E94EA6=0100 E94EAA=9001 E94EAC=00000000 E94EB2=576F726C6400 F53F34=00000000')
PLAY_MESSAGES = (['1000 mouse=500,400 buttons=- ' + PLAY_MESSAGE] + ['%d' % (1040 + 40 * k) for k in range(12)] +
                 ['1600 E38E60=01', '1640 E38E60=00', '16000', '16040'])
PLAY_ACTIVATE = ['1000 mouse=500,400 buttons=-', '1040 msg=0x1C,0,0', '1080', '1120 E39128=01000000 msg=0x1C,1,0',
                 '1160 msg=5,0,0x03000400', '1200 msg=3,0,0', '1240 E38E5C=00000000 msg=5,0,0', '1280']
PLAY_RESOLUTION = ['1000 mouse=500,400 buttons=-', '1040 key=0x76', '1080', '1120 key=0x77', '1160', '1200 key=0x78',
                   '1240', '1280']
PLAY_BRIEFING = ['1000 window=0x35 redraw 6FDCA0=C0000000 6FDC9C=90000000 mouse=%d,%d buttons=-' % (192 + 0x230 + 8, 144 + 0xA8 + 8),
                 '1040', '1080 buttons=L', '1120 buttons=-', '1160 mouse=%d,%d' % (192 + 0x230 + 8, 144 + 0x17E + 8),
                 '1200', '1240 buttons=L', '1280 buttons=-', '1320']
for _save, _d in (('actium', '1024x768x565'), ('iken', '800x600x565'), ('alexc', '640x480x555')):
    CASES['play-messages-%s-%s' % (_save, _d)] = (_save, _d, 1, 0, PLAY_MESSAGES)
    CASES['play-activate-%s-%s' % (_save, _d)] = (_save, _d, 1, 0, PLAY_ACTIVATE)
    CASES['play-resolution-%s-%s' % (_save, _d)] = (_save, _d, 1, 0, PLAY_RESOLUTION)
CASES['play-briefing-actium-1024x768x565'] = ('actium', '1024x768x565', 0x35, 0, PLAY_BRIEFING)

# package p8ui (phase 8: present/ui/buttons.c, frames.c, info/queries.c): windows whose per-frame part
# was ported in phase 8, entered directly (full redraw) with the cursor moved over their controls (the
# hovered rectangle lights frames): 0x52 the festival from the festival square, 0x56 the message
# options, 0x2A / 0x2D / 0x2E / 0x4C the dialogs over the advisors; the empire map's help and close
# buttons (0x005C7CE8, 0x005C7D10); the advisor pages 2..7 through their tabs
def p8ui_hover_script(display, points):
    dx, dy = wincore_origin(display)
    lines = ['1000 mouse=%d,%d buttons=-' % (dx + points[0][0], dy + points[0][1])]
    for k, (x, y) in enumerate(points[1:]):
        lines.append('%d mouse=%d,%d' % (1040 + 40 * k, dx + x, dy + y))
    return lines + ['%d' % (1040 + 40 * len(points))]


P8UI_HOVERS = {
    0x52: [(0x84 + 40, 0xF6 + 10), (0x84 + 40, 0x114 + 10), (0x84 + 40, 0x132 + 10), (300, 300)],
    0x56: [(0x80 + 5, 0x74 + 5), (0x80 + 5, 0x8C + 5), (0x80 + 5, 0x1AC + 5), (0x80 + 5, 0x1C4 + 5), (10, 10)],
    0x2A: [(0xB2 + 5, 0xDD + 5), (0xD2 + 5, 0xDD + 5), (0xB4 + 20, 0x100 + 5), (10, 10)],
    0x2D: [(0x80 + 5, 0xD8 + 5), (0x120 + 5, 0xD8 + 5), (0x150 + 5, 0x11B + 5), (0x90 + 5, 0x11B + 5)],
    0x2E: [(0xE0 + 5, 0xDA + 5), (0xE0 + 5, 0xEE + 5), (0x80 + 5, 0x150 + 5), (400 + 5, 0x150 + 5)],
    0x4C: [(0x152 + 5, 0xD5 + 5), (0x80 + 5, 0x150 + 5), (400 + 5, 0x150 + 5), (10, 10)],
}
for _w, _pts in P8UI_HOVERS.items():
    for _save, _d in (('actium', '1024x768x565'), ('iken', '800x600x565'), ('alexc', '640x480x555')):
        CASES['p8ui-window%02x-%s-%s' % (_w, _save, _d)] = (_save, _d, _w, 0, p8ui_hover_script(_d, _pts))
for _save, _d in (('actium', '1024x768x565'), ('alexc', '640x480x555')):
    _w, _h = (int(v) for v in _d.split('x')[:2])
    CASES['p8ui-empire-buttons-%s-%s' % (_save, _d)] = (_save, _d, 1, 0, empire_click_script(
        _d, (0, 0), [(0x14 + 10, _h - 0x2C + 10), (300, 300, 'R'), (_w - 0x2C + 10, _h - 0x2C + 10)]))
for _save, _d in (('iken', '800x600x565'), ('alexc', '640x480x555')):
    CASES['p8ui-advisors-pages-%s-%s' % (_save, _d)] = (_save, _d, 1, 0, advisor_click_script(_d, 'tabs'))


def parse(case):
    if case in CASES:
        save, display, window, frames = CASES[case][:4]
        return SAVES[save], display, window, frames  # SAVES['boot'] is None: a booted game
    path, _, rest = case.partition('@')
    parts = rest.split(':')
    return path, parts[0], int(parts[1], 0), int(parts[2]) if len(parts) > 2 else 3


def script(case, frames):
    """the case's frames: its script lines (framestep's format) when it has some, else idle frames"""
    import framestep
    if case in CASES and len(CASES[case]) > 4:
        import tempfile
        with tempfile.NamedTemporaryFile('w', suffix='.script', delete=False) as f:
            f.write('\n'.join(CASES[case][4]) + '\n')
        try:
            return framestep.read_script(f.name)
        finally:
            os.unlink(f.name)
    return framestep.default_script(frames, 40)


def check(case, chained):
    import framestep
    save, display, window, frames = parse(case)
    o, _ = framestep.options(['--display', display, '--window', str(window)] + (['--chained'] if chained else []))
    lines = []
    try:
        if save is None:  # a booted game in the window (no save loaded), as framestep.setup does
            from game import Game
            g = Game.booted(display=display)
            g.write(framestep.WINDOW, bytes([window]))
            g.write(framestep.REDRAW, b'\x01')
            _, w, h, _, _ = g.screen_info()
            g.cursor = (w // 2, h // 2)
            g.message(framestep.WM_MOUSEMOVE, 0, (h // 2) << 16 | (w // 2))
        else:
            g = framestep.setup(o, os.path.join(ROOT, save))
        if case in CASES and len(CASES[case]) > 5:  # the case's own preparation (player files, a dialog)
            import tempfile
            tmp = tempfile.mkdtemp()
            os.makedirs(os.path.join(tmp, 'player'))
            os.environ['IMHOTEP_PLAYER_FILES'] = os.path.join(tmp, 'player')
            CASES[case][5](g, tmp)
        failed = framestep.run(g, script(case, frames), o, out=lines.append)
    except Exception as e:  # noqa: BLE001 - reported as the case's result
        return 'error: %s' % str(e)[:300]
    whole = framestep.run.whole
    if failed:
        return 'differs:\n      ' + '\n      '.join(lines[:12])
    if whole:
        return 'not ported (replayed whole: %s)' % ', '.join('0x%08X' % a for a in sorted(whole))
    return 'equal'


def main():
    argv = sys.argv[1:]
    jobs = 1
    if '-j' in argv:
        k = argv.index('-j')
        jobs = int(argv[k + 1])
        del argv[k:k + 2]
    chained = '--chained' in argv
    if chained:
        argv.remove('--chained')
    cases = argv or list(CASES)
    from concurrent.futures import ProcessPoolExecutor
    passed = not_ported = 0
    with ProcessPoolExecutor(jobs) as pool:
        for case, result in zip(cases, pool.map(check, cases, [chained] * len(cases))):
            print('  %-36s %s' % (case, result), flush=True)
            passed += result == 'equal'
            not_ported += result.startswith('not ported')
    print('oracle_frame_check: %d / %d cases equal to the original (%d not ported)' % (passed, len(cases), not_ported))
    return 0 if passed == len(cases) else 1


if __name__ == '__main__':
    sys.exit(main())
