/* Imhotep - the construction UI handlers of Pharaoh.exe (package ui, docs/commands.md 2.1, 2.3, 2.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The player's construction commands at the UI-handler boundary: choose a building, mouse down /
 * drag / mouse up on the city view, cancel (right click), undo. Their inputs are globals the
 * platform sets (submenu 0x007056EC, item 0x00D3B198, hovered tile 0x00BCA300/304/308, tile under
 * the mouse 0x00BCA2C4/C8/CC, rotation 0x00BC9714). */
#ifndef IMHOTEP_COMMAND_CONSTRUCTION_UI_H
#define IMHOTEP_COMMAND_CONSTRUCTION_UI_H

#include <stdint.h>

#include "state/state.h"

void construction_choose_building(game_state *st);   /* @bin 0x00405200 */
int32_t construction_cancel(game_state *st);          /* @bin 0x00405BA0 */
void construction_mouse_down(game_state *st);         /* @bin 0x00405C20 */
void construction_drag(game_state *st);               /* @bin 0x00405D90 */
void construction_mouse_up(game_state *st);           /* @bin 0x00405E90 */
void construction_undo_button(game_state *st);        /* @bin 0x00406010 */
int32_t construction_undo_snapshot(game_state *st);   /* @bin 0x00484F60 */
void construction_preview(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                          int32_t type);              /* @bin 0x0046FEF0 */

/* Terrain predicates stored in 0x00605A48 (by their address in the exe) */
int32_t terrain_pred_405b40(uint32_t terrain);        /* @bin 0x00405B40 */
int32_t terrain_pred_405b60(uint32_t terrain);        /* @bin 0x00405B60 */
int32_t terrain_pred_405b80(uint32_t terrain);        /* @bin 0x00405B80 */

void ferry_half_delete(game_state *st, int32_t id);   /* @bin 0x00486060 */
void ferry_place_step(game_state *st);                /* @bin 0x00486020 */

#endif
