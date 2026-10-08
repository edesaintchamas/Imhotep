/* Imhotep - the advisors, group d: the trade advisor (page 6), the trade resource dialog
 * (window 0x28) and the trade button callbacks (see advisors.h).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_PRESENT_UI_ADVISORS_D_H
#define IMHOTEP_PRESENT_UI_ADVISORS_D_H

#include <stdint.h>

#include "state/state.h"

/* d_trade.c */
void trade_advisor_draw(game_state *st);              /* @bin 0x00559E10 page 6 background */
void trade_advisor_draw_list(game_state *st);         /* @bin 0x0055A0E0 the resource list */
void trade_advisor_draw_scroll_thumb(game_state *st); /* @bin 0x0055A060 */
void trade_dialog_draw(game_state *st, int32_t unused); /* @bin 0x0055AD30 window 0x28 */
int32_t trade_advisor_scroll_input(game_state *st);   /* @bin 0x0055BB90 */

/* d_callbacks.c */
void trade_cb_select(game_state *st);       /* @bin 0x00559820 */
void trade_cb_toggle_prices(game_state *st); /* @bin 0x00559860 */
void trade_cb_empire(game_state *st);       /* @bin 0x00559890 */
void trade_cb_close(game_state *st);        /* @bin 0x00559E00 */
void empire_window_open(game_state *st);    /* @bin 0x00409FA0 */

#endif
