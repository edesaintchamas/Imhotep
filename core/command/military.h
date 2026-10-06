/* Imhotep - player commands: legions (formations) and ships (package military).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The UI handlers of docs/commands.md §3.7 (button callbacks of the legion / ship windows and of
 * the military advisor, and the left clicks of the "choose a destination" windows 0x1E, 0x1F,
 * 0x20, 0x49), with the formation and ship helpers they call. Inputs are globals of the state:
 * the window's formation 0x00EADCCC, ships 0x00EADCC8 (transport, figure type 0x4D) and
 * 0x00EADCC4 (warship, type 0x4E), the selected formation 0x00BCB9C0, the selected ships
 * 0x00D05700 / 0x00D05704, the button parameters 0x00D3B198 / 0x00D3B194, the tile under the
 * mouse 0x00BCA2C4 (grid offset, 0 = off map) / 0x00BCA2C8 x / 0x00BCA2CC y, the mouse cursor
 * 0x00E390DC. */
#ifndef IMHOTEP_COMMAND_MILITARY_H
#define IMHOTEP_COMMAND_MILITARY_H

#include <stdint.h>

#include "state/state.h"

/* ---- commands (cdecl, no arguments) ---- */
void legion_order(game_state *st);               /* @bin 0x00406720 layout / stance buttons */
void legion_toggle_0x77(game_state *st);         /* @bin 0x00407110 */
void legion_return_to_fort(game_state *st);      /* @bin 0x00406FB0 */
void legion_window_close(game_state *st);        /* @bin 0x004071C0 */
void legion_click_target(game_state *st);        /* @bin 0x0040C440 window 0x1E left click */
void legion_click_target_0x49(game_state *st);   /* @bin 0x0040C570 window 0x49 left click */
void advisor_legion_return(game_state *st);      /* @bin 0x0040A350 */
void advisor_legion_service(game_state *st);     /* @bin 0x0040A390 */
void transport_order(game_state *st);            /* @bin 0x00406B40 */
void transport_return(game_state *st);           /* @bin 0x00406E80 */
void warship_order(game_state *st);              /* @bin 0x00406C90 */
void warship_return(game_state *st);             /* @bin 0x00406DE0 */
void transport_click_target(game_state *st);     /* @bin 0x0040BA40 window 0x1F left click */
void warship_click_target(game_state *st);       /* @bin 0x0040B830 window 0x20 left click */
void advisor_warship_return(game_state *st);     /* @bin 0x0040A5C0 */
void advisor_warship_service(game_state *st);    /* @bin 0x0040A600 */
void figure_order_select(game_state *st);        /* @bin 0x0053D730 */
void advisor_dispatch_troops(game_state *st);    /* @bin 0x0040A450 (0x0040A550 jumps to it) */
int32_t city_click_legion(game_state *st);       /* @bin 0x0040B630 */
int32_t city_click_warship(game_state *st);      /* @bin 0x0040B6D0 */
int32_t city_click_transport(game_state *st);    /* @bin 0x0040B790 */

/* ---- helpers ---- */
int32_t tile_soldier_formation(game_state *st, int32_t offset);       /* @bin 0x004B5520 */
int32_t tile_transport(game_state *st, int32_t offset);               /* @bin 0x004B5570 */
int32_t tile_warship(game_state *st, int32_t offset);                 /* @bin 0x004B55B0 */
int32_t tile_fort_formation(game_state *st, int32_t offset);          /* @bin 0x004B55F0 */
int32_t tile_enemy_figure(game_state *st, int32_t offset);            /* @bin 0x004B5640 */
int32_t formation_attack(game_state *st, int32_t x, int32_t y, int32_t id, int32_t target); /* @bin 0x004B66E0 */
int32_t formation_charge_to(game_state *st, int32_t x, int32_t y, int32_t id); /* @bin 0x004B6A40 */
void formation_return(game_state *st, int32_t id, int32_t action);   /* @bin 0x004B6C20 */
int32_t player_legion_by_row(game_state *st, int32_t row);           /* @bin 0x004B78B0 */
int32_t player_warship_by_row(game_state *st, int32_t row);          /* @bin 0x004BC620 */
void ship_return_to_wharf(game_state *st, int32_t id);               /* @bin 0x004BC670 */
void ships_count(game_state *st);                                    /* @bin 0x004B8AA0 */
void military_requests(game_state *st, int32_t *first_other, int32_t *first_kind2, int32_t *n_other,
                       int32_t *n_kind2);                            /* @bin 0x00521AA0 */
void military_requests_open(game_state *st, uint8_t *other, uint8_t *kind2); /* @bin 0x00521A60 */

int32_t request_row_check(game_state *st, int32_t row, int32_t *event); /* @bin 0x0044D520 */

#endif
