/* Imhotep - the city message list.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/message.h"

#include "state/ported.h"

#define MESSAGES 0x00E95228u     /* 1000 records of 0x30 bytes */
#define MESSAGE_SIZE 0x30u
#define MESSAGE_COUNT 1000
#define MESSAGE_TEMP 0x00E951F8u /* swap buffer (0x30 bytes, part of the state) */
#define MESSAGE_CURRENT 0x00E951E4u
#define MESSAGE_TOTAL 0x00E951ECu
#define MESSAGE_SCROLL 0x00E9511Cu
#define MESSAGE_SCROLL2 0x00E95120u

void game_copy(game_state *st, uint32_t src, uint32_t dst, int32_t n)
{
    /* @bin 0x0040F820 */
    for (; n > 7; n -= 8) {
        U32(st, dst) = U32(st, src);
        U32(st, dst + 4) = U32(st, src + 4);
        src += 8;
        dst += 8;
    }
    for (; n > 0; n--) {
        U8(st, dst++) = U8(st, src++);
    }
}

void message_sort(game_state *st)
{
    /* @bin 0x004E1640 the current message's id (+0x0C), or -1 */
    int32_t current_id = -1;
    if (S32(st, MESSAGE_CURRENT) >= 0) {
        current_id = S16(st, MESSAGES + MESSAGE_SIZE * U32(st, MESSAGE_CURRENT) + 0x0C);
    }
    /* @bin 0x004E1670 1000 passes over the pairs (k, k + 1), k < 999: a used message (+0x0A != 0)
     * moves before an unused one, and before a used one with a lower +0x0C (signed) */
    for (int pass = 0; pass < 1000; pass++) {
        for (uint32_t k = 0; k < MESSAGE_COUNT - 1; k++) {
            uint32_t a = MESSAGES + MESSAGE_SIZE * k, b = a + MESSAGE_SIZE;
            int swap = S16(st, a + 0x0A) == 0 ? S16(st, b + 0x0A) != 0
                                              : S16(st, a + 0x0C) < S16(st, b + 0x0C) && S16(st, b + 0x0A) != 0;
            if (swap) {
                game_copy(st, a, MESSAGE_TEMP, 0x30);
                game_copy(st, b, a, 0x30);
                game_copy(st, MESSAGE_TEMP, b, 0x30);
            }
        }
    }
    /* @bin 0x004E16D7 */
    int32_t used = 0;
    S32(st, MESSAGE_TOTAL) = 0;
    S32(st, MESSAGE_SCROLL) = 0;
    for (uint32_t k = 0; k < MESSAGE_COUNT; k++) {
        if (S16(st, MESSAGES + MESSAGE_SIZE * k + 0x0A) != 0) {
            used++;
        }
    }
    S32(st, MESSAGE_TOTAL) = used;
    if (used > 10) {
        S32(st, MESSAGE_SCROLL) = used - 10;
    } else {
        S32(st, MESSAGE_SCROLL2) = 0;
    }
    /* @bin 0x004E171B the current message follows its id; unchanged when no longer found */
    if (current_id >= 0) {
        for (uint32_t k = 0; k < MESSAGE_COUNT; k++) {
            if (S16(st, MESSAGES + MESSAGE_SIZE * k + 0x0C) == current_id) {
                S32(st, MESSAGE_CURRENT) = (int32_t) k;
                return;
            }
        }
    }
}

static int32_t run_message_sort(game_state *st, const int32_t *a)
{
    (void) a;
    message_sort(st);
    return 0;
}

static int32_t run_game_copy(game_state *st, const int32_t *a)
{
    if (!state_ptr(st, (uint32_t) a[0], 1) || !state_ptr(st, (uint32_t) a[1], 1)) {
        state_unported(st, 0x0040F820u); /* a buffer outside the state: the caller's stack */
        return 0;
    }
    game_copy(st, (uint32_t) a[0], (uint32_t) a[1], a[2]);
    return 0;
}

const ported_function message_ports[] = {
    {0x004E1640u, "message_sort", 0, 0, 0, run_message_sort, 0},
    {0x0040F820u, "game_copy", 3, 0, 0, run_game_copy, 0},
};
const int message_ports_count = sizeof(message_ports) / sizeof(message_ports[0]);
