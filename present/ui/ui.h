/* Imhotep - the window machinery and the small windows (roadmap phase 7.6, package "wincore").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The current window is u8 0x00E38DDA. Each window id has three entry points, reached through the
 * three dispatchers ported here (docs/presentation.md 3.2): the full redraw 0x004D71C0 (when the
 * redraw flag 0x00E38DCA is set), the per-frame draw 0x004D75D0 and the input 0x004D8310. The
 * windows' own functions are called through GAME_CALL when they belong to another package (info,
 * advisors, messages and empire, front end, editor). */
#ifndef IMHOTEP_PRESENT_UI_UI_H
#define IMHOTEP_PRESENT_UI_UI_H

#include <stdint.h>

#include "present_layout.h"
#include "state/state.h"

/* dispatchers (dispatch.c) */
void ui_redraw(game_state *st, int32_t force);       /* @bin 0x004D71C0 */
void ui_draw_frame(game_state *st);                  /* @bin 0x004D75D0 */
void ui_input(game_state *st);                       /* @bin 0x004D8310 */
void empire_editor_input(game_state *st, int32_t mouse_x, int32_t mouse_y); /* @bin 0x004D9CB5 (inside 0x004D8310) */

/* rectangle tables (entries of 0x1C, i16 x0, y0, x1, y1 at +0..+6) */
int32_t rect_hit(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count); /* 0x00426450 (msgempire) */
int32_t rect_highlight(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count,
                       int32_t hit);                                                           /* @bin 0x004D6740 */
void rect_layout_apply(game_state *st, uint32_t table, uint32_t list);                         /* @bin 0x004D66F0 */

#endif
