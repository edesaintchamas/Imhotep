/* Imhotep - the requests for troops (event type 1 with good 0x25, 0x0044C9E0) and the empire's
 * armies they go to (package month2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/events_military.h"

#include "city/events.h"
#include "city/month_events.h"
#include "city/month_message.h"
#include "figure/actions_c2.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

/* the empire's armies on the march: 50 records of 0x144 bytes at 0x00776FB0 (+0 strength,
 * +0x134 size, +0x13C / +0x13E the two sides' objects, +0x141 state, +0x142 active) */
#define ARMY(i) (G_ROUTES + 0x144u * (uint32_t) (i))

int32_t army_travel_months(game_state *st, uint32_t r)
{
    /* @bin 0x004477D0 (thiscall on an army) +0x134 * +0 / 400, at least 1 */
    int32_t v = (int32_t) (U32(st, r + 0x134) * (uint32_t) U8(st, r)) / 400;
    return v < 1 ? 1 : v;
}

int32_t army_of_city(game_state *st, int32_t city, uint8_t *ours)
{
    /* @bin 0x00447430 (cdecl) the first object of the 200 at 0x0077F748 (0x62 bytes) with +1 set, +0
     * == 1 and +0x18 clear (ours, k); the first active army (+0x141 above 1, else +0x142 := 0) between
     * the city's object (city +0x56) and k: its index, *ours := its +0x13C is k; none: -1 */
    int32_t k = 0;
    uint32_t a = G_EMPIRE_OBJECTS;
    for (; a < G_EMPIRE_OBJECTS_END; a += 0x62, k++) {
        if (U8(st, a + 1) != 0 && U8(st, a) == 1 && U8(st, a + 0x18) == 0) {
            break;
        }
    }
    if (a >= G_EMPIRE_OBJECTS_END) {
        return -1;
    }
    int32_t obj = S16(st, G_EMPIRE_CITIES + 0x56u + 0x6Au * (uint32_t) city);
    for (int32_t j = 0; j < 0x32; j++) {
        uint32_t r = ARMY(j);
        if (U8(st, r + 0x141) <= 1) {
            U8(st, r + 0x142) = 0;
        }
        if (U8(st, r + 0x142) == 0) {
            continue;
        }
        int32_t s0 = S16(st, r + 0x13C), s1 = S16(st, r + 0x13E);
        if ((s0 == obj || s1 == obj) && (s0 == k || S16(st, r + 0x13E) == k)) {
            *ours = S16(st, r + 0x13C) == k;
            return j;
        }
    }
    return -1;
}

int32_t event_army_months(game_state *st, uint32_t e)
{
    /* @bin 0x0044C110 (thiscall) the months the event's army takes: type 2 by its army +0x58 (when
     * still active), others by the city +0x1C (after 0x00447430) */
    if (U8(st, e + 2) == 2) {
        int32_t i = S16(st, e + 0x58);
        if (i < 0 || i >= 0x32) {
            return 0;
        }
        uint32_t r = ARMY(i);
        if (U8(st, r + 0x141) <= 1) {
            U8(st, r + 0x142) = 0;
        }
        if (U8(st, r + 0x142) == 0) {
            return 0;
        }
        return army_travel_months(st, r);
    }
    if (!event_type_linkable(st, e)) {
        return 0;
    }
    int32_t c = S16(st, e + 0x1C);
    uint8_t ours = 0;
    army_of_city(st, empire_city_index(st, (uint32_t) c), &ours);
    return army_travel_months(st, ARMY(c));
}

int32_t event_troops_months_left(game_state *st, uint32_t e)
{
    /* @bin 0x0044C9A0 (thiscall) months left - +0x61 - the army's months (12 when none) + 6, at
     * least 1 */
    int32_t left = event_months_left(st, e);
    int32_t r = event_army_months(st, e);
    if (r <= 0) {
        r = 0xC;
    }
    int32_t v = left - (int32_t) U8(st, e + 0x61) - r + 6;
    return v < 1 ? 1 : v;
}

int32_t event_troops_sent(game_state *st, int32_t ev)
{
    /* @bin 0x00446260 (cdecl) the companies sent for event ev (0x004B8360) against its +0xC: too few
     * and city +0x498C clear: 0x004B8890(100) and 0; else (+0x498C cleared: 0) a loss percent by how
     * many more were sent (0x004CDE60): 70, 50, 25, 15, 10, 5 or 0; 1 */
    uint32_t rec = ev >= 0 && ev < S32(st, G_EVENTS) ? G_EVENT_RECORDS + 0x7Cu * (uint32_t) ev : 0;
    int32_t sent = company_strength(st, ev, NULL, NULL);
    int32_t need = rec != 0 ? S16(st, rec + 0xC) : 1;
    uint32_t flag = CURRENT_CITY(st) + CITY_U498C;
    if (sent < need && S32(st, flag) == 0) {
        companies_losses(st, 0x64, ev);
        return 0;
    }
    if (S32(st, flag) != 0) {
        S32(st, flag) = 0;
        companies_losses(st, 0, ev);
        return 1;
    }
    int32_t p = GAME_CALL(st, FN_CALC_RATIO_PERCENT, sent - need, sent);
    int32_t loss = p < 0xA ? 0x46 : p < 0x19 ? 0x32 : p < 0x32 ? 0x19 : p < 0x4B ? 0xF : p < 0x64 ? 0xA : p >= 0x96 ? 0 : 5;
    companies_losses(st, loss, ev);
    return 1;
}

/* the formations: records of 0x90 bytes, 1..49 (record k at 0x00BCB9C8 + 0x90 k): +0 in use, +3,
 * +4 type, +8 the figures (i16, +0x28 of them), +0x2E / +0x30 a tile, +0x42 active, +0x67, +0x7A,
 * +0x82 the event they serve */
#define FORMATION(k) (G_FORMATION_BASE + 0x90u * (uint32_t) (k))

int32_t ship_strength(game_state *st, int32_t f)
{
    /* @bin 0x004B8330 the figure's +0x128 / 10 */
    return S32(st, FIGURE_ADDR(f) + FIG_HIT_POINTS) / 10;
}

int32_t company_strength(game_state *st, int32_t ev, int32_t *ships, int32_t *companies)
{
    /* @bin 0x004B8360 (cdecl) for event ev: the companies sent (formations in use, active, +0x67,
     * with figures: (+0x7A + 200 for type 13, else 100) * count / 100), *companies; plus the ships of
     * the player (figures 0x4E, +0x132) by 0x004B8330, *ships; their total */
    if (ev < 0) {
        return 0;
    }
    int32_t sum = 0;
    for (int32_t k = 1; k < 0x32; k++) {
        uint32_t f = FORMATION(k);
        uint8_t n = U8(st, f + 0x28);
        if (U8(st, f) != 1 || U8(st, f + 0x42) == 0 || U8(st, f + 0x67) == 0 || n == 0 || S16(st, f + 0x82) != ev) {
            continue;
        }
        int32_t base = S16(st, f + 0x7A) + (S16(st, f + 4) == 0xD ? 200 : 100);
        sum += (int32_t) ((uint32_t) base * n) / 100;
    }
    if (companies) {
        *companies = sum;
    }
    int32_t total = sum;
    for (int32_t id = 1; id < FIGURE_COUNT; id++) {
        uint32_t r = FIGURE_ADDR(id);
        if (U8(st, r + FIG_STATE) != 0 && U8(st, r + FIG_TYPE) == 0x4E && U8(st, r + FIG_OWNER) == U32(st, CURRENT_PLAYER_ADDR) &&
            S16(st, r + FIG_U132) == ev) {
            total += ship_strength(st, id);
        }
    }
    if (ships) {
        *ships = total - sum;
    }
    return total;
}

int32_t formation_ship(game_state *st, int32_t k)
{
    /* @bin 0x004B8460 the first figure 0x4D of formation k (+0x92); 0x00D056F8 the figure loop
     * counter (2000 when none) */
    int32_t id = 1;
    for (; id < FIGURE_COUNT; id++) {
        uint32_t r = FIGURE_ADDR(id);
        if (U8(st, r + FIG_STATE) != 0 && U8(st, r + FIG_TYPE) == 0x4D && (int32_t) U8(st, r + FIG_U92) == k) {
            S32(st, G_CURRENT_FIGURE) = id;
            return id;
        }
    }
    S32(st, G_CURRENT_FIGURE) = id;
    return 0;
}

static int figure_fighting(game_state *st, int16_t id)
{
    return id > 0 && U8(st, FIGURE_ADDR(id) + FIG_STATE) == 1 && U8(st, FIGURE_ADDR(id) + FIG_ACTION) != 4;
}

void companies_losses(game_state *st, int32_t pct, int32_t ev)
{
    /* @bin 0x004B8890 (cdecl) the companies and ships sent for event ev lose pct percent: in each
     * company that keep (0x004CDE40) of its fighting figures; all kept: the company goes home (its
     * ship 0x004B8460 sunk when 0x0040B5E0, +3 := 1, +0x82 := -1); the first `keep` fighting ones get
     * +0xE := 2. Then the ships lose pct of ten times their strength in order (100: all) */
    int32_t ships = 0, mine = 0;
    company_strength(st, ev, &ships, &mine);
    for (int32_t k = 1; k < 0x32; k++) {
        uint32_t f = FORMATION(k);
        if (U8(st, f) != 1 || U8(st, f + 0x42) == 0 || S16(st, f + 0x82) != ev) {
            continue;
        }
        int32_t alive = 0;
        int32_t n = U8(st, f + 0x28);
        for (int32_t i = 0; i < n; i++) {
            if (figure_fighting(st, S16(st, f + 8 + 2u * (uint32_t) i))) {
                alive++;
            }
        }
        int32_t keep = GAME_CALL(st, FN_CALC_PERCENTAGE, alive, pct);
        if (keep >= alive) {
            if (GAME_CALL(st, FN_FORMATION_HAS_ACTION_0X12, k) != 0) {
                int32_t s = formation_ship(st, k);
                S32(st, G_CURRENT_FIGURE) = s;
                if (s != 0) {
                    uint32_t r = FIGURE_ADDR(s);
                    S32(st, r + FIG_HIT_POINTS) = 0;
                    U8(st, r + FIG_STATE) = 4;
                    S16(st, r + FIG_U132) = -1;
                }
            }
            U8(st, f + 3) = 1;
            S16(st, f + 0x82) = -1;
        }
        n = U8(st, f + 0x28);
        for (int32_t i = 0; i < n; i++) {
            int16_t id = S16(st, f + 8 + 2u * (uint32_t) i);
            if (figure_fighting(st, id) && keep != 0) {
                keep--;
                U8(st, FIGURE_ADDR(id) + FIG_STATE) = 2;
            }
        }
    }
    int32_t lose = GAME_CALL(st, FN_CALC_PERCENTAGE, (int32_t) ((uint32_t) ships * 10u), pct);
    uint32_t player = U32(st, CURRENT_PLAYER_ADDR);
    int32_t id = 1;
    for (; id < FIGURE_COUNT; id++) {
        uint32_t r = FIGURE_ADDR(id);
        if (U8(st, r + FIG_STATE) == 0 || U8(st, r + FIG_TYPE) != 0x4E || U8(st, r + FIG_OWNER) != player || S16(st, r + FIG_U132) != ev) {
            continue;
        }
        int32_t s = S32(st, r + FIG_HIT_POINTS);
        if (s >= lose && pct != 100) {
            S32(st, G_CURRENT_FIGURE) = id;
            S32(st, r + FIG_HIT_POINTS) = s - lose;
            return;
        }
        S32(st, r + FIG_HIT_POINTS) = 0;
        U8(st, r + FIG_ACTION) = 4;
        lose -= s;
        S16(st, r + FIG_U132) = -1;
    }
    S32(st, G_CURRENT_FIGURE) = id;
}

void companies_return(game_state *st, int32_t ev)
{
    /* @bin 0x004B8690 (cdecl) the companies of event ev come home (+0x82 := -1): by ship (0x0040B5E0:
     * the ship 0x004B8460 sails to the water tile towards the company's tile, 0x0040C180, +0x38 :=
     * 0x13) or on foot (fighting figures +0x38 := 0x10); the player's ships 0x4E sail back to their
     * dock (0x004806C0, +0x38 := 0x13) */
    for (int32_t k = 1; k < 0x32; k++) {
        uint32_t f = FORMATION(k);
        if (U8(st, f) != 1 || U8(st, f + 0x42) == 0 || S16(st, f + 0x82) != ev) {
            continue;
        }
        S16(st, f + 0x82) = -1;
        if (GAME_CALL(st, FN_FORMATION_HAS_ACTION_0X12, k) != 0) {
            int32_t s = formation_ship(st, k);
            if (s == 0) {
                continue;
            }
            uint32_t r = FIGURE_ADDR(s);
            S16(st, r + FIG_SOURCE_X) = S16(st, r + FIG_X);
            S16(st, r + FIG_SOURCE_Y) = S16(st, r + FIG_Y);
            int32_t px = 0, py = 0;
            line_search_water(st, S16(st, r + FIG_X), S16(st, r + FIG_Y), S16(st, f + 0x2E), S16(st, f + 0x30), &px, &py);
            S16(st, r + FIG_DEST_X) = (int16_t) px;
            S16(st, r + FIG_DEST_Y) = (int16_t) py;
            S16(st, r + FIG_U30) = S16(st, f + 0x2E);
            S16(st, r + FIG_U32) = S16(st, f + 0x30);
            U8(st, r + FIG_ACTION) = 0x13;
        } else {
            int32_t n = U8(st, f + 0x28);
            for (int32_t i = 0; i < n; i++) {
                int16_t id = S16(st, f + 8 + 2u * (uint32_t) i);
                if (figure_fighting(st, id)) {
                    U8(st, FIGURE_ADDR(id) + FIG_ACTION) = 0x10;
                }
            }
        }
    }
    uint32_t sx = U32(st, G_ROAD_ACCESS_X), sy = U32(st, G_ROAD_ACCESS_Y);
    for (int32_t id = 1; id < FIGURE_COUNT; id++) {
        uint32_t r = FIGURE_ADDR(id);
        if (U8(st, r + FIG_STATE) == 0 || U8(st, r + FIG_TYPE) != 0x4E || S16(st, r + FIG_U132) != ev ||
            U8(st, r + FIG_OWNER) != U32(st, CURRENT_PLAYER_ADDR)) {
            continue;
        }
        S16(st, r + FIG_SOURCE_X) = S16(st, r + FIG_X);
        S16(st, r + FIG_SOURCE_Y) = S16(st, r + FIG_Y);
        uint32_t b = BUILDING_ADDR(S16(st, r + FIG_HOME));
        GAME_CALL(st, FN_BUILDING_FIND_ROAD_8, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE));
        S16(st, r + FIG_DEST_X) = (int16_t) U16(st, G_ROAD_ACCESS_X);
        S16(st, r + FIG_DEST_Y) = (int16_t) U16(st, G_ROAD_ACCESS_Y);
        S16(st, r + FIG_U132) = -1;
        U32(st, G_ROAD_ACCESS_X) = sx;
        U32(st, G_ROAD_ACCESS_Y) = sy;
        U8(st, r + FIG_ACTION) = 0x13;
    }
}

static void put16(uint8_t *b, uint32_t off, int32_t v)
{
    b[off] = (uint8_t) v;
    b[off + 1] = (uint8_t) ((uint32_t) v >> 8);
}

static void start_link(game_state *st, uint32_t e, uint32_t link, int32_t slot)
{
    if (event_link_active(st, e, S16(st, e + link))) {
        int32_t v = event_outcome_message(st, e, slot);
        event_start(st, e, S16(st, e + link), v);
    }
}

void event_troops_request(game_state *st, uint32_t e)
{
    /* @bin 0x0044C9E0 (thiscall) a request for troops: stage +0x30 (0 asked, 5 sent, 6 / 7 marching
     * back, 8 over), +0x2E months; messages 0x10A (subtype 1) / 0x110, reminders 0x82 at +0x61 / +0x62
     * months before; when its months run out: not sent: the event +0x26; sent: 0x00446260 decides
     * (the event +0x24, else +0x54), or late (+0x2E above 2) the event +0x52; the army's way back
     * (0x0044C110); back: message 0x120 / 0x11F and 0x004B8690 */
    if (S16(st, e + 0x30) == 8) {
        return;
    }
    uint8_t blk[32] = {0};
    int on = U8(st, e + 0x56) != 0, one = U8(st, e + 0x60) == 1;
    int32_t id = one ? 0x10A : 0x110;
    put16(blk, 2, one ? (on ? 0 : 1) : (on ? 0x12 : 0x13));
    put16(blk, 0x10, event_message_kind(st, e));
    event_fill_block(st, e, blk);
    int32_t left = event_months_left(st, e);
    int32_t base = (one ? 0 : 0x12) + (on ? 0 : 1);
    if (left == 0) {
        S32(st, G_MESSAGE_POPUP) = 1;
        int16_t s = S16(st, e + 0x30);
        if (s == 0) {
            start_link(st, e, 0x26, 1);
        } else if (s == 5) {
            int16_t m;
            if (S16(st, e + 0x2E) > 2) {
                start_link(st, e, 0x52, 2);
                S16(st, e + 0x30) = 7;
                m = (int16_t) (event_army_months(st, e) - S16(st, e + 0x2E));
            } else if (event_troops_sent(st, S16(st, e)) != 0) {
                start_link(st, e, 0x24, 0);
                S16(st, e + 0x30) = 6;
                m = (int16_t) event_army_months(st, e);
            } else {
                start_link(st, e, 0x54, 3);
                S16(st, e + 0x30) = 8;
                U8(st, e + 0x32) = 0;
                return;
            }
            S16(st, e + 0x2E) = m;
            if (m < 1) {
                S16(st, e + 0x2E) = 1;
            }
            return;
        }
        S16(st, e + 0x30) = 8;
        U8(st, e + 0x32) = 0;
        return;
    }
    switch (S16(st, e + 0x30)) {
    case 5:
        S16(st, e + 0x2E) = (int16_t) (S16(st, e + 0x2E) - 1);
        if (S16(st, e + 0x2E) < 1) {
            S16(st, e + 0x2E) = 1;
        }
        return;
    case 6:
    case 7:
        S16(st, e + 0x2E) = (int16_t) (S16(st, e + 0x2E) - 1);
        if (S16(st, e + 0x2E) >= 1) {
            return;
        }
        S32(st, G_MESSAGE_POPUP) = 1;
        message_post(st, S16(st, e + 0x30) == 6 ? 0x120 : 0x11F, 0, S32(st, CURRENT_CITY(st) + CITY_EXIT_OFFSET), NULL, 0);
        companies_return(st, S16(st, e));
        S16(st, e + 0x30) = 8;
        U8(st, e + 0x32) = 0;
        return;
    case 0:
        break;
    default:
        return;
    }
    put16(blk, 0xC, event_troops_months_left(st, e));
    if (U8(st, e + 0x32) == 0) {
        /* @bin 0x0044CC4A the start, at its date */
        if (U8(st, e + 0x28) & 0x11) {
            return;
        }
        if (S32(st, G_YEAR) != S16(st, e + 0x14) + S16(st, G_START_YEAR) ||
            S32(st, G_MONTH) != (int32_t) U8(st, e + 3)) {
            return;
        }
        U8(st, e + 0x32) = 1;
        S16(st, e + 0x2E) = S16(st, e + 0x2C);
        S32(st, G_MESSAGE_POPUP) = 1;
        put16(blk, 0, base + 2);
        message_post(st, id, S16(st, e), 0, blk, 0);
        return;
    }
    /* @bin 0x0044CCE1 reminders */
    int32_t r = event_army_months(st, e);
    if (r <= 0) {
        r = 0xC;
    }
    int32_t d = left - r;
    if (d == (int32_t) U8(st, e + 0x61)) {
        put16(blk, 0, base + 4);
    } else if (d == (int32_t) U8(st, e + 0x62)) {
        put16(blk, 0, base + 6);
    } else {
        return;
    }
    message_post(st, 0x82, S16(st, e), 0, blk, 0);
}

#define THIS_REC ((uint32_t) a[PORT_THIS_ARG])

static int32_t run_travel(game_state *st, const int32_t *a) { return army_travel_months(st, THIS_REC); }
static int32_t run_army_months(game_state *st, const int32_t *a) { return event_army_months(st, THIS_REC); }
static int32_t run_left(game_state *st, const int32_t *a) { return event_troops_months_left(st, THIS_REC); }
static int32_t run_sent(game_state *st, const int32_t *a) { return event_troops_sent(st, a[0]); }
static int32_t run_request(game_state *st, const int32_t *a) { event_troops_request(st, THIS_REC); return 0; }
static int32_t run_ship_strength(game_state *st, const int32_t *a) { return ship_strength(st, a[0]); }
static int32_t run_company_strength(game_state *st, const int32_t *a)
{
    if (a[1] != 0 || a[2] != 0) {
        state_unported(st, FN_COMPANY_STRENGTH); /* outputs in the caller's stack */
        return 0;
    }
    return company_strength(st, a[0], NULL, NULL);
}
static int32_t run_formation_ship(game_state *st, const int32_t *a) { return formation_ship(st, a[0]); }
static int32_t run_losses(game_state *st, const int32_t *a) { companies_losses(st, a[0], a[1]); return 0; }
static int32_t run_return(game_state *st, const int32_t *a) { companies_return(st, a[0]); return 0; }

const ported_function month2_military_ports[] = {
    {0x004477D0u, "army_travel_months", 0, 32, 0, run_travel, 0},
    {0x00447430u, "army_of_city (output in the caller's stack, in events_military.c)", 2, 32, 0, NULL, 0},
    {0x0044C110u, "event_army_months", 0, 32, 0, run_army_months, 0},
    {0x0044C9A0u, "event_troops_months_left", 0, 32, 0, run_left, 0},
    {0x00446260u, "event_troops_sent", 1, 32, 0, run_sent, 0},
    {0x0044C9E0u, "event_troops_request", 0, 0, 0, run_request, 0},
    {0x004B8330u, "ship_strength", 1, 32, 0, run_ship_strength, 0},
    {0x004B8360u, "company_strength", 3, 32, 0, run_company_strength, 0},
    {0x004B8460u, "formation_ship", 1, 32, 0, run_formation_ship, 0},
    {0x004B8890u, "companies_losses", 2, 0, 0, run_losses, 0},
    {0x004B8690u, "companies_return", 1, 0, 0, run_return, 0},
};
const int month2_military_ports_count = sizeof(month2_military_ports) / sizeof(month2_military_ports[0]);
