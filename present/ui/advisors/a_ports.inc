/* Imhotep - the advisors: port entries of group a (see advisors.h),
 * included by ports.c into the single table advisors_ports.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "a.h"
#include "advisors.h"

#include "state/ported.h"

static int32_t run_labour(game_state *st, const int32_t *a) { (void) a; advisor_labour_draw(st); return 0; }
static int32_t run_labour_rows(game_state *st, const int32_t *a) { (void) a; advisor_labour_rows_draw(st); return 0; }
static int32_t run_priority(game_state *st, const int32_t *a) { (void) a; advisor_labour_priority_draw(st); return 0; }
static int32_t run_legions_count(game_state *st, const int32_t *a) { (void) a; return legions_count(st); }
static int32_t run_military(game_state *st, const int32_t *a) { (void) a; advisor_military_draw(st); return 0; }
static int32_t run_at_wharf(game_state *st, const int32_t *a) { return ship_at_wharf(st, a[0]); }
static int32_t run_fleet(game_state *st, const int32_t *a) { (void) a; advisor_fleet_draw(st); return 0; }
static int32_t run_health(game_state *st, const int32_t *a) { return ship_health_text(st, a[0]); }
static int32_t run_state(game_state *st, const int32_t *a) { return ship_state_text(st, a[0]); }

#define ADVISORS_A_ENTRIES \
    {0x00521080u, "advisor_labour_draw", 0, 0, 0, run_labour, 0}, \
    {0x005215D0u, "advisor_labour_rows_draw", 0, 0, 0, run_labour_rows, 0}, \
    {0x00521800u, "advisor_labour_priority_draw", 0, 0, 0, run_priority, 0}, \
    {0x004B78F0u, "legions_count", 0, 32, 0, run_legions_count, 0}, \
    {0x00521B50u, "advisor_military_draw", 0, 0, 0, run_military, 0}, \
    {0x00522D80u, "ship_at_wharf", 1, 8, 0, run_at_wharf, 0}, \
    {0x00522E10u, "advisor_fleet_draw", 0, 0, 0, run_fleet, 0}, \
    {0x0050FC10u, "ship_health_text", 1, 32, 0, run_health, 0}, \
    {0x0050FC70u, "ship_state_text", 1, 32, 0, run_state, 0},
