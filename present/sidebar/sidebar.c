/* Imhotep - the city sidebar and the top menu bar: drawing (roadmap phase 7.5).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Every function here draws through the original's image draw 0x0053E590 (thiscall on the image
 * object 0x00F55BB0: image id, x, y), its fills, lines and text functions (package blit), so the
 * pixels are theirs; what is ported is the layout: which image, where, when, and the presentation
 * globals the original writes on the way. */
#include "sidebar.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define CITY_TREASURY 0x0Cu            /* i32 city +0x0C (0x00EA187C) */
#define CITY_HAPPINESS 0x10u           /* i32 city +0x10 (0x00EA1880): shown with "Happiness" */
#define CITY_POPULATION 0x24u          /* i32 city +0x24 (0x00EA1894) */
#define CITY_FONT_SMALL 0x00F56F84u    /* i32 first image of the small font */
#define CITY_FONT_LARGE 0x00F56F8Cu    /* i32 first image of the large font (bar texts) */
#define SIDEBAR_IMAGES 0x00F567E4u     /* i32 first image of the sidebar panels */
#define NUMBER_SUFFIX 0x0057C048u      /* the empty string given as the numbers' suffix */

static void draw_image(game_state *st, int32_t id, int32_t x, int32_t y)
{
    GAME_CALLT(st, IMAGES, 0x0053E590u, id, x, y);
}

static void text_group(game_state *st, int32_t group, int32_t id, int32_t x, int32_t y, int32_t font,
                       int32_t colour)
{
    /* @bin 0x004D62F0 a text of Pharaoh_Text.eng at (x, y) of the 640x480 layout, offset by the
     * layout origin (0x00E38DA8, 0x00E38DA4) */
    GAME_CALL(st, 0x004D62F0u, group, id, x, y, font, colour, S32(st, LAYOUT_DX), S32(st, LAYOUT_DY));
}

static void text_number(game_state *st, int32_t value, uint32_t suffix, int32_t x, int32_t y, int32_t font,
                        int32_t colour)
{
    /* @bin 0x004D6860 a number with a suffix string (prefix character 0x40) */
    GAME_CALL(st, 0x004D6860u, value, 0x40, (int32_t) suffix, x, y, font, colour, S32(st, LAYOUT_DX),
              S32(st, LAYOUT_DY));
}

static int32_t city_i32(game_state *st, uint32_t offset)
{
    return S32(st, CURRENT_CITY(st) + offset);
}

int32_t cityview_pixel_width(game_state *st)
{
    /* @bin 0x00430CC0 (0x0070E458 + 2) * 0x0070E464: the city view's width in pixels */
    return (int32_t) ((uint32_t) (S32(st, 0x0070E458u) + 2) * (uint32_t) S32(st, 0x0070E464u));
}

void sidebar_set_x(game_state *st)
{
    /* @bin 0x0051E930 the sidebar's x (expanded, collapsed) by screen width */
    int32_t w = S32(st, SCREEN_WIDTH);
    if (w == 0x280) {
        S32(st, SIDEBAR_X) = 0x1DE;
        S32(st, SIDEBAR_X_COLLAPSED) = 0x256;
    } else if (w == 0x320) {
        S32(st, SIDEBAR_X) = 0x256;
        S32(st, SIDEBAR_X_COLLAPSED) = 0x2CE;
    } else if (w == 0x400) {
        S32(st, SIDEBAR_X) = 0x346;
        S32(st, SIDEBAR_X_COLLAPSED) = 0x3BE;
    }
}

void buttons_draw(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count)
{
    /* @bin 0x00425730 a button table: entries of 0x24 bytes (+0 x, +2 y: i16 relative to (x, y);
     * +0x0A image group, +0x0C image offset: i16; +0x18 enabled, +0x19 pressed, +0x1A hovered,
     * +0x1B press animation: u8). Drawn when the window or the sidebar is redrawn, or while the
     * button animates. Image: base + 3 disabled, + 2 pressed, + 1 hovered; the animation counts
     * down in the upper six bits of +0x1B and shows base + (low two bits) - 1 */
    if (count <= 0) {
        return;
    }
    uint32_t e = table;
    do {
        int32_t bx = P16(st, e) + x;
        int32_t by = P16(st, e + 2) + y;
        S32(st, 0x00D3B5E0u) = bx;
        S32(st, 0x00D3B5DCu) = by;
        int32_t id = S32(st, IMAGE_GROUPS + 8u * (uint32_t) (int32_t) P16(st, e + 0x0A)) + P16(st, e + 0x0C);
        S32(st, 0x00D3B5C0u) = id;
        if (U8(st, REDRAW_SIDEBAR) != 0 || U8(st, REDRAW_ALL) != 0 || P8(st, e + 0x1B) != 0) {
            if (P8(st, e + 0x18) == 0) {
                id += 3;
                S32(st, 0x00D3B5C0u) = id;
            } else if (P8(st, e + 0x1B) != 0) {
                uint32_t anim = P8(st, e + 0x1B);
                uint32_t phase = anim & 3;
                uint32_t steps = anim >> 2;
                if ((int32_t) steps <= 0) {
                    phase = (phase != 2) + 1u;
                    steps = 0x10;
                }
                P8(st, e + 0x1B) = (uint8_t) ((uint8_t) ((uint8_t) steps + 0x3F) << 2 | (uint8_t) phase);
                id = S32(st, 0x00D3B5C0u) + (int32_t) phase - 1;
                S32(st, 0x00D3B5C0u) = id;
                bx = S32(st, 0x00D3B5E0u);
                by = S32(st, 0x00D3B5DCu);
            } else if (P8(st, e + 0x19) != 0) {
                if (P8(st, e + 0x19) == 0x14) {
                    U8(st, 0x00E38DC8u) = 1; /* @bin 0x00425801 present now */
                }
                id += 2;
                S32(st, 0x00D3B5C0u) = id;
            } else if (P8(st, e + 0x1A) != 0) {
                id += 1;
                S32(st, 0x00D3B5C0u) = id;
            }
            draw_image(st, id, bx, by);
        }
        e += 0x24;
    } while (--count != 0);
}

void menubar_draw(game_state *st, uint32_t table, int32_t count)
{
    /* @bin 0x00425330 the top menu titles: entries of 0x10 bytes (+0 x, +2 end x, +4 y, +6 text
     * group: i16), laid out left to right from the first x, 10 pixels apart; the open menu
     * (0x00705730, 1-based) highlighted with the shadow flag 0x00D3B510 and colour 0xFAC1 */
    U8(st, 0x00E38DE1u) = 1;
    U8(st, 0x00E38DE0u) = 1;
    S32(st, TEXT_X) = 0;
    S32(st, TEXT_X) = P16(st, table);
    int32_t k = 1;
    if (count < k) {
        return;
    }
    uint32_t e = table;
    do {
        int32_t group = P16(st, e + 6);
        int32_t y = P16(st, e + 4);
        P16(st, e) = (int16_t) S32(st, TEXT_X);
        int32_t x = (int16_t) S32(st, TEXT_X);
        GAME_CALL(st, 0x004CC110u, group, 0);
        if (k == S32(st, 0x00705730u)) {
            S32(st, 0x00D3B510u) = 1;
            GAME_CALL(st, 0x004CBF30u, S32(st, 0x00E20768u), S32(st, CITY_FONT_SMALL));
            GAME_CALL(st, 0x004CCBB0u, group, 0, x, y, S32(st, CITY_FONT_LARGE), 0xFAC1);
            S32(st, 0x00D3B510u) = 0;
        } else {
            GAME_CALL(st, 0x004CCBB0u, group, 0, x, y, S32(st, CITY_FONT_LARGE), 0);
        }
        P16(st, e + 2) = (int16_t) S32(st, TEXT_X);
        S32(st, TEXT_X) = S32(st, TEXT_X) + 10;
        e += 0x10;
        k++;
    } while (k <= count);
}

void menubar_draw_button(game_state *st)
{
    /* @bin 0x00425900 the button at the right of the menu bar (record 0x005C79C8: +0 x, +2 y,
     * +0x0A group: i16; +0x19 pressed, +0x1A hovered: u8; +0x1C image offset: i32) */
    int32_t id = S32(st, IMAGE_GROUPS + 8u * (uint32_t) (int32_t) S16(st, 0x005C79D2u));
    int32_t x = S16(st, 0x005C79C8u);
    uint8_t pressed = U8(st, 0x005C79E1u);
    S32(st, 0x00D3B5E0u) = x;
    int32_t y = S16(st, 0x005C79CAu);
    S32(st, 0x00D3B5DCu) = y;
    S32(st, 0x00D3B5C0u) = id;
    if (pressed != 0) {
        if (pressed == 0x14) {
            U8(st, 0x00E38DC8u) = 1;
        }
        id = id + S32(st, 0x005C79E4u) + 4;
        S32(st, 0x00D3B5C0u) = id;
        draw_image(st, id, x, y);
        return;
    }
    if (U8(st, 0x005C79E2u) != 0) {
        id = id + S32(st, 0x005C79E4u) + 1;
        S32(st, 0x00D3B5C0u) = id;
    }
    draw_image(st, id, x, y);
}

void topbar_draw_background(game_state *st)
{
    /* @bin 0x00536670 the bar's background image (sidebar images + 8), at x 0 / -240 / -360 for
     * widths 1024 / 800 / 640 (the editor flag selects the same draw) */
    int32_t w = S32(st, SCREEN_WIDTH);
    int32_t x;
    if (w == 0x280) {
        x = -0x168;
    } else if (w == 0x320) {
        x = -0xF0;
    } else if (w == 0x400) {
        x = 0;
    } else {
        return;
    }
    draw_image(st, S32(st, SIDEBAR_IMAGES) + 8, x, 0);
}

/* @bin 0x0051F628 (index table of the switch at 0x0051F0EF): windows whose frame draws no top bar
 * texts */
static int topbar_hidden(uint8_t window)
{
    static const uint8_t hidden[] = {0, 4, 5, 7, 13, 14, 15, 16, 17, 18, 19, 20, 21, 23, 38, 40, 42, 43, 44, 45,
                                     46, 47, 48, 63, 72, 74, 76, 77, 82};
    for (unsigned i = 0; i < sizeof(hidden); i++) {
        if (hidden[i] == window) {
            return 1;
        }
    }
    return 0;
}

void topbar_draw(game_state *st, int32_t force)
{
    /* @bin 0x0051F0A0 the top menu bar: menu titles, treasury, population, date; redrawn when one
     * of them changed (last values 0x00F53DA8 treasury, 0x00F53DAC population, 0x00F53DA4 day)
     * or when forced; 0x00F53F74 asks for a background redraw */
    int32_t font = S32(st, CITY_FONT_LARGE);
    if (S32(st, EDITOR_MODE) == 1 && U8(st, WINDOW_ID) == 1) {
        GAME_CALL(st, 0x004CC770u, 0x00E37078, 0x104, 4, S32(st, 0x00F56F9Cu), 0); /* the map name */
    }
    uint8_t window = U8(st, WINDOW_ID);
    if (window <= 0x52 && topbar_hidden(window)) {
        return;
    }
    if (window >= 0x33) {
        return;
    }
    int32_t changed = 1;
    if (city_i32(st, CITY_POPULATION) == S32(st, 0x00F53DACu)) {
        changed = force;
    }
    if (city_i32(st, CITY_TREASURY) != S32(st, 0x00F53DA8u)) {
        changed = 1;
    }
    if (S32(st, 0x0078D7C0u) != S32(st, 0x00F53DA4u) || changed != 0) {
        S32(st, 0x00F53F74u) = 1;
        topbar_draw_background(st);
    } else {
        int32_t pending = S32(st, 0x00F53F74u);
        if (pending <= 0) {
            return;
        }
        if (pending == 1 || pending == 2) {
            topbar_draw_background(st);
        }
    }
    S32(st, 0x00F53F74u) = 0;
    S32(st, 0x00F53DACu) = city_i32(st, CITY_POPULATION);
    S32(st, 0x00F53DA8u) = city_i32(st, CITY_TREASURY);
    int32_t w;
    if (S32(st, EDITOR_MODE) == 1) {
        menubar_draw(st, 0x005C9230u, 4);
        /* @bin 0x0051F1CC the button at width - width / 5 */
        w = S32(st, SCREEN_WIDTH);
        S16(st, 0x005C79C8u) = (int16_t) (w - w / 5);
        menubar_draw_button(st);
        return;
    }
    menubar_draw(st, 0x005C91F0u, 4);
    S32(st, TEXT_X) = 0;
    int32_t money_font = font;
    if (city_i32(st, CITY_TREASURY) < 0) {
        money_font = S32(st, 0x00F56FA4u); /* @bin 0x0051F226 debts in the red font */
    }
    w = S32(st, SCREEN_WIDTH);
    /* @bin 0x0051F255.. per width: x of the treasury, population and date texts, and the bar's
     * three anchors 0x00F53DC4, 0x00F53DC0, 0x00F53DBC */
    int32_t money_x, money_n, pop_x, pop_n, date_x, date_n, year_flag;
    if (w == 0x280) {
        S32(st, 0x00F53DC4u) = 0xF2;
        S32(st, 0x00F53DC0u) = 0x16F;
        S32(st, 0x00F53DBCu) = 0x1E5;
        money_x = 0xFA, money_n = 0xF5, pop_x = 0x177, pop_n = 0x16F, date_x = 0x203, date_n = 0x1F8,
        year_flag = 2;
    } else if (w == 0x320) {
        S32(st, 0x00F53DC4u) = 0x152;
        S32(st, 0x00F53DC0u) = 0x1CA;
        S32(st, 0x00F53DBCu) = 0x28C;
        money_x = 0x157, money_n = 0x153, pop_x = 0x1D6, pop_n = 0x1D2, date_x = 0x28F, date_n = 0x28F,
        year_flag = 0;
    } else if (w == 0x400) {
        S32(st, 0x00F53DC4u) = 0x1ED;
        S32(st, 0x00F53DC0u) = 0x27D;
        S32(st, 0x00F53DBCu) = 0x375;
        money_x = 0x1EF, money_n = 0x1F5, pop_x = 0x285, pop_n = 0x28B, date_x = 0x373, date_n = 0x373,
        year_flag = 0;
    } else {
        goto button;
    }
    text_group(st, 6, 0, money_x, 5, money_font, 0);
    text_number(st, city_i32(st, CITY_TREASURY), NUMBER_SUFFIX, S32(st, TEXT_X) + money_n, 5, money_font, 0);
    S32(st, TEXT_X) = 0;
    text_group(st, 6, 1, pop_x, 5, font, 0xFFFF);
    text_number(st, city_i32(st, CITY_POPULATION), NUMBER_SUFFIX, S32(st, TEXT_X) + pop_n, 5, font, 0xFFFF);
    S32(st, TEXT_X) = 0;
    text_group(st, 0x19, S32(st, 0x0078D7C0u), date_x, 5, font, 0); /* the month */
    /* @bin 0x004CCCB0 the year (0x0078D7BC) */
    GAME_CALL(st, 0x004CCCB0u, year_flag, S32(st, 0x0078D7BCu), S32(st, TEXT_X) + date_n, 5, font, 0);
button:
    if (S32(st, SCREEN_WIDTH) == 0x280) {
        S16(st, 0x005C79C8u) = (int16_t) (S32(st, 0x00F53DBCu) - 10);
    } else {
        S16(st, 0x005C79C8u) = (int16_t) (S32(st, 0x00F53DBCu) - 0x2F);
    }
    menubar_draw_button(st);
}

void sidebar_draw_messages(game_state *st, int32_t force)
{
    /* @bin 0x0051F680 the expanded sidebar's message count (0x00E951EC) and the button states that
     * follow it: 0x005C7538 (an event 0x00BC968C with 0x00BC9694), 0x005C755C messages,
     * 0x005C7580 0x00E951D8 > 0, 0x005C75A4 0x00E38EBC = 0 */
    if (S32(st, EDITOR_MODE) == 1 || U8(st, WINDOW_ID) != 1 || S32(st, SIDEBAR_COLLAPSED) != 0) {
        return;
    }
    int32_t count = S32(st, 0x00E951ECu);
    if (count == S32(st, 0x00F53DA0u) && force == 0) {
        return;
    }
    S32(st, 0x00F53DA0u) = count;
    if (S32(st, 0x00BC968Cu) == 0) {
        U8(st, 0x005C7538u) = 0;
    } else {
        U8(st, 0x005C7538u) = 1;
        if (S32(st, 0x00BC9694u) == 0) {
            U8(st, 0x005C7538u) = 0;
        }
    }
    U8(st, 0x005C755Cu) = count > 0;
    U8(st, 0x005C7580u) = S32(st, 0x00E951D8u) > 0;
    U8(st, 0x005C75A4u) = S32(st, 0x00E38EBCu) == 0;
    if (count == 0) {
        return;
    }
    GAME_CALL(st, 0x004CCE90u, count, 0x40, (int32_t) NUMBER_SUFFIX, S32(st, SIDEBAR_X) + 0x30, 0x1C4, 0x20,
              S32(st, CITY_FONT_SMALL), 0);
    GAME_CALL(st, 0x004CCE90u, S32(st, 0x00E951ECu), 0x40, (int32_t) NUMBER_SUFFIX, S32(st, SIDEBAR_X) + 0x2F,
              0x1C5, 0x20, S32(st, CITY_FONT_SMALL), 0xFFFF);
}

void sidebar_draw_buttons(game_state *st)
{
    /* @bin 0x0051EC90 the sidebar's button tables: expanded 0x005C7300 (2), 0x005C7370 (16),
     * 0x005C7980 (2); collapsed 0x005C7348 (1), 0x005C75B0 (12) */
    if (S32(st, 0x00BC968Cu) == 0) {
        U8(st, 0x005C7538u) = 0;
    } else {
        U8(st, 0x005C7538u) = 1;
        if (S32(st, 0x00BC9694u) == 0) {
            U8(st, 0x005C7538u) = 0;
        }
    }
    int32_t collapsed = S32(st, SIDEBAR_COLLAPSED);
    if (collapsed == 0) {
        buttons_draw(st, S32(st, SIDEBAR_X), 0x1E, 0x005C7300u, 2);
        buttons_draw(st, S32(st, SIDEBAR_X), 0x1E, 0x005C7370u, 0x10);
        buttons_draw(st, S32(st, SIDEBAR_X), 0x1E, 0x005C7980u, 2);
    } else if (collapsed == 1) {
        buttons_draw(st, S32(st, SIDEBAR_X_COLLAPSED), 0x1E, 0x005C7348u, 1);
        buttons_draw(st, S32(st, SIDEBAR_X_COLLAPSED), 0x1E, 0x005C75B0u, 0x0C);
    }
}

void sidebar_draw_overlay_name(game_state *st, int32_t force, int32_t x)
{
    /* @bin 0x0051ED30 the name of the current overlay 0x00E94E0C (text group 0x0E; "none": group
     * 6 text 4) when it changed (last 0x00F53DB0), forced or with the sidebar redrawn */
    int32_t overlay = S32(st, 0x00E94E0Cu);
    int32_t draw = 1;
    if (overlay == S32(st, 0x00F53DB0u)) {
        draw = force;
    }
    if (U8(st, REDRAW_SIDEBAR) != 0) {
        draw = 1;
    }
    if (S32(st, SIDEBAR_COLLAPSED) != 0 || draw == 0) {
        return;
    }
    S32(st, 0x00F53DB0u) = overlay;
    int32_t group = 0x0E, id = overlay;
    if (overlay == 0) {
        group = 6, id = 4;
    }
    GAME_CALL(st, 0x004D6250u, group, id, x, 0x22, 0x75, S32(st, CITY_FONT_LARGE), 0xFFFF, S32(st, LAYOUT_DX),
              S32(st, LAYOUT_DY));
}

void sidebar_draw_build_title(game_state *st, int32_t force, int32_t x)
{
    /* @bin 0x0051EDD0 the picture of the last build menu 0x007056EC (images 0x00F567C4 + k) */
    if (force == 0 && S32(st, SIDEBAR_COLLAPSED) != 0) {
        return;
    }
    static const struct {
        int32_t menu, offset;
    } pictures[] = {{1, 1}, {3, 5}, {2, 9}, {0x0C, 2}, {10, 6}, {4, 10}, {8, 3}, {6, 7}, {7, 0x0B}, {5, 4}, {9, 8}, {0x0B, 0x0C}};
    int32_t menu = S32(st, 0x007056ECu);
    int32_t offset = 0;
    for (unsigned i = 0; i < sizeof(pictures) / sizeof(pictures[0]); i++) {
        if (pictures[i].menu == menu) {
            offset = pictures[i].offset;
            break;
        }
    }
    draw_image(st, S32(st, 0x00F567C4u) + offset, x, 0xD3);
}

void sidebar_draw_minimap(game_state *st, int32_t force)
{
    /* @bin 0x0051EFB0 the minimap, when forced or when something moved: the request flag
     * 0x00F53FD0 (cleared here), a drag in the editor (0x006059E4), scrolling (0x00E38DCC) */
    if (S32(st, SIDEBAR_COLLAPSED) != 0) {
        return;
    }
    int32_t changed = 0;
    if (S32(st, 0x00F53FD0u) != 0) {
        S32(st, 0x00F53FD0u) = 0;
        changed = 1;
    }
    int32_t editor = S32(st, EDITOR_MODE);
    if (editor == 1 && S32(st, 0x006059E4u) != 0) {
        changed = 1;
    }
    if (S32(st, 0x00E38DCCu) != 0) {
        changed = 1;
    }
    if (force == 0 && changed == 0) {
        return;
    }
    if (editor == 0) {
        GAME_CALL(st, 0x0043E810u, S32(st, SIDEBAR_X) + 0x0D, 0x3B, 0x49, 0x6F);
        return;
    }
    GAME_CALL(st, 0x0043E810u, S32(st, SIDEBAR_X) + 8, 0x24, 0x49, 0x6F);
    int32_t x = S32(st, SIDEBAR_X);
    GAME_CALL(st, 0x004CD510u, x + 7, 0x23, x + 0x99, 0x23, 0x4208);
    x = S32(st, SIDEBAR_X);
    GAME_CALL(st, 0x004CD510u, x + 7, 0x24, x + 7, 0x93, 0x4208);
    x = S32(st, SIDEBAR_X);
    GAME_CALL(st, 0x004CD510u, x + 0x99, 0x24, x + 0x99, 0x93, 0xC618);
}

void sidebar_draw(game_state *st)
{
    /* @bin 0x0051E990 the full redraw of the sidebar (window 1 and the city windows): the bottom
     * strip, the panel, buttons, overlay name, build picture, minimap, the panel's lower part by
     * width, then the top bar; the editor has its own panel */
    int32_t w = S32(st, SCREEN_WIDTH);
    if (w == 0x400) {
        draw_image(st, S32(st, SIDEBAR_IMAGES) + 2, 1000, 0); /* @bin 0x00537120 the right edge */
    } else if (w == 0x320) {
        draw_image(st, S32(st, SIDEBAR_IMAGES) + 3, 0x2F8, 0); /* @bin 0x00537100 */
    }
    w = S32(st, SCREEN_WIDTH);
    if (w == 0x400) {
        GAME_CALL(st, 0x004CD9D0u, 0, S32(st, SCREEN_HEIGHT) - 9, 0x400, 9, 0);
    } else {
        GAME_CALL(st, 0x004CD9D0u, 0, S32(st, SCREEN_HEIGHT) - 6, w, 6, 0);
    }
    U8(st, REDRAW_SIDEBAR) = 1;
    if (S32(st, EDITOR_MODE) == 0) {
        if (S32(st, SIDEBAR_COLLAPSED) == 0) {
            draw_image(st, S32(st, SIDEBAR_IMAGES), S32(st, SIDEBAR_X), 0x1E);
        } else {
            draw_image(st, S32(st, SIDEBAR_IMAGES) + 1, S32(st, SIDEBAR_X_COLLAPSED), 0x1E);
        }
        sidebar_draw_buttons(st);
        sidebar_draw_overlay_name(st, 1, S32(st, SIDEBAR_X) + 0x0B);
        sidebar_draw_build_title(st, 0, S32(st, SIDEBAR_X) + 0x0B);
        sidebar_draw_minimap(st, 1);
        w = S32(st, SCREEN_WIDTH);
        int32_t collapsed = S32(st, SIDEBAR_COLLAPSED);
        if (w == 0x400 && collapsed == 0) {
            draw_image(st, S32(st, SIDEBAR_IMAGES) + 4, S32(st, SIDEBAR_X), 0x1E0);
        } else if (w == 0x400 && collapsed == 1) {
            draw_image(st, S32(st, SIDEBAR_IMAGES) + 5, S32(st, SIDEBAR_X_COLLAPSED), 0x1E0);
        } else if (w == 0x320 && collapsed == 0) {
            draw_image(st, S32(st, SIDEBAR_IMAGES) + 6, S32(st, SIDEBAR_X), 0x1E0);
        } else if (w == 0x320 && collapsed == 1) {
            draw_image(st, S32(st, SIDEBAR_IMAGES) + 7, S32(st, SIDEBAR_X_COLLAPSED), 0x1E0);
        }
        topbar_draw(st, 1);
        sidebar_draw_messages(st, 1);
        return;
    }
    int32_t x = S32(st, SIDEBAR_X);
    GAME_CALL(st, 0x004CDA40u, x + 6, 0x18, S32(st, SCREEN_WIDTH) - x - 6, S32(st, SCREEN_HEIGHT2) - 0x18, 0);
    GAME_CALL0(st, 0x004DDDC0u);
    draw_image(st, S32(st, 0x00F55C64u), S32(st, SIDEBAR_X), 0x18);
    w = S32(st, SCREEN_WIDTH);
    if (w == 0x400) {
        draw_image(st, S32(st, 0x00F55C64u) + 2, S32(st, SIDEBAR_X), 0x1E0);
    } else if (w == 0x320) {
        draw_image(st, S32(st, 0x00F55C64u) + 1, S32(st, SIDEBAR_X), 0x1E0);
    }
    GAME_CALL0(st, 0x00533E70u);
    buttons_draw(st, S32(st, SIDEBAR_X), 0x18, 0x005C7760u, 0x0F);
    sidebar_draw_minimap(st, 1);
    GAME_CALL(st, 0x004D62F0u, 0x2C, 0x84, S32(st, SIDEBAR_X) + 0x0B, 0x98, S32(st, CITY_FONT_LARGE), 0,
              S32(st, LAYOUT_DX), S32(st, LAYOUT_DY));
    GAME_CALL(st, 0x004D62F0u, 2, 0, S32(st, SIDEBAR_X) + 0x5E, 0x98, S32(st, CITY_FONT_LARGE), 0,
              S32(st, LAYOUT_DX), S32(st, LAYOUT_DY));
    topbar_draw(st, 1);
    sidebar_draw_messages(st, 1);
}

void sidebar_slide(game_state *st)
{
    /* @bin 0x0051FD60 window 2: the sidebar slides in or out by a step that grows with the
     * frame time 0x00E39104 (1 up to 24 ms, 2 up to 60, 3 up to 96, else 4) over 47 steps; the
     * offsets are the tables 0x005F8260 (out) and 0x005F831C - 4 k (in); at the end the sidebar
     * state flips and the city view is redrawn */
    int32_t dt = S32(st, 0x00E39104u);
    int32_t step = S32(st, 0x00F53F7Cu);
    if (dt <= 0x18) {
        step += 1;
    } else if (dt <= 0x3C) {
        step += 2;
    } else if (dt <= 0x60) {
        step += 3;
    } else {
        step += 4;
    }
    S32(st, 0x00F53F7Cu) = step;
    if (step >= 0x2F) {
        if (S32(st, SIDEBAR_COLLAPSED) == 0) {
            GAME_CALL0(st, 0x004DDEC0u);
            S32(st, SIDEBAR_COLLAPSED) = 1;
        } else {
            GAME_CALL0(st, 0x004DDF20u);
            S32(st, SIDEBAR_COLLAPSED) = 0;
        }
        U8(st, WINDOW_ID) = 1;
        GAME_CALL0(st, 0x004DE080u);
        GAME_CALL(st, 0x004D71C0u, 1);
        return;
    }
    int32_t dx;
    if (S32(st, SIDEBAR_COLLAPSED) == 1) {
        dx = S32(st, 0x005F831Cu - 4u * (uint32_t) step);
    } else {
        dx = S32(st, 0x005F8260u + 4u * (uint32_t) step);
    }
    GAME_CALL0(st, 0x004DDDC0u);
    int32_t w = S32(st, SCREEN_WIDTH);
    if (w == 0x400) {
        S32(st, 0x00D3B558u) = 1000; /* the clip's right edge, at the screen's decorated border */
    } else if (w == 0x320) {
        S32(st, 0x00D3B558u) = 0x2F8;
    }
    U8(st, REDRAW_SIDEBAR) = 1;
    draw_image(st, S32(st, SIDEBAR_IMAGES) + 1, S32(st, SIDEBAR_X_COLLAPSED), 0x1E);
    buttons_draw(st, S32(st, SIDEBAR_X_COLLAPSED), 0x1E, 0x005C75B0u, 0x0C);
    buttons_draw(st, S32(st, SIDEBAR_X_COLLAPSED), 0x1E, 0x005C7348u, 1);
    draw_image(st, S32(st, SIDEBAR_IMAGES), S32(st, SIDEBAR_X) + dx, 0x1E);
    buttons_draw(st, S32(st, SIDEBAR_X) + dx, 0x1E, 0x005C7370u, 0x10);
    buttons_draw(st, S32(st, SIDEBAR_X) + dx, 0x1E, 0x005C7300u, 2);
    buttons_draw(st, S32(st, SIDEBAR_X) + dx, 0x1E, 0x005C7980u, 2);
    sidebar_draw_overlay_name(st, 1, S32(st, SIDEBAR_X) + dx + 0x0B);
    sidebar_draw_build_title(st, 1, S32(st, SIDEBAR_X) + dx + 0x0B);
    int32_t m = dx <= 8 ? dx : 8;
    GAME_CALL(st, 0x004CD9D0u, S32(st, SIDEBAR_X) + dx + 8, 0x3B, m - dx + 0x91, 0x6F, 0);
    w = S32(st, SCREEN_WIDTH);
    if (w == 0x400) {
        draw_image(st, S32(st, SIDEBAR_IMAGES) + 5, S32(st, SIDEBAR_X_COLLAPSED), 0x1E0);
        draw_image(st, S32(st, SIDEBAR_IMAGES) + 4, S32(st, SIDEBAR_X) + dx, 0x1E0);
    } else if (w == 0x320) {
        draw_image(st, S32(st, SIDEBAR_IMAGES) + 7, S32(st, SIDEBAR_X_COLLAPSED), 0x1E0);
        draw_image(st, S32(st, SIDEBAR_IMAGES) + 6, S32(st, SIDEBAR_X) + dx, 0x1E0);
    }
    S32(st, 0x00D3B558u) = S32(st, SCREEN_WIDTH);
}

void panel_draw(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h)
{
    /* @bin 0x00536820 a bordered panel of w x h tiles of 16 pixels (images 0x00F5683C + k: top
     * row 0, 1..10, 11; middle rows 12 + r, 13 + c + r, 23 + r with r cycling by 12 up to 120;
     * bottom row 132, 133..142, 143; the inner column index c cycles 0..9) */
    int32_t base_row = 0, step = 0, c = 0;
    for (int32_t row = 0; row < h; row++) {
        c = 0;
        int32_t ty = row * 16 + y;
        for (int32_t col = 0; col < w; col++) {
            int32_t tx = col * 16 + x;
            int32_t id;
            S32(st, 0x00D3B5E0u) = tx;
            S32(st, 0x00D3B5DCu) = ty;
            int32_t base = S32(st, 0x00F5683Cu);
            if (row == 0) {
                if (col == 0) {
                    id = base;
                } else if (col >= w - 1) {
                    id = base + 0x0B;
                } else {
                    id = base + c + 1;
                    c++;
                }
                step = 0;
            } else if (row >= h - 1) {
                if (col == 0) {
                    id = base + 0x84;
                } else if (col >= w - 1) {
                    id = base + 0x8F;
                } else {
                    id = base + c + 0x85;
                    c++;
                }
                step = 0;
            } else {
                if (col == 0) {
                    id = base + base_row + 0x0C;
                } else if (col >= w - 1) {
                    id = base + base_row + 0x17;
                } else {
                    id = base + c + base_row + 0x0D;
                    c++;
                }
                step = 0x0C;
            }
            S32(st, 0x00D3B5C0u) = id;
            draw_image(st, id, tx, ty);
            if (c >= 10) {
                c = 0;
            }
        }
        base_row += step;
        if (base_row >= 0x78) {
            base_row = 0;
        }
    }
}

void sidebar_draw_city_texts(game_state *st)
{
    /* @bin 0x0051F790 the city view's texts each frame: the editor / victory banners, the
     * countdowns (0x00784C48 time left to the victory, 0x00784C50 a deadline: months left on a red
     * band), cheat and debug texts, the event levels, and the pause panel */
    int32_t font = S32(st, CITY_FONT_SMALL);
    if (U8(st, 0x006DE514u) != 0 || S32(st, CURRENT_CITY(st) + 0x44D0u) != 0) {
        /* @bin 0x0051F79C / 0x0051F81A the banner text 6:6 (or 6:5 for city +0x44D0), a black shadow
         * then 0x7F9A (555) / 0xFF3A (565) */
        int32_t id = U8(st, 0x006DE514u) != 0 ? 6 : 5;
        S32(st, TEXT_X) = 0;
        int32_t colour = S32(st, 0x00E92DFCu) == 2 ? -0xC6 : 0x7F9A;
        text_group(st, 6, id, 8, 0x2F, font, 0);
        text_group(st, 6, id, 7, 0x2E, font, (int16_t) colour);
    } else if (U16(st, 0x00784B60u) == 0) {
        if (S32(st, 0x00784C48u) != 0) {
            int32_t kind = S32(st, CURRENT_CITY(st) + 0x47A8u);
            int32_t end = S32(st, 0x0078D7B8u);
            if (kind == 0) {
                end += 7;
            } else if (kind == 1) {
                end += 2;
            }
            int32_t year = S32(st, 0x0078D7BCu);
            int32_t left = end > year + 1 ? end - year - 1 : 0;
            int32_t months = (int32_t) (((uint32_t) left * 3u + 3u) * 4u - (uint32_t) S32(st, 0x0078D7C0u));
            GAME_CALL(st, 0x004CDA40u, 1, 0x28, 0xE6, 0x14, 0x1FE3);
            font = S32(st, CITY_FONT_SMALL);
            for (int32_t i = 0; i <= 1; i++) {
                for (int32_t y = 0x2D; y - 0x2E <= 1; y += 2) {
                    S32(st, TEXT_X) = 0;
                    text_group(st, 6, 2, i + 7, y, font, 0);
                    text_number(st, months, NUMBER_SUFFIX, S32(st, TEXT_X) + i + 7, y, font, 0);
                }
            }
            S32(st, TEXT_X) = 0;
            text_group(st, 6, 2, 7, 0x2E, font, 0xF800);
            text_number(st, months, NUMBER_SUFFIX, S32(st, TEXT_X) + 7, 0x2E, font, 0xF800);
        } else if (S32(st, 0x00784C50u) != 0) {
            int32_t end = S32(st, 0x0078D7B8u);
            int32_t year = S32(st, 0x0078D7BCu);
            int32_t left = end > year + 1 ? end - year - 1 : 0;
            int32_t months = (int32_t) (((uint32_t) left * 3u + 3u) * 4u - (uint32_t) S32(st, 0x0078D7C0u));
            GAME_CALL(st, 0x004CDA40u, 1, 0x2A, 0xC8, 0x12, 0x1FE3);
            S32(st, TEXT_X) = 0;
            text_group(st, 6, 3, 2, 0x2E, S32(st, CITY_FONT_SMALL), 0xF800);
            text_number(st, months, NUMBER_SUFFIX, S32(st, TEXT_X) + 2, 0x2E, S32(st, CITY_FONT_SMALL), 0xF800);
        }
    }
    S32(st, TEXT_X) = 0;
    GAME_CALL0(st, 0x0051F780u);
    GAME_CALL0(st, 0x0051F780u);
    if (S32(st, 0x00605A0Cu) == 1) {
        /* @bin 0x0051FAA3 the debug "Allow All" (0x005F89C0), with costs skipped (Ctrl+A) */
        GAME_CALL(st, 0x004CC770u, 0x005F89C0, S32(st, TEXT_X) + 10, 0x3D, S32(st, CITY_FONT_SMALL), 0xFFFF);
    }
    if (S32(st, 0x00F53F70u) != 0) {
        /* @bin 0x0051FAD4 debug: "Happiness" city +0x10 and "Max prosperity" city +0x46A8 */
        text_number(st, city_i32(st, CITY_HAPPINESS), 0x005F89B4u, 2, 0x50, S32(st, CITY_FONT_SMALL), 0);
        text_number(st, city_i32(st, CITY_HAPPINESS), 0x005F89B4u, 1, 0x51, S32(st, CITY_FONT_SMALL), 0xFFFF);
        text_number(st, city_i32(st, 0x46A8u), 0x005F89A4u, 2, 100, S32(st, CITY_FONT_SMALL), 0);
        text_number(st, city_i32(st, 0x46A8u), 0x005F89A4u, 1, 0x65, S32(st, CITY_FONT_SMALL), 0xFFFF);
    }
    if (S32(st, 0x00605A00u) != 0) {
        /* @bin 0x0051FBCC debug: the event 0x00605A08 (text group 0x9D), its kind 0x00605A00 and
         * level 0x00605A04 */
        int32_t saved = S32(st, TEXT_X);
        S32(st, TEXT_X) = 0;
        text_group(st, 0x9D, S32(st, 0x00605A08u), 0, 0x79, S32(st, CITY_FONT_SMALL), 0xFFFF);
        static const uint32_t kinds[4] = {0x005F8998u, 0x005F898Cu, 0x005F8980u, 0x005F8974u};
        uint32_t k = (uint32_t) S32(st, 0x00605A00u) - 1u;
        if (k <= 3) {
            GAME_CALL(st, 0x004CC770u, (int32_t) kinds[k], S32(st, TEXT_X) + 1, 0x79, S32(st, CITY_FONT_SMALL), 0xFFFF);
        }
        GAME_CALL(st, 0x004CC770u, 0x005F896C, S32(st, TEXT_X), 0x65, S32(st, CITY_FONT_SMALL), 0xFFFF);
        text_number(st, S32(st, 0x00605A04u), 0x005F8964u, S32(st, TEXT_X) + 2, 0x79, S32(st, CITY_FONT_SMALL),
                    0xFFFF);
        S32(st, TEXT_X) = saved;
    }
    if (U8(st, 0x00E38E60u) != 0) {
        /* @bin 0x0051FCF9 paused: a 28 x 3 panel centred on the city view, text 13:2 */
        int32_t x = (cityview_pixel_width(st) - 0x1C0) / 2;
        panel_draw(st, x, 0x28, 0x1C, 3);
        GAME_CALL(st, 0x004D6250u, 0x0D, 2, x, 0x3A, 0x1C0, S32(st, CITY_FONT_LARGE), 0, S32(st, LAYOUT_DX),
                  S32(st, LAYOUT_DY));
    }
}

/* ---- port table ---- */

static int32_t run_set_x(game_state *st, const int32_t *a) { (void) a; sidebar_set_x(st); return 0; }
static int32_t run_draw(game_state *st, const int32_t *a) { (void) a; sidebar_draw(st); return 0; }
static int32_t run_buttons(game_state *st, const int32_t *a) { (void) a; sidebar_draw_buttons(st); return 0; }
static int32_t run_overlay_name(game_state *st, const int32_t *a) { sidebar_draw_overlay_name(st, a[0], a[1]); return 0; }
static int32_t run_build_title(game_state *st, const int32_t *a) { sidebar_draw_build_title(st, a[0], a[1]); return 0; }
static int32_t run_minimap(game_state *st, const int32_t *a) { sidebar_draw_minimap(st, a[0]); return 0; }
static int32_t run_city_texts(game_state *st, const int32_t *a) { (void) a; sidebar_draw_city_texts(st); return 0; }
static int32_t run_topbar(game_state *st, const int32_t *a) { topbar_draw(st, a[0]); return 0; }
static int32_t run_messages(game_state *st, const int32_t *a) { sidebar_draw_messages(st, a[0]); return 0; }
static int32_t run_slide(game_state *st, const int32_t *a) { (void) a; sidebar_slide(st); return 0; }
static int32_t run_buttons_draw(game_state *st, const int32_t *a) { buttons_draw(st, a[0], a[1], (uint32_t) a[2], a[3]); return 0; }
static int32_t run_menubar(game_state *st, const int32_t *a) { menubar_draw(st, (uint32_t) a[0], a[1]); return 0; }
static int32_t run_menubar_button(game_state *st, const int32_t *a) { (void) a; menubar_draw_button(st); return 0; }
static int32_t run_topbar_bg(game_state *st, const int32_t *a) { (void) a; topbar_draw_background(st); return 0; }
static int32_t run_panel(game_state *st, const int32_t *a) { panel_draw(st, a[0], a[1], a[2], a[3]); return 0; }
static int32_t run_view_width(game_state *st, const int32_t *a) { (void) a; return cityview_pixel_width(st); }

static int32_t run_edge_1024(game_state *st, const int32_t *a)
{
    (void) a;
    draw_image(st, S32(st, SIDEBAR_IMAGES) + 2, 1000, 0);
    return 0;
}

static int32_t run_edge_800(game_state *st, const int32_t *a)
{
    (void) a;
    draw_image(st, S32(st, SIDEBAR_IMAGES) + 3, 0x2F8, 0);
    return 0;
}

const ported_function sidebar_ports[] = {
    {0x0051E930u, "sidebar_set_x", 0, 0, 0, run_set_x, 0},
    {0x0051E990u, "sidebar_draw", 0, 0, 0, run_draw, 0},
    {0x0051EC90u, "sidebar_draw_buttons", 0, 0, 0, run_buttons, 0},
    {0x0051ED30u, "sidebar_draw_overlay_name", 2, 0, 0, run_overlay_name, 0},
    {0x0051EDD0u, "sidebar_draw_build_title", 2, 0, 0, run_build_title, 0},
    {0x0051EFB0u, "sidebar_draw_minimap", 1, 0, 0, run_minimap, 0},
    {0x0051F790u, "sidebar_draw_city_texts", 0, 0, 0, run_city_texts, 0},
    {0x0051F0A0u, "topbar_draw", 1, 0, 0, run_topbar, 0},
    {0x0051F680u, "sidebar_draw_messages", 1, 0, 0, run_messages, 0},
    {0x0051FD60u, "sidebar_slide", 0, 0, 0, run_slide, 0},
    {0x00425730u, "buttons_draw", 4, 0, 0, run_buttons_draw, 0},
    {0x00425330u, "menubar_draw", 2, 0, 0, run_menubar, 0},
    {0x00425900u, "menubar_draw_button", 0, 0, 0, run_menubar_button, 0},
    {0x00536670u, "topbar_draw_background", 0, 0, 0, run_topbar_bg, 0},
    {0x00536820u, "panel_draw", 4, 0, 0, run_panel, 0},
    {0x00430CC0u, "cityview_pixel_width", 0, 32, 0, run_view_width, 0},
    {0x00537120u, "sidebar_draw_edge_1024", 0, 0, 0, run_edge_1024, 0},
    {0x00537100u, "sidebar_draw_edge_800", 0, 0, 0, run_edge_800, 0},
};
const int sidebar_ports_count = sizeof(sidebar_ports) / sizeof(sidebar_ports[0]);
