/* Imhotep - figure actions, group b: figure types 0x15-0x46 and 0x52 (action table 0x005DC28C).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Each action runs on the current figure (global 0x00D056F8), called by figure_update_all. */
#ifndef IMHOTEP_FIGURE_ACTIONS_B_H
#define IMHOTEP_FIGURE_ACTIONS_B_H

#include <stdint.h>

#include "state/layout.h"
#include "state/state.h"
#include "state/records.h"

#define FIGB_CUR(st) S32(st, G_CURRENT_FIGURE)
#define FIGB_F(st) FIGURE_ADDR(FIGB_CUR(st))
#define FIGB_SPRITE(g) (G_DAY2_REDRAW_GROUPS + 8u * (uint32_t) (g))

/* shared pieces (actions_b.c) */
void figb_walker_image(game_state *st, uint32_t walk_base, uint32_t die_base);
int32_t figb_dir_rel(game_state *st);
int32_t figb_dir_rel_prev(game_state *st);
void figb_anim_advance(game_state *st, uint32_t f, int limit);

/* @bin 0x00493610 a service roamer's states (roam, return, die, fight) */
void figb_roamer_action(game_state *st);

/* @bin 0x00492360 the head of a follower chain (type 0x13, or the leader of a type 0x15) */
int32_t figb_leader_of(game_state *st, int32_t id);

/* @bin 0x0049B3C0 the type 0x27 figure at the head of a +0x68 chain (8 links at most) */
int32_t figb_chain_head_27(game_state *st, int32_t id);

/* @bin 0x004E0CC0, 0x004F6420, 0x00545A30, 0x00558F00, 0x00556230, 0x0054F8A0 a monument's
 * tile for its building b, written to *x, *y (actions_b2.c) */
void figb_tile_4e0cc0(game_state *st, int32_t b, int32_t *x, int32_t *y);
void figb_tile_4f6420(game_state *st, int32_t b, int32_t *x, int32_t *y);
void figb_tile_545a30(game_state *st, int32_t b, int32_t *x, int32_t *y);
void figb_tile_558f00(game_state *st, int32_t b, int32_t *x, int32_t *y);
void figb_tile_556230(game_state *st, int32_t b, int32_t *x, int32_t *y);
void figb_tile_54f8a0(game_state *st, int32_t b, int32_t *x, int32_t *y);

/* helpers (actions_b7.c) */
/* @bin 0x004B22E0 a follower's sub-steps towards its leader's previous tile */
void figb_follow(game_state *st, int32_t id, int32_t leader, int32_t steps);
/* @bin 0x004578B0, 0x004579D0 stock levels of a type 0x46 / 0xBE building */
int32_t figb_stock_max(game_state *st, int32_t b);
int32_t figb_stock_max2(game_state *st, int32_t b);
/* @bin 0x00497CF0 start an attack on a target in range */
int32_t figb_look_for_target(game_state *st, int32_t id);

/* @bin 0x00495840, 0x00495950, 0x00495A40, 0x00495AF0 docks (type 0x4B / 0xC0 buildings) */
int32_t figb_dock_load(game_state *st, int32_t b);
int32_t figb_dock_unload(game_state *st, int32_t b);
int32_t figb_dock_free(game_state *st, int32_t id);
int32_t figb_dock_lost(game_state *st, int32_t id);

/* @bin 0x00499B50 the best target in range, or 0 */
int32_t figb_find_target(game_state *st, int32_t id, int32_t range);

/* @bin 0x004BD250 state 2 (removed at the end of its update) */
void figb_remove(game_state *st, int32_t id);

/* @bin 0x004991A0 the soldiers' shared states (actions_b6.c) */
void figb_soldier_action(game_state *st);

/* @bin 0x00497A30 a water tile in front of the figure's building (0x00D3B4E4 / 0x00D3B4E0) */
int32_t figb_water_point(game_state *st, int32_t id);

/* @bin 0x004EBC90 ring search for figures in an action (*x, *y: the tile reached) */
int32_t figb_ring_search(game_state *st, int32_t x0, int32_t y0, int32_t ntypes, uint32_t types, uint32_t mask,
                         int32_t *x, int32_t *y, int32_t radius, int32_t except, uint32_t action);
/* @bin 0x004EABF0 a free reachable tile next to (x, y) away from (fx, fy) */
int32_t figb_flee_tile(game_state *st, int32_t x, int32_t y, int32_t fx, int32_t fy, int32_t *ox, int32_t *oy);
/* @bin 0x004EB000 an animal's watch for hunters */
int32_t figb_animal_flee(game_state *st, int32_t id);

/* @bin 0x00492650 a building for a type 0x17 figure to go to (0: none) */
int32_t figb_find_target_building(game_state *st, int32_t id);

/* @bin 0x00492D20 a monument for a type 0x18 figure to go to (0: none) */
int32_t figb_find_monument(game_state *st, int32_t id);

/* @bin 0x00492FC0 after a type 0x18 figure's visit */
void figb_monument_damage(game_state *st);

/* @bin 0x00496060 a type 0x27 figure arrived at building `dest` */
void figb_deliver(game_state *st, int32_t id, int32_t b, int32_t dest);

/* @bin 0x00495DE0, 0x00495C10, 0x00495E10, 0x00495F00 a type 0x27 figure's trip */
int32_t figb_go_home_or_die(game_state *st, int32_t id);
int32_t figb_go_to_building(game_state *st, int32_t id, int32_t b);
void figb_next_stop(game_state *st, int32_t id, int32_t b, int32_t dest);
void figb_next_stop_split(game_state *st, int32_t id, int32_t b, int32_t dest);

/* actions (actions_b1.c ...) */
void figb_action_15(game_state *st);
void figb_action_16(game_state *st);
void figb_action_17(game_state *st);
void figb_action_18(game_state *st);
void figb_action_19(game_state *st);
void figb_action_1a(game_state *st);
void figb_action_1b(game_state *st);
void figb_action_1c(game_state *st);
void figb_action_1d(game_state *st);
void figb_action_1e(game_state *st);
void figb_action_1f(game_state *st);
void figb_action_20(game_state *st);
void figb_action_21(game_state *st);
void figb_action_22(game_state *st);
void figb_action_23(game_state *st);
void figb_action_24(game_state *st);
void figb_action_25(game_state *st);
void figb_action_26(game_state *st);
void figb_action_27(game_state *st);
void figb_action_28(game_state *st);
void figb_action_29(game_state *st);
void figb_action_2a(game_state *st);
void figb_action_42(game_state *st);
void figb_action_3a(game_state *st);
void figb_action_3b(game_state *st);
void figb_action_3c(game_state *st);
void figb_action_3d(game_state *st);
void figb_action_3e(game_state *st);
void figb_action_43(game_state *st);
void figb_action_44(game_state *st);
void figb_action_45(game_state *st);
void figb_action_46(game_state *st);
void figb_action_52(game_state *st);
void figb_action_2b(game_state *st);
void figb_action_2c(game_state *st);
void figb_action_2d(game_state *st);
void figb_action_2e(game_state *st);
void figb_action_2f(game_state *st);
void figb_action_30(game_state *st);
void figb_action_31(game_state *st);
void figb_action_32(game_state *st);
void figb_action_33(game_state *st);
void figb_action_34(game_state *st);
void figb_action_35(game_state *st);
void figb_action_36(game_state *st);
void figb_action_37(game_state *st);
void figb_action_38(game_state *st);
void figb_action_39(game_state *st);
void figb_action_3f(game_state *st);
void figb_action_40(game_state *st);
void figb_action_41(game_state *st);

#endif
