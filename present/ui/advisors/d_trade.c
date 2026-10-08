/* Imhotep - the advisors, group d: the trade advisor (page 6) and the trade resource dialog
 * (window 0x28).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The page's resource list is 0x00F53E70 (i32, 1-based, count 0x00F53E6C: goods_available_update
 * 0x005596D0); the city's list scroll is CITY+0x4834 (first row shown, 1-based); CITY+0x49CC (u8)
 * shows prices instead of stock and orders; the resource of the dialog is 0x0060598C. City fields
 * per resource (docs/commands.md 3.4): +0x2826 i16 status (0 none, 1 import, 2 export, 3 ...),
 * +0x286E i16 limit, +0x28B6 i16 industry on/off, +0x2A88 i32 stockpile, +0x48F0 i32 stored
 * amount. Hover indexes of the input dispatch 0x004D8310: 0x007056C8 (page: row buttons from 4),
 * 0x007056C4, 0x007056C0 (dialog). */
#include "advisors.h"
#include "d.h"

#include "city/day_buildings.h"
#include "city/day_city.h"
#include "city/goods.h"
#include "draw/draw.h"
#include "draw/text.h"
#include "sidebar/sidebar.h"
#include "sim/call.h"
#include "sim/math.h"
#include "state/ported.h"
#include "state/records.h"

#define LIST_COUNT 0x00F53E6Cu      /* i32; entries i32 at +4 i (1..count) */
#define ANIM 0x00F53F48u            /* i32 the window's opening step (draws once > 5) */
#define TEXT_X 0x00E20778u          /* i32 text cursor x after a draw */
#define HOVER_1 0x007056C8u
#define HOVER_2 0x007056C4u
#define HOVER_3 0x007056C0u
#define TRADE_RESOURCE 0x0060598Cu
#define RESOURCE_IMAGES 0x00F571FCu /* i32 first resource icon */
#define FONT_9C 0x00F56F9Cu
#define FONT_A4 0x00F56FA4u
#define FONT_94 0x00F56F94u
#define SUFFIX 0x0057C048u          /* " " */
#define ROUTES_RECT 0x00603320u     /* page 6 rect table entry 2: the open routes' tooltip area */
#define INDUSTRY_TOTAL 0x0078D6DCu  /* i32 per resource: industry buildings */
#define INDUSTRY_WORKING 0x0078D64Cu /* i32 per resource */
#define PRICE_BUY 0x005D2C48u       /* i32 per resource (8-byte entries) */
#define PRICE_SELL 0x005D2C4Cu

#define C_SCROLL 0x4834u
#define C_DIALOG 0x4838u
#define C_PRICES 0x49CCu
#define C_STATUS 0x2826u
#define C_LIMIT 0x286Eu
#define C_INDUSTRY 0x28B6u
#define C_STOCKPILE 0x2A88u
#define C_STORED 0x48F0u
#define C_ROUTE_LIMIT 0x2894u

static int32_t px(game_state *st) { return S32(st, ADV_PANEL_X); }
static int32_t py(game_state *st) { return S32(st, ADV_PANEL_Y); }
static int32_t dx(game_state *st) { return S32(st, ADV_DX); }
static int32_t dy(game_state *st) { return S32(st, ADV_DY); }
static uint32_t city(game_state *st) { return CURRENT_CITY(st); }

static int16_t status_of(game_state *st, int32_t r)
{
    return S16(st, city(st) + C_STATUS + 2u * (uint32_t) r);
}

static int16_t limit_of(game_state *st, int32_t r)
{
    return S16(st, city(st) + C_LIMIT + 2u * (uint32_t) r);
}

/* @bin 0x0055ACCC (and its copies) resources 0x18-0x1A, 0x1E: counted in blocks (hundredths) */
static int is_block(int32_t r)
{
    static const uint8_t other[12] = {0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0};
    uint32_t k = (uint32_t) r - 0x18u; /* @bin 0x0055A3C7 unsigned compare (ja) */
    return k <= 0x0B && other[k] == 0;
}

/* @bin 0x0055A3E2 amounts in hundredths: the blocks and resources 0x1C, 0x0A, 0x24 */
static int in_hundredths(int32_t r)
{
    return is_block(r) || r == 0x1C || r == 0x0A || r == 0x24;
}

static void group(game_state *st, int32_t g, int32_t i, int32_t x, int32_t y, int32_t font)
{
    layout_group(st, g, i, x, y, font, 0, dx(st), dy(st));
}

static void group_centred(game_state *st, int32_t g, int32_t i, int32_t x, int32_t y, int32_t w, int32_t font)
{
    layout_group_centred(st, g, i, x, y, w, font, 0, dx(st), dy(st));
}

static void number(game_state *st, int32_t v, int32_t x, int32_t y, int32_t font)
{
    layout_number_at(st, v, 0x40, SUFFIX, x, y, font, 0, dx(st), dy(st));
}

static void frame(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h, int32_t lit)
{
    layout_frame(st, 0, x, y, w, h, lit, dx(st), dy(st));
}

void trade_advisor_draw(game_state *st)
{
    /* @bin 0x00559E10 page 6: the goods lists, the panel (40 x 27 pieces at the layout's origin),
     * shrinking rectangles while the window opens (0x00F53F48 1..4), the title and the column
     * headers (prices or stock / orders) */
    goods_available_update(st);
    int32_t step = S32(st, ANIM);
    int32_t x = dx(st);
    int32_t y = dy(st);
    int32_t k = 5 - step;
    S32(st, ADV_PANEL_X) = x;
    S32(st, ADV_PANEL_Y) = y;
    S32(st, ADV_PANEL_W) = 0x28;
    S32(st, ADV_PANEL_H) = 0x1B;
    if (k >= 5) {
        return;
    }
    if (k > 0) {
        rect_draw(st, k * 16 + x, y + k * 8, (0x28 - 2 * k) << 4, (0x1B - 2 * k) << 4, 0);
        return;
    }
    panel_draw(st, x, y, 0x28, 0x1B);
    image_draw(st, S32(st, 0x00F5681Cu) + 4, px(st) + 10, py(st) + 10);
    group(st, 0x36, 0, px(st) + 0x3C, py(st) + 0x0C, S32(st, ADV_FONT_LARGE));
    if (U8(st, city(st) + C_PRICES) == 0) {
        group(st, 0x36, 1, px(st) + 0x64, py(st) + 0x26, S32(st, ADV_FONT_NORMAL));
    } else {
        group(st, 0x36, 0x16, px(st) + 0xAF, py(st) + 0x26, S32(st, ADV_FONT_NORMAL));
        group(st, 0x36, 0x17, px(st) + 0x113, py(st) + 0x26, S32(st, ADV_FONT_NORMAL));
    }
}

void trade_advisor_draw_scroll_thumb(game_state *st)
{
    /* @bin 0x0055A060 the list's scroll thumb (more than 15 rows): at 256 * (scroll - 1) / (count -
     * 15) % of the bar (0 at the top, 100 % past the end) */
    int32_t count = S32(st, LIST_COUNT);
    int32_t range = count - 0x0F;
    if (range <= 0) {
        return;
    }
    int32_t scroll = S32(st, city(st) + C_SCROLL);
    int32_t pct;
    if (scroll <= 1) {
        pct = 0;
    } else if (scroll + 0x0E > count) {
        pct = 0x64;
    } else {
        pct = calc_ratio_percent(scroll - 1, range);
    }
    int32_t off = calc_percentage(0x100, pct);
    image_draw(st, S32(st, 0x00F56494u) + 0x27, px(st) + 0x255, py(st) + off + 0x4E);
}

/* @bin 0x0055A596 the open routes (0x00559FC0 into the 16 counters kept for the whole list): one
 * route's name (group 0x17, 0x27 + its kind), or "several" (0x36/0x2D) with the tooltip rectangle
 * 0x00603320 moved to the row */
static void routes(game_state *st, int32_t *kinds, int32_t n, int32_t row, int32_t y)
{
    if (n == 1) {
        int32_t i = 0;
        while (kinds[i] == 0) {
            i++;
            if (i >= 0x10) {
                return;
            }
        }
        group(st, 0x17, i + 0x27, S32(st, TEXT_X) + px(st) + 0x144, py(st) + y + 0x3D, S32(st, FONT_9C));
    } else if (n != 0) {
        int32_t t = row * 22;
        U16(st, ROUTES_RECT + 6) = (uint16_t) (t + 0x4C);
        U16(st, ROUTES_RECT + 2) = (uint16_t) (t + 0x38);
        group(st, 0x36, 0x2D, S32(st, TEXT_X) + px(st) + 0x144, py(st) + y + 0x3D, S32(st, FONT_9C));
    }
}

static void row_orders(game_state *st, int32_t r, int32_t *kinds, int32_t row, int32_t y)
{
    int32_t font = S32(st, FONT_9C);
    int32_t ty = py(st) + y + 0x3D;
    int16_t status = status_of(st, r);
    if (status == 0) {
        /* @bin 0x0055A4A9 no order: who sells / buys it */
        int32_t idx;
        if (GAME_CALL(st, 0x00443950u, r) != 0 && GAME_CALL(st, 0x00443B20u, r) != 0) {
            idx = 0x21;
        } else if (GAME_CALL(st, 0x00443950u, r) != 0) {
            idx = 0x1F;
        } else if (GAME_CALL(st, 0x00443B20u, r) != 0) {
            idx = 0x20;
        } else {
            if (GAME_CALL(st, 0x00443A20u, r) != 0 && GAME_CALL(st, 0x004439C0u, r) != 0) {
                idx = 0x24;
            } else if (GAME_CALL(st, 0x004439C0u, r) != 0) {
                idx = 0x22;
            } else if (GAME_CALL(st, 0x00443A20u, r) != 0) {
                idx = 0x23;
            } else {
                return;
            }
            font = S32(st, FONT_94);
        }
        group(st, 0x36, idx, px(st) + 0x144, py(st) + y + 0x3D, font);
        return;
    }
    if (status == 1) {
        /* @bin 0x0055A57C importing */
        S32(st, TEXT_X) = 0;
        if (r == 0x13) {
            S32(st, TEXT_X) = 0;
            int32_t n = trade_open_routes_559fc0(st, kinds);
            group(st, 0x36, 5, px(st) + 0x144, ty, S32(st, FONT_9C));
            int32_t v = S16(st, city(st) + C_ROUTE_LIMIT) * 100;
            number(st, v, S32(st, TEXT_X) + px(st) + 0x144, py(st) + y + 0x3D, S32(st, FONT_9C));
            routes(st, kinds, n, row, y);
            return;
        }
        group(st, 0x36, 5, px(st) + 0x144, ty, font);
        int32_t v = limit_of(st, r) * 100;
        if (in_hundredths(r)) {
            v /= 100;
        }
        if (is_block(r)) {
            text_count_draw(st, 8, 0x38, v, S32(st, TEXT_X) + px(st) + 0x144, py(st) + y + 0x3D, S32(st, FONT_9C), 0);
        } else {
            number(st, v, S32(st, TEXT_X) + px(st) + 0x144, py(st) + y + 0x3D, S32(st, FONT_9C));
        }
        return;
    }
    if (status == 2) {
        /* @bin 0x0055A808 exporting */
        S32(st, TEXT_X) = 0;
        group(st, 0x36, 6, px(st) + 0x144, ty, font);
        int32_t v = limit_of(st, r) * 100;
        if (in_hundredths(r)) {
            v /= 100;
        }
        number(st, v, S32(st, TEXT_X) + px(st) + 0x144, py(st) + y + 0x3D, S32(st, FONT_9C));
        return;
    }
    if (status == 3) {
        if (r == 0x13) {
            /* @bin 0x0055A8EF */
            S32(st, TEXT_X) = 0;
            int32_t n = trade_open_routes_559fc0(st, kinds);
            group(st, 0x36, 0x25, px(st) + 0x144, ty, S32(st, FONT_9C));
            routes(st, kinds, n, row, y);
            return;
        }
        group(st, 0x36, 0x25, px(st) + 0x144, ty, font);
        return;
    }
    group(st, 0x36, 0x26, px(st) + 0x144, ty, font);
}

static void row_prices(game_state *st, int32_t r, int32_t y)
{
    /* @bin 0x0055AA55 buying and selling prices, and the unit */
    number_draw_centred(st, S32(st, PRICE_BUY + 8u * (uint32_t) r), 0x40, SUFFIX, px(st) + 0xAF, py(st) + y + 0x3D,
                        0x64, S32(st, FONT_9C), 0);
    number_draw_centred(st, S32(st, PRICE_SELL + 8u * (uint32_t) r), 0x40, SUFFIX, px(st) + 0x113,
                        py(st) + y + 0x3D, 0x64, S32(st, FONT_9C), 0);
    int32_t idx;
    if (is_block(r)) {
        idx = 0x31;
    } else if (r == 0x0A) {
        idx = 0x2F;
    } else {
        idx = r != 0x1C ? 0x32 : 0x30;
    }
    group(st, 0x36, idx, px(st) + 0x1A9, py(st) + y + 0x3D, S32(st, FONT_9C));
}

void trade_advisor_draw_list(game_state *st)
{
    /* @bin 0x0055A0E0 page 6's foreground: up to 15 rows of the resource list from the scroll
     * (clamped to [1, count - 14]), each with icon, name (another font when its industry is off),
     * stock and stockpiling or prices, its order; the scroll bar; the prices / empire buttons */
    int32_t kinds[16] = {0};
    uint32_t sc = city(st) + C_SCROLL;
    int32_t count = S32(st, LIST_COUNT);
    if (count - S32(st, sc) + 1 < 0x0F) {
        S32(st, sc) = count - 0x0E;
    }
    if (S32(st, sc) < 1) {
        S32(st, sc) = 1;
    }
    int32_t i = S32(st, sc);
    if (S32(st, ANIM) <= 5) {
        return;
    }
    layout_inner_panel(st, px(st) + 0x10, py(st) + 0x34, 0x24, 0x15, dx(st), dy(st));
    if (S32(st, LIST_COUNT) >= 0x0F) {
        /* @bin 0x0055A1AD the scroll arrows (0x005C5B90 up, 0x005C5B78 down) and the bar */
        GAME_CALL(st, 0x00425E80u, px(st) + 0x23D, py(st) + 0x34, 0x005C5B90, 1, 10);
        GAME_CALL(st, 0x00425C40u, px(st) + 0x23D, py(st) + 0x34, 0x005C5B90, 1);
        GAME_CALL(st, 0x004256A0u, px(st) + 0x23D, py(st) + 0x34, 0x005C5B90, 1);
        GAME_CALL(st, 0x00425E80u, px(st) + 0x255, py(st) + 0x16C, 0x005C5B78, 1, 10);
        GAME_CALL(st, 0x00425C40u, px(st) + 0x255, py(st) + 0x16C, 0x005C5B78, 1);
        GAME_CALL(st, 0x004256A0u, px(st) + 0x255, py(st) + 0x16C, 0x005C5B78, 1);
        layout_inner_panel(st, px(st) + 0x251, py(st) + 0x4C, 2, 0x12, dx(st), dy(st));
    }
    U16(st, ROUTES_RECT + 2) = 0;
    U16(st, ROUTES_RECT + 6) = 0;
    uint32_t entry = LIST_COUNT + 4u * (uint32_t) i;
    int32_t row = 0;
    for (int32_t y = 0;;) {
        int32_t r = S32(st, entry);
        image_draw(st, S32(st, RESOURCE_IMAGES) + r, px(st) + 0x16, py(st) + y + 0x39);
        if (row == S32(st, HOVER_1) - 4) {
            frame(st, px(st) + 0x14, py(st) + y + 0x36, 0x23A, 0x18, 1);
        }
        int32_t font = S16(st, city(st) + C_INDUSTRY + 2u * (uint32_t) r) == 1 ? S32(st, FONT_A4) : S32(st, FONT_9C);
        group(st, 0x17, r, px(st) + 0x2C, py(st) + y + 0x3D, font);
        if (U8(st, city(st) + C_PRICES) == 0) {
            int32_t v = S32(st, city(st) + C_STORED + 4u * (uint32_t) r);
            if (in_hundredths(r)) {
                v /= 100;
            }
            number_draw_centred(st, v, 0x40, SUFFIX, px(st) + 0x8C, py(st) + y + 0x3D, 0x64, S32(st, FONT_9C), 0);
            if (S32(st, city(st) + C_STOCKPILE + 4u * (uint32_t) r) != 0) {
                group(st, 0x36, 3, px(st) + 0xE8, py(st) + y + 0x3D, S32(st, FONT_9C));
            }
            row_orders(st, r, kinds, row, y);
        } else {
            row_prices(st, r, y);
        }
        i++;
        entry += 4;
        if (i > S32(st, LIST_COUNT)) {
            break;
        }
        row++;
        y += 0x16;
        if (y >= 0x14A) {
            break;
        }
    }
    /* @bin 0x0055AB65 the buttons: prices / stock (hover 1), the empire (hover 2) */
    frame(st, px(st) + 0x18E, py(st) + 0x18C, 0xC8, 0x18, S32(st, HOVER_1) == 1);
    group_centred(st, 0x36, U8(st, city(st) + C_PRICES) != 0 ? 0x2E : 2, px(st) + 0x190, py(st) + 0x192, 0xC8,
                  S32(st, ADV_FONT_NORMAL));
    frame(st, px(st) + 0x62, py(st) + 0x18C, 0xC8, 0x18, S32(st, HOVER_1) == 2);
    group_centred(st, 0x36, 0x1E, px(st) + 0x64, py(st) + 0x192, 0xC8, S32(st, ADV_FONT_NORMAL));
    trade_advisor_draw_scroll_thumb(st);
}

/* @bin 0x0055AF9A the industry line: "n buildings", "n of m working", "industry off" */
static void dialog_industry(game_state *st, int32_t r, int32_t res)
{
    int32_t font = S32(st, ADV_FONT_NORMAL);
    int32_t y = py(st) + 0xAC;
    uint32_t total = INDUSTRY_TOTAL + 4u * (uint32_t) res;
    int32_t n = S32(st, total);
    if (n <= 0) {
        group(st, 0x36, 7, px(st) + 0x62, y, font);
        return;
    }
    if (S16(st, city(st) + C_INDUSTRY + 2u * (uint32_t) r) == 1) {
        number(st, n, px(st) + 0x62, py(st) + 0xAC, S32(st, ADV_FONT_NORMAL));
        group(st, 0x36, S32(st, total) == 1 ? 0x0A : 0x0B, S32(st, TEXT_X) + px(st) + 0x62, py(st) + 0xAC,
              S32(st, ADV_FONT_NORMAL));
        return;
    }
    uint32_t working = INDUSTRY_WORKING + 4u * (uint32_t) res;
    if (n == S32(st, working)) {
        number(st, n, px(st) + 0x62, py(st) + 0xAC, S32(st, ADV_FONT_NORMAL));
        group(st, 0x36, S32(st, total) == 1 ? 8 : 9, S32(st, TEXT_X) + px(st) + 0x62, py(st) + 0xAC,
              S32(st, ADV_FONT_NORMAL));
        return;
    }
    number(st, S32(st, working), px(st) + 0x62, py(st) + 0xAC, S32(st, ADV_FONT_NORMAL));
    group(st, 0x36, 0x0C, S32(st, TEXT_X) + px(st) + 0x62, py(st) + 0xAC, S32(st, ADV_FONT_NORMAL));
    number(st, S32(st, total) - S32(st, working), S32(st, TEXT_X) + px(st) + 0x62, py(st) + 0xAC,
           S32(st, ADV_FONT_NORMAL));
    group(st, 0x36, S32(st, working) == 1 ? 0x0D : 0x0E, S32(st, TEXT_X) + px(st) + 0x62, py(st) + 0xAC,
          S32(st, ADV_FONT_NORMAL));
}

/* @bin 0x0055B4E6 an order column (import at x0 0x1F, export at 0x14B): its label and limit when
 * the status is `active`, else the centred alternative */
static void dialog_order(game_state *st, int32_t active, int32_t idx, int32_t x_label, int32_t x_block,
                         int32_t x_number, int32_t x_centred)
{
    int32_t r = S32(st, TRADE_RESOURCE);
    if (S16(st, city(st) + C_STATUS + 2u * (uint32_t) r) != active) {
        group_centred(st, 0x36, idx, px(st) + x_centred, py(st) + 0xDD, 0xC8, S32(st, ADV_FONT_NORMAL));
        return;
    }
    S32(st, TEXT_X) = 0;
    group(st, 0x36, idx, px(st) + x_label, py(st) + 0xDD, S32(st, ADV_FONT_NORMAL));
    r = S32(st, TRADE_RESOURCE);
    if (in_hundredths(r)) {
        if (is_block(r)) {
            text_count_draw(st, 8, 0x38, limit_of(st, r), S32(st, TEXT_X) + px(st) + x_block, py(st) + 0xDD,
                            S32(st, ADV_FONT_NORMAL), 0);
        } else {
            number(st, limit_of(st, r), S32(st, TEXT_X) + px(st) + x_number, py(st) + 0xDD, S32(st, ADV_FONT_NORMAL));
        }
    } else {
        number(st, limit_of(st, r) * 100, S32(st, TEXT_X) + px(st) + x_number, py(st) + 0xDD,
               S32(st, ADV_FONT_NORMAL));
    }
}

void trade_dialog_draw(game_state *st, int32_t unused)
{
    /* @bin 0x0055AD30 window 0x28, the resource 0x0060598C: panel 38 x 15 pieces, icon and name, the
     * industry line, the stock, the import and export columns (framed when they can be clicked; lit
     * by hover 0x007056C4 = 1, 2), the industry and stockpile buttons. The argument is not read. */
    (void) unused;
    uint8_t can_import = 1;
    uint8_t can_export = 1;
    S32(st, city(st) + C_DIALOG) = 1;
    panel_draw(st, px(st) + 0x10, py(st) + 0x80, 0x26, 0x0F);
    int32_t r = S32(st, TRADE_RESOURCE);
    image_draw(st, S32(st, RESOURCE_IMAGES) + r, px(st) + 0x1A, py(st) + 0x88);
    group(st, 0x17, S32(st, TRADE_RESOURCE), px(st) + 0x3C, py(st) + 0x89, S32(st, ADV_FONT_LARGE));
    r = S32(st, TRADE_RESOURCE);
    int32_t imp = 0;
    int32_t exp = 0;
    uint32_t status = (uint32_t) (int32_t) S16(st, city(st) + C_STATUS + 2u * (uint32_t) r);
    /* @bin 0x0055ADFF the labels of the two columns by status (jump table 0x0055BAFC; others
     * default) */
    if (status == 0) {
        if (GAME_CALL(st, 0x00443950u, r) != 0) {
            imp = 0x27;
        } else {
            imp = GAME_CALL(st, 0x004439C0u, S32(st, TRADE_RESOURCE)) != 0 ? 0x22 : 0x29;
            can_import = 0;
        }
        if (GAME_CALL(st, 0x00443B20u, S32(st, TRADE_RESOURCE)) != 0) {
            exp = 0x28;
        } else if (GAME_CALL(st, 0x00443A20u, S32(st, TRADE_RESOURCE)) != 0) {
            exp = 0x23;
            can_export = 0;
        } else {
            exp = 0x2A;
            can_export = 0;
        }
    } else if (status == 1 || status == 3) {
        imp = status == 1 ? 0x13 : 0x2B;
        if (GAME_CALL(st, 0x00443B20u, r) != 0) {
            exp = 0x28;
        } else if (GAME_CALL(st, 0x00443A20u, S32(st, TRADE_RESOURCE)) != 0) {
            exp = 0x23;
            can_export = 0;
        } else {
            exp = 0x2A;
            can_export = 0;
        }
    } else {
        exp = status == 2 ? 0x14 : 0x2C;
        if (GAME_CALL(st, 0x00443950u, r) != 0) {
            imp = 0x27;
        } else {
            imp = GAME_CALL(st, 0x004439C0u, S32(st, TRADE_RESOURCE)) != 0 ? 0x22 : 0x29;
            can_import = 0;
        }
    }
    r = S32(st, TRADE_RESOURCE);
    int32_t res = r == 9 ? 1 : r; /* @bin 0x0055AF44 resource 9 counts the industry of 1 */
    S32(st, TEXT_X) = 0;
    if (GAME_CALL(st, 0x00443B90u, res) == 0) {
        group(st, 0x36, 0x19, px(st) + 0x62, py(st) + 0xAC, S32(st, ADV_FONT_NORMAL));
    } else {
        dialog_industry(st, S32(st, TRADE_RESOURCE), res);
    }
    /* @bin 0x0055B2BC the stock */
    r = S32(st, TRADE_RESOURCE);
    int32_t stored = S32(st, city(st) + C_STORED + 4u * (uint32_t) r);
    S32(st, TEXT_X) = 0;
    if (is_block(r)) {
        text_count_draw(st, 8, 0x38, stored / 100, px(st) + 0x62, py(st) + 0xC0, S32(st, ADV_FONT_NORMAL), 0);
    } else {
        if (in_hundredths(r)) {
            stored /= 100;
        }
        number(st, stored, px(st) + 0x62, py(st) + 0xC0, S32(st, ADV_FONT_NORMAL));
    }
    group(st, 0x36, 0x0F, S32(st, TEXT_X) + px(st) + 0x62, py(st) + 0xC0, S32(st, ADV_FONT_NORMAL));
    /* @bin 0x0055B3E0 the columns' frames (0xF800 when hovered) */
    if (can_import) {
        frame(st, px(st) + 0x1A, py(st) + 0xD4, 0x123, 0x1E, S32(st, HOVER_2) == 1 ? 0xF800 : 0);
    }
    if (can_export) {
        frame(st, px(st) + 0x141, py(st) + 0xD4, 0x123, 0x1E, S32(st, HOVER_2) == 2 ? 0xF800 : 0);
    }
    dialog_order(st, 1, imp, 0x1F, 0x18, 0x1A, 0x26);
    dialog_order(st, 2, exp, 0x14B, 0x144, 0x146, 0x14E);
    /* @bin 0x0055B8C6 the industry button (when there is industry), the stockpile button */
    if (S32(st, INDUSTRY_TOTAL + 4u * (uint32_t) res) > 0) {
        frame(st, px(st) + 0x62, py(st) + 0xFA, 0x1B0, 0x1E, S32(st, HOVER_1) != 0);
        r = S32(st, TRADE_RESOURCE);
        group_centred(st, 0x36, S16(st, city(st) + C_INDUSTRY + 2u * (uint32_t) r) == 0 ? 0x10 : 0x11, px(st) + 0x72,
                      py(st) + 0x103, 0x190, S32(st, ADV_FONT_NORMAL));
    }
    frame(st, px(st) + 0x62, py(st) + 0x120, 0x1B0, 0x32, S32(st, HOVER_3) != 0);
    r = S32(st, TRADE_RESOURCE);
    int on = S32(st, city(st) + C_STOCKPILE + 4u * (uint32_t) r) != 0;
    group_centred(st, 0x36, on ? 0x1A : 0x1C, px(st) + 0x72, py(st) + 0x128, 0x190, S32(st, ADV_FONT_NORMAL));
    group_centred(st, 0x36, on ? 0x1B : 0x1D, px(st) + 0x72, py(st) + 0x13C, 0x190, S32(st, ADV_FONT_NORMAL));
}

int32_t trade_advisor_scroll_input(game_state *st)
{
    /* @bin 0x0055BB90 page 6: the left button held on the scroll bar (x 0x250..0x26C, y 0x4E..0x16E
     * of the panel) sets the scroll from the position; 1 when it did (full redraw 2) */
    int32_t count = S32(st, LIST_COUNT);
    if (count <= 0x0F) {
        return 0;
    }
    if (U8(st, 0x00E38E00u) == 0) {
        return 0;
    }
    int32_t mx = S32(st, 0x00E38E34u);
    int32_t x = px(st);
    if (mx < x + 0x250 || mx > x + 0x26C) {
        return 0;
    }
    int32_t y = py(st);
    int32_t my = S32(st, 0x00E38E30u);
    if (my < y + 0x4E || my > y + 0x16E) {
        return 0;
    }
    int32_t off = my - y - 0x4E;
    if (off > 0x100) {
        off = 0x100;
    }
    int32_t v = calc_percentage(count - 0x0F, calc_ratio_percent(off, 0x100));
    U8(st, ADV_REDRAW) = 2;
    S32(st, city(st) + C_SCROLL) = v + 1;
    return 1;
}
