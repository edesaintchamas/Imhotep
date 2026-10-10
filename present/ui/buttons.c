/* Imhotep - button and rectangle callbacks of the player's windows (roadmap 8.2, package "p8ui"):
 * the yes / no popup, the empire map, the overlays menu, the message window and list, the
 * storage and trade arrows, the name list, the campaign and mission menus, the cheat window.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Each is reached through a window's button table (entries of 0x24: left function +0x10) or
 * rectangle table (entries of 0x1C: function +8), with the clicked index in 0x00D3B198. */
#include "ui.h"

#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"


/* ---- the yes / no popup (window 0x3F, buttons 0x005C8168) ---- */

static void popup_answer(game_state *st, uint8_t yes)
{
    /* @bin 0x00426590 (yes: 1) / 0x004265B0 (no: 0) the answer 0x00D3B439, the popup's action
     * 0x00705704 called, the redraw, back to the window 0x0070570C */
    U8(st, G_CONFIRM_ANSWER) = yes;
    GAME_CALL0(st, U32(st, G_POPUP_ACTION));
    uint8_t back = U8(st, G_POPUP_BACK);
    U8(st, G_REDRAW) = 1;
    U8(st, G_WINDOW) = back;
}

/* ---- the empire map (window 0x14) ---- */

static int32_t half(int32_t v)
{
    return v / 2; /* @bin 0x00407AC0 cdq / sub / sar 1: rounded toward zero */
}

static int32_t distance(game_state *st, int32_t mx, int32_t my, int32_t cx, int32_t cy)
{
    /* @bin 0x00407AD2 the larger of |mx - cx| (0x00D3B4FC) and |my - cy| (0x00D3B4F8) */
    int32_t dx = mx > cx ? mx - cx : mx < cx ? cx - mx : 0;
    S32(st, G_DIST_X) = dx;
    int32_t dy = my > cy ? my - cy : my < cy ? cy - my : 0;
    S32(st, G_DIST_Y) = dy;
    return dx < dy ? dy : dx;
}

static void empire_pick(game_state *st, uint32_t object_out, uint32_t route_out)
{
    /* @bin 0x00407A10 the empire object nearest the mouse (map coordinates: the mouse less the map's
     * origin 0x005D2A18 / 0x005D2A1C plus the scroll 0x00775910 / 0x0077590C) within 30 pixels:
     * objects of 0x62 bytes from 0x0077F749 while in use (+0), their box +0x0D..+0x13 (i16 x, y, w, h,
     * or the alternative x, y +0x13 / +0x15 when 0x00784CB4 is set) widened by 8; 1-based into
     * *object_out. Then the route points 0x00F535D4 (0x00F53FDC of them, 0x14 bytes: i32 x, y, w, h,
     * screen coordinates): a nearer one clears *object_out and goes into *route_out (else -1) */
    int32_t mx = S32(st, G_MOUSE_X) - S32(st, G_MAP_X) + S32(st, G_SCROLL_X);
    int32_t my = S32(st, G_MOUSE_Y) - S32(st, G_MAP_Y) + S32(st, G_SCROLL_Y);
    int32_t best = 0x1E;
    S32(st, object_out) = 0;
    int32_t i = 0;
    for (uint32_t p = G_EMPIRE_OBJECT_ALT_X; p < G_EMPIRE_OBJECT_ALT_X + 200u * 0x62u; p += 0x62, i++) {
        if (U8(st, p - 0x13) == 0) {
            break;
        }
        int32_t x, y;
        if (S32(st, G_EMPIRE_ALT_POSITIONS) != 0) {
            x = S16(st, p);
            y = S16(st, p + 2);
        } else {
            x = S16(st, p - 0x10);
            y = S16(st, p - 0x0E);
        }
        int32_t w = S16(st, p - 0x0C), h = S16(st, p - 0x0A);
        if (x - 8 > mx || w + x + 8 <= mx || y - 8 > my || h + y + 8 <= my) {
            continue;
        }
        int32_t d = distance(st, mx, my, half(w) + x, half(h) + y);
        if (d < best) {
            best = d;
            S32(st, object_out) = i + 1;
        }
    }
    S32(st, route_out) = -1;
    for (int32_t k = 0; k < S32(st, G_MARKER_COUNT); k++) {
        uint32_t r = G_MARKER_X + (uint32_t) k * 0x14u;
        int32_t cx = half(S32(st, r + 8)) + S32(st, r);
        int32_t cy = half(S32(st, r + 0x0C)) + S32(st, r + 4);
        int32_t d = distance(st, S32(st, G_MOUSE_X), S32(st, G_MOUSE_Y), cx, cy);
        if (d < best) {
            best = d;
            S32(st, object_out) = 0;
            S32(st, route_out) = k;
        }
    }
}

static void empire_open_route(game_state *st)
{
    /* @bin 0x00409490 the trade city's open-route button (0x005C3DA0): for a city 0x00784F18 (0x6A
     * bytes from 0x0077DE00) whose +5 is below 50: popup 0x77 when the treasury (player +0x187C) is
     * -5000 or less, else the open-route question 0x004022D0 (popup 2 by land when the route's +0
     * (0x144 bytes from 0x007770F0, by the city's +4) is 1, else 4) */
    uint32_t c = (uint32_t) S32(st, G_EMPIRE_SELECTED) * 0x6Au;
    if (U8(st, G_EMPIRE_CITY_OPEN + c) >= 0x32) {
        return;
    }
    if (S32(st, CITY_BASE + CITY_TREASURY + U32(st, CURRENT_PLAYER_ADDR) * 0x49D8u) <= -5000) {
        GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 0x77, FN_NOTHING, 0);
        return;
    }
    uint32_t route = U8(st, G_EMPIRE_CITY_ROUTE + c);
    if (U8(st, G_ROUTES + ROUTE_KIND + route * ROUTE_SIZE) == 1) {
        GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 2, FN_CMD_TRADE_OPEN_ROUTE, 2);
    } else {
        GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 4, FN_CMD_TRADE_OPEN_ROUTE, 2);
    }
}

static void empire_help(game_state *st)
{
    /* @bin 0x00409510 the empire map's help button (0x005C7CE8): message 0x20 */
    GAME_CALL(st, FN_MESSAGE_WINDOW_OPEN, 0x20, 1, 0);
}

static void empire_close(game_state *st)
{
    /* @bin 0x0040A090 (and its thunk 0x00409520) back to the city; in the editor the edited
     * object's position (0x00784F10: +0x7843D8 x / +0x7843DA y, 0x20 bytes) is the scroll
     * 0x00775910 / 0x0077590C, with 0x00442800 */
    int16_t sy = S16(st, G_SCROLL_Y);
    if (S32(st, G_EDITOR) == 1) {
        int32_t o = S32(st, G_EMPIRE_CURRENT);
        int16_t sx = S16(st, G_SCROLL_X);
        S16(st, G_SCENARIO_EMPIRE) = (int16_t) o;
        S16(st, G_EMPIRE_OBJECTS_END + (uint32_t) o * 0x20u) = sx;
        S16(st, G_EMPIRE_SCROLL_Y + (uint32_t) o * 0x20u) = sy;
        GAME_CALL0(st, FN_EMPIRE_SCROLL_CLAMP);
    }
    U8(st, G_WINDOW) = 1;
    U8(st, G_REDRAW) = 1;
}

/* ---- the overlays menu (window 8) ---- */

static void overlay_monuments_refresh(game_state *st)
{
    /* @bin 0x00547A60 the buildings 1..0x0078D790 of the monument types 0xE5, 0xEA, 0xEB, 0xEC
     * (0x108 bytes, type +0x10 from 0x00799AF0) refreshed by 0x005468D0 on their monument object
     * (0x006F9320, 0x006F5648, 0x006DE9E0, 0x006E9188) */
    for (int32_t b = 1; b <= S32(st, G_BUILDING_HIGHEST); b++) {
        int16_t t = S16(st, BUILDING_ADDR(1) + BLD_TYPE + (uint32_t) (b - 1) * 0x108u);
        uint32_t obj = t == 0xE5 ? G_MONUMENT_E5 : t == 0xEA ? G_MONUMENT_EA : t == 0xEB ? G_MONUMENT_EB
                     : t == 0xEC ? G_MONUMENT_EC : 0;
        if (obj != 0) {
            GAME_CALLT(st, obj, FN_MONUMENT_REDRAW_5468D0, b);
        }
    }
}

static void overlays_group(game_state *st)
{
    /* @bin 0x00409DA0 an overlay group (0x005C2BF0): its overlay 0x005CF734[p]; 0 goes back to the
     * normal view (the old overlay kept in 0x00E94E10); the groups that are an overlay themselves
     * (4, 0x2D, 0x1F, 0x33) are chosen; then the city and 0x00547A60 */
    int32_t old = S32(st, G_OVERLAY);
    int32_t ov = S32(st, G_OVL_GROUP_OVERLAYS + (uint32_t) S32(st, G_PARAM1) * 4u);
    if (ov == 0) {
        S32(st, G_OVERLAY) = 0;
        S32(st, G_OVERLAY_PREV) = old;
    } else if (ov == 4 || ov == 0x2D || ov == 0x1F || ov == 0x33) {
        S32(st, G_OVERLAY_PREV) = 0;
        S32(st, G_OVERLAY) = ov;
    }
    U8(st, G_WINDOW) = 1;
    U8(st, G_REDRAW) = 1;
    overlay_monuments_refresh(st);
}

static void overlays_item(game_state *st)
{
    /* @bin 0x00409E00 an item of the open group 0x007056DC (0x005C2D08): the overlay
     * 0x005CF75C[group * 30 + p] when not 0; then the city and 0x00547A60 */
    uint32_t k = (uint32_t) S32(st, G_PARAM1) + (uint32_t) S32(st, G_OVL_GROUP) * 30u;
    int32_t ov = S32(st, G_OVL_ITEMS + k * 4u);
    if (ov != 0) {
        S32(st, G_OVERLAY_PREV) = 0;
        S32(st, G_OVERLAY) = ov;
    }
    U8(st, G_WINDOW) = 1;
    U8(st, G_REDRAW) = 1;
    overlay_monuments_refresh(st);
}

/* ---- arrows: storage info, trade advisor, mission menu, window 0x53 ---- */

static void storage_scroll(game_state *st)
{
    /* @bin 0x0040C6B0 the storage info's list (window 9, arrows 0x005C5B48 / 0x005C5B60): the first
     * row (building +0xD0, 0x108 bytes from 0x007999E8, of 0x00EADD6C) up (p 0, at least 1) or down
     * (p != 0: while first + 0x00502780 rows - 1 stays within 0x00F53E6C) */
    uint32_t r = BUILDING_BASE + BLD_UE0 + U32(st, G_INFO_BUILDING) * 0x108u;
    if (S32(st, G_PARAM1) == 0) {
        int32_t v = S32(st, r) - 1;
        S32(st, r) = v;
        if (v <= 0) {
            S32(st, r) = 1;
        }
        U8(st, G_REDRAW) = 1;
        return;
    }
    S32(st, r) = S32(st, r) + 1;
    int32_t rows = GAME_CALL0(st, FN_STORAGE_ROWS_VISIBLE);
    r = BUILDING_BASE + BLD_UE0 + U32(st, G_INFO_BUILDING) * 0x108u;
    if (rows + S32(st, r) - 1 > S32(st, G_STORAGE_LIST)) {
        rows = GAME_CALL0(st, FN_STORAGE_ROWS_VISIBLE);
        r = BUILDING_BASE + BLD_UE0 + U32(st, G_INFO_BUILDING) * 0x108u;
        S32(st, r) = S32(st, G_STORAGE_LIST) - rows + 1;
    }
    U8(st, G_REDRAW) = 1;
}

static void trade_scroll(game_state *st)
{
    /* @bin 0x0040C760 the trade advisor's list (arrows 0x005C5B78 / 0x005C5B90): the first row
     * (player +0x60A4) up (p 0, at least 1) or down (p != 0, at most 0x00F53E6C - 13) */
    uint32_t r = CITY_BASE + CITY_TRADE_SCROLL + U32(st, CURRENT_PLAYER_ADDR) * 0x49D8u;
    if (S32(st, G_PARAM1) == 0) {
        int32_t v = S32(st, r) - 1;
        S32(st, r) = v;
        if (v <= 0) {
            S32(st, r) = 1;
        }
        U8(st, G_REDRAW) = 1;
        return;
    }
    int32_t v = S32(st, r) + 1;
    S32(st, r) = v;
    if (v + 0x0D > S32(st, G_STORAGE_LIST)) {
        S32(st, r) = S32(st, G_STORAGE_LIST) - 0x0D;
    }
    U8(st, G_REDRAW) = 1;
}

static void mission_menu_toggle(game_state *st)
{
    /* @bin 0x0041E0E0 the mission menu's switch (0x0057CD50): 0x00704DAC becomes 2 when it was 3,
     * else 3 */
    int32_t v = S32(st, G_SCENARIO_KIND);
    U8(st, G_REDRAW) = 1;
    S32(st, G_SCENARIO_KIND) = (v != 3) + 2;
}

static void mission_menu_scroll(game_state *st)
{
    /* @bin 0x0041F460 the mission menu's list (arrows 0x0057CC30 / 0x0057CCC0, the arrow keys):
     * 0x00704DB4 entries shown 13 at a time (15 unless 0x00704DB0 is 1), first 0x00704DB8; p 0 up,
     * p 1 down */
    int32_t page = (S32(st, G_SCENARIO_LIST_MODE) != 1 ? 2 : 0) + 0x0D;
    int32_t n = S32(st, G_SCENARIO_ROWS);
    if (n <= page) {
        return;
    }
    int32_t p = S32(st, G_PARAM1);
    if (p == 0) {
        int32_t first = S32(st, G_SCENARIO_FIRST);
        if (first <= 0) {
            return;
        }
        U8(st, G_REDRAW) = 1;
        S32(st, G_SCENARIO_FIRST) = first - 1;
        return;
    }
    if (p == 1) {
        page = (S32(st, G_SCENARIO_LIST_MODE) != 1 ? 2 : 0) + 0x0D;
        if (S32(st, G_SCENARIO_FIRST) + page >= n) {
            return;
        }
        S32(st, G_SCENARIO_FIRST) = S32(st, G_SCENARIO_FIRST) + 1;
    }
    U8(st, G_REDRAW) = 1;
}

static void list53_scroll(game_state *st)
{
    /* @bin 0x004C5400 window 0x53's list (buttons 0x005E01D0): 0x00D3A4BC entries, 20 shown from
     * 0x00D3A4B8; p 0 up, p 1 down; the redraw flag 2 */
    int32_t n = S32(st, G_HIGHSCORE_ROWS);
    if (n <= 0x14) {
        return;
    }
    int32_t p = S32(st, G_PARAM1);
    if (p == 0) {
        int32_t first = S32(st, G_HIGHSCORE_FIRST);
        if (first <= 0) {
            return;
        }
        U8(st, G_REDRAW) = 2;
        S32(st, G_HIGHSCORE_FIRST) = first - 1;
        return;
    }
    if (p == 1) {
        if (S32(st, G_HIGHSCORE_FIRST) + 0x14 >= n) {
            return;
        }
        S32(st, G_HIGHSCORE_FIRST) = S32(st, G_HIGHSCORE_FIRST) + 1;
    }
    U8(st, G_REDRAW) = 2;
}

/* ---- the message window (5) and the message list (0x11) ---- */

static void msgwin_scroll(game_state *st)
{
    /* @bin 0x004C7E90 the message window's text (arrows 0x005C7DF8 / 0x005C7E20): first line
     * 0x00D3B130 of 0x00D3B134, p 0 up / p 1 down; 0x004C8040, 0x00D3B12C cleared, redraw flag 2 */
    int32_t p = S32(st, G_PARAM1);
    if (p == 0) {
        int32_t v = S32(st, G_SCROLL);
        if (v > 0) {
            S32(st, G_SCROLL) = v - 1;
        }
    } else if (p == 1) {
        int32_t v = S32(st, G_SCROLL);
        if (v < S32(st, G_MESSAGE_WINDOW_SCROLL_MAX)) {
            S32(st, G_SCROLL) = v + 1;
        }
    }
    GAME_CALL0(st, FN_MESSAGE_WINDOW_LINES_CLEAR);
    S32(st, G_MW_DRAGGING) = 0;
    U8(st, G_REDRAW) = 2;
}

static void msgwin_back(game_state *st)
{
    /* @bin 0x004C7EE0 the message window's back button (0x005C7DA8): the history 0x00D3AAE0 (its
     * depth; the messages 0x00D3AE00[], first lines 0x00D3AAE0[]) popped into 0x00D3B124 /
     * 0x00D3B130; 0x004C8040, 0x00D3B138 and 0x00D3B12C cleared, the redraw */
    int32_t n = S32(st, G_HISTORY);
    if (n <= 0) {
        return;
    }
    int32_t msg = S32(st, G_MW_HISTORY_MESSAGES + (uint32_t) n * 4u);
    int32_t line = S32(st, G_HISTORY + (uint32_t) n * 4u);
    S32(st, G_MW_TYPE) = msg;
    S32(st, G_HISTORY) = n - 1;
    S32(st, G_SCROLL) = line;
    GAME_CALL0(st, FN_MESSAGE_WINDOW_LINES_CLEAR);
    S32(st, G_TEXT_LINES) = 0;
    S32(st, G_MW_DRAGGING) = 0;
    U8(st, G_REDRAW) = 1;
}

static void msgwin_close_help(game_state *st)
{
    /* @bin 0x004E2360 the message window's button 0x005C7E48: 0x004C7F30, then message 10 */
    GAME_CALL0(st, FN_MESSAGE_WINDOW_CLOSE);
    GAME_CALL(st, FN_MESSAGE_WINDOW_OPEN, 0x0A, 0, 0);
}

static int32_t message_figure_tile(game_state *st, int32_t id)
{
    /* @bin 0x004B80B0 the map offset of the figure (0x90-byte records 1.. from 0x00BCBA58 (+0 of record i at 0x00BCB9C8 + i * 0x90), while
     * below 0x00BCD5E8) in use (+0 = 1), not dying (+0x42 = 0, +0x69 = 0) whose +0x88 (i16) is id: the
     * tile 0x00BCA2D8 + x (+0x2E) + y (+0x30) * 228 when x or y is positive; else 0 */
    int32_t i = 1;
    uint32_t p = G_FORMATION_U42 + 0x90u;
    for (;; p += 0x90, i++) {
        if (p >= G_FORMATION_U42 + 50u * 0x90u) {
            return 0;
        }
        if (U8(st, p - 0x42) == 1 && U8(st, p) == 0 && U8(st, p + 0x27) == 0 && S16(st, p + 0x46) == id) {
            break;
        }
    }
    int16_t y = S16(st, G_FORMATION_Y + (uint32_t) i * 0x90u);
    int16_t x = S16(st, G_FORMATION_X + (uint32_t) i * 0x90u);
    int32_t tile = S32(st, G_MAP_START) + x + y * 0xE4;
    if (x > 0 || y > 0) {
        return tile;
    }
    return 0;
}

static void msgwin_go_to(game_state *st)
{
    /* @bin 0x004E2380 the message window's go-to button (0x005C7E70): the message 0x00E951E4's
     * tile (0x30 bytes from 0x00E95200: +0x2C), or for a message of kind 7 (0x50 bytes from
     * 0x00E20990 by 0x00D3B124: +0x2A) its figure's (0x004B80B0 of +0x28) when positive; the city
     * view centred there (0x00430CD0) when 0 < tile < 0xCB10; back to the city */
    uint32_t m = U32(st, G_MESSAGE_CURRENT) * 0x30u;
    int32_t tile = S32(st, MESSAGE_BASE + MSG_PARAM_B + m);
    if (S16(st, G_MESSAGE_TYPES + MSGTYPE_U0A + U32(st, G_MW_TYPE) * 0x50u) == 7) {
        int32_t t = message_figure_tile(st, S32(st, MESSAGE_BASE + MSG_PARAM_A + m));
        if (t > 0) {
            tile = t;
        }
    }
    if (tile > 0 && tile < 0xCB10) {
        GAME_CALL(st, FN_CAMERA_GO_TO, tile);
    }
    U8(st, G_WINDOW) = 1;
    U8(st, G_REDRAW) = 1;
}

static int message_goes_somewhere(game_state *st, uint32_t e, int16_t kind)
{
    /* the test of 0x004E23E0's loops: kind 1, or kind 7 with its figure's tile or its own tile
     * (+0x28 / +0x2C of the 0x30-byte record whose +0x32 is e + 0) positive */
    if (kind == 1) {
        return 1;
    }
    if (kind != 7) {
        return 0;
    }
    return message_figure_tile(st, S32(st, e - (MSG_ID - MSG_PARAM_A))) > 0 || S32(st, e - (MSG_ID - MSG_PARAM_B)) > 0;
}

static void message_next_important(game_state *st)
{
    /* @bin 0x004E23E0 the sidebar's next-message button: 0x004E1640; the messages (0x30 bytes, +0x32
     * the text index, i16 +0x30 its year, from 0x00E95232 below 0x00EA0D82) of this year or the last
     * (0x0078D7BC - 1) that go somewhere counted into 0x00E951D8; none: 0x00E951D4 = 0; else the
     * next one after 0x00E951D4 (wrapping) centres the city view (0x00430CD0) and the city comes
     * back */
    GAME_CALL0(st, FN_MESSAGE_SORT);
    S32(st, G_UE951D8) = 0;
    for (uint32_t e = MESSAGE_BASE + MSG_ID; e < MESSAGE_ADDR(MESSAGE_COUNT) - 0x26u; e += 0x30) {
        int16_t text = S16(st, e);
        if (text == 0 || S16(st, e - (MSG_ID - MSG_YEAR)) < S32(st, G_YEAR) - 1) {
            continue;
        }
        int16_t kind = S16(st, G_MESSAGE_TYPES + MSGTYPE_U0A + (uint32_t) (int32_t) text * 0x50u);
        if (message_goes_somewhere(st, e, kind)) {
            S32(st, G_UE951D8) = S32(st, G_UE951D8) + 1;
        }
    }
    int32_t count = S32(st, G_UE951D8);
    if (count <= 0) {
        S32(st, G_NEXT_MESSAGE_INDEX) = 0;
        return;
    }
    if (S32(st, G_NEXT_MESSAGE_INDEX) >= count) {
        S32(st, G_NEXT_MESSAGE_INDEX) = 0;
    }
    int32_t k = 0;
    for (uint32_t e = MESSAGE_BASE + MSG_ID; e < MESSAGE_ADDR(MESSAGE_COUNT) - 0x26u; e += 0x30) {
        int16_t text = S16(st, e);
        if (text == 0 || S16(st, e - (MSG_ID - MSG_YEAR)) < S32(st, G_YEAR) - 1) {
            continue;
        }
        int16_t kind = S16(st, G_MESSAGE_TYPES + MSGTYPE_U0A + (uint32_t) (int32_t) text * 0x50u);
        if (!message_goes_somewhere(st, e, kind)) {
            continue;
        }
        k++;
        int32_t cur = S32(st, G_NEXT_MESSAGE_INDEX);
        if (cur >= k) {
            continue;
        }
        int32_t tile = S32(st, e - (MSG_ID - MSG_PARAM_B));
        S32(st, G_NEXT_MESSAGE_INDEX) = cur + 1;
        if (kind == 7) {
            /* @bin 0x004E24E3 the figure's tile asked again; neither positive: the next message */
            tile = message_figure_tile(st, S32(st, e - (MSG_ID - MSG_PARAM_A)));
            if (tile <= 0) {
                tile = S32(st, e - (MSG_ID - MSG_PARAM_B));
                if (tile <= 0) {
                    continue;
                }
            }
        }
        GAME_CALL(st, FN_CAMERA_GO_TO, tile);
        U8(st, G_WINDOW) = 1;
        U8(st, G_REDRAW) = 1;
        return;
    }
}

static void msglist_scroll(game_state *st)
{
    /* @bin 0x004E2160 the message list (window 0x11, arrows 0x005C8118 / 0x005C8140): first row
     * 0x00E95120 of 0x00E9511C, p 0 up / p 1 down; 0x00E95118 cleared, the redraw */
    int32_t p = S32(st, G_PARAM1);
    if (p == 0) {
        int32_t v = S32(st, G_MESSAGE_SCROLL);
        if (v > 0) {
            S32(st, G_MESSAGE_SCROLL) = v - 1;
        }
    } else if (p == 1) {
        int32_t v = S32(st, G_MESSAGE_SCROLL);
        if (v < S32(st, G_MESSAGE_SCROLL_ROWS)) {
            S32(st, G_MESSAGE_SCROLL) = v + 1;
        }
    }
    S32(st, G_MSGLIST_DRAGGING) = 0;
    U8(st, G_REDRAW) = 1;
}

static void msglist_help(game_state *st)
{
    /* @bin 0x004E21B0 the message list's help button (0x005C7E98): message 0x22, the redraw */
    GAME_CALL(st, FN_MESSAGE_WINDOW_OPEN, 0x22, 0, 0);
    U8(st, G_REDRAW) = 1;
}

/* ---- front end: names (0x51), campaign map (0x35), mission end (0x43), cheats (0x55) ---- */

static void names_back(game_state *st)
{
    /* @bin 0x0040C810 window 0x51's back rectangle (0x005C47F0): the name window 0x36 again */
    S32(st, G_UE20998) = 0;
    U8(st, G_WINDOW) = 0x36;
    U8(st, G_REDRAW) = 1;
}

static void names_pick(game_state *st)
{
    /* @bin 0x0040C830 a name of window 0x51's list (0x005C4358): text p of group 0xC5 (0x004CC110,
     * its address in 0x00E20768) copied into the player's name 0x00E38E74; the text field 1
     * (0x004CC500) refreshed (0x004CC3C0, 0x004CC3E0); the pick kept in 0x006059FC */
    GAME_CALL(st, FN_TEXT_SEEK, 0xC5, S32(st, G_PARAM1));
    uint32_t src = U32(st, G_TEXT_CURSOR);
    uint32_t i = 0;
    do {
        P8(st, G_PLAYER_NAME + i) = P8(st, src + i);
    } while (P8(st, src + i++) != 0);
    GAME_CALL(st, FN_TEXT_BOX_FOCUS, 1);
    GAME_CALL0(st, FN_TEXT_BOX_CURSOR_START);
    GAME_CALL0(st, FN_TEXT_BOX_CURSOR_END);
    S32(st, G_NAME_PICK) = S32(st, G_PARAM1);
}

static void campaign_map_pick(game_state *st)
{
    /* @bin 0x00418350 window 0x35's button 0x005797B0: with a mission chosen (0x00E38EAA >= 0,
     * signed byte) window 0x34 opens on it (0x00E38EAB), the choice cleared, 0x0051F780(1) */
    if (U8(st, G_WINDOW) == 0x35) {
        int8_t m = S8(st, G_STEP_PREV);
        if (m >= 0) {
            U8(st, G_WINDOW) = 0x34;
            S8(st, G_STEP) = m;
            S8(st, G_STEP_PREV) = -1;
            GAME_CALL(st, FN_NOTHING, 1);
        }
    }
    U8(st, G_REDRAW) = 1;
}

static void mission_end_replay(game_state *st)
{
    /* @bin 0x004EA830 window 0x43's replay button (0x005C6664): a jump to 0x00418510 */
    GAME_CALL0(st, FN_NEWGAME_REPLAY);
}

static void cheat_ok(game_state *st)
{
    /* @bin 0x0041D3A0 the cheat window's OK (0x0057C560): the notice 0xDA the first time
     * (0x007027E0), then as Enter (0x0041D3F0) */
    if (U8(st, G_CHEATS_ON) != 0) {
        GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0xDA, 0, 0);
    }
    U8(st, G_CHEATS_ON) = 0;
    GAME_CALL0(st, FN_CHEAT_ENTER);
}

/* ---- port table ---- */

#define VOID0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) { (void) a; fn(st); return 0; }

static void popup_yes(game_state *st) { popup_answer(st, 1); }
static void popup_no(game_state *st) { popup_answer(st, 0); }

VOID0(run_popup_yes, popup_yes)
VOID0(run_popup_no, popup_no)
VOID0(run_open_route, empire_open_route)
VOID0(run_empire_help, empire_help)
VOID0(run_empire_close, empire_close)
VOID0(run_overlay_refresh, overlay_monuments_refresh)
VOID0(run_overlays_group, overlays_group)
VOID0(run_overlays_item, overlays_item)
VOID0(run_storage_scroll, storage_scroll)
VOID0(run_trade_scroll, trade_scroll)
VOID0(run_mm_toggle, mission_menu_toggle)
VOID0(run_mm_scroll, mission_menu_scroll)
VOID0(run_list53, list53_scroll)
VOID0(run_msg_scroll, msgwin_scroll)
VOID0(run_msg_back, msgwin_back)
VOID0(run_msg_close_help, msgwin_close_help)
VOID0(run_msg_go_to, msgwin_go_to)
VOID0(run_next_msg, message_next_important)
VOID0(run_list_scroll, msglist_scroll)
VOID0(run_list_help, msglist_help)
VOID0(run_names_back, names_back)
VOID0(run_names_pick, names_pick)
VOID0(run_campaign_pick, campaign_map_pick)
VOID0(run_end_replay, mission_end_replay)
VOID0(run_cheat_ok, cheat_ok)

static int32_t run_pick(game_state *st, const int32_t *a)
{
    empire_pick(st, (uint32_t) a[0], (uint32_t) a[1]);
    return 0;
}

static int32_t run_figure_tile(game_state *st, const int32_t *a)
{
    return message_figure_tile(st, a[0]);
}

const ported_function ui_button_ports[] = {
    {0x00426590u, "popup_yes", 0, 0, 0, run_popup_yes, 0},
    {0x004265B0u, "popup_no", 0, 0, 0, run_popup_no, 0},
    {0x00407A10u, "empire_pick", 2, 0, 0, run_pick, 0},
    {0x00409490u, "empire_open_route", 0, 0, 0, run_open_route, 0},
    {0x00409510u, "empire_help", 0, 0, 0, run_empire_help, 0},
    {0x00409520u, "empire_close (thunk)", 0, 0, 0, run_empire_close, 0},
    {0x0040A090u, "empire_close", 0, 0, 0, run_empire_close, 0},
    {0x00547A60u, "overlay_monuments_refresh", 0, 0, 0, run_overlay_refresh, 0},
    {0x00409DA0u, "overlays_group", 0, 0, 0, run_overlays_group, 0},
    {0x00409E00u, "overlays_item", 0, 0, 0, run_overlays_item, 0},
    {0x0040C6B0u, "storage_scroll", 0, 0, 0, run_storage_scroll, 0},
    {0x0040C760u, "trade_scroll", 0, 0, 0, run_trade_scroll, 0},
    {0x0041E0E0u, "mission_menu_toggle", 0, 0, 0, run_mm_toggle, 0},
    {0x0041F460u, "mission_menu_scroll", 0, 0, 0, run_mm_scroll, 0},
    {0x004C5400u, "list53_scroll", 0, 0, 0, run_list53, 0},
    {0x004C7E90u, "msgwin_scroll", 0, 0, 0, run_msg_scroll, 0},
    {0x004C7EE0u, "msgwin_back", 0, 0, 0, run_msg_back, 0},
    {0x004E2360u, "msgwin_close_help", 0, 0, 0, run_msg_close_help, 0},
    {0x004B80B0u, "message_figure_tile", 1, 32, 0, run_figure_tile, 0},
    {0x004E2380u, "msgwin_go_to", 0, 0, 0, run_msg_go_to, 0},
    {0x004E23E0u, "message_next_important", 0, 0, 0, run_next_msg, 0},
    {0x004E2160u, "msglist_scroll", 0, 0, 0, run_list_scroll, 0},
    {0x004E21B0u, "msglist_help", 0, 0, 0, run_list_help, 0},
    {0x0040C810u, "names_back", 0, 0, 0, run_names_back, 0},
    {0x0040C830u, "names_pick", 0, 0, 0, run_names_pick, 0},
    {0x00418350u, "campaign_map_pick", 0, 0, 0, run_campaign_pick, 0},
    {0x004EA830u, "mission_end_replay (thunk)", 0, 0, 0, run_end_replay, 0},
    {0x0041D3A0u, "cheat_ok", 0, 0, 0, run_cheat_ok, 0},
};
const int ui_button_ports_count = sizeof(ui_button_ports) / sizeof(ui_button_ports[0]);
