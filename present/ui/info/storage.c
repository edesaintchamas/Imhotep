/* Imhotep - window 9: info panels of the storage buildings (package "info").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The panels of building types 0x46 / 0xBE, 0x47 / 0xBF and 0x48 / 0xC1 (reached from the redraw
 * 0x004F8E60 by its jump table: the orders sub-panel when 0x00EADCC0 is set, else the main panel),
 * and their per-frame draws (reached from 0x004F94A0: the orders button, the order rows with their
 * hover highlight and the fill-limit buttons). The orders' resource lists 0x00F53DD8 (count +
 * 0x00F53DDC[]) and 0x00F53E6C (count + 0x00F53E70[]) are only read here: no function of this file
 * writes them (checked in the disassembly: the stores of these functions are the help id
 * 0x00EADD54, the sound flag 0x00EADD94, the text cursor 0x00E20778, the redraw flag 0x00E38DCA
 * and the storage yard's scroll row at building +0xDC). Every drawing goes through the primitives
 * of present/draw/ by GAME_CALL; the layout (which text, image, where) is from each function's
 * disassembly. */
#include "info.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define IMAGES 0x00F55BB0u          /* `this` of the image draw 0x0053E590 */
#define TEXT_X 0x00E20778u          /* i32 text cursor x after a draw */
#define REDRAW_ALL 0x00E38DCAu      /* u8 full redraw of the window this frame */
#define PANEL_ICONS 0x00F571FCu     /* i32 first image of the panels' resource icons */
#define SCROLL_IMAGES 0x00F56494u   /* i32 image group of the scroll bar thumb (0x00502840: + 0x27) */
#define FONT_SMALL 0x00F56F94u      /* i32 the font of the cart-pusher line */
#define FONT_ROW 0x00F56F9Cu        /* i32 the font of the order rows */
#define FONT_RED 0x00F56FA4u        /* i32 the font of the refused / full lines */
#define NUMBER_SUFFIX 0x0057C048u   /* the empty suffix string of the numbers */
#define ROW_HOVER 0x007056C4u       /* i32 the hovered order row (+1) */
#define GRANARY_SPECIAL 0x00784BBCu /* i32 granaries show text 0x62/4 instead of their stock when set */
#define YARD_NOTICE 0x00EADCB8u     /* i32 1 / 2: the storage yard's notice (texts 0x63/0xD, 0x63/0xE) */
#define ORDER_BUTTONS 0x005C81F8u   /* button tables of the order rows, 0x48 bytes per row */
#define SCROLL_UP 0x005C5B48u       /* the storage yard orders' scroll buttons */
#define SCROLL_DOWN 0x005C5B60u

/* @bin 0x007900C2 storage records of 0xC4 bytes (index: building +0xC1): u8 order per resource at
 * +0, i16 fill limits at +0x2A + 2 res (accepting) and +0x72 + 2 res (getting) */
#define STORAGE_ORDERS 0x007900C2u
#define STORAGE_FILL 0x007900ECu
#define STORAGE_GET 0x00790134u
#define STORAGE_SIZE 0xC4u

/* @bin building fields read by these panels (0x007999D8 + 0x108 id) */
#define B_SPACE_RESOURCE 0x12u /* i16 a storage space's resource */
#define B_FIGURE 0x2Au         /* i16 the cart pusher */
#define B_NEXT_SPACE 0x3Au     /* i16 next storage space of a yard */
#define B_SPACE_AMOUNT 0x3Cu   /* i16 a storage space's amount */
#define B_WORKERS 0x40u        /* i16 */
#define B_STOCK 0x56u          /* i16[] stock by index (bazaar: 0x00500B79's mapping; granary: by resource) */
#define B_FIGURE2 0xB6u        /* i16 a second cart pusher */
#define B_STORAGE 0xC1u        /* u8 the storage record */
#define B_YARD_SCROLL 0xDCu    /* i32 the storage yard panel's scroll row */
#define B_ORDERS_SCROLL 0xE0u  /* i32 the storage yard orders' scroll row */

/* @bin figure fields (0x00BCD5F8 + 0x184 id) */
#define F_RESOURCE_IMAGE 0x0Bu /* u8 the carried resource (its icon) */
#define F_STATE 0x0Eu          /* u8 1: alive */
#define F_ACTION2 0x10u        /* u8 the action shown when +0x38 is 6 or 7 */
#define F_ACTION 0x38u         /* u8 */

static int32_t X(game_state *st) { return S32(st, INFO_X); }
static int32_t Y(game_state *st) { return S32(st, INFO_Y); }
static int32_t W(game_state *st) { return S32(st, INFO_W); }
static int32_t H(game_state *st) { return S32(st, INFO_H); }
static int32_t F(game_state *st, uint32_t font) { return S32(st, font); }

static uint32_t building(game_state *st)
{
    return BUILDING_ADDR(S32(st, INFO_BUILDING));
}

static uint32_t figure_addr(int32_t f)
{
    return FIGURE_BASE + FIGURE_SIZE * (uint32_t) f;
}

static void image(game_state *st, int32_t id, int32_t x, int32_t y)
{
    /* @bin 0x0053E590 thiscall on 0x00F55BB0 (image, x, y) */
    GAME_CALLT(st, IMAGES, 0x0053E590u, id, x, y);
}

static void text(game_state *st, int32_t group, int32_t id, int32_t x, int32_t y, int32_t font)
{
    /* @bin 0x004D62F0 (group, index, x, y, font, colour 0, origin x, origin y) */
    GAME_CALL(st, 0x004D62F0u, group, id, x, y, font, 0, X(st), Y(st));
}

static void text_box(game_state *st, int32_t group, int32_t id, int32_t x, int32_t y, int32_t w, int32_t font)
{
    /* @bin 0x004D6160 a wrapped text (group, index, x, y, width, font, 0, origin x, origin y) */
    GAME_CALL(st, 0x004D6160u, group, id, x, y, w, font, 0, X(st), Y(st));
}

static void text_centred(game_state *st, int32_t group, int32_t id, int32_t x, int32_t y, int32_t w, int32_t font)
{
    /* @bin 0x004D6250 a centred text (group, index, x, y, width, font, 0, origin x, origin y) */
    GAME_CALL(st, 0x004D6250u, group, id, x, y, w, font, 0, X(st), Y(st));
}

static void number(game_state *st, int32_t value, int32_t x, int32_t y, int32_t font)
{
    /* @bin 0x004D6860 a number (prefix 0x40, the empty suffix 0x0057C048) */
    GAME_CALL(st, 0x004D6860u, value, 0x40, (int32_t) NUMBER_SUFFIX, x, y, font, 0, X(st), Y(st));
}

static void inner_panel(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h)
{
    /* @bin 0x004D65B0 (x, y, width, height in blocks, origin x, origin y) */
    GAME_CALL(st, 0x004D65B0u, x, y, w, h, X(st), Y(st));
}

static void frame_box(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h, int32_t on)
{
    /* @bin 0x004D63D0 a highlight frame (0, x, y, width, height, on, origin x, origin y) */
    GAME_CALL(st, 0x004D63D0u, 0, x, y, w, h, on, X(st), Y(st));
}

static void panel_open(game_state *st, uint32_t sound, int32_t group)
{
    /* @bin 0x00500A54 the building's sound on the first draw (0x0047D350(wav, 1, 0): no-op), the
     * window panel and the centred title (group, 0) at y + 10 */
    if (S32(st, INFO_SOUND) != 0) {
        GAME_CALL(st, 0x0047D350u, (int32_t) sound, 1, 0);
        S32(st, INFO_SOUND) = 0;
    }
    GAME_CALL(st, 0x00536820u, X(st), Y(st), W(st), H(st));
    text_centred(st, group, 0, X(st), Y(st) + 10, W(st) << 4, F(st, INFO_FONT_LARGE));
}

static int32_t cart_text(int32_t action)
{
    /* @bin 0x00501862 the cart pusher's line by its action (index bytes 0x00501A14, targets
     * 0x00501A08; the same tables at 0x00501A2C, 0x005026C0, 0x005026D8): 0xB, 0xE, 0x11 -> 0x11;
     * 0xF, 0x10, 0x12..0x15 -> 0x21; the others -> 0x10 */
    switch (action) {
    case 0x0B: case 0x0E: case 0x11:
        return 0x11;
    case 0x0F: case 0x10: case 0x12: case 0x13: case 0x14: case 0x15:
        return 0x21;
    default:
        return 0x10;
    }
}

static int cart_pusher(game_state *st, int32_t f, int32_t image_y, int32_t text_y)
{
    /* @bin 0x005017FF / 0x0050240C (granary, storage yard) a cart pusher alive (+0x0E == 1): the icon
     * of its resource at (x + 0x20, image_y) and the text 0x63 chosen by its action (+0x38, or +0x10
     * for actions 6 and 7) at (x + 0x40, text_y). 0: the figure is none or not alive */
    if (f == 0) {
        return 0;
    }
    uint32_t a = figure_addr(f);
    if (U8(st, a + F_STATE) != 1) {
        return 0;
    }
    int32_t action = U8(st, a + F_ACTION);
    if (action == 6 || action == 7) {
        action = U8(st, a + F_ACTION2);
    }
    int32_t y = Y(st) + image_y;
    int32_t x = X(st) + 0x20;
    image(st, U8(st, a + F_RESOURCE_IMAGE) + S32(st, PANEL_ICONS), x, y);
    text_box(st, 0x63, cart_text(action), X(st) + 0x40, Y(st) + text_y, (W(st) - 6) * 0x10, F(st, FONT_SMALL));
    return 1;
}

/* ---- bazaar (types 0x46 / 0xBE) ---- */

static int bazaar_good(int32_t res)
{
    /* @bin 0x005013A0 the goods a bazaar sells (index bytes 0x005013CC on res - 1): 1..8, 0xD, 0xF,
     * 0x11, 0x13. Registered without a wrapper (ported inline by its callers, core/state/ported.c),
     * so it is inline here too */
    return (res >= 1 && res <= 8) || res == 0x0D || res == 0x0F || res == 0x11 || res == 0x13;
}

static int32_t bazaar_index(int32_t res)
{
    /* @bin 0x00500B7F the bazaar's stock index of a resource (jump table 0x0050109C; the same at
     * 0x005010E8): 1..8 -> 0..7, 0xF -> 8, 0xD -> 9, 0x11 -> 10, 0x13 -> 11, others -1 */
    switch (res) {
    case 1: return 0;
    case 2: return 1;
    case 3: return 2;
    case 4: return 3;
    case 5: return 4;
    case 6: return 5;
    case 7: return 6;
    case 8: return 7;
    case 0x0D: return 9;
    case 0x0F: return 8;
    case 0x11: return 10;
    case 0x13: return 11;
    default: return -1;
    }
}

static int16_t bazaar_stock(game_state *st, int32_t res)
{
    /* @bin 0x00500BE4 i16 at building +0x56 + 2 index (index -1 reads +0x54) */
    return S16(st, building(st) + B_STOCK + 2u * (uint32_t) bazaar_index(res));
}

static void info_bazaar(game_state *st)
{
    /* @bin 0x00500A40 the bazaar's panel: stock of the listed goods (0x00F53DD8), the four fixed
     * goods, the workers, the buyer */
    S32(st, INFO_HELP) = 2;
    panel_open(st, 0x005F7B24u, 0x61); /* "wavs\market.wav" */
    if (S32(st, INFO_ROAD) <= 0) {
        text_box(st, 0x45, 0x19, X(st) + 0x20, Y(st) + 0x38, (W(st) - 4) * 0x10, F(st, INFO_FONT_TEXT));
    } else if (S16(st, building(st) + B_WORKERS) <= 0) {
        text_box(st, 0x61, 2, X(st) + 0x20, Y(st) + 0x38, (W(st) - 4) * 0x10, F(st, INFO_FONT_TEXT));
    } else {
        int32_t count = S32(st, STORAGE_ROWS);
        int32_t total = 0;
        for (int32_t i = 0; i < count; i++) {
            int32_t res = S32(st, STORAGE_ROWS + 4 + 4u * (uint32_t) i);
            if (res != 0) {
                total += bazaar_stock(st, res);
            }
        }
        if (count <= 0 || total == 0) {
            /* @bin 0x00500BFC nothing in stock */
            text_box(st, 0x61, 4, X(st) + 0x20, Y(st) + 0x2D, (W(st) - 4) * 0x10, F(st, INFO_FONT_TEXT));
        }
        int32_t n = S32(st, STORAGE_ROWS);
        for (int32_t i = 0, dx = 0; i < n; i++, dx += 0x6E) {
            int32_t res = S32(st, STORAGE_ROWS + 4 + 4u * (uint32_t) i);
            if (res == 0) {
                continue;
            }
            /* @bin 0x00500C5B the icon at (x + 0x20 + 0x6E i, y + 0x54), the stock at x + 0x40 */
            int32_t y = Y(st) + 0x54;
            image(st, S32(st, PANEL_ICONS) + res, dx + X(st) + 0x20, y);
            int32_t font = F(st, INFO_FONT_TEXT);
            number(st, bazaar_stock(st, res), dx + X(st) + 0x40, Y(st) + 0x54, font);
            n = S32(st, STORAGE_ROWS);
        }
        /* @bin 0x00500D4B the four fixed goods: icons 0xD, 0x13, 0x11, 0xF and building +0x68,
         * +0x6C, +0x6A, +0x66 */
        static const struct { int32_t icon, dx; uint32_t field; } fixed[4] = {
            {0x0D, 0x20, 0x68}, {0x13, 0x8E, 0x6C}, {0x11, 0xFC, 0x6A}, {0x0F, 0x16A, 0x66}};
        for (int k = 0; k < 4; k++) {
            image(st, S32(st, PANEL_ICONS) + fixed[k].icon, X(st) + fixed[k].dx, Y(st) + 0x68);
            number(st, S16(st, building(st) + fixed[k].field), X(st) + fixed[k].dx + 0x20, Y(st) + 0x6E,
                   F(st, INFO_FONT_TEXT));
        }
    }
    inner_panel(st, X(st) + 0x10, Y(st) + 0x80, W(st) - 2, 5);
    GAME_CALL(st, 0x004FC5C0u, Y(st) + 0x8E);
    int32_t f = S16(st, building(st) + B_FIGURE);
    if (f != 0 && U8(st, figure_addr(f) + F_STATE) == 1) {
        uint32_t a = figure_addr(f);
        /* @bin 0x00500F77 the buyer: action 2 -> icon at y + 0xBE and text 0x61/0xC; else y + 0xB8, 0x61/0xB */
        int returning = U8(st, a + F_ACTION) == 2;
        image(st, U8(st, a + F_RESOURCE_IMAGE) + S32(st, PANEL_ICONS), X(st) + 0x20,
              Y(st) + (returning ? 0xBE : 0xB8));
        text_box(st, 0x61, returning ? 0x0C : 0x0B, X(st) + 0x40, Y(st) + 0xB8, (W(st) - 6) * 0x10, F(st, FONT_SMALL));
        return;
    }
    if (S16(st, building(st) + B_WORKERS) > 0) {
        text_box(st, 0x61, 0x0A, X(st) + 0x20, Y(st) + 0xB8, (W(st) - 4) * 0x10, F(st, FONT_SMALL));
    }
}

static void info_bazaar_orders(game_state *st)
{
    /* @bin 0x005013E0 the bazaar's orders sub-panel: one row of 4 lines per listed good that
     * 0x005013A0 accepts */
    int32_t shown = 0;
    for (int32_t i = 0; i < S32(st, STORAGE_LIST); i++) {
        if (bazaar_good(S32(st, STORAGE_LIST + 4 + 4u * (uint32_t) i))) {
            shown++;
        }
    }
    int32_t rows = GAME_CALL(st, 0x004CDE80u, shown * 4, 3);
    S32(st, INFO_HELP) = 2;
    GAME_CALL(st, 0x00536820u, X(st), 0x20, 0x1D, rows + 7);
    text_centred(st, 0x61, 7, X(st), 0x2A, W(st) << 4, F(st, INFO_FONT_LARGE));
    inner_panel(st, X(st) + 0x10, 0x4A, W(st) - 2, rows);
}

static void info_bazaar_frame(game_state *st)
{
    /* @bin 0x00501140 the bazaar panel's orders button (hover: 0x007056C8 == 1) and its text 0x61/6 */
    frame_box(st, X(st) + 100, H(st) * 0x10 - 0x22 + Y(st), (W(st) - 0x0C) * 0x10, 0x14, S32(st, INFO_BUTTON_HOVER) == 1);
    text_centred(st, 0x61, 6, X(st) + 0x50, H(st) * 0x10 - 0x1E + Y(st), (W(st) - 0x0A) * 0x10, F(st, INFO_FONT_TEXT));
}

static void info_bazaar_orders_frame(game_state *st)
{
    /* @bin 0x005011D0 the bazaar's order rows: per accepted good its icon on both sides (image
     * + 0x0047D350(res, 3), a no-op), its name, the hover frame, and "buy" (0x61/8) or "don't buy"
     * (0x61/9) by its order byte */
    uint32_t orders = STORAGE_ORDERS + STORAGE_SIZE * U8(st, building(st) + B_STORAGE);
    if (S32(st, STORAGE_LIST) < 1) {
        U8(st, REDRAW_ALL) = 1;
        return;
    }
    int32_t row = 1;
    int32_t y = 0x4E;
    for (int32_t i = 0;;) {
        int32_t res = S32(st, STORAGE_LIST + 4 + 4u * (uint32_t) i);
        if (res != 0 && bazaar_good(res)) {
            int32_t icons = S32(st, PANEL_ICONS);
            int32_t x = X(st) + 0x20;
            int32_t extra = GAME_CALL(st, 0x0047D350u, res, 3);
            image(st, extra + icons + res, x, y);
            icons = S32(st, PANEL_ICONS);
            x = X(st) + 0x198;
            extra = GAME_CALL(st, 0x0047D350u, res, 3);
            image(st, extra + icons + res, x, y);
            text(st, 0x17, res, X(st) + 0x48, y + 4, F(st, FONT_ROW));
            if (S32(st, ROW_HOVER) == row) {
                frame_box(st, X(st) + 0xB4, y, 0xD2, 0x14, 1);
            }
            uint8_t order = U8(st, orders + (uint32_t) res);
            if (order == 0) {
                text(st, 0x61, 8, X(st) + 0x104, y + 5, F(st, FONT_ROW));
            } else if (order == 1) {
                text(st, 0x61, 9, X(st) + 0xF0, y + 5, F(st, FONT_RED));
            }
            row++;
            y += 0x14;
        }
        i++;
        if (i >= S32(st, STORAGE_LIST)) {
            break;
        }
    }
    U8(st, REDRAW_ALL) = 1;
}

/* ---- granary (types 0x47 / 0xBF) ---- */

static void info_granary(game_state *st)
{
    /* @bin 0x005014A0 the granary's panel: total food and free space, the listed foods
     * (0x00F53DD8) in two columns, the workers, the cart pushers */
    S32(st, INFO_HELP) = 3;
    panel_open(st, 0x005F7B34u, 0x62); /* "wavs\granary.wav" */
    if (S32(st, INFO_ROAD) <= 0) {
        text_box(st, 0x45, 0x19, X(st) + 0x20, Y(st) + 0x28, (W(st) - 4) * 0x10, F(st, INFO_FONT_TEXT));
    } else if (S32(st, GRANARY_SPECIAL) != 0) {
        text_box(st, 0x62, 4, X(st) + 0x20, Y(st) + 0x28, (W(st) - 4) * 0x10, F(st, INFO_FONT_TEXT));
    } else {
        /* @bin 0x005015A4 the total of the eight foods (i16 at +0x58..+0x66) and the free space (+0x56) */
        int32_t total = 0;
        for (uint32_t k = 0; k < 8; k++) {
            total += S16(st, building(st) + 0x58u + 2 * k);
        }
        S32(st, TEXT_X) = 0;
        text(st, 0x62, 2, X(st) + 0x22, Y(st) + 0x28, F(st, INFO_FONT_TEXT));
        GAME_CALL(st, 0x004CCC30u, 8, 0x10, total, S32(st, TEXT_X) + X(st) + 0x22, Y(st) + 0x28, F(st, INFO_FONT_TEXT), 0);
        S32(st, TEXT_X) = 0;
        text(st, 0x62, 3, X(st) + 0xDC, Y(st) + 0x28, F(st, INFO_FONT_TEXT));
        GAME_CALL(st, 0x004CCC30u, 8, 0x10, S16(st, building(st) + B_STOCK), S32(st, TEXT_X) + X(st) + 0xDC,
                  Y(st) + 0x28, F(st, INFO_FONT_TEXT), 0);
        for (int32_t i = 0; i < S32(st, STORAGE_ROWS); i++) {
            int32_t res = S32(st, STORAGE_ROWS + 4 + 4u * (uint32_t) i);
            if (res == 0) {
                continue;
            }
            /* @bin 0x005016B8 entries 0, 1 in the left column, the others at x + 0xF0; even rows at
             * y + 0x44, odd at y + 0x5C */
            int32_t x = i < 2 ? X(st) + 0x22 : X(st) + 0xF0;
            int32_t y = i % 2 == 0 ? Y(st) + 0x44 : Y(st) + 0x5C;
            image(st, S32(st, PANEL_ICONS) + res, x, y);
            S32(st, TEXT_X) = 0;
            number(st, S16(st, building(st) + B_STOCK + 2u * (uint32_t) res), x + 0x22, y + 7, F(st, INFO_FONT_TEXT));
            text(st, 0x17, res, S32(st, TEXT_X) + x + 0x22, y + 7, F(st, INFO_FONT_TEXT));
        }
    }
    inner_panel(st, X(st) + 0x10, Y(st) + 0xA3, W(st) - 2, 5);
    GAME_CALL(st, 0x004FC5C0u, Y(st) + 0xA3);
    if (cart_pusher(st, S16(st, building(st) + B_FIGURE), 0xC8, 0xC8)) {
        return;
    }
    if (cart_pusher(st, S16(st, building(st) + B_FIGURE2), 0xC8, 0xC8)) {
        return;
    }
    if (S16(st, building(st) + B_WORKERS) > 0) {
        text_box(st, 0x63, 0x0F, X(st) + 0x20, Y(st) + 0xC8, (W(st) - 4) * 0x10, F(st, FONT_SMALL));
    }
}

static void info_granary_orders(game_state *st)
{
    /* @bin 0x00501AD0 the granary's orders sub-panel: 4 lines per listed food */
    int32_t rows = GAME_CALL(st, 0x004CDE80u, S32(st, STORAGE_ROWS) * 4, 3);
    S32(st, INFO_HELP) = 3;
    GAME_CALL(st, 0x00536820u, X(st), 0x20, 0x1D, rows + 7);
    text_centred(st, 0x62, 6, X(st), 0x2A, W(st) << 4, F(st, INFO_FONT_LARGE));
    inner_panel(st, X(st) + 0x10, 0x4A, W(st) - 2, rows);
}

static void info_granary_frame(game_state *st)
{
    /* @bin 0x00501A40 the granary panel's orders button and its text 0x62/5 */
    frame_box(st, X(st) + 100, H(st) * 0x10 - 0x22 + Y(st), (W(st) - 0x0C) * 0x10, 0x14, S32(st, INFO_BUTTON_HOVER) == 1);
    text_centred(st, 0x62, 5, X(st) + 0x50, H(st) * 0x10 - 0x1E + Y(st), (W(st) - 0x0A) * 0x10, F(st, INFO_FONT_TEXT));
}

static int32_t fill_text(int16_t fill)
{
    /* @bin 0x00501D5A the fill limit's text: up to 800 -> 0x19, up to 1600 -> 0x1A, up to 2400 ->
     * 0x1B, above -> 0x1C */
    if (fill <= 0x320) {
        return 0x19;
    }
    if (fill <= 0x640) {
        return 0x1A;
    }
    return 0x1B + (fill > 0x960);
}

static int32_t get_text(int16_t fill)
{
    /* @bin 0x00501E56 the "get" limit's text: 0x19, 0x1A, 0x1B, above 2400 -> 0x1F */
    if (fill <= 0x320) {
        return 0x19;
    }
    if (fill <= 0x640) {
        return 0x1A;
    }
    return fill > 0x960 ? 0x1F : 0x1B;
}

static void order_row(game_state *st, uint32_t store, int32_t res, int32_t dx, int32_t y, int32_t accept_tail,
                      int32_t refuse_text, int32_t table)
{
    /* @bin 0x00501CDA / 0x00502C7B the order's texts (group 0x63) at x + dx by the order byte:
     * 0 accept (0x20 or 0x12 above 2400, the limit, then accept_tail up to 2400), 1 refuse (8, red),
     * 2 get (0x13, the limit, then accept_tail up to 2400), 3 refuse_text (red); then for orders
     * 0 and 2 the limit button (table, 2 states) at (x + 0xC8, y - 3) */
    uint32_t order = STORAGE_ORDERS + STORAGE_SIZE * store + (uint32_t) res;
    uint32_t fill = STORAGE_FILL + STORAGE_SIZE * store + 2u * (uint32_t) res;
    uint32_t get = STORAGE_GET + STORAGE_SIZE * store + 2u * (uint32_t) res;
    switch (U8(st, order)) {
    case 0:
        S32(st, TEXT_X) = 0;
        text(st, 0x63, S16(st, fill) <= 0x960 ? 0x20 : 0x12, X(st) + dx, y, F(st, FONT_ROW));
        text(st, 0x63, fill_text(S16(st, fill)), S32(st, TEXT_X) + X(st) + dx, y, F(st, FONT_ROW));
        if (S16(st, fill) <= 0x960) {
            text(st, 0x63, accept_tail, S32(st, TEXT_X) + X(st) + dx, y, F(st, FONT_ROW));
        }
        break;
    case 1:
        text(st, 0x63, 8, X(st) + dx, y, F(st, FONT_RED));
        break;
    case 2:
        S32(st, TEXT_X) = 0;
        text(st, 0x63, 0x13, X(st) + dx, y, F(st, FONT_ROW));
        text(st, 0x63, get_text(S16(st, get)), S32(st, TEXT_X) + X(st) + dx, y, F(st, FONT_ROW));
        if (S16(st, get) <= 0x960) {
            text(st, 0x63, accept_tail, S32(st, TEXT_X) + X(st) + dx, y, F(st, FONT_ROW));
        }
        break;
    case 3:
        text(st, 0x63, refuse_text, X(st) + dx, y, F(st, FONT_RED));
        break;
    default:
        break;
    }
    uint8_t o = U8(st, order);
    if (o == 2 || o == 0) {
        /* @bin 0x00501F21 the row's limit buttons: 0x00425BC0 (.., 2, 10), 0x00425990, 0x00425730 */
        int32_t by = y - 3;
        GAME_CALL(st, 0x00425BC0u, X(st) + 0xC8, by, table, 2, 10);
        GAME_CALL(st, 0x00425990u, X(st) + 0xC8, by, table, 2);
        GAME_CALL(st, 0x00425730u, X(st) + 0xC8, by, table, 2);
    }
}

static void info_granary_orders_frame(game_state *st)
{
    /* @bin 0x00501B60 the granary's order rows (every listed food: icon, name, hover frame, order
     * texts, limit buttons) under the "all" button */
    uint32_t store = U8(st, building(st) + B_STORAGE);
    int32_t hover = S32(st, INFO_BUTTON_HOVER) == 1;
    int32_t h = (GAME_CALL(st, 0x004CDE80u, S32(st, STORAGE_ROWS) * 4, 3) + 7) * 0x10;
    frame_box(st, X(st) + 0x50, h - 8, (W(st) - 0x0A) * 0x10, 0x14, hover);
    text_centred(st, 0x63, 7, X(st) + 0x50, h - 4, (W(st) - 0x0A) * 0x10, F(st, INFO_FONT_TEXT));
    if (S32(st, STORAGE_ROWS) < 1) {
        U8(st, REDRAW_ALL) = 1;
        return;
    }
    int32_t table = (int32_t) ORDER_BUTTONS;
    int32_t y = 0x53;
    for (int32_t row = 0; row < S32(st, STORAGE_ROWS); table += 0x48, y += 0x14) {
        int32_t res = S32(st, STORAGE_ROWS + 4 + 4u * (uint32_t) row);
        image(st, S32(st, PANEL_ICONS) + res, X(st) + 0x20, y - 5);
        text(st, 0x17, res, X(st) + 0x48, y - 1, F(st, FONT_ROW));
        row++;
        if (S32(st, ROW_HOVER) == row) {
            frame_box(st, X(st) + 0xB4, y - 5, 0xFA, 0x14, 1);
        }
        order_row(st, store, res, 0xFA, y, 0x1E, 0x14, table);
    }
    U8(st, REDRAW_ALL) = 1;
}

/* ---- storage yard (types 0x48 / 0xC1) ---- */

static int is_hundreds(int32_t res)
{
    /* @bin 0x00502229 goods counted in hundreds (jump table 0x00502694 on res - 0x18, then 0x1C, 0xA,
     * 0x24): 0x18, 0x19, 0x1A, 0x1E, 0x23, 0x1C, 0x0A, 0x24 */
    return res == 0x18 || res == 0x19 || res == 0x1A || res == 0x1E || res == 0x23 || res == 0x1C || res == 0x0A ||
           res == 0x24;
}

static int is_unit_text(int32_t res)
{
    /* @bin 0x00502293 the goods with a unit word (jump table 0x005026A8): 0x18, 0x19, 0x1A, 0x1E, 0x23 */
    return res == 0x18 || res == 0x19 || res == 0x1A || res == 0x1E || res == 0x23;
}

static void info_storage_yard(game_state *st)
{
    /* @bin 0x00501FD0 the storage yard's panel: the amounts stored by good (from its eight spaces),
     * the workers, the cart pushers, the yard's notice */
    int32_t amounts[36] = {0};
    int32_t total = 0;
    int32_t spaces = 0;
    S32(st, INFO_HELP) = 4;
    panel_open(st, 0x005F7B48u, 0x63); /* "wavs\warehouse.wav" */
    int32_t b = S32(st, INFO_BUILDING);
    /* @bin 0x0050206B the scroll row (building +0xDC): 0 -> 1; past the list -> count - 15 (1 if
     * the list has at most 15 entries) */
    uint32_t scroll = BUILDING_ADDR(b) + B_YARD_SCROLL;
    if (S32(st, scroll) == 0) {
        S32(st, scroll) = 1;
    }
    int32_t count = S32(st, STORAGE_LIST);
    if (S32(st, scroll) >= count) {
        S32(st, scroll) = count > 0x0F ? count - 0x0F : 1;
    }
    /* @bin 0x005020B0 the eight spaces: the chain of building +0x3A; a space with an amount (+0x3C,
     * i16 > 0) adds it to its resource (+0x12). The chain continues from the last id even when it
     * is not positive, as the original does */
    int32_t id = b;
    for (int k = 0; k < 8; k++) {
        id = S16(st, BUILDING_ADDR(id) + B_NEXT_SPACE);
        if (id > 0) {
            int16_t amount = S16(st, BUILDING_ADDR(id) + B_SPACE_AMOUNT);
            if (amount > 0) {
                int32_t res = S16(st, BUILDING_ADDR(id) + B_SPACE_RESOURCE);
                if (res >= 0 && res < 36) {
                    amounts[res] += amount; /* (the original's stack array; resources are 0..35) */
                }
                total += amount;
                spaces++;
            }
        }
    }
    if (S32(st, INFO_ROAD) <= 0) {
        text_box(st, 0x45, 0x19, X(st) + 0x20, Y(st) + 0x38, (W(st) - 4) * 0x10, F(st, INFO_FONT_TEXT));
    } else if (total == 0) {
        text_box(st, 0x63, 0x16, X(st) + 0x20, Y(st) + 0x38, (W(st) - 4) * 0x10, F(st, INFO_FONT_TEXT));
    } else {
        int32_t offset = 0;
        for (int32_t res = 1; res < 0x24; res++) {
            if (amounts[res] == 0) {
                continue;
            }
            int32_t amount = GAME_CALL(st, 0x0045D510u, S32(st, INFO_BUILDING), res);
            /* @bin 0x005021A3 four rows of 0x1E in the left column, then the right one at x + 0xDC */
            int32_t x, y;
            if (offset <= 0x5A) {
                y = Y(st) + offset + 0x2D;
                x = X(st) + 0x0A;
            } else {
                y = Y(st) + offset - 0x4B;
                x = X(st) + 0xDC;
            }
            image(st, S32(st, PANEL_ICONS) + res, x, y);
            /* @bin 0x005021F5 full (a multiple of 400 with the eight spaces used): the red font */
            int32_t font = (amount % 400 == 0 && spaces == 8) ? F(st, FONT_RED) : F(st, INFO_FONT_TEXT);
            S32(st, TEXT_X) = 0;
            if (is_hundreds(res)) {
                amount /= 100;
            }
            number(st, amount, x + 0x18, y + 7, font);
            if (is_unit_text(res)) {
                int32_t tx = S32(st, TEXT_X) + x + 0x18;
                text(st, 0x63, amount == 1 ? 0x17 : 0x18, tx, y + 7, font);
                text(st, 0x17, res, tx, y + 0x16, font);
            } else if (res == 0x1C || res == 0x0A) {
                /* @bin 0x00502336 the plural name (res + 0x36) unless one */
                text(st, 0x17, amount == 1 ? res : res + 0x36, S32(st, TEXT_X) + x + 0x18, y + 7, font);
            } else {
                text(st, 0x17, res, S32(st, TEXT_X) + x + 0x18, y + 7, font);
            }
            offset += 0x1E;
        }
    }
    inner_panel(st, X(st) + 0x10, Y(st) + 0xB7, W(st) - 2, 5);
    GAME_CALL(st, 0x004FC5C0u, Y(st) + 0xBC);
    if (!cart_pusher(st, S16(st, building(st) + B_FIGURE), 0xE4, 0xE7) &&
        !cart_pusher(st, S16(st, building(st) + B_FIGURE2), 0xE4, 0xE7) &&
        S16(st, building(st) + B_WORKERS) > 0) {
        text_box(st, 0x63, 0x0F, X(st) + 0x20, Y(st) + 0xE7, (W(st) - 4) * 0x10, F(st, FONT_SMALL));
    }
    /* @bin 0x005025E8 the yard's notice 0x00EADCB8: 1 -> 0x63/0xD, 2 -> 0x63/0xE */
    int32_t notice = S32(st, YARD_NOTICE);
    if (notice == 1 || notice == 2) {
        text_box(st, 0x63, notice == 1 ? 0x0D : 0x0E, X(st) + 0x20, H(st) * 0x10 - 0x55 + Y(st), (W(st) - 4) * 0x10,
                 F(st, INFO_FONT_TEXT));
    }
}

static int32_t yard_rows(game_state *st)
{
    /* @bin 0x005028F0 the rows shown: 0x00502780 when below the list's count, else the count */
    int32_t a = GAME_CALL0(st, 0x00502780u);
    int32_t count = S32(st, STORAGE_LIST);
    return a < count ? GAME_CALL0(st, 0x00502780u) : count;
}

static void info_storage_yard_orders(game_state *st)
{
    /* @bin 0x005028F0 the storage yard's orders sub-panel */
    int32_t rows = GAME_CALL(st, 0x004CDE80u, yard_rows(st) << 2, 3);
    S32(st, INFO_HELP) = 4;
    GAME_CALL(st, 0x00536820u, X(st), 0x20, 0x1D, rows + 7);
    text_centred(st, 0x63, 3, X(st), 0x2A, W(st) << 4, F(st, INFO_FONT_LARGE));
    inner_panel(st, X(st) + 0x10, 0x4A, W(st) - 2, rows);
}

static void info_storage_yard_frame(game_state *st)
{
    /* @bin 0x005026F0 the storage yard panel's orders button and its text 0x63/2 */
    frame_box(st, X(st) + 100, H(st) * 0x10 - 0x22 + Y(st), (W(st) - 0x0C) * 0x10, 0x14, S32(st, INFO_BUTTON_HOVER) == 1);
    text_centred(st, 0x63, 2, X(st) + 0x50, H(st) * 0x10 - 0x1E + Y(st), (W(st) - 0x0A) * 0x10, F(st, INFO_FONT_TEXT));
}

static void info_storage_yard_thumb(game_state *st)
{
    /* @bin 0x00502840 the scroll bar's thumb of the yard's order list (when the list has more
     * entries than rows): image 0x00F56494 + 0x27 at (x + 0x19E, 0x6A + 0x004CDE40(track, pos)) */
    int32_t hidden = S32(st, STORAGE_LIST) - GAME_CALL0(st, 0x00502780u);
    if (hidden <= 0) {
        return;
    }
    /* (0x00502780 is called with the 3 pushed for 0x004CDE80 on the stack) */
    int32_t rows = GAME_CALL(st, 0x00502780u, 3);
    int32_t track = (GAME_CALL(st, 0x004CDE80u, rows << 2, 3) - 6) << 4;
    int32_t pos;
    int32_t scroll = S32(st, building(st) + B_ORDERS_SCROLL);
    if (scroll <= 1) {
        pos = 0;
    } else {
        int32_t shown = GAME_CALL0(st, 0x00502780u);
        if (shown + scroll > S32(st, STORAGE_LIST)) {
            pos = 100;
        } else {
            pos = GAME_CALL(st, 0x004CDE60u, scroll - 1, hidden);
        }
    }
    int32_t y = GAME_CALL(st, 0x004CDE40u, track, pos);
    image(st, S32(st, SCROLL_IMAGES) + 0x27, X(st) + 0x19E, y + 0x6A);
}

static void info_storage_yard_orders_frame(game_state *st)
{
    /* @bin 0x00502990 the storage yard's order rows from its scroll row (building +0xE0): the
     * "all" button, the scroll buttons and bar, per row the good's icon, name, hover frame, order
     * texts and limit buttons, then the thumb 0x00502840 */
    uint32_t store = U8(st, building(st) + B_STORAGE);
    int32_t lines = GAME_CALL(st, 0x004CDE80u, yard_rows(st) * 4, 3) + 7;
    int32_t hover = S32(st, INFO_BUTTON_HOVER) == 1;
    frame_box(st, X(st) + 0x50, lines * 0x10, (W(st) - 0x0A) * 0x10, 0x14, hover);
    text_centred(st, 0x63, 7, X(st) + 0x50, lines * 0x10 + 4, (W(st) - 0x0A) * 0x10, F(st, INFO_FONT_TEXT));
    /* @bin 0x00502A4C the first entry shown: building +0xE0 (0 -> 1); at or past the count:
     * count - rows - 1 when rows + 1 < count, else 1 (a local: the building is not written) */
    int32_t scroll = S32(st, building(st) + B_ORDERS_SCROLL);
    if (scroll == 0) {
        scroll = 1;
    }
    if (scroll >= S32(st, STORAGE_LIST)) {
        int32_t t = GAME_CALL0(st, 0x00502780u);
        if (S32(st, STORAGE_LIST) > t + 1) {
            int32_t t2 = GAME_CALL0(st, 0x00502780u);
            scroll = S32(st, STORAGE_LIST) - t2 - 1;
        } else {
            scroll = 1;
        }
    }
    if (S32(st, STORAGE_LIST) > GAME_CALL0(st, 0x00502780u)) {
        /* @bin 0x00502AB2 the scroll buttons (tables 0x005C5B48, 0x005C5B60) and the bar */
        int32_t by = lines * 0x10 - 0x4E;
        GAME_CALL(st, 0x00425E80u, X(st) + 0x19E, by, (int32_t) SCROLL_UP, 1, 10);
        GAME_CALL(st, 0x00425C40u, X(st) + 0x19E, by, (int32_t) SCROLL_UP, 1);
        GAME_CALL(st, 0x004256A0u, X(st) + 0x19E, lines * 0x10 - 0x48, (int32_t) SCROLL_UP, 1);
        GAME_CALL(st, 0x00425E80u, X(st) + 0x186, 0x52, (int32_t) SCROLL_DOWN, 1, 10);
        GAME_CALL(st, 0x00425C40u, X(st) + 0x186, 0x52, (int32_t) SCROLL_DOWN, 1);
        GAME_CALL(st, 0x004256A0u, X(st) + 0x186, 0x52, (int32_t) SCROLL_DOWN, 1);
        inner_panel(st, X(st) + 0x19B, 0x6A, 2, lines == 0x1B ? 0x10 : 0x18);
    }
    int32_t row = 0;
    if (GAME_CALL0(st, 0x00502780u) > 0) {
        int32_t table = (int32_t) ORDER_BUTTONS;
        int32_t y = 0x53;
        uint32_t p = STORAGE_LIST + 4u * (uint32_t) scroll;
        for (;;) {
            int32_t res = S32(st, p);
            if (res != 0) {
                image(st, S32(st, PANEL_ICONS) + res, X(st) + 0x19, y - 5);
                text(st, 0x17, res, X(st) + 0x3C, y - 1, F(st, FONT_ROW));
                if (S32(st, ROW_HOVER) == row + 1) {
                    frame_box(st, X(st) + 0xB4, y - 5, 0xE6, 0x14, 1);
                }
                order_row(st, store, res, 0xF0, y, 0x1D, 0x15, table);
                /* @bin 0x00502F04 the next entry; past the list's count: done. An empty entry
                 * keeps the same entry for the next row */
                scroll++;
                p += 4;
                if (scroll > S32(st, STORAGE_LIST)) {
                    break;
                }
            }
            row++;
            table += 0x48;
            y += 0x14;
            if (row >= GAME_CALL0(st, 0x00502780u)) {
                break;
            }
        }
    }
    U8(st, REDRAW_ALL) = 1;
    info_storage_yard_thumb(st);
}

#define RUN(fn) \
    static int32_t run_##fn(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
RUN(info_bazaar)
RUN(info_bazaar_orders)
RUN(info_bazaar_frame)
RUN(info_bazaar_orders_frame)
RUN(info_granary)
RUN(info_granary_orders)
RUN(info_granary_frame)
RUN(info_granary_orders_frame)
RUN(info_storage_yard)
RUN(info_storage_yard_orders)
RUN(info_storage_yard_frame)
RUN(info_storage_yard_orders_frame)
RUN(info_storage_yard_thumb)

const ported_function info_storage_ports[] = {
    {0x00500A40u, "info_bazaar", 0, 0, 0, run_info_bazaar, 0},
    {0x005013E0u, "info_bazaar_orders", 0, 0, 0, run_info_bazaar_orders, 0},
    {0x00501140u, "info_bazaar_frame", 0, 0, 0, run_info_bazaar_frame, 0},
    {0x005011D0u, "info_bazaar_orders_frame", 0, 0, 0, run_info_bazaar_orders_frame, 0},
    {0x005014A0u, "info_granary", 0, 0, 0, run_info_granary, 0},
    {0x00501AD0u, "info_granary_orders", 0, 0, 0, run_info_granary_orders, 0},
    {0x00501A40u, "info_granary_frame", 0, 0, 0, run_info_granary_frame, 0},
    {0x00501B60u, "info_granary_orders_frame", 0, 0, 0, run_info_granary_orders_frame, 0},
    {0x00501FD0u, "info_storage_yard", 0, 0, 0, run_info_storage_yard, 0},
    {0x005028F0u, "info_storage_yard_orders", 0, 0, 0, run_info_storage_yard_orders, 0},
    {0x005026F0u, "info_storage_yard_frame", 0, 0, 0, run_info_storage_yard_frame, 0},
    {0x00502990u, "info_storage_yard_orders_frame", 0, 0, 0, run_info_storage_yard_orders_frame, 0},
    {0x00502840u, "info_storage_yard_thumb", 0, 0, 0, run_info_storage_yard_thumb, 0},
};
const int info_storage_ports_count = sizeof(info_storage_ports) / sizeof(info_storage_ports[0]);
