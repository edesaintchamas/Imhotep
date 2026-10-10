/* Imhotep - small helpers of the daily passes (package day2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/day2_misc.h"

#include "city/day_buildings.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

/* @bin 0x00481AA0 the (dx, dy) byte pairs of each ring by footprint size: sizes 1..5 use rings
 * 1..5 for `ring` 1..5 and ring 6 otherwise; any other size reads the 6x6 rings with `ring` 0..4
 * for rings 1..5 and ring 6 otherwise (desirability.c relies on this shift). The 1x1 rings are
 * built at start-up by 0x0046F8D0 in .data, the others are static data */
static const uint32_t ring_base[6][6] = {
    {G_RINGS_1X1_RING6 + 0x140, G_RINGS_1X1_RING6 + 0x120, G_RINGS_1X1_RING6 + 0xF0, G_RINGS_1X1_RING6 + 0xB0, G_RINGS_1X1_RING6 + 0x60, G_RINGS_1X1_RING6},
    {G_RINGS, G_RINGS + 0x18, G_RINGS + 0x40, G_RINGS + 0x78, G_RINGS + 0xC0, G_RINGS + 0x118},
    {G_RINGS + 0x180, G_RINGS + 0x1A0, G_RINGS + 0x1D0, G_RINGS + 0x210, G_RINGS + 0x260, G_RINGS + 0x2C0},
    {G_RINGS + 0x330, G_RINGS + 0x358, G_RINGS + 0x390, G_RINGS + 0x3D8, G_RINGS + 0x430, G_RINGS + 0x498},
    {G_RINGS + 0x510, G_RINGS + 0x540, G_RINGS + 0x580, G_RINGS + 0x5D0, G_RINGS + 0x630, G_RINGS + 0x6A0},
    {G_RINGS + 0x720, G_RINGS + 0x758, G_RINGS + 0x7A0, G_RINGS + 0x7F8, G_RINGS + 0x860, G_RINGS + 0x8D8},
};

int32_t ring_tile_outside(game_state *st, int32_t x, int32_t y, int32_t size, int32_t ring, int32_t i)
{
    int row, col;
    if (size >= 1 && size <= 5) {
        row = size - 1;
        col = ring >= 1 && ring <= 5 ? ring - 1 : 5;
    } else {
        row = 5;
        col = ring >= 0 && ring <= 4 ? ring : 5;
    }
    uint32_t a = ring_base[row][col] + 2u * (uint32_t) i;
    int32_t ex = x + S8(st, a), ey = y + S8(st, a + 1);
    /* @bin 0x00481ED2 signed compares */
    if (ex < -1 || ey < -1) {
        return 1;
    }
    if (ex > S32(st, G_MAP_WIDTH)) {
        return 1;
    }
    return ey > S32(st, G_MAP_HEIGHT);
}

int32_t tile_bit4_pattern(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x0047CBD0 by the view orientation (0x00BCA2E0) the 2x2 block's top-left moves; then
     * the first of the 4 patterns (0x005DBDA8, 4 dwords each) that the terrain bit 4 of the four
     * tiles (offsets 0x005CCCC8) matches, else 999 */
    int32_t o = S32(st, G_VIEW_ORIENTATION);
    if (o == 6) {
        y--;
    } else if (o == 4) {
        x--;
        y--;
    } else if (o == 2) {
        x--;
    }
    int32_t base = S32(st, G_MAP_START) + y * 0xE4 + x;
    for (int32_t k = 0; k < 4; k++) {
        int n = 0;
        for (uint32_t j = 0; j < 4; j++) {
            int32_t t = base + S32(st, G_U5CCCC8 + 4u * j);
            int set = (U8(st, G_GRID_TERRAIN + 4u * (uint32_t) t) & 4) != 0;
            if ((S32(st, G_U5DBDA8 + 16u * (uint32_t) k + 4u * j) != 0) == set) {
                n++;
            }
        }
        if (n == 4) {
            return k;
        }
    }
    return 999;
}

int32_t direction_between(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    /* @bin 0x0051CC00 the 16-step direction from (x1, y1) to (x2, y2) (0: none), by the slope in
     * percent (0x004CDE60): thresholds 500, 200, -200, -500; only al is returned */
    int32_t dx = x1 > x2 ? x1 - x2 : x2 - x1;
    int32_t dy = y1 > y2 ? y1 - y2 : y2 - y1;
    int32_t r;
    if (dx > dy) {
        r = calc_ratio_percent(dx, dy);
    } else if (dx < dy) {
        r = -calc_ratio_percent(dy, dx);
    } else {
        r = 100;
    }
    if (x1 == x2) {
        if (y1 > y2) {
            return 0;
        }
        if (y1 < y2) {
            return 8;
        }
    }
    if (x1 > x2) {
        if (y1 == y2) {
            return 0xC;
        }
        if (y1 > y2) {
            return r >= 500 ? 0xC : r >= 200 ? 0xD : r > -200 ? 0xE : r <= -500 ? 0 : 0xF;
        }
        return r >= 500 ? 0xC : r >= 200 ? 0xB : r > -200 ? 0xA : r > -500 ? 9 : 8;
    }
    if (x1 >= x2) {
        return 0;
    }
    if (y1 == y2) {
        return 4;
    }
    if (y1 > y2) {
        return r >= 500 ? 4 : r >= 200 ? 3 : r > -200 ? 2 : r > -500 ? 1 : 0;
    }
    return r >= 500 ? 4 : r >= 200 ? 5 : r > -200 ? 6 : r <= -500 ? 8 : 7;
}

void screen_buffer_clear(game_state *st)
{
    /* @bin 0x004CF1C0 the 16-bit screen buffer (pointer 0x00E92E18, width 0x00E38DAC x height
     * 0x00E38DB0) is cleared with 0x0040F8F0; display memory, outside the state when allocated
     * (0 in the oracle: the headless game has no screen) */
    uint32_t p = U32(st, G_BACK_BUFFER);
    if (p != 0) {
        uint32_t n = (uint32_t) S32(st, G_SCREEN_H) * (uint32_t) S32(st, G_SCREEN_W) * 2u;
        uint8_t *m = state_ptr(st, p, n);
        if (m != NULL) {
            for (uint32_t i = 0; i < n; i++) {
                m[i] = 0;
            }
        }
    }
}

/* the floodplain tile object at tile t (0x00D05720 + 4 t) is missing or still dry (+0x18 == 0
 * means flooded-able and not yet reached) */
static int flood_object_blocks(game_state *st, int32_t t, uint8_t floodable)
{
    if (floodable == 0) {
        return 1;
    }
    uint32_t obj = U32(st, G_UD05720 + 4u * (uint32_t) t);
    return obj == 0 || P8(st, obj + 0x18) != 0;
}

int32_t shore_fit(game_state *st, int32_t x, int32_t y, int32_t size, int32_t floodable)
{
    /* @bin 0x0047CC70 a 2x2 shore building (dock, wharf) at (x, y): the first of the 4 water
     * patterns (0x005DBDA8, offsets 0x005CCCC8) the tiles match, with the three tiles in front of
     * it (0x005DBDE8, 0x005DBE08, 0x005DBE28, 2 per pattern) in water (terrain bit 4, or bit
     * 0x10000 when `floodable`: a floodplain tile not yet flooded, object +0x18 == 0);
     * 0x00BC96D8 the pattern, 0x00BC96DC the building's orientation; returns the number of tiles
     * with terrain bits counted (0x40, 1, ...), 999 when none fits */
    uint8_t fl = (uint8_t) floodable;
    if (size == 1) {
        int32_t o = S32(st, G_VIEW_ORIENTATION);
        if (o == 6) {
            y--;
        } else if (o == 4) {
            x--;
            y--;
        } else if (o == 2) {
            x--;
        }
    }
    if (x < 0 || y < 0 || x + 1 >= S32(st, G_MAP_WIDTH) || y + 1 >= S32(st, G_MAP_HEIGHT)) {
        return 999;
    }
    int32_t state = 0; /* @bin 0x0047CCF0 the argument reused: 1 a dry floodplain tile, 2 water */
    S32(st, G_BUILD_TURN_VIEW) = 0;
    int32_t base = S32(st, G_MAP_START) + y * 0xE4 + x;
    for (int32_t k = 0;;) {
        int32_t count = 0, match = 0;
        for (uint32_t j = 0; j < 4; j++) {
            int32_t c = S32(st, G_U5CCCC8 + 4u * j) + base;
            uint32_t d = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) c);
            if (S32(st, G_U5DBDA8 + 16u * (uint32_t) k + 4u * j) != 0) {
                uint32_t obj = U32(st, G_UD05720 + 4u * (uint32_t) c);
                int dry = 0;
                if (obj != 0 && P8(st, obj + 0x18) == 0 && state != 2) {
                    state = 1;
                    if (fl != 0) {
                        dry = 1;
                    }
                }
                if ((d & 4) && state != 1) {
                    state = 2;
                } else if (!dry) {
                    break;
                }
                match++;
                count += (d & 0x40) != 0;
                count += (d & 1) != 0;
            } else {
                if ((d & 0x10004u) && flood_object_blocks(st, c, fl)) {
                    break;
                }
                match++;
                if ((d & 0x40) || (d & 0x26341237u) || (d & 8)) {
                    count++;
                }
            }
        }
        uint32_t mask = (fl != 0 ? 0x10000u : 0) | 4;
        for (uint32_t i = 0; i < 2; i++) {
            static const uint32_t front[3] = {G_U5DBDE8, G_U5DBE08, G_U5DBE28};
            for (int f = 0; f < 3; f++) {
                int32_t c = S32(st, front[f] + 8u * (uint32_t) k + 4u * i) + base;
                if (!(U32(st, G_GRID_TERRAIN + 4u * (uint32_t) c) & mask) && flood_object_blocks(st, c, fl)) {
                    match = 0;
                }
            }
        }
        if (match == 4) {
            S32(st, G_BUILD_TURN) = k;
            int32_t o = S32(st, G_BUILD_TURN_VIEW) - S32(st, G_VIEW_ORIENTATION) / 2;
            S32(st, G_BUILD_TURN_VIEW) = o < 0 ? o + 4 : o;
            return count;
        }
        k++;
        S32(st, G_BUILD_TURN_VIEW) = k;
        if (k >= 4) {
            return 999;
        }
    }
}

int32_t pyramid_ramp_blocked(game_state *st, int32_t b)
{
    /* @bin 0x004F6020 a pyramid part (type 0xB7) whose first part has no stage yet (+0xC8 < 1):
     * 1 unless its kind has a ramp (0x004ECEA0) and the ramp tile (table 0x005F5D08 by size,
     * from +0xC) walks (0x004ED360) with 0x00EAAE90 set; 0 for other buildings */
    if (U16(st, BUILDING_ADDR(b) + BLD_TYPE) != 0xB7) {
        return 0;
    }
    int32_t p = b - S16(st, BUILDING_ADDR(b) + BLD_PART);
    uint32_t sz = U8(st, BUILDING_ADDR(p) + BLD_MONUMENT_KIND);
    if (S16(st, BUILDING_ADDR(p) + BLD_UC8) >= 1) {
        return 0;
    }
    if ((uint8_t) GAME_CALL(st, FN_PYRAMID_KIND_OK, (int32_t) sz) == 0) {
        return 1;
    }
    int32_t t = (int32_t) (U32(st, G_PYRAMID_RAMP_START + 4u * sz) + U32(st, BUILDING_ADDR(p) + BLD_GRID_OFFSET) - U32(st, G_MAP_START));
    GAME_CALL(st, FN_RAMP_WALK, t % 0xE4, t / 0xE4, 3, 0);
    return S32(st, G_RAMP_WALK_TILE) == 0;
}

static int32_t run_ring_tile_outside(game_state *st, const int32_t *a)
{
    return ring_tile_outside(st, a[0], a[1], a[2], a[3], a[4]);
}

static int32_t run_tile_bit4_pattern(game_state *st, const int32_t *a)
{
    return tile_bit4_pattern(st, a[0], a[1]);
}

static int32_t run_direction_between(game_state *st, const int32_t *a)
{
    (void) st;
    return direction_between(a[0], a[1], a[2], a[3]);
}

static int32_t run_screen_buffer_clear(game_state *st, const int32_t *a)
{
    (void) a;
    screen_buffer_clear(st);
    return 0;
}

static int32_t run_shore_fit(game_state *st, const int32_t *a)
{
    return shore_fit(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_pyramid_ramp_blocked(game_state *st, const int32_t *a)
{
    return pyramid_ramp_blocked(st, a[0]);
}

const ported_function day2_misc_ports[] = {
    {0x004F6020u, "pyramid_ramp_blocked", 1, 8, 0, run_pyramid_ramp_blocked, 0},
    {0x0047CC70u, "shore_fit", 4, 32, 0, run_shore_fit, 0},
    {0x0047CBD0u, "tile_bit4_pattern", 2, 32, 0, run_tile_bit4_pattern, 0},
    {0x0051CC00u, "direction_between", 4, 8, 0, run_direction_between, 0},
    {0x004CF1C0u, "screen_buffer_clear", 0, 0, 0, run_screen_buffer_clear, 0},
    {0x00481AA0u, "ring_tile_outside", 5, 32, 0, run_ring_tile_outside, 0},
};
const int day2_misc_ports_count = sizeof(day2_misc_ports) / sizeof(day2_misc_ports[0]);
