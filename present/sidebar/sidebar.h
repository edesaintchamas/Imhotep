/* Imhotep - the city sidebar, the minimap, the top menu bar and the tooltips (roadmap phase 7.5).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Ports of Pharaoh.exe's (@bin addresses below and in each .c) sidebar drawing (0x0051E990 family), button tables (0x00425730), top
 * menu bar (0x0051F0A0, 0x0051F680), minimap (0x0043E810), tooltips (0x004C8750, 0x004C9750,
 * 0x004CABD0) and the sidebar's input dispatch. Image draws, blitters and text are package
 * "blit"'s, the city view "cityview"'s: called through GAME_CALL until they are ported. */
#ifndef IMHOTEP_PRESENT_SIDEBAR_H
#define IMHOTEP_PRESENT_SIDEBAR_H

#include <stdint.h>

#include "state/state.h"

/* globals (docs/presentation.md 2.1, 3.1) */
#define SCREEN_WIDTH 0x00E38DB0u       /* i32 */
#define SCREEN_HEIGHT 0x00E38DACu      /* i32 */
#define SCREEN_HEIGHT2 0x00E38D98u     /* i32, the same value (set together by 0x004CF960) */
#define LAYOUT_DX 0x00E38DA8u          /* i32 (width - 640) / 2 */
#define LAYOUT_DY 0x00E38DA4u          /* i32 (height - 480) / 2 */
#define SIDEBAR_X 0x00F53F64u          /* i32 x of the expanded sidebar (0x0051E930) */
#define SIDEBAR_X_COLLAPSED 0x00F53F60u /* i32 x of the collapsed sidebar */
#define SIDEBAR_COLLAPSED 0x00F53F78u  /* i32 0 expanded, 1 collapsed */
#define EDITOR_MODE 0x00605A3Cu        /* i32 1 in the scenario editor */
#define WINDOW_ID 0x00E38DDAu          /* u8 */
#define REDRAW_ALL 0x00E38DCAu         /* u8 full redraw of the window this frame */
#define REDRAW_SIDEBAR 0x00E38DC9u     /* u8 */
#define TEXT_X 0x00E20778u             /* i32 text cursor x after a draw (text: package blit) */
#define IMAGES 0x00F55BB0u             /* `this` of the image draw 0x0053E590 */
#define IMAGE_GROUPS 0x00F55BB4u       /* i32 first image of group g at +8 g */

void sidebar_set_x(game_state *st);
void sidebar_draw(game_state *st);
void sidebar_draw_buttons(game_state *st);
void sidebar_draw_overlay_name(game_state *st, int32_t force, int32_t x);
void sidebar_draw_build_title(game_state *st, int32_t force, int32_t x);
void sidebar_draw_minimap(game_state *st, int32_t force);
void sidebar_draw_city_texts(game_state *st);
void sidebar_draw_messages(game_state *st, int32_t force);
void sidebar_slide(game_state *st);
void topbar_draw(game_state *st, int32_t force);
void topbar_draw_background(game_state *st);
void buttons_draw(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count);
void menubar_draw(game_state *st, uint32_t table, int32_t count);
void menubar_draw_button(game_state *st);
void panel_draw(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h);
int32_t cityview_pixel_width(game_state *st);

/* tooltips (tooltip.c, tooltip_text.c, tooltip_draw.c) */
int32_t mouse_in(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h);
int32_t tooltip_update(game_state *st);
int32_t tooltip_end(game_state *st);
int32_t tooltip_family_hover(game_state *st);
int32_t tooltip_empire_city_goods(game_state *st, uint8_t *buys);
int32_t tooltip_trade_city_goods(game_state *st, uint8_t *buys);
int32_t tooltip_overlay_text(game_state *st);
void tooltip_draw(game_state *st);

/* minimap (minimap.c) and input (input.c) */
void minimap_first_tile(game_state *st, int32_t w, int32_t h);
void minimap_draw(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h);
int32_t buttons_click(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count);

#endif
