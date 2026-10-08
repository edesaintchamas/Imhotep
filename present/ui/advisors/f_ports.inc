/* Imhotep - the advisors: port entries of group f (see advisors.h),
 * included by ports.c into the single table advisors_ports.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "advisors.h"
#include "f.h"

#include "state/ported.h"

static int32_t run_page12(game_state *st, const int32_t *a)
{
    (void) a;
    advisors_draw_page12(st);
    return 0;
}

static int32_t run_page13(game_state *st, const int32_t *a)
{
    (void) a;
    advisors_draw_page13(st);
    return 0;
}

static int32_t run_strip(game_state *st, const int32_t *a)
{
    advisors_draw_strip(st, a[0], a[1], a[2]);
    return 0;
}

#define ADVISORS_F_ENTRIES \
    {0x0052A250u, "advisors_draw_page12", 0, 0, 0, run_page12, 0}, \
    {0x0052C570u, "advisors_draw_page13", 0, 0, 0, run_page13, 0}, \
    {0x00536BA0u, "advisors_draw_strip", 3, 0, 0, run_strip, 0},
