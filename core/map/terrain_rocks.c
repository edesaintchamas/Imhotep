/* Imhotep - the rock, dune and flag 0x20 sub-passes of the terrain images (package terr).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Sub-passes of the terrain rebuild 0x004DEA50, ported for any rectangle and any state (view
 * orientation, buildings next to cliffs), with every global the original writes (cursor
 * 0x00BC9768, the 3x3 walk 0x00BC9760 / 0x00BC9764, the neighbour scratch 0x00BC97C0, the scan
 * position 0x00D3B4E0 / 0x00D3B4E4):
 *   0x004770F0 special rocks over a rectangle (0x004798E0 redraws the 3x3 square around a tile),
 *   0x00475DC0 rocks (gold rocks, cliffs 0x00475FD0), 0x00477210 dunes, 0x00476530 flag 0x20,
 *   whole map, rows of the drawn area (0x00BC9804 / 0x00BC9B94 / 0x00BC9F24). */
#include "map/terrain_rocks.h"

#include "map/map_building.h"
#include "map/terrain_land.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/pre_load.h"

#define GRID 228
#define GROUP(g) S32(st, G_IMAGE_GROUPS + 8u * (uint32_t) (g))

#define TER(o) U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (o))
#define IMG(o) S32(st, G_GRID_IMAGE + 4u * (uint32_t) (o))
#define EDGE(o) U8(st, G_GRID_EDGE + (uint32_t) (o))
#define BITS(o) U8(st, G_GRID_BITFIELDS + (uint32_t) (o))
#define RND(o) U8(st, G_GRID_RANDOM + (uint32_t) (o))
#define BGRID(o) S16(st, G_GRID_BUILDING + 2u * (uint32_t) (o))
#define BUILDING(id) BUILDING_ADDR(id)
#define ROW_OFF(r) S32(st, G_ROW_OFFSET + 4u * (uint32_t) (r))
#define ROW_END(r) S32(st, G_ROW_END + 4u * (uint32_t) (r))
#define ROW_START(r) S32(st, G_ROW_FIRST + 4u * (uint32_t) (r))

/* @bin 0x0047DB00 neighbour order of the scratch */
static const int32_t NEIGHBOURS[8] = {-228, -227, 1, 229, 228, 227, -1, -229};

enum {
    G_SPECIAL_ROCKS = 503, /* @bin 0x004798E0 [0x00F56B6C] */
    G_ROCKS = 505,         /* @bin 0x00475DC0 [0x00F56B7C] */
    G_DUNES = 510,         /* @bin 0x00477210 [0x00F56BA4] */
    G_ROCK_GOLD = 539,     /* @bin 0x00475DC0 [0x00F56C8C] */
    G_FLAG_0X20 = 328,     /* @bin 0x00476530 [0x00F565F4] */
    G_CLIFF = 743,         /* @bin 0x00475DC0 [0x00F572EC] */
};

int32_t terrain_block_free(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t flag, int32_t need)
{
    /* @bin 0x0047CAE0 inside the map (signed compares); tile offsets from the block table
     * 0x005CC8C0 (rows of 0x30 bytes, 8 per tile) */
    if (x < 0 || y < 0 || x + w - 1 >= S32(st, G_MAP_WIDTH) || y + h - 1 >= S32(st, G_MAP_HEIGHT)) {
        return 0;
    }
    int32_t c = S32(st, G_MAP_START) + y * GRID + x;
    S32(st, G_TILE_CURSOR) = c;
    for (int32_t j = 0; j < h; j++) {
        for (int32_t i = 0; i < w; i++) {
            int32_t o = S32(st, G_FOOTPRINT_OFFSETS + 0x30u * (uint32_t) j + 8u * (uint32_t) i) + c;
            if ((TER(o) & 0xEEFED77Fu) != flag) {
                return 0;
            }
            if (need != 0 && IMG(o) != need - 1) {
                return 0;
            }
        }
    }
    return 1;
}

int32_t terrain_scan_mask(game_state *st, int32_t x, int32_t y, int32_t n, int32_t d, uint32_t mask)
{
    /* @bin 0x00482650 the row offset advances by xa - xb + 227 after each row (228 when the row
     * is not empty) */
    int32_t xa = x - d, xb = x + n - 1 + d, ya = y - d, yb = y + n - 1 + d;
    if (xa < 0) {
        xa = 0;
    }
    if (ya < 0) {
        ya = 0;
    }
    if (xb >= S32(st, G_MAP_WIDTH)) {
        xb = S32(st, G_MAP_WIDTH) - 1;
    }
    if (yb >= S32(st, G_MAP_HEIGHT)) {
        yb = S32(st, G_MAP_HEIGHT) - 1;
    }
    S32(st, G_ROAD_ACCESS_Y) = ya;
    int32_t o = S32(st, G_MAP_START) + ya * GRID + xa;
    for (int32_t r = ya; r <= yb; r++) {
        int32_t i = xa;
        S32(st, G_ROAD_ACCESS_X) = i;
        while (i <= xb) {
            if (TER(o) & mask) {
                return 1;
            }
            i++;
            o++;
            S32(st, G_ROAD_ACCESS_X) = i;
        }
        o += xa - xb + 0xE3;
        S32(st, G_ROAD_ACCESS_Y) = r + 1;
    }
    return 0;
}

static void scratch_all_flags(game_state *st, uint32_t mask)
{
    /* @bin 0x0047DBC0 scratch[i] = (neighbour terrain & mask) == mask */
    int32_t c = S32(st, G_TILE_CURSOR);
    for (uint32_t i = 0; i < 8; i++) {
        S32(st, G_NEIGHBOUR_FLAGS + 4u * i) = (TER(c + NEIGHBOURS[i]) & mask) == mask;
    }
}

int32_t terrain_cliff_index(game_state *st, int32_t tile)
{
    /* @bin 0x00475FD0 scratch (16 dwords cleared) := cliff neighbours (0x0047DBC0(0x20000002))
     * that have no 0x80000; neighbours with building type 0xE6, or 0xE5 / 0xEA..0xEC whose +0x68
     * is not 0x12 and whose main building's +0x58 is 1, count too. Image index tables
     * 0x005DBCC8.. by orientation / 2 */
    int32_t cliff[8];
    for (uint32_t i = 0; i < 16; i++) {
        S32(st, G_NEIGHBOUR_FLAGS + 4u * i) = 0;
    }
    S32(st, G_TILE_CURSOR) = tile;
    scratch_all_flags(st, 0x20000002u);
    for (uint32_t i = 0; i < 8; i++) {
        cliff[i] = S32(st, G_NEIGHBOUR_FLAGS + 4u * i);
    }
    scratch_all_flags(st, 0x80000u);
    for (uint32_t i = 0; i < 8; i++) {
        S32(st, G_NEIGHBOUR_FLAGS + 4u * i) = S32(st, G_NEIGHBOUR_FLAGS + 4u * i) != 0 ? 0 : cliff[i];
    }
    int32_t c = S32(st, G_TILE_CURSOR);
    for (uint32_t i = 0; i < 8; i++) {
        int32_t id = BGRID(c + NEIGHBOURS[i]);
        if (id <= 0) {
            continue;
        }
        int32_t type = S16(st, BUILDING(id) + BLD_TYPE);
        if (type == 0xE6 ||
            ((type == 0xE5 || type == 0xEA || type == 0xEB || type == 0xEC) && S32(st, BUILDING(id) + BLD_U68) != 0x12 &&
             S32(st, BUILDING(id - S16(st, BUILDING(id) + BLD_PART)) + BLD_MONUMENT_STAGE) == 1)) {
            S32(st, G_NEIGHBOUR_FLAGS + 4u * i) = 1;
        }
    }
    int32_t n = 0;
    for (uint32_t i = 0; i < 8; i++) {
        n += S32(st, G_NEIGHBOUR_FLAGS + 4u * i) == 1;
    }
    int32_t r = RND(tile), half = S32(st, G_VIEW_ORIENTATION) / 2;
/* table k (0..13) */
#define T(k) S32(st, G_CLIFF_IMAGES + 16u * (k) + 4u * (uint32_t) half)
    int32_t s0 = S32(st, G_NEIGHBOUR_FLAGS), s1 = S32(st, G_NEIGHBOUR_FLAGS + 4), s2 = S32(st, G_NEIGHBOUR_FLAGS + 8), s3 = S32(st, G_NEIGHBOUR_FLAGS + 12);
    int32_t s4 = S32(st, G_NEIGHBOUR_FLAGS + 16), s5 = S32(st, G_NEIGHBOUR_FLAGS + 20), s6 = S32(st, G_NEIGHBOUR_FLAGS + 24),
            s7 = S32(st, G_NEIGHBOUR_FLAGS + 28);
    if (n == 8) {
        return r % 0x18 + 0x30;
    }
    if (n == 7) {
        if (!s1) return T(0) + r % 3;
        if (!s7) return T(1) + r % 3;
        if (!s5) return T(2) + r % 3;
        if (!s3) return T(3) + r % 3;
    } else if (n == 6) {
        if (!s7 && !s3) return T(12) + r % 3;
        if (!s1 && !s5) return T(13) + r % 3;
    }
    if (s6 && s7 && s0 && s1 && s2 && n < 8) return T(4) + r % 6;
    if (s4 && s5 && s6 && s7 && s0 && n < 8) return T(5) + r % 6;
    if (s2 && s3 && s4 && s5 && s6 && n < 8) return T(6) + r % 6;
    if (s0 && s1) {
        if (s2 && s3 && s4 && n < 8) return T(7) + r % 6;
        if (s1 && s2) return T(8) + r % 3;
    }
    if (s6 && s7 && s0) return T(9) + r % 3;
    if (s4 && s5 && s6) return T(10) + r % 3;
    if (s2 && s3 && s4) return T(11) + r % 3;
#undef T
    return -1;
}

/* ---- special rocks --------------------------------------------------------------------------- */

static void scratch_special_rocks(game_state *st)
{
    /* @bin 0x0047DEE0 scratch[i] = neighbour terrain & 2 and bit-field byte & 0x80 */
    int32_t c = S32(st, G_TILE_CURSOR);
    for (uint32_t i = 0; i < 8; i++) {
        int32_t n = c + NEIGHBOURS[i];
        S32(st, G_NEIGHBOUR_FLAGS + 4u * i) = (TER(n) & 2) && (BITS(n) & 0x80);
    }
}

static void scratch_normalize(game_state *st)
{
    /* @bin 0x00484F20 */
    for (uint32_t i = 0; i < 8; i++) {
        S32(st, G_NEIGHBOUR_FLAGS + 4u * i) = S32(st, G_NEIGHBOUR_FLAGS + 4u * i) != 0;
    }
}

void special_rocks_tile_redraw(game_state *st, int32_t x, int32_t y, int32_t flag)
{
    /* @bin 0x004798E0 (x, y, flag) inside the map: flag 1 sets terrain 2 and bit-field 0x80, else
     * clears terrain 0x20300002 and 0x80; then every special rock of the 3x3 square (clamped)
     * matches its neighbours (table 0x005CBEA8, 17 records) */
    if (!map_tile_inside(st, S32(st, G_VIEW_DX) + x, S32(st, G_VIEW_DY) + y)) {
        return;
    }
    int32_t o = S32(st, G_MAP_START) + y * GRID + x;
    if (flag == 1) {
        TER(o) |= 2;
        BITS(o) |= 0x80;
    } else {
        TER(o) &= 0xDFCFFFFDu;
        BITS(o) &= 0x7F;
    }
    int32_t xa = x - 1, ya = y - 1, w = 3, h = 3;
    if (xa < 0) {
        w = xa + 3;
        xa = 0;
    } else if (xa + 3 > S32(st, G_MAP_WIDTH)) {
        w = S32(st, G_MAP_WIDTH) - xa;
    }
    if (ya < 0) {
        h = ya + 3;
        ya = 0;
    } else if (ya + 3 > S32(st, G_MAP_HEIGHT)) {
        h = S32(st, G_MAP_HEIGHT) - ya;
    }
    S32(st, G_WALK_Y) = ya;
    S32(st, G_TILE_CURSOR) = S32(st, G_MAP_START) + ya * GRID + xa;
    while (S32(st, G_WALK_Y) < ya + h) {
        int32_t c = S32(st, G_TILE_CURSOR);
        for (S32(st, G_WALK_X) = xa; S32(st, G_WALK_X) < xa + w;) {
            if ((TER(c) & 2) && (BITS(c) & 0x80)) {
                scratch_special_rocks(st);
                scratch_normalize(st);
                int32_t hit = land_pattern_match(st, G_PATTERN_SPECIAL_ROCKS, 0x11, -1, 0);
                c = S32(st, G_TILE_CURSOR);
                IMG(c) = hit ? GROUP(G_SPECIAL_ROCKS) + S32(st, G_MATCH_VARIANT) + S32(st, G_MATCH_OFFSET)
                             : GROUP(G_SPECIAL_ROCKS);
                BITS(c) &= 0xF0;
                EDGE(c) |= 0x40;
            }
            S32(st, G_WALK_X) += 1;
            c++;
            S32(st, G_TILE_CURSOR) = c;
        }
        S32(st, G_WALK_Y) += 1;
        S32(st, G_TILE_CURSOR) = c + 0xE4 - w;
    }
}

void special_rocks_rect(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    /* @bin 0x004770F0 (x0, y0, x1, y1) clamped to the map; every special rock (terrain 2,
     * bit-field 0x80) is redrawn with 0x004798E0(x, y, 1). The tile offset advances by
     * w - x1 + x0 - 1 + [0x00BCA2DC] after each row (228 when the row is not empty). */
    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    int32_t w = S32(st, G_MAP_WIDTH);
    if (x1 >= w) {
        x1 = w - 1;
    }
    if (y1 >= S32(st, G_MAP_HEIGHT)) {
        y1 = S32(st, G_MAP_HEIGHT) - 1;
    }
    int32_t step = w - x1 + x0 - 1;
    int32_t o = S32(st, G_MAP_START) + y0 * GRID + x0;
    for (int32_t y = y0; y <= y1; y++) {
        for (int32_t x = x0; x <= x1; x++, o++) {
            if ((TER(o) & 2) && (BITS(o) & 0x80)) {
                special_rocks_tile_redraw(st, x, y, 1);
            }
        }
        o += S32(st, G_MAP_STRIDE_EXTRA) + step;
    }
}

/* ---- rocks, dunes, flag 0x20: whole map, rows of the drawn area -------------------------------- */

static void clear_rows(game_state *st, uint32_t mask, int check_bits)
{
    /* the first loop of 0x00475DC0 / 0x00477210 / 0x00476530: image 0, bit-field & 0xF0, edge
     * 0x40 on the tiles with terrain & mask (bit-field 0x80 excluded for rocks and dunes), not
     * 0x600 */
    int32_t h = S32(st, G_MAP_HEIGHT);
    for (int32_t r = 0; r < h; r++) {
        int32_t sx = ROW_START(r), ex = ROW_END(r), o = ROW_OFF(r);
        for (int32_t x = sx; x < ex; x++, o++) {
            uint32_t v = TER(o);
            if ((v & mask) && !(check_bits && (BITS(o) & 0x80)) && !(v & 0x600)) {
                IMG(o) = 0;
                BITS(o) &= 0xF0;
                EDGE(o) |= 0x40;
            }
        }
    }
}

void rocks_all(game_state *st)
{
    /* @bin 0x00475DC0 rocks (terrain 2, no bit-field 0x80, no 0x600, image 0 after the clear):
     * gold (0x100002) and plain rocks as 3x3, 2x2 blocks (0x0047CAE0, 0x0047BF50) or single
     * tiles; cliffs (0x20000002) from 0x00475FD0 */
    clear_rows(st, 2, 1);
    for (int32_t r = 0; r < S32(st, G_MAP_HEIGHT); r++) {
        int32_t sx = ROW_START(r), ex = ROW_END(r), o = ROW_OFF(r);
        for (int32_t x = sx; x < ex; x++, o++) {
            uint32_t v = TER(o);
            if (!(v & 2) || (BITS(o) & 0x80) || (v & 0x600) || IMG(o) != 0) {
                continue;
            }
            uint32_t flag;
            int32_t base;
            if ((v & 0x100002u) == 0x100002u) {
                flag = 0x100002u;
                base = GROUP(G_ROCK_GOLD);
            } else if ((v & 0x20000002u) == 0x20000002u) {
                int32_t cliff = GROUP(G_CLIFF);
                int32_t k = terrain_cliff_index(st, o);
                IMG(o) = k != -1 ? k + cliff : (RND(o) & 7) + GROUP(G_ROCKS);
                continue;
            } else {
                flag = 2;
                base = GROUP(G_ROCKS);
            }
            if (terrain_block_free(st, x, r, 3, 3, flag, 1)) {
                map_building_tiles(st, 0, x, r, 3, 3, (RND(o) & 1) + base + 0xC, flag);
            } else if (terrain_block_free(st, x, r, 2, 2, flag, 1)) {
                map_building_tiles(st, 0, x, r, 2, 2, (RND(o) & 3) + base + 8, flag);
            } else {
                IMG(o) = (RND(o) & 7) + base;
            }
        }
    }
}

void dunes_all(game_state *st)
{
    /* @bin 0x00477210 dunes (terrain 0x2000000): as the rocks, with 0x00482650(x, y, n, 4, 0x200)
     * before each placement (its result picks between identical images) */
    clear_rows(st, 0x2000000u, 1);
    for (int32_t r = 0; r < S32(st, G_MAP_HEIGHT); r++) {
        int32_t sx = ROW_START(r), ex = ROW_END(r), o = ROW_OFF(r);
        for (int32_t x = sx; x < ex; x++, o++) {
            uint32_t v = TER(o);
            if (!(v & 0x2000000u) || (BITS(o) & 0x80) || (v & 0x600) || IMG(o) != 0) {
                continue;
            }
            if (terrain_block_free(st, x, r, 3, 3, 0x2000000u, 1)) {
                terrain_scan_mask(st, x, r, 3, 4, 0x200);
                map_building_tiles(st, 0, x, r, 3, 3, (RND(o) & 1) + GROUP(G_DUNES) + 0xC, 0x2000000u);
            } else if (terrain_block_free(st, x, r, 2, 2, 0x2000000u, 1)) {
                terrain_scan_mask(st, x, r, 2, 4, 0x200);
                map_building_tiles(st, 0, x, r, 2, 2, (RND(o) & 3) + GROUP(G_DUNES) + 8, 0x2000000u);
            } else {
                terrain_scan_mask(st, x, r, 1, 4, 0x200);
                IMG(o) = (RND(o) & 7) + GROUP(G_DUNES);
            }
        }
    }
}

void flag_0x20_all(game_state *st)
{
    /* @bin 0x00476530 terrain 0x20 (no 0x600): 3x3 block image +7, 2x2 block +6 / +5 / +4 (random
     * & 3: 0-1 / 2 / 3), else the tile's image by x & 3 and the row's parity */
    clear_rows(st, 0x20, 0);
    for (int32_t r = 0; r < S32(st, G_MAP_HEIGHT); r++) {
        int32_t sx = ROW_START(r), ex = ROW_END(r), o = ROW_OFF(r);
        for (int32_t x = sx; x < ex; x++, o++) {
            uint32_t v = TER(o);
            if (!(v & 0x20) || (v & 0x600) || IMG(o) != 0) {
                continue;
            }
            if (terrain_block_free(st, x, r, 3, 3, 0x20, 1)) {
                map_building_tiles(st, 0, x, r, 3, 3, GROUP(G_FLAG_0X20) + 7, 0x20);
            } else if (terrain_block_free(st, x, r, 2, 2, 0x20, 1)) {
                int32_t k = RND(o) & 3;
                map_building_tiles(st, 0, x, r, 2, 2, GROUP(G_FLAG_0X20) + (k <= 1 ? 6 : k <= 2 ? 5 : 4), 0x20);
            } else {
                static const int32_t EVEN[4] = {0, 1, 0, 1}, ODD[4] = {2, 3, 2, 3};
                IMG(o) = GROUP(G_FLAG_0X20) + ((r & 1) == 0 ? EVEN[x & 3] : ODD[x & 3]);
            }
        }
    }
}

/* ---- table ------------------------------------------------------------------------------------ */

static int32_t run_block_free(game_state *st, const int32_t *a)
{
    return terrain_block_free(st, a[0], a[1], a[2], a[3], (uint32_t) a[4], a[5]);
}

static int32_t run_scan_mask(game_state *st, const int32_t *a)
{
    return terrain_scan_mask(st, a[0], a[1], a[2], a[3], (uint32_t) a[4]);
}

static int32_t run_cliff_index(game_state *st, const int32_t *a)
{
    return terrain_cliff_index(st, a[0]);
}

static int32_t run_all_flags(game_state *st, const int32_t *a)
{
    scratch_all_flags(st, (uint32_t) a[0]);
    return 0;
}

static int32_t run_scratch_special(game_state *st, const int32_t *a)
{
    (void) a;
    scratch_special_rocks(st);
    return 0;
}

static int32_t run_special_tile(game_state *st, const int32_t *a)
{
    special_rocks_tile_redraw(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_special_rect(game_state *st, const int32_t *a)
{
    special_rocks_rect(st, a[0], a[1], a[2], a[3]);
    return 0;
}

#define RUN0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
RUN0(run_rocks, rocks_all)
RUN0(run_dunes, dunes_all)
RUN0(run_flag_0x20, flag_0x20_all)

const ported_function terr_rocks_ports[] = {
    {0x0047CAE0u, "terrain_block_free", 6, 32, 0, run_block_free, 0},
    {0x00482650u, "terrain_scan_mask", 5, 32, 0, run_scan_mask, 0},
    {0x00475FD0u, "terrain_cliff_index", 1, 32, 0, run_cliff_index, 0},
    {0x0047DBC0u, "scratch_all_flags", 1, 0, 0, run_all_flags, 0},
    {0x0047DEE0u, "scratch_special_rocks", 0, 0, 0, run_scratch_special, 0},
    {0x004798E0u, "special_rocks_tile_redraw", 3, 0, 0, run_special_tile, 0},
    {0x004770F0u, "special_rocks_rect", 4, 0, 0, run_special_rect, 0},
    {0x00475DC0u, "rocks_all", 0, 0, 0, run_rocks, 0},
    {0x00477210u, "dunes_all", 0, 0, 0, run_dunes, 0},
    {0x00476530u, "flag_0x20_all", 0, 0, 0, run_flag_0x20, 0},
};
const int terr_rocks_ports_count = sizeof(terr_rocks_ports) / sizeof(terr_rocks_ports[0]);
