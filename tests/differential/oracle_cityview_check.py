"""Imhotep - differential test: the city view (roadmap 7.4), state and back buffer against the original.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tests/differential/oracle_cityview_check.py [CASE|GROUP ...] [-j JOBS] [--list] [--png]
        [--isolate]

Each case boots the reference game with the emulated DirectDraw (docs/oracle.md "Display") at a
resolution and pixel format, loads a save, optionally advances it some ticks, then sets up the view
in the original (orientation: the rotate button 0x00409EB0 (2: clockwise) run n times; camera: 0x00430CD0 centred on a
tile; overlay 0x00E94E0C; construction: the choose-building handler 0x00405200, the hovered tile
0x00BCA300.. and optionally a drag 0x00405C20 / 0x00405D90 as tests/differential/oracle_command_check.py
build() does; the animation flags: the frame clock 0x00E92E0C past the view's 60 ms step and the
51 animation timer flags 0x00605B38 raised). Then the function under test runs in the
original and in the port from that same state (tools/oracle/harness.py compare: unported callees
replayed): the state and the back buffer must be equal.
Functions: 'view' = 0x00430D30(camera x, camera y, map start); any other ADDR[:ARGS] runs after the
view was drawn once in the original (so that the view's per-frame globals are set), e.g. the ground
pass 0x00430EF0 or the upper pass 0x00436B60:0.
--isolate replays the ports of the other phase-7 packages (blit, sidebar, frame, sound, ui: the
functions they claim in local/oracle/claims.txt) from the original's record instead of running them,
so that the city view's own ports are checked alone while theirs are in progress.
Exit status: 0 if every case is equal, 1 otherwise, 2 on setup error.
"""
import os
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))
sys.path.insert(0, os.path.join(ROOT, 'tests', 'differential'))

SAVES = {
    'actium': 'local/saves/downloaded/1592/39-Actium-Dec30c.sav',
    'iken': 'local/saves/downloaded/1564/CaesarAlan-Iken.sav',
    'alexc': 'local/saves/downloaded/1591/38-AlexC-Feb15.sav',
    'sais': 'local/saves/downloaded/1582/sais 6374.sav',
    'maritis': 'local/saves/downloaded/1590/37-Maritis-Dec44c.sav',
    'itjtawy': 'local/saves/downloaded/1562/CaesarAlan-Itjtawy.sav',
    'pak00': 'local/saves/generated/pak00/m00.sav',
}
DISPLAYS = ['640x480x555', '800x600x565', '1024x768x565', '1024x768x555']

VIEW = 0x00430D30
CAMERA_X, CAMERA_Y, MAP_START = 0x00BCA2BC, 0x00BCA2C0, 0x00BCA2D8
ROTATE_CW = 0x00430B20
ROTATE_BUTTON = 0x00409EB0   # the rotate buttons' callback (parameter 0x00D3B198: 2 clockwise): 0x00430B20,
                             # then the view orientation 0x00BCA2E0, 0x0047D6E0, the clamp, the redraw flag
GOTO_TILE = 0x00430CD0
OVERLAY = 0x00E94E0C
FRAME_CLOCK, VIEW_ANIM_CLOCK = 0x00E92E0C, 0x0070E3C0
ANIM_FLAGS = 0x00605B38          # u8[51] animation timer flags (0x0040F270)
SET_TYPE, MOUSE_DOWN, DRAG = 0x00405200, 0x00405C20, 0x00405D90
SUBMENU, ITEM, TYPE = 0x007056EC, 0x00D3B198, 0x006059E8
HOVER = 0x00BCA300               # offset, x (0x00BCA304), y (0x00BCA308)
MOUSE_TILE = 0x00BCA2C4          # offset, x, y
ROTATION = 0x00BC9714
GRID = 228
FIGURE_GRID, FIGURE_BASE, FIGURE_SIZE = 0x00B350DC, 0x00BCD5F8, 0x184
SCROLLING = 0x00E38DCC        # i32 the map scrolls: no ghost (0x0043B630)


def case(save, display='1024x768x565', ticks=0, rot=0, goto=None, overlay=0, build=None, anim=False,
         func='view', pokes=None, sets=None):
    """pokes: [(x, y, {offset: (size, value)})]: fields written into the first figure on tile (x, y)
    (a synthetic state, to reach drawing paths no corpus save shows: figure types, work cycles);
    sets: [(address, size, value)]: globals written (debug switches, ...)"""
    return dict(save=save, display=display, ticks=ticks, rot=rot, goto=goto, overlay=overlay, build=build,
                anim=anim, func=func, pokes=pokes, sets=sets)


CASES = {}
# the city view at each resolution and pixel format
for _s in SAVES:
    for _d in DISPLAYS:
        CASES['view-%s-%s' % (_s, _d)] = case(_s, _d)
# orientations, cameras, ticks (animations)
for _s in ('actium', 'iken', 'alexc', 'sais', 'maritis', 'itjtawy', 'pak00'):
    for _r in (1, 2, 3):
        CASES['rot%d-%s' % (_r, _s)] = case(_s, rot=_r, anim=True)
    for _t in (37, 400):
        CASES['ticks%d-%s' % (_t, _s)] = case(_s, ticks=_t, anim=True)
    CASES['anim-%s' % _s] = case(_s, anim=True)
for _s, _tiles in (('actium', [(40, 40), (110, 70), (180, 160)]), ('iken', [(30, 60), (100, 120), (60, 190)]),
                   ('alexc', [(50, 100), (150, 90), (120, 200)]), ('sais', [(60, 60), (120, 140)]),
                   ('maritis', [(80, 80), (140, 150)]), ('itjtawy', [(70, 90), (130, 130)]),
                   ('pak00', [(40, 60), (100, 100)])):
    for _x, _y in _tiles:
        for _r in (0, 2):
            CASES['camera-%s-%d-%d-rot%d' % (_s, _x, _y, _r)] = case(_s, goto=(_x, _y), rot=_r, anim=True)
# figures (0x00438E50) on cameras where the corpus has them: between tiles (+0x0C), with second images
for _s, _tiles in (('actium', [(50, 45), (108, 75), (69, 54)]), ('iken', [(109, 63), (130, 93), (132, 51)]),
                   ('alexc', [(80, 37), (74, 90), (142, 82)]), ('sais', [(59, 75), (67, 111)]),
                   ('itjtawy', [(109, 170), (87, 61)]), ('maritis', [(41, 99), (116, 73)])):
    for _x, _y in _tiles:
        for _r in (0, 1, 2, 3):
            CASES['figs-%s-%d-%d-rot%d' % (_s, _x, _y, _r)] = case(_s, goto=(_x, _y), rot=_r)
# synthetic figure kinds (pokes into the cart pusher on Actium's tile 69, 54: it has a second image):
# types 1, 0x0D, 0x24, 0x3C, 0x4A, 0x4D; the load before / after (+0x65); directions 0..7 of type 4;
# a work cycle between buildings 19 and 27 (+0x120, 0x004F0D30, 0x004F08C0)
FIG = (69, 54)
_FIGPOKES = {
    'type01': {0x0A: (1, 1)}, 'type0d-front': {0x0A: (1, 0x0D), 0x6D: (1, 3), 0x6C: (1, -2)},
    'type0d-back': {0x0A: (1, 0x0D), 0x6D: (1, -4), 0x6C: (1, 5)}, 'type24': {0x0A: (1, 0x24), 0x0B: (1, 0x10)},
    'type24-odd': {0x0A: (1, 0x24), 0x0B: (1, 0x33), 0x01: (1, 1)}, 'type3c': {0x0A: (1, 0x3C)},
    'type4a': {0x0A: (1, 0x4A)}, 'type4d': {0x0A: (1, 0x4D)}, 'type41': {0x41: (1, 1), 0x0A: (1, 0x0B)},
    'between': {0x0C: (1, 1), 0x4A: (2, 7), 0x4C: (2, 4)}, 'standing': {0x39: (1, 3), 0x78: (1, 2)},
    'work': {0x0C: (1, 0), 0x120: (2, 1), 0x4A: (2, 19), 0x4C: (2, 3), 0x4E: (2, 27), 0x50: (2, 8),
             0x52: (2, 2), 0x54: (2, 5)},
    'work-main': {0x0C: (1, 0), 0x120: (2, 1), 0x4A: (2, 0), 0x4E: (2, 27), 0x50: (2, 0), 0x52: (2, 9),
                  0x54: (2, 5)},
    'work-high': {0x0C: (1, 0), 0x120: (2, 1), 0x4A: (2, 27), 0x4C: (2, 40), 0x4E: (2, 128), 0x50: (2, 13),
                  0x52: (2, 1), 0x54: (2, 3)},
}
for _d in range(8):  # the load (+0x180 image at +0x6C, +0x6D) behind or in front by direction: types 4, 0x26, 9
    _FIGPOKES['dir%d' % _d] = {0x0A: (1, 4), 0x11: (1, _d), 0x6C: (1, 12), 0x6D: (1, 7)}
    _FIGPOKES['dir%d-t26' % _d] = {0x0A: (1, 0x26), 0x11: (1, _d), 0x6C: (1, -9), 0x6D: (1, 4)}
    _FIGPOKES['dir%d-t09' % _d] = {0x0A: (1, 9), 0x11: (1, _d), 0x6C: (1, 5), 0x6D: (1, -6)}
_FIGPOKES['type4d-load'] = {0x0A: (1, 0x4D), 0x6C: (1, 3), 0x6D: (1, 8)}
for _n, _p in _FIGPOKES.items():
    for _r in (0, 1, 2, 3):
        CASES['fig-%s-rot%d' % (_n, _r)] = case('actium', goto=FIG, rot=_r, pokes=[FIG + (_p,)])
    CASES['fig-%s-sel' % _n] = case('actium', goto=FIG, pokes=[FIG + (_p,)], func='figsel')
    for _r in (0, 1, 2, 3):
        CASES['figdraw-%s-rot%d' % (_n, _r)] = case('actium', goto=FIG, rot=_r, pokes=[FIG + (_p,)], func='figdraw')
# the debug grid values 0x0043E300 (switch 0x0078D7D8, grid 0x00E38E94 = 0..0x1B, 0x1C: none)
for _k in range(0x1D):
    CASES['debug%02x-actium' % _k] = case('actium', sets=[(0x0078D7D8, 4, 1), (0x00E38E94, 4, _k)])
CASES['debug03-iken-640'] = case('iken', '640x480x555', rot=1, sets=[(0x0078D7DC, 4, 1), (0x00E38E94, 4, 3)])
CASES['debug15-alexc-800'] = case('alexc', '800x600x565', sets=[(0x0078D7D4, 4, 1), (0x00E38E94, 4, 0x15)])
# figures on top (+0x77) of 0x00439740
for _r in (0, 2):
    CASES['ontop-actium-rot%d' % _r] = case('actium', goto=FIG, rot=_r, anim=True, pokes=[FIG + ({0x77: (1, 1)},)])
    CASES['ontop-actium-ovl-rot%d' % _r] = case('actium', goto=FIG, rot=_r, overlay=0x33,
                                               pokes=[FIG + ({0x77: (1, 1)},)])
# overlays (0x00E94E0C), docs/presentation.md 3.1: the values the overlay menu sets (0x00409DA0: top
# items 0x005CF734 = 4, 0x1F, 0x2D, 0x33; 0x00409E00: submenus 0x005CF75C) and the hot keys
# (0x004168E0 from the WndProc: 8, 9, 0xA, 0x1F, 0x2D, 0x33, 0x19, 0x30, 0x2B)
OVERLAYS = [4, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10, 0x11, 0x12, 0x13, 0x15, 0x16, 0x17, 0x18, 0x19,
            0x1A, 0x1B, 0x1F, 0x2B, 0x2D, 0x30, 0x31, 0x32, 0x33]
for _o in OVERLAYS:
    CASES['overlay%02x-actium' % _o] = case('actium', overlay=_o)
    CASES['overlay%02x-iken-rot1' % _o] = case('iken', overlay=_o, rot=1, anim=True)
    CASES['overlay%02x-alexc-800' % _o] = case('alexc', '800x600x565', overlay=_o, rot=2)
    CASES['overlay%02x-sais-640' % _o] = case('sais', '640x480x555', overlay=_o, rot=3, anim=True)
    CASES['overlay%02x-maritis-t37' % _o] = case('maritis', overlay=_o, ticks=37, anim=True)
    CASES['overlay%02x-itjtawy-555' % _o] = case('itjtawy', '1024x768x555', overlay=_o, goto=(70, 90))
    CASES['overlay%02x-pak00-t400' % _o] = case('pak00', overlay=_o, ticks=400, anim=True, rot=1)
# construction ghosts: (type, hovered tile[, drag end]) on Iken's free land, Actium, AlexC
# types and tiles of tests/differential/oracle_command_check.py (docs/commands.md 2: road 5, walls 6,
# canal 8, clear 9, house 10, plaza 0x26, gardens 0x27, well 0x5C, bridge 0x52, ferry 0x88)
GHOSTS_ACTIUM = {
    'road': (5, (97, 70), (105, 70)), 'wall': (6, (106, 70), (112, 71)), 'canal': (8, (90, 72), (96, 72)),
    'clear': (9, (97, 69), (99, 71)), 'house': (10, (97, 74), (99, 76)), 'plaza': (0x26, (80, 64), (90, 64)),
    'gardens': (0x27, (100, 76), (103, 78)), 'well': (0x5C, (98, 68)), 'bandstand': (0x1E, (111, 60)),
    'house1': (10, (130, 130)),
}
GHOSTS_IKEN = {
    't1e': (0x1E, (71, 43)), 't1f': (0x1F, (71, 43)), 't46': (0x46, (73, 93)), 't48': (0x48, (63, 83)),
    't5c': (0x5C, (78, 88)), 't5e': (0x5E, (63, 83)), 't64': (0x64, (28, 33)), 't6d': (0x6D, (143, 188)),
    'ferry': (0x88, (96, 38)), 'bridge': (0x52, (149, 39)), 'tb5': (0xB5, (153, 98)), 'tcc': (0xCC, (38, 58)),
    'td3': (0xD3, (68, 63)), 'te2': (0xE2, (33, 43)), 't8a': (0x8A, (93, 48)), 'tca': (0xCA, (58, 18)),
    'road': (5, (64, 52), (70, 52)), 'house': (10, (64, 60)),
}
for _n, _g in GHOSTS_ACTIUM.items():
    CASES['ghost-%s-actium' % _n] = case('actium', build=_g)
    CASES['ghost-%s-actium-rot2' % _n] = case('actium', build=_g, rot=2)
for _n, _g in GHOSTS_IKEN.items():
    CASES['ghost-%s-iken' % _n] = case('iken', build=_g)
    CASES['ghost-%s-iken-rot1' % _n] = case('iken', build=_g, rot=1)
    CASES['ghost-%s-iken-overlay' % _n] = case('iken', build=_g, overlay=2)
# more ghosts on Iken (free land, shore, river): every special draw of 0x0043B630, refusals
GHOSTS_MORE = {
    'fort28': (0x28, (68, 63)), 'fort2c': (0x2C, (71, 43)), 'fort2d': (0x2D, (63, 83)),
    'venue21': (0x21, (71, 43)), 'venued1': (0xD1, (68, 63)), 'venue1e': (0x1E, (63, 83)),
    'd2': (0xD2, (68, 63)), 'd2b': (0xD2, (33, 43)),
    'statue29': (0x29, (78, 88)), 'statue2a': (0x2A, (71, 43)), 'statue2b': (0x2B, (63, 83)),
    'gateab': (0xAB, (58, 18)), 'gateca': (0xCA, (64, 52)),
    'complex41': (0x41, (63, 83)), 'complex43': (0x43, (68, 63)), 'complex45': (0x45, (33, 43)),
    'addd3': (0xD3, (63, 83)), 'addd4': (0xD4, (68, 63)),
    'lift7': (7, (96, 38)), 'shore4a': (0x4A, (96, 38)), 'shore4b': (0x4B, (96, 38)), 'shoreb6': (0xB6, (96, 38)),
    'shorec0': (0xC0, (96, 38)), 'shore4c': (0x4C, (96, 38)), 'shore4c2': (0x4C, (63, 83)),
    'bridge53': (0x53, (149, 39)), 'bridge52b': (0x52, (96, 38)), 'ferry2': (0x88, (63, 83)),
    'mon_e1': (0xE1, (63, 83)), 'mon_e3': (0xE3, (63, 83)), 'mon_e4': (0xE4, (63, 83)), 'mon_e5': (0xE5, (63, 83)),
    'mon_e6': (0xE6, (63, 83)), 'mon_ea': (0xEA, (63, 83)), 'mon_eb': (0xEB, (63, 83)), 'mon_ec': (0xEC, (63, 83)),
    'mon_d5': (0xD5, (63, 83)), 'mon_de': (0xDE, (63, 83)), 'pyramid': (0xB7, (63, 83)),
    't0b': (0x0B, (63, 83)), 't10': (0x10, (64, 60)), 't38': (0x38, (63, 83)), 't48': (0x48, (63, 83)),
    'tc1': (0xC1, (63, 83)), 'tc2': (0xC2, (63, 83)), 'tc3': (0xC3, (63, 83)), 'tcf': (0xCF, (63, 83)),
    'td0': (0xD0, (63, 83)), 't26': (0x26, (64, 52)), 't3b': (0x3B, (63, 83)), 't47': (0x47, (63, 83)),
    't4d': (0x4D, (63, 83)), 'tbb': (0xBB, (63, 83)), 't5f': (0x5F, (63, 83)), 't32': (0x32, (63, 83)),
    't6a': (0x6A, (96, 38)), 't66': (0x66, (63, 83)), 't70': (0x70, (63, 83)), 'tb4': (0xB4, (63, 83)),
    'te_7e': (0x7E, (63, 83)), 'te_84': (0x84, (96, 38)), 'te_85': (0x85, (63, 83)), 'te_8b': (0x8B, (63, 83)),
    'te_d6': (0xD6, (96, 38)), 'te_81': (0x81, (63, 83)), 'te_82': (0x82, (96, 38)), 'te_7f': (0x7F, (63, 83)),
    'canal': (8, (63, 83)), 'canal_drag': (8, (63, 83), (63, 88)), 'road_shore': (5, (96, 38)),
    'edge_house': (10, (2, 100)), 'edge_fort': (0x28, (3, 3)), 'edge_complex': (0x41, (5, 120)),
}
for _n, _g in GHOSTS_MORE.items():
    for _r in range(4):
        CASES['gfn-%s-iken-rot%d' % (_n, _r)] = case('iken', build=_g, rot=_r, func='0x0043B630')
# ghosts that can be placed (green): pak00 m00 (free 13x13 land at (43, 43), no complex, no fort),
# Maritis (free land at (67, 28), no complex)
GHOSTS_GREEN = {
    'fort28': (0x28, (49, 49)), 'fort2c': (0x2C, (47, 47)), 'venue21': (0x21, (47, 47)), 'venued1': (0xD1, (46, 46)),
    'venue1e': (0x1E, (48, 48)), 'venue1f': (0x1F, (48, 48)), 'd2': (0xD2, (49, 49)), 'statue2a': (0x2A, (49, 49)),
    'gateab': (0xAB, (49, 49)), 'gateca': (0xCA, (48, 48)), 'complex41': (0x41, (49, 49)), 'complex44': (0x44, (49, 47)),
    'complex41b': (0x41, (44, 50)), 'complex41c': (0x41, (50, 44)), 'complex43b': (0x43, (46, 46)),
    'mon_e1': (0xE1, (49, 49)), 'mon_e5': (0xE5, (49, 49)), 'mon_d5': (0xD5, (49, 49)), 'mon_de': (0xDE, (49, 49)),
    'pyramid': (0xB7, (49, 49)), 'house': (10, (49, 49)), 'well': (0x5C, (49, 49)), 't48': (0x48, (49, 49)),
    'tcf': (0xCF, (49, 49)), 'tc3': (0xC3, (49, 49)), 't66': (0x66, (49, 49)), 't3b': (0x3B, (49, 49)),
    'road': (5, (49, 49)), 'canal': (8, (49, 49)), 'houses_drag': (10, (45, 45), (50, 48)),
}
for _n, _g in GHOSTS_GREEN.items():
    for _r in range(4):
        CASES['gfn-%s-pak00-rot%d' % (_n, _r)] = case('pak00', build=_g, rot=_r, func='0x0043B630')
for _n, _t in (('complex42', 0x42), ('venue21', 0x21), ('fort2d', 0x2D), ('gateca', 0xCA), ('d2', 0xD2)):
    for _r in range(4):
        CASES['gfn-%s-maritis-rot%d' % (_n, _r)] = case('maritis', build=(_t, (73, 34)), rot=_r, func='0x0043B630')
# fully free 16x16 land (no flood plain): pak00 around (34, 59), Maritis around (47, 38)
for _n, _t in (('complex41', 0x41), ('complex42', 0x42), ('complex45', 0x45), ('fort28', 0x28), ('venue21', 0x21),
               ('venued1', 0xD1), ('d2', 0xD2), ('mon_e6', 0xE6), ('mon_ea', 0xEA), ('mon_de', 0xDE), ('pyramid', 0xB7),
               ('gateab', 0xAB), ('statue2b', 0x2B)):
    for _r in range(4):
        CASES['gfn-%s-pak00free-rot%d' % (_n, _r)] = case('pak00', build=(_t, (34, 59)), rot=_r, func='0x0043B630')
        CASES['gfn-%s-maritisfree-rot%d' % (_n, _r)] = case('maritis', build=(_t, (47, 38)), rot=_r, func='0x0043B630')
for _n in ('fort28', 'venue21', 'complex41', 'mon_e5', 'pyramid', 'gateca', 'bridge52b'):
    for _d in ('640x480x555', '800x600x565', '1024x768x555'):
        CASES['gfn-%s-iken-%s' % (_n, _d)] = case('iken', _d, build=GHOSTS_MORE[_n], rot=1, func='0x0043B630')
# the ghost alone (0x0043B630 after a view drawn by the original): every orientation
for _n, _g in GHOSTS_ACTIUM.items():
    for _r in range(4):
        CASES['gfn-%s-actium-rot%d' % (_n, _r)] = case('actium', build=_g, rot=_r, func='0x0043B630')
for _n, _g in GHOSTS_IKEN.items():
    for _r in range(4):
        CASES['gfn-%s-iken-rot%d' % (_n, _r)] = case('iken', build=_g, rot=_r, func='0x0043B630')
for _n in ('road', 'house', 'well', 'gardens'):
    CASES['ghost-%s-actium-640' % _n] = case('actium', '640x480x555', build=GHOSTS_ACTIUM[_n], rot=3)
# the passes alone, after a view drawn by the original
for _s in ('actium', 'iken', 'alexc', 'maritis'):
    for _f in ('0x00430EF0', '0x00436B60:0', '0x0043E300', '0x00439740:0'):
        CASES['pass%s-%s' % (_f.split(':')[0][-6:], _s)] = case(_s, anim=True, func=_f)
    for _f in ('0x004316F0', '0x00439F10', '0x00439740:9999'):
        CASES['pass%s-%s-overlay' % (_f.split(':')[0][-6:], _s)] = case(_s, overlay=2, func=_f)
        for _o in (4, 0xA, 0x11, 0x1B, 0x1C, 0x1F, 0x2D, 0x33):
            CASES['pass%s-%s-overlay%02x' % (_f.split(':')[0][-6:], _s, _o)] = case(_s, overlay=_o, anim=True, func=_f)

# the building parts of the upper pass (present/city/parts.c), the pass alone at cameras on the buildings
# that have them: Itjtawy 0xB7 (0x004EF550), 0xD2 (0x0041FFF0), 0x6B (0x004384D0); Iken 0xD0 (0x004E5E90),
# 0xCA (0x004C0160), 0xA1 (0x00438470); AlexC 0xE1 (0x004206F0, 0x0041F8C0 on odd orientations), 0xE3
# (0x00420D60), 0xE4 (0x00421060), 0xCA; every orientation, a resolution and pixel format per orientation
PARTS_DISPLAYS = ['1024x768x565', '800x600x565', '640x480x555', '1024x768x555']
for _s, _tiles in (('itjtawy', [(117, 195), (110, 185), (101, 182)]),
                   ('iken', [(122, 87), (202, 101), (74, 42), (195, 106)]),
                   ('alexc', [(68, 101), (49, 98), (94, 40), (140, 134), (20, 99)])):
    for _x, _y in _tiles:
        for _r in range(4):
            CASES['parts-%s-%d-%d-rot%d' % (_s, _x, _y, _r)] = case(_s, PARTS_DISPLAYS[_r], goto=(_x, _y), rot=_r,
                                                                     anim=True, func='0x00436B60:0')

GROUPS = {
    'quick': ['view-actium-1024x768x565', 'view-iken-800x600x565', 'anim-alexc', 'rot1-iken', 'overlay02-actium',
              'ghost-house-actium'],
}


def setup(g, c):
    """the view set-up of case c, run by the original; returns the calls to compare"""
    import harness
    for _ in range(c['rot']):
        g.set_u32(ITEM, 2)
        g.call(ROTATE_BUTTON)
    goto = c['goto'] or (c['build'][1] if c['build'] else None)  # a ghost: the camera on its tile
    if goto:
        x, y = goto
        g.call(GOTO_TILE, g.i32(MAP_START) + y * GRID + x)
        if c['build']:
            g.set_u32(SCROLLING, 0)  # set by the jump; the ghost is not drawn while the map scrolls
            g.set_u32(0x00605994, 0)  # a venue ghost's 'fits' flag: no ghost is drawn while it is set
    g.set_u32(OVERLAY, c['overlay'])
    for a, size, value in c.get('sets') or ():
        g.write(a, (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little'))
    for x, y, fields in c.get('pokes') or ():
        f = int.from_bytes(g.read(FIGURE_GRID + 2 * (g.i32(MAP_START) + y * GRID + x), 2), 'little', signed=True)
        if f <= 0:
            raise RuntimeError('no figure on tile %d, %d' % (x, y))
        for off, (size, value) in fields.items():
            g.write(FIGURE_BASE + FIGURE_SIZE * f + off, (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little'))
    if c['anim']:
        g.set_u32(FRAME_CLOCK, (g.u32(VIEW_ANIM_CLOCK) + 100) & 0xFFFFFFFF)
        g.write(ANIM_FLAGS, b'\x01' * 51)
    if c['build']:
        import oracle_command_check as cc
        btype, (x, y) = c['build'][0], c['build'][1]
        menus = cc.menu_types()
        if btype in menus:
            sub, item = menus[btype]
            g.set_u32(SUBMENU, sub)
            g.set_u32(ITEM, item)
            g.call(SET_TYPE)
        else:
            g.set_u32(TYPE, btype)  # not in a game menu (monuments, editor types): the type alone

        def hover(x, y):
            off = g.i32(MAP_START) + y * GRID + x
            for base in (HOVER, MOUSE_TILE):
                g.set_u32(base, off)
                g.set_u32(base + 4, x)
                g.set_u32(base + 8, y)
        hover(x, y)
        if len(c['build']) > 2:
            g.call(MOUSE_DOWN)
            hover(*c['build'][2])
            g.call(DRAG)
    view = (VIEW, [g.u32(CAMERA_X), g.u32(CAMERA_Y), g.u32(MAP_START)])
    if c['func'] == 'view':
        return [view]
    ghost_type = g.u32(TYPE)
    if c['func'].lower() == '0x0043b630':
        g.set_u32(TYPE, 0)  # the view under the ghost drawn without it: the ghost's pixels all show
    g.call(view[0], *view[1])  # the original draws once: the view's per-frame globals
    g.set_u32(TYPE, ghost_type)
    if c['anim']:
        g.set_u32(0x0070E3C4, 1)
    if c['func'] == 'figdraw':  # the figure draw alone on the poked figure, at the screen centre
        x, y, _ = c['pokes'][0]
        f = int.from_bytes(g.read(FIGURE_GRID + 2 * (g.i32(MAP_START) + y * GRID + x), 2), 'little', signed=True)
        return [(0x00438E50, [f, g.i32(0x00E38DB0) // 2, g.i32(0x00E38DAC) // 2, 0])]
    if c['func'] == 'figsel':  # the upper pass drawing only the poked figure (its screen position kept)
        x, y, _ = c['pokes'][0]
        f = int.from_bytes(g.read(FIGURE_GRID + 2 * (g.i32(MAP_START) + y * GRID + x), 2), 'little', signed=True)
        return [(0x00436B60, [f])]
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
    if not ranges and not bad:
        return True, '%s: equal (the original changed %d bytes)' % (name, written)
    pixels, rects = harness.compare.screen
    if pixels:
        lines.append('    screen: %d pixels differ in %d rectangles: %s' % (pixels, len(rects), ', '.join(
            '%dx%d at %d,%d' % (w, h, x, y) for x, y, w, h in rects[:6])))
        if png:
            _, w, h, pitch, fmt = g.screen_info()
            stem = os.path.join(screen.SCREENS, 'cityview_' + name)
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
    return False, '%s: %d bytes differ in %d runs\n%s' % (name, sum(n for _, n in ranges), len(ranges), '\n'.join(lines))


ISOLATE = ('blit', 'sidebar', 'frame', 'sound', 'ui')
# ports that cannot be replayed (they write their caller's stack: output pointers), always run natively
NOT_REPLAYABLE = {0x004F0BA0, 0x00486A30, 0x004F0AB0}  # they write their caller's stack (outputs, route buffer)


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
                print('unknown case %s' % a)
                return 2
            names += matched
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
    print('cityview: %d / %d cases equal' % (ok, len(names)))
    return 0 if ok == len(names) else 1


if __name__ == '__main__':
    sys.exit(main())
