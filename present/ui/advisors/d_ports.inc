/* Imhotep - the advisors: port entries of group d (see advisors.h),
 * included by ports.c into the single table advisors_ports.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "advisors.h"
#include "d.h"

#include "state/ported.h"

static int32_t run_page(game_state *st, const int32_t *a) { (void) a; trade_advisor_draw(st); return 0; }
static int32_t run_list(game_state *st, const int32_t *a) { (void) a; trade_advisor_draw_list(st); return 0; }
static int32_t run_thumb(game_state *st, const int32_t *a) { (void) a; trade_advisor_draw_scroll_thumb(st); return 0; }
static int32_t run_dialog(game_state *st, const int32_t *a) { trade_dialog_draw(st, a[0]); return 0; }
static int32_t run_scroll(game_state *st, const int32_t *a) { (void) a; return trade_advisor_scroll_input(st); }
static int32_t run_select(game_state *st, const int32_t *a) { (void) a; trade_cb_select(st); return 0; }
static int32_t run_prices(game_state *st, const int32_t *a) { (void) a; trade_cb_toggle_prices(st); return 0; }
static int32_t run_d_empire(game_state *st, const int32_t *a) { (void) a; trade_cb_empire(st); return 0; }
static int32_t run_close(game_state *st, const int32_t *a) { (void) a; trade_cb_close(st); return 0; }
static int32_t run_open_empire(game_state *st, const int32_t *a) { (void) a; empire_window_open(st); return 0; }

#define ADVISORS_D_ENTRIES \
    {0x00559E10u, "trade_advisor_draw", 0, 0, 0, run_page, 0}, \
    {0x0055A0E0u, "trade_advisor_draw_list", 0, 0, 0, run_list, 0}, \
    {0x0055A060u, "trade_advisor_draw_scroll_thumb", 0, 0, 0, run_thumb, 0}, \
    {0x0055AD30u, "trade_dialog_draw", 1, 0, 0, run_dialog, 0}, \
    {0x0055BB90u, "trade_advisor_scroll_input", 0, 32, 0, run_scroll, 0}, \
    {0x00559820u, "trade_cb_select", 0, 0, 0, run_select, 0}, \
    {0x00559860u, "trade_cb_toggle_prices", 0, 0, 0, run_prices, 0}, \
    {0x00559890u, "trade_cb_empire", 0, 0, 0, run_d_empire, 0}, \
    {0x00559E00u, "trade_cb_close", 0, 0, 0, run_close, 0}, \
    {0x00409FA0u, "empire_window_open", 0, 0, 0, run_open_empire, 0},
