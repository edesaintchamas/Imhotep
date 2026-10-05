/* Imhotep - the building constructor (package terr).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x0046A400 (city, type, x, y, n, id) takes building id (0: the first free run of n records,
 * not in the removed list) and fills the fields a new building starts with (docs/formats/save.md
 * §5). */
#include "building/building_create.h"

#include "building/day_access.h"
#include "city/month_mood.h"
#include "city/rest_notice.h"
#include "state/ported.h"
#include "state/post_load.h"
#include "state/records.h"

#define B(id) BUILDING_ADDR(id)

int32_t building_removed_listed(game_state *st, int32_t id)
{
    /* @bin 0x004851B0 (id) id is in the removed list (0x00BA746C, 269 words) while it is in use
     * (0x00BC968C == 1, 0x00BC9694 == 1, count 0x00BC95F0 > 0) */
    if (id <= 0 || S32(st, 0x00BC968Cu) != 1 || S32(st, 0x00BC9694u) != 1 || S32(st, 0x00BC95F0u) <= 0) {
        return 0;
    }
    for (uint32_t i = 0; i < 269; i++) {
        if (S16(st, 0x00BA746Cu + 2u * i) == id) {
            return 1;
        }
    }
    return 0;
}

int32_t footprint_touches_water(game_state *st, int32_t x, int32_t y, int32_t size)
{
    /* @bin 0x00481000 (x, y, size) a tile of the footprint ring of the size (0x005CC9E0 + 0x60 *
     * size, at most 24 offsets, 0 ends) has terrain 4 */
    int32_t c = S32(st, 0x00BCA2D8u) + y * 228 + x;
    uint32_t list = 0x005CC9E0u + 0x60u * (uint32_t) size;
    for (uint32_t i = 0; i < 0x18; i++) {
        int32_t d = S32(st, list + 4u * i);
        if (d == 0) {
            return 0;
        }
        if (U8(st, 0x00AF598Cu + 4u * (uint32_t) (d + c)) & 4) {
            return 1;
        }
    }
    return 0;
}

static int32_t free_run(game_state *st, int32_t n)
{
    /* @bin 0x0046A4DD the first id from 1 whose record and the n - 1 next are unused and not in
     * the removed list (the next n - 1 records' use is checked before their list entries) */
    for (int32_t c = 1; c < 4000; c++) {
        if (U8(st, B(c)) != 0 || building_removed_listed(st, c) != 0) {
            continue;
        }
        if (n <= 1) {
            return c;
        }
        int ok = 1;
        for (int32_t k = c + 1; k < c + n && ok; k++) {
            ok = U8(st, B(k)) == 0;
        }
        for (int32_t k = c + 1; k < c + n && ok; k++) {
            ok = building_removed_listed(st, k) == 0;
        }
        if (ok) {
            return c;
        }
    }
    return 0;
}

int32_t building_create(game_state *st, int32_t city, int32_t type, int32_t x, int32_t y, int32_t n, int32_t id)
{
    /* @bin 0x0046A400 no free record: notice 0x41 (0x004E2970) and 0 */
    if (id == 0) {
        id = free_run(st, n);
    } else if (U8(st, B(id)) != 0 || building_removed_listed(st, id) != 0) {
        id = 0;
    }
    if (id == 0) {
        notice_add_4e2970(st, 0x41);
        return 0;
    }
    uint32_t b = B(id);
    uint32_t t = 0x14u * (uint32_t) type;
    U8(st, b + 0x00) = 3;
    U8(st, b + 0x01) = (uint8_t) city;
    U8(st, b + 0x02) = U8(st, 0x00EA1875u + 0x49D8u * (uint32_t) city);
    S16(st, b + 0x10) = (int16_t) type;
    U8(st, b + 0x03) = U8(st, 0x005D06DCu + t); /* size */
    S16(st, b + 0x16) = (int16_t) S32(st, 0x0078D7B4u);
    S32(st, 0x0078D7B4u) += 1;
    U8(st, b + 0xC2) = 0x32;
    S16(st, b + 0x20) = 0;
    /* @bin 0x0046A4B0 house level +5 by type: 10-19: 1, 20-23: 2, 24-27: 3, 28-29: 4 */
    int16_t ty = S16(st, b + 0x10);
    U8(st, b + 0x05) = ty >= 10 && ty <= 0x13 ? 1 : ty >= 0x14 && ty <= 0x17 ? 2 : ty >= 0x18 && ty <= 0x1B ? 3
                     : ty >= 0x1C && ty <= 0x1D ? 4 : 0;
    ty = S16(st, b + 0x10);
    S16(st, b + 0x12) = ty >= 10 && ty <= 0x1D ? (int16_t) (type - 10) : 0;
    S16(st, b + 0xCE) = 0;
    if (U8(st, b + 0x05) != 0) {
        S32(st, b + 0xE4) = S32(st, 0x0070CE0Cu + 0x58u * (uint32_t) S16(st, b + 0x12));
        S32(st, b + 0xE8) = 0;
        S32(st, b + 0xFC) = 0;
        house_mood_month(st, id, 1);
    } else {
        S32(st, b + 0xE4) = 0;
    }
    /* @bin 0x0046A628 +0xAA: types 0x8C-0x95 and 0xCA: [0x00BC9714]; 0x29-0x2B: (v % 4) << 4 +
     * the orientation-turned (v + 4) / 4 */
    int32_t t16 = S16(st, b + 0x10);
    if ((t16 >= 0x8C && t16 <= 0x95) || t16 == 0xCA) {
        U8(st, b + 0xAA) = U8(st, 0x00BC9714u);
    } else if (t16 >= 0x29 && t16 <= 0x2B) {
        int32_t v = S32(st, 0x00BC9714u);
        int32_t lo = v % 4;
        int32_t k = (v + 4) / 4;
        if (k >= 4) {
            k = 0;
        }
        k = S32(st, 0x00BCA2E0u) / 2 + k + 1;
        if (k >= 4) {
            k -= 4;
        }
        U8(st, b + 0xAA) = (uint8_t) ((uint8_t) (lo << 4) + (uint8_t) k);
    }
    /* @bin 0x0046A6A6 the type's service kind +0x43 and part number +0x12 (and +0xCE) */
    int16_t p = -1, extra = -1;
    uint8_t k43 = 0;
    switch (S16(st, b + 0x10)) {
    case 0x66: k43 = 1; break;
    case 0x67: k43 = 3; break;
    case 0x68: k43 = 5; break;
    case 0x65: k43 = 0x10; break;
    case 0xE0: k43 = 0x20; break;
    case 0x69: k43 = 4; break;
    case 0x64: k43 = 0xE; break;
    case 0xC2: k43 = 2; p = 0xD; break;
    case 0xC4: k43 = 6; break;
    case 0xC3: k43 = 0x16; break;
    case 0x6A: k43 = 0x18; break;
    case 0xDD: k43 = 0x1E; break;
    case 0x6B: k43 = 0x19; break;
    case 0xD8: k43 = 0x1A; break;
    case 0xA1: k43 = 0x15; break;
    case 0xD9: k43 = 0x1D; break;
    case 0xA2: k43 = 0x12; break;
    case 0x6C: k43 = 0x14; break;
    case 0x6D: k43 = 0xB; break;
    case 0x6E: k43 = 0xF; p = 9; break;
    case 0x6F: k43 = 0x11; p = 1; break;
    case 0xCB: k43 = 0x17; p = 0xB; break;
    case 0xCC: k43 = 0xC; p = 5; extra = 0xD; break;
    case 0xE8: k43 = 0x22; p = 0x17; extra = 0x18; break;
    case 0xE9: k43 = 0x21; p = 0x14; break;
    case 0xCD: k43 = 0x1C; p = 4; break;
    case 0x70: k43 = 0xA; p = 0x11; break;
    case 0x71: k43 = 0x13; p = 0xA; break;
    case 0x72: k43 = 0xD; p = 5; break;
    case 0xB1: p = 4; break;
    case 0xE7: p = 0x19; extra = 5; break;
    case 0xB2: case 0xB3: p = 0; break;
    case 0x5F: p = 8; extra = 0xE; break;
    case 0x4A:
        U8(st, b + 0x43) = 0;
        S16(st, b + 0x12) = 4;
        rec4a_create(st, id); /* @bin 0x0046A280 */
        break;
    case 0xBB: case 0xBC: case 0xBD: p = 0xF; break;
    case 0x35: case 0x33: p = 0xC; break;
    case 0x2F: p = 0x10; break;
    case 0x20: p = 0x13; break;
    case 0xE2: p = 0x16; extra = 0xD; break;
    default: break;
    }
    if (S16(st, b + 0x10) != 0x4A) {
        U8(st, b + 0x43) = k43;
        if (p >= 0) {
            S16(st, b + 0x12) = p;
        }
        if (extra >= 0) {
            S16(st, b + 0xCE) = extra;
        }
    }
    if (S16(st, b + 0x10) == 0x47 || S16(st, b + 0x10) == 0xBF) {
        S16(st, b + 0x56) = 0xC80;
    }
    S16(st, b + 0x06) = (int16_t) x;
    S16(st, b + 0x08) = (int16_t) y;
    int32_t o = S32(st, 0x00BCA2D8u) + y * 228 + x;
    S32(st, b + 0x0C) = o;
    uint8_t r = (uint8_t) (U8(st, 0x00ACF85Cu + (uint32_t) o) & 0x7F);
    U8(st, b + 0x4D) = r;
    U8(st, b + 0x34) = r & 6;
    U8(st, b + 0x4C) = U8(st, 0x005D06E0u + t);
    U8(st, b + 0xC0) = (uint8_t) footprint_touches_water(st, x, y, U8(st, b + 0x03));
    if (S16(st, b + 0x10) == 0x88) {
        /* @bin 0x0046AAEA the first of a pair (0x00BCA310 == 0) is kept in 0x00BC9800; the second
         * (1, 2) and it point at each other (+0xC4) */
        int32_t mode = S32(st, 0x00BCA310u);
        if (mode == 0) {
            S32(st, 0x00BC9800u) = id;
        } else if (mode == 1 || mode == 2) {
            int32_t first = S32(st, 0x00BC9800u);
            S32(st, b + 0xC4) = first;
            S32(st, B(first) + 0xC4) = id;
        }
    }
    int16_t tt = S16(st, b + 0x10);
    if (tt == 0x1F || tt == 0x1E || tt == 0x21) {
        S16(st, b + 0x9E) = 0;
        S16(st, b + 0xA0) = 0;
        S16(st, b + 0xA2) = 0;
        S16(st, b + 0xA4) = 0;
        U8(st, b + 0xA7) = 0;
        U8(st, b + 0xA8) = 0;
        U8(st, b + 0xA9) = 0;
    }
    tt = S16(st, b + 0x10);
    if (tt == 0x73 || tt == 0x6C || tt == 0xB1 || tt == 0xB2 || tt == 0xB3 || tt == 0xE7 || tt == 0xC3 ||
        (uint8_t) building_needs_area_check(st, id) != 0 || S16(st, b + 0x10) == 0xC7) {
        U8(st, b + 0xA6) = 0;
    }
    if (S32(st, 0x0078D790u) <= id) {
        S32(st, 0x0078D790u) = id;
    }
    return id;
}

/* ---- registration -------------------------------------------------------------------------- */

static int32_t run_listed(game_state *st, const int32_t *a) { return building_removed_listed(st, a[0]); }
static int32_t run_water(game_state *st, const int32_t *a) { return footprint_touches_water(st, a[0], a[1], a[2]); }
static int32_t run_create(game_state *st, const int32_t *a) { return building_create(st, a[0], a[1], a[2], a[3], a[4], a[5]); }

static int32_t run_chkstk(game_state *st, const int32_t *a)
{
    /* @bin 0x00561F30 the CRT stack probe (_chkstk: eax bytes of stack below esp, touched page by
     * page): nothing in the game state; registered so that the functions with large frames
     * (0x004BE160, ...) are compared without it */
    (void) st;
    (void) a;
    return 0;
}

const ported_function terr_create_ports[] = {
    {0x004851B0u, "building_removed_listed", 1, 32, 0, run_listed, 0},
    {0x00481000u, "footprint_touches_water", 3, 32, 0, run_water, 0},
    {0x0046A400u, "building_create", 6, 32, 0, run_create, 0},
    {0x00561F30u, "chkstk (stack probe)", 0, 0, 0, run_chkstk, 0},
};
const int terr_create_ports_count = sizeof(terr_create_ports) / sizeof(terr_create_ports[0]);
