/* Imhotep - the message list and the replies to scenario-event messages.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * docs/commands.md section 3.8. Messages: 1000 records of 0x30 bytes at 0x00E95228. */
#include "command/orders.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define MESSAGES 0x00E95228u
#define MESSAGE_SIZE 0x30u
#define MESSAGE_COUNT 1000u
#define MESSAGE_SCROLL 0x00E95120u   /* i32 first row shown */
#define MESSAGE_ROWS 0x00E9511Cu     /* i32 rows the window shows (set by its drawing) */
#define MESSAGE_COUNT_NOW 0x00E951ECu /* i32 messages in the list */
#define MESSAGE_CURRENT 0x00E951E4u
#define MESSAGE_CLICKED 0x00E951E8u
#define MESSAGE_TYPE_CUR 0x00E951DCu
#define MESSAGE_TYPES 0x00E20970u    /* message type records, 0x50 bytes (+0x4A kind) */
#define EVENTS 0x00785D70u           /* i32 count, then event records of 0x7C bytes */
#define REPLIES_ALLOWED 0x00D3B186u  /* u8 bits 1, 2, 4: replies offered by the message window */

void cmd_message_delete(game_state *st)
{
    /* @bin 0x004E2300 right click on row p1: type +0x0A := 0, list sorted again */
    int32_t i = S32(st, MESSAGE_SCROLL) + S32(st, CMD_PARAM1);
    S32(st, MESSAGE_CLICKED) = i;
    if (i >= S32(st, MESSAGE_COUNT_NOW)) {
        return;
    }
    S16(st, MESSAGES + MESSAGE_SIZE * (uint32_t) i + 0x0A) = 0;
    GAME_CALL0(st, 0x004E1640u);
    int32_t s = S32(st, MESSAGE_SCROLL);
    if (s >= S32(st, MESSAGE_ROWS)) {
        s--;
        S32(st, MESSAGE_SCROLL) = s;
    }
    if (s < 0) {
        S32(st, MESSAGE_SCROLL) = 0;
    }
    U8(st, UI_REDRAW) = 1;
}

void cmd_messages_delete_read(game_state *st)
{
    /* @bin 0x004E2290 every read message (+0x0E) whose type's kind (+0x4A) is not 4 is deleted */
    for (uint32_t k = 0; k < MESSAGE_COUNT; k++) {
        uint32_t m = MESSAGES + MESSAGE_SIZE * k;
        int16_t t = S16(st, m + 0x0A);
        if (t != 0 && U8(st, m + 0x0E) != 0 && S16(st, MESSAGE_TYPES + 0x50u * (uint32_t) (int32_t) t + 0x4A) != 4) {
            S16(st, m + 0x0A) = 0;
        }
    }
    GAME_CALL0(st, 0x004E1640u);
    int32_t s = S32(st, MESSAGE_SCROLL);
    int32_t n = S32(st, MESSAGE_ROWS);
    if (s >= n) {
        s = n - 1;
        S32(st, MESSAGE_SCROLL) = s;
    }
    if (s < 0) {
        S32(st, MESSAGE_SCROLL) = 0;
    }
    U8(st, UI_REDRAW) = 1;
}

void cmd_message_open(game_state *st)
{
    /* @bin 0x004E21D0 left click on row p1: read flag +0x0E := 1, the message's fields to the
     * window's globals, then the window opens (0x004C5BF0) */
    int32_t i = S32(st, MESSAGE_SCROLL) + S32(st, CMD_PARAM1);
    S32(st, MESSAGE_CLICKED) = i;
    S32(st, MESSAGE_CURRENT) = i;
    if (i >= S32(st, MESSAGE_COUNT_NOW)) {
        return;
    }
    uint32_t m = MESSAGES + MESSAGE_SIZE * (uint32_t) i;
    U8(st, m + 0x0E) = 1;
    S32(st, 0x00D3B178u) = S16(st, m + 0x08);
    S32(st, 0x00D3B174u) = U8(st, m + 0x0F);
    U32(st, 0x00D3B170u) = U32(st, m + 0x00);
    U32(st, 0x00D3B16Cu) = U32(st, m + 0x04);
    int32_t type = S16(st, m + 0x0A);
    S32(st, MESSAGE_TYPE_CUR) = type;
    GAME_CALL(st, 0x004E14B0u, type);
    /* @bin 0x004E2250 third argument: a byte, 1 when +0x10 >= 0 (the upper bytes of the pushed
     * dword are a stack slot's old contents, unused by 0x004C5BF0) */
    uint8_t reply = S16(st, MESSAGES + MESSAGE_SIZE * U32(st, MESSAGE_CLICKED) + 0x10) >= 0;
    message_window_open(st, S32(st, MESSAGE_TYPE_CUR), 0, reply);
}

static void message_window_lines_clear(game_state *st)
{
    /* @bin 0x004C8040 0x00D3A6F8 .. 0x00D3AADF := 0 (50 x 20 bytes), then 0x00D3A6F0 and 0x00D3A6EC */
    for (uint32_t a = 0x00D3A6F8u; a < 0x00D3AAE0u; a += 4) {
        U32(st, a) = 0;
    }
    U32(st, 0x00D3A6F0u) = 0;
    U32(st, 0x00D3A6ECu) = 0;
}

void message_window_open(game_state *st, int32_t type, int32_t param, uint8_t replies)
{
    /* @bin 0x004C5BF0 (UI state only) the message window (5) for a message type: the replies offered
     * (u16 0x00D3B186 := the current message's +0x26 when `replies`, else 0), cleared line buffers,
     * 0x00D3B180 := the type's +0x84 != 0 when 0x00E950DC = 1 (video), the previous window kept in
     * 0x00D3B17C */
    U8(st, 0x00D3B184u) = replies;
    if (replies != 0) {
        S16(st, 0x00D3B186u) = S16(st, MESSAGES + MESSAGE_SIZE * U32(st, MESSAGE_CURRENT) + 0x26);
    } else {
        S16(st, 0x00D3B186u) = 0;
    }
    for (uint32_t i = 0; i < 200; i++) {
        U32(st, 0x00D3AE04u + 4 * i) = 0;
    }
    for (uint32_t i = 0; i < 200; i++) {
        U32(st, 0x00D3AAE4u + 4 * i) = 0;
    }
    S32(st, 0x00D3B124u) = type;
    S32(st, 0x00D3B140u) = param;
    U32(st, 0x00D3AAE0u) = 0;
    U32(st, 0x00D3B130u) = 0;
    int video = S32(st, MESSAGE_TYPES + 0x50u * (uint32_t) type + 0x84) != 0;
    U32(st, 0x00D3B138u) = 0;
    U32(st, 0x00D3B12Cu) = 0;
    U32(st, 0x00D3B180u) = (uint32_t) video;
    if (S32(st, 0x00E950DCu) != 1) {
        U32(st, 0x00D3B180u) = 0;
    }
    message_window_lines_clear(st);
    U8(st, UI_REDRAW) = 1;
    uint8_t w = U8(st, UI_WINDOW);
    U8(st, UI_WINDOW) = 5;
    U32(st, 0x00D3B17Cu) = w;
}

uint32_t event_of_message(game_state *st, uint32_t message)
{
    /* @bin 0x0044EDD0 (thiscall on 0x00785D70) the event of kind 0x17 (+0x02), active (+0x32),
     * with +0x0C and +0x04 equal to the message's +0x08 and +0x0A (message = the record + 0x10);
     * its +0x00 := its index */
    int32_t n = S32(st, EVENTS);
    for (int32_t i = 0; i < n; i++) {
        uint32_t r = EVENTS + 4 + 0x7Cu * (uint32_t) i;
        if (U8(st, r + 0x02) == 0x17 && U8(st, r + 0x32) != 0 && S16(st, message + 8) == S16(st, r + 0x0C) &&
            S16(st, message + 0x0A) == S16(st, r + 0x04)) {
            S16(st, r) = (int16_t) i;
            return r;
        }
    }
    return 0;
}

void message_window_close(game_state *st)
{
    /* @bin 0x004C7F30 back to the window saved at 0x00D3B17C */
    if (S32(st, 0x00D3B180u) != 0) {
        GAME_CALL0(st, 0x00539170u);
    }
    uint8_t w = U8(st, 0x00D3B17Cu);
    S32(st, 0x00D3B180u) = 0;
    U8(st, UI_WINDOW) = w;
    S32(st, 0x00D3A6E8u) = 0;
    U8(st, UI_REDRAW) = 1;
}

/* @bin 0x0044EEA9 the current message's +0x26 := 0, and its event */
static uint32_t reply_event(game_state *st)
{
    uint32_t m = MESSAGES + MESSAGE_SIZE * U32(st, MESSAGE_CURRENT);
    S16(st, m + 0x26) = 0;
    return event_of_message(st, m + 0x10);
}

void cmd_event_reply_1(game_state *st)
{
    /* @bin 0x0044EEA0 reply bit 1: event status +0x30 := 7 */
    if (!(U8(st, REPLIES_ALLOWED) & 1)) {
        return;
    }
    uint32_t e = reply_event(st);
    if (e == 0) {
        return;
    }
    S16(st, e + 0x30) = 7;
    GAME_CALLT0(st, e, 0x0044E890u);
    message_window_close(st);
}

void cmd_event_reply_2(game_state *st)
{
    /* @bin 0x0044EEF0 reply bit 2: status 1 -> 2 and 3 -> 4 (with +0x2E := 1), else 8 */
    if (!(U8(st, REPLIES_ALLOWED) & 2)) {
        return;
    }
    uint32_t e = reply_event(st);
    if (e == 0) {
        return;
    }
    int16_t s = S16(st, e + 0x30);
    if (s == 1 || s == 3) {
        S16(st, e + 0x30) = s == 1 ? 2 : 4;
        S16(st, e + 0x2E) = 1;
    } else {
        S16(st, e + 0x30) = 8;
        GAME_CALLT0(st, e, 0x0044E890u);
    }
    message_window_close(st);
}

void cmd_event_reply_4(game_state *st)
{
    /* @bin 0x0044EE30 reply bit 4: status 6; city +0x49A4 + the event's +0x04 := 1 */
    if (!(U8(st, REPLIES_ALLOWED) & 4)) {
        return;
    }
    uint32_t e = reply_event(st);
    if (e == 0) {
        return;
    }
    S16(st, e + 0x30) = 6;
    GAME_CALLT0(st, e, 0x0044E890u);
    message_window_close(st);
    U8(st, CURRENT_CITY(st) + 0x49A4 + (uint32_t) (int32_t) S16(st, e + 0x04)) = 1;
    GAME_CALL0(st, 0x005596D0u);
}

#define V(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
V(run_delete, cmd_message_delete)
V(run_delete_read, cmd_messages_delete_read)
V(run_open, cmd_message_open)
V(run_reply_1, cmd_event_reply_1)
V(run_reply_2, cmd_event_reply_2)
V(run_reply_4, cmd_event_reply_4)
V(run_close, message_window_close)
V(run_lines_clear, message_window_lines_clear)

static int32_t run_window_open(game_state *st, const int32_t *a)
{
    message_window_open(st, a[0], a[1], (uint8_t) a[2]);
    return 0;
}

static int32_t run_event_of_message(game_state *st, const int32_t *a)
{
    return (int32_t) event_of_message(st, (uint32_t) a[0]);
}

const ported_function orders_messages_ports[] = {
    {0x004E2300u, "cmd_message_delete", 0, 0, 0, run_delete, 0},
    {0x004E2290u, "cmd_messages_delete_read", 0, 0, 0, run_delete_read, 0},
    {0x004E21D0u, "cmd_message_open", 0, 0, 0, run_open, 0},
    {0x0044EEA0u, "cmd_event_reply_1", 0, 0, 0, run_reply_1, 0},
    {0x0044EEF0u, "cmd_event_reply_2", 0, 0, 0, run_reply_2, 0},
    {0x0044EE30u, "cmd_event_reply_4", 0, 0, 0, run_reply_4, 0},
    {0x0044EDD0u, "event_of_message", 1, 32, 0x00785D70u, run_event_of_message, 0},
    {0x004C7F30u, "message_window_close", 0, 0, 0, run_close, 0},
    {0x004C5BF0u, "message_window_open", 3, 0, 0, run_window_open, 0},
    {0x004C8040u, "message_window_lines_clear", 0, 0, 0, run_lines_clear, 0},
};
const int orders_messages_ports_count = sizeof(orders_messages_ports) / sizeof(orders_messages_ports[0]);
