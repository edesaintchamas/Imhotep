/* Imhotep - scenario event handlers of the month: requests (type 1), ... (package month2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The record layout: core/city/events.c. The handlers are reached from the month dispatcher
 * 0x0044C660 (core/city/month_events.c) through GAME_CALLT with the record as `this`. */
#include "city/events.h"

#include "city/goods.h"

#include "city/month_events.h"
#include "city/month_message.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


static void put16(uint8_t *b, uint32_t off, int32_t v)
{
    b[off] = (uint8_t) v;
    b[off + 1] = (uint8_t) ((uint32_t) v >> 8);
}

/* @bin 0x0044CE05 the request's text group by its subtype +0x60 (jump tables 0x0044D3D4..):
 * subtypes 0, 3..6 only (others leave the words unset), one higher when +0x56 is 0 */
static int request_base(game_state *st, uint32_t e, int32_t *base)
{
    static const int16_t b[7] = {0x24, -1, -1, 0x36, 0x48, 0x5A, 0x6C};
    uint8_t s = U8(st, e + 0x60);
    if (s > 6 || b[s] < 0) {
        return 0;
    }
    *base = b[s] + (U8(st, e + 0x56) == 0);
    return 1;
}

static int32_t request_message(game_state *st, uint32_t e)
{
    /* @bin 0x0044CDFC 0x82, subtypes 3, 4, 5: 0x113, 0x114, 0x14A */
    switch (U8(st, e + 0x60)) {
    case 3: return 0x113;
    case 4: return 0x114;
    case 5: return 0x14A;
    default: return 0x82;
    }
}

static int request_met(game_state *st, uint32_t e)
{
    /* @bin 0x0044D0C6 good 0x24: the treasury (city +0xC) >= +0xC; else the city's stock of the
     * good (city +0x48F0, i32 by good) >= +0xC * 100 */
    uint32_t c = CURRENT_CITY(st);
    int16_t g = S16(st, e + 4);
    if (g == 0x24) {
        return S32(st, c + CITY_TREASURY) >= S16(st, e + 0xC);
    }
    return S32(st, c + CITY_STORAGE_STOCK + 4u * (uint32_t) (int32_t) g) >= (int32_t) ((uint32_t) S16(st, e + 0xC) * 100u);
}

void event_request(game_state *st, uint32_t e)
{
    /* @bin 0x0044CDC0 (thiscall) type 1, a request of goods (+4, +0xC units; 0x25: 0x0044C9E0). +0x30
     * its stage (0 asked, 1 overdue, 2/3 ending, 4/8 over), +0x32 running, +0x2E months left, +0x33
     * met. Starts at its date with message `base + 2`; reminder at 6 months; at 0: overdue (24 more
     * months, message 0x82, 0x004E96C0(-3), city +0x4588 := 3); overdue at 0: the event +0x26 starts;
     * stages 2 / 3 at 0: the event +0x24 / +0x52 starts; the first month it is met: message 0x135
     * (money) or 0x13A */
    if (S16(st, e + 4) == 0x25) {
        GAME_CALLT0(st, e, FN_EVENT_TROOPS_REQUEST);
        return;
    }
    int16_t stage = S16(st, e + 0x30);
    if (stage == 8 || stage == 4) {
        return;
    }
    uint8_t blk[32] = {0};
    int32_t base = 0;
    int valid = request_base(st, e, &base);
    if (!valid) {
        /* subtypes 1, 2 (asked with good 0x25 only, 0x00449440) leave the block's words +0 / +2 as
         * the original's leftover stack bytes */
        state_unported(st, FN_EVENT_REQUEST);
        return;
    }
    int32_t id = request_message(st, e);
    if (valid) {
        put16(blk, 2, base);
    }
    put16(blk, 0x10, event_message_kind(st, e));
    event_fill_block(st, e, blk);
    stage = S16(st, e + 0x30);
    if (stage == 2 || stage == 3) {
        /* @bin 0x0044D357 */
        S16(st, e + 0x2E) = (int16_t) (S16(st, e + 0x2E) - 1);
        if (S16(st, e + 0x2E) > 0) {
            return;
        }
        S32(st, G_MESSAGE_POPUP) = 1;
        if (S16(st, e + 0x30) == 2) {
            if (event_link_active(st, e, S16(st, e + 0x24))) {
                int32_t v = event_outcome_message(st, e, 0);
                event_start(st, e, S16(st, e + 0x24), v);
            }
        } else if (event_link_active(st, e, S16(st, e + 0x52))) {
            int32_t v = event_outcome_message(st, e, 2);
            event_start(st, e, S16(st, e + 0x52), v);
        }
        S16(st, e + 0x30) = 8;
        U8(st, e + 0x32) = 0;
        U32(st, e + 0x28) |= 4;
        return;
    }
    if (U8(st, e + 0x32) == 0) {
        /* @bin 0x0044D1B0 the start, at its date */
        if (U8(st, e + 0x28) & 0x11) {
            return;
        }
        if (S32(st, G_YEAR) != S16(st, e + 0x14) + S16(st, G_START_YEAR) || S32(st, G_MONTH) != (int32_t) U8(st, e + 3)) {
            return;
        }
        U8(st, e + 0x32) = 1;
        if (request_met(st, e)) {
            U8(st, e + 0x33) = 1;
        }
        S16(st, e + 0x2E) = S16(st, e + 0x2C);
        S32(st, G_MESSAGE_POPUP) = 1;
        if (valid) {
            put16(blk, 0, base + 2);
        }
        message_post(st, id, S16(st, e), 0, blk, 0);
        /* @bin 0x0044D302 the tutorial step 8 */
        if (U8(st, G_MISSION_LIST_FLAGS + 8) != 0 || (int32_t) S8(st, G_MISSION) != S32(st, G_MISSION_LISTS + G_MISSION_LIST_STRIDE * 8) || U8(st, e + 2) != 1 ||
            S16(st, e + 4) != 0xD || S32(st, G_CUSTOM) != 0) {
            return;
        }
        GAME_CALL(st, FN_TUTORIAL_STEP, 8);
        return;
    }
    S16(st, e + 0x2E) = (int16_t) (S16(st, e + 0x2E) - 1);
    int16_t left = S16(st, e + 0x2E);
    if (stage == 0) {
        if (left == 6) {
            S32(st, G_MESSAGE_POPUP) = 1;
            put16(blk, 0xC, left);
            if (valid) {
                put16(blk, 0, base + 4);
            }
            message_post(st, id, S16(st, e), 0, blk, 0);
        } else if (left <= 0) {
            S32(st, G_MESSAGE_POPUP) = 1;
            if (valid) {
                put16(blk, 0, base + 6);
            }
            S16(st, e + 0x2E) = 0x18;
            put16(blk, 0xC, 0x18);
            S16(st, e + 0x30) = 1;
            message_post(st, 0x82, S16(st, e), 0, blk, 0);
            GAME_CALL(st, FN_CITY_ADD_0X42D8, -3);
            S32(st, CURRENT_CITY(st) + CITY_U4588) = 3;
        }
    } else if (stage == 1 && left <= 0) {
        if (event_link_active(st, e, S16(st, e + 0x26))) {
            int32_t v = event_outcome_message(st, e, 1);
            event_start(st, e, S16(st, e + 0x26), v);
        }
        S16(st, e + 0x30) = 4;
        U8(st, e + 0x32) = 0;
    }
    /* @bin 0x0044D0B6 the first month the request is met */
    if (U8(st, e + 0x33) != 0) {
        return;
    }
    int16_t g = S16(st, e + 4);
    uint32_t c = CURRENT_CITY(st);
    int32_t six = U8(st, e + 0x60) == 6;
    if (g == 0x24) {
        if (S32(st, c + CITY_TREASURY) < S16(st, e + 0xC)) {
            return;
        }
        U8(st, e + 0x33) = 1;
        S32(st, G_MESSAGE_POPUP) = 1;
        put16(blk, 0xC, S16(st, e + 0x2E));
        put16(blk, 4, -1);
        put16(blk, 2, -1);
        put16(blk, 0, -1);
        /* the fifth argument's upper bytes are a leftover register (ecx, not known after the calls
         * above); only its low byte is used by 0x004E0CF0 itself */
        message_post(st, 0x135, S16(st, e), 0, blk, (uint32_t) six);
        return;
    }
    uint32_t need = (uint32_t) S16(st, e + 0xC) * 100u;
    if (S32(st, c + CITY_STORAGE_STOCK + 4u * (uint32_t) (int32_t) g) < (int32_t) need) {
        return;
    }
    U8(st, e + 0x33) = 1;
    S32(st, G_MESSAGE_POPUP) = 1;
    put16(blk, 0xC, S16(st, e + 0x2E));
    S32(st, G_REQUEST_EVENT) = S16(st, e);
    put16(blk, 4, -1);
    put16(blk, 2, -1);
    put16(blk, 0, -1);
    /* @bin 0x0044D194 the fifth argument: edx, the amount * 100 with its low byte := subtype == 6 */
    message_post(st, 0x13A, S16(st, e), 0, blk, (need & ~0xFFu) | (uint32_t) six);
}

/* the empire: cities 0x0077DE00 (61 x 0x6A: +0 in use, +2 kind, +5 open, +6+good sells, +0x2A+good
 * buys, +0x56 i16 its object), objects 0x0077F754 (0x62 bytes: +0 i16 image, +0xC kind, +0xD,
 * +0x12 14 goods, +0x28 8 goods) */
#define EMPIRE_CITY(i) (G_EMPIRE_CITIES + 0x6Au * (uint32_t) (i))
#define EMPIRE_OBJ(i) (G_EMPIRE_OBJECTS + 0xCu + 0x62u * (uint32_t) (i))

int32_t empire_object_image(game_state *st, int32_t obj)
{
    /* @bin 0x004425C0 the image of an empire object by its kind +0xC (table 0x00442644) */
    uint32_t o = EMPIRE_OBJ(obj);
    switch (U8(st, o + 0xC)) {
    case 0:
        return S16(st, G_SCENARIO_RANK) == 10 ? S32(st, G_UF56954) : S32(st, G_UF56444);
    case 1: return S32(st, G_UF56954);
    case 2: return S32(st, G_UF5693C);
    case 3: return (int32_t) (U32(st, G_UF56444) + 6u);
    case 4: return (int32_t) (U32(st, G_UF56964) + 7u);
    case 5: return (int32_t) (U32(st, G_U5D2A38 + 8u * U8(st, o + 0xD)) + U32(st, G_UF56444));
    case 6: return (int32_t) (U32(st, G_U5D2A38 + 8u * U8(st, o + 0xD)) + U32(st, G_UF56964));
    default: return 0;
    }
}

int32_t empire_object_sells(game_state *st, int32_t obj, uint32_t g)
{
    /* @bin 0x00443000 one of the 14 bytes at +0x12 is g */
    for (uint32_t i = 0; i < 14; i++) {
        if (U8(st, EMPIRE_OBJ(obj) + 0x12 + i) == g) {
            return 1;
        }
    }
    return 0;
}

int32_t empire_object_buys(game_state *st, int32_t obj, uint32_t g)
{
    /* @bin 0x00443040 one of the 8 bytes at +0x28 is g */
    for (uint32_t i = 0; i < 8; i++) {
        if (U8(st, EMPIRE_OBJ(obj) + 0x28 + i) == g) {
            return 1;
        }
    }
    return 0;
}

int32_t empire_open_seller(game_state *st, int32_t g)
{
    /* @bin 0x00443B20 a city in use, trading (kind 1, 3, 5), open (+5), selling g (+6+g == 1) whose
     * route trades it (0x00447790) */
    for (int32_t i = 0; i < 61; i++) {
        uint32_t c = EMPIRE_CITY(i);
        if (U8(st, c) == 0) {
            continue;
        }
        uint8_t k = U8(st, c + 2);
        if ((k == 1 || k == 3 || k == 5) && U8(st, c + 5) != 0 && U8(st, c + 6 + (uint32_t) g) == 1 &&
            empire_city_trades(st, i, g)) {
            return 1;
        }
    }
    return 0;
}

int32_t trade_order_check(game_state *st, int32_t g)
{
    /* @bin 0x005599C0 the city's trade order of good g (i16 city +0x2826 + 2g): import (1, 3) with no
     * open buyer (0x00443950) or export (2, 4) with no open seller (0x00443B20) is cleared: 1 */
    uint32_t a = CURRENT_CITY(st) + CITY_TRADE_STATUS + 2u * (uint32_t) g;
    int16_t v = S16(st, a);
    if (v == 1 || v == 3) {
        if (goods_trade_city_flag5(st, g) == 0) {
            S16(st, a) = 0;
            return 1;
        }
    }
    v = S16(st, a);
    if ((v == 2 || v == 4) && empire_open_seller(st, g) == 0) {
        S16(st, a) = 0;
        return 1;
    }
    return 0;
}

void event_city_status(game_state *st, uint32_t e)
{
    /* @bin 0x0044E2A0 (thiscall) type 19, at its date: the empire city +0x1C (0x004449D0) changes
     * kind by the subtype +0x60 and its object's kind (table 0x0044E5BC); the city's goods are taken
     * again from the object, trade orders checked (0x005599C0), a message (0x152, 0x153 or 0x83);
     * then the event +0x24 starts */
    if (!event_due(st, e, 0)) {
        return;
    }
    U32(st, e + 0x28) |= 4;
    int32_t ci = empire_city_index(st, (uint32_t) (int32_t) S16(st, e + 0x1C));
    if (ci != 0) {
        uint32_t c = EMPIRE_CITY(ci);
        int32_t obj = S16(st, c + 0x56);
        uint32_t o = EMPIRE_OBJ(obj);
        uint8_t k = U8(st, o + 0xC);
        uint8_t nk = 0;
        int ok = 1, clear5 = 0;
        int32_t id = 0x83, w2 = 0, w0 = 0;
        switch (U8(st, e + 0x60)) {
        case 0:
            ok = k == 3 || k == 4;
            nk = k == 3 ? 5 : 6;
            w2 = 0x7E; w0 = 0x7F; id = 0x152;
            break;
        case 1:
            ok = k == 5 || k == 6;
            nk = k == 5 ? 3 : 4;
            w2 = 0x82; w0 = 0x83; id = 0x153;
            break;
        case 2:
            ok = k == 6 || k == 4 || k == 2;
            nk = k == 6 ? 5 : k == 4 ? 3 : 1;
            w2 = 0x86; w0 = 0x87;
            break;
        case 3:
        case 4:
            ok = k == 5 || k == 3 || k == 1;
            nk = k == 5 ? 6 : k == 3 ? 4 : 2;
            clear5 = 1;
            w2 = U8(st, e + 0x60) == 3 ? 0x8A : 0x8E;
            w0 = w2 + 1;
            break;
        default:
            ok = 0;
            break;
        }
        if (ok) {
            uint8_t blk[32] = {0};
            put16(blk, 2, w2);
            put16(blk, 0, w0);
            put16(blk, 0x10, event_message_kind(st, e));
            event_fill_block(st, e, blk);
            U8(st, o + 0xC) = nk;
            S16(st, o) = (int16_t) empire_object_image(st, obj);
            U8(st, c + 2) = U8(st, o + 0xC);
            if (clear5) {
                U8(st, c + 5) = 0;
            }
            for (uint32_t g = 1; g < 0x24; g++) {
                U8(st, c + 0x2A + g) = 0;
                U8(st, c + 6 + g) = 0;
                uint8_t ck = U8(st, c + 2);
                if (ck == 1 || ck == 3 || ck == 5 || ck == 0) {
                    if (empire_object_sells(st, obj, g)) {
                        U8(st, c + 0x2A + g) = 1;
                    }
                    if (empire_object_buys(st, obj, g)) {
                        U8(st, c + 6 + g) = 1;
                    }
                }
                trade_order_check(st, (int32_t) g);
            }
            S32(st, G_MESSAGE_POPUP) = 1;
            message_post(st, id, 0, 0, blk, 0);
        }
    }
    if (event_link_active(st, e, S16(st, e + 0x24))) {
        int32_t v = event_outcome_message(st, e, 0);
        event_start(st, e, S16(st, e + 0x24), v);
    }
}

static int32_t have_amount(game_state *st, uint32_t e)
{
    /* @bin 0x0044E910 money (good 0x24): the asked amount itself; else 0x0045AAC0(player, good) / 100 */
    int16_t g = S16(st, e + 4);
    if (g == 0x24) {
        return S16(st, e + 0xC);
    }
    return GAME_CALL(st, FN_GOODS_AMOUNT_45AAC0, S32(st, CURRENT_PLAYER_ADDR), g) / 100;
}

void event_festival_request(game_state *st, uint32_t e)
{
    /* @bin 0x0044E890 (thiscall) type 23: goods or money (+4, +0xC) asked; at its date and when its
     * months run out (+0x2E) the amount the city has is compared: all (5), half (7), less (0), kept
     * in the message block +0x16; the stage +0x30 moves on (start: 1 / 2, 2: 3 / 4, 4: 5 / 8). Stages
     * 6, 7, 8: message 0x84 and the event +0x24, +0x26 or +0x52 starts (6: the amount is given:
     * money to the treasury, or goods by 0x0045AFE0) */
    if (S16(st, e + 0x30) == 9) {
        return;
    }
    uint8_t blk[32] = {0};
    int on = U8(st, e + 0x56) != 0;
    put16(blk, 2, on ? 0x108 : 0x109);
    put16(blk, 0x10, event_message_kind(st, e));
    event_fill_block(st, e, blk);
    int16_t stage = S16(st, e + 0x30);
    if (stage == 6 || stage == 7 || stage == 8) {
        /* @bin 0x0044EC17 */
        S32(st, G_MESSAGE_POPUP) = 1;
        int slot, link;
        if (stage == 6) {
            int32_t amt = S16(st, e + 0xC);
            if (S16(st, e + 4) == 0x24) {
                uint32_t c = CURRENT_CITY(st);
                put16(blk, 0x14, amt);
                S32(st, c + CITY_TREASURY) += amt;
                S32(st, c + CITY_GIFTS_THIS_YEAR) += amt;
                put16(blk, 0, 0x119 - on);
            } else {
                int32_t r = GAME_CALL(st, FN_GOODS_TO_STORAGE, S16(st, e + 4), (int32_t) ((uint32_t) amt * 100u), 0);
                put16(blk, 0, 0x117 - on);
                put16(blk, 0x14, (int16_t) r / 100);
            }
            slot = 0;
            link = 0x24;
        } else if (stage == 7) {
            put16(blk, 0, 0x11D - on);
            slot = 1;
            link = 0x26;
        } else {
            put16(blk, 0, 0x115 - on);
            slot = 2;
            link = 0x52;
        }
        message_post(st, 0x84, S16(st, e), 0, blk, 0);
        if (event_link_active(st, e, S16(st, e + (uint32_t) link))) {
            int32_t v = event_outcome_message(st, e, slot);
            event_start(st, e, S16(st, e + (uint32_t) link), v);
        }
        S16(st, e + 0x30) = 9;
        U8(st, e + 0x32) = 0;
        U32(st, e + 0x28) |= 4;
        return;
    }
    int32_t need, have, part;
    if (U8(st, e + 0x32) != 0) {
        S16(st, e + 0x2E) = (int16_t) (S16(st, e + 0x2E) - 1);
        if (S16(st, e + 0x2E) > 0) {
            return;
        }
        need = S16(st, e + 0xC);
        have = have_amount(st, e);
        int full = need <= have, half = need / 2 <= have;
        int16_t s = S16(st, e + 0x30);
        if (s != 2 && s != 4) {
            return;
        }
        if (full) {
            part = 5;
            put16(blk, 0, (S16(st, e + 4) == 0x24 ? 0x10D : 0x10B) - on);
        } else if (half) {
            part = 7;
            put16(blk, 0, (s == 2 ? 0x10F : 0x113) - on);
        } else {
            part = 0;
            put16(blk, 0, (s == 2 ? 0x111 : 0x115) - on);
        }
        put16(blk, 0x16, part);
        S32(st, G_MESSAGE_POPUP) = 1;
        message_post(st, 0x84, S16(st, e), 0, blk, 0);
        S16(st, e + 0x30) = (int16_t) (s == 2 ? (part != 0 ? 3 : 4) : (part != 0 ? 5 : 8));
        return;
    }
    /* @bin 0x0044EADE the start, at its date */
    if (U8(st, e + 0x28) & 0x11) {
        return;
    }
    if (S32(st, G_YEAR) != S16(st, e + 0x14) + S16(st, G_START_YEAR) || S32(st, G_MONTH) != (int32_t) U8(st, e + 3)) {
        return;
    }
    need = S16(st, e + 0xC);
    U8(st, e + 0x32) = 1;
    have = have_amount(st, e);
    if (need <= have) {
        part = 5;
        put16(blk, 0, (S16(st, e + 4) == 0x24 ? 0x10D : 0x10B) - on);
    } else if (need / 2 <= have) {
        part = 7;
        put16(blk, 0, 0x10F - on);
    } else {
        part = 0;
        S16(st, e + 0x2E) = 1;
        put16(blk, 0, 0x111 - on);
    }
    put16(blk, 0x16, part);
    S32(st, G_MESSAGE_POPUP) = 1;
    message_post(st, 0x84, S16(st, e), 0, blk, 0);
    S16(st, e + 0x30) = (int16_t) (part != 0 ? 1 : 2);
}

#define THIS_REC ((uint32_t) a[PORT_THIS_ARG])

static int32_t run_request(game_state *st, const int32_t *a) { event_request(st, THIS_REC); return 0; }
static int32_t run_city_status(game_state *st, const int32_t *a) { event_city_status(st, THIS_REC); return 0; }
static int32_t run_fest_req(game_state *st, const int32_t *a) { event_festival_request(st, THIS_REC); return 0; }
static int32_t run_obj_image(game_state *st, const int32_t *a) { return empire_object_image(st, a[0]); }
static int32_t run_obj_sells(game_state *st, const int32_t *a) { return empire_object_sells(st, a[0], (uint32_t) a[1]); }
static int32_t run_obj_buys(game_state *st, const int32_t *a) { return empire_object_buys(st, a[0], (uint32_t) a[1]); }
static int32_t run_open_seller(game_state *st, const int32_t *a) { return empire_open_seller(st, a[0]); }
static int32_t run_order_check(game_state *st, const int32_t *a) { return trade_order_check(st, a[0]); }

const ported_function month2_handlers_ports[] = {
    {0x0044CDC0u, "event_request", 0, 0, 0, run_request, 0},
    {0x0044E2A0u, "event_city_status", 0, 0, 0, run_city_status, 0},
    {0x0044E890u, "event_festival_request", 0, 0, 0, run_fest_req, 0},
    {0x004425C0u, "empire_object_image", 1, 32, 0, run_obj_image, 0},
    {0x00443000u, "empire_object_sells", 2, 32, 0, run_obj_sells, 0},
    {0x00443040u, "empire_object_buys", 2, 32, 0, run_obj_buys, 0},
    {0x00443B20u, "empire_open_seller", 1, 32, 0, run_open_seller, 0},
    {0x005599C0u, "trade_order_check", 1, 8, 0, run_order_check, 0},
};
const int month2_handlers_ports_count = sizeof(month2_handlers_ports) / sizeof(month2_handlers_ports[0]);
