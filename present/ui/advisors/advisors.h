/* Imhotep - the advisors (window 7) and the dialogs over the hall (roadmap phase 7.6, package
 * "advisors").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Ports of Pharaoh.exe's advisor window: the hall 0x00520EA0 / 0x00520F40 (window 7, the page in
 * 0x00605A44), every advisor page's drawing, the dialogs drawn over the hall (windows 0x26, 0x28,
 * 0x2A-0x2E, 0x4C, 0x4D, 0x4F, 0x50, 0x52: 0x004D71C0) and the button callbacks their button tables
 * lead to (input dispatch 0x004D8310, package wincore). The callbacks that change the simulation
 * are the phase-5 commands of core/command/ (docs/commands.md 3). Drawing primitives, text and the
 * button tables' drawing are packages blit and sidebar's (present/draw, present/sidebar).
 *
 * Files (one table, advisors_ports in ports.c, registered in present/present.c; each group lists
 * its entries in X_ports.inc):
 *   hall.c, callbacks.c, ports.c  advisors_ports  the hall, the page switch, the button callbacks
 *   a_*.c    a_ports.inc  pages 1-3 (0x00521080, 0x00521B50, 0x00522E10), dialog 0x2A
 *   b_*.c    b_ports.inc  pages 5, 7 (0x004F6AF0, 0x00525CB0), dialog 0x4F (0x00538050)
 *   c_*.c    c_ports.inc  pages 8-10 (0x00527BF0, 0x00528360, 0x00528950), dialogs 0x2B, 0x52
 *   d_*.c    d_ports.inc  page 6 (trade 0x00559E10), dialog 0x28, the trade callbacks
 *   e_*.c    e_ports.inc  pages 4, 11, 14 (0x00523EA0, 0x005298E0, 0x0052B520), dialogs 0x26,
 *                              0x2C / 0x4D, 0x2D, 0x2E
 *   f_*.c    f_ports.inc  pages 12, 13 (0x0052A250, 0x0052C570)
 *   (hall.c, callbacks.c: dialogs 0x4C 0x00537940, 0x50 0x00538850, helpers 0x00446610, 0x004D6200)
 */
#ifndef IMHOTEP_PRESENT_UI_ADVISORS_H
#define IMHOTEP_PRESENT_UI_ADVISORS_H

#include <stdint.h>

#include "present_layout.h"
#include "state/state.h"

/* hall.c */
void advisors_draw(game_state *st);              /* @bin 0x00520EA0 */
void advisors_draw_hall(game_state *st);         /* @bin 0x00520F40 */
void advisors_draw_grave_goods(game_state *st);  /* @bin 0x00537940 window 0x4C */
void advisors_draw_rating_info(game_state *st);  /* @bin 0x00538850 window 0x50 */

/* callbacks.c: button callbacks (parameters in 0x00D3B198 / 0x00D3B194) and helpers */
void advisor_tab_click(game_state *st);          /* @bin 0x0040B4E0 */
void advisor_help_click(game_state *st);         /* @bin 0x00407290 */
void advisor_open_empire(game_state *st);        /* @bin 0x00409530 */
void advisor_festival_help(game_state *st);      /* @bin 0x0040AAC0 */
void advisor_festival_close(game_state *st);     /* @bin 0x0040AAE0 */
void advisor_trade_help(game_state *st);         /* @bin 0x00559DE0 */
void advisor_salary_close(game_state *st);       /* @bin 0x0040AF00 */
void advisor_ratings_open(game_state *st);       /* @bin 0x0040D1A0 */
void advisor_set_4430(game_state *st);           /* @bin 0x0040ACE0 */
void advisor_labour_row(game_state *st);         /* @bin 0x0040A6C0 */
void advisor_legion_go_to(game_state *st);       /* @bin 0x0040A2F0 */
void advisor_ship_go_to(game_state *st);         /* @bin 0x0040A560 */
void advisor_donation_open(game_state *st);      /* @bin 0x0040AD00 */
void advisor_salary_open(game_state *st);        /* @bin 0x0040AD30 */
void advisor_gift_open(game_state *st);          /* @bin 0x0040AD90 */
void advisor_request_row(game_state *st);        /* @bin 0x0040AE10 */
void advisor_population_graph(game_state *st);  /* @bin 0x0040A8A0 */
void advisor_festival_open(game_state *st);      /* @bin 0x0040AA50 */
void advisor_rating_explain(game_state *st);     /* @bin 0x0040D130 */
void advisor_monument_go_to(game_state *st);     /* @bin 0x0040CF20 */
int32_t events_any_pending(game_state *st);      /* @bin 0x00446610 */
void layout_group_at(game_state *st, int32_t group, int32_t index, uint32_t rect, int32_t font, int32_t colour,
                     int32_t ox, int32_t oy, int32_t dx, int32_t dy); /* @bin 0x004D6200 */

#endif
