/* Imhotep - the advisors: the package's single port table advisors_ports (hall.c, callbacks.c, and
 * the entries of groups a-f, X_ports.inc: one table, as the registry holds a bounded number of
 * tables, MAX_EXTRA in core/state/ported.c). @bin each entry is the port of the original function
 * at its address (the functions and their tags: hall.c, callbacks.c, [a-f]_*.c).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "advisors.h"

#include "state/ported.h"

#include "a_ports.inc"
#include "b_ports.inc"
#undef RUN0
#include "c_ports.inc"
#undef RUN
#include "d_ports.inc"
#include "e_ports.inc"
#include "f_ports.inc"

#define VOID0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) { (void) a; fn(st); return 0; }

VOID0(run_draw, advisors_draw)
VOID0(run_hall, advisors_draw_hall)
VOID0(run_grave_goods, advisors_draw_grave_goods)
VOID0(run_rating_info, advisors_draw_rating_info)
VOID0(run_tab, advisor_tab_click)
VOID0(run_help, advisor_help_click)
VOID0(run_empire, advisor_open_empire)
VOID0(run_festival_help, advisor_festival_help)
VOID0(run_festival_close, advisor_festival_close)
VOID0(run_trade_help, advisor_trade_help)
VOID0(run_salary_close, advisor_salary_close)
VOID0(run_ratings_open, advisor_ratings_open)
VOID0(run_set_4430, advisor_set_4430)
VOID0(run_labour_row, advisor_labour_row)
VOID0(run_legion_go_to, advisor_legion_go_to)
VOID0(run_ship_go_to, advisor_ship_go_to)
VOID0(run_donation_open, advisor_donation_open)
VOID0(run_salary_open, advisor_salary_open)
VOID0(run_gift_open, advisor_gift_open)
VOID0(run_request_row, advisor_request_row)
VOID0(run_population_graph, advisor_population_graph)
VOID0(run_festival_open, advisor_festival_open)
VOID0(run_rating_explain, advisor_rating_explain)
VOID0(run_monument_go_to, advisor_monument_go_to)

static int32_t run_events_any_pending(game_state *st, const int32_t *a)
{
    (void) a;
    return events_any_pending(st);
}

static int32_t run_layout_group_at(game_state *st, const int32_t *a)
{
    layout_group_at(st, a[0], a[1], (uint32_t) a[2], a[3], a[4], a[5], a[6], a[7], a[8]);
    return 0;
}

const ported_function advisors_ports[] = {
    {0x00520EA0u, "advisors_draw", 0, 0, 0, run_draw, 0},
    {0x00520F40u, "advisors_draw_hall", 0, 0, 0, run_hall, 0},
    {0x00537940u, "advisors_draw_grave_goods", 0, 0, 0, run_grave_goods, 0},
    {0x00538850u, "advisors_draw_rating_info", 0, 0, 0, run_rating_info, 0},
    {0x0040B4E0u, "advisor_tab_click", 0, 0, 0, run_tab, 0},
    {0x00407290u, "advisor_help_click", 0, 0, 0, run_help, 0},
    {0x00409530u, "advisor_open_empire", 0, 0, 0, run_empire, 0},
    {0x0040AAC0u, "advisor_festival_help", 0, 0, 0, run_festival_help, 0},
    {0x0040AAE0u, "advisor_festival_close", 0, 0, 0, run_festival_close, 0},
    {0x00559DE0u, "advisor_trade_help", 0, 0, 0, run_trade_help, 0},
    {0x0040AF00u, "advisor_salary_close", 0, 0, 0, run_salary_close, 0},
    {0x0040D1A0u, "advisor_ratings_open", 0, 0, 0, run_ratings_open, 0},
    {0x0040ACE0u, "advisor_set_4430", 0, 0, 0, run_set_4430, 0},
    {0x0040A6C0u, "advisor_labour_row", 0, 0, 0, run_labour_row, 0},
    {0x0040A2F0u, "advisor_legion_go_to", 0, 0, 0, run_legion_go_to, 0},
    {0x0040A560u, "advisor_ship_go_to", 0, 0, 0, run_ship_go_to, 0},
    {0x0040AD00u, "advisor_donation_open", 0, 0, 0, run_donation_open, 0},
    {0x0040AD30u, "advisor_salary_open", 0, 0, 0, run_salary_open, 0},
    {0x0040AD90u, "advisor_gift_open", 0, 0, 0, run_gift_open, 0},
    {0x0040AE10u, "advisor_request_row", 0, 0, 0, run_request_row, 0},
    {0x0040A8A0u, "advisor_population_graph", 0, 0, 0, run_population_graph, 0},
    {0x0040AA50u, "advisor_festival_open", 0, 0, 0, run_festival_open, 0},
    {0x0040D130u, "advisor_rating_explain", 0, 0, 0, run_rating_explain, 0},
    {0x0040CF20u, "advisor_monument_go_to", 0, 0, 0, run_monument_go_to, 0},
    {0x00446610u, "events_any_pending", 0, 32, 0, run_events_any_pending, 0},
    {0x004D6200u, "layout_group_at", 9, 0, 0, run_layout_group_at, 0},
    ADVISORS_A_ENTRIES
    ADVISORS_B_ENTRIES
    ADVISORS_C_ENTRIES
    ADVISORS_D_ENTRIES
    ADVISORS_E_ENTRIES
    ADVISORS_F_ENTRIES
};
const int advisors_ports_count = (int) (sizeof(advisors_ports) / sizeof(advisors_ports[0]));
