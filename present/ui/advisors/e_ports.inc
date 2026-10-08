/* Imhotep - the advisors: port entries of group e (see advisors.h),
 * included by ports.c into the single table advisors_ports.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "e.h"

#include "state/ported.h"

static int32_t run_gift_amounts(game_state *st, const int32_t *a)
{
    (void) a;
    advisor_gift_amounts(st);
    return 0;
}

static int32_t run_king_name(game_state *st, const int32_t *a)
{
    (void) a;
    return (int32_t) advisor_king_name(st);
}

static int32_t run_event_months(game_state *st, const int32_t *a)
{
    return advisor_event_months(st, (uint32_t) a[PORT_THIS_ARG]);
}

static int32_t run_kingdom(game_state *st, const int32_t *a)
{
    (void) a;
    advisor_kingdom_draw(st);
    return 0;
}

#define RUN0(name, fn)                                    \
    static int32_t name(game_state *st, const int32_t *a) \
    {                                                     \
        (void) a;                                         \
        fn(st);                                           \
        return 0;                                         \
    }
RUN0(run_religion, advisor_religion_draw)
RUN0(run_monuments, advisor_monuments_draw)
RUN0(run_salary, advisor_salary_draw)
RUN0(run_donation, advisor_donation_draw)
RUN0(run_gift, advisor_gift_draw)
RUN0(run_open_route, advisor_open_route_draw)

#define ADVISORS_E_ENTRIES \
    {0x004E7DB0u, "advisor_gift_amounts", 0, 0, 0, run_gift_amounts, 0}, \
    {0x00447840u, "advisor_king_name", 0, 32, 0, run_king_name, 0}, \
    {0x0044C100u, "advisor_event_months", 0, 32, 0, run_event_months, 0}, \
    {0x00523EA0u, "advisor_kingdom_draw", 0, 0, 0, run_kingdom, 0}, \
    {0x005298E0u, "advisor_religion_draw", 0, 0, 0, run_religion, 0}, \
    {0x0052B520u, "advisor_monuments_draw", 0, 0, 0, run_monuments, 0}, \
    {0x00524E20u, "advisor_salary_draw", 0, 0, 0, run_salary, 0}, \
    {0x005250F0u, "advisor_donation_draw", 0, 0, 0, run_donation, 0}, \
    {0x005255A0u, "advisor_gift_draw", 0, 0, 0, run_gift, 0}, \
    {0x00525B90u, "advisor_open_route_draw", 0, 0, 0, run_open_route, 0},
