/* Imhotep - the festival's performers: entertainment venues send their dancers / musicians / jugglers
 * (0x00461950, 0x00461B20) (package month2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/festival.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

/* @bin 0x005DA544 four i32 shares (percent) and 0x005DA554 four 0-terminated lists of building
 * types (12 i32 each, 0x30 bytes): the four kinds of venues; 0x0070573C i32 by building type: the
 * number of such buildings */
#define SHARES 0x005DA544u
#define LISTS 0x005DA554u
#define LIST_END 0x005DA614u
#define TYPE_COUNT 0x0070573Cu

void festival_performer(game_state *st, int32_t b)
{
    /* @bin 0x00461B20 a performer (figure 0x5B) leaves building b from its road tile (0x00480180,
     * 0x00D3B4E4 / 0x00D3B4E0): building +0x2A := it, figure +0x38 := 8, +0x5A := b, +0x36 := 0; its
     * +0x0B by the building type (table 0x00461E6C); +0x70 / +0x98 from a counter by that kind
     * (table 0x00461F58: the counter of 0x00D0569C..0x00D056E0 cycles, its limit, the value) */
    uint32_t r = BUILDING_ADDR(b);
    if (GAME_CALL(st, 0x00480180u, S16(st, r + 6), S16(st, r + 8), U8(st, r + 3)) == 0) {
        return;
    }
    int32_t f = GAME_CALL(st, 0x004B4800u, U8(st, r + 1), 0x5B, S32(st, 0x00D3B4E4u), S32(st, 0x00D3B4E0u), 0);
    S16(st, r + 0x2A) = (int16_t) f;
    uint32_t fr = FIGURE_ADDR(f);
    int32_t type = S16(st, r + 0x10);
    U8(st, fr + 0x38) = 8;
    S16(st, fr + 0x5A) = (int16_t) b;
    S16(st, fr + 0x36) = 0;
    uint8_t kind;
    switch (type) {
    case 0x1E: case 0x22: kind = 0x10; break;
    case 0x1F: case 0x20: case 0x24: case 0xE2: kind = 0xF; break;
    case 0x21: case 0x23: kind = 0x11; break;
    case 0x33: case 0x35: kind = 0x1D; break;
    case 0x3C: case 0x3D: case 0x3E: case 0x3F: case 0x40: case 0x41: case 0x42: case 0x43: case 0x44:
    case 0x45: case 0x97: case 0x9C: kind = 0x1B; break;
    default: kind = 0x28; break;
    }
    U8(st, fr + 0x0B) = kind;
    uint32_t counter;
    int32_t limit, value;
    switch (kind) {
    case 0xF: counter = 0x00D056BCu; limit = 0x20; value = 0x107; break;
    case 0x10: counter = 0x00D056B8u; limit = 0x20; value = 0x108; break;
    case 0x11: counter = 0x00D056B4u; limit = 0x10; value = 0x109; break;
    case 0x1D: counter = 0x00D0569Cu; limit = 0x20; value = 0x10F; break;
    case 0x28: counter = 0x00D056D4u; limit = 0x20; value = 0x101; break;
    default: { /* 0x1B: by 0x004C5120(the building), 0..4 */
        int32_t lv = GAME_CALL(st, 0x004C5120u, S16(st, fr + 0x5A));
        if ((uint32_t) lv > 4) {
            return;
        }
        counter = 0x00D056B0u - 4u * (uint32_t) lv;
        limit = 0x10;
        value = 0x10A + lv;
        break;
    }
    }
    int32_t n = S32(st, counter);
    S16(st, fr + 0x70) = (int16_t) n;
    n++;
    S32(st, fr + 0x98) = value;
    S32(st, counter) = n;
    if (n >= limit) {
        S32(st, counter) = 0;
    }
}

void festival_performers(game_state *st, int32_t amount)
{
    /* @bin 0x00461950 (cdecl) the performers wanted of each kind: share * amount / 100, the part a
     * kind cannot fill (fewer buildings) passing to the next; then the buildings in use (+0 == 1)
     * whose main part (0x00475B50) works (+0x40 or +5 set, and with +5 its +0x1C) send one each
     * (0x00461B20) while their kind still wants some; stop when none is wanted */
    int32_t want[4], have[4];
    for (int i = 0; i < 4; i++) {
        int32_t part = (int32_t) (U32(st, SHARES + 4u * (uint32_t) i) * (uint32_t) amount) / 100;
        want[i] = i == 0 ? part : want[i] + part;
        have[i] = 0;
        for (uint32_t p = LISTS + 0x30u * (uint32_t) i; S32(st, p) != 0; p += 4) {
            have[i] += S32(st, TYPE_COUNT + 4u * (uint32_t) S32(st, p));
        }
        if (i != 3) {
            want[i + 1] = 0;
        }
        if (have[i] > want[i]) {
            have[i] = want[i];
        } else if (have[i] < want[i] && i != 3) {
            want[i + 1] = want[i] - have[i];
        }
    }
    for (int32_t b = 0; b <= S32(st, 0x0078D790u); b++) {
        uint32_t r = BUILDING_ADDR(b);
        if (U8(st, r) != 1) {
            continue;
        }
        uint32_t m = BUILDING_ADDR(GAME_CALL(st, 0x00475B50u, b));
        if (S16(st, m + 0x40) == 0 && U8(st, m + 5) == 0) {
            continue;
        }
        if (U8(st, m + 5) != 0 && S16(st, m + 0x1C) == 0) {
            continue;
        }
        int none = 1;
        for (int i = 0; i < 4; i++) {
            if (have[i] == 0) {
                continue;
            }
            none = 0;
            for (uint32_t p = LISTS + 0x30u * (uint32_t) i; S32(st, p) != 0; p += 4) {
                if ((int32_t) S16(st, r + 0x10) == S32(st, p)) {
                    have[i]--;
                    festival_performer(st, b);
                }
                if (have[i] == 0) {
                    break;
                }
            }
        }
        if (none) {
            return;
        }
    }
}

static int32_t run_performers(game_state *st, const int32_t *a)
{
    festival_performers(st, a[0]);
    return 0;
}

static int32_t run_performer(game_state *st, const int32_t *a)
{
    festival_performer(st, a[0]);
    return 0;
}

const ported_function month2_festival_ports[] = {
    {0x00461950u, "festival_performers", 1, 0, 0, run_performers, 0},
    {0x00461B20u, "festival_performer", 1, 0, 0, run_performer, 0},
};
const int month2_festival_ports_count = sizeof(month2_festival_ports) / sizeof(month2_festival_ports[0]);
