/* Imhotep - ported game functions by their address in Pharaoh.exe.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "state/ported.h"

#include <stddef.h>

#include "city/desirability.h"
#include "city/goods.h"
#include "map/routing.h"
#include "map/terrain.h"
#include "sim/small.h"
#include "state/post_load.h"
#include "state/pre_load.h"
#include "state/scenario_start.h"

static int32_t run_desirability_buildings(game_state *st, const int32_t *args)
{
    (void) args;
    desirability_buildings(st);
    return 0;
}

static int32_t run_desirability_tiles(game_state *st, const int32_t *args)
{
    (void) args;
    desirability_tiles(st);
    return 0;
}

static int32_t run_map_row_ranges(game_state *st, const int32_t *args)
{
    (void) args;
    map_row_ranges(st);
    return 0;
}

#define VOID0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
VOID0(run_routing_clear_ee0c50, routing_clear_ee0c50)
VOID0(run_routing_rebuild_ed4140, routing_rebuild_ed4140)
VOID0(run_routing_rebuild_ebab20, routing_rebuild_ebab20)
VOID0(run_routing_rebuild_ec7630, routing_rebuild_ec7630)
VOID0(run_routing_rebuild_ee0c50, routing_rebuild_ee0c50)
VOID0(run_goods_available_update, goods_available_update)
VOID0(run_load_rebuild_storage_links, load_rebuild_storage_links)
VOID0(run_load_rebuild_0x88_links, load_rebuild_0x88_links)
VOID0(run_load_rebuild_0x4a_links, load_rebuild_0x4a_links)
VOID0(run_pre_load_clear, pre_load_clear)

static int32_t run_fill_ed4140(game_state *st, const int32_t *a)
{
    routing_fill_ed4140(st, a[0], a[1], a[2], a[3]);
    return 0;
}

static int32_t run_fill_ec7630(game_state *st, const int32_t *a)
{
    routing_fill_ec7630(st, a[0], a[1], a[2], a[3]);
    return 0;
}

static int32_t run_fill_ee0c50(game_state *st, const int32_t *a)
{
    routing_fill_ee0c50(st, a[0], a[1], a[2], a[3], (int8_t) a[4]);
    return 0;
}

#define INT1(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) { return fn(st, a[0]); }
INT1(run_goods_trade_city, goods_trade_city)
INT1(run_goods_trade_city_flag5, goods_trade_city_flag5)
INT1(run_goods_chain_b90, goods_chain_b90)
INT1(run_goods_chain_d90, goods_chain_d90)
INT1(run_goods_event_list, goods_event_list)
INT1(run_trade_limit_adjusted, trade_limit_adjusted)
INT1(run_storage_create, storage_create)
INT1(run_rec88_create, rec88_create)

static int32_t run_trade_city_mood(game_state *st, const int32_t *a)
{
    (void) a;
    return trade_city_mood(st);
}

static int32_t run_empire_city_trades(game_state *st, const int32_t *a)
{
    return empire_city_trades(st, a[0], a[1]);
}

static int32_t run_rec4a_create(game_state *st, const int32_t *a)
{
    rec4a_create(st, a[0]);
    return 0;
}

static int32_t run_map_tile_inside(game_state *st, const int32_t *a)
{
    return map_tile_inside(st, a[0], a[1]);
}

static int32_t run_terrain_rebuild(game_state *st, const int32_t *a)
{
    (void) a;
    terrain_state t = {st, {0, 0, 0}};
    terrain_rebuild(&t);
    return 0;
}

static int32_t run_floodplain_build(game_state *st, const int32_t *a)
{
    terrain_state t = {st, {0, 0, 0}};
    terrain_floodplain_build(&t, a[0], a[1], a[2], a[3]);
    return 0;
}

/* the sub-pass ports cover the whole map: called with another rectangle, they are not ported */
static int whole_map(game_state *st, const int32_t *a, uint32_t addr)
{
    int n = addr == 0x00475DC0u || addr == 0x00476530u || addr == 0x00477210u || addr == 0x0047B570u ||
                    addr == 0x0047B1A0u || addr == 0x0047D870u
                ? 0
                : 4;
    /* the flags the whole-map ports assume (as called by 0x004DEA50) */
    int flags_ok = addr == 0x00475CC0u   ? a[4] == 0
                   : addr == 0x00475720u ? a[4] == 1
                   : addr == 0x0047BAE0u ? a[4] == 0
                   : addr == 0x004767D0u ? a[4] == 0 && a[5] == 0
                   : addr == 0x0047D870u ? a[0] == 0
                                         : 1;
    if ((n == 4 && (a[0] > 0 || a[1] > 0 || a[2] < S32(st, 0x00BCA2D0u) - 1 || a[3] < S32(st, 0x00BCA2D4u) - 1)) ||
        !flags_ok) {
        state_unported(st, addr);
        return 0;
    }
    return 1;
}

#define SUBPASS(name, addr) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        terrain_state t = {st, {0, 0, 0}}; \
        if (whole_map(st, a, addr)) { \
            terrain_subpass(&t, addr); \
        } \
        return 0; \
    }
SUBPASS(run_pass_special_rocks, 0x004770F0u)
SUBPASS(run_pass_rocks, 0x00475DC0u)
SUBPASS(run_pass_trees, 0x00475BF0u)
SUBPASS(run_pass_flag_0x20, 0x00476530u)
SUBPASS(run_pass_signposts, 0x0047D870u)
SUBPASS(run_pass_land, 0x004767D0u)
SUBPASS(run_pass_floodplain, 0x004777C0u)
SUBPASS(run_pass_meadow, 0x00477500u)
SUBPASS(run_pass_flag_0x40000, 0x004776F0u)
SUBPASS(run_pass_dunes, 0x00477210u)
SUBPASS(run_pass_water, 0x00475CC0u)
SUBPASS(run_pass_rubble, 0x00477890u)
SUBPASS(run_pass_roads, 0x0047B570u)
SUBPASS(run_pass_plaza, 0x00475720u)
SUBPASS(run_pass_walls, 0x0047B1A0u)
SUBPASS(run_pass_canals, 0x0047BAE0u)

static int32_t run_nothing(game_state *st, const int32_t *a)
{
    (void) st;
    (void) a;
    return 0; /* @bin 0x0051F780 an empty function */
}

VOID0(run_anim_counters, anim_counters)
VOID0(run_clear_0x0078d784, clear_0x0078d784)
VOID0(run_countdown_0x00ea89dc, countdown_0x00ea89dc)

/* @bin addresses of the original functions (each port carries its own tags) */
const ported_function ported_functions[] = {
    {0x0046A0C0u, "desirability_buildings", 0, 0, 0, run_desirability_buildings},
    {0x0046A140u, "desirability_tiles", 0, 0, 0, run_desirability_tiles},
    {0x0046F7B0u, "map_row_ranges", 0, 0, 0, run_map_row_ranges},
    {0x00517A20u, "routing_clear_ee0c50", 0, 0, 0, run_routing_clear_ee0c50},
    {0x005178B0u, "routing_rebuild_ed4140", 0, 0, 0, run_routing_rebuild_ed4140},
    {0x005178E0u, "routing_rebuild_ebab20", 0, 0, 0, run_routing_rebuild_ebab20},
    {0x00517660u, "routing_rebuild_ec7630", 0, 0, 0, run_routing_rebuild_ec7630},
    {0x00517370u, "routing_rebuild_ee0c50", 0, 0, 0, run_routing_rebuild_ee0c50},
    {0x00517690u, "routing_fill_ed4140", 4, 0, 0, run_fill_ed4140},
    {0x005173A0u, "routing_fill_ec7630", 4, 0, 0, run_fill_ec7630},
    {0x00516F90u, "routing_fill_ee0c50", 5, 0, 0, run_fill_ee0c50},
    {0x005596D0u, "goods_available_update", 0, 0, 0, run_goods_available_update},
    {0x004439C0u, "goods_trade_city", 1, 32, 0, run_goods_trade_city},
    {0x00443950u, "goods_trade_city_flag5", 1, 32, 0, run_goods_trade_city_flag5},
    {0x00443B90u, "goods_chain_b90", 1, 32, 0, run_goods_chain_b90},
    {0x00443D90u, "goods_chain_d90", 1, 32, 0, run_goods_chain_d90},
    {0x00559480u, "goods_event_list", 1, 8, 0, run_goods_event_list},
    {0x004C5190u, "trade_city_mood", 0, 32, 0, run_trade_city_mood},
    {0x004C5240u, "trade_limit_adjusted", 1, 32, 0, run_trade_limit_adjusted},
    {0x00447790u, "empire_city_trades", 2, 8, 0, run_empire_city_trades},
    {0x0046B100u, "storage_create", 1, 32, 0, run_storage_create},
    {0x0046B320u, "load_rebuild_storage_links", 0, 0, 0, run_load_rebuild_storage_links},
    {0x0046EFD0u, "rec88_create", 1, 32, 0, run_rec88_create},
    {0x0046F030u, "load_rebuild_0x88_links", 0, 0, 0, run_load_rebuild_0x88_links},
    {0x0046A280u, "rec4a_create", 1, 0, 0, run_rec4a_create},
    {0x0046A320u, "load_rebuild_0x4a_links", 0, 0, 0, run_load_rebuild_0x4a_links},
    {0x0046F930u, "map_tile_inside", 2, 8, 0, run_map_tile_inside},
    {0x0046FA40u, "pre_load_clear", 0, 0, 0, run_pre_load_clear},
    {0x004DEA50u, "terrain_rebuild", 0, 0, 0, run_terrain_rebuild},
    {0x0051F780u, "nothing", 0, 0, 0, run_nothing},
    {0x004CDC70u, "anim_counters", 0, 0, 0, run_anim_counters},
    {0x00452150u, "clear_0x0078d784", 0, 0, 0, run_clear_0x0078d784},
    {0x004E7E40u, "countdown_0x00ea89dc", 0, 0, 0, run_countdown_0x00ea89dc},
    /* sub-passes of 0x004DEA50, run over the whole map (the harness passes the same arguments) */
    {0x004770F0u, "pass_special_rocks", 4, 0, 0, run_pass_special_rocks},
    {0x00475DC0u, "pass_rocks", 0, 0, 0, run_pass_rocks},
    {0x00475BF0u, "pass_trees", 4, 0, 0, run_pass_trees},
    {0x00476530u, "pass_flag_0x20", 0, 0, 0, run_pass_flag_0x20},
    {0x0047D870u, "pass_signposts", 1, 0, 0, run_pass_signposts},
    {0x004767D0u, "pass_land", 6, 0, 0, run_pass_land},
    {0x004777C0u, "pass_floodplain", 4, 0, 0, run_pass_floodplain},
    {0x00477500u, "pass_meadow", 4, 0, 0, run_pass_meadow},
    {0x004776F0u, "pass_flag_0x40000", 4, 0, 0, run_pass_flag_0x40000},
    {0x00477210u, "pass_dunes", 0, 0, 0, run_pass_dunes},
    {0x00475CC0u, "pass_water", 5, 0, 0, run_pass_water},
    {0x00477890u, "pass_rubble", 4, 0, 0, run_pass_rubble},
    {0x0047B570u, "pass_roads", 0, 0, 0, run_pass_roads},
    {0x00475720u, "pass_plaza", 5, 0, 0, run_pass_plaza},
    {0x0047B1A0u, "pass_walls", 0, 0, 0, run_pass_walls},
    {0x0047BAE0u, "pass_canals", 5, 0, 0, run_pass_canals},
    {0x004BE160u, "floodplain_build", 4, 0, 0x00D05720u, run_floodplain_build},
};
const int ported_function_count = sizeof(ported_functions) / sizeof(ported_functions[0]);

const ported_function *ported_find(uint32_t address)
{
    for (int i = 0; i < ported_function_count; i++) {
        if (ported_functions[i].address == address) {
            return &ported_functions[i];
        }
    }
    return NULL;
}
