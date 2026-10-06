/* Imhotep - the drag types of the construction place command (package place).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_COMMAND_PLACE_DRAG_H
#define IMHOTEP_COMMAND_PLACE_DRAG_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00474130 */
void place_houses(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
/* @bin 0x004E2D80 */
void place_notice_4e2d80(game_state *st, int32_t unused);
/* @bin 0x004E2DD0 */
void place_notice_4e2dd0(game_state *st, int32_t unused);
/* @bin 0x004E2FB0 */
void place_notice_4e2fb0(game_state *st, int32_t quiet);
/* @bin 0x0051BC60 */
int32_t place_path_fill(game_state *st, int32_t unused, int32_t x, int32_t y, int32_t canal);
/* @bin 0x0047B310 */
void place_road_tile(game_state *st, int32_t x, int32_t y, int32_t set);
/* @bin 0x0047B960 */
void place_canal_tile(game_state *st, int32_t x, int32_t y, int32_t arg3, int32_t set);
/* @bin 0x0051C930 */
int32_t place_path_walk(game_state *st, int32_t unused, int32_t xs, int32_t ys, int32_t x, int32_t y, int32_t mode);
/* @bin 0x00474F50 */
void place_road(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
/* @bin 0x00475090 */
void place_canal(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
/* @bin 0x0047AD20 */
void place_wall_tile(game_state *st, int32_t x, int32_t y, int32_t mode, uint32_t flag);
/* @bin 0x00475220 */
void place_walls(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t flag);
/* @bin 0x004754A0 */
void place_plaza(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
/* @bin 0x004759E0 */
void place_gardens(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2);

#endif
