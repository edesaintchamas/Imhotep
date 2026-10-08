/* Imhotep - the city view's tile and screen coordinates (the frame's sound needs them).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The view keeps (0x0070E3F8 .. 0x0070E494, written by the city view's set-up): the view-to-tile table
 * 0x0070E494 (0xE7 cells per row, a grid offset or >= 0x0FFF0000 for none), the first cell row / column
 * 0x0070E474 / 0x0070E478, its pixel origin 0x0070E490 (x) / 0x0070E48C (y), the tile width - 1
 * 0x0070E45C and height 0x0070E44C, the counts of columns 0x0070E464 / 0x0070E458 and rows 0x0070E460;
 * the camera tile 0x00BCA2E8 / 0x00BCA2EC and the orientation 0x00BCA2E0 (0, 2, 4, 6). */
#ifndef IMHOTEP_FRAME_VIEW_H
#define IMHOTEP_FRAME_VIEW_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004F0BA0 the screen position (x, y) of the tile (tile_x, tile_y); for an orientation other
 * than 0, 2, 4, 6 nothing is written */
void view_tile_to_screen(game_state *st, int32_t tile_x, int32_t tile_y, int32_t *x, int32_t *y);
/* @bin 0x004F0AB0 the tile of a view cell (row, column * 2 + 1) when the table has none there */
void view_cell_tile(game_state *st, int32_t row, int32_t col, int32_t *tile_y, int32_t *tile_x);
/* @bin 0x00430960 the grid offset of the tile at the screen point (x, y), 0 off the view; leaves the
 * cell and its edge flags in 0x0070E3F8 .. 0x0070E408 */
int32_t view_screen_to_tile(game_state *st, int32_t x, int32_t y);

#endif
