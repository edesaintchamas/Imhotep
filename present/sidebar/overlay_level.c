/* Imhotep - the overlays' building levels shown by the minimap (roadmap phase 7.5).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * With an overlay on, the minimap (0x00439980) colours a building's tiles by a level 0..10 that
 * one of these functions computes from the building record (+5 occupied flag, +0x10 type, service
 * counters) and a few city counts. Only the minimap calls them. One of them (0x00434820) writes
 * the building's +0xC3 flag. */
#include "sidebar.h"

#include "city/day_x87.h"
#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

static uint32_t rec(int32_t id)
{
    return BUILDING_ADDR(id);
}

static int16_t type_of(game_state *st, int32_t id)
{
    return S16(st, rec(id) + BLD_TYPE);
}

static int occupied(game_state *st, int32_t id)
{
    return U8(st, rec(id) + BLD_HOUSE_SIZE) != 0;
}

static int32_t ten_minus_tenth(game_state *st, int32_t id, uint32_t offset)
{
    /* 10 - byte / 10 (a service's remaining coverage, 100 = none needed) */
    return 10 - (int32_t) U8(st, rec(id) + offset) / 10;
}

int32_t overlay_level_entertainment(game_state *st, int32_t tile)
{
    /* @bin 0x004339E0 tile byte 0x00A5D4CC in steps of 10: 0 -> 0, 1..9 -> 1, ..., >= 80 -> 9 */
    int32_t v = U8(st, G_GRID_FERTILITY + (uint32_t) tile);
    if (v == 0) {
        return 0;
    }
    if (v >= 0x50) {
        return 9;
    }
    return v / 10 + 1;
}

int32_t overlay_level_desirability(game_state *st, int32_t tile)
{
    /* @bin 0x00433A60 tile desirability 0x00AC2D4C (i8): 0 for an empty plain tile, else 0..9 by
     * thresholds -10, -5, 0, =0, 5, 10, 15, 20, 25 */
    uint8_t terrain = U8(st, G_GRID_TERRAIN + 4u * (uint32_t) tile);
    int32_t v = S8(st, G_GRID_DESIRABILITY + (uint32_t) tile);
    if ((terrain & 8) == 0 && v == 0) {
        return 0;
    }
    if (v < -10) {
        return 0;
    }
    if (v < -5) {
        return 1;
    }
    if (v < 0) {
        return 2;
    }
    if (v == 0) {
        return 3;
    }
    if (v < 5) {
        return 4;
    }
    if (v < 10) {
        return 5;
    }
    if (v < 15) {
        return 6;
    }
    if (v < 20) {
        return 7;
    }
    return v < 25 ? 8 : 9;
}

static int32_t x87_level(int32_t v, int times4)
{
    /* @bin 0x00433B88 ftol(v [* 4.0f] * 0.0003125f * -9.0f) (x87, 53-bit precision; the float
     * constants 0x0056F550, 0x0056F558, 0x0056F554), then 4 - that, at most 9 */
    x87_val x = x87_from_int(v);
    if (times4) {
        x = x87_int_mul_float(v, 0x40800000u); /* exact */
    }
    x87_val c = {0, 0xA3D70Au | 0x800000u, (int) ((0x39A3D70Au >> 23) & 0xFF) - 150};
    x87_val nine = {1, 9, 0};
    x = x87_mul(x87_mul(x, c), nine);
    int32_t n = 4 - x87_ftol(x);
    return n > 9 ? 9 : n;
}

int32_t overlay_level_resource(game_state *st, int32_t id, int32_t a, int32_t b)
{
    /* @bin 0x00433AE0 a stock level: by type (jump table 0x00433BF8: 0x46 / 0xBE the word at
     * +0x56 + 2 b, 0x47 / 0xBF at +0x56 + 2 a, 0x48 / 0xC1 0x0045D510(id, a), 0x49
     * 0x0045D510(0x00475B50(id), a)), else for an occupied building 4 times the word at
     * +0x54 + 2 b; level = 4 - ftol(v * 0.0003125 * -9) capped at 9 */
    int16_t t = type_of(st, id);
    uint32_t k = (uint32_t) ((int32_t) t - 0x46);
    if (k <= 0x7B) {
        if (t == 0x46 || t == 0xBE) {
            return x87_level(S16(st, rec(id) + BLD_INVENTORY + 2u * (uint32_t) b), 0);
        }
        if (t == 0x47 || t == 0xBF) {
            return x87_level(S16(st, rec(id) + BLD_INVENTORY + 2u * (uint32_t) a), 0);
        }
        if (t == 0x48 || t == 0xC1) {
            return x87_level(GAME_CALL(st, FN_BUILDING_PARTS_STOCK, id, a), 0);
        }
        if (t == 0x49) {
            int32_t r = GAME_CALL(st, FN_BUILDING_MAIN_PART, id, a);
            return x87_level(GAME_CALL(st, FN_BUILDING_PARTS_STOCK, r, a), 0);
        }
    }
    if (!occupied(st, id)) {
        return 0;
    }
    return x87_level(S16(st, rec(id) + BLD_HOUSE_INVENTORY + 2u * (uint32_t) b), 1);
}

static int32_t walker_level(game_state *st, int32_t id, uint32_t offset)
{
    /* @bin 0x00433C90 / 0x00433D50 for a building with a walker in (0x00488030), its drawing tile
     * only: by the view's orientation 0x00BCA2E0 (0, 2, 4, 6) the tile edge byte must be 0x50,
     * 0x40, 0x42, 0x52; the level is the word at offset / 100 */
    if ((GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_IS_TYPE_0X487F80, id) & 0xFF) != 0) {
        uint8_t edge = U8(st, G_GRID_EDGE + (uint32_t) S32(st, G_VIEW_TILE));
        int32_t o = S32(st, G_VIEW_ORIENTATION);
        if (o == 0) {
            if (edge != 0x50) {
                return 0;
            }
        } else if (o == 2) {
            if (edge != 0x40) {
                return 0;
            }
        } else if (o == 4) {
            if (edge != 0x42) {
                return 0;
            }
        } else if (o == 6) {
            if (edge != 0x52) {
                return 0;
            }
        }
    }
    return S16(st, rec(id) + offset) / 100;
}

int32_t overlay_level_crime(game_state *st, int32_t id)
{
    /* @bin 0x00433C90 the word +0x48 / 100, none for types 0xA7 and 0x63 */
    int16_t t = type_of(st, id);
    if (t == 0xA7 || t == 0x63) {
        return 0;
    }
    return walker_level(st, id, 0x48);
}

int32_t overlay_level_tile_word(game_state *st, int32_t tile)
{
    /* @bin 0x00433D30 the tile grid 0x00A9CC1C (i16) / 100 */
    return S16(st, G_GRID_DAMAGE + 2u * (uint32_t) tile) / 100;
}

int32_t overlay_level_damage(game_state *st, int32_t id)
{
    /* @bin 0x00433D50 remembers the tile's image (0x00B7482C) in 0x00D3B5C0; the word +0x46 / 100,
     * none for type 0x51 */
    S32(st, G_GLYPH_LAST) = S32(st, G_GRID_IMAGE + 4u * (uint32_t) S32(st, G_VIEW_TILE));
    if (type_of(st, id) == 0x51) {
        return 0;
    }
    return walker_level(st, id, 0x46);
}

static int32_t occupied_i32_percent(game_state *st, int32_t id, uint32_t offset)
{
    if (!occupied(st, id)) {
        return 0;
    }
    return S32(st, rec(id) + offset) / 100;
}

int32_t overlay_level_0xFC(game_state *st, int32_t id) { return occupied_i32_percent(st, id, 0xFC); } /* @bin 0x00433E00 */
int32_t overlay_level_0xE8(game_state *st, int32_t id) { return occupied_i32_percent(st, id, 0xE8); } /* @bin 0x00433E40 */

int32_t overlay_level_0xE4(game_state *st, int32_t id)
{
    /* @bin 0x00433E80 the i32 +0xE4 / 100, none for types 0x37, 0xB8, 0x63 */
    int16_t t = type_of(st, id);
    if (t == 0x37 || t == 0xB8 || t == 0x63) {
        return 0;
    }
    return occupied_i32_percent(st, id, 0xE4);
}

int32_t overlay_level_religion(game_state *st, int32_t id)
{
    /* @bin 0x00433ED0 remembers the tile's image in 0x00D3B5C0; none for types 0x1E..0x25, 0xE2
     * (jump table 0x00433F94) or with no temple in the city (the counts 0x007057B4..0x007057D0 and
     * 0x00705AC4); else 10 - (i8 +0x80) / 10 */
    S32(st, G_GLYPH_LAST) = S32(st, G_GRID_IMAGE + 4u * (uint32_t) S32(st, G_VIEW_TILE));
    int16_t t = type_of(st, id);
    if ((t >= 0x1E && t <= 0x25) || t == 0xE2) {
        return 0;
    }
    if (!occupied(st, id)) {
        return 0;
    }
    uint32_t sum = U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x25u) + U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x24u) + U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x23u) + U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x22u) +
                   U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x21u) + U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xE2u) + U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x20u) + U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x1Fu) +
                   U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x1Eu);
    if (sum == 0) {
        return 0;
    }
    return 10 - (int32_t) S8(st, rec(id) + BLD_ENTERTAINMENT) / 10;
}

int32_t overlay_level_0x81(game_state *st, int32_t id)
{
    /* @bin 0x00434060 none for types 0x33, 0x35 or without 0x00705808 + 0x00705810; the byte +0x81:
     * 1 -> 8, 2 -> 5, 3 -> 2, else 0 */
    int16_t t = type_of(st, id);
    if (t == 0x33 || t == 0x35 || !occupied(st, id)) {
        return 0;
    }
    if (U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x33u) + U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x35u) == 0) {
        return 0;
    }
    uint8_t v = U8(st, rec(id) + BLD_EDUCATION);
    if (v == 1) {
        return 8;
    }
    if (v == 2) {
        return 5;
    }
    if (v == 3) {
        return 2;
    }
    return 0;
}

int32_t overlay_level_0x6C(game_state *st, int32_t id)
{
    /* @bin 0x004340E0 */
    int16_t t = type_of(st, id);
    if (t == 0x1F || t == 0x24 || !occupied(st, id) || U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x24u) + U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x1Fu) == 0) {
        return 0;
    }
    return ten_minus_tenth(st, id, 0x6C);
}

int32_t overlay_level_0x6D(game_state *st, int32_t id)
{
    /* @bin 0x00434150 */
    int16_t t = type_of(st, id);
    if (t == 0x1E || t == 0x22 || t == 0x24 || !occupied(st, id) ||
        U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x24u) + U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x22u) + U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x1Eu) == 0) {
        return 0;
    }
    return ten_minus_tenth(st, id, 0x6D);
}

int32_t overlay_level_0x6F(game_state *st, int32_t id)
{
    /* @bin 0x004341D0 */
    int16_t t = type_of(st, id);
    if (t == 0x21 || t == 0x22 || t == 0x23 || !occupied(st, id) ||
        U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x23u) + U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x22u) + U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x21u) == 0) {
        return 0;
    }
    return ten_minus_tenth(st, id, 0x6F);
}

int32_t overlay_level_0x71(game_state *st, int32_t id)
{
    /* @bin 0x00434250 */
    int16_t t = type_of(st, id);
    if (t == 0x20 || t == 0x25 || !occupied(st, id) || U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x25u) + U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x20u) == 0) {
        return 0;
    }
    return ten_minus_tenth(st, id, 0x71);
}

int32_t overlay_level_stock(game_state *st, int32_t id)
{
    /* @bin 0x004342C0 a house's food stock against its need: none for types 0x46, 0xBE, 0x4C, 0x47,
     * 0xBF, an empty building or without 0x00705854; need: the type table 0x0070CDF4 (stride 0x58)
     * flags it, the word +0x1C; r = 0x004CDE60(sum of the 8 words at +0x54, need):
     * > 200 -> 0, >= 100 -> 1, > 0 -> 5, else 10; no need: 0 */
    int16_t t = type_of(st, id);
    if (t == 0x46 || t == 0xBE || t == 0x4C || t == 0x47 || t == 0xBF || !occupied(st, id) ||
        S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x46u) == 0) {
        return 0;
    }
    int32_t need = S32(st, G_HOUSE_LEVEL_U28 + 0x58u * (uint32_t) (int32_t) S16(st, rec(id) + BLD_SUBTYPE));
    if (need != 0) {
        need = S16(st, rec(id) + BLD_POPULATION);
    }
    int32_t sum = 0;
    for (uint32_t i = 0; i < 8; i++) {
        sum += S16(st, rec(id) + BLD_HOUSE_INVENTORY + 2 * i);
    }
    if (need == 0) {
        return 0;
    }
    int32_t r = GAME_CALL(st, FN_CALC_RATIO_PERCENT, sum, need);
    if (r > 200) {
        return 0;
    }
    if (r >= 100) {
        return 1;
    }
    return r <= 0 ? 10 : 5;
}

int32_t overlay_level_0x77(game_state *st, int32_t id)
{
    /* @bin 0x004343A0 */
    if (type_of(st, id) == 0xB4 || !occupied(st, id) || S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xB4u) == 0) {
        return 0;
    }
    return ten_minus_tenth(st, id, 0x77);
}

int32_t overlay_level_health(game_state *st, int32_t id)
{
    /* @bin 0x00434400 none for types 0x62, 0x96, 0x3C..0x45, 0xD2 or an empty building, or when the
     * 15 counts 0x0070573C[0x004C06E0(i, j)] (i < 5, j < 3) and 0x00705C78 are all 0; else
     * (i8 +0x83) * 17 / 10 */
    int16_t t = type_of(st, id);
    if (t == 0x62 || t == 0x96 || (t >= 0x3C && t <= 0x45) || t == 0xD2 || !occupied(st, id)) {
        return 0;
    }
    uint32_t sum = 0;
    for (int32_t i = 0; i < 5; i++) {
        for (int32_t j = 0; j < 3; j++) {
            int32_t k = GAME_CALL(st, FN_GOD_BUILDING_TYPE, i, j);
            sum += U32(st, G_TYPE_COUNT_ACTIVE + 4u * (uint32_t) k);
        }
    }
    if (U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x14Fu) + sum == 0) {
        return 0;
    }
    return (int32_t) S8(st, rec(id) + BLD_RELIGION) * 17 / 10;
}

static int32_t service_level(game_state *st, int32_t id, int16_t skip, uint32_t count, uint32_t offset)
{
    if (type_of(st, id) == skip || !occupied(st, id) || S32(st, count) == 0) {
        return 0;
    }
    return ten_minus_tenth(st, id, offset);
}

int32_t overlay_level_0x72(game_state *st, int32_t id) { return service_level(st, id, 0x33, G_TYPE_COUNT_ACTIVE + 4u * 0x33u, 0x72); } /* @bin 0x00434510 */
int32_t overlay_level_0x73(game_state *st, int32_t id) { return service_level(st, id, 0x35, G_TYPE_COUNT_ACTIVE + 4u * 0x35u, 0x73); } /* @bin 0x00434570 */
int32_t overlay_level_0x75(game_state *st, int32_t id) { return service_level(st, id, 0x31, G_TYPE_COUNT_ACTIVE + 4u * 0x31u, 0x75); } /* @bin 0x004345D0 */
int32_t overlay_level_0x6E(game_state *st, int32_t id) { return service_level(st, id, 0x31, G_TYPE_COUNT_ACTIVE + 4u * 0xCEu, 0x6E); } /* @bin 0x00434630 */
int32_t overlay_level_0x76(game_state *st, int32_t id) { return service_level(st, id, 0x2E, G_TYPE_COUNT_ACTIVE + 4u * 0x2Eu, 0x76); } /* @bin 0x00434690 */
int32_t overlay_level_0x78(game_state *st, int32_t id) { return service_level(st, id, 0x2F, G_TYPE_COUNT_ACTIVE + 4u * 0x2Fu, 0x78); } /* @bin 0x004346F0 */
int32_t overlay_level_0x70(game_state *st, int32_t id) { return service_level(st, id, 0xB8, G_TYPE_COUNT_ACTIVE + 4u * 0xB8u, 0x70); } /* @bin 0x00434750 */

int32_t overlay_level_0x46F1C0(game_state *st, int32_t id)
{
    /* @bin 0x004347B0 0x0046F1C0(id) / 25 when occupied and the counts 0x00705A2C, 0x00705A30,
     * 0x00705A28, 0x00705898, 0x00705894 are not all 0 */
    if (!occupied(st, id)) {
        return 0;
    }
    if (U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xBCu) + U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xBDu) + U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xBBu) + U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x57u) +
            U32(st, G_TYPE_COUNT_ACTIVE + 4u * 0x56u) == 0) {
        return 0;
    }
    return GAME_CALL(st, FN_BUILDING_0X46F1C0, id) / 25;
}

static int walker_on_duty(game_state *st, int32_t id)
{
    /* the building's figure +0x2A is of type 4 with +0x38 = 8 and +0x66 set */
    int32_t f = S16(st, rec(id) + BLD_WALKER);
    if (f == 0) {
        return 0;
    }
    uint32_t a = FIGURE_ADDR(f);
    return U8(st, a + FIG_TYPE) == 4 && U8(st, a + FIG_ACTION) == 8 && U8(st, a + FIG_SERVICE_MAX) != 0;
}

int32_t overlay_level_problems(game_state *st, int32_t id)
{
    /* @bin 0x00434820 for an empty building: sets its flag +0xC3 when it has a problem (types 0x5C,
     * 0xB4: +0x35 = 0; a walker in, types 0x6A..0x6D: its walker on duty; types 0x6E..0x72 and
     * those of the table 0x00434944: its walker on duty, or +0x3C <= 0); returns the flag */
    uint32_t b = rec(id);
    if (occupied(st, id)) {
        return 0;
    }
    int16_t t = S16(st, b + BLD_TYPE);
    if (t == 0x5C || t == 0xB4) {
        if (U8(st, b + BLD_WATER_SUPPLY) == 0) {
            U8(st, b + BLD_PROBLEM_OVERLAY) = 1;
        }
        return U8(st, b + BLD_PROBLEM_OVERLAY) != 0;
    }
    if ((GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_IS_TYPE_0X487F80, id) & 0xFF) != 0 || (t >= 0x6A && t <= 0x6D)) {
        if (walker_on_duty(st, id)) {
            U8(st, b + BLD_PROBLEM_OVERLAY) = 1;
        }
        return U8(st, b + BLD_PROBLEM_OVERLAY) != 0;
    }
    int listed = t >= 0x6E && t <= 0x72;
    static const int16_t types[] = {0x20, 0x2F, 0x33, 0x35, 0x4A, 0x71, 0xB1, 0xB2, 0xBB, 0xBC,
                                    0xBD, 0xC2, 0xCB, 0xCC, 0xCD, 0xE2, 0xE7, 0xE8, 0xE9};
    for (unsigned i = 0; i < sizeof(types) / sizeof(types[0]) && !listed; i++) {
        listed = types[i] == t;
    }
    if (listed) {
        if (walker_on_duty(st, id) || S16(st, b + BLD_STORED) <= 0) {
            U8(st, b + BLD_PROBLEM_OVERLAY) = 1;
        }
    }
    return U8(st, b + BLD_PROBLEM_OVERLAY) != 0;
}

int32_t overlay_level_0x18(game_state *st, int32_t id)
{
    /* @bin 0x00434A10 for an empty building: 0x004CDE60(i16 +0x18, 300) / 10 */
    if (occupied(st, id)) {
        return 0;
    }
    return GAME_CALL(st, FN_CALC_RATIO_PERCENT, S16(st, rec(id) + BLD_SERVICE), 0x12C) / 10;
}

int32_t overlay_level_listed(game_state *st, int32_t id)
{
    /* @bin 0x00434A60 1 when the type is in the zero-ended list 0x005D2540 */
    int32_t t = type_of(st, id);
    for (uint32_t a = G_LISTED_TYPES; S32(st, a) != 0; a += 4) {
        if (S32(st, a) == t) {
            return 1;
        }
    }
    return 0;
}

int32_t overlay_level_one(game_state *st, int32_t id)
{
    /* @bin 0x00433C80 */
    (void) st;
    (void) id;
    return 1;
}

/* ---- port table ---- */

#define RUN1(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) { return fn(st, a[0]); }
RUN1(run_entertainment, overlay_level_entertainment)
RUN1(run_desirability, overlay_level_desirability)
RUN1(run_crime, overlay_level_crime)
RUN1(run_tile_word, overlay_level_tile_word)
RUN1(run_damage, overlay_level_damage)
RUN1(run_fc, overlay_level_0xFC)
RUN1(run_e8, overlay_level_0xE8)
RUN1(run_e4, overlay_level_0xE4)
RUN1(run_religion, overlay_level_religion)
RUN1(run_81, overlay_level_0x81)
RUN1(run_6c, overlay_level_0x6C)
RUN1(run_6d, overlay_level_0x6D)
RUN1(run_6f, overlay_level_0x6F)
RUN1(run_71, overlay_level_0x71)
RUN1(run_stock, overlay_level_stock)
RUN1(run_77, overlay_level_0x77)
RUN1(run_health, overlay_level_health)
RUN1(run_72, overlay_level_0x72)
RUN1(run_73, overlay_level_0x73)
RUN1(run_75, overlay_level_0x75)
RUN1(run_6e, overlay_level_0x6E)
RUN1(run_76, overlay_level_0x76)
RUN1(run_78, overlay_level_0x78)
RUN1(run_70, overlay_level_0x70)
RUN1(run_46f1c0, overlay_level_0x46F1C0)
RUN1(run_problems, overlay_level_problems)
RUN1(run_18, overlay_level_0x18)
RUN1(run_listed, overlay_level_listed)
RUN1(run_one, overlay_level_one)

static int32_t run_resource(game_state *st, const int32_t *a) { return overlay_level_resource(st, a[0], a[1], a[2]); }

const ported_function overlay_level_ports[] = {
    {0x004339E0u, "overlay_level_entertainment", 1, 32, 0, run_entertainment, 0},
    {0x00433A60u, "overlay_level_desirability", 1, 32, 0, run_desirability, 0},
    {0x00433AE0u, "overlay_level_resource", 3, 32, 0, run_resource, 0},
    {0x00433C80u, "overlay_level_one", 1, 32, 0, run_one, 0},
    {0x00433C90u, "overlay_level_crime", 1, 32, 0, run_crime, 0},
    {0x00433D30u, "overlay_level_tile_word", 1, 32, 0, run_tile_word, 0},
    {0x00433D50u, "overlay_level_damage", 1, 32, 0, run_damage, 0},
    {0x00433E00u, "overlay_level_0xFC", 1, 32, 0, run_fc, 0},
    {0x00433E40u, "overlay_level_0xE8", 1, 32, 0, run_e8, 0},
    {0x00433E80u, "overlay_level_0xE4", 1, 32, 0, run_e4, 0},
    {0x00433ED0u, "overlay_level_religion", 1, 32, 0, run_religion, 0},
    {0x00434060u, "overlay_level_0x81", 1, 32, 0, run_81, 0},
    {0x004340E0u, "overlay_level_0x6C", 1, 32, 0, run_6c, 0},
    {0x00434150u, "overlay_level_0x6D", 1, 32, 0, run_6d, 0},
    {0x004341D0u, "overlay_level_0x6F", 1, 32, 0, run_6f, 0},
    {0x00434250u, "overlay_level_0x71", 1, 32, 0, run_71, 0},
    {0x004342C0u, "overlay_level_stock", 1, 32, 0, run_stock, 0},
    {0x004343A0u, "overlay_level_0x77", 1, 32, 0, run_77, 0},
    {0x00434400u, "overlay_level_health", 1, 32, 0, run_health, 0},
    {0x00434510u, "overlay_level_0x72", 1, 32, 0, run_72, 0},
    {0x00434570u, "overlay_level_0x73", 1, 32, 0, run_73, 0},
    {0x004345D0u, "overlay_level_0x75", 1, 32, 0, run_75, 0},
    {0x00434630u, "overlay_level_0x6E", 1, 32, 0, run_6e, 0},
    {0x00434690u, "overlay_level_0x76", 1, 32, 0, run_76, 0},
    {0x004346F0u, "overlay_level_0x78", 1, 32, 0, run_78, 0},
    {0x00434750u, "overlay_level_0x70", 1, 32, 0, run_70, 0},
    {0x004347B0u, "overlay_level_0x46F1C0", 1, 32, 0, run_46f1c0, 0},
    {0x00434820u, "overlay_level_problems", 1, 32, 0, run_problems, 0},
    {0x00434A10u, "overlay_level_0x18", 1, 32, 0, run_18, 0},
    {0x00434A60u, "overlay_level_listed", 1, 32, 0, run_listed, 0},
};
const int overlay_level_ports_count = sizeof(overlay_level_ports) / sizeof(overlay_level_ports[0]);
