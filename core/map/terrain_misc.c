/* Imhotep - the signpost, plaza and wall sub-passes of the terrain images (package terr).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Sub-passes of the terrain rebuild 0x004DEA50, ported for any arguments and state:
 *   0x0047D870 (clear) the entry / exit signposts (placed near the scenario's entry and exit
 *   points, view orientation 0x00E94E04; clear != 0 removes them),
 *   0x00475720 (x0, y0, x1, y1, clear) plaza tiles of a rectangle,
 *   0x0047B1A0 walls over the whole map (cursor 0x00BC9768 and 0x00BC9764 / 0x00BC9760). */
#include "map/terrain_misc.h"

#include "map/map_building.h"
#include "map/terrain_land.h"
#include "state/layout.h"
#include "state/ported.h"

#define GRID 228
#define GROUP(g) S32(st, G_DAY2_REDRAW_GROUPS + 8u * (uint32_t) (g))

#define TER(o) U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (o))
#define IMG(o) S32(st, G_IMAGE + 4u * (uint32_t) (o))
#define EDGE(o) U8(st, G_EDGE + (uint32_t) (o))
#define BITS(o) U8(st, G_DAY2_DESTROY_BITS + (uint32_t) (o))
#define RND(o) U8(st, G_RANDOM + (uint32_t) (o))
#define BGRID(o) S16(st, G_BUILDING_GRID + 2u * (uint32_t) (o))
#define BUILDING(id) BUILDING_ADDR(id)

enum {
    G_WALLS = 407,     /* @bin 0x0047B1A0 [0x00F5686C] */
    G_PLAZA = 437,     /* @bin 0x00475720 [0x00F5695C] */
    G_SIGNPOSTS = 521, /* @bin 0x0047D870 [0x00F56BFC] */
};

/* @bin 0x0047DB00 neighbour order of the scratch */
static const int32_t NEIGHBOURS[8] = {-228, -227, 1, 229, 228, 227, -1, -229};

/* ---- signposts -------------------------------------------------------------------------------- */

int32_t terrain_find_empty(game_state *st, int32_t x, int32_t y, int32_t n, int32_t d, int32_t exclude)
{
    /* @bin 0x00482810 the square clamped to the map; the row offset advances by xa - xb + 227
     * after each row (228 when the row is not empty) */
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
            if (o != exclude && TER(o) == 0) {
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

static int32_t edge_direction(int32_t px, int32_t py, int32_t w, int32_t h)
{
    /* @bin 0x0047D8D4 the map edge of a point: west 2, east 6, north 0, south 4, else -1 */
    if (px == 0) {
        return 2;
    }
    if (px == w - 1) {
        return 6;
    }
    if (py == 0) {
        return 0;
    }
    return py == h - 1 ? 4 : -1;
}

static void signpost_place(game_state *st, uint32_t point, int32_t dir, uint32_t xy, uint32_t tile_var, int32_t extra)
{
    /* @bin 0x0047D96F the first empty tile (terrain 0) within 1..9 tiles of the point (not the
     * point itself): its position at xy (x, y), offset at tile_var, terrain 2, image group 521 +
     * ((orientation 0x00E94E04 + dir) mod 8) / 2 + extra */
    int32_t exclude = S32(st, G_MAP_START) + S16(st, point) + S16(st, point + 2) * GRID;
    for (int32_t k = 1; k < 10; k++) {
        if (terrain_find_empty(st, S16(st, point), S16(st, point + 2), 1, k, exclude)) {
            break;
        }
    }
    int32_t y = S32(st, G_ROAD_ACCESS_Y), x = S32(st, G_ROAD_ACCESS_X);
    S32(st, xy + 4) = y;
    S32(st, xy) = x;
    int32_t o = S32(st, G_MAP_START) + y * GRID + x;
    S32(st, tile_var) = o;
    TER(o) |= 2;
    int32_t d = S32(st, G_UE94E04) + dir;
    if (d >= 8) {
        d -= 8;
    }
    IMG(o) = d / 2 + GROUP(G_SIGNPOSTS) + extra;
}

void signposts_update(game_state *st, int32_t clear)
{
    /* @bin 0x0047D870 clear: terrain & 0xDFCFFFFD at the two signposts (0x00BCA318 / 0x00BCA31C,
     * 0x00BCA320 / 0x00BCA324); else the entry (scenario +0x38C, 0x00784C64) and exit
     * (0x00784C68) points on a map edge get their signpost (exit images +4) */
    if (clear != 0) {
        int32_t start = S32(st, G_MAP_START);
        TER(start + S32(st, G_ENTRY_FLAG_X) + S32(st, G_ENTRY_FLAG_Y) * GRID) &= 0xDFCFFFFDu;
        TER(start + S32(st, G_EXIT_FLAG_X) + S32(st, G_EXIT_FLAG_Y) * GRID) &= 0xDFCFFFFDu;
        return;
    }
    int32_t w = S32(st, G_MAP_WIDTH), h = S32(st, G_MAP_HEIGHT);
    int32_t entry = edge_direction(S16(st, G_ENTRY_POINT_X), S16(st, G_ENTRY_POINT_Y), w, h);
    int32_t exit = edge_direction(S16(st, G_EXIT_POINT_X), S16(st, G_EXIT_POINT_Y), w, h);
    if (entry >= 0) {
        signpost_place(st, G_ENTRY_POINT_X, entry, G_ENTRY_FLAG_X, G_ENTRY_FLAG_TILE, 0);
    }
    if (exit >= 0) {
        signpost_place(st, G_EXIT_POINT_X, exit, G_EXIT_FLAG_X, G_EXIT_FLAG_TILE, 4);
    }
}

/* ---- plaza ------------------------------------------------------------------------------------ */

int32_t plaza_block_free(game_state *st, int32_t tile)
{
    /* @bin 0x00475670 the east, south and south-east tiles: +1 for terrain 0x40, -1 for no
     * bit-field 0x80, for terrain 4, 8, 0x100 and for an image; 3 means all three are plain
     * plaza tiles */
    static const int32_t D[3] = {1, GRID, GRID + 1};
    int32_t u = 0;
    for (int i = 0; i < 3; i++) {
        int32_t o = tile + D[i];
        uint32_t v = TER(o);
        u += (v & 0x40) ? 1 : 0;
        u -= (BITS(o) & 0x80) ? 0 : 1;
        u -= (v & 4) ? 1 : 0;
        u -= (v & 8) ? 1 : 0;
        u -= (v & 0x100) ? 1 : 0;
        u -= IMG(o) != 0 ? 1 : 0;
    }
    return u == 3;
}

void plaza_rect(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t clear)
{
    /* @bin 0x00475720 the corners ordered, clamped to the map; with clear, the plaza tiles
     * (terrain 0x40, bit-field 0x80) not under a building of type 0x8A lose their image first;
     * then each plaza tile without an image: a 2x2 block (0x00475670) image +6 / +7 (random & 1)
     * by 0x0047BF50, else the image of the tile's parity, edge 0x40. The tile offset advances by
     * width - x1 + x0 - 1 + [0x00BCA2DC] after each row (228 in all when the row is not empty). */
    if (x0 > x1) {
        int32_t s = x0;
        x0 = x1;
        x1 = s;
    }
    if (y0 > y1) {
        int32_t s = y0;
        y0 = y1;
        y1 = s;
    }
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
    if (clear != 0) {
        int32_t o = S32(st, G_MAP_START) + y0 * GRID + x0;
        for (int32_t y = y0; y <= y1; y++) {
            for (int32_t x = x0; x <= x1; x++, o++) {
                if (!(TER(o) & 0x40) || !(BITS(o) & 0x80)) {
                    continue;
                }
                int32_t id = BGRID(o);
                if (id != 0 && S16(st, BUILDING(id) + BLD_TYPE) == 0x8A) {
                    continue;
                }
                IMG(o) = 0;
                BITS(o) &= 0xF0;
                EDGE(o) |= 0x40;
            }
            o += S32(st, G_MAP_STRIDE_EXTRA) + step;
        }
        w = S32(st, G_MAP_WIDTH); /* @bin 0x00475861 re-read after the clearing */
        step = w - x1 + x0 - 1;
    }
    int32_t o = S32(st, G_MAP_START) + y0 * GRID + x0;
    for (int32_t y = y0; y <= y1; y++) {
        for (int32_t x = x0; x <= x1; x++, o++) {
            if (!(TER(o) & 0x40) || IMG(o) != 0 || !(BITS(o) & 0x80)) {
                continue;
            }
            int32_t odd = RND(o) & 1;
            if (plaza_block_free(st, o)) {
                map_building_tiles(st, 0, x, y, 2, 2, GROUP(G_PLAZA) + (odd ? 7 : 6), 0x40);
                continue;
            }
            if ((x & 1) && (y & 1)) {
                IMG(o) = GROUP(G_PLAZA) + 1;
            } else if (!(x & 1) && !(y & 1)) {
                IMG(o) = GROUP(G_PLAZA);
            } else {
                IMG(o) = GROUP(G_PLAZA) + 2;
            }
            EDGE(o) |= 0x40;
        }
        o += S32(st, G_MAP_STRIDE_EXTRA) + step;
    }
}

/* ---- walls ------------------------------------------------------------------------------------ */

static int32_t scratch_walls_buildings(game_state *st)
{
    /* @bin 0x0047E000 the scratch: orthogonal neighbours' terrain & 0x8000 (as is), diagonals 0;
     * returns 1 when an orthogonal neighbour has 0x8000 */
    int32_t c = S32(st, G_TILE_CURSOR);
    uint32_t n = TER(c - GRID) & 0x8000, e = TER(c + 1) & 0x8000, s = TER(c + GRID) & 0x8000,
             w = TER(c - 1) & 0x8000;
    S32(st, G_NEIGHBOUR_FLAGS + 28) = 0;
    S32(st, G_NEIGHBOUR_FLAGS + 20) = 0;
    S32(st, G_NEIGHBOUR_FLAGS + 12) = 0;
    S32(st, G_NEIGHBOUR_FLAGS + 4) = 0;
    S32(st, G_NEIGHBOUR_FLAGS + 0) = (int32_t) n;
    S32(st, G_NEIGHBOUR_FLAGS + 8) = (int32_t) e;
    S32(st, G_NEIGHBOUR_FLAGS + 16) = (int32_t) s;
    S32(st, G_NEIGHBOUR_FLAGS + 24) = (int32_t) w;
    return n != 0 || e != 0 || s != 0 || w != 0;
}

static void scratch_add_walls(game_state *st)
{
    /* @bin 0x0047E750 scratch[i] += neighbour terrain bit 14 (0x4000) */
    int32_t c = S32(st, G_TILE_CURSOR);
    for (uint32_t i = 0; i < 8; i++) {
        S32(st, G_NEIGHBOUR_FLAGS + 4u * i) += (int32_t) ((TER(c + NEIGHBOURS[i]) >> 14) & 1);
    }
}

void walls_all(game_state *st)
{
    /* @bin 0x0047B1A0 every wall tile (terrain 0x4000 without 8) of the map: inverted 0x4000
     * neighbours matched in table 0x005CB9D8 (48 records), image group 407 + variant + offset
     * (also when nothing matched), bit-field & 0xF0, edge 0x40; next to a 0x8000 tile, table
     * 0x005CBCD8 (15 records) on the 0x8000 / 0x4000 neighbours. The calls to the empty function
     * 0x0051F780 are left out. */
    S32(st, G_TILE_CURSOR) = S32(st, G_MAP_START);
    S32(st, G_WALK_Y) = 0;
    if (S32(st, G_MAP_HEIGHT) <= 0) {
        return;
    }
    do {
        S32(st, G_WALK_X) = 0;
        if (S32(st, G_MAP_WIDTH) > 0) {
            do {
                int32_t c = S32(st, G_TILE_CURSOR);
                uint32_t v = TER(c);
                if ((v & 0x4000) && !(v & 8)) {
                    land_neighbour_flags(st, 0x4000);
                    for (uint32_t i = 0; i < 8; i++) { /* @bin 0x00484F00 */
                        S32(st, G_NEIGHBOUR_FLAGS + 4u * i) = S32(st, G_NEIGHBOUR_FLAGS + 4u * i) == 0;
                    }
                    land_pattern_match(st, G_PATTERN_WALLS, 0x30, -1, 0);
                    c = S32(st, G_TILE_CURSOR);
                    BITS(c) &= 0xF0;
                    EDGE(c) |= 0x40;
                    IMG(c) = GROUP(G_WALLS) + S32(st, G_MATCH_VARIANT) + S32(st, G_MATCH_OFFSET);
                    if (scratch_walls_buildings(st)) {
                        scratch_add_walls(st);
                        if (land_pattern_match(st, G_PATTERN_WALLS_BUILDING, 0xF, -1, 0)) {
                            IMG(S32(st, G_TILE_CURSOR)) = GROUP(G_WALLS) + S32(st, G_MATCH_VARIANT) + S32(st, G_MATCH_OFFSET);
                        }
                    }
                }
                S32(st, G_WALK_X) += 1;
                S32(st, G_TILE_CURSOR) += 1;
            } while (S32(st, G_WALK_X) < S32(st, G_MAP_WIDTH));
        }
        S32(st, G_TILE_CURSOR) += S32(st, G_MAP_STRIDE_EXTRA);
        S32(st, G_WALK_Y) += 1;
    } while (S32(st, G_WALK_Y) < S32(st, G_MAP_HEIGHT));
}

/* ---- table ------------------------------------------------------------------------------------ */

static int32_t run_find_empty(game_state *st, const int32_t *a)
{
    return terrain_find_empty(st, a[0], a[1], a[2], a[3], a[4]);
}

static int32_t run_signposts(game_state *st, const int32_t *a)
{
    signposts_update(st, a[0]);
    return 0;
}

static int32_t run_plaza_free(game_state *st, const int32_t *a)
{
    return plaza_block_free(st, a[0]);
}

static int32_t run_plaza(game_state *st, const int32_t *a)
{
    plaza_rect(st, a[0], a[1], a[2], a[3], a[4]);
    return 0;
}

static int32_t run_walls(game_state *st, const int32_t *a)
{
    (void) a;
    walls_all(st);
    return 0;
}

static int32_t run_walls_buildings(game_state *st, const int32_t *a)
{
    (void) a;
    return scratch_walls_buildings(st);
}

static int32_t run_add_walls(game_state *st, const int32_t *a)
{
    (void) a;
    scratch_add_walls(st);
    return 0;
}

const ported_function terr_misc_ports[] = {
    {0x00482810u, "terrain_find_empty", 5, 32, 0, run_find_empty, 0},
    {0x0047D870u, "signposts_update", 1, 0, 0, run_signposts, 0},
    {0x00475670u, "plaza_block_free", 1, 32, 0, run_plaza_free, 0},
    {0x00475720u, "plaza_rect", 5, 0, 0, run_plaza, 0},
    {0x0047E000u, "scratch_walls_buildings", 0, 32, 0, run_walls_buildings, 0},
    {0x0047E750u, "scratch_add_walls", 0, 0, 0, run_add_walls, 0},
    {0x0047B1A0u, "walls_all", 0, 0, 0, run_walls, 0},
};
const int terr_misc_ports_count = sizeof(terr_misc_ports) / sizeof(terr_misc_ports[0]);
