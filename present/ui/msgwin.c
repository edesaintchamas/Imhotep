/* Imhotep - window 5, the message window's buttons and input (roadmap 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The message window (help pages and messages, drawn by 0x004C5E60 / 0x004C6C70, package
 * msgempire): the message entry 0x00D3B124 indexes the table 0x00E209B8 (entries of 0x50: +0 i16
 * kind, +2 i16 sub kind, +0x0A i16 width, +0x0C i16 height in 16-pixel pieces); the window's origin
 * 0x00D3B160 / 0x00D3B15C, its text box 0x00D3B154 / 0x00D3B150 (0x00D3B14C, 0x00D3B148 its size in
 * pieces) with a scroll bar when 0x00D3B134 (its line count) is set; the scroll position
 * 0x00D3B130; the pages visited 0x00D3AAE0 (stacks 0x00D3AE04 / 0x00D3AAE4); the links of the text
 * 0x00D3A6EC (entries of 0x14 at 0x00D3A6F8: x0, x1, y0, y1, target). With a video (0x00D3B180) the
 * window has a fixed layout. 0x00D3A6E8 is the message's advisor (8 with options 0x00D3B186: the
 * option buttons 0x005E02C0, 0x004C5CE0). */
#include "ui.h"

#include "draw/text.h"
#include "sim/call.h"
#include "state/ported.h"

#define MSG_ENTRY 0x00D3B124u
#define MSG_X 0x00D3B160u
#define MSG_Y 0x00D3B15Cu
#define MSG_TEXT_X 0x00D3B154u
#define MSG_TEXT_Y 0x00D3B150u
#define MSG_TEXT_W 0x00D3B14Cu
#define MSG_TEXT_H 0x00D3B148u
#define MSG_LINES 0x00D3B134u
#define MSG_SCROLL 0x00D3B130u
#define MSG_VIDEO 0x00D3B180u
#define MSG_ADVISOR 0x00D3A6E8u
#define MSG_OPTIONS 0x00D3B186u
#define MSG_HISTORY 0x00D3AAE0u
#define MSG_TABLE 0x00E209B8u

static int msg_options(game_state *st)
{
    return S32(st, MSG_ADVISOR) == 8 && U16(st, MSG_OPTIONS) != 0;
}

static uint32_t advisor_button(game_state *st)
{
    /* the button to the advisor's page, by the message's advisor */
    switch (S32(st, MSG_ADVISOR)) {
    case 1: return 0x005C7EC0u;
    case 2: return 0x005C7F88u;
    case 3: return 0x005C7FB0u;
    case 4: return 0x005C7F38u;
    case 5: return 0x005C7EE8u;
    case 6: return 0x005C7FD8u;
    case 7: return 0x005C8050u;
    default: return 0x005C7E48u;
    }
}

static int32_t entry16(game_state *st, uint32_t field)
{
    return S16(st, MSG_TABLE + field + U32(st, MSG_ENTRY) * 0x50u);
}

static void draw(game_state *st, int32_t x, int32_t y, uint32_t table)
{
    GAME_CALL(st, 0x00425730u, x, y, (int32_t) table, 1); /* buttons_draw (sidebar) */
}

static int32_t click(game_state *st, int32_t x, int32_t y, uint32_t table)
{
    return GAME_CALL(st, 0x00425990u, x, y, (int32_t) table, 1); /* buttons_click (sidebar) */
}

static int32_t hover_click(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t base)
{
    GAME_CALL(st, 0x00425BC0u, x, y, (int32_t) table, 1, base); /* buttons_hover (sidebar) */
    return click(st, x, y, table);
}

void message_window_frame(game_state *st)
{
    /* @bin 0x004C7340 window 5 per frame: its buttons (back 0x005C7DA8 when pages were visited, the
     * advisor's page, 0x005C7E70 for sub kinds 1 and 7, close 0x005C7DD0, the scroll arrows
     * 0x005C7DF8 / 0x005C7E20) or the option buttons */
    U8(st, UI_REDRAW_BUTTONS) = 1;
    int options = msg_options(st);
    if (S32(st, MSG_VIDEO) != 0) {
        if (options) {
            GAME_CALL0(st, 0x004C5CE0u);
        } else {
            draw(st, S32(st, MSG_X) + 0x10, S32(st, MSG_Y) + 0x198, advisor_button(st));
            draw(st, S32(st, MSG_X) + 0x1B4, S32(st, MSG_Y) + 0x19A, 0x005C7DD0u);
        }
        U8(st, 0x00E38DC8u) = 0;
        return;
    }
    if (entry16(st, 0) == 0 && S32(st, MSG_HISTORY) > 0) {
        draw(st, S32(st, MSG_X) + 0x10, entry16(st, 0x0C) * 16 + S32(st, MSG_Y) - 0x24, 0x005C7DA8u);
        text_group_draw(st, 0x0C, 0, S32(st, MSG_X) + 0x34, entry16(st, 0x0C) * 16 + S32(st, MSG_Y) - 0x1F,
                        S32(st, 0x00F56F8Cu), 0);
    }
    if (entry16(st, 0) == 2) {
        if (options) {
            GAME_CALL0(st, 0x004C5CE0u);
        } else {
            draw(st, S32(st, MSG_X) + 0x10, entry16(st, 0x0C) * 16 + S32(st, MSG_Y) - 0x28, advisor_button(st));
            int16_t sub = (int16_t) entry16(st, 2);
            if (sub == 1 || sub == 7) {
                draw(st, S32(st, MSG_X) + 0x40, S32(st, MSG_TEXT_Y) + 0x24, 0x005C7E70u);
            }
        }
    }
    draw(st, entry16(st, 0x0A) * 16 + S32(st, MSG_X) - 0x26, entry16(st, 0x0C) * 16 + S32(st, MSG_Y) - 0x24,
         0x005C7DD0u);
    if (S32(st, MSG_LINES) != 0) {
        int32_t x = S32(st, MSG_TEXT_W) * 16 + S32(st, MSG_TEXT_X) - 1;
        draw(st, x, S32(st, MSG_TEXT_Y), 0x005C7DF8u);
        draw(st, S32(st, MSG_TEXT_W) * 16 + S32(st, MSG_TEXT_X) - 1, S32(st, MSG_TEXT_H) * 16 + S32(st, MSG_TEXT_Y) - 0x1A,
             0x005C7E20u);
    }
}

int32_t message_window_scroll_drag(game_state *st)
{
    /* @bin 0x004C7DB0 the left button held on the scroll bar: the scroll position follows the mouse
     * (0x004CDE60 / 0x004CDE40), the thumb at 0x00D3B128; 1 when it did */
    if (S32(st, MSG_LINES) <= 0 || U8(st, UI_LEFT_HELD) == 0) {
        return 0;
    }
    int32_t left = S32(st, MSG_TEXT_W) * 16 + S32(st, MSG_TEXT_X);
    int32_t mx = S32(st, UI_MOUSE_X);
    int32_t h = S32(st, MSG_TEXT_H) * 16 - 0x34;
    if (mx < left + 1 || mx > left + 0x29) {
        return 0;
    }
    int32_t top = S32(st, MSG_TEXT_Y), my = S32(st, UI_MOUSE_Y);
    if (my < top + 0x1A || my > top + h + 0x1A) {
        return 0;
    }
    int32_t pos = my - top - 0x0B;
    if (pos > h) {
        pos = h;
    }
    int32_t pct = GAME_CALL(st, 0x004CDE60u, pos, h);
    S32(st, MSG_SCROLL) = GAME_CALL(st, 0x004CDE40u, S32(st, MSG_LINES), pct);
    S32(st, 0x00D3B12Cu) = 1;
    S32(st, 0x00D3B128u) = pos - 0x19;
    GAME_CALL0(st, 0x004C8040u);
    if (S32(st, 0x00D3B128u) < 0) {
        S32(st, 0x00D3B128u) = 0;
    }
    U8(st, UI_REDRAW) = 2;
    return 1;
}

int32_t message_window_link_at(game_state *st)
{
    /* @bin 0x004C7F70 a left press on a link of the text: its target, else -1 */
    if (U8(st, UI_LEFT_PRESSED) == 0) {
        return -1;
    }
    int32_t n = S32(st, 0x00D3A6ECu);
    if (n <= 0) {
        return -1;
    }
    int32_t mx = S32(st, UI_MOUSE_X), my = S32(st, UI_MOUSE_Y);
    uint32_t e = 0x00D3A6F8u;
    for (int32_t k = 0; k < n; k++, e += 0x14) {
        if (mx >= S32(st, e) && mx <= S32(st, e + 4) && my >= S32(st, e + 8) && my <= S32(st, e + 12)) {
            return S32(st, e + 16);
        }
    }
    return -1;
}

void message_window_input(game_state *st)
{
    /* @bin 0x004C7770 window 5's input: right click closes (0x004C7F30) unless the message has
     * options; the buttons as drawn by 0x004C7340, the scroll bar, then a link opens its page (the
     * current one pushed on the history while it has room for 199) */
    int options = msg_options(st);
    if (U8(st, UI_RIGHT_RELEASED) != 0 && !options) {
        GAME_CALL0(st, 0x004C7F30u);
        return;
    }
    if (S32(st, MSG_VIDEO) != 0) {
        if (options) {
            U8(st, 0x00E38DC8u) = 0;
            S32(st, UI_HOVER_RECT) = rect_hit(st, S32(st, MSG_X), S32(st, MSG_Y), 0x005E02C0u, 3);
            GAME_CALL(st, 0x004261D0u, S32(st, MSG_X), S32(st, MSG_Y), 0x005E02C0, 3);
            return;
        }
        if (click(st, S32(st, MSG_X) + 0x10, S32(st, MSG_Y) + 0x198, advisor_button(st)) != 0 ||
            click(st, S32(st, MSG_X) + 0x1B4, S32(st, MSG_Y) + 0x19A, 0x005C7DD0u) != 0) {
            return;
        }
        U8(st, 0x00E38DC8u) = 0;
        return;
    }
    if (entry16(st, 0) == 0 &&
        hover_click(st, S32(st, MSG_X) + 0x10, entry16(st, 0x0C) * 16 + S32(st, MSG_Y) - 0x24, 0x005C7DA8u, 0x0A) != 0) {
        return;
    }
    int close_button = 1;
    if (entry16(st, 0) == 2) {
        if (options) {
            S32(st, UI_HOVER_RECT) = rect_hit(st, S32(st, MSG_X), S32(st, MSG_Y), 0x005E02C0u, 3);
            if (GAME_CALL(st, 0x004261D0u, S32(st, MSG_X), S32(st, MSG_Y), 0x005E02C0, 3) != 0) {
                return;
            }
            close_button = 0;
        } else {
            if (click(st, S32(st, MSG_X) + 0x10, entry16(st, 0x0C) * 16 + S32(st, MSG_Y) - 0x24, advisor_button(st)) != 0) {
                return;
            }
            int16_t sub = (int16_t) entry16(st, 2);
            if ((sub == 1 || sub == 7) &&
                hover_click(st, S32(st, MSG_X) + 0x40, S32(st, MSG_TEXT_Y) + 0x24, 0x005C7E70u, 0x0F) != 0) {
                return;
            }
        }
    } else if (options) {
        close_button = 0;
    }
    if (close_button &&
        hover_click(st, entry16(st, 0x0A) * 16 + S32(st, MSG_X) - 0x26, entry16(st, 0x0C) * 16 + S32(st, MSG_Y) - 0x24,
                    0x005C7DD0u, 0x0B) != 0) {
        return;
    }
    if (hover_click(st, S32(st, MSG_TEXT_W) * 16 + S32(st, MSG_TEXT_X) - 1, S32(st, MSG_TEXT_Y), 0x005C7DF8u, 0x0C) != 0 ||
        hover_click(st, S32(st, MSG_TEXT_W) * 16 + S32(st, MSG_TEXT_X) - 1,
                    S32(st, MSG_TEXT_H) * 16 + S32(st, MSG_TEXT_Y) - 0x1A, 0x005C7E20u, 0x0D) != 0) {
        return;
    }
    if (message_window_scroll_drag(st) != 0 || options) {
        return;
    }
    int32_t target = message_window_link_at(st);
    if (target < 0) {
        return;
    }
    int32_t n = S32(st, MSG_HISTORY);
    if (n < 0xC7) {
        S32(st, 0x00D3AE04u + (uint32_t) n * 4u) = S32(st, MSG_ENTRY);
        S32(st, 0x00D3AAE4u + (uint32_t) n * 4u) = S32(st, MSG_SCROLL);
        S32(st, MSG_HISTORY) = n + 1;
    }
    S32(st, MSG_ENTRY) = target;
    S32(st, MSG_SCROLL) = 0;
    S32(st, 0x00D3B138u) = 0;
    S32(st, 0x00D3B12Cu) = 0;
    GAME_CALL0(st, 0x004C8040u);
    U8(st, UI_REDRAW) = 1;
}

/* ---- port table ---- */

static int32_t run_frame(game_state *st, const int32_t *a) { (void) a; message_window_frame(st); return 0; }
static int32_t run_input(game_state *st, const int32_t *a) { (void) a; message_window_input(st); return 0; }
static int32_t run_drag(game_state *st, const int32_t *a) { (void) a; return message_window_scroll_drag(st); }
static int32_t run_link(game_state *st, const int32_t *a) { (void) a; return message_window_link_at(st); }

const ported_function ui_msgwin_ports[] = {
    {0x004C7340u, "message_window_frame", 0, 0, 0, run_frame, 0},
    {0x004C7770u, "message_window_input", 0, 0, 0, run_input, 0},
    {0x004C7DB0u, "message_window_scroll_drag", 0, 32, 0, run_drag, 0},
    {0x004C7F70u, "message_window_link_at", 0, 32, 0, run_link, 0},
};
const int ui_msgwin_ports_count = sizeof(ui_msgwin_ports) / sizeof(ui_msgwin_ports[0]);
