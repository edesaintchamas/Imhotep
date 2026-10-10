/* Imhotep - map values computed for the hovered tile (package cmdrest, docs/commands.md 2.1, 5).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The original computes these in its city-view drawing code, every frame, for the tile under the
 * mouse (0x00BCA300 offset, 0x00BCA304 x, 0x00BCA308 y); the place step 0x004728A0 reads them.
 * They are map rules, so core has them as "hover" functions that the platform calls each frame
 * for the hovered tile while a ferry (type 0x88) or a bridge (0x52, 0x53) is being placed, as
 * the original's draw does:
 *   - ferry_reach (0x0047F3E0, called by the ferry ghost draw 0x0043D300): how far the water
 *     reaches in front of a landing, 0x00BC96E0..0x00BC96F0;
 *   - bridge_measure (0x0047F200, called by the city-view draw 0x0043B630): the bridge that
 *     starts at a tile, 0x00BC96F4..0x00BC9704;
 *   - ferry_hover (the rule part of 0x0043D300): ferry_reach, then the landing validity
 *     0x00BCA314, with every state write of 0x0043D300's own callees; the drawing calls
 *     (0x004F0BA0 screen position, 0x0043D010, the sprite calls 0x0053E590 / 0x0053F250 /
 *     0x0053F4F0 on 0x00F55BB0, the draw globals 0x0070E120 / 0x0070E13C) are left out. */
#include "command/hover.h"

#include "building/day_access.h"
#include "city/day2_misc.h"
#include "command/construction_checks.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


static uint32_t terrain(game_state *st, int32_t off)
{
    return U32(st, G_GRID_TERRAIN + 4u * (uint32_t) off);
}

int32_t edge_neighbours_with(game_state *st, int32_t off, uint32_t mask)
{
    /* @bin 0x0047EC80 the edge neighbours (-228, +1, +228, -1) whose terrain has a bit of mask */
    int32_t n = 0;
    n += (terrain(st, off - 0xE4) & mask) != 0;
    n += (terrain(st, off + 1) & mask) != 0;
    n += (terrain(st, off + 0xE4) & mask) != 0;
    n += (terrain(st, off - 1) & mask) != 0;
    return n;
}

int32_t corner_neighbours_with(game_state *st, int32_t off, uint32_t mask)
{
    /* @bin 0x0047ECC0 the corner neighbours (-227, +229, +227, -229) */
    int32_t n = 0;
    n += (terrain(st, off - 0xE3) & mask) != 0;
    n += (terrain(st, off + 0xE5) & mask) != 0;
    n += (terrain(st, off + 0xE3) & mask) != 0;
    n += (terrain(st, off - 0xE5) & mask) != 0;
    return n;
}

int32_t ferry_reach(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x0047F3E0 (x, y, unused) from the landing's 2x2 shore pattern (0x0047CBD0): its
     * direction 0x00BC96E0 and grid step 0x00BC96EC; then the water tiles in front, up to 40:
     * 0x00BC96E4 the count reached (+1), 0x00BC96E8 the tile before the first dry one (and
     * 0x00BC96E4 := 40 then). Returns eax: the step (or twice the pattern when none fits). */
    S32(st, G_REACH_END) = 0;
    S32(st, G_REACH_LEN) = 0;
    S32(st, G_REACH_STEP) = 0;
    S32(st, G_REACH_DIR) = 0;
    int32_t off = S32(st, G_MAP_START) + y * 0xE4 + x;
    S32(st, G_REACH_START) = off;
    int32_t k = tile_bit4_pattern(st, x, y) * 2;
    int32_t step;
    /* @bin 0x0047F431 jump table 0x0047F50C on twice the pattern (0..6) */
    switch ((uint32_t) k) {
    case 0: S32(st, G_REACH_DIR) = 0; step = -0xE4; break;
    case 2: S32(st, G_REACH_DIR) = 2; step = 1; break;
    case 4: S32(st, G_REACH_DIR) = 4; step = 0xE4; break;
    case 6: S32(st, G_REACH_DIR) = 6; step = -1; break;
    default: return k;
    }
    S32(st, G_REACH_STEP) = step;
    int32_t n = 1;
    for (int32_t count = 0;;) {
        /* @bin 0x0047F4AC the tile two steps on, the cursor one step on */
        uint8_t t = (uint8_t) terrain(st, off + 2 * step);
        off += step;
        n++;
        if ((t & 1) || U8(st, G_SPRITE + (uint32_t) (off + step)) != 0) {
            break;
        }
        if (!(t & 4)) {
            /* @bin 0x0047F4EF dry land: the end tile, the length set to 40 */
            S32(st, G_REACH_LEN) = n;
            S32(st, G_REACH_END) = off;
            S32(st, G_REACH_LEN) = 0x28;
            return step;
        }
        if (S32(st, G_FERRY_STATE) != 0 && (t & 0x48)) {
            break;
        }
        if (++count >= 0x28) {
            break;
        }
    }
    S32(st, G_REACH_LEN) = n;
    return step;
}

int32_t bridge_measure(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x0047F200 a bridge from the shore water tile (x, y): water (bit 4) without bits
     * 0x48, three edge neighbours in water and none with bit 0x10000; the direction is the side
     * of the dry neighbour (north, east, south, west in that order); then the water tiles across,
     * at most 4 more that each have their 4 corners in water: 0x00BC96F8 the length, 0x00BC96FC
     * the far end when the bridge lands on a tile of the other shore with the same conditions
     * (else 0) */
    S32(st, G_BRIDGE_END) = 0;
    S32(st, G_BRIDGE_LENGTH) = 0;
    S32(st, G_BRIDGE_STEP) = 0;
    S32(st, G_BRIDGE_DIR) = 0;
    int32_t off = S32(st, G_MAP_START) + y * 0xE4 + x;
    S32(st, G_BRIDGE_START) = off;
    uint8_t t = (uint8_t) terrain(st, off);
    if (!(t & 4) || (t & 0x48)) {
        return 0;
    }
    if (edge_neighbours_with(st, off, 4) != 3 || edge_neighbours_with(st, off, 0x10000) != 0) {
        return 0;
    }
    int32_t step;
    if (!((uint8_t) terrain(st, off - 0xE4) & 4)) {
        step = 0xE4;
        S32(st, G_BRIDGE_DIR) = 4;
    } else if (!((uint8_t) terrain(st, off + 1) & 4)) {
        step = -1;
        S32(st, G_BRIDGE_DIR) = 6;
    } else if (!((uint8_t) terrain(st, off + 0xE4) & 4)) {
        step = -0xE4;
        S32(st, G_BRIDGE_DIR) = 0;
    } else if (!((uint8_t) terrain(st, off - 1) & 4)) {
        step = 1;
        S32(st, G_BRIDGE_DIR) = 2;
    } else {
        return 0;
    }
    S32(st, G_BRIDGE_STEP) = step;
    S32(st, G_BRIDGE_LENGTH) = 1;
    for (int32_t i = 0;;) {
        off += step;
        S32(st, G_BRIDGE_LENGTH) = S32(st, G_BRIDGE_LENGTH) + 1;
        uint8_t next = (uint8_t) terrain(st, off + step);
        if (next & 1) {
            return 0;
        }
        uint32_t here = terrain(st, off);
        if ((here & 0x4000000u) && (here & 0x40)) {
            return 0;
        }
        /* @bin 0x0047F35D the 8 neighbours in the grid 0x00EE0C50: none 1 or 3 */
        for (uint32_t k = 0; k < 8; k++) {
            uint8_t v = U8(st, G_GRID_ROUTING + (uint32_t) (off + S32(st, G_DIRECTION_OFFSETS + 4u * k)));
            if (v == 1 || v == 3) {
                return 0;
            }
        }
        if (!(next & 4)) {
            /* @bin 0x0047F39F the next tile is dry: this one is the far end */
            S32(st, G_BRIDGE_END) = off;
            if (edge_neighbours_with(st, off, 4) == 3 && edge_neighbours_with(st, off, 0x10000) == 0) {
                return 0;
            }
            S32(st, G_BRIDGE_END) = 0;
            return 0;
        }
        if (next & 0x48) {
            return 0;
        }
        if (corner_neighbours_with(st, off, 4) != 4) {
            return 0;
        }
        if (++i >= 4) {
            return 0;
        }
    }
}

/* @bin 0x0043D030 (d, x, y, screen x, screen y, reach) the rule part of the ferry route draw:
 * the tiles of a widening fan (rows 0 .. 2 * reach, half width up to 5) in direction d from
 * (x, y); found when one is another ferry landing not yet linked (+0xC4 = -1) or a free landing
 * that the ferry from (x, y) would join (0x00486A30). The calls of the scan are kept (their
 * state writes: 0x0047CC70 0x00BC96D8 / 0x00BC96DC, 0x00486A30 the road search and route
 * grids); the screen positions (0x004F0BA0) and the sprites are drawing. */
static int ferry_scan(game_state *st, int32_t d, int32_t x, int32_t y, int32_t reach)
{
    int found = 0;
    int32_t rows = reach * 2;
    if (rows < 0) {
        return 0;
    }
    int32_t opp = (d + 4) % 8; /* @bin 0x0043D0A4 signed remainder */
    int32_t c = 0;
    for (int32_t i = 0; i <= rows; i++) {
        for (int32_t j = -c; j <= c; j++) {
            int32_t dx, dy;
            /* @bin 0x0043D0E2 */
            if (d == 0) {
                dx = j;
                dy = -i;
            } else if (d == 2) {
                dx = i;
                dy = -j;
            } else if (d == 4) {
                dx = -j;
                dy = i;
            } else {
                dx = -i;
                dy = j;
            }
            int32_t tx = x + dx, ty = y + dy;
            int32_t fit = shore_fit(st, tx, ty, 1, 0); /* @bin 0x0043D14C */
            int32_t origin = S32(st, G_MAP_START) + y * 0xE4 + x;
            int32_t off = S32(st, G_MAP_START) + ty * 0xE4 + tx;
            int32_t b = S16(st, G_GRID_BUILDING + 2u * (uint32_t) off);
            /* @bin 0x0043D15E another landing, not linked */
            if (ferry_landing_at_offset(st, off) != 0 && S32(st, BUILDING_ADDR(b) + BLD_FERRY_PARTNER) == -1 && off != origin &&
                S16(st, G_GRID_BUILDING + 2u * (uint32_t) off) != S16(st, G_GRID_BUILDING + 2u * (uint32_t) origin)) {
                found = 1;
            }
            int32_t k = tile_bit4_pattern(st, tx, ty) * 2; /* @bin 0x0043D221 */
            if (k == opp && fit == 0) {
                /* @bin 0x0043D24E mode 0 and the length into a local of the draw: not kept */
                if (GAME_CALL(st, FN_FERRY_LANDINGS_JOINED, tx, ty, x, y, 2, k, opp, 0, 0) != 0) {
                    found = 1;
                }
            }
        }
        if (++c > 5) {
            c = 5;
        }
    }
    return found;
}

/* @bin 0x0043DD70 (unused, floodable) the rule part of the ferry ghost draw: the landing's shore
 * fit at the hovered tile (0x0047CC70: 0x00BC96D8 / 0x00BC96DC), and for a building whose
 * image is one of the water-lift group 0x00F56CCC .. + 0xB the flag 0x00BCA32C */
static void ferry_ghost(game_state *st, int32_t floodable)
{
    int32_t x = S32(st, G_HOVER_X), y = S32(st, G_HOVER_Y);
    uint32_t type = U32(st, G_CONSTRUCTION_UI_TYPE);
    int32_t img = S32(st, G_IMAGE_GROUPS + 8u * U32(st, G_MODELS + MODELS_IMAGE_GROUP + 20u * type)) + S32(st, G_MODELS + MODELS_IMAGE_OFFSET + 20u * type);
    int32_t fit = shore_fit(st, x, y, 1, floodable);
    if (S32(st, CURRENT_CITY(st) + CITY_TREASURY) <= -5000 || fit != 0) {
        return; /* @bin 0x0043DDEC / 0x0043DE3C the red ghost */
    }
    int32_t base = S32(st, G_IMAGE_GROUPS + 8u * 547u);
    if (img >= base && img <= base + 0xB) {
        U8(st, G_WATER_LIFT_FLOOD) = (uint8_t) map_area_test(st, x, y, 2, 2, 0x10000); /* @bin 0x0043DE6A */
        /* 0x00487D30 (the image) writes nothing */
    }
}

void ferry_hover(game_state *st)
{
    /* @bin 0x0043D300 the rule part of the ferry ghost draw: 0x00BCA314 := 0 (0x0043D321) */
    U8(st, G_FERRY_VALID) = 0;
    int32_t x = S32(st, G_HOVER_X), y = S32(st, G_HOVER_Y);
    if (S32(st, G_FERRY_STATE) == 0) {
        /* first landing */
        ferry_reach(st, x, y); /* @bin 0x0043D361 */
        int32_t d = S32(st, G_REACH_DIR);
        if (d < 0) {
            d += 8;
        }
        int32_t fit = shore_fit(st, x, y, 1, 0); /* @bin 0x0043D3A2 */
        int32_t hov = S32(st, G_HOVER_TILE);
        int32_t b = S16(st, G_GRID_BUILDING + 2u * (uint32_t) hov);
        int existing = 0;
        if (ferry_landing_at_offset(st, hov) != 0 && S32(st, BUILDING_ADDR(b) + BLD_FERRY_PARTNER) == -1) {
            /* @bin 0x0043D3F5 a landing not linked under the mouse: its routes drawn */
            existing = 1;
            ferry_scan(st, d, x, y, S32(st, G_REACH_LEN));
        }
        if (fit == 0 && ferry_scan(st, d, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), S32(st, G_REACH_LEN))) {
            U8(st, G_FERRY_VALID) = 1; /* @bin 0x0043D498 */
            ferry_ghost(st, 0);
            return;
        }
        (void) existing; /* @bin 0x0043D4BB either way 0x00BCA314 stays 0 */
        return;
    }
    /* second landing: joined to the first one 0x00BC9800 */
    int32_t k = tile_bit4_pattern(st, x, y); /* @bin 0x0043D537 */
    uint32_t first = BUILDING_ADDR(S32(st, G_FERRY_BUILDING));
    int32_t size = U8(st, first + BLD_SIZE);
    int32_t rot = (int32_t) U8(st, first + BLD_ORIENTATION) * 2;
    int32_t fx = S16(st, first + BLD_X), fy = S16(st, first + BLD_Y);
    int32_t hov = S32(st, G_HOVER_TILE);
    if (ferry_landing_at_offset(st, hov) != 0) {
        int32_t b = S16(st, G_GRID_BUILDING + 2u * (uint32_t) S32(st, G_HOVER_TILE));
        if (b != S32(st, G_FERRY_BUILDING) && S32(st, BUILDING_ADDR(b) + BLD_FERRY_PARTNER) == -1) {
            return; /* @bin 0x0043D5BF another landing under the mouse: drawn, not valid */
        }
    }
    ferry_scan(st, rot, fx, fy, S32(st, G_REACH_LEN)); /* @bin 0x0043D65E the first landing's routes */
    int32_t opp = (rot + 4) % 8;
    /* @bin 0x0043D696 the route buffer and the length are locals of the draw (mode != 1) */
    if (GAME_CALL(st, FN_FERRY_LANDINGS_JOINED, x, y, fx, fy, size, k * 2, opp, 0, 0) == 1) {
        U8(st, G_FERRY_VALID) = 1; /* @bin 0x0043D6A7 */
        ferry_ghost(st, 0);
    }
}

static int32_t run_reach(game_state *st, const int32_t *a) { return ferry_reach(st, a[0], a[1]); }
static int32_t run_bridge(game_state *st, const int32_t *a) { return bridge_measure(st, a[0], a[1]); }
static int32_t run_edge(game_state *st, const int32_t *a) { return edge_neighbours_with(st, a[0], (uint32_t) a[1]); }
static int32_t run_corner(game_state *st, const int32_t *a) { return corner_neighbours_with(st, a[0], (uint32_t) a[1]); }

const ported_function hover_ports[] = {
    {0x0047F3E0u, "ferry_reach", 3, 32, 0, run_reach, 0},
    {0x0047F200u, "bridge_measure", 3, 0, 0, run_bridge, 0}, /* the third argument is not read */
    {0x0047EC80u, "edge_neighbours_with", 2, 32, 0, run_edge, 0},
    {0x0047ECC0u, "corner_neighbours_with", 2, 32, 0, run_corner, 0},
    /* 0x0043D300 itself (the ferry ghost, drawing included) is present/city/ghost_ferry.c's; ferry_hover
     * above is its rule part, for callers that need the landing validity without drawing */
};
const int hover_ports_count = sizeof(hover_ports) / sizeof(hover_ports[0]);
