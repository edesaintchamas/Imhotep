/* Imhotep - registration of the presentation code's port tables.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "present.h"

#include <stdlib.h>

#include "state/ported.h"

/* The tables hold ports of the original's drawing and UI functions (@bin addresses in each module),
 * dispatched by sim_call like core's (core/sim/call.c).
 * One line per module of present/: PRESENT_TABLE(name) here, and REGISTER(name) below. */
#define PRESENT_TABLE(name) PORTED_TABLE(name)

PRESENT_TABLE(cityview_ports)
PRESENT_TABLE(cityview_ground_ports)
PRESENT_TABLE(cityview_common_ports)
PRESENT_TABLE(cityview_upper_ports)
PRESENT_TABLE(cityview_overlay_ports)
PRESENT_TABLE(cityview_overlay_column_ports)
PRESENT_TABLE(cityview_overlay_tile_ports)
PRESENT_TABLE(cityview_overlay_problem_ports)
PRESENT_TABLE(cityview_anim_ports)
PRESENT_TABLE(cityview_figure_ports)
PRESENT_TABLE(cityview_storage_ports)
PRESENT_TABLE(cityview_levels_ports)
PRESENT_TABLE(cityview_marks_ports)
PRESENT_TABLE(cityview_camera_ports)
PRESENT_TABLE(cityview_ghost_ports)
PRESENT_TABLE(cityview_parts_ports)
PRESENT_TABLE(cityview_ghost_ferry_ports)
PRESENT_TABLE(cityview_ghost_big_ports)
PRESENT_TABLE(cityview_ghost_gate_ports)
PRESENT_TABLE(cityview_ghost_monument_ports)
PRESENT_TABLE(sidebar_ports)
PRESENT_TABLE(minimap_ports)
PRESENT_TABLE(overlay_level_ports)
PRESENT_TABLE(tooltip_ports)
PRESENT_TABLE(tooltip_text_ports)
PRESENT_TABLE(tooltip_draw_ports)
PRESENT_TABLE(sidebar_input_ports)
PRESENT_TABLE(draw_ports)
PRESENT_TABLE(frontend_victory_ports)
PRESENT_TABLE(frontend_numpad_ports)
PRESENT_TABLE(frontend_scenario_ports)
PRESENT_TABLE(frontend_screens_ports)
PRESENT_TABLE(frontend_files_ports)
PRESENT_TABLE(frontend_options_ports)
PRESENT_TABLE(frontend_campaign_ports)
PRESENT_TABLE(ui_dispatch_ports)
PRESENT_TABLE(ui_window_ports)
PRESENT_TABLE(ui_msgwin_ports)
PRESENT_TABLE(ui_empire_editor_ports)
PRESENT_TABLE(frontend_family_ports)
PRESENT_TABLE(info_ports)
PRESENT_TABLE(info_figure_ports)
PRESENT_TABLE(info_house_ports)
PRESENT_TABLE(info_industry_ports)
PRESENT_TABLE(info_storage_ports)
PRESENT_TABLE(info_service_ports)
PRESENT_TABLE(info_service2_ports)
PRESENT_TABLE(info_service3_ports)
PRESENT_TABLE(info_military_ports)
PRESENT_TABLE(info_monument_ports)
PRESENT_TABLE(info_phrase_ports)
PRESENT_TABLE(frontend_menu_ports)
PRESENT_TABLE(msglist_ports)
PRESENT_TABLE(msg_expand_ports)
PRESENT_TABLE(msg_keyword_ports)
PRESENT_TABLE(richtext_ports)
PRESENT_TABLE(msgwindow_ports)
PRESENT_TABLE(empire_ports)
PRESENT_TABLE(mission_ports)
PRESENT_TABLE(msgpopup_ports)
PRESENT_TABLE(empire_editor_ports)
PRESENT_TABLE(advisors_ports)
PRESENT_TABLE(ui_debug_panel_ports)
PRESENT_TABLE(ui_cityframe_ports)
PRESENT_TABLE(frontend_highscore_ports)
PRESENT_TABLE(boot_loading_ports)
PRESENT_TABLE(play_ports)
PRESENT_TABLE(ui_button_ports)
PRESENT_TABLE(info_query_ports)
PRESENT_TABLE(ui_frame_ports)
PRESENT_TABLE(fade_ports)

static const struct {
    const ported_function *table;
    const int *count;
} present_modules[] = {
    {0, 0}, /* placeholder: replace with {name, &name_count} entries */
    {cityview_ports, &cityview_ports_count},
    {cityview_ground_ports, &cityview_ground_ports_count},
    {cityview_common_ports, &cityview_common_ports_count},
    {cityview_upper_ports, &cityview_upper_ports_count},
    {cityview_overlay_ports, &cityview_overlay_ports_count},
    {cityview_overlay_column_ports, &cityview_overlay_column_ports_count},
    {cityview_overlay_tile_ports, &cityview_overlay_tile_ports_count},
    {cityview_overlay_problem_ports, &cityview_overlay_problem_ports_count},
    {cityview_anim_ports, &cityview_anim_ports_count},
    {cityview_figure_ports, &cityview_figure_ports_count},
    {cityview_storage_ports, &cityview_storage_ports_count},
    {cityview_levels_ports, &cityview_levels_ports_count},
    {cityview_marks_ports, &cityview_marks_ports_count},
    {cityview_camera_ports, &cityview_camera_ports_count},
    {cityview_ghost_ports, &cityview_ghost_ports_count},
    {cityview_parts_ports, &cityview_parts_ports_count},
    {cityview_ghost_ferry_ports, &cityview_ghost_ferry_ports_count},
    {cityview_ghost_big_ports, &cityview_ghost_big_ports_count},
    {cityview_ghost_gate_ports, &cityview_ghost_gate_ports_count},
    {cityview_ghost_monument_ports, &cityview_ghost_monument_ports_count},
    {sidebar_ports, &sidebar_ports_count},
    {minimap_ports, &minimap_ports_count},
    {overlay_level_ports, &overlay_level_ports_count},
    {tooltip_ports, &tooltip_ports_count},
    {tooltip_text_ports, &tooltip_text_ports_count},
    {tooltip_draw_ports, &tooltip_draw_ports_count},
    {sidebar_input_ports, &sidebar_input_ports_count},
    {draw_ports, &draw_ports_count},
    {frontend_victory_ports, &frontend_victory_ports_count},
    {frontend_numpad_ports, &frontend_numpad_ports_count},
    {frontend_scenario_ports, &frontend_scenario_ports_count},
    {frontend_screens_ports, &frontend_screens_ports_count},
    {frontend_files_ports, &frontend_files_ports_count},
    {frontend_options_ports, &frontend_options_ports_count},
    {frontend_campaign_ports, &frontend_campaign_ports_count},
    {ui_dispatch_ports, &ui_dispatch_ports_count},
    {ui_window_ports, &ui_window_ports_count},
    {ui_msgwin_ports, &ui_msgwin_ports_count},
    {ui_empire_editor_ports, &ui_empire_editor_ports_count},
    {frontend_family_ports, &frontend_family_ports_count},
    {info_ports, &info_ports_count},
    {info_figure_ports, &info_figure_ports_count},
    {info_house_ports, &info_house_ports_count},
    {info_industry_ports, &info_industry_ports_count},
    {info_storage_ports, &info_storage_ports_count},
    {info_service_ports, &info_service_ports_count},
    {info_service2_ports, &info_service2_ports_count},
    {info_service3_ports, &info_service3_ports_count},
    {info_military_ports, &info_military_ports_count},
    {info_monument_ports, &info_monument_ports_count},
    {info_phrase_ports, &info_phrase_ports_count},
    {frontend_menu_ports, &frontend_menu_ports_count},
    {msglist_ports, &msglist_ports_count},
    {msg_expand_ports, &msg_expand_ports_count},
    {msg_keyword_ports, &msg_keyword_ports_count},
    {richtext_ports, &richtext_ports_count},
    {msgwindow_ports, &msgwindow_ports_count},
    {empire_ports, &empire_ports_count},
    {mission_ports, &mission_ports_count},
    {msgpopup_ports, &msgpopup_ports_count},
    {empire_editor_ports, &empire_editor_ports_count},
    {ui_debug_panel_ports, &ui_debug_panel_ports_count},
    {ui_cityframe_ports, &ui_cityframe_ports_count},
    {frontend_highscore_ports, &frontend_highscore_ports_count},
    {boot_loading_ports, &boot_loading_ports_count},
    {advisors_ports, &advisors_ports_count},
    {play_ports, &play_ports_count},
    {ui_button_ports, &ui_button_ports_count},
    {info_query_ports, &info_query_ports_count},
    {ui_frame_ports, &ui_frame_ports_count},
    {fade_ports, &fade_ports_count},
};

void present_register(void)
{
    static int done;
    if (done) {
        return;
    }
    done = 1;
    for (unsigned i = 0; i < sizeof(present_modules) / sizeof(present_modules[0]); i++) {
        if (present_modules[i].table && ported_add_table(present_modules[i].table, present_modules[i].count) != 0) {
            abort(); /* more tables than the registry holds (MAX_EXTRA, core/state/ported.c): raise it */
        }
    }
}
