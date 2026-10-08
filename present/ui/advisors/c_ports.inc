/* Imhotep - the advisors: port entries of group c (see advisors.h),
 * included by ports.c into the single table advisors_ports.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "advisors.h"
#include "c.h"

#include "state/ported.h"

#define RUN(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
RUN(run_page8, advisor_draw_page8)
RUN(run_page9, advisor_draw_page9)
RUN(run_page10, advisor_draw_page10)
RUN(run_festival_panel, advisor_draw_festival_panel)
RUN(run_festival_button, advisor_draw_festival_button)
RUN(run_festival, festival_dialog_draw)
RUN(run_festival_sizes, festival_dialog_draw_sizes)
RUN(run_festival_building, festival_dialog_draw_building)

#define ADVISORS_C_ENTRIES \
    {0x00527BF0u, "advisor_draw_page8", 0, 0, 0, run_page8, 0}, \
    {0x00528360u, "advisor_draw_page9", 0, 0, 0, run_page9, 0}, \
    {0x00528950u, "advisor_draw_page10", 0, 0, 0, run_page10, 0}, \
    {0x00528F10u, "advisor_draw_festival_panel", 0, 0, 0, run_festival_panel, 0}, \
    {0x005292B0u, "advisor_draw_festival_button", 0, 0, 0, run_festival_button, 0}, \
    {0x00529320u, "festival_dialog_draw", 0, 0, 0, run_festival, 0}, \
    {0x00529530u, "festival_dialog_draw_sizes", 0, 0, 0, run_festival_sizes, 0}, \
    {0x00516080u, "festival_dialog_draw_building", 0, 0, 0, run_festival_building, 0},
