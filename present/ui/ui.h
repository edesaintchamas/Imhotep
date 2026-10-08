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

#include "state/state.h"

/* globals (docs/presentation.md 3.2, 5.2) */
#define UI_WINDOW 0x00E38DDAu          /* u8 the current window */
#define UI_REDRAW 0x00E38DCAu          /* u8 full redraw of the window this frame */
#define UI_REDRAW_BUTTONS 0x00E38DC9u  /* u8 the windows' buttons are drawn this frame */
#define UI_DX 0x00E38DA8u              /* i32 (screen width - 640) / 2 */
#define UI_DY 0x00E38DA4u              /* i32 (screen height - 480) / 2 */
#define UI_SCREEN_W 0x00E38DB0u        /* i32 */
#define UI_SCREEN_H 0x00E38DACu        /* i32 */
#define UI_MOUSE_X 0x00E38E34u         /* i32 */
#define UI_MOUSE_Y 0x00E38E30u         /* i32 */
#define UI_LEFT_PRESSED 0x00E38E03u    /* u8 */
#define UI_LEFT_HELD 0x00E38E00u       /* u8 */
#define UI_LEFT_RELEASED 0x00E38E04u   /* u8 */
#define UI_RIGHT_PRESSED 0x00E38DF7u   /* u8 */
#define UI_RIGHT_RELEASED 0x00E38DF8u  /* u8 */
#define UI_HOVER_RECT 0x007056C8u      /* i32 the highlighted rectangle of a window's table (1-based) */
#define UI_HOVER_RECT2 0x007056C4u     /* i32 a second one */
#define UI_HOVER_RECT3 0x007056C0u     /* i32 a third one */
#define UI_EDITOR 0x00605A3Cu          /* i32 1 in the scenario editor */
#define UI_ADVISOR_X 0x00F53F0Cu       /* i32 the advisor windows' origin */
#define UI_ADVISOR_Y 0x00F53F08u
#define UI_SIDEBAR_X 0x00F53F64u       /* i32 */
#define UI_SIDEBAR_X_COLLAPSED 0x00F53F60u
#define UI_SIDEBAR_COLLAPSED 0x00F53F78u

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
