/* Imhotep - shared figure helpers under the figure actions (package figx).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_FIGURE_HELPERS_X_H
#define IMHOTEP_FIGURE_HELPERS_X_H

#include <stdint.h>

#include "state/state.h"

int32_t hunter_find_prey(game_state *st, int32_t x, int32_t y, int32_t range); /* @bin 0x004A2320 */

void cart_unloaded(game_state *st, int32_t amount, int32_t fid, int32_t terrain); /* @bin 0x0048AEF0 */
void figure_new_slot_0x7f(game_state *st, int32_t fid);                         /* @bin 0x00444B30 */

int32_t direction_near(int32_t dir, int32_t ref);                                         /* @bin 0x004B1D10 */
int32_t direction_to_fine(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1); /* @bin 0x0051CD80 */
void formation_morale_hit(game_state *st, int32_t formation);                             /* @bin 0x004B7AB0 */
void figure_hit_sound(game_state *st, int32_t fid);                                       /* @bin 0x004B5050 */

void figure_attack_sound(game_state *st, int32_t fid);                                    /* @bin 0x004B5210 */
int32_t line_of_fire_clear(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty); /* @bin 0x004B4420 */
int32_t missile_create(game_state *st, int32_t from, int32_t x, int32_t y, int32_t tx, int32_t ty,
                       int32_t type);                                                     /* @bin 0x004B45C0 */
void figure_spawn_ring(game_state *st, int32_t x, int32_t y, int32_t size, int32_t many); /* @bin 0x0048C0E0 */

void figure_fight_end(game_state *st, int32_t fid); /* @bin 0x004B1DA0 */
void figure_fight(game_state *st, int32_t fid);     /* @bin 0x004B17D0 */

void animal_set_action(game_state *st, int32_t fid, int32_t action); /* @bin 0x004ECA70 */
void animal_choose_action(game_state *st, int32_t fid);               /* @bin 0x004ECD40 */

int32_t boat_ram_target(game_state *st, int32_t fid);                                     /* @bin 0x004A5C10 */
int32_t figure_held_back(game_state *st, int32_t fid);                                    /* @bin 0x004BCE90 */
int32_t warship_find_target(game_state *st, int32_t x, int32_t y, int32_t range);         /* @bin 0x004BCF70 */
int32_t warship_find_target_of(game_state *st, int32_t fid, int32_t range);               /* @bin 0x004BCF40 */

int32_t tile_neighbours_with(game_state *st, uint32_t mask, int32_t tile);            /* @bin 0x00477690 */
int32_t tile_ring_solid(game_state *st, int32_t x, int32_t y, int32_t ring);             /* @bin 0x00481490 */
void tree_image_update(game_state *st, int32_t tile);                                    /* @bin 0x00486600 */

int32_t find_gatherable(game_state *st, int32_t x, int32_t y, int32_t unused, uint32_t mask, int32_t x0, int32_t y0,
                        int32_t x1, int32_t y1); /* @bin 0x00486810 */

void animal_set_action2(game_state *st, int32_t fid, int32_t action); /* @bin 0x004EC4A0 */

int32_t figure_in_attack(game_state *st, int32_t fid);                                         /* @bin 0x004B63D0 */
int32_t formation_move_to(game_state *st, int32_t x, int32_t y, int32_t formation, int32_t silent); /* @bin 0x004B6400 */

int32_t nearest_building_0x63(game_state *st, int32_t x, int32_t y); /* @bin 0x00469A50 */

#endif
