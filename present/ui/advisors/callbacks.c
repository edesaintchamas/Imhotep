/* Imhotep - the advisors: button callbacks of the advisor pages and the dialogs over the hall.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The input dispatch 0x004D8310 (package wincore) runs the button tables of window 7 (the page
 * tabs 0x005F8320, the help button 0x005C7B80, then the page's tables) and of the dialogs; a click
 * calls the entry's function (cdecl, no arguments) with its parameters in 0x00D3B198 / 0x00D3B194.
 * Most of these open a window or move the camera; those that change the simulation are the
 * phase-5 commands of core/command (docs/commands.md 3), reached from here or from the tables
 * directly. */
#include "advisors.h"

#include "city/monument_rating.h"
#include "command/military.h"
#include "present_layout.h"
#include "sim/call.h"
#include "state/records.h"

static uint32_t city(game_state *st)
{
    return CURRENT_CITY(st);
}

static void open_window(game_state *st, uint8_t window)
{
    U8(st, G_WINDOW) = window;
    U8(st, G_REDRAW) = 1;
}

void advisor_tab_click(game_state *st)
{
    /* @bin 0x0040B4E0 a page tab (p1 = page; 0: back to the city): the page is shown when it is
     * available (0x006FDC88[page]) or when 0x00E38EBC is set; 0x00E38EA4 keeps it */
    int32_t page = S32(st, G_PARAM1);
    if (page == 0) {
        open_window(st, 1);
        return;
    }
    if (S32(st, G_CUSTOM) != 0 || U8(st, G_ADVISOR_AVAILABLE + (uint32_t) page) != 0) {
        S32(st, G_PAGE) = page;
        S32(st, G_LAST_ADVISOR) = page;
        U8(st, G_REDRAW) = 1;
    }
}

void advisor_help_click(game_state *st)
{
    /* @bin 0x00407290 the help button: the page's help message (i32 table 0x00576148 by page 1-14) */
    int32_t page = S32(st, G_PAGE);
    if (page <= 0 || page > 0x0E) {
        return;
    }
    GAME_CALL(st, FN_MESSAGE_WINDOW_OPEN, S32(st, G_ADVISOR_HELP + 4u * (uint32_t) page), 1, 0);
    U8(st, G_REDRAW) = 1;
}

void advisor_open_empire(game_state *st)
{
    open_window(st, 0x14); /* @bin 0x00409530 the empire map */
}

void advisor_festival_help(game_state *st)
{
    /* @bin 0x0040AAC0 the festival dialog's help: message 0x1D */
    GAME_CALL(st, FN_MESSAGE_WINDOW_OPEN, 0x1D, 1, 0);
    U8(st, G_REDRAW) = 1;
}

void advisor_trade_help(game_state *st)
{
    /* @bin 0x00559DE0 the trade resource dialog's help (window 0x28, table 0x006035E0): message 0x2E */
    GAME_CALL(st, FN_MESSAGE_WINDOW_OPEN, 0x2E, 1, 0);
    U8(st, G_REDRAW) = 1;
}

void advisor_festival_close(game_state *st)
{
    /* @bin 0x0040AAE0 close the festival dialog: back to window 7, or 9 when p2 is set (opened from
     * a building: table 0x005C7C58) */
    U8(st, G_REDRAW) = 1;
    U8(st, G_WINDOW) = S32(st, G_PARAM2) != 0 ? 9 : 7;
}

void advisor_salary_close(game_state *st)
{
    /* @bin 0x0040AF00 close the salary dialog: 0x2C -> 7, 0x4D -> 9 */
    uint8_t w = U8(st, G_WINDOW);
    if (w == 0x2C) {
        open_window(st, 7);
        return;
    }
    if (w == 0x4D) {
        U8(st, G_WINDOW) = 9;
    }
    U8(st, G_REDRAW) = 1;
}

void advisor_ratings_open(game_state *st)
{
    open_window(st, 0x50); /* @bin 0x0040D1A0 (rows of table 0x005C59E8) */
}

void advisor_set_4430(game_state *st)
{
    /* @bin 0x0040ACE0 (table 0x005F7650, p1 1-4): CITY+0x4430 := p1 */
    U8(st, G_REDRAW) = 1;
    S32(st, city(st) + CITY_RATING_SELECTED) = S32(st, G_PARAM1);
}

void advisor_labour_row(game_state *st)
{
    /* @bin 0x0040A6C0 a labour category row (p1 = category): 0x00605988 := category; 0x00605984 :=
     * the number of categories with a priority (CITY+0x2B3C + 0x14 cat > 0), plus one when below 9
     * and this category has none (the chooser's last choice); opens the chooser, window 0x2A */
    uint32_t c = city(st);
    int32_t cat = S32(st, G_PARAM1);
    S32(st, G_LABOUR_SELECTED) = cat;
    int32_t n = 0;
    S32(st, G_PRIORITY_CHOICES) = 0;
    for (uint32_t k = 0; k < 9; k++) {
        if (S32(st, c + CITY_LABOUR_PRIORITY + CITY_LABOUR_STRIDE * k) > 0) {
            n++;
        }
    }
    S32(st, G_PRIORITY_CHOICES) = n;
    if (n < 9) {
        n++;
        S32(st, G_PRIORITY_CHOICES) = n;
        if (S32(st, c + CITY_LABOUR_PRIORITY + CITY_LABOUR_STRIDE * (uint32_t) cat) != 0) {
            S32(st, G_PRIORITY_CHOICES) = n - 1;
        }
    }
    open_window(st, 0x2A);
}

void advisor_legion_go_to(game_state *st)
{
    /* @bin 0x0040A2F0 a legion row's "go to": the camera on the formation's tile (+0x2E, +0x30)
     * when it is in the city (+0x82 < 0), back to the city view */
    int32_t f = GAME_CALL(st, FN_PLAYER_LEGION_BY_ROW, S32(st, G_PARAM1));
    if (f == 0) {
        return;
    }
    uint32_t r = G_FORMATION_BASE + 0x90u * (uint32_t) f;
    if (S16(st, r + 0x82) >= 0) {
        return;
    }
    int32_t y = S16(st, r + 0x30);
    int32_t x = S16(st, r + 0x2E);
    GAME_CALL(st, FN_CAMERA_GO_TO, S32(st, G_MAP_START) + x + y * 0xE4);
    open_window(st, 1);
}

void advisor_ship_go_to(game_state *st)
{
    /* @bin 0x0040A560 a warship row's "go to": the camera on the figure's tile (+0x14, +0x16) when
     * +0x132 < 0 */
    int32_t f = GAME_CALL(st, FN_PLAYER_WARSHIP_BY_ROW, S32(st, G_PARAM1));
    if (f == 0) {
        return;
    }
    uint32_t r = FIGURE_ADDR(f);
    if (S16(st, r + FIG_U132) >= 0) {
        return;
    }
    int32_t y = S16(st, r + FIG_Y);
    int32_t x = S16(st, r + FIG_X);
    GAME_CALL(st, FN_CAMERA_GO_TO, S32(st, G_MAP_START) + x + y * 0xE4);
    open_window(st, 1);
}

void advisor_donation_open(game_state *st)
{
    /* @bin 0x0040AD00 the donation dialog (window 0x2D): the amount CITY+0x445C capped by the
     * savings CITY+0x4450 */
    uint32_t c = city(st);
    U8(st, G_WINDOW) = 0x2D;
    if (S32(st, c + CITY_DONATION) > S32(st, c + CITY_SAVINGS)) {
        S32(st, c + CITY_DONATION) = S32(st, c + CITY_SAVINGS);
    }
    U8(st, G_REDRAW) = 1;
}

void advisor_salary_open(game_state *st)
{
    /* @bin 0x0040AD30 the salary dialog when CITY+0x499C is set: window 0x2C (p1 0, over the hall)
     * or 0x4D (else: the panel at the layout's origin, keys cleared 0x004CF1F0) */
    if (S32(st, city(st) + CITY_MANSION_BUILT) == 0) {
        return;
    }
    if (S32(st, G_PARAM1) == 0) {
        open_window(st, 0x2C);
        return;
    }
    S32(st, G_ADVISOR_X) = S32(st, G_LAYOUT_DX);
    S32(st, G_ADVISOR_Y) = S32(st, G_LAYOUT_DY);
    GAME_CALL0(st, FN_FRAME_CLEAR_KEYS);
    open_window(st, 0x4D);
}

void advisor_gift_open(game_state *st)
{
    /* @bin 0x0040AD90 the gift dialog (window 0x2E): the chosen size CITY+0x4558 lowered while its
     * cost (+0x457C large, +0x4578 medium) exceeds the savings CITY+0x4450 */
    uint32_t c = city(st);
    U8(st, G_WINDOW) = 0x2E;
    if (S32(st, c + CITY_GIFT_SIZE) == 2 && S32(st, c + CITY_GIFT_COST + 8u) > S32(st, c + CITY_SAVINGS)) {
        S32(st, c + CITY_GIFT_SIZE) = 1;
    }
    if (S32(st, c + CITY_GIFT_SIZE) == 1 && S32(st, c + CITY_GIFT_COST + 4u) > S32(st, c + CITY_SAVINGS)) {
        S32(st, c + CITY_GIFT_SIZE) = 0;
    }
    U8(st, G_REDRAW) = 1;
}

void advisor_request_row(game_state *st)
{
    /* @bin 0x0040AE10 a request row of the kingdom page (p1 = row): 0x0044D520 classifies it;
     * -6..-3 and -1 explain why it cannot be sent (popups 0x62, 0x64, 0x0A, 0x0C, 8), -2 asks to
     * dispatch the event it returned (popup 0x0E), a positive n asks to dispatch event n - 1
     * (popup 6); both confirm into 0x00402340 with the event in 0x006057E4. CITY+0x466D := 0 */
    int32_t event = 0; /* the original's uninitialised local: written by 0x0044D520 when it is read */
    int32_t r = request_row_check(st, S32(st, G_PARAM1), &event);
    if (r == 0) {
        return;
    }
    U8(st, city(st) + CITY_U466D) = 0;
    switch (r) {
    case -6: GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 0x62, (int32_t) FN_NOTHING, 0); return;
    case -5: GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 0x64, (int32_t) FN_NOTHING, 0); return;
    case -4: GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 0x0A, (int32_t) FN_NOTHING, 0); return;
    case -3: GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 0x0C, (int32_t) FN_NOTHING, 0); return;
    case -2:
        S32(st, G_REQUEST_EVENT) = event;
        GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 0x0E, FN_CMD_REQUEST_CONFIRMED, 2);
        return;
    case -1: GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 8, (int32_t) FN_NOTHING, 0); return;
    default:
        S32(st, G_REQUEST_EVENT) = r - 1;
        GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 6, FN_CMD_REQUEST_CONFIRMED, 2);
        U8(st, G_REDRAW) = 1;
        return;
    }
}

void advisor_population_graph(game_state *st)
{
    /* @bin 0x0040A8A0 the population page's two small graphs (p1 0 / 1): the big graph 0x00605A40
     * (0-5) swaps with the clicked one; 0x005F825C := -1 (the hovered bar) */
    static const int8_t next[6][2] = {{2, 5}, {4, 3}, {0, 4}, {5, 1}, {1, 2}, {3, 0}}; /* @bin 0x0040A8A0 */
    int32_t v = S32(st, G_GRAPH);
    int32_t p = S32(st, G_PARAM1);
    if (v >= 0 && v <= 5 && (p == 0 || p == 1)) {
        S32(st, G_GRAPH) = next[v][p];
    }
    S32(st, G_GRAPH_HOVER) = -1;
    U8(st, G_REDRAW) = 1;
}

void advisor_festival_open(game_state *st)
{
    /* @bin 0x0040AA50 the festival dialog unless one is planned (CITY+0x44F8): popup 0x58 without
     * a festival square (i16 CITY+0x4814), 0x60 when the treasury is -5000 or less; window 0x2B
     * (p1 0) or 0x52 (from the building) */
    uint32_t c = city(st);
    if (S32(st, c + CITY_FESTIVAL_SIZE) != 0) {
        return;
    }
    if (S16(st, c + CITY_U4814) == 0) {
        GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 0x58, (int32_t) FN_NOTHING, 0);
        return;
    }
    if (S32(st, c + CITY_TREASURY) <= -5000) {
        GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 0x60, (int32_t) FN_NOTHING, 0);
        return;
    }
    U8(st, G_REDRAW) = 1;
    U8(st, G_WINDOW) = S32(st, G_PARAM1) != 0 ? 0x52 : 0x2B;
}

void advisor_rating_explain(game_state *st)
{
    /* @bin 0x0040D130 a rating's explanation (window 0x4F), except: p1 1 when CITY+0x2740 is 0
     * and CITY+0x4644 + CITY+0x4634 > 3; p1 0 when CITY+0x10 is 100 */
    uint32_t c = city(st);
    int32_t p = S32(st, G_PARAM1);
    if (p == 1) {
        if (S32(st, c + CITY_U2740) == 0 && S32(st, c + CITY_U4644) + S32(st, c + CITY_U4634) > 3) {
            return;
        }
    } else if (p == 0) {
        if (S32(st, c + CITY_SENTIMENT) == 100) {
            return;
        }
    }
    open_window(st, 0x4F);
}

void advisor_monument_go_to(game_state *st)
{
    /* @bin 0x0040CF20 a monument row (p1 = row of CITY+0x473C): the camera on an unfinished
     * monument (progress < 100 by its type: jump tables 0x0040D0B8 / 0x0040D0F0), back to the city
     * view. The same tests as 0x004F50F0 (core/command/menu.c) */
    int32_t id = S32(st, city(st) + CITY_MONUMENTS + 4u * (uint32_t) S32(st, G_PARAM1));
    uint32_t b = BUILDING_ADDR(id);
    int32_t done;
    switch (S16(st, b + BLD_TYPE)) {
    case 0xB7: {
        int32_t out[0x1C];
        pyramid_status(st, id, out);
        done = out[0xD] >= 100; /* @bin 0x0040CF7C [esp + 0x44] before the add: out + 0x34 */
        break;
    }
    case 0xCF:
    case 0xD0: done = U8(st, b + BLD_U55) >= 100; break;  /* @bin 0x0040CFAA unsigned */
    case 0xD2: done = S16(st, city(st) + CITY_U433A) >= 0x10; break; /* @bin 0x0040CF91 */
    case 0xD5: done = GAME_CALL(st, FN_MONUMENT_D5_STATUS, id, 0) >= 100; break;
    case 0xDE: done = GAME_CALL(st, FN_MONUMENT_DE_STATUS, id, 0) >= 100; break;
    case 0xE1: done = GAME_CALLT(st, G_MONUMENT_E1, FN_MONUMENT_PROGRESS_40D940, id, 0) >= 100; break;
    case 0xE3: done = GAME_CALLT(st, G_MONUMENT_E3, FN_MONUMENT_PROGRESS_417050, id, 0) >= 100; break;
    case 0xE4: done = GAME_CALLT(st, G_MONUMENT_E4, FN_MONUMENT_PROGRESS_4E6710, id, 0) >= 100; break;
    case 0xE5: done = GAME_CALLT(st, G_MONUMENT_E5, FN_MONUMENT_PROGRESS_PARTS_9, id, 0) >= 100; break;
    case 0xE6: done = GAME_CALLT(st, G_MONUMENT_E6, FN_MONUMENT_PROGRESS_401540, id, 0) >= 100; break;
    case 0xEA: done = GAME_CALLT(st, G_MONUMENT_EA, FN_MONUMENT_PROGRESS_PARTS_10, id, 0) >= 100; break;
    case 0xEB: done = GAME_CALLT(st, G_MONUMENT_EB, FN_MONUMENT_PROGRESS_PARTS_21, id, 0) >= 100; break;
    case 0xEC: done = GAME_CALLT(st, G_MONUMENT_EC, FN_MONUMENT_PROGRESS_PARTS_31, id, 0) >= 100; break;
    default: return;
    }
    if (done) {
        return;
    }
    GAME_CALL(st, FN_CAMERA_GO_TO, S32(st, b + BLD_GRID_OFFSET));
    open_window(st, 1);
}

int32_t events_any_pending(game_state *st)
{
    /* @bin 0x00446610 1 when an event of the list 0x00785D70 (events_next 0x0044BB90) is pending
     * (0x0044D700) */
    int32_t e = GAME_CALLT(st, G_EVENTS, FN_EVENTS_NEXT, 0, 0);
    while (e != 0) {
        if ((uint8_t) GAME_CALLT0(st, (uint32_t) e, FN_EVENT_PENDING) != 0) {
            return 1;
        }
        e = GAME_CALLT(st, G_EVENTS, FN_EVENTS_NEXT, e, 0);
    }
    return 0;
}

void layout_group_at(game_state *st, int32_t group, int32_t index, uint32_t rect, int32_t font, int32_t colour,
                     int32_t ox, int32_t oy, int32_t dx, int32_t dy)
{
    /* @bin 0x004D6200 a text of group / index at the rectangle's (i16 left, top) + the layout
     * origin + (ox, oy): 0x004D62F0 */
    int32_t y = P16(st, rect + 2) + S32(st, G_LAYOUT_DY) + oy;
    int32_t x = P16(st, rect) + S32(st, G_LAYOUT_DX) + ox;
    GAME_CALL(st, FN_LAYOUT_GROUP, group, index, x, y, font, colour, dx, dy);
}
