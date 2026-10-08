/* Imhotep - the three per-window dispatchers: full redraw, per-frame draw, input (roadmap 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Window ids and their functions: docs/presentation.md 3.2. The windows' functions are called
 * through GAME_CALL (ported ones dispatch to their port); the input blocks that the original
 * inlines into 0x004D8310 are ported here, whichever window they belong to (window 0x15's, the
 * empire editor's routes, in empire_editor.c). */
#include "ui.h"

#include "draw/draw.h"
#include "sidebar/sidebar.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"

#define DX() S32(st, UI_DX)
#define DY() S32(st, UI_DY)
#define AX() S32(st, UI_ADVISOR_X)
#define AY() S32(st, UI_ADVISOR_Y)
#define SW() S32(st, UI_SCREEN_W)
#define SH() S32(st, UI_SCREEN_H)

/* ---- rectangle tables ---- */

int32_t rect_hit(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count)
{
    /* 0x00426450 (package msgempire's port): the rectangle under the mouse, 1-based, else 0 */
    return GAME_CALL(st, 0x00426450u, x, y, (int32_t) table, count);
}

void rect_layout_apply(game_state *st, uint32_t table, uint32_t list)
{
    /* @bin 0x004D66F0 the layout registry's list of rectangle overrides (nodes: +0 index (1-based),
     * +8 x, +0x0C y, +0x10 w, +0x14 h, +0x18 next) written into the table's entries */
    for (uint32_t n = list; n != 0; n = P32(st, n + 0x18)) {
        uint32_t e = table + (uint32_t) P32(st, n) * 0x1Cu - 0x1Cu;
        P16(st, e) = (int16_t) P32(st, n + 8);
        P16(st, e + 2) = (int16_t) P32(st, n + 0x0C);
        P16(st, e + 4) = (int16_t) ((uint16_t) P32(st, n + 0x10) + (uint16_t) P32(st, n + 8));
        P16(st, e + 6) = (int16_t) ((uint16_t) P32(st, n + 0x14) + (uint16_t) P32(st, n + 0x0C));
    }
}

int32_t rect_highlight(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count, int32_t hit)
{
    /* @bin 0x004D6740 the first time a window's table is used (flag +0x10 of its registry entry
     * 0x00E94614 + 16 key), the registry's overrides are applied to it and 0 returned; then the hit
     * rectangle `hit` (from 0x00426450) is returned, and in layout mode 2 (0x00E94D98) it is drawn
     * framed over the panel, kept in 0x00E945F8..0x00E94604 / 0x00E94DA0 */
    (void) count;
    uint32_t key = (uint32_t) layout_window_key(st, U8(st, UI_WINDOW));
    if (S32(st, 0x00E94624u + key * 16u) == 0) {
        key = (uint32_t) layout_window_key(st, U8(st, UI_WINDOW));
        rect_layout_apply(st, table, U32(st, 0x00E9461Cu + key * 16u));
        key = (uint32_t) layout_window_key(st, U8(st, UI_WINDOW));
        S32(st, 0x00E94624u + key * 16u) = 1;
        return 0;
    }
    if (hit == 0) {
        return 0;
    }
    uint32_t e = table + (uint32_t) hit * 0x1Cu - 0x1Cu;
    if (S32(st, 0x00E94D98u) == 2) {
        panel_draw(st, 0, 0, 6, 6);
        S32(st, 0x00E94DA0u) = (int32_t) e;
        S32(st, 0x00E94DA4u) = hit;
        S32(st, 0x00E94600u) = P16(st, e);
        S32(st, 0x00E94604u) = P16(st, e + 2);
        S32(st, 0x00E94608u) = P16(st, e + 4) - P16(st, e);
        S32(st, 0x00E945F8u) = hit;
        S32(st, 0x00E9460Cu) = P16(st, e + 6) - P16(st, e + 2);
        S32(st, 0x00E945FCu) = 0;
        int32_t y0 = P16(st, e + 2), x0 = P16(st, e);
        panel_draw_frame(st, 0, x0 + x, y0 + y, P16(st, e + 4) - x0, P16(st, e + 6) - y0, 0xFFFF);
    }
    return hit;
}

static int32_t rect_input(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count, uint32_t slot)
{
    /* the inlined sequence of 0x004D8310: hit 0x00426450, highlight 0x004D6740 into `slot` (slot 0:
     * the result is not stored, as for advisor page 11, 0x004DC43A), click 0x004261D0 (its result) */
    int32_t hit = rect_hit(st, x, y, table, count);
    int32_t lit = rect_highlight(st, x, y, table, count, hit);
    if (slot != 0) {
        S32(st, slot) = lit;
    }
    return GAME_CALL(st, 0x004261D0u, x, y, (int32_t) table, count);
}

static int32_t click(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count)
{
    return GAME_CALL(st, 0x00425990u, x, y, (int32_t) table, count); /* buttons_click (sidebar) */
}

static void hover(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count, int32_t base)
{
    GAME_CALL(st, 0x00425BC0u, x, y, (int32_t) table, count, base); /* buttons_hover (sidebar) */
}

static int32_t hover_click(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count, int32_t base)
{
    hover(st, x, y, table, count, base);
    return click(st, x, y, table, count);
}

static int32_t editor_buttons(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count)
{
    return GAME_CALL(st, 0x00425C40u, x, y, (int32_t) table, count); /* buttons_editor_click (sidebar) */
}

static void editor_hover(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count, int32_t base)
{
    GAME_CALL(st, 0x00425E80u, x, y, (int32_t) table, count, base);
}

static void buttons_draw_at(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count)
{
    GAME_CALL(st, 0x00425730u, x, y, (int32_t) table, count); /* buttons_draw (sidebar) */
}

static void editor_buttons_draw(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count)
{
    GAME_CALL(st, 0x004256A0u, x, y, (int32_t) table, count);
}

static void close_to(game_state *st, uint8_t window)
{
    U8(st, UI_WINDOW) = window;
    U8(st, UI_REDRAW) = 1;
}

/* ---- full redraw 0x004D71C0 ---- */

void ui_redraw(game_state *st, int32_t force)
{
    /* @bin 0x004D71C0 the current window's full redraw, when forced or the redraw flag is set; the
     * window is 0x00D3A6C4 instead while 0x00D3A6E4 is set (help over a window); then the flag is
     * cleared. Switch tables 0x004D7574 (bytes) / 0x004D74B8 */
    if (force != 0) {
        U8(st, UI_REDRAW) = 1;
    } else if (U8(st, UI_REDRAW) == 0) {
        return;
    }
    uint32_t w = U32(st, 0x00D3A6E4u) != 0 ? U32(st, 0x00D3A6C4u) : U8(st, UI_WINDOW);
    if (w > 0x57) {
        U8(st, UI_REDRAW) = 0;
        return;
    }
    switch (w) {
    case 0x00: GAME_CALL(st, 0x00520AB0u, 0); break;
    case 0x01: case 0x03: case 0x16: case 0x18: case 0x19: case 0x1B: case 0x1C: case 0x1E: case 0x1F:
    case 0x20: case 0x25: case 0x27: case 0x46: case 0x49: case 0x4B:
        GAME_CALL0(st, 0x0051E990u);
        break;
    case 0x04: GAME_CALL0(st, 0x0041A850u); break;
    case 0x05: GAME_CALL0(st, 0x004C5CC0u); break;
    case 0x06: GAME_CALL0(st, 0x0051FFD0u); break;
    case 0x07: GAME_CALL0(st, 0x00520EA0u); break;
    case 0x08: GAME_CALL0(st, 0x005208C0u); break;
    case 0x09: GAME_CALL0(st, 0x0051E990u); GAME_CALL0(st, 0x004F8E60u); break;
    case 0x0D: GAME_CALL0(st, 0x00532340u); break;
    case 0x0F:
        GAME_CALL(st, 0x00530D70u, S32(st, 0x00605954u), S32(st, 0x0060594Cu), S32(st, 0x00605950u));
        break;
    case 0x10: GAME_CALL0(st, 0x005325F0u); break;
    case 0x11: GAME_CALL0(st, 0x0051E990u); GAME_CALL0(st, 0x004E1870u); break;
    case 0x14: GAME_CALL0(st, 0x0052D4D0u); break;
    case 0x15: GAME_CALL0(st, 0x0052D4E0u); break;
    case 0x17: GAME_CALL0(st, 0x0041C2D0u); break;
    case 0x23: GAME_CALL0(st, 0x004CAF10u); break;
    case 0x26: GAME_CALL0(st, 0x00525B90u); break;
    case 0x28:
        GAME_CALL0(st, 0x00520F40u);
        GAME_CALL0(st, 0x00559E10u);
        GAME_CALL0(st, 0x0055A0E0u);
        GAME_CALL(st, 0x0055AD30u, 1);
        break;
    case 0x2A:
        GAME_CALL0(st, 0x00520F40u);
        GAME_CALL0(st, 0x00521080u);
        GAME_CALL0(st, 0x005215D0u);
        GAME_CALL0(st, 0x00521800u);
        break;
    case 0x2B: GAME_CALL0(st, 0x00520F40u); GAME_CALL0(st, 0x00529320u); break;
    case 0x2C: GAME_CALL0(st, 0x00520F40u); GAME_CALL0(st, 0x00524E20u); break;
    case 0x2D: GAME_CALL0(st, 0x00520F40u); GAME_CALL0(st, 0x005250F0u); break;
    case 0x2E: GAME_CALL0(st, 0x00520F40u); GAME_CALL0(st, 0x005255A0u); break;
    case 0x2F: GAME_CALL0(st, 0x00516630u); break;
    case 0x30: GAME_CALL0(st, 0x004CB4D0u); break;
    case 0x33: GAME_CALL0(st, 0x004194C0u); break;
    case 0x34: GAME_CALL0(st, 0x0041AAA0u); break;
    case 0x35: case 0x3B: GAME_CALL0(st, 0x0041A180u); break;
    case 0x36: GAME_CALL0(st, 0x00520B60u); break;
    case 0x37: GAME_CALL0(st, 0x00417E80u); break;
    case 0x38: GAME_CALL0(st, 0x00417EB0u); break;
    case 0x39: GAME_CALL0(st, 0x00417EE0u); break;
    case 0x3A: GAME_CALL0(st, 0x00417F10u); break;
    case 0x3C: GAME_CALL0(st, 0x00520E00u); break;
    case 0x3D: GAME_CALL0(st, 0x0041E5C0u); break;
    case 0x3E: GAME_CALL0(st, 0x00534540u); break;
    case 0x3F: GAME_CALL0(st, 0x00425570u); break;
    case 0x43: GAME_CALL0(st, 0x004191F0u); break;
    case 0x4C: GAME_CALL0(st, 0x00520F40u); GAME_CALL0(st, 0x00537940u); break;
    case 0x4D: GAME_CALL0(st, 0x00524E20u); break; /* @bin 0x004D735A into case 0x2C, past 0x00520F40 */
    case 0x4F: GAME_CALL0(st, 0x00538050u); break;
    case 0x50: GAME_CALL0(st, 0x00538850u); break;
    case 0x51: GAME_CALL0(st, 0x00520D00u); break;
    case 0x52: GAME_CALL0(st, 0x00516080u); break;
    case 0x53: GAME_CALL0(st, 0x004C5660u); break;
    case 0x57: GAME_CALL0(st, 0x00520AF0u); break;
    default: break;
    }
    U8(st, UI_REDRAW) = 0;
}

/* ---- per-frame draw 0x004D75D0 ---- */

static void advisor_page_frame(game_state *st)
{
    /* @bin 0x004D7C28 window 7: its buttons, then the page's own per-frame part (0x00605A44) */
    U8(st, UI_REDRAW_BUTTONS) = 1;
    GAME_CALL0(st, 0x00521060u);
    buttons_draw_at(st, AX(), (S32(st, 0x00F53F00u) - 2) * 16 + AY(), 0x005C7B80u, 1);
    switch (S32(st, 0x00605A44u)) {
    case 1:
        editor_buttons_draw(st, AX(), AY(), 0x005C8D08u, 2);
        GAME_CALL0(st, 0x005215D0u);
        break;
    case 2: GAME_CALL0(st, 0x00522A60u); break;
    case 3: GAME_CALL0(st, 0x00523BB0u); break;
    case 4: GAME_CALL0(st, 0x00524840u); break;
    case 5: GAME_CALL0(st, 0x004F7770u); break;
    case 6: GAME_CALL0(st, 0x0055A0E0u); break;
    case 7: GAME_CALL0(st, 0x005271F0u); break;
    case 0x0B: GAME_CALL0(st, 0x005292B0u); break;
    case 0x0C: editor_buttons_draw(st, AX(), AY(), 0x005C8D38u, 2); break;
    case 0x0E: GAME_CALL0(st, 0x0052B6D0u); break;
    default: break;
    }
}

static void info_frame(game_state *st)
{
    /* @bin 0x004D77EC window 9 (building / figure info): its buttons 0x005C7B10 at the bottom of
     * the panel; the row depends on the building's type for the panels with a list */
    U8(st, UI_REDRAW_BUTTONS) = 1;
    GAME_CALL0(st, 0x004F94A0u);
    if (U32(st, 0x00EADCC0u) != 0) {
        uint32_t b = U32(st, 0x00EADD6Cu);
        int16_t type = S16(st, 0x007999E8u + b * 0x108u);
        int32_t n;
        if (type == 0x48 || type == 0xC1) {
            n = GAME_CALL0(st, 0x005027B0u);
        } else if (type == 0x46 || type == 0xBE) {
            n = 0;
            for (uint32_t p = 0x00F53E70u; p < 0x00F53F00u; p += 4) {
                if (GAME_CALL(st, 0x005013A0u, S32(st, p)) != 0) {
                    n++;
                }
            }
        } else {
            n = S32(st, 0x00F53DD8u);
        }
        if (n != 0) {
            int32_t rows = GAME_CALL(st, 0x004CDE80u, n * 4, 3);
            buttons_draw_at(st, S32(st, 0x00EADD90u), (rows + 7) * 16 - 8, 0x005C7B10u, 2);
        } else {
            buttons_draw_at(st, S32(st, 0x00EADD90u), 0x1B0, 0x005C7B10u, 2);
        }
        return;
    }
    int32_t y = S32(st, 0x00EADD84u) * 16 + S32(st, 0x00EADD8Cu) - 0x28;
    int32_t mode = S32(st, 0x00EADD50u);
    buttons_draw_at(st, S32(st, 0x00EADD90u), y, 0x005C7B10u, 2);
    if (mode != 6 && mode != 5 && S32(st, 0x00EADD50u) == 2) {
        GAME_CALL0(st, 0x00515BF0u);
    }
}

void ui_draw_frame(game_state *st)
{
    /* @bin 0x004D75D0 the current window's per-frame drawing; then, for all windows, the buttons
     * flag 0x00E38DC9 cleared, 0x00414C40 when 0x00E3908C is set, and 0x0051F780 */
    uint8_t w = U8(st, UI_WINDOW);
    switch (w) {
    case 0x00:
        if (U32(st, 0x00F55B88u) != 0 && GAME_CALLT(st, 0x00E92BF8u, 0x0055F5A1u, 0) == 0 &&
            U8(st, 0x00E38E64u) != 0) {
            GAME_CALLT(st, 0x00E92BF8u, 0x0055F449u, 0x005761AC, 0, 0); /* the main menu's music */
        }
        GAME_CALL0(st, 0x0051F780u);
        GAME_CALL0(st, 0x004180C0u);
        break;
    case 0x17: {
        GAME_CALL0(st, 0x0041BE10u);
        int32_t n = GAME_CALL0(st, 0x0041BB30u);
        if (n != 0) {
            U8(st, UI_REDRAW_BUTTONS) = 1;
            buttons_draw_at(st, DX(), DY(), 0x0057BE30u, n);
        }
        break;
    }
    case 0x04: break;
    case 0x3F: GAME_CALL0(st, 0x00425640u); break;
    case 0x01:
        if (S32(st, UI_EDITOR) == 0) {
            GAME_CALL0(st, 0x004DDDC0u);
            GAME_CALL0(st, 0x0051EC90u);
            GAME_CALL(st, 0x0051ED30u, 0, S32(st, UI_SIDEBAR_X) + 0x0B);
            GAME_CALL(st, 0x0051EFB0u, 0);
            GAME_CALL0(st, 0x0051F790u);
            GAME_CALL0(st, 0x0043E1A0u);
            GAME_CALL0(st, 0x00534610u);
            GAME_CALL0(st, 0x00534DF0u);
            GAME_CALL0(st, 0x00535080u);
            GAME_CALL0(st, 0x00536120u);
            GAME_CALL0(st, 0x004E1320u);
            GAME_CALL0(st, 0x004E61F0u);
        } else {
            if (S32(st, 0x00605A38u) == 0) {
                GAME_CALL0(st, 0x004DDDC0u);
            } else {
                GAME_CALL0(st, 0x004138D0u);
                editor_buttons_draw(st, DX() + 0x0C, DY() + 0x28, 0x005C8EB8u, 2);
            }
            GAME_CALL(st, 0x0051EFB0u, 0);
            GAME_CALL0(st, 0x00533E70u);
            GAME_CALL0(st, 0x00535080u);
        }
        break;
    case 0x03: GAME_CALL0(st, 0x004DDDC0u); GAME_CALL0(st, 0x00425430u); break;
    case 0x11: GAME_CALL0(st, 0x004E1A50u); break;
    case 0x06: GAME_CALL0(st, 0x004DDDC0u); GAME_CALL0(st, 0x00520000u); break;
    case 0x08: GAME_CALL0(st, 0x004DDDC0u); GAME_CALL0(st, 0x005208F0u); break;
    case 0x05: GAME_CALL0(st, 0x004C7340u); break;
    case 0x13:
        GAME_CALL(st, 0x00538FA0u, 0);
        if (U32(st, 0x00D3B180u) != 0) {
            GAME_CALL0(st, 0x004C7340u);
        }
        break;
    case 0x4A: {
        uint32_t bink = U32(st, 0x006FDC54u);
        if (bink != 0) {
            GAME_CALLT0(st, bink, 0x00413470u);
        }
        break;
    }
    case 0x12: break;
    case 0x23:
        if (S32(st, 0x00D3A6C4u) == 1) {
            GAME_CALL0(st, 0x004DDDC0u);
        }
        GAME_CALL0(st, 0x004CAF10u);
        break;
    case 0x09: info_frame(st); break;
    case 0x02: GAME_CALL0(st, 0x0051FD60u); break;
    case 0x0A: GAME_CALL0(st, 0x00531190u); break;
    case 0x56: GAME_CALL0(st, 0x00531480u); break;
    case 0x45: GAME_CALL0(st, 0x00531BE0u); break;
    case 0x0B:
        GAME_CALL0(st, 0x00531EF0u);
        if (S32(st, 0x00F53F1Cu) > 0x14) {
            editor_buttons_draw(st, DX() + 0xD0, DY() + 0x3C, 0x005C8DF8u, 8);
        }
        break;
    case 0x0C:
        GAME_CALL0(st, 0x00531CC0u);
        if (S32(st, 0x00F53F20u) > 0x14) {
            editor_buttons_draw(st, DX() + 0x80, DY() + 0x28, 0x005C8D98u, 4);
        }
        break;
    case 0x0D: GAME_CALL0(st, 0x00532340u); break;
    case 0x10: GAME_CALL0(st, 0x005325F0u); break;
    case 0x16: GAME_CALL0(st, 0x005305E0u); break;
    case 0x19: GAME_CALL0(st, 0x00530780u); break;
    case 0x18: GAME_CALL0(st, 0x0044AAF0u); break;
    case 0x1B: GAME_CALL0(st, 0x00532FE0u); break;
    case 0x27: GAME_CALL0(st, 0x00448640u); break;
    case 0x46: GAME_CALL0(st, 0x00537260u); break;
    case 0x4B: GAME_CALL0(st, 0x005374A0u); break;
    case 0x1C: GAME_CALL0(st, 0x00533470u); break;
    case 0x25: GAME_CALL0(st, 0x00532920u); break;
    case 0x24: GAME_CALL0(st, 0x00534320u); break;
    case 0x4E: GAME_CALL0(st, 0x00537CF0u); break;
    case 0x14:
        U8(st, UI_REDRAW_BUTTONS) = 1;
        GAME_CALL0(st, 0x0052F8B0u);
        GAME_CALL0(st, 0x0052D4D0u);
        GAME_CALL0(st, 0x0052D940u);
        buttons_draw_at(st, 0x14, SH() - 0x2C, 0x005C7CE8u, 1);
        buttons_draw_at(st, SW() - 0x2C, SH() - 0x2C, 0x005C7D10u, 1);
        buttons_draw_at(st, 0x18, SH() - 0x6E, 0x005C7D38u, 1);
        break;
    case 0x15: {
        GAME_CALL0(st, 0x0052F8B0u);
        GAME_CALL0(st, 0x0052D940u);
        GAME_CALL0(st, 0x0052E6F0u);
        GAME_CALL0(st, 0x0052EC50u);
        GAME_CALL0(st, 0x0051F780u);
        int32_t mode = S32(st, 0x00775928u);
        if (mode == 1) {
            if (S32(st, 0x00775920u) == 0) {
                editor_buttons_draw(st, 0xDF, SH() - 0x72, 0x005C8C60u, 5);
            }
        } else if (mode == 2) {
            if (U8(st, 0x0077F748u + U32(st, 0x0077591Cu) * 0x62u) == 0) {
                editor_buttons_draw(st, 0xDF, SH() - 0x72, 0x005C8C60u, 5);
            }
        }
        break;
    }
    case 0x26:
        U8(st, UI_REDRAW_BUTTONS) = 1;
        buttons_draw_at(st, DX(), DY(), 0x005C7D60u, 2);
        break;
    case 0x0E: GAME_CALL(st, 0x005309B0u, S32(st, 0x00605974u), S32(st, 0x00605970u)); break;
    case 0x0F: case 0x48:
        GAME_CALL(st, 0x00530D70u, S32(st, 0x00605954u), S32(st, 0x0060594Cu), S32(st, 0x00605950u));
        break;
    case 0x07: advisor_page_frame(st); break;
    case 0x28:
        U8(st, UI_REDRAW_BUTTONS) = 1;
        GAME_CALL(st, 0x0055AD30u, 0);
        buttons_draw_at(st, AX(), AY(), 0x006035E0u, 2);
        if (S16(st, 0x00EA4096u + (U32(st, 0x00EAAC28u) * 0x24ECu + U32(st, 0x0060598Cu)) * 2u) == 2) {
            editor_buttons_draw(st, AX(), AY(), 0x00603580u, 2);
        }
        if (S16(st, 0x00EA4096u + (U32(st, 0x00EAAC28u) * 0x24ECu + U32(st, 0x0060598Cu)) * 2u) == 1) {
            editor_buttons_draw(st, AX(), AY(), 0x006035B0u, 2);
        }
        break;
    case 0x2A: GAME_CALL0(st, 0x005219C0u); break;
    case 0x2B:
        U8(st, UI_REDRAW_BUTTONS) = 1;
        GAME_CALL0(st, 0x00529530u);
        buttons_draw_at(st, AX(), AY(), 0x005C7BC8u, 4);
        break;
    case 0x52:
        U8(st, UI_REDRAW_BUTTONS) = 1;
        GAME_CALL0(st, 0x00516210u);
        buttons_draw_at(st, 0x1E, 0x1E, 0x005C7C58u, 4);
        break;
    case 0x2C: case 0x4D:
        U8(st, UI_REDRAW_BUTTONS) = 1;
        GAME_CALL0(st, 0x00524E20u);
        break;
    case 0x2D:
        U8(st, UI_REDRAW_BUTTONS) = 1;
        GAME_CALL0(st, 0x005253C0u);
        editor_buttons_draw(st, AX(), AY(), 0x005C8D68u, 2);
        break;
    case 0x2E: U8(st, UI_REDRAW_BUTTONS) = 1; GAME_CALL0(st, 0x00525720u); break;
    case 0x4C: U8(st, UI_REDRAW_BUTTONS) = 1; GAME_CALL0(st, 0x00537A50u); break;
    case 0x43: GAME_CALL0(st, 0x00419DB0u); break;
    case 0x33:
        if (U8(st, 0x00E38DD9u) != 1) {
            GAME_CALL0(st, 0x00419DB0u);
        }
        break;
    case 0x35: case 0x3B:
        U8(st, UI_REDRAW_BUTTONS) = 1;
        GAME_CALL0(st, 0x00418F70u);
        buttons_draw_at(st, S32(st, 0x006FDCA0u) + 0x234, S32(st, 0x006FDC9Cu) + 0x19A, 0x00579788u, 1);
        buttons_draw_at(st, S32(st, 0x006FDCA0u) + 0x6E, S32(st, 0x006FDC9Cu) + 0x1A0, 0x005C81B0u, 2);
        if (w == 0x35 && (uint8_t) GAME_CALLT0(st, 0x006FDD08u, 0x0041B8F0u) != 0) {
            buttons_draw_at(st, S32(st, 0x006FDCA0u) + 0x0A, S32(st, 0x006FDC9Cu) + 0x19C, 0x005797B0u, 1);
        }
        break;
    case 0x34:
        U8(st, UI_REDRAW_BUTTONS) = 1;
        if (U8(st, 0x007027A4u) != 0) {
            buttons_draw_at(st, S32(st, 0x006FDCA0u) + 0x244, S32(st, 0x006FDC9Cu) + 0x19A, 0x00579788u, 1);
        }
        GAME_CALL0(st, 0x00419140u);
        break;
    case 0x3D: {
        U8(st, UI_REDRAW_BUTTONS) = 1;
        int32_t n = GAME_CALL0(st, 0x0041E170u);
        int32_t table = GAME_CALL(st, 0x0041E100u, n);
        GAME_CALL(st, 0x00425730u, DX(), DY(), table, n);
        if ((uint8_t) GAME_CALL0(st, 0x0041E140u) != 0) {
            table = GAME_CALL(st, 0x0041E130u, 2);
            GAME_CALL(st, 0x00425730u, DX(), DY(), table, 2);
        }
        GAME_CALL0(st, 0x0041F220u);
        break;
    }
    case 0x2F: {
        U8(st, UI_REDRAW_BUTTONS) = 1;
        int32_t n = GAME_CALL0(st, 0x005165B0u);
        buttons_draw_at(st, DX(), DY(), 0x005F7EE0u, n);
        GAME_CALL0(st, 0x00516720u);
        break;
    }
    case 0x30: GAME_CALL0(st, 0x004CB5A0u); break;
    case 0x3E: GAME_CALL0(st, 0x005345A0u); break;
    case 0x36:
        U8(st, UI_REDRAW_BUTTONS) = 1;
        GAME_CALL0(st, 0x00520B90u);
        buttons_draw_at(st, DX() + 0x1D0, DY() + 0xF9, 0x00579788u, 1);
        if (rect_input(st, DX() + 0x160, DY() + 0xF6, 0x005C5AF8u, 1, UI_HOVER_RECT) != 0) {
            return; /* @bin 0x004D8178 past the common end */
        }
        if (U8(st, UI_RIGHT_RELEASED) != 0) {
            U8(st, UI_RIGHT_RELEASED) = 0;
            GAME_CALL0(st, 0x00516BB0u);
            return;
        }
        break;
    case 0x51: {
        int32_t x, y;
        int32_t res = S32(st, 0x00E38E68u); /* the resolution: 1 640x480, 2 800x600, 3 1024x768 */
        if (res == 1) {
            x = 0x6E;
            y = 0x1E;
        } else if (res == 2) {
            x = 0xBE;
            y = 0x5A;
        } else {
            x = 0x12E;
            y = 0xAE;
        }
        if (rect_input(st, x, y, 0x005C4358u, 0x2B, UI_HOVER_RECT) != 0) {
            return;
        }
        GAME_CALL0(st, 0x00520D00u);
        break;
    }
    case 0x1E: case 0x49: case 0x1F: case 0x20:
        GAME_CALL0(st, 0x004DDDC0u);
        GAME_CALL(st, 0x0051EFB0u, 0);
        GAME_CALL0(st, 0x0051F790u);
        break;
    case 0x53:
        GAME_CALL0(st, 0x004C5660u);
        buttons_draw_at(st, DX(), DY(), 0x005E01D0u, 2);
        break;
    case 0x55: GAME_CALL0(st, 0x0041D210u); break;
    default: break;
    }
    uint8_t refresh = U8(st, 0x00E3908Cu);
    U8(st, UI_REDRAW_BUTTONS) = 0;
    if (refresh != 0) {
        GAME_CALL0(st, 0x00414C40u);
    }
    GAME_CALL0(st, 0x0051F780u);
}

/* ---- input 0x004D8310 ---- */

static void city_mouse(game_state *st)
{
    /* @bin 0x004D8A44 the city view's mouse: the hovered tile 0x00405F80; off the map nothing;
     * a left press tries the legion / transport / warship under the mouse, then the construction */
    GAME_CALL0(st, 0x00405F80u);
    int32_t tile = S32(st, 0x00BCA2C4u);
    S32(st, 0x00605994u) = 0;
    if (tile == 0) {
        return;
    }
    if (U8(st, UI_LEFT_PRESSED) != 0) {
        if (GAME_CALL0(st, 0x0040B630u) != 0 || GAME_CALL0(st, 0x0040B790u) != 0 ||
            GAME_CALL0(st, 0x0040B6D0u) != 0) {
            return;
        }
        GAME_CALL0(st, 0x00405C20u);
    } else if (U8(st, UI_LEFT_HELD) != 0) {
        GAME_CALL0(st, 0x00405D90u);
    } else if (U8(st, UI_LEFT_RELEASED) != 0) {
        GAME_CALL0(st, 0x00405E90u);
    } else if (U8(st, UI_RIGHT_RELEASED) != 0) {
        if (GAME_CALL0(st, 0x00405BA0u) == 0) {
            GAME_CALL0(st, 0x004F7E50u);
        }
    }
}

static int32_t topbar_input(game_state *st)
{
    /* the menu bar's right button and the top bar's figures (right click: their help); 1 when done */
    if ((uint8_t) GAME_CALL0(st, 0x00425840u) != 0) {
        return 1;
    }
    GAME_CALL0(st, 0x00406250u);
    if (S32(st, 0x0070569Cu) != 0 && U8(st, UI_RIGHT_RELEASED) != 0) {
        GAME_CALL0(st, 0x004072C0u);
        return 1;
    }
    return 0;
}

static void city_input(game_state *st)
{
    /* @bin 0x004D865B window 1: menu bar, scrolling, minimap, sidebar, then the city's mouse */
    S32(st, 0x00E94E14u) = 0;
    uint32_t menus = S32(st, UI_EDITOR) == 1 ? 0x005C9230u : 0x005C91F0u;
    S32(st, 0x007056A8u) = GAME_CALL(st, 0x00426100u, (int32_t) menus, 4);
    if (GAME_CALL(st, 0x00426030u, (int32_t) menus, 4) != 0) {
        return;
    }
    if (S32(st, UI_EDITOR) == 0) {
        GAME_CALL0(st, 0x00417B40u);
    }
    GAME_CALL(st, 0x004DDDE0u, GAME_CALL0(st, 0x004CF660u));
    if (S32(st, UI_EDITOR) == 1) {
        int32_t count;
        if (S32(st, 0x00605A38u) > 0) {
            int32_t hit = rect_hit(st, DX() + 0x0C, DY() + 0x28, 0x005763D8u, 8);
            S32(st, UI_HOVER_RECT) = rect_highlight(st, DX() + 0x0C, DY() + 0x28, 0x005763D8u, 8, hit);
        } else {
            hover(st, S32(st, UI_SIDEBAR_X), 0x1E, 0x005C7760u, 0x0F, 1);
        }
        GAME_CALL0(st, 0x00406190u);
        if (GAME_CALL(st, 0x004061E0u, 0) != 0 || (uint8_t) GAME_CALL0(st, 0x00425840u) != 0) {
            return;
        }
        if (S32(st, 0x00605A38u) == 0) {
            count = 0x0F;
        } else {
            if (U8(st, UI_RIGHT_PRESSED) != 0) {
                S32(st, 0x00605A38u) = 0;
                U8(st, UI_REDRAW) = 1;
                return;
            }
            GAME_CALL(st, 0x004CC500u, 5);
            count = 2;
            if (rect_input(st, DX() + 0x0C, DY() + 0x28, 0x005763D8u, 0x0A, UI_HOVER_RECT) != 0 ||
                editor_buttons(st, DX() + 0x0C, DY() + 0x28, 0x005C8EB8u, 2) != 0) {
                return;
            }
        }
        if (click(st, S32(st, UI_SIDEBAR_X), 0x18, 0x005C7760u, count) != 0) {
            U8(st, UI_REDRAW_BUTTONS) = 1;
            return;
        }
        if (S32(st, 0x00605A38u) == 1) {
            return;
        }
        city_mouse(st);
        return;
    }
    int32_t collapsed = S32(st, UI_SIDEBAR_COLLAPSED);
    if (collapsed == 0) {
        GAME_CALL0(st, 0x00406190u);
        if (GAME_CALL(st, 0x004061E0u, 0) != 0 || topbar_input(st) != 0) {
            return;
        }
        int32_t x = S32(st, UI_SIDEBAR_X);
        if (hover_click(st, x, 0x1E, 0x005C7300u, 2, 0x0A) != 0) {
            U8(st, UI_REDRAW_BUTTONS) = 1;
            return;
        }
        hover(st, S32(st, UI_SIDEBAR_X), 0x1E, 0x005C7370u, 0x10, 0x1E);
        if (GAME_CALL(st, 0x004261D0u, S32(st, UI_SIDEBAR_X), 0x1E, 0x005C6CB8, 1) != 0) {
            return;
        }
        if (click(st, S32(st, UI_SIDEBAR_X), 0x1E, 0x005C7370u, 0x10) != 0) {
            U8(st, UI_REDRAW_BUTTONS) = 1;
            return;
        }
        if (hover_click(st, S32(st, UI_SIDEBAR_X), 0x1E, 0x005C7980u, 2, 0x32) != 0) {
            U8(st, UI_REDRAW_BUTTONS) = 1;
            return;
        }
    } else if (collapsed == 1) {
        if (topbar_input(st) != 0) {
            return;
        }
        if (hover_click(st, S32(st, UI_SIDEBAR_X_COLLAPSED), 0x18, 0x005C7348u, 1, 0x0A) != 0 ||
            hover_click(st, S32(st, UI_SIDEBAR_X_COLLAPSED), 0x18, 0x005C75B0u, 0x0C, 0x1E) != 0) {
            U8(st, UI_REDRAW_BUTTONS) = 1;
            return;
        }
    }
    city_mouse(st);
}

static void build_menu_input(game_state *st)
{
    /* @bin 0x004D8B30 window 6: right click closes; the menu's items 0x00403E00, then the sidebar */
    if (U8(st, UI_RIGHT_RELEASED) != 0) {
        close_to(st, 1);
        return;
    }
    if (GAME_CALL0(st, 0x00403E00u) != 0) {
        return;
    }
    int32_t r;
    if (S32(st, UI_EDITOR) == 1) {
        if (click(st, S32(st, UI_SIDEBAR_X), 0x1E, 0x005C7760u, S32(st, 0x00605A38u) != 0 ? 2 : 0x0F) != 0) {
            U8(st, UI_REDRAW_BUTTONS) = 1;
            return;
        }
    }
    int32_t collapsed = S32(st, UI_SIDEBAR_COLLAPSED);
    if (collapsed == 0) {
        if (GAME_CALL(st, 0x004261D0u, S32(st, UI_SIDEBAR_X), 0x1E, 0x005C6CB8, 1) != 0) {
            return;
        }
        r = click(st, S32(st, UI_SIDEBAR_X), 0x1E, 0x005C7370u, 0x10);
    } else if (collapsed == 1) {
        r = click(st, S32(st, UI_SIDEBAR_X_COLLAPSED), 0x18, 0x005C75B0u, 0x0C);
    } else {
        return;
    }
    if (r != 0) {
        U8(st, UI_REDRAW_BUTTONS) = 1;
    }
}

static void overlays_menu_input(game_state *st)
{
    /* @bin 0x004D8C08 window 8: right press closes; the overlay groups 0x005C2BF0, then the open
     * group's items 0x005C2D08 */
    if (U8(st, UI_RIGHT_PRESSED) != 0) {
        close_to(st, 1);
        return;
    }
    if (rect_input(st, S32(st, UI_SIDEBAR_X) - 0xAA, S32(st, 0x007056E8u) + 0x20, 0x005C2BF0u, 0x0A, UI_HOVER_RECT) != 0) {
        return;
    }
    GAME_CALL0(st, 0x00409E40u);
    if (S32(st, 0x007056DCu) == 0) {
        return;
    }
    rect_input(st, S32(st, UI_SIDEBAR_X) - 0x15C, S32(st, 0x007056E8u) + S32(st, 0x007056D0u) * 24 + 0x20,
               0x005C2D08u, S32(st, 0x007056D8u), UI_HOVER_RECT2);
}

static void empire_input(game_state *st)
{
    /* @bin 0x004D9A47 window 0x14, the empire map: scrolling, its three buttons, a left press on
     * the map picks the object under the mouse (0x00407A10), right press clears it; a trade city
     * (kind 1, 3, 5, not yet open) shows its open-route button 0x005C3DA0 */
    GAME_CALL(st, 0x00442730u, GAME_CALL0(st, 0x004CF660u));
    if (hover_click(st, 0x14, SH() - 0x2C, 0x005C7CE8u, 1, 0x0A) != 0 ||
        hover_click(st, SW() - 0x2C, SH() - 0x2C, 0x005C7D10u, 1, 0x0B) != 0 ||
        hover_click(st, 0x18, SH() - 0x6E, 0x005C7D38u, 1, 0x0C) != 0) {
        return;
    }
    if (U8(st, UI_LEFT_PRESSED) != 0) {
        int32_t mx = S32(st, UI_MOUSE_X), left = S32(st, 0x005D2A18u);
        if (mx >= left && mx < S32(st, 0x00E38D9Cu) - 2 * left) {
            int32_t my = S32(st, UI_MOUSE_Y), top = S32(st, 0x005D2A1Cu);
            if (my >= top && my < S32(st, 0x00E38D98u) - S32(st, 0x005D2A20u) - top) {
                GAME_CALL(st, 0x00407A10u, 0x00784F14, 0x005D2E88);
                U8(st, UI_REDRAW) = 1;
            }
        }
    }
    if (U8(st, UI_RIGHT_PRESSED) != 0) {
        S32(st, 0x00784F14u) = 0;
        S32(st, 0x005D2E88u) = -1;
        U8(st, UI_REDRAW) = 1;
    } else {
        int32_t obj = S32(st, 0x00784F14u);
        if (obj != 0 && U8(st, 0x0077F748u + (uint32_t) (obj - 1) * 0x62u) == 1) {
            int32_t city = GAME_CALL(st, 0x00444920u, obj - 1);
            S32(st, 0x00784F18u) = city;
            uint32_t c = (uint32_t) city * 0x6Au;
            uint8_t kind = U8(st, 0x0077DE02u + c);
            if ((kind == 1 || kind == 3 || kind == 5) && U8(st, 0x0077DE05u + c) == 0) {
                if (rect_input(st, (SW() - 0x1F4) / 2, SH() - 0x69, 0x005C3DA0u, 1, UI_HOVER_RECT2) != 0) {
                    return;
                }
            }
        }
    }
    if (U8(st, UI_RIGHT_RELEASED) != 0) {
        GAME_CALL0(st, 0x00409520u);
    }
}

static void advisors_input(game_state *st)
{
    /* @bin 0x004DBF4E window 7: the advisor buttons 0x005F8320, the help button, right click back
     * to the city (the page's flag +0x23C of the player cleared), then the page's own tables */
    int32_t ay = S32(st, UI_ADVISOR_Y) + 0x1B0;
    int32_t dx = DX();
    hover(st, dx, ay, 0x005F8320u, 0x0E, 0x0A);
    if (click(st, dx, ay, 0x005F8320u, 0x0E) != 0) {
        return;
    }
    hover(st, AX(), (S32(st, 0x00F53F00u) - 2) * 16 + AY(), 0x005C7B80u, 1, 0x19);
    if (click(st, AX(), (S32(st, 0x00F53F00u) - 2) * 16 + AY(), 0x005C7B80u, 1) != 0) {
        return;
    }
    if (U8(st, UI_RIGHT_RELEASED) != 0) {
        uint32_t p = U32(st, 0x00EAAC28u) * 0x49D8u;
        close_to(st, 1);
        U8(st, 0x00EA623Cu + p) = 0;
        return;
    }
    switch (S32(st, 0x00605A44u)) {
    case 1:
        if (rect_input(st, AX(), AY(), 0x005C4AB8u, 9, UI_HOVER_RECT) != 0) {
            return;
        }
        editor_hover(st, AX(), AY(), 0x005C8D08u, 2, 0x0A);
        editor_buttons(st, AX(), AY(), 0x005C8D08u, 2);
        return;
    case 2: rect_input(st, AX(), AY(), 0x005C4CD0u, 0x14, UI_HOVER_RECT); return;
    case 3: rect_input(st, AX(), AY(), 0x005C4F00u, 0x14, UI_HOVER_RECT); return;
    case 4: rect_input(st, AX(), AY(), 0x005C5368u, 8, UI_HOVER_RECT); return;
    case 5: rect_input(st, AX(), AY(), 0x005F7650u, 4, UI_HOVER_RECT); return;
    case 6: {
        int32_t n = S32(st, 0x00F53E6Cu);
        if (n > 0x0F) {
            n = 0x0F;
        }
        if (rect_input(st, AX(), AY(), 0x006032E8u, n + 3, UI_HOVER_RECT) != 0) {
            return;
        }
        GAME_CALL0(st, 0x0055BB90u);
        return;
    }
    case 7: rect_input(st, AX(), AY(), 0x005C5130u, 2, UI_HOVER_RECT); return;
    case 0x0B:
        if (rect_input(st, AX(), AY(), 0x005C5168u, 1, UI_HOVER_RECT) != 0) {
            return;
        }
        rect_input(st, AX(), AY(), 0x005C59E8u, 5, 0); /* @bin 0x004DC43A the highlight is not stored */
        return;
    case 0x0C:
        editor_hover(st, AX(), AY(), 0x005C8D38u, 2, 0x0A);
        editor_buttons(st, AX(), AY(), 0x005C8D38u, 2);
        return;
    case 0x0E: rect_input(st, AX(), AY(), 0x005C5798u, 8, UI_HOVER_RECT); return;
    case 0x0D: rect_input(st, AX(), AY(), 0x005C5878u, 0x0C, UI_HOVER_RECT); return;
    default: return;
    }
}

static void family_input(game_state *st)
{
    /* @bin 0x004DC56E window 0x28 (the family / kingdom page over the hall) */
    S32(st, UI_HOVER_RECT) = 0;
    S32(st, UI_HOVER_RECT2) = 0;
    S32(st, UI_HOVER_RECT3) = 0;
    if (U8(st, UI_RIGHT_RELEASED) != 0) {
        close_to(st, 7);
        return;
    }
    if (hover_click(st, AX(), AY(), 0x006035E0u, 2, 0x0A) != 0) {
        return;
    }
    int16_t mode = S16(st, 0x00EA4096u + (U32(st, 0x00EAAC28u) * 0x24ECu + U32(st, 0x0060598Cu)) * 2u);
    if (mode == 2) {
        editor_hover(st, AX(), AY(), 0x00603580u, 2, 0x0A);
        if (editor_buttons(st, AX(), AY(), 0x00603580u, 2) != 0) {
            return;
        }
    } else if (mode == 1) {
        editor_hover(st, AX(), AY(), 0x006035B0u, 2, 0x0A);
        if (editor_buttons(st, AX(), AY(), 0x006035B0u, 2) != 0) {
            return;
        }
    }
    if (rect_input(st, AX(), AY(), 0x006034E0u, 1, UI_HOVER_RECT) != 0 ||
        rect_input(st, AX(), AY(), 0x00603520u, 1, UI_HOVER_RECT2) != 0) {
        return;
    }
    if (S32(st, UI_HOVER_RECT2) == 0) {
        if (rect_input(st, AX(), AY(), 0x00603540u, 1, UI_HOVER_RECT2) != 0) {
            return;
        }
        if (S32(st, UI_HOVER_RECT2) != 0) {
            S32(st, UI_HOVER_RECT2)++;
        }
    }
    rect_input(st, AX(), AY(), 0x00603560u, 1, UI_HOVER_RECT3);
}

static void family_select_input(game_state *st)
{
    /* @bin 0x004DCDF0 window 0x33: the family list (table and count by 0x004EA880 / 0x004EA8D0) */
    int32_t x = S32(st, 0x00F53DB8u), y = S32(st, 0x00F53DB4u) - 0x70;
    int32_t n = GAME_CALL(st, 0x004EA880u, DX(), DY());
    int32_t table = GAME_CALL(st, 0x004EA8D0u, n);
    int32_t hit = rect_hit(st, x, y, (uint32_t) table, n);
    n = GAME_CALL0(st, 0x004EA880u);
    table = GAME_CALL(st, 0x004EA8D0u, n);
    S32(st, UI_HOVER_RECT) =
        rect_highlight(st, S32(st, 0x00F53DB8u), S32(st, 0x00F53DB4u) - 0x70, (uint32_t) table, n, hit);
    n = GAME_CALL0(st, 0x004EA880u);
    table = GAME_CALL(st, 0x004EA8D0u, n);
    GAME_CALL(st, 0x004261D0u, S32(st, 0x00F53DB8u), S32(st, 0x00F53DB4u) - 0x70, table, n);
}

static void mission_menu_input(game_state *st)
{
    /* @bin 0x004DCFCA window 0x3D */
    if (GAME_CALL0(st, 0x0041F4D0u) != 0) {
        return;
    }
    int32_t n = GAME_CALL0(st, 0x0041E170u);
    int32_t table = GAME_CALL(st, 0x0041E100u, n);
    if (click(st, DX(), DY(), (uint32_t) table, n) != 0) {
        return;
    }
    if ((uint8_t) GAME_CALL0(st, 0x0041E140u) != 0) {
        table = GAME_CALL(st, 0x0041E130u, 2);
        if (click(st, DX(), DY(), (uint32_t) table, 2) != 0) {
            return;
        }
    }
    if (S32(st, 0x00704DB0u) == 1) {
        table = GAME_CALL(st, 0x0041E120u, 1);
        int32_t hit = rect_hit(st, DX() + 0x168, DY() + 0x19A, (uint32_t) table, 1);
        table = GAME_CALL(st, 0x0041E120u, 1);
        S32(st, UI_HOVER_RECT2) = rect_highlight(st, DX() + 0x168, DY() + 0x19A, (uint32_t) table, 1, hit);
        table = GAME_CALL(st, 0x0041E120u, 1);
        /* @bin 0x004DD0B9 the click's rectangle is 10 pixels left of the hit's (0x15E, not 0x168) */
        if (GAME_CALL(st, 0x004261D0u, DX() + 0x15E, DY() + 0x19A, table, 1) != 0) {
            return;
        }
    }
    n = GAME_CALL(st, 0x0041F210u, DX(), DY());
    table = GAME_CALL(st, 0x0041E190u, n);
    int32_t hit = rect_hit(st, DX(), DY(), (uint32_t) table, n);
    n = GAME_CALL0(st, 0x0041F210u);
    table = GAME_CALL(st, 0x0041E190u, n);
    S32(st, UI_HOVER_RECT) = rect_highlight(st, DX(), DY(), (uint32_t) table, n, hit);
    n = GAME_CALL0(st, 0x0041F210u);
    table = GAME_CALL(st, 0x0041E190u, n);
    if (GAME_CALL(st, 0x004261D0u, DX(), DY(), table, n) != 0) {
        return;
    }
    if (U8(st, UI_RIGHT_RELEASED) != 0) {
        GAME_CALL0(st, 0x0041F650u);
    }
}

static void target_input(game_state *st, uint32_t clear, uint32_t on_click)
{
    /* @bin 0x004DD4B5 windows 0x1E, 0x49, 0x1F, 0x20 (a target for a legion / ships): the hovered
     * tile, the minimap, scrolling; right click back to the city (0x1F, 0x20 clear their figure
     * 0x00D05708 / 0x00D0570C); a left press picks the target */
    GAME_CALL0(st, 0x00405F80u);
    if (S32(st, UI_SIDEBAR_COLLAPSED) == 0) {
        GAME_CALL0(st, 0x00406190u);
        if (GAME_CALL(st, 0x004061E0u, 1) != 0) {
            return;
        }
    }
    GAME_CALL(st, 0x004DDDE0u, GAME_CALL0(st, 0x004CF660u));
    if (U8(st, UI_RIGHT_RELEASED) != 0) {
        U8(st, UI_WINDOW) = 1;
        if (clear != 0) {
            S32(st, clear) = 0;
        }
        GAME_CALL0(st, 0x004E2AB0u);
        U8(st, UI_REDRAW) = 1;
        return;
    }
    GAME_CALL0(st, 0x00405F80u);
    if (U8(st, UI_LEFT_PRESSED) != 0) {
        GAME_CALL0(st, on_click);
    }
}

static int32_t right_closes(game_state *st, uint8_t to)
{
    if (U8(st, UI_RIGHT_PRESSED) != 0) {
        close_to(st, to);
        return 1;
    }
    return 0;
}

static int32_t right_release_closes(game_state *st, uint8_t to)
{
    if (U8(st, UI_RIGHT_RELEASED) != 0) {
        close_to(st, to);
        return 1;
    }
    return 0;
}

static void video_start(game_state *st, uint32_t name, int32_t w, int32_t h, int32_t next)
{
    /* windows 0x44, 0x40, 0x41: the intro videos, centred, then window `next` */
    S32(st, 0x00F54118u) = 2;
    GAME_CALL0(st, 0x004CF1C0u);
    GAME_CALL(st, 0x00538DC0u, (int32_t) name, (SW() - w) / 2, (SH() - h) / 2, 1, next);
}

void ui_input(game_state *st)
{
    /* @bin 0x004D8310 the current window's input: the text field, the hover globals 0x007056A8 /
     * 0x007056B0 / 0x007056B8 cleared and the mouse kept in 0x00E38E2C / 0x00E38E28; a finished drag
     * (0x00E38E14 = 2) ends and redraws; then the window's handlers in the original's order */
    int32_t drag = S32(st, 0x00E38E14u);
    int32_t mx = S32(st, UI_MOUSE_X), my = S32(st, UI_MOUSE_Y);
    S32(st, 0x00E20990u) = 0;
    S32(st, 0x007056B8u) = 0;
    S32(st, 0x007056B0u) = 0;
    S32(st, 0x007056A8u) = 0;
    S32(st, 0x00E38E2Cu) = mx;
    S32(st, 0x00E38E28u) = my;
    if (drag == 2) {
        S32(st, 0x00E38E14u) = 0;
        U8(st, UI_REDRAW) = 1;
    }
    uint8_t w = U8(st, UI_WINDOW);
    switch (w) {
    case 0x00: rect_input(st, DX(), DY(), 0x0057BD88u, 5, UI_HOVER_RECT); return;
    case 0x17: {
        int32_t n = GAME_CALL0(st, 0x0041BB30u);
        if (n > 0 && click(st, DX(), DY(), 0x0057BE30u, n) != 0) {
            return;
        }
        if (rect_input(st, DX(), DY(), 0x0057BE78u, 0x0B, UI_HOVER_RECT) != 0 || U8(st, UI_RIGHT_RELEASED) == 0) {
            return;
        }
        if (S32(st, 0x006FDC98u) == 1 || S32(st, 0x007027B0u) == 0) {
            GAME_CALL0(st, 0x004CB6A0u);
        }
        return;
    }
    case 0x04: GAME_CALL0(st, 0x0041AA60u); return;
    case 0x3F: GAME_CALL0(st, 0x00426530u); return;
    case 0x44: video_start(st, 0x005E1E18u, 0x1F4, 0x124, 0x40); return;
    case 0x40: video_start(st, 0x005E1E08u, 0x1F4, 0x124, 0x41); return;
    case 0x41: video_start(st, 0x005E1DF8u, 0x280, 0x1E0, 0x3C); return;
    case 0x42:
        S32(st, 0x007028F4u) = 0;
        GAME_CALL0(st, 0x004C5A40u);
        GAME_CALL(st, 0x0041AA20u, 2, 0x33, 0x3E8);
        return;
    case 0x43: {
        int32_t n = 1;
        if (S8(st, 0x00E38EA8u) >= 2 || S32(st, 0x00E38EBCu) != 0 || U8(st, 0x00E38EA9u) != 0) {
            n = 3;
        }
        rect_input(st, S32(st, 0x00F53DB8u), S32(st, 0x00F53DB4u), 0x005C65D0u, n, UI_HOVER_RECT);
        return;
    }
    case 0x01: city_input(st); return;
    case 0x12:
        if (S32(st, UI_EDITOR) == 1 || S32(st, UI_SIDEBAR_COLLAPSED) != 0) {
            return;
        }
        GAME_CALL(st, 0x004261D0u, S32(st, UI_SIDEBAR_X), 0x18, 0x005C6CB8, 1);
        return;
    case 0x06: build_menu_input(st); return;
    case 0x08: overlays_menu_input(st); return;
    case 0x05: GAME_CALL0(st, 0x004C7770u); return;
    case 0x13:
        if (S32(st, 0x00D3B180u) != 0) {
            GAME_CALL0(st, 0x004C7770u);
            return;
        }
        if (U8(st, UI_RIGHT_RELEASED) != 0) {
            GAME_CALL0(st, 0x00539170u);
        }
        if (U8(st, UI_LEFT_RELEASED) != 0) {
            GAME_CALL0(st, 0x00539170u);
        }
        return;
    case 0x4A: {
        if (S32(st, 0x00D3B180u) != 0) {
            GAME_CALL0(st, 0x004C7770u);
            return;
        }
        uint32_t bink = U32(st, 0x006FDC54u);
        if (bink != 0 && (U8(st, UI_RIGHT_RELEASED) != 0 || U8(st, UI_LEFT_RELEASED) != 0)) {
            GAME_CALLT0(st, bink, 0x00413420u);
        }
        return;
    }
    case 0x09: GAME_CALL0(st, 0x004F96E0u); return;
    case 0x03: {
        uint32_t menus = S32(st, UI_EDITOR) == 1 ? 0x005C9230u : 0x005C91F0u;
        if (GAME_CALL(st, 0x00425F20u, (int32_t) menus, 4) != 0) {
            return;
        }
        if (U8(st, UI_RIGHT_RELEASED) != 0) {
            GAME_CALL0(st, 0x00426090u);
        }
        return;
    }
    case 0x11: GAME_CALL0(st, 0x004E1E10u); return;
    case 0x02: GAME_CALL(st, 0x004DDDE0u, GAME_CALL0(st, 0x004CF660u)); return;
    case 0x0A:
        if (!right_release_closes(st, 1)) {
            rect_input(st, DX(), DY(), 0x005C2E20u, 5, UI_HOVER_RECT);
        }
        return;
    case 0x56:
        if (!right_release_closes(st, 1)) {
            rect_input(st, DX(), DY(), 0x005C2EB0u, 0x0E, UI_HOVER_RECT);
        }
        return;
    case 0x45:
        if (U8(st, UI_RIGHT_RELEASED) != 0) {
            uint32_t p = U32(st, 0x00EAAC28u) * 0x49D8u;
            int32_t limit = S32(st, 0x00E39080u);
            close_to(st, 1);
            if (S32(st, 0x00EA6018u + p) > limit) {
                S32(st, 0x00EA6018u + p) = limit;
            }
            return;
        }
        editor_buttons(st, DX() + 0x120, DY() + 0x70, 0x005C8EE8u, 2);
        return;
    case 0x0B:
        if (right_release_closes(st, 1) || rect_input(st, DX(), DY(), 0x005C3070u, 6, UI_HOVER_RECT) != 0) {
            return;
        }
        editor_buttons(st, DX() + 0xD0, DY() + 0x3C, 0x005C8DF8u, 8);
        return;
    case 0x0C:
        if (right_release_closes(st, 1) || rect_input(st, DX(), DY(), 0x005C3038u, 2, UI_HOVER_RECT) != 0) {
            return;
        }
        editor_buttons(st, DX() + 0x80, DY() + 0x28, 0x005C8D98u, 4);
        return;
    case 0x0D:
    case 0x10:
        GAME_CALL(st, 0x004CC500u, w == 0x0D ? 2 : 4);
        if (S32(st, 0x00E20998u) != 0) {
            S32(st, 0x00E20998u) = 0;
            S32(st, 0x00D3B198u) = 1;
            GAME_CALL0(st, w == 0x0D ? 0x00402720u : 0x004099B0u);
        }
        if (w == 0x0D) {
            if (U8(st, UI_RIGHT_RELEASED) != 0) {
                U8(st, UI_RIGHT_RELEASED) = 0;
                close_to(st, U8(st, 0x00D3B1BCu));
                return;
            }
        } else if (right_closes(st, 1)) {
            return;
        }
        if (rect_input(st, DX(), DY(), 0x005C3118u, 0x0C, UI_HOVER_RECT) != 0 ||
            hover_click(st, DX(), DY(), w == 0x0D ? 0x005C79F0u : 0x005C7A80u, 4, 0x0A) != 0) {
            return;
        }
        GAME_CALL0(st, 0x004029D0u);
        return;
    case 0x16:
        if (!right_closes(st, 0x25)) {
            rect_input(st, DX() + 0xA0, DY() + 0x2C, 0x005C3368u, 2, UI_HOVER_RECT);
        }
        return;
    case 0x19:
        if (!right_closes(st, 1)) {
            rect_input(st, DX(), DY() + 0x50, 0x005C5E20u, 3, UI_HOVER_RECT);
        }
        return;
    case 0x18:
        if (U8(st, UI_RIGHT_PRESSED) != 0) {
            GAME_CALL0(st, 0x00448CF0u);
            return;
        }
        rect_input(st, DX(), DY(), 0x005D9E20u, 0x1E, UI_HOVER_RECT);
        return;
    case 0x1B:
        if (!right_closes(st, 1)) {
            rect_input(st, DX() + 0x10, DY() + 0x20, 0x005C5EB0u, 0x2F, UI_HOVER_RECT);
        }
        return;
    case 0x46:
        if (!right_closes(st, 1)) {
            rect_input(st, DX(), DY(), 0x005C6CD8u, 0x14, UI_HOVER_RECT);
        }
        return;
    case 0x4B:
        if (!right_closes(st, 0x1C)) {
            rect_input(st, DX(), DY(), 0x005C6F08u, 0x13, UI_HOVER_RECT);
        }
        return;
    case 0x27:
        if (right_closes(st, 1) || rect_input(st, DX(), DY(), 0x005D9A90u, 0x0E, UI_HOVER_RECT) != 0 ||
            hover_click(st, DX(), DY(), 0x005D9A48u, 2, 0x0A) != 0) {
            return;
        }
        GAME_CALL0(st, 0x004483F0u);
        return;
    case 0x1C:
        if (!right_closes(st, 1)) {
            rect_input(st, DX() + 0x10, DY() + 0x20, 0x005C63D8u, 0x12, UI_HOVER_RECT);
        }
        return;
    case 0x25:
        if (!right_closes(st, 1)) {
            rect_input(st, DX() + 0x0C, DY() + 0x0C, 0x005C5CD0u, 0x0C, UI_HOVER_RECT);
        }
        return;
    case 0x24:
        if (!right_closes(st, 0x15)) {
            rect_input(st, DX() + 0x10, DY() + 0x20, 0x005C3488u, 0x10, UI_HOVER_RECT);
        }
        return;
    case 0x4E:
        if (!right_closes(st, 0x15)) {
            rect_input(st, DX() + 0x10, DY(), 0x005C3648u, 0x45, UI_HOVER_RECT);
        }
        return;
    case 0x47: right_closes(st, 0x15); return;
    case 0x14: empire_input(st); return;
    case 0x15:
        empire_editor_input(st, mx, my);
        return;
    case 0x26: hover_click(st, DX(), DY(), 0x005C7D60u, 2, 0x0A); return;
    case 0x0E:
        if (U8(st, UI_RIGHT_PRESSED) != 0) {
            close_to(st, U8(st, 0x00605980u));
            return;
        }
        rect_input(st, S32(st, 0x00605974u), S32(st, 0x00605970u), 0x005C4208u, 0x0C, UI_HOVER_RECT);
        return;
    case 0x0F:
    case 0x48:
        if (U8(st, UI_RIGHT_PRESSED) != 0) {
            uint8_t back = U8(st, 0x00605958u);
            S32(st, 0x00605A58u) = 0;
            close_to(st, back);
            return;
        }
        rect_input(st, S32(st, 0x00605954u), S32(st, 0x0060594Cu), 0x00704DC8u, S32(st, 0x00605948u), UI_HOVER_RECT);
        return;
    case 0x07: advisors_input(st); return;
    case 0x28: family_input(st); return;
    case 0x2A:
        if (!right_release_closes(st, 7)) {
            rect_input(st, AX(), AY(), 0x005C4BB8u, S32(st, 0x00605984u) + 1, UI_HOVER_RECT);
        }
        return;
    case 0x2B:
        if (right_release_closes(st, 7) || hover_click(st, AX(), AY(), 0x005C7BC8u, 4, 0x0A) != 0) {
            return;
        }
        rect_input(st, AX(), AY(), 0x005C51A8u, 8, UI_HOVER_RECT);
        return;
    case 0x52:
        if (right_release_closes(st, 9) || hover_click(st, 0x1E, 0x1E, 0x005C7C58u, 4, 0x0A) != 0) {
            return;
        }
        rect_input(st, 0x1E, 0x1E, 0x005C5288u, 8, UI_HOVER_RECT);
        return;
    case 0x2C:
        if (!right_release_closes(st, 7)) {
            rect_input(st, AX(), AY(), 0x005C5468u, 0x0C, UI_HOVER_RECT);
        }
        return;
    case 0x4D:
        if (!right_release_closes(st, 9)) {
            rect_input(st, AX(), AY(), 0x005C5468u, 0x0C, UI_HOVER_RECT);
        }
        return;
    case 0x2D:
        if (right_release_closes(st, 7) || rect_input(st, AX(), AY(), 0x005C55B8u, 7, UI_HOVER_RECT) != 0) {
            return;
        }
        editor_hover(st, AX(), AY(), 0x005C8D68u, 2, 0x0A);
        editor_buttons(st, AX(), AY(), 0x005C8D68u, 2);
        return;
    case 0x2E:
        if (!right_release_closes(st, 7)) {
            rect_input(st, AX(), AY(), 0x005C5680u, 5, UI_HOVER_RECT);
        }
        return;
    case 0x4C:
        if (right_release_closes(st, 7) || rect_input(st, AX(), AY(), 0x005C5710u, 3, UI_HOVER_RECT) != 0) {
            return;
        }
        editor_hover(st, AX() + 0x130, AY() + 0xE9, 0x005C5768u, 2, 0x0A);
        editor_buttons(st, AX() + 0x130, AY() + 0xE9, 0x005C5768u, 2);
        editor_buttons_draw(st, AX() + 0x130, AY() + 0xE9, 0x005C5768u, 2);
        return;
    case 0x33:
        if (U8(st, 0x00E38DD9u) == 1) {
            if (U8(st, UI_RIGHT_RELEASED) != 0) {
                GAME_CALL(st, 0x0051F780u, 0);
                GAME_CALL(st, 0x0051F780u, 1);
                GAME_CALL0(st, 0x00418640u);
            }
            return;
        }
        family_select_input(st);
        return;
    case 0x35:
        if (click(st, S32(st, 0x006FDCA0u) + 0x234, S32(st, 0x006FDC9Cu) + 0x19A, 0x00579788u, 1) != 0 ||
            click(st, S32(st, 0x006FDCA0u) + 0x6E, S32(st, 0x006FDC9Cu) + 0x1A0, 0x005C81B0u, 2) != 0) {
            return;
        }
        if ((uint8_t) GAME_CALLT0(st, 0x006FDD08u, 0x0041B8F0u) != 0 &&
            click(st, S32(st, 0x006FDCA0u) + 0x0A, S32(st, 0x006FDC9Cu) + 0x19C, 0x005797B0u, 1) != 0) {
            return;
        }
        GAME_CALL0(st, 0x00418FE0u);
        return;
    case 0x34:
        if (U8(st, 0x007027A4u) != 0 &&
            click(st, S32(st, 0x006FDCA0u) + 0x244, S32(st, 0x006FDC9Cu) + 0x19A, 0x00579788u, 1) != 0) {
            return;
        }
        GAME_CALL0(st, 0x00419090u);
        return;
    case 0x3B:
        if (click(st, S32(st, 0x006FDCA0u) + 0x234, S32(st, 0x006FDC9Cu) + 0x19A, 0x00579788u, 1) != 0 ||
            click(st, S32(st, 0x006FDCA0u) + 0x6E, S32(st, 0x006FDC9Cu) + 0x1A0, 0x005C81B0u, 2) != 0) {
            return;
        }
        GAME_CALL0(st, 0x00418FE0u);
        return;
    case 0x3D: mission_menu_input(st); return;
    case 0x2F: {
        if (GAME_CALL0(st, 0x005168E0u) != 0) {
            return;
        }
        int32_t n = GAME_CALL0(st, 0x005165B0u);
        if (click(st, DX(), DY(), 0x005F7EE0u, n) != 0 ||
            rect_input(st, DX(), DY(), 0x005F7F28u, 0x10, UI_HOVER_RECT) != 0) {
            return;
        }
        if (U8(st, UI_RIGHT_RELEASED) != 0) {
            GAME_CALL0(st, 0x00402BB0u);
        }
        return;
    }
    case 0x30:
        if (rect_input(st, DX(), DY(), 0x005E0408u, 5, UI_HOVER_RECT) != 0) {
            return;
        }
        if (U8(st, UI_RIGHT_RELEASED) != 0) {
            GAME_CALL0(st, 0x004CB970u);
        }
        return;
    case 0x3E:
        if (rect_input(st, DX() + 0x50, DY() + 0x40, 0x005C68A8u, 1, UI_HOVER_RECT) != 0) {
            return;
        }
        /* @bin 0x004DD31E timeGetTime, compared signed with the screen's start 0x006057E8 + 10 s */
        if ((int32_t) platform_clock_ms(st) > (int32_t) (U32(st, 0x006057E8u) + 0x2710u)) {
            GAME_CALL0(st, 0x00402DE0u);
        }
        return;
    case 0x36:
        GAME_CALL(st, 0x004CC500u, 1);
        if (click(st, DX() + 0x1D0, DY() + 0xF9, 0x00579788u, 1) != 0 || S32(st, 0x00E20998u) == 0) {
            return;
        }
        GAME_CALL0(st, 0x00418220u);
        return;
    case 0x51:
        if (U8(st, UI_RIGHT_RELEASED) != 0) {
            S32(st, 0x00E20998u) = 0;
            close_to(st, 0x36);
        }
        return;
    case 0x3C:
        if (U8(st, UI_RIGHT_RELEASED) != 0 || U8(st, UI_LEFT_RELEASED) != 0) {
            close_to(st, 0);
        }
        return;
    case 0x37:
        if (U8(st, UI_RIGHT_RELEASED) != 0 || U8(st, UI_LEFT_PRESSED) != 0) {
            close_to(st, 0x38);
        }
        return;
    case 0x38:
    case 0x39:
        if (U8(st, UI_RIGHT_RELEASED) != 0 || U8(st, UI_LEFT_PRESSED) != 0) {
            U8(st, 0x00D3B43Du) = 1; /* the quit request */
            U8(st, UI_REDRAW) = 1;
        }
        return;
    case 0x3A:
        if (U8(st, UI_RIGHT_RELEASED) != 0 || U8(st, UI_LEFT_PRESSED) != 0) {
            close_to(st, 0);
        }
        return;
    case 0x1E: target_input(st, 0, 0x0040C440u); return;
    case 0x49: target_input(st, 0, 0x0040C570u); return;
    case 0x1F: target_input(st, 0x00D05708u, 0x0040BA40u); return;
    case 0x20: target_input(st, 0x00D0570Cu, 0x0040B830u); return;
    case 0x4F: {
        if (right_release_closes(st, 7)) {
            return;
        }
        int32_t k = S32(st, 0x00EA5FD8u + U32(st, 0x00EAAC28u) * 0x49D8u);
        if (U8(st, 0x006FDC88u + (uint32_t) k) == 0) {
            return;
        }
        hover(st, AX() + 0x20, AY() + 0x110, U32(st, 0x005F78F0u + (uint32_t) k * 4u), 1, k + 0x28);
        k = S32(st, 0x00EA5FD8u + U32(st, 0x00EAAC28u) * 0x49D8u);
        click(st, AX() + 0x20, AY() + 0x110, U32(st, 0x005F78F0u + (uint32_t) k * 4u), 1);
        return;
    }
    case 0x50: right_release_closes(st, 7); return;
    case 0x53:
        if (right_release_closes(st, 0) || GAME_CALL0(st, 0x004C57D0u) != 0) {
            return;
        }
        click(st, DX(), DY(), 0x005E01D0u, 2);
        return;
    case 0x54:
        rect_input(st, 0, 0, 0x005F8910u, S32(st, 0x00F535C8u), UI_HOVER_RECT);
        return;
    case 0x55: GAME_CALL0(st, 0x0041D310u); return;
    default: return;
    }
}

/* ---- port table ---- */

static int32_t run_redraw(game_state *st, const int32_t *a) { ui_redraw(st, a[0]); return 0; }
static int32_t run_frame(game_state *st, const int32_t *a) { (void) a; ui_draw_frame(st); return 0; }
static int32_t run_input(game_state *st, const int32_t *a) { (void) a; ui_input(st); return 0; }
static int32_t run_rect_highlight(game_state *st, const int32_t *a)
{
    return rect_highlight(st, a[0], a[1], (uint32_t) a[2], a[3], a[4]);
}
static int32_t run_layout_apply(game_state *st, const int32_t *a)
{
    rect_layout_apply(st, (uint32_t) a[0], (uint32_t) a[1]);
    return 0;
}

const ported_function ui_dispatch_ports[] = {
    {0x004D71C0u, "ui_redraw", 1, 0, 0, run_redraw, 0},
    {0x004D75D0u, "ui_draw_frame", 0, 0, 0, run_frame, 0},
    {0x004D8310u, "ui_input", 0, 0, 0, run_input, 0},
    {0x004D6740u, "rect_highlight", 7, 32, 0, run_rect_highlight, 0},
    {0x004D66F0u, "rect_layout_apply", 2, 0, 0, run_layout_apply, 0},
};
const int ui_dispatch_ports_count = sizeof(ui_dispatch_ports) / sizeof(ui_dispatch_ports[0]);
