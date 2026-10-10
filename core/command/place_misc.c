/* Imhotep - per-type checks and work of the construction place command (package place): bridges,
 * gatehouses, temple complexes, monuments, and the predicates of the advice notices.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "command/place_misc.h"

#include "building/rest_redraw.h"
#include "command/place.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define TER(o) U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (o))
#define FGRID(o) S16(st, G_FIGURE_GRID + 2u * (uint32_t) (o))

static int32_t offset_of(game_state *st, int32_t x, int32_t y)
{
    return S32(st, G_MAP_START) + y * GRID_SIZE + x;
}

static int near_edge(game_state *st, int32_t x, int32_t y)
{
    return (uint8_t) GAME_CALL(st, FN_MAP_NEAR_EDGE, x, y, 3) != 0;
}

/* ---- bridges -------------------------------------------------------------------------------- */

void place_bridge(game_state *st, int32_t x, int32_t y, int32_t kind)
{
    /* @bin 0x0047F530 (x, y, kind) the bridge measured by the preview (length 0x00BC96F8 > 1, valid
     * 0x00BC96FC, direction 0x00BC96F4 turned by the view, step 0x00BC9700): road bit on its tiles
     * and its sprite pieces; kind 1 (high bridge) needs length 5. Length 0 when refused. */
    if (S32(st, G_BRIDGE_END) <= 0) {
        S32(st, G_BRIDGE_LENGTH) = 0;
        return;
    }
    int32_t len = S32(st, G_BRIDGE_LENGTH);
    if (len <= 1 || (kind == 1 && len < 5)) {
        S32(st, G_BRIDGE_LENGTH) = 0;
        return;
    }
    int32_t off = offset_of(st, x, y);
    int32_t dir = S32(st, G_BRIDGE_DIR) - S32(st, G_VIEW_ORIENTATION);
    S32(st, G_BRIDGE_DIR) = dir;
    if (dir < 0) {
        dir += 8;
        S32(st, G_BRIDGE_DIR) = dir;
    }
    int32_t mid = 0; /* @bin 0x0047F5BB the middle piece of a high bridge */
    if (kind == 1) {
        if (len == 9 || len == 10) {
            mid = 4;
        } else if (len == 11 || len == 12) {
            mid = 5;
        } else if (len == 13 || len == 14) {
            mid = 6;
        } else if (len == 15) {
            mid = 7;
        } else {
            mid = len != 16 ? 8 : 7;
        }
    }
    int32_t step = S32(st, G_BRIDGE_STEP);
    for (int32_t i = 0; i < len; i++, off += step) {
        TER(off) |= 0x40u;
        uint32_t s = G_SPRITE + (uint32_t) off;
        int32_t v = -1;
        if (kind == 0) {
            if (i == len - 1) {
                v = dir == 4 ? 4 : dir == 6 ? 1 : dir == 0 ? 6 : dir == 2 ? 3 : -1;
            } else if (i == 0) {
                v = dir == 4 ? 6 : dir == 6 ? 3 : dir == 0 ? 4 : dir == 2 ? 1 : -1;
            } else {
                v = dir == 4 ? 5 : dir == 6 ? 2 : dir == 0 ? 5 : dir == 2 ? 2 : -1;
            }
        } else if (i == len - 2 || i == 1) {
            v = 0xD;
        } else if (i == len - 1) {
            v = dir == 4 ? 7 : dir == 6 ? 8 : dir == 0 ? 9 : dir == 2 ? 0xA : -1;
        } else if (i == 0) {
            v = dir == 4 ? 9 : dir == 6 ? 0xA : dir == 0 ? 7 : dir == 2 ? 8 : -1;
        } else if (i == mid) {
            v = dir == 4 ? 0xE : dir == 6 ? 0xF : dir == 0 ? 0xE : dir == 2 ? 0xF : -1;
        } else {
            v = dir == 4 ? 0xB : dir == 6 ? 0xC : dir == 0 ? 0xB : dir == 2 ? 0xC : -1;
        }
        if (v >= 0) {
            U8(st, s) = (uint8_t) v;
        }
    }
    GAME_CALL0(st, FN_ROUTING_REBUILD_ED4140);
    GAME_CALL0(st, FN_ROUTING_REBUILD_EE0C50);
    GAME_CALL0(st, FN_ROUTING_REBUILD_EC7630);
}

/* ---- gatehouses and temple complexes -------------------------------------------------------- */

uint8_t place_gatehouse_fits(game_state *st, int32_t x, int32_t y, int32_t rot)
{
    /* @bin 0x004BFF60 (x, y, rotation) the 4 blocks of table 0x005DF810 (0x10 bytes: +0 road
     * allowed (u8), +4 size, +8 / +0xC offsets, swapped by the rotation) each tile free (a road tile
     * passes where allowed) */
    for (uint32_t e = G_GATEHOUSE_PARTS + 4u; e < G_GATEHOUSE_PARTS + 0x44u; e += 0x10) {
        int32_t bx, by;
        if (rot == 0) {
            bx = S32(st, e + 4) + x;
            by = S32(st, e) + y;
        } else if (rot == 1) {
            bx = S32(st, e) + x;
            by = S32(st, e + 4) + y;
        } else {
            bx = x;
            by = y;
        }
        int32_t n = S32(st, e - 4);
        for (int32_t j = 0; j < n; j++) {
            for (int32_t k = 0; k < n; k++) {
                if (U8(st, e - 8) != 0 && GAME_CALL(st, FN_LAND_BLOCK_FITS, bx + j, by + k, 1, 1, 0x40, 0, 0) == 0) {
                    continue;
                }
                if (GAME_CALL(st, FN_LAND_BLOCK_FITS, bx + j, by + k, 1, 1, -1, 0, 0) == 0) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

int32_t place_temple_complex_fits(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x004C3730 (x, y) a 7 x 13 area from the anchor of table 0x005E00CC (by the building
     * rotation 0x00BC9714 and the view / 2), its axes chosen by both: -1 when a tile is not
     * buildable (terrain & 0xEEFFD77F) or has a figure, -2 on a scenario special point; else 1 */
    int32_t rot = S32(st, G_BUILD_ORIENTATION);
    uint32_t e = G_COMPLEX_ORIGIN + 8u * (uint32_t) (rot + 4 * (S32(st, G_VIEW_ORIENTATION) / 2));
    int32_t x0 = S32(st, e) + x, y0 = S32(st, e + 4) + y;
    for (int32_t i = 0; i < 7; i++) {
        for (int32_t j = 0; j < 13; j++) {
            int32_t view = S32(st, G_VIEW_ORIENTATION);
            rot = S32(st, G_BUILD_ORIENTATION);
            int k; /* 1: (x0 + j, y0 + i), 2: (x0 - i, y0 - j), 3: (x0 - j, y0 - i), 4: (x0 + i, y0 + j) */
            static const int map[4][4] = {{1, 2, 3, 4}, {4, 1, 2, 3}, {3, 4, 1, 2}, {2, 3, 4, 1}};
            int r = rot == 0 ? 0 : rot == 1 ? 1 : rot == 2 ? 2 : 3;
            int v = view == 0 ? 0 : view == 2 ? 1 : view == 4 ? 2 : 3;
            k = map[v][r];
            int32_t tx, ty;
            switch (k) {
            case 1: tx = x0 + j; ty = y0 + i; break;
            case 2: tx = x0 - i; ty = y0 - j; break;
            case 3: tx = x0 - j; ty = y0 - i; break;
            default: tx = x0 + i; ty = y0 + j; break;
            }
            int32_t o = offset_of(st, tx, ty);
            uint32_t t = TER(o) & 0xEEFFD77Fu;
            if (FGRID(o) != 0) {
                return -1;
            }
            if (place_special_point_in(st, tx, ty, tx, ty)) {
                return -2;
            }
            if (t != 0) {
                return -1;
            }
        }
    }
    return 1;
}

/* ---- monuments ------------------------------------------------------------------------------- */

/* the tiles of a part (size x size from cell (px, py) of the anchor (x, y)): not near the map edge,
 * terrain & 0xFEFFD76E clear */
static int part_free(game_state *st, int32_t px, int32_t py, int32_t size)
{
    S32(st, G_TILE_CURSOR) = offset_of(st, px, py);
    for (int32_t r = 0, r6 = 0; r < size; r++, r6 += 6) {
        for (int32_t c = 0; c < size; c++) {
            if (near_edge(st, S32(st, G_VIEW_DX) + c + px, S32(st, G_VIEW_DY) + r + py)) {
                return 0;
            }
            int32_t o = S32(st, G_FOOTPRINT_OFFSETS + 8u * (uint32_t) (r6 + c)) + S32(st, G_TILE_CURSOR);
            if (TER(o) & 0xFEFFD76Eu) {
                return 0;
            }
        }
    }
    return 1;
}

uint8_t place_monument_de_fits(game_state *st, int32_t offset)
{
    /* @bin 0x004DEF00 (grid offset) the 0x74 parts of monument 0xDE (layout 0x004DED50 by the view)
     * free */
    int32_t rel = offset - S32(st, G_MAP_START);
    int32_t x = rel % GRID_SIZE, y = rel / GRID_SIZE;
    for (int32_t i = 0; i < 0x74; i++) {
        int32_t lay[13];
        int32_t view = S32(st, G_VIEW_ORIENTATION);
        monument_part_layout_4ded50(st, lay, i, view, view);
        if (!part_free(st, lay[0] + x, lay[1] + y, lay[2])) {
            return 0;
        }
    }
    return 1;
}

uint8_t place_monument_d5_fits(game_state *st, int32_t offset)
{
    /* @bin 0x00541AE0 (grid offset) the 100 parts of table 0x00601DD8 (0x34 bytes: +0 x, +4 y,
     * +8 size) free */
    int32_t rel = offset - S32(st, G_MAP_START);
    int32_t x = rel % GRID_SIZE, y = rel / GRID_SIZE;
    for (uint32_t e = G_SUN_TEMPLE_PARTS + 4u; e < G_SUN_TEMPLE_PARTS + 4u + 100u * 0x34u; e += 0x34) {
        if (!part_free(st, S32(st, e - 4) + x, S32(st, e) + y, S32(st, e + 4))) {
            return 0;
        }
    }
    return 1;
}

uint8_t place_monument_d5_done(game_state *st)
{
    /* @bin 0x00542EE0 a monument 0xD5 (state 1) whose status 0x00542AD0 is 100; its 0x63 parts
     * skipped */
    for (int32_t i = 1; i < BUILDING_COUNT; i++) {
        uint32_t b = BUILDING_ADDR(i);
        if (U8(st, b + BLD_STATE) == 1 && S16(st, b + BLD_TYPE) == 0xD5) {
            if (GAME_CALL(st, FN_MONUMENT_D5_STATUS, i, 0) == 0x64) {
                return 1;
            }
            i += 0x63;
        }
    }
    return 0;
}

static uint32_t vfn(game_state *st, uint32_t obj, uint32_t slot)
{
    return U32(st, U32(st, obj) + slot); /* the object's virtual table */
}

uint8_t place_monument_fits(game_state *st, uint32_t obj, int32_t offset, uint32_t parts, int32_t n)
{
    /* @bin 0x004E5330 (this; grid offset, parts (0x34 bytes: +0 x, +4 y, +8 size), n) each part's
     * tiles not near the map edge and accepted by the object's virtual +0x10 (terrain, part) */
    int32_t rel = offset - S32(st, G_MAP_START);
    int32_t x = rel % GRID_SIZE, y = rel / GRID_SIZE;
    uint8_t ok = 1;
    for (int32_t k = 0; k < n; k++) {
        uint32_t p = parts + 0x34u * (uint32_t) k + 8;
        if (!ok) {
            return 0;
        }
        int32_t py = S32(st, p - 4) + y, px = S32(st, p - 8) + x;
        S32(st, G_TILE_CURSOR) = offset_of(st, px, py);
        for (int32_t r = 0, r6 = 0; r < S32(st, p) && ok; r++, r6 += 6) {
            for (int32_t c = 0; c < S32(st, p) && ok; c++) {
                if (near_edge(st, S32(st, G_VIEW_DX) + c + px, S32(st, G_VIEW_DY) + r + py)) {
                    ok = 0;
                } else {
                    int32_t o = S32(st, G_FOOTPRINT_OFFSETS + 8u * (uint32_t) (r6 + c)) + S32(st, G_TILE_CURSOR);
                    ok = (uint8_t) GAME_CALLT(st, obj, vfn(st, obj, 0x10), (int32_t) TER(o), k);
                }
            }
        }
    }
    return ok;
}

uint8_t place_monument_exists(game_state *st, uint32_t obj, int32_t type, int32_t n)
{
    /* @bin 0x004E5790 (object, type, n) among buildings 1 .. n - 1, the first of the type in use:
     * 1 when the object's virtual +0xC (building, 0) is below 100 */
    uint8_t found = 0;
    int stop = 0;
    for (int32_t i = 1; i < n && !stop; i++) {
        uint32_t b = BUILDING_ADDR(i);
        if (U8(st, b + BLD_STATE) == 1 && S16(st, b + BLD_TYPE) == type) {
            stop = 1;
            if (GAME_CALLT(st, obj, vfn(st, obj, 0xC), i, 0) < 0x64) {
                found = 1;
            }
        }
    }
    return found;
}

uint8_t place_monument_fits_5463e0(game_state *st, uint32_t obj, int32_t offset, uint32_t parts, int32_t n)
{
    /* @bin 0x005463E0 (this; grid offset, parts, n) as 0x004E5330, and the part this+0x50 must have
     * one of its four neighbours of the anchor (N, S, W, E) accepted by the virtual +0x10 */
    int32_t rel = offset - S32(st, G_MAP_START);
    int32_t ex = rel % GRID_SIZE, ey = rel / GRID_SIZE;
    uint8_t ok = 1;
    if (n <= 0) {
        return ok;
    }
    uint32_t p = parts + 4;
    for (int32_t k = 0; k < n; k++, p += 0x34) {
        if (!ok) {
            return ok;
        }
        if (k == S32(st, obj + 0x50)) {
            static const uint32_t near_[4] = {G_GRID_TERRAIN - 4u * GRID_SIZE, G_GRID_TERRAIN + 4u * GRID_SIZE, G_GRID_TERRAIN - 4u, G_GRID_TERRAIN + 4u};
            int any = 0;
            for (int d = 0; d < 4; d++) {
                int32_t o = (S32(st, p) + ey) * GRID_SIZE + S32(st, p - 4) + S32(st, G_MAP_START) + ex;
                uint32_t t = U32(st, near_[d] + 4u * (uint32_t) o);
                uint8_t r = (uint8_t) GAME_CALLT(st, obj, vfn(st, obj, 0x10), (int32_t) t, k);
                if (d < 3) {
                    any |= r != 0;
                } else if (r == 0 && !any) {
                    ok = 0;
                }
            }
        }
        int32_t py = S32(st, p) + ey, px = S32(st, p - 4) + ex;
        S32(st, G_TILE_CURSOR) = offset_of(st, px, py);
        int32_t size = S32(st, p + 4);
        for (int32_t r = 0, r6 = 0; r < size && ok; r++, r6 += 6) {
            for (int32_t c = 0; c < S32(st, p + 4) && ok; c++) {
                if (near_edge(st, S32(st, G_VIEW_DX) + c + px, S32(st, G_VIEW_DY) + r + py)) {
                    ok = 0;
                } else {
                    int32_t o = S32(st, G_FOOTPRINT_OFFSETS + 8u * (uint32_t) (r6 + c)) + S32(st, G_TILE_CURSOR);
                    ok = (uint8_t) GAME_CALLT(st, obj, vfn(st, obj, 0x10), (int32_t) TER(o), k);
                }
            }
            size = S32(st, p + 4);
        }
    }
    return ok;
}

/* ---- predicates of the advice notices -------------------------------------------------------- */

int32_t place_trade_sells_443900(game_state *st, int32_t res)
{
    /* @bin 0x00443900 an empire object (200 of 0x62 bytes at 0x0077F748: +1 set, +0 == 1, +0x18
     * clear) for which 0x00443000(object, res) holds */
    for (int32_t i = 0; i < 200; i++) {
        uint32_t e = G_EMPIRE_OBJECTS + 0x62u * (uint32_t) i;
        if (U8(st, e + 1) != 0 && U8(st, e) == 1 && U8(st, e + 0x18) == 0 && GAME_CALL(st, FN_EMPIRE_OBJECT_SELLS, i, res) != 0) {
            return 1;
        }
    }
    return 0;
}

int32_t place_tower_wall_near(game_state *st, int32_t x, int32_t y, int32_t size)
{
    /* @bin 0x00480770 (x, y, size) one of the up to 24 offsets of table 0x005CC9E0[size] (0 ends
     * it) on a wall tile (terrain 0x4000) */
    int32_t off = offset_of(st, x, y);
    uint32_t t = G_RING_OFFSETS + 0x60u * (uint32_t) size;
    for (int i = 0; i < 0x18; i++, t += 4) {
        int32_t d = S32(st, t);
        if (d == 0) {
            return 0;
        }
        if (TER(d + off) & 0x4000u) {
            return 1;
        }
    }
    return 0;
}

uint8_t place_type_farm_489880(int32_t type)
{
    /* @bin 0x00489880 (this ignored; building) types 0x66..0x69 and 0xC4 (jump table 0x004898B4) */
    return (type >= 0x66 && type <= 0x69) || type == 0xC4;
}

int32_t place_food_ratio_447530(game_state *st)
{
    /* @bin 0x00447530 a food measure of the current city, in percent of 0x004C5340: farms by
     * 0x00488140 (48 per unit for the 0x00488050 kind, 64 for the others), other producers by
     * their model +0x14 * 4 (rated by 0x004CDE40(total, 80)), traded foods of the empire objects
     * (0x0077DE00, 61 of 0x6A bytes), two CITY ratios; the x87 sum (farms / 10.0 + the rest,
     * truncated by __ftol 0x0056133C) is exact in integers: the divisor is 10 */
    int32_t p = S32(st, CURRENT_PLAYER_ADDR);
    uint32_t c = CITY_ADDR(p);
    int32_t r18 = 0, r14 = 0;
    if (S32(st, c + CITY_U476C) != 0) {
        r18 = (int32_t) ((uint32_t) (S32(st, c + CITY_U4770) / S32(st, c + CITY_U476C)) * (uint32_t) S32(st, G_INDUSTRY_WORKING + 4u * 8u));
    }
    if (S32(st, c + CITY_U4778) != 0) {
        r14 = (int32_t) ((uint32_t) (S32(st, c + CITY_U477C) / S32(st, c + CITY_U4778)) * (uint32_t) S32(st, G_INDUSTRY_WORKING + 4u * 7u));
    }
    int32_t other = 0, farm_a = 0, farm_b = 0;
    for (int32_t i = 1; i < BUILDING_COUNT; i++) {
        uint32_t b = BUILDING_ADDR(i);
        if (U8(st, b + BLD_STATE) != 1) {
            continue;
        }
        if ((uint8_t) GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_IS_TYPE_0X487F80, i) == 0 && S16(st, b + BLD_TYPE) != 0xC2) {
            continue;
        }
        if ((uint8_t) GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_IS_TYPE_0X487F80, i) != 0 &&
            place_type_farm_489880(S16(st, b + BLD_TYPE)) == 0) {
            continue;
        }
        if ((uint8_t) GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_NEEDS_AREA_CHECK, i) != 0) {
            farm_a += GAME_CALLT(st, G_BUILDING_MANAGER, FN_FARM_YIELD_488140, i, 1);
        } else if ((uint8_t) GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_IS_TYPE_0X487F80, i) != 0) {
            farm_b += GAME_CALLT(st, G_BUILDING_MANAGER, FN_FARM_YIELD_488140, i, 1);
        } else if (S16(st, b + BLD_WORKERS) > 0 && S16(st, b + BLD_INDUSTRY_PROGRESS) > 0) {
            int32_t v = (int32_t) ((uint32_t) S32(st, G_TYPE_WORKERS + 0x28u * (uint32_t) S16(st, b + BLD_TYPE)) << 4);
            other += v / 4;
        }
    }
    int32_t farms = farm_a * 48 + farm_b * 64;
    int32_t rated = GAME_CALL(st, FN_CALC_PERCENTAGE, other, 0x50);
    int32_t traded = 0;
    for (uint32_t e = G_EMPIRE_CITIES + 2u; e < G_EMPIRE_CITY_END + 2u; e += 0x6A) {
        if (U8(st, e - 2) == 0) {
            continue;
        }
        uint8_t k = U8(st, e);
        if (k != 1 && k != 3 && k != 5) {
            continue;
        }
        if (U8(st, e + 3) == 0) {
            continue;
        }
        for (int32_t r = 1; r <= 8; r++) {
            int16_t s = S16(st, c + CITY_TRADE_STATUS + 2u * (uint32_t) r);
            if ((s == 1 || s == 3) && U8(st, e + (uint32_t) r + 0x28) == 1) {
                int32_t v = S32(st, G_TRADE_ROUTE_LIMITS + 4u * (uint32_t) (r + 36 * U8(st, e + 2)));
                traded += (int32_t) ((uint32_t) v * 100u) / 12;
            }
        }
    }
    int32_t rest = traded + rated + r14 + r18;
    int32_t total = (int32_t) (((int64_t) farms + 10 * (int64_t) rest) / 10); /* @bin 0x00447749 */
    int32_t d = GAME_CALL(st, FN_TRADE_TAX_4C5340, p);
    return (int32_t) ((uint32_t) total * 100u) / d;
}

/* ---- table ------------------------------------------------------------------------------------ */

static int32_t run_bridge(game_state *st, const int32_t *a)
{
    place_bridge(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_gatehouse(game_state *st, const int32_t *a)
{
    return place_gatehouse_fits(st, a[0], a[1], a[2]);
}

static int32_t run_complex(game_state *st, const int32_t *a)
{
    return place_temple_complex_fits(st, a[0], a[1]);
}

static int32_t run_de(game_state *st, const int32_t *a)
{
    return place_monument_de_fits(st, a[0]);
}

static int32_t run_d5(game_state *st, const int32_t *a)
{
    return place_monument_d5_fits(st, a[0]);
}

static int32_t run_d5_done(game_state *st, const int32_t *a)
{
    (void) a;
    return place_monument_d5_done(st);
}

static int32_t run_fits(game_state *st, const int32_t *a)
{
    return place_monument_fits(st, (uint32_t) a[PORT_THIS_ARG], a[0], (uint32_t) a[1], a[2]);
}

static int32_t run_exists(game_state *st, const int32_t *a)
{
    return place_monument_exists(st, (uint32_t) a[0], a[1], a[2]);
}

static int32_t run_fits_5463e0(game_state *st, const int32_t *a)
{
    return place_monument_fits_5463e0(st, (uint32_t) a[PORT_THIS_ARG], a[0], (uint32_t) a[1], a[2]);
}

/* @bin the per-monument wrappers: (this; offset) → 0x004E5330 / 0x005463E0 with the parts at this +
 * a fixed offset and their count; (this) → 0x004E5790(this, type, count) */
#define FITS(name, base, n) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        uint32_t t = (uint32_t) a[PORT_THIS_ARG]; \
        return place_monument_fits(st, t, a[0], t + (base), n); \
    }
#define FITS2(name, base, n) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        uint32_t t = (uint32_t) a[PORT_THIS_ARG]; \
        return place_monument_fits_5463e0(st, t, a[0], t + (base), n); \
    }
#define EXISTS(name, type, n) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        return place_monument_exists(st, (uint32_t) a[PORT_THIS_ARG], type, n); \
    }
FITS(run_401400, 0x4C, 0x10)
EXISTS(run_401430, 0xE6, 0x10)
FITS(run_40d850, 0x4C, 0x19)
EXISTS(run_40d880, 0xE1, 0x19)
FITS(run_416f70, 0x4C, 9)
EXISTS(run_416fa0, 0xE3, 9)
FITS(run_4e65f0, 0x4C, 2)
EXISTS(run_4e6650, 0xE4, 2)
FITS2(run_5456c0, 0x1E5C, 0xA7)
EXISTS(run_545700, 0xE5, 0xA7)
FITS2(run_54f3f0, 0x5C88, 0x201)
EXISTS(run_54f430, 0xEC, 0x201)
FITS2(run_555e00, 0x4EE0, 0x1B5)
EXISTS(run_555e40, 0xEB, 0x1B5)
FITS2(run_558b60, 0x1CBC, 0x9E)
EXISTS(run_558ba0, 0xEA, 0x9E)

static int32_t run_443900(game_state *st, const int32_t *a)
{
    return place_trade_sells_443900(st, a[0]);
}

static int32_t run_480770(game_state *st, const int32_t *a)
{
    return place_tower_wall_near(st, a[0], a[1], a[2]);
}

static int32_t run_489880(game_state *st, const int32_t *a)
{
    return place_type_farm_489880(S16(st, BUILDING_ADDR(a[0]) + BLD_TYPE));
}

static int32_t run_447530(game_state *st, const int32_t *a)
{
    (void) a;
    return place_food_ratio_447530(st);
}

const ported_function place_misc_ports[] = {
    {0x0047F530u, "place_bridge", 3, 0, 0, run_bridge, 0},
    {0x004BFF60u, "place_gatehouse_fits", 3, 8, 0, run_gatehouse, 0},
    {0x004C3730u, "place_temple_complex_fits", 2, 32, 0, run_complex, 0},
    {0x004DEF00u, "place_monument_de_fits", 1, 8, 0, run_de, 0},
    {0x00541AE0u, "place_monument_d5_fits", 1, 8, 0, run_d5, 0},
    {0x00542EE0u, "place_monument_d5_done", 0, 8, 0, run_d5_done, 0},
    {0x004E5330u, "place_monument_fits", 3, 8, 0, run_fits, 0},
    {0x004E5790u, "place_monument_exists", 3, 8, 0, run_exists, 0},
    {0x005463E0u, "place_monument_fits_5463e0", 3, 8, 0, run_fits_5463e0, 0},
    {0x00401400u, "monument_e6_fits", 1, 8, G_MONUMENT_E6, run_401400, 0},
    {0x00401430u, "monument_e6_exists", 0, 8, G_MONUMENT_E6, run_401430, 0},
    {0x0040D850u, "monument_e1_fits", 1, 8, G_MONUMENT_E1, run_40d850, 0},
    {0x0040D880u, "monument_e1_exists", 0, 8, G_MONUMENT_E1, run_40d880, 0},
    {0x00416F70u, "monument_e3_fits", 1, 8, G_MONUMENT_E3, run_416f70, 0},
    {0x00416FA0u, "monument_e3_exists", 0, 8, G_MONUMENT_E3, run_416fa0, 0},
    {0x004E65F0u, "monument_e4_fits", 1, 8, G_MONUMENT_E4, run_4e65f0, 0},
    {0x004E6650u, "monument_e4_exists", 0, 8, G_MONUMENT_E4, run_4e6650, 0},
    {0x005456C0u, "monument_e5_fits", 1, 8, G_MONUMENT_E5, run_5456c0, 0},
    {0x00545700u, "monument_e5_exists", 0, 8, G_MONUMENT_E5, run_545700, 0},
    {0x0054F3F0u, "monument_ec_fits", 1, 8, G_MONUMENT_EC, run_54f3f0, 0},
    {0x0054F430u, "monument_ec_exists", 0, 8, G_MONUMENT_EC, run_54f430, 0},
    {0x00555E00u, "monument_eb_fits", 1, 8, G_MONUMENT_EB, run_555e00, 0},
    {0x00555E40u, "monument_eb_exists", 0, 8, G_MONUMENT_EB, run_555e40, 0},
    {0x00558B60u, "monument_ea_fits", 1, 8, G_MONUMENT_EA, run_558b60, 0},
    {0x00558BA0u, "monument_ea_exists", 0, 8, G_MONUMENT_EA, run_558ba0, 0},
    {0x00443900u, "place_trade_sells_443900", 1, 32, 0, run_443900, 0},
    {0x00480770u, "place_tower_wall_near", 3, 32, 0, run_480770, 0},
    {0x00489880u, "place_type_farm_489880", 1, 8, G_BUILDING_MANAGER, run_489880, 0},
    {0x00447530u, "place_food_ratio_447530", 0, 32, 0, run_447530, 0},
};
const int place_misc_ports_count = sizeof(place_misc_ports) / sizeof(place_misc_ports[0]);
