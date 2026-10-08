/* Imhotep - the advisors: group b's declarations (pages 5 and 7, dialog 0x4F; see advisors.h).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_PRESENT_UI_ADVISORS_B_H
#define IMHOTEP_PRESENT_UI_ADVISORS_B_H

#include <stdint.h>

#include "state/state.h"

/* b_ratings.c */
void advisor_ratings_draw(game_state *st);                                          /* @bin 0x004F6AF0 */
void ratings_frame(game_state *st, int32_t k);                                      /* @bin 0x004F7720 */
void ratings_column(game_state *st, int32_t x, int32_t y, int32_t value, int32_t top); /* @bin 0x004F77E0 */

/* b_dialog.c */
void advisor_explain_draw(game_state *st); /* @bin 0x00538050 window 0x4F */
/* b_population.c */
void advisor_population_draw(game_state *st);                                               /* @bin 0x00525CB0 */
void population_graph_history(game_state *st, int32_t big, int32_t x, int32_t y);          /* @bin 0x00527360 */
void population_graph_census(game_state *st, int32_t big, int32_t x, int32_t y);           /* @bin 0x00527710 */
void population_graph_society(game_state *st, int32_t big, int32_t x, int32_t y);          /* @bin 0x005278E0 */
void population_graph_scale(game_state *st, int32_t max);                                   /* @bin 0x00527AD0 */
int32_t population_history_at(game_state *st, int32_t months, int32_t i);                   /* @bin 0x004E9760 */
int32_t population_history_dates(game_state *st, int32_t months);                           /* @bin 0x004E97C0 */

#endif
