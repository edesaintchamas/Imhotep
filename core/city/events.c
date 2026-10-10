/* Imhotep - scenario events: requests and invasions of the month, starting linked events, the
 * message chosen for an event's outcome (package month2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The event list 0x00785D70 (core/city/month_events.c): i32 count, records of 0x7C bytes from +4.
 * Record fields used here: +0 i16 index, +2 type, +3 month, +4/+0xC/+0x14/+0x1C/+0x58 ranges
 * (i16 value, then fixed / low / high), +0x24 +0x26 +0x52 +0x54 the events linked to the four
 * outcomes, +0x28 flags, +0x2C months allowed, +0x34 the event that started this one, +0x36,
 * +0x56, +0x57 reminders sent, +0x60 subtype, +0x63..+0x68 copied from the starter, +0x6A..+0x6D
 * the outcomes' message levels. */
#include "city/events.h"

#include "city/month_events.h"
#include "city/month_message.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define EVENT_REC 0x7Cu

int32_t event_before_date(game_state *st, uint32_t e)
{
    /* @bin 0x0044D6D0 (thiscall) the record's date (+0x14 + start year, month +3) is after now */
    int32_t y = S16(st, e + 0x14) + S16(st, G_START_YEAR);
    if (S32(st, G_YEAR) < y) {
        return 1;
    }
    if (S32(st, G_YEAR) > y) {
        return 0;
    }
    return S32(st, G_MONTH) < (int32_t) U8(st, e + 3);
}

int32_t event_pending(game_state *st, uint32_t e)
{
    /* @bin 0x0044D700 (thiscall) not +0x28 bits 0 / 4, its date reached, and the months since it
     * not above +0x2C */
    if (U8(st, e + 0x28) & 0x11) {
        return 0;
    }
    if (event_before_date(st, e)) {
        return 0;
    }
    int32_t m = (S32(st, G_YEAR) - S16(st, e + 0x14) - S16(st, G_START_YEAR)) * 12 - (int32_t) U8(st, e + 3) +
                S32(st, G_MONTH);
    return m <= S16(st, e + 0x2C);
}

int32_t event_linked_flag(game_state *st, uint32_t e)
{
    /* @bin 0x0044EF60 (thiscall) the starter (+0x34) exists and has +0x28 bit 4 */
    uint32_t o = event_at(st, S16(st, e + 0x34));
    if (o == 0) {
        return 0;
    }
    return (U32(st, o + 0x28) >> 4) & 1;
}

int32_t event_type_linkable(game_state *st, uint32_t e)
{
    /* @bin 0x00449390 (thiscall) by the type: 0x00449370 (table 0x005D9C18) and not 0x00449380
     * (table 0x005D9C38) */
    uint8_t t = U8(st, e + 2);
    if (U8(st, G_U5D9C18 + t) == 0) {
        return 0;
    }
    return U8(st, G_U5D9C38 + t) == 0;
}

static int32_t event_type_has_good(game_state *st, uint32_t e)
{
    /* @bin 0x004493C0 (thiscall) table 0x005D9C78 by the type */
    return U8(st, G_EVENT_TYPE_FLAGS + U8(st, e + 2));
}

static int32_t event_good_ok_type1(game_state *st, uint32_t e, int32_t g)
{
    /* @bin 0x00449440 (thiscall) the goods a type-1 event may ask, by its subtype +0x60 (tables
     * 0x004494BC, 0x004494E0, 0x0044950C) */
    switch (U8(st, e + 0x60)) {
    case 0:
    case 6:
        return g != 0x25 && g != 0x15 && g != 0x1B;
    case 1:
    case 2:
        return g == 0x25;
    case 3:
        /* @bin 0x00449474 goods 1..36 whose byte in 0x004494E0 is 0 */
        return (g >= 1 && g <= 8) || g == 15 || g == 17 || g == 19 || g == 36;
    case 4:
        /* @bin 0x0044948D goods 12..36 whose byte in 0x0044950C is 0 */
        return g == 12 || g == 20 || g == 24 || g == 25 || g == 26 || g == 30 || g == 36;
    case 5:
        return g > 0 && g <= 8;
    default:
        return 0;
    }
}

int32_t event_good_allowed(game_state *st, uint32_t e, int32_t g)
{
    /* @bin 0x00449550 (thiscall) g in the type's range [0x005D9C98[type], 0x005D9D10[type]) (0x004493D0,
     * 0x004493E0); type 1: 0x00449440, type 23: not 0x25 / 0x1B (0x00449530), types 13..16: not 0x1B
     * / 0x15 */
    uint8_t t = U8(st, e + 2);
    if (g < S32(st, G_U5D9C98 + 4u * t) || g >= S32(st, G_U5D9D10 + 4u * t)) {
        return 0;
    }
    if (t == 1) {
        return event_good_ok_type1(st, e, g);
    }
    if (t == 0x17) {
        return g != 0x25 && g != 0x1B;
    }
    if (t == 0xD || t == 0xE || t == 0xF || t == 0x10) {
        return g != 0x1B && g != 0x15;
    }
    return 1;
}

int32_t event_range_count(game_state *st, uint32_t p)
{
    /* @bin 0x0044C960 (thiscall on a range) the number of values it can take */
    if (S16(st, p + 2) >= 0) {
        if (S16(st, p + 4) < 0) {
            return 1;
        }
        return (S16(st, p + 6) >= 0) + 2;
    }
    if (S16(st, p + 4) >= 0 && S16(st, p + 6) >= 0) {
        return S16(st, p + 6) - S16(st, p + 4) + 1;
    }
    return 0;
}

int32_t events_started_later(game_state *st, uint32_t self, int32_t n)
{
    /* @bin 0x0044C430 (thiscall on the list) record n is at or after the one being stepped
     * (0x005D9A40) and of the walk's kind (type 2 with 0x0078A634 set, other types without) */
    int32_t cur = S32(st, G_U5D9A40);
    if (cur < 0 || cur > n) {
        return 0;
    }
    if (n < 0 || n >= S32(st, self)) {
        return 0;
    }
    uint32_t r = self + 4 + EVENT_REC * (uint32_t) n;
    if (U8(st, r + 2) == 2) {
        return U8(st, G_U78A634) != 0;
    }
    return U8(st, G_U78A634) == 0;
}

void event_init(game_state *st, uint32_t e)
{
    /* @bin 0x004489C0 (thiscall) a new record's defaults (then 0x00448A60: +0x3A..+0x4A) */
    S16(st, e) = -1;
    U8(st, e + 2) = 0;
    U8(st, e + 3) = 0;
    S16(st, e + 0x24) = -1;
    S16(st, e + 0x26) = -1;
    S16(st, e + 0x52) = -1;
    S16(st, e + 0x54) = -1;
    U32(st, e + 0x28) = 2;
    S16(st, e + 0x18) = 1;
    S16(st, e + 0x1A) = 2;
    S16(st, e + 0x16) = -1;
    S16(st, e + 0x10) = 5;
    S16(st, e + 0x12) = 10;
    S16(st, e + 0xE) = -1;
    S16(st, e + 0x5C) = 0;
    S16(st, e + 0x5E) = 0x31;
    S16(st, e + 0x5A) = -1;
    S16(st, e + 6) = 1;
    S16(st, e + 8) = -1;
    S16(st, e + 0xA) = -1;
    S16(st, e + 0x20) = 1;
    S16(st, e + 0x22) = 8;
    S16(st, e + 0x1E) = -1;
    S16(st, e + 0x2C) = 2;
    S16(st, e + 0x2E) = 0;
    S16(st, e + 0x30) = 0;
    U8(st, e + 0x32) = 0;
    U8(st, e + 0x33) = 0;
    S16(st, e + 0x34) = -1;
    U8(st, e + 0x36) = 0;
    S16(st, e + 0x38) = 0;
    static const uint8_t zero[] = {0x3A, 0x3C, 0x3E, 0x40, 0x42, 0x44, 0x46, 0x4A};
    for (unsigned k = 0; k < sizeof(zero); k++) {
        S16(st, e + zero[k]) = 0;
    }
}

static int event_in_use(game_state *st, int32_t i)
{
    /* @bin 0x00448D69 a formation (0x90 bytes, +0x42 the first byte read) in use (1), not +0x42+0x42 /
     * +0x69, for event i (+0x78); or a figure 1..1999 alive (+0xE) of type 0x5C for event i
     * (+0x186) */
    for (uint32_t p = G_FORMATION_BASE + 0x90u + 0x42u; p < G_FORMATION_BASE + 0x90u * 50 + 0x42u; p += 0x90) {
        if (U8(st, p - 0x42) == 1 && U8(st, p) == 0 && U8(st, p + 0x27) == 0 && S16(st, p + 0x36) == i) {
            return 1;
        }
    }
    for (uint32_t p = FIGURE_ADDR(1) + FIG_TYPE; p < FIGURE_ADDR(FIGURE_COUNT) + FIG_TYPE; p += FIGURE_SIZE) {
        if (U8(st, p - FIG_TYPE + FIG_STATE) != 0 && U8(st, p) == 0x5C && S32(st, p - FIG_TYPE + FIG_U17C) == i) {
            return 1;
        }
    }
    return 0;
}

uint32_t events_alloc(game_state *st, uint32_t self, int8_t quiet)
{
    /* @bin 0x00448D10 (thiscall on the list) a record to reuse (+0x28 bits 3 and 2, +0x32 clear, its
     * date passed; for type 2 none of its formations or figures left), else a new one at the end
     * (at most 0x8C); none: 0 (and the error log unless `quiet`). With `quiet`, 0 above 0x77 */
    if (quiet != 0 && S32(st, self) > 0x77) {
        return 0;
    }
    for (int32_t i = 0; i < S32(st, self); i++) {
        uint32_t r = self + 4 + EVENT_REC * (uint32_t) i;
        uint32_t f = U32(st, r + 0x28);
        if (!(f & 8) || !((f >> 2) & 1) || U8(st, r + 0x32) != 0 || event_due(st, r, 1)) {
            continue;
        }
        if (U8(st, r + 2) != 2 || !event_in_use(st, i)) {
            return r;
        }
    }
    int32_t n = S32(st, self);
    if (n > 0x8B) {
        if (quiet == 0) {
            GAME_CALL(st, FN_PLATFORM_LOG, (int32_t) G_U5DA174, 0, 0); /* "ERR Could not add event" */
        }
        return 0;
    }
    n++;
    S32(st, self) = n;
    uint32_t r = self + 4 + EVENT_REC * (uint32_t) (n - 1);
    event_init(st, r);
    S16(st, r) = (int16_t) (S16(st, self) - 1);
    return r;
}

static void range_take(game_state *st, uint32_t p, int16_t v)
{
    /* @bin 0x0044C4FA the range's value := v when v is one it can take: fixed (+2 >= 0) with a
     * single value, or within +4..+6 */
    int16_t lo, hi;
    if (S16(st, p + 2) < 0) {
        lo = S16(st, p + 4);
        hi = S16(st, p + 6);
    } else if (event_range_count(st, p) == 1) {
        lo = hi = S16(st, p + 2);
    } else {
        lo = hi = -1;
    }
    if (v >= lo && v <= hi) {
        S16(st, p) = v;
    }
}

void event_start(game_state *st, uint32_t e, int32_t n, int32_t kind)
{
    /* @bin 0x0044C490 (thiscall) a copy of record n (a reused or new record, 0x00448D10) is reset
     * (0x00448A90), flagged 8, takes e's city (+0x1C) and good (+4) when they fit, records e as its
     * starter (+0x34, +0x63, +0x66, +0x68, +0x37) and `kind` (+0x64); its delay +0x14 (months)
     * becomes a date; with no delay it runs now unless the walk reaches it later */
    uint32_t src = event_at(st, n);
    if (src == 0) {
        return;
    }
    uint32_t r = events_alloc(st, G_EVENTS, 0);
    if (r == 0) {
        return;
    }
    int16_t idx = S16(st, r);
    for (uint32_t k = 0; k < EVENT_REC; k += 4) {
        U32(st, r + k) = U32(st, src + k);
    }
    S16(st, r) = idx;
    event_reset(st, r);
    U32(st, r + 0x28) = 8;
    if (event_type_linkable(st, r) == event_type_linkable(st, e)) {
        range_take(st, r + 0x1C, S16(st, e + 0x1C));
    }
    if (event_type_has_good(st, r) && event_type_has_good(st, e) && event_good_allowed(st, r, S16(st, e + 4))) {
        range_take(st, r + 4, S16(st, e + 4));
    }
    S16(st, r + 0x64) = (int16_t) kind;
    S16(st, r + 0x34) = S16(st, e);
    U8(st, r + 0x63) = event_type_linkable(st, e) ? U8(st, e + 0x1C) : 0;
    S16(st, r + 0x68) = S16(st, e + 0xC);
    S16(st, r + 0x66) = S16(st, e + 4);
    U8(st, r + 0x37) = U8(st, e + 0x36);
    /* @bin 0x0044C5E0 month := (month + delay) % 12 (idiv), years := delay / 12, + 1 when the new
     * month (its low byte) is before the current one */
    int32_t d = S16(st, r + 0x14);
    int32_t m = (S32(st, G_MONTH) + d) % 12;
    int32_t q = d / 12;
    U8(st, r + 3) = (uint8_t) m;
    if ((int32_t) (uint8_t) m < S32(st, G_MONTH)) {
        q++;
    }
    S16(st, r + 0x14) = (int16_t) (U32(st, G_YEAR) - U32(st, G_START_YEAR) + (uint32_t) q);
    if (d != 0) {
        return;
    }
    if (events_started_later(st, G_EVENTS, S16(st, r))) {
        return;
    }
    if (U8(st, r + 2) == 2) {
        event_request_month(st, r);
    } else {
        event_month_step(st, r);
    }
}

static void put16(uint8_t *b, uint32_t off, int32_t v)
{
    b[off] = (uint8_t) v;
    b[off + 1] = (uint8_t) ((uint32_t) v >> 8);
}

void event_request_month(game_state *st, uint32_t e)
{
    /* @bin 0x00451540 (thiscall) a request / invasion (type 2, kind +4 = 1..4) while pending
     * (0x0044D700): with its starter flagged (0x0044EF60) and city +0x42D8 >= 15, message 0x78 and
     * stop; when its months run out (0x0044D750 = 0) the troops come (0x00446A30), message 0x85,
     * the next event starts; else reminders at 24, 12, 6 and 1 months left (bits of +0x57) and
     * every month when it is due (0x0044D680) */
    if ((U32(st, e + 0x28) >> 2) & 1) {
        return;
    }
    uint8_t blk[32] = {0};
    int16_t kind4 = S16(st, e + 4);
    switch (kind4) {
    case 1: put16(blk, 2, 0xB6); put16(blk, 0x10, 0xBE); break;
    case 2: put16(blk, 2, 0xAC); put16(blk, 0x10, 0xB4 + (event_linked_flag(st, e) != 0)); break;
    case 3: put16(blk, 2, 0xA2); put16(blk, 0x10, 0xAA + (event_linked_flag(st, e) != 0)); break;
    case 4: put16(blk, 2, 0xFF); put16(blk, 0x10, 0x107); break;
    default: break;
    }
    event_fill_block(st, e, blk);
    if (!event_pending(st, e)) {
        return;
    }
    if (event_linked_flag(st, e) && S32(st, CURRENT_CITY(st) + CITY_RATING_KINGDOM) >= 0xF) {
        S32(st, G_MESSAGE_POPUP) = 1;
        message_post(st, 0x78, 0, 0, NULL, 0);
        GAME_CALLT0(st, e, FN_EVENT_STOP);
        return;
    }
    int32_t left = event_months_left(st, e);
    if (left == 0) {
        int sent = 0;
        int32_t r = 0;
        /* @bin 0x0045164D 0x00446A30(kind, +0xC, +0x1C, +0x38, index): kind 0x007848FC, 12 or 13 */
        int32_t a1 = S16(st, e + 0xC), a2 = S16(st, e + 0x1C), a3 = S16(st, e + 0x38), a4 = S16(st, e);
        switch (kind4) {
        case 1:
            r = GAME_CALL(st, FN_EVENT_TROOPS, S16(st, G_ENEMY), a1, a2, a3, a4);
            if (r != 0) {
                S32(st, G_MESSAGE_POPUP) = 1;
                sent = 1;
                event_outcome_message(st, e, 0);
                put16(blk, 0, 0xBC);
            }
            break;
        case 2:
        case 3:
        case 4:
            r = GAME_CALL(st, FN_EVENT_TROOPS, kind4 == 4 ? 0xD : 0xC, a1, a2, a3, a4);
            if (r != 0) {
                S32(st, G_MESSAGE_POPUP) = 1;
                sent = 1;
                put16(blk, 0, kind4 == 2 ? 0xB2 : kind4 == 3 ? 0xA8 : 0x105);
            }
            break;
        default:
            r = 0;
            break;
        }
        U32(st, e + 0x28) |= 4;
        if (sent) {
            S32(st, G_MESSAGE_POPUP) = 1;
            message_post(st, 0x85, S16(st, G_INVASION_SEQUENCE), r, blk, 0);
        }
        int32_t v = event_outcome_message(st, e, 0);
        event_start(st, e, S16(st, e + 0x24), v);
        return;
    }
    /* @bin 0x0045179D the reminder by the months left (table 0x004519E8): its bit, its block word
     * +0 and its message, by the kind */
    uint8_t bit;
    int32_t msg;
    static const int16_t w_by[5][4] = {
        {0xBB, 0xB1, 0xA7, 0x104}, /* 1 month */
        {0xBA, 0xB0, 0xA6, 0x103}, /* 6 */
        {0xB9, 0xAF, 0xA5, 0x102}, /* 12 */
        {0xB8, 0xAE, 0xA4, 0x101}, /* 24 */
        {0xB7, 0xAD, 0xA3, 0x100}, /* other, when due */
    };
    static const int16_t m_by[5][4] = {
        {0x71, 0x7F, 0x7C, 0x71},
        {0x71, 0x7F, 0x7C, 0x71},
        {0x76, 0x80, 0x7D, 0x76},
        {0x77, 0x81, 0x7E, 0x77},
        {0x77, 0x81, 0x7E, 0x77},
    };
    int row;
    switch (left) {
    case 1: bit = 0x10; row = 0; break;
    case 6: bit = 8; row = 1; break;
    case 12: bit = 4; row = 2; break;
    case 24: bit = 2; row = 3; break;
    default:
        if (!event_due(st, e, 1) || left <= 1) {
            return;
        }
        bit = 1;
        row = 4;
        break;
    }
    if (kind4 < 1 || kind4 > 4) {
        /* the block's words +0 and +2 (+0x10 too) stay the original's leftover stack bytes */
        state_unported(st, FN_EVENT_REQUEST_MONTH);
        return;
    }
    put16(blk, 0, w_by[row][kind4 - 1]);
    msg = m_by[row][kind4 - 1];
    uint8_t sentbits = U8(st, e + 0x57);
    if (bit & sentbits) {
        return;
    }
    U8(st, e + 0x57) = sentbits | bit;
    S32(st, G_MESSAGE_POPUP) = event_due(st, e, 1) != 0;
    message_post(st, msg, 0, 0, blk, 0);
}

/* the message family of each outcome slot (0x0044F120 / 0x0044F470: the row and column of the
 * scenario's table 0x007854A8; 0x0044F8C0, 0x0044FB20, 0x0044FC00, 0x0044FCE0: the message kind) */

static int32_t outcome_row0(game_state *st, uint32_t e)
{
    /* @bin 0x0044F170 (thiscall) */
    uint8_t s = U8(st, e + 0x60);
    switch (U8(st, e + 2)) {
    case 1: {
        static const int8_t t[7] = {8, 0, 4, 0xB, 0xE, 0x11, 0x14};
        return s < 7 ? t[s] : -1;
    }
    case 2:
        switch (S16(st, e + 4)) {
        case 1: return 0x20;
        case 2: return 0x21;
        case 3: return 0x22;
        case 4: return 0x32;
        default: return -1;
        }
    case 3: return 0x2B;
    case 6: return 0x2C;
    case 7: return 0x2D;
    case 8: return 0x29;
    case 9: return 0x2A;
    case 10: return 0x2E;
    case 11: case 12: return 0x2F;
    case 13: return 0x25;
    case 14: return 0x26;
    case 15: return 0x27;
    case 16: return 0x28;
    case 17: return 0x23;
    case 18: return 0x24;
    case 19: return s < 5 ? 0x17 + s : -1;
    case 20: return s < 4 ? 0x1C + s : -1;
    case 21: return 0x30;
    case 22: return 0x31;
    case 23: return 0x35;
    default: return -1;
    }
}

static int32_t outcome_row(game_state *st, uint32_t e, int32_t slot)
{
    /* @bin 0x0044F120 (thiscall) by the slot: 0x0044F170, 0x0044F360, 0x0044F3D0, 0x0044F440 */
    uint8_t t = U8(st, e + 2), s = U8(st, e + 0x60);
    switch (slot) {
    case 0:
        return outcome_row0(st, e);
    case 1: {
        static const int8_t a[7] = {0xA, 3, 7, 0xD, 0x10, 0x13, 0x16};
        return t == 1 ? (s < 7 ? a[s] : -1) : t == 0x17 ? 0x33 : -1;
    }
    case 2: {
        static const int8_t a[7] = {9, 2, 6, 0xC, 0xF, 0x12, 0x15};
        return t == 1 ? (s < 7 ? a[s] : -1) : t == 0x17 ? 0x34 : -1;
    }
    case 3:
        return t == 1 && s == 1 ? 1 : t == 1 && s == 2 ? 5 : -1;
    default:
        return -1;
    }
}

static int32_t outcome_column(game_state *st, uint32_t e)
{
    /* @bin 0x0044F470 (thiscall) */
    uint8_t s = U8(st, e + 0x60);
    switch (U8(st, e + 2)) {
    case 1: {
        static const int8_t t[7] = {2, 0, 1, 3, 4, 5, 6};
        return s < 7 ? t[s] : -1;
    }
    case 2:
        switch (S16(st, e + 4)) {
        case 1: return 0x10;
        case 2: return 0x11;
        case 3: return 0x12;
        case 4: return 0x22;
        default: return -1;
        }
    case 3: return 0x1B;
    case 6: return 0x1C;
    case 7: return 0x1D;
    case 8: return 0x19;
    case 9: return 0x1A;
    case 10: return 0x1E;
    case 11: case 12: return 0x1F;
    case 13: return 0x15;
    case 14: return 0x16;
    case 15: return 0x17;
    case 16: return 0x18;
    case 17: return 0x13;
    case 18: return 0x14;
    case 19: return s < 5 ? 7 + s : -1;
    case 20: return s < 4 ? 0xC + s : -1;
    case 21: return 0x20;
    case 22: return 0x21;
    case 23: return 0x23;
    default: return -1;
    }
}

static int32_t outcome_kind0(game_state *st, uint32_t e)
{
    /* @bin 0x0044F8C0 (thiscall) */
    uint8_t s = U8(st, e + 0x60);
    int on = U8(st, e + 0x56) != 0;
    switch (U8(st, e + 2)) {
    case 1: {
        static const int16_t a[7] = {0x2E, 8, 0x1A, 0x40, 0x52, 0x64, 0x76};
        static const int16_t b[7] = {0x2F, 9, 0x1B, 0x41, 0x53, 0x65, 0x77};
        return s < 7 ? (on ? a[s] : b[s]) : -1;
    }
    case 2:
        switch (S16(st, e + 4)) {
        case 1: return 0xBD;
        case 2: return 0xB3;
        case 3: return 0xA9;
        case 4: return 0x106;
        default: return -1;
        }
    case 3: return 0xD9;
    case 6: return 0xDD;
    case 7: return 0xE1;
    case 8: return 0xE5;
    case 9: return 0xE9;
    case 10: return 0xED;
    case 11: return 0xF1;
    case 13: return 0xD1;
    case 14: return 0xD5;
    case 15: return 0xC9;
    case 16: return 0xCD;
    case 17: return 0xC1;
    case 18: return 0xC5;
    case 19: return s < 5 ? 0x80 + 4 * s : -1;
    case 20: return s < 4 ? 0x94 + 4 * s : -1;
    case 21: return 0xF9;
    case 22: return 0xFD;
    case 23: return 0x11F - on;
    default: return -1;
    }
}

static int32_t outcome_kind(game_state *st, uint32_t e, int32_t slot)
{
    /* @bin 0x0044FE31 by the slot (table 0x0044FEB0): 0x0044F8C0, 0x0044FB20, 0x0044FC00, 0x0044FCE0 */
    uint8_t t = U8(st, e + 2), s = U8(st, e + 0x60);
    int on = U8(st, e + 0x56) != 0;
    switch (slot) {
    case 0:
        return outcome_kind0(st, e);
    case 1: {
        static const int16_t a[7] = {0x32, 0xC, 0x1E, 0x44, 0x56, 0x68, 0x7A};
        static const int16_t b[7] = {0x33, 0xD, 0x1F, 0x45, 0x57, 0x69, 0x7B};
        return t == 1 ? (s < 7 ? (on ? a[s] : b[s]) : -1) : t == 0x17 ? 0x123 - on : -1;
    }
    case 2: {
        static const int16_t a[7] = {0x30, 0xA, 0x1C, 0x42, 0x54, 0x66, 0x78};
        static const int16_t b[7] = {0x31, 0xB, 0x1D, 0x43, 0x55, 0x67, 0x79};
        return t == 1 ? (s < 7 ? (on ? a[s] : b[s]) : -1) : t == 0x17 ? 0x121 - on : -1;
    }
    default:
        if (t != 1 || (s != 1 && s != 2)) {
            return -1;
        }
        return on ? (s == 1 ? 0xE : 0x20) : (s == 1 ? 0xF : 0x21);
    }
}

/* @bin 0x0044FEC0 (cases by the table 0x00451418, handlers by 0x0045121C, read out of the code):
 * the message of kind k at level 0 is k itself; levels 1 and 2 are these */
static const int16_t msg_levels[][3] = {
    {0x008, 0x12A, 0x12B},
    {0x009, 0x12C, 0x12D},
    {0x00A, 0x12E, 0x12F},
    {0x00B, 0x130, 0x131},
    {0x00C, 0x132, 0x133},
    {0x00D, 0x134, 0x135},
    {0x00E, 0x136, 0x137},
    {0x00F, 0x138, 0x139},
    {0x010, 0x13A, 0x13B},
    {0x011, 0x13C, 0x13D},
    {0x01A, 0x13E, 0x13F},
    {0x01B, 0x140, 0x141},
    {0x01C, 0x142, 0x143},
    {0x01D, 0x144, 0x145},
    {0x01E, 0x146, 0x147},
    {0x01F, 0x148, 0x149},
    {0x020, 0x14A, 0x14B},
    {0x021, 0x14C, 0x14D},
    {0x022, 0x14E, 0x14F},
    {0x023, 0x150, 0x151},
    {0x02E, 0x152, 0x153},
    {0x02F, 0x154, 0x155},
    {0x030, 0x156, 0x157},
    {0x031, 0x158, 0x159},
    {0x032, 0x15A, 0x15B},
    {0x033, 0x15C, 0x15D},
    {0x034, 0x15E, 0x15F},
    {0x035, 0x160, 0x161},
    {0x040, 0x162, 0x163},
    {0x041, 0x164, 0x165},
    {0x042, 0x166, 0x167},
    {0x043, 0x168, 0x169},
    {0x044, 0x16A, 0x16B},
    {0x045, 0x16C, 0x16D},
    {0x046, 0x16E, 0x16F},
    {0x047, 0x170, 0x171},
    {0x052, 0x172, 0x173},
    {0x053, 0x174, 0x175},
    {0x054, 0x176, 0x177},
    {0x055, 0x178, 0x179},
    {0x056, 0x17A, 0x17B},
    {0x057, 0x17C, 0x17D},
    {0x058, 0x17E, 0x17F},
    {0x059, 0x180, 0x181},
    {0x064, 0x182, 0x183},
    {0x065, 0x184, 0x185},
    {0x066, 0x186, 0x187},
    {0x067, 0x188, 0x189},
    {0x068, 0x18A, 0x18B},
    {0x069, 0x18C, 0x18D},
    {0x06A, 0x18E, 0x18F},
    {0x06B, 0x190, 0x191},
    {0x076, 0x192, 0x193},
    {0x077, 0x194, 0x195},
    {0x078, 0x196, 0x197},
    {0x079, 0x198, 0x199},
    {0x07A, 0x19A, 0x19B},
    {0x07B, 0x19C, 0x19D},
    {0x07C, 0x19E, 0x19F},
    {0x07D, 0x1A0, 0x1A1},
    {0x080, 0x1A2, 0x1A3},
    {0x081, 0x1A4, 0x1A5},
    {0x084, 0x1A6, 0x1A7},
    {0x085, 0x1A8, 0x1A9},
    {0x088, 0x1AA, 0x1AB},
    {0x089, 0x1AC, 0x1AD},
    {0x08C, 0x1AE, 0x1AF},
    {0x08D, 0x1B0, 0x1B1},
    {0x090, 0x1B2, 0x1B3},
    {0x091, 0x1B4, 0x1B5},
    {0x094, 0x1B6, 0x1B7},
    {0x095, 0x1B8, 0x1B9},
    {0x098, 0x1BA, 0x1BB},
    {0x099, 0x1BC, 0x1BD},
    {0x09C, 0x1BE, 0x1BF},
    {0x09D, 0x1C0, 0x1C1},
    {0x0A0, 0x1C2, 0x1C3},
    {0x0A1, 0x1C4, 0x1C5},
    {0x0A9, 0x1C6, 0x1C7},
    {0x0AA, 0x1C8, 0x1C9},
    {0x0B3, 0x1CA, 0x1CB},
    {0x0B4, 0x1CC, 0x1CD},
    {0x0BD, 0x1CE, 0x1CF},
    {0x0BE, 0x1D0, 0x1D1},
    {0x0C1, 0x1D2, 0x1D3},
    {0x0C2, 0x1D4, 0x1D5},
    {0x0C5, 0x1D6, 0x1D7},
    {0x0C6, 0x1D8, 0x1D9},
    {0x0C9, 0x1DA, 0x1DB},
    {0x0CA, 0x1DC, 0x1DD},
    {0x0CD, 0x1DE, 0x1DF},
    {0x0CE, 0x1E0, 0x1E1},
    {0x0D1, 0x1E2, 0x1E3},
    {0x0D2, 0x1E4, 0x1E5},
    {0x0D5, 0x1E6, 0x1E7},
    {0x0D6, 0x1E8, 0x1E9},
    {0x0D9, 0x1EA, 0x1EB},
    {0x0DA, 0x1EC, 0x1ED},
    {0x0DD, 0x1EE, 0x1EF},
    {0x0DE, 0x1F0, 0x1F1},
    {0x0E1, 0x1F2, 0x1F3},
    {0x0E2, 0x1F4, 0x1F5},
    {0x0E5, 0x1F6, 0x1F7},
    {0x0E6, 0x1F8, 0x1F9},
    {0x0E9, 0x1FA, 0x1FB},
    {0x0EA, 0x1FC, 0x1FD},
    {0x0ED, 0x1FE, 0x1FF},
    {0x0EE, 0x200, 0x201},
    {0x0F1, 0x202, 0x203},
    {0x0F2, 0x204, 0x205},
    {0x0F5, 0x206, 0x207},
    {0x0F6, 0x208, 0x209},
    {0x0F9, 0x20A, 0x20B},
    {0x0FA, 0x20C, 0x20D},
    {0x0FD, 0x20E, 0x20F},
    {0x0FE, 0x210, 0x211},
    {0x106, 0x212, 0x213},
    {0x107, 0x214, 0x215},
    {0x11E, 0x216, 0x217},
    {0x11F, 0x218, 0x219},
    {0x120, 0x21A, 0x21B},
    {0x121, 0x21C, 0x21D},
    {0x122, 0x21E, 0x21F},
    {0x123, 0x220, 0x221},
    {0x124, 0x222, 0x223},
    {0x125, 0x224, 0x225},
};

int32_t event_message_id(int32_t kind, int32_t level)
{
    /* @bin 0x0044FEC0 (stdcall) level % 3 (idiv: negative levels match nothing) */
    int32_t l = level % 3;
    if ((uint32_t) (kind - 8) > 0x11Du) {
        return -1;
    }
    for (unsigned k = 0; k < sizeof(msg_levels) / sizeof(msg_levels[0]); k++) {
        if (msg_levels[k][0] == kind) {
            return l == 0 ? kind : l == 1 ? msg_levels[k][1] : l == 2 ? msg_levels[k][2] : -1;
        }
    }
    return -1;
}

int32_t event_outcome_message(game_state *st, uint32_t e, int32_t slot)
{
    /* @bin 0x0044FD30 (thiscall) the outcome's linked event n (+0x24, +0x26, +0x52, +0x54) and
     * level (+0x6A + slot). Level 6 or more: the level from the scenario table 0x007854A8 (54 rows
     * of 36) at this event's row and n's column (level 0 when outside). Levels 3..5 with n: n's kind
     * (0x0044F660); levels 0..5: this event's kind for the slot; then 0x0044FEC0(kind, level) */
    static const uint8_t link[4] = {0x24, 0x26, 0x52, 0x54};
    if ((uint32_t) slot > 3) {
        return -1;
    }
    int32_t n = S16(st, e + link[slot]);
    int32_t lvl = U8(st, e + 0x6A + (uint32_t) slot);
    if (lvl >= 6) {
        uint32_t o = event_at(st, n);
        if (o == 0) {
            return -1;
        }
        int32_t row = outcome_row(st, e, slot);
        int32_t col = outcome_column(st, o);
        if (row < 0 || row >= 0x36 || col < 0 || col >= 0x24) {
            lvl = 0;
            return event_message_id(outcome_kind(st, e, slot), lvl);
        }
        lvl = U8(st, G_U7854A8 + 0x24u * (uint32_t) row + (uint32_t) col);
    }
    if (lvl >= 3 && lvl <= 5) {
        uint32_t o = event_at(st, n);
        if (o != 0) {
            return event_message_id(event_message_kind(st, o), lvl);
        }
    }
    if (lvl >= 6) {
        return -1;
    }
    return event_message_id(outcome_kind(st, e, slot), lvl);
}

/* the run wrappers: thiscall functions on a record take it from args[PORT_THIS_ARG] */
#define THIS_REC ((uint32_t) a[PORT_THIS_ARG])

static int32_t run_before(game_state *st, const int32_t *a) { return event_before_date(st, THIS_REC); }
static int32_t run_pending(game_state *st, const int32_t *a) { return event_pending(st, THIS_REC); }
static int32_t run_linked(game_state *st, const int32_t *a) { return event_linked_flag(st, THIS_REC); }
static int32_t run_linkable(game_state *st, const int32_t *a) { return event_type_linkable(st, THIS_REC); }
static int32_t run_has_good(game_state *st, const int32_t *a) { return event_type_has_good(st, THIS_REC); }
static int32_t run_good_ok1(game_state *st, const int32_t *a) { return event_good_ok_type1(st, THIS_REC, a[0]); }
static int32_t run_good_ok(game_state *st, const int32_t *a) { return event_good_allowed(st, THIS_REC, a[0]); }
static int32_t run_range_count(game_state *st, const int32_t *a) { return event_range_count(st, THIS_REC); }
static int32_t run_later(game_state *st, const int32_t *a) { return events_started_later(st, THIS_REC, a[0]); }
static int32_t run_init(game_state *st, const int32_t *a) { event_init(st, THIS_REC); return 0; }
static int32_t run_alloc(game_state *st, const int32_t *a) { return (int32_t) events_alloc(st, THIS_REC, (int8_t) a[0]); }
static int32_t run_start(game_state *st, const int32_t *a) { event_start(st, THIS_REC, a[0], a[1]); return 0; }
static int32_t run_request(game_state *st, const int32_t *a) { event_request_month(st, THIS_REC); return 0; }
static int32_t run_outcome(game_state *st, const int32_t *a) { return event_outcome_message(st, THIS_REC, a[0]); }
static int32_t run_row(game_state *st, const int32_t *a) { return outcome_row(st, THIS_REC, a[0]); }
static int32_t run_row0(game_state *st, const int32_t *a) { return outcome_row0(st, THIS_REC); }
static int32_t run_column(game_state *st, const int32_t *a) { return outcome_column(st, THIS_REC); }
static int32_t run_kind0(game_state *st, const int32_t *a) { return outcome_kind0(st, THIS_REC); }
static int32_t run_kind1(game_state *st, const int32_t *a) { return outcome_kind(st, THIS_REC, 1); }
static int32_t run_kind2(game_state *st, const int32_t *a) { return outcome_kind(st, THIS_REC, 2); }
static int32_t run_kind3(game_state *st, const int32_t *a) { return outcome_kind(st, THIS_REC, 3); }
static int32_t run_row1(game_state *st, const int32_t *a) { return outcome_row(st, THIS_REC, 1); }
static int32_t run_row2(game_state *st, const int32_t *a) { return outcome_row(st, THIS_REC, 2); }
static int32_t run_row3(game_state *st, const int32_t *a) { return outcome_row(st, THIS_REC, 3); }
static int32_t run_msg_id(game_state *st, const int32_t *a) { (void) st; return event_message_id(a[0], a[1]); }

const ported_function month2_events_ports[] = {
    {0x0044D6D0u, "event_before_date", 0, 8, 0, run_before, 0},
    {0x0044D700u, "event_pending", 0, 8, 0, run_pending, 0},
    {0x0044EF60u, "event_linked_flag", 0, 8, 0, run_linked, 0},
    {0x00449390u, "event_type_linkable", 0, 8, 0, run_linkable, 0},
    {0x004493C0u, "event_type_has_good", 0, 8, 0, run_has_good, 0},
    {0x00449370u, "event type table 0x005D9C18 (in events.c)", 0, 8, 0, NULL, 0},
    {0x00449380u, "event type table 0x005D9C38 (in events.c)", 0, 8, 0, NULL, 0},
    {0x004493D0u, "event type table 0x005D9C98 (in events.c)", 0, 32, 0, NULL, 0},
    {0x004493E0u, "event type table 0x005D9D10 (in events.c)", 0, 32, 0, NULL, 0},
    {0x00449440u, "event_good_ok_type1", 1, 8, 0, run_good_ok1, 0},
    {0x00449530u, "event_good_ok_type23 (in events.c)", 1, 8, 0, NULL, 0},
    {0x00449550u, "event_good_allowed", 1, 8, 0, run_good_ok, 0},
    {0x0044C960u, "event_range_count", 0, 32, 0, run_range_count, 0},
    {0x0044C430u, "events_started_later", 1, 8, 0, run_later, 0},
    {0x004489C0u, "event_init", 0, 0, 0, run_init, 0},
    {0x00448D10u, "events_alloc", 1, 32, 0, run_alloc, 0},
    {0x0044C490u, "event_start", 2, 0, 0, run_start, 0},
    {0x00451540u, "event_request_month", 0, 0, 0, run_request, 0},
    {0x0044FD30u, "event_outcome_message", 1, 32, 0, run_outcome, 0},
    {0x0044F120u, "event_outcome_row", 1, 32, 0, run_row, 0},
    {0x0044F170u, "event_outcome_row0", 0, 32, 0, run_row0, 0},
    {0x0044F360u, "event_outcome_row1", 0, 32, 0, run_row1, 0},
    {0x0044F3D0u, "event_outcome_row2", 0, 32, 0, run_row2, 0},
    {0x0044F440u, "event_outcome_row3", 0, 32, 0, run_row3, 0},
    {0x0044F470u, "event_outcome_column", 0, 32, 0, run_column, 0},
    {0x0044F8C0u, "event_outcome_kind0", 0, 32, 0, run_kind0, 0},
    {0x0044FB20u, "event_outcome_kind1", 0, 32, 0, run_kind1, 0},
    {0x0044FC00u, "event_outcome_kind2", 0, 32, 0, run_kind2, 0},
    {0x0044FCE0u, "event_outcome_kind3", 0, 32, 0, run_kind3, 0},
    {0x0044FEC0u, "event_message_id", 2, 32, 0, run_msg_id, 0},
};
const int month2_events_ports_count = sizeof(month2_events_ports) / sizeof(month2_events_ports[0]);
