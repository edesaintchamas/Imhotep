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

#include "present_layout.h"
#include "state/state.h"

/* globals (docs/presentation.md 2.1, 3.1): named in present_layout.h */

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
