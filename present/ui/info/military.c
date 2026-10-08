/* Imhotep - window 9: info panels (military): the legion (formation) panel and the ship panels
 * (package "info").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Kinds 4 (legion), 6 (transport ship) and 5 (warship) of the info window (0x004F8E60 calls the
 * panel with the formation 0x00EADCCC; the ship panels read their figure from 0x00EADCC8 /
 * 0x00EADCC4). Each draws the window panel, its texts and the five order buttons; the ships' order
 * sub-panels pick the lit button and the help text from the hovered button 0x007056C8 or the ship's
 * action. The per-frame draw of the legion (0x00510B00, from 0x004F94A0) redraws its buttons. */
#include "info.h"

#include "figure/formation.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define GFX 0x00F55BB0u          /* `this` of the image draws */
#define IMG_DRAW 0x0053E590u     /* image id at (x, y) */
#define IMG_FLIPPED 0x0053FA40u  /* the same, flipped horizontally */
#define PANEL 0x00536820u        /* the window panel (x, y, w, h in blocks) */
#define TEXT_CENTRED 0x004D6250u /* layout text centred (group, index, x, y, width, font, colour, dx, dy) */
#define TEXT_LINES 0x004D6160u   /* layout text wrapped (group, index, x, y, width, font, colour, dx, dy) */
#define TEXT 0x004D62F0u         /* layout text (group, index, x, y, font, colour, dx, dy) */
#define NUMBER 0x004D6860u       /* layout number (value, prefix, suffix, x, y, font, colour, dx, dy) */
#define FRAME 0x004D63D0u        /* layout frame (kind, x, y, w, h, lit, dx, dy) */
#define INNER 0x004D65B0u        /* layout inner panel (x, y, w, h, dx, dy) */
#define GREY 0x004CF090u         /* a greyed rectangle (x, y, w, h, shift) */
#define PERCENT 0x004CDE60u      /* value in percent of a maximum */
#define SHIP_DOCKED 0x004A4300u  /* u8: the ship's "return to dock" order is unavailable (assumed) */

#define TEXT_WIDTH 0x00E20778u   /* i32 the width of the last layout text (written by 0x004D62F0) */
#define IMG_X 0x00D3B5E0u        /* i32 the image draw's x, kept in a global by the button loops */
#define IMG_Y 0x00D3B5DCu        /* i32 */
#define FONT_SMALL_TITLE 0x00F56F9Cu /* i32 the order sub-panel's title font */
#define FONT_SMALL_TEXT 0x00F56F94u  /* i32 the order sub-panel's text font */
#define IMAGE_COUNT 0x0126517Cu  /* i32 image entries */
#define IMAGE_ENTRIES 0x010D6A00u /* image entries (0x40 bytes): +0xC width, +0xE height (i16) */

static int32_t px(game_state *st) { return S32(st, INFO_X); }
static int32_t py(game_state *st) { return S32(st, INFO_Y); }

static void draw_image(game_state *st, int32_t id, int32_t x, int32_t y)
{
    GAME_CALLT(st, GFX, IMG_DRAW, id, x, y);
}

static void text(game_state *st, int32_t group, int32_t index, int32_t dx, int32_t dy, uint32_t font)
{
    int32_t x = px(st), y = py(st);
    GAME_CALL(st, TEXT, group, index, x + dx, y + dy, S32(st, font), 0, x, y);
}

/* @bin 0x0051024F the image entry of id (entry 0 past the count): width (+0xC) or height (+0xE) */
static int32_t image_dim(game_state *st, int32_t id, uint32_t field)
{
    int32_t i = id < S32(st, IMAGE_COUNT) ? id : 0;
    return S16(st, IMAGE_ENTRIES + field + ((uint32_t) i << 6));
}

/* @bin 0x0050F3D5 the 0..100 percentage's text, (0x0050F5B7 the same steps from 0x1A) */
static int32_t morale_text(int32_t m)
{
    if (m <= 0) {
        return 0x1A;
    }
    if (m <= 0x14) {
        return 0x1B;
    }
    if (m <= 0x28) {
        return 0x1C;
    }
    if (m <= 0x37) {
        return 0x1D;
    }
    if (m <= 0x46) {
        return 0x1E;
    }
    return m > 0x5A ? 0x20 : 0x1F;
}

/* the experience / health lines common to the legion and the transport (0x0050F4F2.., 0x0051043B..):
 * soldiers, morale, health image and text, then the training or the health line */
static void formation_lines(game_state *st, uint32_t f, int32_t y_soldiers, int32_t y_morale, int32_t y_health,
                            int32_t y_last)
{
    text(st, 0x8A, 0x17, 0x64, y_soldiers, INFO_FONT_TEXT);
    {
        int32_t x = px(st), y = py(st);
        GAME_CALL(st, NUMBER, U8(st, f + 0x28), 0x40, 0x0057C048, x + 0x126, y + y_soldiers,
                  S32(st, INFO_FONT_TEXT), 0, x, y);
    }
    int32_t m = GAME_CALL(st, PERCENT, S16(st, f + 0x4A), S16(st, f + 0x4C));
    text(st, 0x8A, 0x18, 0x64, y_morale, INFO_FONT_TEXT);
    text(st, 0x8A, morale_text(m), 0x12C, y_morale, INFO_FONT_TEXT);
    S32(st, TEXT_WIDTH) = 0;
    text(st, 0x8A, 0x49, 0x64, y_health, INFO_FONT_TEXT);
    text(st, 0x8A, 0x19, S32(st, TEXT_WIDTH) + 0x64, y_health, INFO_FONT_TEXT);
    {
        /* @bin 0x0050F6AB (health + 10) / 20: the image and the text */
        int32_t y = py(st) + y_health, x = px(st) + 0x113;
        draw_image(st, (S16(st, f + 0x7A) + 10) / 20 + S32(st, 0x00F5642Cu), x, y);
    }
    text(st, 0x8A, (S16(st, f + 0x7A) + 10) / 20 + 0x3C, 0x12C, y_health, INFO_FONT_TEXT);
    if (S16(st, f + 0x64) != 0) {
        text(st, 0x8A, 0x3B, 0x64, y_last, INFO_FONT_TEXT);
    } else {
        S32(st, TEXT_WIDTH) = 0;
        text(st, 0x8A, 0x49, 0x64, y_last, INFO_FONT_TEXT);
        text(st, 0x8A, 0x24, S32(st, TEXT_WIDTH) + 0x64, y_last, INFO_FONT_TEXT);
        /* @bin 0x0050F7C7 training / 5 */
        text(st, 0x8A, S16(st, f + 0x2C) / 5 + 0x25, 0x12C, y_last, INFO_FONT_TEXT);
    }
}

/* ---- the ships' order sub-panels (0x0050F890 transport, 0x0050FE50 warship) ---- */

static int32_t ship_selected(int warship, uint8_t action)
{
    if (warship) {
        /* @bin 0x0050FF5E jump table 0x00510178 / 0x00510164 on action - 10 */
        switch (action) {
        case 10: return 2;
        case 13: return 3;
        case 20: return 0;
        case 23: return 1;
        default: return 4;
        }
    }
    /* @bin 0x0050F99E jump table 0x0050FBD4 / 0x0050FBC0 on action - 10 */
    switch (action) {
    case 10: return 1;
    case 11: return 3;
    case 12: case 13: return 2;
    case 20: return 0;
    default: return 4;
    }
}

static int32_t ship_help(game_state *st, uint32_t f, int warship)
{
    int32_t h = S32(st, INFO_BUTTON_HOVER);
    if (h != 0) {
        /* @bin 0x0050FABC / 0x0051007C the hovered button's text */
        if (h == 1) {
            return warship ? 9 : 0x13;
        }
        if (h == 2) {
            return warship ? 0xB : 0x15;
        }
        if (h == 3) {
            if (warship) {
                return 0xD;
            }
            return U8(st, f + 0x130) != 0 ? 0x19 : 0x17;
        }
        return h == 4 ? 0xF : 0x11;
    }
    uint8_t action = U8(st, f + 0x38);
    if (warship) {
        /* @bin 0x005100C9 jump table 0x0051019C / 0x00510188 on action - 10 */
        switch (action) {
        case 10: return 0xD;
        case 13: return 0xF;
        case 20: return 9;
        case 23: return 0xB;
        default: return 0x11;
        }
    }
    /* @bin 0x0050FB16 jump table 0x0050FBF8 / 0x0050FBE0 on action - 10 */
    switch (action) {
    case 10: return 0x15;
    case 11: return 0xF;
    case 12: return 0x17;
    case 13: return 0x19;
    case 20: return 0x13;
    default: return 0x11;
    }
}

static void ship_orders(game_state *st, int32_t fig, int warship)
{
    /* @bin 0x0050F8F1 / 0x0050FEAC the ship's busy action: 0xB transport, 0xD warship (0xF both) */
    uint8_t busy = warship ? 0x0D : 0x0B;
    {
        int32_t x = px(st), y = py(st);
        GAME_CALL(st, INNER, x + 0x10, y + 0xE6, S32(st, INFO_W) - 2, 5, x, y);
    }
    uint32_t f = FIGURE_ADDR(fig);
    int32_t pct = GAME_CALL(st, PERCENT, S32(st, f + 0x128), S32(st, f + 0x12C));
    int32_t sel = 1;
    uint8_t action = U8(st, f + 0x38);
    if (!((action == busy || action == 0x0F) && pct <= 15)) {
        int flag = 0;
        for (int32_t i = 0; i < 5; i++) {
            uint32_t e = (uint32_t) (S32(st, INFO_BUTTON_HOVER) - 1);
            if (e > 4) {
                sel = 1;
                flag = 1;
                continue;
            }
            /* @bin 0x0050FEDC jump table 0x00510150 (0x0050F91C 0x0050FBAC) on hover - 1 */
            if (e <= 2) {
                if (i == (int32_t) e) {
                    sel = i;
                }
            } else if (e == 3) {
                if (i == 3) {
                    action = U8(st, f + 0x38);
                    if (action != busy && action != 0x0F && pct != 100) {
                        sel = i;
                    } else {
                        flag = 1;
                    }
                }
            } else {
                if (i == 4 && (uint8_t) GAME_CALL(st, SHIP_DOCKED, fig) == 0) {
                    sel = i;
                } else if (i == S32(st, INFO_BUTTON_HOVER) - 1) {
                    flag = 1;
                }
            }
        }
        if (flag) {
            sel = ship_selected(warship, U8(st, f + 0x38));
        }
    }
    for (int32_t i = 0; i < 5; i++) {
        int32_t x = px(st), y = py(st);
        GAME_CALL(st, FRAME, 0, i * 85 + x + 0x13, y + 0x8B, 0x54, 0x54, sel == i ? 1 : 0, x, y);
        action = U8(st, f + 0x38);
        if ((action == busy || action == 0x0F) && pct <= 15) {
            /* @bin 0x0050FFCF every button greyed while the crew is below 16 % */
            GAME_CALL(st, GREY, i * 85 + px(st) + 0x13, py(st) + 0x8B, 0x54, 0x54, 0);
        } else if (i == 3) {
            if (action == busy || action == 0x0F || pct == 100) {
                GAME_CALL(st, GREY, px(st) + 0x112, py(st) + 0x8B, 0x54, 0x54, 0);
            }
        } else if (i == 4) {
            if ((uint8_t) GAME_CALL(st, SHIP_DOCKED, fig) != 0) {
                GAME_CALL(st, GREY, px(st) + 0x167, py(st) + 0x8B, 0x54, 0x54, 0);
            }
        }
    }
    int32_t t = ship_help(st, f, warship);
    text(st, 0xB8, t, 0x18, 0xEC, FONT_SMALL_TITLE);
    {
        int32_t x = px(st), y = py(st);
        GAME_CALL(st, TEXT_LINES, 0xB8, t + 1, x + 0x18, y + 0xFC, (S32(st, INFO_W) - 4) << 4,
                  S32(st, FONT_SMALL_TEXT), 0, x, y);
    }
}

void info_transport_orders(game_state *st, int32_t fig)
{
    /* @bin 0x0050F890 the transport ship's order buttons and the hovered order's text */
    ship_orders(st, fig, 0);
}

void info_warship_orders(game_state *st, int32_t fig)
{
    /* @bin 0x0050FE50 the warship's order buttons and the hovered order's text */
    ship_orders(st, fig, 1);
}

static void panel_and_title(game_state *st, int32_t help, int32_t group, int32_t index)
{
    S32(st, INFO_HELP) = help;
    GAME_CALL(st, PANEL, px(st), py(st), S32(st, INFO_W), S32(st, INFO_H));
    int32_t x = px(st), y = py(st);
    GAME_CALL(st, TEXT_CENTRED, group, index, x, y + 0xA, S32(st, INFO_W) << 4, S32(st, INFO_FONT_LARGE), 0, x, y);
}

void info_transport(game_state *st)
{
    /* @bin 0x0050F320 the transport ship's panel (kind 6), help 0x16F */
    int32_t fig = S32(st, INFO_TRANSPORT);
    panel_and_title(st, 0x16F, 0xB8, 0);
    uint32_t f = FIGURE_ADDR(fig);
    int32_t pct = GAME_CALL(st, PERCENT, S32(st, f + 0x128), S32(st, f + 0x12C));
    text(st, 0xB8, 2, 0x64, 0x28, INFO_FONT_TEXT);
    /* @bin 0x0050F3D5 the crew's text by percentage */
    int32_t t;
    if (pct <= 0) {
        t = 8;
    } else if (pct <= 0x1E) {
        t = 7;
    } else if (pct <= 0x32) {
        t = 6;
    } else if (pct <= 0x46) {
        t = 5;
    } else {
        t = pct <= 0x5A ? 4 : 3;
    }
    text(st, 0xB8, t, 0x12C, 0x28, INFO_FONT_TEXT);
    uint32_t form = FORMATION_ADDR(U8(st, f + 0x92));
    if (U8(st, f + 0x130) != 0) {
        /* @bin 0x0050F45B the legion on board: its type (0xB, 0xC, else) */
        int32_t k;
        int32_t type = S16(st, form + 4);
        if (type == 0xB) {
            k = 0x1F;
        } else if (type == 0xC) {
            k = 0x21;
        } else {
            k = 0x20;
        }
        S32(st, TEXT_WIDTH) = 0;
        text(st, 0xB8, k, 0x64, 0x3C, INFO_FONT_TEXT);
        text(st, 0x8A, U8(st, form + 2), S32(st, TEXT_WIDTH) + 0x64, 0x3C, INFO_FONT_TEXT);
        text(st, 0xB8, 0x22, S32(st, TEXT_WIDTH) + 0x64, 0x3C, INFO_FONT_TEXT);
        formation_lines(st, form, 0x4C, 0x5C, 0x6C, 0x7C);
    }
    for (int32_t i = 0; i < 5; i++) {
        int32_t x = i * 85 + px(st) + 0x15;
        int32_t y = py(st) + 0x8D;
        int32_t id;
        if (i == 2 && U8(st, f + 0x130) != 0) {
            id = S32(st, 0x00F55CE4u) + 3;
        } else if (i == 4 && (uint8_t) GAME_CALL(st, SHIP_DOCKED, fig) != 0) {
            id = S32(st, 0x00F55CE4u) + 6;
        } else {
            /* @bin 0x0050F84F the buttons' images: table 0x005F78DC {0, 1, 2, 4, 5} */
            id = S32(st, 0x005F78DCu + 4u * (uint32_t) i) + S32(st, 0x00F55CE4u);
        }
        draw_image(st, id, x, y);
    }
    info_transport_orders(st, fig);
}

void info_warship(game_state *st)
{
    /* @bin 0x0050FCA0 the warship's panel (kind 5), help 0x16D */
    int32_t fig = S32(st, INFO_WARSHIP);
    panel_and_title(st, 0x16D, 0xB8, 1);
    text(st, 0xB8, 2, 0x64, 0x3C, INFO_FONT_TEXT);
    int32_t t = GAME_CALL(st, 0x0050FC10u, fig);
    text(st, 0xB8, t, 0x12C, 0x3C, INFO_FONT_TEXT);
    text(st, 0xB8, 0x1B, 0x64, 0x50, INFO_FONT_TEXT);
    t = GAME_CALL(st, 0x0050FC70u, fig);
    text(st, 0xB8, t, 0x12C, 0x50, INFO_FONT_TEXT);
    for (int32_t i = 0; i < 5; i++) {
        S32(st, IMG_Y) = py(st) + 0x8D;
        S32(st, IMG_X) = i * 85 + px(st) + 0x15;
        int32_t id;
        if (i == 4 && (uint8_t) GAME_CALL(st, SHIP_DOCKED, fig) != 0) {
            id = S32(st, 0x00F55CDCu) + 5;
        } else {
            id = S32(st, 0x00F55CDCu) + i;
        }
        draw_image(st, id, S32(st, IMG_X), S32(st, IMG_Y));
    }
    info_warship_orders(st, fig);
}

/* ---- the legion ---- */

static void legion_button(game_state *st, uint32_t form, int32_t i, int32_t base, int special_first)
{
    int32_t x = S32(st, IMG_X), y = S32(st, IMG_Y);
    int32_t mode = S32(st, 0x00BCA2E0u);
    int plain = mode == 0 || mode == 4;
    if (special_first && i == 1) {
        /* @bin 0x005108BF type 0xD: button 1 by +0x7E */
        draw_image(st, base + (S16(st, form + 0x7E) != 0 ? 6 : 1), x, y);
        return;
    }
    if (U8(st, form + 0x77) != 0 && i < 4) {
        /* @bin 0x00510908 the formation buttons, flipped unless the mode 0x00BCA2E0 is 0 or 4 */
        if (plain) {
            draw_image(st, base + i, x, y);
        } else {
            GAME_CALLT(st, GFX, IMG_FLIPPED, base + i, x, y);
        }
        return;
    }
    if (i == 4 && U8(st, form + 3) != 0) {
        draw_image(st, base + 5, x, y);
        return;
    }
    /* @bin 0x0051096A the other buttons: flipped when the mode is 0 or 4 */
    if (plain) {
        GAME_CALLT(st, GFX, IMG_FLIPPED, base + i, x, y);
    } else {
        draw_image(st, base + i, x, y);
    }
}

void info_legion(game_state *st, int32_t formation)
{
    /* @bin 0x005101B0 the legion's panel (kind 4), help 0x57 */
    uint32_t form = FORMATION_ADDR(formation);
    S32(st, INFO_HELP) = 0x57;
    GAME_CALL(st, PANEL, px(st), py(st), S32(st, INFO_W), S32(st, INFO_H));
    S32(st, INFO_DISTRIBUTION) = 0;
    {
        int32_t x = px(st), y = py(st);
        GAME_CALL(st, TEXT_CENTRED, 0x8A, U8(st, form + 2), x, y + 0xA, S32(st, INFO_W) << 4,
                  S32(st, INFO_FONT_LARGE), 0, x, y);
    }
    /* @bin 0x00510228 the banner (images 0x00F56814 + the legion's number) */
    int32_t id = U8(st, form + 2) + S32(st, 0x00F56814u);
    int32_t banner_h = image_dim(st, id, 0xE);
    draw_image(st, id, (0x28 - image_dim(st, id, 0xC)) / 2 + px(st) + 0x10, py(st) + 0x10);
    /* @bin 0x00510292 the soldier figure image and the title by type: 0xD, 0xC, else */
    int32_t type = S16(st, form + 4);
    int32_t title;
    if (type == 0xD) {
        id = S32(st, 0x00F5680Cu);
        title = 0x4C;
    } else if (type == 0xC) {
        id = S32(st, 0x00F5680Cu) + 0x12;
        title = 0x4A;
    } else {
        id = S32(st, 0x00F5680Cu) + 9;
        title = 0x4B;
    }
    if (S16(st, form + 0x5A) != 0) {
        id += 8;
    }
    int32_t w2 = image_dim(st, id, 0xC);
    int32_t h2 = image_dim(st, id, 0xE);
    draw_image(st, id, (0x28 - w2) / 2 + px(st) + 0x10, py(st) + banner_h + 0x10);
    {
        int32_t x = px(st), y = py(st);
        GAME_CALL(st, TEXT_CENTRED, 0x8A, title, x, y + 0x1E, S32(st, INFO_W) << 4, S32(st, INFO_FONT_LARGE), 0, x, y);
    }
    /* @bin 0x00510363 the training and health bars: images 0x00F565CC - training / 5 + 20 and
     * 0x00F56B1C - (health + 3) / 5 + 20, under the soldier */
    id = S32(st, 0x00F565CCu) - S16(st, form + 0x2C) / 5 + 0x14;
    draw_image(st, id, (0x28 - image_dim(st, id, 0xC)) / 2 + px(st) + 0x10, py(st) + h2 + banner_h + 0x10);
    id = S32(st, 0x00F56B1Cu) - (S16(st, form + 0x7A) + 3) / 5 + 0x14;
    draw_image(st, id, (0x28 - image_dim(st, id, 0xC)) / 2 + px(st) + 0x10, py(st) + h2 + banner_h + 0x10);

    formation_lines(st, form, 0x3C, 0x50, 0x64, 0x78);

    if (U8(st, form + 0x28) == 0) {
        /* @bin 0x005107CB no soldier left: the legion's state text */
        int32_t x = px(st), y = py(st);
        int32_t group = 0x8A, index;
        if (S16(st, form + 0x64) != 0) {
            group = 0x59;
            index = 1;
        } else {
            index = S32(st, 0x007058B8u) != 0 ? 0xA : 0xB;
        }
        GAME_CALL(st, TEXT_LINES, group, index, x + 0x20, y + 0xAC, (S32(st, INFO_W) - 4) << 4,
                  S32(st, INFO_FONT_TEXT), 0, x, y);
        return;
    }
    for (int32_t i = 0; i < 5; i++) {
        S32(st, IMG_X) = i * 85 + px(st) + 0x15;
        S32(st, IMG_Y) = py(st) + 0x8D;
        /* @bin 0x0051089E the buttons' images by type: 0xD 0x00F55CD4, 0xB 0x00F55CCC, else 0x00F55CC4 */
        int16_t t = S16(st, form + 4);
        if (t == 0xD) {
            legion_button(st, form, i, S32(st, 0x00F55CD4u), 1);
        } else if (t == 0xB) {
            legion_button(st, form, i, S32(st, 0x00F55CCCu), 0);
        } else {
            legion_button(st, form, i, S32(st, 0x00F55CC4u), 0);
        }
    }
}

void info_legion_frame(game_state *st, int32_t formation)
{
    /* @bin 0x00510B00 the legion's per-frame draw: the lit button, the order's text, the return button */
    uint32_t form = FORMATION_ADDR(formation);
    if (U8(st, form + 0x28) == 0) {
        return;
    }
    for (int32_t i = 0; i < 5; i++) {
        int32_t h = S32(st, INFO_BUTTON_HOVER);
        int lit = 0;
        if (h != 0) {
            lit = i == h - 1;
        } else {
            /* @bin 0x00510B2E the button of the legion's formation +0x2A */
            int16_t a = S16(st, form + 0x2A);
            if ((a == 2 || a == 1) && i == 0) {
                lit = 1;
            } else if ((a == 3 || a == 4) && i == 1) {
                lit = 1;
            } else if (a == 0 && i == 1 && S16(st, form + 0x7E) == 0) {
                lit = 1;
            } else if (a == 0xF && i == 2) {
                lit = 1;
            } else if (a == 6 && i == 3) {
                lit = 1;
            }
        }
        int32_t x = px(st), y = py(st);
        GAME_CALL(st, FRAME, 0, i * 85 + x + 0x13, y + 0x8B, 0x54, 0x54, lit, x, y);
    }
    {
        int32_t x = px(st), y = py(st);
        GAME_CALL(st, INNER, x + 0x10, y + 0xE6, S32(st, INFO_W) - 2, 5, x, y);
    }
    int32_t title, body;
    /* @bin 0x00510BEF the texts of the hovered button, else (0x00510C5B) of the formation's +0x2A */
    switch (S32(st, INFO_BUTTON_HOVER)) {
    case 1:
        title = 0xC;
        body = 0xD;
        break;
    case 2:
        if (S16(st, form + 4) == 0xD) {
            title = 0x14;
            body = 0x15;
        } else {
            title = 0xE;
            body = 0xF;
        }
        break;
    case 3:
        title = 0x10;
        body = 0x11;
        break;
    case 4:
        title = 0x12;
        body = 0x13;
        break;
    case 5:
        title = 0x3A;
        body = 0x16;
        break;
    default:
        switch (S16(st, form + 0x2A)) {
        case 1: case 2:
            title = 0xC;
            body = 0xD;
            break;
        case 3: case 4:
            title = 0xE;
            body = 0xF;
            break;
        case 0:
            title = 0x14;
            body = 0x15;
            break;
        case 0xF:
            title = 0x10;
            body = 0x11;
            break;
        case 6:
            title = 0x12;
            body = 0x13;
            break;
        default:
            title = 0x3A;
            body = 0x16;
            break;
        }
        break;
    }
    text(st, 0x8A, title, 0x18, 0xEC, FONT_SMALL_TITLE);
    {
        int32_t x = px(st), y = py(st);
        GAME_CALL(st, TEXT_LINES, 0x8A, body, x + 0x18, y + 0xFC, (S32(st, INFO_W) - 4) << 4,
                  S32(st, FONT_SMALL_TEXT), 0, x, y);
    }
    {
        /* @bin 0x00510D1E the return button (lit when 0x007056C4 is 1) and its text */
        int lit = S32(st, INFO_BUTTON_HOVER2) == 1;
        int32_t y = py(st), x = px(st);
        GAME_CALL(st, FRAME, 0, ((S32(st, INFO_W) - 0x14) << 4) / 2 + x, (S32(st, INFO_H) << 4) + y - 0x28, 0x140,
                  0x1E, lit, x, y);
        y = py(st);
        x = px(st);
        GAME_CALL(st, TEXT_CENTRED, 0x8A, 0x4D, ((S32(st, INFO_W) - 0x14) << 4) / 2 + x,
                  ((S32(st, INFO_H) - 2) << 4) + y, 0x140, S32(st, INFO_FONT_TEXT), 0, x, y);
    }
}

static int32_t run_legion(game_state *st, const int32_t *a)
{
    info_legion(st, a[0]);
    return 0;
}

static int32_t run_legion_frame(game_state *st, const int32_t *a)
{
    info_legion_frame(st, a[0]);
    return 0;
}

static int32_t run_transport(game_state *st, const int32_t *a)
{
    (void) a;
    info_transport(st);
    return 0;
}

static int32_t run_transport_orders(game_state *st, const int32_t *a)
{
    info_transport_orders(st, a[0]);
    return 0;
}

static int32_t run_warship(game_state *st, const int32_t *a)
{
    (void) a;
    info_warship(st);
    return 0;
}

static int32_t run_warship_orders(game_state *st, const int32_t *a)
{
    info_warship_orders(st, a[0]);
    return 0;
}

const ported_function info_military_ports[] = {
    {0x005101B0u, "info_legion", 1, 0, 0, run_legion, 0},
    {0x00510B00u, "info_legion_frame", 1, 0, 0, run_legion_frame, 0},
    {0x0050F320u, "info_transport", 1, 0, 0, run_transport, 0},
    {0x0050F890u, "info_transport_orders", 1, 0, 0, run_transport_orders, 0},
    {0x0050FCA0u, "info_warship", 1, 0, 0, run_warship, 0},
    {0x0050FE50u, "info_warship_orders", 1, 0, 0, run_warship_orders, 0},
};
const int info_military_ports_count = sizeof(info_military_ports) / sizeof(info_military_ports[0]);
