/* Imhotep - terrain image rebuild at scenario start.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Reimplements what Pharaoh.exe 2.1.0.0 does to the image grid when a scenario starts
 * (0x004D52A0): zero it, rebuild every tile image from the terrain flags and the per-tile grids
 * (0x004DEA50), build the floodplain (0x004BE160), rebuild again. Rules: docs/formats/map.md §6.
 *
 * The sub-passes of 0x004DEA50 are general ports (any rectangle, view orientation and state) in
 * core/map/terrain_land.c, terrain_water.c, terrain_roads.c, terrain_flood.c, terrain_rocks.c,
 * terrain_veg.c and terrain_misc.c; this file runs them in the original's order. The rules are
 * also confirmed tile for tile against the image grids the game stored in the 29 comparable
 * scenario maps (tests/differential/terrain_check.c, @diff terrain_check).
 *
 * The original computes three values with x87 doubles (grass level, floodplain fertility, meadow
 * levels). They are replaced by integer formulas proven equal to the double computation for every
 * input 0..255 at 53-bit precision, and confirmed by the differential test. Precision: the CRT
 * startup sets _controlfp(_PC_53, _MCW_PC) (@bin 0x005640EC, from 0x00561896); no other code
 * changes the precision bits (the other FLDCW sites are __ftol 0x0056133C, which switches the
 * rounding mode only, and CRT math error handling), and the game uses DirectDraw, not Direct3D. */
#include "map/terrain.h"

#include <string.h>

#include "map/floodplain.h"
#include "map/terrain_flood.h"
#include "map/terrain_land.h"
#include "map/terrain_misc.h"
#include "map/terrain_rings.h"
#include "map/terrain_roads.h"
#include "map/terrain_rocks.h"
#include "map/terrain_tables.h"
#include "map/terrain_veg.h"
#include "map/terrain_water.h"
#include "state/scenario_start.h"

/* Neighbour order of the 8-flag scratch at 0x00BC97C0. @bin 0x0047DB00 */
static const int NEIGHBOURS[8] = {-228, -227, 1, 229, 228, 227, -1, -229};

#define GRID_AT(t, type, base, size, o) STATE_AT((t)->g, type, (base) + (size) * (uint32_t) (o))
#define TER(t, o) GRID_AT(t, state_u32, 0x00AF598Cu, 4u, o) /* terrain flags, chunk 10 */
#define MAP_W(t) S32((t)->g, 0x00BCA2D0u)
#define MAP_H(t) S32((t)->g, 0x00BCA2D4u)
#define MAP_START_(t) S32((t)->g, 0x00BCA2D8u)

static int32_t off(const terrain_state *t, int x, int y)
{
    return MAP_START_(t) + y * GRID_SIDE + x;
}

/* neighbours with terrain & mask and without exclude (@bin 0x00477690 / 0x004775F0: no scratch) */
static int ncount(const terrain_state *t, int o, uint32_t mask, uint32_t exclude)
{
    int n = 0;
    for (int i = 0; i < 8; i++) {
        uint32_t v = TER(t, o + NEIGHBOURS[i]);
        n += (v & mask) && !(v & exclude);
    }
    return n;
}

/* @bin 0x004BE160 (this = 0x00D05720) floodplain build over the rectangle grown by one tile and
 * clamped to the map: passes from depth 30 down while a pass adds a tile. A tile not yet in the
 * local depth grid, on floodplain (terrain 0x10000) or next to it, gets depth 30 when next to
 * water (terrain 4 without 0x90000), else one less than the deepest floodplain neighbour (with
 * the marker rule); surrounded tiles of plain water are skipped. It gets terrain & 0xFDFBF7FF |
 * 0x80, the land pass around it (0x004746D0), and an object in the list of its depth
 * (0x004BD740). Then the deepest non-empty lists are counted (+0x32FB8) and the terrain rebuilt.
 * @diff terrain_check */
void terrain_floodplain_build(terrain_state *t, int32_t ax, int32_t ay, int32_t bx, int32_t by)
{
    static uint8_t depth[GRID_SIZE]; /* the stack buffer of 0x004BE160 */
    floodplain_manager_reset(t->g);  /* @bin 0x004BE0F0 */
    memset(depth, 0, sizeof(depth));
    int32_t xa = ax <= bx ? ax : bx, xb = ax <= bx ? bx : ax;
    int32_t ya = ay <= by ? ay : by, yb = ay <= by ? by : ay;
    xa--, xb++, ya--, yb++;
    if (xa < 0) {
        xa = 0;
    }
    if (MAP_W(t) <= xb) {
        xb = MAP_W(t) - 1;
    }
    if (ya < 0) {
        ya = 0;
    }
    if (MAP_H(t) <= yb) {
        yb = MAP_H(t) - 1;
    }
    int marker = FLOODPLAIN_MAX_DEPTH, changed;
    do {
        changed = 0;
        for (int32_t y = ya; y <= yb; y++) {
            for (int32_t x = xa; x <= xb; x++) {
                int o = off(t, x, y);
                if (depth[o]) {
                    continue;
                }
                uint8_t flag = (uint8_t) ((TER(t, o) >> 16) & 1);
                int n_fp = ncount(t, o, 0x10000, 0);
                if (!flag && !n_fp) {
                    continue;
                }
                int d = FLOODPLAIN_MAX_DEPTH;
                int n_water = ncount(t, o, 4, 0x90000);
                if (n_water + n_fp == 8 && (TER(t, o) & 0x10004) == 4) {
                    continue;
                }
                if (n_water == 0) {
                    d = 0;
                    for (int i = 0; i < 8; i++) {
                        int n = o + NEIGHBOURS[i], nd = depth[n];
                        if ((TER(t, n) & 0x10000) && d < nd && (nd == 1 || nd != marker)) {
                            d = nd - 1;
                            if (d < 1) {
                                d = 1;
                            }
                        }
                    }
                    if (d == 0) {
                        continue;
                    }
                }
                TER(t, o) = (TER(t, o) & 0xFDFBF7FFu) | 0x80;
                terrain_land_around(t->g, x, y, 1); /* @bin 0x004746D0(x, y, 1) */
                depth[o] = (uint8_t) d;
                U32(t->g, FLOODPLAIN_MANAGER + 4u * (uint32_t) o) = floodplain_object_create(
                    t->g, FLOODPLAIN_LISTS + FLOODPLAIN_LIST_SIZE * (uint32_t) (d - 1), o, x, y, d, flag);
                changed = 1;
                U8(t->g, FLOODPLAIN_MANAGER + 0x32FBCu) = 1;
                t->stats.floodplain_objects++;
            }
        }
        marker--;
    } while (changed);
    S32(t->g, FLOODPLAIN_MANAGER + 0x32FB8u) = 0;
    for (int k = 0x1D; k >= 0 && S32(t->g, FLOODPLAIN_LISTS + FLOODPLAIN_LIST_SIZE * (uint32_t) k) != 0; k--) {
        S32(t->g, FLOODPLAIN_MANAGER + 0x32FB8u) += 1;
    }
    terrain_rebuild(t); /* @bin 0x004DEA50 */
}

/* ---- sequence ---------------------------------------------------------------------------- */

/* @bin 0x004DEA50 sub-passes in order, with the arguments the original passes (all general
 * ports). @diff terrain_check except canals (no data) */
void terrain_rebuild(terrain_state *t)
{
    /* the map size is re-read for each call; the two calls to the empty function 0x0051F780 are
     * left out */
    game_state *st = t->g;
#define X1 (MAP_W(t) - 1)
#define Y1 (MAP_H(t) - 1)
    special_rocks_rect(st, 0, 0, X1, Y1); /* 0x004770F0 */
    rocks_all(st);                        /* 0x00475DC0 */
    trees_rect(st, 0, 0, X1, Y1);         /* 0x00475BF0 */
    flag_0x20_all(st);                    /* 0x00476530 */
    if (S32(st, 0x00605A3Cu) == 0) {      /* @bin 0x004DEAC7 */
        signposts_update(st, 0);          /* 0x0047D870 */
    }
    terrain_land(st, 0, 0, X1, Y1, 0, 0); /* 0x004767D0 */
    floodplain_pass(st, 0, 0, X1, Y1);    /* 0x004777C0 */
    meadow_rect(st, 0, 0, X1, Y1);        /* 0x00477500 */
    flag_0x40000_rect(st, 0, 0, X1, Y1);  /* 0x004776F0 */
    dunes_all(st);                        /* 0x00477210 */
    floodplain_pass(st, 0, 0, X1, Y1);    /* 0x004777C0 */
    water_pass(st, 0, 0, X1, Y1, 0);      /* 0x00475CC0 */
    rubble_rect(st, 0, 0, X1, Y1);        /* 0x00477890 */
    roads_all(st);                        /* 0x0047B570 */
    plaza_rect(st, 0, 0, X1, Y1, 1);      /* 0x00475720 */
    walls_all(st);                        /* 0x0047B1A0 */
    canals_rect(st, 0, 0, X1, Y1, 0);     /* 0x0047BAE0 */
#undef X1
#undef Y1
}

void terrain_scenario_start(terrain_state *t)
{
    /* @bin 0x004D52A0 FUN_0040f8f0(image grid, 0x32C40, 0) */
    memset(state_ptr(t->g, 0x00B7482Cu, 4u * GRID_SIZE), 0, 4u * GRID_SIZE);
    terrain_rebuild(t);
    terrain_floodplain_build(t, 0, 0, MAP_W(t) - 1, MAP_H(t) - 1);
}

void terrain_store_groups(game_state *g, const image_groups *groups)
{
    for (int i = 0; i < groups->group_count; i++) {
        S32(g, 0x00F55BB4u + 8u * (uint32_t) i) = groups->group_first[i];
    }
}

enum { IN_IMAGE, IN_EDGE, IN_TERRAIN, IN_BITFIELDS, IN_RANDOM, IN_SCENARIO, IN_FERTILITY, IN_VEGETATION, IN_MOISTURE, IN_COUNT };

/* the variables the chunks are loaded into (@bin 0x005E17D8 map schema, 0x005E09A0 save schema) */
static const uint32_t IN_ADDR[IN_COUNT] = {
    0x00B7482C, 0x00B67D1C, 0x00AF598C, 0x00B285CC, 0x00ACF85C, 0x007848D8, 0x00A5D4CC, 0x00A43EAC, 0x00A509BC,
};
static const uint32_t IN_SIZE[IN_COUNT] = {
    4 * GRID_SIZE, GRID_SIZE, 4 * GRID_SIZE, GRID_SIZE, GRID_SIZE, 0x394, GRID_SIZE, GRID_SIZE, GRID_SIZE,
};

static int init_from(terrain_state *t, game_state *g, const map_file *m, const int index[IN_COUNT],
                     const image_groups *groups);

int terrain_init(terrain_state *t, game_state *g, const map_file *m, const image_groups *groups)
{
    static const int MAP[IN_COUNT] = {
        MAP_CHUNK_IMAGE, MAP_CHUNK_EDGE, MAP_CHUNK_TERRAIN, MAP_CHUNK_BITFIELDS, MAP_CHUNK_RANDOM,
        MAP_CHUNK_SCENARIO, MAP_CHUNK_FERTILITY, MAP_CHUNK_VEGETATION, MAP_CHUNK_MOISTURE,
    };
    return init_from(t, g, m, MAP, groups);
}

int terrain_init_campaign(terrain_state *t, game_state *g, const map_file *m, const image_groups *groups)
{
    static const int CAMPAIGN[IN_COUNT] = {
        CAMPAIGN_CHUNK_IMAGE, CAMPAIGN_CHUNK_EDGE, CAMPAIGN_CHUNK_TERRAIN, CAMPAIGN_CHUNK_BITFIELDS,
        CAMPAIGN_CHUNK_RANDOM, CAMPAIGN_CHUNK_SCENARIO, CAMPAIGN_CHUNK_FERTILITY, CAMPAIGN_CHUNK_VEGETATION,
        CAMPAIGN_CHUNK_MOISTURE,
    };
    if (init_from(t, g, m, CAMPAIGN, groups) != 0) {
        return -1;
    }
    /* runtime grids a saved game carries (docs/formats/save.md §3) */
    static const int OPT[2] = {CAMPAIGN_CHUNK_AQUEDUCT, CAMPAIGN_CHUNK_DESIRABILITY};
    static const uint32_t OPT_ADDR[2] = {0x00AE8E7C, 0x00AC2D4C};
    for (int i = 0; i < 2; i++) {
        if ((uint32_t) OPT[i] >= m->count || !m->chunk[OPT[i]] || m->size[OPT[i]] < GRID_SIZE) {
            return -1;
        }
        state_load_section(g, OPT_ADDR[i], m->chunk[OPT[i]], GRID_SIZE);
    }
    return 0;
}

static int init_from(terrain_state *t, game_state *g, const map_file *m, const int index[IN_COUNT],
                     const image_groups *groups)
{
    memset(t, 0, sizeof(*t));
    t->g = g;
    for (int i = 0; i < IN_COUNT; i++) {
        if ((uint32_t) index[i] >= m->count || !m->chunk[index[i]] || m->size[index[i]] < IN_SIZE[i]) {
            return -1;
        }
        state_load_section(g, IN_ADDR[i], m->chunk[index[i]], i == IN_SCENARIO ? m->size[index[i]] : IN_SIZE[i]);
    }
    /* scenario_info: width, height, start offset at +44, +48, +52 (map.md §5: consistent on 33/33
     * maps, and the row ranges reproduce every map's drawn area) @diff terrain_check */
    if (S32(g, 0x00784904u) <= 0 || S32(g, 0x00784908u) <= 0 || S32(g, 0x00784904u) > GRID_SIDE - 2 ||
        S32(g, 0x00784908u) > GRID_SIDE - 2) {
        return -1;
    }
    scenario_start_map(g); /* @bin 0x004D52A0 globals, 0x0046F7B0 rows (also clears the image grid) */
    state_load_section(g, IN_ADDR[IN_IMAGE], m->chunk[index[IN_IMAGE]], IN_SIZE[IN_IMAGE]);
    terrain_store_groups(g, groups);
    /* the ring tables the game's start-up builds (0x00540B50 -> 0x00481F20), read by the tree
     * images (0x00486600 -> 0x00481490) */
    ring_offsets_build(g);
    /* the pattern tables' counters start from the exe's values (core/map/terrain_tables.c holds
     * the same bytes, generated from the same addresses) */
    static const struct { uint32_t addr; const void *data; size_t size; } P[] = {
        {0x005CA658u, terrain_pattern_shore, sizeof(terrain_pattern_shore)},
        {0x005CBFB8u, terrain_pattern_deep, sizeof(terrain_pattern_deep)},
        {0x005CAC58u, terrain_pattern_floodplain, sizeof(terrain_pattern_floodplain)},
        {0x005CBEA8u, terrain_pattern_special_rocks, sizeof(terrain_pattern_special_rocks)},
        {0x005CC268u, terrain_pattern_roads, sizeof(terrain_pattern_roads)},
        {0x005CB9D8u, terrain_pattern_walls, sizeof(terrain_pattern_walls)},
        {0x005CBCD8u, terrain_pattern_walls_building, sizeof(terrain_pattern_walls_building)},
        {0x005CC678u, terrain_pattern_canals, sizeof(terrain_pattern_canals)},
    };
    for (size_t i = 0; i < sizeof(P) / sizeof(P[0]); i++) {
        state_load_section(g, P[i].addr, (const uint8_t *) P[i].data, P[i].size);
    }
    return 0;
}
