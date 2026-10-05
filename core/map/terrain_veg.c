/* Imhotep - the tree, meadow, flag 0x40000 and rubble sub-passes of the terrain images over any
 * rectangle (package terr).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Sub-passes of the terrain rebuild 0x004DEA50, also called on small rectangles by the game. Each
 * pass walks a rectangle clamped to the map and redraws a tile of its kind with a tile function
 * that sets or clears the tile's terrain flag and redraws the tiles of that kind in the 3x3 square
 * around it, walking the cursor 0x00BC9768 and 0x00BC9764 (x), 0x00BC9760 (y):
 *   0x00475BF0 / 0x00479770 trees (terrain 1), 0x00477500 / 0x00479B70 meadow (terrain 0x800),
 *   0x004776F0 / 0x0047AB10 flag 0x40000, 0x00477890 / 0x0047A120 rubble (terrain 0x1000). */
#include "map/terrain_veg.h"

#include "figure/actions_a.h"
#include "figure/helpers_x.h"
#include "map/terrain_land.h"
#include "state/ported.h"
#include "state/pre_load.h"

#define GRID 228
#define MAP_W 0x00BCA2D0u
#define MAP_H 0x00BCA2D4u
#define MAP_START 0x00BCA2D8u
#define MAP_STRIDE_EXTRA 0x00BCA2DCu
#define CURSOR 0x00BC9768u
#define WALK_X 0x00BC9764u
#define WALK_Y 0x00BC9760u
#define SCRATCH 0x00BC97C0u
#define M_VARIANT 0x00BC9740u
#define M_OFFSET 0x00BC9748u
#define GROUP(g) S32(st, 0x00F55BB4u + 8u * (uint32_t) (g))

#define TER(o) U32(st, 0x00AF598Cu + 4u * (uint32_t) (o))
#define IMG(o) S32(st, 0x00B7482Cu + 4u * (uint32_t) (o))
#define EDGE(o) U8(st, 0x00B67D1Cu + (uint32_t) (o))
#define BITS(o) U8(st, 0x00B285CCu + (uint32_t) (o))
#define RND(o) U8(st, 0x00ACF85Cu + (uint32_t) (o))
#define AQ(o) U8(st, 0x00AE8E7Cu + (uint32_t) (o))
#define MOIST(o) U8(st, 0x00A509BCu + (uint32_t) (o))
#define FERT(o) U8(st, 0x00A5D4CCu + (uint32_t) (o))

enum {
    G_0X40000 = 508,       /* @bin 0x0047AB10 [0x00F56B94] */
    G_RUBBLE = 531,        /* @bin 0x0047A120 [0x00F56C4C] */
    G_MEADOW = 534,        /* @bin 0x00479B70 0x216 */
    G_MEADOW_A2B11 = 551,  /* @bin 0x00479B70 0x227 */
    G_MEADOW_A2B0 = 552,   /* @bin 0x00479B70 0x228 */
    G_MEADOW_DRY = 563,    /* @bin 0x00479B70 0x233 */
    G_0X40000_BLOOD = 721, /* @bin 0x0047AB10 [0x00F5723C] while the map reveal has not reached it */
};

/* ---- the rectangle walk -------------------------------------------------------------------- */

typedef int (*tile_test)(uint32_t terrain);
typedef void (*tile_redraw)(game_state *st, int32_t x, int32_t y);

static void rect_walk(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1, tile_test test,
                      tile_redraw redraw)
{
    /* @bin 0x00475BF0 / 0x00477500 / 0x004776F0 / 0x00477890 the rectangle clamped to the map
     * (x0, y0 >= 0; x1 < width, y1 < height); the tile offset advances by the row's tiles, then
     * by width - x1 + x0 - 1 + [0x00BCA2DC] (228 in all when the row is not empty) */
    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    int32_t w = S32(st, MAP_W);
    if (x1 >= w) {
        x1 = w - 1;
    }
    if (y1 >= S32(st, MAP_H)) {
        y1 = S32(st, MAP_H) - 1;
    }
    int32_t step = w - x1 + x0 - 1;
    int32_t o = S32(st, MAP_START) + y0 * GRID + x0;
    for (int32_t y = y0; y <= y1; y++) {
        for (int32_t x = x0; x <= x1; x++, o++) {
            if (test(TER(o))) {
                redraw(st, x, y);
            }
        }
        o += S32(st, MAP_STRIDE_EXTRA) + step;
    }
}

/* the 3x3 square around (x, y) clamped to the map: (x - 1, y - 1) and its width, height */
static void square_3x3(game_state *st, int32_t x, int32_t y, int32_t *xa, int32_t *ya, int32_t *w, int32_t *h)
{
    *xa = x - 1;
    *ya = y - 1;
    *w = 3;
    *h = 3;
    if (*xa < 0) {
        *w = *xa + 3;
        *xa = 0;
    } else if (*xa + 3 > S32(st, MAP_W)) {
        *w = S32(st, MAP_W) - *xa;
    }
    if (*ya < 0) {
        *h = *ya + 3;
        *ya = 0;
    } else if (*ya + 3 > S32(st, MAP_H)) {
        *h = S32(st, MAP_H) - *ya;
    }
}

static int inside(game_state *st, int32_t x, int32_t y)
{
    return map_tile_inside(st, S32(st, 0x00BCA2E8u) + x, S32(st, 0x00BCA2ECu) + y) != 0;
}

/* ---- trees ------------------------------------------------------------------------------------ */

static int is_tree(uint32_t v)
{
    return (v & 1) && !(v & 0x600);
}

void trees_tile_redraw(game_state *st, int32_t x, int32_t y, int32_t flag)
{
    /* @bin 0x00479770 (x, y, flag) inside the map: terrain 1 set (flag 1) or cleared; the trees of
     * the 3x3 square: 0x00486600, bit-field & 0xF0, edge 0x40, aqueduct grid 0 */
    if (!inside(st, x, y)) {
        return;
    }
    int32_t o = S32(st, MAP_START) + y * GRID + x;
    if (flag == 1) {
        TER(o) |= 1;
    } else {
        TER(o) &= 0xFFFFFFFEu;
    }
    int32_t xa, ya, w, h;
    square_3x3(st, x, y, &xa, &ya, &w, &h);
    S32(st, WALK_Y) = ya;
    S32(st, CURSOR) = S32(st, MAP_START) + ya * GRID + xa;
    while (S32(st, WALK_Y) < ya + h) {
        int32_t c = S32(st, CURSOR);
        for (S32(st, WALK_X) = xa; S32(st, WALK_X) < xa + w;) {
            if (is_tree(TER(c))) {
                tree_image_update(st, c);
                c = S32(st, CURSOR);
                BITS(c) &= 0xF0;
                EDGE(c) |= 0x40;
                AQ(c) = 0;
            }
            S32(st, WALK_X) += 1;
            c++;
            S32(st, CURSOR) = c;
        }
        S32(st, WALK_Y) += 1;
        S32(st, CURSOR) = c + 0xE4 - w;
    }
}

static void trees_redraw_1(game_state *st, int32_t x, int32_t y)
{
    trees_tile_redraw(st, x, y, 1);
}

void trees_rect(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    /* @bin 0x00475BF0 every tree (terrain 1 without 0x600): 0x00479770(x, y, 1) */
    rect_walk(st, x0, y0, x1, y1, is_tree, trees_redraw_1);
}

/* ---- meadow ----------------------------------------------------------------------------------- */

static int is_meadow(uint32_t v)
{
    /* @bin 0x00477500, 0x00479B70 terrain 0x800 without 0x1700, 0x40, 0x20; 8 only with
     * 0x10000000 */
    return (v & 0x800) && !(v & 0x1700) && !(v & 0x40) && (!(v & 8) || (v & 0x10000000u)) && !(v & 0x20);
}

/* x87 trunc((f - 1) * [0x0056F588] (1/99) * [0x0056F580] (3.0)) equals (f - 1) * 3 / 99 (0 for
 * f = 0); trunc((m - 1) / [0x0056F570] (96.0) * [0x0056F578] (12.0)) equals (m - 1) >> 3 (0 for
 * m = 0): checked for every byte with the exe's constants at 53-bit precision.
 * @bin 0x00479D1F, 0x00479D55 */
static int32_t meadow_fertility_level(int32_t f)
{
    return f == 0 ? 0 : (f - 1) * 3 / 99;
}

static int32_t meadow_moisture_level(int32_t m)
{
    return m == 0 ? 0 : (m - 1) >> 3;
}

void meadow_tile_redraw(game_state *st, int32_t x, int32_t y, int32_t flag)
{
    /* @bin 0x00479B70 (x, y, flag) inside the map: terrain 0x800 set (flag 1) or cleared; the
     * meadow tiles of the 3x3 square get the image of their fertility and moisture levels,
     * bit-field & 0xF0, edge 0x40, aqueduct grid 0. Only the start and the end of the walk are
     * written to the cursor; 0x00BC9760 holds the end row, 0x00BC9764 each row's end. */
    if (!inside(st, x, y)) {
        return;
    }
    int32_t o = S32(st, MAP_START) + y * GRID + x;
    if (flag == 1) {
        TER(o) |= 0x800;
    } else {
        TER(o) &= 0xFFFFF7FFu;
    }
    int32_t xa, ya, w, h;
    square_3x3(st, x, y, &xa, &ya, &w, &h);
    S32(st, WALK_Y) = ya;
    int32_t c = S32(st, MAP_START) + ya * GRID + xa;
    S32(st, CURSOR) = c;
    if (ya >= ya + h) {
        return;
    }
    S32(st, WALK_Y) = ya + h;
    for (int32_t r = 0; r < h; r++) {
        S32(st, WALK_X) = xa;
        if (xa < xa + w) {
            S32(st, WALK_X) = xa + w;
            for (int32_t i = 0; i < w; i++, c++) {
                if (!is_meadow(TER(c))) {
                    continue;
                }
                int32_t a = meadow_fertility_level(FERT(c));
                int32_t m = MOIST(c) > 100 ? 100 : MOIST(c);
                int32_t b = meadow_moisture_level(m);
                int32_t group = G_MEADOW, idx = b;
                if (a == 0) {
                    if (b == 0) {
                        group = G_MEADOW_DRY;
                        idx = RND(c) & 7;
                    }
                } else if (a == 2) {
                    if (b == 0) {
                        group = G_MEADOW_A2B0;
                        idx = RND(c) & 7;
                        a = 0;
                    } else if (b == 0xB) {
                        group = G_MEADOW_A2B11;
                        idx = RND(c) & 7;
                        a = 0;
                    }
                }
                IMG(c) = GROUP(group) + a * 12 + idx;
                BITS(c) &= 0xF0;
                EDGE(c) |= 0x40;
                AQ(c) = 0;
            }
        }
        c += 0xE4 - w;
    }
    S32(st, CURSOR) = c;
}

static void meadow_redraw_1(game_state *st, int32_t x, int32_t y)
{
    meadow_tile_redraw(st, x, y, 1);
}

void meadow_rect(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    /* @bin 0x00477500 every meadow tile: 0x00479B70(x, y, 1) */
    rect_walk(st, x0, y0, x1, y1, is_meadow, meadow_redraw_1);
}

/* ---- flag 0x40000 ----------------------------------------------------------------------------- */

static int is_0x40000(uint32_t v)
{
    return (v & 0x40000) && !(v & 8);
}

void flag_0x40000_tile_redraw(game_state *st, int32_t x, int32_t y, int32_t flag)
{
    /* @bin 0x0047AB10 (x, y, flag) inside the map: cursor := the tile, terrain 0x40000 set (flag
     * 1) or cleared; 0x0047A1C0 (map reveal) at the square's corner picks the group; the 0x40000
     * tiles of the 3x3 square match their inverted 0x40000 neighbours (table 0x005CA658, 48
     * records, variant from the random grid), bit-field & 0xF0, edge 0x40, then 0x00486600 */
    if (!inside(st, x, y)) {
        return;
    }
    int32_t o = S32(st, MAP_START) + y * GRID + x;
    S32(st, CURSOR) = o;
    if (flag == 1) {
        TER(o) |= 0x40000;
    } else {
        TER(o) &= 0xFFFBFFFFu;
    }
    int32_t xa, ya, w, h;
    square_3x3(st, x, y, &xa, &ya, &w, &h);
    uint8_t blood = (uint8_t) figure_near_map_edge(st, xa);
    S32(st, WALK_Y) = ya;
    S32(st, CURSOR) = S32(st, MAP_START) + ya * GRID + xa;
    while (S32(st, WALK_Y) < ya + h) {
        int32_t c = S32(st, CURSOR);
        for (S32(st, WALK_X) = xa; S32(st, WALK_X) < xa + w;) {
            if (is_0x40000(TER(c))) {
                land_neighbour_flags(st, 0x40000);
                for (uint32_t i = 0; i < 8; i++) { /* @bin 0x00484F00 */
                    S32(st, SCRATCH + 4u * i) = S32(st, SCRATCH + 4u * i) == 0;
                }
                land_pattern_match(st, 0x005CA658u, 0x30, RND(S32(st, CURSOR)), 0);
                c = S32(st, CURSOR);
                IMG(c) = GROUP(blood ? G_0X40000_BLOOD : G_0X40000) + S32(st, M_VARIANT) + S32(st, M_OFFSET);
                BITS(c) &= 0xF0;
                EDGE(c) |= 0x40;
                tree_image_update(st, c);
                c = S32(st, CURSOR);
            }
            S32(st, WALK_X) += 1;
            c++;
            S32(st, CURSOR) = c;
        }
        S32(st, WALK_Y) += 1;
        S32(st, CURSOR) = c + 0xE4 - w;
    }
}

static void flag_0x40000_redraw_1(game_state *st, int32_t x, int32_t y)
{
    flag_0x40000_tile_redraw(st, x, y, 1);
}

void flag_0x40000_rect(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    /* @bin 0x004776F0 every tile with terrain 0x40000 without 8: 0x0047AB10(x, y, 1) */
    rect_walk(st, x0, y0, x1, y1, is_0x40000, flag_0x40000_redraw_1);
}

/* ---- rubble ----------------------------------------------------------------------------------- */

static int is_rubble(uint32_t v)
{
    return (v & 0x1000) && !(v & 0x700) && !(v & 0x68);
}

void rubble_tile_redraw(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x0047A120 (x, y) inside the map: cursor := the tile, bit-field & 0xF0, image group
     * 531 + random & 7, aqueduct grid 0, edge 0x40, terrain 0x1000 */
    if (!inside(st, x, y)) {
        return;
    }
    int32_t o = S32(st, MAP_START) + y * GRID + x;
    BITS(o) &= 0xF0;
    S32(st, CURSOR) = o;
    AQ(o) = 0;
    IMG(o) = (RND(o) & 7) + GROUP(G_RUBBLE);
    EDGE(o) |= 0x40;
    TER(o) |= 0x1000;
}

void rubble_rect(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    /* @bin 0x00477890 every tile with terrain 0x1000 without 0x768: 0x0047A120(x, y) */
    rect_walk(st, x0, y0, x1, y1, is_rubble, rubble_tile_redraw);
}

/* ---- table ------------------------------------------------------------------------------------ */

#define RUN3(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        fn(st, a[0], a[1], a[2]); \
        return 0; \
    }
#define RUN4(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        fn(st, a[0], a[1], a[2], a[3]); \
        return 0; \
    }
RUN3(run_trees_tile, trees_tile_redraw)
RUN4(run_trees_rect, trees_rect)
RUN3(run_meadow_tile, meadow_tile_redraw)
RUN4(run_meadow_rect, meadow_rect)
RUN3(run_0x40000_tile, flag_0x40000_tile_redraw)
RUN4(run_0x40000_rect, flag_0x40000_rect)
RUN4(run_rubble_rect, rubble_rect)

static int32_t run_rubble_tile(game_state *st, const int32_t *a)
{
    rubble_tile_redraw(st, a[0], a[1]);
    return 0;
}

const ported_function terr_veg_ports[] = {
    {0x00479770u, "trees_tile_redraw", 3, 0, 0, run_trees_tile, 0},
    {0x00475BF0u, "trees_rect", 4, 0, 0, run_trees_rect, 0},
    {0x00479B70u, "meadow_tile_redraw", 3, 0, 0, run_meadow_tile, 0},
    {0x00477500u, "meadow_rect", 4, 0, 0, run_meadow_rect, 0},
    {0x0047AB10u, "flag_0x40000_tile_redraw", 3, 0, 0, run_0x40000_tile, 0},
    {0x004776F0u, "flag_0x40000_rect", 4, 0, 0, run_0x40000_rect, 0},
    {0x0047A120u, "rubble_tile_redraw", 2, 0, 0, run_rubble_tile, 0},
    {0x00477890u, "rubble_rect", 4, 0, 0, run_rubble_rect, 0},
};
const int terr_veg_ports_count = sizeof(terr_veg_ports) / sizeof(terr_veg_ports[0]);
