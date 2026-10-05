/* Imhotep - figure actions, group D (action table 0x005DC28C, figure types 0x51 .. 0x6D).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_FIGURE_ACTIONS_D_H
#define IMHOTEP_FIGURE_ACTIONS_D_H

#include "state/state.h"

#include <stdint.h>

/* @bin 0x004A34F0 the type-0x55 figure that leads id's sledge (0: none), *distance links away;
 * 0x004A3540 without the distance */
int32_t figure_sledge_leader(game_state *st, int32_t id, int32_t *distance);

/* @bin 0x004F3360 the next pyramid level (3 is skipped) */
int32_t pyramid_level_next(int32_t level);
/* @bin 0x004F3370 the free block slot of pyramid b nearest to its reference point (*px, *py);
 * check: skip the parts 0x004EEA10 reports busy. Returns 1 when found. */
int8_t pyramid_block_slot(game_state *st, int32_t b, int32_t *px, int32_t *py, int8_t check);

/* @bin 0x0048E820 the enemy figure nearest (x, y) by weighted distance below max_distance (0: none) */
int32_t figure_find_enemy(game_state *st, int32_t x, int32_t y, int32_t max_distance);
/* @bin 0x0048E6F0 every 10th call, send figure id after a near enemy; returns 1 when it does */
int32_t figure_seek_enemy(game_state *st, int32_t id);
/* @bin 0x004BC1E0 the monuments' handlers for (x, y, id), the first non-zero result */
int32_t monument_effect_at(game_state *st, int32_t x, int32_t y, int32_t id);
/* @bin 0x004BC2E0 / 0x004BC3B0 the first monument at work phase 2 / 3 taking worker id (0: none) */
int32_t monument_work_phase(game_state *st, int32_t id, int32_t phase);
/* @bin 0x004BEEB0 (thiscall on 0x00D05720) find work for the laborer fid */
void labor_assign(game_state *st, uint32_t self, int32_t fid);
/* @bin 0x004BF920 (thiscall on 0x00D05720) the laborer fid reached its building */
void labor_arrived(game_state *st, int32_t fid);
/* @bin 0x004BF9D0 (thiscall on 0x00D05720) does a building want laborers? */
int8_t labor_wanted_anywhere(game_state *st, uint32_t self);

/* @bin 0x004A9730 the laborer's sledge destination still takes stone (else a new partner) */
int8_t laborer_sledge_ready(game_state *st, int32_t id);
/* @bin 0x004B23C0 sledge part id follows prev (step: 0x004F1D10's argument; lag: frames behind) */
void sledge_follow(game_state *st, int32_t id, int32_t prev, int32_t step, int32_t lag);

/* Each action works on the current figure (0x00D056F8), as called by figure_update_all. */
void figure_action_51(game_state *st); /* @bin 0x004A8800 */
void figure_action_55(game_state *st); /* @bin 0x004A98B0 (actions_d2.c) */
void figure_action_56(game_state *st); /* @bin 0x004AB980 */
void figure_action_57(game_state *st); /* @bin 0x004ABD50 */
void figure_action_58(game_state *st); /* @bin 0x004ABE40 */
void figure_action_59(game_state *st); /* @bin 0x004AC450 */
void figure_action_5A(game_state *st); /* @bin 0x004AC9F0 */
void figure_action_5B(game_state *st); /* @bin 0x004ACEB0 */
void figure_action_5C(game_state *st); /* @bin 0x004ADF60 (actions_d4.c) */
void figure_action_5D(game_state *st); /* @bin 0x004AEE10 (actions_d4.c) */
void figure_action_5F(game_state *st); /* @bin 0x004AF140 */
void figure_action_60(game_state *st); /* @bin 0x004AF2C0 */
void figure_action_61(game_state *st); /* @bin 0x004AF4C0 */
void figure_action_62(game_state *st); /* @bin 0x004AF4E0 */
void figure_action_63(game_state *st); /* @bin 0x004AF7C0 */
void figure_action_64(game_state *st); /* @bin 0x004AF970 (actions_d4.c) */
void figure_action_65(game_state *st); /* @bin 0x004AFC40 (actions_d4.c) */
void figure_action_6A(game_state *st); /* @bin 0x004AFEB0 (actions_d3.c) */
void figure_action_6B(game_state *st); /* @bin 0x004B03B0 (actions_d3.c) */
void figure_action_6C(game_state *st); /* @bin 0x004B0760 (actions_d3.c) */
void figure_action_6D(game_state *st); /* @bin 0x004B0EC0 (actions_d3.c) */

#endif
