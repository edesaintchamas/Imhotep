/* Imhotep - the advisors: port entries of group b (see advisors.h),
 * included by ports.c into the single table advisors_ports.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "advisors.h"
#include "b.h"

#include "state/ported.h"

static int32_t run_ratings(game_state *st, const int32_t *a)
{
    (void) a;
    advisor_ratings_draw(st);
    return 0;
}

static int32_t run_ratings_frame(game_state *st, const int32_t *a)
{
    ratings_frame(st, a[0]);
    return 0;
}

static int32_t run_ratings_column(game_state *st, const int32_t *a)
{
    ratings_column(st, a[0], a[1], a[2], a[3]);
    return 0;
}

static int32_t run_explain(game_state *st, const int32_t *a)
{
    (void) a;
    advisor_explain_draw(st);
    return 0;
}

static int32_t run_population(game_state *st, const int32_t *a)
{
    (void) a;
    advisor_population_draw(st);
    return 0;
}

static int32_t run_history(game_state *st, const int32_t *a)
{
    population_graph_history(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_census(game_state *st, const int32_t *a)
{
    population_graph_census(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_society(game_state *st, const int32_t *a)
{
    population_graph_society(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_scale(game_state *st, const int32_t *a)
{
    population_graph_scale(st, a[0]);
    return 0;
}

static int32_t run_history_at(game_state *st, const int32_t *a)
{
    return population_history_at(st, a[0], a[1]);
}

static int32_t run_history_dates(game_state *st, const int32_t *a)
{
    return population_history_dates(st, a[0]);
}

#define ADVISORS_B_ENTRIES \
    {0x004F6AF0u, "advisor_ratings_draw", 0, 0, 0, run_ratings, 0}, \
    {0x004F7720u, "ratings_frame", 1, 0, 0, run_ratings_frame, 0}, \
    {0x004F77E0u, "ratings_column", 4, 0, 0, run_ratings_column, 0}, \
    {0x00538050u, "advisor_explain_draw", 0, 0, 0, run_explain, 0}, \
    {0x00525CB0u, "advisor_population_draw", 0, 0, 0, run_population, 0}, \
    {0x00527360u, "population_graph_history", 3, 0, 0, run_history, 0}, \
    {0x00527710u, "population_graph_census", 3, 0, 0, run_census, 0}, \
    {0x005278E0u, "population_graph_society", 3, 0, 0, run_society, 0}, \
    {0x00527AD0u, "population_graph_scale", 1, 0, 0, run_scale, 0}, \
    {0x004E9760u, "population_history_at", 2, 32, 0, run_history_at, 0}, \
    {0x004E97C0u, "population_history_dates", 1, 32, 0, run_history_dates, 0},
