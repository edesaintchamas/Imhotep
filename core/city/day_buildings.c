/* Imhotep - daily passes over the buildings (day dispatcher 0x00451C50).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/day_buildings.h"

#include "building/day_access.h"
#include "city/labour.h"
#include "map/route_search.h"
#include "city/day_x87.h"
#include "sim/call.h"
#include "sim/math.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


int32_t calc_ratio_percent(int32_t value, int32_t total)
{
    /* @bin 0x004CDE60 value * 100 / total, 0 for total 0 (signed division) */
    if (total == 0) {
        return 0;
    }
    return (int32_t) ((uint32_t) value * 100u) / total;
}

int32_t share_by_weight(game_state *st, int32_t amount, int32_t group, uint32_t table)
{
    /* @bin 0x0045A0A0 table: three groups of 0x1C bytes (i32 count, i32 ids, ..., i32 weight at
     * +0x18); each group sums 0x00705AF0[id] over its ids; the result is
     * ftol(weight[group] / weighted total * amount) in x87 arithmetic */
    if (amount <= 0) {
        return 0;
    }
    int32_t sums[3];
    for (uint32_t g = 0; g < 3; g++) {
        uint32_t t = table + 0x1Cu * g;
        sums[g] = 0;
        for (int32_t k = 0, n = S32(st, t); k < n; k++) {
            sums[g] += S32(st, G_DAY_BUILDINGS_TYPE_COUNT + 4u * U32(st, t + 4 + 4u * (uint32_t) k));
        }
    }
    int32_t total = (int32_t) ((uint32_t) S32(st, table + 0x50) * (uint32_t) sums[2] +
                               (uint32_t) S32(st, table + 0x34) * (uint32_t) sums[1] +
                               (uint32_t) S32(st, table + 0x18) * (uint32_t) sums[0]);
    if (total == 0) {
        return 0;
    }
    int32_t weight = S32(st, table + 0x1Cu * (uint32_t) group + 0x18);
    return x87_ftol(x87_mul_int(x87_div(weight, total), amount));
}

int32_t building_service_share(game_state *st, int32_t id)
{
    /* @bin 0x0045A140 */
    uint32_t b = BUILDING_ADDR(id);
    if (U8(st, b + BLD_STATE) != 1) {
        return 0;
    }
    switch (S16(st, b + BLD_TYPE)) {
    case 0x4D: return share_by_weight(st, S32(st, CURRENT_CITY(st) + CITY_SAVINGS), 0, G_U5DA498);
    case 0x4E: return share_by_weight(st, S32(st, CURRENT_CITY(st) + CITY_SAVINGS), 1, G_U5DA498);
    case 0x4F: return share_by_weight(st, S32(st, CURRENT_CITY(st) + CITY_SAVINGS), 2, G_U5DA498);
    case 0x56: case 0x57: return share_by_weight(st, S32(st, CURRENT_CITY(st) + CITY_TREASURY), 0, G_U5DA4F0);
    case 0xB8: return share_by_weight(st, S32(st, CURRENT_CITY(st) + CITY_TREASURY), 1, G_U5DA4F0);
    case 0x54: case 0x55: case 0xBB: case 0xBC: case 0xBD:
        return share_by_weight(st, S32(st, CURRENT_CITY(st) + CITY_TREASURY), 2, G_U5DA4F0);
    default: return 0;
    }
}

void day_service_shares(game_state *st)
{
    /* @bin 0x004684E0 day step 0x22: +0xB8 := 0x0045A140(id) for buildings with +5 clear */
    for (int32_t id = 1; id <= S32(st, G_BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) != 1) {
            continue;
        }
        S32(st, b + BLD_UB8) = 0;
        if (U8(st, b + BLD_HOUSE_SIZE) == 0) {
            S32(st, b + BLD_UB8) = building_service_share(st, id);
        }
    }
}

void day_decay_0x18(game_state *st)
{
    /* @bin 0x004558B0 day step 0xC: i16 +0x18 down by one (to 0) for every used record but types
     * 0x3B, 0xAC and 0xAD; the highest id is read once */
    int32_t n = S32(st, G_BUILDING_HIGHEST);
    for (int32_t id = 1; id <= n; id++) {
        uint32_t b = BUILDING_ADDR(id);
        int16_t type = S16(st, b + BLD_TYPE);
        if (U8(st, b + BLD_STATE) == 0 || type == 0x3B || type == 0xAC || type == 0xAD) {
            continue;
        }
        int16_t v = S16(st, b + BLD_SERVICE);
        S16(st, b + BLD_SERVICE) = v > 1 ? (int16_t) (v - 1) : 0;
    }
}

int32_t building_road_access_4(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x004806A0 */
    return building_road_access(st, x, y, 4);
}

int32_t area_fertility_489050(game_state *st, int32_t x, int32_t y, int32_t width, int32_t rows)
{
    /* @bin 0x00489050 (thiscall on 0x00BCA350) the average fertility (grid 0x00A5D4CC, + 1 per
     * tile) of the tiles of offset table 0x005CC8C0 around (x, y), + 1: x87 trunc(sum / count),
     * the count a double from 0.0 (0x0056F5A0) adding 1.0 per tile */
    int32_t base = S32(st, G_MAP_START) + y * GRID_SIZE + x;
    S32(st, G_TILE_CURSOR) = base;
    x87_val count = x87_from_double(0); /* 0x0056F5A0: 0.0 */
    x87_val one = x87_from_double(0x3FF0000000000000ull); /* 0x0056F520: 1.0 */
    int32_t sum = 0;
    for (int32_t i = 0; i < rows; i++) {
        uint32_t row = G_FOOTPRINT_OFFSETS + 0x30u * (uint32_t) i;
        for (int32_t j = 0; j < width; j++) {
            count = x87_add(count, one);
            sum += U8(st, G_FERTILITY_GRID + (uint32_t) (base + S32(st, row + 8u * (uint32_t) j))) + 1;
        }
    }
    /* sum / 0.0 (no tiles): the indefinite integer, low half 0, then + 1 */
    return (count.m == 0 ? 0 : x87_ftol(x87_divv(x87_from_int(sum), count))) + 1;
}

int32_t building_road_flag_4890f0(game_state *st, int32_t id)
{
    /* @bin 0x004890F0 (thiscall) +0x35 set */
    return U8(st, BUILDING_ADDR(id) + BLD_WATER_SUPPLY) != 0;
}

int32_t farm_fertility_489110(game_state *st, int32_t id)
{
    /* @bin 0x00489110 (thiscall on 0x00BCA350) the 3 x 3 fertility, + 20 (+ 40 without the area
     * test 0x004894A0) for working farms of the types of 0x00488080 with +0x35; at most 99 */
    uint32_t b = BUILDING_ADDR(id);
    int32_t r = area_fertility_489050(st, S16(st, b + BLD_X), S16(st, b + BLD_Y), 3, 3);
    /* @bin 0x00488080 the same types as 0x00487F80 */
    if (building_road_flag_4890f0(st, id) && building_type_0x487f80(S16(st, b + BLD_TYPE))) {
        r += building_area_test_3x3(st, id) ? 0x14 : 0x28;
    }
    return r > 0x63 ? 0x63 : r;
}

int32_t trade_tax_4c5340(game_state *st, int32_t player)
{
    /* @bin 0x004C5340 20 with the kingdom building of level 4, or of level 0 that is 0x004C52D0
     * of kind 1; else 25 */
    uint32_t c = CITY_ADDR(player);
    if (S16(st, c + CITY_U4478) != 0 && GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, S32(st, c + CITY_U472C)) == 4) {
        return 0x14;
    }
    c = CURRENT_CITY(st);
    if (S16(st, c + CITY_U4478) != 0 && GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, S32(st, c + CITY_U472C)) == 0 &&
        (int8_t) GAME_CALL(st, FN_BUILDING_0XA9_IS, S32(st, CURRENT_CITY(st) + CITY_U472C), 1) != 0) {
        return 0x14;
    }
    return 0x19;
}

void day_totals_45d630(game_state *st, int32_t player)
{
    /* @bin 0x0045D630 */
    uint32_t c = CITY_ADDR(player);
    int32_t local[9] = {0};
    for (uint32_t k = 0; k < 9; k++) {
        S32(st, c + CITY_GRANARY_FOOD + 4 * k) = 0;
    }
    S32(st, c + CITY_FOOD_TOTAL) = 0;
    S32(st, c + CITY_FOOD_TYPES) = 0;
    S32(st, c + CITY_U2B18) = 0;
    S32(st, c + CITY_U2B1C) = 0;
    S32(st, c + CITY_U45B0) = 0;
    S32(st, c + CITY_U45B4) = 0;
    S32(st, c + CITY_U45B8) = 0;
    int32_t sum_62 = 0, sum_66 = 0;
    for (int32_t id = 1; id <= S32(st, G_BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) != 1) {
            continue;
        }
        int16_t type = S16(st, b + BLD_TYPE);
        if (type != 0x47 && type != 0xBF) {
            /* @bin 0x0045D6D8 other buildings with +5 and +0x1C set add their eight i16 at +0x54 */
            if (U8(st, b + BLD_HOUSE_SIZE) == 0 || S16(st, b + BLD_POPULATION) == 0) {
                continue;
            }
            for (uint32_t i = 0; i < 8; i++) {
                local[1 + i] += S16(st, b + BLD_HOUSE_INVENTORY + 2 * i);
            }
            continue;
        }
        /* @bin 0x0045D76F types 0x47 and 0xBF */
        U8(st, b + BLD_U44) = 0;
        if (!building_road_access_4(st, S16(st, b + BLD_X), S16(st, b + BLD_Y))) {
            continue;
        }
        U8(st, b + BLD_U44) = 1;
        int32_t pct = calc_ratio_percent(S16(st, b + BLD_WORKERS), S32(st, G_TYPE_WORKERS + 0x28u * (uint32_t) (int32_t) type));
        if (pct < 100) {
            S32(st, c + CITY_U45B0) += 1;
        }
        if (pct < 50) {
            int32_t sum = 0;
            for (uint32_t i = 0; i < 8; i++) {
                sum += S16(st, b + BLD_MONUMENT_STAGE + 2 * i);
            }
            S32(st, c + CITY_U45B4) += 1;
            if (sum > 0) {
                S32(st, c + CITY_U45B8) += 1;
            }
            continue;
        }
        sum_62 += S16(st, b + BLD_U62);
        sum_66 += S16(st, b + BLD_U66);
        for (uint32_t k = 0; k < 9; k++) {
            S32(st, c + CITY_GRANARY_FOOD + 4 * k) += S16(st, b + BLD_INVENTORY + 2 * k);
        }
        S32(st, c + CITY_U2B1C) += 1;
        if (S32(st, G_CUSTOM) != 0) {
            continue;
        }
        /* @bin 0x0045D856 two one-time messages of the scenario 0x00E38EA8 */
        if ((int32_t) S8(st, G_MISSION) == S32(st, G_MISSION_LISTS + G_MISSION_LIST_STRIDE * 2) && U8(st, G_MISSION_LIST_FLAGS + 2) == 0 &&
            U8(st, G_MISSION_LIST_FLAGS + 1) != 0 && sum_66 >= 400) {
            GAME_CALL(st, FN_TUTORIAL_STEP, 2);
            if (S32(st, G_CUSTOM) != 0) {
                continue;
            }
        }
        if ((int32_t) S8(st, G_MISSION) == S32(st, G_MISSION_LISTS + G_MISSION_LIST_STRIDE * 7) && U8(st, G_MISSION_LIST_FLAGS + 7) == 0 && sum_62 >= 800) {
            GAME_CALL(st, FN_TUTORIAL_STEP, 7);
        }
    }
    /* @bin 0x0045D8E8 */
    for (uint32_t i = 0; i < 8; i++) {
        int32_t v = S32(st, c + CITY_GRANARY_FOOD + 4 + 4 * i);
        if (v != 0 || local[1 + i] != 0) {
            S32(st, c + CITY_FOOD_TOTAL) += v;
            S32(st, c + CITY_FOOD_TYPES) += 1;
        }
    }
    int32_t r = trade_tax_4c5340(st, player);
    int32_t div = calc_percentage(S32(st, c + CITY_POPULATION), r);
    S32(st, c + CITY_U45AC) = div;
    if (div <= 0) {
        S32(st, c + CITY_U2B18) = S32(st, c + CITY_FOOD_TOTAL) > 0;
    } else {
        S32(st, c + CITY_U2B18) = S32(st, c + CITY_FOOD_TOTAL) / div;
    }
    if (S32(st, G_U784BBC) != 0) {
        S32(st, c + CITY_FOOD_TYPES) = 1;
        S32(st, c + CITY_U2B18) = 0xC;
    }
}

void day_step_0x11(game_state *st)
{
    /* @bin 0x0045D5D0 */
    if (U8(st, CITY_ADDR(1) + CITY_ACTIVE) != 0) {
        day_totals_45d630(st, 1);
    }
    int32_t n = S32(st, G_BUILDING_HIGHEST);
    int32_t flag = S32(st, G_U784BBC);
    for (int32_t id = 1; id <= n; id++) {
        uint32_t b = BUILDING_ADDR(id);
        int16_t type = S16(st, b + BLD_TYPE);
        if (U8(st, b + BLD_STATE) == 1 && (type == 0x46 || type == 0xBE) && flag != 0) {
            S16(st, b + BLD_INVENTORY) = 200;
        }
    }
}

void buildings_highest_update(game_state *st)
{
    /* @bin 0x0045AA30 day step 0xA: the highest used building id (1..3999) and the highest ever;
     * 0x0078D788 the highest used record of the 199 at 0x00790184 (stride 0xC4) */
    int32_t highest = 0;
    S32(st, G_BUILDING_HIGHEST) = 0;
    for (int32_t id = 1; id < 4000; id++) {
        if (U8(st, BUILDING_ADDR(id) + BLD_STATE) != 0) {
            highest = id;
        }
    }
    S32(st, G_BUILDING_HIGHEST) = highest;
    if (highest > S32(st, G_BUILDING_HIGHEST_EVER)) {
        S32(st, G_BUILDING_HIGHEST_EVER) = highest;
    }
    S32(st, G_STORAGE_HIGHEST) = 0;
    for (int32_t i = 1; i < 200; i++) {
        if (U8(st, G_STORAGE_SETTINGS + 0xC4u + 0xC4u * (uint32_t) (i - 1)) != 0) {
            S32(st, G_STORAGE_HIGHEST) = i;
        }
    }
}

void spawn_figure_3(game_state *st, int32_t player, int32_t x, int32_t y, int32_t amount)
{
    /* @bin 0x00459020 a figure of type 3 at (x, y): +0x38 := 8, +0x36 := 0, +0x64 := amount;
     * then 0x004E9250(player, amount) */
    int32_t f = GAME_CALL(st, FN_FIGURE_CREATE, player, 3, x, y, 0);
    uint32_t a = FIGURE_ADDR(f);
    U8(st, a + FIG_ACTION) = 8;
    S16(st, a + FIG_WAIT_TICKS) = 0;
    U8(st, a + FIG_MIGRANT_PEOPLE) = (uint8_t) amount;
    GAME_CALL(st, FN_CITY_PEOPLE_ADD_4E9250, player, amount);
}

void day_list_45d83c(game_state *st)
{
    /* @bin 0x00459070 day step 0x18: the buildings listed at 0x0078E178 (count 0x0078D83C, index
     * 0x0078D840 kept in the global) with a negative +0x1E spawn 0x00459020 with -(+0x1E), which
     * is taken from +0x1C; at 0 the building's state becomes 2 */
    S32(st, G_HOUSE_LIST_INDEX) = 0;
    while (S32(st, G_HOUSE_LIST_INDEX) < S32(st, G_HOUSE_LIST_COUNT)) {
        int32_t i = S32(st, G_HOUSE_LIST_INDEX);
        int32_t id = S16(st, G_HOUSE_LIST + 2u * (uint32_t) i);
        S32(st, G_HOUSE_LIST_INDEX) = i + 1;
        uint32_t b = BUILDING_ADDR(id);
        int16_t v = S16(st, b + BLD_ROOM_LEFT);
        if (v >= 0) {
            continue;
        }
        int32_t amount = -(int32_t) v;
        spawn_figure_3(st, S32(st, CURRENT_PLAYER_ADDR), S16(st, b + BLD_X), S16(st, b + BLD_Y), amount);
        int16_t left = S16(st, b + BLD_POPULATION);
        if (amount >= left) {
            U8(st, b + BLD_STATE) = 2;
        } else {
            S16(st, b + BLD_POPULATION) = (int16_t) (left - amount);
        }
    }
}

/* 0x00469D50 (a test on the scenario's text 0x133, read with 0x004CC110) stays with the
 * original: the game's text buffer (0x00E36C70) is loaded from the game's text files into memory
 * outside the simulation state */

void building_spawn_469e40(game_state *st, int32_t id, int32_t flag)
{
    /* @bin 0x00469E40 the building's figure +0x2A: kept while alive, of type 0x17 (then 0x18)
     * or 0, and tied to the building (+0x5A); else a new one of type 0x17 (0x18 with flag) on a
     * road within 2 tiles */
    uint32_t b = BUILDING_ADDR(id);
    for (int pass = 0; pass < 2; pass++) {
        int16_t fid = S16(st, b + BLD_WALKER);
        if (fid <= 0) {
            continue;
        }
        uint32_t f = FIGURE_ADDR(fid);
        uint8_t type = U8(st, f + FIG_TYPE);
        if (U8(st, f + FIG_STATE) != 0 && (type == (pass ? 0x18 : 0x17) || type == 0) && S16(st, f + FIG_HOME) == id) {
            return;
        }
        S16(st, b + BLD_WALKER) = 0;
    }
    if (!building_road_within(st, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE), 2)) {
        return;
    }
    int32_t fid = GAME_CALL(st, FN_FIGURE_CREATE, U8(st, b + BLD_OWNER), (uint8_t) flag != 0 ? 0x18 : 0x17,
                            S32(st, ROAD_ACCESS_X), S32(st, ROAD_ACCESS_Y), 4);
    if (fid == 0) {
        return;
    }
    S16(st, b + BLD_WALKER) = (int16_t) fid;
    uint32_t f = FIGURE_ADDR(fid);
    U8(st, f + FIG_ACTION) = 8;
    S16(st, f + FIG_HOME) = (int16_t) id;
    S16(st, f + FIG_WAIT_TICKS) = (int16_t) ((U8(st, b + BLD_RANDOM) & 0xF) + 10);
}

void day_spawn_469b90(game_state *st)
{
    /* @bin 0x00469B90 day step 0x2D: every 10 days (0x00978D80), up to population * 0.02 (x87)
     * buildings with +5, +0x1C and +0xE4 == 1000 call 0x00469E40 (flag for the first three when
     * 0x00469D50 holds) and reload +0xE4 from their +0x12 entry of table 0x0070CE0C */
    S32(st, G_U978D80) += 1;
    if (S32(st, G_U978D80) < 10) {
        return;
    }
    S32(st, G_U978D80) = 0;
    int32_t n = x87_ftol(x87_int_mul_float(S32(st, CURRENT_CITY(st) + CITY_POPULATION), U32(st, G_U56F568)));
    int32_t first = 3;
    for (int32_t id = 1; id <= S32(st, G_BUILDING_HIGHEST) && n != 0; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) != 1 || U8(st, b + BLD_HOUSE_SIZE) == 0 || S16(st, b + BLD_POPULATION) == 0 || S32(st, b + BLD_CRIME_RISK) != 1000) {
            continue;
        }
        int32_t flag = (int8_t) GAME_CALL0(st, FN_MISSION_GOALS_PENDING) != 0 && first != 0;
        building_spawn_469e40(st, id, flag);
        n--;
        S32(st, b + BLD_CRIME_RISK) = S32(st, G_HOUSE_LEVEL_U40 + 0x58u * (uint32_t) (int32_t) S16(st, b + BLD_SUBTYPE));
        if (first > 0) {
            first--;
        }
    }
}

void city1_clear_4e8610(game_state *st)
{
    /* @bin 0x004E8610 */
    if (U8(st, CITY_ADDR(1) + CITY_ACTIVE) != 0) {
        S32(st, CITY_ADDR(1) + CITY_HOUSE_CAPACITY) = 0;
        S32(st, CITY_ADDR(1) + CITY_HOUSE_ROOM) = 0;
    }
}

void buildings_list_active(game_state *st)
{
    /* @bin 0x004583F0 the list 0x0078E178 (i16, count 0x0078D83C) of used buildings with +5 */
    int32_t n = S32(st, G_BUILDING_HIGHEST), count = 0;
    S32(st, G_HOUSE_LIST_COUNT) = 0;
    if (n < 1) {
        return;
    }
    for (int32_t id = 1; id <= n; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) == 1 && U8(st, b + BLD_HOUSE_SIZE) != 0) {
            S16(st, G_HOUSE_LIST + 2u * (uint32_t) count) = (int16_t) id;
            count++;
        }
    }
    S32(st, G_HOUSE_LIST_COUNT) = count;
}

void day_step_0x16(game_state *st)
{
    /* @bin 0x00458440 */
    city1_clear_4e8610(st);
    buildings_list_active(st);
    int32_t count = S32(st, G_HOUSE_LIST_COUNT);
    S32(st, G_HOUSE_LIST_INDEX) = 0;
    if (count <= 0) {
        return;
    }
    S32(st, G_HOUSE_LIST_INDEX) = count;
    for (int32_t i = 0; i < count; i++) {
        uint32_t b = BUILDING_ADDR(S16(st, G_HOUSE_LIST + 2u * (uint32_t) i));
        S16(st, b + BLD_ROOM_LEFT) = 0;
        if (S16(st, b + BLD_DISTANCE) <= 0) {
            continue;
        }
        /* @bin 0x0045849B the type's amount (table 0x0070CE14 by +0x12), times 4 with +4 */
        int32_t v = S32(st, G_HOUSE_LEVEL_U48 + 0x58u * (uint32_t) (int32_t) S16(st, b + BLD_SUBTYPE));
        if (U8(st, b + BLD_HOUSE_MERGED) != 0) {
            v = (int32_t) ((uint32_t) v << 2);
        }
        uint32_t c = CURRENT_CITY(st);
        int16_t have = S16(st, b + BLD_POPULATION);
        S32(st, c + CITY_HOUSE_CAPACITY) += v;
        S32(st, c + CITY_HOUSE_ROOM) += v - have;
        if (U8(st, b + BLD_NO_ROOM) == 0) {
            S16(st, b + BLD_ROOM_LEFT) = (int16_t) (v - have);
        }
        if (S16(st, b + BLD_POPULATION) > S16(st, b + BLD_MAX_POPULATION)) {
            S16(st, b + BLD_MAX_POPULATION) = S16(st, b + BLD_POPULATION);
        }
    }
}

void day_step_0x1c(game_state *st)
{
    /* @bin 0x00455430 type 0x5C buildings listed at 0x0078D9A8 (count 0x0078D7F0, at most 499
     * kept); others with +5 clear and building_is_type_0x487f80: +0x35 := a tile with terrain
     * 0x1000000 within size; with +5: +0x35 := +0x77 != 0, +0x3F := 0. Then 0x0047FC40(id, 2)
     * for the listed ones (index 0x0078D7F4) */
    S32(st, G_WORK_COUNT) = 0;
    for (int32_t id = 1; id <= S32(st, G_BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) != 1) {
            continue;
        }
        if (S16(st, b + BLD_TYPE) == 0x5C) {
            int32_t k = S32(st, G_WORK_COUNT);
            S16(st, G_WORK_LIST + 2u * (uint32_t) k) = (int16_t) id;
            S32(st, G_WORK_COUNT) = k + 1 < 0x1F4 ? k + 1 : 0x1F3;
        } else if (U8(st, b + BLD_HOUSE_SIZE) == 0) {
            if (building_is_type_0x487f80(st, id)) {
                U8(st, b + BLD_WATER_SUPPLY) = map_area_has_terrain(st, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE), 0x1000000) ? 1 : 0;
            }
        } else {
            U8(st, b + BLD_WATER_SUPPLY) = 0;
            U8(st, b + BLD_WELL_ACCESS) = 0;
            if (U8(st, b + BLD_ACCESS_WATER) != 0) {
                U8(st, b + BLD_WATER_SUPPLY) = 1;
            }
        }
    }
    if (S32(st, G_WORK_COUNT) <= 0) {
        return;
    }
    S32(st, G_WORK_INDEX) = 0;
    while (S32(st, G_WORK_INDEX) < S32(st, G_WORK_COUNT)) {
        int32_t i = S32(st, G_WORK_INDEX);
        int32_t id = S16(st, G_WORK_LIST + 2u * (uint32_t) i);
        S32(st, G_WORK_INDEX) = i + 1;
        map_mark_buildings_around(st, id, 2);
    }
}

void buildings_water_level(game_state *st)
{
    /* @bin 0x004539E0 day step 0x26: +0xBE := the highest i8 of grid 0x00AC2D4C under the
     * building (one tile: that tile; else the size x size offsets of 0x005CC8C0, from -9999),
     * + 10 with +0xC0, + 10 / 12 / 14 / 16 / 18 by grid 0x00AB623C (1, 2, 3, 4, 5 and more) */
    int32_t n = S32(st, G_BUILDING_HIGHEST);
    for (int32_t id = 1; id <= n; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) != 1) {
            continue;
        }
        int32_t tile = S32(st, b + BLD_GRID_OFFSET);
        uint32_t size = U8(st, b + BLD_SIZE);
        if (size == 1) {
            U8(st, b + BLD_DESIRABILITY) = U8(st, G_DESIRABILITY_GRID + (uint32_t) tile);
        } else {
            int32_t best = -9999;
            for (uint32_t i = 0; i < size; i++) {
                for (uint32_t j = 0; j < size; j++) {
                    int32_t v = S8(st, G_DESIRABILITY_GRID + (uint32_t) (tile + S32(st, G_FOOTPRINT_OFFSETS + 0x30u * i + 8u * j)));
                    if (v > best) {
                        best = v;
                    }
                }
            }
            U8(st, b + BLD_DESIRABILITY) = (uint8_t) best;
        }
        if (U8(st, b + BLD_NEAR_WATER) != 0) {
            U8(st, b + BLD_DESIRABILITY) += 10;
        }
        uint32_t k = U8(st, G_ELEVATION_GRID + (uint32_t) tile);
        if (k == 1) {
            U8(st, b + BLD_DESIRABILITY) += 10;
        } else if (k == 2) {
            U8(st, b + BLD_DESIRABILITY) += 12;
        } else if (k == 3) {
            U8(st, b + BLD_DESIRABILITY) += 14;
        } else if (k == 4) {
            U8(st, b + BLD_DESIRABILITY) += 16;
        } else if (k >= 5) {
            U8(st, b + BLD_DESIRABILITY) += 18;
        }
    }
}

void day_stock_totals_45e040(game_state *st)
{
    /* @bin 0x0045E040 day step 8: totals of the eight i16 at +0x58 of the type 0x47 / 0xBF
     * buildings with +0x44 and +0x20 > 0 (0x0078D848 + 4 * k), skipping the goods their record
     * 0x007900C0 + 0xC4 * (+0xC1) marks 2; those holding more than 100 listed at 0x0078D8D4 (count
     * 0x0078D9A0, at most 99; the last slot is overwritten) */
    for (uint32_t k = 0; k < 0x32; k++) {
        S32(st, G_STORAGE_BUILDINGS + 4 * k) = 0;
    }
    for (uint32_t k = 0; k < 0x24; k++) {
        S32(st, G_U78D844 + 4 * k) = 0;
    }
    S32(st, G_STORAGE_LIST_COUNT) = 0;
    uint32_t slot = G_STORAGE_BUILDINGS;
    for (int32_t id = 1; id <= S32(st, G_BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        int16_t type = S16(st, b + BLD_TYPE);
        if (U8(st, b + BLD_STATE) != 1 || (type != 0x47 && type != 0xBF) || U8(st, b + BLD_U44) == 0 || S16(st, b + BLD_DISTANCE) <= 0) {
            continue;
        }
        uint32_t rec = G_STORAGE_SETTINGS + 0xC4u * U8(st, b + BLD_STORAGE_ID);
        int32_t sum = 0;
        for (uint32_t k = 1; k <= 8; k++) {
            if (U8(st, rec + 2 + k) == 2) {
                continue;
            }
            int32_t v = S16(st, b + BLD_INVENTORY + 2 * k);
            sum += v;
            S32(st, G_U78D844 + 4 * k) += v;
        }
        if (sum > 100) {
            S16(st, slot) = (int16_t) id;
            if (slot < G_STORAGE_BUILDINGS + 2 * 98) {
                S32(st, G_STORAGE_LIST_COUNT) += 1;
                slot += 2;
            }
        }
    }
}

void labour_categories_need(game_state *st)
{
    /* @bin 0x00459100 day step 0x19: per labour category of the current city (0x14 bytes at
     * +0x2B2C): workers needed by its buildings, workers they have, buildings; each building's
     * category in +0x42 (-1 for category 0 with building_needs_area_check) */
    uint32_t cats = CURRENT_CITY(st) + CITY_LABOUR_NEEDED;
    for (uint32_t k = 0; k < 10; k++) {
        S32(st, cats + CITY_LABOUR_STRIDE * k + (CITY_LABOUR_BUILDINGS - CITY_LABOUR_NEEDED)) = 0;
        S32(st, cats + CITY_LABOUR_STRIDE * k + (CITY_LABOUR_WORKERS - CITY_LABOUR_NEEDED)) = 0;
        S32(st, cats + CITY_LABOUR_STRIDE * k + (CITY_LABOUR_AVAILABLE - CITY_LABOUR_NEEDED)) = 0;
        S32(st, cats + CITY_LABOUR_STRIDE * k) = 0;
    }
    for (int32_t id = 1; id <= S32(st, G_BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) != 1) {
            continue;
        }
        int16_t type = S16(st, b + BLD_TYPE);
        int32_t cat = S32(st, G_TYPE_LABOUR_CATEGORY + 0x14u * (uint32_t) (int32_t) type);
        if (cat == 0 && building_needs_area_check(st, id)) {
            cat = -1;
        }
        U8(st, b + BLD_U42) = (uint8_t) cat;
        if (cat < 0) {
            continue;
        }
        if (cat == 3) {
            if (S16(st, b + BLD_PREV_PART) != 0) {
                continue;
            }
        } else if (cat == 1 || cat == 0) {
            if (building_resource_in_stock(st, id)) {
                continue;
            }
        }
        int16_t workers = S16(st, b + BLD_SERVICE);
        if (workers <= 0) {
            continue;
        }
        uint32_t rec = CURRENT_CITY(st) + CITY_LABOUR_NEEDED + CITY_LABOUR_STRIDE * (uint32_t) cat;
        S32(st, rec) += S32(st, G_TYPE_WORKERS + 0x28u * (uint32_t) (int32_t) type);
        S32(st, rec + (CITY_LABOUR_WORKERS - CITY_LABOUR_NEEDED)) += workers;
        S32(st, rec + (CITY_LABOUR_BUILDINGS - CITY_LABOUR_NEEDED)) += 1;
    }
}


void road_network_fill(game_state *st, int32_t tile, int32_t id)
{
    /* @bin 0x004689E0 flood fill of the network from `tile` over the routing grid 0x00EE0C50
     * (0 and 1, or 2 with terrain bit 0x400), in a ring queue; 0x0078C400 counts the tiles; the
     * second grid gets 0x0078C408, which goes up every 30 first-neighbour steps (0x0078C404) */
    for (uint32_t k = 0; k < 0x3E8; k++) {
        S32(st, G_DAY_WATER_QUEUE + 4 * k) = 0;
    }
    int32_t tail = 0;
    int32_t mark = S32(st, G_U78C408);
    S32(st, G_QUEUE_HEAD) = 0;
    S32(st, G_QUEUE_TAIL) = 0;
    int32_t at = tile;
    for (int32_t steps = 1;; steps++) {
        U8(st, G_NETWORK2 + (uint32_t) at) = (uint8_t) mark;
        U8(st, G_GRID_ROAD_NETWORK + (uint32_t) at) = (uint8_t) id;
        int32_t next = -1;
        for (uint32_t d = 0; d < 4; d++) {
            int32_t t = at + S32(st, G_U5CD678 + 4 * d);
            int8_t c = S8(st, G_GRID_ROUTING + (uint32_t) t);
            if (c < 0 || c > 2) {
                continue;
            }
            if (c == 2 && !(U32(st, G_GRID_TERRAIN + 4u * (uint32_t) t) & 0x400)) {
                continue;
            }
            if (U8(st, G_GRID_ROAD_NETWORK + (uint32_t) t) != 0) {
                continue;
            }
            U8(st, G_NETWORK2 + (uint32_t) t) = (uint8_t) mark;
            U8(st, G_GRID_ROAD_NETWORK + (uint32_t) t) = (uint8_t) id;
            S32(st, G_U78C400) += 1;
            if (next == -1) {
                next = t;
                S32(st, G_U78C404) += 1;
            } else {
                S32(st, G_DAY_WATER_QUEUE + 4u * (uint32_t) tail) = t;
                if (++tail >= 0x3E8) {
                    tail = 0;
                }
            }
        }
        if (next == -1) {
            int32_t head = S32(st, G_QUEUE_HEAD);
            if (head == tail) {
                break;
            }
            next = S32(st, G_DAY_WATER_QUEUE + 4u * (uint32_t) head);
            S32(st, G_QUEUE_HEAD) = head + 1 < 0x3E8 ? head + 1 : 0;
        }
        if (S32(st, G_U78C404) > 0x1E) {
            mark++;
            S32(st, G_U78C404) = 0;
        }
        at = next;
        if (next <= -1 || steps >= 0xCB10) {
            break;
        }
    }
    S32(st, G_QUEUE_TAIL) = tail;
    S32(st, G_U78C408) = mark;
}

void road_networks_update(game_state *st)
{
    /* @bin 0x00468860 day step 7: number the road networks (terrain & 0x44 == 0x40) from 1 and
     * keep the ten largest in the current city's list (+0x46AC: id, tile count) */
    uint32_t list = CURRENT_CITY(st) + CITY_ROAD_NETWORKS;
    for (uint32_t k = 0; k < 10; k++) {
        S32(st, list + 8 * k) = 0;
        S32(st, list + 8 * k + (CITY_ROAD_NETWORK_TILES - CITY_ROAD_NETWORKS)) = 0;
    }
    for (uint32_t k = 0; k < 0xCB10; k++) {
        U8(st, G_GRID_ROAD_NETWORK + k) = 0;
    }
    for (uint32_t k = 0; k < 0xCB10; k++) {
        U8(st, G_NETWORK2 + k) = 0;
    }
    int32_t id = 1;
    S32(st, G_U78C408) = 1;
    S32(st, G_U78C404) = 0;
    int32_t tile = S32(st, G_MAP_START);
    S32(st, G_TILE_CURSOR) = tile;
    for (int32_t y = 0; y < S32(st, G_MAP_HEIGHT); y++) {
        for (int32_t x = 0; x < S32(st, G_MAP_WIDTH); x++) {
            if ((U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile) & 0x44) == 0x40 && U8(st, G_GRID_ROAD_NETWORK + (uint32_t) tile) == 0) {
                S32(st, G_U78C400) = 1;
                road_network_fill(st, tile, id);
                uint32_t c = CURRENT_CITY(st) + CITY_ROAD_NETWORKS;
                int32_t size = S32(st, G_U78C400);
                int32_t k = 0;
                while (k < 10 && size <= S32(st, c + 8u * (uint32_t) k + (CITY_ROAD_NETWORK_TILES - CITY_ROAD_NETWORKS))) {
                    k++;
                }
                if (k < 10) {
                    for (int32_t j = 9; j > k; j--) {
                        S32(st, c + 8u * (uint32_t) j) = S32(st, c + 8u * (uint32_t) (j - 1));
                        S32(st, c + 8u * (uint32_t) j + (CITY_ROAD_NETWORK_TILES - CITY_ROAD_NETWORKS)) = S32(st, c + 8u * (uint32_t) (j - 1) + (CITY_ROAD_NETWORK_TILES - CITY_ROAD_NETWORKS));
                    }
                    S32(st, c + 8u * (uint32_t) k) = id;
                    S32(st, c + 8u * (uint32_t) k + (CITY_ROAD_NETWORK_TILES - CITY_ROAD_NETWORKS)) = size;
                }
                tile = S32(st, G_TILE_CURSOR);
                id++;
            }
            tile++;
            S32(st, G_TILE_CURSOR) = tile;
        }
        tile += S32(st, G_MAP_STRIDE_EXTRA);
        S32(st, G_TILE_CURSOR) = tile;
    }
}

void farms_season_488630(game_state *st, uint32_t self)
{
    /* @bin 0x00488630 (thiscall on 0x00BCA350) on day 0 of a month: ten flags at `self` by the
     * month table 0x0056F5B8 (ten rows of four months, -1 ends a row): a row naming the month
     * before (by (month - 1) % 11) clears its flag (and the scan goes on from row 0 with the next
     * column, as the original does), a row naming this month sets it */
    int32_t prev = (S32(st, G_MONTH) - 1) % 11;
    if (S32(st, G_TICK_DAY) != 0) {
        return;
    }
    for (int32_t row = 0; row < 10; row++) {
        for (int32_t col = 0; col < 4; col++) {
            int32_t m = S32(st, G_U56F5B8 + 4u * (uint32_t) (col + 4 * row));
            if (m == prev && P8(st, self + (uint32_t) row) != 0) {
                P8(st, self + (uint32_t) row) = 0;
                row = 0;
                continue;
            }
            if (m == -1) {
                break;
            }
            if (m == S32(st, G_MONTH)) {
                P8(st, self + (uint32_t) row) = 1;
                break;
            }
        }
    }
}

int32_t farm_grow_488350(game_state *st, uint32_t self, int32_t id)
{
    /* @bin 0x00488350 (thiscall on 0x00BCA350) for building_is_type_0x487f80 buildings: +0xAD
     * counts down first; else +0x54 grows by 0x00488140(id, 0), at most 0x00BCA34C, and
     * 0x00488E60(id, 0) */
    if (!building_is_type_0x487f80(st, id)) {
        return 0;
    }
    uint32_t b = BUILDING_ADDR(id);
    if (U8(st, b + BLD_UAD) != 0) {
        U8(st, b + BLD_UAD) -= 1;
        return 1;
    }
    int32_t r = farm_yield_488140(st, self, id, 0);
    if (r != 0) {
        S16(st, b + BLD_INDUSTRY_PROGRESS) += (int16_t) r;
        if (S16(st, b + BLD_INDUSTRY_PROGRESS) > S32(st, G_HARVEST_SIZE)) {
            S16(st, b + BLD_INDUSTRY_PROGRESS) = (int16_t) S32(st, G_HARVEST_SIZE);
        }
    }
    farm_redraw_488e60(st, self, id, 0);
    return 1;
}

void day_production_455900(game_state *st)
{
    /* @bin 0x00455900 day step 0x14: production progress +0x54 of the buildings with a resource
     * (+0x43) */
    farms_season_488630(st, G_BUILDING_MANAGER);
    for (int32_t id = 1; id <= S32(st, G_BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        uint8_t state = U8(st, b + BLD_STATE);
        int16_t type = S16(st, b + BLD_TYPE);
        if (state != 1 || U8(st, b + BLD_OUTPUT_RESOURCE) == 0) {
            /* @bin 0x00455AEA type 0xB1 in any used state */
            if (type != 0xB1 || state == 0) {
                continue;
            }
            int16_t w = S16(st, b + BLD_WORKERS);
            if (w <= 0 || (S16(st, b + BLD_SUBTYPE) != 0 && S16(st, b + BLD_STORED) <= 0)) {
                continue;
            }
            S16(st, b + BLD_INDUSTRY_PROGRESS) += w;
            if (S16(st, b + BLD_INDUSTRY_PROGRESS) > 400) {
                S16(st, b + BLD_INDUSTRY_PROGRESS) = 400;
            }
            continue;
        }
        if (building_needs_area_check(st, id)) {
            if (U8(st, b + BLD_SHOWS) != 0 && !building_resource_in_stock(st, id)) {
                S16(st, b + BLD_WORKERS) = S16(st, G_TYPE_WORKERS + 0x28u * (uint32_t) (int32_t) type);
            } else {
                S16(st, b + BLD_WORKERS) = 0;
            }
        }
        if (S16(st, b + BLD_SERVICE) < 1 && !building_needs_area_check(st, id)) {
            continue;
        }
        if (S16(st, b + BLD_WORKERS) <= 0) {
            continue;
        }
        if (S16(st, b + BLD_SUBTYPE) != 0 && S16(st, b + BLD_STORED) < 100) {
            continue;
        }
        if (S16(st, b + BLD_INPUT2) != 0 && S16(st, b + BLD_STOCK2) < (type != 0xCC ? 100 : 25)) {
            continue;
        }
        if (U8(st, b + BLD_UAD) != 0) {
            U8(st, b + BLD_UAD) -= 1;
            continue;
        }
        if (U8(st, b + BLD_UA9) != 0) {
            U8(st, b + BLD_UA9) -= 1;
        }
        if (!farm_grow_488350(st, G_BUILDING_MANAGER, id)) {
            S16(st, b + BLD_INDUSTRY_PROGRESS) += S16(st, b + BLD_WORKERS);
        }
        /* @bin 0x00455A15 a bonus of half the workers for two groups of types, by the city's
         * kingdom building (+0x4478, +0x472C) */
        int bonus = 0;
        switch (type) {
        case 0x4A: case 0x6D: case 0x6F: case 0x71: case 0xA1: case 0xA2: case 0xD9: {
            uint32_t c = CURRENT_CITY(st);
            bonus = S16(st, c + CITY_U4478) != 0 && GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, S32(st, c + CITY_U472C)) == 2;
            break;
        }
        case 0x6A: case 0x6B: case 0xB1: case 0xB2: case 0xB3: case 0xCC: case 0xD8: case 0xDD: case 0xE7: {
            uint32_t c = CURRENT_CITY(st);
            bonus = S16(st, c + CITY_U4478) != 0 && GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, S32(st, c + CITY_U472C)) == 2 &&
                    (int8_t) GAME_CALL(st, FN_BUILDING_0XA9_IS, S32(st, CURRENT_CITY(st) + CITY_U472C), 1) != 0;
            break;
        }
        default:
            break;
        }
        if (bonus) {
            S16(st, b + BLD_INDUSTRY_PROGRESS) += (int16_t) (S16(st, b + BLD_WORKERS) / 2);
        }
        if (building_is_type_0x487f80(st, id)) {
            continue;
        }
        int32_t cap = S16(st, b + BLD_SUBTYPE) != 0 ? 400 : 200;
        if (S16(st, b + BLD_INDUSTRY_PROGRESS) > cap) {
            S16(st, b + BLD_INDUSTRY_PROGRESS) = (int16_t) cap;
        }
    }
}


void buildings_count_4677c0(game_state *st)
{
    /* @bin 0x004677C0 day step 0x21: buildings counted by type (0x00705AF0) and working ones
     * (0x0070573C, workers +0x40 > 0), with the service counters 0x0078D6DC / 0x0078D64C, the
     * temples' and festival places' coverage counts, the city's police and fire lists; then each
     * building's three timers +0xA7..+0xA9 count down (their count at +0xA6) */
    /* @bin 0x004678CE the service buildings: (type, buildings, working ones) */
    static const uint32_t services[][3] = {
        {0x66u, G_U78D6DC + 4 * 1, G_INDUSTRY_WORKING + 4 * 1}, {0xC2u, G_U78D6DC + 4 * 2, G_INDUSTRY_WORKING + 4 * 2}, {0x67u, G_U78D6DC + 4 * 3, G_INDUSTRY_WORKING + 4 * 3},
        {0x69u, G_U78D6DC + 4 * 4, G_INDUSTRY_WORKING + 4 * 4}, {0x68u, G_U78D6DC + 4 * 5, G_INDUSTRY_WORKING + 4 * 5}, {0xC4u, G_U78D6DC + 4 * 6, G_INDUSTRY_WORKING + 4 * 6},
        {0x73u, G_U78D6DC + 4 * 8, G_INDUSTRY_WORKING + 4 * 8}, {0x70u, G_U78D6DC + 4 * 10, G_INDUSTRY_WORKING + 4 * 10}, {0x6Du, G_U78D6DC + 4 * 11, G_INDUSTRY_WORKING + 4 * 11},
        {0xCCu, G_U78D6DC + 4 * 12, G_INDUSTRY_WORKING + 4 * 12}, {0x72u, G_U78D6DC + 4 * 13, G_INDUSTRY_WORKING + 4 * 13}, {0x64u, G_U78D6DC + 4 * 14, G_INDUSTRY_WORKING + 4 * 14},
        {0x6Eu, G_U78D6DC + 4 * 15, G_INDUSTRY_WORKING + 4 * 15}, {0x65u, G_U78D6DC + 4 * 16, G_INDUSTRY_WORKING + 4 * 16}, {0xE0u, G_U78D6DC + 4 * 32, G_INDUSTRY_WORKING + 4 * 32},
        {0x6Fu, G_U78D6DC + 4 * 17, G_INDUSTRY_WORKING + 4 * 17}, {0xA2u, G_U78D6DC + 4 * 18, G_INDUSTRY_WORKING + 4 * 18}, {0x71u, G_U78D6DC + 4 * 19, G_INDUSTRY_WORKING + 4 * 19},
        {0x6Cu, G_U78D6DC + 4 * 20, G_INDUSTRY_WORKING + 4 * 20}, {0xA1u, G_U78D6DC + 4 * 21, G_INDUSTRY_WORKING + 4 * 21}, {0xC3u, G_U78D6DC + 4 * 22, G_INDUSTRY_WORKING + 4 * 22},
        {0xCBu, G_U78D6DC + 4 * 23, G_INDUSTRY_WORKING + 4 * 23}, {0x6Au, G_U78D6DC + 4 * 24, G_INDUSTRY_WORKING + 4 * 24}, {0x6Bu, G_U78D6DC + 4 * 25, G_INDUSTRY_WORKING + 4 * 25},
        {0xD8u, G_U78D6DC + 4 * 26, G_INDUSTRY_WORKING + 4 * 26}, {0xCDu, G_U78D6DC + 4 * 28, G_INDUSTRY_WORKING + 4 * 28}, {0xD9u, G_U78D6DC + 4 * 29, G_INDUSTRY_WORKING + 4 * 29},
        {0xDDu, G_U78D6DC + 4 * 30, G_INDUSTRY_WORKING + 4 * 30}, {0xE8u, G_U78D6DC + 4 * 34, G_INDUSTRY_WORKING + 4 * 34}, {0xE9u, G_U78D6DC + 4 * 33, G_INDUSTRY_WORKING + 4 * 33},
    };
    for (uint32_t k = 0; k < 0x24; k++) {
        S32(st, G_INDUSTRY_WORKING + 4 * k) = 0;
    }
    for (uint32_t k = 0; k < 0x24; k++) {
        S32(st, G_U78D6DC + 4 * k) = 0;
    }
    uint32_t c0 = CURRENT_CITY(st);
    S16(st, c0 + CITY_U4488) = 0;
    S32(st, c0 + CITY_U4630) = 0;
    for (uint32_t k = 0; k < 5; k++) {
        S32(st, c0 + CITY_U4460 + 4 * k) = 0;
    }
    S32(st, c0 + CITY_U1C) = 0;
    for (uint32_t k = 0; k < 0xED; k++) {
        S32(st, G_DAY_BUILDINGS_TYPE_COUNT + 4 * k) = 0;
    }
    for (uint32_t k = 0; k < 0xED; k++) {
        S32(st, G_TYPE_COUNT + 4 * k) = 0;
    }
    for (int32_t id = 1; id <= S32(st, G_BUILDING_HIGHEST); id++) {
        int32_t player = S32(st, CURRENT_PLAYER_ADDR);
        uint32_t c = CITY_ADDR(player);
        uint32_t b = BUILDING_ADDR(id);
        uint8_t state = U8(st, b + BLD_STATE);
        if (state != 1 && state != 3) {
            continue;
        }
        int16_t type = S16(st, b + BLD_TYPE);
        uint32_t t4 = 4u * (uint32_t) (int32_t) type;
        if (U8(st, b + BLD_HOUSE_SIZE) != 0) {
            S32(st, G_DAY_BUILDINGS_TYPE_COUNT + t4) += 1;
            if (S16(st, b + BLD_POPULATION) != 0) {
                S32(st, G_TYPE_COUNT + t4) += 1;
            }
            continue;
        }
        int working = S16(st, b + BLD_WORKERS) > 0;
        int part = 0; /* the type-0x1E / 0x1F / 0x21 records: their timers count down */
        unsigned k;
        for (k = 0; k < sizeof(services) / sizeof(services[0]); k++) {
            if ((uint32_t) (int32_t) type == services[k][0]) {
                break;
            }
        }
        if (type == 0x5A || type == 0x5C || type == 0xB4) {
            S32(st, G_DAY_BUILDINGS_TYPE_COUNT + t4) += 1;
            if (U8(st, b + BLD_WATER_SUPPLY) > 0 && (type == 0x5C || working)) {
                S32(st, G_TYPE_COUNT + t4) += 1;
            }
        } else if (k < sizeof(services) / sizeof(services[0])) {
            S32(st, services[k][1]) += 1;
            if (working) {
                S32(st, services[k][2]) += 1;
            }
        } else if (type == 0x4C) {
            S32(st, G_U78D6DC + 4 * 7) += 1;
            if (working) {
                S16(st, c + CITY_U4488) += 1;
                if (S16(st, b + BLD_SECOND_WALKER) == 0) {
                    S32(st, c + CITY_U4630) += 1;
                }
                S32(st, G_INDUSTRY_WORKING + 4 * 7) += 1;
            }
        } else if (type == 0x4B || type == 0xC0) {
            /* @bin 0x00468068 the first ten working ones with road access in the city's list */
            int32_t n = S32(st, G_DOCK_COUNT);
            if (n < 10) {
                if (working && U8(st, b + BLD_WATER_SUPPLY) != 0) {
                    S16(st, c + CITY_U4460 + 2u * (uint32_t) n) = (int16_t) id;
                }
                S32(st, G_DOCK_COUNT) = n + 1;
            }
        } else if ((type >= 0x41 && type <= 0x45) || (type >= 0x9C && type <= 0xA0)) {
            if (building_main_part(st, id) == id) {
                S32(st, G_DAY_BUILDINGS_TYPE_COUNT + t4) += 1;
                if (working) {
                    S32(st, G_TYPE_COUNT + t4) += 1;
                }
            }
        } else if (type >= 0x8C && type <= 0x95) {
            S32(st, G_DAY_BUILDINGS_TYPE_COUNT + t4) += 1;
            if (road_next_to_4802c0(st, S16(st, b + BLD_X), S16(st, b + BLD_Y)) != 0) {
                S32(st, G_TYPE_COUNT + 4u * (uint32_t) (int32_t) S16(st, b + BLD_TYPE)) += 1;
            }
        } else {
            S32(st, G_DAY_BUILDINGS_TYPE_COUNT + t4) += 1;
            if (type == 0x1F) {
                int32_t main = building_main_part(st, id);
                if (main != id) {
                    continue;
                }
                if (S16(st, BUILDING_ADDR(main) + BLD_WORKERS) > 0) {
                    S32(st, G_U7057B8) += 1;
                }
                part = 1;
            } else if (type == 0x1E) {
                if (building_main_part(st, id) != id) {
                    continue;
                }
                int32_t main = building_main_part(st, id);
                if (S16(st, BUILDING_ADDR(main) + BLD_WORKERS) > 0) {
                    S32(st, G_U7057B8) += 1;
                    S32(st, G_U7057B4) += 1;
                }
                part = 1;
            } else if (type == 0x21) {
                if (working) {
                    S32(st, G_U7057B8) += 1;
                    S32(st, G_U7057B4) += 1;
                    S32(st, G_U7057C0) += 1;
                }
                part = 1;
            } else if (type == 0x20) {
                if (working) {
                    S32(st, G_U7057BC) += 1;
                }
            } else if (type == 0xE2) {
                if (working) {
                    S32(st, G_U705AC4) += 1;
                }
            } else {
                if (type == 0x5F) {
                    S32(st, c + CITY_RECRUITER) = id;
                }
                if (working) {
                    S32(st, G_TYPE_COUNT + t4) += 1;
                }
            }
        }
        /* @bin 0x004680CD the building's figure +0x2E must be alive and tied to it (+0x5E) */
        int16_t fig = S16(st, b + BLD_IMMIGRANT);
        if (fig != 0 && !(U8(st, FIGURE_ADDR(fig) + FIG_STATE) == 1 && S16(st, FIGURE_ADDR(fig) + FIG_DEST_BUILDING) == id)) {
            S16(st, b + BLD_IMMIGRANT) = 0;
        }
        if (!part && !building_needs_area_check(st, id)) {
            continue;
        }
        uint8_t running = 0;
        for (uint32_t t = 0; t < 3; t++) {
            if (U8(st, b + BLD_UA7 + t) > 0) {
                U8(st, b + BLD_UA7 + t) -= 1;
                running++;
            }
        }
        U8(st, b + BLD_SHOWS) = running;
    }
}

void farm_redraw_488e60(game_state *st, uint32_t self, int32_t id, int32_t flag)
{
    /* @bin 0x00488E60 (thiscall on 0x00BCA350) 0x00488C20(id, x, y, flag) on the same object */
    uint32_t b = BUILDING_ADDR(id);
    farm_draw_488c20(st, self, id, S16(st, b + BLD_X), S16(st, b + BLD_Y), flag);
}

void farm_draw_488c20(game_state *st, uint32_t self, int32_t id, int32_t x, int32_t y, int32_t flag)
{
    /* @bin 0x00488C20 (thiscall on 0x00BCA350) types 0xC3 / 0xC2 get their 2 x 2 / 3 x 3 image
     * (0x00F564DC / 0x00F56764); a farm its crop image (a local table of image groups by farm
     * type, 0x00F55BB4) at growth +0x54 / 10, drawn by 0x00488840 (no flood object within
     * 0x004894D0: nothing) or 0x00488690 */
    uint32_t b = BUILDING_ADDR(id);
    int16_t type = S16(st, b + BLD_TYPE);
    if (type == 0xC3 || type == 0xC2) {
        int32_t size = type == 0xC3 ? 2 : 3;
        GAME_CALL(st, FN_MAP_BUILDING_TILES, id, x, y, size, size, S32(st, type == 0xC3 ? G_UF564DC : G_UF56764), 8);
        return;
    }
    /* @bin 0x00488C4F the group table (index -1 reads the caller's saved ebx: not reproducible) */
    static const int32_t groups[10] = {0x171, 0x1CF, 0x145, 0x11A, 0x142, 0x144, 0x180, 0x180, 0x180, 0x2CB};
    int32_t k = -1;
    if (building_is_type_0x487f80(st, id)) {
        switch (S16(st, b + BLD_TYPE)) {
        case 0x64: k = 0; break;
        case 0x65: k = 1; break;
        case 0xE0: k = 9; break;
        case 0x66: k = 2; break;
        case 0x67: k = 3; break;
        case 0x68: k = 4; break;
        case 0x69: k = 5; break;
        case 0xC4: k = 7; break;
        case 0xC3: k = 8; break;
        default: break;
        }
    }
    if (k < 0) {
        state_unported(st, FN_FARM_DRAW_488C20);
        return;
    }
    int32_t img = S32(st, G_DAY2_REDRAW_GROUPS + 8u * (uint32_t) groups[k]);
    int32_t stage = S16(st, b + BLD_INDUSTRY_PROGRESS) / 10; /* the divisor is ftol(10.0) */
    if (building_area_test_3x3(st, id)) {
        /* @bin 0x004894D0 */
        if (map_area_test(st, S16(st, b + BLD_X), S16(st, b + BLD_Y), 3, 3, 4)) {
            return;
        }
        GAME_CALLT(st, self, FN_FARM_CROP_LARGE, id, x, y, img, stage, flag);
    } else {
        GAME_CALLT(st, self, FN_FARM_CROP_SMALL, id, x, y, img, stage, flag);
    }
}

int32_t farm_yield_488140(game_state *st, uint32_t self, int32_t id, int32_t full)
{
    /* @bin 0x00488140 (thiscall on 0x00BCA350) the day's growth: fertility 0x00489110 times
     * (0.16 * (1/99)) times the workers (all needed ones with `full`) times 10, in x87 doubles;
     * doubled for types 0x66 / 0x64 of building_is_type_0x487f80 with +0xA9 set; at least 1 */
    uint32_t b = BUILDING_ADDR(id);
    int32_t workers = (uint8_t) full != 0 ? S32(st, G_TYPE_WORKERS + 0x28u * (uint32_t) (int32_t) S16(st, b + BLD_TYPE))
                                           : S16(st, b + BLD_WORKERS);
    int32_t fertility = farm_fertility_489110(st, id);
    (void) self;
    x87_val c1 = x87_from_double(0x3FC47AE147AE147Bull); /* 0x0056F5B0: 0.16 */
    x87_val c2 = x87_from_double(0x3F84AFD6A052BF5Bull); /* 0x0056F588: 1/99 */
    x87_val c3 = x87_from_double(0x4024000000000000ull); /* 0x0056F5A8: 10.0 */
    x87_val v = x87_mul(x87_mul(x87_mul(x87_from_int(fertility), x87_mul(c1, c2)), x87_from_int(workers)), c3);
    if (U8(st, b + BLD_UA9) != 0 && building_is_type_0x487f80(st, id)) {
        int16_t type = S16(st, b + BLD_TYPE);
        if (type == 0x66 || type == 0x64) {
            v = x87_add(v, v);
        }
    }
    int32_t r = x87_ftol(v);
    return r < 1 ? 1 : r;
}

#define VOID0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
VOID0(run_service_shares, day_service_shares)
VOID0(run_decay_0x18, day_decay_0x18)
VOID0(run_step_0x11, day_step_0x11)
VOID0(run_highest_update, buildings_highest_update)
VOID0(run_list_45d83c, day_list_45d83c)
VOID0(run_spawn_469b90, day_spawn_469b90)
VOID0(run_city1_clear, city1_clear_4e8610)
VOID0(run_list_active, buildings_list_active)
VOID0(run_step_0x16, day_step_0x16)
VOID0(run_step_0x1c, day_step_0x1c)
VOID0(run_water_level, buildings_water_level)
VOID0(run_stock_totals, day_stock_totals_45e040)
VOID0(run_categories_need, labour_categories_need)
VOID0(run_networks_update, road_networks_update)
VOID0(run_production, day_production_455900)
VOID0(run_count_4677c0, buildings_count_4677c0)

static int32_t run_farms_season(game_state *st, const int32_t *a)
{
    (void) a;
    farms_season_488630(st, G_BUILDING_MANAGER);
    return 0;
}

static int32_t run_farm_draw(game_state *st, const int32_t *a)
{
    farm_draw_488c20(st, G_BUILDING_MANAGER, a[0], a[1], a[2], a[3]);
    return 0;
}

static int32_t run_area_4894d0(game_state *st, const int32_t *a)
{
    uint32_t b = BUILDING_ADDR(a[0]);
    return map_area_test(st, S16(st, b + BLD_X), S16(st, b + BLD_Y), 3, 3, 4);
}

static int32_t run_farm_redraw(game_state *st, const int32_t *a)
{
    farm_redraw_488e60(st, G_BUILDING_MANAGER, a[0], a[1]);
    return 0;
}

static int32_t run_farm_yield(game_state *st, const int32_t *a)
{
    return farm_yield_488140(st, G_BUILDING_MANAGER, a[0], a[1]);
}

static int32_t run_area_fertility(game_state *st, const int32_t *a)
{
    return area_fertility_489050(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_road_flag(game_state *st, const int32_t *a)
{
    return building_road_flag_4890f0(st, a[0]);
}

static int32_t run_type_488080(game_state *st, const int32_t *a)
{
    return building_type_0x487f80(S16(st, BUILDING_ADDR(a[0]) + BLD_TYPE));
}

static int32_t run_farm_fertility(game_state *st, const int32_t *a)
{
    return farm_fertility_489110(st, a[0]);
}

static int32_t run_trade_tax(game_state *st, const int32_t *a)
{
    return trade_tax_4c5340(st, a[0]);
}

static int32_t run_farm_grow(game_state *st, const int32_t *a)
{
    return farm_grow_488350(st, G_BUILDING_MANAGER, a[0]);
}

static int32_t run_network_fill(game_state *st, const int32_t *a)
{
    road_network_fill(st, a[0], a[1]);
    return 0;
}

static int32_t run_spawn_469e40(game_state *st, const int32_t *a)
{
    building_spawn_469e40(st, a[0], a[1]);
    return 0;
}

static int32_t run_ratio_percent(game_state *st, const int32_t *a)
{
    (void) st;
    return calc_ratio_percent(a[0], a[1]);
}

static int32_t run_share_by_weight(game_state *st, const int32_t *a)
{
    return share_by_weight(st, a[0], a[1], (uint32_t) a[2]);
}

static int32_t run_service_share(game_state *st, const int32_t *a)
{
    return building_service_share(st, a[0]);
}

static int32_t run_road_access_4(game_state *st, const int32_t *a)
{
    return building_road_access_4(st, a[0], a[1]);
}

static int32_t run_totals_45d630(game_state *st, const int32_t *a)
{
    day_totals_45d630(st, a[0]);
    return 0;
}

static int32_t run_spawn_figure_3(game_state *st, const int32_t *a)
{
    spawn_figure_3(st, a[0], a[1], a[2], a[3]);
    return 0;
}

const ported_function day_buildings_ports[] = {
    {0x004CDE60u, "calc_ratio_percent", 2, 32, 0, run_ratio_percent, 0},
    {0x0045A0A0u, "share_by_weight", 3, 32, 0, run_share_by_weight, 0},
    {0x0045A140u, "building_service_share", 1, 32, 0, run_service_share, 0},
    {0x004684E0u, "day_service_shares", 0, 0, 0, run_service_shares, 0},
    {0x004558B0u, "day_decay_0x18", 0, 0, 0, run_decay_0x18, 0},
    {0x004806A0u, "building_road_access_4", 2, 32, 0, run_road_access_4, 0},
    {0x0045D630u, "day_totals_45d630", 1, 0, 0, run_totals_45d630, 0},
    {0x0045D5D0u, "day_step_0x11", 0, 0, 0, run_step_0x11, 0},
    {0x0045AA30u, "buildings_highest_update", 0, 0, 0, run_highest_update, 0},
    {0x00459020u, "spawn_figure_3", 4, 0, 0, run_spawn_figure_3, 0},
    {0x00459070u, "day_list_45d83c", 0, 0, 0, run_list_45d83c, 0},
    {0x00469E40u, "building_spawn_469e40", 2, 0, 0, run_spawn_469e40, 0},
    {0x00469B90u, "day_spawn_469b90", 0, 0, 0, run_spawn_469b90, 0},
    {0x004E8610u, "city1_clear_4e8610", 0, 0, 0, run_city1_clear, 0},
    {0x004583F0u, "buildings_list_active", 0, 0, 0, run_list_active, 0},
    {0x00458440u, "day_step_0x16", 0, 0, 0, run_step_0x16, 0},
    {0x00455430u, "day_step_0x1c", 0, 0, 0, run_step_0x1c, 0},
    {0x004539E0u, "buildings_water_level", 0, 0, 0, run_water_level, 0},
    {0x0045E040u, "day_stock_totals_45e040", 0, 0, 0, run_stock_totals, 0},
    {0x00459100u, "labour_categories_need", 0, 0, 0, run_categories_need, 0},
    {0x004689E0u, "road_network_fill", 2, 0, 0, run_network_fill, 0},
    {0x00468860u, "road_networks_update", 0, 0, 0, run_networks_update, 0},
    {0x00488630u, "farms_season_488630", 0, 0, G_BUILDING_MANAGER, run_farms_season, 0},
    {0x00488C20u, "farm_draw_488c20", 4, 0, G_BUILDING_MANAGER, run_farm_draw, 0},
    {0x004894D0u, "building_area_test_4894d0", 1, 8, G_BUILDING_MANAGER, run_area_4894d0, 0},
    {0x00488E60u, "farm_redraw_488e60", 2, 0, G_BUILDING_MANAGER, run_farm_redraw, 0},
    {0x00488140u, "farm_yield_488140", 2, 32, G_BUILDING_MANAGER, run_farm_yield, 0},
    {0x00489050u, "area_fertility_489050", 4, 32, G_BUILDING_MANAGER, run_area_fertility, 0},
    {0x004890F0u, "building_road_flag_4890f0", 1, 8, G_BUILDING_MANAGER, run_road_flag, 0},
    {0x00488080u, "building_type_488080", 1, 8, G_BUILDING_MANAGER, run_type_488080, 0},
    {0x00489110u, "farm_fertility_489110", 1, 32, G_BUILDING_MANAGER, run_farm_fertility, 0},
    {0x004C5340u, "trade_tax_4c5340", 1, 32, 0, run_trade_tax, 0},
    {0x00488350u, "farm_grow_488350", 1, 8, G_BUILDING_MANAGER, run_farm_grow, 0},
    {0x00455900u, "day_production_455900", 0, 0, 0, run_production, 0},
    {0x004677C0u, "buildings_count_4677c0", 0, 0, 0, run_count_4677c0, 0},
    /* x87 truncation, done inline by the ports (city/day_x87.h: x87_ftol) */
    {0x0056133Cu, "__ftol (x87_ftol in city/day_x87.h)", 0, 32, 0, NULL, 0},
};
const int day_buildings_ports_count = sizeof(day_buildings_ports) / sizeof(day_buildings_ports[0]);
