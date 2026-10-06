/* Imhotep - clearing land (player command, docs/commands.md section 2.5).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_COMMAND_CLEAR_H
#define IMHOTEP_COMMAND_CLEAR_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00474380 1 for the monument building types that clearing asks to confirm */
int clear_is_monument_type(int32_t type);

/* @bin 0x004743F0 clear land over the rectangle (preview 1: the drag ghost) or ask to confirm */
void clear_land(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2);

/* @bin 0x00474700 clear land over the rectangle; 0x00BC971C := the count of cleared items */
void clear_land_apply(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2);

/* @bin 0x004021D0 / 0x00402210 / 0x00402250 / 0x00402290 confirmation callbacks (monument,
 * temple complex, fort, bridge): answer 0x00D3B439 */
void clear_confirm_monument(game_state *st);
void clear_confirm_temple(game_state *st);
void clear_confirm_fort(game_state *st);
void clear_confirm_bridge(game_state *st);

/* @bin 0x004264D0 open the yes/no (kind 2) or OK popup with text_id; callback on the answer */
void popup_confirm_open(game_state *st, int32_t text_id, int32_t callback, int32_t kind);

/* @bin 0x0047F9B0 a bridge through grid offset off: the count of its tiles with a figure (bits
 * 0x40 of the bitfields cleared along it) */
int32_t bridge_figures(game_state *st, int32_t off);

/* @bin 0x0047F810 remove the bridge through off (preview: only mark it in the bitfields) */
void bridge_remove(game_state *st, int32_t off, int32_t preview);

/* @bin 0x0047B480 roads around the square (x - 1, y - 1, size): tiles with terrain 0x40 and not
 * 0xC get their road images again */
void clear_roads_square(game_state *st, int32_t x, int32_t y, int32_t size);

/* @bin 0x004F4930 a pyramid part cleared or undone: its ramp and the land around redone */
void clear_pyramid_area(game_state *st, int32_t id);

#endif
