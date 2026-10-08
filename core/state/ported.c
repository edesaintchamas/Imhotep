/* Imhotep - ported game functions by their address in Pharaoh.exe.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "state/ported.h"

#include <stddef.h>
#include <string.h>

#include "city/desirability.h"
#include "city/goods.h"
#include "map/routing.h"
#include "map/terrain.h"
#include "sim/small.h"
#include "sim/tick.h"
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

static int32_t run_nothing(game_state *st, const int32_t *a)
{
    (void) st;
    (void) a;
    return 0; /* @bin 0x0051F780 an empty function */
}

static int32_t run_game_tick(game_state *st, const int32_t *a)
{
    (void) a;
    game_tick(st, st->env);
    return 0;
}

static int32_t run_day_dispatch(game_state *st, const int32_t *a)
{
    (void) a;
    day_dispatch(st, st->env);
    return 0;
}

static int32_t run_day_cascade(game_state *st, const int32_t *a)
{
    (void) a;
    day_cascade(st, st->env);
    return 0;
}

/* @bin 0x0040F8F0 fill(p, n, byte): dwords then the remaining bytes (the game's own memset) */
static int32_t run_fill_bytes(game_state *st, const int32_t *a)
{
    uint8_t *p = state_ptr(st, (uint32_t) a[0], (uint32_t) a[1]);
    if (p) {
        memset(p, (uint8_t) a[2], (uint32_t) a[1]);
    } else if (a[1] != 0) {
        state_unported(st, 0x0040F8F0u); /* outside the state: a caller's local buffer */
    }
    return 0;
}

VOID0(run_anim_counters, anim_counters)
VOID0(run_clear_0x0078d784, clear_0x0078d784)
VOID0(run_countdown_0x00ea89dc, countdown_0x00ea89dc)

/* @bin addresses of the original functions (each port carries its own tags) */
const ported_function state_ports[] = {
    {0x0046A0C0u, "desirability_buildings", 0, 0, 0, run_desirability_buildings, 0},
    {0x0046A140u, "desirability_tiles", 0, 0, 0, run_desirability_tiles, 0},
    {0x0046F7B0u, "map_row_ranges", 0, 0, 0, run_map_row_ranges, 0},
    {0x00517A20u, "routing_clear_ee0c50", 0, 0, 0, run_routing_clear_ee0c50, 0},
    {0x005178B0u, "routing_rebuild_ed4140", 0, 0, 0, run_routing_rebuild_ed4140, 0},
    {0x005178E0u, "routing_rebuild_ebab20", 0, 0, 0, run_routing_rebuild_ebab20, 0},
    {0x00517660u, "routing_rebuild_ec7630", 0, 0, 0, run_routing_rebuild_ec7630, 0},
    {0x00517370u, "routing_rebuild_ee0c50", 0, 0, 0, run_routing_rebuild_ee0c50, 0},
    {0x00517690u, "routing_fill_ed4140", 4, 0, 0, run_fill_ed4140, 0},
    {0x005173A0u, "routing_fill_ec7630", 4, 0, 0, run_fill_ec7630, 0},
    {0x00516F90u, "routing_fill_ee0c50", 5, 0, 0, run_fill_ee0c50, 0},
    {0x005596D0u, "goods_available_update", 0, 0, 0, run_goods_available_update, 0},
    {0x004439C0u, "goods_trade_city", 1, 32, 0, run_goods_trade_city, 0},
    {0x00443950u, "goods_trade_city_flag5", 1, 32, 0, run_goods_trade_city_flag5, 0},
    {0x00443B90u, "goods_chain_b90", 1, 32, 0, run_goods_chain_b90, 0},
    {0x00443D90u, "goods_chain_d90", 1, 32, 0, run_goods_chain_d90, 0},
    {0x00559480u, "goods_event_list", 1, 8, 0, run_goods_event_list, 0},
    {0x004C5190u, "trade_city_mood", 0, 32, 0, run_trade_city_mood, 0},
    {0x004C5240u, "trade_limit_adjusted", 1, 32, 0, run_trade_limit_adjusted, 0},
    {0x00447790u, "empire_city_trades", 2, 8, 0, run_empire_city_trades, 0},
    {0x0046B100u, "storage_create", 1, 32, 0, run_storage_create, 0},
    {0x0046B320u, "load_rebuild_storage_links", 0, 0, 0, run_load_rebuild_storage_links, 0},
    {0x0046EFD0u, "rec88_create", 1, 32, 0, run_rec88_create, 0},
    {0x0046F030u, "load_rebuild_0x88_links", 0, 0, 0, run_load_rebuild_0x88_links, 0},
    {0x0046A280u, "rec4a_create", 1, 0, 0, run_rec4a_create, 0},
    {0x0046A320u, "load_rebuild_0x4a_links", 0, 0, 0, run_load_rebuild_0x4a_links, 0},
    {0x0046F930u, "map_tile_inside", 2, 8, 0, run_map_tile_inside, 0},
    {0x0046FA40u, "pre_load_clear", 0, 0, 0, run_pre_load_clear, 0},
    {0x004DEA50u, "terrain_rebuild", 0, 0, 0, run_terrain_rebuild, 0},
    {0x004D7090u, "game_tick", 0, 0, 0, run_game_tick, 0},
    {0x00451C50u, "day_dispatch", 0, 0, 0, run_day_dispatch, 0},
    {0x00452370u, "day_cascade", 0, 0, 0, run_day_cascade, 0},
    {0x0051F780u, "nothing", 0, 0, 0, run_nothing, 0},
    {0x004CDC70u, "anim_counters", 0, 0, 0, run_anim_counters, 0},
    {0x00452150u, "clear_0x0078d784", 0, 0, 0, run_clear_0x0078d784, 0},
    {0x004E7E40u, "countdown_0x00ea89dc", 0, 0, 0, run_countdown_0x00ea89dc, 0},
    {0x004BE160u, "floodplain_build", 4, 0, 0x00D05720u, run_floodplain_build, 0},
    {0x0040F8F0u, "fill_bytes", 3, 0, 0, run_fill_bytes, 0},
    /* helpers ported inside their callers (not callable on their own): the oracle does not record
     * them, since every call from ported code is handled by the port */
    {0x00481830u, "desirability_rings (in desirability.c)", 7, 0, 0, NULL, 0},
    {0x004818A0u, "desirability_ring (in desirability.c)", 5, 0, 0, NULL, 0},
    /* 0x00481AA0: day2_misc_ports (ring_tile_outside) */
    {0x005013A0u, "storage_kind (in goods.c)", 1, 32, 0, NULL, 0},
    {0x0046FC30u, "pre_load_outside_tiles (in pre_load.c)", 0, 0, 0, NULL, 0},
    {0x0046FC80u, "pre_load_random_grid (in pre_load.c)", 0, 0, 0, NULL, 0},
    {0x00475B80u, "pre_load_image_grid (in pre_load.c)", 0, 0, 0, NULL, 0},
    {0x00484EE0u, "pre_load_table_flags (in pre_load.c)", 0, 0, 0, NULL, 0},
    {0x004BE0F0u, "floodplain_clear (in pre_load.c)", 0, 0, 0, NULL, 0},
    {0x004BD6A0u, "floodplain_list_release (in pre_load.c)", 0, 0, 0, NULL, 0},
    /* the CRT heap: state_alloc / state_dealloc (core/state/state.h) */
    {0x00561A1Du, "operator new (state_alloc)", 1, 32, 0, NULL, 0},
    {0x00561A2Bu, "operator delete (state_dealloc)", 1, 0, 0, NULL, 0},
    {0x00561B68u, "malloc (state_alloc)", 1, 32, 0, NULL, 0},
    {0x00561B7Au, "_nh_malloc (state_alloc)", 2, 32, 0, NULL, 0},
    {0x00561BDCu, "free (state_dealloc)", 1, 0, 0, NULL, 0},
};
const int state_ports_count = sizeof(state_ports) / sizeof(state_ports[0]);

/* the modules' tables (core/state/ported.h) */
PORTED_TABLE(state_ports)
PORTED_TABLE(platform_ports)
PORTED_TABLE(frame_sound_ports)
PORTED_TABLE(frame_view_ports)
PORTED_TABLE(text_ports)
PORTED_TABLE(random_ports)
PORTED_TABLE(figure_ports)
PORTED_TABLE(house_ports)
PORTED_TABLE(message_ports)
PORTED_TABLE(route_search_ports)
PORTED_TABLE(building_daily_ports)
PORTED_TABLE(labour_ports)
PORTED_TABLE(day_basic_ports)
PORTED_TABLE(day_access_ports)
PORTED_TABLE(day_buildings_ports)
PORTED_TABLE(day_labour_ports)
PORTED_TABLE(day_anim_ports)
PORTED_TABLE(day_city_ports)
PORTED_TABLE(day_state_ports)
PORTED_TABLE(day_water_ports)
PORTED_TABLE(day_road_ports)
PORTED_TABLE(day_flood_ports)
PORTED_TABLE(x87_ports)
PORTED_TABLE(day_goals_ports)
PORTED_TABLE(day_houses_ports)
PORTED_TABLE(month_ports)
PORTED_TABLE(month_mood_ports)
PORTED_TABLE(month_health_ports)
PORTED_TABLE(month_ratings_ports)
PORTED_TABLE(month_message_ports)
PORTED_TABLE(month_events_ports)
PORTED_TABLE(month2_events_ports)
PORTED_TABLE(month2_handlers_ports)
PORTED_TABLE(month2_festival_ports)
PORTED_TABLE(month2_monument_ports)
PORTED_TABLE(month2_actions_ports)
PORTED_TABLE(month2_troops_ports)
PORTED_TABLE(month2_military_ports)
PORTED_TABLE(figa_ports)
PORTED_TABLE(figa_service_ports)
PORTED_TABLE(figa_service2_ports)
PORTED_TABLE(figd_ports)
PORTED_TABLE(figd_boat_ports)
PORTED_TABLE(figc_ports)
PORTED_TABLE(figc2_ports)
PORTED_TABLE(figb_ports)
PORTED_TABLE(bigb_update_ports)
PORTED_TABLE(bigb_market_ports)
PORTED_TABLE(bigb_route_ports)
PORTED_TABLE(bigb_formation_ports)
PORTED_TABLE(bigb_figure_ports)
PORTED_TABLE(bigb_spawn_ports)
PORTED_TABLE(bigb_map_ports)
PORTED_TABLE(bigb_gatherer_ports)
PORTED_TABLE(bigb_misc_ports)
PORTED_TABLE(bigb_path_ports)
PORTED_TABLE(bigb_link_ports)
PORTED_TABLE(bigb_misc2_ports)
PORTED_TABLE(bigb_extra_ports)
PORTED_TABLE(bigb_invader_ports)
PORTED_TABLE(bigb_army_route_ports)
PORTED_TABLE(bigb_enemy_ports)
PORTED_TABLE(bigb2_land_ports)
PORTED_TABLE(bigb2_queue_ports)
PORTED_TABLE(bigb2_monument_ports)
PORTED_TABLE(bigb2_water_ports)
PORTED_TABLE(bigb2_roads_ports)
PORTED_TABLE(bigb2_flood_ports)
PORTED_TABLE(figx_route_ports)
PORTED_TABLE(figx_ports)
PORTED_TABLE(figx_a_ports)
PORTED_TABLE(figx_b_ports)
PORTED_TABLE(day2_misc_ports)
PORTED_TABLE(day2_monument_ports)
PORTED_TABLE(day2_gods_ports)
PORTED_TABLE(day2_events_ports)
PORTED_TABLE(day2_flood_ports)
PORTED_TABLE(day2_farm_ports)
PORTED_TABLE(day2_docks_ports)
PORTED_TABLE(day2_destroy_ports)
PORTED_TABLE(day2_redraw_ports)
PORTED_TABLE(terr_rocks_ports)
PORTED_TABLE(terr_veg_ports)
PORTED_TABLE(terr_misc_ports)
PORTED_TABLE(terr_rings_ports)
PORTED_TABLE(terr_monument_ports)
PORTED_TABLE(terr_pyramid_ports)
PORTED_TABLE(terr_take_ports)
PORTED_TABLE(terr_coast_ports)
PORTED_TABLE(terr_create_ports)
PORTED_TABLE(rest_destroy_ports)
PORTED_TABLE(rest_monument_ports)
PORTED_TABLE(rest_redraw_ports)
PORTED_TABLE(rest_gods_ports)
PORTED_TABLE(rest_notice_ports)
PORTED_TABLE(rest_misc_ports)
PORTED_TABLE(last_terrain_ports)
PORTED_TABLE(last_figure_ports)
PORTED_TABLE(last_city_ports)
PORTED_TABLE(last_graphics_ports)
PORTED_TABLE(sg3_ports)
PORTED_TABLE(clear_ports)
PORTED_TABLE(undo_ports)
PORTED_TABLE(ui_ports)
PORTED_TABLE(ui_checks_ports)
PORTED_TABLE(ui_path_ports)
PORTED_TABLE(ui_lift_ports)
PORTED_TABLE(place_ports)
PORTED_TABLE(place_notice_ports)
PORTED_TABLE(place_drag_ports)
PORTED_TABLE(place_ent_ports)
PORTED_TABLE(place_misc_ports)
PORTED_TABLE(military_ports)

PORTED_TABLE(orders_ports)
PORTED_TABLE(orders_storage_ports)
PORTED_TABLE(orders_messages_ports)
PORTED_TABLE(trade_ports)
PORTED_TABLE(kingdom_ports)
PORTED_TABLE(cheats_ports)
PORTED_TABLE(rotate_ports)
PORTED_TABLE(map_add_ports)
PORTED_TABLE(map_add_monument_ports)
PORTED_TABLE(menu_ports)
PORTED_TABLE(hover_ports)
PORTED_TABLE(advisor_ports)
PORTED_TABLE(models_ports)
PORTED_TABLE(newgame_start_ports)
PORTED_TABLE(newgame_flow_ports)
PORTED_TABLE(newgame_flow2_ports)
PORTED_TABLE(newgame_figures_ports)
PORTED_TABLE(newgame_graphics_ports)
PORTED_TABLE(reset_ports)
PORTED_TABLE(loader_ports)
PORTED_TABLE(scenario_ports)
PORTED_TABLE(monument_need_ports)
PORTED_TABLE(newgame_map_load_ports)
PORTED_TABLE(newgame_setup_ports)
PORTED_TABLE(newgame_start2_ports)
PORTED_TABLE(frame_ports)
PORTED_TABLE(frame_autosave_ports)
PORTED_TABLE(frame_wndproc_ports)
PORTED_TABLE(boot_settings_ports)
PORTED_TABLE(boot_ctor_ports)
PORTED_TABLE(boot_monument_ports)
PORTED_TABLE(boot_step_ports)
PORTED_TABLE(boot_ports)
PORTED_TABLE(boot_sound_ports)
PORTED_TABLE(route_points_ports)
PORTED_TABLE(display_ports)
static const struct {
    const ported_function *table;
    const int *count;
} ported_modules[] = {
    {state_ports, &state_ports_count},
    {platform_ports, &platform_ports_count},
    {frame_sound_ports, &frame_sound_ports_count},
    {frame_view_ports, &frame_view_ports_count},
    {text_ports, &text_ports_count},
    {random_ports, &random_ports_count},
    {figure_ports, &figure_ports_count},
    {house_ports, &house_ports_count},
    {message_ports, &message_ports_count},
    {route_search_ports, &route_search_ports_count},
    {building_daily_ports, &building_daily_ports_count},
    {labour_ports, &labour_ports_count},
    {day_basic_ports, &day_basic_ports_count},
    {day_access_ports, &day_access_ports_count},
    {day_buildings_ports, &day_buildings_ports_count},
    {day_labour_ports, &day_labour_ports_count},
    {day_anim_ports, &day_anim_ports_count},
    {day_city_ports, &day_city_ports_count},
    {day_state_ports, &day_state_ports_count},
    {day_water_ports, &day_water_ports_count},
    {day_road_ports, &day_road_ports_count},
    {day_flood_ports, &day_flood_ports_count},
    {x87_ports, &x87_ports_count},
    {day_goals_ports, &day_goals_ports_count},
    {day_houses_ports, &day_houses_ports_count},
    {month_ports, &month_ports_count},
    {month_mood_ports, &month_mood_ports_count},
    {month_health_ports, &month_health_ports_count},
    {month_ratings_ports, &month_ratings_ports_count},
    {month_message_ports, &month_message_ports_count},
    {month_events_ports, &month_events_ports_count},
    {month2_events_ports, &month2_events_ports_count},
    {month2_handlers_ports, &month2_handlers_ports_count},
    {month2_festival_ports, &month2_festival_ports_count},
    {month2_monument_ports, &month2_monument_ports_count},
    {month2_actions_ports, &month2_actions_ports_count},
    {month2_troops_ports, &month2_troops_ports_count},
    {month2_military_ports, &month2_military_ports_count},
    {figa_ports, &figa_ports_count},
    {figa_service_ports, &figa_service_ports_count},
    {figa_service2_ports, &figa_service2_ports_count},
    {figd_ports, &figd_ports_count},
    {figd_boat_ports, &figd_boat_ports_count},
    {figc_ports, &figc_ports_count},
    {figc2_ports, &figc2_ports_count},
    {figb_ports, &figb_ports_count},
    {bigb_update_ports, &bigb_update_ports_count},
    {bigb_market_ports, &bigb_market_ports_count},
    {bigb_route_ports, &bigb_route_ports_count},
    {bigb_formation_ports, &bigb_formation_ports_count},
    {bigb_figure_ports, &bigb_figure_ports_count},
    {bigb_spawn_ports, &bigb_spawn_ports_count},
    {bigb_map_ports, &bigb_map_ports_count},
    {bigb_gatherer_ports, &bigb_gatherer_ports_count},
    {bigb_misc_ports, &bigb_misc_ports_count},
    {bigb_path_ports, &bigb_path_ports_count},
    {bigb_link_ports, &bigb_link_ports_count},
    {bigb_misc2_ports, &bigb_misc2_ports_count},
    {bigb_extra_ports, &bigb_extra_ports_count},
    {bigb_invader_ports, &bigb_invader_ports_count},
    {bigb_army_route_ports, &bigb_army_route_ports_count},
    {bigb_enemy_ports, &bigb_enemy_ports_count},
    {bigb2_land_ports, &bigb2_land_ports_count},
    {bigb2_queue_ports, &bigb2_queue_ports_count},
    {bigb2_monument_ports, &bigb2_monument_ports_count},
    {bigb2_water_ports, &bigb2_water_ports_count},
    {bigb2_roads_ports, &bigb2_roads_ports_count},
    {bigb2_flood_ports, &bigb2_flood_ports_count},
    {figx_route_ports, &figx_route_ports_count},
    {figx_ports, &figx_ports_count},
    {figx_a_ports, &figx_a_ports_count},
    {figx_b_ports, &figx_b_ports_count},
    {day2_misc_ports, &day2_misc_ports_count},
    {day2_monument_ports, &day2_monument_ports_count},
    {day2_gods_ports, &day2_gods_ports_count},
    {day2_events_ports, &day2_events_ports_count},
    {day2_flood_ports, &day2_flood_ports_count},
    {day2_farm_ports, &day2_farm_ports_count},
    {day2_docks_ports, &day2_docks_ports_count},
    {day2_destroy_ports, &day2_destroy_ports_count},
    {day2_redraw_ports, &day2_redraw_ports_count},
    {terr_rocks_ports, &terr_rocks_ports_count},
    {terr_veg_ports, &terr_veg_ports_count},
    {terr_misc_ports, &terr_misc_ports_count},
    {terr_rings_ports, &terr_rings_ports_count},
    {terr_monument_ports, &terr_monument_ports_count},
    {terr_pyramid_ports, &terr_pyramid_ports_count},
    {terr_take_ports, &terr_take_ports_count},
    {terr_coast_ports, &terr_coast_ports_count},
    {terr_create_ports, &terr_create_ports_count},
    {rest_destroy_ports, &rest_destroy_ports_count},
    {rest_monument_ports, &rest_monument_ports_count},
    {rest_redraw_ports, &rest_redraw_ports_count},
    {rest_gods_ports, &rest_gods_ports_count},
    {rest_notice_ports, &rest_notice_ports_count},
    {rest_misc_ports, &rest_misc_ports_count},
    {last_terrain_ports, &last_terrain_ports_count},
    {last_figure_ports, &last_figure_ports_count},
    {last_city_ports, &last_city_ports_count},
    {last_graphics_ports, &last_graphics_ports_count},
    {sg3_ports, &sg3_ports_count},
    {clear_ports, &clear_ports_count},
    {undo_ports, &undo_ports_count},
    {ui_ports, &ui_ports_count},
    {ui_checks_ports, &ui_checks_ports_count},
    {ui_path_ports, &ui_path_ports_count},
    {ui_lift_ports, &ui_lift_ports_count},
    {place_ports, &place_ports_count},
    {place_notice_ports, &place_notice_ports_count},
    {place_drag_ports, &place_drag_ports_count},
    {place_ent_ports, &place_ent_ports_count},
    {place_misc_ports, &place_misc_ports_count},
    {military_ports, &military_ports_count},
    {orders_ports, &orders_ports_count},
    {orders_storage_ports, &orders_storage_ports_count},
    {orders_messages_ports, &orders_messages_ports_count},
    {trade_ports, &trade_ports_count},
    {kingdom_ports, &kingdom_ports_count},
    {cheats_ports, &cheats_ports_count},
    {rotate_ports, &rotate_ports_count},
    {map_add_ports, &map_add_ports_count},
    {map_add_monument_ports, &map_add_monument_ports_count},
    {menu_ports, &menu_ports_count},
    {hover_ports, &hover_ports_count},
    {advisor_ports, &advisor_ports_count},
    {models_ports, &models_ports_count},
    {newgame_start_ports, &newgame_start_ports_count},
    {newgame_flow_ports, &newgame_flow_ports_count},
    {newgame_flow2_ports, &newgame_flow2_ports_count},
    {newgame_figures_ports, &newgame_figures_ports_count},
    {newgame_graphics_ports, &newgame_graphics_ports_count},
    {reset_ports, &reset_ports_count},
    {loader_ports, &loader_ports_count},
    {scenario_ports, &scenario_ports_count},
    {monument_need_ports, &monument_need_ports_count},
    {newgame_map_load_ports, &newgame_map_load_ports_count},
    {newgame_setup_ports, &newgame_setup_ports_count},
    {newgame_start2_ports, &newgame_start2_ports_count},
    {frame_ports, &frame_ports_count},
    {frame_autosave_ports, &frame_autosave_ports_count},
    {frame_wndproc_ports, &frame_wndproc_ports_count},
    {boot_settings_ports, &boot_settings_ports_count},
    {boot_ctor_ports, &boot_ctor_ports_count},
    {boot_monument_ports, &boot_monument_ports_count},
    {boot_step_ports, &boot_step_ports_count},
    {boot_ports, &boot_ports_count},
    {boot_sound_ports, &boot_sound_ports_count},
    {route_points_ports, &route_points_ports_count},
    {display_ports, &display_ports_count},
};
#define NMODULES ((int) (sizeof(ported_modules) / sizeof(ported_modules[0])))

/* tables added at start-up by code outside core (present/: present_register) */
#define MAX_EXTRA 512
static struct {
    const ported_function *table;
    const int *count;
} extra_modules[MAX_EXTRA];
static int nextra;

/* open-addressing index of every port by address, built on first use */
#define INDEX_SIZE 16384
static const ported_function *index_table[INDEX_SIZE];
static int index_built;

int ported_add_table(const ported_function *table, const int *count)
{
    if (nextra == MAX_EXTRA) {
        return -1;
    }
    extra_modules[nextra].table = table;
    extra_modules[nextra].count = count;
    nextra++;
    if (index_built) { /* rebuilt with the new table on the next lookup */
        for (int i = 0; i < INDEX_SIZE; i++) {
            index_table[i] = NULL;
        }
        index_built = 0;
    }
    return 0;
}

int ported_count(void)
{
    int n = 0;
    for (int m = 0; m < NMODULES; m++) {
        n += *ported_modules[m].count;
    }
    for (int m = 0; m < nextra; m++) {
        n += *extra_modules[m].count;
    }
    return n;
}

const ported_function *ported_at(int i)
{
    for (int m = 0; m < NMODULES; m++) {
        if (i < *ported_modules[m].count) {
            return &ported_modules[m].table[i];
        }
        i -= *ported_modules[m].count;
    }
    for (int m = 0; m < nextra; m++) {
        if (i < *extra_modules[m].count) {
            return &extra_modules[m].table[i];
        }
        i -= *extra_modules[m].count;
    }
    return NULL;
}

static void build_index(void)
{
    for (int i = 0, n = ported_count(); i < n; i++) {
        const ported_function *p = ported_at(i);
        uint32_t h = (p->address * 2654435761u) >> 19;
        while (index_table[h % INDEX_SIZE]) {
            h++;
        }
        index_table[h % INDEX_SIZE] = p;
    }
    index_built = 1;
}

const ported_function *ported_find(uint32_t address)
{
    if (!index_built) {
        build_index();
    }
    for (uint32_t h = (address * 2654435761u) >> 19;; h++) {
        const ported_function *p = index_table[h % INDEX_SIZE];
        if (!p || p->address == address) {
            return p;
        }
    }
}
