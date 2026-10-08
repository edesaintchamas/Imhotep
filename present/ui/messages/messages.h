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

#include "state/state.h"

/* @bin the message list window's layout, set by 0x004E1870 */
#define MSGLIST_Y 0x00E9510Cu        /* i32 outer panel y */
#define MSGLIST_X 0x00E95110u        /* i32 outer panel x */
#define MSGLIST_W 0x00E95108u        /* i32 outer panel width in 16-pixel pieces */
#define MSGLIST_H 0x00E95104u        /* i32 outer panel height in pieces */
#define MSGLIST_IN_X 0x00E95100u     /* i32 inner panel x */
#define MSGLIST_IN_Y 0x00E950FCu     /* i32 inner panel y */
#define MSGLIST_IN_W 0x00E950F8u     /* i32 inner panel width in pieces */
#define MSGLIST_IN_H 0x00E950F4u     /* i32 inner panel height in pieces */
#define MSGLIST_THUMB_Y 0x00E95114u  /* i32 the scroll thumb's offset while dragged */
#define MSGLIST_DRAGGING 0x00E95118u /* i32 1 while the thumb is dragged */
#define MSGLIST_ROWS 0x00E9511Cu     /* i32 scrollable rows (0: no scroll bar) */
#define MSGLIST_SCROLL 0x00E95120u   /* i32 first message shown */
#define MSG_COUNT 0x00E951ECu        /* i32 messages in the list */
#define MSG_CURRENT 0x00E951E4u      /* i32 the message the keyword handlers read */
#define MSG_TYPE_CUR 0x00E951DCu     /* i32 the type of the row being drawn */
#define MESSAGES 0x00E95228u         /* message records, 0x30 bytes */
#define MESSAGE_TYPES 0x00E20970u    /* message type records, 0x50 bytes */
#define TEXT_BASE 0x00E36C74u        /* u32 base of the message texts (type +0x8C: offset) */
#define HOVER_ROW 0x007056C8u        /* i32 hovered row / button (1-based, 0 none) */
#define HOVER_DELETE 0x007056C4u     /* i32 the "delete read" button hovered */
#define MOUSE_X 0x00E38E34u
#define MOUSE_Y 0x00E38E30u
#define LEFT_HELD 0x00E38E00u
#define RIGHT_RELEASED 0x00E38DF8u

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
