/* Imhotep - trade commands: opening a route, import / export orders, limits, stockpiling, industry.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * docs/commands.md section 3.4. The resource of the trade window is 0x0060598C. */
#include "command/orders.h"

#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define EMPIRE_CITY_SIZE 0x6Au

static uint32_t status_of(game_state *st, int32_t res)
{
    return CURRENT_CITY(st) + CITY_TRADE_STATUS + 2u * (uint32_t) res;
}

void text_seek2(game_state *st, int32_t group, int32_t index)
{
    /* @bin 0x004CC160 the twin of 0x004CC110 (text_seek) on the second cursor 0x00E20764 */
    uint32_t p = U32(st, G_TEXT_GROUPS + 8u * (uint32_t) group) + U32(st, G_TEXT_BUFFER);
    U32(st, G_TEXT_PTR2) = p;
    while (index > 0) {
        if (P8(st, p) == 0) {
            uint8_t prev = P8(st, p - 1);
            if (prev >= 0x20 || prev == 0) {
                index--;
            }
        }
        p++;
        U32(st, G_TEXT_PTR2) = p;
    }
    while (P8(st, p) < 0x20) {
        p++;
        U32(st, G_TEXT_PTR2) = p;
    }
}

/* @bin 0x00559A72 a refusal: title and text of group 5, the modal message box 0x00538CD0 (a
 * platform call: a window loop that leaves only UI globals) */
static void refuse(game_state *st, int32_t title, int32_t text)
{
    GAME_CALL(st, FN_TEXT_SEEK, 5, title);
    text_seek2(st, 5, text);
    GAME_CALL(st, FN_PLATFORM_MESSAGE_BOX, (int32_t) U32(st, G_TEXT_CURSOR), (int32_t) U32(st, G_TEXT_PTR2), 0);
}

/* @bin 0x00559AF0 limit := the resource's default 0x00775810[res] / 100 */
static void default_limit(game_state *st)
{
    int32_t res = S32(st, G_TRADE_RESOURCE);
    S16(st, CURRENT_CITY(st) + CITY_TRADE_LIMIT + 2u * (uint32_t) res) =
        (int16_t) (S32(st, G_DEFAULT_LIMIT + 4u * (uint32_t) res) / 100);
}

int32_t empire_buyer_open(game_state *st, int32_t res)
{
    /* @bin 0x00443A20 an empire city in use (+0x00) of kind 1, 3 or 5 (+0x02) that buys the
     * resource (+0x06 + res = 1) and trades it (0x00447790) */
    int32_t i = 0;
    for (uint32_t c = G_EMPIRE_CITIES; c < G_EMPIRE_CITY_END; c += EMPIRE_CITY_SIZE, i++) {
        if (U8(st, c) == 0) {
            continue;
        }
        uint8_t kind = U8(st, c + 2);
        if ((kind == 1 || kind == 3 || kind == 5) && U8(st, c + 6 + (uint32_t) res) == 1 &&
            (uint8_t) GAME_CALL(st, FN_EMPIRE_CITY_TRADES, i, res) != 0) {
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
        U8(st, G_BUILD_BUTTONS_A + off) = 1;
        U32(st, G_SUBMENU) = U32(st, G_BUILD_BUTTONS_A + 4u + off);
        if (GAME_CALL0(st, FN_BUILD_MENU_COUNT) <= 0) {
            U8(st, G_BUILD_BUTTONS_A + off) = 0;
        }
        U8(st, G_BUILD_BUTTONS_B + off) = 1;
        U32(st, G_SUBMENU) = U32(st, G_BUILD_BUTTONS_B + 4u + off);
        if (GAME_CALL0(st, FN_BUILD_MENU_COUNT) <= 0) {
            U8(st, G_BUILD_BUTTONS_B + off) = 0;
        }
    }
}

void cmd_trade_open_route(game_state *st)
{
    /* @bin 0x004022D0 confirmed: pay the route's cost (i16 +0x4E), route open (+0x05 := 1) */
    if (U8(st, G_CONFIRM_ANSWER) != 1) {
        return;
    }
    uint32_t city = G_EMPIRE_CITIES + EMPIRE_CITY_SIZE * U32(st, G_EMPIRE_SELECTED);
    GAME_CALL(st, FN_PLACE_SPEND, S32(st, CURRENT_PLAYER_ADDR), S16(st, city + 0x4E));
    U8(st, G_EMPIRE_CITIES + EMPIRE_CITY_SIZE * U32(st, G_EMPIRE_SELECTED) + 0x05) = 1;
    /* @bin 0x00402316 the build menus' availability (newly tradable goods) */
    GAME_CALL0(st, FN_BUILD_MENU_UPDATE);
    build_buttons_update(st);
    S32(st, G_POPUP_BACK) = 0x26; /* the window the popup returns to */
    U8(st, G_REDRAW) = 1;
}

void cmd_trade_import(game_state *st)
{
    /* @bin 0x00559A60 refused (message box) when no open route sells it (0x004439C0) or ... */
    if (GAME_CALL(st, FN_GOODS_TRADE_CITY, S32(st, G_TRADE_RESOURCE)) == 0) {
        refuse(st, 0x70, 0x71);
        return;
    }
    if (GAME_CALL(st, FN_GOODS_TRADE_CITY_FLAG5, S32(st, G_TRADE_RESOURCE)) == 0) {
        refuse(st, 0x74, 0x76);
        return;
    }
    /* @bin 0x00559AE3 jump table 0x00559B34: 1 -> 0, 3 -> 1 (+ 0x00443330, default limit),
     * 0 / 2 / 4 -> 3 */
    uint32_t f = status_of(st, S32(st, G_TRADE_RESOURCE));
    switch (S16(st, f)) {
    case 1:
        S16(st, f) = 0;
        break;
    case 3:
        S16(st, f) = 1;
        GAME_CALL0(st, FN_DAY_STEP_0X02);
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
    if (empire_buyer_open(st, S32(st, G_TRADE_RESOURCE)) == 0) {
        refuse(st, 0x72, 0x73);
        return;
    }
    if (GAME_CALL(st, FN_EMPIRE_OPEN_SELLER, S32(st, G_TRADE_RESOURCE)) == 0) {
        refuse(st, 0x74, 0x75);
        return;
    }
    /* @bin 0x00559BD7 jump table 0x00559C40: 2 -> 0, 4 -> 2 (+ 0x00443330, default limit),
     * 0 / 1 / 3 -> 4 with the stockpile flag cleared */
    int32_t res = S32(st, G_TRADE_RESOURCE);
    uint32_t f = status_of(st, res);
    switch (S16(st, f)) {
    case 2:
        S16(st, f) = 0;
        break;
    case 4:
        S16(st, f) = 2;
        GAME_CALL0(st, FN_DAY_STEP_0X02);
        default_limit(st);
        break;
    case 0:
    case 1:
    case 3:
        S32(st, CURRENT_CITY(st) + CITY_STOCKPILE + 4u * (uint32_t) res) = 0;
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
    uint32_t f = status_of(st, S32(st, G_TRADE_RESOURCE));
    S16(st, f) = (int16_t) (S16(st, f) + 1);
    if (S16(st, f) >= 3) {
        S16(st, f) = 0;
    }
    if (S16(st, f) == 1 && GAME_CALL(st, FN_GOODS_TRADE_CITY_FLAG5, S32(st, G_TRADE_RESOURCE)) == 0) {
        S16(st, status_of(st, S32(st, G_TRADE_RESOURCE))) = 2;
    }
    if (S16(st, status_of(st, S32(st, G_TRADE_RESOURCE))) == 2) {
        if (GAME_CALL(st, FN_EMPIRE_OPEN_SELLER, S32(st, G_TRADE_RESOURCE)) == 0) {
            S16(st, status_of(st, S32(st, G_TRADE_RESOURCE))) = 0;
        }
        int32_t res = S32(st, G_TRADE_RESOURCE);
        S32(st, CURRENT_CITY(st) + CITY_STOCKPILE + 4u * (uint32_t) res) = 0;
    }
}

void cmd_trade_limit(game_state *st)
{
    /* @bin 0x00559CD0 p1 1: -1, 0: +1; then capped by 0x00559C60 on 100 * limit */
    int32_t res = S32(st, G_TRADE_RESOURCE);
    uint32_t f = CURRENT_CITY(st) + CITY_TRADE_LIMIT + 2u * (uint32_t) res;
    int32_t p1 = S32(st, G_PARAM1);
    if (p1 == 1) {
        S16(st, f) = (int16_t) (S16(st, f) - 1);
    } else if (p1 == 0) {
        S16(st, f) = (int16_t) (S16(st, f) + 1);
    }
    int32_t v = GAME_CALL(st, FN_VALUE_CAP, res, S16(st, f) * 100);
    S16(st, CURRENT_CITY(st) + CITY_TRADE_LIMIT + 2u * (uint32_t) S32(st, G_TRADE_RESOURCE)) = (int16_t) (v / 100);
}

void cmd_trade_stockpile(game_state *st)
{
    /* @bin 0x00559D70 toggled; when set, an export status 2 or 4 becomes 0 */
    int32_t res = S32(st, G_TRADE_RESOURCE);
    uint32_t f = CURRENT_CITY(st) + CITY_STOCKPILE + 4u * (uint32_t) res;
    if (S32(st, f) == 0) {
        S32(st, f) = 1;
        uint32_t s = status_of(st, res);
        if (S16(st, s) == 2 || S16(st, s) == 4) {
            S16(st, s) = 0;
        }
    } else {
        S32(st, f) = 0;
    }
    U8(st, G_REDRAW) = 1;
}

void cmd_trade_industry(game_state *st)
{
    /* @bin 0x005598A0 toggled when the city has such industries (0x0078D6DC[res], res 9 counts as
     * 1) */
    int32_t res = S32(st, G_TRADE_RESOURCE);
    int32_t k = res == 9 ? 1 : res;
    if (S32(st, G_U78D6DC + 4u * (uint32_t) k) <= 0) {
        return;
    }
    uint32_t f = CURRENT_CITY(st) + CITY_INDUSTRY_OFF + 2u * (uint32_t) res;
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
