/* Imhotep - the houses' daily evolution pass (day step 0x27).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "building/day_houses.h"

#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


void houses_counters_clear(game_state *st)
{
    /* @bin 0x00454D70 */
    static const uint16_t fields[] = {
        CITY_U42A4, CITY_U42AC, CITY_U42B4, CITY_U42BC, CITY_U4304,
        CITY_U4308, CITY_U430C, CITY_U4388, CITY_U4310, CITY_U4314,
        CITY_U4350, CITY_U4354, CITY_U4358, CITY_U4320, CITY_U4324,
        CITY_U4328, CITY_U433C, CITY_U4340, CITY_U432C, CITY_U4318,
        CITY_U431C, CITY_U4344, CITY_U4348, CITY_U434C, CITY_U435C,
        CITY_U474C, CITY_U4748,
    };
    uint32_t c = CURRENT_CITY(st);
    for (unsigned i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        S32(st, c + fields[i]) = 0;
    }
}

void building_check_position(game_state *st, int32_t id)
{
    /* @bin 0x0046B740 when the building's tile (+0xC) is not that of (+6, +8) or not its own in
     * grid 0x00B4E6FC, the grid is searched (0x0078D5C4 counts searches): found, the building
     * takes the tile; not found, it is removed (state 4, 0x0078D5C0 counts) */
    uint32_t b = BUILDING_ADDR(id);
    int32_t x = S16(st, b + BLD_X), y = S16(st, b + BLD_Y);
    int32_t tile = S32(st, b + BLD_GRID_OFFSET);
    U8(st, b + BLD_U7E) = 0;
    if (tile == S32(st, G_MAP_START) + y * GRID_SIZE + x && S16(st, G_GRID_BUILDING + 2u * (uint32_t) tile) == id) {
        return;
    }
    S32(st, G_POSITION_SEARCHES) += 1;
    int32_t t = S32(st, G_MAP_START);
    for (int32_t row = 0; row < S32(st, G_MAP_HEIGHT); row++) {
        for (int32_t col = 0; col < S32(st, G_MAP_WIDTH); col++, t++) {
            if (S16(st, G_GRID_BUILDING + 2u * (uint32_t) t) == id) {
                S32(st, b + BLD_GRID_OFFSET) = t;
                int32_t off = t - S32(st, G_MAP_START);
                S16(st, b + BLD_X) = (int16_t) (off % GRID_SIZE);
                S16(st, b + BLD_Y) = (int16_t) (off / GRID_SIZE);
                return;
            }
        }
        t += S32(st, G_MAP_STRIDE_EXTRA);
    }
    U8(st, b + BLD_STATE) = 4;
    S32(st, G_POSITION_LOST) += 1;
}

void house_consume(game_state *st, int32_t id)
{
    /* @bin 0x00454B40 the level's (+0x12) four amounts per month (0x0070CDF8 + 0x58 * level,
     * times 4 for large houses) are used a quarter a day; an amount not a multiple of 4 uses
     * instead 0 or 1 by the day of the month (days 0 mod 4: 1, 1, 0 for remainders 1, 2, 3;
     * days 2 mod 4: 0, 1, 1; others: 0, 0, 1) */
    uint32_t b = BUILDING_ADDR(id);
    uint32_t rec = 0x58u * (uint32_t) (int32_t) S16(st, b + BLD_SUBTYPE);
    int big = U8(st, b + BLD_SIZE) > 1;
    int32_t amount[4];
    for (uint32_t k = 0; k < 4; k++) {
        amount[k] = S32(st, G_HOUSE_LEVEL_GOODS + rec + 4 * k);
        if (big) {
            amount[k] = (int32_t) ((uint32_t) amount[k] << 2);
        }
    }
    int32_t part[4] = {0, 0, 0, 0}; /* part[0] unused (the original's slot is never read) */
    switch (S32(st, G_TICK_DAY) % 4) {
    case 0: part[1] = 1, part[2] = 1, part[3] = 0; break;
    case 2: part[1] = 0, part[2] = 1, part[3] = 1; break;
    case 1: case 3: part[1] = 0, part[2] = 0, part[3] = 1; break;
    default: break;
    }
    int32_t use[4];
    for (int k = 0; k < 4; k++) {
        int32_t r = amount[k] % 4;
        use[k] = r != 0 ? part[r < 0 ? 0 : r] : amount[k] / 4;
    }
    /* @bin 0x00454CB6 amount 3 from +0x64, 0 from +0x66, 1 from +0x68, 2 from +0x6A */
    static const uint8_t slot[4] = {BLD_HOUSE_INVENTORY + 2 * 9, BLD_HOUSE_INVENTORY + 2 * 10, BLD_HOUSE_INVENTORY + 2 * 11,
                                    BLD_HOUSE_INVENTORY + 2 * 8};
    static const int order[4] = {3, 0, 1, 2};
    for (int i = 0; i < 4; i++) {
        int k = order[i];
        if (use[k] <= 0) {
            continue;
        }
        int16_t have = S16(st, b + slot[k]);
        S16(st, b + slot[k]) = use[k] > have ? 0 : (int16_t) (have - use[k]);
    }
}


static int32_t house_image(game_state *st, int32_t level)
{
    return S32(st, G_IMAGE_GROUPS + 8u * U32(st, G_HOUSE_LEVEL_GROUP + 4u * (uint32_t) level));
}

void house_set_type(game_state *st, int32_t id, int32_t type)
{
    /* @bin 0x0046CEA0 the house's type (+0x10) and level (+0x12, type - 10) and its image: the
     * level's image (0x005D03EC -> 0x00F55BB4), large houses (+4) + 4 (+ 1 with 0x005D043C),
     * small ones + 0x005D043C + a variant (0x005D034C) by the tile's hash 0x00ACF85C */
    uint32_t b = BUILDING_ADDR(id);
    S16(st, b + BLD_TYPE) = (int16_t) type;
    int16_t level = (int16_t) (type - 10);
    S16(st, b + BLD_SUBTYPE) = level;
    int32_t img = house_image(st, level);
    int32_t variants = S32(st, G_HOUSE_LEVEL_VARIANTS + 4u * (uint32_t) (int32_t) level);
    if (U8(st, b + BLD_HOUSE_MERGED) != 0) {
        img += 4;
        if (S32(st, G_HOUSE_LEVEL_IMAGE_ADD + 4u * (uint32_t) (int32_t) level) != 0) {
            img++;
        }
    } else {
        img += S32(st, G_HOUSE_LEVEL_IMAGE_ADD + 4u * (uint32_t) (int32_t) level);
        if (variants != 0) {
            img += U8(st, G_GRID_RANDOM + (uint32_t) S32(st, b + BLD_GRID_OFFSET)) & (uint32_t) (variants - 1);
        }
    }
    GAME_CALL(st, FN_MAP_BUILDING_TILES, id, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE), U8(st, b + BLD_SIZE), img, 8);
}

int32_t houses_redraw_only(game_state *st)
{
    /* @bin 0x004539A0 with 0x00978D70 set, houses are only redrawn (small ones) and not evolved */
    if (S32(st, G_U978D70) == 0) {
        return 0;
    }
    int32_t id = S32(st, G_HOUSE_ID);
    uint32_t b = BUILDING_ADDR(id);
    if (U8(st, b + BLD_HOUSE_MERGED) == 0) {
        house_set_type(st, id, S16(st, b + BLD_TYPE));
    }
    return 1;
}

int32_t house_desirability_trend(game_state *st, int32_t id)
{
    /* @bin 0x00453AF0 by the house's desirability +0xBE against its level's bounds (+0, +4 of the
     * level record; the upper one 1000 from level 0x13): -1 at or below the lower, 1 at or above
     * the upper, else 0 */
    uint32_t b = BUILDING_ADDR(id);
    int32_t level = S16(st, b + BLD_SUBTYPE);
    uint32_t rec = G_HOUSE_LEVELS + 0x58u * (uint32_t) level;
    int32_t low = S32(st, rec), high = S32(st, rec + 4);
    if (level >= 0x13) {
        high = 1000;
    }
    int32_t d = S8(st, b + BLD_DESIRABILITY);
    if (d <= low) {
        return -1;
    }
    return d >= high;
}

int32_t house_merge_neighbours(game_state *st, int32_t id, int32_t n)
{
    /* @bin 0x0046B9E0 the n tiles of offset table 0x005DA2F4 from (MERGE_X, MERGE_Y): every other
     * house there gives its people (returned) and goods (summed in 0x0078D590) and is removed
     * (state 5) */
    for (uint32_t k = 0; k < 12; k++) {
        S32(st, G_GOODS_SUM + 4 * k) = 0;
    }
    int32_t base = S32(st, G_MAP_START) + S32(st, G_MERGE_Y) * GRID_SIZE + S32(st, G_MERGE_X);
    int32_t people = 0;
    for (int32_t k = 0; k < n; k++) {
        int32_t t = base + S32(st, G_MERGE_TILES + 4u * (uint32_t) k);
        if (!(U32(st, G_GRID_TERRAIN + 4u * (uint32_t) t) & 8)) {
            continue;
        }
        int32_t other = S16(st, G_GRID_BUILDING + 2u * (uint32_t) t);
        if (other == id) {
            continue;
        }
        uint32_t o = BUILDING_ADDR(other);
        if (U8(st, o + BLD_HOUSE_SIZE) == 0) {
            continue;
        }
        people += S16(st, o + BLD_POPULATION);
        for (uint32_t j = 0; j < 12; j++) {
            S32(st, G_GOODS_SUM + 4 * j) += S16(st, o + BLD_HOUSE_INVENTORY + 2 * j);
        }
        S16(st, o + BLD_POPULATION) = 0;
        U8(st, o + BLD_STATE) = 5;
    }
    return people;
}

void house_become_large(game_state *st, int32_t id, int32_t people)
{
    /* @bin 0x0046BDA0 the house becomes a 2 x 2 house at (MERGE_X, MERGE_Y) with the merged
     * people and goods */
    uint32_t b = BUILDING_ADDR(id);
    U8(st, b + BLD_SIZE) = 2;
    U8(st, b + BLD_HOUSE_SIZE) = 2;
    S16(st, b + BLD_POPULATION) = (int16_t) (S16(st, b + BLD_POPULATION) + people);
    for (uint32_t j = 0; j < 12; j++) {
        S16(st, b + BLD_HOUSE_INVENTORY + 2 * j) = (int16_t) (S16(st, b + BLD_HOUSE_INVENTORY + 2 * j) + S32(st, G_GOODS_SUM + 4 * j));
    }
    int32_t level = S16(st, b + BLD_SUBTYPE);
    int32_t img = house_image(st, level) + 4;
    if (S32(st, G_HOUSE_LEVEL_IMAGE_ADD + 4u * (uint32_t) level) != 0) {
        img++;
    }
    GAME_CALL(st, FN_BUILDING_CLEAR_TILES, id, S16(st, b + BLD_X), S16(st, b + BLD_Y));
    int32_t x = S32(st, G_MERGE_X), y = S32(st, G_MERGE_Y);
    S16(st, b + BLD_X) = (int16_t) x;
    S16(st, b + BLD_Y) = (int16_t) y;
    S32(st, b + BLD_GRID_OFFSET) = S32(st, G_MAP_START) + y * GRID_SIZE + x;
    U8(st, b + BLD_HOUSE_MERGED) = 1;
    GAME_CALL(st, FN_MAP_BUILDING_TILES, id, S16(st, b + BLD_X), S16(st, b + BLD_Y), 2, 2, img, 8);
}

int32_t house_try_merge(game_state *st, int32_t id)
{
    /* @bin 0x0046B840 a small house (+4 clear, tile hash & 7 below 5) whose four tiles of table
     * 0x005DA2F4 hold itself or small houses of its level merges with them */
    uint32_t b = BUILDING_ADDR(id);
    if (U8(st, b + BLD_HOUSE_MERGED) != 0) {
        return 0;
    }
    int32_t tile = S32(st, b + BLD_GRID_OFFSET);
    if ((U8(st, G_GRID_RANDOM + (uint32_t) tile) & 7) >= 5) {
        return 0;
    }
    int32_t n = 0;
    for (uint32_t k = 0; k < 4; k++) {
        int32_t t = tile + S32(st, G_MERGE_TILES + 4 * k);
        if (!(U32(st, G_GRID_TERRAIN + 4u * (uint32_t) t) & 8)) {
            continue;
        }
        int32_t other = S16(st, G_GRID_BUILDING + 2u * (uint32_t) t);
        if (other == id) {
            n++;
            continue;
        }
        uint32_t o = BUILDING_ADDR(other);
        if (U8(st, o + BLD_STATE) == 1 && U8(st, o + BLD_HOUSE_SIZE) != 0 && S16(st, o + BLD_SUBTYPE) == S16(st, b + BLD_SUBTYPE) && U8(st, o + BLD_HOUSE_MERGED) == 0) {
            n++;
        }
    }
    if (n != 4) {
        return 0;
    }
    S32(st, G_MERGE_X) = S16(st, b + BLD_X) + S32(st, G_MERGE_CORNERS);
    S32(st, G_MERGE_Y) = S16(st, b + BLD_Y) + S32(st, G_MERGE_CORNERS + 4);
    house_become_large(st, id, house_merge_neighbours(st, id, 4));
    return 1;
}

/* @bin 0x0046B480 the tiles counted for an expansion to n tiles at anchor k (0x005DA2E4) */
static int32_t expansion_count(game_state *st, int32_t id, uint32_t b, int32_t anchor, int32_t n, int pass)
{
    int32_t count = 0;
    for (int32_t j = 0; j < n; j++) {
        int32_t t = anchor + S32(st, G_MERGE_TILES + 4u * (uint32_t) j);
        uint32_t terrain = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) t);
        if (pass > 1) {
            terrain &= 0xEEFED77Fu;
            if (terrain == 0) {
                count++;
            }
        }
        if (!(terrain & 8)) {
            if (pass == 3 && (terrain & 0x20)) {
                count++;
            }
            continue;
        }
        int32_t other = S16(st, G_GRID_BUILDING + 2u * (uint32_t) t);
        if (other == id) {
            count++;
            continue;
        }
        uint32_t o = BUILDING_ADDR(other);
        if (U8(st, o + BLD_STATE) == 1 && U8(st, o + BLD_HOUSE_SIZE) != 0 && S16(st, o + BLD_SUBTYPE) <= S16(st, b + BLD_SUBTYPE)) {
            count++;
        }
    }
    return count;
}

int32_t house_find_expansion(game_state *st, int32_t id, int32_t n)
{
    /* @bin 0x0046B480 a place for the house grown to n tiles: at one of the four anchors of
     * 0x005DA2E4 (coordinates 0x005DA2C4), all n tiles of table 0x005DA2F4 hold the house or
     * houses of its level or lower; failing that, also empty land (terrain & 0xEEFED77F == 0);
     * failing that, also tiles with terrain 0x20. Found: MERGE_X / Y and 1; else +0x7E := 1 */
    uint32_t b = BUILDING_ADDR(id);
    int32_t tile = S32(st, b + BLD_GRID_OFFSET);
    for (int pass = 1; pass <= 3; pass++) {
        for (int32_t k = 0; k < 4; k++) {
            int32_t anchor = tile + S32(st, G_MERGE_ANCHORS + 4u * (uint32_t) k);
            if (expansion_count(st, id, b, anchor, n, pass) == n) {
                S32(st, G_MERGE_X) = S16(st, b + BLD_X) + S32(st, G_MERGE_CORNERS + 8u * (uint32_t) k);
                S32(st, G_MERGE_Y) = S16(st, b + BLD_Y) + S32(st, G_MERGE_CORNERS + 4 + 8u * (uint32_t) k);
                return 1;
            }
        }
    }
    U8(st, b + BLD_U7E) = 1;
    return 0;
}

void house_clear_expansion(game_state *st, int32_t id, int32_t n)
{
    /* @bin 0x0046B940 the other houses on the n tiles from (MERGE_X, MERGE_Y) are split first:
     * 2 x 2 houses (+4) by 0x0046BE90, size 2 by 0x0046C1C0, size 3 by 0x0046C500 */
    int32_t base = S32(st, G_MAP_START) + S32(st, G_MERGE_Y) * GRID_SIZE + S32(st, G_MERGE_X);
    for (int32_t j = 0; j < n; j++) {
        int32_t t = base + S32(st, G_MERGE_TILES + 4u * (uint32_t) j);
        if (!(U32(st, G_GRID_TERRAIN + 4u * (uint32_t) t) & 8)) {
            continue;
        }
        int32_t other = S16(st, G_GRID_BUILDING + 2u * (uint32_t) t);
        if (other == id) {
            continue;
        }
        uint32_t o = BUILDING_ADDR(other);
        if (U8(st, o + BLD_HOUSE_SIZE) == 0) {
            continue;
        }
        if (U8(st, o + BLD_HOUSE_MERGED) == 1) {
            house_split_large(st, other);
        } else if (U8(st, o + BLD_SIZE) == 2) {
            house_devolve_0x14(st, other);
        } else if (U8(st, o + BLD_SIZE) == 3) {
            GAME_CALL(st, FN_HOUSE_SPLIT, other);
        }
    }
}

static void house_grow(game_state *st, int32_t id, int32_t people, int16_t type, uint8_t size, int hash)
{
    /* @bin 0x0046BAB0 / 0x0046BBC0 / 0x0046BCB0 the house becomes `type`, size x size at
     * (MERGE_X, MERGE_Y), with the merged people and goods */
    uint32_t b = BUILDING_ADDR(id);
    S16(st, b + BLD_TYPE) = type;
    S16(st, b + BLD_SUBTYPE) = (int16_t) (type - 10);
    U8(st, b + BLD_SIZE) = size;
    U8(st, b + BLD_HOUSE_SIZE) = size;
    S16(st, b + BLD_POPULATION) = (int16_t) (S16(st, b + BLD_POPULATION) + people);
    for (uint32_t j = 0; j < 12; j++) {
        S16(st, b + BLD_HOUSE_INVENTORY + 2 * j) = (int16_t) (S16(st, b + BLD_HOUSE_INVENTORY + 2 * j) + S32(st, G_GOODS_SUM + 4 * j));
    }
    int32_t level = S16(st, b + BLD_SUBTYPE);
    int32_t img = house_image(st, level) + S32(st, G_HOUSE_LEVEL_IMAGE_ADD + 4u * (uint32_t) level);
    if (hash) {
        img += U8(st, G_GRID_RANDOM + (uint32_t) S32(st, b + BLD_GRID_OFFSET)) & 1;
    }
    GAME_CALL(st, FN_BUILDING_CLEAR_TILES, id, S16(st, b + BLD_X), S16(st, b + BLD_Y));
    int32_t x = S32(st, G_MERGE_X), y = S32(st, G_MERGE_Y);
    S16(st, b + BLD_X) = (int16_t) x;
    S16(st, b + BLD_Y) = (int16_t) y;
    S32(st, b + BLD_GRID_OFFSET) = S32(st, G_MAP_START) + y * GRID_SIZE + x;
    GAME_CALL(st, FN_MAP_BUILDING_TILES, id, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE), U8(st, b + BLD_SIZE), img, 8);
}

void house_grow_0x14(game_state *st, int32_t id, int32_t people) { house_grow(st, id, people, 0x14, 2, 1); }
void house_grow_0x18(game_state *st, int32_t id, int32_t people) { house_grow(st, id, people, 0x18, 3, 0); }
void house_grow_0x1c(game_state *st, int32_t id, int32_t people) { house_grow(st, id, people, 0x1C, 4, 0); }

int32_t house_can_evolve(game_state *st, int32_t id, int32_t up)
{
    /* @bin 0x00453B40 the needs of the house's level (of the next level with `up`), record
     * 0x0070CDD4 + 0x58 * level: water, entertainment, religion, education, health, magistrate,
     * services, goods count, and the four goods (a quarter, rounded, for 1 x 1 houses); the
     * first one missing counts in the city's statistics and returns 0 */
    uint32_t b = BUILDING_ADDR(id);
    int32_t level = S16(st, b + BLD_SUBTYPE) + (up != 0);
    uint32_t rec = G_HOUSE_LEVELS + 8 + 0x58u * (uint32_t) level;
    int32_t water = S32(st, rec + 4);
    int32_t ent = S32(st, rec), rel = S32(st, rec + 0xC), edu = S32(st, rec + 8);
    int32_t health = S32(st, rec + 0x14), mag = S32(st, rec + 0x18), svc = S32(st, rec + 0x1C);
    int32_t kinds = S32(st, rec + 0x20);
    int32_t goods[4];
    for (uint32_t k = 0; k < 4; k++) {
        goods[k] = S32(st, rec + 0x24 + 4 * k);
        if (U8(st, b + BLD_SIZE) == 1 && goods[k] > 0) {
            goods[k] = (goods[k] + 2) / 4;
            if (goods[k] < 1) {
                goods[k] = 1;
            }
        }
    }
#define FAIL(off) \
    do { \
        S32(st, CURRENT_CITY(st) + (off)) += 1; \
        return 0; \
    } while (0)
    if (U8(st, b + BLD_WATER_SUPPLY) == 0) {
        if (water >= 2) {
            FAIL(CITY_U4304);
        }
        if (water == 1 && U8(st, b + BLD_WELL_ACCESS) == 0) {
            FAIL(CITY_U4308);
        }
    }
    int8_t have = S8(st, b + BLD_ENTERTAINMENT);
    if (ent > have) {
        FAIL(have == 0 ? CITY_U4388 : CITY_U430C);
    }
    have = S8(st, b + BLD_EDUCATION);
    if (rel > have) {
        FAIL(have != 0 ? CITY_U4310 : CITY_U4314);
    }
    uint32_t c = CURRENT_CITY(st);
    if (rel == 2) {
        S32(st, c + CITY_U4318) += 1;
        S32(st, c + CITY_U431C) += 1;
    } else if (rel == 1) {
        S32(st, c + CITY_U4318) += 1;
    }
    have = S8(st, b + BLD_RELIGION);
    if (edu > have) {
        if (edu == 1) {
            FAIL(CITY_U4350);
        } else if (edu == 2) {
            FAIL(CITY_U4354);
        } else if (edu == 3) {
            FAIL(CITY_U4358);
        }
    } else if (edu >= 1) {
        S32(st, c + CITY_U435C) += 1;
    }
    int32_t u = U8(st, b + BLD_ACCESS_DENTIST);
    if (health > u) {
        FAIL(CITY_U4324);
    }
    if (health == 1) {
        S32(st, c + CITY_U4344) += 1;
    }
    u = U8(st, b + BLD_ACCESS_MAGISTRATE);
    if (mag > u) {
        FAIL(u != 0 ? CITY_U4328 : CITY_U474C);
    }
    if (mag == 2) {
        S32(st, c + CITY_U4348) += 1;
        S32(st, c + CITY_U4748) += 1;
    } else if (mag == 1) {
        S32(st, c + CITY_U4348) += 1;
    }
    have = S8(st, b + BLD_HEALTH);
    if (svc > have) {
        FAIL(svc >= 2 ? CITY_U4340 : CITY_U433C);
    }
    if (svc >= 1) {
        S32(st, c + CITY_U434C) += 1;
    }
    /* @bin 0x00453FD1 the kinds of goods held among the city's goods list 0x00F53DDC (count
     * 0x00F53DD8), mapped to the house's goods slots by the switch at 0x00453FFD */
    int32_t held = 0;
    for (int32_t k = 0; k < S32(st, G_GOODS_COUNT); k++) {
        int32_t g = S32(st, G_GOODS_LIST + 4u * (uint32_t) k);
        if (g == 0) {
            continue;
        }
        int32_t slot;
        switch (g) {
        case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: slot = g - 1; break;
        case 13: slot = 9; break;
        case 15: slot = 8; break;
        case 17: slot = 10; break;
        case 19: slot = 11; break;
        default: slot = -1; break;
        }
        if (S16(st, b + BLD_HOUSE_INVENTORY + (uint32_t) (2 * slot)) != 0) {
            held++;
        }
    }
    if (kinds > held) {
        FAIL(CITY_U432C);
    }
#undef FAIL
    if (goods[0] > S16(st, b + BLD_HOUSE_INVENTORY + 2 * 9)) {
        return 0;
    }
    if (goods[3] != 0 && S16(st, b + BLD_HOUSE_INVENTORY + 2 * 8) <= 0) {
        return 0;
    }
    if (goods[1] > S16(st, b + BLD_HOUSE_INVENTORY + 2 * 10)) {
        return 0;
    }
    if (goods[2] > S16(st, b + BLD_HOUSE_INVENTORY + 2 * 11)) {
        return 0;
    }
    if (goods[2] >= 2 && S32(st, CURRENT_CITY(st) + CITY_U46A4) < 2) {
        S32(st, CURRENT_CITY(st) + CITY_U46FC) += 1;
        return 0;
    }
    return 1;
}

static void house_split_part(game_state *st, int32_t n, int32_t people, const int32_t *goods, int32_t img)
{
    uint32_t o = BUILDING_ADDR(n);
    S16(st, o + BLD_POPULATION) = (int16_t) people;
    for (uint32_t j = 0; j < 12; j++) {
        S16(st, o + BLD_HOUSE_INVENTORY + 2 * j) = (int16_t) goods[j];
    }
    S16(st, o + BLD_DISTANCE) = 0;
    img += U8(st, G_GRID_RANDOM + (uint32_t) S32(st, o + BLD_GRID_OFFSET)) & 1;
    GAME_CALL(st, FN_MAP_BUILDING_TILES, n, S16(st, o + BLD_X), S16(st, o + BLD_Y), 1, 1, img, 8);
}

static void house_split_in_four(game_state *st, int32_t id, int to_0x13)
{
    /* @bin 0x0046BE90 / 0x0046C1C0 a 2 x 2 house becomes four 1 x 1 houses of its type (0x13,
     * level 9, for 0x0046C1C0): a quarter of the people and goods each (the house keeps the
     * remainders too); the three new ones by 0x0046A400 at (x + 1, y), (x, y + 1), (x + 1, y + 1) */
    uint32_t b = BUILDING_ADDR(id);
    int32_t people = S16(st, b + BLD_POPULATION);
    int32_t quarter[12], rest[12];
    for (uint32_t j = 0; j < 12; j++) {
        int32_t g = S16(st, b + BLD_HOUSE_INVENTORY + 2 * j);
        quarter[j] = g / 4;
        rest[j] = g % 4;
    }
    int32_t pq = people / 4;
    GAME_CALL(st, FN_BUILDING_CLEAR_TILES, id, S16(st, b + BLD_X), S16(st, b + BLD_Y));
    if (to_0x13) {
        S16(st, b + BLD_TYPE) = 0x13;
        S16(st, b + BLD_SUBTYPE) = 9;
    }
    U8(st, b + BLD_SIZE) = 1;
    U8(st, b + BLD_HOUSE_SIZE) = 1;
    U8(st, b + BLD_HOUSE_MERGED) = 0;
    S16(st, b + BLD_POPULATION) = (int16_t) (people % 4 + pq);
    for (uint32_t j = 0; j < 12; j++) {
        S16(st, b + BLD_HOUSE_INVENTORY + 2 * j) = (int16_t) (rest[j] + quarter[j]);
    }
    S16(st, b + BLD_DISTANCE) = 0;
    int32_t level = S16(st, b + BLD_SUBTYPE);
    int32_t img = house_image(st, level) + S32(st, G_HOUSE_LEVEL_IMAGE_ADD + 4u * (uint32_t) level);
    GAME_CALL(st, FN_MAP_BUILDING_TILES, id, S16(st, b + BLD_X), S16(st, b + BLD_Y), 1, 1,
              (int32_t) (U8(st, G_GRID_RANDOM + (uint32_t) S32(st, b + BLD_GRID_OFFSET)) & 1) + img, 8);
    int32_t x = S16(st, b + BLD_X), y = S16(st, b + BLD_Y);
    static const int8_t at[3][2] = {{1, 0}, {0, 1}, {1, 1}};
    for (int k = 0; k < 3; k++) {
        int32_t n = GAME_CALL(st, FN_BUILDING_CREATE, S32(st, CURRENT_PLAYER_ADDR), S16(st, b + BLD_TYPE), x + at[k][0],
                              y + at[k][1], 0, 0);
        house_split_part(st, n, pq, quarter, img);
    }
}

void house_split_large(game_state *st, int32_t id) { house_split_in_four(st, id, 0); }
void house_devolve_0x14(game_state *st, int32_t id) { house_split_in_four(st, id, 1); }

static void house_shrink(game_state *st, int32_t id, int32_t parts, int16_t type, uint8_t size)
{
    /* @bin 0x0046CB30 / 0x0046CCF0 the house shrinks by one tile on each side to `type`, size x
     * size, keeping 1 / parts of its people and goods plus the remainders; the freed column and
     * row get terrain 0x20, then the plaza pass and the routing grids of the old square */
    uint32_t b = BUILDING_ADDR(id);
    int32_t people = S16(st, b + BLD_POPULATION);
    int32_t part[12], rest[12];
    for (uint32_t j = 0; j < 12; j++) {
        int32_t g = S16(st, b + BLD_HOUSE_INVENTORY + 2 * j);
        part[j] = g / parts;
        rest[j] = g % parts;
    }
    GAME_CALL(st, FN_BUILDING_CLEAR_TILES, id, S16(st, b + BLD_X), S16(st, b + BLD_Y));
    S16(st, b + BLD_TYPE) = type;
    S16(st, b + BLD_SUBTYPE) = (int16_t) (type - 10);
    U8(st, b + BLD_SIZE) = size;
    U8(st, b + BLD_HOUSE_SIZE) = size;
    U8(st, b + BLD_HOUSE_MERGED) = 0;
    S16(st, b + BLD_POPULATION) = (int16_t) (people % parts + people / parts);
    for (uint32_t j = 0; j < 12; j++) {
        S16(st, b + BLD_HOUSE_INVENTORY + 2 * j) = (int16_t) (rest[j] + part[j]);
    }
    S16(st, b + BLD_DISTANCE) = 0;
    int32_t level = S16(st, b + BLD_SUBTYPE);
    int32_t img = house_image(st, level) + S32(st, G_HOUSE_LEVEL_IMAGE_ADD + 4u * (uint32_t) level);
    if (size == 2) {
        img += U8(st, G_GRID_RANDOM + (uint32_t) S32(st, b + BLD_GRID_OFFSET)) & 1;
    }
    GAME_CALL(st, FN_MAP_BUILDING_TILES, id, S16(st, b + BLD_X), S16(st, b + BLD_Y), size, size, img, 8);
    int32_t x = S16(st, b + BLD_X), y = S16(st, b + BLD_Y);
    int32_t tile = S32(st, G_MAP_START) + y * GRID_SIZE + x;
    for (int32_t k = 0; k <= size; k++) {
        U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (tile + size + GRID_SIZE * k)) |= 0x20;
        U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (tile + GRID_SIZE * size + k)) |= 0x20;
    }
    GAME_CALL0(st, FN_FLAG_0X20_ALL);
    GAME_CALL(st, FN_ROUTING_FILL_EE0C50, x, y, x + size, y + size, 1);
    GAME_CALL(st, FN_ROUTING_FILL_EC7630, x, y, x + size, y + size);
}

void house_devolve_0x18(game_state *st, int32_t id) { house_shrink(st, id, 6, 0x17, 2); }
void house_devolve_0x1c(game_state *st, int32_t id) { house_shrink(st, id, 8, 0x1B, 3); }

/* the evolution of one house type (0x00452980..0x00453900): the parts that differ */
typedef struct {
    int16_t type;
    uint32_t addr;
    uint8_t needs_people; /* returns at once without people (+0x1C) */
    uint8_t merge;        /* house_try_merge first */
    uint8_t recheck;      /* a "1" result is checked again for the next level */
    int16_t down;         /* the type below, or 0: a devolution function */
    uint32_t down_fn;
    int16_t up;           /* the type above, or 0: an expansion */
    int32_t expand;       /* tiles of the expansion (0x0046B480, 0x0046B940, house_merge_neighbours) */
    uint32_t expand_fn;   /* its last step */
} house_evolution;

static const house_evolution evolutions[] = {
    {0x0A, FN_HOUSE_EVOLVE_0X0A, 1, 1, 1, 0, 0, 0x0B, 0, 0},
    {0x0B, FN_HOUSE_EVOLVE_0X0B, 1, 1, 1, 0x0A, 0, 0x0C, 0, 0},
    {0x0C, FN_HOUSE_EVOLVE_0X0C, 0, 1, 1, 0x0B, 0, 0x0D, 0, 0},
    {0x0D, FN_HOUSE_EVOLVE_0X0D, 0, 1, 1, 0x0C, 0, 0x0E, 0, 0},
    {0x0E, FN_HOUSE_EVOLVE_0X0E, 0, 1, 1, 0x0D, 0, 0x0F, 0, 0},
    {0x0F, FN_HOUSE_EVOLVE_0X0F, 0, 1, 1, 0x0E, 0, 0x10, 0, 0},
    {0x10, FN_HOUSE_EVOLVE_0X10, 0, 1, 1, 0x0F, 0, 0x11, 0, 0},
    {0x11, FN_HOUSE_EVOLVE_0X11, 0, 1, 1, 0x10, 0, 0x12, 0, 0},
    {0x12, FN_HOUSE_EVOLVE_0X12, 0, 1, 1, 0x11, 0, 0x13, 0, 0},
    {0x13, FN_HOUSE_EVOLVE_0X13, 0, 1, 1, 0x12, 0, 0, 4, FN_HOUSE_GROW_0X14},
    {0x14, FN_HOUSE_EVOLVE_0X14, 0, 0, 1, 0, FN_HOUSE_DEVOLVE_0X14, 0x15, 0, 0},
    {0x15, FN_HOUSE_EVOLVE_0X15, 0, 0, 1, 0x14, 0, 0x16, 0, 0},
    {0x16, FN_HOUSE_EVOLVE_0X16, 0, 0, 1, 0x15, 0, 0x17, 0, 0},
    {0x17, FN_HOUSE_EVOLVE_0X17, 0, 0, 1, 0x16, 0, 0, 9, FN_HOUSE_GROW_0X18},
    {0x18, FN_HOUSE_EVOLVE_0X18, 0, 0, 1, 0, FN_HOUSE_DEVOLVE_0X18, 0x19, 0, 0},
    {0x19, FN_HOUSE_EVOLVE_0X19, 0, 0, 1, 0x18, 0, 0x1A, 0, 0},
    {0x1A, FN_HOUSE_EVOLVE_0X1A, 0, 0, 1, 0x19, 0, 0x1B, 0, 0},
    {0x1B, FN_HOUSE_EVOLVE_0X1B, 0, 0, 1, 0x1A, 0, 0, 0x10, FN_HOUSE_GROW_0X1C},
    {0x1C, FN_HOUSE_EVOLVE_0X1C, 0, 0, 1, 0, FN_HOUSE_DEVOLVE_0X1C, 0x1D, 0, 0},
    {0x1D, FN_HOUSE_EVOLVE_0X1D, 0, 0, 0, 0x1C, 0, 0x1C, 0, 0},
};

static void house_evolve(game_state *st, const house_evolution *e)
{
    int32_t id = S32(st, G_HOUSE_ID);
    if (e->needs_people && S16(st, BUILDING_ADDR(id) + BLD_POPULATION) <= 0) {
        return;
    }
    if (e->merge) {
        house_try_merge(st, S32(st, G_HOUSE_ID));
    }
    if (houses_redraw_only(st)) {
        return;
    }
    S32(st, G_HOUSE_RESULT) = house_desirability_trend(st, S32(st, G_HOUSE_ID));
    int32_t v;
    if (!house_can_evolve(st, S32(st, G_HOUSE_ID), 0)) {
        S32(st, G_HOUSE_RESULT) = -1;
        v = -1;
    } else {
        v = S32(st, G_HOUSE_RESULT);
        if (e->recheck && v == 1) {
            v = house_can_evolve(st, S32(st, G_HOUSE_ID), v);
            S32(st, G_HOUSE_RESULT) = v;
        }
    }
    if (e->type == 0x0A) {
        /* @bin 0x00452980 the lowest house never devolves and keeps no delay */
        if (v != 0 && v != -1) {
            house_set_type(st, S32(st, G_HOUSE_ID), e->up);
        }
        return;
    }
    id = S32(st, G_HOUSE_ID);
    uint32_t b = BUILDING_ADDR(id);
    if (v == -1) {
        /* devolution after three days (+0x84) */
        if (U8(st, b + BLD_DEVOLVE_DAYS) <= 1) {
            U8(st, b + BLD_DEVOLVE_DAYS) += 1;
            return;
        }
        U8(st, b + BLD_DEVOLVE_DAYS) = 0;
        if (e->down != 0) {
            house_set_type(st, id, e->down);
        } else if (e->down_fn == FN_HOUSE_DEVOLVE_0X14) {
            house_devolve_0x14(st, id);
        } else if (e->down_fn == FN_HOUSE_DEVOLVE_0X18) {
            house_devolve_0x18(st, id);
        } else {
            house_devolve_0x1c(st, id);
        }
        return;
    }
    U8(st, b + BLD_DEVOLVE_DAYS) = 0;
    if (v == 0) {
        return;
    }
    if (e->up != 0) {
        house_set_type(st, id, e->up);
        return;
    }
    /* @bin 0x00453168 expansion to a larger house */
    if (house_find_expansion(st, id, e->expand) == 0) {
        return;
    }
    if (e->type == 0x13) {
        U8(st, BUILDING_ADDR(S32(st, G_HOUSE_ID)) + BLD_HOUSE_MERGED) = 0;
    }
    house_clear_expansion(st, S32(st, G_HOUSE_ID), e->expand);
    int32_t people = house_merge_neighbours(st, S32(st, G_HOUSE_ID), e->expand);
    if (e->expand_fn == FN_HOUSE_GROW_0X14) {
        house_grow_0x14(st, S32(st, G_HOUSE_ID), people);
    } else if (e->expand_fn == FN_HOUSE_GROW_0X18) {
        house_grow_0x18(st, S32(st, G_HOUSE_ID), people);
    } else {
        house_grow_0x1c(st, S32(st, G_HOUSE_ID), people);
    }
    S32(st, G_HOUSES_EXPANDED) = 1;
    GAME_CALL0(st, FN_FLAG_0X20_ALL);
}

static const house_evolution *evolution_of(uint32_t addr)
{
    for (unsigned i = 0; i < sizeof(evolutions) / sizeof(evolutions[0]); i++) {
        if (evolutions[i].addr == addr) {
            return &evolutions[i];
        }
    }
    return NULL;
}

void houses_evolve_all(game_state *st)
{
    /* @bin 0x004528B0 day step 0x27: every house (types 0x0A..0x1D) is checked (0x0046B740) and
     * evolved by its type's function (table 0x005DA3D4, 0x0078D7E4 := 0 before); on days 0 and 7
     * it uses its goods; the routing grids are rebuilt when 0x0078D588 was set */
    houses_counters_clear(st);
    S32(st, G_HOUSES_EXPANDED) = 0;
    for (S32(st, G_HOUSE_ID) = 1; S32(st, G_HOUSE_ID) < 0xFA0; S32(st, G_HOUSE_ID) += 1) {
        int32_t id = S32(st, G_HOUSE_ID);
        uint32_t b = BUILDING_ADDR(id);
        int16_t type = S16(st, b + BLD_TYPE);
        if (U8(st, b + BLD_STATE) != 1 || type < 0xA || type > 0x1D) {
            continue;
        }
        building_check_position(st, id);
        S32(st, G_HOUSE_RESULT) = 0;
        id = S32(st, G_HOUSE_ID);
        uint32_t fn = U32(st, G_HOUSE_EVOLVE_FNS + 4u * (uint32_t) (int32_t) S16(st, BUILDING_ADDR(id) + BLD_TYPE));
        const house_evolution *e = evolution_of(fn);
        if (e) {
            house_evolve(st, e);
        } else {
            GAME_CALL0(st, fn);
        }
        if (S32(st, G_TICK_DAY) == 0) {
            house_consume(st, S32(st, G_HOUSE_ID));
        }
        if (S32(st, G_TICK_DAY) == 7) {
            house_consume(st, S32(st, G_HOUSE_ID));
        }
    }
    if (S32(st, G_HOUSES_EXPANDED) != 0) {
        GAME_CALL0(st, FN_ROUTING_REBUILD_EE0C50);
        GAME_CALL0(st, FN_ROUTING_REBUILD_EC7630);
    }
}

static int32_t run_counters_clear(game_state *st, const int32_t *a)
{
    (void) a;
    houses_counters_clear(st);
    return 0;
}

static int32_t run_check_position(game_state *st, const int32_t *a)
{
    building_check_position(st, a[0]);
    return 0;
}

static int32_t run_consume(game_state *st, const int32_t *a)
{
    house_consume(st, a[0]);
    return 0;
}

static int32_t run_evolve_all(game_state *st, const int32_t *a)
{
    (void) a;
    houses_evolve_all(st);
    return 0;
}

#define HOUSE_RUN(n) \
    static int32_t run_evolve_##n(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        house_evolve(st, &evolutions[n]); \
        return 0; \
    }
HOUSE_RUN(0) HOUSE_RUN(1) HOUSE_RUN(2) HOUSE_RUN(3) HOUSE_RUN(4) HOUSE_RUN(5) HOUSE_RUN(6) HOUSE_RUN(7)
HOUSE_RUN(8) HOUSE_RUN(9) HOUSE_RUN(10) HOUSE_RUN(11) HOUSE_RUN(12) HOUSE_RUN(13) HOUSE_RUN(14)
HOUSE_RUN(15) HOUSE_RUN(16) HOUSE_RUN(17) HOUSE_RUN(18) HOUSE_RUN(19)

static int32_t run_find_expansion(game_state *st, const int32_t *a)
{
    return house_find_expansion(st, a[0], a[1]);
}

static int32_t run_clear_expansion(game_state *st, const int32_t *a)
{
    house_clear_expansion(st, a[0], a[1]);
    return 0;
}

static int32_t run_grow_14(game_state *st, const int32_t *a)
{
    house_grow_0x14(st, a[0], a[1]);
    return 0;
}

static int32_t run_grow_18(game_state *st, const int32_t *a)
{
    house_grow_0x18(st, a[0], a[1]);
    return 0;
}

static int32_t run_grow_1c(game_state *st, const int32_t *a)
{
    house_grow_0x1c(st, a[0], a[1]);
    return 0;
}

static int32_t run_split_large(game_state *st, const int32_t *a)
{
    house_split_large(st, a[0]);
    return 0;
}

static int32_t run_devolve_14(game_state *st, const int32_t *a)
{
    house_devolve_0x14(st, a[0]);
    return 0;
}

static int32_t run_devolve_18(game_state *st, const int32_t *a)
{
    house_devolve_0x18(st, a[0]);
    return 0;
}

static int32_t run_devolve_1c(game_state *st, const int32_t *a)
{
    house_devolve_0x1c(st, a[0]);
    return 0;
}

static int32_t run_set_type(game_state *st, const int32_t *a)
{
    house_set_type(st, a[0], a[1]);
    return 0;
}

static int32_t run_redraw_only(game_state *st, const int32_t *a)
{
    (void) a;
    return houses_redraw_only(st);
}

static int32_t run_trend(game_state *st, const int32_t *a)
{
    return house_desirability_trend(st, a[0]);
}

static int32_t run_merge_neighbours(game_state *st, const int32_t *a)
{
    return house_merge_neighbours(st, a[0], a[1]);
}

static int32_t run_become_large(game_state *st, const int32_t *a)
{
    house_become_large(st, a[0], a[1]);
    return 0;
}

static int32_t run_try_merge(game_state *st, const int32_t *a)
{
    return house_try_merge(st, a[0]);
}

static int32_t run_can_evolve(game_state *st, const int32_t *a)
{
    return house_can_evolve(st, a[0], a[1]);
}

const ported_function day_houses_ports[] = {
    {0x0046CEA0u, "house_set_type", 2, 0, 0, run_set_type, 0},
    {0x0046BE90u, "house_split_large", 1, 0, 0, run_split_large, 0},
    {0x0046C1C0u, "house_devolve_0x14", 1, 0, 0, run_devolve_14, 0},
    {0x0046CB30u, "house_devolve_0x18", 1, 0, 0, run_devolve_18, 0},
    {0x0046CCF0u, "house_devolve_0x1c", 1, 0, 0, run_devolve_1c, 0},
    {0x0046B480u, "house_find_expansion", 2, 32, 0, run_find_expansion, 0},
    {0x0046B940u, "house_clear_expansion", 2, 0, 0, run_clear_expansion, 0},
    {0x0046BAB0u, "house_grow_0x14", 2, 0, 0, run_grow_14, 0},
    {0x0046BBC0u, "house_grow_0x18", 2, 0, 0, run_grow_18, 0},
    {0x0046BCB0u, "house_grow_0x1c", 2, 0, 0, run_grow_1c, 0},
    {0x004539A0u, "houses_redraw_only", 0, 32, 0, run_redraw_only, 0},
    {0x00453AF0u, "house_desirability_trend", 1, 32, 0, run_trend, 0},
    {0x0046B9E0u, "house_merge_neighbours", 2, 32, 0, run_merge_neighbours, 0},
    {0x0046BDA0u, "house_become_large", 2, 0, 0, run_become_large, 0},
    {0x0046B840u, "house_try_merge", 1, 32, 0, run_try_merge, 0},
    {0x00453B40u, "house_can_evolve", 2, 32, 0, run_can_evolve, 0},
    {0x00452980u, "house_evolve_0x0a", 0, 0, 0, run_evolve_0, 0},
    {0x00452A20u, "house_evolve_0x0b", 0, 0, 0, run_evolve_1, 0},
    {0x00452B00u, "house_evolve_0x0c", 0, 0, 0, run_evolve_2, 0},
    {0x00452BD0u, "house_evolve_0x0d", 0, 0, 0, run_evolve_3, 0},
    {0x00452CA0u, "house_evolve_0x0e", 0, 0, 0, run_evolve_4, 0},
    {0x00452D70u, "house_evolve_0x0f", 0, 0, 0, run_evolve_5, 0},
    {0x00452E40u, "house_evolve_0x10", 0, 0, 0, run_evolve_6, 0},
    {0x00452F10u, "house_evolve_0x11", 0, 0, 0, run_evolve_7, 0},
    {0x00452FE0u, "house_evolve_0x12", 0, 0, 0, run_evolve_8, 0},
    {0x004530B0u, "house_evolve_0x13", 0, 0, 0, run_evolve_9, 0},
    {0x004531C0u, "house_evolve_0x14", 0, 0, 0, run_evolve_10, 0},
    {0x00453280u, "house_evolve_0x15", 0, 0, 0, run_evolve_11, 0},
    {0x00453340u, "house_evolve_0x16", 0, 0, 0, run_evolve_12, 0},
    {0x00453400u, "house_evolve_0x17", 0, 0, 0, run_evolve_13, 0},
    {0x00453500u, "house_evolve_0x18", 0, 0, 0, run_evolve_14, 0},
    {0x004535C0u, "house_evolve_0x19", 0, 0, 0, run_evolve_15, 0},
    {0x00453680u, "house_evolve_0x1a", 0, 0, 0, run_evolve_16, 0},
    {0x00453740u, "house_evolve_0x1b", 0, 0, 0, run_evolve_17, 0},
    {0x00453840u, "house_evolve_0x1c", 0, 0, 0, run_evolve_18, 0},
    {0x00453900u, "house_evolve_0x1d", 0, 0, 0, run_evolve_19, 0},
    {0x00454D70u, "houses_counters_clear", 0, 0, 0, run_counters_clear, 0},
    {0x0046B740u, "building_check_position", 1, 0, 0, run_check_position, 0},
    {0x00454B40u, "house_consume", 1, 0, 0, run_consume, 0},
    {0x004528B0u, "houses_evolve_all", 0, 0, 0, run_evolve_all, 0},
};
const int day_houses_ports_count = sizeof(day_houses_ports) / sizeof(day_houses_ports[0]);
