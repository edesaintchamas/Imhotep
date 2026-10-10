/* Imhotep - the message list window (0x11) and the message display window (roadmap phase 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Ports of Pharaoh.exe's message list (full redraw 0x004E1870, per frame 0x004E1A50, input
 * 0x004E1E10) and the message text expansion ("[keyword]" replaced by the text of a handler,
 * 0x004CB980). The commands the list's buttons reach (open, delete, delete read) are core's
 * (core/command/orders_messages.c). */
#ifndef IMHOTEP_PRESENT_UI_MESSAGES_H
#define IMHOTEP_PRESENT_UI_MESSAGES_H

#include <stdint.h>

#include "present_layout.h"
#include "state/state.h"

/* @bin 0x004E1870 the message type records (0x50 bytes) as these ports offset their fields: from
 * 0x40 below G_MESSAGE_TYPES (+0x4A is MSGTYPE_U0A) */
#define MESSAGE_TYPES (G_MESSAGE_TYPES - 0x40u)

void msglist_redraw(game_state *st);
void msglist_draw(game_state *st);
void msglist_input(game_state *st);
int32_t msglist_scroll_drag(game_state *st);
int32_t buttons_hovered(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count);

/* text expansion (expand.c) */
/* @bin 0x00448330 the expanded text of entry `index` of the message strings into out[0..size) */
void message_text_expand(game_state *st, int32_t index, char *out, int32_t size);
/* @bin 0x004CB980 text with "[keyword]" expanded into out[0..size): 1 when the end was copied */
int32_t text_expand(game_state *st, uint32_t self, uint32_t text, char *out, int32_t size);

/* rich text (richtext.c) */
/* @bin 0x004C8070 text laid out in lines of width (measure 1: lines counted in 0x00D3B138) */
void rich_text_draw(game_state *st, const uint8_t *text, int32_t x, int32_t y, int32_t width, int32_t measure,
                    int32_t raised);
/* @bin 0x004C8480 */
int32_t rich_word_width(game_state *st, const uint8_t *p);

/* the message window (window.c) */
void message_window_draw(game_state *st);       /* @bin 0x004C5E60 */
void message_window_draw_video(game_state *st); /* @bin 0x004C6C70 */
void message_reply_buttons(game_state *st);     /* @bin 0x004C5CE0 */
void message_scroll_thumb(game_state *st);      /* @bin 0x004C72B0 */
void panel_draw_pattern(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h); /* @bin 0x00536990 */

#endif
