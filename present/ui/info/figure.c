/* Imhotep - window 9: info panels (figure) (package "info").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The panels of the info window's kinds 0 and 1 (no building: terrain, or the figures on the
 * clicked tile), the figure panel they share with the statue panels (types 0x29-0x2B, 0x38): the
 * figure thumbnails (the city view drawn at each figure into the spare screen, then copied into the
 * list), the selected figure's portrait, name, kind and what it carries or trades. Everything is
 * drawn through the primitives of present/draw/ (GAME_CALL), so the pixels are theirs; what is
 * ported is the layout and the window globals written on the way (the help id 0x00EADD54, the
 * sound flag 0x00EADD94, the figure view flag 0x00EADD48, the text cursor 0x00E20778). */
#include "info.h"

#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"


static int32_t px(game_state *st)
{
    return S32(st, G_INFO_X);
}

static int32_t py(game_state *st)
{
    return S32(st, G_INFO_Y);
}

static void draw_image(game_state *st, int32_t id, int32_t x, int32_t y)
{
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, id, x, y);
}

static void window_panel(game_state *st)
{
    /* @bin 0x00536820 the panel at (x, y), w x h blocks */
    GAME_CALL(st, FN_PANEL_DRAW, px(st), py(st), S32(st, G_INFO_W), S32(st, G_INFO_H));
}

static void text_title(game_state *st, int32_t group, int32_t id, int32_t y, int32_t font)
{
    /* @bin 0x004D6250 a text centred over the panel's width */
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, group, id, px(st), y, S32(st, G_INFO_W) << 4, font, 0, px(st), py(st));
}

static void text_block(game_state *st, int32_t group, int32_t id, int32_t x, int32_t y, int32_t width,
                       int32_t font)
{
    /* @bin 0x004D6160 a text wrapped in a width */
    GAME_CALL(st, FN_LAYOUT_GROUP_LINES, group, id, x, y, width, font, 0, px(st), py(st));
}

static void text(game_state *st, int32_t group, int32_t id, int32_t x, int32_t y, int32_t font)
{
    /* @bin 0x004D62F0 a text at (x, y); advances 0x00E20778 */
    GAME_CALL(st, FN_LAYOUT_GROUP, group, id, x, y, font, 0, px(st), py(st));
}

static void number(game_state *st, int32_t value, int32_t x, int32_t y, int32_t font)
{
    /* @bin 0x004D6860 a number with an empty suffix (prefix character 0x40) */
    GAME_CALL(st, FN_LAYOUT_NUMBER_AT, value, 0x40, (int32_t) G_NUMBER_SUFFIX, x, y, font, 0, px(st), py(st));
}

static int32_t text_x(game_state *st)
{
    return S32(st, G_TEXT_X);
}

static void sound_once(game_state *st, uint32_t wav)
{
    /* the panel's sound on the first draw (0x0047D350: the sound request, file name, once) */
    if (S32(st, G_INFO_SOUND) != 0) {
        GAME_CALL(st, FN_RETURN_ZERO, (int32_t) wav, 1, 0);
        S32(st, G_INFO_SOUND) = 0;
    }
}

static int32_t resource_icon(uint32_t r)
{
    /* @bin 0x004FBF6E the resource's icon in the resource icons (jump table 0x004FC004; the same
     * table at 0x004FC118 and 0x004FBE80): 0..7 -> 1..8, 8 -> 0xF, 9 -> 0xD, 10 -> 0x11, 11 -> 0x13,
     * any other (unsigned compare) -> -1 */
    static const int32_t icons[12] = {1, 2, 3, 4, 5, 6, 7, 8, 0x0F, 0x0D, 0x11, 0x13};
    return r > 0x0Bu ? -1 : icons[r];
}

static int resource_in_hundreds(uint32_t r)
{
    /* @bin 0x004FB435 resources counted in units of 100 (jump table 0x004FBEB8 and 0x1C, 0x0A, 0x24) */
    return r == 0x18 || r == 0x19 || r == 0x1A || r == 0x1E || r == 0x23 || r == 0x1C || r == 0x0A ||
           r == 0x24;
}

/* ---------------------------------------------------------------------------------------------- */
/* figure helpers */

static int32_t figure_text_group(game_state *st, int32_t f)
{
    /* @bin 0x0053D280 the text group of a figure type's description (table 0x006012EC by type; -4:
     * by the second type +0x0B; -2: by the scenario's climate 0x005E4C20[0x00784CB0] 0x44 -> 0xD2,
     * 0x45 -> 0xD1, 0x46 -> 0xD0; -3: by the home building's 0x004C5120 0..4 -> 0xE6..0xEA; else -1) */
    uint32_t fo = FIGURE_ADDR(f);
    int32_t home = S16(st, fo + FIG_HOME);
    int32_t g = S32(st, G_FIGURE_TEXT_GROUPS + 4u * U8(st, fo + FIG_TYPE));
    if (g > 0) {
        return g;
    }
    if (g == -4) {
        g = S32(st, G_FIGURE_TEXT_GROUPS + 4u * U8(st, fo + FIG_RESOURCE));
        if ((uint32_t) (g + 4) > 3u) {
            return g;
        }
        if (g != -3) {
            return -1;
        }
    } else if (g == -2) {
        int32_t c = S32(st, G_HERD_RECORDS + 0x30u * (U32(st, G_U784CB0) & 0xFFu));
        if (c == 0x44) {
            return 0xD2;
        }
        if (c == 0x45) {
            return 0xD1;
        }
        if (c == 0x46) {
            return 0xD0;
        }
        return -1;
    } else if (g != -3) {
        return -1;
    }
    uint32_t k = (uint32_t) GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, home);
    if (k > 4u) {
        return -1;
    }
    return 0xE6 + (int32_t) k;
}

static int32_t by_home_kind(game_state *st, int32_t home, uint32_t fallback_byte)
{
    /* @bin 0x004FA8F3 the home building's 0x004C5120 1..4 -> images 0x00F55C8C + 3, 2, 4, 1 (tables
     * 0x004FAAFC, 0x004FAB0C); otherwise the type byte's own image */
    uint32_t k = (uint32_t) GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, home);
    static const int32_t add[4] = {3, 2, 4, 1};
    if (k - 1u <= 3u) {
        return S32(st, G_UF55C8C) + add[k - 1u];
    }
    return (int32_t) U8(st, fallback_byte) + S32(st, G_PORTRAIT_IMAGES);
}

static int32_t figure_portrait(game_state *st, int32_t f)
{
    /* @bin 0x004FA8C0 the figure's portrait image: 0x00F55C7C + type, with the exceptions of the
     * jump table 0x004FAA6C / 0x004FAAA4 */
    uint32_t fo = FIGURE_ADDR(f);
    uint32_t type = U8(st, fo + FIG_TYPE);
    int32_t extra = S32(st, G_UF571EC);
    switch (type) {
    case 0x18: return extra + 3;
    case 0x1B: return by_home_kind(st, S16(st, fo + FIG_HOME), fo + FIG_TYPE);
    case 0x1E: return S32(st, G_PORTRAIT_IMAGES) + 0x1D;
    case 0x49: {
        /* @bin 0x004FA94D by the scenario's climate */
        int32_t c = S32(st, G_HERD_RECORDS + 0x30u * (U32(st, G_U784CB0) & 0xFFu));
        if (c == 0x44) {
            return S32(st, G_UF55C8C);
        }
        if (c == 0x45) {
            return S32(st, G_UF55C84);
        }
        if (c == 0x46) {
            return S32(st, G_PORTRAIT_IMAGES) + (int32_t) type;
        }
        return S32(st, G_PORTRAIT_IMAGES);
    }
    case 0x5B:
        if (U8(st, fo + FIG_RESOURCE) != 0x1B) {
            return (int32_t) U8(st, fo + FIG_RESOURCE) + S32(st, G_PORTRAIT_IMAGES);
        }
        return by_home_kind(st, S16(st, fo + FIG_HOME), fo + FIG_RESOURCE);
    case 0x66: return extra + 4;
    case 0x67: return extra + 5;
    case 0x68: return extra + 2;
    case 0x69: return extra;
    case 0x6A: return extra + 6;
    case 0x6B: return extra + 7;
    case 0x6C: return extra + 1;
    case 0x6D: return extra + 8;
    default: return S32(st, G_PORTRAIT_IMAGES) + (int32_t) type;
    }
}

static void figure_draw_resource(game_state *st, int32_t r, int32_t x, int32_t y)
{
    /* @bin 0x004FBF00 a resource icon (the image index given), 0x00E20778 kept */
    int32_t saved = S32(st, G_TEXT_X);
    draw_image(st, S32(st, G_RESOURCE_ICONS) + r, x, y);
    S32(st, G_TEXT_X) = saved;
}

static void figure_draw_loads(game_state *st, int32_t f, int32_t x, int32_t y)
{
    /* @bin 0x004FBF30 the four resources of figure +0x9C (i32, -1: none), 32 pixels apart */
    int32_t saved = S32(st, G_TEXT_X);
    uint32_t p = FIGURE_ADDR(f) + FIG_LOADS;
    for (int k = 4; k != 0; k--, p += 4) {
        int32_t r = S32(st, p);
        if (r != -1) {
            draw_image(st, S32(st, G_RESOURCE_ICONS) + resource_icon((uint32_t) r), x, y);
            x += 0x20;
            S32(st, G_TEXT_X) = saved;
        }
    }
}

static void figure_draw_loads_held(game_state *st, int32_t f, int32_t x, int32_t y)
{
    /* @bin 0x004FC040 the resources of figure +0x9C whose amount +0xBC (i32) is positive */
    int32_t saved = S32(st, G_TEXT_X);
    uint32_t p = FIGURE_ADDR(f) + FIG_LOAD_AMOUNTS;
    for (int k = 4; k != 0; k--, p += 4) {
        int32_t r = S32(st, p - (FIG_LOAD_AMOUNTS - FIG_LOADS));
        if (r != -1) {
            int32_t icon = resource_icon((uint32_t) r);
            if (S32(st, p) > 0) {
                draw_image(st, S32(st, G_RESOURCE_ICONS) + icon, x, y);
                x += 0x20;
                S32(st, G_TEXT_X) = saved;
            }
        }
    }
}

/* ---------------------------------------------------------------------------------------------- */
/* the figure panel */

static void description(game_state *st, uint32_t fo, int32_t group, int32_t y, int32_t font)
{
    /* @bin 0x004FAE3C the type's description (group of 0x0053D280, entry +0x7D: i8) when both exist */
    if (group == -1) {
        return;
    }
    int8_t id = (int8_t) U8(st, fo + FIG_U7D);
    if (id < 0) {
        return;
    }
    text_block(st, group, id, px(st) + 0x5A, py(st) + y, (S32(st, G_INFO_W) - 9) * 16, font);
}

static void name(game_state *st, uint32_t fo, int32_t x, int check_sign)
{
    /* @bin 0x004FAD0C the figure's name: group +0x98 (i32, -1: none), entry +0x70 (i16) */
    int32_t g = S32(st, fo + FIG_NAME_GROUP);
    S32(st, G_TEXT_X) = 0;
    if (g != -1 && (!check_sign || g >= 0)) {
        text(st, g, S16(st, fo + FIG_NAME_INDEX), px(st) + x, py(st) + 0x6C, S32(st, G_FONT_LARGE2));
    }
}

static int32_t building_type(game_state *st, int32_t b)
{
    return S16(st, BUILDING_ADDR(b) + BLD_TYPE);
}

static uint32_t trade_panel(game_state *st, int32_t f, int32_t type, int32_t font)
{
    /* @bin 0x004FB8DE trade caravans and ships (types 0x13, 0x14, 0x15: a 0x15 follower shows its
     * leader +0x68, twice): the city +0x6E, what the route buys and sells or what was traded */
    uint32_t fo = FIGURE_ADDR(f);
    if (U8(st, fo + FIG_TYPE) == 0x15) {
        f = S16(st, fo + FIG_LEADER);
    }
    fo = FIGURE_ADDR(f);
    if (U8(st, fo + FIG_TYPE) == 0x15) {
        f = S16(st, fo + FIG_LEADER);
    }
    fo = FIGURE_ADDR(f);
    uint32_t city = U8(st, fo + FIG_U6E);
    S32(st, G_TEXT_X) = 0;
    text(st, 0x40, type, px(st) + 0x28, py(st) + 0x6E, font);
    uint32_t cr = 0x6Au * city; /* @bin 0x004FB962 the empire cities, 0x6A bytes, name +3 (u8) */
    if (U8(st, G_EMPIRE_LANGUAGE) != 0) {
        text(st, 0x15, U8(st, G_EMPIRE_CITIES + 3u + cr), text_x(st) + px(st) + 0x28, py(st) + 0x6E, font);
    } else {
        text(st, 0xC3, U8(st, G_EMPIRE_CITIES + 3u + cr), text_x(st) + px(st) + 0x28, py(st) + 0x6E, font);
    }
    uint32_t route = U8(st, fo + FIG_U7F);
    uint8_t action = U8(st, fo + FIG_ACTION);
    uint32_t ro = 0x58u * route; /* @bin 0x004FBA8B the trade records, 0x58 bytes */
    int32_t id;
    if (type == 0x14) {
        id = action == 0x0C ? 6 : action == 0x0A ? 7 : action == 0x0D ? 8 : 9;
    } else if (action == 0x0A) {
        id = 10;
    } else if (action == 0x0B) {
        id = (S32(st, G_TRADER_RECORDS + 4u + ro) == 0 && S32(st, G_TRADER_RECORDS + ro) == 0) ? 0x0D : 8;
    } else if (action == 9) {
        id = 0x0C;
    } else {
        id = 8;
    }
    text(st, 0x81, id, px(st) + 0x28, py(st) + 0x96, font);
    if (S32(st, G_TRADER_RECORDS + 4u + ro) == 0 && S32(st, G_TRADER_RECORDS + ro) == 0) {
        /* @bin 0x004FBB39 nothing traded yet: what the city buys (+6) and sells (+0x2A) that is open */
        S32(st, G_TEXT_X) = 0;
        text(st, 0x81, 2, px(st) + 0x28, py(st) + 0xAA, font);
        for (int32_t i = 1; i < 0x24; i++) {
            if (U8(st, G_EMPIRE_CITIES + 6u + cr + (uint32_t) i) != 0 && (uint8_t) GAME_CALL(st, FN_EMPIRE_CITY_TRADES, (int32_t) city, i) != 0) {
                draw_image(st, S32(st, G_RESOURCE_ICONS) + i, text_x(st) + px(st) + 0x28, py(st) + 0xA7);
                S32(st, G_TEXT_X) += 0x19;
            }
        }
        S32(st, G_TEXT_X) = 0;
        text(st, 0x81, 3, px(st) + 0x28, py(st) + 0xC8, font);
        for (int32_t i = 1; i < 0x24; i++) {
            if (U8(st, G_EMPIRE_CITIES + 0x2Au + cr + (uint32_t) i) != 0 && (uint8_t) GAME_CALL(st, FN_EMPIRE_CITY_TRADES, (int32_t) city, i) != 0) {
                draw_image(st, S32(st, G_RESOURCE_ICONS) + i, text_x(st) + px(st) + 0x28, py(st) + 0xC5);
                S32(st, G_TEXT_X) += 0x19;
            }
        }
        return fo;
    }
    /* @bin 0x004FBC57 the amounts bought (+8) and sold (+0x2C) by resource */
    for (int pass = 0; pass < 2; pass++) {
        uint32_t table = pass == 0 ? G_TRADER_RECORDS + 8u : G_TRADER_RECORDS + 0x2Cu;
        int32_t y = pass == 0 ? 0xAA : 0xC8;
        S32(st, G_TEXT_X) = 0;
        text(st, 0x81, pass == 0 ? 4 : 5, px(st) + 0x28, py(st) + y, font);
        for (int32_t i = 1; i < 0x24; i++) {
            int32_t v = U8(st, table + ro + (uint32_t) i);
            if (v == 0) {
                continue;
            }
            if (!resource_in_hundreds((uint32_t) i)) {
                v *= 100;
            }
            number(st, v, text_x(st) + px(st) + 0x28, py(st) + y, font);
            draw_image(st, S32(st, G_RESOURCE_ICONS) + i, text_x(st) + px(st) + 0x28, py(st) + y - 3);
            S32(st, G_TEXT_X) += 0x19;
        }
    }
    return fo;
}

static void carrier_panel(game_state *st, int32_t f, int32_t type, int32_t portrait, int32_t group, int32_t font)
{
    /* @bin 0x004FB2DD types 4, 9, 0x26 (cart pushers and the like): the home building, the load
     * +0x0B (u8) and its amount +0x74 (i16), the destination +0x5E */
    uint32_t fo = FIGURE_ADDR(f);
    (void) type;
    if (portrait >= 0) {
        draw_image(st, portrait, px(st) + 0x1C, py(st) + 0x70);
    }
    name(st, fo, 0x5A, 0);
    S32(st, G_TEXT_X) = 0;
    text(st, 0x29, building_type(st, S16(st, fo + FIG_HOME)), px(st) + 0x5C, py(st) + 0x8B, font);
    text(st, 0x40, 4, text_x(st) + px(st) + 0x5C, py(st) + 0x8B, font);
    S32(st, G_TEXT_X) = 0;
    if (!(U8(st, fo + FIG_TYPE) == 0x26 && U8(st, fo + FIG_ACTION) == 8) && S16(st, fo + FIG_AMOUNT) != 0 &&
        U8(st, fo + FIG_RESOURCE) != 0) {
        draw_image(st, (int32_t) U8(st, fo + FIG_RESOURCE) + S32(st, G_RESOURCE_ICONS), px(st) + 0x5C, py(st) + 0x96);
    }
    int32_t amount = S16(st, fo + FIG_AMOUNT);
    if (resource_in_hundreds(U8(st, fo + FIG_RESOURCE))) {
        amount /= 100;
    }
    if (amount != 0) {
        number(st, amount, text_x(st) + px(st) + 0x6E, py(st) + 0x98, font);
    }
    uint8_t r = U8(st, fo + FIG_RESOURCE);
    int unit = 0; /* 1: the unit's text (0x17 group) follows */
    if (r == 0x18 || r == 0x19 || r == 0x1A || r == 0x1E || r == 0x23) {
        /* @bin 0x004FB4C8 "load" / "loads" of 0x63 */
        if (amount == 1) {
            text(st, 0x63, 0x17, text_x(st) + px(st) + 0x6E, py(st) + 0x98, font);
            unit = 1;
        } else if (amount != 0) {
            text(st, 0x63, 0x18, text_x(st) + px(st) + 0x6E, py(st) + 0x98, font);
            unit = 1;
        }
    } else if (r == 0x1C || r == 0x0A) {
        unit = amount != 0;
    } else if (amount != 0) {
        if (r != 0) {
            text(st, 0x81, 0x14, text_x(st) + px(st) + 0x6E, py(st) + 0x98, font);
        }
        unit = 1;
    }
    if (unit) {
        /* @bin 0x004FB56D the resource's name; 0x1C, 0x0A: plural + 0x36 unless one */
        r = U8(st, fo + FIG_RESOURCE);
        int32_t id = r;
        if ((r == 0x1C || r == 0x0A) && amount != 1) {
            id = r + 0x36;
        }
        text(st, 0x17, id, text_x(st) + px(st) + 0x6E, py(st) + 0x98, font);
    }
    description(st, fo, group, 0xAA, font);
    int32_t from = S16(st, fo + FIG_HOME);
    if (from == 0) {
        return;
    }
    int32_t to = S16(st, fo + FIG_DEST_BUILDING);
    if (to == 0) {
        return;
    }
    S32(st, G_TEXT_X) = 0;
    uint8_t t = U8(st, fo + FIG_TYPE);
    uint8_t a = U8(st, fo + FIG_ACTION);
    /* @bin 0x004FB660 going back home: type 4 action 0xF, type 9 actions 0xB 0xE 0x11, type 0x26
     * actions 0xA 0xD 0xE */
    int back = (t == 4 && a == 0x0F) || (t == 9 && (a == 0x0B || a == 0x0E || a == 0x11)) ||
               (t == 0x26 && (a == 0x0A || a == 0x0D || a == 0x0E));
    if (t == 0x26 && U8(st, fo + FIG_ACTION) == 8) {
        return;
    }
    /* @bin 0x004FB6E3 "from A to B": back home 0x10 home, 0xE, destination; else 0xF destination,
     * 0xE, home */
    int32_t first = back ? from : to;
    int32_t second = back ? to : from;
    text(st, 0x81, back ? 0x10 : 0x0F, px(st) + 0x28, py(st) + 0xC8, font);
    text(st, 0x29, building_type(st, first), text_x(st) + px(st) + 0x28, py(st) + 0xC8, font);
    text(st, 0x81, 0x0E, text_x(st) + px(st) + 0x28, py(st) + 0xC8, font);
    text(st, 0x29, building_type(st, second), text_x(st) + px(st) + 0x28, py(st) + 0xC8, font);
}

static void figure_panel(game_state *st, int32_t f)
{
    /* @bin 0x004FAB20 the selected figure's panel */
    uint32_t fo = FIGURE_ADDR(f);
    int32_t type = U8(st, fo + FIG_TYPE);
    int32_t group = figure_text_group(st, f);
    int32_t font = S32(st, G_FONT_ROW);
    if (type == 0x5B) {
        type = U8(st, fo + FIG_RESOURCE);
    }
    S32(st, G_INFO_HELP) = 0x2A;
    int32_t portrait = figure_portrait(st, f);
    GAME_CALL(st, FN_LAYOUT_FRAME, 0, px(st) + 0x18, py(st) + 0x66, (S32(st, G_INFO_W) - 3) * 16, 0x7A, 0, px(st),
              py(st));
    uint32_t last = fo; /* the figure whose button is shown (the trade panel's leader) */
    if (type == 0x13 || type == 0x15 || type == 0x14) {
        last = trade_panel(st, f, type, font);
    } else if ((type >= 0x2B && type <= 0x35) || type == 0x63) {
        /* @bin 0x004FABD7 soldiers: the name and the formation's kind (0x00BCBA32 + 0x90 * +0x60) */
        S32(st, G_TEXT_X) = 0;
        int32_t formation = S16(st, fo + FIG_FORMATION);
        int32_t g = S32(st, fo + FIG_NAME_GROUP);
        if (g != -1) {
            text(st, g, S16(st, fo + FIG_NAME_INDEX), px(st) + 0x1C, py(st) + 0x6C, S32(st, G_FONT_LARGE2));
        }
        S32(st, G_TEXT_X) = 0;
        text(st, 0x25, U8(st, G_FORMATION_BASE + 0x6Au + (uint32_t) formation * 0x90u) + 0x0E, px(st) + 0x1C, py(st) + 0x95,
             font);
    } else {
        uint8_t raw = U8(st, fo + FIG_TYPE);
        if (raw == 0x5C || raw == 0x5D || raw == 0x65 || raw == 0x64) {
            /* @bin 0x004FB8A0 animals and the like: the type's name only */
            S32(st, G_TEXT_X) = 0;
            text(st, 0x40, raw, px(st) + 0x1C, py(st) + 0x6C, S32(st, G_FONT_LARGE2));
        } else if (raw == 0x19 || raw == 0x43 || raw == 0x44 || raw == 0x45 || raw == 0x46) {
            /* @bin 0x004FB84A the portrait and the type's name */
            if (portrait >= 0) {
                draw_image(st, portrait, px(st) + 0x1C, py(st) + 0x70);
            }
            S32(st, G_TEXT_X) = 0;
            text(st, 0x40, type, px(st) + 0x5C, py(st) + 0x8B, font);
        } else if (type == 4 || type == 9 || type == 0x26) {
            carrier_panel(st, f, type, portrait, group, font);
        } else if (type == 0x27) {
            /* @bin 0x004FACE6 the porter: what it carries (action 8) or holds (action 9) */
            if (portrait >= 0) {
                draw_image(st, portrait, px(st) + 0x1C, py(st) + 0x70);
            }
            name(st, fo, 0x5A, 0);
            S32(st, G_TEXT_X) = 0;
            text(st, 0x40, 0x27, px(st) + 0x5C, py(st) + 0x8B, font);
            uint8_t a = U8(st, fo + FIG_ACTION);
            if (a == 8) {
                text(st, 0x81, 0x11, text_x(st) + px(st) + 0x5A, py(st) + 0x8B, font);
                figure_draw_loads(st, f, text_x(st) + px(st) + 0x5A, py(st) + 0x87);
            } else if (a == 9) {
                text(st, 0x81, 0x12, text_x(st) + px(st) + 0x5A, py(st) + 0x8B, font);
                figure_draw_loads_held(st, f, text_x(st) + px(st) + 0x5A, py(st) + 0x87);
            }
            description(st, fo, group, 0xA0, font);
        } else if (type == 0x42) {
            /* @bin 0x004FAE96 the resource +0x7A (u8) it carries */
            if (portrait >= 0) {
                draw_image(st, portrait, px(st) + 0x1C, py(st) + 0x70);
            }
            int32_t icon = resource_icon(U8(st, fo + FIG_COLLECTING));
            name(st, fo, 0x5A, 0);
            S32(st, G_TEXT_X) = 0;
            text(st, 0x40, 0x42, px(st) + 0x5C, py(st) + 0x8B, font);
            text(st, 0x81, 0x15, text_x(st) + px(st) + 0x5A, py(st) + 0x8B, font);
            figure_draw_resource(st, icon, text_x(st) + px(st) + 0x5A, py(st) + 0x87);
            description(st, fo, group, 0xA0, font);
        } else if (type == 0x49) {
            /* @bin 0x004FB057 */
            name(st, fo, 0x5A, 0);
            S32(st, G_TEXT_X) = 0;
            text(st, 0x40, 0x49, px(st) + 0x5C, py(st) + 0x8B, font);
            if (portrait >= 0) {
                draw_image(st, portrait, px(st) + 0x1C, py(st) + 0x70);
            }
            if (U8(st, fo + FIG_ACTION) == 0x0C) {
                text(st, 0x81, 0x12, text_x(st) + px(st) + 0x5E, py(st) + 0x8B, font);
                draw_image(st, S32(st, G_RESOURCE_ICONS) + 8, text_x(st) + px(st) + 0x5E, py(st) + 0x87);
            }
            description(st, fo, group, 0xA0, font);
        } else {
            /* @bin 0x004FB1C1 the other figures: portrait (none for 0x56), name, type name (0x55
             * with +0x68 set: 0x60; 0x1B whose home's 0x004C5120 is 4: text 0x3B/0x26) */
            if (raw == 0x56) {
                portrait = -1;
            }
            if (raw == 0x55) {
                type = S16(st, fo + FIG_LEADER) != 0 ? 0x60 : 0x55;
            }
            if (portrait >= 0) {
                draw_image(st, portrait, px(st) + 0x1C, py(st) + 0x70);
            }
            name(st, fo, 0x5A, 1);
            S32(st, G_TEXT_X) = 0;
            if (type == 0x1B && GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, S16(st, fo + FIG_HOME)) == 4) {
                text(st, 0x3B, 0x26, text_x(st) + px(st) + 0x5C, py(st) + 0x8B, font);
            } else {
                text(st, 0x40, type, text_x(st) + px(st) + 0x5C, py(st) + 0x8B, font);
            }
            description(st, fo, group, 0xA0, font);
        }
    }
    /* @bin 0x004FBE2B the figure's button (+0x131: u8, shown when 0x006FDC88[it] is set; tables
     * 0x005F78F0) */
    uint8_t button = U8(st, last + FIG_U131);
    if (button != 0 && U8(st, G_ADVISOR_AVAILABLE + button) != 0) {
        GAME_CALL(st, FN_BUTTONS_DRAW, px(st) + 0x28, S32(st, G_INFO_H) * 16 + py(st) - 0x28,
                  S32(st, G_ADVISOR_BUTTON_IMAGES + 4u * button), 1);
    }
}

/* ---------------------------------------------------------------------------------------------- */
/* thumbnails and the figure list */

static void figure_thumbnail(game_state *st, int32_t tile, int32_t f)
{
    /* @bin 0x004FA330 the city view centred on the figure's tile, drawn into the spare screen
     * (0x004304C0 the tile's screen place, camera 0x00BCA2BC / 0x00BCA2C0 moved and restored) */
    int32_t cx = S32(st, G_CAMERA_X);
    int32_t cy = S32(st, G_CAMERA_Y);
    GAME_CALL(st, FN_VIEW_FIND, tile);
    S32(st, G_CAMERA_X) = S32(st, G_ROAD_ACCESS_X) - 2;
    S32(st, G_CAMERA_Y) = S32(st, G_ROAD_ACCESS_Y) - 6;
    GAME_CALL0(st, FN_CAMERA_CLAMP);
    GAME_CALL0(st, FN_THUMBNAIL_GEOMETRY);
    GAME_CALL(st, FN_THUMBNAIL_VIEW, S32(st, G_CAMERA_X), S32(st, G_CAMERA_Y), S32(st, G_MAP_START), f);
    S32(st, G_CAMERA_Y) = cy;
    S32(st, G_CAMERA_X) = cx;
    if (S32(st, G_SIDEBAR_COLLAPSED) == 1) {
        GAME_CALL0(st, FN_VIEW_GEOMETRY_A);
    } else {
        GAME_CALL0(st, FN_VIEW_GEOMETRY_B);
    }
}

static void figure_thumbnails(game_state *st)
{
    /* @bin 0x004FA3C0 the thumbnails of the listed figures, then the city view redrawn: each copied
     * from the city view's place 0x0070E3E4 / 0x0070E3E0 (x >= 0, y >= the height of image
     * 0x00F567E4 + 8, entry +0x16 of the 64-byte image entries 0x010D69F8) */
    if (S32(st, G_INFO_FIGURE_COUNT) <= 0) {
        return;
    }
    int32_t image = S32(st, G_SIDEBAR_IMAGES) + 8;
    if (image >= S32(st, G_IMAGE_COUNT)) {
        image = 0;
    }
    int32_t top = S16(st, G_IMGS_H + ((uint32_t) image << 6));
    for (int32_t i = 0; i < S32(st, G_INFO_FIGURE_COUNT); i++) {
        int32_t f = S32(st, G_FIGURES + 4u * (uint32_t) i);
        figure_thumbnail(st, S32(st, FIGURE_ADDR(f) + FIG_GRID_OFFSET), f);
        int32_t x = S32(st, G_SELECTED_X);
        int32_t y = S32(st, G_SELECTED_Y);
        S32(st, G_BLIT_X) = x;
        S32(st, G_BLIT_Y) = y;
        S16(st, G_UD3B5BE) = 0x0C;
        S16(st, G_UD3B5BC) = 0x30;
        if (x < 0) {
            S32(st, G_BLIT_X) = 0;
        }
        if (y < top) {
            S32(st, G_BLIT_Y) = top;
        }
        GAME_CALL(st, FN_BLOCK_FROM_SCREEN, S32(st, G_FIGURE_THUMB_OFFSETS + 4u * (uint32_t) i) + S32(st, G_SPARE_SCREEN),
                  S32(st, G_SCREEN_PITCH) - 0x60, 0);
    }
    GAME_CALL0(st, FN_CITY_VIEW_DRAW_CURRENT);
}

static void figure_list(game_state *st)
{
    /* @bin 0x004FA780 the figure list: the inner panel, the thumbnails in frames 60 pixels apart
     * (the selected one pressed), the selected figure's panel; "no one here" when empty */
    GAME_CALL(st, FN_LAYOUT_INNER_PANEL, px(st) + 0x10, py(st) + 0x28, S32(st, G_INFO_W) - 2, 0x0C, px(st), py(st));
    if (S32(st, G_INFO_FIGURE_COUNT) <= 0) {
        text_title(st, 0x46, 0, py(st) + 0x78, S32(st, G_FONT_ROW));
        S32(st, G_INFO_FIGURE_VIEW) = 1;
        return;
    }
    int32_t dx = 0;
    for (int32_t i = 0; i < S32(st, G_INFO_FIGURE_COUNT); i++, dx += 0x3C) {
        GAME_CALL(st, FN_LAYOUT_FRAME, 0, dx + px(st) + 0x19, py(st) + 0x2D, 0x34, 0x34,
                  i == S32(st, G_FIGURE_ROW), px(st), py(st));
        S32(st, G_BLIT_X) = dx + px(st) + 0x1B;
        S32(st, G_BLIT_Y) = py(st) + 0x2F;
        S16(st, G_UD3B5BE) = 0x0C;
        S16(st, G_UD3B5BC) = 0x30;
        GAME_CALL(st, FN_BLOCK_TO_SCREEN, S32(st, G_FIGURE_THUMB_OFFSETS + 4u * (uint32_t) i) + S32(st, G_SPARE_SCREEN),
                  S32(st, G_SCREEN_PITCH) - 0x60, 0);
    }
    figure_panel(st, S32(st, G_FIGURES + 4u * (uint32_t) S32(st, G_FIGURE_ROW)));
    S32(st, G_INFO_FIGURE_VIEW) = 1;
}

/* ---------------------------------------------------------------------------------------------- */
/* the panels */

static void info_panel_none(game_state *st)
{
    /* @bin 0x004FA4A0 kind 0: the panel, its title (0x46, 0) and the text 0xD/1 at the bottom */
    window_panel(st);
    text_title(st, 0x46, 0, py(st) + 8, S32(st, G_FONT_TITLE));
    text_title(st, 0x0D, 1, S32(st, G_INFO_H) * 16 + py(st) - 0x16, S32(st, G_FONT_ROW));
}

static void simple_panel(game_state *st, int32_t help, uint32_t wav, int32_t group, int32_t title,
                         int32_t textid, int32_t dy, int thumbs)
{
    /* the terrain and statue panels: help, sound, (thumbnails,) panel, title, a text block */
    S32(st, G_INFO_HELP) = help;
    sound_once(st, wav);
    if (thumbs) {
        figure_thumbnails(st);
    }
    window_panel(st);
    text_title(st, group, title, py(st) + 10, S32(st, G_FONT_TITLE));
    text_block(st, group, textid, px(st) + 0x20, S32(st, G_INFO_H) * 16 + py(st) + dy, (S32(st, G_INFO_W) - 4) * 16,
               S32(st, G_FONT_LARGE));
}

static void info_panel_park(game_state *st)
{
    /* @bin 0x004FE300 terrain kind 0xC (help 0x4F, "wavs\park.wav" 0x005F79E4, texts 0x4F) */
    simple_panel(st, 0x4F, G_WAV_PARK, 0x4F, 0, 1, -0x9E, 0);
}

static void info_panel_wall(game_state *st)
{
    /* @bin 0x004FE3C0 terrain kind 9 (help 0x55, "wavs\wall.wav" 0x005F79F4, texts 0x8B) */
    simple_panel(st, 0x55, G_WAV_WALL, 0x8B, 0, 1, -0x9E, 0);
}

static void info_panel_wall_0x17(game_state *st)
{
    /* @bin 0x004FE490 terrain kind 0x17 (help 0x55, "wavs\wall.wav", texts 0xA5) */
    simple_panel(st, 0x55, G_WAV_WALL, 0xA5, 0, 1, -0x9E, 0);
}

static void info_panel_wall_0x18(game_state *st)
{
    /* @bin 0x004FE560 terrain kind 0x18 (help 0x55, "wavs\wall.wav", texts 0xA6) */
    simple_panel(st, 0x55, G_WAV_WALL, 0xA6, 0, 1, -0x9E, 0);
}

static void info_panel_canal(game_state *st)
{
    /* @bin 0x004FE630 terrain kind 7 (help 0x3B, "wavs\aquaduct.wav" 0x005F7A04, texts 0x8D: 1 when
     * the tile byte 0x00EADD60 is set, else 2) */
    S32(st, G_INFO_HELP) = 0x3B;
    sound_once(st, G_WAV_AQUADUCT);
    window_panel(st);
    text_title(st, 0x8D, 0, py(st) + 10, S32(st, G_FONT_TITLE));
    text_block(st, 0x8D, S32(st, G_INFO_TILE_BYTE) != 0 ? 1 : 2, px(st) + 0x20, (S32(st, G_INFO_H) - 8) * 16 + py(st),
               (S32(st, G_INFO_W) - 4) * 16, S32(st, G_FONT_LARGE));
}

static void info_panel_plaza(game_state *st)
{
    /* @bin 0x004FE730 terrain kind 0xD with figures (help 0x4F, "wavs\plaza.wav" 0x005F7A18, texts
     * 0x89, the figure list) */
    S32(st, G_INFO_HELP) = 0x4F;
    sound_once(st, G_WAV_PLAZA);
    figure_thumbnails(st);
    window_panel(st);
    text_title(st, 0x89, 0, py(st) + 10, S32(st, G_FONT_TITLE));
    figure_list(st);
    text_block(st, 0x89, 1, px(st) + 0x20, S32(st, G_INFO_H) * 16 + py(st) - 0x71, (S32(st, G_INFO_W) - 4) * 16,
               S32(st, G_FONT_LARGE));
}

static void info_panel_rubble(game_state *st)
{
    /* @bin 0x005058F0 terrain kind 8 (help 0, "wavs\ruin.wav" 0x005F7CA8, texts 0x8C, the building
     * type's name of 0x00EADD68) */
    S32(st, G_INFO_HELP) = 0;
    sound_once(st, G_WAV_RUIN);
    window_panel(st);
    text_title(st, 0x8C, 0, py(st) + 10, S32(st, G_FONT_TITLE));
    text(st, 0x29, S32(st, G_INFO_TERRAIN), px(st) + 0x20, S32(st, G_INFO_H) * 16 + py(st) - 0xAD,
         S32(st, G_FONT_LARGE));
    text_block(st, 0x8C, 1, px(st) + 0x20, S32(st, G_INFO_H) * 16 + py(st) - 0x8F, (S32(st, G_INFO_W) - 4) * 16,
               S32(st, G_FONT_LARGE));
}

static void info_panel_statue(game_state *st)
{
    /* @bin 0x004FE800 building types 0x29-0x2B (help 0x4F, "wavs\statue.wav" 0x005F7A28, texts 0x50
     * 0 / 1, the thumbnails of the figures listed) */
    simple_panel(st, 0x4F, G_WAV_STATUE, 0x50, 0, 1, -0x9E, 1);
}

static void info_panel_statue_0x38(game_state *st)
{
    /* @bin 0x004FE8D0 building type 0x38 (as 0x004FE800, texts 0x50 2 / 3) */
    simple_panel(st, 0x4F, G_WAV_STATUE, 0x50, 2, 3, -0x9E, 1);
}

static void info_panel_terrain(game_state *st)
{
    /* @bin 0x004FA530 kind 1: the terrain kind's panel (0x00EADD4C), or the figures on the tile */
    int32_t kind = S32(st, G_INFO_TERRAIN_KIND);
    /* @bin 0x004FA538 the help by terrain kind (jump table 0x004FA754 / 0x004FA768) */
    switch (kind) {
    case 6: S32(st, G_INFO_HELP) = 0x39; break;
    case 7: S32(st, G_INFO_HELP) = 0x3B; break;
    case 9: case 0x17: case 0x18: S32(st, G_INFO_HELP) = 0x55; break;
    case 0x0B: S32(st, G_INFO_HELP) = 0x3A; break;
    default: S32(st, G_INFO_HELP) = 0; break;
    }
    if (kind == 0x0C) {
        info_panel_park(st);
        return;
    }
    if (kind == 0x0D) {
        if (S32(st, G_INFO_FIGURE_COUNT) <= 0) {
            info_panel_plaza(st);
            return;
        }
    } else if (kind == 9) {
        info_panel_wall(st);
        return;
    } else if (kind == 0x17) {
        info_panel_wall_0x17(st);
        return;
    } else if (kind == 0x18) {
        info_panel_wall_0x18(st);
        return;
    } else if (kind == 7) {
        info_panel_canal(st);
        return;
    } else if (kind == 8) {
        info_panel_rubble(st);
        return;
    }
    if (S32(st, G_INFO_SOUND) != 0) {
        /* @bin 0x004FA5E3 "wavs\empty_land.wav" 0x005F792C, or the first figure's sound 0x0053D0B0 */
        int32_t count = S32(st, G_INFO_FIGURE_COUNT);
        S32(st, G_INFO_SOUND) = 0;
        if (count <= 0) {
            GAME_CALL(st, FN_RETURN_ZERO, (int32_t) G_WAV_EMPTY_LAND, 1, 0);
        } else {
            GAME_CALL0(st, FN_INFO_FIGURE_SPEECH);
        }
    }
    if (S32(st, G_INFO_FIGURE_COUNT) > 0) {
        int32_t f = S32(st, G_FIGURES + 4u * (uint32_t) S32(st, G_FIGURE_ROW));
        if (f != 0) {
            /* @bin 0x004FA62C help 0x2A for figure types below 0x43, else 0x14A */
            S32(st, G_INFO_HELP) = U8(st, FIGURE_ADDR(f) + FIG_TYPE) < 0x43 ? 0x2A : 0x14A;
        }
    }
    figure_thumbnails(st);
    window_panel(st);
    if (S32(st, G_INFO_FIGURE_COUNT) == 0) {
        text_title(st, 0x46, S32(st, G_INFO_TERRAIN_KIND) + 10, py(st) + 10, S32(st, G_FONT_TITLE));
    }
    int32_t k = S32(st, G_INFO_TERRAIN_KIND);
    if (k == 3) {
        text_block(st, 0x46, 0x27, px(st) + 0x24, S32(st, G_INFO_H) * 16 + py(st) - 0x71, (S32(st, G_INFO_W) - 4) * 16,
                   S32(st, G_FONT_LARGE));
    } else if (k != 6 && k != 0x0D) {
        text_block(st, 0x46, k + 0x24, px(st) + 0x24, S32(st, G_INFO_H) * 16 + py(st) - 0x71,
                   (S32(st, G_INFO_W) - 4) * 16, S32(st, G_FONT_LARGE));
    }
    figure_list(st);
}

/* ---------------------------------------------------------------------------------------------- */

static int32_t run_none(game_state *st, const int32_t *a) { (void) a; info_panel_none(st); return 0; }
static int32_t run_terrain(game_state *st, const int32_t *a) { (void) a; info_panel_terrain(st); return 0; }
static int32_t run_thumbnails(game_state *st, const int32_t *a) { (void) a; figure_thumbnails(st); return 0; }
static int32_t run_thumbnail(game_state *st, const int32_t *a) { figure_thumbnail(st, a[0], a[1]); return 0; }
static int32_t run_list(game_state *st, const int32_t *a) { (void) a; figure_list(st); return 0; }
static int32_t run_portrait(game_state *st, const int32_t *a) { return figure_portrait(st, a[0]); }
static int32_t run_panel(game_state *st, const int32_t *a) { figure_panel(st, a[0]); return 0; }
static int32_t run_resource(game_state *st, const int32_t *a) { figure_draw_resource(st, a[0], a[1], a[2]); return 0; }
static int32_t run_loads(game_state *st, const int32_t *a) { figure_draw_loads(st, a[0], a[1], a[2]); return 0; }
static int32_t run_loads_held(game_state *st, const int32_t *a) { figure_draw_loads_held(st, a[0], a[1], a[2]); return 0; }
static int32_t run_park(game_state *st, const int32_t *a) { (void) a; info_panel_park(st); return 0; }
static int32_t run_wall(game_state *st, const int32_t *a) { (void) a; info_panel_wall(st); return 0; }
static int32_t run_wall17(game_state *st, const int32_t *a) { (void) a; info_panel_wall_0x17(st); return 0; }
static int32_t run_wall18(game_state *st, const int32_t *a) { (void) a; info_panel_wall_0x18(st); return 0; }
static int32_t run_canal(game_state *st, const int32_t *a) { (void) a; info_panel_canal(st); return 0; }
static int32_t run_plaza(game_state *st, const int32_t *a) { (void) a; info_panel_plaza(st); return 0; }
static int32_t run_rubble(game_state *st, const int32_t *a) { (void) a; info_panel_rubble(st); return 0; }
static int32_t run_statue(game_state *st, const int32_t *a) { (void) a; info_panel_statue(st); return 0; }
static int32_t run_statue38(game_state *st, const int32_t *a) { (void) a; info_panel_statue_0x38(st); return 0; }
static int32_t run_text_group(game_state *st, const int32_t *a) { return figure_text_group(st, a[0]); }

const ported_function info_figure_ports[] = {
    {0x004FA4A0u, "info_panel_none", 0, 0, 0, run_none, 0},
    {0x004FA530u, "info_panel_terrain", 0, 0, 0, run_terrain, 0},
    {0x004FA3C0u, "info_figure_thumbnails", 0, 0, 0, run_thumbnails, 0},
    {0x004FA330u, "info_figure_thumbnail", 2, 0, 0, run_thumbnail, 0},
    {0x004FA780u, "info_figure_list", 0, 0, 0, run_list, 0},
    {0x004FA8C0u, "info_figure_portrait", 1, 32, 0, run_portrait, 0},
    {0x004FAB20u, "info_figure_panel", 1, 0, 0, run_panel, 0},
    {0x004FBF00u, "info_figure_draw_resource", 3, 0, 0, run_resource, 0},
    {0x004FBF30u, "info_figure_draw_loads", 3, 0, 0, run_loads, 0},
    {0x004FC040u, "info_figure_draw_loads_held", 3, 0, 0, run_loads_held, 0},
    {0x004FE300u, "info_panel_park", 0, 0, 0, run_park, 0},
    {0x004FE3C0u, "info_panel_wall", 0, 0, 0, run_wall, 0},
    {0x004FE490u, "info_panel_wall_0x17", 0, 0, 0, run_wall17, 0},
    {0x004FE560u, "info_panel_wall_0x18", 0, 0, 0, run_wall18, 0},
    {0x004FE630u, "info_panel_canal", 0, 0, 0, run_canal, 0},
    {0x004FE730u, "info_panel_plaza", 0, 0, 0, run_plaza, 0},
    {0x005058F0u, "info_panel_rubble", 0, 0, 0, run_rubble, 0},
    {0x004FE800u, "info_panel_statue", 0, 0, 0, run_statue, 0},
    {0x004FE8D0u, "info_panel_statue_0x38", 0, 0, 0, run_statue38, 0},
    {0x0053D280u, "figure_text_group", 1, 32, 0, run_text_group, 0},
};
const int info_figure_ports_count = sizeof(info_figure_ports) / sizeof(info_figure_ports[0]);
