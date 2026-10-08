/* Imhotep - the advisors: group f's declarations (pages 12 and 13, see advisors.h).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_PRESENT_UI_ADVISORS_F_H
#define IMHOTEP_PRESENT_UI_ADVISORS_F_H

#include <stdint.h>

#include "state/state.h"

/* f_pages.c */
void advisors_draw_page12(game_state *st);                          /* @bin 0x0052A250 */
void advisors_draw_page13(game_state *st);                          /* @bin 0x0052C570 */
/* @bin 0x00536BA0 a horizontal strip of n 16-px pieces at (x, y) (images [0x00F56844] + 0x2A left,
 * 0x2B..0x2F middle, cycling, 0x30 right) */
void advisors_draw_strip(game_state *st, int32_t x, int32_t y, int32_t n);

#endif
