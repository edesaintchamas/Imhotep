/* Imhotep - more helpers of the daily building update 0x00462180 (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Building record: docs/formats/save.md §5 (chunk 32), figure record: chunk 21. */
#include <string.h>

#include "building/building_spawn.h"
#include "figure/formation.h"
#include "sim/call.h"
#include "sim/math.h"
#include "state/ported.h"
#include "state/records.h"

#define ROAD_X 0x00D3B4E4u
#define ROAD_Y 0x00D3B4E0u
#define MAP_START 0x00BCA2D8u
#define TERRAIN 0x00AF598Cu       /* u32 per tile */
#define BUILDING_GRID 0x00B4E6FCu /* i16 per tile */
#define ADJACENT 0x005CC9E0u      /* per size 0x60 bytes: up to 24 i32 grid offsets around it, 0 ends */
#define ADJACENT_XY 0x005CCE98u   /* per size 24 x (i32 dx, i32 dy) */
#define FIGURE_TYPES 0x007090D0u  /* records of 0x38 bytes per figure type */

void building_formation_size(game_state *st, int32_t id)
{
    /* @bin 0x004B76B0 the formation of building `id` (+0x50): +0x46 asks for figures of the kind of
     * +0x12 while it has fewer (+0x28) than it may (+0x29); extra figures go (action 10) */
    uint32_t b = BUILDING_ADDR(id);
    uint32_t f = FORMATION_ADDR(S16(st, b + 0x50));
    S16(st, f + 0x46) = 0;
    if (U8(st, f + 0x03) == 0 || S16(st, f + 0x64) != 0) {
        return;
    }
    uint8_t have = U8(st, f + 0x28), may = U8(st, f + 0x29);
    if (have == may) {
        return;
    }
    if (have < may) {
        int16_t kind = S16(st, b + 0x12);
        if (kind == 0xD) {
            S16(st, f + 0x46) = 3;
        }
        if (kind == 0xB) {
            S16(st, f + 0x46) = 1;
        }
        if (kind == 0xC) {
            S16(st, f + 0x46) = 2;
        }
        return;
    }
    int32_t extra = have - may;
    for (int k = 15; k >= 0; k--) {
        int16_t fid = S16(st, f + 0x08u + 2u * (uint32_t) k);
        if (fid > 0) {
            extra--;
            U8(st, FIGURE_ADDR(fid) + 0x38) = 10;
            if (extra <= 0) {
                break;
            }
        }
    }
    formations_rebuild(st);
}

void building_cycle_image(game_state *st, int32_t id)
{
    /* @bin 0x0047C260 +0x54 counts 0..4; the building's image (0x00B7482C) := 0x00F565DC + it */
    uint32_t b = BUILDING_ADDR(id);
    int32_t g = S32(st, b + 0x0C);
    S16(st, b + 0x54) += 1;
    if (S16(st, b + 0x54) >= 5) {
        S16(st, b + 0x54) = 0;
    }
    S32(st, 0x00B7482Cu + 4u * (uint32_t) g) = S16(st, b + 0x54) + S32(st, 0x00F565DCu);
}

int32_t figure_group_new(game_state *st)
{
    /* @bin 0x0046F140 the first free record of 0x2C bytes at 0x009784D8 (1 .. 49), cleared, in use */
    for (int32_t i = 1; i < 50; i++) {
        uint32_t r = 0x009784D8u + 0x2Cu * (uint32_t) i;
        if (U8(st, r) == 0) {
            memset(state_ptr(st, r, 0x2C), 0, 0x2C); /* 0x0040F8F0 */
            U8(st, r) = 1;
            return i;
        }
    }
    return 0;
}

/* the road tile at index i of the size's ring, as (x, y) in 0x00D3B4E4 / 0x00D3B4E0 */
static void adjacent_xy(game_state *st, int32_t x, int32_t y, int32_t size, int32_t i)
{
    uint32_t e = ADJACENT_XY + 8u * (uint32_t) (i + size * 24);
    S32(st, ROAD_X) = S32(st, e) + x;
    S32(st, ROAD_Y) = S32(st, e + 4) + y;
}

int32_t building_find_open_tile(game_state *st, int32_t x, int32_t y, int32_t size)
{
    /* @bin 0x004807D0 a tile around the building with terrain bit 0x40, or none of the bits of
     * 0xEEFED77F */
    int32_t base = S32(st, MAP_START) + y * GRID_SIZE + x;
    for (int32_t i = 0; i < 24; i++) {
        int32_t o = S32(st, ADJACENT + 0x60u * (uint32_t) size + 4u * (uint32_t) i);
        if (o == 0) {
            return 0;
        }
        uint32_t v = U32(st, TERRAIN + 4u * (uint32_t) (o + base)) & 0xEEFED77Fu;
        if ((v & 0x40) || v == 0) {
            adjacent_xy(st, x, y, size, i);
            return 1;
        }
    }
    return 0;
}

int32_t building_find_road_8(game_state *st, int32_t x, int32_t y, int32_t size)
{
    /* @bin 0x004806C0 a road tile (terrain 4 without 8) around the building with 8 or more
     * (0x00484F40 after 0x0047DB00(4) on it) */
    int32_t base = S32(st, MAP_START) + y * GRID_SIZE + x;
    for (int32_t i = 0; i < 24; i++) {
        int32_t o = S32(st, ADJACENT + 0x60u * (uint32_t) size + 4u * (uint32_t) i);
        if (o == 0) {
            return 0;
        }
        int32_t t = o + base;
        uint32_t v = U32(st, TERRAIN + 4u * (uint32_t) t);
        if ((v & 4) && !(v & 8)) {
            S32(st, 0x00BC9768u) = t;
            GAME_CALL(st, 0x0047DB00u, 4);
            if (GAME_CALL0(st, 0x00484F40u) >= 8) {
                adjacent_xy(st, x, y, size, i);
                return 1;
            }
        }
    }
    return 0;
}

int32_t building_find_edge_tile(game_state *st, int32_t x, int32_t y, int32_t orientation)
{
    /* @bin 0x00480460 on the border of a 9 x 15 rectangle around (x, y) (15 x 9 unless the
     * orientation is 0 or 4), the tile with bit 0x40 and not 4 (not of a building of type 0x88 /
     * 0x8A) whose grid 0x00A76AEC value comes first in the city's list (+0x46AC, 10 x 8 bytes) */
    int32_t rows, cols, x0, y0;
    if (orientation == 0 || orientation == 4) {
        rows = 9;
        cols = 15;
        x0 = x - 1;
        y0 = y - 3;
    } else {
        rows = 15;
        cols = 9;
        x0 = x - 3;
        y0 = y - 1;
    }
    int32_t start = S32(st, MAP_START) + y0 * GRID_SIZE + x0;
    int32_t best = 12, found = start;
    for (int32_t c = 0; c < cols; c++) {
        int32_t step = c == 0 || c == cols - 1 ? 1 : rows - 1;
        int32_t t = start + c;
        for (int32_t r = 0; r < rows; r += step, t += GRID_SIZE * step) {
            uint32_t v = U32(st, TERRAIN + 4u * (uint32_t) t);
            if (v & 8) {
                int16_t type = S16(st, BUILDING_ADDR(S16(st, BUILDING_GRID + 2u * (uint32_t) t)) + 0x10);
                if (type == 0x8A || type == 0x88) {
                    continue;
                }
            }
            if (!(v & 0x40) || (v & 4)) {
                continue;
            }
            uint32_t k = U8(st, 0x00A76AECu + (uint32_t) t);
            uint32_t list = CURRENT_CITY(st) + 0x46ACu;
            int32_t rank = 11;
            for (int32_t i = 0; i < 10; i++) {
                if (U32(st, list + 8u * (uint32_t) i) == k) {
                    rank = i;
                    break;
                }
            }
            if (rank < best) {
                best = rank;
                found = t;
            }
        }
    }
    if (best >= 12) {
        return 0;
    }
    int32_t o = found - S32(st, MAP_START);
    S32(st, ROAD_X) = o % GRID_SIZE;
    S32(st, ROAD_Y) = o / GRID_SIZE;
    return 1;
}

int32_t building_find_missing_worker(game_state *st, int32_t type)
{
    /* @bin 0x00460EA0 the first building of the type matching figure `type` (0x19: 0x4C, 0x4D: 0xB5,
     * else 0xB6) whose figure +0xB6 is not out (cleared then) */
    int32_t btype = type == 0x19 ? 0x4C : (type != 0x4D) + 0xB5;
    for (int32_t id = 1; id < BUILDING_COUNT; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b) == 0 || S16(st, b + 0x10) != btype) {
            continue;
        }
        int16_t f = S16(st, b + 0xB6);
        if (f <= 0) {
            return id;
        }
        uint32_t a = FIGURE_ADDR(f);
        uint8_t t = U8(st, a + 0x0A);
        if (U8(st, a + 0x0E) == 0 || ((int32_t) t != type && t != 0) || S16(st, a + 0x5A) != id) {
            S16(st, b + 0xB6) = 0;
            return id;
        }
    }
    return 0;
}

void figure_init_type_values(game_state *st, int32_t id)
{
    /* @bin 0x004620C0 from the figure type's record (0x007090D0) */
    uint32_t f = FIGURE_ADDR(id);
    S32(st, f + 0x124) = 0;
    S32(st, f + 0x128) = S32(st, FIGURE_TYPES + 0x38u * U8(st, f + 0x0A));
    S32(st, f + 0x12C) = S32(st, FIGURE_TYPES + 0x38u * U8(st, f + 0x0A));
    U8(st, f + 0x130) = 0;
    S32(st, f + 0x13C) = S32(st, FIGURE_TYPES + 0x1Cu + 0x38u * U8(st, f + 0x0A));
    U8(st, f + 0x154) = 0;
}

void figure_init_type_values_full(game_state *st, int32_t id)
{
    /* @bin 0x00461F90 */
    uint32_t f = FIGURE_ADDR(id);
    uint32_t t = FIGURE_TYPES + 0x38u * U8(st, f + 0x0A);
    S32(st, f + 0x124) = 0;
    S32(st, f + 0x128) = S32(st, t);
    S32(st, f + 0x12C) = S32(st, t);
    U8(st, f + 0x130) = 0;
    S32(st, f + 0x138) = 0;
    S32(st, f + 0x134) = 0;
    S32(st, f + 0x13C) = S32(st, t + 0x1C);
    S32(st, f + 0x140) = S32(st, t + 0x24);
    S32(st, f + 0x144) = S32(st, t + 0x2C);
    S32(st, f + 0x148) = S32(st, t + 0x30);
    S32(st, f + 0x150) = S32(st, t + 0x04);
    U8(st, f + 0x154) = 0;
    S32(st, f + 0x158) = calc_percentage(S32(st, t + 0x04), 20); /* 0x004CDE40 */
}

#define VOID1(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        fn(st, a[0]); \
        return 0; \
    }
#define INT3(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) { return fn(st, a[0], a[1], a[2]); }
VOID1(run_formation_size, building_formation_size)
VOID1(run_cycle_image, building_cycle_image)
VOID1(run_type_values, figure_init_type_values)
VOID1(run_type_values_full, figure_init_type_values_full)
INT3(run_open_tile, building_find_open_tile)
INT3(run_road_8, building_find_road_8)
INT3(run_edge_tile, building_find_edge_tile)

static int32_t run_group_new(game_state *st, const int32_t *a)
{
    (void) a;
    return figure_group_new(st);
}

static int32_t run_missing_worker(game_state *st, const int32_t *a)
{
    return building_find_missing_worker(st, a[0]);
}

const ported_function bigb_misc_ports[] = {
    {0x004B76B0u, "building_formation_size", 1, 0, 0, run_formation_size, 0},
    {0x0047C260u, "building_cycle_image", 1, 0, 0, run_cycle_image, 0},
    {0x0046F140u, "figure_group_new", 0, 32, 0, run_group_new, 0},
    {0x004807D0u, "building_find_open_tile", 3, 32, 0, run_open_tile, 0},
    {0x004806C0u, "building_find_road_8", 3, 32, 0, run_road_8, 0},
    {0x00480460u, "building_find_edge_tile", 3, 32, 0, run_edge_tile, 0},
    {0x00460EA0u, "building_find_missing_worker", 1, 32, 0, run_missing_worker, 0},
    {0x004620C0u, "figure_init_type_values", 1, 0, 0, run_type_values, 0},
    {0x00461F90u, "figure_init_type_values_full", 1, 0, 0, run_type_values_full, 0},
};
const int bigb_misc_ports_count = sizeof(bigb_misc_ports) / sizeof(bigb_misc_ports[0]);

int32_t building_distance(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t a, int32_t b, game_state *st)
{
    /* @bin 0x0045DC50 the larger of |x1 - x2| and |y1 - y2| (kept in 0x00D3B4FC, 0x00D3B4F8) plus
     * |a - b| (0 when a is -1) */
    int32_t extra = a > b ? a - b : b - a;
    if (a == -1) {
        extra = 0;
    }
    int32_t dx = x1 > x2 ? x1 - x2 : x1 < x2 ? x2 - x1 : 0;
    S32(st, 0x00D3B4FCu) = dx;
    int32_t dy = y1 > y2 ? y1 - y2 : y1 < y2 ? y2 - y1 : 0;
    S32(st, 0x00D3B4F8u) = dy;
    return (dx < dy ? dy : dx) + extra;
}

int32_t building_find_supplier(game_state *st, int32_t id, int32_t good, int32_t big_only)
{
    /* @bin 0x0045BB50 the nearest building (0x0045DC50) among the records 0x007900BC: of type 0x48
     * holding `good` in its 8 parts (400 or more when big_only; of kind 2 only when big_only), or of
     * type 0x47 with 400 of it (goods 1..8, not of kind 2); of the same +0x14 unless the building
     * is of the types below; its road tile in 0x00D3B4E4 / 0x00D3B4E0 */
    uint32_t b = BUILDING_ADDR(id);
    int good_range = good > 0 && good <= 8;
    if (S32(st, 0x00EA42F8u + 4u * (U32(st, CURRENT_PLAYER_ADDR) * 0x1276u + (uint32_t) good)) != 0) {
        return 0;
    }
    int same_city = 1;
    switch (S16(st, b + 0x10)) {
    case 0x47: case 0x48:
        if (U8(st, 0x007900C2u + 0xC4u * U8(st, b + 0xC1) + (uint32_t) good) == 2) {
            same_city = 0;
        }
        break;
    case 0xB7: case 0xD5: case 0xDE: case 0xE1: case 0xE3: case 0xE4: case 0xE5: case 0xE6: case 0xEA:
    case 0xEB: case 0xEC:
        same_city = 0;
        break;
    default:
        break;
    }
    int32_t lowest = 10000, best = 0;
    for (int32_t i = 1; i <= S32(st, 0x0078D788u); i++) {
        uint32_t r = 0x007900BCu + 0xC4u * (uint32_t) i;
        if (U8(st, r + 4) == 0) {
            continue;
        }
        int32_t sid = S16(st, r);
        if (sid == 0) {
            continue;
        }
        uint32_t s = BUILDING_ADDR(sid);
        if (U8(st, s) != 1 || sid == id) {
            continue;
        }
        if (same_city && U8(st, s + 0x14) != U8(st, b + 0x14)) {
            continue;
        }
        uint8_t kind = U8(st, r + 6 + (uint32_t) good);
        if (S16(st, s + 0x10) == 0x48) {
            int32_t sum = 0, cur = sid;
            for (int k = 0; k < 8; k++) {
                cur = S16(st, BUILDING_ADDR(cur) + 0x3A);
                if (cur > 0) {
                    uint32_t p = BUILDING_ADDR(cur);
                    int16_t v = S16(st, p + 0x3C);
                    if (v > 0 && S16(st, p + 0x12) == good) {
                        sum += v;
                    }
                }
            }
            if ((sum < 400 && (uint8_t) big_only != 0) || sum <= 0 || (kind == 2 && (uint8_t) big_only == 0)) {
                continue;
            }
        } else if (!good_range || S16(st, s + 0x10) != 0x47 || S16(st, s + 0x56u + 2u * (uint32_t) good) < 400 ||
                   kind == 2) {
            continue;
        }
        int32_t d = building_distance(S16(st, s + 6), S16(st, s + 8), S16(st, b + 6), S16(st, b + 8),
                                      S16(st, b + 0x20), S16(st, s + 0x20), st);
        if (d < lowest) {
            lowest = d;
            best = sid;
        }
    }
    if (best <= 0) {
        return 0;
    }
    uint32_t s = BUILDING_ADDR(best);
    S32(st, 0x00D3B4E0u) = S16(st, s + 0x28);
    S32(st, 0x00D3B4E4u) = S16(st, s + 0x26);
    return best;
}

int32_t storage_fill_level(int32_t amount)
{
    /* @bin 0x0045B490 400: 3, 300..399: 2, 200..299: 1, else 0 */
    if (amount == 400) {
        return 3;
    }
    if (amount < 400 && amount >= 300) {
        return 2;
    }
    if (amount < 300 && amount >= 200) {
        return 1;
    }
    return 0;
}

int32_t storage_take(game_state *st, int32_t player, int32_t id, int32_t good, int32_t amount)
{
    /* @bin 0x0045B570 from the parts (+0x3A chain, 8) of a building of type 0x48 / 0xC1 holding
     * `good`, the least filled first: takes `amount`, counted in the city's arrays 0x00EA60D0 (+)
     * and 0x00EA6160 (-); each part's image (0x00B7482C) by its fill; returns what is left */
    if (amount <= 0) {
        return 0;
    }
    int16_t type = S16(st, BUILDING_ADDR(id) + 0x10);
    if (type != 0x48 && type != 0xC1) {
        return amount;
    }
    int more;
    do {
        int32_t lowest = 401, part = 0, cur = id;
        for (int k = 0; k < 8; k++) {
            cur = S16(st, BUILDING_ADDR(cur) + 0x3A);
            if (cur > 0) {
                uint32_t p = BUILDING_ADDR(cur);
                int16_t v = S16(st, p + 0x3C);
                if (S16(st, p + 0x12) == good && v > 0 && v < lowest) {
                    lowest = v;
                    part = cur;
                }
            }
        }
        if (part == 0) {
            return amount;
        }
        uint32_t p = BUILDING_ADDR(part);
        uint32_t idx = 4u * ((uint32_t) player * 0x1276u + (uint32_t) good);
        int16_t have = S16(st, p + 0x3C);
        more = 0;
        if (have <= amount) {
            amount -= have;
            S16(st, p + 0x3C) = 0;
            S16(st, p + 0x12) = 0;
            S32(st, 0x00EA60D0u + idx) += have;
            S32(st, 0x00EA6160u + idx) -= have;
            more = amount > 0;
        } else {
            S16(st, p + 0x3C) = (int16_t) (have - amount);
            S32(st, 0x00EA60D0u + idx) += amount;
            S32(st, 0x00EA6160u + idx) -= amount;
            amount = 0;
        }
        int32_t g = S32(st, p + 0x0C);
        int16_t left = S16(st, p + 0x3C);
        int32_t image;
        if (left > 0) {
            /* @bin 0x0045B6A6 goods below 0x1F: images from 0x00F566BC, the others from 0x00F57204 */
            image = good < 0x1F ? S32(st, 0x00F566BCu) + good * 4 - 4 + storage_fill_level(left)
                                : S32(st, 0x00F57204u) + good * 4 - 0x7C + storage_fill_level(left);
        } else {
            image = S32(st, 0x00F566B4u);
        }
        S32(st, 0x00B7482Cu + 4u * (uint32_t) g) = image;
    } while (more);
    return amount;
}

int32_t storage_take_stock(game_state *st, int32_t player, int32_t id, int32_t good, int32_t amount)
{
    /* @bin 0x0045E360 from the building's stock of `good` (+0x56 + 2 good) to +0x56, counted in the
     * city's array 0x00EA60AC (-); returns what is left */
    if (amount <= 0) {
        return 0;
    }
    uint32_t b = BUILDING_ADDR(id);
    uint32_t slot = b + 0x56u + 2u * (uint32_t) good;
    uint32_t c = 0x00EA60ACu + 4u * ((uint32_t) player * 0x1276u + (uint32_t) good);
    int16_t have = S16(st, slot);
    if (have <= amount) {
        S16(st, b + 0x56) = (int16_t) (S16(st, b + 0x56) + have);
        S16(st, slot) = 0;
        S32(st, c) -= have;
        return amount - have;
    }
    S16(st, slot) = (int16_t) (have - amount);
    S32(st, c) -= amount;
    S16(st, b + 0x56) = (int16_t) (S16(st, b + 0x56) + amount);
    return 0;
}

/* @bin 0x0045C130 the most a building of `type` holds in +0x3C / +0xCC */
static int32_t stock_capacity(int16_t type)
{
    switch (type) {
    case 0x20: case 0x33:
        return 300;
    case 0x2F: case 0x35:
        return 500;
    case 0x54: case 0x55: case 0x56: case 0x57: case 0xB8: case 0xBB: case 0xBC: case 0xBD:
        return 100000;
    default:
        return 200;
    }
}

static int good_blocked(game_state *st, int32_t good)
{
    return S32(st, 0x00EA42F8u + 4u * (U32(st, CURRENT_PLAYER_ADDR) * 0x1276u + (uint32_t) good)) != 0;
}

/* @bin 0x0045C17E a part of the storage (+0x3A chain of 8) with some of `good` (not blocked) */
static int parts_hold(game_state *st, int32_t id, int32_t good)
{
    int32_t cur = id;
    for (int k = 0; k < 8; k++) {
        cur = S16(st, BUILDING_ADDR(cur) + 0x3A);
        if (cur > 0) {
            uint32_t p = BUILDING_ADDR(cur);
            if (S16(st, p + 0x3C) > 0 && !good_blocked(st, S16(st, p + 0x12)) && S16(st, p + 0x12) == good) {
                return 1;
            }
        }
    }
    return 0;
}

int32_t storage_cart_good(game_state *st, int32_t id, int32_t pending)
{
    /* @bin 0x0045BF00 the good a cart of storage `id` (type 0x48) carries: -1 none (fewer than half
     * the workers); 0 with a good of kind 2 still to fill (`pending`; 0x0078D58C := it); goods 0x1C
     * and 10 for the building of city +0x44AC; then a good a workshop of the same zone (grid
     * 0x00A76AEC) needs; then the first of kind 3 */
    uint32_t b = BUILDING_ADDR(id);
    uint32_t rec = U8(st, b + 0xC1);
    uint8_t zone = U8(st, 0x00A76AECu + (uint32_t) (S32(st, 0x00BCA2D8u) + S16(st, b + 0x26) + S16(st, b + 0x28) * GRID_SIZE));
    int32_t r = GAME_CALL(st, 0x004CDE60u, S16(st, b + 0x40),
                          S32(st, 0x00705EB8u + 0x28u * (uint32_t) (int32_t) S16(st, b + 0x10)));
    if (r < 50) {
        return -1;
    }
    if (pending != 0) {
        for (int32_t g = 1; g < 0x24; g++) {
            if (U8(st, 0x007900C2u + 0xC4u * rec + (uint32_t) g) != 2 || good_blocked(st, g)) {
                continue;
            }
            if (building_parts_stock(st, id, g) <= S16(st, 0x00790134u + 0xC4u * rec + 2u * (uint32_t) g)) {
                S32(st, 0x0078D58Cu) = g;
                return 0;
            }
        }
    }
    if (S32(st, 0x007058B8u) > 0 && S32(st, 0x00BCB9B4u) != 0) {
        int32_t kid = S32(st, CURRENT_CITY(st) + 0x44AC);
        uint32_t k = BUILDING_ADDR(kid);
        if (U8(st, b + 0x14) == U8(st, k + 0x14)) {
            int16_t s3c = S16(st, k + 0x3C), scc = S16(st, k + 0xCC);
            int32_t cap = stock_capacity(S16(st, k + 0x10));
            /* @bin 0x0045C119 the fuller of the two first */
            if (s3c > scc) {
                if (scc + S16(st, k + 0x106) * 100 < cap && parts_hold(st, id, 0x1C)) {
                    return 0x1C;
                }
                if (s3c + S16(st, k + 0x104) * 100 < cap && parts_hold(st, id, 10)) {
                    return 10;
                }
            } else {
                if (s3c + S16(st, k + 0x104) * 100 < cap && parts_hold(st, id, 10)) {
                    return 10;
                }
                if (scc + S16(st, k + 0x106) * 100 < cap && parts_hold(st, id, 0x1C)) {
                    return 0x1C;
                }
            }
        }
    }
    /* @bin 0x0045C282 */
    static const int8_t need_index[0x21] = {0x15, -1, -1, -1, -1, -1, 0x16, 0x0D, 8, 5, 7, 0x18, 9, 0x13, 1, 0x10, 10,
                                            -1, 4, 0x0F, 0x0B, 0x0C, 3, 6, -1, -1, 0x0E, 0x11, 0x12, 0x17, 0x14, 0x19, 0x1A};
    for (int32_t i = 1; i <= S32(st, 0x0078D790u); i++) {
        uint32_t w = BUILDING_ADDR(i);
        if (U8(st, w) == 0 || S16(st, w + 0x40) == 0) {
            continue;
        }
        int16_t wt = S16(st, w + 0x10);
        if (!(wt >= 0x6E && wt <= 0x72)) {
            switch (wt) {
            case 0x20: case 0x2F: case 0x33: case 0x35: case 0x4A: case 0x71: case 0xB1: case 0xB2: case 0xBB:
            case 0xBC: case 0xBD: case 0xC2: case 0xCB: case 0xCC: case 0xCD: case 0xE2: case 0xE7: case 0xE8:
            case 0xE9:
                break;
            default:
                continue;
            }
        }
        if (GAME_CALL(st, 0x00459240u, i) != 0) {
            continue;
        }
        int32_t cur = id;
        for (int k = 0; k < 8; k++) {
            cur = S16(st, BUILDING_ADDR(cur) + 0x3A);
            if (cur <= 0) {
                continue;
            }
            uint32_t p = BUILDING_ADDR(cur);
            if (S16(st, p + 0x3C) <= 0) {
                continue;
            }
            int32_t g = S16(st, p + 0x12);
            if (good_blocked(st, g) || U8(st, w + 0x14) != zone) {
                continue;
            }
            uint32_t gi = (uint32_t) g - 2u;
            if (gi > 0x20u || need_index[gi] < 0) {
                continue;
            }
            int32_t n = need_index[gi];
            if (S32(st, 0x00EA5574u + 4u * (U32(st, CURRENT_PLAYER_ADDR) * 0x1276u + (uint32_t) n)) <= 0) {
                continue;
            }
            int32_t wcap = stock_capacity(S16(st, w + 0x10));
            if (S16(st, w + 0x12) == n) {
                if (S16(st, w + 0x3C) < wcap && S16(st, w + 0x3C) + 100 <= wcap) {
                    return S16(st, p + 0x12);
                }
            } else if (S16(st, w + 0xCE) != 0 && S16(st, w + 0xCE) == n) {
                if (S16(st, w + 0xCC) < wcap && S16(st, w + 0xCC) + 100 <= wcap) {
                    return S16(st, p + 0x12);
                }
            }
        }
    }
    int32_t cur = id;
    for (int k = 0; k < 8; k++) {
        cur = S16(st, BUILDING_ADDR(cur) + 0x3A);
        if (cur > 0) {
            uint32_t p = BUILDING_ADDR(cur);
            if (S16(st, p + 0x3C) > 0 && U8(st, 0x007900C2u + 0xC4u * rec + (uint32_t) (int32_t) S16(st, p + 0x12)) == 3) {
                return S16(st, p + 0x12);
            }
        }
    }
    return -1;
}

static int32_t run_cart_good48(game_state *st, const int32_t *a)
{
    return storage_cart_good(st, a[0], a[1]);
}

static int32_t run_fill_level(game_state *st, const int32_t *a)
{
    (void) st;
    return storage_fill_level(a[0]);
}

static int32_t run_take(game_state *st, const int32_t *a)
{
    return storage_take(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_take_stock(game_state *st, const int32_t *a)
{
    return storage_take_stock(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_distance(game_state *st, const int32_t *a)
{
    return building_distance(a[0], a[1], a[2], a[3], a[4], a[5], st);
}

static int32_t run_find_supplier(game_state *st, const int32_t *a)
{
    return building_find_supplier(st, a[0], a[1], a[2]);
}

const ported_function bigb_misc2_ports[] = {
    {0x0045DC50u, "building_distance", 6, 32, 0, run_distance, 0},
    {0x0045BB50u, "building_find_supplier", 3, 32, 0, run_find_supplier, 0},
    {0x0045B490u, "storage_fill_level", 1, 32, 0, run_fill_level, 0},
    {0x0045BF00u, "storage_cart_good", 2, 32, 0, run_cart_good48, 0},
    {0x0045B570u, "storage_take", 4, 32, 0, run_take, 0},
    {0x0045E360u, "storage_take_stock", 4, 32, 0, run_take_stock, 0},
};
const int bigb_misc2_ports_count = sizeof(bigb_misc2_ports) / sizeof(bigb_misc2_ports[0]);
