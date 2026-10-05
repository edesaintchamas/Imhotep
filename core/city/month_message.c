/* Imhotep - posting a city message (0x004E0CF0).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The message list (0x00E95228, 1000 records of 0x30 bytes) is game state; the window and sound
 * functions the original calls (0x0053B090, 0x004CA980, 0x004CE3F0, 0x0047D350, 0x004C5BF0,
 * 0x004E2970, 0x0044C1A0) keep going through GAME_CALL. */
#include "city/month_message.h"

#include <string.h>

#include "sim/call.h"
#include "state/ported.h"

#define MESSAGES 0x00E95228u
#define MESSAGE_SIZE 0x30u
#define MESSAGE_TYPES 0x00E209B0u /* 0x50 bytes per message id */
#define WINDOW 0x00E38DDAu
#define POPUP 0x00E950DCu
#define SETTINGS 0x00E38EF0u

int32_t message_free_slot(game_state *st)
{
    /* @bin 0x004E1750 the first record whose +0x0A (i16) is 0 */
    for (int32_t k = 0; k < 1000; k++) {
        if (S16(st, MESSAGES + MESSAGE_SIZE * (uint32_t) k + 0x0A) == 0) {
            return k;
        }
    }
    return -1;
}

void message_kind(game_state *st, int32_t id)
{
    /* @bin 0x004E14B0 (its switch table 0x004E1528) */
    int32_t v;
    switch (id) {
    case 0x71: case 0x76: case 0x77: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:
    case 0x80: case 0x81:
        v = 5;
        break;
    case 0x82: case 0x135: case 0x13A: v = 4; break;
    case 0x84: v = 8; break;
    case 0x87: case 0x88: v = 1; break;
    case 0x12D: case 0x12E: case 0x12F: v = 6; break;
    default: v = 0; break;
    }
    S32(st, 0x00D3A6E8u) = v;
}

static int32_t notice(game_state *st, int32_t n)
{
    GAME_CALL(st, 0x004E2970u, n, 0, 0);
    return 0;
}

int32_t message_allowed(game_state *st, int32_t id, const uint8_t *block)
{
    /* @bin 0x004E0F90 each filtered message id has its bit in 0x00E38EF0 and its notice */
    uint32_t f = U32(st, SETTINGS);
    switch (id) {
    case 0x84: return 1;
    case 0x15D: return (f & 2) ? notice(st, 0xDB) : 1;
    case 0x15C: return (f & 2) ? notice(st, 0xDC) : 1;
    case 0x15B: return (f & 2) ? notice(st, 0xDD) : 1;
    case 0x156: return notice(st, 0xDE);
    case 0x15A: return (f & 2) ? notice(st, 0xDE) : 1;
    case 0x157: return notice(st, 0xDF);
    case 0x159: return (f & 2) ? notice(st, 0xDF) : 1;
    case 0x158: return (f & 2) ? notice(st, 0xE0) : 1;
    case 0x115: return (f & 0x80) ? notice(st, 0xE1) : 1;
    case 0x116: return (f & 0x80) ? notice(st, 0xE2) : 1;
    case 0x10C: return (f & 0x200) ? notice(st, 0xE3) : 1;
    case 0x10B: return (f & 0x200) ? notice(st, 0xE4) : 1;
    case 0x112: return (f & 0x100) ? notice(st, 0xE5) : 1;
    case 0x111: return (f & 0x100) ? notice(st, 0xE6) : 1;
    case 0x83: {
        int16_t w;
        if (!block) {
            state_unported(st, 0x004E0F90u); /* the original reads through a null pointer */
            return 1;
        }
        memcpy(&w, block, 2); /* the original reads its block unconditionally */
        return (w == 0xC0 && (f & 0x10)) ? notice(st, 0xE7) : 1;
    }
    default: break;
    }
    if (id >= 0x64 && id <= 0x6D && (f & 4)) {
        return notice(st, 0xE8);
    }
    if (((id >= 0x141 && id <= 0x145) || id == 0x14B) && (f & 0x40)) {
        return notice(st, 0xE9);
    }
    if (id >= 0x89 && id <= 0x8B && (f & 0x20)) {
        return notice(st, 0xEA);
    }
    if (id == 0x135 || (id == 0x13A && (f & 8))) {
        GAME_CALL(st, 0x004E2970u, 0xEB, 0, 0);
        GAME_CALL(st, 0x0044C1A0u, S32(st, 0x006057E4u));
        return 0;
    }
    if (id == 0x13A) {
        return 1;
    }
    if (id == 0x146) {
        return (f & 0x400) ? notice(st, 0xF6) : 1;
    }
    if (id == 0x148) {
        return (f & 0x800) ? notice(st, 0xF7) : 1;
    }
    if (id == 0x88 && (f & 0x1000)) {
        return notice(st, 0x10F);
    }
    return 1;
}

void message_post(game_state *st, int32_t id, int32_t a, int32_t b, const uint8_t *block, uint32_t arg5)
{
    /* @bin 0x004E0CF0 */
    if ((uint8_t) arg5 != 0) {
        S32(st, POPUP) = 1;
    } else {
        S32(st, POPUP) = message_allowed(st, id, block);
    }
    uint8_t w = U8(st, WINDOW);
    if (w == 0x1E || w == 0x49 || w == 0x1F || w == 0x20) {
        U8(st, WINDOW) = 1;
    }
    int32_t k = message_free_slot(st);
    if (k == -1) {
        return;
    }
    GAME_CALL(st, 0x0053B090u, 9);
    if (S32(st, POPUP) == 0) {
        U8(st, 0x005C755Fu) = 1;
    }
    /* @bin 0x004E0D67 the record: +0x0A id, +0x0E shown, +0x08 year, +0x0F month, +0, +4 the
     * parameters, +0x0C its sequence number (0x00E951F0), +0x10..+0x2D the block (or -1, -1, -1) */
    uint32_t m = MESSAGES + MESSAGE_SIZE * (uint32_t) k;
    S16(st, m + 0x0A) = (int16_t) id;
    U8(st, m + 0x0E) = 0;
    S16(st, m + 0x08) = (int16_t) S32(st, 0x0078D7BCu);
    U8(st, m + 0x0F) = (uint8_t) S32(st, 0x0078D7C0u);
    S32(st, m) = a;
    S32(st, m + 4) = b;
    S32(st, 0x00E951ECu) += 1;
    S16(st, m + 0x0C) = (int16_t) S32(st, 0x00E951F0u);
    S32(st, 0x00E951E8u) = k;
    S32(st, 0x00E951F0u) += 1;
    uint32_t flag;
    if (block == NULL) {
        S16(st, m + 0x10) = -1;
        S16(st, m + 0x12) = -1;
        flag = 0;
        S16(st, m + 0x14) = -1;
    } else {
        for (uint32_t i = 0; i < 30; i++) {
            U8(st, m + 0x10 + i) = block[i];
        }
        int16_t first;
        memcpy(&first, block, 2);
        flag = first >= 0;
    }
    arg5 = (arg5 & 0xFFFFFF00u) | flag; /* the original reuses its argument's byte */
    if (U8(st, WINDOW) == 1) {
        message_kind(st, id);
    }
    uint32_t type = MESSAGE_TYPES + 0x50u * (uint32_t) id;
    if (S16(st, type + 0x0A) == 1 || S16(st, type + 0x0A) == 7) {
        S32(st, 0x00E951D8u) = 1;
        U8(st, 0x00E38DCAu) = 1;
    }
    k = S32(st, 0x00E951E8u);
    if (S32(st, POPUP) != 0) {
        if (U8(st, WINDOW) == 1) {
            /* @bin 0x004E0E6A the message window opens on it */
            m = MESSAGES + MESSAGE_SIZE * (uint32_t) k;
            S32(st, 0x00E951E4u) = k;
            S32(st, 0x00EA0DA8u) = 5;
            U8(st, m + 0x0E) = 1;
            S32(st, 0x00D3B178u) = S16(st, m + 0x08);
            S32(st, 0x00D3B174u) = U8(st, m + 0x0F);
            S32(st, 0x00D3B170u) = S32(st, m);
            U8(st, 0x00E3908Cu) = 0;
            S32(st, 0x00D3B16Cu) = S32(st, m + 4);
            S32(st, 0x00E951DCu) = id;
            GAME_CALL0(st, 0x004CA980u);
            if (GAME_CALL(st, 0x004CE3F0u, (int32_t) (U32(st, type + 0x44) + U32(st, 0x00E36C74u))) == 0) {
                GAME_CALL(st, 0x0047D350u, S32(st, type + 0x40) == 1 ? 7 : 6);
            } else {
                S32(st, 0x00F54118u) = 1;
            }
            GAME_CALL(st, 0x004C5BF0u, S32(st, 0x00E951DCu), 0, (int32_t) arg5);
        } else {
            /* @bin 0x004E0F25 the first free entry of the 20 pending ones */
            for (uint32_t i = 0; i < 20; i++) {
                if (S32(st, 0x00E95080u + 4 * i) == 0) {
                    S32(st, 0x00E95080u + 4 * i) = S16(st, MESSAGES + MESSAGE_SIZE * (uint32_t) k + 0x0C);
                    break;
                }
            }
        }
    } else if (S32(st, 0x00E950E0u) == 1) {
        GAME_CALL(st, 0x0047D350u, S32(st, type + 0x40) == 1 ? 7 : 6);
    }
    S32(st, 0x00E950E0u) = 1;
    S32(st, POPUP) = 0;
    U8(st, 0x00E38DC9u) = 1;
}

static const uint8_t *block_arg(game_state *st, int32_t p, uint32_t addr, int *bad)
{
    if (p == 0) {
        return NULL;
    }
    const uint8_t *b = state_ptr(st, (uint32_t) p, 30);
    if (!b) {
        state_unported(st, addr); /* a block in the caller's stack */
        *bad = 1;
    }
    return b;
}

static int32_t run_post(game_state *st, const int32_t *a)
{
    int bad = 0;
    const uint8_t *b = block_arg(st, a[3], 0x004E0CF0u, &bad);
    if (!bad) {
        message_post(st, a[0], a[1], a[2], b, (uint32_t) a[4]);
    }
    return 0;
}

static int32_t run_allowed(game_state *st, const int32_t *a)
{
    int bad = 0;
    static const uint8_t none[30] = {0};
    const uint8_t *b = a[0] == 0x83 ? block_arg(st, a[1], 0x004E0F90u, &bad) : none;
    if (bad || !b) {
        if (!bad) {
            state_unported(st, 0x004E0F90u);
        }
        return 0;
    }
    return message_allowed(st, a[0], b);
}

static int32_t run_free(game_state *st, const int32_t *a)
{
    (void) a;
    return message_free_slot(st);
}

static int32_t run_kind(game_state *st, const int32_t *a)
{
    message_kind(st, a[0]);
    return 0;
}

const ported_function month_message_ports[] = {
    {0x004E0CF0u, "message_post", 5, 0, 0, run_post, 0},
    {0x004E0F90u, "message_allowed", 2, 32, 0, run_allowed, 0},
    {0x004E1750u, "message_free_slot", 0, 32, 0, run_free, 0},
    {0x004E14B0u, "message_kind", 1, 0, 0, run_kind, 0},
};
const int month_message_ports_count = sizeof(month_message_ports) / sizeof(month_message_ports[0]);
