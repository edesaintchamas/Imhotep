/* Imhotep - figure actions, group c: the boats' helpers (transport ship 0x4D, warship 0x4E) and
 * the map searches they use.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_FIGURE_ACTIONS_C2_H
#define IMHOTEP_FIGURE_ACTIONS_C2_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x0040C180 along the line (x0, y0) -> (x1, y1), the farthest open-water tile; *px, *py */
int32_t line_search_water(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t *px, int32_t *py);
/* @bin 0x0040BDA0 along the line (x0, y0) -> (x1, y1), the farthest dry tile; *px, *py */
int32_t line_search_land(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t *px, int32_t *py);
/* @bin 0x004467A0 the nearest free water tile around (*px, *py) (rings of growing size) */
int32_t spiral_free_water(game_state *st, int32_t *px, int32_t *py);
/* @bin 0x004AD880 the nearest free land tile around (*px, *py) */
int32_t spiral_free_land(game_state *st, int32_t *px, int32_t *py);
/* @bin 0x004A4850 the best of three line searches from the boat `id`, offsets (a3, a4) (a5, a6)
 * (a7, a8); then the nearest free water tile */
int32_t boat_flee_target(game_state *st, int32_t *px, int32_t *py, int32_t a3, int32_t a4, int32_t a5, int32_t a6,
                         int32_t a7, int32_t a8, int32_t id);

int32_t boat_at_dock(game_state *st, int32_t id);                              /* @bin 0x004A4300 */
int32_t boat_join_battle(game_state *st, int32_t id, int32_t building);        /* @bin 0x004A4340 */
int32_t boat_formation_ready(game_state *st, int32_t id);                      /* @bin 0x004A43F0 */
void formation_embark(game_state *st, int32_t fmi);                            /* @bin 0x004A4470 */
void formation_land(game_state *st, int32_t fmi, int32_t x, int32_t y);        /* @bin 0x004A44F0 */
int32_t tile_player_boat(game_state *st, int32_t tile, int32_t id);            /* @bin 0x004A59C0 */
int32_t tile_enemy_boat(game_state *st, int32_t tile, int32_t id);             /* @bin 0x004A5A20 */
int32_t boat_nearest(game_state *st, int32_t id, int32_t range, int32_t player_boats); /* @bin 0x004A5560 */
void boat_ram_sound(game_state *st, int32_t k);                                /* @bin 0x004A5880 */
int32_t boat_ahead(game_state *st, int32_t id);                                /* @bin 0x004A5A80 */
int32_t warship_target_at(game_state *st, int32_t x, int32_t y, int32_t range); /* @bin 0x004BCB10 */
int32_t warship_target(game_state *st, int32_t id, int32_t range);             /* @bin 0x004BCAE0 */
int32_t enemy_warship_at(game_state *st, int32_t x, int32_t y, int32_t range);  /* @bin 0x004BCD90 */
int32_t enemy_warship(game_state *st, int32_t id, int32_t range);              /* @bin 0x004BCD60 */
void boat_unstick(game_state *st, uint32_t self, int32_t id);                /* @bin 0x004BE4A0 */
int32_t dock_battle_value(game_state *st, int32_t building);                   /* @bin 0x004069E0 */
int32_t best_dock(game_state *st, int32_t id);                                 /* @bin 0x00406A30 */
int32_t battle_add_boat(game_state *st, uint32_t self, int32_t id);           /* @bin 0x004F68F0 */
void boat_seek_dock(game_state *st, int32_t id);                               /* @bin 0x004A45D0 */
void boat_sink(game_state *st, int32_t id);                                    /* @bin 0x004A46F0 */
void boat_moored(game_state *st, int32_t id, int32_t building, int32_t *moving); /* @bin 0x004A4770 */
void boat_go(game_state *st, int32_t id, int32_t *moving);                     /* @bin 0x004A4950 */
void boat_flee(game_state *st, int32_t id, int32_t *moving);                   /* @bin 0x004A4A80 */
void boat_to_battle(game_state *st, int32_t id, int32_t *moving);              /* @bin 0x004A4C40 */
void boat_to_formation(game_state *st, int32_t id, int32_t *moving);           /* @bin 0x004A4D00 */
void boat_to_landing(game_state *st, int32_t id, int32_t *moving);             /* @bin 0x004A4E10 */
void boat_face_enemy(game_state *st, int32_t id);                              /* @bin 0x004A4F60 */

void warship_moored(game_state *st, int32_t id, int32_t *moving);    /* @bin 0x004A58E0 */
void warship_ram(game_state *st, int32_t id, int32_t *moving);       /* @bin 0x004A6160 */
void warship_attack(game_state *st, int32_t id, int32_t *moving);    /* @bin 0x004A6300 */
void warship_go(game_state *st, int32_t id, int32_t *moving);        /* @bin 0x004A6890 */
void warship_flee(game_state *st, int32_t id, int32_t *moving);      /* @bin 0x004A6970 */
void warship_to_battle(game_state *st, int32_t id, int32_t *moving); /* @bin 0x004A6B40 */
void warship_fire(game_state *st, int32_t id);                       /* @bin 0x004A6BF0 */
void warship_face_enemy(game_state *st, int32_t id);                 /* @bin 0x004A6CC0 */
void warship_hunt(game_state *st, int32_t id, int32_t *moving);      /* @bin 0x004A6D40 */
/* @bin 0x004B3340 the figure's next direction (route step, or 8 / 10 towards the destination) */
int32_t figure_next_direction(game_state *st, int32_t id);

#endif
