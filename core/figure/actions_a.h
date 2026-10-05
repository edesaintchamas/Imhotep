/* Imhotep - figure actions, group A (action table 0x005DC28C, figure types 0x01 .. 0x14, 0x69).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_FIGURE_ACTIONS_A_H
#define IMHOTEP_FIGURE_ACTIONS_A_H

#include <stdint.h>

#include "state/state.h"

/* Each action works on the current figure (0x00D056F8), as called by figure_update_all. */
void figure_action_01(game_state *st); /* @bin 0x00489CA0 */
void figure_action_05(game_state *st); /* @bin 0x0048BFF0 */
void figure_action_06(game_state *st); /* @bin 0x0048C200 */
void figure_action_02(game_state *st); /* @bin 0x0048A260 */
void figure_action_03(game_state *st); /* @bin 0x0048A720 */
void figure_action_04(game_state *st); /* @bin 0x0048B620 */
void figure_action_07(game_state *st); /* @bin 0x0048C2B0 */
void figure_action_08(game_state *st); /* @bin 0x0048C6A0 */
void figure_action_09(game_state *st); /* @bin 0x0048CE30 */

/* helpers (actions_a2.c) */
int32_t cart_storage_for_good(game_state *st, int32_t fid, int32_t good, int32_t terrain); /* @bin 0x0045DCC0 */
int32_t cart_storage_to_empty(game_state *st, int32_t bid);                                 /* @bin 0x0045DE80 */
int32_t cart_workshop_for_good(game_state *st, int32_t fid, int32_t good, int32_t terrain); /* @bin 0x0045F450 */
int32_t cart_take_from_storage(game_state *st, int32_t from, int32_t to);                   /* @bin 0x0045E150 */
int32_t cart_find_destination(game_state *st, int32_t fid, int32_t terrain); /* @bin 0x0048AE50 */
int32_t cart_good_sprites(int32_t good);                                    /* @bin 0x0048B320 */
void cart_sprite_load(game_state *st, int32_t fid);                         /* @bin 0x0048B4A0 */
void cart_delivered(game_state *st, int32_t taken, int32_t fid, int32_t terrain, int32_t last); /* @bin 0x0048CAA0 */
void figure_action_0a(game_state *st); /* @bin 0x0048DED0 */
void figure_action_0e(game_state *st); /* @bin 0x00490B50 */
void figure_action_0f(game_state *st); /* @bin 0x00490E00 */
void figure_action_13(game_state *st); /* @bin 0x00491420 */
void figure_action_14(game_state *st); /* @bin 0x00491A90 */
void figure_action_0b(game_state *st); /* @bin 0x0048E9E0 */

/* figure helpers (actions_a3.c) */
void figure_advance(game_state *st, int32_t id, int32_t steps);      /* @bin 0x004B27E0 */
void figure_move(game_state *st, int32_t id, int32_t speed);         /* @bin 0x004A5DA0 */
void figure_route_release(game_state *st, int32_t id);               /* @bin 0x004B3250 */
void figure_dying(game_state *st, int32_t id);                       /* @bin 0x004B1D50 */
int figure_near_map_edge(game_state *st, int32_t x);                 /* @bin 0x0047A1C0 */
void figure_count_on_tile(game_state *st, int32_t id);               /* @bin 0x004B53B0 */
void figure_tile_list_remove(game_state *st, int32_t id);            /* @bin 0x004B5420 */
int32_t figure_step_straight(game_state *st, int32_t id, int32_t steps); /* @bin 0x004B3FF0 */
void figure_line_to(game_state *st, int32_t id, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                    int32_t exact);                                      /* @bin 0x004B4220 */
void figure_roam(game_state *st, int32_t id, int32_t steps);            /* @bin 0x004B1EF0 */
int32_t figure_0a_divert(game_state *st, int32_t id);                   /* @bin 0x0048E5E0 */
void figure_substep(game_state *st, int32_t id); /* @bin 0x004B3F00 */
void figure_line_error(game_state *st, int32_t id); /* @bin 0x004B4180 */
void figure_line_step_x(game_state *st, int32_t id); /* @bin 0x004B43E0 */
void figure_line_step_y(game_state *st, int32_t id); /* @bin 0x004B43A0 */
void figure_route_direction(game_state *st, int32_t id); /* @bin 0x004B33D0 */
void figure_turn_around(game_state *st, int32_t id); /* @bin 0x004B21D0 */
void figure_enter_next_tile(game_state *st, int32_t id); /* @bin 0x004B3C60 */
uint8_t figure_route_slot(game_state *st, int32_t id, int32_t *slot); /* @bin 0x00489970 */
void figure_find_route(game_state *st, int32_t id); /* @bin 0x004B2D20 */
int32_t figure_route_to_any(game_state *st, int32_t id, uint32_t list, int32_t n); /* @bin 0x004B28F0 */
int32_t cart_workshop_with_room(game_state *st, int32_t fid, int32_t good, int32_t terrain); /* @bin 0x0045EAB0 */

/* route searches (route_a.c) */
int32_t route_free_tile(game_state *st, int32_t tile);                                             /* @bin 0x0051AFF0 */
int32_t route_walk_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty);              /* @bin 0x00519840 */
int32_t route_walk_to_any(game_state *st, int32_t x, int32_t y, uint32_t list, int32_t n);         /* @bin 0x00519E30 */
int32_t route_walk_free_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty);         /* @bin 0x00518880 */
int32_t route_any_free_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty);          /* @bin 0x00517F20 */
int32_t route_any_free_to_any(game_state *st, int32_t x, int32_t y, uint32_t list, int32_t n);     /* @bin 0x00518B30 */
int32_t route_walk_free_to_marked(game_state *st, int32_t x, int32_t y, uint32_t list, int32_t n); /* @bin 0x005190A0 */
void figure_move_hurried(game_state *st, int32_t id, int32_t pressed); /* @bin 0x004A5680 */
int32_t walker_find_workplace(game_state *st, int32_t fid); /* @bin 0x00467450 */
int32_t road_wear(game_state *st, int32_t kind, int32_t tile);              /* @bin 0x0047E230 */
int32_t roam_open_directions(game_state *st, int32_t tile, int32_t fid);   /* @bin 0x0047E340 */
void neighbours_with_flags(game_state *st, int32_t mask); /* @bin 0x0047DB00 */
int32_t neighbours_count(game_state *st);                 /* @bin 0x00484F40 */
int32_t queue_push(game_state *st, uint32_t q, int32_t value);            /* @bin 0x004F65B0 */
int32_t queue_length(game_state *st, uint32_t q);                          /* @bin 0x004F6630 */
int32_t queue_full(game_state *st, uint32_t q);                            /* @bin 0x004F6650 */
void tile_wear(game_state *st, int32_t tile, int32_t limit);              /* @bin 0x004690F0 */
int32_t ferry_on_route(game_state *st, int32_t id, int32_t b);            /* @bin 0x004B3470 */
void figure_check_next_tile(game_state *st, int32_t id);                  /* @bin 0x004B3530 */
int32_t figure_serve_tile(game_state *st, int32_t id);                     /* @bin 0x004BAA10 */

/* service area scans (service_a.c) */
int32_t serve_houses(game_state *st, int32_t x, int32_t y, uint32_t field);
int32_t count_houses(game_state *st, int32_t x, int32_t y);                                   /* @bin 0x00484990 */
int32_t serve_houses_god(game_state *st, int32_t x, int32_t y, int32_t god);                  /* @bin 0x00483FB0 */
int32_t serve_houses_4e(game_state *st, int32_t x, int32_t y);                                /* @bin 0x00484A90 */
int32_t clear_buildings_58(game_state *st, int32_t x, int32_t y);                             /* @bin 0x00484890 */
int32_t lower_houses(game_state *st, int32_t x, int32_t y, int32_t amount, uint32_t field);   /* 0x00482DD0 / EF0 */
int32_t clear_houses_87(game_state *st, int32_t x, int32_t y);                                /* @bin 0x00483160 */
void lower_buildings_e4(game_state *st, int32_t x, int32_t y, int32_t amount);                /* @bin 0x00483260 */
int32_t lower_main_parts(game_state *st, int32_t x, int32_t y, uint32_t field);               /* 0x00482C10 / 3000 */
void inspect_houses(game_state *st, int32_t x, int32_t y);
int32_t serve_houses_e90(game_state *st, int32_t x, int32_t y);                               /* @bin 0x00483E90 */                                    /* @bin 0x0042FF20 */
int32_t route_store(game_state *st, int32_t mode, int32_t slot, int32_t x0, int32_t y0, int32_t tx, int32_t ty); /* @bin 0x0051B400 */
void road_wear_up(game_state *st, int32_t kind, int32_t tile);                                  /* @bin 0x0047E250 */
int32_t find_enemy_near(game_state *st, int32_t x, int32_t y, int32_t range, int32_t fid); /* @bin 0x004999E0 */
int32_t cart_storage_with_room(game_state *st, int32_t force, int32_t fid, int32_t good, int32_t terrain); /* @bin 0x0045D9E0 */
int32_t cart_dock_for_good(game_state *st, int32_t skip, int32_t fid, int32_t good, int32_t terrain); /* @bin 0x0045ACF0 */
void figure_meet(game_state *st, int32_t id, int32_t first); /* @bin 0x004B1210 */
int32_t bazaar_sell(game_state *st, int32_t bazaar, int32_t x, int32_t y); /* @bin 0x00483910 */
int32_t route_even_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty); /* @bin 0x00518400 */
int32_t route_even_to_any(game_state *st, int32_t x, int32_t y, uint32_t list, int32_t n); /* @bin 0x00518DD0 */
int32_t route_any_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty); /* @bin 0x005186D0 */
int32_t route_not3_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty); /* @bin 0x00519390 */
int32_t route_or5_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty); /* @bin 0x005195E0 */
int32_t route_water_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty); /* @bin 0x00519BE0 */
int32_t route_boat_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty); /* @bin 0x0051A030 */
int32_t route_boat_to_any(game_state *st, int32_t x, int32_t y, uint32_t list, int32_t n); /* @bin 0x0051A220 */
int32_t route_free6_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty); /* @bin 0x0051D420 */
int32_t route_free6_to_any(game_state *st, int32_t x, int32_t y, uint32_t list, int32_t n); /* @bin 0x0051D6D0 */
int32_t route_ec7630_to_any(game_state *st, int32_t x, int32_t y, uint32_t list, int32_t n); /* @bin 0x0051B220 */

#endif
