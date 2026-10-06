/* Imhotep - the terrain checks of the construction preview (package ui).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Square scans of the terrain grid (0x00482720, 0x004828E0, 0x004829B0), the ghost footprint
 * marks of the drag preview (0x0047D360, 0x0047D4D0, 0x0047D530, 0x004BFA50) and the ferry
 * landing test (0x00486CF0, 0x00486BF0). The scans leave the tile they stopped at in 0x00D3B4E0
 * (y) and 0x00D3B4E4 (x), as 0x00482650 does. */
#include "command/construction_checks.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define MAP_W 0x00BCA2D0u
#define MAP_H 0x00BCA2D4u
#define MAP_START 0x00BCA2D8u
#define VIEW_ORIENTATION 0x00BCA2E0u
#define SCAN_Y 0x00D3B4E0u
#define SCAN_X 0x00D3B4E4u
#define TER(o) U32(st, 0x00AF598Cu + 4u * (uint32_t) (o))
#define BITFIELDS 0x00B285CCu
#define BUILDING_GRID 0x00B4E6FCu
#define FIGURE_GRID 0x00B350DCu   /* i16 per tile, read by 0x0047D360 */
#define OBJECT_GRID 0x00D05720u   /* u32 pointer per tile, read by 0x0047D530 */
#define SQUARE 0x005CC8C0u        /* @bin i32 offsets: row r at + 0x30 r, column c at + 8 c */
#define FOOT_ROWS 0x00BC9738u
#define FOOT_COLS 0x00BC973Cu
#define CURSOR 0x00BC9768u
#define NO_ROOM 0x00605994u
#define GRID_W 228

/* the n x n square at (x, y) grown by d, clipped to the map: [*xa, *xb] x [*ya, *yb] */
static void square(game_state *st, int32_t x, int32_t y, int32_t n, int32_t d, int32_t *xa, int32_t *xb, int32_t *ya,
                   int32_t *yb)
{
    *xa = x - d;
    *xb = x + n + d - 1;
    *ya = y - d;
    *yb = y + n + d - 1;
    if (*xa < 0) {
        *xa = 0;
    }
    if (*ya < 0) {
        *ya = 0;
    }
    if (*xb >= S32(st, MAP_W)) {
        *xb = S32(st, MAP_W) - 1;
    }
    if (*yb >= S32(st, MAP_H)) {
        *yb = S32(st, MAP_H) - 1;
    }
}

int32_t terrain_scan_predicate(game_state *st, int32_t x, int32_t y, int32_t n, int32_t d, uint32_t predicate)
{
    /* @bin 0x00482720 1 when predicate(terrain) (a function of the exe, 0x00605A48) holds for a
     * tile of the square */
    int32_t xa, xb, ya, yb;
    square(st, x, y, n, d, &xa, &xb, &ya, &yb);
    S32(st, SCAN_Y) = ya;
    int32_t o = S32(st, MAP_START) + ya * GRID_W + xa;
    for (int32_t r = ya; r <= yb; r = ++S32(st, SCAN_Y)) {
        S32(st, SCAN_X) = xa;
        for (int32_t i = xa; i <= xb; i = S32(st, SCAN_X)) {
            int32_t t = (int32_t) TER(o);
            if ((uint8_t) sim_call(st, NULL, predicate, &t, 1, 0) != 0) {
                return 1;
            }
            o++;
            S32(st, SCAN_X) = i + 1;
        }
        o += xa - xb + 0xE3;
    }
    return 0;
}

int32_t terrain_all_mask(game_state *st, int32_t x, int32_t y, int32_t n, int32_t d, uint32_t mask)
{
    /* @bin 0x004828E0 1 when every tile of the square has terrain & mask */
    int32_t xa, xb, ya, yb;
    square(st, x, y, n, d, &xa, &xb, &ya, &yb);
    S32(st, SCAN_Y) = ya;
    int32_t o = S32(st, MAP_START) + ya * GRID_W + xa;
    for (int32_t r = ya; r <= yb; r = ++S32(st, SCAN_Y)) {
        S32(st, SCAN_X) = xa;
        for (int32_t i = xa; i <= xb; i++) {
            if ((TER(o) & mask) == 0) {
                return 0;
            }
            o++;
            S32(st, SCAN_X) = i + 1;
        }
        o += xa - xb + 0xE3;
    }
    return 1;
}

int32_t terrain_count_mask(game_state *st, int32_t x, int32_t y, int32_t n, int32_t d, uint32_t mask, int32_t count)
{
    /* @bin 0x004829B0 1 when at least `count` tiles of the square have terrain & mask; the scan
     * counters are set to their end values first */
    int32_t xa, xb, ya, yb;
    square(st, x, y, n, d, &xa, &xb, &ya, &yb);
    S32(st, SCAN_Y) = ya;
    int32_t o = S32(st, MAP_START) + ya * GRID_W + xa;
    if (ya <= yb) {
        int32_t rows = yb - ya + 1;
        S32(st, SCAN_Y) = rows + ya;
        for (; rows != 0; rows--) {
            S32(st, SCAN_X) = xa;
            if (xa <= xb) {
                int32_t k = xb - xa + 1;
                int32_t p = o;
                o += k;
                S32(st, SCAN_X) = k + xa;
                for (; k != 0; k--, p++) {
                    if (TER(p) & mask) {
                        count--;
                    }
                }
            }
            o += xa - xb + 0xE3;
        }
    }
    return count <= 0;
}

/* @bin 0x0047D360 / 0x0047D530 / 0x004BFA50 the anchor moved by the view orientation for a
 * footprint of n tiles: 6 y, 4 x and y, 2 x by 1 - n */
static void anchor(game_state *st, int32_t n, int32_t *x, int32_t *y)
{
    int32_t o = S32(st, VIEW_ORIENTATION);
    if (o == 6) {
        *y += 1 - n;
    } else if (o == 4) {
        *x += 1 - n;
        *y += 1 - n;
    } else if (o == 2) {
        *x += 1 - n;
    }
}

void ghost_mark(game_state *st)
{
    /* @bin 0x0047D4D0 bitfield 0x10 over the footprint at the cursor (0x00BC9738 rows of
     * 0x00BC973C tiles); 0x00605994 := 1 */
    int32_t rows = S32(st, FOOT_ROWS);
    int32_t c = S32(st, CURSOR), cols = S32(st, FOOT_COLS);
    S32(st, NO_ROOM) = 1;
    for (int32_t r = 0; r < rows; r++) {
        for (int32_t i = 0; i < cols; i++) {
            int32_t t = c + S32(st, SQUARE + 0x30u * (uint32_t) r + 8u * (uint32_t) i);
            U8(st, BITFIELDS + (uint32_t) t) |= 0x10;
        }
    }
}

void ghost_footprint(game_state *st, int32_t n, int32_t x, int32_t y, uint32_t mask, int32_t no_anchor,
                     int32_t floodable)
{
    /* @bin 0x0047D360 (n, x, y, mask, no_anchor, floodable) the footprint is marked (0x0047D4D0)
     * when it lies 3 tiles inside the map, its tiles are all floodplain (terrain 0x10000) or all
     * not (floodplain only when `floodable`, low byte), none has terrain & mask & 0xEEFED77F and
     * none has a figure (0x00B350DC) */
    S32(st, FOOT_ROWS) = n;
    S32(st, FOOT_COLS) = n;
    if (no_anchor == 0 && S32(st, VIEW_ORIENTATION) != 0) {
        anchor(st, n, &x, &y);
    }
    if (x < 3 || y < 3 || n - 1 + x >= S32(st, MAP_W) - 3 || n - 1 + y >= S32(st, MAP_H) - 3) {
        return;
    }
    int flood = 0, dry = 0;
    int32_t c = S32(st, MAP_START) + y * GRID_W + x;
    S32(st, CURSOR) = c;
    for (int32_t r = 0; r < n; r++) {
        for (int32_t i = 0; i < n; i++) {
            int32_t t = S32(st, SQUARE + 0x30u * (uint32_t) r + 8u * (uint32_t) i) + c;
            uint32_t ter = TER(t);
            if (ter & 0x10000u) {
                if ((uint8_t) floodable == 0 || dry) {
                    return;
                }
                flood = 1;
            } else {
                if (flood) {
                    return;
                }
                dry = 1;
            }
            if (ter & mask & 0xEEFED77Fu) {
                return;
            }
            if (S16(st, FIGURE_GRID + 2u * (uint32_t) t) != 0) {
                return;
            }
        }
    }
    ghost_mark(st);
}

int32_t footprint_fits(game_state *st, int32_t n, int32_t x, int32_t y, uint32_t mask, int32_t floodable)
{
    /* @bin 0x0047D530 (n, x, y, mask, floodable) 1 when the footprint is on the map, no tile is
     * near the edge (0x0046F9A0(x, y, 3)), floodplain (terrain & mask & 0x10000) only when
     * `floodable` (low byte), no terrain & mask & 0xEEFED77F, and the tile's object (0x00D05720)
     * is absent or has +0x18 set */
    S32(st, FOOT_ROWS) = n;
    S32(st, FOOT_COLS) = n;
    if (S32(st, VIEW_ORIENTATION) != 0) {
        anchor(st, n, &x, &y);
    }
    if (x < 0 || y < 0 || n - 1 + x >= S32(st, MAP_W) || n - 1 + y >= S32(st, MAP_H)) {
        return 0;
    }
    S32(st, CURSOR) = S32(st, MAP_START) + y * GRID_W + x;
    for (int32_t r = 0; r < S32(st, FOOT_ROWS); r++) {
        for (int32_t i = 0; i < S32(st, FOOT_COLS); i++) {
            int32_t t = S32(st, SQUARE + 0x30u * (uint32_t) r + 8u * (uint32_t) i) + S32(st, CURSOR);
            if ((uint8_t) GAME_CALL(st, 0x0046F9A0u, t % GRID_W, t / GRID_W, 3) != 0) {
                return 0;
            }
            uint32_t obj = U32(st, OBJECT_GRID + 4u * (uint32_t) t);
            uint32_t ter = TER(t) & mask;
            if ((ter & 0x10000u) && (uint8_t) floodable == 0) {
                return 0;
            }
            if (ter & 0xEEFED77Fu) {
                return 0;
            }
            if (obj != 0 && P8(st, obj + 0x18) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

void gatehouse_ghost(game_state *st, int32_t x, int32_t y, int32_t rotation)
{
    /* @bin 0x004BFA50 the four parts of a gatehouse: part ids 0x005DF78C[rotation] (4 dwords),
     * part records 0x005DF814 (16 bytes: +0 size, +4 dy / dx, +8 dx / dy by rotation 0 / 1) */
    for (uint32_t k = 0; k < 4; k++) {
        int32_t p = S32(st, 0x005DF78Cu + 16u * (uint32_t) rotation + 4u * k);
        uint32_t rec = 0x005DF814u + 16u * (uint32_t) p;
        int32_t px, py;
        if (rotation == 0) {
            px = S32(st, rec + 8) + x;
            py = S32(st, rec + 4) + y;
        } else if (rotation == 1) {
            px = S32(st, rec + 4) + x;
            py = S32(st, rec + 8) + y;
        } else {
            px = x;
            py = y;
        }
        int32_t o = S32(st, VIEW_ORIENTATION);
        if (o != 0) {
            int32_t s = S32(st, rec);
            if (o == 2) {
                px += s - 1;
            } else if (o == 4) {
                px += s - 1;
                py += s - 1;
            } else {
                py += s - 1;
            }
        }
        ghost_footprint(st, S32(st, rec), px, py, 0xFFFFFFBFu, 0, 0);
    }
}

int32_t ferry_landing_at(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00486BF0 999 off the map; 1 when the tile is the anchor of a whole type-0x88 building
     * (its 2 x 2 tiles, 0x005CC8C0, carry the same building), else 0 */
    int32_t o = S32(st, VIEW_ORIENTATION);
    if (o == 6) {
        y--;
    } else if (o == 4) {
        x--;
        y--;
    } else if (o == 2) {
        x--;
    }
    if (x < 0 || y < 0 || x + 1 >= S32(st, MAP_W) || y + 1 >= S32(st, MAP_H)) {
        return 999;
    }
    int32_t t = S32(st, MAP_START) + y * GRID_W + x;
    int32_t b = S16(st, BUILDING_GRID + 2u * (uint32_t) t);
    if (b == 0 || S16(st, BUILDING_ADDR(b) + 0x10) != 0x88) {
        return 0;
    }
    for (uint32_t r = 0; r < 2; r++) {
        for (uint32_t i = 0; i < 2; i++) {
            int32_t u = S32(st, SQUARE + 0x30u * r + 8u * i) + t;
            if (b != S16(st, BUILDING_GRID + 2u * (uint32_t) u)) {
                return 0;
            }
        }
    }
    return 1;
}

int32_t ferry_landing_at_offset(game_state *st, int32_t offset)
{
    /* @bin 0x00486CF0 */
    int32_t r = offset - S32(st, MAP_START);
    return ferry_landing_at(st, r % GRID_W, r / GRID_W);
}

static int32_t run_scan_predicate(game_state *st, const int32_t *a)
{
    return terrain_scan_predicate(st, a[0], a[1], a[2], a[3], (uint32_t) a[4]);
}

static int32_t run_all_mask(game_state *st, const int32_t *a)
{
    return terrain_all_mask(st, a[0], a[1], a[2], a[3], (uint32_t) a[4]);
}

static int32_t run_count_mask(game_state *st, const int32_t *a)
{
    return terrain_count_mask(st, a[0], a[1], a[2], a[3], (uint32_t) a[4], a[5]);
}

static int32_t run_ghost_mark(game_state *st, const int32_t *a)
{
    (void) a;
    ghost_mark(st);
    return 0;
}

static int32_t run_ghost_footprint(game_state *st, const int32_t *a)
{
    ghost_footprint(st, a[0], a[1], a[2], (uint32_t) a[3], a[4], a[5]);
    return 0;
}

static int32_t run_footprint_fits(game_state *st, const int32_t *a)
{
    return footprint_fits(st, a[0], a[1], a[2], (uint32_t) a[3], a[4]);
}

static int32_t run_gatehouse_ghost(game_state *st, const int32_t *a)
{
    gatehouse_ghost(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_ferry_landing_at(game_state *st, const int32_t *a)
{
    return ferry_landing_at(st, a[0], a[1]);
}

static int32_t run_ferry_landing_at_offset(game_state *st, const int32_t *a)
{
    return ferry_landing_at_offset(st, a[0]);
}

const ported_function ui_checks_ports[] = {
    {0x00482720u, "terrain_scan_predicate", 5, 32, 0, run_scan_predicate, 0},
    {0x004828E0u, "terrain_all_mask", 5, 32, 0, run_all_mask, 0},
    {0x004829B0u, "terrain_count_mask", 6, 32, 0, run_count_mask, 0},
    {0x0047D4D0u, "ghost_mark", 0, 0, 0, run_ghost_mark, 0},
    {0x0047D360u, "ghost_footprint", 6, 0, 0, run_ghost_footprint, 0},
    {0x0047D530u, "footprint_fits", 5, 32, 0, run_footprint_fits, 0},
    {0x004BFA50u, "gatehouse_ghost", 3, 0, 0, run_gatehouse_ghost, 0},
    {0x00486BF0u, "ferry_landing_at", 2, 32, 0, run_ferry_landing_at, 0},
    {0x00486CF0u, "ferry_landing_at_offset", 1, 32, 0, run_ferry_landing_at_offset, 0},
};
const int ui_checks_ports_count = sizeof(ui_checks_ports) / sizeof(ui_checks_ports[0]);
