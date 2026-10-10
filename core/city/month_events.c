/* Imhotep - the scenario events of the month (the list 0x00785D70, 0x0044C660 and its handlers).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The list: i32 count, then records of 0x7C bytes from +4 (+0 i16 index, +2 type, +3 month,
 * +0x14 i16 year from the scenario's start 0x007848D8, +0x24 i16 the next event, +0x28 flags).
 * A handler runs when its date comes (0x0044D680), marks the record (+0x28 bit 2), does its
 * action, posts its message and starts the next event (0x0044C490). Handlers not ported yet go
 * through GAME_CALLT with the record as `this`. */
#include "city/month_events.h"

#include "city/month.h"
#include "city/month_health.h"
#include "city/month_message.h"
#include "random/random.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define EVENT_REC 0x7Cu

uint32_t event_at(game_state *st, int32_t n)
{
    /* @bin 0x0044C0D0 (stdcall) the record n, or 0 */
    if (n >= 0 && n < S32(st, G_EVENTS)) {
        return G_EVENTS + 4 + EVENT_REC * (uint32_t) n;
    }
    return 0;
}

int32_t event_due(game_state *st, uint32_t e, int8_t force)
{
    /* @bin 0x0044D680 (thiscall) unless forced, not when +0x28 has bit 0 or 2; the year and month */
    if (force == 0 && (U8(st, e + 0x28) & 5)) {
        return 0;
    }
    if (S32(st, G_YEAR) != S16(st, e + 0x14) + S16(st, G_START_YEAR)) {
        return 0;
    }
    return S32(st, G_MONTH) == (int32_t) U8(st, e + 3);
}

int32_t event_link_active(game_state *st, uint32_t e, int32_t n)
{
    /* @bin 0x004496D0 (thiscall) the record n exists, has +0x28 bit 0, and is not this one */
    uint32_t o = event_at(st, n);
    if (o == 0 || !(U8(st, o + 0x28) & 1)) {
        return 0;
    }
    return n != S16(st, e);
}

int32_t event_months_left(game_state *st, uint32_t e)
{
    /* @bin 0x0044D750 (thiscall) (+0x14 + start - year) * 12 + +0x2C + month (+3) - month */
    int32_t y = S16(st, e + 0x14) + S16(st, G_START_YEAR) - S32(st, G_YEAR);
    return (int32_t) ((uint32_t) S16(st, e + 0x2C) + (uint32_t) y * 12u + U8(st, e + 3) - (uint32_t) S32(st, G_MONTH));
}

int32_t empire_city_index(game_state *st, uint32_t v)
{
    /* @bin 0x004449D0 the first of the 61 records of 0x0077DE00 (0x6A bytes) in use whose +4 is v */
    for (uint32_t k = 0; k < 61; k++) {
        uint32_t r = G_EMPIRE_CITIES + 0x6Au * k;
        if (U8(st, r) != 0 && U8(st, r + 4) == v) {
            return (int32_t) k;
        }
    }
    return 0;
}

int32_t event_message_kind(game_state *st, uint32_t e)
{
    /* @bin 0x0044F660 (thiscall) by the type (+2), and +0x56, +0x60 or +4 (tables 0x0044F7EC..) */
    uint8_t s = U8(st, e + 0x60);
    switch (U8(st, e + 2)) {
    case 1: {
        static const int16_t a[7] = {0x34, 0x10, 0x22, 0x46, 0x58, 0x6A, 0x7C};
        static const int16_t b[7] = {0x35, 0x11, 0x23, 0x47, 0x59, 0x6B, 0x7D};
        return s < 7 ? (U8(st, e + 0x56) != 0 ? a[s] : b[s]) : -1;
    }
    case 2:
        switch (S16(st, e + 4)) {
        case 1: return 0xBE;
        case 2: return 0xB4;
        case 3: return 0xAA;
        case 4: return 0x107;
        default: return -1;
        }
    case 3: return 0xDA;
    case 6: return 0xDE;
    case 7: return 0xE2;
    case 8: return 0xE6;
    case 9: return 0xEA;
    case 10: return 0xEE;
    case 11: return 0xF2;
    case 13: return 0xD2;
    case 14: return 0xD6;
    case 15: return 0xCA;
    case 16: return 0xCE;
    case 17: return 0xC2;
    case 18: return 0xC6;
    case 19: {
        static const int16_t c[5] = {0x81, 0x85, 0x89, 0x8D, 0x91};
        return s < 5 ? c[s] : -1;
    }
    case 20: {
        static const int16_t c[4] = {0x95, 0x99, 0x9D, 0xA1};
        return s < 4 ? c[s] : -1;
    }
    case 21: return 0xFA;
    case 22: return 0xFE;
    case 23: return U8(st, e + 0x56) != 0 ? 0x124 : 0x125;
    default: return -1;
    }
}

static void put16(uint8_t *b, uint32_t off, int32_t v)
{
    b[off] = (uint8_t) v;
    b[off + 1] = (uint8_t) ((uint32_t) v >> 8);
}

void event_fill_block(game_state *st, uint32_t e, uint8_t *b)
{
    /* @bin 0x0044E6F0 (thiscall) the message block +4..+0x1D from the record and its linked one
     * (+0x34) */
    int32_t city = empire_city_index(st, (uint32_t) (int32_t) S16(st, e + 0x1C));
    put16(b, 6, city);
    put16(b, 0x12, city);
    put16(b, 0x18, S16(st, e + 0xC));
    put16(b, 0x1A, S16(st, e + 4));
    b[0x1D] = U8(st, e + 0x36);
    if (event_at(st, S16(st, e + 0x34)) != 0) {
        if (S16(st, e + 0x64) >= 0) {
            put16(b, 0x10, S16(st, e + 0x64));
        }
        if (U8(st, e + 0x63) != 0) {
            put16(b, 0x12, empire_city_index(st, U8(st, e + 0x63)));
        }
        put16(b, 0x18, S16(st, e + 0x68));
        put16(b, 0x1A, S16(st, e + 0x66));
        b[0x1D] = U8(st, e + 0x37);
    }
    put16(b, 4, -1);
    put16(b, 8, S16(st, e + 0xC));
    put16(b, 0xA, S16(st, e + 4));
    put16(b, 0xC, S16(st, e + 0x2C));
    put16(b, 0xE, event_months_left(st, e));
    put16(b, 0x14, 0);
    put16(b, 0x16, 0);
    b[0x1C] = U8(st, e + 0x36);
}

/* the game RNG (core/random, 0x004D7090) on the state */

static void rng_partial(game_state *st)
{
    /* @bin 0x00448B1D the step as 0x00448A90 inlines it first: the pool and the two registers,
     * r1_7bit; the other derived values are not stored */
    int32_t k = S32(st, G_RANDOM_POOL_INDEX);
    U32(st, G_RANDOM_POOL + 4u * (uint32_t) k) = U32(st, G_RANDOM_R1_7BIT);
    k++;
    S32(st, G_RANDOM_POOL_INDEX) = k < 100 ? k : 0;
    uint32_t a = U32(st, G_RANDOM_IV1), b = U32(st, G_RANDOM_IV2);
    for (int i = 0; i < 31; i++) {
        uint32_t x = ((a >> 4) ^ a) & 1, y = ((b >> 4) ^ b) & 1;
        a >>= 1;
        b >>= 1;
        if (x) {
            a |= 0x40000000u;
        }
        if (y) {
            b |= 0x40000000u;
        }
    }
    U32(st, G_RANDOM_IV1) = a;
    U32(st, G_RANDOM_R1_7BIT) = a & 0x7F;
    U32(st, G_RANDOM_IV2) = b;
}

void event_range_pick(game_state *st, uint32_t p)
{
    /* @bin 0x004487F0 (thiscall) i16 +0 from +2 (a fixed value, >= 0), or a game RNG pick: +2 < 0:
     * in +4 .. +6 (exclusive); else one of +2, +4, +6 (those not negative) */
    int16_t fixed = S16(st, p + 2);
    if (fixed >= 0) {
        if (S16(st, p + 4) < 0) {
            S16(st, p) = fixed;
            return;
        }
    } else if (S16(st, p + 4) >= 0 && S16(st, p + 6) >= 0 && S16(st, p + 6) - S16(st, p + 4) + 1 == 1) {
        S16(st, p) = fixed;
        return;
    }
    random_next_state(st);
    int32_t r = (int32_t) (U32(st, G_RANDOM_R1_15BIT) & 0xFFFF);
    fixed = S16(st, p + 2);
    if (fixed < 0) {
        int16_t lo = S16(st, p + 4);
        S16(st, p) = (int16_t) (r % (S16(st, p + 6) - lo) + lo);
        return;
    }
    int16_t lo = S16(st, p + 4);
    int32_t n = lo < 0 ? 1 : S16(st, p + 6) >= 0 ? 3 : 2;
    switch (r % n) {
    case 0: S16(st, p) = fixed; break;
    case 1: S16(st, p) = lo; break;
    default: S16(st, p) = S16(st, p + 6); break;
    }
}

void event_reset(game_state *st, uint32_t e)
{
    /* @bin 0x00448A90 (thiscall) the record's ranges picked again (+4, +0xC, +0x14, +0x1C, +0x58),
     * its state cleared, +0x61 := 4 + rand % 5 and +0x62 := rand & 1 (two game RNG steps, the
     * first inlined without its derived values) */
    U32(st, e + 0x28) &= ~4u;
    event_range_pick(st, e + 4);
    event_range_pick(st, e + 0xC);
    event_range_pick(st, e + 0x14);
    event_range_pick(st, e + 0x1C);
    event_range_pick(st, e + 0x58);
    uint16_t when = (uint16_t) (U8(st, e + 3) + (uint16_t) S16(st, e + 0x14));
    U8(st, e + 0x57) = 0;
    S16(st, e + 0x30) = 0;
    U16(st, e + 0x2E) = when;
    S16(st, e + 0x34) = -1;
    S16(st, e + 0x64) = -1;
    U8(st, e + 0x32) = 0;
    U8(st, e + 0x33) = 0;
    U8(st, e + 0x63) = 0;
    if (U8(st, e + 2) == 3) {
        for (uint32_t k = 0; k < 4; k++) {
            U16(st, e + 0x40 + 4 * k) = U16(st, G_U784C60);
            U16(st, e + 0x42 + 4 * k) = U16(st, G_U784C62);
        }
    }
    rng_partial(st);
    U8(st, e + 0x61) = (uint8_t) ((int32_t) (U32(st, G_RANDOM_IV1) & 0x7FFF) % 5 + 4);
    random_next_state(st);
    U8(st, e + 0x62) = (uint8_t) (U32(st, G_RANDOM_IV1) & 1);
}

static void event_chain(game_state *st, uint32_t e)
{
    /* the handlers' common end: the next event (+0x24) starts (0x0044FD30, 0x0044C490) */
    if (event_link_active(st, e, S16(st, e + 0x24))) {
        int32_t v = GAME_CALLT(st, e, FN_EVENT_OUTCOME_MESSAGE, 0);
        GAME_CALLT(st, e, FN_EVENT_START, S16(st, e + 0x24), v);
    }
}

static int start(game_state *st, uint32_t e)
{
    if (!event_due(st, e, 0)) {
        return 0;
    }
    U32(st, e + 0x28) |= 4;
    return 1;
}

static void post(game_state *st, int32_t id, int32_t b, const uint8_t *block)
{
    S32(st, G_MESSAGE_POPUP) = 1;
    message_post(st, id, 0, b, block, 0);
}

void event_0x44db20(game_state *st, uint32_t e)
{
    /* @bin 0x0044DB20 type 6: when city +0x447C (i16) > 0, +0x4480 := 0x30 and message 0x109 */
    if (!start(st, e)) {
        return;
    }
    uint32_t c = CURRENT_CITY(st);
    if (S16(st, c + CITY_U447C) > 0) {
        S16(st, c + CITY_U4480) = 0x30;
        S32(st, G_MESSAGE_POPUP) = 1;
        uint8_t b[32] = {0};
        put16(b, 2, 0xDB);
        put16(b, 0, 0xDC);
        put16(b, 0x10, event_message_kind(st, e));
        event_fill_block(st, e, b);
        message_post(st, 0x109, 0, 0, b, 0);
    }
    event_chain(st, e);
}

void event_0x44df90(game_state *st, uint32_t e)
{
    /* @bin 0x0044DF90 type 12: 0x00469F80(0x6D); message 0x10F with its result when not 0 */
    if (!start(st, e)) {
        return;
    }
    int32_t r = GAME_CALL(st, FN_BUILDING_DESTROY_FIRST_OF_TYPE, 0x6D);
    if (r != 0) {
        post(st, 0x10F, r, NULL);
    }
    event_chain(st, e);
}

static void post_block(game_state *st, uint32_t e, int32_t w0, int32_t w2, int32_t id, int32_t a, int32_t b)
{
    /* the handlers' message with a block: +0 and +2 given, +0x10 the kind, the rest 0x0044E6F0 */
    uint8_t blk[32] = {0};
    put16(blk, 2, w2);
    put16(blk, 0, w0);
    put16(blk, 0x10, event_message_kind(st, e));
    event_fill_block(st, e, blk);
    message_post(st, id, a, b, blk, 0);
}

void event_0x44d970(game_state *st, uint32_t e)
{
    /* @bin 0x0044D970 type 15: the pair of 0x005D2C48 (8 bytes) by +4 both rise by +0xC; message
     * 0x115 */
    if (!start(st, e)) {
        return;
    }
    int32_t amt = S16(st, e + 0xC), g = S16(st, e + 4);
    uint32_t t = G_PRICE_BUY + 8u * (uint32_t) g;
    S32(st, t) += amt;
    S32(st, t + 4) += amt;
    S32(st, G_MESSAGE_POPUP) = 1;
    post_block(st, e, 0xC8, 0xC7, 0x115, amt, g);
    event_chain(st, e);
}

void event_0x44da30(game_state *st, uint32_t e)
{
    /* @bin 0x0044DA30 type 16: when the pair's second is above 0: both fall by +0xC (to 2, 0 when
     * not above it); message 0x116. Nothing at all otherwise */
    if (!start(st, e)) {
        return;
    }
    int32_t g = S16(st, e + 4), amt = S16(st, e + 0xC);
    uint32_t t = G_PRICE_BUY + 8u * (uint32_t) g;
    int32_t v = S32(st, t + 4);
    if (v <= 0) {
        return;
    }
    if (v > amt) {
        S32(st, t) -= amt;
        S32(st, t + 4) -= amt;
    } else {
        S32(st, t) = 2;
        S32(st, t + 4) = 0;
    }
    S32(st, G_MESSAGE_POPUP) = 1;
    post_block(st, e, 0xCC, 0xCB, 0x116, amt, g);
    event_chain(st, e);
}

void event_0x44dbd0(game_state *st, uint32_t e)
{
    /* @bin 0x0044DBD0 type 7: when city +0x447E (i16) > 0, +0x4482 := 0x30 and message 0x108 */
    if (!start(st, e)) {
        return;
    }
    uint32_t c = CURRENT_CITY(st);
    if (S16(st, c + CITY_U447E) > 0) {
        S16(st, c + CITY_U4482) = 0x30;
        S32(st, G_MESSAGE_POPUP) = 1;
        if (U8(st, G_U784CB0) == 2) {
            post_block(st, e, 0xE0, 0xDF, 0x108, 0, 0);
        } else {
            post_block(st, e, 0xF4, 0xF3, 0x108, 0, 0);
        }
    }
    event_chain(st, e);
}

void event_0x44dca0(game_state *st, uint32_t e)
{
    /* @bin 0x0044DCA0 type 8: city +0x2C0C += +0xC unless it would pass 45 (with 0x004C38F0);
     * message 0x10B */
    if (!start(st, e)) {
        return;
    }
    int32_t r = city_0x4c38f0(st);
    uint32_t c = CURRENT_CITY(st);
    int32_t v = (int32_t) ((uint32_t) S16(st, e + 0xC) + U32(st, c + CITY_WAGE_KINGDOM));
    if (r + v <= 0x2D) {
        S32(st, c + CITY_WAGE_KINGDOM) = v;
        S32(st, G_MESSAGE_POPUP) = 1;
        post_block(st, e, 0xE4, 0xE3, 0x10B, 0, 0);
    }
    event_chain(st, e);
}

void event_0x44dd60(game_state *st, uint32_t e)
{
    /* @bin 0x0044DD60 type 9: city +0x2C0C -= +0xC unless it would fall below 5; message 0x10C */
    if (!start(st, e)) {
        return;
    }
    int32_t r = city_0x4c38f0(st);
    uint32_t c = CURRENT_CITY(st);
    int32_t v = (int32_t) (U32(st, c + CITY_WAGE_KINGDOM) - (uint32_t) S16(st, e + 0xC));
    if (r + v >= 5) {
        S32(st, c + CITY_WAGE_KINGDOM) = v;
        S32(st, G_MESSAGE_POPUP) = 1;
        post_block(st, e, 0xE8, 0xE7, 0x10C, 0, 0);
    }
    event_chain(st, e);
}

void event_0x44de20(game_state *st, uint32_t e)
{
    /* @bin 0x0044DE20 type 10: 0x004E85D0(-50, -40 or -25 by the mood +0x18); message 0x10D */
    if (!start(st, e)) {
        return;
    }
    int32_t m = S32(st, CURRENT_CITY(st) + CITY_HEALTH);
    GAME_CALL(st, FN_CITY_MOOD_ADD, m > 0x50 ? -0x32 : m > 0x3C ? -0x28 : -0x19);
    S32(st, G_MESSAGE_POPUP) = 1;
    post_block(st, e, 0xEC, 0xEB, 0x10D, 0, 0);
    event_chain(st, e);
}

void event_0x44dee0(game_state *st, uint32_t e)
{
    /* @bin 0x0044DEE0 type 11: 0x00469F80(0xA1); message 0x10E with its result when not 0 */
    if (!start(st, e)) {
        return;
    }
    int32_t r = GAME_CALL(st, FN_BUILDING_DESTROY_FIRST_OF_TYPE, 0xA1);
    if (r != 0) {
        S32(st, G_MESSAGE_POPUP) = 1;
        post_block(st, e, 0xF0, 0xEF, 0x10E, 0, r);
    }
    event_chain(st, e);
}

int32_t empire_city_open(game_state *st, uint32_t v)
{
    /* @bin 0x00444980 the record of 0x0077DE00 in use whose +4 is v: its +5 != 0 (none: 0) */
    for (uint32_t k = 0; k < 61; k++) {
        uint32_t r = G_EMPIRE_CITIES + 0x6Au * k;
        if (U8(st, r) != 0 && U8(st, r + 4) == v) {
            return U8(st, r + 5) != 0;
        }
    }
    return 0;
}

static uint32_t trade_slot(game_state *st, uint32_t e)
{
    /* the i32 of 0x0077646C for the good +4 of the city +0x1C (36 per city) */
    return G_TRADE_ROUTE_LIMITS + 4u * (uint32_t) (S16(st, e + 4) + 36 * S16(st, e + 0x1C));
}

void event_0x44d780(game_state *st, uint32_t e)
{
    /* @bin 0x0044D780 type 13: the trade slot below 40 rises to 15, 25 or 40; message 0x111 when the
     * city is open */
    if (!start(st, e)) {
        return;
    }
    int32_t g = S16(st, e + 4), k = S16(st, e + 0x1C);
    uint32_t t = trade_slot(st, e);
    int32_t v = S32(st, t);
    if (v < 0x28) {
        S32(st, t) = v >= 0x19 ? 0x28 : v < 0xF ? 0xF : 0x19;
        if (empire_city_open(st, (uint32_t) k)) {
            uint8_t blk[32] = {0};
            put16(blk, 2, 0xCF);
            put16(blk, 0, 0xD0);
            put16(blk, 0x10, event_message_kind(st, e));
            event_fill_block(st, e, blk);
            S32(st, G_MESSAGE_POPUP) = 1;
            message_post(st, 0x111, empire_city_index(st, (uint32_t) k), g, blk, 0);
        }
    }
    event_chain(st, e);
}

void event_0x44d870(game_state *st, uint32_t e)
{
    /* @bin 0x0044D870 type 14: the trade slot above 0 falls to 25, 15 or 0; 0x005599C0(good);
     * message 0x112 when the city is open */
    if (!start(st, e)) {
        return;
    }
    int32_t g = S16(st, e + 4), k = S16(st, e + 0x1C);
    uint32_t t = trade_slot(st, e);
    int32_t v = S32(st, t);
    if (v > 0) {
        S32(st, t) = v > 0x19 ? 0x19 : v <= 0xF ? 0 : 0xF;
        GAME_CALL(st, FN_TRADE_ORDER_CHECK, g);
        if (empire_city_open(st, (uint32_t) k)) {
            uint8_t blk[32] = {0};
            put16(blk, 2, 0xD3);
            put16(blk, 0, 0xD4);
            put16(blk, 0x10, event_message_kind(st, e));
            event_fill_block(st, e, blk);
            S32(st, G_MESSAGE_POPUP) = 1;
            message_post(st, 0x112, empire_city_index(st, (uint32_t) k), g, blk, 0);
        }
    }
    event_chain(st, e);
}

void event_0x44c7e0(game_state *st, uint32_t e)
{
    /* @bin 0x0044C7E0 type 17: message 0x83 (the linked event's index, else this one's), city
     * +0x42D8 += +0xC; then +0x28 bit 2, +0x32 := 0 */
    if (!event_due(st, e, 0)) {
        return;
    }
    uint32_t linked = event_at(st, S16(st, e + 0x34));
    int32_t who = S16(st, e);
    uint8_t blk[32] = {0};
    put16(blk, 2, 0xBF);
    put16(blk, 0, 0xC0);
    put16(blk, 0x10, event_message_kind(st, e));
    event_fill_block(st, e, blk);
    if (linked != 0) {
        who = S16(st, linked);
    }
    S32(st, G_MESSAGE_POPUP) = 1;
    message_post(st, 0x83, who, 0, blk, 0);
    city_add_0x42d8(st, S16(st, e + 0xC));
    U32(st, e + 0x28) |= 4;
    U8(st, e + 0x32) = 0;
    event_chain(st, e);
}

void event_0x44c8a0(game_state *st, uint32_t e)
{
    /* @bin 0x0044C8A0 type 18: message 0x83, city +0x42D8 -= +0xC, +0x4588 := +0xC; then +0x28
     * bit 2, +0x32 := 0 */
    if (!event_due(st, e, 0)) {
        return;
    }
    uint8_t blk[32] = {0};
    put16(blk, 2, 0xC3);
    put16(blk, 0, 0xC4);
    put16(blk, 0x10, event_message_kind(st, e));
    event_fill_block(st, e, blk);
    S32(st, G_MESSAGE_POPUP) = 1;
    message_post(st, 0x83, S16(st, e), 0, blk, 0);
    city_add_0x42d8(st, -(int32_t) S16(st, e + 0xC));
    S32(st, CURRENT_CITY(st) + CITY_U4588) = S16(st, e + 0xC);
    U32(st, e + 0x28) |= 4;
    U8(st, e + 0x32) = 0;
    event_chain(st, e);
}

void event_0x44e5d0(game_state *st, uint32_t e)
{
    /* @bin 0x0044E5D0 type 20: a message by +0x60 (table 0x0044E6E0): 0x106, 0x107, else 0x83 */
    if (!start(st, e)) {
        return;
    }
    S32(st, G_MESSAGE_POPUP) = 1;
    uint8_t blk[32] = {0};
    int32_t id = 0x83;
    switch (U8(st, e + 0x60)) {
    case 0: put16(blk, 2, 0x92); put16(blk, 0, 0x93); id = 0x106; break;
    case 1: put16(blk, 2, 0x96); put16(blk, 0, 0x97); id = 0x107; break;
    case 2: put16(blk, 2, 0x9A); put16(blk, 0, 0x9B); break;
    case 3: put16(blk, 2, 0x9E); put16(blk, 0, 0x9F); break;
    default: put16(blk, 0, -1); put16(blk, 2, -1); put16(blk, 0x10, -1); break;
    }
    if (U8(st, e + 0x60) <= 3) {
        put16(blk, 0x10, event_message_kind(st, e));
    }
    event_fill_block(st, e, blk);
    message_post(st, id, 0, 0, blk, 0);
    event_chain(st, e);
}

void event_0x44e7b0(game_state *st, uint32_t e, int32_t high)
{
    /* @bin 0x0044E7B0 types 21 (0) and 22 (100), unless 0x00605A3C == 1: the flood manager
     * 0x00D05720 by 0x004BED00 / 0x004BED40; message 0x158 / 0x15D */
    if (S32(st, G_EDITOR) == 1 || !start(st, e)) {
        return;
    }
    uint8_t blk[32] = {0};
    int32_t id;
    if (high == 0) {
        GAME_CALLT0(st, G_FLOODPLAIN_MANAGER, FN_FLOOD_WORSE);
        put16(blk, 2, 0xF7);
        put16(blk, 0, 0xF8);
        id = 0x158;
    } else {
        GAME_CALLT0(st, G_FLOODPLAIN_MANAGER, FN_FLOOD_BETTER);
        put16(blk, 2, 0xFB);
        put16(blk, 0, 0xFC);
        id = 0x15D;
    }
    put16(blk, 0x10, event_message_kind(st, e));
    event_fill_block(st, e, blk);
    S32(st, G_MESSAGE_POPUP) = 1;
    message_post(st, id, 0, 0, blk, 0);
    event_chain(st, e);
}

/* @bin 0x0044E000..0x0044E230 types 24..29: an action, then (0x0044E0E0: before it) a message */
static void simple(game_state *st, uint32_t e, uint32_t action, int with_arg, int msg_first, int32_t msg)
{
    if (!start(st, e)) {
        return;
    }
    if (msg_first) {
        post(st, msg, 0, NULL);
    }
    if (with_arg) {
        GAME_CALL(st, action, S16(st, e + 0xC));
    } else {
        GAME_CALL0(st, action);
    }
    if (!msg_first) {
        post(st, msg, 0, NULL);
    }
    event_chain(st, e);
}

void event_month_step(game_state *st, uint32_t e)
{
    /* @bin 0x0044C660 (thiscall) by the type (+2, table 0x0044C700) */
    switch (U8(st, e + 2)) {
    case 1: GAME_CALLT0(st, e, FN_EVENT_REQUEST); break;
    case 4: case 5: GAME_CALL0(st, FN_NOTHING); break;
    case 6: event_0x44db20(st, e); break;
    case 7: event_0x44dbd0(st, e); break;
    case 8: event_0x44dca0(st, e); break;
    case 9: event_0x44dd60(st, e); break;
    case 10: event_0x44de20(st, e); break;
    case 11: event_0x44dee0(st, e); break;
    case 12: event_0x44df90(st, e); break;
    case 13: event_0x44d780(st, e); break;
    case 14: event_0x44d870(st, e); break;
    case 15: event_0x44d970(st, e); break;
    case 16: event_0x44da30(st, e); break;
    case 17: event_0x44c7e0(st, e); break;
    case 18: event_0x44c8a0(st, e); break;
    case 19: GAME_CALLT0(st, e, FN_EVENT_CITY_STATUS); break;
    case 20: event_0x44e5d0(st, e); break;
    case 21: event_0x44e7b0(st, e, 0); break;
    case 22: event_0x44e7b0(st, e, 100); break;
    case 23: GAME_CALLT0(st, e, FN_EVENT_FESTIVAL_REQUEST); break;
    case 24: simple(st, e, FN_PLAGUE_FROGS_END, 0, 0, 0x1E3); break;
    case 25: simple(st, e, FN_PLAGUE_FROGS, 1, 0, 0x1E4); break;
    case 26: simple(st, e, FN_PLAGUE_HAIL, 0, 1, 0x1E5); break;
    case 27: simple(st, e, FN_PLAGUE_FLIES, 1, 0, 0x1E6); break;
    case 28: simple(st, e, FN_PLAGUE_FIRE, 1, 0, 0x1EB); break;
    case 29: simple(st, e, FN_PLAGUE_TOMB_ROBBERS, 1, 0, 0x1F0); break;
    default: break;
    }
}

static int32_t run_at(game_state *st, const int32_t *a)
{
    return (int32_t) event_at(st, a[0]);
}

static int32_t run_city_open(game_state *st, const int32_t *a)
{
    return empire_city_open(st, (uint32_t) a[0]);
}

static int32_t run_city_index(game_state *st, const int32_t *a)
{
    return empire_city_index(st, (uint32_t) a[0]);
}

/* The thiscall helpers take any record as `this`: they are ported inside their callers only. */
const ported_function month_events_ports[] = {
    {0x0044C0D0u, "event_at", 1, 32, 0, run_at, 0},
    {0x004449D0u, "empire_city_index", 1, 32, 0, run_city_index, 0},
    {0x0044D680u, "event_due (in month_events.c)", 1, 8, 0, NULL, 0},
    {0x004496D0u, "event_link_active (in month_events.c)", 1, 8, 0, NULL, 0},
    {0x0044D750u, "event_months_left (in month_events.c)", 0, 32, 0, NULL, 0},
    {0x0044F660u, "event_message_kind (in month_events.c)", 0, 32, 0, NULL, 0},
    {0x0044E6F0u, "event_fill_block (in month_events.c)", 1, 0, 0, NULL, 0},
    {0x0044C660u, "event_month_step (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044DB20u, "event_0x44db20 (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044DF90u, "event_0x44df90 (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044D970u, "event_0x44d970 (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044DA30u, "event_0x44da30 (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044DBD0u, "event_0x44dbd0 (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044DCA0u, "event_0x44dca0 (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044DD60u, "event_0x44dd60 (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044DE20u, "event_0x44de20 (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044DEE0u, "event_0x44dee0 (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044D780u, "event_0x44d780 (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044D870u, "event_0x44d870 (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044C7E0u, "event_0x44c7e0 (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044C8A0u, "event_0x44c8a0 (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044E5D0u, "event_0x44e5d0 (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044E7B0u, "event_0x44e7b0 (in month_events.c)", 1, 0, 0, NULL, 0},
    {0x00444980u, "empire_city_open", 1, 8, 0, run_city_open, 0},
    {0x004487F0u, "event_range_pick (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x00448A90u, "event_reset (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044E000u, "event type 24 (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044E070u, "event type 25 (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044E0E0u, "event type 26 (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044E150u, "event type 27 (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044E1C0u, "event type 28 (in month_events.c)", 0, 0, 0, NULL, 0},
    {0x0044E230u, "event type 29 (in month_events.c)", 0, 0, 0, NULL, 0},
};
const int month_events_ports_count = sizeof(month_events_ports) / sizeof(month_events_ports[0]);
