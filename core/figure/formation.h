/* Imhotep - the daily update of the formations (companies of soldiers, chunk 24) (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_FIGURE_FORMATION_H
#define IMHOTEP_FIGURE_FORMATION_H

#include <stdint.h>

#include "state/layout.h"
#include "state/state.h"

/* @bin chunk 24: 50 formations of 0x90 bytes (record 0 unused); fields FM_* in state/layout/figure.h */
#define FORMATION_BASE G_FORMATION_BASE
#define FORMATION_SIZE 0x90u
#define FORMATION_ADDR(id) (FORMATION_BASE + FORMATION_SIZE * (uint32_t) (id))

/* @bin 0x004B7790 day steps 5 (arg 0) and 0x1D (arg 1) */
void formations_daily(game_state *st, int32_t arg);

/* @bin 0x004B77D0 counts the formations in use (0x00BCB9B4, last id 0x00BCB9B8, city +0x4698 those
 * of type 0xD); a formation with no +0x5E takes the place of its first figure still fighting */
void formations_count(game_state *st);

/* @bin 0x004B7C80 the figures of each formation, rebuilt from the figures' +0x60; totals */
void formations_rebuild(game_state *st);

/* @bin 0x004B8130 the facing (+0x6B) of each formation from its move since the last call */
void formations_facing(game_state *st);

/* @bin 0x004B7920 a counter (+0x1D) of figures of types 0xB..0xD in action 8 goes down */
void formations_soldier_counters(game_state *st);

/* @bin 0x004B79F0 */
void formations_morale_days(game_state *st);

/* @bin 0x004B7C00 / 0x004B7C40 0x004B7B50(id, delta) on every formation in use, of the player
 * (+0x42 set) / not */
void formations_morale_player(game_state *st, int32_t delta);
void formations_morale_others(game_state *st, int32_t delta);

/* @bin 0x004B8080 player formations: +0x29 := 0x10 */
void formations_reset_0x29(game_state *st);

/* @bin 0x004B8B20 the first 6 formations: timers and retreat */
void formations_timers(game_state *st);

/* @bin 0x004BA690 the building formation `id` makes for */
void formations_target_building(game_state *st, int32_t id);

/* @bin 0x004B8CF0 the enemy formations (enemies.c) */
void formations_enemies(game_state *st);
int32_t formation_find_place(game_state *st, int32_t id, int32_t x, int32_t y); /* @bin 0x004BA7E0 */

int32_t figure_type_is_0x44_0x46(int32_t type);                /* @bin 0x004EA9A0 */
int32_t figure_type_is_enemy_0x52(int32_t type);                /* @bin 0x004EA9C0 */
int32_t formation_has_action_0x12(game_state *st, int32_t id); /* @bin 0x0040B5E0 */

/* invaders.c */
void formations_invaders(game_state *st);                                     /* @bin 0x004B92E0 */
int32_t invader_tile_ok(game_state *st, uint32_t terrain, int32_t mode);      /* @bin 0x004EC410 */
int32_t formation_count_0x44(game_state *st, int32_t id, int32_t action);     /* @bin 0x004EAA60 */
int32_t formation_count_0x44_active(game_state *st, int32_t id);              /* @bin 0x004EAA10 */
int32_t invader_find_tile(game_state *st, int32_t x0, int32_t y0, int32_t r, int32_t *x, int32_t *y,
                          int32_t mode);                                       /* @bin 0x004EB4C0 */

#endif
