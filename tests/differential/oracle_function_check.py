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

# name: (calls, fills, skip states with type-0x88 buildings). Calls: (address, args).
TESTS = {
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
