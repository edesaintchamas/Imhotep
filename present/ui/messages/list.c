/* Imhotep - the message list window, id 0x11 (roadmap phase 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Full redraw 0x004E1870 (after the city 0x0051E990, from 0x004D71C0), per frame 0x004E1A50 (from
 * 0x004D75D0) and input 0x004E1E10 (from 0x004D8310). Drawing and the button tables go through the
 * ported primitives (present/draw, present/sidebar) by their addresses. */
#include "messages.h"

#include "draw/mem.h"
#include "draw/text.h"
#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"


/* @bin button tables of the window (0x004E1A50, 0x004E1E10) */

static int32_t x0(game_state *st) { return S32(st, G_MSGLIST_X); }
static int32_t y0(game_state *st) { return S32(st, G_MSGLIST_Y); }

/* @bin 0x004E1A34 (w * 16) / 2 by cdq / sar: rounds toward zero */
static int32_t half_width(game_state *st) { return (int32_t) (S32(st, G_MSGLIST_W) << 4) / 2; }

void msglist_redraw(game_state *st)
{
    /* @bin 0x004E1870 the layout from the 640x480 origin, the city behind, the panels, the title;
     * "no messages" when the list is empty, else the column titles and the help line */
    int32_t dy = S32(st, G_LAYOUT_DY);
    int32_t dx = S32(st, G_LAYOUT_DX);
    S32(st, G_MSGLIST_X) = dx;
    S32(st, G_MSGLIST_Y) = dy + 0x20;
    S32(st, G_MSGLIST_W) = 0x1E;
    S32(st, G_MSGLIST_H) = 0x16;
    S32(st, G_MSGLIST_IN_X) = dx + 0x10;
    S32(st, G_MSGLIST_IN_Y) = dy + 0x66;
    S32(st, G_MSGLIST_IN_W) = 0x1A;
    S32(st, G_MSGLIST_IN_H) = 0x0D;
    GAME_CALL0(st, FN_CITY_VIEW_DRAW_CURRENT);
    GAME_CALL(st, FN_PANEL_DRAW, x0(st), y0(st), S32(st, G_MSGLIST_W), S32(st, G_MSGLIST_H));
    GAME_CALL(st, FN_TEXT_GROUP_DRAW_CENTRED, 0x3F, 0, x0(st), y0(st) + 0x10, S32(st, G_MSGLIST_W) << 4, S32(st, G_FONT_TITLE), 0xFFFF);
    GAME_CALL(st, FN_PANEL_DRAW_INNER, S32(st, G_MSGLIST_IN_X), S32(st, G_MSGLIST_IN_Y), S32(st, G_MSGLIST_IN_W),
              S32(st, G_MSGLIST_IN_H));
    if (S32(st, G_MESSAGE_TOTAL) <= 0) {
        GAME_CALL(st, FN_TEXT_GROUP_DRAW_LINES, 0x3F, 1, S32(st, G_MSGLIST_IN_X) + 0x10, S32(st, G_MSGLIST_IN_Y) + 0x50,
                  (S32(st, G_MSGLIST_IN_W) - 3) << 4, S32(st, G_FONT_ROW), 0xFFFF);
        return;
    }
    GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x3F, 2, S32(st, G_MSGLIST_IN_X) + 0x2A, S32(st, G_MSGLIST_IN_Y) - 0x12,
              S32(st, G_FONT_LARGE), 0);
    GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x3F, 3, S32(st, G_MSGLIST_IN_X) + 0xC8, S32(st, G_MSGLIST_IN_Y) - 0x12,
              S32(st, G_FONT_LARGE), 0);
    GAME_CALL(st, FN_TEXT_GROUP_DRAW_LINES, 0x3F, 4, S32(st, G_MSGLIST_IN_X) + 0x28,
              (S32(st, G_MSGLIST_IN_H) << 4) + S32(st, G_MSGLIST_IN_Y) + 3, (S32(st, G_MSGLIST_IN_W) << 4) - 0x32,
              S32(st, G_FONT_LARGE), 0);
    GAME_CALL(st, FN_TEXT_GROUP_DRAW_CENTRED, 0x3F, 6, half_width(st) + x0(st) - 0x8C, (S32(st, G_MSGLIST_H) << 4) + y0(st) - 0x21,
              0x118, S32(st, G_FONT_LARGE), 0);
}

void msglist_draw(game_state *st)
{
    /* @bin 0x004E1A50 per frame: the help and close buttons; with messages, up to 10 rows from the
     * scroll position (icon by read flag and type kind, date, money / text of the row), the "delete
     * read" frame (lit when hovered), and with a scroll bar its buttons and thumb */
    U8(st, G_REDRAW_SIDEBAR) = 1;
    GAME_CALL(st, FN_BUTTONS_DRAW, x0(st) + 0x10, (S32(st, G_MSGLIST_H) << 4) + y0(st) - 0x2A, (int32_t) G_MSGLIST_HELP_BUTTON, 1);
    GAME_CALL(st, FN_BUTTONS_DRAW, (S32(st, G_MSGLIST_W) << 4) + x0(st) - 0x26, (S32(st, G_MSGLIST_H) << 4) + y0(st) - 0x24,
              (int32_t) G_MSGLIST_CLOSE_BUTTON, 1);
    int32_t count = S32(st, G_MESSAGE_TOTAL);
    if (count <= 0) {
        return;
    }
    int32_t shown = count > 10 ? 10 : count;
    int32_t index = S32(st, G_MESSAGE_SCROLL);
    if (shown > 0) {
        uint32_t m = MESSAGE_BASE + 0x30u * (uint32_t) index; /* record; the original walks +0x0E */
        int32_t dy = 0;
        int32_t row = 0;
        do {
            int32_t type = S16(st, m + 0x0A);
            S32(st, G_MESSAGE_SHOWN_ID) = type;
            /* @bin 0x004E1B13 the type's kind (+0x4A): 4 -> 4, 1 -> 2, else 0 */
            uint16_t kind = U16(st, MESSAGE_TYPES + 0x50u * (uint32_t) type + 0x4A);
            int32_t icon = kind == 4 ? 4 : (kind == 1 ? 2 : 0);
            if (U8(st, m + 0x0E) == 0 && icon != 4) {
                GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, S32(st, G_ICONS) + icon + 0x0E, S32(st, G_MSGLIST_IN_X) + 8,
                           dy + S32(st, G_MSGLIST_IN_Y) + 6);
            } else {
                GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, S32(st, G_ICONS) + icon + 0x0F, S32(st, G_MSGLIST_IN_X) + 0x0C,
                           dy + S32(st, G_MSGLIST_IN_Y) + 6);
            }
            row++;
            int32_t font = S32(st, G_HOVER) == row ? S32(st, G_FONT_RED) : S32(st, G_FONT_9C);
            S32(st, G_TEXT_X) = 0;
            GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x19, (int32_t) U8(st, m + 0x0F), S32(st, G_MSGLIST_IN_X) + 0x2A,
                      dy + S32(st, G_MSGLIST_IN_Y) + 8, font, 0);
            GAME_CALL(st, FN_TEXT_MONEY_DRAW, 0, (int32_t) S16(st, m + 0x08), S32(st, G_MSGLIST_IN_X) + 0x5A,
                      dy + S32(st, G_MSGLIST_IN_Y) + 8, font, 0);
            uint32_t text = U32(st, MESSAGE_TYPES + 0x50u * (uint32_t) S32(st, G_MESSAGE_SHOWN_ID) + 0x8C) + U32(st, G_MM_TEXT);
            int16_t sub = S16(st, m + 0x12);
            if (sub >= 0) {
                char buf[201];
                for (int k = 0; k < 201; k++) {
                    buf[k] = 0;
                }
                int32_t saved = S32(st, G_MESSAGE_CURRENT);
                S32(st, G_MESSAGE_CURRENT) = index;
                message_text_expand(st, sub, buf, 200);
                S32(st, G_MESSAGE_CURRENT) = saved;
                text_draw_host(st, buf, S32(st, G_MSGLIST_IN_X) + 0xC8, dy + S32(st, G_MSGLIST_IN_Y) + 8, font, 0);
            } else {
                GAME_CALL(st, FN_TEXT_DRAW, (int32_t) text, S32(st, G_MSGLIST_IN_X) + 0xC8, dy + S32(st, G_MSGLIST_IN_Y) + 8,
                          font, 0);
            }
            dy += 0x14;
            index++;
            m += 0x30;
        } while (row < shown);
    }
    int32_t colour = S32(st, G_HOVER_2) != 0 ? 0xF800 : 0;
    GAME_CALL(st, FN_PANEL_DRAW_FRAME, 0, half_width(st) + x0(st) - 0x8C, (S32(st, G_MSGLIST_H) << 4) + y0(st) - 0x28, 0x118,
              0x1C, colour);
    if (S32(st, G_MESSAGE_SCROLL_ROWS) <= 0) {
        return;
    }
    GAME_CALL(st, FN_BUTTONS_DRAW, (S32(st, G_MSGLIST_IN_W) << 4) + S32(st, G_MSGLIST_IN_X), S32(st, G_MSGLIST_IN_Y),
              (int32_t) G_MSGLIST_UP_BUTTON, 1);
    GAME_CALL(st, FN_BUTTONS_DRAW, (S32(st, G_MSGLIST_IN_W) << 4) + S32(st, G_MSGLIST_IN_X),
              (S32(st, G_MSGLIST_IN_H) << 4) + S32(st, G_MSGLIST_IN_Y) - 0x1A, (int32_t) G_MSGLIST_DOWN_BUTTON, 1);
    int32_t pct;
    int32_t s = S32(st, G_MESSAGE_SCROLL);
    if (s <= 0) {
        pct = 0;
    } else if (s >= S32(st, G_MESSAGE_SCROLL_ROWS)) {
        pct = 100;
    } else {
        pct = GAME_CALL(st, FN_CALC_RATIO_PERCENT, s, S32(st, G_MESSAGE_SCROLL_ROWS));
    }
    int32_t off = GAME_CALL(st, FN_CALC_PERCENTAGE, (S32(st, G_MSGLIST_IN_H) << 4) - 0x4D, pct);
    if (S32(st, G_MSGLIST_DRAGGING) != 0) {
        off = S32(st, G_MSGLIST_THUMB_Y);
    }
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, S32(st, G_SCROLL_IMAGES) + 0x27,
               (S32(st, G_MSGLIST_IN_W) << 4) + S32(st, G_MSGLIST_IN_X) + 9, S32(st, G_MSGLIST_IN_Y) + off + 0x1A);
}

int32_t buttons_hovered(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count)
{
    /* @bin 0x00426450 the 1-based entry (0x1C bytes: i16 left, top, right, bottom relative to (x, y))
     * under the mouse, remembered in 0x00E38E28 / 0x00E38E2C; 0 for none */
    if (count <= 0) {
        return 0;
    }
    int32_t mx = S32(st, G_MOUSE_X);
    int32_t my = S32(st, G_MOUSE_Y);
    uint32_t e = table;
    int32_t i = 0;
    do {
        if (P16(st, e) + x <= mx && P16(st, e + 4) + x > mx && P16(st, e + 2) + y <= my && P16(st, e + 6) + y > my) {
            S32(st, G_HIT_Y) = S32(st, G_MOUSE_Y);
            S32(st, G_HIT_X) = S32(st, G_MOUSE_X);
            return i + 1;
        }
        e += 0x1C;
        i++;
    } while (i < count);
    return 0;
}

int32_t msglist_scroll_drag(game_state *st)
{
    /* @bin 0x004E2090 left button held over the scroll bar: the scroll position from the mouse's
     * height, the thumb follows the mouse; 1 when it took the input */
    if (S32(st, G_MESSAGE_SCROLL_ROWS) <= 0) {
        return 0;
    }
    if (U8(st, G_LEFT_HELD) == 0) {
        return 0;
    }
    int32_t bx = (S32(st, G_MSGLIST_IN_W) << 4) + S32(st, G_MSGLIST_IN_X);
    int32_t range = (S32(st, G_MSGLIST_IN_H) << 4) - 0x34;
    int32_t mx = S32(st, G_MOUSE_X);
    if (mx < bx + 1) {
        return 0;
    }
    if (mx > bx + 0x29) {
        return 0;
    }
    int32_t iy = S32(st, G_MSGLIST_IN_Y);
    int32_t my = S32(st, G_MOUSE_Y);
    if (my < iy + 0x1A) {
        return 0;
    }
    if (my > iy + range + 0x1A) {
        return 0;
    }
    int32_t d = my - iy - 0x0B;
    if (d > range) {
        d = range;
    }
    int32_t pct = GAME_CALL(st, FN_CALC_RATIO_PERCENT, d, range);
    S32(st, G_MESSAGE_SCROLL) = GAME_CALL(st, FN_CALC_PERCENTAGE, S32(st, G_MESSAGE_SCROLL_ROWS), pct);
    S32(st, G_MSGLIST_DRAGGING) = 1;
    S32(st, G_MSGLIST_THUMB_Y) = d - 0x19;
    if (d - 0x19 < 0) {
        S32(st, G_MSGLIST_THUMB_Y) = 0;
    }
    U8(st, G_REDRAW) = 1;
    return 1;
}

void msglist_input(game_state *st)
{
    /* @bin 0x004E1E10 hover and click of the buttons (help, close, up, down), the rows (left: open,
     * right: delete; table 0x005C4830), "delete read" (0x005C4810), the scroll bar; a right click
     * elsewhere goes back to the city (window 1) */
    int32_t hx = x0(st) + 0x10, hy = (S32(st, G_MSGLIST_H) << 4) + y0(st) - 0x2A;
    GAME_CALL(st, FN_BUTTONS_HOVER, hx, hy, (int32_t) G_MSGLIST_HELP_BUTTON, 1, 10);
    if (GAME_CALL(st, FN_BUTTONS_CLICK, x0(st) + 0x10, (S32(st, G_MSGLIST_H) << 4) + y0(st) - 0x2A, (int32_t) G_MSGLIST_HELP_BUTTON, 1) != 0) {
        return;
    }
    GAME_CALL(st, FN_BUTTONS_HOVER, (S32(st, G_MSGLIST_W) << 4) + x0(st) - 0x26, (S32(st, G_MSGLIST_H) << 4) + y0(st) - 0x24,
              (int32_t) G_MSGLIST_CLOSE_BUTTON, 1, 0x0B);
    if (GAME_CALL(st, FN_BUTTONS_CLICK, (S32(st, G_MSGLIST_W) << 4) + x0(st) - 0x26, (S32(st, G_MSGLIST_H) << 4) + y0(st) - 0x24,
                  (int32_t) G_MSGLIST_CLOSE_BUTTON, 1) != 0) {
        return;
    }
    GAME_CALL(st, FN_BUTTONS_HOVER, (S32(st, G_MSGLIST_IN_W) << 4) + S32(st, G_MSGLIST_IN_X), S32(st, G_MSGLIST_IN_Y),
              (int32_t) G_MSGLIST_UP_BUTTON, 1, 0x0C);
    if (GAME_CALL(st, FN_BUTTONS_CLICK, (S32(st, G_MSGLIST_IN_W) << 4) + S32(st, G_MSGLIST_IN_X), S32(st, G_MSGLIST_IN_Y),
                  (int32_t) G_MSGLIST_UP_BUTTON, 1) != 0) {
        return;
    }
    GAME_CALL(st, FN_BUTTONS_HOVER, (S32(st, G_MSGLIST_IN_W) << 4) + S32(st, G_MSGLIST_IN_X),
              (S32(st, G_MSGLIST_IN_H) << 4) + S32(st, G_MSGLIST_IN_Y) - 0x1A, (int32_t) G_MSGLIST_DOWN_BUTTON, 1, 0x0C);
    if (GAME_CALL(st, FN_BUTTONS_CLICK, (S32(st, G_MSGLIST_IN_W) << 4) + S32(st, G_MSGLIST_IN_X),
                  (S32(st, G_MSGLIST_IN_H) << 4) + S32(st, G_MSGLIST_IN_Y) - 0x1A, (int32_t) G_MSGLIST_DOWN_BUTTON, 1) != 0) {
        return;
    }
    S32(st, G_HOVER) = buttons_hovered(st, S32(st, G_MSGLIST_IN_X), S32(st, G_MSGLIST_IN_Y) + 4, G_MSGLIST_ROWS_RECTS, 10);
    if (GAME_CALL(st, FN_BUTTONS_RECT_CLICK, S32(st, G_MSGLIST_IN_X), S32(st, G_MSGLIST_IN_Y) + 4, (int32_t) G_MSGLIST_ROWS_RECTS, 10) != 0) {
        return;
    }
    S32(st, G_HOVER_2) = buttons_hovered(st, half_width(st) + x0(st) - 0x8C, (S32(st, G_MSGLIST_H) << 4) + y0(st) - 0x28,
                                            G_MSGLIST_DELETE_READ_RECT, 1);
    if (GAME_CALL(st, FN_BUTTONS_RECT_CLICK, half_width(st) + x0(st) - 0x8C, (S32(st, G_MSGLIST_H) << 4) + y0(st) - 0x28,
                  (int32_t) G_MSGLIST_DELETE_READ_RECT, 1) != 0) {
        return;
    }
    if (msglist_scroll_drag(st) != 0) {
        return;
    }
    if (U8(st, G_RIGHT_RELEASED) != 0) {
        U8(st, G_WINDOW) = 1;
        U8(st, G_REDRAW) = 1;
    }
}

static int32_t run_redraw(game_state *st, const int32_t *a) { (void) a; msglist_redraw(st); return 0; }
static int32_t run_draw(game_state *st, const int32_t *a) { (void) a; msglist_draw(st); return 0; }
static int32_t run_input(game_state *st, const int32_t *a) { (void) a; msglist_input(st); return 0; }
static int32_t run_drag(game_state *st, const int32_t *a) { (void) a; return msglist_scroll_drag(st); }
static int32_t run_hovered(game_state *st, const int32_t *a)
{
    return buttons_hovered(st, a[0], a[1], (uint32_t) a[2], a[3]);
}

const ported_function msglist_ports[] = {
    {0x004E1870u, "msglist_redraw", 0, 0, 0, run_redraw, 0},
    {0x004E1A50u, "msglist_draw", 0, 0, 0, run_draw, 0},
    {0x004E1E10u, "msglist_input", 0, 0, 0, run_input, 0},
    {0x004E2090u, "msglist_scroll_drag", 0, 32, 0, run_drag, 0},
    {0x00426450u, "buttons_hovered", 4, 32, 0, run_hovered, 0},
};
const int msglist_ports_count = sizeof(msglist_ports) / sizeof(msglist_ports[0]);
