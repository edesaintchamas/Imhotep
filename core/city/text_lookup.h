/* Imhotep - lookups in the game's text (Pharaoh_Text.eng, mapped with state_map_external).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_TEXT_LOOKUP_H
#define IMHOTEP_CITY_TEXT_LOOKUP_H

#include <stdint.h>

#include "state/layout/g_ui.h"
#include "state/state.h"

/* kept for the ports outside core/city that use it (docs/naming.md); core/city uses G_TEXT_CURSOR */
#define TEXT_CURSOR G_TEXT_CURSOR /* address of the current string */

/* @bin 0x004CC110 TEXT_CURSOR := string `index` of text group `group` */
void text_seek(game_state *st, int32_t group, int32_t index);

/* @bin 0x00469D50 does the mission (its flag string, text group 0x133) still have goals left:
 * monuments of types 0xDE, 0xE5, 0xEA .. 0xEC not finished (+0x16 == -1), or a positive goal
 * counter among 0x00784E78 + 4 * (i + 1) for the listed i */
int32_t mission_goals_pending(game_state *st);

/* @bin 0x004CBF30 width in pixels of a string in a font (first glyph id): spaces 10 / 4 / 6 by
 * font, other characters their glyph's width (+1 in the three known fonts) */
int32_t text_width(game_state *st, uint32_t text, int32_t font);
/* @bin 0x004CC000 string length (at most 10,000) */
int32_t text_length(game_state *st, uint32_t text);
/* @bin 0x004CBEA0 copy n bytes forward, one at a time (src, dst, n) */
void text_copy(game_state *st, uint32_t src, uint32_t dst, int32_t n);

#endif
