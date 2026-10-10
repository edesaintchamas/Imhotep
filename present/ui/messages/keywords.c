/* Imhotep - message texts: the "[keyword]" handlers (roadmap phase 8.2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The keyword table 0x005D2EC8 ({index, name, handler} records of 0x0C bytes, stored in 0x00784F34
 * by the static constructor 0x004482D0) names the fields a message text may hold: "[amount]",
 * "[city_name]", "[god]", "[quantity_needed_remainder_main]" ... text_expand (0x004CB980,
 * expand.c) calls the handler of each field and copies the string it returns. The handlers read the
 * current message (0x00E951E4, records of 0x30 bytes at 0x00E951E0), the building the message is
 * about (0x00EADD6C) and the monument's course globals (0x00EADB30 ...). They write their number
 * into a buffer of their own (sprintf) or return a string of the game's text (0x004CC110, the text
 * cursor 0x00E20768 restored). */
#include "messages.h"

#include "city/text_lookup.h"
#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"


/* the message's field at `addr` (the address for message 0): addr + 0x30 * index */
static uint32_t msg_field(game_state *st, uint32_t addr)
{
    return addr + 0x30u * U32(st, G_MESSAGE_CURRENT);
}

/* @bin 0x00447806 .. (every handler that returns a text string) string `index` of group `group`,
 * the text cursor restored */
static uint32_t text_string(game_state *st, int32_t group, int32_t index)
{
    uint32_t saved = U32(st, TEXT_CURSOR);
    text_seek(st, group, index);
    uint32_t s = U32(st, TEXT_CURSOR);
    U32(st, TEXT_CURSOR) = saved;
    return s;
}

static uint32_t print_d(game_state *st, uint32_t buf, int32_t v)
{
    GAME_CALL(st, FN_CRT_SPRINTF, (int32_t) buf, (int32_t) G_FMT_D, v);
    return buf;
}

static uint32_t print_d_s(game_state *st, uint32_t buf, int32_t v, uint32_t s)
{
    GAME_CALL(st, FN_CRT_SPRINTF, (int32_t) buf, (int32_t) G_FMT_D_S, v, (int32_t) s);
    return buf;
}

/* @bin 0x00447945 (and 0x004479CA) kinds 0x18 .. 0x1A, 0x1E, 0x23 (jump table 0x00447990 / byte
 * table 0x00447998), 0x0A, 0x1C, 0x24 count as they are; others in hundreds */
static int32_t kind_amount(int32_t kind, int32_t v)
{
    switch (kind) {
    case 0x18:
    case 0x19:
    case 0x1A:
    case 0x1E:
    case 0x23:
    case 0x1C:
    case 0x0A:
    case 0x24:
        return v;
    default:
        return (int32_t) ((uint32_t) v * 100u);
    }
}

/* @bin 0x00447D8B (and the other course handlers) loads of the course: x400 for kinds 0x0C and 0x1D
 * (`both`), 0x0C only otherwise, else x4 */
static int32_t course_amount(int32_t kind, int32_t v, int both)
{
    if (kind == 0x0C || (both && kind == 0x1D)) {
        return (int32_t) ((uint32_t) v * 400u);
    }
    return (int32_t) ((uint32_t) v * 4u);
}

/* @bin 0x00510F90 the name of a course resource (group 0xB2), 0 for other kinds */
uint32_t keyword_course_resource(game_state *st, int32_t kind)
{
    int32_t index;
    switch (kind) {
    case 0x0C:
        index = 0x5E;
        break;
    case 0x18:
        index = 0x5A;
        break;
    case 0x19:
        index = 0x5B;
        break;
    case 0x1A:
        index = 0x5C;
        break;
    case 0x1D:
        index = 0x60;
        break;
    case 0x1E:
        index = 0x5D;
        break;
    case 0x23:
        index = 0x5F;
        break;
    default:
        return 0;
    }
    return text_string(st, 0xB2, index);
}

/* @bin 0x00510DC0 the name of a monument (group 0xB2) from one of its buildings: a monument of the
 * mission's list (the player city's three ids +0xEA5FAC, their entries 0x00784DE0 into the table
 * 0x005D19C8 of 0x10-byte records: +0 1 for a pyramid named by its size 0x005E4EF4, else +2 its
 * kind), or by the building's type when its +6 is -1. 0 when the building is not in the list. */
uint32_t keyword_monument_name(game_state *st, int32_t building)
{
    int32_t b = GAME_CALL(st, FN_BUILDING_MAIN_PART, building); /* the monument's main part */
    uint32_t rec = BUILDING_BASE + BLD_TYPE + (uint32_t) b * 0x108u;
    int32_t index = 0;
    if (S16(st, rec + 6) == -1) {
        /* @bin 0x00510DE4 jump table 0x00510F14 / bytes 0x00510F34 on type - 0xDE */
        switch (S16(st, rec)) {
        case 0xDE:
            index = 0x59;
            break;
        case 0xE1:
            index = 0x51;
            break;
        case 0xE4:
            index = 0x58;
            break;
        case 0xE5:
            index = 0x53;
            break;
        case 0xEA:
            index = 0x54;
            break;
        case 0xEB:
            index = 0x55;
            break;
        case 0xEC:
            index = 0x56;
            break;
        default:
            index = 0; /* @bin 0x00510DF3 the other types: string 0 */
            break;
        }
        return text_string(st, 0xB2, index);
    }
    uint32_t ids = CITY_BASE + CITY_MONUMENT_ID + (uint32_t) S32(st, G_PLAYER) * 0x49D8u;
    int32_t k = 0;
    while (S32(st, ids + 4u * (uint32_t) k) != b) {
        k++;
        if (k >= 3) {
            return 0;
        }
    }
    int32_t e = S16(st, G_MONUMENT_SLOTS + 2u * (uint32_t) k);
    if (e < 0) {
        return 0;
    }
    uint32_t def = G_MONUMENT_MODELS + (uint32_t) e * 0x10u;
    if (S16(st, def) == 1) {
        /* @bin 0x00510EB4 jump table 0x00510F7C on the size - 1 */
        switch (S32(st, G_PYRAMID_KIND)) {
        case 1:
            index = 0x4C;
            break;
        case 2:
            index = 0x4D;
            break;
        case 3:
            index = 0x4B;
            break;
        case 4:
            index = 0x4A;
            break;
        case 5:
            index = 0x49;
            break;
        default:
            return 0;
        }
    } else {
        /* @bin 0x00510E50 jump table 0x00510F44 on the kind - 1 */
        switch (S16(st, def + 2)) {
        case 1:
            index = 0x4F;
            break;
        case 2:
            index = 0x4E;
            break;
        case 3:
            index = 0x50;
            break;
        case 4:
            index = 0x59;
            break;
        case 5:
            index = 0x58;
            break;
        case 6:
            index = 0x51;
            break;
        case 7:
            index = 0x57;
            break;
        case 10:
            index = 0x53;
            break;
        case 11:
            index = 0x54;
            break;
        case 12:
            index = 0x55;
            break;
        case 13:
            index = 0x56;
            break;
        case 14:
            index = 0x52;
            break;
        default:
            return 0;
        }
    }
    return text_string(st, 0xB2, index);
}

/* @bin 0x00447FD0 "load" or "loads" (group 8, string 0x36 for 1, else 0x37) */
static uint32_t loads_word(game_state *st, int32_t n)
{
    return text_string(st, 8, (n != 1) + 0x36);
}

/* the building type of the message's building (i16 +0 of its 0x108-byte record) */
static uint16_t msg_building_type(game_state *st)
{
    return U16(st, BUILDING_BASE + BLD_TYPE + U32(st, G_INFO_BUILDING) * 0x108u);
}

static int32_t div_s(int32_t a, int32_t b)
{
    return a / b; /* @bin 0x51EB851F multiply-shift: signed division, truncated */
}

static int32_t run_greeting(game_state *st, const int32_t *a)
{
    (void) a;
    /* @bin 0x00447800 group 0x20, string [city +0xEA5CBC] + 0xB */
    uint32_t city = (uint32_t) S32(st, G_PLAYER) * 0x49D8u;
    return (int32_t) text_string(st, 0x20, (int32_t) (U32(st, CITY_BASE + CITY_RANK + city) + 0xBu));
}

static int32_t run_city_name(game_state *st, const int32_t *a)
{
    (void) a;
    /* @bin 0x00447890 the message's city (+0x5E, the reason phrase's +0x6A): the byte 0x0077DE03 + 0x6A x city,
     * group 0x15 in [0x00E39088] mode, else 0xC3 */
    int32_t c = U8(st, G_MSG_NESTED) ? S16(st, msg_field(st, MESSAGE_BASE + MSG_PHRASE_CITY)) : S16(st, msg_field(st, MESSAGE_BASE + MSG_CITY));
    int32_t name = U8(st, G_EMPIRE_CITY_NAME + (uint32_t) c * 0x6Au);
    return (int32_t) text_string(st, U8(st, G_EMPIRE_LANGUAGE) ? 0x15 : 0xC3, name);
}

static int32_t run_amount(game_state *st, const int32_t *a)
{
    (void) a;
    /* @bin 0x00447910 the amount (+0x60) of the kind (+0x62); the reason phrase's +0x70 / +0x72 */
    int nested = U8(st, G_MSG_NESTED) != 0;
    int32_t v = S16(st, msg_field(st, nested ? MESSAGE_BASE + MSG_PHRASE_AMOUNT : MESSAGE_BASE + MSG_AMOUNT));
    int32_t kind = S16(st, msg_field(st, nested ? MESSAGE_BASE + MSG_PHRASE_KIND : MESSAGE_BASE + MSG_KIND));
    return (int32_t) print_d(st, G_BUF_AMOUNT, kind_amount(kind, v));
}

static int32_t run_amount_granted(game_state *st, const int32_t *a)
{
    (void) a;
    /* @bin 0x004479B0 the amount granted (+0x6C) of the kind (+0x62) */
    int32_t v = S16(st, msg_field(st, MESSAGE_BASE + MSG_AMOUNT_GRANTED));
    int32_t kind = S16(st, msg_field(st, MESSAGE_BASE + MSG_KIND));
    return (int32_t) print_d(st, G_BUF_AMOUNT_GRANTED, kind_amount(kind, v));
}

static int32_t run_item(game_state *st, const int32_t *a)
{
    (void) a;
    /* @bin 0x00447A30 the kind (+0x62; the reason phrase's +0x72) + 0x36 of group 0x17 */
    int32_t kind = U8(st, G_MSG_NESTED) ? S16(st, msg_field(st, MESSAGE_BASE + MSG_PHRASE_KIND)) : S16(st, msg_field(st, MESSAGE_BASE + MSG_KIND));
    return (int32_t) text_string(st, 0x17, kind + 0x36);
}

static int32_t run_time_allotted(game_state *st, const int32_t *a)
{
    (void) a;
    return (int32_t) print_d(st, G_BUF_TIME_ALLOTTED, S16(st, msg_field(st, MESSAGE_BASE + MSG_TIME))); /* @bin 0x00447A80 */
}

static int32_t run_travel_time(game_state *st, const int32_t *a)
{
    (void) a;
    return (int32_t) print_d(st, G_BUF_TRAVEL_TIME, S16(st, msg_field(st, MESSAGE_BASE + MSG_TIME))); /* @bin 0x00447AB0 */
}

static int32_t run_time_until_attack(game_state *st, const int32_t *a)
{
    (void) a;
    return (int32_t) print_d(st, G_BUF_TIME_UNTIL_ATTACK, S16(st, msg_field(st, MESSAGE_BASE + MSG_ATTACK_TIME))); /* @bin 0x00447AE0 */
}

static int32_t run_reason_phrase(game_state *st, const int32_t *a)
{
    (void) a;
    /* @bin 0x00447B10 the phrase (+0x68) expanded into 0x00785334 (200 bytes), its fields read from
     * the phrase's own slots while 0x007854A4 is 1 */
    int32_t phrase = S16(st, msg_field(st, MESSAGE_BASE + MSG_PHRASE));
    U8(st, G_MSG_NESTED) = 1;
    GAME_CALL(st, FN_MESSAGE_TEXT_EXPAND, phrase, G_BUF_REASON_PHRASE, 200);
    U8(st, G_MSG_NESTED) = 0;
    return G_BUF_REASON_PHRASE;
}

static int32_t run_pharaoh_name(game_state *st, const int32_t *a)
{
    (void) a;
    return (int32_t) text_string(st, 0x97, S32(st, G_PHARAOH_NAME)); /* @bin 0x00447B50 */
}

static int32_t run_foreign_army(game_state *st, const int32_t *a)
{
    (void) a;
    return (int32_t) text_string(st, 0x25, S16(st, G_SCENARIO_ENEMY) + 0x1C); /* @bin 0x00447B80 */
}

static int32_t run_god(game_state *st, const int32_t *a)
{
    (void) a;
    /* @bin 0x00447BB0 the god (byte +0x74; the reason phrase's +0x75), 0 when above 4, group 0x9D */
    uint8_t g = U8(st, G_MSG_NESTED) ? U8(st, msg_field(st, MESSAGE_BASE + MSG_PHRASE_GOD)) : U8(st, msg_field(st, MESSAGE_BASE + MSG_GOD));
    if (g >= 5) {
        g = 0;
    }
    return (int32_t) text_string(st, 0x9D, g);
}

static int32_t run_monument_name(game_state *st, const int32_t *a)
{
    (void) a;
    return (int32_t) keyword_monument_name(st, S32(st, G_INFO_BUILDING)); /* @bin 0x00447C20 */
}

static int32_t run_percent_complete(game_state *st, const int32_t *a)
{
    (void) a;
    /* @bin 0x00447C30 by the building's type (jump table 0x00447CFC, bytes 0x00447D28 on type -
     * 0xB7), clamped to 0 .. 100, "%d%%" into 0x0078532C; 0 for other types */
    uint32_t id = U32(st, G_INFO_BUILDING);
    int32_t v;
    switch (S16(st, BUILDING_BASE + BLD_TYPE + id * 0x108u)) {
    case 0xB7:
        v = S32(st, G_PYRAMID_PERCENT);
        break;
    case 0xCF:
    case 0xD0:
        v = U8(st, BUILDING_BASE + BLD_U55 + id * 0x108u);
        break;
    case 0xD2: {
        int32_t p = S16(st, CITY_BASE + CITY_U433A + (uint32_t) S32(st, G_PLAYER) * 0x49D8u);
        v = (int32_t) ((uint32_t) p * 100u) / 16; /* @bin 0x00447CB8 cdq; and 0xF; sar 4 */
        break;
    }
    case 0xD5:
        v = S32(st, G_MON_D5_PERCENT);
        break;
    case 0xDE:
        v = S32(st, G_MON_DE_PERCENT);
        break;
    case 0xE1:
        v = S32(st, G_MON_E1_PERCENT);
        break;
    case 0xE3:
        v = S32(st, G_MON_E3_PERCENT);
        break;
    case 0xE4:
        v = S32(st, G_MON_E4_PERCENT);
        break;
    case 0xE5:
    case 0xEA:
    case 0xEB:
    case 0xEC:
        v = S32(st, G_MON_E5_PERCENT);
        break;
    case 0xE6:
        v = S32(st, G_MON_E6_PERCENT);
        break;
    default:
        return 0;
    }
    if (v < 0) {
        v = 0;
    } else if (v > 100) {
        v = 100;
    }
    GAME_CALL(st, FN_CRT_SPRINTF, G_BUF_PERCENT_COMPLETE, (int32_t) G_FMT_PERCENT, v);
    return G_BUF_PERCENT_COMPLETE;
}

static int32_t run_courses_complete(game_state *st, const int32_t *a)
{
    (void) a;
    return (int32_t) print_d(st, G_BUF_COURSES_COMPLETE, S32(st, G_PYRAMID_COURSES_DONE)); /* @bin 0x00447D60 */
}

/* @bin 0x00447D80, 0x00447DE0, 0x00447F70, 0x00448270: a course quantity "%d %s" with the
 * resource's name; 0 when the resource has none */
static int32_t course_quantity(game_state *st, uint32_t kind_at, uint32_t value_at, int both, uint32_t buf)
{
    int32_t kind = S32(st, kind_at);
    int32_t n = course_amount(kind, S32(st, value_at), both);
    uint32_t s = keyword_course_resource(st, kind);
    if (s == 0) {
        return 0;
    }
    return (int32_t) print_d_s(st, buf, n, s);
}

static int32_t run_needed_current_main(game_state *st, const int32_t *a)
{
    (void) a;
    return course_quantity(st, G_COURSE_MAIN, G_PYRAMID_NEEDED_MAIN, 1, G_BUF_NEEDED_CURRENT_MAIN); /* @bin 0x00447D80 */
}

static int32_t run_needed_current_second(game_state *st, const int32_t *a)
{
    (void) a;
    return course_quantity(st, G_COURSE_SECOND, G_PYRAMID_NEEDED_SECOND, 0, G_BUF_NEEDED_CURRENT_SECOND); /* @bin 0x00447DE0 */
}

static int32_t run_needed_remainder_second(game_state *st, const int32_t *a)
{
    (void) a;
    return course_quantity(st, G_COURSE_SECOND, G_PYRAMID_REMAINDER_SECOND, 1, G_BUF_NEEDED_REMAINDER_SECOND); /* @bin 0x00447F70 */
}

static int32_t run_total_second(game_state *st, const int32_t *a)
{
    (void) a;
    return course_quantity(st, G_COURSE_SECOND, G_PYRAMID_TOTAL_SECOND, 0, G_BUF_TOTAL_SECOND); /* @bin 0x00448270 */
}

static int32_t run_needed_remainder_main(game_state *st, const int32_t *a)
{
    (void) a;
    /* @bin 0x00447E30 the main course's remainder by the building's type: 0xB7 the loads [0x00EADC68]
     * (x400 / x4), 0xD5 [0x00EADC3C] / 100, 0xE3 [0x00EADBBC] / 100 (8 when the resource is 0x1A and
     * it is above 800), 0xE4 [0x00EADB9C] / 100, 0xE1 by [0x00EADC04] (resource 0x1D: 3200 above 4000,
     * else - 800; others: 8 or 0 up to 4000 (800 and above: 8), above it (- 3200) / 100), 0xDE
     * [0x00EADB74] / 100; other types 0 */
    int32_t kind = S32(st, G_COURSE_MAIN);
    uint16_t type = msg_building_type(st);
    int32_t n;
    if (type == 0xB7) {
        n = course_amount(kind, S32(st, G_PYRAMID_REMAINDER_MAIN), 1);
    } else if (type == 0xD5) {
        n = div_s(S32(st, G_MON_D5_REMAINDER), 100);
    } else if (type == 0xE3) {
        int32_t v = S32(st, G_MON_E3_REMAINDER);
        n = (kind == 0x1A && v > 0x320) ? 8 : div_s(v, 100);
    } else if (type == 0xE4) {
        n = div_s(S32(st, G_MON_E4_REMAINDER), 100);
    } else if (type == 0xE1) {
        int32_t v = S32(st, G_MON_E1_REMAINDER);
        if (kind == 0x1D) {
            n = v > 0xFA0 ? 0xC80 : v - 0x320;
        } else if (v > 0xFA0) {
            n = div_s(v - 0xC80, 100);
        } else {
            n = v < 0x320 ? 0 : 8;
        }
    } else if (type == 0xDE) {
        n = div_s(S32(st, G_MON_DE_REMAINDER), 100);
    } else {
        return 0;
    }
    uint32_t s = keyword_course_resource(st, kind);
    if (s == 0) {
        return 0;
    }
    return (int32_t) print_d_s(st, G_BUF_NEEDED_REMAINDER_MAIN, n, s);
}

static int32_t run_loads_current_main(game_state *st, const int32_t *a)
{
    (void) a;
    /* @bin 0x00448000 [0x00EADC60] "%d %s" with load(s); 0 when the word is missing */
    int32_t n = S32(st, G_PYRAMID_NEEDED_MAIN);
    uint32_t s = loads_word(st, n);
    if (s == 0) {
        return 0;
    }
    return (int32_t) print_d_s(st, G_BUF_LOADS_CURRENT_MAIN, n, s);
}

static int32_t run_loads_current_second(game_state *st, const int32_t *a)
{
    (void) a;
    int32_t n = S32(st, G_PYRAMID_NEEDED_SECOND); /* @bin 0x00448040 */
    return (int32_t) print_d_s(st, G_BUF_LOADS_CURRENT_SECOND, n, loads_word(st, n));
}

static int32_t run_loads_remainder_second(game_state *st, const int32_t *a)
{
    (void) a;
    int32_t n = S32(st, G_PYRAMID_REMAINDER_SECOND); /* @bin 0x00448180 */
    return (int32_t) print_d_s(st, G_BUF_LOADS_REMAINDER_SECOND, n, loads_word(st, n));
}

static int32_t run_loads_remainder_main(game_state *st, const int32_t *a)
{
    (void) a;
    /* @bin 0x00448070 as 0x00447E30 in loads: 0xB7 [0x00EADC68], the others / 400 (0xE3 with
     * resource 0x1A above 800: 2; 0xE1: resource 0x1D 8 above 4000, else (- 800) / 400; others 2 or
     * 0 up to 4000, above it (- 3200) / 400); other types 0 */
    uint16_t type = msg_building_type(st);
    int32_t n;
    if (type == 0xB7) {
        n = S32(st, G_PYRAMID_REMAINDER_MAIN);
    } else if (type == 0xD5) {
        n = div_s(S32(st, G_MON_D5_REMAINDER), 400);
    } else if (type == 0xE4) {
        n = div_s(S32(st, G_MON_E4_REMAINDER), 400);
    } else if (type == 0xE3) {
        int32_t v = S32(st, G_MON_E3_REMAINDER);
        n = (S32(st, G_COURSE_MAIN) == 0x1A && v > 0x320) ? 2 : div_s(v, 400);
    } else if (type == 0xE1) {
        int32_t v = S32(st, G_MON_E1_REMAINDER);
        if (S32(st, G_COURSE_MAIN) == 0x1D) {
            n = v > 0xFA0 ? 8 : div_s(v - 0x320, 400);
        } else if (v > 0xFA0) {
            n = div_s(v - 0xC80, 400);
        } else {
            n = v < 0x320 ? 0 : 2;
        }
    } else if (type == 0xDE) {
        n = div_s(S32(st, G_MON_DE_REMAINDER), 400);
    } else {
        return 0;
    }
    return (int32_t) print_d_s(st, G_BUF_LOADS_REMAINDER_MAIN, n, loads_word(st, n));
}

static int32_t run_total_main(game_state *st, const int32_t *a)
{
    (void) a;
    /* @bin 0x004481B0 0xB7 [0x00EADC70] (x400 / x4), 0xD5 [0x00EADC44] / 100, 0xDE [0x00EADB7C] / 100;
     * other types 0 */
    int32_t kind = S32(st, G_COURSE_MAIN);
    uint16_t type = msg_building_type(st);
    int32_t n;
    if (type == 0xB7) {
        n = course_amount(kind, S32(st, G_PYRAMID_TOTAL_MAIN), 1);
    } else if (type == 0xD5) {
        n = div_s(S32(st, G_MON_D5_TOTAL), 100);
    } else if (type == 0xDE) {
        n = div_s(S32(st, G_MON_DE_TOTAL), 100);
    } else {
        return 0;
    }
    uint32_t s = keyword_course_resource(st, kind);
    if (s == 0) {
        return 0;
    }
    return (int32_t) print_d_s(st, G_BUF_TOTAL_MAIN, n, s);
}

static int32_t run_loads_word(game_state *st, const int32_t *a)
{
    return (int32_t) loads_word(st, a[0]);
}

static int32_t run_course_resource(game_state *st, const int32_t *a)
{
    return (int32_t) keyword_course_resource(st, a[0]);
}

static int32_t run_monument_by_building(game_state *st, const int32_t *a)
{
    return (int32_t) keyword_monument_name(st, a[0]);
}

const ported_function msg_keyword_ports[] = {
    {0x00447800u, "keyword_greeting", 0, 32, 0, run_greeting, 0},
    {0x00447890u, "keyword_city_name", 0, 32, 0, run_city_name, 0},
    {0x00447910u, "keyword_amount", 0, 32, 0, run_amount, 0},
    {0x004479B0u, "keyword_amount_granted", 0, 32, 0, run_amount_granted, 0},
    {0x00447A30u, "keyword_item", 0, 32, 0, run_item, 0},
    {0x00447A80u, "keyword_time_allotted", 0, 32, 0, run_time_allotted, 0},
    {0x00447AB0u, "keyword_travel_time", 0, 32, 0, run_travel_time, 0},
    {0x00447AE0u, "keyword_time_until_attack", 0, 32, 0, run_time_until_attack, 0},
    {0x00447B10u, "keyword_reason_phrase", 0, 32, 0, run_reason_phrase, 0},
    {0x00447B50u, "keyword_pharaoh_name", 0, 32, 0, run_pharaoh_name, 0},
    {0x00447B80u, "keyword_a_foreign_army", 0, 32, 0, run_foreign_army, 0},
    {0x00447BB0u, "keyword_god", 0, 32, 0, run_god, 0},
    {0x00447C20u, "keyword_monument_name", 0, 32, 0, run_monument_name, 0},
    {0x00447C30u, "keyword_percent_complete", 0, 32, 0, run_percent_complete, 0},
    {0x00447D60u, "keyword_number_courses_complete", 0, 32, 0, run_courses_complete, 0},
    {0x00447D80u, "keyword_quantity_needed_current_course_main", 0, 32, 0, run_needed_current_main, 0},
    {0x00447DE0u, "keyword_quantity_needed_current_course_secondary", 0, 32, 0, run_needed_current_second, 0},
    {0x00447E30u, "keyword_quantity_needed_remainder_main", 0, 32, 0, run_needed_remainder_main, 0},
    {0x00447F70u, "keyword_quantity_needed_remainder_secondary", 0, 32, 0, run_needed_remainder_second, 0},
    {0x00447FD0u, "keyword_loads_word", 1, 32, 0, run_loads_word, 0},
    {0x00448000u, "keyword_number_loads_current_course_main", 0, 32, 0, run_loads_current_main, 0},
    {0x00448040u, "keyword_number_loads_current_course_secondary", 0, 32, 0, run_loads_current_second, 0},
    {0x00448070u, "keyword_number_loads_remainder_main", 0, 32, 0, run_loads_remainder_main, 0},
    {0x00448180u, "keyword_number_loads_remainder_secondary", 0, 32, 0, run_loads_remainder_second, 0},
    {0x004481B0u, "keyword_quantity_total_main", 0, 32, 0, run_total_main, 0},
    {0x00448270u, "keyword_quantity_total_secondary", 0, 32, 0, run_total_second, 0},
    {0x00510DC0u, "keyword_monument_by_building", 1, 32, 0, run_monument_by_building, 0},
    {0x00510F90u, "keyword_course_resource", 1, 32, 0, run_course_resource, 0},
};
const int msg_keyword_ports_count = sizeof(msg_keyword_ports) / sizeof(msg_keyword_ports[0]);
