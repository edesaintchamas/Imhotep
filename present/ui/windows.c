/* Imhotep - the small windows: top menu, build menu, overlays menu, yes / no popup (roadmap 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Window 3 (a top menu open over the city), 6 (a build menu), 8 (the overlays menu), 0x3F (the
 * yes / no popup), 0x43, 5's entry point, and the button helpers they share (the editor's square
 * buttons 0x004256A0 / 0x00425E80, the layout registry's sizes 0x004D5ED0..0x004D5F60). */
#include "ui.h"

#include "draw/draw.h"
#include "draw/text.h"
#include "present_layout.h"
#include "sidebar/sidebar.h"
#include "sim/call.h"
#include "state/ported.h"


/* ---- the layout registry's sizes ---- */

int32_t layout_panel_left(game_state *st)
{
    /* @bin 0x004D5ED0 */
    return S32(st, G_MENU_BAR_WIDTH) + 0x14;
}

int32_t layout_panel_width(game_state *st)
{
    /* @bin 0x004D5EE0 */
    return S32(st, G_MENU_BAR_PIECES);
}

int32_t layout_text_group_width(game_state *st, int32_t group)
{
    /* @bin 0x004D5EF0 a menu's width (in 16-pixel pieces) by its text group: groups 1, 2, 3, 4, 7,
     * 10 read 0x00E945D0.. (set by the layout registry), others 10 */
    switch (group) {
    case 1: return S32(st, G_MENU_WIDTHS);
    case 2: return S32(st, G_GROUP_WIDTH_2);
    case 3: return S32(st, G_GROUP_WIDTH_3);
    case 4: return S32(st, G_GROUP_WIDTH_4);
    case 7: return S32(st, G_GROUP_WIDTH_7);
    case 10: return S32(st, G_GROUP_WIDTH_10);
    default: return 10;
    }
}

int32_t layout_menu_width(game_state *st, int32_t menu)
{
    /* @bin 0x004D5F60 menu 0..4's width (0x00E945D0 + 4 menu), else 10 */
    if (menu < 0 || menu > 4) {
        return 10;
    }
    return S32(st, G_MENU_WIDTHS + (uint32_t) menu * 4u);
}

/* ---- the editor's square buttons (entries of 0x18: i16 x, y, image, size at +6; +0x0C on,
 * +0x0D pressed, +0x0F kind) ---- */

void editor_buttons_draw(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count)
{
    /* @bin 0x004256A0 each button's image (+4): kinds 4, 5 one less while pressed (+0x0D), others
     * one more while on (+0x0C); the position kept in 0x00D3B5E0 / 0x00D3B5DC, the image in
     * 0x00D3B5C0; a pressed button sets 0x00E38DC8 */
    if (count <= 0) {
        return;
    }
    uint32_t e = table;
    for (int32_t k = count; k != 0; k--, e += 0x18) {
        int32_t bx = P16(st, e) + x;
        S32(st, G_BLIT_X) = bx;
        int32_t by = P16(st, e + 2) + y;
        S32(st, G_BLIT_Y) = by;
        uint8_t kind = P8(st, e + 0x0F);
        int32_t id;
        if (kind == 4 || kind == 5) {
            id = P16(st, e + 4);
            S32(st, G_GLYPH_LAST) = id;
            if (P8(st, e + 0x0D) != 0) {
                id = P16(st, e + 4) - 1;
                S32(st, G_GLYPH_LAST) = id;
            }
        } else {
            id = P16(st, e + 4);
            if (P8(st, e + 0x0C) != 0) {
                id++;
            }
            S32(st, G_GLYPH_LAST) = id;
        }
        image_draw(st, id, bx, by);
        if (P8(st, e + 0x0D) != 0) {
            U8(st, G_PRESENT_NOW) = 1;
        }
    }
}

void editor_buttons_hover(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count, int32_t base)
{
    /* @bin 0x00425E80 the square button under the mouse (size +6): 0x007056B0 := base + its index,
     * the mouse kept in 0x00E38E2C / 0x00E38E28 */
    int32_t mx = S32(st, G_MOUSE_X), my = S32(st, G_MOUSE_Y);
    uint32_t e = table;
    for (int32_t k = 0; k < count; k++, e += 0x18) {
        int32_t bx = P16(st, e), size = P16(st, e + 6);
        if (bx + x > mx || bx + size + x <= mx) {
            continue;
        }
        int32_t by = P16(st, e + 2);
        if (by + y > my || by + size + y <= my) {
            continue;
        }
        S32(st, G_HIT_X) = mx;
        S32(st, G_HIT_Y) = my;
        S32(st, G_SQUARE_BUTTON_HOVER) = k + base;
        return;
    }
}

/* ---- window 3: a top menu open ---- */

void menu_draw(game_state *st)
{
    /* @bin 0x00425430 the open menu: its panel (0x00536990, width from 0x004D5EF0 by the text
     * group, 20 count + 20 pixels high) and its items (entries of 0x10: i16 y, text index; the
     * hovered one lit with 0x00D3B510 set) */
    if (U8(st, G_WINDOW) != 3 || S32(st, G_MENU_TITLE) == 0) {
        return;
    }
    uint32_t m = U32(st, G_MENU_OPEN);
    U32(st, G_MENU_ITEM) = P32(st, m + 8);
    int32_t count = P16(st, m + 0x0C);
    int32_t x = P16(st, m);
    int32_t top = P16(st, m + 4);
    int32_t group = P16(st, m + 6);
    U16(st, G_UD3B5BE) = 0x0A;
    U16(st, G_UD3B5BC) = (uint16_t) ((count * 5 + 5) * 4);
    int32_t w = layout_text_group_width(st, group);
    U16(st, G_UD3B5BE) = (uint16_t) w;
    GAME_CALL(st, FN_PANEL_DRAW_PATTERN, x, top + 0x12, (int16_t) w, S16(st, G_UD3B5BC) / 16);
    for (int32_t k = 1; k <= count; k++) {
        uint32_t it = U32(st, G_MENU_ITEM);
        int32_t index = P16(st, it + 2);
        int32_t iy = P16(st, it) + top + 0x1E;
        if (k == S32(st, G_MENU_HOVER)) {
            S32(st, G_TEXT_OUTLINE) = 1;
            U16(st, G_UD3B5BE) = (uint16_t) w;
            U16(st, G_UD3B5BC) = 0x10;
            text_group_draw(st, group, index, x + 8, iy, S32(st, G_FONT_RED), 0xFAC1);
            S32(st, G_TEXT_OUTLINE) = 0;
        } else {
            text_group_draw(st, group, index, x + 8, iy, S32(st, G_FONT_LARGE), 0);
        }
        U32(st, G_MENU_ITEM) += 0x10;
    }
}

int32_t menu_item_at(game_state *st, uint32_t items, int32_t count, int32_t x, int32_t y)
{
    /* @bin 0x00426150 the open menu's item under the mouse (rows of 15 pixels, the menu's width from
     * 0x004D5F60): 1-based, else 0 */
    uint32_t e = items;
    for (int32_t k = 1; k <= count; k++, e += 0x10) {
        int32_t iy = P16(st, e) + y + 0x1E;
        if (x > S32(st, G_MOUSE_X)) {
            continue;
        }
        int32_t w = layout_menu_width(st, S32(st, G_MENU_TITLE) - 1);
        if (w * 16 + x <= S32(st, G_MOUSE_X)) {
            continue;
        }
        int32_t my = S32(st, G_MOUSE_Y);
        if (iy > my || iy + 0x0F <= my) {
            continue;
        }
        return k;
    }
    return 0;
}

void menu_close(game_state *st)
{
    /* @bin 0x00426090 back to the window under the menu */
    U8(st, G_WINDOW) = U8(st, G_MENU_PREV);
    U8(st, G_UE38DE1) = 2;
    U8(st, G_UE38DE0) = 1;
    S32(st, G_MENU_HOVER) = 0;
    S32(st, G_MENU_TITLE) = 0;
    S32(st, G_MENU_OPEN) = 0;
    S32(st, G_TOPBAR_REDRAW) = 1;
    U8(st, G_REDRAW) = 1;
}

void menu_forget(game_state *st)
{
    /* @bin 0x004260D0 */
    S32(st, G_MENU_HOVER) = 0;
    S32(st, G_MENU_TITLE) = 0;
    S32(st, G_MENU_OPEN) = 0;
    S32(st, G_TOPBAR_REDRAW) = 1;
    U8(st, G_REDRAW) = 1;
}

int32_t menu_input(game_state *st, uint32_t table, int32_t count)
{
    /* @bin 0x00425F20 window 3's input: a left press chooses the hovered item (its function +4 with
     * its parameter +0x0C in 0x00D3B198; one that sets 0x0070571C keeps the menu open) or closes the
     * menu; otherwise the title under the mouse opens its menu, and the hovered item is found */
    S32(st, G_MENU_REDRAW) = 0;
    if (U8(st, G_LEFT_PRESSED) != 0) {
        int32_t sel = S32(st, G_MENU_HOVER);
        if (sel == 0) {
            menu_close(st);
            return 1;
        }
        U8(st, G_UE38DE1) = 1;
        U8(st, G_UE38DE0) = 1;
        U8(st, G_WINDOW) = U8(st, G_MENU_PREV);
        uint32_t it = P32(st, U32(st, G_MENU_OPEN) + 8) + (uint32_t) sel * 16u - 16u;
        U32(st, G_MENU_ITEM) = it;
        S32(st, G_PARAM1) = (int32_t) P32(st, it + 0x0C);
        GAME_CALL0(st, P32(st, it + 4));
        if (S32(st, G_MENU_REDRAW) == 1) {
            U8(st, G_REDRAW) = 1;
            S32(st, G_TOPBAR_REDRAW) = 1;
            U8(st, G_WINDOW) = 3;
            return 1;
        }
        menu_forget(st);
        return 1;
    }
    int32_t title = GAME_CALL(st, FN_MENUBAR_HOVER, (int32_t) table, count);
    uint32_t m;
    if (title != S32(st, G_MENU_TITLE) && title != 0) {
        S32(st, G_MENU_TITLE) = title;
        U8(st, G_UE38DE1) = 1;
        U8(st, G_UE38DE0) = 1;
        S32(st, G_MENU_HOVER) = 0;
        m = (uint32_t) title * 16u + table - 16u;
        U32(st, G_MENU_OPEN) = m;
        S32(st, G_TOPBAR_REDRAW) = 1;
        U8(st, G_REDRAW) = 1;
    } else {
        m = U32(st, G_MENU_OPEN);
    }
    S32(st, G_MENU_HOVER) = menu_item_at(st, P32(st, m + 8), P16(st, m + 0x0C), P16(st, m), P16(st, m + 4));
    return 0;
}

/* ---- window 0x3F: the yes / no popup ---- */


void popup_draw(game_state *st)
{
    /* @bin 0x00425570 the popup's panel (30 x 10 pieces), its title and its message (group 5; a
     * message wider than 420 pixels as lines) */
    panel_draw(st, S32(st, G_POPUP_X), S32(st, G_POPUP_Y), 0x1E, 0x0A);
    text_group_draw_centred(st, 5, S32(st, G_POPUP_TEXT), S32(st, G_POPUP_X), S32(st, G_POPUP_Y) + 0x14, 0x1E0,
                            S32(st, G_FONT_TITLE), 0);
    if (text_group_width(st, 5, S32(st, G_POPUP_TEXT) + 1, S32(st, G_FONT_LARGE)) < 0x1A4) {
        text_group_draw_centred(st, 5, S32(st, G_POPUP_TEXT) + 1, S32(st, G_POPUP_X), S32(st, G_POPUP_Y) + 0x3C, 0x1E0,
                                S32(st, G_FONT_LARGE), 0);
    } else {
        text_group_draw_lines(st, 5, S32(st, G_POPUP_TEXT) + 1, S32(st, G_POPUP_X) + 0x1E, S32(st, G_POPUP_Y) + 0x3C,
                              0x1A4, S32(st, G_FONT_LARGE), 0);
    }
}

void popup_draw_buttons(game_state *st)
{
    /* @bin 0x00425640 per frame: the buttons 0x005C8168, or the "right click to close" line */
    int32_t n = S32(st, G_POPUP_BUTTONS);
    if (n == 0) {
        text_group_draw_centred(st, 0x0D, 1, S32(st, G_POPUP_X), S32(st, G_POPUP_Y) + 0x80, 0x1E0, S32(st, G_FONT_LARGE),
                                0);
        return;
    }
    GAME_CALL(st, FN_BUTTONS_DRAW, S32(st, G_POPUP_X), S32(st, G_POPUP_Y), G_POPUP_BUTTON_TABLE, n);
}

void popup_input(game_state *st)
{
    /* @bin 0x00426530 the buttons; with none, a right click calls the action and goes back */
    int32_t n = S32(st, G_POPUP_BUTTONS);
    U8(st, G_REDRAW_SIDEBAR) = 1;
    if (GAME_CALL(st, FN_BUTTONS_CLICK, S32(st, G_POPUP_X), S32(st, G_POPUP_Y), G_POPUP_BUTTON_TABLE, n) != 0 ||
        S32(st, G_POPUP_BUTTONS) != 0 || U8(st, G_RIGHT_RELEASED) == 0) {
        return;
    }
    GAME_CALL0(st, U32(st, G_POPUP_ACTION));
    U8(st, G_REDRAW) = 1;
    U8(st, G_WINDOW) = U8(st, G_POPUP_BACK);
}

/* ---- window 0x43 ---- */

void window43_draw(game_state *st)
{
    /* @bin 0x004191F0 a centred panel (34 x 15 pieces) with group 0x3E's title and text; its origin
     * kept in 0x00F53DB8 / 0x00F53DB4, the screen's margins to 1024 x 768 in 0x00D3B56C / 0x00D3B568 */
    int32_t x = S32(st, G_LAYOUT_DX) + 0x30;
    int32_t y = S32(st, G_LAYOUT_DY) + 0x80;
    int32_t w = S32(st, G_SCREEN_W), h = S32(st, G_SCREEN_H);
    S32(st, G_PIC_X) = (0x400 - w) / 2;
    S32(st, G_PANEL_X) = x;
    S32(st, G_PANEL_Y) = y;
    S32(st, G_PIC_Y) = (0x300 - h) / 2;
    S32(st, G_PIC_W) = w;
    S32(st, G_PIC_H) = h;
    panel_draw(st, x, y, 0x22, 0x0F);
    text_group_draw_centred(st, 0x3E, 0, S32(st, G_PANEL_X), S32(st, G_PANEL_Y) + 0x10, 0x220,
                            S32(st, G_FONT_TITLE), 0);
    text_group_draw_centred(st, 0x3E, 2, S32(st, G_PANEL_X), S32(st, G_PANEL_Y) + 0x2F, 0x220,
                            S32(st, G_FONT_LARGE), 0);
}

/* ---- window 5's full redraw ---- */

void message_window_redraw(game_state *st)
{
    /* @bin 0x004C5CC0 the message window: 0x004C6C70 (with a video, 0x00D3B180) or 0x004C5E60 */
    if (U32(st, G_MESSAGE_WINDOW_VIDEO) != 0) {
        GAME_CALL0(st, FN_MESSAGE_WINDOW_DRAW_VIDEO);
    } else {
        GAME_CALL0(st, FN_MESSAGE_WINDOW_DRAW);
    }
}

/* ---- window 6: a build menu ---- */


static int32_t menu_x(game_state *st)
{
    int32_t x = S32(st, G_SIDEBAR_X);
    if (S32(st, G_SIDEBAR_COLLAPSED) == 1) {
        x = S32(st, G_SIDEBAR_X_COLLAPSED);
    }
    if (S32(st, G_EDITOR) == 1) {
        x = S32(st, G_SIDEBAR_X);
    }
    return x;
}

static void menu_button(game_state *st, int32_t x, int32_t y, int32_t w, int32_t style)
{
    /* 0x004D6480 (package frontend): a button frame of w pieces at (x, y) */
    GAME_CALL(st, FN_LAYOUT_SMALL_BUTTON, 6, x, y, w, 1, style, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
}

static void menu_text(game_state *st, int32_t group, int32_t index, int32_t x, int32_t y, int32_t width, int32_t font,
                      int32_t colour)
{
    layout_group_centred(st, group, index, x, y, width, font, colour, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
}

void build_menu_draw(game_state *st)
{
    /* @bin 0x00520000 the open build menu's items (each the next available item, 0x00403F10), one
     * row of 24 pixels each; the hovered one (0x007056C8) lit. Most menus show their text group
     * 0x30 string (by menu, an offset from the item's index), the menu 0x25 group 0x40, 0x1C the
     * gods' names (group 0xC6, a god already honoured left out), the others the building's name
     * (group 0x1C, or for 0x24 the building's own type, group 0xBD) and its cost (0x00705EA4) */
    int32_t sx = menu_x(st);
    if (S32(st, G_MENU_COUNT) <= 0) {
        return;
    }
    int32_t r = -1;
    int32_t k = 0;
    do {
        r = GAME_CALL(st, FN_BUILD_MENU_NEXT_ITEM, S32(st, G_SUBMENU), r);
        int32_t kk = k + 1;
        int32_t hover = S32(st, G_HOVER);
        int32_t style = (hover != kk) + 1;
        int32_t colour = hover != kk ? 0x3EFA : 0xFAC1;
        int32_t font = hover == kk ? S32(st, G_FONT_ROW) : S32(st, G_FONT_LARGE);
        int32_t menu = S32(st, G_SUBMENU);
        int32_t row = k * 24;
        int32_t offset = 0;
        int simple = 1;
        switch (menu) {
        case 0x0E: offset = -1; break;
        case 0x0F: offset = 4; break;
        case 0x10: offset = 6; break;
        case 0x12: offset = 0x11; break;
        case 0x13: offset = 0x13; break;
        case 0x1A: offset = 0x16; break;
        case 0x1B: offset = 0x22; break;
        default: simple = 0; break;
        }
        if (simple) {
            menu_button(st, sx - 0xAA, row + S32(st, G_MENU_Y) + 0x6E, 0x0A, style);
            int32_t index = S32(st, G_MENU_TYPES_1 + (uint32_t) (r + S32(st, G_SUBMENU) * 30) * 4u) + offset;
            menu_text(st, 0x30, index, sx - 0xAA, row + S32(st, G_MENU_Y) + 0x71, 0xA0, font, colour);
        } else if (menu == 0x11) {
            menu_button(st, sx - 0xAA, row + S32(st, G_MENU_Y) + 0x6E, 0x0A, style);
            int32_t v = S32(st, G_MENU_TYPES_1 + (uint32_t) (r + S32(st, G_SUBMENU) * 30) * 4u);
            menu_text(st, 0x30, v > 8 ? v + 0x27 : v + 9, sx - 0xAA, row + S32(st, G_MENU_Y) + 0x71, 0xA0, font, colour);
        } else if (menu == 0x25) {
            menu_button(st, sx - 0xAA, row + S32(st, G_MENU_Y) + 0x6E, 0x0A, style);
            int32_t index;
            if (k != 0) {
                index = U8(st, G_U784CB0) + 0x66;
            } else if (U8(st, G_U784CB0) == 2) {
                index = 0x53;
            } else if (U8(st, G_U784CB0) == 1) {
                index = 0x52;
            } else {
                index = 0x54;
            }
            menu_text(st, 0x40, index, sx - 0xAA, row + S32(st, G_MENU_Y) + 0x71, 0xA0, font, colour);
        } else if (menu == 0x1C) {
            /* @bin 0x00520632 the gods (0x005CF0DC: the item's god, its name 0x00784DDE): a god whose
             * festival flag of the player is clear is left out when it is the city's first or second */
            int32_t god = S16(st, G_SCENARIO_GODS + U32(st, G_GOD_MENU_GODS + (uint32_t) r * 4u) * 2u);
            int skip = 0;
            if (r == 1) {
                if (god == S16(st, G_MONUMENT_SLOTS) && S32(st, CITY_BASE + CITY_MONUMENT_TYPES + U32(st, CURRENT_PLAYER_ADDR) * 0x49D8u) == 0) {
                    skip = 1;
                }
            } else if (r == 2) {
                if (god == S16(st, G_MONUMENT_SLOTS) && S32(st, CITY_BASE + CITY_MONUMENT_TYPES + U32(st, CURRENT_PLAYER_ADDR) * 0x49D8u) == 0) {
                    skip = 1;
                } else if (god == S16(st, G_MONUMENT_SLOT_2) &&
                           S32(st, CITY_BASE + CITY_MONUMENT_TYPES + 4 + U32(st, CURRENT_PLAYER_ADDR) * 0x49D8u) == 0) {
                    skip = 1;
                }
            }
            if (!skip && god != 0) {
                menu_button(st, sx - 0x12A, row + S32(st, G_MENU_Y) + 0x6E, 0x12, style);
                menu_text(st, 0xC6, god, sx - 0x10A, row + S32(st, G_MENU_Y) + 0x71, 0xB0, font, colour);
            }
        } else {
            int32_t w = layout_panel_width(st) + 7;
            int32_t left = layout_panel_left(st);
            menu_button(st, sx - left - 0x61, row + S32(st, G_MENU_Y) + 0x6E, w, style);
            if (S32(st, G_SUBMENU) == 0x24) {
                int32_t b = S32(st, CITY_BASE + CITY_TEMPLE_COMPLEX_ID + U32(st, CURRENT_PLAYER_ADDR) * 0x49D8u);
                int32_t x = sx - layout_panel_left(st) - 0x61;
                int32_t index = r + S16(st, BUILDING_BASE + BLD_TYPE + (uint32_t) b * 0x108u) * 2 - 0x82;
                menu_text(st, 0xBD, index, x, row + S32(st, G_MENU_Y) + 0x71, 0xB0, font, colour);
            } else {
                int32_t y = row + S32(st, G_MENU_Y) + 0x71;
                int32_t x = sx - layout_panel_left(st) - 0x59;
                menu_text(st, 0x1C, S32(st, G_MENU_TYPES_1 + (uint32_t) (r + S32(st, G_SUBMENU) * 30) * 4u), x, y, 0xB0,
                          font, colour);
            }
            int32_t type = S32(st, G_MENU_TYPES_1 + (uint32_t) (r + S32(st, G_SUBMENU) * 30) * 4u);
            if (type == 7) {
                type = 0x5A;
            }
            int32_t cost = S32(st, G_MODEL_BUILDING + (uint32_t) type * 40u);
            if (type != 0x39 && cost != 0) {
                layout_number_at(st, cost, 0x40, G_STR_DB_CAP, sx - 0x52, row + S32(st, G_MENU_Y) + 0x72, font, 0,
                                 S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
            }
        }
        k = kk;
    } while (k < S32(st, G_MENU_COUNT));
}

void build_menu_redraw(game_state *st)
{
    /* @bin 0x0051FFD0 window 6's full redraw: the sidebar, the city; with redraw 2 the menu too */
    GAME_CALL0(st, FN_SIDEBAR_DRAW);
    GAME_CALL0(st, FN_CITY_VIEW_DRAW_CURRENT);
    if (U8(st, G_REDRAW) >= 2) {
        S32(st, G_UF53F30) = 0;
        build_menu_draw(st);
    }
}

int32_t build_menu_input(game_state *st)
{
    /* @bin 0x00403E00 the build menu's rows (0x005C28A8): the hovered one in 0x007056C8; a left press
     * on one chooses its item (0x00405200 with the item + 1 in 0x00D3B198); returns it, else 0 */
    int32_t x0 = S32(st, G_SIDEBAR_X) - 0x102;
    int32_t x = x0;
    if (S32(st, G_SIDEBAR_COLLAPSED) == 1) {
        x = S32(st, G_SIDEBAR_X_COLLAPSED) - 0x102;
    }
    if (S32(st, G_EDITOR) == 1) {
        x = x0;
    }
    int32_t y = S32(st, G_MENU_Y) + 0x6E;
    x += 0xA1 - layout_panel_left(st);
    S32(st, G_HOVER) = rect_hit(st, x, y, G_BUILD_MENU_RECTS, S32(st, G_MENU_COUNT));
    int32_t r = -1;
    uint32_t e = G_BUILD_MENU_RECTS;
    for (int32_t k = 0; k < S32(st, G_MENU_COUNT); k++, e += 0x1C) {
        r = GAME_CALL(st, FN_BUILD_MENU_NEXT_ITEM, S32(st, G_SUBMENU), r);
        int32_t mx = S32(st, G_MOUSE_X);
        if (P16(st, e) + x > mx || P16(st, e + 4) + x <= mx) {
            continue;
        }
        int32_t my = S32(st, G_MOUSE_Y);
        if (P16(st, e + 2) + y > my || P16(st, e + 6) + y <= my || U8(st, G_LEFT_PRESSED) == 0) {
            continue;
        }
        S32(st, G_PARAM1) = r + 1;
        GAME_CALL0(st, FN_CONSTRUCTION_CHOOSE_BUILDING);
        return r + 1;
    }
    return 0;
}

/* ---- window 8: the overlays menu ---- */


void overlays_menu_draw(game_state *st)
{
    /* @bin 0x005208F0 the ten groups (names 0x005CF734, group 0x0E), the hovered one lit (all lit
     * while a group is open), and the open group's rows (names 0x005CF75C + 120 group) next to it */
    int32_t sx = S32(st, G_SIDEBAR_X);
    if (S32(st, G_SIDEBAR_COLLAPSED) == 1) {
        sx = S32(st, G_SIDEBAR_X_COLLAPSED);
    }
    int32_t x = sx - 0xAA;
    for (int32_t k = 0; k < 10; k++) {
        int32_t style = (S32(st, G_HOVER) != k + 1) + 1;
        if (S32(st, G_OVL_GROUP) > 0) {
            style = 2;
        }
        menu_button(st, x, k * 24 + S32(st, G_MENU_Y) + 0x22, 0x0A, style);
        menu_text(st, 0x0E, S32(st, G_OVL_GROUP_OVERLAYS + (uint32_t) k * 4u), x, k * 24 + S32(st, G_MENU_Y) + 0x25, 0xA0,
                  S32(st, G_FONT_LARGE), 0);
    }
    if (S32(st, G_OVL_GROUP) <= 0) {
        return;
    }
    image_draw(st, S32(st, G_STATUS_IMAGE), sx - 0xB9, S32(st, G_MENU_Y) + S32(st, G_OVL_ROW) * 24 + 0x28);
    int32_t x2 = sx - 0x15C;
    for (int32_t k = 0; k < S32(st, G_OVL_ROWS); k++) {
        int32_t style = (S32(st, G_HOVER_2) != k + 1) + 1;
        menu_button(st, x2, S32(st, G_MENU_Y) + (S32(st, G_OVL_ROW) + k) * 24 + 0x22, 0x0A, style);
        menu_text(st, 0x0E, S32(st, G_OVL_ITEMS + (uint32_t) (k + S32(st, G_OVL_GROUP) * 30) * 4u), x2,
                  S32(st, G_MENU_Y) + (S32(st, G_OVL_ROW) + k) * 24 + 0x25, 0xA0, S32(st, G_FONT_LARGE), 0);
    }
}

void overlays_menu_redraw(game_state *st)
{
    /* @bin 0x005208C0 window 8's full redraw: the sidebar, the city; with redraw 2 the menu too */
    GAME_CALL0(st, FN_SIDEBAR_DRAW);
    GAME_CALL0(st, FN_CITY_VIEW_DRAW_CURRENT);
    if (U8(st, G_REDRAW) >= 2) {
        S32(st, G_UF53F30) = 0;
        overlays_menu_draw(st);
    }
}

int32_t overlays_menu_hover(game_state *st)
{
    /* @bin 0x00409E40 a hovered group opens (its rows by 0x00405170), and stays open 500 ms of the
     * frame clock 0x00E92E0C after the mouse leaves the menu */
    int32_t hover = S32(st, G_HOVER);
    if (hover == 0 && S32(st, G_HOVER_2) == 0) {
        if (S32(st, G_FRAME_CLOCK) - S32(st, G_OVL_HOVER_TIME) > 0x1F4) {
            S32(st, G_OVL_GROUP) = 0;
            S32(st, G_OVL_ROWS) = 0;
        }
        return 0;
    }
    S32(st, G_OVL_HOVER_TIME) = S32(st, G_FRAME_CLOCK);
    if (hover == 0) {
        return 0;
    }
    int32_t group = S32(st, G_OVL_GROUP_IDS + (uint32_t) hover * 4u);
    S32(st, G_OVL_ROW) = hover - 1;
    S32(st, G_OVL_GROUP) = group;
    S32(st, G_OVL_ROWS) = GAME_CALL(st, FN_BUILD_MENU_ITEMS, group);
    return 0;
}

/* ---- port table ---- */

#define RUN0(name, call) \
    static int32_t name(game_state *st, const int32_t *a) { (void) a; call; return 0; }
RUN0(run_menu_draw, menu_draw(st))
RUN0(run_menu_close, menu_close(st))
RUN0(run_menu_forget, menu_forget(st))
RUN0(run_popup_draw, popup_draw(st))
RUN0(run_popup_buttons, popup_draw_buttons(st))
RUN0(run_popup_input, popup_input(st))
RUN0(run_w43, window43_draw(st))
RUN0(run_msg_redraw, message_window_redraw(st))
RUN0(run_bm_draw, build_menu_draw(st))
RUN0(run_bm_redraw, build_menu_redraw(st))
RUN0(run_ovl_draw, overlays_menu_draw(st))
RUN0(run_ovl_redraw, overlays_menu_redraw(st))
static int32_t run_menu_input(game_state *st, const int32_t *a) { return menu_input(st, (uint32_t) a[0], a[1]); }
static int32_t run_item_at(game_state *st, const int32_t *a) { return menu_item_at(st, (uint32_t) a[0], a[1], a[2], a[3]); }
static int32_t run_ebd(game_state *st, const int32_t *a)
{
    editor_buttons_draw(st, a[0], a[1], (uint32_t) a[2], a[3]);
    return 0;
}
static int32_t run_ebh(game_state *st, const int32_t *a)
{
    editor_buttons_hover(st, a[0], a[1], (uint32_t) a[2], a[3], a[4]);
    return 0;
}
static int32_t run_left(game_state *st, const int32_t *a) { (void) a; return layout_panel_left(st); }
static int32_t run_width(game_state *st, const int32_t *a) { (void) a; return layout_panel_width(st); }
static int32_t run_tgw(game_state *st, const int32_t *a) { return layout_text_group_width(st, a[0]); }
static int32_t run_mw(game_state *st, const int32_t *a) { return layout_menu_width(st, a[0]); }
static int32_t run_bm_input(game_state *st, const int32_t *a) { (void) a; return build_menu_input(st); }
static int32_t run_ovl_hover(game_state *st, const int32_t *a) { (void) a; return overlays_menu_hover(st); }

const ported_function ui_window_ports[] = {
    {0x00425430u, "menu_draw", 0, 0, 0, run_menu_draw, 0},
    {0x00425F20u, "menu_input", 2, 32, 0, run_menu_input, 0},
    {0x00426090u, "menu_close", 0, 0, 0, run_menu_close, 0},
    {0x004260D0u, "menu_forget", 0, 0, 0, run_menu_forget, 0},
    {0x00426150u, "menu_item_at", 4, 32, 0, run_item_at, 0},
    {0x00425570u, "popup_draw", 0, 0, 0, run_popup_draw, 0},
    {0x00425640u, "popup_draw_buttons", 0, 0, 0, run_popup_buttons, 0},
    {0x00426530u, "popup_input", 0, 0, 0, run_popup_input, 0},
    {0x004191F0u, "window43_draw", 0, 0, 0, run_w43, 0},
    {0x004256A0u, "editor_buttons_draw", 4, 0, 0, run_ebd, 0},
    {0x00425E80u, "editor_buttons_hover", 5, 0, 0, run_ebh, 0},
    {0x004D5ED0u, "layout_panel_left", 0, 32, 0, run_left, 0},
    {0x004D5EE0u, "layout_panel_width", 0, 32, 0, run_width, 0},
    {0x004D5EF0u, "layout_text_group_width", 1, 32, 0, run_tgw, 0},
    {0x004D5F60u, "layout_menu_width", 1, 32, 0, run_mw, 0},
    {0x004C5CC0u, "message_window_redraw", 0, 0, 0, run_msg_redraw, 0},
    {0x00520000u, "build_menu_draw", 0, 0, 0, run_bm_draw, 0},
    {0x0051FFD0u, "build_menu_redraw", 0, 0, 0, run_bm_redraw, 0},
    {0x00403E00u, "build_menu_input", 0, 32, 0, run_bm_input, 0},
    {0x005208F0u, "overlays_menu_draw", 0, 0, 0, run_ovl_draw, 0},
    {0x005208C0u, "overlays_menu_redraw", 0, 0, 0, run_ovl_redraw, 0},
    {0x00409E40u, "overlays_menu_hover", 0, 32, 0, run_ovl_hover, 0},
};
const int ui_window_ports_count = sizeof(ui_window_ports) / sizeof(ui_window_ports[0]);
