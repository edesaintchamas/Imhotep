"""Imhotep - differential test: every ported game function against the original, on saved states.

Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.

  local/venv/bin/python tests/differential/oracle_function_check.py [ADDR ...]

For each test state (player saves loaded by the original, in the emulator) and each entry of
TESTS, the original functions and their ports (core/state/ported.c, build/state_run) run on the
same state; the whole state (data segments, simulation heap) and every return value must be
equal (tools/oracle/harness.py). The state is restored between entries.
Exit status: 0 if all equal, 1 otherwise, 2 on setup error.
"""
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'oracle'))
import harness  # noqa: E402

STATES = [
    'local/saves/downloaded/1592/39-Actium-Dec30c.sav',      # no floodplain, no type-0x88 buildings
    'local/saves/downloaded/1564/CaesarAlan-Iken.sav',       # floodplain, 4 type-0x88 buildings
    'local/saves/downloaded/1591/38-AlexC-Feb15.sav',        # 1,704 buildings
]

GOODS = list(range(0, 41))

UI_XY = [(98, 68), (50, 60), (96, 38), (120, 100), (0, 0), (1, 2), (139, 139), (225, 10), (-1, 5), (200, 119)]



def mapadd_calls(types, first=3300):
    """package mapadd: each type created (0x0046A400) at its own id from `first` and spot, put on the map
    as new by 0x00470990 (flag 1), then again as a redraw (flag 0); size from the type table 0x005D06DC"""
    from disasm import image
    base, mem = image()
    calls = []
    for j, t in enumerate(types):
        b, x, y, size = first + j, 12 + 8 * (j % 12), 12 + 8 * (j // 12), mem[0x005D06DC - base + 0x14 * t]
        calls += [(0x0046A400, [1, t, x, y, 0, b]), (0x00470990, [1, t, b, x, y, size, 1, 0]),
                  (0x00470990, [1, t, b, x, y, size, 0, 0])]
    return calls


# the cases of 0x00470990's switch (jump tables 0x004726A0 / 0x004724F4) built from one record (or parts at
# free ids); the monuments, which take the ids after theirs, have entries of their own
MAPADD_TYPES = [t for t in range(0x0A, 0xED) if t not in (
    0x26, 0x27, 0x30, 0x34, 0x36, 0x38, 0x39, 0x3A, 0x3B, 0x49, 0x52, 0x53, 0x57, 0x5B, 0x60, 0x61, 0x62, 0x89, 0x8B,
    0x96, 0xA3, 0xA4, 0xA5, 0xA6, 0xA8, 0xA9, 0xAA, 0xAC, 0xAE, 0xAF, 0xB0, 0xC5, 0xC6, 0xC8, 0xC9, 0xD6, 0xDF) and
    not 0x74 <= t <= 0x87 and t not in (0xB7, 0xCF, 0xD0, 0xD5, 0xDE, 0xE1, 0xE3, 0xE4, 0xE5, 0xE6, 0xEA, 0xEB, 0xEC)]

# name: (calls, fills, skip states with type-0x88 buildings). Calls: (address, args).
TESTS = {
    'trade refusal message box': ([(0x00559A60, []), (0x00559B50, [])], [(0x0060598C, 1, 1)], False),
    'trade refusal message box 2': ([(0x00559B50, []), (0x00559A60, [])], [(0x0060598C, 1, 5)], False),
    'desirability': ([(0x0046A0C0, []), (0x0046A140, [])], [(0x00AC2D4C, 51984, 0x55), (0x00BC9768, 4, 0xAA)], False),
    'map row ranges': ([(0x0046F7B0, [])], [(0x00BC9804, 0xAB0, 0x77), (0x00BCA2E8, 0x18, 0x77)], False),
    'map tile inside': ([(0x0046F930, [x, y]) for x in range(-2, 230, 7) for y in range(-2, 230, 7)], [], False),
    'routing grids': ([(0x00517A20, []), (0x005178B0, []), (0x005178E0, []), (0x00517660, [])],
                      [(0x00EBAB20, 51984, 0x11), (0x00EC7630, 51984, 0x22), (0x00ED4140, 51984, 0x33)], False),
    'routing ee0c50': ([(0x00517370, [])], [(0x00EE0C50, 51984, 0x44)], False),
    'routing fillers': ([(0x00517690, [3, 5, 40, 60]), (0x005173A0, [-5, -5, 300, 300]),
                         (0x00516F90, [10, 10, 100, 90, 0])],
                        [(0x00ED4140, 51984, 0x5A), (0x00EC7630, 51984, 0x5A), (0x00EE0C50, 51984, 0x5A)], False),
    'goods predicates': ([(a, [r]) for a in (0x004439C0, 0x00443950, 0x00443B90, 0x00443D90, 0x00559480, 0x004C5240)
                          for r in GOODS] + [(0x004C5190, []), (0x00447790, [0, 5]), (0x00447790, [3, 9])], [], False),
    'goods lists': ([(0x005596D0, [])], [(0x00F53DD8, 0x128, 0x66)], False),
    'post-load links': ([(0x0046A320, []), (0x0046F030, []), (0x0046B320, [])],
                        [(0x007900B8, 200 * 0xC4, 0), (0x00EAAF68, 50 * 0xE0, 0), (0x009783A4, 11 * 0x1C, 0)], False),
    'pre-load clear': ([(0x0046FA40, [])], [], False),
    # the month / year cascade (core/city/month*.c); each entry runs on the state as loaded
    'month small steps': ([(a, []) for a in (0x00455530, 0x004E8630, 0x004E1850, 0x004EA740, 0x0045A4D0, 0x0045A500,
                                             0x0045A530, 0x0045A5A0, 0x004C5300, 0x0045A450, 0x0045A490, 0x004448E0,
                                             0x0045A560, 0x004474E0, 0x004E9700, 0x0045A3F0, 0x00459E50, 0x0045A2E0,
                                             0x0045A350, 0x00452160, 0x004B7960, 0x004E8DB0, 0x004BE640, 0x00452200,
                                             0x004BC7A0, 0x0045A5D0)], [], False),
    'month finance': ([(0x004599D0, []), (0x0045E420, []), (0x004EA000, [])], [], False),
    'month mood': ([(0x004E81D0, []), (0x004E8840, [])], [], False),
    'month ratings': ([(0x0046E180, [1]), (0x0046E1E0, [])], [], False),
    'month locusts': ([(0x004E7700, [])], [(0x0078D79C, 4, 0xFF), (0x0078D7C0, 4, 0), (0x0078D7C0, 1, 9)], False),
    'month events': ([(0x0044BA90, []), (0x004464F0, [])], [], False),
    'month messages': ([(0x004E0CF0, [0x79, 0, 0, 0, 0]), (0x004E0CF0, [0x109, 1, 2, 0x00E95228, 0])], [], False),
    # package bigb: the daily building update, the formations, the paths of type-0x88 buildings
    'bigb building update': ([(0x00462180, [])], [], False),
    'bigb formations': ([(0x004B7790, [0]), (0x004B7790, [1]), (0x004B92E0, [])], [], False),
    'bigb route searches': ([(0x00519A30, [60, 60]), (0x0051B040, [50, 50, -2, -2]),
                             (0x0051A420, [50, 50, -1, -1, 0, 600]), (0x005181C0, [40, 40, 70, 80]),
                             (0x0051DE70, [60, 60, 90, 90])], [], False),
    'bigb type-0x88 links': ([(0x0043F060, [1, 1, 1]), (0x0043F060, [2, 1, 3]), (0x0043F060, [3, 0, 1]),
                              (0x004A36B0, [1])], [], False),
    # package figb: figure actions of types 0x15-0x46, 0x52 (core/figure/actions_b*.c) and their helpers
    'figb figure loop': ([(0x00489B70, [])], [], False),
    'figb helpers': ([(0x00499B50, [i, r]) for i in (5, 29, 41, 100, 300) for r in (10, 40)] +
                     [(a, [i]) for a in (0x00492360, 0x0049B3C0, 0x00495A40, 0x00495AF0, 0x00497CF0)
                      for i in (3, 29, 77, 150)] +
                     [(a, [b]) for a in (0x004578B0, 0x004579D0, 0x00495840, 0x00495950) for b in (1, 20, 57, 200)] +
                     [(0x004B22E0, [i, i + 1, 2]) for i in (10, 60)] +
                     [(a, [i]) for a in (0x00492650, 0x00492D20, 0x00497A30, 0x004EB000) for i in (7, 99)] +
                     [(0x00492FC0, [])], [], False),
    'figb monument tiles': ([(a, [b, 0x00D3B4E4, 0x00D3B4E0]) for a in (0x004E0CC0, 0x004F6420, 0x00545A30, 0x00558F00,
                                                                       0x00556230, 0x0054F8A0)
                             for b in (1, 5, 40, 120)], [], False),
    # package figc: figure actions of types 0x47-0x50, 0x53, 0x54, 0x66-0x68 (core/figure/actions_c*.c), helpers
    'figc line searches': ([(a, [x, y, x + dx, y + dy, 0x00D3B4E4, 0x00D3B4E0]) for a in (0x0040C180, 0x0040BDA0)
                            for (x, y) in ((99, 39), (147, 57), (177, 144)) for (dx, dy) in
                            ((10, -10), (0, 10), (-7, 3), (12, 5), (-3, -11))], [], False),
    'figc spirals': ([(0x004467A0, [0x00D3B4E4, 0x00D3B4E0]), (0x004AD880, [0x00D3B4E4, 0x00D3B4E0])],
                     [(0x00D3B4E4, 1, 90), (0x00D3B4E5, 3, 0), (0x00D3B4E0, 1, 70), (0x00D3B4E1, 3, 0)], False),
    'figc helpers': ([(a, [i]) for a in (0x004A34B0, 0x004A35E0, 0x004B5770, 0x004A5A80, 0x004A4300, 0x004A43F0)
                      for i in (5, 17, 120, 153, 296)] +
                     [(0x004B57E0, [i, t]) for i in (5, 17, 120) for t in (4, 0x46)] +
                     [(a, [i, r]) for a in (0x004BCAE0, 0x004BCD60) for i in (17, 120) for r in (5, 60, 228)] +
                     [(0x004A5560, [i, r, f]) for i in (120, 153) for r in (18, 60) for f in (0, 1)] +
                     [(0x004EAB10, [100, 100, k, 90, 90, 0]) for k in (0, 7, 15)] +
                     [(0x0047D350, [0])], [], False),
    'figc docks and workers': ([(a, [i]) for a in (0x00406A30, 0x004F1CB0, 0x004F2410) for i in (17, 120, 191, 338)] +
                               [(0x004069E0, [b]) for b in (1, 50, 120, 300)] +
                               [(0x004EF3A0, [b, 191]) for b in (1, 50, 120)] +
                               [(0x004EAAC0, [0]), (0x004EAAC0, [1]), (0x004EADA0, [0x4D]), (0x004EADA0, [0x4C]),
                                (0x004867E0, [100, 100, 255, 1])], [], False),
    # package figd: figure actions of types 0x51-0x6D (core/figure/actions_d*.c) and their helpers;
    # ids 120 / 153 / 222 / 241 are boats and a laborer in the Iken state, other figures elsewhere
    'figd helpers': ([(0x004BF9D0, []), (0x004BC1E0, [50, 60, 100]), (0x004BC2E0, [0, 0, 100]),
                      (0x004BC3B0, [0, 0, 100]), (0x0048E820, [50, 50, 30]), (0x0048E820, [100, 120, 200]),
                      (0x0048E6F0, [100]), (0x004A3540, [5]), (0x004A3540, [300]), (0x004F3360, [1]),
                      (0x004F3360, [2]), (0x004BEEB0, [241]), (0x004BF920, [241]), (0x004A9730, [241]),
                      (0x004B23C0, [241, 240, 1, 5])], [], False),
    'figd boats': ([(a, [i, 0x00BCD5F8]) for a in (0x004AE650, 0x004AE2C0, 0x004ADD90, 0x004AE490, 0x004AD450,
                                                    0x004AE270, 0x004AEB00, 0x004AEC80, 0x004AD350, 0x004ADB40)
                    for i in (120, 153, 222)] +
                   [(0x004AED10, [120]), (0x004AECE0, []), (0x004AEAB0, [0x00BCD5F8, 0x00BCD5FC]),
                    (0x004ADC80, [153]), (0x004AE1F0, [153, 0]), (0x004AD4C0, [153, 50, 50])], [], False),
    # package month2: events, festival performers, monument rating, event actions (core/city/events*.c,
    # festival.c, monument_rating.c); thiscall functions on event records are checked by harness.py
    'month2 message ids': ([(0x0044FEC0, [k, l]) for k in range(0, 0x130, 7) for l in (-1, 0, 1, 2, 4)], [], False),
    'month2 festival': ([(0x00461950, [300]), (0x00461950, [1500])], [], False),
    'month2 monument rating': ([(0x0046EF50, []), (0x004F4FF0, [])], [], False),
    'month2 event actions': ([(0x004E85D0, [-30]), (0x004E85D0, [150]), (0x004BED00, []), (0x004BED40, []),
                              (0x004BECF0, []), (0x004E6EB0, []), (0x004E6ED0, []), (0x004E7200, [5]),
                              (0x00469C60, [2]), (0x004E7AC0, [1]), (0x00469F80, [0xA1]), (0x00469F80, [0x6D])] +
                             [(0x005599C0, [g]) for g in (1, 5, 12, 30)] +
                             [(0x004ED360, [x, y, m, 0]) for m in (2, 3, 4, 5, 6) for (x, y) in ((47, 51), (121, 92))],
                             [], False),
    'month2 troops': ([(0x00446A30, [k, a, w, 7, 0]) for k in (0xB, 0xC, 0xD) for a in (17, 40) for w in (1, 8)] +
                      [(0x00446260, [i]) for i in (0, 3)], [], False),
    'month2 companies': ([(0x004B8360, [ev, 0, 0]) for ev in (0, 1, 5)] + [(0x004B8690, [1])] +
                         [(0x004B8890, [p, 1]) for p in (0, 50, 100)], [], False),
    # package day2 (core/city/day2_*.c, core/building/day2_*.c); thiscall functions on varying
    # objects (0x0044BE30, 0x004BD850, 0x004BE0D0, 0x004BE490, 0x004BE580, 0x004BD380) are checked
    # by harness.py
    'day2 monuments': ([(0x004F57B0, [g]) for g in (12, 24, 25, 26, 30)], [], False),
    'day2 gods': ([(0x004C47A0, [g, 0, f]) for g in range(5) for f in (-1, 1, 2, 3, 4)], [], False),
    'day2 rings': ([(0x00481AA0, [x, y, s, r, i]) for (x, y) in ((-1, 3), (150, 200)) for s in range(8)
                    for r in (0, 1, 5, 6) for i in (0, 3)], [], False),
    'day2 helpers': ([(0x0047CBD0, [x, y]) for (x, y) in ((5, 5), (60, 40), (100, 100))] +
                     [(0x0051CC00, [10, 10, x, y]) for x in (0, 9, 10, 11, 20, 30) for y in (0, 7, 10, 13, 20)] +
                     [(0x004CF1C0, [])] +
                     [(0x0047CC70, [x, y, s, f]) for (x, y) in ((20, 30), (60, 61), (100, 90)) for (s, f) in ((1, 0), (2, 1))] +
                     [(0x004F6020, [b]) for b in (5, 20, 90, 300)], [], False),
    'day2 docks and stock': ([(a, [f]) for a in (0x004808A0, 0x004809B0) for f in (1, 5, 20, 50, 100)] +
                             [(0x00460890, [b, n, k]) for b in (5, 20, 90, 300) for (n, k) in ((100, 10), (250, 0))],
                             [], False),
    'day2 farms': ([(0x00489510, []), (0x004883D0, [20]), (0x00489180, [20, 50])] +
                   [(a, [b, 50, 50, 500, st, 0]) for a in (0x00488690, 0x00488840) for b in (30, 60) for st in (0, 70)],
                   [], False),
    'day2 buildings': ([(0x0046D840, [])] + [(0x0046AC50, [b]) for b in (30, 60, 120)] +
                       [(0x0047C2B0, [b, 50, 50, 2, 2]) for b in (0, 45)] +
                       [(0x0046D100, [b, 0]) for b in (40, 80)] + [(0x004695D0, [b, 1]) for b in (160, 200)] +
                       [(0x0043FD00, [b, t]) for (b, t) in ((70, 0x1E), (75, 0xD1))] + [(0x0046C500, [210])],
                       [], False),
    # package x87 (core/sim/x87_angle.c); every (dx, dy) in [-227, 227]^2 was checked offline
    'x87 direction': ([(0x0051CAE0, [100, 100 + dy, 100 + dx, 100]) for dx in (-227, -5, -1, 0, 1, 5, 227)
                       for dy in (-227, -3, 0, 2, 227)], [], False),
    # package rest: redraw callees (fills: images, edges, building grid), destruction, gods, notices
    'rest redraw': ([(0x0046D840, [])], [(0x00B7482C, 207936, 0), (0x00B67D1C, 51984, 0), (0x00B4E6FC, 103968, 0)],
                    False),
    'rest destroy': ([(0x004C0920, [b]) for b in (0, 5, 40)] + [(0x004C04A0, [7]), (0x004B82E0, [1]), (0x004B82E0, [2]),
                     (0x0046A2E0, [5]), (0x004F4AE0, [20]), (0x004E5950, [0]), (0x004F52B0, [1])] +
                     [(0x0045B700, [b, n]) for b in (49, 129, 307) for n in (150, 100000)],
                     [], False),
    'rest gods': ([(a, []) for a in (0x004F5340, 0x004C3950, 0x004C3AC0, 0x004C3ED0, 0x004C41B0, 0x004C42B0,
                                     0x004C45E0, 0x004B4710)] +
                  [(0x004F3AA0, [k]) for k in (1, 2)] + [(0x004C4020, [k]) for k in (0, 1)], [], False),
    'rest walls': ([(0x0047AFD0, [x, y, n]) for (x, y) in ((91, 28), (95, 30), (193, 81), (194, 84), (0, 0), (227, 227))
                    for n in (3, 5, 8)], [(0x00B7482C, 207936, 0)], False),
    'rest notices and figures': ([(0x004E2970, [k]) for k in (1, 2, 6)] +
                                 [(a, [f]) for a in (0x00480C60, 0x00480D60) for f in (1, 30, 200)], [], False),
    # package last (core/map/last_terrain.c, core/figure/last_figure.c, core/city/last_city.c,
    # core/graphics/last_graphics.c)
    'last canals': ([(0x0047BD50, [0, 0, 227, 227]), (0x0047BE30, [0]), (0x0047BE30, [1])], [], False),
    'last canals all': ([(0x0047BD50, [-5, -5, 300, 300])], [(0x00B285CC, 51984, 0)], False),
    'last figures': ([(0x00445E50, [b, g, 1, 5]) for b in (49, 129, 307) for g in range(1, 37)] +
                     [(0x004BC4A0, [a, t]) for _ in range(10) for (a, t) in ((12, 11), (17, 23), (12, 34), (17, 47), (3, 48))] +
                     [(0x00458C30, [x, y]) for (x, y) in ((50, 50), (10, 120))] +
                     [(0x00480860, [v]) for v in (0, 3)], [], False),
    'last city': ([(0x004E91E0, [0, 5]), (0x004E91E0, [1, 7])], [], False),
    # graphics slots 5-8 (maus0_2a, maus0_1a, Obelisk3x3a, Obelisk5x5d, maus1_3d) and 29 (TombRam)
    'last graphics': ([(0x0053DD90, [n, s]) for (n, s) in ((0x005E446C, 5), (0x005E446C, 5), (0x005E449C, 5),
                                                           (0x005E48BC, 6), (0x005E4868, 7), (0x005E437C, 8),
                                                           (0, 8), (0x005F75AC, 29), (0x005790C0, 29))] +
                      [(0x00562370, [0x005E446C, 0x005E449C]), (0x00562370, [0x005E4868, 0x005E4868])], [], False),
    'last graphics first fill': ([(0x0053DD90, [0x005F75AC, 29])], [(0x010D69E8, 4, 0)], False),
    # package sg3 (core/graphics/sg3.c): 0x0053DFF0 under 0x0053DD90, reloads whose group tables differ:
    # AbuSimbel / AbuSimbelDummy (26), LibExtra / LibExtraDummy / LibExtra2 (23), TombThut (25), TombTut (27),
    # TombSeti / TombSetiFin (226 images) / TombSetiDummy (28), TombRam (29); first fills of slots 29 and 26;
    # a 565 screen (0x00E92DFC = 2: the pixel conversions, graphics heap only)
    'sg3 reloads': ([(0x0053DD90, [n, s]) for (n, s) in (
        (0x005760B0, 26), (0x005790F0, 26), (0x005760B0, 26), (0x00576284, 23), (0x005F75F8, 23), (0x00576278, 23),
        (0x005F75E0, 25), (0x00579100, 25), (0x005F75D8, 27), (0x005790E0, 27), (0x005F75B4, 28), (0x005F75C0, 28),
        (0x005790D0, 28), (0x005F75AC, 29), (0x005790C0, 29))], [], False),
    'sg3 first fill 26': ([(0x0053DD90, [0x005760B0, 26])], [(0x010B0534, 4, 0)], False),
    'sg3 565 screen': ([(0x0053DD90, [n, s]) for (n, s) in ((0x005760B0, 26), (0x005790F0, 26), (0x005F75AC, 29))],
                       [(0x00E92DFC, 1, 2)], False),
    # package ui (core/command/construction_ui.c, construction_checks.c, construction_path.c)
    'ui terrain predicates': ([(a, [t]) for a in (0x00405B40, 0x00405B60, 0x00405B80)
                               for t in (0, 2, 0x100002, 0x200002, 0x20100002, 0x102, 3, -1)], [], False),
    'ui terrain scans': ([(0x00482720, [x, y, n, d, p]) for (x, y) in UI_XY for (n, d, p) in
                          ((2, 1, 0x00405B60), (2, 1, 0x00405B40), (1, 0, 0x00405B80))] +
                         [(0x004828E0, [x, y, n, d, m]) for (x, y) in UI_XY for (n, d, m) in
                          ((2, 0, 0x4000), (1, 0, 0x40), (3, 0, 0x10000), (2, 1, 0x80))] +
                         [(0x004829B0, [x, y, n, d, m, c]) for (x, y) in UI_XY for (n, d, m, c) in
                          ((3, 0, 0x80, 4), (2, 1, 0x4, 1), (1, 0, 0x80, 1))], [], False),
    'ui ghost footprints': ([(0x0047D360, [n, x, y, m, a, f]) for (x, y) in UI_XY for (n, m, a, f) in
                             ((2, -0x41, 0, 0), (3, -1, 0, 1), (1, -1, 1, 0), (5, -1, 0, 0x101))] +
                            [(0x0047D530, [n, x, y, m, f]) for (x, y) in UI_XY for (n, m, f) in ((3, -1, 1), (6, -1, 0))] +
                            [(0x004BFA50, [x, y, r]) for (x, y) in UI_XY for r in (0, 1)] + [(0x0047D4D0, [])],
                            [(0x00BCA2E0, 1, 2)], False),
    'ui ghost footprints rotated': ([(0x0047D360, [3, x, y, -1, 0, 1]) for (x, y) in UI_XY] +
                                    [(0x0047D530, [3, x, y, -1, 1]) for (x, y) in UI_XY] +
                                    [(0x004BFA50, [x, y, 1]) for (x, y) in UI_XY], [(0x00BCA2E0, 1, 6)], False),
    'ui ferry landings': ([(0x00486BF0, [x, y]) for (x, y) in UI_XY + [(96, 38), (97, 39), (104, 43), (151, 57)]] +
                         [(0x00486CF0, [o]) for o in range(229, 51984 - 229, 1499)], [], False),
    'ui path tile tests': ([(a, [o]) for a in (0x0047B8E0, 0x0047E090, 0x0047E0D0, 0x0051C390, 0x0051C510, 0x0051C630)
                            for o in range(229, 51984 - 229, 211)], [], False),
    'ui road path': ([(0x0051B680, [1, 98, 68, 0])], [], False),
    'ui canal path': ([(0x0051B680, [1, 77, 71, 1])], [], False),
    'ui road path 2': ([(0x0051B680, [1, 58, 47, 0])], [], False),
    'ui canal path 2': ([(0x0051B680, [1, 120, 100, 1])], [], False),
    'ui wall path': ([(0x0051C790, [1, 58, 47])], [], False),
    'ui undo snapshot': ([(0x00484F60, [])], [], False),
    'ui previews': ([(0x0046FEF0, [x, y, x, y, t]) for (x, y) in UI_XY[:4]
                     for t in (0x5C, 0x46, 0x64, 0x6A, 0xA1, 0x6D, 0x3B, 0x8A, 0x4D, 0x41, 0x28, 0x88, 0x4B, 0xB6, 0x4C,
                               0x5E, 0x5F, 0x32, 0x54, 0x52, 0x38, 0xAB, 0xD2, 0xB7, 0)], [(0x006059B4, 0x1C, 0)], False),
    'ui water lift': ([(0x0047EDA0, [p, x1, y1, x2, y2]) for (p, x1, y1, x2, y2) in
                       ((0, 95, 30, 99, 30), (1, 90, 30, 100, 30), (0, 99, 30, 99, 40), (0, 99, 30, 99, 30))] +
                      [(0x0047F190, [o]) for o in range(229, 51984 - 229, 2003)] +
                      [(0x0047CA50, [k, x, y, 3, 3]) for k in (0, 1) for (x, y) in UI_XY],
                      [(0x006059E4, 1, 1)], False),
    'ui choose and cancel': ([(0x00405200, []), (0x00405BA0, [])], [(0x007056EC, 1, 5), (0x00D3B198, 1, 1)], False),
    # military commands' helpers (core/command/military.c); tile helpers on grid offsets all over the map
    'military tiles': ([(a, [o]) for a in (0x004B5520, 0x004B5570, 0x004B55B0, 0x004B55F0, 0x004B5640)
                        for o in range(0, 51984, 211)], [], False),
    'military rows': ([(0x004B78B0, [r]) for r in range(0, 9)] + [(0x004BC620, [r]) for r in range(0, 9)] +
                      [(0x004B8AA0, [])], [], False),
    'military formation orders': ([(0x004B6C20, [1, 0xE]), (0x004B6C20, [2, 0xE]), (0x004B6C20, [3, 0]),
                                   (0x004B66E0, [99, 69, 1, -1]), (0x004B66E0, [120, 100, 2, 5]),
                                   (0x004B66E0, [0, 0, 4, 0]), (0x004B6A40, [99, 69, 3]), (0x004B6A40, [60, 60, 4]),
                                   (0x004B6A40, [1, 1, 2])], [], False),
    'military formation transport': ([(0x004B6C20, [1, 0xE]), (0x004B6C20, [2, -1])],
                                     [(0x00BCBA4C + 0x90, 1, 64), (0x00BCBA4C + 0x120, 1, 120)], False),
    'military ship home': ([(0x004BC670, [f]) for f in (64, 120, 153)], [], False),
    # package clear: clearing land, its confirmations and undo (core/command/clear.c, undo.c)
    'clear land': ([(0x00484F60, []), (0x004743F0, [1, 95, 65, 104, 74]), (0x004743F0, [0, 104, 74, 95, 65]),
                    (0x004743F0, [0, -3, -3, 2, 2]), (0x004743F0, [1, 130, 130, 240, 240]),
                    (0x004743F0, [0, 76, 90, 86, 100])], [], False),
    'clear apply': ([(0x00484F60, []), (0x00474700, [1, 60, 60, 75, 75]), (0x00474700, [0, 60, 60, 75, 75]),
                     (0x00474700, [0, 120, 10, 100, 30]), (0x00474700, [1, 168, 70, 173, 75])], [], False),
    'clear confirmed': ([(0x00484F60, []), (0x004021D0, []), (0x00402210, []), (0x00402250, []), (0x00402290, [])],
                        [(0x00D3B439, 1, 1), (0x00BC96A4, 1, 76), (0x00BC96A0, 1, 85), (0x00BC969C, 1, 125),
                         (0x00BC9698, 1, 100)], False),
    'clear refused': ([(0x00484F60, []), (0x004021D0, []), (0x00402250, [])],
                      [(0x00D3B439, 1, 2), (0x00BC96A4, 1, 76), (0x00BC96A0, 1, 85), (0x00BC969C, 1, 125),
                       (0x00BC9698, 1, 100)], False),
    'clear bridges iken': ([(0x00484F60, []), (0x00402290, [])],
                           [(0x00D3B439, 1, 1), (0x00BC96A4, 1, 128), (0x00BC96A0, 1, 68), (0x00BC969C, 1, 175),
                            (0x00BC9698, 1, 80)], False),
    'clear bridges alexc': ([(0x00484F60, []), (0x00474700, [1, 25, 93, 35, 99]), (0x00402290, [])],
                            [(0x00D3B439, 1, 1), (0x00BC96A4, 1, 25), (0x00BC96A0, 1, 93), (0x00BC969C, 1, 35),
                             (0x00BC9698, 1, 99), (0x00BC96B0, 1, 1)], False),
    'clear popup': ([(0x004264D0, [0x68, 0x004021D0, 2]), (0x004264D0, [0x14, 0x00402290, 0])], [], False),
    'clear bridges': ([(0x0047F9B0, [o]) for o in range(0, 51984, 97)] +
                      [(0x0047F810, [o, o & 1]) for o in range(0, 51984, 211)], [], False),
    'clear roads square': ([(0x0047B480, [x, y, n]) for x, y, n in ((0, 0, 5), (60, 60, 12), (220, 220, 9), (100, 40, 30))],
                           [], False),
    'undo restore clear': ([(0x00485230, [])], [(0x00BC968C, 1, 1), (0x00BC9694, 1, 1), (0x00BC9690, 1, 9),
                                                 (0x00BC9684, 1, 7)], False),
    'undo restore roads': ([(0x00485230, [])], [(0x00BC968C, 1, 1), (0x00BC9694, 1, 1), (0x00BC9690, 1, 5)], False),
    'undo restore gardens': ([(0x00485230, [])], [(0x00BC968C, 1, 1), (0x00BC9694, 1, 1), (0x00BC9690, 1, 0x27)], False),
    'undo restore bridge': ([(0x00485230, [])], [(0x00BC968C, 1, 1), (0x00BC9694, 1, 1), (0x00BC9690, 1, 0x52)], False),
    'undo restore none': ([(0x00485230, [])], [(0x00BC968C, 1, 1), (0x00BC9694, 1, 0)], False),
    'undo readd': ([(0x004858E0, [b]) for b in (1, 2, 5, 11, 16, 50, 64, 117, 225, 300)], [], False),
    'goods to storage': ([(0x0045AFE0, [r, 900, d]) for r in range(0, 0x24, 3) for d in (1, 0)], [], False),
    # package orders (core/command): helpers with arguments
    'orders requests': ([(0x0044C1A0, [e]) for e in (-1, 0, 3, 8, 16, 46, 47, 400)] +
                        [(0x004B84B0, [e]) for e in (-1, 0, 2, 5, 400)], [], False),
    'orders grave goods rows': ([(0x0040CC40, [k]) for k in (-1, 0, 5, 18, 40)] +
                                [(0x0040CD30, [r]) for r in (-1, 0, 1, 2, 3, 5, 9, 36)], [], False),
    'orders trade helpers': ([(0x00443A20, [r]) for r in GOODS] + [(0x004CC160, [5, k]) for k in (0, 0x71, 0x76)],
                             [], False),
    'orders view': ([(0x00430530, [o]) for o in (2, 4, 6, 0)] + [(0x004304C0, [v]) for v in (0, 5000, 26000, -1)] +
                    [(0x0047E830, [v]) for v in (-229, -1, 0, 230, 26000, 51983, 51984, 0xFFF0000)], [], False),
    # package place (core/command/place*.c): helpers of the place command 0x004728A0 with arguments
    'place small helpers': ([(0x0046FDE0, [t]) for t in range(0, 0xEE)] +
                            [(0x00470900, r) for r in ([0, 0, 227, 227], [10, 10, 12, 12], [60, 80, 50, 70], [97, 69, 99, 71],
                                                       [-5, -5, -1, -1], [43, 58, 43, 58], [78, 98, 78, 98])] +
                            [(0x004B5830, r) for r in ([0, 0, 0, 0], [97, 70, 105, 70], [60, 60, 200, 200])] +
                            [(0x0040F920, [0x00B285CC, 0xCB10, 0xAF]), (0x00485200, []),
                             (0x004E9680, [1, 50]), (0x004E9680, [1, -20]), (0x004E9680, [0, 5]), (0x004F61E0, [])] +
                            [(0x004B9EE0, [x, y]) for x, y in ((97, 70), (60, 60), (150, 100), (0, 0))], [], False),
    'place goods and roads': ([(0x0045B4D0, [1, r, n]) for r, n in ((0x17, 500), (0x1A, 10000), (0x1E, 24000), (5, 0))] +
                              [(0x00457AC0, [x, y]) for x, y in ((153, 98), (97, 70), (33, 138))] + [(0x00459FA0, [])],
                              [], False),
    'place checks': ([(0x004BFF60, [x, y, r]) for x, y in ((58, 18), (78, 88), (143, 173)) for r in (0, 1, 2)] +
                     [(0x004C3730, [x, y]) for x, y in ((63, 83), (23, 58), (120, 120))] +
                     [(0x004DEF00, [o]) for o in (20000, 30123, 45000)] + [(0x00541AE0, [o]) for o in (20000, 30123)] +
                     [(0x00542EE0, []), (0x00447530, [])] + [(0x00443900, [r]) for r in GOODS] +
                     [(0x00480770, [x, y, n]) for x, y in ((97, 70), (106, 70)) for n in (1, 2, 3)] +
                     [(0x004ECEC0, [o, v, 0x00BC9720]) for o in (20000, 30123) for v in (0, 1, 4)], [], False),
    'place notices': ([(a, [t]) for a in (0x004E2D30, 0x004E2E20, 0x004E3100, 0x004E3140, 0x004E3180, 0x004E31C0,
                                          0x004E3200, 0x004E3240, 0x004E3330, 0x004E33A0, 0x004E3460, 0x004E3520,
                                          0x004E35E0, 0x004E36A0, 0x004E3760, 0x004E39D0, 0x004E3AA0, 0x004E3CF0,
                                          0x004E3DA0, 0x004E3F60, 0x004E4120, 0x004E42D0, 0x004E4490, 0x004E4550,
                                          0x004E47D0, 0x004E4E10, 0x004E4ED0, 0x004E5070, 0x004E2D80, 0x004E2DD0,
                                          0x004E2FB0)
                       for t in (0xA, 0x1E, 0x1F, 0x20, 0x21, 0x28, 0x2F, 0x33, 0x35, 0x3B, 0x47, 0x5C, 0x5F, 0x6E, 0x6F,
                                 0x71, 0x72, 0xB1, 0xB7, 0xC2, 0xCB, 0xCC, 0xD5, 0xDE, 0xE1, 0xE2, 0xE3, 0xE4, 0xE7,
                                 0xE8, 0xE9)] +
                      [(0x004E2B10, [t, 97, 70, 2]) for t in (0x46, 0x48, 0x8C, 0x5C, 0x41)] +
                      [(0x004E3280, [t, 106, 71, 2]) for t in (0x3B, 0xAC)] + [(0x004E32E0, [0x5C, 97, 70])],
                      [(0x00E94E20, 4, 0)], False),
    'place entertainment helpers': ([(0x004413B0, [x, y, t]) for x, y in ((111, 60), (71, 43)) for t in (0x1F, 0x1E, 0x21, 0xD1)] +
                                    [(0x0043F3A0, [a, b, n]) for n in (2, 3, 4, 5, 6) for a in range(0, 3) for b in range(0, 3)] +
                                    [(0x0043F250, [x, y, x - 1, y - 1, n]) for x, y in ((111, 60), (112, 61), (71, 43))
                                     for n in (2, 3, 4, 5)], [], False),
    'place drags': ([(0x00484F60, []), (0x00474130, [0, 97, 70, 99, 72]), (0x00474F50, [0, 97, 72, 104, 72]),
                     (0x00475090, [0, 90, 74, 96, 74]), (0x00475220, [0, 106, 70, 112, 71, 0x4000]),
                     (0x004754A0, [0, 80, 64, 90, 64]), (0x004759E0, [0, 100, 76, 103, 78])], [], False),
    'place drags preview': ([(0x00484F60, []), (0x00474130, [1, 97, 70, 99, 72]), (0x00474F50, [1, 97, 72, 104, 72]),
                             (0x00475090, [1, 90, 74, 96, 74]), (0x00475220, [1, 106, 70, 112, 71, 0x804000])],
                            [], False),
    # package mapadd (core/building/map_add.c, map_add_monument.c): the obelisks (0xCF, 0xD0) are not
    # here: a new one makes the original load "Obelisk3x3a" / "Obelisk5x5a", which stops on a message box
    # (graphic file out of sync) in the oracle; they are checked on Iken's obelisk with tools/oracle/harness.py
    'mapadd types 1': (mapadd_calls(MAPADD_TYPES[0:40]), [], False),
    'mapadd types 2': (mapadd_calls(MAPADD_TYPES[40:80]), [], False),
    'mapadd types 3': (mapadd_calls(MAPADD_TYPES[80:]), [], False),
    'mapadd fort types': (mapadd_calls([0x39, 0x28, 0x2C, 0x2D]), [], False),
    'mapadd pyramid': (mapadd_calls([0xB7]), [], False),
    'mapadd sun temple': (mapadd_calls([0xD5]), [], False),
    'mapadd mastaba': (mapadd_calls([0xDE]), [], False),
    'mapadd monuments e1 e3 e4 e6': (mapadd_calls([0xE1]) + mapadd_calls([0xE3], 3340) + mapadd_calls([0xE4], 3360) +
                                     mapadd_calls([0xE6], 3380), [], False),
    'mapadd tomb e5': (mapadd_calls([0xE5]), [], False),
    'mapadd tomb ea': (mapadd_calls([0xEA]), [], False),
    'mapadd tomb eb': (mapadd_calls([0xEB]), [], False),
    'mapadd tomb ec': (mapadd_calls([0xEC]), [], False),
    # package cmdrest: build-menu availability (output filled first), menu counts, monument menus,
    # hover functions (ferry reach, bridge measure), the advisor command, the model files
    'cmdrest menu update': ([(0x00404350, [])], [(0x0070A8DC, 0x11D0, 0x5A)], False),
    'cmdrest menu update m2': ([(0x00404350, [])], [(0x0070A8DC, 0x11D0, 0x5A), (0x00E38EBC, 4, 0),
                                                    (0x00E38EA8, 1, 2)], False),
    'cmdrest menu update m5': ([(0x00404350, [])], [(0x0070A8DC, 0x11D0, 0x5A), (0x00E38EBC, 4, 0),
                                                    (0x00E38EA8, 1, 5)], False),
    'cmdrest menu update m0': ([(0x00404350, [])], [(0x0070A8DC, 0x11D0, 0x5A), (0x00E38EBC, 4, 0),
                                                    (0x00E38EA8, 1, 0)], False),
    'cmdrest menu update editor': ([(0x00404350, [])], [(0x0070A8DC, 0x11D0, 0x5A), (0x00605A3C, 1, 1)], False),
    **{'cmdrest menu count %d' % m: ([(0x00404120, [])], [(0x007056EC, 4, 0), (0x007056EC, 1, m)], False)
       for m in (3, 6, 9, 10, 0x16, 0x1C, 0x24, 0x25)},
    'cmdrest monument menus': ([(0x004F5470, [t]) for t in (0xB1, 0xB2, 0xB3, 0xE7, 0xB0)] +
                               [(0x004F5050, [k, a]) for k in (0xB, 0xC, 0x14, 0x18, 0x21) for a in (0, 1)] +
                               [(0x004F5020, []), (0x00403EF0, [])], [], False),
    'cmdrest ferry reach': ([(0x0047F3E0, [x, y, 0]) for x, y in UI_XY + [(96, 25), (121, 26), (101, 55), (43, 81)]],
                            [], False),
    'cmdrest bridge measure': ([(0x0047F200, [x, y, 0]) for x, y in UI_XY + [(149, 39), (142, 46), (127, 50), (146, 44)]] +
                               [(0x0047EC80, [20000, 4]), (0x0047ECC0, [20000, 4])], [], False),
    **{'cmdrest advisor %d' % a: ([(0x0040A0E0, [])], [(0x00D3B198, 4, 0), (0x00D3B198, 1, a)], False)
       for a in (1, 4, 9)},
    'cmdrest advisor refused': ([(0x0040A0E0, [])], [(0x00D3B198, 4, 0), (0x00D3B198, 1, 3), (0x00E38EBC, 4, 0),
                                                     (0x006FDC8B, 1, 0), (0x00E38DDA, 1, 5)], False),
    **{'cmdrest models %d' % d: ([(0x00540C90, []), (0x00540ED0, []), (0x00541160, []), (0x004BC720, [])],
                                 [(0x00E39080, 4, 0), (0x00E39080, 1, d), (0x00705EA4, 0x2508, 0x77),
                                  (0x0070CDCC, 0x6E0, 0x77), (0x007083AC, 0x2530, 0x66), (0x00EA1384, 0x478, 0x55)], False)
       for d in (0, 3)},
}


def main():
    only = {int(a, 16) for a in sys.argv[1:]}
    failed = total = 0
    with tempfile.TemporaryDirectory() as tmp:
        for path in STATES:
            path = os.path.join(ROOT, path)
            g = harness.oracle_state(path, 0)
            snap = g.state_bytes()
            has88 = 'Iken' in path
            for name, (calls, fills, skip88) in TESTS.items():
                if only and not any(a in only for a, _ in calls):
                    continue
                if skip88 and has88:
                    continue
                g.load_state(snap)
                for a, n, v in fills:
                    g.write(a, bytes([v]) * n)
                try:
                    written, ranges, bad = harness.compare(g, calls, tmp)
                except RuntimeError as e:
                    print('  %s / %s: %s' % (os.path.basename(path), name, e))
                    return 2
                total += 1
                ok = not ranges and not bad
                failed += not ok
                print('  %-28s %-16s %s (%d bytes written)' % (os.path.basename(path)[:28], name,
                                                              'equal' if ok else 'DIFFERENT', written), flush=True)
                for addr, n in ranges[:5]:
                    print('      %s: %d bytes' % (harness.diff.name_address(addr), n))
                for (a, args), ro, rp in bad[:5]:
                    print('      0x%08X(%s): original %d, port %d' % (a, ','.join(map(str, args)), ro, rp))
    print('oracle_function_check: %d / %d function tests equal to the original' % (total - failed, total))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
