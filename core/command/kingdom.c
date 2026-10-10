/* Imhotep - kingdom commands: dispatching a request, grave goods for the royal tombs.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * docs/commands.md section 3.6. Scenario events: i32 count at 0x00785D70, records of 0x7C bytes
 * from 0x00785D74. */
#include "command/orders.h"

#include "random/random.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define EVENT_SIZE 0x7Cu
#define FORMATION_SIZE 0x90u
#define POPUP_NOTHING FN_NOTHING     /* the empty callback */

void request_dispatch(game_state *st, int32_t event)
{
    /* @bin 0x0044C1A0 status +0x30 := 3 if it was set, else 2 */
    if (event < 0 || event >= S32(st, G_EVENTS)) {
        return;
    }
    uint32_t r = G_EVENT_RECORDS + EVENT_SIZE * (uint32_t) event;
    int32_t amount = S16(st, r + 0x0C);
    S16(st, r + 0x30) = (int16_t) ((S16(st, r + 0x30) != 0) + 2);
    int32_t kind = S16(st, r + 0x04);
    if (kind == 0x25) {
        /* @bin 0x0044C300 troops: status 5, months +0x2E from 0x0044C110 (at least: 12 when <= 0) */
        S16(st, r + 0x30) = 5;
        int32_t months = GAME_CALLT0(st, r, FN_EVENT_ARMY_MONTHS);
        int32_t index = S16(st, r);
        S16(st, r + 0x2E) = (int16_t) months;
        request_send_troops(st, index);
        if (S16(st, r + 0x2E) <= 0) {
            S16(st, r + 0x2E) = 0xC;
        }
        return;
    }
    /* @bin 0x0044C200 / 0x0044C335 one step of the game RNG (the 0x004D7090 step, inline), then
     * +0x2E := 1 + (iv1 & 3) */
    random_next_state(st);
    S16(st, r + 0x2E) = (int16_t) ((U32(st, G_RANDOM_IV1) & 3) + 1);
    if (kind == 0x24) {
        /* @bin 0x0044C2CC money: treasury -= amount, +0x4758 += amount */
        uint32_t c = CURRENT_CITY(st);
        U32(st, c + CITY_TREASURY) -= (uint32_t) amount;
        U32(st, c + CITY_REQUESTS_THIS_YEAR) += (uint32_t) amount;
        return;
    }
    /* @bin 0x0044C40A goods: amount * 100 of resource `kind` from the storage yards */
    GAME_CALL(st, FN_PLACE_TAKE_GOODS, S32(st, CURRENT_PLAYER_ADDR), kind, amount * 100);
}

void cmd_request_confirmed(game_state *st)
{
    /* @bin 0x00402340 the request popup's callback: dispatch 0x006057E4 on yes */
    if (U8(st, G_CONFIRM_ANSWER) == 1) {
        request_dispatch(st, S32(st, G_REQUEST_EVENT));
    }
}

void request_send_troops(game_state *st, int32_t event)
{
    /* @bin 0x004B84B0 the companies that go to a troops request (army +0x1C of the event) */
    if (event < 0 || event >= S32(st, G_EVENTS)) {
        return;
    }
    int32_t army = S16(st, G_EVENT_RECORDS + EVENT_SIZE * (uint32_t) event + 0x1C);
    if (army < 0 || army >= 0x32) {
        return;
    }
    int ships = U8(st, G_ROUTES + ROUTE_KIND + ROUTE_SIZE * (uint32_t) army) == 2;
    /* @bin 0x004B8515 formations 1..49 in use (+0x00 = 1), with +0x42, +0x67, figures (+0x28) and
     * no request (+0x82 < 0): the ones with a ship (0x0040B5E0) for a sea army, else the others */
    for (int32_t id = 1; id < 50; id++) {
        uint32_t f = G_FORMATION_BASE + FORMATION_SIZE * (uint32_t) id;
        if (U8(st, f) != 1 || U8(st, f + 0x42) == 0 || U8(st, f + 0x67) == 0 || U8(st, f + 0x28) == 0 ||
            S16(st, f + 0x82) >= 0) {
            continue;
        }
        int has = GAME_CALL(st, FN_FORMATION_HAS_ACTION_0X12, id) != 0;
        if (ships ? !has : has) {
            continue;
        }
        S16(st, f + 0x82) = (int16_t) event;
        U8(st, f + 0x03) = 0;
        if (has) {
            int32_t fig = GAME_CALL(st, FN_FORMATION_SHIP, id);
            S32(st, G_CURRENT_FIGURE) = fig;
            if (fig != 0) {
                U8(st, FIGURE_ADDR(fig) + FIG_ACTION) = 0x11;
            }
            continue;
        }
        /* @bin 0x004B85A5 destination +0x32 / +0x34 := city +0x2780 / +0x2782; its figures in use
         * (+0x0E = 1) not in action 4 get action 0xF */
        uint32_t c = CURRENT_CITY(st);
        S16(st, f + 0x32) = S16(st, c + CITY_EXIT_X);
        S16(st, f + 0x34) = S16(st, c + CITY_EXIT_Y);
        for (uint32_t k = 0; k < U8(st, f + 0x28); k++) {
            int32_t fid = S16(st, f + 8 + 2 * k);
            if (fid <= 0) {
                continue;
            }
            uint32_t p = FIGURE_ADDR(fid);
            if (U8(st, p + FIG_STATE) == 1 && U8(st, p + FIG_ACTION) != 4) {
                U8(st, p + FIG_ACTION) = 0xF;
                GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, fid);
            }
        }
    }
    if (!ships) {
        return;
    }
    /* @bin 0x004B8636 the player's figures 1..1999 in use of type 0x4E, with +0x02 and no request
     * (+0x132 < 0): request := event, action 0x11 */
    uint32_t player = U32(st, CURRENT_PLAYER_ADDR);
    S32(st, G_CURRENT_FIGURE) = 2000;
    for (int32_t fid = 1; fid < 2000; fid++) {
        uint32_t p = FIGURE_ADDR(fid);
        if (U8(st, p + FIG_STATE) != 0 && U8(st, p + FIG_TYPE) == 0x4E && U8(st, p + FIG_OWNER) == player &&
            U8(st, p + FIG_U02) != 0 && S16(st, p + FIG_U132) < 0) {
            S16(st, p + FIG_U132) = (int16_t) event;
            U8(st, p + FIG_ACTION) = 0x11;
        }
    }
}

/* ---- grave goods ---- */

int32_t grave_goods_count(game_state *st, int32_t skip)
{
    /* @bin 0x0040CC40 resources 0..35 still asked (> 0), except `skip` */
    int32_t n = 0;
    for (int32_t i = 0; i < 36; i++) {
        if (i != skip && S32(st, G_GOODS_NEEDED + 4u * (uint32_t) i) > 0) {
            n++;
        }
    }
    return n;
}

int32_t grave_goods_row(game_state *st, int32_t row)
{
    /* @bin 0x0040CD30 the resource (1..35) of the row-th asked resource (row 0: resource 1) */
    if (grave_goods_count(st, 0) == 0) {
        return 0;
    }
    int32_t n = 0;
    for (int32_t res = 1; res < 36; res++) {
        if (S32(st, G_GOODS_NEEDED + 4u * (uint32_t) res) > 0) {
            n++;
        }
        if (n == row) {
            return res;
        }
    }
    return 0;
}

static void grave_goods_window(game_state *st)
{
    /* @bin 0x0040AE00 window 0x4C */
    U8(st, G_WINDOW) = 0x4C;
    U8(st, G_REDRAW) = 1;
}

void cmd_grave_goods_choose(game_state *st)
{
    /* @bin 0x0040CD70 row p1's resource: refused (OK popup 0x20) when nothing is left to send, or
     * (popup 0x1E) when the city's stock +0x48F0 has less than 100 (1 for 0xA, 0x18-0x1A, 0x1C, 0x1E,
     * 0x23, 0x24); else the amount window opens */
    int32_t res = grave_goods_row(st, S32(st, G_PARAM1) + 1);
    if (res == 0) {
        return;
    }
    int32_t sent = S32(st, G_GOAL_COUNTERS + 4u * (uint32_t) res);
    int32_t left = S32(st, G_GOODS_NEEDED + 4u * (uint32_t) res) - sent;
    int32_t min = 100;
    if ((res >= 0x18 && res <= 0x1A) || res == 0x1E || res == 0x23 || res == 0x1C || res == 0x0A || res == 0x24) {
        min = 1;
    }
    if (left <= 0) {
        GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 0x20, (int32_t) POPUP_NOTHING, 0);
        return;
    }
    uint32_t c = CURRENT_CITY(st);
    if (S32(st, c + CITY_STORAGE_STOCK + 4u * (uint32_t) res) < min) {
        GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 0x1E, (int32_t) POPUP_NOTHING, 1);
        return;
    }
    S32(st, c + CITY_GRAVE_GOODS_RESOURCE) = res;
    S32(st, c + CITY_GRAVE_GOODS_SENT) = sent;
    S32(st, c + CITY_GRAVE_GOODS_AMOUNT) = 0;
    grave_goods_window(st);
    U8(st, G_REDRAW) = 1;
}

/* @bin 0x0040B3B0 the most that can be sent: what is left to send, at most the stock / 100 */
static int32_t grave_goods_most(game_state *st, uint32_t c, int32_t *left)
{
    int32_t res = S32(st, c + CITY_GRAVE_GOODS_RESOURCE);
    *left = S32(st, G_GOODS_NEEDED + 4u * (uint32_t) res) - S32(st, G_GOAL_COUNTERS + 4u * (uint32_t) res);
    return S32(st, c + CITY_STORAGE_STOCK + 4u * (uint32_t) res) / 100;
}

void cmd_grave_goods_max(game_state *st)
{
    /* @bin 0x0040B3B0 amount +0x2984 := min(left, stock / 100) */
    uint32_t c = CURRENT_CITY(st);
    int32_t left;
    int32_t stock = grave_goods_most(st, c, &left);
    S32(st, c + CITY_GRAVE_GOODS_AMOUNT) = left < stock ? left : stock;
    U8(st, G_REDRAW) = 1;
}

void cmd_grave_goods_more(game_state *st)
{
    /* @bin 0x0040B420 amount + 1, at most min(stock / 100, left) */
    uint32_t c = CURRENT_CITY(st);
    int32_t v = S32(st, c + CITY_GRAVE_GOODS_AMOUNT) + 1;
    S32(st, c + CITY_GRAVE_GOODS_AMOUNT) = v;
    int32_t left;
    int32_t stock = grave_goods_most(st, c, &left);
    int32_t most = stock < left ? stock : left;
    S32(st, c + CITY_GRAVE_GOODS_AMOUNT) = v < most ? v : most;
    U8(st, G_REDRAW) = 1;
}

void cmd_grave_goods_less(game_state *st)
{
    /* @bin 0x0040B4B0 amount - 1 (not below 0) on a press of the button (u8 0x00E38E03) */
    if (U8(st, G_LEFT_PRESSED) != 0) {
        uint32_t c = CURRENT_CITY(st);
        if (S32(st, c + CITY_GRAVE_GOODS_AMOUNT) > 0) {
            S32(st, c + CITY_GRAVE_GOODS_AMOUNT) -= 1;
        }
    }
    U8(st, G_REDRAW) = 1;
}

void cmd_grave_goods_send(game_state *st)
{
    /* @bin 0x0040B1F0 sent += amount; amount * 100 taken from the storage yards */
    int32_t player = S32(st, CURRENT_PLAYER_ADDR);
    uint32_t c = CITY_ADDR(player);
    int32_t res = S32(st, c + CITY_GRAVE_GOODS_RESOURCE);
    int32_t amount = S32(st, c + CITY_GRAVE_GOODS_AMOUNT);
    U32(st, G_GOAL_COUNTERS + 4u * (uint32_t) res) += (uint32_t) amount;
    GAME_CALL(st, FN_PLACE_TAKE_GOODS, player, res, amount * 100);
    if ((uint8_t) GAME_CALL0(st, FN_GOALS_COUNTERS_MET_4EA930) == 0) {
        U8(st, G_WINDOW) = 7;
        U8(st, G_REDRAW) = 1;
        return;
    }
    /* @bin 0x0040B23C all goals met: the tombs 0xE5, 0xEA, 0xEB, 0xEC (jump tables 0x0040B340 /
     * 0x0040B360) whose +0x12 is 0 and whose progress (thiscall on their monument object) is 100
     * are finished (+0x58 := 1) and redrawn (0x005468D0); with +0x12 set they are only redrawn */
    for (int32_t id = 1; id <= S32(st, G_BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        int16_t t = S16(st, b + BLD_TYPE);
        uint32_t obj, progress;
        switch (t) {
        case 0xE5:
            obj = G_MONUMENT_E5, progress = FN_MONUMENT_PROGRESS_PARTS_9;
            break;
        case 0xEA:
            obj = G_MONUMENT_EA, progress = FN_MONUMENT_PROGRESS_PARTS_10;
            break;
        case 0xEB:
            obj = G_MONUMENT_EB, progress = FN_MONUMENT_PROGRESS_PARTS_21;
            break;
        case 0xEC:
            obj = G_MONUMENT_EC, progress = FN_MONUMENT_PROGRESS_PARTS_31;
            break;
        default:
            continue;
        }
        if (S16(st, b + BLD_PART) == 0) {
            if (GAME_CALLT(st, obj, progress, id, 0) != 0x64) {
                continue;
            }
            S32(st, b + BLD_MONUMENT_STAGE) = 1;
        }
        GAME_CALLT(st, obj, FN_MONUMENT_REDRAW_5468D0, id);
    }
    GAME_CALL0(st, FN_TERRAIN_REBUILD);
    U8(st, G_WINDOW) = 7;
    U8(st, G_REDRAW) = 1;
}

void cmd_grave_goods_cancel(game_state *st)
{
    /* @bin 0x0040B380 sent := the value kept when the window opened (+0x2980) */
    uint32_t c = CURRENT_CITY(st);
    U8(st, G_WINDOW) = 7;
    U8(st, G_REDRAW) = 1;
    S32(st, G_GOAL_COUNTERS + 4u * U32(st, c + CITY_GRAVE_GOODS_RESOURCE)) = S32(st, c + CITY_GRAVE_GOODS_SENT);
}

#define V(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
V(run_confirmed, cmd_request_confirmed)
V(run_choose, cmd_grave_goods_choose)
V(run_max, cmd_grave_goods_max)
V(run_more, cmd_grave_goods_more)
V(run_less, cmd_grave_goods_less)
V(run_send, cmd_grave_goods_send)
V(run_cancel, cmd_grave_goods_cancel)
V(run_window, grave_goods_window)

static int32_t run_dispatch(game_state *st, const int32_t *a)
{
    request_dispatch(st, a[0]);
    return 0;
}

static int32_t run_troops(game_state *st, const int32_t *a)
{
    request_send_troops(st, a[0]);
    return 0;
}

static int32_t run_count(game_state *st, const int32_t *a) { return grave_goods_count(st, a[0]); }
static int32_t run_row(game_state *st, const int32_t *a) { return grave_goods_row(st, a[0]); }

const ported_function kingdom_ports[] = {
    {0x00402340u, "cmd_request_confirmed", 0, 0, 0, run_confirmed, 0},
    {0x0044C1A0u, "request_dispatch", 1, 0, 0, run_dispatch, 0},
    {0x004B84B0u, "request_send_troops", 1, 0, 0, run_troops, 0},
    {0x0040CD70u, "cmd_grave_goods_choose", 0, 0, 0, run_choose, 0},
    {0x0040B3B0u, "cmd_grave_goods_max", 0, 0, 0, run_max, 0},
    {0x0040B420u, "cmd_grave_goods_more", 0, 0, 0, run_more, 0},
    {0x0040B4B0u, "cmd_grave_goods_less", 0, 0, 0, run_less, 0},
    {0x0040B1F0u, "cmd_grave_goods_send", 0, 0, 0, run_send, 0},
    {0x0040B380u, "cmd_grave_goods_cancel", 0, 0, 0, run_cancel, 0},
    {0x0040CC40u, "grave_goods_count", 1, 32, 0, run_count, 0},
    {0x0040CD30u, "grave_goods_row", 1, 32, 0, run_row, 0},
    {0x0040AE00u, "grave_goods_window", 0, 0, 0, run_window, 0},
};
const int kingdom_ports_count = sizeof(kingdom_ports) / sizeof(kingdom_ports[0]);
