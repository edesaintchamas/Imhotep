/* Imhotep - tooltips: which help text the mouse asks for, after how long (roadmap phase 7.5).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Each frame 0x004C8750 picks the tooltip id 0x00D3A6E4 (text group 0x44) from what the mouse hovers
 * in the current window: the buttons' hover ids 0x007056B8 (button tables), 0x007056C8 (list rows),
 * 0x007056A8 / 0x0070569C / 0x007056A0 (the sidebar's other controls), set by the input dispatch.
 * The tooltip shows once the mouse has rested 150 ms of frame clock (0x00E92E0C against the
 * last-move time 0x00D3A6C8) with tooltips on (0x00E38EAC = 2). The kind 0x00D3A6C0 selects the
 * drawing (0x004CABD0): 0 a help text, 1 an overlay value (0x004C9750), 2 the city's ratings,
 * 3 the trade routes, 4 a figure's name. */
#include "sidebar.h"

#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

int32_t mouse_in(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h)
{
    /* @bin 0x004CF620 1 when the cursor (0x00E38E34, 0x00E38E30) is in [x, x + w) x [y, y + h) */
    int32_t mx = S32(st, G_MOUSE_X);
    if (x > mx || x + w <= mx) {
        return 0;
    }
    int32_t my = S32(st, G_MOUSE_Y);
    if (y > my || y + h <= my) {
        return 0;
    }
    return 1;
}

int32_t tooltip_family_hover(game_state *st)
{
    /* @bin 0x004C92A0 the family tree window's portraits: entries (a 0x00576104 - 0x44 + 4 k,
     * b 0x00576104 + 4 k), k < 17, at x 35 a + 0x15 (+ layout x), y 0x32 and 0x86 (+ layout y),
     * 24 x 24: returns a or b under the mouse (b only when not 0x1B and in 0..0x23), else 0 */
    for (uint32_t e = G_FAMILY_ROW_B; e <= G_FAMILY_ROW_B + 0x40u; e += 4) {
        int32_t a = S32(st, e - 0x44);
        int32_t b = S32(st, e);
        int32_t x = 35 * a;
        if (mouse_in(st, x + S32(st, G_LAYOUT_DX) + 0x15, S32(st, G_LAYOUT_DY) + 0x32, 0x18, 0x18) != 0) {
            return a;
        }
        if (b != 0x1B && b >= 0 && b < 0x24 &&
            mouse_in(st, x + S32(st, G_LAYOUT_DX) + 0x15, S32(st, G_LAYOUT_DY) + 0x86, 0x18, 0x18) != 0) {
            return b;
        }
    }
    return 0;
}

int32_t tooltip_empire_city_goods(game_state *st, uint8_t *buys)
{
    /* @bin 0x004C9340 the empire map's selected city (0x00784F18, records of 0x6A at 0x0077DDF8;
     * kinds 1, 3, 5 trade): the good under the mouse among those it sells (0x00443000) and buys
     * (0x00443040) that the player trades (0x00447790); *buys 0 sold, 1 bought. Two layouts by
     * the city's +0x0D: a grid of two columns 135 pixels apart (sold at x, bought 0x118 further,
     * rows of 20 from height - 0x5D), or two rows of icons 32 apart (text cursor 0x00E20778) at
     * height - 0x50 and - 0x3A */
    int32_t object = S32(st, G_SELECTED) - 1;
    uint32_t city = 0x6Au * U32(st, G_EMPIRE_SELECTED);
    uint8_t kind = U8(st, G_EMPIRE_CITIES + 2u + city);
    if (kind != 1 && kind != 3 && kind != 5) {
        return 0;
    }
    int32_t x0 = (S32(st, G_SCREEN_W) - 0x1F4) / 2;
    int32_t base_y = S32(st, G_SCREEN_H) - 0x6C;
    if (U8(st, G_EMPIRE_CITIES + 5u + city) == 0) {
        S32(st, G_TEXT_X) = 0;
        for (int32_t g = 1; g < 0x24; g++) {
            if (GAME_CALL(st, FN_EMPIRE_OBJECT_SELLS, object, g) != 0 &&
                (GAME_CALL(st, FN_EMPIRE_CITY_TRADES, S32(st, G_EMPIRE_SELECTED), g) & 0xFF) != 0) {
                if (mouse_in(st, S32(st, G_TEXT_X) + x0 + 0x64, base_y + 0x1C, 0x13, 0x11) != 0) {
                    *buys = 0;
                    return g;
                }
                S32(st, G_TEXT_X) = S32(st, G_TEXT_X) + 0x20;
            }
        }
        S32(st, G_TEXT_X) = 0;
        for (int32_t g = 1; g < 0x24; g++) {
            if (GAME_CALL(st, FN_EMPIRE_OBJECT_BUYS, object, g) != 0 &&
                (GAME_CALL(st, FN_EMPIRE_CITY_TRADES, S32(st, G_EMPIRE_SELECTED), g) & 0xFF) != 0) {
                if (mouse_in(st, S32(st, G_TEXT_X) + x0 + 0x64, base_y + 0x32, 0x13, 0x11) != 0) {
                    *buys = 1;
                    return g;
                }
                S32(st, G_TEXT_X) = S32(st, G_TEXT_X) + 0x20;
            }
        }
        return 0;
    }
    int32_t y = base_y + 0x0F;
    int32_t column = 0, count = 0;
    for (int32_t g = 1; g < 0x24; g++) {
        if (GAME_CALL(st, FN_EMPIRE_OBJECT_SELLS, object, g) != 0 &&
            (GAME_CALL(st, FN_EMPIRE_CITY_TRADES, S32(st, G_EMPIRE_SELECTED), g) & 0xFF) != 0) {
            if (mouse_in(st, column * 0x87 + x0, y, 0x1A, 0x1A) != 0) {
                *buys = 0;
                return g;
            }
            column++;
            count++;
            if (count % 2 == 0) {
                column = 0;
                y += 0x14;
            }
        }
    }
    y = base_y + 0x0F;
    column = 0, count = 0;
    for (int32_t g = 1; g < 0x24; g++) {
        if (GAME_CALL(st, FN_EMPIRE_OBJECT_BUYS, object, g) != 0 &&
            (GAME_CALL(st, FN_EMPIRE_CITY_TRADES, S32(st, G_EMPIRE_SELECTED), g) & 0xFF) != 0) {
            if (mouse_in(st, column * 0x87 + x0 + 0x118, y, 0x1A, 0x1A) != 0) {
                *buys = 1;
                return g;
            }
            column++;
            count++;
            if (count % 2 == 0) {
                column = 0;
                y += 0x14;
            }
        }
    }
    return 0;
}

int32_t tooltip_trade_city_goods(game_state *st, uint8_t *buys)
{
    /* @bin 0x004C95D0 the trade window's city 0x0077591C (records of 0x62 at 0x0077F760): the good
     * under the mouse among those it sells and buys, at the positions of the tables 0x005C4048
     * (sold) and 0x005C4128 (bought) (+0xDF, height - 0x72 + y; entries of 0x1C used in order);
     * a city of kind 0 lists what it sells at 0x005C3EC0 (*buys = 1) */
    int32_t city = S32(st, G_EDITOR_OBJECT);
    uint8_t kind = U8(st, G_EMPIRE_OBJECTS + EMPOBJ_CITY_KIND + 0x62u * (uint32_t) city);
    if (kind == 1 || kind == 3 || kind == 5) {
        uint32_t e = G_TRADE_SOLD_SLOTS + 2u;
        for (int32_t g = 1; g < 0x24; g++) {
            int32_t x = S16(st, e - 2) + 0xDF;
            int32_t y = S16(st, e) + S32(st, G_SCREEN_H) - 0x72;
            if (GAME_CALL(st, FN_EMPIRE_OBJECT_SELLS, city, g) != 0) {
                if (mouse_in(st, x, y, 0x1A, 0x1A) != 0) {
                    *buys = 0;
                    return g;
                }
                e += 0x1C;
            }
        }
        e = G_TRADE_BOUGHT_SLOTS + 2u;
        for (int32_t g = 1; g < 0x24; g++) {
            int32_t y = S16(st, e) + S32(st, G_SCREEN_H) - 0x72;
            int32_t x = S16(st, e - 2) + 0xDF;
            if (GAME_CALL(st, FN_EMPIRE_OBJECT_BUYS, city, g) != 0) {
                if (mouse_in(st, x, y, 0x1A, 0x1A) != 0) {
                    *buys = 1;
                    return g;
                }
                e += 0x1C;
            }
        }
        return 0;
    }
    if (kind != 0) {
        return 0;
    }
    uint32_t e = G_TRADE_KIND0_SLOTS + 2u;
    for (int32_t g = 1; g < 0x24; g++) {
        int32_t y = S16(st, e) + S32(st, G_SCREEN_H) - 0x72;
        int32_t x = S16(st, e - 2) + 0xDF;
        if (GAME_CALL(st, FN_EMPIRE_OBJECT_SELLS, city, g) != 0) {
            if (mouse_in(st, x, y, 0x1A, 0x1A) != 0) {
                *buys = 1;
                return g;
            }
            e += 0x1C;
        }
    }
    return 0;
}

static int32_t no_tip(game_state *st)
{
    /* the mouse is on nothing with a tooltip: restart the delay */
    S32(st, G_TIP_SINCE) = S32(st, G_FRAME_CLOCK);
    return 0;
}

static int32_t special_tip(game_state *st, int32_t kind, int32_t window)
{
    /* a drawn panel instead of a help text, shown at once */
    S32(st, G_TIP_SAVED_SCROLL) = S32(st, G_SCROLL);
    S32(st, G_TIP_ID) = 1;
    S32(st, G_TIP_KIND) = kind;
    S32(st, G_TIP_WINDOW) = window;
    S32(st, G_TIP_SAVED_VISIBLE) = S32(st, G_MESSAGE_WINDOW_VISIBLE);
    S32(st, G_TIP_SAVED_LINES) = S32(st, G_TEXT_LINES);
    U8(st, G_REDRAW) = 2;
    return 1;
}

static int32_t hovered_tile_tip(game_state *st)
{
    /* @bin 0x004C8967 over the city (no overlay, or 0x33): the ratings panel for a palace's or a
     * mansion's tile (types 0x55, 0xBB, 0xBC, 0xBD) */
    int32_t tile = S32(st, G_MOUSE_OFFSET);
    if (tile == 0) {
        return no_tip(st);
    }
    int32_t overlay = S32(st, G_OVERLAY);
    if (overlay != 0 && overlay != 0x33) {
        return no_tip(st);
    }
    uint8_t terrain = U8(st, G_GRID_TERRAIN + 4u * (uint32_t) tile);
    int32_t building = S16(st, G_GRID_BUILDING + 2u * (uint32_t) tile);
    if ((terrain & 0x40) != 0) {
        return no_tip(st);
    }
    int16_t type = S16(st, BUILDING_ADDR(building) + BLD_TYPE);
    if (type == 0xD1 || building <= 0) {
        return no_tip(st);
    }
    if (type != 0x55 && type != 0xBB && type != 0xBC && type != 0xBD) {
        return no_tip(st);
    }
    return special_tip(st, 2, 1);
}

int32_t tooltip_update(game_state *st)
{
    /* @bin 0x004C8750 the tooltip id for what the mouse hovers (see the file comment); returns 1
     * when a tooltip is to be drawn */
    if (U8(st, G_REDRAW) != 0) {
        return 0;
    }
    if (S32(st, G_TIPS_ON) == 0) {
        return no_tip(st);
    }
    int32_t clock = S32(st, G_FRAME_CLOCK);
    int32_t elapsed = clock - S32(st, G_TIP_SINCE);
    uint8_t window = U8(st, G_WINDOW);
    int32_t b = S32(st, G_HOVER_BUTTON);
    int32_t r;
    if (window == 1) {
        if (S32(st, G_EDITOR) == 1) {
            /* @bin 0x004C87AF the editor's buttons; a figure of type 0x24 under the mouse shows
             * its name (kind 4) */
            int32_t figure = S16(st, G_FIGURE_GRID + 2u * (uint32_t) S32(st, G_MOUSE_OFFSET));
            if (S32(st, G_U605A38) > 0) {
                int32_t row = S32(st, G_HOVER);
                if (row >= 1 && row <= 8) {
                    S32(st, G_TIP_ID) = row + 0xA6;
                }
            }
            b = S32(st, G_HOVER_BUTTON);
            if (b == 0x0E) {
                S32(st, G_TIP_ID) = 0xB1;
            } else {
                if (b >= 1 && b <= 0x10) {
                    S32(st, G_TIP_ID) = b + 0x90;
                }
                if (b == 0x14) {
                    S32(st, G_TIP_ID) = 0xAF;
                } else if (b == 0x15) {
                    S32(st, G_TIP_ID) = 0xB0;
                }
            }
            while (figure != 0) {
                if (U8(st, FIGURE_ADDR(figure) + FIG_TYPE) == 0x24) {
                    return special_tip(st, 4, 1);
                }
                figure = S16(st, FIGURE_ADDR(figure) + FIG_NEXT_ON_TILE);
            }
            return no_tip(st);
        }
        int32_t collapsed = S32(st, G_SIDEBAR_COLLAPSED);
        if (collapsed == 0) {
            if (b == 0x14) {
                int32_t m = S32(st, G_MENUBAR_BUTTON_HOVER);
                S32(st, G_TIP_ID) = m != 0 ? m + 0x2B : b;
                goto show;
            }
            if (b >= 0x0A && b < 0x1E) {
                S32(st, G_TIP_ID) = b;
                goto show;
            }
            if (b >= 0x1E && b < 0x32) {
                if (b == 0x2D && S32(st, G_CUSTOM) != 0) {
                    goto show;
                }
                S32(st, G_TIP_ID) = b - 10;
                goto show;
            }
            if (b >= 0x32 && b < 0x46) {
                S32(st, G_TIP_ID) = b - 9;
                goto show;
            }
            if (S32(st, G_MINIMAP_HOVER) != 0) {
                S32(st, G_TIP_ID) = 0x24;
                goto show;
            }
        } else if (collapsed == 1) {
            if (b == 0x14) {
                int32_t m = S32(st, G_MENUBAR_BUTTON_HOVER);
                S32(st, G_TIP_ID) = m != 0 ? m + 0x2B : 0x0C;
                goto show;
            }
            if (b >= 0x0A && b < 0x1E) {
                S32(st, G_TIP_ID) = 0x0C;
                goto show;
            }
            if (b >= 0x1E && b < 0x32) {
                S32(st, G_TIP_ID) = b - 10;
                goto show;
            }
        } else {
            return 0;
        }
        if (S32(st, G_MENUBAR_HOVER) != 0) {
            S32(st, G_TIP_ID) = S32(st, G_MENUBAR_HOVER) + 0x32;
            goto show;
        }
        if (S32(st, G_TOPBAR_HOVER) != 0) {
            S32(st, G_TIP_ID) = S32(st, G_TOPBAR_HOVER) + 0x3C;
            goto show;
        }
        return hovered_tile_tip(st);
    }
    int32_t row = S32(st, G_HOVER);
    switch (window) {
    case 7: {
        /* @bin 0x004C8AD8 the advisors (advisor 0x00605A44) */
        if (b >= 0x0A && b < 0x19) {
            S32(st, G_TIP_ID) = b + 0x3D;
            goto show;
        }
        if (b >= 0x19 && b < 0x1E) {
            S32(st, G_TIP_ID) = b - 0x18;
            goto show;
        }
        int32_t advisor = S32(st, G_PAGE);
        if (advisor == 1) {
            if (row != 0) {
                S32(st, G_TIP_ID) = 0x5C;
            } else if (S32(st, G_SQUARE_BUTTON_HOVER) != 0) {
                S32(st, G_TIP_ID) = 0x5D;
            } else {
                return no_tip(st);
            }
        } else if (advisor == 4) {
            if (row == 2) {
                if (S32(st, CURRENT_CITY(st) + CITY_MANSION_BUILT) == 0) {
                    return 0;
                }
                S32(st, G_TIP_ID) = row + 0x5F;
            } else if (row == 0) {
                return no_tip(st);
            } else if (row > 2) {
                if (row != 3) {
                    return no_tip(st);
                }
                S32(st, G_TIP_ID) = 0x85;
            } else {
                S32(st, G_TIP_ID) = row + 0x5F;
            }
        } else if (advisor == 5) {
            if (row < 1 || row > 4) {
                return no_tip(st);
            }
            S32(st, G_TIP_ID) = row + 0x67;
        } else if (advisor == 2) {
            if (row != 0x13) {
                return no_tip(st);
            }
            S32(st, G_TIP_ID) = 0x90;
        } else if (advisor == 3) {
            if (row != 0x13) {
                return no_tip(st);
            }
            S32(st, G_TIP_ID) = 0x8F;
        } else if (advisor == 6) {
            if (row == 3) {
                /* @bin 0x004C8C1F the ratings advisor's trade routes panel (kind 3) */
                return special_tip(st, 3, 7);
            }
            if (row == 1) {
                S32(st, G_TIP_ID) = 0x6C;
            } else if (row == 2) {
                S32(st, G_TIP_ID) = 0x2A;
            } else if (row == 0) {
                return no_tip(st);
            } else {
                S32(st, G_TIP_ID) = 0x6D;
            }
        } else if (advisor == 7) {
            if (row == 0) {
                return no_tip(st);
            }
            S32(st, G_TIP_ID) = 0x71;
        } else if (advisor == 0x0B) {
            if (row == 0) {
                return no_tip(st);
            }
            S32(st, G_TIP_ID) = 0x72;
        } else if (advisor == 0x0C) {
            if (S32(st, G_SQUARE_BUTTON_HOVER) == 0) {
                return no_tip(st);
            }
            S32(st, G_TIP_ID) = 0x7A;
        } else {
            return no_tip(st);
        }
        goto show;
    }
    case 0x2A:
        if (row == 1) {
            S32(st, G_TIP_ID) = 0x5E;
        } else if (row == 0) {
            return no_tip(st);
        } else {
            S32(st, G_TIP_ID) = 0x5F;
        }
        goto show;
    case 0x2C:
    case 0x4D:
    case 0x28:
        return no_tip(st);
    case 0x2D:
        if (row == 1) {
            S32(st, G_TIP_ID) = 0x64;
        } else if (row == 2) {
            S32(st, G_TIP_ID) = 0x65;
        } else if (row != 0) {
            S32(st, G_TIP_ID) = 0x66;
        } else if (S32(st, G_SQUARE_BUTTON_HOVER) != 0) {
            S32(st, G_TIP_ID) = 0x67;
        } else {
            return no_tip(st);
        }
        goto show;
    case 0x2B:
    case 0x52:
        /* @bin 0x004C8DC5 / 0x004C8E65 */
        if (b == 0x0A) {
            S32(st, G_TIP_ID) = 1;
            goto show;
        }
        if (b == 0x0B) {
            S32(st, G_TIP_ID) = 2;
            goto show;
        }
        if (b == 0x0C) {
            S32(st, G_TIP_ID) = 0x73;
            goto show;
        }
        if (b == 0x0D) {
            S32(st, G_TIP_ID) = 0x74;
            goto show;
        }
        if (row >= 1 && row <= 5) {
            if (S16(st, G_GOD_MODE + 2u * (uint32_t) (row - 1)) != 0) {
                S32(st, G_TIP_ID) = 0x74 + row;
            }
            goto show;
        }
        return no_tip(st);
    case 9: {
        /* @bin 0x004C8F44 the info panel (its kind 0x00EADD50, building 0x00EADD6C) */
        int32_t info = S32(st, G_INFO_KIND);
        if (info == 4 || info == 6 || info == 5) {
            if (b >= 0x0A && b < 0x1E) {
                S32(st, G_TIP_ID) = b - 9;
            }
            goto show;
        }
        if (b >= 0x0A && b < 0x1E) {
            S32(st, G_TIP_ID) = b - 9;
            goto show;
        }
        if (b >= 0x28 && b < 0x37) {
            S32(st, G_TIP_ID) = S32(st, G_TIP_BUTTON_IDS + 4u * (uint32_t) b);
            goto show;
        }
        if (info != 2) {
            return no_tip(st);
        }
        int16_t type = S16(st, BUILDING_ADDR(S32(st, G_INFO_BUILDING)) + BLD_TYPE);
        if (type < 0x0A || type > 0x1D || row == 0) {
            return no_tip(st);
        }
        S32(st, G_TIP_ID) = 0x7B;
        goto show;
    }
    case 0x0D:
    case 0x10:
        return 0;
    case 0x14: {
        uint8_t buys;
        r = tooltip_empire_city_goods(st, &buys);
        if (r != 0) {
            S32(st, G_TIP_ID) = r + 0x85;
            goto show;
        }
        b = S32(st, G_HOVER_BUTTON);
        if (b == 0x0C) {
            S32(st, G_TIP_ID) = 0x46;
            goto show;
        }
        if (b < 0x0A || b >= 0x1E) {
            return no_tip(st);
        }
        S32(st, G_TIP_ID) = b - 9;
        goto show;
    }
    case 0x15: {
        uint8_t buys;
        r = tooltip_trade_city_goods(st, &buys);
        if (r != 0) {
            S32(st, G_TIP_ID) = r + 0x85;
            goto show;
        }
        b = S32(st, G_HOVER_BUTTON);
        if (b == 0x0C) {
            S32(st, G_TIP_ID) = 0x46;
            goto show;
        }
        if (b >= 0x0A && b < 0x1E) {
            S32(st, G_TIP_ID) = b - 9;
            goto show;
        }
        row = S32(st, G_HOVER);
        if (row > 0 && row < 9) {
            S32(st, G_TIP_ID) = row + 0x9D;
            goto show;
        }
        if (row == 0x0A) {
            S32(st, G_TIP_ID) = 0xA6;
            goto show;
        }
        return no_tip(st);
    }
    case 0x11: {
        /* @bin 0x004C912D the message list: a row's message 0x00E951E8 (first shown 0x00E95120) */
        if (b == 0x0A) {
            S32(st, G_TIP_ID) = 1;
            goto show;
        }
        if (b == 0x0B) {
            S32(st, G_TIP_ID) = 2;
            goto show;
        }
        if (row == 0) {
            return no_tip(st);
        }
        int32_t m = S32(st, G_MESSAGE_SCROLL) + row - 1;
        S32(st, G_MESSAGE_SLOT) = m;
        if (m >= S32(st, G_MESSAGE_TOTAL)) {
            S32(st, G_TIP_SINCE) = clock;
            return 0;
        }
        goto show;
    }
    case 5:
        S32(st, G_TIP_SINCE) = clock;
        return 0;
    case 0x4E:
        r = tooltip_family_hover(st);
        if (r == 0) {
            return no_tip(st);
        }
        S32(st, G_TIP_ID) = r + 0x85;
        goto show;
    default:
        return no_tip(st);
    }
show:
    /* @bin 0x004C9211 */
    if (S32(st, G_TIPS_ON) != 2) {
        S32(st, G_TIP_SINCE) = clock;
        S32(st, G_TIP_ID) = 0;
        return 0;
    }
    if (elapsed < 0x96) {
        S32(st, G_TIP_ID) = 0;
        return 0;
    }
    S32(st, G_TIP_SAVED_VISIBLE) = S32(st, G_MESSAGE_WINDOW_VISIBLE);
    S32(st, G_TIP_SAVED_SCROLL) = S32(st, G_SCROLL);
    S32(st, G_TIP_WINDOW) = U8(st, G_WINDOW);
    S32(st, G_TIP_SAVED_LINES) = S32(st, G_TEXT_LINES);
    S32(st, G_UD3A6E0) = 0;
    S32(st, G_TIP_KIND) = 0;
    if (U8(st, G_REDRAW) == 0) {
        U8(st, G_REDRAW) = 2;
    }
    return 1;
}

int32_t tooltip_end(game_state *st)
{
    /* @bin 0x004CA930 after the draw: restore the text box state saved by the update (0x00D3A6BC,
     * 0x00D3A6B8, 0x00D3A6B4 into 0x00D3B130, 0x00D3B13C, 0x00D3B138), clear the id, ask for a
     * redraw */
    if (S32(st, G_TIP_ID) <= 0) {
        return 0;
    }
    S32(st, G_SCROLL) = S32(st, G_TIP_SAVED_SCROLL);
    S32(st, G_TIP_ID) = 0;
    S32(st, G_MESSAGE_WINDOW_VISIBLE) = S32(st, G_TIP_SAVED_VISIBLE);
    S32(st, G_TEXT_LINES) = S32(st, G_TIP_SAVED_LINES);
    if (U8(st, G_REDRAW) == 0) {
        U8(st, G_REDRAW) = 2;
    }
    return 1;
}

/* ---- port table ---- */

static int32_t run_update(game_state *st, const int32_t *a) { (void) a; return tooltip_update(st); }
static int32_t run_end(game_state *st, const int32_t *a) { (void) a; return tooltip_end(st); }
static int32_t run_family(game_state *st, const int32_t *a) { (void) a; return tooltip_family_hover(st); }
static int32_t run_in(game_state *st, const int32_t *a) { return mouse_in(st, a[0], a[1], a[2], a[3]); }

static int32_t run_empire(game_state *st, const int32_t *a)
{
    /* the flag is a byte of the caller's frame in the original: written when it is in the state */
    uint8_t buys = 0;
    int32_t r = tooltip_empire_city_goods(st, &buys);
    uint8_t *p = state_ptr(st, (uint32_t) a[0], 1);
    if (p && r != 0) {
        *p = buys;
    }
    return r;
}

static int32_t run_trade(game_state *st, const int32_t *a)
{
    /* the flag is a byte of the caller's frame in the original: written when it is in the state */
    uint8_t buys = 0;
    int32_t r = tooltip_trade_city_goods(st, &buys);
    uint8_t *p = state_ptr(st, (uint32_t) a[0], 1);
    if (p && r != 0) {
        *p = buys;
    }
    return r;
}

const ported_function tooltip_ports[] = {
    {0x004C8750u, "tooltip_update", 0, 32, 0, run_update, 0},
    {0x004CA930u, "tooltip_end", 0, 32, 0, run_end, 0},
    {0x004C92A0u, "tooltip_family_hover", 0, 32, 0, run_family, 0},
    {0x004C9340u, "tooltip_empire_city_goods", 1, 32, 0, run_empire, 0},
    {0x004C95D0u, "tooltip_trade_city_goods", 1, 32, 0, run_trade, 0},
    {0x004CF620u, "mouse_in", 4, 32, 0, run_in, 0},
};
const int tooltip_ports_count = sizeof(tooltip_ports) / sizeof(tooltip_ports[0]);
