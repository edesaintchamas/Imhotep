/* Imhotep - trade commands: opening a route, import / export orders, limits, stockpiling, industry.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * docs/commands.md section 3.4. The resource of the trade window is 0x0060598C. */
#include "command/orders.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define TRADE_RESOURCE 0x0060598Cu
#define EMPIRE_SELECTED 0x00784F18u   /* i32 empire city of the empire window */
#define EMPIRE_CITIES 0x0077DE00u     /* empire city records, 0x6A bytes */
#define EMPIRE_CITY_SIZE 0x6Au
#define EMPIRE_CITY_END 0x0077F742u   /* @bin 0x00443A67 the scan stops before this record (61 cities) */
#define TRADE_STATUS 0x2826u          /* city +: i16 per resource: 0 none, 1, 2 (?), 3 import, 4 export */
#define TRADE_LIMIT 0x286Eu           /* city +: i16 per resource */
#define TRADE_INDUSTRY 0x28B6u        /* city +: i16 per resource */
#define TRADE_STOCKPILE 0x2A88u       /* city +: i32 per resource */
#define DEFAULT_LIMIT 0x00775810u     /* i32 per resource, hundredths */
#define INDUSTRY_COUNT 0x0078D6DCu    /* i32 per resource */
#define TEXT_CURSOR_1 0x00E20768u
#define TEXT_CURSOR_2 0x00E20764u
#define TEXT_BUFFER 0x00E36C70u
#define TEXT_GROUPS 0x00E34C38u

static uint32_t status_of(game_state *st, int32_t res)
{
    return CURRENT_CITY(st) + TRADE_STATUS + 2u * (uint32_t) res;
}

void text_seek2(game_state *st, int32_t group, int32_t index)
{
    /* @bin 0x004CC160 the twin of 0x004CC110 (text_seek) on the second cursor 0x00E20764 */
    uint32_t p = U32(st, TEXT_GROUPS + 8u * (uint32_t) group) + U32(st, TEXT_BUFFER);
    U32(st, TEXT_CURSOR_2) = p;
    while (index > 0) {
        if (P8(st, p) == 0) {
            uint8_t prev = P8(st, p - 1);
            if (prev >= 0x20 || prev == 0) {
                index--;
            }
        }
        p++;
        U32(st, TEXT_CURSOR_2) = p;
    }
    while (P8(st, p) < 0x20) {
        p++;
        U32(st, TEXT_CURSOR_2) = p;
    }
}

/* @bin 0x00559A72 a refusal: title and text of group 5, the modal message box 0x00538CD0 (a
 * platform call: a window loop that leaves only UI globals) */
static void refuse(game_state *st, int32_t title, int32_t text)
{
    GAME_CALL(st, 0x004CC110u, 5, title);
    text_seek2(st, 5, text);
    GAME_CALL(st, 0x00538CD0u, (int32_t) U32(st, TEXT_CURSOR_1), (int32_t) U32(st, TEXT_CURSOR_2), 0);
}

/* @bin 0x00559AF0 limit := the resource's default 0x00775810[res] / 100 */
static void default_limit(game_state *st)
{
    int32_t res = S32(st, TRADE_RESOURCE);
    S16(st, CURRENT_CITY(st) + TRADE_LIMIT + 2u * (uint32_t) res) =
        (int16_t) (S32(st, DEFAULT_LIMIT + 4u * (uint32_t) res) / 100);
}

int32_t empire_buyer_open(game_state *st, int32_t res)
{
    /* @bin 0x00443A20 an empire city in use (+0x00) of kind 1, 3 or 5 (+0x02) that buys the
     * resource (+0x06 + res = 1) and trades it (0x00447790) */
    int32_t i = 0;
    for (uint32_t c = EMPIRE_CITIES; c < EMPIRE_CITY_END; c += EMPIRE_CITY_SIZE, i++) {
        if (U8(st, c) == 0) {
            continue;
        }
        uint8_t kind = U8(st, c + 2);
        if ((kind == 1 || kind == 3 || kind == 5) && U8(st, c + 6 + (uint32_t) res) == 1 &&
            (uint8_t) GAME_CALL(st, 0x00447790u, i, res) != 0) {
            return 1;
        }
    }
    return 0;
}

void build_buttons_update(game_state *st)
{
    /* @bin 0x004051A0 the 12 rows of the two sidebar button tables (0x005C7388, 0x005C75C8; 0x24-byte
     * entries, +4 the build menu): enabled (+0 := 1) unless the menu has no building available
     * (0x00404120 on the menu 0x007056EC <= 0) */
    for (uint32_t off = 0; off < 0x1B0; off += 0x24) {
        U8(st, 0x005C7388u + off) = 1;
        U32(st, 0x007056ECu) = U32(st, 0x005C738Cu + off);
        if (GAME_CALL0(st, 0x00404120u) <= 0) {
            U8(st, 0x005C7388u + off) = 0;
        }
        U8(st, 0x005C75C8u + off) = 1;
        U32(st, 0x007056ECu) = U32(st, 0x005C75CCu + off);
        if (GAME_CALL0(st, 0x00404120u) <= 0) {
            U8(st, 0x005C75C8u + off) = 0;
        }
    }
}

void cmd_trade_open_route(game_state *st)
{
    /* @bin 0x004022D0 confirmed: pay the route's cost (i16 +0x4E), route open (+0x05 := 1) */
    if (U8(st, CMD_ANSWER) != 1) {
        return;
    }
    uint32_t city = EMPIRE_CITIES + EMPIRE_CITY_SIZE * U32(st, EMPIRE_SELECTED);
    GAME_CALL(st, 0x004E9680u, S32(st, CURRENT_PLAYER_ADDR), S16(st, city + 0x4E));
    U8(st, EMPIRE_CITIES + EMPIRE_CITY_SIZE * U32(st, EMPIRE_SELECTED) + 0x05) = 1;
    /* @bin 0x00402316 the build menus' availability (newly tradable goods) */
    GAME_CALL0(st, 0x00404350u);
    build_buttons_update(st);
    S32(st, 0x0070570Cu) = 0x26; /* the window the popup returns to */
    U8(st, UI_REDRAW) = 1;
}

void cmd_trade_import(game_state *st)
{
    /* @bin 0x00559A60 refused (message box) when no open route sells it (0x004439C0) or ... */
    if (GAME_CALL(st, 0x004439C0u, S32(st, TRADE_RESOURCE)) == 0) {
        refuse(st, 0x70, 0x71);
        return;
    }
    if (GAME_CALL(st, 0x00443950u, S32(st, TRADE_RESOURCE)) == 0) {
        refuse(st, 0x74, 0x76);
        return;
    }
    /* @bin 0x00559AE3 jump table 0x00559B34: 1 -> 0, 3 -> 1 (+ 0x00443330, default limit),
     * 0 / 2 / 4 -> 3 */
    uint32_t f = status_of(st, S32(st, TRADE_RESOURCE));
    switch (S16(st, f)) {
    case 1:
        S16(st, f) = 0;
        break;
    case 3:
        S16(st, f) = 1;
        GAME_CALL0(st, 0x00443330u);
        default_limit(st);
        break;
    case 0:
    case 2:
    case 4:
        S16(st, f) = 3;
        break;
    default:
        break;
    }
}

void cmd_trade_export(game_state *st)
{
    /* @bin 0x00559B50 refused when no open route buys it (0x00443A20) or 0x00443B20 */
    if (empire_buyer_open(st, S32(st, TRADE_RESOURCE)) == 0) {
        refuse(st, 0x72, 0x73);
        return;
    }
    if (GAME_CALL(st, 0x00443B20u, S32(st, TRADE_RESOURCE)) == 0) {
        refuse(st, 0x74, 0x75);
        return;
    }
    /* @bin 0x00559BD7 jump table 0x00559C40: 2 -> 0, 4 -> 2 (+ 0x00443330, default limit),
     * 0 / 1 / 3 -> 4 with the stockpile flag cleared */
    int32_t res = S32(st, TRADE_RESOURCE);
    uint32_t f = status_of(st, res);
    switch (S16(st, f)) {
    case 2:
        S16(st, f) = 0;
        break;
    case 4:
        S16(st, f) = 2;
        GAME_CALL0(st, 0x00443330u);
        default_limit(st);
        break;
    case 0:
    case 1:
    case 3:
        S32(st, CURRENT_CITY(st) + TRADE_STOCKPILE + 4u * (uint32_t) res) = 0;
        S16(st, f) = 4;
        break;
    default:
        break;
    }
}

void cmd_trade_status(game_state *st)
{
    /* @bin 0x005598F0 status 0 -> 1 -> 2 -> 0: 1 needs 0x00443950 (else 2), 2 needs 0x00443B20
     * (else 0); reaching 2 clears the stockpile flag */
    uint32_t f = status_of(st, S32(st, TRADE_RESOURCE));
    S16(st, f) = (int16_t) (S16(st, f) + 1);
    if (S16(st, f) >= 3) {
        S16(st, f) = 0;
    }
    if (S16(st, f) == 1 && GAME_CALL(st, 0x00443950u, S32(st, TRADE_RESOURCE)) == 0) {
        S16(st, status_of(st, S32(st, TRADE_RESOURCE))) = 2;
    }
    if (S16(st, status_of(st, S32(st, TRADE_RESOURCE))) == 2) {
        if (GAME_CALL(st, 0x00443B20u, S32(st, TRADE_RESOURCE)) == 0) {
            S16(st, status_of(st, S32(st, TRADE_RESOURCE))) = 0;
        }
        int32_t res = S32(st, TRADE_RESOURCE);
        S32(st, CURRENT_CITY(st) + TRADE_STOCKPILE + 4u * (uint32_t) res) = 0;
    }
}

void cmd_trade_limit(game_state *st)
{
    /* @bin 0x00559CD0 p1 1: -1, 0: +1; then capped by 0x00559C60 on 100 * limit */
    int32_t res = S32(st, TRADE_RESOURCE);
    uint32_t f = CURRENT_CITY(st) + TRADE_LIMIT + 2u * (uint32_t) res;
    int32_t p1 = S32(st, CMD_PARAM1);
    if (p1 == 1) {
        S16(st, f) = (int16_t) (S16(st, f) - 1);
    } else if (p1 == 0) {
        S16(st, f) = (int16_t) (S16(st, f) + 1);
    }
    int32_t v = GAME_CALL(st, 0x00559C60u, res, S16(st, f) * 100);
    S16(st, CURRENT_CITY(st) + TRADE_LIMIT + 2u * (uint32_t) S32(st, TRADE_RESOURCE)) = (int16_t) (v / 100);
}

void cmd_trade_stockpile(game_state *st)
{
    /* @bin 0x00559D70 toggled; when set, an export status 2 or 4 becomes 0 */
    int32_t res = S32(st, TRADE_RESOURCE);
    uint32_t f = CURRENT_CITY(st) + TRADE_STOCKPILE + 4u * (uint32_t) res;
    if (S32(st, f) == 0) {
        S32(st, f) = 1;
        uint32_t s = status_of(st, res);
        if (S16(st, s) == 2 || S16(st, s) == 4) {
            S16(st, s) = 0;
        }
    } else {
        S32(st, f) = 0;
    }
    U8(st, UI_REDRAW) = 1;
}

void cmd_trade_industry(game_state *st)
{
    /* @bin 0x005598A0 toggled when the city has such industries (0x0078D6DC[res], res 9 counts as
     * 1) */
    int32_t res = S32(st, TRADE_RESOURCE);
    int32_t k = res == 9 ? 1 : res;
    if (S32(st, INDUSTRY_COUNT + 4u * (uint32_t) k) <= 0) {
        return;
    }
    uint32_t f = CURRENT_CITY(st) + TRADE_INDUSTRY + 2u * (uint32_t) res;
    S16(st, f) = S16(st, f) == 0;
}

#define V(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
V(run_open_route, cmd_trade_open_route)
V(run_build_buttons, build_buttons_update)
V(run_import, cmd_trade_import)
V(run_export, cmd_trade_export)
V(run_status, cmd_trade_status)
V(run_limit, cmd_trade_limit)
V(run_stockpile, cmd_trade_stockpile)
V(run_industry, cmd_trade_industry)

static int32_t run_buyer_open(game_state *st, const int32_t *a) { return empire_buyer_open(st, a[0]); }

static int32_t run_text_seek2(game_state *st, const int32_t *a)
{
    text_seek2(st, a[0], a[1]);
    return 0;
}

const ported_function trade_ports[] = {
    {0x004022D0u, "cmd_trade_open_route", 0, 0, 0, run_open_route, 0},
    {0x00559A60u, "cmd_trade_import", 0, 0, 0, run_import, 0},
    {0x00559B50u, "cmd_trade_export", 0, 0, 0, run_export, 0},
    {0x005598F0u, "cmd_trade_status", 0, 0, 0, run_status, 0},
    {0x00559CD0u, "cmd_trade_limit", 0, 0, 0, run_limit, 0},
    {0x00559D70u, "cmd_trade_stockpile", 0, 0, 0, run_stockpile, 0},
    {0x005598A0u, "cmd_trade_industry", 0, 0, 0, run_industry, 0},
    {0x00443A20u, "empire_buyer_open", 1, 32, 0, run_buyer_open, 0},
    {0x004CC160u, "text_seek2", 2, 0, 0, run_text_seek2, 0},
    {0x004051A0u, "build_buttons_update", 0, 0, 0, run_build_buttons, 0},
};
const int trade_ports_count = sizeof(trade_ports) / sizeof(trade_ports[0]);
