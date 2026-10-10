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
#include "state/layout.h"
#include "state/ported.h"
#include "state/post_load.h"
#include "state/records.h"

#define B(id) BUILDING_ADDR(id)

int32_t building_removed_listed(game_state *st, int32_t id)
{
    /* @bin 0x004851B0 (id) id is in the removed list (0x00BA746C, 269 words) while it is in use
     * (0x00BC968C == 1, 0x00BC9694 == 1, count 0x00BC95F0 > 0) */
    if (id <= 0 || S32(st, G_UNDO_READY) != 1 || S32(st, G_UNDO_POSSIBLE) != 1 || S32(st, G_UNDO_COUNT) <= 0) {
        return 0;
    }
    for (uint32_t i = 0; i < 269; i++) {
        if (S16(st, G_UNDO_IDS + 2u * i) == id) {
            return 1;
        }
    }
    return 0;
}

int32_t footprint_touches_water(game_state *st, int32_t x, int32_t y, int32_t size)
{
    /* @bin 0x00481000 (x, y, size) a tile of the footprint ring of the size (0x005CC9E0 + 0x60 *
     * size, at most 24 offsets, 0 ends) has terrain 4 */
    int32_t c = S32(st, G_MAP_START) + y * 228 + x;
    uint32_t list = G_RING_OFFSETS + 0x60u * (uint32_t) size;
    for (uint32_t i = 0; i < 0x18; i++) {
        int32_t d = S32(st, list + 4u * i);
        if (d == 0) {
            return 0;
        }
        if (U8(st, G_GRID_TERRAIN + 4u * (uint32_t) (d + c)) & 4) {
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
        if (U8(st, B(c) + BLD_STATE) != 0 || building_removed_listed(st, c) != 0) {
            continue;
        }
        if (n <= 1) {
            return c;
        }
        int ok = 1;
        for (int32_t k = c + 1; k < c + n && ok; k++) {
            ok = U8(st, B(k) + BLD_STATE) == 0;
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
    } else if (U8(st, B(id) + BLD_STATE) != 0 || building_removed_listed(st, id) != 0) {
        id = 0;
    }
    if (id == 0) {
        notice_add_4e2970(st, 0x41);
        return 0;
    }
    uint32_t b = B(id);
    uint32_t t = 0x14u * (uint32_t) type;
    U8(st, b + BLD_STATE) = 3;
    U8(st, b + BLD_OWNER) = (uint8_t) city;
    U8(st, b + BLD_U02) = U8(st, CITY_ADDR(city) + CITY_U05);
    S16(st, b + BLD_TYPE) = (int16_t) type;
    U8(st, b + BLD_SIZE) = U8(st, G_MODELS + t); /* size */
    S16(st, b + BLD_SEQUENCE) = (int16_t) S32(st, G_BUILDING_SEQUENCE);
    S32(st, G_BUILDING_SEQUENCE) += 1;
    U8(st, b + BLD_SENTIMENT) = 0x32;
    S16(st, b + BLD_DISTANCE) = 0;
    /* @bin 0x0046A4B0 house level +5 by type: 10-19: 1, 20-23: 2, 24-27: 3, 28-29: 4 */
    int16_t ty = S16(st, b + BLD_TYPE);
    U8(st, b + BLD_HOUSE_SIZE) = ty >= 10 && ty <= 0x13 ? 1 : ty >= 0x14 && ty <= 0x17 ? 2 : ty >= 0x18 && ty <= 0x1B ? 3
                     : ty >= 0x1C && ty <= 0x1D ? 4 : 0;
    ty = S16(st, b + BLD_TYPE);
    S16(st, b + BLD_SUBTYPE) = ty >= 10 && ty <= 0x1D ? (int16_t) (type - 10) : 0;
    S16(st, b + BLD_INPUT2) = 0;
    if (U8(st, b + BLD_HOUSE_SIZE) != 0) {
        S32(st, b + BLD_CRIME_RISK) = S32(st, G_HOUSE_LEVEL_CRIME_RISK + 0x58u * (uint32_t) S16(st, b + BLD_SUBTYPE));
        S32(st, b + BLD_MALARIA_RISK) = 0;
        S32(st, b + BLD_DISEASE_RISK) = 0;
        house_mood_month(st, id, 1);
    } else {
        S32(st, b + BLD_CRIME_RISK) = 0;
    }
    /* @bin 0x0046A628 +0xAA: types 0x8C-0x95 and 0xCA: [0x00BC9714]; 0x29-0x2B: (v % 4) << 4 +
     * the orientation-turned (v + 4) / 4 */
    int32_t t16 = S16(st, b + BLD_TYPE);
    if ((t16 >= 0x8C && t16 <= 0x95) || t16 == 0xCA) {
        U8(st, b + BLD_ORIENTATION) = U8(st, G_BUILD_ORIENTATION);
    } else if (t16 >= 0x29 && t16 <= 0x2B) {
        int32_t v = S32(st, G_BUILD_ORIENTATION);
        int32_t lo = v % 4;
        int32_t k = (v + 4) / 4;
        if (k >= 4) {
            k = 0;
        }
        k = S32(st, G_VIEW_ORIENTATION) / 2 + k + 1;
        if (k >= 4) {
            k -= 4;
        }
        U8(st, b + BLD_ORIENTATION) = (uint8_t) ((uint8_t) (lo << 4) + (uint8_t) k);
    }
    /* @bin 0x0046A6A6 the type's service kind +0x43 and part number +0x12 (and +0xCE) */
    int16_t p = -1, extra = -1;
    uint8_t k43 = 0;
    switch (S16(st, b + BLD_TYPE)) {
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
        U8(st, b + BLD_OUTPUT_RESOURCE) = 0;
        S16(st, b + BLD_SUBTYPE) = 4;
        rec4a_create(st, id); /* @bin 0x0046A280 */
        break;
    case 0xBB: case 0xBC: case 0xBD: p = 0xF; break;
    case 0x35: case 0x33: p = 0xC; break;
    case 0x2F: p = 0x10; break;
    case 0x20: p = 0x13; break;
    case 0xE2: p = 0x16; extra = 0xD; break;
    default: break;
    }
    if (S16(st, b + BLD_TYPE) != 0x4A) {
        U8(st, b + BLD_OUTPUT_RESOURCE) = k43;
        if (p >= 0) {
            S16(st, b + BLD_SUBTYPE) = p;
        }
        if (extra >= 0) {
            S16(st, b + BLD_INPUT2) = extra;
        }
    }
    if (S16(st, b + BLD_TYPE) == 0x47 || S16(st, b + BLD_TYPE) == 0xBF) {
        S16(st, b + BLD_INVENTORY) = 0xC80;
    }
    S16(st, b + BLD_X) = (int16_t) x;
    S16(st, b + BLD_Y) = (int16_t) y;
    int32_t o = S32(st, G_MAP_START) + y * 228 + x;
    S32(st, b + BLD_GRID_OFFSET) = o;
    uint8_t r = (uint8_t) (U8(st, G_GRID_RANDOM + (uint32_t) o) & 0x7F);
    U8(st, b + BLD_RANDOM) = r;
    U8(st, b + BLD_ROAM_DIRECTION) = r & 6;
    U8(st, b + BLD_U4C) = U8(st, G_MODELS + 4 + t);
    U8(st, b + BLD_NEAR_WATER) = (uint8_t) footprint_touches_water(st, x, y, U8(st, b + BLD_SIZE));
    if (S16(st, b + BLD_TYPE) == 0x88) {
        /* @bin 0x0046AAEA the first of a pair (0x00BCA310 == 0) is kept in 0x00BC9800; the second
         * (1, 2) and it point at each other (+0xC4) */
        int32_t mode = S32(st, G_FERRY_STATE);
        if (mode == 0) {
            S32(st, G_FERRY_BUILDING) = id;
        } else if (mode == 1 || mode == 2) {
            int32_t first = S32(st, G_FERRY_BUILDING);
            S32(st, b + BLD_FERRY_PARTNER) = first;
            S32(st, B(first) + BLD_FERRY_PARTNER) = id;
        }
    }
    int16_t tt = S16(st, b + BLD_TYPE);
    if (tt == 0x1F || tt == 0x1E || tt == 0x21) {
        S16(st, b + BLD_PERFORMERS) = 0;
        S16(st, b + BLD_UA0) = 0;
        S16(st, b + BLD_UA2) = 0;
        S16(st, b + BLD_UA4) = 0;
        U8(st, b + BLD_UA7) = 0;
        U8(st, b + BLD_UA8) = 0;
        U8(st, b + BLD_UA9) = 0;
    }
    tt = S16(st, b + BLD_TYPE);
    if (tt == 0x73 || tt == 0x6C || tt == 0xB1 || tt == 0xB2 || tt == 0xB3 || tt == 0xE7 || tt == 0xC3 ||
        (uint8_t) building_needs_area_check(st, id) != 0 || S16(st, b + BLD_TYPE) == 0xC7) {
        U8(st, b + BLD_SHOWS) = 0;
    }
    if (S32(st, G_BUILDING_HIGHEST) <= id) {
        S32(st, G_BUILDING_HIGHEST) = id;
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
