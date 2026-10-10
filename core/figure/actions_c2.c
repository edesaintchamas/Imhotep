/* Imhotep - figure actions, group c: the boats' helpers (transport ship 0x4D, warship 0x4E) and
 * the map searches they use.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). These helpers take the figure id as an
 * argument; several return results through pointers (the original's caller frame). */
#include "figure/actions_c2.h"

#include <string.h>

#include "city/day_x87.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define FIG(id) FIGURE_ADDR(id)
/* a formation record + FM_TYPE, as the original addresses it: a field is FORMATION(i) - FM_TYPE + FM_* */
#define FORMATION(i) ((uint32_t) ((int32_t) (G_FORMATION_BASE + FM_TYPE) + 0x90 * (int32_t) (i)))

static int32_t type_speed(game_state *st, uint8_t type)
{
    return S32(st, G_PROPS + FIGTYPE_SPEED + 56u * type); /* @bin 0x007090EC */
}

static uint32_t terrain(game_state *st, uint32_t idx, int32_t delta)
{
    return U32(st, G_GRID_TERRAIN + 4u * (idx + (uint32_t) delta));
}

/* ---- line searches ------------------------------------------------------------------------- */

/* @bin 0x0040C1F2 .. 0x0040C2C1 (x87, 53-bit precision, core/city/day_x87.h) the lines' steps:
 * q = dy / dx (fild, fidiv), its inverse 1.0 / q; a row k away gives x = __ftol(k * (1 / q) + x0),
 * a column k away y = __ftol(k * q + y0). dx = 0: the inverse is 0.0; dy = 0: q is 0.0. */
static int32_t line_row_x(int32_t k, int32_t dx, int32_t dy, int32_t x0)
{
    if (dx == 0) {
        return x0; /* 0.0 * k + x0 */
    }
    x87_val inv = x87_divv(x87_from_int(1), x87_div(dy, dx));
    return x87_ftol(x87_add(x87_mul_int(inv, k), x87_from_int(x0)));
}

static int32_t line_col_y(int32_t k, int32_t dx, int32_t dy, int32_t y0)
{
    if (dy == 0) {
        return y0;
    }
    x87_val q = x87_div(dy, dx);
    return x87_ftol(x87_add(x87_mul_int(q, k), x87_from_int(y0)));
}

/* @bin 0x0040C2C9 open water: the tile and its four neighbours are water, the routing grid is not
 * 0xFF and the last distance search reached it */
static int water_open(game_state *st, uint32_t idx)
{
    return (terrain(st, idx, 0) & 4) && (terrain(st, idx, -228) & 4) && (terrain(st, idx, 228) & 4) &&
           (terrain(st, idx, 1) & 4) && (terrain(st, idx, -1) & 4) && U8(st, G_GRID_ROUTING_WATER + idx) != 0xFF &&
           S32(st, G_GRID_ROAD_DISTANCE + 4u * idx) > 0;
}

/* @bin 0x0040BED7 dry land: terrain bit 0x40, or no water on the tile and its eight neighbours */
static int land_open(game_state *st, uint32_t idx)
{
    uint32_t t = terrain(st, idx, 0);
    if (t & 0x40) {
        return 1;
    }
    if (t & 4) {
        return 0;
    }
    static const int32_t around[8] = {-228, 228, 1, -1, -227, -229, 229, 227};
    for (int i = 0; i < 8; i++) {
        if (terrain(st, idx, around[i]) & 4) {
            return 0;
        }
    }
    return 1;
}

static int32_t sign(int32_t v)
{
    return v < 0 ? -1 : v > 0 ? 1 : 0;
}

/* the common part of 0x0040C180 and 0x0040BDA0: one step per row, then one per column; the tile
 * farthest from (x0, y0) (ties: the later) */
static int32_t line_search(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int land, int32_t *bx,
                           int32_t *by)
{
    int32_t dx = x1 - x0, dy = y1 - y0;
    int32_t sx = sign(dx), sy = sign(dy);
    int32_t best = 0;
    *bx = x0;
    *by = y0;
    uint32_t base = U32(st, G_MAP_START);
    int32_t ady = dy < 0 ? -dy : dy, adx = dx < 0 ? -dx : dx;
    /* rows: x = trunc((y - y0) / (dy / dx) + x0); no step when dx = 0 gives x0 */
    for (int32_t j = 1, y = y0; j <= ady; j++) {
        y += sy;
        int32_t k = y - y0;
        int32_t x = line_row_x(k, dx, dy, x0);
        uint32_t idx = base + 228u * (uint32_t) y + (uint32_t) x;
        if (land ? land_open(st, idx) : water_open(st, idx)) {
            int32_t ex = x - x0;
            int32_t d = (int32_t) ((uint32_t) k * (uint32_t) k + (uint32_t) ex * (uint32_t) ex);
            if (d >= best) {
                best = d;
                *bx = x;
                *by = y;
            }
        }
    }
    /* columns: y = trunc((x - x0) * (dy / dx) + y0); skipped when dx = 0 (no columns) */
    for (int32_t i = 1, x = x0; i <= adx; i++) {
        x += sx;
        int32_t k = x - x0;
        int32_t y = line_col_y(k, dx, dy, y0);
        uint32_t idx = base + 228u * (uint32_t) y + (uint32_t) x;
        if (land ? land_open(st, idx) : water_open(st, idx)) {
            int32_t ey = y - y0;
            int32_t d = (int32_t) ((uint32_t) ey * (uint32_t) ey + (uint32_t) k * (uint32_t) k);
            if (d >= best) {
                best = d;
                *bx = x;
                *by = y;
            }
        }
    }
    return best;
}

int32_t line_search_water(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t *px, int32_t *py)
{
    /* @bin 0x0040C180 */
    if (x0 == x1 && y0 == y1) {
        *px = x0;
        *py = y0;
        return 0;
    }
    GAME_CALL(st, FN_ROUTE_DISTANCES_ED4140, x0, y0);
    int32_t bx, by;
    int32_t best = line_search(st, x0, y0, x1, y1, 0, &bx, &by);
    *px = bx;
    *py = by;
    return best;
}

int32_t line_search_land(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t *px, int32_t *py)
{
    /* @bin 0x0040BDA0 */
    if (x0 == x1 && y0 == y1) {
        *px = x0;
        *py = y0;
        return 0;
    }
    int32_t bx, by;
    int32_t best = line_search(st, x0, y0, x1, y1, 1, &bx, &by);
    *px = bx;
    *py = by;
    if (bx != x0 || by != y0) {
        return best;
    }
    /* @bin 0x0040C097 nothing found: the first neighbour without water, in this order */
    uint32_t idx = U32(st, G_MAP_START) + 228u * (uint32_t) by + (uint32_t) bx;
    static const struct {
        int32_t delta, ox, oy;
    } next[8] = {{-228, 0, -1}, {-227, 1, -1}, {228, 0, 1}, {-229, -1, -1},
                 {1, 1, 0},     {229, 1, 1},   {-1, -1, 0}, {227, -1, 1}};
    for (int i = 0; i < 8; i++) {
        if ((terrain(st, idx, next[i].delta) & 4) == 0) {
            *py = by + next[i].oy;
            *px = bx + next[i].ox;
            break;
        }
    }
    return best;
}

/* ---- nearest free tile ----------------------------------------------------------------------- */

static int free_water(game_state *st, uint32_t idx)
{
    return S8(st, G_GRID_ROUTING_WATER + idx) >= 0 && U16(st, G_FIGURE_GRID + 2u * idx) == 0;
}

static int free_land(game_state *st, uint32_t idx)
{
    int8_t r = S8(st, G_GRID_ROUTING + idx);
    return r >= 0 && r <= 4 && (r & 1) == 0 && U16(st, G_GRID_BUILDING + 2u * idx) == 0;
}

/* @bin 0x004467E6, 0x004AD8D0 rings of growing size around the tile (absolute grid coordinates,
 * clipped to the map area): the top row, the bottom row, the left column, the right column; up to
 * 227 rings */
static int spiral(game_state *st, int32_t *px, int32_t *py, int land)
{
    int32_t x = *px, y = *py;
    uint32_t idx0 = U32(st, G_MAP_START) + 228u * (uint32_t) y + (uint32_t) x;
    if (land ? free_land(st, idx0) : free_water(st, idx0)) {
        return 1;
    }
    int32_t ys = S32(st, G_VIEW_DY), xs = S32(st, G_VIEW_DX);
    int32_t ymax = y + ys + 1, ymin = y + ys - 1, xmax = x + xs + 1, xmin = x + xs - 1;
    for (int32_t r = 1; r < 0xE4; r++) {
        int32_t x_lo = xmin, x_hi = xmax, y_lo = ymin, y_hi = ymax;
        int clip_left = 0, clip_right = 0, clip_top = 0, clip_bottom = 0;
        if (x_lo < S32(st, G_VIEW_DX)) {
            clip_left = 1;
            x_lo = S32(st, G_VIEW_DX);
        }
        if (x_hi > S32(st, G_MAP_XE)) {
            clip_right = 1;
            x_hi = S32(st, G_MAP_XE);
        }
        if (y_lo < S32(st, G_VIEW_DY)) {
            clip_top = 1;
            y_lo = S32(st, G_VIEW_DY);
        }
        if (y_hi > S32(st, G_MAP_YE)) {
            clip_bottom = 1;
            y_hi = S32(st, G_MAP_YE);
        }
        int32_t fx = 0, fy = 0, found = 0;
        if (!clip_top) {
            for (int32_t i = x_lo; i <= x_hi && !found; i++) {
                uint32_t idx = 228u * (uint32_t) y_lo + (uint32_t) i;
                if (land ? free_land(st, idx) : free_water(st, idx)) {
                    found = 1, fx = i, fy = y_lo;
                }
            }
        }
        if (!found && !clip_bottom) {
            for (int32_t i = x_lo; i <= x_hi && !found; i++) {
                uint32_t idx = 228u * (uint32_t) y_hi + (uint32_t) i;
                if (land ? free_land(st, idx) : free_water(st, idx)) {
                    found = 1, fx = i, fy = y_hi;
                }
            }
        }
        if (!found && !clip_left) {
            for (int32_t j = y_lo; j <= y_hi && !found; j++) {
                uint32_t idx = 228u * (uint32_t) j + (uint32_t) x_lo;
                if (land ? free_land(st, idx) : free_water(st, idx)) {
                    found = 1, fx = x_lo, fy = j;
                }
            }
        }
        if (!found && !clip_right) {
            for (int32_t j = y_lo; j <= y_hi && !found; j++) {
                uint32_t idx = 228u * (uint32_t) j + (uint32_t) x_hi;
                if (land ? free_land(st, idx) : free_water(st, idx)) {
                    found = 1, fx = x_hi, fy = j;
                }
            }
        }
        if (found) {
            *px = fx - S32(st, G_VIEW_DX);
            *py = fy - S32(st, G_VIEW_DY);
            return 1;
        }
        xmax++, ymax++, xmin--, ymin--;
    }
    return 0;
}

int32_t spiral_free_water(game_state *st, int32_t *px, int32_t *py)
{
    /* @bin 0x004467A0 */
    return spiral(st, px, py, 0);
}

int32_t spiral_free_land(game_state *st, int32_t *px, int32_t *py)
{
    /* @bin 0x004AD880 */
    return spiral(st, px, py, 1);
}

int32_t boat_flee_target(game_state *st, int32_t *px, int32_t *py, int32_t a3, int32_t a4, int32_t a5, int32_t a6,
                         int32_t a7, int32_t a8, int32_t id)
{
    /* @bin 0x004A4850 */
    uint32_t f = FIG(id);
    int32_t ox, oy;
    int32_t x = S16(st, f + FIG_X), y = S16(st, f + FIG_Y);
    int32_t best = 0;
    int32_t r = line_search_water(st, x, y, x + a3, y + a4, &ox, &oy);
    if (r >= 0) {
        *px = ox;
        *py = oy;
        best = r;
    }
    x = S16(st, f + FIG_X), y = S16(st, f + FIG_Y);
    r = line_search_water(st, x, y, x + a5, y + a6, &ox, &oy);
    if (r >= best) {
        *px = ox;
        *py = oy;
        best = r;
    }
    x = S16(st, f + FIG_X), y = S16(st, f + FIG_Y);
    r = line_search_water(st, x, y, x + a7, y + a8, &ox, &oy);
    if (r >= best) {
        *px = ox;
        *py = oy;
    }
    return (uint8_t) spiral_free_water(st, px, py);
}

/* ---- the boats' states ---------------------------------------------------------------------- */

/* @bin 0x004806C0 the tile next to a building (x +0x06, y +0x08, size +0x03): FOUND_X, FOUND_Y */
static void building_tile(game_state *st, uint32_t b)
{
    GAME_CALL(st, FN_BUILDING_FIND_ROAD_8, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE));
}

void boat_seek_dock(game_state *st, int32_t id)
{
    /* @bin 0x004A45D0 a dock (0x00406A30 of the current figure): go there (0x4D: action 0x0B,
     * 0x4E: action 0x0D); none: wait (action 8) */
    int32_t dock = best_dock(st, S32(st, G_CURRENT_FIGURE));
    uint32_t f = FIG(id);
    if (dock <= 0) {
        uint8_t t = U8(st, f + FIG_TYPE);
        if (t == 0x4D || t == 0x4E) {
            U8(st, f + FIG_ACTION) = 8;
        }
        return;
    }
    uint8_t t = U8(st, f + FIG_TYPE);
    if (t == 0x4D) {
        U8(st, f + FIG_ACTION) = 0x0B;
    } else if (t == 0x4E) {
        U8(st, f + FIG_ACTION) = 0x0D;
    }
    int32_t saved_y = S32(st, G_ROAD_ACCESS_Y), saved_x = S32(st, G_ROAD_ACCESS_X);
    building_tile(st, BUILDING_ADDR(dock));
    U16(st, f + FIG_SOURCE_X) = U16(st, f + FIG_X);
    U16(st, f + FIG_SOURCE_Y) = U16(st, f + FIG_Y);
    U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
    U16(st, f + FIG_DEST_BUILDING) = (uint16_t) dock;
    U16(st, f + FIG_DEST_X) = U16(st, G_ROAD_ACCESS_X);
    U16(st, f + FIG_DEST_Y) = U16(st, G_ROAD_ACCESS_Y);
    S32(st, G_ROAD_ACCESS_X) = saved_x;
    S32(st, G_ROAD_ACCESS_Y) = saved_y;
    GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
}

void boat_sink(game_state *st, int32_t id)
{
    /* @bin 0x004A46F0 */
    GAME_CALL(st, FN_FIGURE_DYING, id);
    uint32_t f = FIG(id);
    if (U16(st, f + FIG_WAIT_TICKS) != 2) {
        return;
    }
    if (U8(st, f + FIG_U130) != 0) {
        /* @bin 0x004A471D the boat's formation (+0x92) loses its boat (+0x80) and its soldiers
         * (+0x04, 16 ids) die */
        uint32_t fm = FORMATION(U8(st, f + FIG_U92));
        S32(st, fm - FM_TYPE + FM_U84) = 0;
        for (int i = 0; i < 16; i++) {
            int16_t s = S16(st, fm - FM_TYPE + FM_FIGURES + 2u * (uint32_t) i);
            if (s == 0) {
                break;
            }
            U8(st, FIG(s) + FIG_STATE) = 2;
        }
    }
    GAME_CALL0(st, FN_FORMATIONS_REBUILD);
}

void boat_moored(game_state *st, int32_t id, int32_t building, int32_t *moving)
{
    /* @bin 0x004A4770 every 50 ticks: back to the building's tile */
    (void) moving;
    uint32_t f = FIG(id);
    U16(st, f + FIG_WAIT_TICKS) += 1;
    if (S16(st, f + FIG_WAIT_TICKS) < 0x32) {
        return;
    }
    U16(st, f + FIG_WAIT_TICKS) = 0;
    U8(st, f + FIG_ANIM_FRAME) = 0;
    if (building == 0) {
        return;
    }
    int32_t saved_x = S32(st, G_ROAD_ACCESS_X), saved_y = S32(st, G_ROAD_ACCESS_Y);
    uint32_t b = BUILDING_ADDR(building);
    U16(st, b + BLD_SECOND_WALKER) = (uint16_t) id;
    U8(st, f + FIG_ACTION) = 9;
    building_tile(st, b);
    U16(st, f + FIG_SOURCE_X) = U16(st, f + FIG_X);
    U16(st, f + FIG_SOURCE_Y) = U16(st, f + FIG_Y);
    U16(st, f + FIG_DEST_X) = U16(st, G_ROAD_ACCESS_X);
    U16(st, f + FIG_DEST_Y) = U16(st, G_ROAD_ACCESS_Y);
    S32(st, G_ROAD_ACCESS_X) = saved_x;
    S32(st, G_ROAD_ACCESS_Y) = saved_y;
    GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
}

void boat_go(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004A4950 move to the destination; at it, stop (and turn to an enemy, 0x004A4300);
     * the building's +0xB4 is the boat when it is at the building's tile */
    uint32_t f = FIG(id);
    int32_t speed = type_speed(st, U8(st, f + FIG_TYPE));
    int32_t building = S16(st, f + FIG_HOME);
    if (U16(st, f + FIG_X) == U16(st, f + FIG_DEST_X) && U16(st, f + FIG_Y) == U16(st, f + FIG_DEST_Y)) {
        U8(st, f + FIG_DIRECTION) = 8;
    } else {
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
    }
    uint8_t dir = U8(st, f + FIG_DIRECTION);
    if (dir == 9 || dir == 0x0A) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
    } else if (dir == 8) {
        *moving = 0;
        uint8_t a = U8(st, f + FIG_ACTION);
        if (a != 0x14 && a != 0x0B) {
            if ((uint8_t) boat_at_dock(st, id) != 0) {
                U8(st, f + FIG_ACTION) = 0x0A;
                S32(st, f + FIG_U15C) = 0x10;
            }
        }
    }
    if (building != 0) {
        uint32_t b = BUILDING_ADDR(building);
        building_tile(st, b);
        if (S32(st, G_ROAD_ACCESS_X) == S16(st, f + FIG_X) && S32(st, G_ROAD_ACCESS_Y) == S16(st, f + FIG_Y)) {
            U16(st, b + BLD_SECOND_WALKER) = (uint16_t) id;
        } else {
            U16(st, b + BLD_SECOND_WALKER) = 0;
        }
    }
}

/* @bin 0x004A4A80, 0x004A6970 +0x15C: `going` moving away, 16 waiting; every 75 ticks with an
 * enemy around (0x004A5560), move 10 tiles away from it (three directions tried) */
static void flee(game_state *st, int32_t id, int32_t *moving, void (*go)(game_state *, int32_t, int32_t *),
                 int32_t going, int reset_frame)
{
    uint32_t f = FIG(id);
    int32_t mode = S32(st, f + FIG_U15C);
    if (mode == going) {
        go(st, id, moving);
        if (U8(st, f + FIG_DIRECTION) == 8) {
            S32(st, f + FIG_U15C) = 0x10;
            *moving = 0;
        }
        return;
    }
    if (mode != 0x10) {
        return;
    }
    *moving = 0;
    U16(st, f + FIG_WAIT_TICKS) += 1;
    if (S16(st, f + FIG_WAIT_TICKS) < 0x4B) {
        return;
    }
    U16(st, f + FIG_WAIT_TICKS) = 0;
    int32_t enemy = boat_nearest(st, id, 0x12, 0);
    if (enemy == 0) {
        return;
    }
    uint32_t e = FIG(enemy);
    uint8_t d = (uint8_t) GAME_CALL(st, FN_ROUTE_DIRECTION, S16(st, e + FIG_X), S16(st, e + FIG_Y), S16(st, f + FIG_X),
                                    S16(st, f + FIG_Y));
    /* @bin 0x004A4B26, 0x004A6A1F the three offsets tried for each direction of the enemy */
    static const int8_t offsets[8][6] = {
        {0, -10, 10, -10, -10, -10}, {0, -10, 10, -10, 10, 0},  {10, -10, 10, 0, 10, 10},
        {10, 0, 10, 10, 0, 10},      {10, 10, 0, 10, -10, 10},  {0, 10, -10, 0, -10, 10},
        {-10, 0, -10, 10, -10, -10}, {-10, 0, -10, -10, 0, -10},
    };
    const int8_t *o = offsets[d <= 6 ? d : 7];
    /* the outputs land in the original's argument slots (moving, id): always written */
    int32_t tx = 0, ty = 0;
    boat_flee_target(st, &tx, &ty, o[0], o[1], o[2], o[3], o[4], o[5], id);
    U16(st, f + FIG_DEST_X) = (uint16_t) tx;
    U16(st, f + FIG_DEST_Y) = (uint16_t) ty;
    GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
    S32(st, f + FIG_U15C) = going;
    if (reset_frame) {
        U8(st, f + FIG_ANIM_FRAME) = 0;
    }
}

void boat_flee(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004A4A80 */
    flee(st, id, moving, boat_go, 9, 1);
}

void boat_to_battle(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004A4C40 move to the target (+0x5E); there, fight (0x004A4340, action 0x0F) unless
     * a dock other than the target is free */
    uint32_t f = FIG(id);
    GAME_CALL(st, FN_FIGURE_MOVE, id, type_speed(st, U8(st, f + FIG_TYPE)));
    uint8_t dir = U8(st, f + FIG_DIRECTION);
    if (dir == 8) {
        U16(st, f + FIG_WAIT_TICKS) = 0;
        *moving = 0;
        if (dock_battle_value(st, S16(st, f + FIG_DEST_BUILDING)) >= 1) {
            if (best_dock(st, id) != S16(st, f + FIG_DEST_BUILDING)) {
                boat_seek_dock(st, id);
                return;
            }
        }
        boat_join_battle(st, id, S16(st, f + FIG_DEST_BUILDING));
        U8(st, f + FIG_ACTION) = 0x0F;
    } else if (dir == 9 || dir == 0x0A) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
    }
}

void boat_to_formation(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004A4D00 move; there, pick up the formation (0x004A43F0) if it is the boat's
     * (+0x92) and has no boat yet (+0x80) */
    uint32_t f = FIG(id);
    GAME_CALL(st, FN_FIGURE_MOVE, id, type_speed(st, U8(st, f + FIG_TYPE)));
    uint8_t dir = U8(st, f + FIG_DIRECTION);
    if (dir == 8) {
        *moving = 0;
        int32_t fmi = boat_formation_ready(st, id);
        if (fmi == 0) {
            return;
        }
        if (fmi == (int32_t) U8(st, f + FIG_U92)) {
            uint32_t fm = FORMATION(fmi);
            if (S32(st, fm - FM_TYPE + FM_U84) == 0) {
                U8(st, f + FIG_U130) = 1;
                U16(st, fm - FM_TYPE + FM_U32) = U16(st, f + FIG_X);
                U16(st, fm - FM_TYPE + FM_U34) = U16(st, f + FIG_Y);
                formation_embark(st, fmi);
                U8(st, f + FIG_ACTION) = 9;
                S32(st, fm - FM_TYPE + FM_U84) = id;
                U16(st, f + FIG_WAIT_TICKS) = 0;
                return;
            }
            U8(st, f + FIG_ACTION) = 0x0C;
            U8(st, f + FIG_U92) = 0;
        }
        U16(st, f + FIG_WAIT_TICKS) = 0;
    } else if (dir == 9) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
    } else if (dir == 0x0A) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        U8(st, f + FIG_ACTION) = 0x0A;
    }
}

void boat_to_landing(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004A4E10 move; there, land the formation (+0x92) at the dry tile nearest to the
     * destination (+0x30, +0x32) */
    uint32_t f = FIG(id);
    GAME_CALL(st, FN_FIGURE_MOVE, id, type_speed(st, U8(st, f + FIG_TYPE)));
    int32_t fmi = U8(st, f + FIG_U92);
    uint8_t dir = U8(st, f + FIG_DIRECTION);
    if (dir == 8) {
        *moving = 0;
        if (U16(st, f + FIG_U30) == 0 && U16(st, f + FIG_U32) == 0) {
            return;
        }
        int32_t x = 0, y = 0;
        line_search_land(st, S16(st, f + FIG_U30), S16(st, f + FIG_U32), S16(st, f + FIG_X), S16(st, f + FIG_Y), &x, &y);
        spiral_free_land(st, &x, &y);
        uint32_t fm = FORMATION(fmi);
        U16(st, fm - FM_TYPE + FM_U32) = (uint16_t) x;
        U16(st, fm - FM_TYPE + FM_U34) = (uint16_t) y;
        U16(st, fm - FM_TYPE + FM_X) = (uint16_t) x;
        U16(st, fm - FM_TYPE + FM_Y) = (uint16_t) y;
        S32(st, fm - FM_TYPE + FM_U84) = 0;
        formation_land(st, fmi, x, y);
        U8(st, f + FIG_U130) = 0;
        U8(st, f + FIG_U92) = 0;
        GAME_CALL(st, FN_FORMATION_MOVE_TO, S16(st, f + FIG_U30), S16(st, f + FIG_U32), fmi, 0);
        U8(st, f + FIG_ACTION) = 9;
        U16(st, f + FIG_U30) = 0;
        U16(st, f + FIG_U32) = 0;
    } else if (dir == 9) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
    } else if (dir == 0x0A) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        U8(st, f + FIG_ACTION) = 0x0A;
    }
}

void boat_face_enemy(game_state *st, int32_t id)
{
    /* @bin 0x004A4F60 every 20 ticks: +0x12 the direction to the nearest enemy (0x004BCD60) */
    uint32_t f = FIG(id);
    U16(st, f + FIG_WAIT_TICKS) += 1;
    if (S16(st, f + FIG_WAIT_TICKS) < 0x14) {
        return;
    }
    U16(st, f + FIG_WAIT_TICKS) = 0;
    int32_t e = enemy_warship(st, id, 5);
    uint32_t ef = FIG(e);
    int32_t d = GAME_CALL(st, FN_DIRECTION_TO_FINE, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, ef + FIG_X), S16(st, ef + FIG_Y));
    U8(st, f + FIG_PREV_DIRECTION) = (uint8_t) d;
}

/* ---- warship ------------------------------------------------------------------------------- */

static int32_t absdiff(int32_t a, int32_t b)
{
    return a > b ? a - b : a < b ? b - a : 0;
}

/* @bin 0x004A6419 the larger of the two distances, both kept in 0x00D3B4FC, 0x00D3B4F8 */
static int32_t tile_distance(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    int32_t dx = absdiff(x1, x2);
    S32(st, G_DIST_X) = dx;
    int32_t dy = absdiff(y1, y2);
    S32(st, G_DIST_Y) = dy;
    return dx >= dy ? dx : dy;
}

static int32_t type_word(game_state *st, uint32_t f, uint32_t off)
{
    return S32(st, G_FIGURE_TYPES + off + 56u * U8(st, f + FIG_TYPE)); /* @bin 0x007090D0 type records */
}
#define TYPE_RANGE 0x14  /* +0x14 of the type record (0x007090E4): firing range */
#define TYPE_RELOAD 0x18 /* +0x18 (0x007090E8): ticks between shots */

/* @bin 0x004A6C6B the shot sequence +0x6A: 1 fires a missile (type 0x3C) at (tx, ty), then counts
 * to 100 */
static void fire_step(game_state *st, int32_t id, uint32_t f, int32_t tx, int32_t ty)
{
    uint8_t a = U8(st, f + FIG_ATTACK_IMAGE);
    if (a == 0) {
        return;
    }
    if (a == 1) {
        GAME_CALL(st, FN_MISSILE_CREATE, id, S16(st, f + FIG_X), S16(st, f + FIG_Y), tx, ty, 0x3C);
    }
    uint8_t n = (uint8_t) (U8(st, f + FIG_ATTACK_IMAGE) + 1);
    U8(st, f + FIG_ATTACK_IMAGE) = n;
    if (n > 0x64) {
        U8(st, f + FIG_ATTACK_IMAGE) = 0;
    }
}

/* @bin 0x004A6465 within range: reload (+0x6B), then fire when there is a target; out of range,
 * or (strict) while reloading: no shot */
static void aim(game_state *st, uint32_t f, int32_t dist, int32_t target, int strict)
{
    if (dist >= type_word(st, f, TYPE_RANGE)) {
        U8(st, f + FIG_ATTACK_IMAGE) = 0;
        return;
    }
    uint8_t c = (uint8_t) (U8(st, f + FIG_U6B) + 1);
    U8(st, f + FIG_U6B) = c;
    if ((int32_t) c <= type_word(st, f, TYPE_RELOAD)) {
        if (strict) {
            U8(st, f + FIG_ATTACK_IMAGE) = 0;
        }
        return;
    }
    U8(st, f + FIG_U6B) = 0;
    U8(st, f + FIG_ATTACK_IMAGE) = target != 0 ? 1 : 0;
}

void warship_moored(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004A58E0 every 50 ticks: back to the building's tile (action 0x0B) */
    uint32_t f = FIG(id);
    *moving = 0;
    U16(st, f + FIG_WAIT_TICKS) += 1;
    if (S16(st, f + FIG_WAIT_TICKS) < 0x32) {
        return;
    }
    U16(st, f + FIG_WAIT_TICKS) = 0;
    int32_t building = S16(st, f + FIG_HOME);
    if (building == 0) {
        return;
    }
    int32_t saved_x = S32(st, G_ROAD_ACCESS_X), saved_y = S32(st, G_ROAD_ACCESS_Y);
    uint32_t b = BUILDING_ADDR(building);
    U16(st, b + BLD_SECOND_WALKER) = (uint16_t) id;
    U8(st, f + FIG_ACTION) = 0x0B;
    building_tile(st, b);
    U16(st, f + FIG_SOURCE_X) = U16(st, f + FIG_X);
    U16(st, f + FIG_SOURCE_Y) = U16(st, f + FIG_Y);
    U16(st, f + FIG_DEST_X) = U16(st, G_ROAD_ACCESS_X);
    U16(st, f + FIG_DEST_Y) = U16(st, G_ROAD_ACCESS_Y);
    S32(st, G_ROAD_ACCESS_X) = saved_x;
    S32(st, G_ROAD_ACCESS_Y) = saved_y;
    GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
}

void warship_ram(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004A6160 move (0x004A5680); ram a boat met (0x004A5A80): damage by the angle (table
     * 0x005DC24C), both turn aside; then 15 ticks before the next ram (+0x164) */
    GAME_CALL(st, FN_FIGURE_MOVE_HURRIED, id, 1);
    uint32_t f = FIG(id);
    int32_t wait = S32(st, f + FIG_U164);
    if (wait != 0) {
        wait -= 1;
        if (wait < 0) {
            wait = 0;
        } else if (wait >= 0x0F) {
            wait = 0x0F;
        }
        S32(st, f + FIG_U164) = wait;
    } else {
        int32_t t = boat_ahead(st, id);
        if (t != 0) {
            uint32_t g = FIG(t);
            int32_t td = S8(st, g + FIG_DIRECTION) >= 8 ? S8(st, g + FIG_PREV_DIRECTION) : S8(st, g + FIG_DIRECTION);
            int32_t k = U8(st, (uint32_t) ((int32_t) G_RAM_DIVISOR + td * 8 + S8(st, f + FIG_DIRECTION)));
            /* @bin 0x004A61E2 +0x134 at least the type's +0x24: damage +0x150, else +0x158 */
            int32_t base = S32(st, f + FIG_U134) >= S32(st, G_PROPS + FIGTYPE_U28 + 56u * U8(st, f + FIG_TYPE)) ? S32(st, f + FIG_U150)
                                                                                             : S32(st, f + FIG_U158);
            if (k == 0) {
                state_unported(st, FN_WARSHIP_RAM); /* idiv by 0: the original faults */
                return;
            }
            int32_t damage = base / k;
            boat_ram_sound(st, k);
            S32(st, g + FIG_HIT_POINTS) = (int32_t) ((uint32_t) S32(st, g + FIG_HIT_POINTS) - (uint32_t) damage);
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            /* @bin 0x004A621E a turn of -1 or +1 by bit 1 of 0x00D3B374 */
            int32_t r = (S32(st, G_R_1_7) & 2) - 1;
            int32_t nd = S8(st, f + FIG_DIRECTION) + r;
            if (nd >= 7) {
                nd = 7;
            } else if (nd < 0) {
                nd = 0;
            }
            int32_t od = S8(st, g + FIG_DIRECTION) - r;
            if (od >= 7) {
                od = 7;
            } else if (od < 0) {
                od = 0;
            }
            U8(st, f + FIG_DIRECTION) = (uint8_t) nd;
            U8(st, g + FIG_DIRECTION) = (uint8_t) od;
            S32(st, f + FIG_U164) = 0x0F;
            U8(st, f + FIG_ACTION) = 0x0A;
            U16(st, f + FIG_TARGET) = 0;
            GAME_CALL(st, FN_FIGURE_SPAWN_RING, S16(st, g + FIG_X), S16(st, g + FIG_Y), 0, 0);
        }
    }
    uint8_t dir = U8(st, f + FIG_DIRECTION);
    if (dir == 8) {
        *moving = 0;
    } else if (dir == 9 || dir == 0x0A) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
    }
}

void warship_attack(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004A6300 by +0x15C: 16 look for a target (0x004BCAE0), 9 chase and fire, 21 fight
     * near home (+0x14C, +0x14E), 22 back home */
    uint32_t f = FIG(id);
    int32_t mode = S32(st, f + FIG_U15C);
    if (mode == 0x10) {
        if (U8(st, f + FIG_ACTION) != 0x0A) {
            S32(st, f + FIG_U15C) = 0x16;
        }
        int32_t t = warship_target(st, id, 0xE4);
        if (t != 0) {
            uint32_t g = FIG(t);
            S32(st, f + FIG_U15C) = 9;
            U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
            U16(st, f + FIG_DEST_X) = U16(st, g + FIG_X);
            U16(st, f + FIG_DEST_Y) = U16(st, g + FIG_Y);
            U16(st, f + FIG_TARGET) = (uint16_t) t;
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            return;
        }
        GAME_CALL(st, FN_FIGURE_MOVE_HURRIED, id, 0);
        if (U8(st, f + FIG_DIRECTION) == 8) {
            *moving = 0;
        }
    } else if (mode == 9) {
        int32_t t = S16(st, f + FIG_TARGET);
        if (U8(st, f + FIG_ACTION) != 0x0A) {
            S32(st, f + FIG_U15C) = 0x16;
        }
        uint32_t g = FIG(t);
        int32_t dist = tile_distance(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, g + FIG_X), S16(st, g + FIG_Y));
        aim(st, f, dist, t, 1);
        fire_step(st, id, f, S16(st, g + FIG_X), S16(st, g + FIG_Y));
        warship_ram(st, id, moving);
        if (U8(st, f + FIG_DIRECTION) == 8) {
            S32(st, f + FIG_U15C) = 0x10;
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        }
    } else if (mode == 0x15) {
        if (U8(st, f + FIG_ACTION) != 0x17) {
            S32(st, f + FIG_U15C) = 0x10;
        }
        int32_t home = tile_distance(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_U14C), S16(st, f + FIG_U14E));
        int go_home = home >= 0x0A;
        int32_t t = 0;
        uint32_t g = 0;
        int32_t dist = 0;
        if (!go_home) {
            t = S16(st, f + FIG_TARGET);
            g = FIG(t);
            dist = tile_distance(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, g + FIG_X), S16(st, g + FIG_Y));
            go_home = dist >= type_word(st, f, TYPE_RANGE);
        }
        if (go_home) {
            S32(st, f + FIG_U15C) = 0x16;
            U16(st, f + FIG_DEST_X) = U16(st, f + FIG_U14C);
            U16(st, f + FIG_DEST_Y) = U16(st, f + FIG_U14E);
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            return;
        }
        aim(st, f, dist, t, 0);
        fire_step(st, id, f, S16(st, g + FIG_X), S16(st, g + FIG_Y));
        warship_ram(st, id, moving);
        if (U8(st, f + FIG_DIRECTION) == 8) {
            S32(st, f + FIG_U15C) = 0x16;
            U16(st, f + FIG_DEST_X) = U16(st, f + FIG_U14C);
            U16(st, f + FIG_DEST_Y) = U16(st, f + FIG_U14E);
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        }
    } else if (mode == 0x16) {
        int32_t t = warship_target(st, id, type_word(st, f, TYPE_RANGE) + 0x0A);
        if (U8(st, f + FIG_ACTION) != 0x17) {
            S32(st, f + FIG_U15C) = 0x10;
        }
        if (t != 0) {
            uint32_t g = FIG(t);
            if (tile_distance(st, S16(st, g + FIG_X), S16(st, g + FIG_Y), S16(st, f + FIG_U14C), S16(st, f + FIG_U14E)) < 0x0A) {
                S32(st, f + FIG_U15C) = 0x15;
                U16(st, f + FIG_TARGET) = (uint16_t) t;
                U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
                U16(st, f + FIG_DEST_X) = U16(st, g + FIG_X);
                U16(st, f + FIG_DEST_Y) = U16(st, g + FIG_Y);
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
                return;
            }
        }
        GAME_CALL(st, FN_FIGURE_MOVE_HURRIED, id, 0);
        if (U8(st, f + FIG_DIRECTION) == 8) {
            *moving = 0;
        }
    }
}

void warship_go(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004A6890 move (0x004A5680); at the destination, turn to an enemy (0x004A4300): guard
     * there (action 0x17, mode 22, home +0x14C, +0x14E) */
    uint32_t f = FIG(id);
    if (U16(st, f + FIG_X) == U16(st, f + FIG_DEST_X) && U16(st, f + FIG_Y) == U16(st, f + FIG_DEST_Y)) {
        U8(st, f + FIG_DIRECTION) = 8;
    } else {
        GAME_CALL(st, FN_FIGURE_MOVE_HURRIED, id, 0);
        *moving = 1;
    }
    uint8_t dir = U8(st, f + FIG_DIRECTION);
    if (dir == 9 || dir == 0x0A) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
    } else if (dir == 8) {
        *moving = 0;
        uint8_t a = U8(st, f + FIG_ACTION);
        if (a != 0x14 && a != 0x0A && a != 0x0D) {
            if ((uint8_t) boat_at_dock(st, id) != 0) {
                U8(st, f + FIG_ACTION) = 0x17;
                S32(st, f + FIG_U15C) = 0x16;
                U16(st, f + FIG_U14C) = U16(st, f + FIG_X);
                U16(st, f + FIG_U14E) = U16(st, f + FIG_Y);
            }
        }
    }
}

void warship_flee(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004A6970 */
    flee(st, id, moving, warship_go, 0x0B, 0);
}

void warship_to_battle(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004A6B40 as 0x004A4C40 with the boat movement 0x004A5680; blocked: sinks */
    GAME_CALL(st, FN_FIGURE_MOVE_HURRIED, id, 0);
    uint32_t f = FIG(id);
    uint8_t dir = U8(st, f + FIG_DIRECTION);
    if (dir == 8) {
        U16(st, f + FIG_WAIT_TICKS) = 0;
        *moving = 0;
        if (dock_battle_value(st, S16(st, f + FIG_DEST_BUILDING)) >= 1) {
            if (best_dock(st, id) != S16(st, f + FIG_DEST_BUILDING)) {
                boat_seek_dock(st, id);
                return;
            }
        }
        boat_join_battle(st, id, S16(st, f + FIG_DEST_BUILDING));
        U8(st, f + FIG_ACTION) = 0x0F;
    } else if (dir == 9) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
    } else if (dir == 0x0A) {
        U8(st, f + FIG_ACTION) = 4;
    }
}

void warship_fire(game_state *st, int32_t id)
{
    /* @bin 0x004A6BF0 reloaded: a target within range (0x004BCAE0, its tile in FOUND_X/Y) */
    uint32_t f = FIG(id);
    uint8_t c = (uint8_t) (U8(st, f + FIG_U6B) + 1);
    U8(st, f + FIG_U6B) = c;
    if ((int32_t) c > type_word(st, f, TYPE_RELOAD)) {
        U8(st, f + FIG_U6B) = 0;
        int32_t t = warship_target(st, id, type_word(st, f, TYPE_RANGE));
        U8(st, f + FIG_ATTACK_IMAGE) = t != 0 ? 1 : 0;
    }
    fire_step(st, id, f, S32(st, G_ROAD_ACCESS_X), S32(st, G_ROAD_ACCESS_Y));
}

void warship_face_enemy(game_state *st, int32_t id)
{
    /* @bin 0x004A6CC0 as 0x004A4F60, unless +0x154 is 2 */
    uint32_t f = FIG(id);
    U16(st, f + FIG_WAIT_TICKS) += 1;
    if (S16(st, f + FIG_WAIT_TICKS) < 0x14) {
        return;
    }
    U16(st, f + FIG_WAIT_TICKS) = 0;
    if (U8(st, f + FIG_U154) == 2) {
        return;
    }
    int32_t e = enemy_warship(st, id, 5);
    uint32_t ef = FIG(e);
    int32_t d = GAME_CALL(st, FN_DIRECTION_TO_FINE, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, ef + FIG_X), S16(st, ef + FIG_Y));
    U8(st, f + FIG_PREV_DIRECTION) = (uint8_t) d;
}

void warship_hunt(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004A6D40 a target (+0x82) alive, else the nearest (0x004BCAE0); 16: go for it, or
     * stop here (action +0x10); 9: chase, fire and ram */
    uint32_t f = FIG(id);
    int32_t t = S16(st, f + FIG_TARGET);
    int need = t == 0;
    if (!need) {
        uint32_t g = FIG(t);
        need = U8(st, g + FIG_STATE) != 1 || U8(st, g + FIG_ACTION) == 4;
    }
    if (need) {
        t = warship_target(st, id, 0xE4);
        U16(st, f + FIG_TARGET) = (uint16_t) t;
        S32(st, f + FIG_U15C) = 0x10;
    }
    int32_t mode = S32(st, f + FIG_U15C);
    if (mode == 9) {
        uint32_t g = FIG(t);
        int32_t dist = tile_distance(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, g + FIG_X), S16(st, g + FIG_Y));
        aim(st, f, dist, t, 1);
        fire_step(st, id, f, S16(st, g + FIG_X), S16(st, g + FIG_Y));
        warship_ram(st, id, moving);
        if (U8(st, f + FIG_DIRECTION) != 8) {
            U16(st, f + FIG_WAIT_TICKS) += 1;
            if (S16(st, f + FIG_WAIT_TICKS) <= 0x0A) {
                return;
            }
            U16(st, f + FIG_WAIT_TICKS) = 0;
            if (U16(st, f + FIG_DEST_X) == U16(st, g + FIG_X) || U16(st, f + FIG_DEST_Y) == U16(st, g + FIG_Y)) {
                return;
            }
        }
        S32(st, f + FIG_U15C) = 0x10;
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        return;
    }
    if (mode != 0x10) {
        return;
    }
    if (t != 0) {
        uint32_t g = FIG(t);
        S32(st, f + FIG_U15C) = 9;
        U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
        U16(st, f + FIG_DEST_X) = U16(st, g + FIG_X);
        U16(st, f + FIG_DEST_Y) = U16(st, g + FIG_Y);
        U16(st, f + FIG_TARGET) = (uint16_t) t;
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        return;
    }
    U8(st, f + FIG_ACTION) = U8(st, f + FIG_ACTION_BEFORE_ATTACK);
    U16(st, f + FIG_DEST_X) = U16(st, f + FIG_X);
    U16(st, f + FIG_DEST_Y) = U16(st, f + FIG_Y);
    U16(st, f + FIG_U14C) = U16(st, f + FIG_X);
    U16(st, f + FIG_U14E) = U16(st, f + FIG_Y);
    uint8_t a = U8(st, f + FIG_ACTION);
    if (a == 0x17) {
        S32(st, f + FIG_U15C) = 0x16;
    } else if (a == 0x0A) {
        S32(st, f + FIG_U15C) = 0x10;
    }
    U16(st, f + FIG_TARGET) = 0;
    GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
}

/* ---- boat helpers -------------------------------------------------------------------------- */

int32_t boat_at_dock(game_state *st, int32_t id)
{
    /* @bin 0x004A4300 the boat is the one at its building's tile (+0xB4) */
    int32_t b = S16(st, FIG(id) + FIG_HOME);
    if (b == 0) {
        return 0;
    }
    return S16(st, BUILDING_ADDR(b) + BLD_SECOND_WALKER) == id ? 1 : 0;
}

int32_t boat_join_battle(game_state *st, int32_t id, int32_t building)
{
    /* @bin 0x004A4340 the building's battle +0xCB (> 0) takes the boat (0x004F68F0, thiscall on
     * the battle record 0x009783A4 + 28 * battle), unless it sinks */
    uint8_t k = U8(st, BUILDING_ADDR(building) + BLD_UCB);
    if (k == 0) {
        return 0;
    }
    if (U8(st, FIG(id) + FIG_ACTION) == 4) {
        return 0;
    }
    battle_add_boat(st, G_REC4A + 28u * k, id);
    return 1;
}

int32_t boat_formation_ready(game_state *st, int32_t id)
{
    /* @bin 0x004A43F0 the boat's formation (+0x92) when all its soldiers (formation +0x24 of
     * them, ids from +0x04) not dying have arrived (+0x11 = 8); else 0 */
    int32_t fmi = U8(st, FIG(id) + FIG_U92);
    if (fmi <= 0) {
        return 0;
    }
    uint32_t fm = FORMATION(fmi);
    int32_t n = U8(st, fm - FM_TYPE + FM_COUNT), ready = 0;
    for (int32_t i = 0; i < n; i++) {
        int16_t s = S16(st, fm - FM_TYPE + FM_FIGURES + 2u * (uint32_t) i);
        if (s == 0) {
            break;
        }
        uint32_t f = FIG(s);
        if (U8(st, f + FIG_ACTION) != 4 && U8(st, f + FIG_DIRECTION) == 8) {
            ready++;
        }
    }
    return ready == n ? fmi : 0;
}

void formation_embark(game_state *st, int32_t fmi)
{
    /* @bin 0x004A4470 the formation's soldiers board (+0x00 = 0, action 0x12, the action before
     * kept at +0x10), off the map; the byte before the formation record is cleared */
    uint32_t fm = FORMATION(fmi);
    U8(st, fm - FM_TYPE + FM_U03) = 0;
    for (int i = 0; i < 16; i++) {
        int16_t s = S16(st, fm - FM_TYPE + FM_FIGURES + 2u * (uint32_t) i);
        if (s == 0) {
            break;
        }
        uint32_t f = FIG(s);
        if (U8(st, f + FIG_ACTION) == 4) {
            continue;
        }
        U8(st, f + FIG_U00) = 0;
        U8(st, f + FIG_ACTION_BEFORE_ATTACK) = U8(st, f + FIG_ACTION);
        U8(st, f + FIG_ACTION) = 0x12;
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, s);
        GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, s);
    }
}

void formation_land(game_state *st, int32_t fmi, int32_t x, int32_t y)
{
    /* @bin 0x004A44F0 the formation's soldiers land on (x, y), back to their action before
     * (+0x10) */
    uint32_t fm = FORMATION(fmi);
    for (int i = 0; i < 16; i++) {
        int16_t s = S16(st, fm - FM_TYPE + FM_FIGURES + 2u * (uint32_t) i);
        if (s == 0) {
            break;
        }
        uint32_t f = FIG(s);
        if (U8(st, f + FIG_ACTION) == 4) {
            continue;
        }
        S32(st, f + FIG_GRID_OFFSET) = (int32_t) (U32(st, G_MAP_START) + 228u * (uint32_t) y + (uint32_t) x);
        U16(st, f + FIG_PREV_X) = (uint16_t) x;
        U16(st, f + FIG_X) = (uint16_t) x;
        U16(st, f + FIG_PREV_Y) = (uint16_t) y;
        U16(st, f + FIG_Y) = (uint16_t) y;
        U8(st, f + FIG_ACTION) = U8(st, f + FIG_ACTION_BEFORE_ATTACK);
        U16(st, f + FIG_CC_X) = (uint16_t) (U16(st, f + FIG_X) * 15);
        U16(st, f + FIG_CC_Y) = (uint16_t) (U16(st, f + FIG_Y) * 15);
        GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, s);
    }
}

/* @bin 0x004A59C0, 0x004A5A20 the first figure of the tile's list (grid 0x00B350DC, next +0x08),
 * other than `id`, of one of the types */
static int32_t tile_figure_of(game_state *st, int32_t tile, int32_t id, const uint8_t *types, int ntypes)
{
    int32_t k = S16(st, G_FIGURE_GRID + 2u * (uint32_t) tile);
    while (k != 0) {
        uint32_t f = FIG(k);
        if (k != id) {
            uint8_t t = U8(st, f + FIG_TYPE);
            for (int i = 0; i < ntypes; i++) {
                if (t == types[i]) {
                    return k;
                }
            }
        }
        k = S16(st, f + FIG_NEXT_ON_TILE);
    }
    return 0;
}

int32_t tile_player_boat(game_state *st, int32_t tile, int32_t id)
{
    /* @bin 0x004A59C0 types 0x4D, 0x4E, 0x19 */
    static const uint8_t t[3] = {0x4D, 0x4E, 0x19};
    return tile_figure_of(st, tile, id, t, 3);
}

int32_t tile_enemy_boat(game_state *st, int32_t tile, int32_t id)
{
    /* @bin 0x004A5A20 types 0x5C, 0x5D, 0x65, 0x64 */
    static const uint8_t t[4] = {0x5C, 0x5D, 0x65, 0x64};
    return tile_figure_of(st, tile, id, t, 4);
}

int32_t boat_nearest(game_state *st, int32_t id, int32_t range, int32_t player_boats)
{
    /* @bin 0x004A5560 in the range x range square around the boat, the nearest boat (enemy, or
     * the player's with the flag), by squared distance (less than 500000) */
    uint32_t f = FIG(id);
    int32_t h = range / 2;
    int32_t x = S16(st, f + FIG_X), y = S16(st, f + FIG_Y);
    uint32_t row = U32(st, G_MAP_START) + 228u * (uint32_t) y - 229u * (uint32_t) h + (uint32_t) x;
    int32_t best = 0, bestd = 0x7A120;
    if (range <= 0) {
        return 0;
    }
    for (int32_t r = 0; r < range; r++, row += 228u) {
        for (int32_t c = 0; c < range; c++) {
            int32_t tile = (int32_t) (row + (uint32_t) c);
            int32_t k = player_boats ? tile_player_boat(st, tile, id) : tile_enemy_boat(st, tile, id);
            if (k == 0) {
                continue;
            }
            uint32_t g = FIG(k);
            int32_t dy = S16(st, g + FIG_Y) - S16(st, f + FIG_Y);
            int32_t dx = S16(st, g + FIG_X) - S16(st, f + FIG_X);
            int32_t d = (int32_t) ((uint32_t) dx * (uint32_t) dx + (uint32_t) dy * (uint32_t) dy);
            if (d < bestd) {
                best = k;
                bestd = d;
            }
        }
    }
    return best;
}

void boat_ram_sound(game_state *st, int32_t k)
{
    /* @bin 0x004A5880 k = 1 .. 5: sound 0x0B .. 0x0F (0x0053B090) */
    if (k >= 1 && k <= 5) {
        GAME_CALL(st, FN_UI_SOUND, 0x0A + k);
    }
}

int32_t boat_ahead(game_state *st, int32_t id)
{
    /* @bin 0x004A5A80 an enemy boat on the boat's tile, else on the next tile in its direction */
    static const int32_t step[8] = {-228, -227, 1, 229, 228, 227, -1, -229};
    uint32_t f = FIG(id);
    int32_t tile = (int32_t) (U32(st, G_MAP_START) + (uint32_t) (int32_t) S16(st, f + FIG_X) +
                              228u * (uint32_t) (int32_t) S16(st, f + FIG_Y));
    int32_t d = S8(st, f + FIG_DIRECTION);
    if ((uint32_t) d > 7) {
        return 0;
    }
    int32_t k = tile_enemy_boat(st, tile, id);
    if (k != 0) {
        return k;
    }
    return tile_enemy_boat(st, tile + step[d], id);
}


int32_t warship_target_at(game_state *st, int32_t x, int32_t y, int32_t range)
{
    /* @bin 0x004BCB10 the best target for a warship at (x, y): enemy soldiers (types 0x2B .. 0x2D,
     * 0x36 .. 0x38, 0x63) within the current figure's range (priority 17), enemy ships 0x5D, 0x64
     * (15), 0x5C, 0x65 (25 with +0x130, else 10); the highest priority minus distance; its tile in
     * FOUND_X, FOUND_Y */
    S32(st, G_BEST_DISTANCE) = range;
    int32_t best = 0, best_prio = 0;
    for (int32_t k = 1; k < FIGURE_COUNT; k++) {
        uint32_t g = FIG(k);
        if (U8(st, g + FIG_STATE) != 1 || U8(st, g + FIG_ACTION) == 4) {
            continue;
        }
        uint8_t t = U8(st, g + FIG_TYPE);
        int32_t prio;
        int kind;
        if ((t >= 0x2B && t <= 0x2D) || (t >= 0x36 && t <= 0x38) || t == 0x63) {
            kind = 0;
        } else if (t == 0x5C || t == 0x65) {
            kind = 1;
        } else if (t == 0x5D || t == 0x64) {
            kind = 2;
        } else {
            continue;
        }
        int32_t d = tile_distance(st, x, y, S16(st, g + FIG_X), S16(st, g + FIG_Y));
        if (kind == 0) {
            uint8_t ct = U8(st, FIG(S32(st, G_CURRENT_FIGURE)) + FIG_TYPE);
            if (d > S32(st, G_PROPS + FIGTYPE_RANGE + 56u * ct)) {
                continue;
            }
            prio = 0x11;
        } else if (kind == 1) {
            prio = U8(st, g + FIG_U130) != 0 ? 0x19 : 0x0A;
        } else {
            prio = 0x0F;
        }
        if (prio - d > best_prio - S32(st, G_BEST_DISTANCE)) {
            S32(st, G_BEST_DISTANCE) = d;
            best = k;
            best_prio = prio;
        }
    }
    if (best == 0) {
        return 0;
    }
    uint32_t g = FIG(best);
    S32(st, G_ROAD_ACCESS_Y) = S16(st, g + FIG_Y);
    S32(st, G_ROAD_ACCESS_X) = S16(st, g + FIG_X);
    return best;
}

int32_t warship_target(game_state *st, int32_t id, int32_t range)
{
    /* @bin 0x004BCAE0 */
    uint32_t f = FIG(id);
    return warship_target_at(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), range);
}

int32_t enemy_warship_at(game_state *st, int32_t x, int32_t y, int32_t range)
{
    /* @bin 0x004BCD90 the nearest enemy warship (type 0x5D) closer than range that 0x004B4420
     * accepts; its tile in FOUND_X, FOUND_Y */
    S32(st, G_BEST_DISTANCE) = range;
    int32_t best = 0;
    for (int32_t k = 1; k < FIGURE_COUNT; k++) {
        uint32_t g = FIG(k);
        if (U8(st, g + FIG_STATE) != 1 || U8(st, g + FIG_ACTION) == 4 || U8(st, g + FIG_TYPE) != 0x5D) {
            continue;
        }
        int32_t gx = S16(st, g + FIG_X), gy = S16(st, g + FIG_Y);
        int32_t dx = absdiff(x, gx);
        S32(st, G_DIST_X) = dx;
        int32_t dy = absdiff(y, gy);
        S32(st, G_DIST_Y) = dy;
        int32_t d = dx >= dy ? dx : dy;
        if (d >= S32(st, G_BEST_DISTANCE)) {
            continue;
        }
        if (GAME_CALL(st, FN_LINE_OF_FIRE_CLEAR, x, y, gx, gy) != 0) {
            S32(st, G_BEST_DISTANCE) = d;
            best = k;
        }
    }
    if (best == 0) {
        return 0;
    }
    uint32_t g = FIG(best);
    S32(st, G_ROAD_ACCESS_Y) = S16(st, g + FIG_Y);
    S32(st, G_ROAD_ACCESS_X) = S16(st, g + FIG_X);
    return best;
}

int32_t enemy_warship(game_state *st, int32_t id, int32_t range)
{
    /* @bin 0x004BCD60 */
    uint32_t f = FIG(id);
    return enemy_warship_at(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), range);
}

void boat_unstick(game_state *st, uint32_t self, int32_t id)
{
    /* @bin 0x004BE4A0 (thiscall on 0x00D05720) a boat on a tile that is not water (grid
     * 0x00ED4140 negative) moves to the nearest free water tile (0x004467A0) */
    (void) self;
    uint32_t f = FIG(id);
    if (S8(st, G_GRID_ROUTING_WATER + U32(st, f + FIG_GRID_OFFSET)) >= 0) {
        return;
    }
    int32_t x = S16(st, f + FIG_X), y = S16(st, f + FIG_Y);
    spiral_free_water(st, &x, &y);
    int32_t dx = x - S16(st, f + FIG_X), dy = y - S16(st, f + FIG_Y);
    if (dx == 0 && dy == 0) {
        return;
    }
    GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, id);
    U16(st, f + FIG_CC_X) = (uint16_t) (U16(st, f + FIG_CC_X) + (uint16_t) (dx << 4));
    U16(st, f + FIG_X) = (uint16_t) x;
    U16(st, f + FIG_CC_Y) = (uint16_t) (U16(st, f + FIG_CC_Y) + (uint16_t) (dy << 4));
    U16(st, f + FIG_Y) = (uint16_t) y;
    S32(st, f + FIG_GRID_OFFSET) = (int32_t) (U32(st, f + FIG_GRID_OFFSET) + (uint32_t) dx + 228u * (uint32_t) dy);
    GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, id);
}

int32_t dock_battle_value(game_state *st, int32_t building)
{
    /* @bin 0x004069E0 a dock (type 0x4A) in use with a battle (+0xCB): the battle record's +0x0C
     * (0x009783B0 + 28 * battle); else -1 */
    uint32_t b = BUILDING_ADDR(building);
    if (U16(st, b + BLD_TYPE) != 0x4A || U8(st, b + BLD_STATE) != 1) {
        return -1;
    }
    uint8_t k = U8(st, b + BLD_UCB);
    if (k == 0) {
        return -1;
    }
    return S32(st, G_DOCK_BATTLES_U0C + 28u * k);
}

int32_t best_dock(game_state *st, int32_t id)
{
    /* @bin 0x00406A30 among the docks in use with workers (+0x40), the lowest battle value
     * (below 200), ties to the nearest (squared distance) */
    uint32_t f = FIG(id);
    int32_t best = 0, bestv = 0xC8, bestd = 100000;
    for (int32_t k = 1; k < BUILDING_COUNT; k++) {
        uint32_t b = BUILDING_ADDR(k);
        if (U8(st, b + BLD_STATE) != 1 || U16(st, b + BLD_TYPE) != 0x4A || U16(st, b + BLD_WORKERS) == 0) {
            continue;
        }
        int32_t v = dock_battle_value(st, k);
        if (v == -1) {
            continue;
        }
        if (v > bestv) {
            continue;
        }
        int32_t dy = S16(st, b + BLD_Y) - S16(st, f + FIG_Y);
        int32_t dx = S16(st, b + BLD_X) - S16(st, f + FIG_X);
        int32_t d = (int32_t) ((uint32_t) dx * (uint32_t) dx + (uint32_t) dy * (uint32_t) dy);
        if (v < bestv) {
            best = k;
            bestv = v;
            bestd = d;
        } else if (d < bestd) {
            bestd = d;
            best = k;
            bestv = v;
        }
    }
    return best;
}

int32_t battle_add_boat(game_state *st, uint32_t self, int32_t id)
{
    /* @bin 0x004F68F0 (thiscall) forwards to 0x004F6900 on the same object */
    return GAME_CALLT(st, self, FN_RING_PUSH_4F6900, id);
}

/* ---- shared ---------------------------------------------------------------------------------- */

int32_t figure_next_direction(game_state *st, int32_t id)
{
    /* @bin 0x004B3340 (not in the Ghidra export) the figure's next direction: on a route (+0x3A
     * the route id > 0) the step +0x3C of the route's directions (grid 0x00C8AD38, 500 per route;
     * 8 at its end +0x3E); else 8 when 0x0051CBA0 gives 8 from the tile to the destination, 10
     * otherwise */
    uint32_t f = FIG(id);
    int16_t route = S16(st, f + FIG_PATH_ID);
    if (route > 0) {
        int16_t step = S16(st, f + FIG_PATH_STEP);
        if (step >= S16(st, f + FIG_PATH_LENGTH)) {
            return 8;
        }
        return U8(st, (uint32_t) ((int32_t) G_FIGURE_PATHS + 500 * route + step));
    }
    uint8_t d = (uint8_t) GAME_CALL(st, FN_ROUTE_DIRECTION, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_DEST_X),
                                    S16(st, f + FIG_DEST_Y));
    return d == 8 ? 8 : 0x0A;
}

/* ---- table ---------------------------------------------------------------------------------- */

/* Pointer arguments: the original's callers pass their own stack; the oracle can pass state
 * addresses (read before, written after). Elsewhere these run only from their ported callers. */
static uint8_t *arg_ptr(game_state *st, int32_t a, uint32_t addr)
{
    uint8_t *p = state_ptr(st, (uint32_t) a, 4);
    if (!p) {
        state_unported(st, addr);
    }
    return p;
}

#define PTR_IN(var, a, addr) \
    int32_t var##_v = 0; \
    uint8_t *var##_p = arg_ptr(st, a, addr); \
    if (!var##_p) { \
        return 0; \
    } \
    memcpy(&var##_v, var##_p, 4);
#define PTR_OUT(var) memcpy(var##_p, &var##_v, 4);

static int32_t run_line_search_water(game_state *st, const int32_t *a)
{
    PTR_IN(x, a[4], FN_LINE_SEARCH_WATER)
    PTR_IN(y, a[5], FN_LINE_SEARCH_WATER)
    int32_t r = line_search_water(st, a[0], a[1], a[2], a[3], &x_v, &y_v);
    PTR_OUT(x)
    PTR_OUT(y)
    return r;
}

static int32_t run_line_search_land(game_state *st, const int32_t *a)
{
    PTR_IN(x, a[4], FN_LINE_SEARCH_LAND)
    PTR_IN(y, a[5], FN_LINE_SEARCH_LAND)
    int32_t r = line_search_land(st, a[0], a[1], a[2], a[3], &x_v, &y_v);
    PTR_OUT(x)
    PTR_OUT(y)
    return r;
}

static int32_t run_spiral_free_water(game_state *st, const int32_t *a)
{
    PTR_IN(x, a[0], FN_SPIRAL_FREE_WATER)
    PTR_IN(y, a[1], FN_SPIRAL_FREE_WATER)
    int32_t r = spiral_free_water(st, &x_v, &y_v);
    PTR_OUT(x)
    PTR_OUT(y)
    return r;
}

static int32_t run_spiral_free_land(game_state *st, const int32_t *a)
{
    PTR_IN(x, a[0], FN_SPIRAL_FREE_LAND)
    PTR_IN(y, a[1], FN_SPIRAL_FREE_LAND)
    int32_t r = spiral_free_land(st, &x_v, &y_v);
    PTR_OUT(x)
    PTR_OUT(y)
    return r;
}

static int32_t run_boat_flee_target(game_state *st, const int32_t *a)
{
    PTR_IN(x, a[0], FN_BOAT_FLEE_TARGET)
    PTR_IN(y, a[1], FN_BOAT_FLEE_TARGET)
    int32_t r = boat_flee_target(st, &x_v, &y_v, a[2], a[3], a[4], a[5], a[6], a[7], a[8]);
    PTR_OUT(x)
    PTR_OUT(y)
    return r;
}

static int32_t run_boat_seek_dock(game_state *st, const int32_t *a)
{
    boat_seek_dock(st, a[0]);
    return 0;
}

static int32_t run_boat_sink(game_state *st, const int32_t *a)
{
    boat_sink(st, a[0]);
    return 0;
}

static int32_t run_boat_face_enemy(game_state *st, const int32_t *a)
{
    boat_face_enemy(st, a[0]);
    return 0;
}

static int32_t run_boat_moored(game_state *st, const int32_t *a)
{
    int32_t m = 0;
    boat_moored(st, a[0], a[1], &m); /* does not use the pointer */
    return 0;
}

#define BOAT_STATE(name, fn, addr) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        PTR_IN(m, a[1], addr) \
        fn(st, a[0], &m_v); \
        PTR_OUT(m) \
        return 0; \
    }
BOAT_STATE(run_boat_go, boat_go, FN_BOAT_GO)
BOAT_STATE(run_boat_flee, boat_flee, FN_BOAT_FLEE_4A4A80)
BOAT_STATE(run_boat_to_battle, boat_to_battle, FN_BOAT_TO_BATTLE)
BOAT_STATE(run_boat_to_formation, boat_to_formation, FN_BOAT_TO_FORMATION)
BOAT_STATE(run_boat_to_landing, boat_to_landing, FN_BOAT_TO_LANDING_4A4E10)
BOAT_STATE(run_warship_moored, warship_moored, FN_WARSHIP_MOORED)
BOAT_STATE(run_warship_ram, warship_ram, FN_WARSHIP_RAM)
BOAT_STATE(run_warship_attack, warship_attack, FN_WARSHIP_ATTACK)
BOAT_STATE(run_warship_go, warship_go, FN_WARSHIP_GO)
BOAT_STATE(run_warship_flee, warship_flee, FN_WARSHIP_FLEE)
BOAT_STATE(run_warship_to_battle, warship_to_battle, FN_WARSHIP_TO_BATTLE)
BOAT_STATE(run_warship_hunt, warship_hunt, FN_WARSHIP_HUNT)

#define INT1(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) { return fn(st, a[0]); }
#define INT2(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) { return fn(st, a[0], a[1]); }
#define INT3(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) { return fn(st, a[0], a[1], a[2]); }
INT1(run_boat_at_dock, boat_at_dock)
INT2(run_boat_join_battle, boat_join_battle)
INT1(run_boat_formation_ready, boat_formation_ready)
INT2(run_tile_player_boat, tile_player_boat)
INT2(run_tile_enemy_boat, tile_enemy_boat)
INT3(run_boat_nearest, boat_nearest)
INT1(run_boat_ahead, boat_ahead)
INT3(run_warship_target_at, warship_target_at)
INT2(run_warship_target, warship_target)
INT3(run_enemy_warship_at, enemy_warship_at)
INT2(run_enemy_warship, enemy_warship)

static int32_t run_boat_unstick(game_state *st, const int32_t *a)
{
    boat_unstick(st, G_UD05720, a[0]);
    return 0;
}

INT1(run_dock_battle_value, dock_battle_value)
INT1(run_best_dock, best_dock)

static int32_t run_battle_add_boat(game_state *st, const int32_t *a)
{
    return battle_add_boat(st, (uint32_t) a[PORT_THIS_ARG], a[0]);
}

static int32_t run_formation_embark(game_state *st, const int32_t *a)
{
    formation_embark(st, a[0]);
    return 0;
}

static int32_t run_formation_land(game_state *st, const int32_t *a)
{
    formation_land(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_boat_ram_sound(game_state *st, const int32_t *a)
{
    boat_ram_sound(st, a[0]);
    return 0;
}

static int32_t run_next_direction(game_state *st, const int32_t *a)
{
    return figure_next_direction(st, a[0]);
}

static int32_t run_warship_fire(game_state *st, const int32_t *a)
{
    warship_fire(st, a[0]);
    return 0;
}

static int32_t run_warship_face_enemy(game_state *st, const int32_t *a)
{
    warship_face_enemy(st, a[0]);
    return 0;
}

const ported_function figc2_ports[] = {
    {0x0040C180u, "line_search_water", 6, 32, 0, run_line_search_water, 0},
    {0x0040BDA0u, "line_search_land", 6, 32, 0, run_line_search_land, 0},
    {0x004467A0u, "spiral_free_water", 2, 8, 0, run_spiral_free_water, 0},
    {0x004AD880u, "spiral_free_land", 2, 8, 0, run_spiral_free_land, 0},
    {0x004A4850u, "boat_flee_target", 9, 8, 0, run_boat_flee_target, 0},
    {0x004A45D0u, "boat_seek_dock", 1, 0, 0, run_boat_seek_dock, 0},
    {0x004A46F0u, "boat_sink", 1, 0, 0, run_boat_sink, 0},
    {0x004A4770u, "boat_moored", 3, 0, 0, run_boat_moored, 0},
    {0x004A4950u, "boat_go", 2, 0, 0, run_boat_go, 0},
    {0x004A4A80u, "boat_flee", 2, 0, 0, run_boat_flee, 0},
    {0x004A4C40u, "boat_to_battle", 2, 0, 0, run_boat_to_battle, 0},
    {0x004A4D00u, "boat_to_formation", 2, 0, 0, run_boat_to_formation, 0},
    {0x004A4E10u, "boat_to_landing", 2, 0, 0, run_boat_to_landing, 0},
    {0x004A4F60u, "boat_face_enemy", 1, 0, 0, run_boat_face_enemy, 0},
    {0x004A58E0u, "warship_moored", 2, 0, 0, run_warship_moored, 0},
    {0x004A6160u, "warship_ram", 2, 0, 0, run_warship_ram, 0},
    {0x004A6300u, "warship_attack", 2, 0, 0, run_warship_attack, 0},
    {0x004A6890u, "warship_go", 2, 0, 0, run_warship_go, 0},
    {0x004A6970u, "warship_flee", 2, 0, 0, run_warship_flee, 0},
    {0x004A6B40u, "warship_to_battle", 2, 0, 0, run_warship_to_battle, 0},
    {0x004A6BF0u, "warship_fire", 1, 0, 0, run_warship_fire, 0},
    {0x004A6CC0u, "warship_face_enemy", 1, 0, 0, run_warship_face_enemy, 0},
    {0x004A6D40u, "warship_hunt", 2, 0, 0, run_warship_hunt, 0},
    {0x004B3340u, "figure_next_direction", 1, 32, 0, run_next_direction, 0},
    {0x004A4300u, "boat_at_dock", 1, 8, 0, run_boat_at_dock, 0},
    {0x004A4340u, "boat_join_battle", 2, 8, 0, run_boat_join_battle, 0},
    {0x004A43F0u, "boat_formation_ready", 1, 32, 0, run_boat_formation_ready, 0},
    {0x004A4470u, "formation_embark", 1, 0, 0, run_formation_embark, 0},
    {0x004A44F0u, "formation_land", 3, 0, 0, run_formation_land, 0},
    {0x004A59C0u, "tile_player_boat", 2, 32, 0, run_tile_player_boat, 0},
    {0x004A5A20u, "tile_enemy_boat", 2, 32, 0, run_tile_enemy_boat, 0},
    {0x004A5560u, "boat_nearest", 3, 32, 0, run_boat_nearest, 0},
    {0x004A5880u, "boat_ram_sound", 1, 0, 0, run_boat_ram_sound, 0},
    {0x004A5A80u, "boat_ahead", 1, 32, 0, run_boat_ahead, 0},
    {0x004BCB10u, "warship_target_at", 3, 32, 0, run_warship_target_at, 0},
    {0x004BCAE0u, "warship_target", 2, 32, 0, run_warship_target, 0},
    {0x004BCD90u, "enemy_warship_at", 3, 32, 0, run_enemy_warship_at, 0},
    {0x004BCD60u, "enemy_warship", 2, 32, 0, run_enemy_warship, 0},
    {0x004BE4A0u, "boat_unstick", 1, 0, G_UD05720, run_boat_unstick, 0},
    {0x004069E0u, "dock_battle_value", 1, 32, 0, run_dock_battle_value, 0},
    {0x00406A30u, "best_dock", 1, 32, 0, run_best_dock, 0},
    {0x004F68F0u, "battle_add_boat", 1, 32, 0, run_battle_add_boat, 0},
};
const int figc2_ports_count = sizeof(figc2_ports) / sizeof(figc2_ports[0]);
