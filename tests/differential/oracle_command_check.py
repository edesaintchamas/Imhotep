"""Imhotep - differential test: player commands against the original (roadmap phase 5).

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tests/differential/oracle_command_check.py [NAME ...] [-j JOBS] [--list]

Each entry of TESTS is a sequence of player commands (docs/commands.md) applied between ticks of
a chained lockstep run (tools/oracle/lockstep.py --commands): the command functions run in the
original and in the port from the same state and must leave equal states, and the ticks after
them must stay equal. A command whose function is not ported fails the entry.
Exit status: 0 if all equal, 1 otherwise, 2 on setup error.

Helpers write the command lines the way the UI makes them (docs/commands.md section 2.1):
build() = choose the building (0x00405200 with its menu and item), mouse down on the first tile
(0x00405C20), drag over the other tiles (0x00405D90), mouse up on the last (0x00405E90).
"""
import json
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))

ACTIUM = 'local/saves/downloaded/1592/39-Actium-Dec30c.sav'
IKEN = 'local/saves/downloaded/1564/CaesarAlan-Iken.sav'
ALEXC = 'local/saves/downloaded/1591/38-AlexC-Feb15.sav'

SET_TYPE, MOUSE_DOWN, DRAG, MOUSE_UP = 0x00405200, 0x00405C20, 0x00405D90, 0x00405E90
CANCEL, UNDO = 0x00405BA0, 0x00406010
SUBMENU, ITEM = 0x007056EC, 0x00D3B198      # read by 0x00405200
ROTATION = 0x00BC9714                       # building rotation (R key, docs/commands.md 5.2)
EDITOR_MENUS = set(range(0xE, 0x14)) | {0x1A, 0x1B, 0x25}  # map-editor tool menus (docs/commands.md 2.1)
MENUS = os.path.join(ROOT, 'local', 'oracle', 'menu_types.json')


def h32(v):
    return (v & 0xFFFFFFFF).to_bytes(4, 'little').hex()


def menu_types():
    """{type: (submenu, item)}: what the original 0x00405200 selects for each menu entry, found by
    running it (cached in local/oracle/menu_types.json)"""
    if os.path.exists(MENUS):
        return {int(k): tuple(v) for k, v in json.load(open(MENUS)).items()}
    from game import Game
    g = Game()
    g.boot()
    g.load_save(open(os.path.join(ROOT, ACTIUM), 'rb').read())
    clean = g.state_bytes()
    out = {}
    for sub in range(0x30):
        if sub in EDITOR_MENUS:
            continue
        for item in range(1, 31):
            g.load_state(clean)
            g.set_u32(0x006059E8, 0)
            g.set_u32(SUBMENU, sub)
            g.set_u32(ITEM, item)
            try:
                g.call(SET_TYPE, timeout=10 ** 6)
            except Exception:
                continue
            t = g.i32(0x006059E8)
            if t > 0 and t not in out:
                out[t] = (sub, item)
    json.dump({str(k): v for k, v in sorted(out.items())}, open(MENUS, 'w'), indent=0)
    return out


def build(tick, btype, path, rotation=None):
    """command lines placing btype along path [(x, y), ...] (first: mouse down, last: mouse up)"""
    sub, item = menu_types()[btype]
    lines = ['%d 0x%08X %X=%s %X=%s' % (tick, SET_TYPE, SUBMENU, h32(sub), ITEM, h32(item))]
    rot = ' %X=%s' % (ROTATION, h32(rotation)) if rotation is not None else ''
    x, y = path[0]
    lines.append('%d 0x%08X hover=%d,%d mouse=%d,%d%s' % (tick, MOUSE_DOWN, x, y, x, y, rot))
    for x, y in path[1:]:
        lines.append('%d 0x%08X hover=%d,%d mouse=%d,%d' % (tick, DRAG, x, y, x, y))
    lines.append('%d 0x%08X hover=%d,%d mouse=%d,%d' % (tick, MOUSE_UP, x, y, x, y))
    return lines


def line(x1, y1, x2, y2):
    """the drag path from (x1, y1) to (x2, y2): start, end"""
    return [(x1, y1), (x2, y2)]


# ---- package ui: construction UI handlers (docs/commands.md 2.1, 2.3, 2.6) ----
IKEN_SPOTS = [  # 6 x 6 blocks where the original's land check 0x0047C880 passes, 8 tiles apart
    (64, 52), (56, 60), (64, 60), (48, 68), (56, 68), (64, 68), (72, 72), (50, 76), (58, 76), (66, 80), (34, 82),
    (82, 82), (74, 84), (42, 88), (26, 90), (34, 90), (72, 92), (86, 92), (94, 92), (42, 96), (50, 96), (58, 96),
    (18, 98), (26, 98), (34, 98), (66, 100), (86, 100), (94, 100), (102, 100), (112, 102), (42, 104), (50, 104),
    (58, 104), (10, 106), (18, 106), (26, 106), (34, 106), (66, 108), (100, 108), (108, 110), (116, 110), (42, 112),
    (50, 112), (58, 112), (10, 114), (18, 114), (26, 114), (34, 114), (74, 114), (66, 116), (82, 118), (90, 118),
    (106, 118), (114, 118), (42, 120), (50, 120), (58, 120), (122, 120), (198, 120), (18, 122), (26, 122), (34, 122),
    (74, 122), (66, 124), (82, 126), (90, 126), (114, 126), (130, 126), (42, 128), (50, 128), (58, 128), (122, 128),
    (30, 130), (74, 130), (188, 130), (66, 132), (98, 132), (82, 134), (90, 134), (130, 134), (46, 136), (54, 136),
    (106, 136), (74, 138), (62, 140), (98, 140), (82, 142), (90, 142), (114, 142), (128, 142), (106, 144), (156, 144),
    (70, 146), (98, 148), (164, 148), (86, 150), (114, 150), (122, 150), (130, 150), (48, 152), (56, 152), (106, 152),
    (156, 152), (172, 152), (66, 154), (74, 154), (96, 156), (164, 156), (114, 158), (122, 158), (130, 158), (56, 160),
    (154, 160), (64, 162), (72, 162), (80, 162), (88, 164), (138, 164), (124, 166), (96, 168), (66, 170), (74, 170),
    (86, 172), (138, 172), (104, 174), (94, 176), (112, 176), (146, 176), (128, 178), (102, 182), (110, 184),
    (118, 184), (82, 186), (126, 186), (94, 188), (134, 188), (102, 190), (110, 192), (118, 192), (126, 194),
    (92, 196), (100, 198), (108, 200), (116, 200), (104, 208), (112, 208),
]
ALEXC_SPOTS = [
    (130, 26), (138, 34), (152, 48), (170, 66), (170, 74), (178, 74), (170, 82), (178, 82), (186, 82), (44, 86),
    (78, 88), (34, 90), (170, 90), (178, 90), (186, 90), (32, 98), (40, 98), (76, 98), (170, 98), (178, 98), (186, 98),
    (196, 98), (126, 100), (204, 100), (76, 106), (170, 106), (178, 106), (186, 106), (196, 106), (8, 108), (126, 108),
    (204, 108), (212, 108), (46, 114), (54, 114), (62, 114), (70, 114), (78, 114), (170, 114), (196, 114), (12, 116),
    (204, 116), (38, 122), (46, 122), (76, 122), (170, 122), (182, 122), (190, 122), (26, 124), (126, 124), (134, 124),
    (198, 124), (146, 126), (106, 128), (114, 128), (48, 130), (56, 130), (64, 130), (72, 130), (182, 130), (190, 130),
    (160, 132), (168, 132), (38, 134), (126, 136), (98, 140), (110, 140), (118, 140), (82, 142), (46, 146), (54, 146),
]
M00 = 'local/saves/generated/pak00/m00.sav'
TYPE = 0x006059E8          # construction type, read by the mouse handlers
CITY_TREASURY = 0x00EA1870 + 0x49D8 + 0xC  # current player 1 (all test saves)


def ui_menu_types():
    """the placeable types of the game's build menus (0x005CE3B8), as found by menu_types()"""
    return sorted(t for t in menu_types() if t <= 0xEC)


def ui_each_type(types, spots, per_tick=6):
    """every type of `types` built on its own spot (mouse down, a drag on the same tile, mouse up)"""
    lines = []
    for i, t in enumerate(types):
        x, y = spots[i]
        lines += build(i // per_tick, t, [(x, y), (x, y)])
    return lines


def ui_raw(tick, btype, path, extra=''):
    """a type the menus do not offer: the flags cleared by choosing a well, then 0x006059E8 set"""
    lines = build(tick, 0x5C, [path[0]])[:1]
    x, y = path[0]
    lines.append('%d 0x%08X %X=%s hover=%d,%d mouse=%d,%d%s' % (tick, MOUSE_DOWN, TYPE, h32(btype), x, y, x, y, extra))
    for x, y in path[1:]:
        lines.append('%d 0x%08X hover=%d,%d mouse=%d,%d' % (tick, DRAG, x, y, x, y))
    lines.append('%d 0x%08X hover=%d,%d mouse=%d,%d' % (tick, MOUSE_UP, x, y, x, y))
    return lines


def ui_choose_menus(tick, subs, items):
    return ['%d 0x%08X %X=%s %X=%s' % (tick, SET_TYPE, SUBMENU, h32(s), ITEM, h32(i)) for s in subs for i in items]


def ui_ferry(tick, x, y):
    """a ferry landing (type 0x88) with the draw code's validity 0x00BCA314 set"""
    lines = build(tick, 0x88, [(x, y), (x, y)])
    lines[-1] += ' BCA314=01'
    return lines


def ui_ferry_next(tick, x, y, drag=None):
    """the next landing without choosing again (the type stays 0x88)"""
    lines = build(tick, 0x88, [(x, y)] + (drag or []) + [(x, y)])[1:]
    lines[-1] += ' BCA314=01'
    return lines


def ui_with(lines, i, sets):
    """lines with SETs appended to line i"""
    lines = list(lines)
    lines[i] += ' ' + sets
    return lines


UI_RAW_TYPES = [0x38, 0x3A, 0x48, 0x4A, 0x4B, 0x52, 0x53, 0x54, 0x55, 0x57, 0x5A, 0x9C, 0xA0, 0xAA, 0xAB, 0xB7, 0xC1,
                0xC9, 0xCA, 0xCF, 0xD0, 0xD5, 0xDE, 0xE3, 0xEC, 0x32]


# package orders: settings and orders (docs/commands.md 3.1-3.6, 3.8, 3.9). Command lines set the callback
# parameters 0x00D3B198 / 0x00D3B194 and the other inputs (window context, selected resource, answer ...)
ORDERS_SAIS = 'local/saves/downloaded/1582/sais 6374.sav'
ORDERS_MARITIS = 'local/saves/downloaded/1590/37-Maritis-Dec44c.sav'
ORDERS_CITY = 0x00EA1870 + 0x49D8  # city record of player 1, the current player of every save used
ORDERS_MONTH = 816                 # ticks per month: the month pass consumes the settings


def orders(tick, addr, *sets, p1=None, p2=None, args=''):
    """a command line: the callback at addr with its parameters p1 / p2 and (address, value, size) inputs"""
    words = ['%d 0x%08X%s' % (tick, addr, args)]
    if p1 is not None:
        words.append('D3B198=%s' % h32(p1))
    if p2 is not None:
        words.append('D3B194=%s' % h32(p2))
    for a, v, n in sets:
        words.append('%X=%s' % (a, (v & ((1 << 8 * n) - 1)).to_bytes(n, 'little').hex()))
    return ' '.join(words)


def orders_city(off, v, n=4):
    """an input in the current city record"""
    return (ORDERS_CITY + off, v, n)


def orders_list(addr, values):
    """a window list: i32 count at addr, then the i32 values"""
    return [(addr, len(values), 4)] + [(addr + 4 + 4 * i, v, 4) for i, v in enumerate(values)]


def orders_finance(gift_ctr):
    """tax and wages at their limits, salary, gifts (with the gifts counter), donations; refusals included"""
    return [
        orders(0, 0x0040A260, orders_city(0x8, 25), p1=0), orders(0, 0x0040A260, p1=1),
        orders(0, 0x0040A260, orders_city(0x8, 0), p1=1), orders(0, 0x0040A260, p1=2), orders(0, 0x0040A260, p1=0),
        orders(0, 0x0040A1D0, orders_city(0x2C08, 100), p1=0), orders(0, 0x0040A1D0, p1=1),
        orders(0, 0x0040A1D0, orders_city(0x2C08, 0), p1=1), orders(0, 0x0040A1D0, p1=0),
        orders(1, 0x0040AF30, orders_city(0x44D4, 3), p1=2), orders(1, 0x0040AF30, orders_city(0x44D4, 0), p1=4),
        orders(1, 0x0040AF30, (0x00E38DDA, 0x2C, 1), p1=10),
        orders(2, 0x0040B140, p1=3), orders(2, 0x0040B140, p1=2), orders(2, 0x0040B140, p1=1),
        orders(2, 0x0040B1C0, orders_city(0x4560, gift_ctr)), orders(2, 0x0040B140, p1=3), orders(2, 0x0040B1C0),
        orders(2, 0x0040B140, p1=2), orders(2, 0x0040B1C0), orders(2, 0x0040B1C0),
        orders(3, 0x0040AFF0, p1=1), orders(3, 0x0040B0B0, p1=0), orders(3, 0x0040B0B0, p1=1), orders(3, 0x0040AF90),
        orders(3, 0x0040AFF0, p1=4), orders(3, 0x0040B0B0, p1=0), orders(3, 0x0040AF90),
        orders(3, 0x0040AFF0, p1=0), orders(3, 0x0040B0B0, p1=1), orders(3, 0x0040AF90),
    ]


def orders_labour(cats):
    """priorities of the labour categories: (category, priority) in order"""
    return [orders(i // 4, 0x0040A730, (0x00605988, c, 4), p1=p) for i, (c, p) in enumerate(cats)]


def orders_trade(route, imports, exports, others):
    """open a route (yes, then no), import / export / status cycles, limits, stockpile, industry"""
    lines = [orders(0, 0x004022D0, (0x00D3B439, 1, 1), (0x00784F18, route, 4)),
             orders(0, 0x004022D0, (0x00D3B439, 0, 1), (0x00784F18, route + 1, 4))]
    for r in imports:
        lines += [orders(1, 0x00559A60, (0x0060598C, r, 4))] * 4
    for r in exports:
        lines += [orders(2, 0x00559B50, (0x0060598C, r, 4))] * 4
    for r in others:
        res = (0x0060598C, r, 4)
        lines += [orders(3, 0x005598F0, res)] * 4 + [orders(3, 0x00559CD0, res, p1=0), orders(3, 0x00559CD0, res, p1=0),
                                                    orders(3, 0x00559CD0, res, p1=1), orders(3, 0x00559CD0, res, p1=2)]
        lines += [orders(4, 0x00559D70, res, (ORDERS_CITY + 0x2826 + 2 * r, 2, 2)), orders(4, 0x00559D70, res),
                  orders(4, 0x00559D70, res, (ORDERS_CITY + 0x2826 + 2 * r, 4, 2)), orders(4, 0x00559D70, res),
                  orders(4, 0x00559D70, res, (ORDERS_CITY + 0x2826 + 2 * r, 1, 2)), orders(4, 0x00559D70, res),
                  orders(4, 0x005598A0, res), orders(4, 0x005598A0, res)]
    lines += [orders(5, 0x00559CD0, (0x0060598C, r, 4), p1=1) for r in imports + exports for _ in range(3)]
    lines += [orders(5, 0x005598A0, (0x0060598C, r, 4)) for r in range(1, 36)]
    return lines


def orders_storage(granary, bazaar, yard, rows=(1, 2, 3)):
    """storage orders: cycles, fill limits (both ways, both limits), all, special, toggle +0xDC"""
    window = 0x00EADD6C
    goods = [1, 2, 3, 4, 5, 6, 7, 8, 0xD, 0xF, 0x11, 0x13]
    lines = []
    for i, b in enumerate((granary, bazaar, yard)):
        if b is None:
            continue
        w = (window, b, 4)
        lists = orders_list(0x00F53DD8, goods[:8]) + orders_list(0x00F53E6C, [9, 1, 0xA, 2, 3, 0x11, 4, 5])
        lines.append(orders(i, 0x00406300, w, *lists, p1=rows[0]))
        lines += [orders(i, 0x00406300, w, p1=r) for r in rows for _ in range(2)]
        lines += [orders(i, 0x00406460, w, p1=0, p2=r) for r in (0, 1) for _ in range(5)]
        lines += [orders(i, 0x00406460, w, p1=1, p2=r) for r in (0, 1, 2) for _ in range(5)]
        lines += [orders(i, 0x00406460, w, p1=2, p2=0), orders(i, 0x00406300, w, p1=1)]
        lines += [orders(i, 0x00406620, w), orders(i, 0x00406680, w, p1=1), orders(i, 0x00406680, w, p1=2),
                  orders(i, 0x00406680, w, p1=3), orders(i, 0x0040C630, w), orders(i, 0x0040C630, w),
                  orders(i, 0x0040C630, w, (0x007999D8 + 0x108 * b + 0xDC, 5, 4))]
    if yard is not None:  # the scroll row of a yard's list (+0xE0): 0, above 14, and the screen modes
        e0 = 0x007999D8 + 0x108 * yard + 0xE0
        lines += [orders(3, 0x00406300, (window, yard, 4), (e0, 20, 4), p1=1),
                  orders(3, 0x00406460, (window, yard, 4), (e0, 0, 4), p1=0, p2=1),
                  orders(3, 0x00406460, (window, yard, 4), (e0, 30, 4), (0x00E38E68, 2, 4), p1=0, p2=1),
                  orders(3, 0x00406460, (window, yard, 4), (e0, 30, 4), (0x00E38E68, 3, 4), p1=0, p2=1),
                  orders(3, 0x00406460, (window, yard, 4), (e0, 12, 4), (0x00E38E68, 1, 4), p1=1, p2=1)]
    return lines


def orders_kingdom(events, troops=None):
    """requests: confirmed (yes / no), dispatched directly, out of range; troops: (event, army, formations)"""
    lines = []
    for i, e in enumerate(events):
        lines += [orders(i, 0x00402340, (0x00D3B439, 0, 1), (0x006057E4, e, 4)),
                  orders(i, 0x00402340, (0x00D3B439, 1, 1), (0x006057E4, e, 4))]
    lines += [orders(0, 0x0044C1A0, args=':-1'), orders(0, 0x0044C1A0, args=':400')]
    if troops:
        for i, (e, army, forms) in enumerate(troops):
            r = 0x00785D74 + 0x7C * e
            lines.append(orders(len(events) + i, 0x0044C1A0, (r + 4, 0x25, 2), (r + 0x1C, army, 2),
                                *[(0x00BCB9C8 + 0x90 * f + 0x67, 1, 1) for f in forms], args=':%d' % e))
    return lines


def orders_messages():
    """open, delete one, delete the read ones, rows past the list"""
    return [orders(0, 0x004E21D0, p1=0), orders(0, 0x004E21D0, p1=3), orders(0, 0x004E2300, p1=2),
            orders(0, 0x004E2300, (0x00E95120, 1, 4), (0x00E9511C, 1, 4), p1=1), orders(1, 0x004E21D0, p1=999),
            orders(1, 0x004E2300, p1=999), orders(1, 0x004E2290, (0x00E9511C, 5, 4)), orders(2, 0x004E2290)]


def orders_replies(event, msg):
    """the replies to a kind-0x17 event, made active (+0x32) and matched by message `msg` (+0x18 / +0x1A)"""
    r = 0x00785D74 + 0x7C * event
    m = 0x00E95228 + 0x30 * msg
    sets = [(r + 2, 0x17, 1), (r + 4, 10, 2), (r + 0xC, 7, 2), (r + 0x32, 1, 1), (m + 0x18, 7, 2), (m + 0x1A, 10, 2),
            (0x00E951E4, msg, 4)]
    def line(t, addr, allowed, status):
        return orders(t, addr, *sets, (0x00D3B186, allowed, 1), (r + 0x30, status, 2))
    return [orders(0, 0x004E21D0, *sets, p1=msg),
            line(1, 0x0044EEA0, 6, 0), line(1, 0x0044EEA0, 7, 0), line(2, 0x0044EEF0, 5, 1),
            line(2, 0x0044EEF0, 7, 1), line(3, 0x0044EEF0, 7, 3), line(4, 0x0044EEF0, 7, 5),
            line(5, 0x0044EE30, 3, 0), line(5, 0x0044EE30, 4, 0),
            orders(6, 0x0044EEA0, (0x00D3B186, 7, 1), (r + 0x32, 0, 1), (0x00E951E4, msg, 4))]


def orders_settings():
    """difficulty (limits, setup window), lower after a defeat, keep / stop governing, message options, rotation"""
    return [orders(0, 0x00402E50, p1=0), orders(0, 0x00402E50, p1=1), orders(0, 0x00402E50, (0x00E39080, 4, 4), p1=0),
            orders(0, 0x00402E50, (0x00E39080, 0, 4), p1=1), orders(0, 0x00402E50, (0x00E38DDA, 0x35, 1), p1=0),
            orders(1, 0x004EA840, orders_city(0x47A8, 4)), orders(1, 0x004EA840, orders_city(0x47A8, 0)),
            orders(1, 0x004EA840, orders_city(0x47A8, 2)),
            orders(2, 0x004EA770, p1=1), orders(2, 0x004EA770, p1=2), orders(2, 0x004EA770, p1=3),
            orders(3, 0x00402CC0, p1=1), orders(3, 0x00402CC0, p1=5), orders(3, 0x00402CC0, p1=12),
            orders(3, 0x00402CC0, p1=0), orders(3, 0x00402CC0, p1=14), orders(3, 0x00402CC0, p1=13),
            orders(4, 0x00409EB0, p1=2), orders(5, 0x00409EB0, p1=2), orders(6, 0x00409EB0, p1=1),
            orders(7, 0x00409EB0, p1=0), orders(8, 0x00409EB0, p1=1), orders(9, 0x00409EB0, p1=0),
            orders(10, 0x00409EB0, p1=2), orders(10, 0x00409EB0, p1=2), orders(11, 0x00409EB0, p1=0),
            orders(12, 0x00409EB0, p1=0), orders(12, 0x00409EB0, p1=3),
            orders(13, 0x004EA7F0)]


def orders_cheats(x, y, skip=()):
    """every cheat (index 5, mockattack2, reaches undefined behaviour in 0x00446A30: skipped), out of range"""
    lines = [orders(i // 6, 0x0041D010, args=':%d' % i) for i in range(34) if i not in (3, 5) + tuple(skip)]
    lines += [orders(6, 0x0041D010, (0x00E38DDA, 1, 1), args=':3') + ' mouse=%d,%d' % (x, y),
              orders(6, 0x0041D010, (0x00E38DDA, 9, 1), args=':3'), orders(6, 0x0041D010, args=':-1'),
              orders(6, 0x0041D010, args=':34')]
    return lines


def orders_festival(god_ok, god_bad):
    """god (available or not), size (refusals: treasury, +0x4514, +0x47A0 / +0x47A4), holding (months counters)"""
    return [orders(0, 0x0040AC50, p1=god_bad), orders(0, 0x0040AC50, p1=god_ok),
            orders(0, 0x0040AC80, orders_city(0x4514, 1), orders_city(0x47A0, 10), orders_city(0x47A4, 10), p1=3),
            orders(0, 0x0040AC80, orders_city(0x4514, 0), p1=3), orders(0, 0x0040AC80, orders_city(0x47A0, -1), p1=2),
            orders(0, 0x0040AC80, orders_city(0x47A0, 10), orders_city(0x47A4, -1), p1=2),
            orders(0, 0x0040AC80, orders_city(0x47A4, 10), orders_city(0xC, -5000), p1=1),
            orders(1, 0x0040AB00, p1=1, p2=0), orders(1, 0x0040AB00, orders_city(0xC, -4999), p1=1, p2=1),
            orders(2, 0x0040AC80, orders_city(0xC, 60000), p1=3), orders(2, 0x0040AB00, p1=1, p2=0),
            orders(3, 0x0040AC80, p1=2), orders(3, 0x0040AB00, orders_city(0x4528, 2), orders_city(0x452C, 0), p1=1),
            orders(4, 0x0040AC80, p1=1), orders(4, 0x0040AB00, p1=1, p2=1)]


def orders_grave_goods():
    """AlexC: rows 0 / 4 (nothing left to send), 2 (resource 0x12), 9 (no row); amounts; send; cancel; stock refusal"""
    stock = orders_city(0x48F0 + 4 * 0x12, 50)
    return [orders(0, 0x0040CD70, p1=0), orders(0, 0x0040CD70, p1=4), orders(0, 0x0040CD70, p1=9),
            orders(1, 0x0040CD70, p1=2), orders(1, 0x0040B420), orders(1, 0x0040B420), orders(1, 0x0040B420),
            orders(1, 0x0040B4B0, (0x00E38E03, 1, 1)), orders(1, 0x0040B4B0, (0x00E38E03, 0, 1)),
            orders(1, 0x0040B3B0), orders(1, 0x0040B420), orders(1, 0x0040B1F0),
            orders(2, 0x0040CD70, p1=2), orders(2, 0x0040B420), orders(2, 0x0040B380),
            orders(3, 0x0040CD70, p1=2), orders(3, 0x0040B4B0, (0x00E38E03, 1, 1)), orders(3, 0x0040B1F0),
            orders(4, 0x0040CD70, stock, p1=2), orders(4, 0x0040B3B0), orders(4, 0x0040B4B0, (0x00E38E03, 1, 1)),
            orders(4, 0x0040B1F0)]


# ---- package place: the place command 0x004728A0 and its per-type work (docs/commands.md 2.2, 2.4) ----
PLACE_SAIS = 'local/saves/downloaded/1582/sais 6374.sav'
PLACE_ITJTAWY = 'local/saves/downloaded/1562/CaesarAlan-Itjtawy.sav'
PLACE_FIGURE = 0x00BCD5F8 + 0x184 * 1999   # a free figure record (all test saves), a figure in the way
PLACE_CITY = 0x00EA1870 + 0x49D8           # current player 1 (all test saves)


def place_figure(lines, x, y, ftype=0x2B):
    """lines with a figure of type ftype (0x2B..0x39, 0x63: 0x004B5830) at (x, y) during the last line
    (the mouse up), removed after it (0x0051F780 is empty)"""
    lines = list(lines)
    tick = lines[-1].split()[0]
    lines[-1] += ' %X=01 %X=%02x %X=%s' % (PLACE_FIGURE + 0xE, PLACE_FIGURE + 0xA, ftype, PLACE_FIGURE + 0x14,
                                           (x & 0xFFFF).to_bytes(2, 'little').hex() + (y & 0xFFFF).to_bytes(2, 'little').hex())
    return lines + ['%s 0x0051F780 %X=00' % (tick, PLACE_FIGURE + 0xE)]


def place_monument(tick, item, x, y, sets=''):
    """the monuments menu (0x1C) entry `item` of the scenario list placed at (x, y)"""
    lines = ['%d 0x%08X %X=%s %X=%s %s' % (tick, SET_TYPE, SUBMENU, h32(0x1C), ITEM, h32(item), sets)]
    return lines + build(tick, 0x5C, [(x, y), (x, y)])[1:]


def place_bridge(lines, x, y, length, direction, step, start, valid):
    """lines with the bridge measure of the city-view draw code 0x0047F200 (an input, as the ferry's
    0x00BCA314) set on the mouse up: valid 0x00BC96FC, length 0x00BC96F8, direction 0x00BC96F4, step
    0x00BC9700, start 0x00BC9704 (values from the original's 0x0047F200 at (x, y))"""
    return ui_with(lines, -1, 'BC96FC=%s BC96F8=%s BC96F4=%s BC9700=%s BC9704=%s' % (
        h32(valid), h32(length), h32(direction), h32(step), h32(start)))


# ---- package mapadd ----
MAPADD_RAW = [0x50, 0x54, 0x55, 0x58, 0x59, 0x5D, 0x91, 0x92, 0x93, 0x94, 0x95, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0xAB, 0xB9,
              0xBA, 0xBE, 0xBF, 0xC0, 0xC1, 0xD7, 0xDA, 0xDB, 0xDC]  # types no build menu offers


def mapadd_raw(tick, btype, spot):
    """btype placed on spot: a bazaar chosen (no placement flag), then 0x006059E8 := btype"""
    x, y = spot
    return build(tick, 0x46, [spot])[:1] + [
        '%d 0x%08X %X=%s hover=%d,%d mouse=%d,%d' % (tick, MOUSE_DOWN, TYPE, h32(btype), x, y, x, y),
        '%d 0x%08X hover=%d,%d mouse=%d,%d' % (tick, MOUSE_UP, x, y, x, y)]


# ---- package cmdrest: build-menu availability, hover functions (docs/commands.md 2.1) ----
CMDREST_SUBMENUS = [(4, 1), (10, 1), (10, 11), (6, 1), (6, 2), (6, 3), (6, 4), (9, 13), (9, 14), (11, 1), (11, 3)]


def cmdrest_menus(tick, entries=CMDREST_SUBMENUS):
    """choose build-menu entries (submenu, item); the submenu openers count their submenu (0x00404120)"""
    return ['%d 0x%08X %X=%s %X=%s' % (tick, SET_TYPE, SUBMENU, h32(s), ITEM, h32(i)) for s, i in entries]


def cmdrest_hover(lines, call, sets=''):
    """lines with the hover function `call` ('0xADDR:x,y,arg', core/command/hover.c) run for the
    hovered tile before the mouse up, as the original's city-view draw does every frame; `sets`
    appended to the mouse up (the ferry validity 0x00BCA314 of the ferry draw 0x0043D300, which
    has no single original function to call: its value here is the one the original's 0x0043D300
    computes at that tile, checked by tests/differential/oracle_hover_check.py)"""
    lines = list(lines)
    tick = lines[-1].split()[0]
    x, y = (int(v) for v in call.split(':')[1].split(',')[:2])
    out = lines[:-1] + ['%s %s hover=%d,%d' % (tick, call, x, y)] + lines[-1:]
    if sets:
        out[-1] += ' ' + sets
    return out


def cmdrest_ferry(tick, first, second, refused):
    """a ferry: first landing (its reach 0x0047F3E0), the second one joined to it (drag preview),
    then a landing the draw finds invalid (notice 0x44)"""
    (x1, y1), (x2, y2), (x3, y3) = first, second, refused
    return (cmdrest_hover(build(tick, 0x88, [first, first]), '0x0047F3E0:%d,%d,0' % (x1, y1), 'BCA314=01') +
            ui_with(build(tick + 1, 0x88, [second, second])[1:], -1, 'BCA314=01') +
            cmdrest_hover(build(tick + 2, 0x88, [refused, refused]), '0x0047F3E0:%d,%d,0' % (x3, y3), 'BCA314=00'))


def cmdrest_bridge(tick, btype, x, y):
    """a bridge at (x, y) with the draw's bridge measure 0x0047F200 (third argument: 0 for 0x52)"""
    lines = build(tick, btype, [(x, y)]) if btype == 0x52 else ui_raw(tick, btype, [(x, y)])
    return cmdrest_hover(lines, '0x0047F200:%d,%d,%d' % (x, y, 0 if btype == 0x52 else 1))


# name: (save, ticks, command lines)
MIL_SAIS = 'local/saves/downloaded/1582/sais 6374.sav'

TESTS = {
    # package place: 0x004728A0 through the construction UI (docs/commands.md 2.2, 2.4)
    'place well actium': (ACTIUM, 3, build(0, 0x5C, [(112, 60)])),
    'place well no water actium': (ACTIUM, 2, build(0, 0x5C, [(118, 68)]) + build(1, 0x5C, [(73, 93)])),
    'place bazaar storage yard actium': (ACTIUM, 3, build(0, 0x46, [(73, 93)]) + build(1, 0x48, [(63, 83)])),
    'place fort actium': (ACTIUM, 3, build(0, 0x28, [(63, 83)])),
    'place gatehouse actium': (ACTIUM, 3, build(0, 0xCA, [(58, 18)], rotation=0) + build(1, 0xCA, [(58, 26)], rotation=1)),
    'place one per city actium': (ACTIUM, 3, build(0, 0x5E, [(63, 83)]) + build(1, 0x5F, [(73, 93)]) +
                                  build(1, 0x56, [(73, 93)]) + build(2, 0x35, [(63, 90)])),
    'place refusals actium': (ACTIUM, 3, build(0, 0xD3, [(3, 53)]) + build(0, 0x1F, [(3, 53)]) + build(0, 0xD1, [(3, 53)]) +
                              build(1, 7, [(3, 53)]) + build(1, 0x88, [(3, 53)]) + build(1, 0x4B, [(3, 53)]) +
                              build(2, 0x46, [(78, 98)]) + build(2, 0x46, [(3, 53)]) + build(2, 0x41, [(3, 53)])),
    'place out of map actium': (ACTIUM, 2, build(0, 0x46, [(139, 139)]) + build(0, 0x48, [(137, 5)]) +
                                build(1, 10, line(130, 130, 145, 145)) + build(1, 5, line(135, 70, 150, 70))),
    'place treasury actium': (ACTIUM, 2, ui_with(build(0, 0x46, [(73, 93)]), 0, '%X=%s' % (CITY_TREASURY, h32(-5000))) +
                              ui_with(build(1, 5, line(97, 70, 105, 70)), 0, '%X=%s' % (CITY_TREASURY, h32(-4999)))),
    'place figures in the way actium': (ACTIUM, 3, place_figure(build(0, 5, line(97, 70, 105, 70)), 100, 75) +
                                        place_figure(build(1, 0x46, [(73, 93)]), 80, 100, 0x63) +
                                        place_figure(build(1, 0x27, line(97, 70, 100, 72)), 97, 60, 0x39) +
                                        place_figure(build(2, 6, line(106, 70, 112, 71)), 112, 80) +
                                        place_figure(build(2, 9, line(97, 69, 99, 71)), 98, 70)),
    'place drags actium': (ACTIUM, 4, build(0, 5, line(97, 70, 105, 70)) + build(0, 5, [(97, 72), (100, 75)]) +
                           build(1, 10, line(97, 74, 99, 76)) + build(1, 6, line(106, 70, 112, 71)) +
                           build(2, 8, line(90, 72, 96, 72)) + build(2, 0x26, line(80, 64, 90, 64)) +
                           build(3, 0x27, line(100, 76, 103, 78)) + build(3, 9, line(97, 69, 99, 71))),
    'place wall kinds actium': (ACTIUM, 3, build(0, 6, line(106, 70, 112, 70)) + build(1, 0xA8, line(106, 70, 112, 70)) +
                                build(2, 0xA9, line(106, 70, 112, 70)) + build(2, 0xA8, line(106, 70, 112, 70))),
    'place venues actium': (ACTIUM, 3, build(0, 0x1F, [(111, 60)])),
    'place bandstand actium': (ACTIUM, 3, build(0, 0x1E, [(111, 60)])),
    'place pavilion actium': (ACTIUM, 3, build(0, 0x21, [(111, 59)])),
    'place view orientation actium': (ACTIUM, 3, ui_with(build(0, 0x48, [(63, 83)]), 0, 'BCA2E0=02000000') +
                                      ui_with(build(1, 0x46, [(73, 93)]), 0, 'BCA2E0=04000000') +
                                      ui_with(build(2, 0x47, [(66, 95)]), 0, 'BCA2E0=06000000')),
    'place venues iken': (IKEN, 3, build(0, 0x1F, [(71, 43)]) + build(1, 0x1E, [(193, 108)])),
    'place bandstand iken': (IKEN, 3, build(0, 0x1E, [(71, 43)])),
    'place water lift iken': (IKEN, 3, build(0, 7, [(158, 138)])),
    'place shipwright iken': (IKEN, 3, build(0, 0x4A, [(153, 98)])),
    'place dock iken': (IKEN, 3, build(0, 0x4B, [(153, 98)])),
    'place fishing wharf iken': (IKEN, 3, build(0, 0x4C, [(153, 98)]) + build(1, 0xB6, [(153, 98)])),
    'place warship wharf iken': (IKEN, 3, build(0, 0xB5, [(153, 98)]) + build(1, 0xB6, [(153, 108)])),
    'place refusals iken': (IKEN, 3, build(0, 0x28, [(143, 173)]) + build(0, 0x41, [(143, 173)]) + build(0, 0x3B, [(143, 173)]) +
                            build(1, 0xA1, [(143, 173)]) + build(1, 0x8A, [(113, 43)]) + build(1, 0x8A, [(123, 213)]) +
                            build(2, 0xD3, [(113, 43)]) + build(2, 0xD1, [(113, 43)]) + build(2, 0x64, [(143, 173)])),
    'place goods 0x35 iken': (IKEN, 3, build(0, 0x35, [(143, 173)])),
    'place farms iken': (IKEN, 3, build(0, 0x64, [(133, 148)]) + build(1, 0x6A, [(143, 188)]) + build(1, 0x6A, [(143, 173)]) +
                         build(2, 0x6D, [(143, 173)]) + build(2, 0x6E, [(143, 173)])),
    'place 0x6d iken': (IKEN, 3, build(0, 0x6D, [(143, 188)]) + build(1, 0x6B, [(143, 173)])),
    'place 0x8a iken': (IKEN, 3, build(0, 0x8A, [(93, 48)]) + build(1, 0xA7, [(143, 173)]) + build(2, 0x47, [(143, 188)])),
    # the obelisk's goods path is not tested: the original then loads "Obelisk5x5a", which stops on a message box
    # (graphic file out of sync) in the oracle
    'place obelisk iken': (IKEN, 3, place_monument(0, 1, 55, 107) + place_monument(1, 1, 3, 143)),
    'place shipwright no road alexc': (ALEXC, 3, build(0, 0x4A, [(138, 148)]) + build(0, 0xD3, [(103, 108)]) +
                                       build(1, 0xD4, [(113, 43)]) + build(1, 0x35, [(78, 88)])),
    'place well water lift alexc': (ALEXC, 3, build(0, 0x5C, [(78, 88)]) + build(1, 7, [(33, 138)]) +
                                    build(2, 0xB5, [(33, 140)])),
    'place monument e3 alexc': (ALEXC, 3, place_monument(0, 1, 183, 111) + place_monument(1, 1, 3, 143)),
    'place monument de alexc': (ALEXC, 3, place_monument(0, 2, 51, 131) +
                                place_monument(1, 2, 51, 131, '%X=%s' % (PLACE_CITY + 0x4968, h32(24000)))),
    'place complex add-on sais': (PLACE_SAIS, 3, build(0, 0xD3, [(68, 63)])),
    'place complex add-on 0xd4 sais': (PLACE_SAIS, 3, build(0, 0xD4, [(68, 63)])),
    'place venues sais': (PLACE_SAIS, 3, build(0, 0x1F, [(94, 68)]) + build(1, 7, [(33, 68)]) + build(2, 0x28, [(108, 58)])),
    'place bandstand sais': (PLACE_SAIS, 3, build(0, 0x1E, [(93, 68)])),
    'place pavilion sais': (PLACE_SAIS, 3, build(0, 0x21, [(92, 67)])),
    'place monument de sais': (PLACE_SAIS, 3, place_monument(0, 1, 71, 139) +
                               place_monument(1, 1, 71, 139, '%X=%s' % (PLACE_CITY + 0x4968, h32(24000)))),
    'place temple complex m00': (M00, 3, build(0, 0x41, [(23, 58)])),
    'place temple complex 0x45 m00': (M00, 3, build(0, 0x45, [(23, 58)], rotation=1)),
    'place palace mansion m00': (M00, 3, build(0, 0xBB, [(33, 43)]) + build(1, 0x4D, [(58, 58)]) + build(2, 0x8A, [(23, 38)])),
    'place one per city m00': (M00, 3, build(0, 0x5F, [(38, 58)]) + build(1, 0x5E, [(58, 58)]) + build(2, 0x5F, [(28, 33)])),
    'place advice e2 m00': (M00, 2, build(0, 0xE2, [(33, 43)])),
    'place advice 0x20 m00': (M00, 2, build(0, 0x20, [(38, 58)])),
    'place advice 0x2f 0x33 m00': (M00, 2, build(0, 0x2F, [(38, 58)]) + build(1, 0x33, [(58, 58)])),
    'place advice cc e8 m00': (M00, 2, build(0, 0xCC, [(38, 58)]) + build(1, 0xE8, [(58, 58)])),
    'place advice e7 72 6e m00': (M00, 2, build(0, 0xE7, [(38, 58)]) + build(1, 0x72, [(58, 58)]) + build(1, 0x6E, [(28, 33)])),
    'place farm special point m00': (M00, 2, build(0, 0x64, [(28, 33)]) + build(1, 0x46, [(43, 58)]) +
                                     build(1, 5, line(40, 58, 46, 58))),
    'place bridges iken': (IKEN, 3, place_bridge(build(0, 0x52, [(149, 39)]), 149, 39, 5, 4, 228, 9270, 10182) +
                           place_bridge(ui_raw(1, 0x53, [(142, 46)]), 142, 46, 5, 2, 1, 10859, 10863) +
                           place_bridge(ui_raw(1, 0x53, [(140, 30)]), 140, 30, 3, 4, 228, 7209, 7665) +
                           place_bridge(build(2, 0x52, [(141, 33)]), 141, 33, 2, 2, 1, 7894, 0) +
                           place_figure(place_bridge(build(2, 0x52, [(128, 88)]), 128, 88, 5, 2, 1, 20421, 20425), 130, 90)),
    'place bridges alexc': (ALEXC, 3, place_bridge(ui_raw(0, 0x53, [(127, 50)]), 127, 50, 5, 4, 228, 11756, 12668) +
                            place_bridge(build(1, 0x52, [(146, 44)]), 146, 44, 4, 0, -228, 10407, 9723)),
    'place pyramid itjtawy': (PLACE_ITJTAWY, 2, place_monument(0, 2, 163, 103) + place_monument(1, 3, 175, 71)),
    'place sphinx itjtawy': (PLACE_ITJTAWY, 2, place_monument(0, 1, 163, 103) + place_monument(1, 1, 3, 143)),
    # package ui: the construction UI handlers (0x00405200, 0x00405C20, 0x00405D90, 0x00405E90, 0x00405BA0,
    # 0x00406010) and the drag preview 0x0046FEF0 (docs/commands.md 2.1, 2.3, 2.6)
    'ui choose menus actium': (ACTIUM, 2, ui_choose_menus(0, range(0x30), [0, 1, 2, 3, 4, 9, 13, 17, 30])),
    'ui choose monuments alexc': (ALEXC, 3, ui_choose_menus(0, [0x1C], range(0, 5)) + [
        '1 0x%08X %X=%s %X=%s 784DE0=%s' % (SET_TYPE, SUBMENU, h32(0x1C), ITEM, h32(1), (r).to_bytes(2, 'little').hex())
        for r in range(1, 38)]),
    'ui build monuments alexc': (ALEXC, 8, [
        '%d 0x%08X %X=%s %X=%s 784DE0=%s' % (i, SET_TYPE, SUBMENU, h32(0x1C), ITEM, h32(1), r.to_bytes(2, 'little').hex())
        for i, r in enumerate((24, 30, 37))] + [l for i, (x, y) in enumerate(((170, 82), (34, 90), (44, 86)))
                                                for l in build(i, 0x5C, [(x, y), (x, y)])[1:]]),
    'ui types 1 iken': (IKEN, 9, ui_each_type(ui_menu_types()[0:36], IKEN_SPOTS[0:36])),
    'ui types 2 iken': (IKEN, 9, ui_each_type(ui_menu_types()[36:72], IKEN_SPOTS[36:72])),
    'ui types 3 iken': (IKEN, 9, ui_each_type(ui_menu_types()[72:108], IKEN_SPOTS[72:108])),
    'ui types 4 iken': (IKEN, 9, ui_each_type(ui_menu_types()[108:], IKEN_SPOTS[108:])),
    'ui types alexc': (ALEXC, 30, ui_each_type(ui_menu_types()[::2][:len(ALEXC_SPOTS)], ALEXC_SPOTS, per_tick=3)),
    'ui raw types alexc': (ALEXC, 16, [l for i, t in enumerate(UI_RAW_TYPES)
                                      for l in ui_raw(i // 2, t, [ALEXC_SPOTS[i], ALEXC_SPOTS[i]])]),
    'ui roads actium': (ACTIUM, 12, build(0, 5, [(95, 68), (100, 68), (104, 68), (104, 72), (100, 72), (104, 72),
                                                 (104, 75)]) +
                        build(2, 5, [(80, 68), (86, 68), (86, 74), (80, 74), (80, 70)]) +
                        build(4, 5, [(58, 47), (58, 52), (63, 52), (58, 52)])),
    'ui walls iken': (IKEN, 10, build(0, 6, [(50, 96), (56, 96), (56, 100), (52, 100), (56, 100)]) +
                      build(2, 0xA8, [(58, 104), (63, 104), (63, 109)]) +
                      build(4, 0xA9, [(42, 112), (47, 112), (42, 112), (42, 117)]) +
                      build(6, 6, [(51, 97), (55, 97)])),
    'ui canals iken': (IKEN, 12, build(0, 8, [(86, 92), (90, 92), (90, 97), (86, 97), (90, 97), (90, 99)]) +
                       build(2, 8, [(96, 26), (96, 30), (100, 30)]) +
                       build(4, 7, [(96, 32), (97, 32)]) +
                       build(6, 8, [(66, 100), (70, 100)])),
    'ui canals alexc': (ALEXC, 8, build(0, 8, [(170, 98), (175, 98), (175, 104), (170, 104)]) +
                        build(2, 7, [(178, 106), (178, 108)])),
    'ui clear actium': (ACTIUM, 10, build(0, 9, [(58, 47), (61, 47), (61, 50), (58, 50), (63, 52)]) +
                        build(2, 9, [(95, 68), (90, 72), (99, 70)]) +
                        build(4, 9, [(30, 30), (30, 30)])),
    'ui plaza gardens houses actium': (ACTIUM, 12, build(0, 0x26, [(58, 47), (63, 47), (63, 52)]) +
                                       build(2, 0x27, [(98, 68), (102, 68), (102, 71), (98, 68)]) +
                                       build(4, 10, [(110, 47), (113, 50), (115, 52), (110, 47)]) +
                                       build(6, 10, [(74, 74), (78, 77)])),
    'ui houses m00': (M00, 12, build(0, 10, [(29, 41), (33, 45), (31, 44)]) + build(2, 5, [(28, 47), (40, 47), (40, 55)]) +
                      build(4, 0x5C, [(44, 44)]) + build(5, 0x46, [(44, 50), (44, 50)]) +
                      build(6, 0x27, [(35, 53), (38, 56)])),
    'ui cancel mid-drag actium': (ACTIUM, 6, build(0, 5, [(95, 68), (104, 68)])[:3] +
                                  ['0 0x%08X' % CANCEL, '0 0x%08X hover=104,70 mouse=104,70' % DRAG,
                                   '0 0x%08X hover=104,70 mouse=104,70' % MOUSE_UP, '1 0x%08X' % CANCEL] +
                                  build(2, 10, [(74, 74), (78, 77)])[:3] + ['2 0x%08X' % CANCEL]),
    'ui mouse up off map actium': (ACTIUM, 6, ui_with(build(0, 5, [(95, 68), (104, 68)]), -1, 'mouse=-1,-1') +
                                   ui_with(build(1, 0x5C, [(98, 70)]), -1, 'mouse=-1,-1') +
                                   ui_with(build(2, 10, [(74, 74), (77, 76)]), -1, 'hover=-1,-1 mouse=-1,-1') +
                                   ui_with(build(3, 0x5C, [(98, 72), (98, 72)]), -1, 'mouse=-1,-1')),
    'ui ferry iken': (IKEN, 8, ui_ferry(0, 96, 25) + ui_ferry_next(1, 96, 27)),
    'ui ferry choose while half placed iken': (IKEN, 6, ui_ferry(0, 96, 25) + build(1, 0x5C, [(64, 52)])),
    # landings where the original's draw computes 0x00BCA314 = 1 (tests/differential/oracle_hover_check.py)
    'ui ferry cancel iken': (IKEN, 6, ui_ferry(0, 96, 25) + ['1 0x%08X' % CANCEL] + ui_ferry(2, 121, 26)),
    'ui ferry link preview iken': (IKEN, 8, ui_ferry(0, 96, 25) +
                                   ui_with(ui_ferry_next(1, 96, 27, [(96, 38), (97, 39)]), 0, '799BA4=ffffffff BC96E4=06000000')),
    'ui ferry select existing iken': (IKEN, 6, ui_with(build(0, 0x88, [(96, 38), (96, 38)]), 1, '799BA4=ffffffff') +
                                      build(1, 0x5C, [(64, 52)])),
    'ui rotations alexc': (ALEXC, 12, [l for i, (t, r) in enumerate(((0x8C, 0), (0x8D, 1), (0x8E, 2), (0x8F, 3),
                                                                     (0x41, 0), (0x42, 1), (0xD3, 2), (0xC8, 1)))
                                       for l in build(i, t, [ALEXC_SPOTS[20 + i], ALEXC_SPOTS[20 + i]], r)] +
                           ui_raw(9, 0xAB, [ALEXC_SPOTS[30]] * 2, ' %X=%s' % (ROTATION, h32(1))) +
                           ui_raw(10, 0xCA, [ALEXC_SPOTS[31]] * 2, ' %X=%s' % (ROTATION, h32(0)))),
    'ui view orientation alexc': (ALEXC, 8, [l for i, (t, o) in enumerate(((0x5C, 2), (0x47, 4), (0x41, 6), (0x88, 2),
                                                                            (0x28, 4), (0x2D, 6)))
                                             for l in ui_with(build(i, t, [ALEXC_SPOTS[40 + i]] * 2), 1,
                                                              'BCA2E0=%s' % h32(o))]),
    'ui refusals actium': (ACTIUM, 10, ui_with(build(0, 0x5C, [(98, 68), (98, 68)]), 1, '%X=%s' % (CITY_TREASURY, h32(-6000))) +
                           build(1, 0x5C, [(58, 47), (58, 47)]) +            # on a road
                           build(2, 0x64, [(98, 70), (98, 70)]) +            # a farm off the floodplain
                           build(3, 0x46, [(0, 0), (0, 0)]) +                # at the map's corner
                           build(4, 0x41, [(137, 137), (137, 137)]) +        # a temple complex at the edge
                           build(5, 10, [(135, 135), (139, 139)]) +          # houses up to the edge
                           build(6, 0x6A, [(99, 70), (99, 70)]) + build(6, 0xA1, [(99, 72), (99, 72)]) +
                           build(7, 0x3B, [(100, 70), (100, 70)])),
    'ui undo actium': (ACTIUM, 8, build(0, 5, [(95, 68), (104, 68)]) + ['1 0x%08X' % UNDO] +
                       build(2, 0x5C, [(98, 70)]) + ['3 0x%08X' % UNDO, '4 0x%08X' % UNDO]),
    'ui editor tools actium': (ACTIUM, 6, ui_with(build(0, 5, [(95, 68), (104, 68)]), 0, '605A3C=01000000') +
                               ui_with(build(1, 0x5C, [(98, 70), (99, 70)]), 0, '605A3C=01000000') +
                               ['2 0x%08X %X=%s %X=%s 605A3C=01000000' % (SET_TYPE, SUBMENU, h32(0x10), ITEM, h32(1)),
                                '2 0x%08X hover=98,72 mouse=98,72' % MOUSE_DOWN,
                                '2 0x%08X hover=98,72 mouse=98,72 605A3C=00000000' % MOUSE_UP]),
    # package clear: clearing land 0x004743F0 / 0x00474700, its confirmations 0x004021D0 (monument),
    # 0x00402210 (temple complex), 0x00402250 (fort), 0x00402290 (bridge), answer 0x00D3B439
    # (docs/commands.md 2.5), and undo 0x00485230 (2.6)
    'clear tiles actium': (ACTIUM, 4, build(0, 9, [(99, 69)]) + build(1, 9, [(30, 30)]) + build(2, 9, [(60, 66)])),
    'clear houses roads actium': (ACTIUM, 6, build(0, 9, [(95, 65), (100, 70), (104, 74)])),
    'clear trees water actium': (ACTIUM, 6, build(0, 9, line(30, 30, 39, 39)) + build(2, 9, line(100, 100, 109, 109))),
    'clear industry block actium': (ACTIUM, 40, build(20, 9, line(60, 60, 69, 69))),
    'clear map edges actium': (ACTIUM, 6, build(0, 9, line(3, 3, 0, 0)) + build(1, 9, line(130, 130, 139, 139))),
    'clear fort yes actium': (ACTIUM, 6, build(0, 9, line(104, 74, 110, 80)) + ['0 0x00402250 D3B439=01']),
    'clear fort no actium': (ACTIUM, 6, build(0, 9, line(104, 74, 110, 80)) + ['0 0x00402250 D3B439=00']),
    'clear monument yes iken': (IKEN, 6, build(0, 9, line(120, 85, 125, 90)) + ['0 0x004021D0 D3B439=01']),
    'clear monument no iken': (IKEN, 6, build(0, 9, line(120, 85, 125, 90)) + ['0 0x004021D0 D3B439=00']),
    'clear temple yes iken': (IKEN, 6, build(0, 9, line(82, 46, 90, 58)) + ['0 0x00402210 D3B439=01']),
    'clear temple no iken': (IKEN, 6, build(0, 9, line(82, 46, 90, 58)) + ['0 0x00402210 D3B439=02']),
    'clear fort yes iken': (IKEN, 6, build(0, 9, line(76, 90, 86, 100)) + ['0 0x00402250 D3B439=01']),
    'clear bridge yes iken': (IKEN, 6, build(0, 9, line(168, 70, 173, 72)) + ['0 0x00402290 D3B439=01']),
    'clear bridge no iken': (IKEN, 6, build(0, 9, line(168, 73, 173, 75)) + ['0 0x00402290 D3B439=00']),
    'clear houses iken': (IKEN, 6, build(0, 9, line(60, 50, 70, 60)) + build(1, 9, line(100, 90, 108, 96))),
    'clear monument yes alexc': (ALEXC, 6, build(0, 9, line(135, 135, 150, 145)) + ['0 0x004021D0 D3B439=01']),
    'clear bridge yes alexc': (ALEXC, 6, build(0, 9, line(27, 95, 31, 97)) + ['0 0x00402290 D3B439=01']),
    'clear temple yes alexc': (ALEXC, 6, build(0, 9, line(98, 105, 110, 112)) + ['0 0x00402210 D3B439=01']),
    'clear block alexc': (ALEXC, 30, build(10, 9, line(160, 60, 175, 75))),
    'clear monument and temple sais': ('local/saves/downloaded/1582/sais 6374.sav', 6,
                                       build(0, 9, line(55, 60, 88, 80)) + ['0 0x004021D0 D3B439=01']),
    'clear bridge yes sais': ('local/saves/downloaded/1582/sais 6374.sav', 6,
                              build(0, 9, line(108, 29, 110, 32)) + ['0 0x00402290 D3B439=01']),
    'clear fort no sais': ('local/saves/downloaded/1582/sais 6374.sav', 6,
                           build(0, 9, line(54, 70, 68, 78)) + ['0 0x00402250 D3B439=00']),
    'clear trees m00': (M00, 6, build(0, 9, line(20, 20, 40, 40)) + build(2, 9, [(45, 45)])),
    'clear pyramid yes itjtawy': ('local/saves/downloaded/1562/CaesarAlan-Itjtawy.sav', 4,
                                  build(0, 9, line(114, 194, 122, 202)) + ['0 0x004021D0 D3B439=01']),
    'undo houses roads actium': (ACTIUM, 8, build(0, 10, line(104, 68, 106, 70)) + ['1 0x%08X' % UNDO] +
                                 build(2, 5, line(113, 61, 117, 61)) + ['3 0x%08X' % UNDO] +
                                 build(4, 0x5C, [(110, 66)]) + ['4 0x00485230', '5 0x00485230']),
    'undo after clear actium': (ACTIUM, 6, build(0, 9, line(95, 65, 104, 74)) + ['1 0x%08X' % UNDO] +
                                build(2, 9, line(60, 60, 69, 69)) + ['3 0x00485230 BC968C=01000000']),
    'undo houses iken': (IKEN, 6, build(0, 10, line(218, 110, 221, 113)) + ['1 0x%08X' % UNDO] +
                         build(2, 6, line(194, 86, 198, 86)) + ['2 0x00485230']),
    'undo after clear iken': (IKEN, 6, build(0, 9, line(120, 85, 125, 90)) + ['0 0x004021D0 D3B439=01'] +
                              ['1 0x00485230 BC968C=01000000'] + build(2, 9, line(60, 50, 70, 60)) +
                              ['3 0x00485230 BC968C=01000000']),
    'undo alexc': (ALEXC, 6, build(0, 10, line(167, 68, 170, 71)) + ['1 0x%08X' % UNDO] +
                   build(2, 9, line(135, 135, 150, 145)) + ['2 0x004021D0 D3B439=01', '3 0x00485230 BC968C=01000000']),
    'undo sais': ('local/saves/downloaded/1582/sais 6374.sav', 6,
                  build(0, 10, line(101, 74, 104, 77)) + ['1 0x%08X' % UNDO] +
                  build(2, 9, line(55, 60, 88, 80)) + ['2 0x004021D0 D3B439=01', '3 0x00485230 BC968C=01000000']),
    'undo m00': (M00, 6, build(0, 10, line(47, 62, 50, 65)) + ['1 0x%08X' % UNDO] +
                 build(2, 0x27, line(41, 62, 43, 64)) + ['3 0x00485230']),
    # package orders: settings and orders (docs/commands.md 3.1-3.6, 3.8, 3.9)
    'orders finance actium': (ACTIUM, ORDERS_MONTH, orders_finance(0)),
    'orders finance iken': (IKEN, ORDERS_MONTH, orders_finance(1)),
    'orders finance alexc': (ALEXC, ORDERS_MONTH, orders_finance(2)),
    'orders finance sais': (ORDERS_SAIS, ORDERS_MONTH, orders_finance(3)),
    'orders finance maritis': (ORDERS_MARITIS, ORDERS_MONTH, orders_finance(4)),
    'orders finance m00': (M00, ORDERS_MONTH, orders_finance(-1)),
    'orders labour actium': (ACTIUM, 120, orders_labour([(2, 1), (5, 1), (3, 1), (5, 2), (2, 0), (2, 0), (8, 9), (0, 3),
                                                        (3, 3), (0, 1), (1, 1), (8, 0)])),
    'orders labour iken': (IKEN, 120, orders_labour([(3, 1), (0, 0), (7, 9), (2, 5), (4, 2), (6, 6), (1, 1), (8, 4),
                                                     (5, 0)])),
    'orders labour sais': (ORDERS_SAIS, 60, orders_labour([(1, 0), (1, 1), (0, 1), (4, 1), (1, 3)])),
    'orders labour m00': (M00, 60, orders_labour([(1, 1), (2, 1), (1, 2)])),
    'orders festival actium': (ACTIUM, ORDERS_MONTH, orders_festival(1, 0)),
    'orders festival iken': (IKEN, ORDERS_MONTH, orders_festival(3, 5)),
    'orders festival alexc': (ALEXC, ORDERS_MONTH, orders_festival(2, 3)),
    'orders festival sais': (ORDERS_SAIS, ORDERS_MONTH, orders_festival(1, 2)),
    'orders festival maritis': (ORDERS_MARITIS, ORDERS_MONTH, orders_festival(2, 0)),
    'orders festival m00': (M00, ORDERS_MONTH, orders_festival(0, 1)),
    'orders trade actium': (ACTIUM, ORDERS_MONTH, orders_trade(1, [1, 4], [19, 20], [10, 20])),
    'orders trade iken': (IKEN, ORDERS_MONTH, orders_trade(3, [4, 11], [2, 12], [13, 19])),
    'orders trade alexc': (ALEXC, ORDERS_MONTH, orders_trade(5, [1, 8], [7, 9], [14, 15])),
    'orders trade sais': (ORDERS_SAIS, ORDERS_MONTH, orders_trade(2, [5, 11], [8, 12], [13, 20])),
    'orders trade maritis': (ORDERS_MARITIS, ORDERS_MONTH, orders_trade(1, [2, 10], [17, 22], [19, 23])),
    'orders trade m00': (M00, 120, orders_trade(0, [], [], [1, 5])),
    'orders storage actium': (ACTIUM, 120, orders_storage(127, 45, 129)),
    'orders storage iken': (IKEN, 120, orders_storage(206, 207, 49, rows=(2, 4, 6))),
    'orders storage alexc': (ALEXC, 120, orders_storage(244, 412, 307, rows=(1, 5, 8))),
    'orders storage sais': (ORDERS_SAIS, 120, orders_storage(26, 111, 7)),
    'orders storage maritis': (ORDERS_MARITIS, 120, orders_storage(109, 77, 89, rows=(3, 2, 1))),
    'orders kingdom actium': (ACTIUM, 300, orders_kingdom([0, 8, 12])),
    'orders kingdom iken': (IKEN, 300, orders_kingdom([3, 16, 0], [(4, 1, [1, 2]), (10, 3, [3])])),
    'orders kingdom alexc': (ALEXC, 300, orders_kingdom([0, 4, 10], [(1, 2, [1, 2, 3])])),
    'orders kingdom sais': (ORDERS_SAIS, 300, orders_kingdom([0, 2])),
    'orders kingdom maritis': (ORDERS_MARITIS, 300, orders_kingdom([0, 1], [(2, 1, [1, 2])])),
    'orders grave goods alexc': (ALEXC, 120, orders_grave_goods()),
    'orders messages actium': (ACTIUM, 60, orders_messages()),
    'orders messages iken': (IKEN, 60, orders_messages()),
    'orders messages alexc': (ALEXC, 60, orders_messages()),
    'orders messages m00': (M00, 30, orders_messages()),
    'orders replies actium': (ACTIUM, 60, orders_replies(9, 0)),
    'orders replies iken': (IKEN, 60, orders_replies(1, 2)),
    'orders replies alexc': (ALEXC, 60, orders_replies(6, 5)),
    'orders settings actium': (ACTIUM, 100, orders_settings()),
    'orders settings iken': (IKEN, 100, orders_settings()),
    'orders settings sais': (ORDERS_SAIS, 100, orders_settings()),
    'orders settings m00': (M00, 60, orders_settings()),
    'orders cheats actium': (ACTIUM, 120, orders_cheats(99, 69)),
    'orders cheats iken': (IKEN, 120, orders_cheats(60, 60)),
    'orders cheats alexc': (ALEXC, 120, orders_cheats(100, 100)),
    'orders cheats sais': (ORDERS_SAIS, 120, orders_cheats(80, 80)),
    'orders cheats maritis': (ORDERS_MARITIS, 120, orders_cheats(60, 60)),
    'orders cheats m00': (M00, 60, orders_cheats(40, 40)),
    # package military: legion and ship commands (core/command/military.c, docs/commands.md 3.7)
    # layout buttons p1 1, 2, 3, 0, move, return to the fort
    'military legion layouts actium': (ACTIUM, 400, [
        '0 0x00406720 EADCCC=03000000 D3B198=01000000',
        '1 0x00406720 D3B198=02000000',
        '2 0x00406720 D3B198=03000000',
        '3 0x00406720 D3B198=00000000',
        '3 0x0040C440 mouse=99,69',
        '150 0x00406FB0 EADCCC=03000000 D3B198=04000000',
    ]),
    # water and off-map targets, enemy formation, already home, own fort
    'military legion refusals actium': (ACTIUM, 60, [
        '0 0x00406720 EADCCC=02000000 D3B198=00000000',
        '0 0x0040C440 mouse=98,52',
        '0 0x0040C440 mouse=-1,-1',
        '1 0x0040C440 BCB9C0=0A000000 mouse=99,69',
        '1 0x00406720 EADCCC=0A000000 D3B198=00000000',
        '2 0x00406FB0 EADCCC=02000000 D3B198=04000000',
        '3 0x0040C440 BCB9C0=02000000 mouse=108,79',
    ]),
    'military legion attack iken': (IKEN, 300, [
        '0 0x00406720 EADCCC=01000000 D3B198=00000000',
        '0 0x0040C440 mouse=94,187',
    ]),
    # unreachable enemy, layout 0 charge, toggle +0x77
    'military legion attack alexc': (ALEXC, 400, [
        '0 0x00406720 EADCCC=01000000 D3B198=02000000',
        '0 0x0040C440 mouse=178,152',
        '1 0x00406720 EADCCC=02000000 D3B198=00000000',
        '1 0x0040C570 mouse=170,160',
        '2 0x00407110 EADCCC=03000000 BCB9C0=03000000 D3B194=00000000',
        '2 0x00407110 EADCCC=03000000 D3B194=01000000',
        '2 0x0040C440 mouse=175,150',
    ]),
    # type 0x0D: layout 0 (window 0x49), toggle, window close, recall, return
    'military chariots sais': (MIL_SAIS, 300, [
        '0 0x00406720 EADCCC=01000000 D3B198=01000000',
        '0 0x0040C570 mouse=63,56',
        '10 0x00407110 EADCCC=01000000 D3B194=01000000',
        '20 0x004071C0 EADD50=04000000 EADCC0=00000000',
        '30 0x00406720 EADCCC=01000000 D3B198=00000000',
        '30 0x0040C440 mouse=63,60',
        '60 0x00406720 EADCCC=02000000 D3B198=01000000',
        '60 0x0040C570 mouse=59,75',
        '61 0x0040C570 BCB9C0=02000000 mouse=-1,-1',
        '62 0x00406720 EADCCC=02000000 D3B198=03000000',
        '120 0x00406FB0 EADCCC=01000000 D3B198=04000000',
    ]),
    # advisor rows: return, service (popups), warship return / service, dispatch
    'military advisor iken': (IKEN, 100, [
        '0 0x00406720 EADCCC=02000000 D3B198=00000000',
        '0 0x0040C440 mouse=100,100',
        '40 0x0040A350 D3B198=02000000',
        '41 0x0040A390 D3B198=01000000',
        '42 0x0040A5C0 D3B198=01000000',
        '43 0x0040A600 D3B198=02000000',
        '44 0x0040A350 D3B198=09000000',
        '44 0x0040A390 D3B198=09000000',
        '44 0x0040A5C0 D3B198=09000000',
        '44 0x0040A600 D3B198=09000000',
        '45 0x0040A450 005F8518=01',
    ]),
    # a troop request made running (event 13): service toggle, dispatch
    'military service alexc': (ALEXC, 60, [
        '0 0x0040A390 D3B198=01000000 7863F0=0000 7863F2=01',
        '0 0x0040A390 D3B198=02000000',
        '1 0x0040A390 D3B198=01000000',
        '2 0x0040A450 005F8518=01 EAA8AE=01',
    ]),
    # event 3 made a kind-2 troop request: warship service, dispatch
    'military service iken': (IKEN, 60, [
        '0 0x0040A600 D3B198=01000000 785EEC=2500 785F18=0000 785F1A=01',
        '0 0x0040A390 D3B198=01000000',
        '1 0x0040A600 D3B198=02000000',
        '1 0x0040A450 005F8518=01',
    ]),
    # orders p1 0..3, moves, attack, return
    'military warship iken': (IKEN, 400, [
        '0 0x00406C90 EADCC4=78000000 D3B198=00000000',
        '0 0x0040B830 mouse=170,140',
        '1 0x0040B830 D05704=A5000000 mouse=94,187',
        '50 0x00406C90 D3B198=01000000',
        '50 0x0040B830 mouse=180,150',
        '100 0x00406C90 D3B198=02000000',
        '101 0x0040B830 mouse=177,150',
        '150 0x00406C90 D3B198=03000000',
        '200 0x00406DE0 D3B198=04000000',
        '251 0x0040B830 D05704=CC000000 mouse=-1,-1',
    ]),
    # orders, moves, board a legion, unload (+0x130 set), return
    'military transport iken': (IKEN, 320, [
        '0 0x00406B40 EADCC8=99000000 D3B198=00000000',
        '0 0x0040BA40 mouse=170,140 E390C4=02000000 E390C8=01000000 E390DC=03000000',
        '50 0x00406B40 D3B198=01000000',
        '50 0x0040BA40 mouse=175,140',
        '100 0x00406B40 D3B198=02000000',
        '100 0x0040BA40 mouse=194,121 E390DC=01000000',
        '150 0x00406B40 D3B198=03000000',
        '200 0x00406E80 D3B198=04000000',
        '250 0x00406B40 D3B198=02000000 BDBF0C=10',
        '250 0x0040BA40 mouse=175,122 E390DC=02000000',
        '251 0x00406B40 D3B198=02000000',
        '251 0x0040BA40 mouse=175,145 E390DC=02000000',
        '252 0x00406B40 D3B198=00000000',
        '252 0x0040BA40 mouse=-1,-1',
        '253 0x0040BA40 mouse=172,141 E390DC=03000000',
    ]),
    # city-view selection clicks, figure order selection 0x0053D730
    'military clicks iken': (IKEN, 20, [
        '0 0x0040B630 mouse=82,93 006059E8=00000000',
        '0 0x0040B6D0 mouse=177,144',
        '0 0x0040B790 mouse=168,120',
        '1 0x0040B6D0 mouse=176,143',
        '1 0x0040B790 mouse=167,119',
        '2 0x0040B630 mouse=100,100',
        '2 0x0040B6D0 mouse=100,100',
        '2 0x0040B790 mouse=-1,-1',
        '3 0x0040B630 mouse=82,93 E38DDA=01 006059E8=05000000',
        '3 0x0053D730 EADCE4=00000000 EADD24=2D000000 F55BA4=0003020100000000 BD1AA9=00',
        '4 0x0053D730 F55BA4=0001000000000000 BD1AA9=02',
        '5 0x0053D730 BD1AA9=09',
    ]),
    # no legion, no ship
    'military refusals m00': (M00, 20, [
        '0 0x00406720 EADCCC=01000000 D3B198=00000000',
        '0 0x00406FB0 EADCCC=01000000',
        '0 0x0040C440 BCB9C0=01000000 mouse=40,40',
        '0 0x0040C570 mouse=40,40',
        '0 0x0040A350 D3B198=01000000',
        '0 0x0040A390 D3B198=01000000',
        '0 0x0040A5C0 D3B198=01000000',
        '0 0x0040A600 D3B198=01000000',
        '0 0x0040A450 005F8518=01',
        '0 0x00406B40 EADCC8=00000000 D3B198=00000000',
        '0 0x00406C90 EADCC4=00000000 D3B198=00000000',
        '0 0x00406E80 D3B198=04000000',
        '0 0x00406DE0 D3B198=04000000',
        '0 0x0040BA40 D05700=00000000 mouse=40,40',
        '0 0x0040B830 D05704=00000000 mouse=40,40',
        '0 0x0040B630 mouse=40,40',
        '0 0x0040B6D0 mouse=40,40',
        '0 0x0040B790 mouse=40,40',
    ]),
    # ---- package mapadd: 0x00470990 (core/building/map_add.c) through the place command, for the
    # types no saved building has: menu types on AlexC, the others (0x006059E8 set directly) on Iken
    'mapadd menu types alexc': (ALEXC, 3, ui_each_type([0x25, 0x32, 0x41, 0x43, 0x4D, 0x6A, 0x71, 0xA2, 0xCD, 0xD8, 0xD9,
                                                        0xDD, 0xE0, 0xE7, 0xE8, 0xE9], ALEXC_SPOTS[40:56], per_tick=4)),
    'mapadd raw types iken': (IKEN, 6, [l for i, t in enumerate(MAPADD_RAW) for l in mapadd_raw(i // 6, t, IKEN_SPOTS[i])]),
    'mapadd raw types alexc': (ALEXC, 6, [l for i, t in enumerate(MAPADD_RAW) for l in mapadd_raw(i // 6, t, ALEXC_SPOTS[i])]),
    # ---- package sg3: 0x0053DFF0 (core/graphics/sg3.c) under the slot loader 0x0053DD90, reached by choosing a
    # monument (0x00405200 -> 0x0053D850 slots 5-8) and placing it (0x00470990): successes and refusals
    'sg3 sphinx itjtawy': (PLACE_ITJTAWY, 3, place_monument(0, 1, 163, 103) + place_monument(1, 1, 3, 143)),
    'sg3 pyramid itjtawy': (PLACE_ITJTAWY, 2, place_monument(0, 2, 163, 103) + place_monument(1, 3, 175, 71)),
    'sg3 choose monuments alexc': (ALEXC, 3, ui_choose_menus(0, [0x1C], range(0, 5)) + [
        '1 0x%08X %X=%s %X=%s 784DE0=%s' % (SET_TYPE, SUBMENU, h32(0x1C), ITEM, h32(1), (r).to_bytes(2, 'little').hex())
        for r in range(1, 38)]),
    'sg3 build monuments alexc': (ALEXC, 6, [
        '%d 0x%08X %X=%s %X=%s 784DE0=%s' % (i, SET_TYPE, SUBMENU, h32(0x1C), ITEM, h32(1), r.to_bytes(2, 'little').hex())
        for i, r in enumerate((24, 30, 37, 26))] + [l for i, (x, y) in enumerate(((170, 82), (34, 90), (44, 86), (3, 143)))
                                                    for l in build(i, 0x5C, [(x, y), (x, y)])[1:]]),
    'sg3 monument sais': (PLACE_SAIS, 3, place_monument(0, 1, 71, 139) +
                          place_monument(1, 1, 71, 139, '%X=%s' % (PLACE_CITY + 0x4968, h32(24000)))),
    'sg3 obelisk iken': (IKEN, 3, place_monument(0, 1, 55, 107) + place_monument(1, 1, 3, 143)),
    'sg3 choose monuments m00': (M00, 2, ui_choose_menus(0, [0x1C], range(0, 5))),
    'sg3 choose monuments sais': (PLACE_SAIS, 2, [
        '0 0x%08X %X=%s %X=%s 784DE0=%s' % (SET_TYPE, SUBMENU, h32(0x1C), ITEM, h32(1), (r).to_bytes(2, 'little').hex())
        for r in range(1, 38)]),
    'sg3 choose monuments list m00': (M00, 2, [
        '0 0x%08X %X=%s %X=%s 784DE0=%s' % (SET_TYPE, SUBMENU, h32(0x1C), ITEM, h32(1), (r).to_bytes(2, 'little').hex())
        for r in range(1, 38)]),
    # ---- package cmdrest: submenu openers (0x00404120 under 0x00405200), the availability update
    # (0x00404350 under the trade-route command), with the monuments of Itjtawy (menus 0x16, 0x1C)
    'cmdrest menus actium': (ACTIUM, 2, cmdrest_menus(0) + build(1, 0x66, [(112, 60)])),
    'cmdrest menus iken': (IKEN, 2, cmdrest_menus(0) + cmdrest_menus(1, [(0x16, 2), (0x1C, 1), (0x24, 1)])),
    'cmdrest menus alexc': (ALEXC, 2, cmdrest_menus(0)),
    'cmdrest menus itjtawy': (PLACE_ITJTAWY, 2, cmdrest_menus(0) + cmdrest_menus(1, [(0x16, 1), (0x16, 4), (0x1C, 2)])),
    'cmdrest menus m00': (M00, 2, cmdrest_menus(0)),
    'cmdrest menu update trade iken': (IKEN, 2, [orders(0, 0x004022D0, (0x00D3B439, 1, 1), (0x00784F18, r, 4))
                                                 for r in (1, 2, 3)] + cmdrest_menus(1)),
    # hover functions (core/command/hover.c) before the mouse up: ferries, bridges (same places as
    # the place package's bridge entries, which set the measure by hand)
    'cmdrest ferry iken': (IKEN, 8, cmdrest_ferry(0, (96, 25), (121, 26), (100, 40))),
    'cmdrest ferry alexc': (ALEXC, 8, cmdrest_ferry(0, (101, 55), (70, 55), (100, 40))),
    'cmdrest ferry sais': (ORDERS_SAIS, 8, cmdrest_ferry(0, (43, 81), (32, 80), (44, 81))),
    'cmdrest bridges iken': (IKEN, 3, cmdrest_bridge(0, 0x52, 149, 39) + cmdrest_bridge(1, 0x53, 142, 46) +
                             cmdrest_bridge(1, 0x53, 140, 30) + cmdrest_bridge(2, 0x52, 141, 33) +
                             cmdrest_bridge(2, 0x52, 100, 40)),
    'cmdrest bridges alexc': (ALEXC, 3, cmdrest_bridge(0, 0x53, 127, 50) + cmdrest_bridge(1, 0x52, 146, 44)),
    # open an advisor 0x0040A0E0 (its refresh 0x0040A160 of the advisors' figures); refused: an
    # advisor the mission does not offer (0x006FDC88[advisor] = 0 in a campaign mission, 0x00E38EBC = 0)
    'cmdrest advisors actium': (ACTIUM, 3, [orders(0, 0x0040A0E0, p1=a) for a in range(1, 13)] +
                                [orders(1, 0x0040A0E0, (0x00E38EBC, 0, 4), (0x006FDC88 + 3, 0, 1), (0x00E38DDA, 5, 1), p1=3),
                                 orders(1, 0x0040A0E0, (0x006FDC88 + 4, 1, 1), p1=4)]),
    'cmdrest advisors iken': (IKEN, 2, [orders(0, 0x0040A0E0, p1=a) for a in (1, 4, 7, 12)]),
    'cmdrest advisors m00': (M00, 2, [orders(0, 0x0040A0E0, p1=a) for a in (1, 2, 4, 9)]),
    # difficulty: the model files of the new level read again (0x00540C90, 0x00540ED0, 0x00541160,
    # 0x004BC720: core/command/models.c), then ticks on the new models (costs, figures, tax sentiment)
    'cmdrest difficulty actium': (ACTIUM, 120, [orders(0, 0x00402E50, p1=1), orders(0, 0x00402E50, p1=1),
                                                orders(40, 0x00402E50, p1=0), orders(40, 0x00402E50, p1=0),
                                                orders(40, 0x00402E50, p1=0)] + build(41, 0x5C, [(112, 60)])),
    'cmdrest difficulty iken': (IKEN, 120, [orders(0, 0x00402E50, (0x00E39080, 0, 4), p1=1),
                                            orders(1, 0x00402E50, (0x00E39080, 3, 4), p1=0),
                                            orders(60, 0x00402E50, (0x007848FC, 0x0F, 2), p1=1)]),
    'cmdrest difficulty sais': (ORDERS_SAIS, 60, [orders(0, 0x00402E50, p1=0), orders(30, 0x00402E50, p1=1)]),
}


def run(name, save, ticks, lines, tmp):
    path = os.path.join(tmp, name.replace(' ', '_').replace('/', '_') + '.txt')
    open(path, 'w').write('\n'.join(lines) + '\n')
    r = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'oracle', 'lockstep.py'), os.path.join(ROOT, save),
                        '--ticks', str(ticks), '--chained', '--commands', path], capture_output=True, text=True)
    out = r.stdout.splitlines()
    bad = [l for l in out if not l.startswith('lockstep:') and ('not ported' in l or 'differ' in l or 'exit' in l)]
    ok = r.returncode == 0 and not bad
    text = '  %s: %s' % (name, 'equal' if ok else 'FAILED')
    for l in (bad or out[-3:] + r.stderr.splitlines()[-3:])[:8] if not ok else []:
        text += '\n    ' + l
    print(text, flush=True)
    return ok


def main():
    argv = sys.argv[1:]
    if '-j' in argv:
        del argv[argv.index('-j'):argv.index('-j') + 2]
    names = [a for a in argv if not a.startswith('--')]
    if '--list' in sys.argv:
        for n, (save, ticks, lines) in TESTS.items():
            print(n, save, ticks)
            print('  ' + '\n  '.join(lines))
        return 0
    todo = [n for n in TESTS if not names or n in names]
    jobs = int(sys.argv[sys.argv.index('-j') + 1]) if '-j' in sys.argv else 1
    from concurrent.futures import ThreadPoolExecutor
    with tempfile.TemporaryDirectory() as tmp, ThreadPoolExecutor(jobs) as pool:
        passed = sum(pool.map(lambda n: run(n, *TESTS[n], tmp), todo))
    print('oracle_command_check: %d / %d command tests equal to the original' % (passed, len(todo)))
    return 0 if passed == len(todo) else 1


if __name__ == '__main__':
    sys.exit(main())
