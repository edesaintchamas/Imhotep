/* Imhotep - the water sub-pass of the terrain images over any rectangle (package bigb2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x00475CC0 (x0, y0, x1, y1, reveal) redraws every water tile of a rectangle with 0x0047A220,
 * which redraws the water tiles of the 3x3 square around it: shore patterns, floodplain banks,
 * irrigation ditches into the floodplain, animated water (terrain 0x4000000), for any view
 * orientation (0x00BCA2E0) and with the 'blood river' image groups while the map reveal of
 * 0x0078D7B0 has not reached the tile's column. core/map/terrain.c has the whole-map version for
 * a new scenario (orientation 0, no reveal); this one is general. */
#include "map/terrain_water.h"

#include "map/terrain_land.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/pre_load.h"

#define GRID 228

#define TER(o) U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (o))
#define IMG(o) S32(st, G_GRID_IMAGE + 4u * (uint32_t) (o))
#define EDGE(o) U8(st, G_GRID_EDGE + (uint32_t) (o))
#define BITS(o) U8(st, G_GRID_BITFIELDS + (uint32_t) (o))
#define RND(o) U8(st, G_GRID_RANDOM + (uint32_t) (o))
#define FP_OBJECT(o) U32(st, G_FLOODPLAIN_MANAGER + 4u * (uint32_t) (o))
#define SCR(i) S32(st, G_NEIGHBOUR_FLAGS + 4u * (uint32_t) ((i) & 7))
#define GROUP(g) S32(st, G_IMAGE_GROUPS + 8u * (uint32_t) (g)) /* first image of group g */

/* @bin 0x0047DB00 neighbour order of the scratch */
static const int32_t NEIGHBOURS[8] = {-228, -227, 1, 229, 228, 227, -1, -229};

static void neighbour_flags_except(game_state *st, uint32_t mask, uint32_t except)
{
    /* @bin 0x0047DC80 (mask, except) scratch[i] = terrain & mask and not terrain & except */
    int32_t c = S32(st, G_TILE_CURSOR);
    for (int i = 0; i < 8; i++) {
        uint32_t v = TER(c + NEIGHBOURS[i]);
        S32(st, G_NEIGHBOUR_FLAGS + 4u * (uint32_t) i) = (v & mask) != 0 && (v & except) == 0;
    }
}

static void scratch_invert(game_state *st)
{
    /* @bin 0x00484F00 */
    for (int i = 0; i < 8; i++) {
        S32(st, G_NEIGHBOUR_FLAGS + 4u * (uint32_t) i) = S32(st, G_NEIGHBOUR_FLAGS + 4u * (uint32_t) i) == 0;
    }
}

static int32_t scratch_count(game_state *st)
{
    /* @bin 0x00484F40 */
    int32_t n = 0;
    for (int i = 0; i < 8; i++) {
        n += S32(st, G_NEIGHBOUR_FLAGS + 4u * (uint32_t) i) != 0;
    }
    return n;
}

static void pattern_counters_reset(game_state *st, uint32_t records, int32_t count)
{
    /* @bin 0x00484EE0 (records, count) byte 14 of each record := 0 */
    for (int32_t i = 0; i < count; i++) {
        U8(st, records + 16u * (uint32_t) i + PATTERN_COUNTER) = 0;
    }
}

int32_t water_reveal_at(game_state *st, int32_t x)
{
    /* @bin 0x0047A1F0 (x, y) the map reveal (0x0078D7B0 counting down from 0x0078D7AC) is at
     * column 0xE4 - x (al) */
    int32_t left = S32(st, G_BLOOD_RIVER_LEFT);
    if (left <= 0) {
        return 0;
    }
    return (S32(st, G_BLOOD_RIVER_DURATION) - left) / 2 == 0xE4 - x;
}

/* @bin 0x0047A4C9..0x0047A666 a water tile next to floodplain: the floodplain bank image by the
 * shore offset and the floodplain objects of the neighbours (adj: object with +0x18 == 0); -1 when
 * none applies. n: floodplain neighbours (scratch, mask 0x10000). */
static int32_t bank_image(game_state *st, int32_t off, const uint8_t adj[8], int32_t n, int blood)
{
    int32_t orient = S32(st, G_VIEW_ORIENTATION), dir = -1, side = (n < 2 ? n : 2) * 2;
    int corner = 0;
    switch (off) {
    case 8: dir = adj[(orient - 4) & 7] ? 4 : -1; break;
    case 0xC: dir = adj[(orient - 2) & 7] ? 6 : -1; break;
    case 0x10: dir = adj[orient & 7] ? 0 : -1; break;
    case 0x14: dir = adj[(orient + 2) & 7] ? 2 : -1; break;
    case 0x18: dir = adj[(orient - 2) & 7] ? 6 : adj[(orient - 4) & 7] ? 4 : -1; corner = 1; break;
    case 0x1C: dir = adj[(orient - 2) & 7] ? 6 : adj[orient & 7] ? 0 : -1; corner = 1; break;
    case 0x20: dir = adj[(orient + 2) & 7] ? 2 : adj[orient & 7] ? 0 : -1; corner = 1; break;
    case 0x24: dir = adj[(orient + 2) & 7] ? 2 : adj[(orient - 4) & 7] ? 4 : -1; corner = 1; break;
    default: return -1;
    }
    if (dir < 0) {
        return -1;
    }
    if (!corner) {
        side = 0; /* @bin 0x0047A507 */
    } else if (side == 2 && (SCR(6) | SCR(4) | SCR(2) | SCR(0))) {
        side = 4; /* @bin 0x0047A5E2 */
    }
    int32_t base = GROUP(528); /* @bin 0x0047A60B group 528 */
    int32_t img = base + (((dir - 4) & 7) + 0x14) * 3 + side;
    if (SCR(orient + dir + 1) != 0 || SCR(orient + dir + 2) != 0) {
        img++;
    }
    if (blood) {
        int32_t e = img - base;
        if (e >= 0x3C && e <= 0x53) {
            img = img + GROUP(720) - base - 0x3C; /* @bin 0x0047A64C */
        }
    }
    return img;
}

/* @bin 0x0047A692..0x0047A8D2 a shore tile next to floodplain without a bank: an irrigation
 * ditch image when a ditch (0x100 without water) is orthogonally next to it; else `img` */
static int32_t ditch_image(game_state *st, int32_t o, int32_t img, int blood)
{
    S32(st, G_TILE_CURSOR) = o;
    neighbour_flags_except(st, 0x100, 4);
    S32(st, G_NEIGHBOUR_FLAGS + 0x1C) = 0;
    S32(st, G_NEIGHBOUR_FLAGS + 0x14) = 0;
    S32(st, G_NEIGHBOUR_FLAGS + 0x0C) = 0;
    S32(st, G_NEIGHBOUR_FLAGS + 0x04) = 0;
    if (scratch_count(st) == 0) {
        return img;
    }
    int32_t off = S32(st, G_MATCH_OFFSET), orient = S32(st, G_VIEW_ORIENTATION), k = -1;
    switch (off) {
    case 8: k = SCR(orient - 4) ? 3 : -2; break;
    case 0xC: k = SCR(orient - 2) ? 0 : -2; break;
    case 0x10: k = SCR(orient) ? 1 : -2; break;
    case 0x14: k = SCR(orient + 2) ? 2 : -2; break;
    default: {
        /* @bin 0x0047A77A */
        int32_t w = SCR(orient - 2);
        if (w != 0 && SCR(orient - 4) != 0) {
            k = 7;
        } else if (w != 0 && SCR(orient) != 0) {
            k = 4;
        } else {
            int32_t e = SCR(orient + 2);
            if (e != 0) {
                if (SCR(orient) != 0) {
                    k = 5;
                } else if (SCR(orient - 4) != 0) {
                    k = 6;
                }
            }
        }
        if (off == 0x18) {
            k = SCR(orient - 4) ? 0xB : w ? 8 : k;
        } else if (off == 0x1C) {
            k = w ? 0xD : SCR(orient) ? 0xA : k;
        } else if (off == 0x20) {
            k = SCR(orient + 2) ? 0xC : SCR(orient) ? 0xF : k;
        } else if (off == 0x24) {
            k = SCR(orient + 2) ? 9 : SCR(orient - 4) ? 0xE : k;
        }
        break;
    }
    }
    if (k < 0) {
        return img;
    }
    return (blood ? GROUP(726) : GROUP(541)) + k;
}

void water_tile_redraw(game_state *st, int32_t x, int32_t y, int32_t flag)
{
    /* @bin 0x0047A220 (x, y, flag) */
    if (!map_tile_inside(st, S32(st, G_VIEW_DX) + x, S32(st, G_VIEW_DY) + y)) {
        return;
    }
    int32_t c = S32(st, G_MAP_START) + y * GRID + x;
    TER(c) = flag == 1 ? TER(c) | 4 : TER(c) & 0xFBFFFFFBu;
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
    int blood = (uint8_t) GAME_CALL(st, FN_FIGURE_NEAR_MAP_EDGE, xa, ya) != 0;
    int32_t saved = S32(st, G_TILE_CURSOR);
    int32_t o = S32(st, G_MAP_START) + ya * GRID + xa;
    for (int32_t yy = ya; yy < ya + h; yy++) {
        for (int32_t xx = xa; xx < xa + w; xx++, o++) {
            uint32_t v = TER(o);
            if (!(v & 4) || (v & 8)) {
                continue;
            }
            S32(st, G_TILE_CURSOR) = o;
            land_neighbour_flags(st, 4);
            scratch_invert(st);
            land_pattern_match(st, G_PATTERN_SHORE, 0x30, RND(o), 0);
            int32_t off = S32(st, G_MATCH_OFFSET), img = -1;
            if (TER(o) & 0x10000) {
                if (off >= 8) {
                    img = (blood ? GROUP(719) : GROUP(516)) + S32(st, G_MATCH_VARIANT) + off;
                }
            } else {
                S32(st, G_TILE_CURSOR) = o;
                land_neighbour_flags(st, 0x10000);
                int32_t n = scratch_count(st);
                if (n != 0) {
                    uint8_t adj[8];
                    for (int i = 0; i < 8; i++) {
                        uint32_t obj = FP_OBJECT(S32(st, G_TILE_CURSOR) + NEIGHBOURS[i]);
                        adj[i] = obj != 0 && P8(st, obj + FLOODOBJ_FLAG) == 0;
                    }
                    img = bank_image(st, off, adj, n, blood);
                    if (img < 0) {
                        img = (blood ? GROUP(719) : GROUP(516)) + S32(st, G_MATCH_VARIANT) + off;
                        img = ditch_image(st, o, img, blood);
                    }
                }
            }
            if (img < 0) {
                /* @bin 0x0047A8E6 */
                if (TER(o) & 0x4000000u) {
                    pattern_counters_reset(st, G_PATTERN_DEEP, 0x1A);
                    S32(st, G_TILE_CURSOR) = o;
                    land_neighbour_flags(st, 0x4000000u);
                    scratch_invert(st);
                    land_pattern_match(st, G_PATTERN_DEEP, 0x1A, RND(S32(st, G_TILE_CURSOR)), 1);
                    if (blood) {
                        img = GROUP(718) + S32(st, G_MATCH_VARIANT) * 15 + S32(st, G_MATCH_OFFSET);
                    } else if ((uint8_t) water_reveal_at(st, xa)) {
                        int32_t r = RND(S32(st, G_TILE_CURSOR)) % 4;
                        img = GROUP(750) + r + (S32(st, G_VIEW_ORIENTATION) / 2) * 4;
                    } else {
                        img = GROUP(558) + S32(st, G_MATCH_VARIANT) * 15 + S32(st, G_MATCH_OFFSET);
                    }
                } else if (off == 0) {
                    img = (blood ? GROUP(719) : GROUP(516)) + S32(st, G_MATCH_VARIANT);
                } else {
                    img = (blood ? GROUP(717) : GROUP(502)) + S32(st, G_MATCH_VARIANT) + off;
                }
            }
            BITS(o) &= 0xF0;
            EDGE(o) |= 0x40;
            IMG(o) = img;
        }
        o += GRID - w;
    }
    S32(st, G_TILE_CURSOR) = saved;
}

void water_pass(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t reveal)
{
    /* @bin 0x00475CC0 with reveal: 0x004877F0 over the rectangle grown by 5 first; clamped to the
     * map; every water tile (4 without 8): 0x0047A220 (x, y, 1) */
    if ((uint8_t) reveal) {
        GAME_CALL(st, FN_COAST_MARKS, x0 - 5, y0 - 5, x1 + 5, y1 + 5);
    }
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
    for (int32_t y = y0; y <= y1; y++) {
        for (int32_t x = x0; x <= x1; x++) {
            uint32_t v = TER(S32(st, G_MAP_START) + y * GRID + x);
            if ((v & 4) && !(v & 8)) {
                water_tile_redraw(st, x, y, 1);
            }
        }
    }
}

/* ---- registration -------------------------------------------------------------------------- */

static int32_t run_water_pass(game_state *st, const int32_t *a)
{
    water_pass(st, a[0], a[1], a[2], a[3], a[4]);
    return 0;
}

static int32_t run_water_tile(game_state *st, const int32_t *a)
{
    water_tile_redraw(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_reveal_at(game_state *st, const int32_t *a)
{
    return water_reveal_at(st, a[0]);
}

static int32_t run_flags_except(game_state *st, const int32_t *a)
{
    neighbour_flags_except(st, (uint32_t) a[0], (uint32_t) a[1]);
    return 0;
}

static int32_t run_invert(game_state *st, const int32_t *a)
{
    (void) a;
    scratch_invert(st);
    return 0;
}

const ported_function bigb2_water_ports[] = {
    {0x00475CC0u, "water_pass", 5, 0, 0, run_water_pass, 0},
    {0x0047A220u, "water_tile_redraw", 3, 0, 0, run_water_tile, 0},
    {0x0047A1F0u, "water_reveal_at", 2, 8, 0, run_reveal_at, 0},
    {0x0047DC80u, "neighbour_flags_except", 2, 0, 0, run_flags_except, 0},
    {0x00484F00u, "scratch_invert", 0, 0, 0, run_invert, 0},
};
const int bigb2_water_ports_count = sizeof(bigb2_water_ports) / sizeof(bigb2_water_ports[0]);
