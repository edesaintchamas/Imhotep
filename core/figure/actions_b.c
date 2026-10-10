/* Imhotep - figure actions, group b: the port table (@bin action table 0x005DC28C).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "figure/actions_b.h"

#include "state/layout.h"
#include "state/ported.h"

#define ACTION(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
ACTION(run_roamer_action, figb_roamer_action)
ACTION(run_monument_damage, figb_monument_damage)
ACTION(run_soldier_action, figb_soldier_action)
ACTION(run_15, figb_action_15)
ACTION(run_16, figb_action_16)
ACTION(run_17, figb_action_17)
ACTION(run_19, figb_action_19)
ACTION(run_1a, figb_action_1a)
ACTION(run_1b, figb_action_1b)
ACTION(run_1c, figb_action_1c)
ACTION(run_1d, figb_action_1d)
ACTION(run_1e, figb_action_1e)
ACTION(run_1f, figb_action_1f)
ACTION(run_20, figb_action_20)
ACTION(run_21, figb_action_21)
ACTION(run_22, figb_action_22)
ACTION(run_23, figb_action_23)
ACTION(run_26, figb_action_26)
ACTION(run_27, figb_action_27)
ACTION(run_2a, figb_action_2a)
ACTION(run_28, figb_action_28)
ACTION(run_42, figb_action_42)
ACTION(run_3e, figb_action_3e)
ACTION(run_43, figb_action_43)
ACTION(run_44, figb_action_44)
ACTION(run_45, figb_action_45)
ACTION(run_46, figb_action_46)
ACTION(run_52, figb_action_52)
ACTION(run_2f, figb_action_2f)
ACTION(run_30, figb_action_30)
ACTION(run_31, figb_action_31)
ACTION(run_32, figb_action_32)
ACTION(run_33, figb_action_33)
ACTION(run_34, figb_action_34)
ACTION(run_35, figb_action_35)
ACTION(run_36, figb_action_36)
ACTION(run_37, figb_action_37)
ACTION(run_38, figb_action_38)
ACTION(run_39, figb_action_39)
ACTION(run_2e, figb_action_2e)
ACTION(run_2c, figb_action_2c)
ACTION(run_2d, figb_action_2d)
ACTION(run_2b, figb_action_2b)
ACTION(run_29, figb_action_29)
ACTION(run_24, figb_action_24)
ACTION(run_25, figb_action_25)
ACTION(run_3a, figb_action_3a)
ACTION(run_3b, figb_action_3b)
ACTION(run_3c, figb_action_3c)
ACTION(run_3d, figb_action_3d)
ACTION(run_18, figb_action_18)
ACTION(run_3f, figb_action_3f)
ACTION(run_40, figb_action_40)
ACTION(run_41, figb_action_41)

static int32_t run_head_27(game_state *st, const int32_t *a) { return figb_chain_head_27(st, a[0]); }
#define TILE(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        int32_t x = S32(st, (uint32_t) a[1]), y = S32(st, (uint32_t) a[2]); \
        fn(st, a[0], &x, &y); \
        S32(st, (uint32_t) a[1]) = x; \
        S32(st, (uint32_t) a[2]) = y; \
        return 1; \
    }
TILE(run_tile_4e0cc0, figb_tile_4e0cc0)
TILE(run_tile_4f6420, figb_tile_4f6420)
TILE(run_tile_545a30, figb_tile_545a30)
TILE(run_tile_558f00, figb_tile_558f00)
TILE(run_tile_556230, figb_tile_556230)
TILE(run_tile_54f8a0, figb_tile_54f8a0)
static int32_t run_follow(game_state *st, const int32_t *a) { figb_follow(st, a[0], a[1], a[2]); return 0; }
static int32_t run_stock_max(game_state *st, const int32_t *a) { return figb_stock_max(st, a[0]); }
static int32_t run_stock_max2(game_state *st, const int32_t *a) { return figb_stock_max2(st, a[0]); }
static int32_t run_look_for_target(game_state *st, const int32_t *a) { return figb_look_for_target(st, a[0]); }
static int32_t run_dock_load(game_state *st, const int32_t *a) { return figb_dock_load(st, a[0]); }
static int32_t run_dock_unload(game_state *st, const int32_t *a) { return figb_dock_unload(st, a[0]); }
static int32_t run_dock_free(game_state *st, const int32_t *a) { return figb_dock_free(st, a[0]); }
static int32_t run_dock_lost(game_state *st, const int32_t *a) { return figb_dock_lost(st, a[0]); }
static int32_t run_find_target(game_state *st, const int32_t *a) { return figb_find_target(st, a[0], a[1]); }
static int32_t run_remove(game_state *st, const int32_t *a) { figb_remove(st, a[0]); return 0; }
static int32_t run_water_point(game_state *st, const int32_t *a) { return figb_water_point(st, a[0]); }
static int32_t run_animal_flee(game_state *st, const int32_t *a) { return figb_animal_flee(st, a[0]); }
/* output pointers: addresses in the state (e.g. from the harness) */
static int32_t run_ring_search(game_state *st, const int32_t *a)
{
    int32_t x = S32(st, (uint32_t) a[5]), y = S32(st, (uint32_t) a[6]);
    int32_t r = figb_ring_search(st, a[0], a[1], a[2], (uint32_t) a[3], (uint32_t) a[4], &x, &y, a[7], a[8], (uint32_t) a[9]);
    S32(st, (uint32_t) a[5]) = x;
    S32(st, (uint32_t) a[6]) = y;
    return r;
}
static int32_t run_flee_tile(game_state *st, const int32_t *a)
{
    int32_t x = S32(st, (uint32_t) a[4]), y = S32(st, (uint32_t) a[5]);
    int32_t r = figb_flee_tile(st, a[0], a[1], a[2], a[3], &x, &y);
    S32(st, (uint32_t) a[4]) = x;
    S32(st, (uint32_t) a[5]) = y;
    return r;
}
static int32_t run_find_target_building(game_state *st, const int32_t *a) { return figb_find_target_building(st, a[0]); }
static int32_t run_find_monument(game_state *st, const int32_t *a) { return figb_find_monument(st, a[0]); }
static int32_t run_deliver(game_state *st, const int32_t *a) { figb_deliver(st, a[0], a[1], a[2]); return 0; }
static int32_t run_go_home_or_die(game_state *st, const int32_t *a) { figb_go_home_or_die(st, a[0]); return 0; }
static int32_t run_go_to_building(game_state *st, const int32_t *a) { return figb_go_to_building(st, a[0], a[1]); }
static int32_t run_next_stop(game_state *st, const int32_t *a) { figb_next_stop(st, a[0], a[1], a[2]); return 0; }
static int32_t run_next_stop_split(game_state *st, const int32_t *a) { figb_next_stop_split(st, a[0], a[1], a[2]); return 0; }
static int32_t run_leader_of(game_state *st, const int32_t *a) { return figb_leader_of(st, a[0]); }

const ported_function figb_ports[] = {
    {0x00493610u, "figb_roamer_action", 1, 0, 0, run_roamer_action, 0},
    {0x00492360u, "figb_leader_of", 1, 32, 0, run_leader_of, 0},
    {0x004923A0u, "figb_action_15", 0, 0, 0, run_15, 0},
    {0x00492580u, "figb_action_16", 0, 0, 0, run_16, 0},
    {0x00492770u, "figb_action_17", 0, 0, 0, run_17, 0},
    {0x004937A0u, "figb_action_19", 0, 0, 0, run_19, 0},
    {0x00493EC0u, "figb_action_1a", 0, 0, 0, run_1a, 0},
    {0x00494010u, "figb_action_1b", 0, 0, 0, run_1b, 0},
    {0x004941B0u, "figb_action_1c", 0, 0, 0, run_1c, 0},
    {0x00494200u, "figb_action_1d", 0, 0, 0, run_1d, 0},
    {0x004942F0u, "figb_action_1e", 0, 0, 0, run_1e, 0},
    {0x004943E0u, "figb_action_1f", 0, 0, 0, run_1f, 0},
    {0x004944D0u, "figb_action_20", 0, 0, 0, run_20, 0},
    {0x004945C0u, "figb_action_21", 0, 0, 0, run_21, 0},
    {0x004946B0u, "figb_action_22", 0, 0, 0, run_22, 0},
    {0x004947A0u, "figb_action_23", 0, 0, 0, run_23, 0},
    {0x00496F20u, "figb_action_28", 0, 0, 0, run_28, 0},
    {0x0049B280u, "figb_action_3f", 0, 0, 0, run_3f, 0},
    {0x0049B2A0u, "figb_action_40", 0, 0, 0, run_40, 0},
    {0x0049B390u, "figb_action_41", 0, 0, 0, run_41, 0},
    {0x0049B3C0u, "figb_chain_head_27", 1, 32, 0, run_head_27, 0},
    {0x0049B400u, "figb_action_42", 0, 0, 0, run_42, 0},
    {0x00496140u, "figb_action_27", 0, 0, 0, run_27, 0},
    {0x00494F70u, "figb_action_26", 0, 0, 0, run_26, 0},
    {0x00497450u, "figb_action_2a", 0, 0, 0, run_2a, 0},
    {0x0049AEB0u, "figb_action_3e", 0, 0, 0, run_3e, 0},
    {0x0049B610u, "figb_action_43", 0, 0, 0, run_43, 0},
    {0x0049B750u, "figb_action_44", 0, 0, 0, run_44, 0},
    {0x0049BD40u, "figb_action_45", 0, 0, 0, run_45, 0},
    {0x0049C750u, "figb_action_46", 0, 0, 0, run_46, 0},
    {0x0049D0F0u, "figb_action_52", 0, 0, 0, run_52, 0},
    {0x004984A0u, "figb_action_2f", 0, 0, 0, run_2f, 0},
    {0x00498590u, "figb_action_30", 0, 0, 0, run_30, 0},
    {0x004986B0u, "figb_action_31", 0, 0, 0, run_31, 0},
    {0x00498750u, "figb_action_32", 0, 0, 0, run_32, 0},
    {0x004987F0u, "figb_action_33", 0, 0, 0, run_33, 0},
    {0x004988B0u, "figb_action_34", 0, 0, 0, run_34, 0},
    {0x00498A10u, "figb_action_35", 0, 0, 0, run_35, 0},
    {0x00498AD0u, "figb_action_36", 0, 0, 0, run_36, 0},
    {0x00498C60u, "figb_action_37", 0, 0, 0, run_37, 0},
    {0x00498DF0u, "figb_action_38", 0, 0, 0, run_38, 0},
    {0x00499010u, "figb_action_39", 0, 0, 0, run_39, 0},
    {0x00498370u, "figb_action_2e", 0, 0, 0, run_2e, 0},
    {0x00497FC0u, "figb_action_2c", 0, 0, 0, run_2c, 0},
    {0x00498160u, "figb_action_2d", 0, 0, 0, run_2d, 0},
    {0x00497DC0u, "figb_action_2b", 0, 0, 0, run_2b, 0},
    {0x00497010u, "figb_action_29", 0, 0, 0, run_29, 0},
    {0x00494800u, "figb_action_24", 0, 0, 0, run_24, 0},
    {0x00494B30u, "figb_action_25", 0, 0, 0, run_25, 0},
    {0x0049A150u, "figb_action_3a", 0, 0, 0, run_3a, 0},
    {0x0049A5A0u, "figb_action_3b", 0, 0, 0, run_3b, 0},
    {0x0049A880u, "figb_action_3c", 0, 0, 0, run_3c, 0},
    {0x0049AC00u, "figb_action_3d", 0, 0, 0, run_3d, 0},
    {0x004930B0u, "figb_action_18", 0, 0, 0, run_18, 0},
    {0x004B22E0u, "figb_follow", 3, 0, 0, run_follow, 0},
    {0x004578B0u, "figb_stock_max", 1, 32, 0, run_stock_max, 0},
    {0x004579D0u, "figb_stock_max2", 1, 32, 0, run_stock_max2, 0},
    {0x00497CF0u, "figb_look_for_target", 1, 32, 0, run_look_for_target, 0},
    {0x00495840u, "figb_dock_load", 1, 32, 0, run_dock_load, 0},
    {0x00495950u, "figb_dock_unload", 1, 32, 0, run_dock_unload, 0},
    {0x00495A40u, "figb_dock_free", 1, 32, 0, run_dock_free, 0},
    {0x00495AF0u, "figb_dock_lost", 1, 32, 0, run_dock_lost, 0},
    {0x00499B50u, "figb_find_target", 2, 32, 0, run_find_target, 0},
    {0x004BD250u, "figb_remove", 1, 0, 0, run_remove, 0},
    {0x004991A0u, "figb_soldier_action", 0, 0, 0, run_soldier_action, 0},
    {0x00497A30u, "figb_water_point", 1, 32, 0, run_water_point, 0},
    {0x004EB000u, "figb_animal_flee", 1, 8, 0, run_animal_flee, 0},
    {0x004EBC90u, "figb_ring_search", 10, 32, 0, run_ring_search, 0},
    {0x004EABF0u, "figb_flee_tile", 6, 8, 0, run_flee_tile, 0},
    {0x00492650u, "figb_find_target_building", 1, 32, 0, run_find_target_building, 0},
    {0x00492D20u, "figb_find_monument", 1, 32, 0, run_find_monument, 0},
    {0x00492FC0u, "figb_monument_damage", 0, 0, 0, run_monument_damage, 0},
    {0x00496060u, "figb_deliver", 3, 0, 0, run_deliver, 0},
    {0x00495DE0u, "figb_go_home_or_die", 1, 0, 0, run_go_home_or_die, 0},
    {0x00495C10u, "figb_go_to_building", 2, 32, 0, run_go_to_building, 0},
    {0x00495E10u, "figb_next_stop", 3, 0, 0, run_next_stop, 0},
    {0x00495F00u, "figb_next_stop_split", 3, 0, 0, run_next_stop_split, 0},
    {0x00495DA0u, "largest_want (in actions_b7.c)", 1, 32, 0, NULL, 0},
    {0x00495D30u, "go home (in figb_go_home_or_die)", 1, 0, 0, NULL, 0},
    {0x00495D70u, "anything left (in figb_go_home_or_die)", 1, 8, 0, NULL, 0},
    /* output pointers: called with addresses in the state (e.g. by the harness) */
    {0x004E0CC0u, "figb_tile_4e0cc0", 3, 0, 0, run_tile_4e0cc0, 0},
    {0x004F6420u, "figb_tile_4f6420", 3, 0, 0, run_tile_4f6420, 0},
    {0x00545A30u, "figb_tile_545a30", 3, 8, G_MONUMENT_E5, run_tile_545a30, 0},
    {0x00558F00u, "figb_tile_558f00", 3, 8, G_MONUMENT_EA, run_tile_558f00, 0},
    {0x00556230u, "figb_tile_556230", 3, 8, G_MONUMENT_EB, run_tile_556230, 0},
    {0x0054F8A0u, "figb_tile_54f8a0", 3, 8, G_MONUMENT_EC, run_tile_54f8a0, 0},
};
const int figb_ports_count = sizeof(figb_ports) / sizeof(figb_ports[0]);
