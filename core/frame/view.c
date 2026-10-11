/* Imhotep - the city view's tile and screen coordinates.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "frame/view.h"

#include "sim/platform.h"
#include "state/layout.h"
#include "state/ported.h"


static int32_t cell(game_state *st, int32_t row, int32_t col)
{
    return S32(st, G_VIEW_TABLE + 4u * (uint32_t) (row * 0xE7 + col));
}

void view_cell_tile(game_state *st, int32_t p1, int32_t p2, int32_t *out3, int32_t *out4)
{
    /* @bin 0x004F0AB9 per orientation (0, 2, 4, else): the line through the cell, solved for the
     * tile (two signed divisions; the first divisor is always 2) */
    int32_t r = S32(st, G_VIEW_ORIENTATION);
    int32_t a, b, c, d, e, f;
    if (r == 0) {
        a = 2 * S32(st, G_U70E418) + 1, b = 0xE6, c = -1, d = 1, e = 1, f = 1;
    } else if (r == 2) {
        a = 0xE6, b = 2 * S32(st, G_U70E414) + 1, c = 1, d = -1, e = 1, f = 1;
    } else if (r == 4) {
        a = 0x1C7 - 2 * S32(st, G_U70E418), b = 0xE6, c = 1, d = -1, e = -1, f = -1;
    } else {
        a = 0xE6, b = 0x1C7 - 2 * S32(st, G_U70E418), c = -1, d = 1, e = -1, f = -1;
    }
    int32_t q = ((p2 - b) * e - (p1 - a) * c) / (f * e - c * d);
    *out4 = q;
    *out3 = (p1 - q * d - a) / e;
}

void view_tile_to_screen(game_state *st, int32_t tile_x, int32_t tile_y, int32_t *x, int32_t *y)
{
    /* @bin 0x004F0BA0 the camera's view cell: its tile (grid offset % 228, / 228), or 0x004F0AB0 */
    int32_t row = S32(st, G_VIEW_FIRST_ROW), col = S32(st, G_VIEW_FIRST_COL);
    int32_t c = cell(st, row, col);
    int32_t cx, cy;
    if (c < 0x0FFF0000) {
        cx = c % 0xE4;
        cy = c / 0xE4;
    } else {
        view_cell_tile(st, row, 2 * col + 1, &cy, &cx);
    }
    /* @bin 0x004F0C0D relative to the camera tile (0x00BCA2E8, 0x00BCA2EC) */
    int32_t dy = S32(st, G_VIEW_DY) - cy + tile_y;
    int32_t dx = S32(st, G_VIEW_DX) - cx + tile_x;
    int32_t w = S32(st, G_TILE_W) + 1, h = S32(st, G_TILE_H);
    switch (S32(st, G_VIEW_ORIENTATION)) { /* @bin 0x004F0D0C the jump table: 0, 2, 4, 6; others write nothing */
    case 0:
        *x = (dx - dy) * w + S32(st, G_VIEW_X);
        *y = (dx + dy - 1) * h + S32(st, G_VIEW_Y);
        break;
    case 2:
        *x = (dx + dy) * w + S32(st, G_VIEW_X);
        *y = (dy - dx - 1) * h + S32(st, G_VIEW_Y);
        break;
    case 4:
        *x = (dy - dx) * w + S32(st, G_VIEW_X);
        *y = S32(st, G_VIEW_Y) - (dx + dy + 1) * h;
        break;
    case 6:
        *x = S32(st, G_VIEW_X) - (dx + dy) * w;
        *y = (dx - dy - 1) * h + S32(st, G_VIEW_Y);
        break;
    default:
        break;
    }
}

int32_t view_screen_to_tile(game_state *st, int32_t x, int32_t y)
{
    if (!zoom_point(st, &x, &y)) { /* @imhotep enhanced zoom: the view's own pixels */
        return 0;
    }
    /* @bin 0x0043096F outside the view's pixels: 0 */
    int32_t x0 = S32(st, G_VIEW_X), y0 = S32(st, G_VIEW_Y);
    int32_t h = S32(st, G_TILE_H), w = S32(st, G_TILE_W) + 1;
    if (x < x0 || x >= (S32(st, G_U70E458) + 2) * S32(st, G_VIEW_COLS) + x0 || y < y0 || y >= h * S32(st, G_VIEW_ROWS) + y0) {
        return 0;
    }
    /* @bin 0x004309CA the cell row (y / tile height) and column (x / tile width), signed */
    int32_t row = (y - y0) / h;
    S32(st, G_VIEW_CELL_ROW) = row;
    int32_t col = (x - x0) / w;
    int32_t col_odd = col & 1, row_odd = row & 1;
    int32_t odd = (row + col) & 1;
    int32_t half = ((x - x0) % w) / 2;
    int32_t dy = (y - y0) % h;
    int32_t col2 = col / 2;
    S32(st, G_VIEW_CELL_COL) = col2;
    /* @bin 0x00430A28 the diamond's half the point is in: the next row, or the next column */
    if (odd == 0) {
        if (dy > half) {
            row++;
            S32(st, G_VIEW_CELL_ROW) = row;
        } else if (col_odd != 0 && row_odd != 0) {
            col2++;
            S32(st, G_VIEW_CELL_COL) = col2;
        }
    } else if (odd == 1 && dy + half >= h - 1) {
        row++;
        S32(st, G_VIEW_CELL_ROW) = row;
        if (col_odd != 0 && row_odd == 0) {
            col2++;
            S32(st, G_VIEW_CELL_COL) = col2;
        }
    }
    /* @bin 0x00430A79 the cell (0x0070E3F8); none: 0 */
    int32_t t = cell(st, S32(st, G_VIEW_FIRST_ROW) + row, S32(st, G_VIEW_FIRST_COL) + col2);
    S32(st, G_VIEW_CELL_ENTRY) = t;
    if (t >= 0x0FFF0000) {
        return 0;
    }
    /* @bin 0x00430AB9 edge flags: 0x0070E3FC 2 on the first row, 1 past the last; 0x0070E400 on odd
     * rows 2 in the first column, 1 past the last */
    S32(st, G_VIEW_CELL_ROW_EDGE) = 0;
    S32(st, G_VIEW_CELL_COL_EDGE) = 0;
    if (row == 0) {
        S32(st, G_VIEW_CELL_ROW_EDGE) = 2;
    } else if (row >= S32(st, G_VIEW_ROWS)) {
        S32(st, G_VIEW_CELL_ROW_EDGE) = 1;
    }
    if ((row & 1) != 0) {
        if (col2 == 0) {
            S32(st, G_VIEW_CELL_COL_EDGE) = 2;
        } else if (col2 >= S32(st, G_VIEW_COLS)) {
            S32(st, G_VIEW_CELL_COL_EDGE) = 1;
        }
    }
    return t;
}

static int32_t run_screen_to_tile(game_state *st, const int32_t *a)
{
    return view_screen_to_tile(st, a[0], a[1]);
}

const ported_function frame_view_ports[] = {
    /* their outputs are pointers into the caller's frame: called directly by ports only */
    {0x004F0BA0u, "view_tile_to_screen", 4, 0, 0, NULL, 0},
    {0x004F0AB0u, "view_cell_tile", 4, 0, 0, NULL, 0},
    {0x00430960u, "view_screen_to_tile", 3, 32, 0, run_screen_to_tile, 0},
};
const int frame_view_ports_count = sizeof(frame_view_ports) / sizeof(frame_view_ports[0]);
