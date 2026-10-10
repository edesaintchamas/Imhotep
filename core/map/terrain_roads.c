/* Imhotep - the road sub-pass of the terrain images over any rectangle (package bigb2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x0047B590 (x0, y0, x1, y1) redraws every road tile (terrain 0x40 without 0x0C) of a rectangle
 * with 0x0047B670, walking the cursor 0x00BC9768 and the globals 0x00BC9764 (x), 0x00BC9760 (y).
 * core/map/terrain.c has the whole-map version for a new scenario; this one is general. */
#include "map/terrain_roads.h"

#include "map/terrain_land.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"

#define TER(o) U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (o))
#define IMG(o) S32(st, G_GRID_IMAGE + 4u * (uint32_t) (o))
#define BGRID(o) S16(st, G_GRID_BUILDING + 2u * (uint32_t) (o))

static int32_t road_paved_at(game_state *st)
{
    /* @bin 0x0047B920 the cursor tile's road is paved: never on floodplain; desirability > 4 yes,
     * < 1 no, else terrain bit 0x2000 */
    int32_t c = S32(st, G_TILE_CURSOR);
    uint32_t v = TER(c);
    if (v & 0x10000) {
        return 0;
    }
    int8_t d = S8(st, G_GRID_DESIRABILITY + (uint32_t) c);
    if (d > 4) {
        return 1;
    }
    if (d <= 0) {
        return 0;
    }
    return (v >> 13) & 1;
}

static void scratch_normalize(game_state *st)
{
    /* @bin 0x00484F20 every scratch flag := (flag != 0) */
    for (uint32_t i = 0; i < 8; i++) {
        S32(st, G_NEIGHBOUR_FLAGS + 4u * i) = S32(st, G_NEIGHBOUR_FLAGS + 4u * i) != 0;
    }
}

static void gatehouse_flags(game_state *st)
{
    /* @bin 0x0047E5C0 a gatehouse-type building (types 0x3A, 0xAA, 0xAB, 200, 0xC9, 0xCA, terrain
     * 0x8000) north / south of the cursor with part +0x12 == 1, east / west with 2, sets the
     * scratch flag of that side */
    static const struct { int32_t d; uint32_t flag; int16_t side; } N[4] = {
        {-228, 0, 1}, {1, 2, 2}, {228, 4, 1}, {-1, 6, 2}};
    int32_t c = S32(st, G_TILE_CURSOR);
    for (int k = 0; k < 4; k++) {
        int32_t n = c + N[k].d;
        if (!(TER(n) & 0x8000)) {
            continue;
        }
        uint32_t b = BUILDING_ADDR((int32_t) BGRID(n));
        int16_t type = S16(st, b + BLD_TYPE);
        if ((type == 0x3A || type == 0xAA || type == 0xAB || type == 200 || type == 0xC9 || type == 0xCA) &&
            S16(st, b + BLD_SUBTYPE) == N[k].side) {
            S32(st, G_NEIGHBOUR_FLAGS + 4u * N[k].flag) = 1;
        }
    }
}

/* @bin 0x0047EB60 (dir, &count) and the inline cases of 0x0047E870: the canal neighbour in
 * direction dir (N 0, E 2, S 4, W 6) links when it has 0x100 and, for a tile without a road
 * (0x40), it has no road (0x8000040) or a road on one of its sides (a, b); for a road tile, it
 * has no road and one of the tile's own sides (c, d) has one */
static int canal_link(game_state *st, int32_t o, int dir)
{
    static const int32_t D[4][5] = {
        {-228, -1, 1, -229, -227}, {1, -228, 228, -227, 229}, {228, -1, 1, 227, 229}, {-1, -228, 228, -229, 227}};
    const int32_t *d = D[dir / 2];
    uint32_t n = TER(o + d[0]);
    if (!(n & 0x100)) {
        return 0;
    }
    int road_n = (n & 0x8000040u) != 0;
    if (!((TER(o) >> 6) & 1)) {
        return !road_n || (TER(o + d[3]) & 0x8000040u) || (TER(o + d[4]) & 0x8000040u);
    }
    if (road_n) {
        return 0;
    }
    return (TER(o + d[1]) & 0x8000040u) || (TER(o + d[2]) & 0x8000040u);
}

static void canal_flags(game_state *st)
{
    /* @bin 0x0047E870 the canal neighbour flags of the cursor tile in the scratch (orthogonal):
     * on floodplain the water neighbours (terrain 4); the canal links (0x0047EB60); a Water Lift
     * (building type 0x5A, terrain 8) north / south with its +0xAA bit 0 clear, east / west with
     * it set, unless the tile and the opposite neighbour both have a road (0x40) */
    int32_t o = S32(st, G_TILE_CURSOR);
    for (uint32_t i = 0; i < 8; i++) {
        S32(st, G_NEIGHBOUR_FLAGS + 4u * (7 - i)) = 0;
    }
    if (TER(o) & 0x10000) {
        static const int32_t W[4] = {-228, 1, 228, -1};
        for (int k = 0; k < 4; k++) {
            if (TER(o + W[k]) & 4) {
                S32(st, G_NEIGHBOUR_FLAGS + 8u * (uint32_t) k) = 1;
            }
        }
    }
    for (int dir = 0; dir < 8; dir += 2) {
        if (canal_link(st, o, dir)) {
            S32(st, G_NEIGHBOUR_FLAGS + 4u * (uint32_t) dir) = 1;
        }
    }
    o = S32(st, G_TILE_CURSOR);
    static const struct { int32_t d, opposite; uint8_t bit; } L[4] = {{-228, 228, 0}, {1, -1, 1}, {228, -228, 0}, {-1, 1, 1}};
    for (int k = 0; k < 4; k++) {
        int32_t n = o + L[k].d;
        if (!(TER(n) & 8)) {
            continue;
        }
        uint32_t b = BUILDING_ADDR((int32_t) BGRID(n));
        if (S16(st, b + BLD_TYPE) == 0x5A && !(TER(o + L[k].opposite) & TER(o) & 0x40) && (U8(st, b + BLD_ORIENTATION) & 1) == L[k].bit) {
            S32(st, G_NEIGHBOUR_FLAGS + 8u * (uint32_t) k) = 1;
        }
    }
}

static void road_tile(game_state *st)
{
    /* @bin 0x0047B670 the cursor's road image (the neighbour flags in the scratch): skipped under
     * a building or with bitfield 0x80; all roads paved (0x00978D74 == 1): paved pattern; a canal
     * (0x100): bridge images of the canal group by the canal pattern (0x0047E870), paved or not;
     * floodplain or a flooded floodplain tile: the floodplain road images (0x0051C5F0) or the
     * floodplain road group; else paved or plain road pattern */
    int32_t c = S32(st, G_TILE_CURSOR), img;
    if (BGRID(c) != 0 || (U8(st, G_GRID_BITFIELDS + (uint32_t) c) & 0x80)) {
        return;
    }
    if (S32(st, G_ALL_ROADS_PAVED) == 1) {
        land_pattern_match(st, G_PATTERN_PAVED_ROADS, 0x30, -1, 0);
        scratch_normalize(st);
        IMG(S32(st, G_TILE_CURSOR)) = S32(st, G_IMAGE_GROUPS + 8u * 530) + S32(st, G_MATCH_VARIANT) + S32(st, G_MATCH_OFFSET);
    } else if (TER(c) & 0x100) {
        int32_t shift = (IMG(c) - S32(st, G_CANAL_BASE) < 0x30) ? 0 : 0x30;
        canal_flags(st);
        land_pattern_match(st, G_PATTERN_CANALS, 0x10, -1, 0);
        if (S32(st, G_MATCH_EXTRA) == 0) {
            int32_t k = S32(st, G_TILE_CURSOR);
            if (!(TER(k - 1) & 0x100)) {
                S32(st, G_MATCH_OFFSET) = 1;
                if (TER(k + 1) & 0x100) {
                    S32(st, G_MATCH_OFFSET) = 0;
                }
            } else {
                S32(st, G_MATCH_OFFSET) = 0;
            }
        }
        if (road_paved_at(st)) {
            IMG(S32(st, G_TILE_CURSOR)) = S32(st, G_CANAL_BASE) + S32(st, G_MATCH_OFFSET) + shift + 0x2A;
        } else {
            img = S32(st, G_CANAL_BASE) + S32(st, G_MATCH_OFFSET) + shift + 0xF;
            if (TER(S32(st, G_TILE_CURSOR)) & 0x10000) {
                img += 0x15; /* @bin 0x0051C620 */
            }
            IMG(S32(st, G_TILE_CURSOR)) = img;
        }
    } else {
        uint32_t obj = U32(st, G_FLOODPLAIN_MANAGER + 4u * (uint32_t) c);
        if (!(TER(c) & 0x10000) && (obj == 0 || P8(st, obj + FLOODOBJ_FLAG) != 0)) {
            if (road_paved_at(st)) {
                land_pattern_match(st, G_PATTERN_PAVED_ROADS, 0x30, -1, 0);
                scratch_normalize(st);
                img = S32(st, G_IMAGE_GROUPS + 8u * 530) + S32(st, G_MATCH_VARIANT) + S32(st, G_MATCH_OFFSET);
            } else {
                land_pattern_match(st, G_PATTERN_ROADS, 0x11, -1, 0);
                scratch_normalize(st);
                IMG(S32(st, G_TILE_CURSOR)) = S32(st, G_IMAGE_GROUPS + 8u * 540) + S32(st, G_MATCH_VARIANT) + S32(st, G_MATCH_OFFSET);
                goto mark;
            }
        } else {
            int32_t rel = IMG(c) - S32(st, G_IMAGE_GROUPS + 8u * 528);
            int32_t m = rel == 0x34 ? 0x56 : rel == 0x30 ? 0x54 : rel == 0x36 ? 0x57 : rel == 0x32 ? 0x55 : rel;
            if (rel == m && (rel < 0x54 || rel > 0x57)) {
                land_pattern_match(st, G_PATTERN_ROADS, 0x11, -1, 0);
                scratch_normalize(st);
                img = S32(st, G_MATCH_VARIANT) + S32(st, G_IMAGE_GROUPS + 8u * 548) + S32(st, G_MATCH_OFFSET);
            } else {
                img = m + S32(st, G_IMAGE_GROUPS + 8u * 528);
            }
        }
        IMG(S32(st, G_TILE_CURSOR)) = img;
    }
mark:
    c = S32(st, G_TILE_CURSOR);
    U8(st, G_GRID_BITFIELDS + (uint32_t) c) &= 0xF0;
    U8(st, G_GRID_EDGE + (uint32_t) c) |= 0x40;
}

void canals_rect(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t dry)
{
    /* @bin 0x0047BAE0 (x0, y0, x1, y1, dry) the rectangle (corners in any order) grown by one
     * tile and clamped; every canal tile (0x100 without 0x0C, aqueduct grid 0x00AE8E7C < 0x10):
     * the canal pattern of its flags (0x0047E870), the dry set (+0x30) when `dry` or when its
     * image is not already a wet canal; a road across (0x40): bridge (bitfield 0x80 cleared,
     * offset by the road north when the pattern has none, + 0x2A paved / 0xF), floodplain + 0x15;
     * the aqueduct grid keeps the pattern's extra byte. 0x00BCA328 := 0. */
    int32_t xa = x0 <= x1 ? x0 : x1, xb = x0 <= x1 ? x1 : x0;
    int32_t ya = y0 <= y1 ? y0 : y1, yb = y0 <= y1 ? y1 : y0;
    xa--, xb++, ya--, yb++;
    if (xa < 0) {
        xa = 0;
    }
    if (xb >= S32(st, G_MAP_WIDTH)) {
        xb = S32(st, G_MAP_WIDTH) - 1;
    }
    if (ya < 0) {
        ya = 0;
    }
    if (yb >= S32(st, G_MAP_HEIGHT)) {
        yb = S32(st, G_MAP_HEIGHT) - 1;
    }
    S32(st, G_WALK_Y) = ya;
    S32(st, G_TILE_CURSOR) = S32(st, G_MAP_START) + ya * 228 + xa;
    while (S32(st, G_WALK_Y) <= yb) {
        for (S32(st, G_WALK_X) = xa; S32(st, G_WALK_X) <= xb; S32(st, G_WALK_X) += 1) {
            int32_t c = S32(st, G_TILE_CURSOR);
            uint32_t v = TER(c);
            if ((v & 0x100) && !(v & 0xC) && U8(st, G_CANAL + (uint32_t) c) <= 0xF) {
                int32_t rel = IMG(c) - S32(st, G_CANAL_BASE);
                int32_t shift = rel >= 0x30 ? 0x30 : (rel >= 0 ? 0 : 0x30);
                if (dry != 0) {
                    shift = 0x30;
                }
                canal_flags(st);
                land_pattern_match(st, G_PATTERN_CANALS, 0x10, -1, 0);
                c = S32(st, G_TILE_CURSOR);
                if (TER(c) & 0x40) {
                    U8(st, G_GRID_BITFIELDS + (uint32_t) c) &= 0x7F;
                    if (S32(st, G_MATCH_EXTRA) == 0) {
                        S32(st, G_MATCH_OFFSET) = (int32_t) ((((uint8_t) ~U8(st, G_GRID_TERRAIN + 4u * (uint32_t) (c - 228))) >> 6) & 1);
                    }
                    S32(st, G_MATCH_OFFSET) = S32(st, G_MATCH_OFFSET) + (road_paved_at(st) ? 0x2A : 0xF);
                    c = S32(st, G_TILE_CURSOR);
                }
                int32_t img = S32(st, G_CANAL_BASE) + S32(st, G_MATCH_VARIANT) + S32(st, G_MATCH_OFFSET) + shift;
                if (TER(c) & 0x10000) {
                    img += 0x15; /* @bin 0x0051C620 */
                }
                IMG(c) = img;
                U8(st, G_GRID_BITFIELDS + (uint32_t) c) &= 0xF0;
                U8(st, G_GRID_EDGE + (uint32_t) c) |= 0x40;
                U8(st, G_CANAL + (uint32_t) c) = (uint8_t) S32(st, G_MATCH_EXTRA);
            }
            S32(st, G_TILE_CURSOR) += 1;
        }
        S32(st, G_WALK_Y) += 1;
        S32(st, G_TILE_CURSOR) += 0xE4 - (xb - xa + 1);
    }
    S32(st, G_UBCA328) = 0;
}

void roads_rect(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    /* @bin 0x0047B590 clamped to the map; every road tile: neighbour flags (0x0047DB00 with
     * 0x8000040), gatehouses (0x0047E5C0), the image (0x0047B670) */
    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x1 >= S32(st, G_MAP_WIDTH)) {
        x1 = S32(st, G_MAP_WIDTH) - 1;
    }
    if (y1 >= S32(st, G_MAP_HEIGHT)) {
        y1 = S32(st, G_MAP_HEIGHT) - 1;
    }
    int32_t step = S32(st, G_MAP_WIDTH) - x1 + x0 - 1;
    S32(st, G_WALK_Y) = y0;
    S32(st, G_TILE_CURSOR) = S32(st, G_MAP_START) + y0 * 228 + x0;
    while (S32(st, G_WALK_Y) <= y1) {
        for (S32(st, G_WALK_X) = x0; S32(st, G_WALK_X) <= x1; S32(st, G_WALK_X) += 1) {
            uint32_t v = TER(S32(st, G_TILE_CURSOR));
            if ((v & 0x40) && !(v & 0xC)) {
                land_neighbour_flags(st, 0x8000040u);
                gatehouse_flags(st);
                road_tile(st);
            }
            S32(st, G_TILE_CURSOR) += 1;
        }
        S32(st, G_TILE_CURSOR) += S32(st, G_MAP_STRIDE_EXTRA) + step;
        S32(st, G_WALK_Y) += 1;
    }
}

void roads_all(game_state *st)
{
    /* @bin 0x0047B570 the whole map */
    roads_rect(st, 0, 0, S32(st, G_MAP_WIDTH) - 1, S32(st, G_MAP_HEIGHT) - 1);
}

static int32_t run_canals(game_state *st, const int32_t *a)
{
    canals_rect(st, a[0], a[1], a[2], a[3], a[4]);
    return 0;
}

static int32_t run_roads_all(game_state *st, const int32_t *a)
{
    (void) a;
    roads_all(st);
    return 0;
}

static int32_t run_roads_rect(game_state *st, const int32_t *a)
{
    roads_rect(st, a[0], a[1], a[2], a[3]);
    return 0;
}

static int32_t run_road_tile(game_state *st, const int32_t *a)
{
    (void) a;
    road_tile(st);
    return 0;
}

static int32_t run_gatehouse(game_state *st, const int32_t *a)
{
    (void) a;
    gatehouse_flags(st);
    return 0;
}

static int32_t run_paved(game_state *st, const int32_t *a)
{
    (void) a;
    return road_paved_at(st);
}

static int32_t run_normalize(game_state *st, const int32_t *a)
{
    (void) a;
    scratch_normalize(st);
    return 0;
}

static int32_t run_canal_flags(game_state *st, const int32_t *a)
{
    (void) a;
    canal_flags(st);
    return 0;
}

static int32_t run_canal_link(game_state *st, const int32_t *a)
{
    uint8_t *count = state_ptr(st, (uint32_t) a[1], 4);
    if (!count) {
        state_unported(st, FN_CANAL_LINK); /* the count lives in the caller's frame */
        return 0;
    }
    if (canal_link(st, S32(st, G_TILE_CURSOR), a[0])) {
        S32(st, G_NEIGHBOUR_FLAGS + 4u * (uint32_t) a[0]) = 1;
        S32(st, (uint32_t) a[1]) += 1;
    }
    return 0;
}

static int32_t run_plus15(game_state *st, const int32_t *a)
{
    /* @bin 0x0051C620 (image) + 0x15 */
    (void) st;
    return a[0] + 0x15;
}

const ported_function bigb2_roads_ports[] = {
    {0x0047B570u, "roads_all", 0, 0, 0, run_roads_all, 0},
    {0x0047BAE0u, "canals_rect", 5, 0, 0, run_canals, 0},
    {0x0047B590u, "roads_rect", 4, 0, 0, run_roads_rect, 0},
    {0x0047B670u, "road_tile", 0, 0, 0, run_road_tile, 0},
    {0x0047E5C0u, "gatehouse_flags", 0, 0, 0, run_gatehouse, 0},
    {0x0047B920u, "road_paved_at", 0, 32, 0, run_paved, 0},
    {0x00484F20u, "scratch_normalize", 0, 0, 0, run_normalize, 0},
    {0x0051C620u, "floodplain_canal_image", 1, 32, 0, run_plus15, 0},
    {0x0047E870u, "canal_flags", 0, 0, 0, run_canal_flags, 0},
    {0x0047EB60u, "canal_link", 2, 0, 0, run_canal_link, 0},
};
const int bigb2_roads_ports_count = sizeof(bigb2_roads_ports) / sizeof(bigb2_roads_ports[0]);
