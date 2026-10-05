/* Imhotep - per-tile routing grids (Pharaoh.exe 0x00517370 .. 0x00517A20 and their fillers).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Each grid holds one byte per tile (228 x 228), rebuilt from the terrain flags, the building
 * grid and the buildings; the walkers' route search reads them. The meaning of each value is not
 * established yet: names below are the grids' addresses (docs/formats/state.md). */
#include "map/routing.h"

#include "sim/call.h"

#include <string.h>

#define SIDE 0xE4
#define GRID (SIDE * SIDE)
#define MAP_WIDTH 0x00BCA2D0u
#define MAP_HEIGHT 0x00BCA2D4u
#define MAP_START 0x00BCA2D8u
#define MAP_ROW_SKIP 0x00BCA2DCu
#define VIEW_ORIENTATION 0x00E94E04u /* 0, 2, 4, 6 */
#define TERRAIN 0x00AF598Cu          /* u32 per tile, chunk 10 */
#define BUILDING_GRID 0x00B4E6FCu    /* i16 per tile, chunk 9 */
#define BUILDINGS 0x007999D8u
#define BUILDING_SIZE 0x108u

static uint32_t T(game_state *st, int32_t o)
{
    return U32(st, TERRAIN + 4u * (uint32_t) o);
}

static int16_t building_type(game_state *st, int32_t id)
{
    return S16(st, BUILDINGS + BUILDING_SIZE * (uint32_t) id + 0x10);
}

static void fill(game_state *st, uint32_t grid)
{
    memset(state_ptr(st, grid, GRID), 0xFF, GRID); /* @bin 0x0040F8F0(grid, 0xCB10, -1) */
}

/* the rectangle clamp shared by the fillers */
static void clamp(game_state *st, int32_t *x0, int32_t *y0, int32_t *x1, int32_t *y1)
{
    if (*x0 < 0) {
        *x0 = 0;
    }
    if (*y0 < 0) {
        *y0 = 0;
    }
    if (S32(st, MAP_WIDTH) <= *x1) {
        *x1 = S32(st, MAP_WIDTH) - 1;
    }
    if (S32(st, MAP_HEIGHT) <= *y1) {
        *y1 = S32(st, MAP_HEIGHT) - 1;
    }
}

void routing_clear_ee0c50(game_state *st)
{
    fill(st, ROUTING_EE0C50); /* @bin 0x00517A20 0x32C4 dwords := -1 */
}

void routing_fill_ed4140(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    /* @bin 0x00517690 */
    clamp(st, &x0, &y0, &x1, &y1);
    int32_t w = S32(st, MAP_WIDTH), h = S32(st, MAP_HEIGHT);
    for (int32_t y = y0; y <= y1; y++) {
        for (int32_t x = x0; x <= x1; x++) {
            int32_t o = S32(st, MAP_START) + y * SIDE + x;
            uint32_t t = T(st, o);
            uint8_t g;
            if (!(t & 4)) {
                g = 0xFC;
            } else if (t & 0x80000) {
                g = 0xFF;
            } else if (!(T(st, o - 228) & 4) || !(T(st, o - 227) & 4) || !(T(st, o + 1) & 4) ||
                       !(T(st, o + 229) & 4) || !(T(st, o + 228) & 4) || !(T(st, o + 227) & 4) ||
                       !(T(st, o - 1) & 4) || !(T(st, o - 229) & 4)) {
                g = 0xFF;
            } else if (y < 1 || x < 1 || h - 2 < y || w - 2 < x) {
                g = 0xFE;
            } else {
                uint8_t c = U8(st, 0x00ADC36Cu + (uint32_t) o); /* chunk 14 */
                g = c == 13 ? 0xFF : (c == 5 || c == 2) ? 0xFD : 0;
            }
            U8(st, ROUTING_ED4140 + (uint32_t) o) = g;
        }
    }
}

void routing_rebuild_ed4140(game_state *st)
{
    /* @bin 0x005178B0 */
    fill(st, ROUTING_ED4140);
    routing_fill_ed4140(st, 0, 0, S32(st, MAP_WIDTH) - 1, S32(st, MAP_HEIGHT) - 1);
}

void routing_rebuild_ebab20(game_state *st)
{
    /* @bin 0x005178E0 depends on the view orientation */
    fill(st, ROUTING_EBAB20);
    int32_t view = S32(st, VIEW_ORIENTATION);
    int32_t o = S32(st, MAP_START);
    for (int32_t y = 0; y < S32(st, MAP_HEIGHT); y++) {
        for (int32_t x = 0; x < S32(st, MAP_WIDTH); x++, o++) {
            uint32_t t = T(st, o);
            uint8_t g = 0xFF;
            if (!(t & 0x4000)) {
                if (t & 0x8000) {
                    g = 0;
                }
            } else {
                int c;
                uint32_t u;
                int known = 1;
                if (view == 0) {
                    c = (T(st, o + 228) & 0xC000) != 0;
                    c += (T(st, o + 229) & 0xC000) != 0;
                    u = T(st, o + 1);
                } else if (view == 2) {
                    c = (T(st, o + 228) & 0xC000) != 0;
                    c += (T(st, o + 227) & 0xC000) != 0;
                    u = T(st, o - 1);
                } else if (view == 4) {
                    c = (T(st, o - 228) & 0xC000) != 0;
                    c += (T(st, o - 229) & 0xC000) != 0;
                    u = T(st, o - 1);
                } else if (view == 6) {
                    c = (T(st, o - 228) & 0xC000) != 0;
                    c += (T(st, o - 227) & 0xC000) != 0;
                    u = T(st, o + 1);
                } else {
                    known = 0;
                    c = 0;
                    u = 0;
                }
                if (known) {
                    c += (u & 0xC000) != 0;
                    g = c == 3 ? 0 : 0xFF;
                }
            }
            U8(st, ROUTING_EBAB20 + (uint32_t) o) = g;
        }
        o += S32(st, MAP_ROW_SKIP);
    }
}

static int blocks_ec7630(int16_t type)
{
    switch (type) {
    case 99: case 0x58: case 0x59: case 0x5D: case 0xB7: case 0xD2: case 0xCF: case 0xD0: case 0xDE:
    case 0xD5: case 0xE1: case 0xE6: case 0xE3: case 0xE5: case 0xEA: case 0xEB: case 0xEC: case 0xE4:
    case 0xD7: case 0x88:
        return 1;
    default:
        return 0;
    }
}

void routing_fill_ec7630(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    /* @bin 0x005173A0 */
    clamp(st, &x0, &y0, &x1, &y1);
    for (int32_t y = y0; y <= y1; y++) {
        for (int32_t x = x0; x <= x1; x++) {
            int32_t o = S32(st, MAP_START) + y * SIDE + x;
            uint32_t t = T(st, o);
            uint8_t g;
            if ((t & 0x80000) || (t & 0x10004) == 0x10004) {
                g = 0xFF;
            } else if (t & 0x8000) {
                g = 4;
            } else if ((t & 0x40) || (t & 0x40000000) || (t & 0x20)) {
                g = 0;
            } else if (t & 0x400) {
                g = 2;
            } else if (t & 8) {
                int16_t type = building_type(st, S16(st, BUILDING_GRID + 2u * (uint32_t) o));
                g = type == 0x48 ? 1 : type == 0xCA ? 4 : blocks_ec7630(type) ? 0xFF : type == 0x39 ? 5 : type == 0x36 ? 0 : 1;
            } else if (t & 0x100) {
                g = 0;
            } else if (t & 0x4000) {
                g = 3;
            } else if ((t & 0x2040011) || (t & 0x1000)) {
                g = 0;
            } else if (t & 0xEEFED77Fu) {
                g = 0xFF;
            } else {
                g = 0;
            }
            U8(st, ROUTING_EC7630 + (uint32_t) o) = g;
        }
    }
}

void routing_rebuild_ec7630(game_state *st)
{
    /* @bin 0x00517660 */
    fill(st, ROUTING_EC7630);
    routing_fill_ec7630(st, 0, 0, S32(st, MAP_WIDTH) - 1, S32(st, MAP_HEIGHT) - 1);
}

void routing_fill_ee0c50(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int flag)
{
    /* @bin 0x00516F90 */
    clamp(st, &x0, &y0, &x1, &y1);
    for (int32_t y = y0; y <= y1; y++) {
        for (int32_t x = x0; x <= x1; x++) {
            int32_t o = S32(st, MAP_START) + y * SIDE + x;
            uint32_t uo = (uint32_t) o;
            if (flag && (T(st, o) & 0x10044) == 0x44 && U8(st, 0x00ADC36Cu + uo) == 0) {
                U32(st, TERRAIN + 4 * uo) &= ~0x40u;
            }
            int32_t base = S32(st, 0x00F56B8Cu); /* an image group's first id (startup numbering) */
            uint32_t t = T(st, o);
            uint8_t *g = &U8(st, ROUTING_EE0C50 + uo);
            if ((t & 0x80000) || (t & 0x10004) == 0x10004) {
                *g = 0xFF;
            } else if ((t & 0x40) || (t & 0x40000000)) {
                *g = 0;
            } else if (t & 0x400) {
                *g = 2;
            } else if (t & 0x10000000) {
                *g = 5;
            } else if (t & 0x8008) {
                int16_t b = S16(st, BUILDING_GRID + 2 * uo);
                if (b == 0) {
                    /* a building flag without a building: cleared, the tile becomes plain land */
                    U32(st, TERRAIN + 4 * uo) = t & ~8u;
                    U8(st, ROUTING_EC7630 + uo) = 4;
                    S32(st, 0x00B7482Cu + 4 * uo) = (U8(st, 0x00ACF85Cu + uo) & 7) + base;
                    U8(st, 0x00B67D1Cu + uo) = 0x40;
                    U8(st, 0x00B285CCu + uo) &= 0xF0;
                    S32(st, 0x00F535BCu) += 1;
                } else {
                    int16_t type = building_type(st, b);
                    if (type == 0x8A || type == 0xD7 || type == 0x88) {
                        *g = 0;
                    } else if (type == 0x36) {
                        *g = 6;
                    } else if (type == 0xD1) {
                        *g = 4;
                    } else if (type == 0x5A) {
                        uint32_t orient = U8(st, BUILDINGS + BUILDING_SIZE * (uint32_t) b + 0xAA);
                        *g = ((orient - (uint32_t) (S32(st, 0x00BCA2E0u) / 2)) & 1) == 0 ? 0xFA : 0xFC;
                    } else {
                        *g = 0xFF;
                    }
                }
            } else if (t & 0x100) {
                *g = 4;
            } else {
                uint32_t obj = U32(st, 0x00D05720u + 4 * uo); /* floodplain object of the tile */
                const uint8_t *flooded = obj ? state_ptr(st, obj + 0x18, 1) : NULL;
                if (flooded && *flooded == 0) {
                    int32_t d = S32(st, 0x00B7482Cu + 4 * uo) - S32(st, 0x00F56C34u);
                    *g = d < 0x30 ? 4 : (d == 0x34 || d == 0x30 || d == 0x36 || d == 0x32) ? 0xFB : 0xFF;
                } else if ((t & 0x10000) || (t & 0x2040011)) {
                    *g = 4;
                } else if (t & 0x1000) {
                    *g = 2;
                } else if (t & 0xEEFFD77Fu) {
                    *g = 0xFF;
                } else {
                    *g = 4;
                }
            }
        }
    }
    if (!flag) {
        return;
    }
    /* @bin 0x005172F8 .. 0x00517358 then every type-0x88 building (state 1 or 3), ids 0..3999: with
     * no link (+0x40 == 0) or a linked building (+0xC4) whose +0x40 is <= 0, route search
     * 0x0043F060(id, 1, 3) and 0x004A36B0(id); else 0x0043F060(id, 1, 1) */
    for (int32_t id = 0; id < 4000; id++) {
        uint32_t p = BUILDINGS + BUILDING_SIZE * (uint32_t) id;
        if (S16(st, p + 0x10) != 0x88 || (U8(st, p) != 1 && U8(st, p) != 3)) {
            continue;
        }
        if (S16(st, p + 0x40) == 0 || S16(st, BUILDINGS + BUILDING_SIZE * U32(st, p + 0xC4) + 0x40) < 1) {
            GAME_CALL(st, 0x0043F060u, id, 1, 3);
            GAME_CALL(st, 0x004A36B0u, id);
        } else {
            GAME_CALL(st, 0x0043F060u, id, 1, 1);
        }
    }
}

void routing_rebuild_ee0c50(game_state *st)
{
    /* @bin 0x00517370 */
    fill(st, ROUTING_EE0C50);
    routing_fill_ee0c50(st, 0, 0, S32(st, MAP_WIDTH) - 1, S32(st, MAP_HEIGHT) - 1, 1);
}
