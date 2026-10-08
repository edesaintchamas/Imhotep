"""Imhotep - differential test: window 9, the building and figure info panels (roadmap 7.6, package
"info", present/ui/info/), state and back buffer against the original.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tests/differential/oracle_info_check.py [CASE|PREFIX|GROUP ...] [-j JOBS] [--list]
        [--png] [--isolate]

Each case boots the reference game with the emulated DirectDraw (docs/oracle.md "Display"), loads a
save, centres the camera on a tile and opens the info window on it as the original's click does: the
mouse tile 0x00BCA2C4 / 0x00BCA2C8 / 0x00BCA2CC set, the city view's input 0x004D8310 calls the open
0x004F7E50 (window 9, the context 0x00EADD6C building, 0x00EADD50 kind, 0x00EADCCC formation, the
figure list 0x00EADD24, the panel's place 0x00EADD8C.. by resolution and mouse y). Then, from that
same state, the function under test runs in the original and in the port (tools/oracle/harness.py
compare: unported callees replayed); the state and the back buffer must be equal:
  draw   the full redraw 0x004F8E60 (city view, the building's sound, the panel);
  open   the open 0x004F7E50 itself, then the redraw;
  frame  the redraw by the original, then the per-frame draw 0x004F94A0 (button highlights);
  input  the redraw by the original, the mouse placed at (dx, dy) from the panel's origin
         (0x00EADD90, 0x00EADD8C) with a left press (or a right release), then the input 0x004F96E0
         and the redraw 0x004F8E60 (the buttons' commands: core/command, docs/commands.md 3.5, 3.7).
Cases: b<TYPE>-<SAVE> the first building of each type in the corpus saves (local/saves/downloaded/*,
local/saves/generated/*/m06.sav); f<TYPE>[b]-<SAVE> a tile with a figure of that type (b: on a
building's tile); t-<kind> terrain tiles; synthetic types (s<TYPE>): a building's type poked to a type
no corpus save has. Options per case: the display, ticks, the orders sub-panel (0x00EADCC0 = 1), sets
(globals written after the open).
--isolate replays the ports of the other phase-7 packages instead of running them.
Exit status: 0 if every case is equal, 1 otherwise, 2 on setup error.
"""
import os
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))

SAVES = {
    'itjtawy5': 'local/saves/downloaded/1562/CaesarAlan-Itjtawy-plus5.sav',
    'itjtawy': 'local/saves/downloaded/1562/CaesarAlan-Itjtawy.sav',
    'iken': 'local/saves/downloaded/1564/CaesarAlan-Iken.sav',
    'sais': 'local/saves/downloaded/1582/sais 6374.sav',
    'maritis': 'local/saves/downloaded/1590/37-Maritis-Dec44c.sav',
    'alexc': 'local/saves/downloaded/1591/38-AlexC-Feb15.sav',
    'actium': 'local/saves/downloaded/1592/39-Actium-Dec30c.sav',
    'maritis-m06': 'local/saves/generated/37-Maritis-Dec44c/m06.sav',
    'alexc-m06': 'local/saves/generated/38-AlexC-Feb15/m06.sav',
    'actium-m06': 'local/saves/generated/39-Actium-Dec30c/m06.sav',
    'iken-m06': 'local/saves/generated/CaesarAlan-Iken/m06.sav',
    'itjtawy5-m06': 'local/saves/generated/CaesarAlan-Itjtawy-plus5/m06.sav',
    'itjtawy-m06': 'local/saves/generated/CaesarAlan-Itjtawy/m06.sav',
    'pak00-m06': 'local/saves/generated/pak00/m06.sav',
    'sais-m06': 'local/saves/generated/sais_6374/m06.sav',
}
DISPLAYS = ['640x480x555', '800x600x565', '1024x768x565', '1024x768x555']

BUILDINGS = {  # type: [(save, x, y)] the first building of the type in up to two saves
    0x0A: [('sais', 54, 46), ('sais-m06', 54, 46)],
    0x0B: [('sais', 79, 99), ('sais-m06', 79, 99)],
    0x0E: [('actium', 87, 65), ('actium-m06', 87, 65)],
    0x0F: [('alexc', 135, 63), ('maritis', 92, 80)],
    0x13: [('sais', 128, 83), ('sais-m06', 106, 90)],
    0x14: [('iken', 132, 68), ('sais', 65, 39)],
    0x15: [('sais', 61, 53), ('sais-m06', 61, 53)],
    0x16: [('sais', 124, 83), ('sais-m06', 140, 93)],
    0x17: [('alexc', 95, 109), ('iken', 187, 106)],
    0x1B: [('itjtawy', 69, 123), ('itjtawy5', 69, 123)],
    0x1D: [('alexc', 119, 121), ('iken', 177, 87)],
    0x1E: [('alexc', 126, 193), ('iken', 192, 108)],
    0x1F: [('alexc', 117, 109), ('iken', 177, 84)],
    0x20: [('alexc', 110, 112), ('iken', 184, 87)],
    0x21: [('alexc', 119, 117), ('iken', 82, 41)],
    0x22: [('alexc', 97, 197), ('iken', 161, 61)],
    0x23: [('alexc', 89, 196), ('iken', 157, 58)],
    0x24: [('alexc', 98, 136), ('iken', 140, 82)],
    0x29: [('alexc', 91, 120), ('iken', 188, 105)],
    0x2A: [('alexc', 108, 92), ('iken', 182, 87)],
    0x2B: [('alexc', 95, 89), ('iken', 182, 84)],
    0x2E: [('actium', 99, 65), ('alexc', 96, 191)],
    0x2F: [('alexc', 107, 99), ('iken', 175, 72)],
    0x31: [('alexc', 147, 166), ('iken', 173, 81)],
    0x33: [('alexc', 98, 118), ('iken', 179, 83)],
    0x35: [('alexc', 90, 111), ('iken', 178, 91)],
    0x36: [('actium', 108, 75), ('alexc', 74, 90)],
    0x37: [('actium', 100, 65), ('alexc', 119, 175)],
    0x39: [('actium', 105, 76), ('alexc', 71, 91)],
    0x3C: [('iken', 137, 60), ('itjtawy', 106, 176)],
    0x3D: [('actium', 77, 58), ('alexc', 91, 114)],
    0x3E: [('alexc', 115, 100), ('itjtawy', 86, 138)],
    0x3F: [('iken', 178, 80), ('itjtawy', 95, 137)],
    0x40: [('alexc', 109, 117), ('iken', 182, 101)],
    0x42: [('alexc', 100, 107), ('alexc-m06', 100, 107)],
    0x44: [('iken', 84, 48), ('sais', 59, 64)],
    0x45: [('itjtawy', 57, 143), ('itjtawy5', 57, 143)],
    0x46: [('actium', 97, 65), ('alexc', 95, 117)],
    0x47: [('actium', 87, 57), ('alexc', 87, 86)],
    0x48: [('actium', 77, 55), ('alexc', 91, 88)],
    0x49: [('actium', 78, 55), ('alexc', 92, 88)],
    0x4A: [('actium', 88, 52), ('alexc', 90, 82)],
    0x4B: [('actium', 78, 51), ('alexc', 93, 82)],
    0x4C: [('actium', 58, 55), ('alexc', 97, 82)],
    0x4E: [('sais', 66, 53), ('sais-m06', 66, 53)],
    0x4F: [('actium', 89, 65), ('alexc', 121, 55)],
    0x51: [('actium', 90, 55), ('alexc', 130, 56)],
    0x56: [('alexc', 92, 100), ('iken', 133, 72)],
    0x5A: [('alexc', 139, 152), ('iken', 150, 69)],
    0x5C: [('sais', 48, 49), ('sais-m06', 48, 49)],
    0x5E: [('alexc', 87, 90), ('iken', 144, 65)],
    0x5F: [('actium', 68, 55), ('alexc', 84, 91)],
    0x63: [('alexc-m06', 150, 169)],
    0x64: [('alexc', 96, 204), ('iken', 145, 75)],
    0x65: [('iken', 147, 71), ('itjtawy', 64, 109)],
    0x66: [('alexc', 95, 201), ('itjtawy', 110, 141)],
    0x67: [('itjtawy', 75, 68), ('itjtawy5', 75, 68)],
    0x68: [('itjtawy', 135, 117), ('itjtawy5', 135, 117)],
    0x69: [('maritis', 93, 99), ('maritis-m06', 93, 99)],
    0x6B: [('itjtawy', 101, 182), ('itjtawy5', 101, 182)],
    0x6C: [('actium', 76, 52), ('sais', 50, 50)],
    0x6D: [('itjtawy', 123, 150), ('itjtawy5', 123, 150)],
    0x6E: [('alexc', 110, 88), ('iken', 134, 43)],
    0x6F: [('iken', 112, 54), ('itjtawy', 87, 118)],
    0x70: [('actium', 72, 55), ('iken', 128, 40)],
    0x72: [('iken', 131, 52), ('itjtawy', 122, 159)],
    0x73: [('actium', 102, 55), ('iken', 192, 97)],
    0x88: [('iken', 96, 38), ('itjtawy', 97, 84)],
    0x8A: [('actium', 107, 54), ('alexc', 94, 144)],
    0x8C: [('iken', 187, 105), ('itjtawy', 121, 176)],
    0x8D: [('actium', 75, 61), ('alexc', 92, 136)],
    0x8E: [('alexc', 91, 135), ('itjtawy', 123, 181)],
    0x8F: [('iken', 129, 70), ('itjtawy', 123, 182)],
    0x90: [('alexc', 91, 136), ('iken', 186, 110)],
    0xA1: [('iken', 195, 106), ('iken-m06', 195, 106)],
    0xA7: [('actium', 89, 55), ('alexc', 133, 58)],
    0xAD: [('iken', 185, 122), ('iken-m06', 185, 122)],
    0xB1: [('alexc', 101, 83), ('iken', 122, 85)],
    0xB2: [('itjtawy', 104, 184), ('itjtawy5', 104, 184)],
    0xB3: [('alexc', 128, 93), ('iken', 123, 82)],
    0xB4: [('actium', 97, 62), ('alexc', 112, 172)],
    0xB5: [('iken', 134, 36), ('itjtawy', 94, 83)],
    0xB6: [('actium', 85, 52), ('iken', 95, 29)],
    0xB7: [('itjtawy', 117, 198), ('itjtawy5', 117, 198)],
    0xB8: [('alexc', 110, 93), ('iken', 135, 57)],
    0xBB: [('maritis', 104, 86), ('maritis-m06', 104, 86)],
    0xBC: [('sais', 110, 102), ('sais-m06', 110, 102)],
    0xBD: [('alexc', 140, 158), ('iken', 189, 111)],
    0xC2: [('itjtawy', 89, 157), ('itjtawy5', 89, 157)],
    0xC3: [('alexc', 90, 160), ('itjtawy', 103, 83)],
    0xC4: [('iken', 123, 48), ('iken-m06', 123, 48)],
    0xC7: [('alexc', 111, 197), ('iken', 128, 50)],
    0xCA: [('alexc', 140, 134), ('iken', 202, 101)],
    0xCB: [('alexc', 92, 134), ('itjtawy', 101, 83)],
    0xCC: [('itjtawy', 111, 153), ('itjtawy5', 111, 153)],
    0xCE: [('actium', 99, 62), ('alexc', 132, 155)],
    0xD0: [('iken', 122, 87), ('iken-m06', 122, 87)],
    0xD1: [('alexc', 140, 94), ('iken', 133, 85)],
    0xD2: [('itjtawy', 107, 187), ('itjtawy5', 107, 187)],
    0xD3: [('alexc', 105, 107), ('itjtawy', 59, 144)],
    0xD4: [('alexc', 106, 108), ('itjtawy', 60, 142)],
    0xDE: [('alexc', 137, 139), ('sais', 72, 75)],
    0xE1: [('alexc', 68, 101), ('alexc-m06', 68, 101)],
    0xE2: [('alexc', 146, 85), ('alexc-m06', 146, 85)],
    0xE3: [('alexc', 49, 98), ('alexc-m06', 49, 98)],
    0xE4: [('alexc', 94, 40), ('alexc-m06', 94, 40)],
}
FIGURES = {  # figure type ('b': on a building's tile): [(save, x, y)]
    '1': [('iken', 108, 45)],
    '1b': [('itjtawy', 109, 152)],
    '3': [('sais', 139, 92)],
    '4': [('actium', 67, 54)],
    '4b': [('actium', 68, 54)],
    '5': [('actium', 69, 52)],
    '5b': [('alexc', 164, 71)],
    '7': [('alexc', 140, 69)],
    '7b': [('alexc', 127, 56)],
    '8': [('actium', 72, 54)],
    '8b': [('alexc', 131, 87)],
    '9': [('alexc', 101, 85)],
    '9b': [('alexc', 142, 120)],
    '10': [('actium', 63, 50)],
    '10b': [('actium', 60, 56)],
    '11': [('iken-m06', 95, 43)],
    '11b': [('actium', 96, 75)],
    '12': [('iken', 133, 77)],
    '12b': [('actium', 108, 75)],
    '13b': [('sais', 59, 71)],
    '14b': [('actium', 96, 75)],
    '15': [('alexc', 166, 66)],
    '15b': [('alexc', 154, 95)],
    '16': [('alexc', 140, 66)],
    '16b': [('alexc', 166, 94)],
    '17': [('alexc', 162, 71)],
    '17b': [('alexc', 165, 71)],
    '18': [('alexc', 99, 96)],
    '18b': [('alexc', 118, 108)],
    '19': [('alexc', 96, 84)],
    '19b': [('alexc', 142, 96)],
    '20': [('actium', 75, 47)],
    '21': [('alexc', 98, 84)],
    '21b': [('alexc', 143, 96)],
    '25': [('actium', 72, 21)],
    '26': [('actium', 76, 58)],
    '26b': [('alexc', 166, 72)],
    '27': [('actium', 77, 54)],
    '27b': [('alexc', 166, 96)],
    '29': [('alexc', 94, 111)],
    '29b': [('alexc', 94, 96)],
    '30': [('alexc', 105, 96)],
    '30b': [('alexc', 106, 96)],
    '31': [('alexc', 137, 60)],
    '31b': [('alexc', 142, 73)],
    '32': [('actium', 85, 55)],
    '32b': [('alexc', 166, 69)],
    '33': [('actium', 92, 55)],
    '33b': [('alexc', 143, 72)],
    '34': [('alexc', 94, 95)],
    '34b': [('alexc', 128, 154)],
    '38': [('actium', 80, 54)],
    '38b': [('alexc', 127, 154)],
    '39': [('alexc', 166, 87)],
    '39b': [('iken', 87, 45)],
    '40': [('alexc', 102, 120)],
    '40b': [('itjtawy5', 72, 126)],
    '42': [('iken', 79, 71)],
    '42b': [('iken', 83, 71)],
    '62b': [('iken', 80, 71)],
    '65': [('actium', 73, 22)],
    '66': [('iken', 90, 44)],
    '66b': [('iken', 88, 44)],
    '68': [('sais', 44, 86)],
    '69': [('pak00-m06', 26, 44)],
    '70': [('actium', 72, 71)],
    '73': [('iken', 195, 99)],
    '73b': [('iken', 202, 103)],
    '75': [('actium', 76, 55)],
    '76': [('iken', 99, 39)],
    '77': [('iken', 125, 98)],
    '78': [('actium', 70, 48)],
    '79': [('alexc', 102, 85)],
    '80': [('itjtawy', 105, 183)],
    '80b': [('itjtawy', 103, 187)],
    '81': [('alexc', 134, 82)],
    '81b': [('alexc', 118, 94)],
    '82': [('iken', 152, 50)],
    '85': [('alexc', 112, 163)],
    '85b': [('itjtawy', 100, 112)],
    '87': [('actium', 80, 54)],
    '87b': [('alexc', 166, 70)],
    '88': [('actium', 106, 57)],
    '88b': [('alexc', 142, 86)],
    '89': [('alexc', 118, 106)],
    '89b': [('alexc', 118, 96)],
    '90': [('alexc', 82, 133)],
    '90b': [('alexc', 130, 154)],
    '91': [('alexc-m06', 103, 96)],
    '91b': [('alexc-m06', 94, 107)],
    '95': [('actium', 73, 22)],
    '102': [('alexc', 175, 149)],
    '105': [('alexc', 142, 82)],
}

OPEN, DRAW, FRAME, INPUT = 0x004F7E50, 0x004F8E60, 0x004F94A0, 0x004F96E0
GOTO_TILE = 0x00430CD0
MAP_START = 0x00BCA2D8
MOUSE_TILE = 0x00BCA2C4          # offset, x (0x00BCA2C8), y (0x00BCA2CC)
MOUSE_X, MOUSE_Y = 0x00E38E34, 0x00E38E30
LEFT_PRESSED, RIGHT_RELEASED = 0x00E38E03, 0x00E38DF8
WINDOW, REDRAW = 0x00E38DDA, 0x00E38DCA
PANEL_X, PANEL_Y = 0x00EADD90, 0x00EADD8C
ORDERS = 0x00EADCC0
GRID = 228
BUILDING_BASE, BUILDING_SIZE = 0x007999D8, 0x108


def case(save, x, y, display='1024x768x565', func='draw', ticks=0, orders=False, sets=None, btype=None,
         mouse=None, click=None):
    """x, y: the clicked tile; btype: the type poked into the tile's building first (synthetic);
    mouse: the mouse position at the open (default: the screen's centre); click: (dx, dy, 'L' | 'R')
    for func 'input'; sets: [(address, size, value)] written after the open"""
    return dict(save=save, x=x, y=y, display=display, func=func, ticks=ticks, orders=orders, sets=sets,
                btype=btype, mouse=mouse, click=click)


CASES = {}
for _t, _where in BUILDINGS.items():
    for _s, _x, _y in _where:
        CASES['b%02x-%s' % (_t, _s)] = case(_s, _x, _y)
        CASES['b%02x-%s-frame' % (_t, _s)] = case(_s, _x, _y, func='frame')
    _s, _x, _y = _where[0]
    CASES['b%02x-%s-open' % (_t, _s)] = case(_s, _x, _y, func='open')
    CASES['b%02x-%s-640' % (_t, _s)] = case(_s, _x, _y, display='640x480x555')
    CASES['b%02x-%s-800' % (_t, _s)] = case(_s, _x, _y, display='800x600x565', mouse=(400, 100))
    if _t in (0x46, 0x47, 0x48, 0xBE, 0xBF, 0xC1):  # storage: the orders sub-panel
        CASES['b%02x-%s-orders' % (_t, _s)] = case(_s, _x, _y, orders=True)
        CASES['b%02x-%s-orders-frame' % (_t, _s)] = case(_s, _x, _y, orders=True, func='frame')
        CASES['b%02x-%s-orders-640' % (_t, _s)] = case(_s, _x, _y, orders=True, display='640x480x555')
for _k, _where in FIGURES.items():
    for _s, _x, _y in _where:
        CASES['f%s-%s' % (_k, _s)] = case(_s, _x, _y)
        CASES['f%s-%s-open' % (_k, _s)] = case(_s, _x, _y, func='open')

# types no corpus save has, each poked into a building of the nearest corpus type (synthetic: both
# sides read the same record), so that every panel of the redraw's jump table is reached
SYNTHETIC = [0x0C, 0x0D, 0x25, 0x38, 0x3A, 0xC8, 0xAA, 0xC9, 0x3B, 0xAC, 0x50, 0x58, 0x59, 0x5D, 0x62, 0xE0, 0x6A,
             0xD8, 0xDD, 0xD9, 0xA2, 0xE8, 0xE9, 0xCD, 0x71, 0xE7, 0xD5, 0xE6, 0xE5, 0xEA, 0xEB, 0xEC]
for _t in SYNTHETIC:
    _host = min(BUILDINGS, key=lambda h: (abs(h - _t), h))
    _s, _x, _y = BUILDINGS[_host][0]
    CASES['s%02x-%s' % (_t, _s)] = case(_s, _x, _y, btype=_t)
    CASES['s%02x-%s-frame' % (_t, _s)] = case(_s, _x, _y, btype=_t, func='frame')
    if _t in (0xE5, 0xE6, 0xEA, 0xEB, 0xEC):  # the monuments' progress records (core/city/monument_rating.c)
        CASES['s%02x-%s-open' % (_t, _s)] = case(_s, _x, _y, btype=_t, func='open')

# terrain tiles (no building, no figure): the text 0x00EADD4C the open chose, found by running the original's
# open over the maps
TERRAIN = {0x01: ('actium', 0, 0), 0x03: ('actium', 72, 18), 0x06: ('actium', 63, 48), 0x07: ('iken', 120, 39),
           0x0A: ('actium', 72, 3), 0x0B: ('iken', 129, 87), 0x0C: ('alexc', 69, 45), 0x0D: ('iken', 81, 51),
           0x10: ('iken', 198, 105), 0x11: ('actium', 69, 0), 0x13: ('alexc', 93, 201), 0x14: ('iken', 117, 6),
           0x15: ('actium', 60, 21), 0x16: ('iken', 39, 75), 0x18: ('iken', 81, 36), 0x19: ('actium', 111, 48)}
for _k, (_s, _x, _y) in TERRAIN.items():
    CASES['t%02x-%s' % (_k, _s)] = case(_s, _x, _y)
    CASES['t%02x-%s-open' % (_k, _s)] = case(_s, _x, _y, func='open')

# the input 0x004F96E0: clicks on the panels' buttons (dx, dy, button[, base]): base 'top' (default: dy from
# the panel's y 0x00EADD8C), 'bottom' (dy from its bottom, 0x00EADD8C + 16 * 0x00EADD84) or 'screen' (dy is
# the screen's y); dx from the panel's x 0x00EADD90. Buttons from the tables of docs/commands.md 3.5, 3.7:
# help / close 0x005C7B10, orders 0x005C5A98 / 0x005C5AB8 / 0x005C5AD8, order rows 0x005C68C8, legion
# 0x005C66D8 / 0x005C6768, ships 0x005C6788 / 0x005C6818, figure list 0x005C5BE8 / 0x005C5CB0, house
# 0x005C5A78, palace 0x005C5188, temples 0x005C5448
CLICKS = {
    'close-right': (200, 100, 'R'), 'help': (19, -0x28 + 5, 'L', 'bottom'), 'close': (429, -0x28 + 8, 'L', 'bottom'),
    'nothing': (200, 60, 'L'),
}
INPUT_CASES = [
    ('b0e-actium', ['close-right', 'help', 'close', 'nothing', ('house', (0x70 + 20, -0x20 + 5, 'L', 'bottom'))]),
    ('b46-actium', ['close-right', 'help', ('orders', (0x64 + 20, -0x22 + 5, 'L', 'bottom'))]),
    ('b47-actium', ['close-right', 'help', ('orders', (0x64 + 20, -0x22 + 5, 'L', 'bottom'))]),
    ('b48-actium', ['close-right', 'help', ('orders', (0x64 + 20, -0x22 + 5, 'L', 'bottom'))]),
    ('b48-alexc', ['close-right', ('orders', (0x64 + 20, -0x22 + 5, 'L', 'bottom'))]),
    ('bd1-alexc', ['close-right', 'help', ('palace', (100, 160, 'L'))]),
    ('b4f-actium', ['close-right', ('temple', (60, 70, 'L'))]),
    ('b22-alexc', ['close-right', 'help', 'close', 'nothing']),
    ('f11-iken-m06', ['close-right', 'help'] + [('order%d' % _i, (19 + 85 * _i + 30, 180, 'L')) for _i in range(5)] +
     [('service', (0x48 * 0 + 150, -0x28 + 10, 'L', 'bottom'))]),
    ('f12-iken', ['close-right'] + [('order%d' % _i, (19 + 85 * _i + 30, 180, 'L')) for _i in range(5)]),
    ('f78-actium', ['close-right', 'help'] + [('order%d' % _i, (19 + 85 * _i + 30, 180, 'L')) for _i in range(5)]),
    ('f77-iken', ['close-right', 'help'] + [('order%d' % _i, (19 + 85 * _i + 30, 180, 'L')) for _i in range(5)]),
    ('f4-actium', ['close-right', 'help', ('fig1', (90, 60, 'L')), ('fig0', (40, 60, 'L')), ('figorders', (0x11D + 20, 0xC3 + 5, 'L'))]),
    ('t0a-actium', ['close-right', 'help', 'close']),
]
for _base, _clicks in INPUT_CASES:
    for _c in _clicks:
        _n, _click = (_c, CLICKS[_c]) if isinstance(_c, str) else _c
        _b = dict(CASES[_base])
        _b.update(func='input', click=_click)
        CASES['%s-in-%s' % (_base, _n)] = _b
for _base, _row in (('b48-actium', 0), ('b48-actium', 2), ('b47-actium', 1), ('b46-actium', 0), ('b48-alexc', 1)):
    _b = dict(CASES[_base])
    _b.update(func='input', click=(0xFA + 20, 0x4E + 20 * _row + 5, 'L', 'screen'), orders=True)
    CASES['%s-orders-in-row%d' % (_base, _row)] = _b
    _b = dict(CASES[_base])
    _b.update(func='input', click=(200, 100, 'R'), orders=True)
    CASES['%s-orders-in-close-right' % _base] = _b

GROUPS = {
    'quick': ['b0e-actium', 'b46-alexc', 'f4-actium'],
}


def setup(g, c):
    """the open of case c, run by the original; returns the calls to compare"""
    import harness
    x, y = c['x'], c['y']
    off = g.i32(MAP_START) + y * GRID + x
    g.call(GOTO_TILE, off)
    if c['btype'] is not None:
        b = int.from_bytes(g.read(0x00B4E6FC + 2 * off, 2), 'little', signed=True)
        g.write(BUILDING_BASE + BUILDING_SIZE * b + 0x10, c['btype'].to_bytes(2, 'little'))
    g.set_u32(MOUSE_TILE, off)
    g.set_u32(MOUSE_TILE + 4, x)
    g.set_u32(MOUSE_TILE + 8, y)
    w, h = g.i32(0x00E38DB0), g.i32(0x00E38DAC)
    mx, my = c['mouse'] or (w // 2, h // 2)
    g.set_u32(MOUSE_X, mx)
    g.set_u32(MOUSE_Y, my)
    g.write(WINDOW, b'\x01')
    if c['func'] == 'open':
        return [(OPEN, []), (DRAW, [])]
    g.call(OPEN)
    if c['orders']:
        g.set_u32(ORDERS, 1)
    for a, size, value in c.get('sets') or ():
        g.write(a, (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little'))
    if c['func'] == 'draw':
        return [(DRAW, [])]
    g.call(DRAW)
    if c['func'] == 'frame':
        return [(FRAME, [])]
    if c['func'] == 'input':
        dx, dy, button = c['click'][:3]
        base = c['click'][3] if len(c['click']) > 3 else 'top'
        y0 = {'top': g.i32(PANEL_Y), 'bottom': g.i32(PANEL_Y) + 16 * g.i32(0x00EADD84), 'screen': 0}[base]
        g.set_u32(MOUSE_X, g.i32(PANEL_X) + dx)
        g.set_u32(MOUSE_Y, y0 + dy)
        g.write(LEFT_PRESSED, b'\x01' if button == 'L' else b'\x00')
        g.write(RIGHT_RELEASED, b'\x01' if button == 'R' else b'\x00')
        return [(INPUT, []), (DRAW, [])]
    return [harness.parse_call(c['func'])]


def check(name, png=False):
    import harness
    import screen
    c = CASES[name]
    lines = []
    try:
        g = harness.oracle_state(os.path.join(ROOT, SAVES[c['save']]), c['ticks'], c['display'])
        calls = setup(g, c)
        with tempfile.TemporaryDirectory() as tmp:
            written, ranges, bad = harness.compare(g, calls, tmp)
    except Exception as e:  # noqa: BLE001
        return False, '%s: error %s' % (name, e)
    for (a, args), ro, rp in bad[:4]:
        lines.append('    0x%08X returned %d, port %d' % (a, ro, rp))
    kind = '%d/%d' % (g.i32(0x00EADD50), g.i32(0x00EADD6C))
    if not ranges and not bad:
        return True, '%s: equal (kind/building %s, the original changed %d bytes, %d replayed)' % (
            name, kind, written, harness.compare.replayed)
    pixels, rects = harness.compare.screen
    if pixels:
        lines.append('    screen: %d pixels differ in %d rectangles: %s' % (pixels, len(rects), ', '.join(
            '%dx%d at %d,%d' % (w, h, x, y) for x, y, w, h in rects[:6])))
        if png:
            _, w, h, pitch, fmt = g.screen_info()
            stem = os.path.join(screen.SCREENS, 'info_' + name)
            screen.write_png(stem + '_original.png', g.screen(), w, h, pitch, fmt, rects)
            screen.write_png(stem + '_port.png', harness.compare.port_screen, w, h, pitch, fmt, rects)
            lines.append('    screens: %s_original.png, %s_port.png' % (stem, stem))
    import diff
    pre, post_o, post_p = harness.compare.last
    for addr, n in ranges[:10]:
        k = addr - harness.STATE_BASE if addr < 0x60000000 else None
        if k is None:
            lines.append('    %s, %d bytes' % (diff.name_address(addr), n))
            continue
        k4 = k & ~3
        lines.append('    %s, %d bytes: before %s original %s port %s' % (
            diff.name_address(addr), n, pre[k4:k4 + 4][::-1].hex(), post_o[k4:k4 + 4][::-1].hex(),
            post_p[k4:k4 + 4][::-1].hex()))
    return False, '%s: %d bytes differ in %d runs (kind/building %s)\n%s' % (
        name, sum(n for _, n in ranges), len(ranges), kind, '\n'.join(lines))


ISOLATE = ('blit', 'sidebar', 'frame', 'sound', 'ui', 'cityview', 'wincore', 'advisors', 'msgempire', 'frontend')
NOT_REPLAYABLE = {0x004F0BA0, 0x00486A30, 0x004F0AB0,  # they write their caller's stack
                  0x00448370, 0x004CB980, 0x004CD010, 0x004CD120}  # (the monument panels' text expansion and wrap)


def isolate():
    """harness.FORCED_REPLAY := the ports claimed by the other phase-7 packages"""
    import harness
    owners = {}
    for line in open(os.path.join(ROOT, 'local', 'oracle', 'claims.txt')):
        f = line.split()
        if len(f) >= 2:
            owners[int(f[0], 16)] = f[1]
    harness.FORCED_REPLAY.update(a for a in harness.ported() if owners.get(a) in ISOLATE and a not in NOT_REPLAYABLE)


def _run(args):
    if args[2]:
        isolate()
    return check(*args[:2])


def main():
    argv = sys.argv[1:]
    jobs = 1
    if '-j' in argv:
        k = argv.index('-j')
        jobs = int(argv[k + 1])
        del argv[k:k + 2]
    if '--list' in argv:
        print('\n'.join(sorted(CASES)))
        return 0
    png = '--png' in argv
    if png:
        argv.remove('--png')
    iso = '--isolate' in argv
    if iso:
        argv.remove('--isolate')
    names = []
    for a in argv or sorted(CASES):
        if a in GROUPS:
            names += GROUPS[a]
        elif a in CASES:
            names.append(a)
        else:
            matched = [n for n in sorted(CASES) if n.startswith(a)]
            if not matched:
                print('no case %s' % a)
            names += matched
    if not names:
        return 2
    ok = 0
    if jobs > 1:
        from multiprocessing import Pool
        with Pool(jobs) as p:
            for good, text in p.imap(_run, [(n, png, iso) for n in names]):
                print(text, flush=True)
                ok += good
    else:
        for n in names:
            good, text = _run((n, png, iso))
            print(text, flush=True)
            ok += good
    print('info: %d / %d cases equal' % (ok, len(names)))
    return 0 if ok == len(names) else 1


if __name__ == '__main__':
    sys.exit(main())
