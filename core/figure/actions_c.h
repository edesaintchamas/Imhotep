/* Imhotep - figure actions, group c (action table 0x005DC28C, figure types 0x47 .. 0x68).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_FIGURE_ACTIONS_C_H
#define IMHOTEP_FIGURE_ACTIONS_C_H

#include <stdint.h>

#include "state/state.h"

/* Each action runs on the current figure (0x00D056F8), as called by figure_update_all. */
void figure_action_47(game_state *st); /* @bin 0x004A1EF0 */
void figure_action_49(game_state *st); /* @bin 0x004A2430 */
void figure_action_4a(game_state *st); /* @bin 0x004A2E90 */
void figure_action_4b(game_state *st); /* @bin 0x004A3070 */
void figure_action_4c(game_state *st); /* @bin 0x004A3B70 */
void figure_action_4d(game_state *st); /* @bin 0x004A4FD0 */
void figure_action_4e(game_state *st); /* @bin 0x004A7030 */
void figure_action_4f(game_state *st); /* @bin 0x004A7600 */
void figure_action_50(game_state *st); /* @bin 0x004A7EA0 */
void figure_action_66(game_state *st); /* @bin 0x004A1130 */
void figure_action_53(game_state *st); /* @bin 0x0049DC50 */
void figure_action_54(game_state *st); /* @bin 0x0049E8B0 */
void figure_action_67(game_state *st); /* @bin 0x004A04D0 */
void figure_action_68(game_state *st); /* @bin 0x0049F710 */

/* helpers */
int32_t ferry_passengers(game_state *st, int32_t id);                 /* @bin 0x004A34B0 */
int32_t figure_may_leave(game_state *st, int32_t id);                 /* @bin 0x004A35E0 */
void ferry_move(game_state *st, int32_t id, int32_t slowness);        /* @bin 0x004A3B00 */
void ferry_board(game_state *st, int32_t id, int32_t building, int32_t n); /* @bin 0x004A36E0 */
void ferry_unload(game_state *st, int32_t id, int32_t building);     /* @bin 0x004A38A0 */
int32_t return_zero(game_state *st, int32_t a);                       /* @bin 0x0047D350 */
int32_t tile_figure_of_type(game_state *st, int32_t id, int32_t type); /* @bin 0x004B57E0 */
int32_t tile_target(game_state *st, int32_t id);                      /* @bin 0x004B5770 */
int32_t herd_spot(game_state *st, int32_t cx, int32_t cy, int32_t k, int32_t x, int32_t y, int32_t flag); /* @bin 0x004EAB10 */
void animal_pick_action(game_state *st, int32_t id);                  /* @bin 0x004EC7C0 */
int32_t ring_search(game_state *st, int32_t x, int32_t y, int32_t *px, int32_t *py, int32_t rings, int32_t id); /* @bin 0x004EC0F0 */
int32_t animal_seek_prey(game_state *st, int32_t id);                 /* @bin 0x004EAEC0 */
void animal_seek_target(game_state *st, int32_t id);                  /* @bin 0x004EC990 */
int32_t ground_rule(game_state *st, int32_t flag);                    /* @bin 0x004EAAC0 */
int32_t type_is_boat(game_state *st, int32_t type);                   /* @bin 0x004EADA0 */
int32_t find_tree(game_state *st, int32_t a, int32_t b, int32_t c, int32_t d); /* @bin 0x004867E0 */
void worker_start_job(game_state *st, int32_t id);                    /* @bin 0x004F1CB0 */
void worker_join(game_state *st, int32_t id);                         /* @bin 0x004F2410 */
void worker_at_b7(game_state *st, int32_t building, int32_t id);      /* @bin 0x004EF3A0 */
int32_t monument_work_401f50(game_state *st, uint32_t self, int32_t a, int32_t b, int32_t c); /* @bin 0x00401F50 */
int32_t monument_work_417940(game_state *st, uint32_t self, int32_t a, int32_t b, int32_t c); /* @bin 0x00417940 */
int32_t monument_work_4e6e70(game_state *st, uint32_t self, int32_t a, int32_t b, int32_t c); /* @bin 0x004E6E70 */

#endif
