/* Imhotep - the message pop-up in the city view (roadmap phase 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The presentation side of the message posting (core's message_post 0x004E0CF0 queues the new
 * messages' ids at 0x00E95080): in window 1, every few frames, the first queued message is opened
 * in the message window (0x004C5BF0), marked read, with its sound or its video. */
#include "messages.h"

#include "command/orders.h"
#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"


void message_popup_clock(game_state *st)
{
    /* @bin 0x004CA980 0x00D3A6C8 := the frame clock 0x00E92E0C, 0x00D3A6E4 := 0 */
    S32(st, G_TIP_ID) = 0;
    S32(st, G_TIP_SINCE) = S32(st, G_FRAME_CLOCK);
}

void message_popup(game_state *st)
{
    /* @bin 0x004E1320 */
    if (U8(st, G_WINDOW) != 1) {
        return;
    }
    int32_t d = S32(st, G_POPUP_DELAY);
    if (d > 0) {
        S32(st, G_POPUP_DELAY) = d - 1;
        return;
    }
    uint32_t q = G_MESSAGE_PENDING;
    while (S32(st, q) == 0) {
        q += 4;
        if (q >= G_QUEUE_END) {
            return;
        }
    }
    int32_t id = S32(st, q);
    S32(st, q) = 0;
    if (id <= 0) {
        return;
    }
    /* the message with that id (+0x0C), among those in use (+0x0A) */
    int32_t i = 0;
    uint32_t m = MESSAGE_BASE;
    for (;; m += 0x30, i++) {
        if (m + 0x0C >= MESSAGE_ADDR(MESSAGE_COUNT) - 0x24u) {
            return;
        }
        if (S16(st, m + 0x0A) == 0) {
            return;
        }
        if (S16(st, m + 0x0C) == id) {
            break;
        }
    }
    if (i <= -1) {
        return;
    }
    S32(st, G_POPUP_DELAY) = 5;
    int32_t type = S16(st, m + 0x0A);
    GAME_CALL(st, FN_MESSAGE_KIND, type);
    U8(st, m + 0x0E) = 1;
    S32(st, G_MESSAGE_WINDOW_YEAR) = S16(st, m + 0x08);
    U8(st, G_FAST_FORWARD) = 0;
    S32(st, G_MESSAGE_CURRENT) = i;
    S32(st, G_MESSAGE_WINDOW_MONTH) = U8(st, m + 0x0F);
    S32(st, G_MESSAGE_WINDOW_PARAM_A) = S32(st, m);
    S32(st, G_MESSAGE_WINDOW_PARAM_B) = S32(st, m + 4);
    S32(st, G_MESSAGE_SHOWN_ID) = type;
    message_popup_clock(st);
    /* @bin 0x004E142A a video message (its name opens, 0x004CE3F0) plays its video, else sound 7
     * (type +0x80 = 1) or 6 */
    uint32_t t = MESSAGE_TYPES + 0x50u * (uint32_t) type;
    if (GAME_CALL(st, FN_FILE_EXISTS, (int32_t) (U32(st, t + 0x84) + U32(st, G_MM_TEXT))) == 0) {
        GAME_CALL(st, FN_RETURN_ZERO, S32(st, t + 0x80) == 1 ? 7 : 6);
    } else {
        S32(st, G_VIDEO_STATE) = 1;
    }
    S32(st, G_MESSAGE_POPUP) = 1;
    uint8_t expanded = S16(st, m + 0x10) >= 0;
    message_window_open(st, S32(st, G_MESSAGE_SHOWN_ID), 0, expanded);
    S32(st, G_MESSAGE_POPUP) = 0;
    U8(st, G_REDRAW_SIDEBAR) = 1;
}

static int32_t run_popup(game_state *st, const int32_t *a) { (void) a; message_popup(st); return 0; }
static int32_t run_clock(game_state *st, const int32_t *a) { (void) a; message_popup_clock(st); return 0; }

const ported_function msgpopup_ports[] = {
    {0x004E1320u, "message_popup", 0, 0, 0, run_popup, 0},
    {0x004CA980u, "message_popup_clock", 0, 0, 0, run_clock, 0},
};
const int msgpopup_ports_count = sizeof(msgpopup_ports) / sizeof(msgpopup_ports[0]);
