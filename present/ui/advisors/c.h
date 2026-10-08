/* Imhotep - the advisors, group c: pages 8-10 and the festival dialog (windows 0x2B, 0x52).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_PRESENT_UI_ADVISORS_C_H
#define IMHOTEP_PRESENT_UI_ADVISORS_C_H

#include <stdint.h>

#include "state/state.h"

#define ADV_OPENING 0x00F53F48u /* i32 the page's opening count (6 on opening; 5 - it > 0: a growing frame) */

/* c_pages.c */
void advisor_draw_page8(game_state *st);       /* @bin 0x00527BF0 */
void advisor_draw_page9(game_state *st);       /* @bin 0x00528360 */
void advisor_draw_page10(game_state *st);      /* @bin 0x00528950 */
void advisor_draw_festival_panel(game_state *st); /* @bin 0x00528F10 (page 11's festival box) */
void advisor_draw_festival_button(game_state *st); /* @bin 0x005292B0 */

/* c_festival.c */
void festival_dialog_draw(game_state *st);       /* @bin 0x00529320 window 0x2B */
void festival_dialog_draw_sizes(game_state *st); /* @bin 0x00529530 */
void festival_dialog_draw_building(game_state *st); /* @bin 0x00516080 window 0x52 */

#endif
