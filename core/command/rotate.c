/* Imhotep - rotating the city view: the view table, the camera and the orientation-dependent grids.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Called by the rotate command 0x00409EB0 (docs/commands.md section 3.9). The view table 0x0070E494
 * (0x1C9 rows of 0xE7 i32) maps view positions to grid offsets for the current orientation. */
#include "command/orders.h"

#include "sim/call.h"
#include "sim/platform.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define VIEW_ROW 0xE7u
#define VIEW_ROWS 0x1C9

int32_t view_width(game_state *st) { return S32(st, G_VIEW_ROWS); }   /* @bin 0x00430CB0 (thunk 0x004DDEB0) */
int32_t view_height(game_state *st) { return S32(st, G_VIEW_COLS); }  /* @bin 0x00430CA0 (thunk 0x004DDEA0) */

static void view_table_clear(game_state *st)
{
    /* @bin 0x00430480 every entry := 0x00F56B44 + 0xFFF0000; bounds 0x0070E420..0x0070E42C reset */
    uint32_t empty = U32(st, G_VIEW_EMPTY_BASE) + 0xFFF0000u;
    S32(st, G_VIEW_COL_MAX) = 0xE6;
    for (uint32_t i = 0; i < 0x19C5Fu; i++) {
        U32(st, G_VIEW_TABLE + 4 * i) = empty;
    }
    S32(st, G_VIEW_ROW_MAX) = 0x1C8;
    S32(st, G_VIEW_COL_MIN) = 0;
    S32(st, G_VIEW_ROW_MIN) = 0;
}

void view_table_build(game_state *st, int32_t orient)
{
    /* @bin 0x00430530 the view position of every tile for orientation 0, 2, 4 (else 6): row and
     * column start and steps by orientation; tiles with an image id >= 6 widen the bounds
     * 0x0070E420..0x0070E42C and get their grid offset in the table, others the empty value */
    view_table_clear(st);
    S32(st, G_U70E410) = 0xE4;
    S32(st, G_U70E40C) = 0xE4;
    S32(st, G_U70E418) = 0;
    S32(st, G_U70E414) = 0;
    S32(st, G_VIEW_TABLE_ORIENTATION) = orient;
    int32_t row, row_step, inner, col, col_shift, col_step;
    if (orient == 0) {
        row = 1, row_step = 1, inner = 1, col = 0xE6, col_shift = -1, col_step = 1;
    } else if (orient == 2) {
        row = 0xE6, row_step = 1, inner = -1, col = 1, col_shift = 1, col_step = 1;
    } else if (orient == 4) {
        row = 0x1C7, row_step = -1, inner = -1, col = 0xE6, col_shift = 1, col_step = -1;
    } else {
        row = 0xE6, row_step = -1, inner = 1, col = 0x1C7, col_shift = -1, col_step = -1;
    }
    S32(st, G_VIEW_COL_MIN) = 0xE6;
    S32(st, G_VIEW_ROW_MIN) = 0x1C8;
    S32(st, G_VIEW_COL_MAX) = 0;
    S32(st, G_VIEW_ROW_MAX) = 0;
    int32_t tile_base = 0;
    int32_t row_base = row * (int32_t) VIEW_ROW;
    uint32_t img = G_GRID_IMAGE;
    while (img < G_GRID_IMAGE + 4u * GRID_TILES) {
        int32_t r = row, c = col, base = row_base;
        for (int32_t k = 0; k < 0xE4; k++, img += 4) {
            int32_t half = c / 2;
            if (S32(st, img) >= 6) {
                if (S32(st, G_VIEW_COL_MIN) >= half) {
                    S32(st, G_VIEW_COL_MIN) = half;
                }
                if (S32(st, G_VIEW_ROW_MIN) >= r) {
                    S32(st, G_VIEW_ROW_MIN) = r;
                }
                if (S32(st, G_VIEW_COL_MAX) <= half) {
                    S32(st, G_VIEW_COL_MAX) = half;
                }
                if (S32(st, G_VIEW_ROW_MAX) <= r) {
                    S32(st, G_VIEW_ROW_MAX) = r;
                }
                if (r >= 0 && half >= 0 && r < VIEW_ROWS && half < (int32_t) VIEW_ROW) {
                    S32(st, G_VIEW_TABLE + 4u * (uint32_t) (half + base)) = tile_base + k;
                }
            } else if (r >= 0 && half >= 0 && r < VIEW_ROWS && half < (int32_t) VIEW_ROW) {
                U32(st, G_VIEW_TABLE + 4u * (uint32_t) (half + base)) = U32(st, G_VIEW_EMPTY_BASE) + 0xFFF0000u;
            }
            base += inner * (int32_t) VIEW_ROW;
            r += inner;
            c += col_step;
        }
        row += row_step;
        row_base += row_step * (int32_t) VIEW_ROW;
        col += col_shift;
        tile_base += 0xE4;
    }
}

void view_find(game_state *st, uint32_t value)
{
    /* @bin 0x004304C0 the first view position holding grid offset `value` (entries below
     * 0xFFF0000, unsigned): row 0x00D3B4E0, column 0x00D3B4E4 (0, 0 if none) */
    S32(st, G_ROAD_ACCESS_Y) = 0;
    S32(st, G_ROAD_ACCESS_X) = 0;
    for (uint32_t row = 0; row < (uint32_t) VIEW_ROWS; row++) {
        for (uint32_t col = 0; col < VIEW_ROW; col++) {
            uint32_t v = U32(st, G_VIEW_TABLE + 4 * (row * VIEW_ROW + col));
            if (v < 0xFFF0000u && v == value) {
                S32(st, G_ROAD_ACCESS_Y) = (int32_t) row;
                S32(st, G_ROAD_ACCESS_X) = (int32_t) col;
                return;
            }
        }
    }
}

/* @bin 0x00430B26 the grid offset at the centre of the view */
static int32_t view_centre(game_state *st)
{
    int32_t i = (S32(st, G_VIEW_ROWS) / 2 + S32(st, G_CAMERA_Y)) * (int32_t) VIEW_ROW + S32(st, G_CAMERA_X) + S32(st, G_VIEW_COLS) / 2;
    return S32(st, G_VIEW_TABLE + 4u * (uint32_t) i);
}

void rotate_view(game_state *st, int dir)
{
    /* @bin 0x00430B20 (dir +1: orientation + 2, 6 -> 0) / 0x00430BE0 (dir -1: - 2, 0 -> 6): the
     * table is rebuilt and the camera moved to keep the centre tile (+1 on x / +2 on y for the
     * orientations 0 and 4) */
    int32_t centre = view_centre(st);
    int32_t o;
    if (dir > 0) {
        o = S32(st, G_CITY_ORIENTATION) + 2;
        S32(st, G_CITY_ORIENTATION) = o;
        if (o > 6) {
            o = 0;
            S32(st, G_CITY_ORIENTATION) = 0;
        }
    } else {
        o = S32(st, G_CITY_ORIENTATION) - 2;
        S32(st, G_CITY_ORIENTATION) = o;
        if (o < 0) {
            S32(st, G_CITY_ORIENTATION) = 6;
        }
        o = S32(st, G_CITY_ORIENTATION);
    }
    view_table_build(st, o);
    if (centre >= 0xFFF0000) {
        return;
    }
    view_find(st, (uint32_t) centre);
    int32_t x = S32(st, G_ROAD_ACCESS_X) - S32(st, G_VIEW_COLS) / 2;
    S32(st, G_CAMERA_X) = x;
    int32_t y = S32(st, G_ROAD_ACCESS_Y) - S32(st, G_VIEW_ROWS) / 2;
    S32(st, G_CAMERA_Y) = y;
    int32_t now = S32(st, G_CITY_ORIENTATION);
    if (now == 4 || now == 0) {
        if (dir > 0) {
            S32(st, G_CAMERA_X) = x + 1;
        } else {
            S32(st, G_CAMERA_Y) = y + 2;
        }
    }
}

static int enhanced_view(game_state *st)
{
    /* @imhotep enhanced fit: a view the original's sizes never have: a zoom, or a fitted screen (always
     * wider than 1024 unless 1024 x 768: core/sim/platform.c platform_display_fit) */
    return platform_display_zoom(st) != 100 || S32(st, G_SCREEN_W) > 0x400;
}

void camera_clamp(game_state *st)
{
    /* @bin 0x004DE080 the camera kept inside the view bounds 0x00E94DC4..0x00E94DD0 (widened in
     * the editor with 0x006FDC50); y even; a change asks for a redraw in the editor only */
    int32_t x0 = S32(st, G_CAMERA_X_MIN), y0 = S32(st, G_CAMERA_Y_MIN);
    int32_t x1 = S32(st, G_CAMERA_X_MAX), y1 = S32(st, G_CAMERA_Y_MAX);
    int moved = 0;
    if (S32(st, G_U6FDC50) != 0 && S32(st, G_EDITOR) == 1) {
        x0 -= 2, y0 -= 5, x1 += 3, y1 += 5;
    }
    if (S32(st, G_CAMERA_X) < x0) {
        S32(st, G_CAMERA_X) = x0;
        moved = 1;
    }
    if (S32(st, G_CAMERA_Y) < y0) {
        S32(st, G_CAMERA_Y) = y0;
        moved = 1;
    }
    if (view_height(st) + S32(st, G_CAMERA_X) > x1) {
        S32(st, G_CAMERA_X) = x1 - view_height(st);
        moved = 1;
    }
    int enhanced = enhanced_view(st);
    if (enhanced && S32(st, G_CAMERA_X) < x0) {
        /* @imhotep enhanced zoom: a view taller than the map (zoomed out, or a large screen) shows the
         * map in its middle; the original's sizes keep its clamp */
        S32(st, G_CAMERA_X) = (x0 + x1 - view_height(st)) / 2;
    }
    int32_t y = S32(st, G_CAMERA_Y);
    if (view_width(st) + y > y1) {
        y = y1 - view_width(st);
        moved = 1;
    }
    if (enhanced && y < y0) {
        y = (y0 + y1 - view_width(st)) / 2; /* @imhotep enhanced zoom: as above, wider than the map */
    }
    if (moved && S32(st, G_EDITOR) == 1) {
        U8(st, G_REDRAW) = 1;
    }
    S32(st, G_CAMERA_Y) = (int32_t) ((uint32_t) y & 0xFFFEu);
}

static int32_t tile_outside(game_state *st, int32_t off)
{
    /* @bin 0x0047E830 (signed / and %) */
    return (uint8_t) GAME_CALL(st, FN_MAP_TILE_INSIDE, off % 0xE4, off / 0xE4) == 0;
}

static void view_bounds(game_state *st)
{
    /* @bin 0x004DDF90 the first and last view rows / columns that hold a map tile: scans of the
     * table's row 0xE4 (0x00741B84..0x00741F1C) from both ends and of its column 0x73 (stride
     * 0x738, 0x0070E660..0x00774D08) from both ends */
    int32_t i = 0;
    uint32_t p = G_VIEW_TABLE + 4u * (VIEW_ROW * 0xE4u);
    int32_t v = S32(st, p);
    while (tile_outside(st, v)) {
        v = S32(st, p + 4);
        p += 4;
        i++;
        if (p >= G_VIEW_TABLE + 4u * (VIEW_ROW * 0xE4u + 0xE6u)) {
            break;
        }
    }
    S32(st, G_CAMERA_X_MIN) = i + 1;
    i = 0xE6;
    p = G_VIEW_TABLE + 4u * (VIEW_ROW * 0xE4u + 0xE6u);
    v = S32(st, p);
    while (tile_outside(st, v)) {
        v = S32(st, p - 4);
        p -= 4;
        i--;
        if (p <= G_VIEW_TABLE + 4u * (VIEW_ROW * 0xE4u)) {
            break;
        }
    }
    S32(st, G_CAMERA_X_MAX) = i - 1;
    i = 0;
    p = G_VIEW_TABLE + 4u * 0x73u;
    v = S32(st, p);
    while (tile_outside(st, v)) {
        v = S32(st, p + 0x738);
        p += 0x738;
        i += 2;
        if (p >= G_VIEW_TABLE + 4u * (VIEW_ROW * 0x1C8u + 0x73u)) {
            break;
        }
    }
    S32(st, G_CAMERA_Y_MIN) = i + 1;
    i = 0x1C6;
    p = G_VIEW_TABLE + 4u * (VIEW_ROW * 0x1C6u + 0x73u);
    v = S32(st, p);
    while (tile_outside(st, v)) {
        v = S32(st, p - 0x738);
        p -= 0x738;
        i -= 2;
        if (p <= G_VIEW_TABLE + 4u * (VIEW_ROW + 0x73u)) {
            break;
        }
    }
    S32(st, G_CAMERA_Y_MIN) = (int32_t) (U32(st, G_CAMERA_Y_MIN) & 0xFFFEu);
    S32(st, G_CAMERA_Y_MAX) = (int32_t) ((uint32_t) (i - 1) & 0xFFFEu);
}

static void edge_draw_tiles(game_state *st)
{
    /* @bin 0x0047D720 bit 0x40 of the edge grid: tiles of no building, and the tile of each
     * building (size 1..5, bitfields low nibble) whose position (edge & 0x3F) is drawn for the
     * orientation: 8 * size (0), 9 * size (6), size (4), 0 (2) */
    int32_t o = S32(st, G_CITY_ORIENTATION);
    for (int32_t row = 0; row < S32(st, G_MAP_HEIGHT); row++) {
        int32_t first = S32(st, G_ROW_FIRST + 4u * (uint32_t) row);
        int32_t end = S32(st, G_ROW_END + 4u * (uint32_t) row);
        uint32_t off = U32(st, G_ROW_OFFSET + 4u * (uint32_t) row);
        if (first >= end) {
            continue;
        }
        for (int32_t n = end - first; n > 0; n--, off++) {
            int32_t size = U8(st, G_GRID_BITFIELDS + off) & 0xF;
            uint8_t e = U8(st, G_GRID_EDGE + off);
            int32_t pos = e & 0x3F;
            if (size == 0) {
                U8(st, G_GRID_EDGE + off) = (uint8_t) (e | 0x40);
                continue;
            }
            e &= 0xBF;
            U8(st, G_GRID_EDGE + off) = e;
            if (size > 5) {
                continue;
            }
            int draw = o == 0 ? pos == 8 * size : o == 6 ? pos == 9 * size : o == 4 ? pos == size : o == 2 ? pos == 0 : 0;
            if (draw) {
                U8(st, G_GRID_EDGE + off) = (uint8_t) (e | 0x40);
            }
        }
    }
}

static void bridge_sprites_rotate(game_state *st)
{
    /* @bin 0x0047FB70 sprite grid of the terrain-bit-4 tiles: 2 <-> 5, and 1, 3, 4, 6 turned one
     * way when 0x00BC96C8 = 1 (1 -> 6, 3 -> 4, 4 -> 1, 6 -> 3), the other way otherwise */
    int32_t last = S32(st, G_ROTATE_LAST);
    for (int32_t row = 0; row < S32(st, G_MAP_HEIGHT); row++) {
        int32_t first = S32(st, G_ROW_FIRST + 4u * (uint32_t) row);
        int32_t end = S32(st, G_ROW_END + 4u * (uint32_t) row);
        uint32_t off = U32(st, G_ROW_OFFSET + 4u * (uint32_t) row);
        if (first >= end) {
            continue;
        }
        for (int32_t n = end - first; n > 0; n--, off++) {
            if (!(U8(st, G_GRID_TERRAIN + 4 * off) & 4)) {
                continue;
            }
            uint8_t s = U8(st, G_GRID_SPRITE + off);
            int one = last == 1;
            switch (s) {
            case 1:
                U8(st, G_GRID_SPRITE + off) = one ? 6 : 4;
                break;
            case 2:
                U8(st, G_GRID_SPRITE + off) = 5;
                break;
            case 3:
                U8(st, G_GRID_SPRITE + off) = one ? 4 : 6;
                break;
            case 4:
                U8(st, G_GRID_SPRITE + off) = one ? 1 : 3;
                break;
            case 5:
                U8(st, G_GRID_SPRITE + off) = 2;
                break;
            case 6:
                U8(st, G_GRID_SPRITE + off) = one ? 3 : 1;
                break;
            default:
                break;
            }
        }
    }
}

static void figures_0x2a_rotate(game_state *st)
{
    /* @bin 0x00497B90 figures of type 0x2A on a tile set in 0x00EBAB20: back to their building's
     * tile (action 8) when 0x00480060 finds no free tile near them, else moved to the tile it found
     * (0x00D3B4E4, 0x00D3B4E0) */
    for (int32_t id = 1; id < 2000; id++) {
        uint32_t f = FIGURE_ADDR(id);
        if (U8(st, f + FIG_TYPE) != 0x2A || U8(st, G_EBAB20 + U32(st, f + FIG_GRID_OFFSET)) == 0) {
            continue;
        }
        if (GAME_CALL(st, FN_MAP_FREE_TILE_NEAR, S16(st, f + FIG_X), S16(st, f + FIG_Y), 1, 2) == 0) {
            U8(st, f + FIG_ACTION) = 8;
            GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, id);
            uint32_t b = BUILDING_ADDR(S16(st, f + FIG_HOME));
            int16_t x = S16(st, b + BLD_X), y = S16(st, b + BLD_Y);
            S16(st, f + FIG_X) = x;
            S16(st, f + FIG_SOURCE_X) = x;
            S16(st, f + FIG_Y) = y;
            S16(st, f + FIG_SOURCE_Y) = y;
            S32(st, f + FIG_GRID_OFFSET) = (int32_t) (U32(st, G_MAP_START) + (uint32_t) (int32_t) S16(st, f + FIG_X) +
                                           (uint32_t) (int32_t) S16(st, f + FIG_Y) * 0xE4u);
            GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, id);
            U8(st, f + FIG_ACTION) = 8;
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            continue;
        }
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, S32(st, G_CURRENT_FIGURE));
        U8(st, f + FIG_PROGRESS_ON_TILE) = 0;
        GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, id);
        uint32_t x = U32(st, G_ROAD_ACCESS_X), y = U32(st, G_ROAD_ACCESS_Y);
        S16(st, f + FIG_X) = (int16_t) x;
        S16(st, f + FIG_PREV_X) = (int16_t) x;
        S16(st, f + FIG_Y) = (int16_t) y;
        S16(st, f + FIG_PREV_Y) = (int16_t) y;
        S16(st, f + FIG_CC_X) = (int16_t) (x * 15u);
        S16(st, f + FIG_CC_Y) = (int16_t) (y * 15u);
        U32(st, f + FIG_GRID_OFFSET) = x + y * 0xE4u + U32(st, G_MAP_START);
        GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, id);
        uint32_t g = FIGURE_ADDR(S32(st, G_CURRENT_FIGURE));
        U8(st, g + FIG_ACTION) = 0xB;
        S16(st, g + FIG_DEST_X) = S16(st, g + FIG_SOURCE_X);
        S16(st, g + FIG_DEST_Y) = S16(st, g + FIG_SOURCE_Y);
    }
}

void map_rotated(game_state *st)
{
    /* @bin 0x0047D6E0 after a rotation: signposts, no undo, edge flags, terrain images, building
     * images, bridges, routing, figures 0x2A, view bounds */
    GAME_CALL(st, FN_SIGNPOSTS_UPDATE, 1);
    S32(st, G_UNDO_POSSIBLE) = 0;
    edge_draw_tiles(st);
    GAME_CALL0(st, FN_TERRAIN_REBUILD);
    GAME_CALL0(st, FN_BUILDINGS_REDRAW);
    bridge_sprites_rotate(st);
    GAME_CALL0(st, FN_ROUTING_REBUILD_EBAB20);
    figures_0x2a_rotate(st);
    view_bounds(st);
}

#define V(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
V(run_clear, view_table_clear)
V(run_clamp, camera_clamp)
V(run_bounds, view_bounds)
V(run_edges, edge_draw_tiles)
V(run_bridges, bridge_sprites_rotate)
V(run_figures, figures_0x2a_rotate)
V(run_rotated, map_rotated)

static int32_t run_build(game_state *st, const int32_t *a)
{
    view_table_build(st, a[0]);
    return 0;
}

static int32_t run_find(game_state *st, const int32_t *a)
{
    view_find(st, (uint32_t) a[0]);
    return 0;
}

static int32_t run_cw(game_state *st, const int32_t *a)
{
    (void) a;
    rotate_view(st, 1);
    return 0;
}

static int32_t run_ccw(game_state *st, const int32_t *a)
{
    (void) a;
    rotate_view(st, -1);
    return 0;
}

static int32_t run_w(game_state *st, const int32_t *a)
{
    (void) a;
    return view_width(st);
}

static int32_t run_h(game_state *st, const int32_t *a)
{
    (void) a;
    return view_height(st);
}

static int32_t run_outside(game_state *st, const int32_t *a) { return tile_outside(st, a[0]); }

const ported_function rotate_ports[] = {
    {0x00430480u, "view_table_clear", 0, 0, 0, run_clear, 0},
    {0x00430530u, "view_table_build", 1, 0, 0, run_build, 0},
    {0x004304C0u, "view_find", 1, 0, 0, run_find, 0},
    {0x00430B20u, "rotate_view_cw", 0, 0, 0, run_cw, 0},
    {0x00430BE0u, "rotate_view_ccw", 0, 0, 0, run_ccw, 0},
    {0x00430CA0u, "view_height", 0, 32, 0, run_h, 0},
    {0x00430CB0u, "view_width", 0, 32, 0, run_w, 0},
    {0x004DDEA0u, "view_height (thunk)", 0, 32, 0, run_h, 0},
    {0x004DDEB0u, "view_width (thunk)", 0, 32, 0, run_w, 0},
    {0x004DE080u, "camera_clamp", 0, 0, 0, run_clamp, 0},
    {0x004DDF90u, "view_bounds", 0, 0, 0, run_bounds, 0},
    {0x0047E830u, "tile_outside", 1, 32, 0, run_outside, 0},
    {0x0047D720u, "edge_draw_tiles", 0, 0, 0, run_edges, 0},
    {0x0047FB70u, "bridge_sprites_rotate", 0, 0, 0, run_bridges, 0},
    {0x00497B90u, "figures_0x2a_rotate", 0, 0, 0, run_figures, 0},
    {0x0047D6E0u, "map_rotated", 0, 0, 0, run_rotated, 0},
};
const int rotate_ports_count = sizeof(rotate_ports) / sizeof(rotate_ports[0]);
