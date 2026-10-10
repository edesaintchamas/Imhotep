/* Imhotep - window 9: info panels (house) (package "info").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The house panel 0x0050E570 (building types 0x0A..0x1D): the vacant lot's panel when nobody lives
 * there, else the occupants, tax and happiness lines (0x004FC150, 0x004FC2D0, 0x004FC3B0), the
 * goods in store (the rows of 0x00F53DD8 and four fixed goods), and the house's evolution text:
 * what it needs to evolve, or the services (temples, entertainment) it lacks (0x0050E3C0). */
#include "info.h"

#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define IMG_DRAW FN_IMAGE_DRAW     /* image id at (x, y) */
#define PANEL FN_PANEL_DRAW        /* the window panel (x, y, w, h in blocks) */
#define TEXT_CENTRED FN_LAYOUT_GROUP_CENTRED /* layout text centred (group, index, x, y, width, font, colour, dx, dy) */
#define TEXT_LINES FN_LAYOUT_GROUP_LINES   /* layout text wrapped (group, index, x, y, width, font, colour, dx, dy) */
#define TEXT FN_LAYOUT_GROUP         /* layout text (group, index, x, y, font, colour, dx, dy) */
#define NUMBER FN_LAYOUT_NUMBER_AT       /* layout number (value, prefix, suffix, x, y, font, colour, dx, dy) */
#define INNER FN_LAYOUT_INNER_PANEL        /* layout inner panel (x, y, w, h, dx, dy) */
#define STRING FN_TEXT_DRAW       /* a string at (x, y): (string, x, y, font, colour) */
#define COUNT_TEXT FN_TEXT_COUNT_DRAW   /* "count string" (group, index, count, x, y, font, colour) */
#define SOUND FN_RETURN_ZERO        /* the panels' sound call (empty in this build) */


static int32_t px(game_state *st) { return S32(st, G_INFO_X); }
static int32_t py(game_state *st) { return S32(st, G_INFO_Y); }
static uint32_t house(game_state *st) { return BUILDING_ADDR(S32(st, G_INFO_BUILDING)); }

static void text(game_state *st, int32_t group, int32_t index, int32_t x, int32_t y, int32_t font, int32_t colour)
{
    GAME_CALL(st, TEXT, group, index, x, y, font, colour, px(st), py(st));
}

static void text_lines_font(game_state *st, int32_t group, int32_t index, int32_t dx, int32_t dy, int32_t wblocks,
                            uint32_t font)
{
    int32_t x = px(st), y = py(st);
    GAME_CALL(st, TEXT_LINES, group, index, x + dx, y + dy, wblocks << 4, S32(st, font), 0, x, y);
}

static void text_lines(game_state *st, int32_t group, int32_t index, int32_t dx, int32_t dy, int32_t wblocks)
{
    text_lines_font(st, group, index, dx, dy, wblocks, G_FONT_LARGE);
}

void info_house_people(game_state *st, int32_t y)
{
    /* @bin 0x004FC150 the occupants and the room left (+0x1E < 0) or the excess (> 0) */
    int32_t font = S32(st, G_FONT_ROW);
    S32(st, G_TEXT_X) = 0;
    GAME_CALLT(st, G_GRAPHICS_GFX, IMG_DRAW, S32(st, G_SIDE_IMAGES) + 0xD, px(st) + 0x22, y + 4);
    int32_t ty = y + 0xE;
    GAME_CALL(st, NUMBER, S16(st, house(st) + BLD_POPULATION), 0x40, G_NUMBER_SUFFIX, px(st) + S32(st, G_TEXT_X) + 0x32, ty, font,
              0, px(st), py(st));
    text(st, 0x7F, 0x14, px(st) + S32(st, G_TEXT_X) + 0x32, ty, font, 0);
    int16_t room = S16(st, house(st) + BLD_ROOM_LEFT);
    if (room < 0) {
        GAME_CALL(st, NUMBER, -(int32_t) room, 0x40, G_NUMBER_SUFFIX, px(st) + S32(st, G_TEXT_X) + 0x32, ty, font, 0,
                  px(st), py(st));
        text(st, 0x7F, 0x15, px(st) + S32(st, G_TEXT_X) + 0x32, ty, font, 0);
    } else if (room > 0) {
        text(st, 0x7F, 0x16, px(st) + S32(st, G_TEXT_X) + 0x32, ty, font, 0);
        GAME_CALL(st, NUMBER, S16(st, house(st) + BLD_ROOM_LEFT), 0x40, G_NUMBER_SUFFIX, px(st) + S32(st, G_TEXT_X) + 0x32, ty,
                  font, 0, px(st), py(st));
    }
}

void info_house_tax(game_state *st, int32_t y)
{
    /* @bin 0x004FC2D0 the tax line: not registered (+0x4E 0), or the taxes paid (0x0046F1C0) */
    int32_t b = S32(st, G_INFO_BUILDING);
    int32_t font = S32(st, G_FONT_ROW);
    S32(st, G_TEXT_X) = 0;
    if (U8(st, BUILDING_ADDR(b) + BLD_TAX_COVERAGE) == 0) {
        text(st, 0x7F, 0x17, px(st) + 0x24, y, font, 0);
        return;
    }
    int32_t tax = GAME_CALL(st, FN_BUILDING_0X46F1C0, b);
    text(st, 0x7F, 0x18, px(st) + S32(st, G_TEXT_X) + 0x24, y, font, 0);
    int32_t w = S32(st, G_TEXT_X);
    GAME_CALL(st, COUNT_TEXT, 8, 0, tax, px(st) + w + 0x24, y, font, 0);
    S32(st, G_TEXT_X) = S32(st, G_TEXT_X) + w;
    text(st, 0x7F, 0x19, px(st) + S32(st, G_TEXT_X) + 0x24, y, font, 0);
}

void info_house_happiness(game_state *st, int32_t y)
{
    /* @bin 0x004FC3B0 the happiness text by +0xC2 (signed byte): 50, 40, 30, 20, 10, 1 */
    int32_t font = S32(st, G_FONT_ROW);
    int8_t c = S8(st, house(st) + BLD_SENTIMENT);
    int32_t k;
    if (c >= 0x32) {
        k = 0x1A;
    } else if (c >= 0x28) {
        k = 0x1B;
    } else if (c >= 0x1E) {
        k = 0x1C;
    } else if (c >= 0x14) {
        k = 0x1D;
    } else if (c >= 0xA) {
        k = 0x1E;
    } else if (c >= 1) {
        k = 0x1F;
    } else {
        k = 0x20;
    }
    text(st, 0x7F, k, px(st) + 0x24, y, font, 0);
}

int32_t info_house_needs(game_state *st, int32_t *needs, int32_t entertainment)
{
    /* @bin 0x0050E3C0 the gods (entertainment 0) or the entertainment kinds (1) the house lacks: one
     * flag per kind in needs[16] (index: table 0x005D2A3C by the walker's type - 0x26); returns how
     * many are set. needs[0]: the city has no temple (0x00443D90(0x12)) / no festival (0x0078D728). */
    uint32_t p;
    int32_t id;
    if (!entertainment) {
        if (GAME_CALL(st, FN_GOODS_CHAIN_D90, 0x12) != 0) {
            needs[0] = 1;
        }
    } else if (S32(st, G_U78D728) <= 0) {
        needs[0] = 1;
    }
    /* @bin 0x0050E3EF the walker table 0x0077DE6A (0x6A bytes): +0 in use, +2 kind 1, 3 or 5, +0x3D,
     * reaching the house (0x00447790(id, 0x13)); +3 its type */
    for (id = 1, p = G_EMPIRE_CITIES + 0x6Au + 2u; p < G_EMPIRE_CITY_END + 2u; p += 0x6A, id++) {
        if (U8(st, p - 2) == 0) {
            continue;
        }
        uint8_t kind = U8(st, p);
        if (kind != 1 && kind != 3 && kind != 5) {
            continue;
        }
        if (U8(st, p + 0x3B) == 0) {
            continue;
        }
        if ((uint8_t) GAME_CALL(st, FN_EMPIRE_CITY_TRADES, id, 0x13) == 0) {
            continue;
        }
        needs[S32(st, G_CITY_KIND_TEXTS + 8u * U8(st, p + 1)) - 0x26]++;
    }
    if (entertainment) {
        /* @bin 0x0050E4D9 the kinds the city has (city +0x284C 1 or 3) cleared */
        for (id = 1, p = G_EMPIRE_CITIES + 0x6Au + 3u; p < G_EMPIRE_CITY_END + 3u; p += 0x6A, id++) {
            if (U8(st, p - 3) == 0) {
                continue;
            }
            int16_t t = S16(st, CURRENT_CITY(st) + CITY_U284C);
            if (t != 1 && t != 3) {
                continue;
            }
            if (U8(st, p + 0x3A) == 0) {
                continue;
            }
            if ((uint8_t) GAME_CALL(st, FN_EMPIRE_CITY_TRADES, id, 0x13) == 0) {
                continue;
            }
            int32_t *n = &needs[S32(st, G_CITY_KIND_TEXTS + 8u * U8(st, p)) - 0x26];
            if (*n != 0) {
                *n = 0;
            }
        }
    }
    int32_t count = 0;
    for (int i = 0; i < 16; i++) {
        if (needs[i] != 0) {
            count++;
        }
    }
    return count;
}

void info_house_vacant(game_state *st)
{
    /* @bin 0x0050EFB0 the vacant lot: title, the lot's text by its access (0x0047FE60) */
    GAME_CALL0(st, FN_INFO_FIGURE_THUMBNAILS);
    GAME_CALL(st, PANEL, px(st), py(st), S32(st, G_INFO_W), S32(st, G_INFO_H));
    {
        int32_t x = px(st), y = py(st);
        GAME_CALL(st, TEXT_CENTRED, 0x80, 0, x, y + 8, S32(st, G_INFO_W) << 4, S32(st, G_FONT_TITLE), 0, x, y);
    }
    {
        int32_t x = px(st), y = py(st);
        GAME_CALL(st, TEXT_CENTRED, 0xD, 1, x, (S32(st, G_INFO_H) << 4) + y - 0x16, S32(st, G_INFO_W) << 4,
                  S32(st, G_FONT_LARGE), 0, x, y);
    }
    GAME_CALL0(st, FN_INFO_FIGURE_LIST);
    uint32_t b = house(st);
    int32_t r = GAME_CALL(st, FN_BUILDING_ROAD_WITHIN, S16(st, b + BLD_X), S16(st, b + BLD_Y), 1, 2);
    int32_t x = px(st), y = py(st);
    GAME_CALL(st, TEXT_LINES, 0x80, r != 0 ? 1 : 2, x + 0x20, (S32(st, G_INFO_H) << 4) + y - 0x71,
              (S32(st, G_INFO_W) - 4) << 4, S32(st, G_FONT_LARGE), 0, x, y);
}

/* @bin 0x0050E7B8 jump table 0x0050EEEC (0x0050E8BA 0x0050EF38): a resource's inventory slot */
static int32_t inventory_slot(int32_t resource)
{
    switch (resource) {
    case 1: return 0;
    case 2: return 1;
    case 3: return 2;
    case 4: return 3;
    case 5: return 4;
    case 6: return 5;
    case 7: return 6;
    case 8: return 7;
    case 13: return 9;
    case 15: return 8;
    case 17: return 0xA;
    case 19: return 0xB;
    default: return -1;
    }
}

static void goods(game_state *st, int32_t resource, int32_t dx, int32_t dy)
{
    GAME_CALLT(st, G_GRAPHICS_GFX, IMG_DRAW, S32(st, G_RESOURCE_ICONS) + resource, px(st) + dx + 0x20, py(st) + dy);
    int32_t font = S32(st, G_FONT_LARGE);
    int32_t slot = inventory_slot(resource);
    int32_t x = px(st), y = py(st);
    uint32_t b = BUILDING_ADDR(S32(st, G_INFO_BUILDING));
    GAME_CALL(st, NUMBER, S16(st, b + BLD_HOUSE_INVENTORY + 2u * (uint32_t) slot), 0x40, G_NUMBER_SUFFIX, x + dx + 0x40, y + dy, font, 0,
              x, y);
}

static void lacks(game_state *st, int32_t entertainment)
{
    /* @bin 0x0050EBBD / 0x0050ED39 the gods or the entertainment the house lacks, listed "a, b or c" */
    int32_t needs[16] = {0};
    int32_t n = info_house_needs(st, needs, entertainment);
    int32_t shown;
    if (n > 4) {
        shown = 4;
    } else {
        shown = n;
        if (n == 0) {
            shown = 1;
            needs[0] = 1;
        }
    }
    S32(st, G_TEXT_X) = 0;
    S32(st, G_TEXT_Y) = 0;
    text_lines(st, 0x7F, U8(st, house(st) + BLD_EVOLVE_STATUS) + 0x28, 0x20, 0x24, S32(st, G_INFO_W) - 4);
    int32_t line = S32(st, G_TEXT_Y) + 0x10;
    S32(st, G_TEXT_X) = 0;
    S32(st, G_TEXT_Y) = line;
    int32_t done = 0;
    for (int32_t j = 0; j < 16; j++) {
        if (needs[j] == 0) {
            continue;
        }
        /* the gods' list reads the line back after each text (0x0050ED16), the entertainment's
         * keeps it (0x0050EDBB) */
        int32_t ly = entertainment ? line : S32(st, G_TEXT_Y);
        int32_t font = S32(st, G_FONT_LARGE);
        if (shown > 1 && done == shown - 1) {
            GAME_CALL(st, STRING, G_STR_OR, S32(st, G_TEXT_X) + px(st) + 0x20, ly + py(st) + 0x17, font, 0);
        } else if (done > 0) {
            GAME_CALL(st, STRING, G_STR_COMMA, S32(st, G_TEXT_X) + px(st) + 0x20, ly + py(st) + 0x17, font, 0);
        }
        ly = entertainment ? line : S32(st, G_TEXT_Y);
        int32_t x = px(st), y = py(st);
        GAME_CALL(st, TEXT, 0x17, j + 0x27, S32(st, G_TEXT_X) + x + 0x20, ly + y + 0x17, S32(st, G_FONT_LARGE), 0, x,
                  y);
        done++;
        if (done >= shown) {
            return;
        }
    }
}

void info_house(game_state *st)
{
    /* @bin 0x0050E570 the house panel (building types 0x0A..0x1D), help 0x38 */
    int32_t type = S16(st, house(st) + BLD_TYPE) - 0xA;
    S32(st, G_INFO_HELP) = 0x38;
    if (S32(st, G_INFO_SOUND) != 0) {
        GAME_CALL(st, SOUND, G_WAV_HOUSING, 1, 0); /* "wavs\housing.wav" */
        S32(st, G_INFO_SOUND) = 0;
    }
    uint32_t b = house(st);
    if (S16(st, b + BLD_POPULATION) <= 0 && U8(st, b + BLD_NO_ROOM) == 0) {
        /* @bin 0x0050E62A no occupant and not evolving: the vacant lot */
        info_house_vacant(st);
        return;
    }
    GAME_CALL(st, PANEL, px(st), py(st), S32(st, G_INFO_W), S32(st, G_INFO_H));
    {
        int32_t x = px(st), y = py(st);
        GAME_CALL(st, TEXT_CENTRED, 0x1D, type, x, y + 0xA, S32(st, G_INFO_W) << 4, S32(st, G_FONT_TITLE), 0, x, y);
    }
    {
        int32_t x = px(st), y = py(st);
        S32(st, G_DISTRIBUTION) = 0;
        GAME_CALL(st, INNER, x + 0x10, y + 0x94, S32(st, G_INFO_W) - 2, 0xA, x, y);
    }
    info_house_people(st, py(st) + 0x9A);
    info_house_tax(st, py(st) + 0xC2);
    info_house_happiness(st, py(st) + 0xD6);
    /* @bin 0x0050E70A the house level table 0x0070CDE8 (0x58 bytes per +0x12 level): +0xC 0, the
     * "no room to grow" text */
    if (S32(st, G_HOUSE_LEVEL_U28 + 0x58u * (uint32_t) S16(st, house(st) + BLD_SUBTYPE)) == 0) {
        text_lines_font(st, 0x7F, 0x21, 0x24, 0xEA, S32(st, G_INFO_W) - 6, G_FONT_ROW);
    }
    /* @bin 0x0050E759 the goods of the order rows (0x00F53DD8), 0x6E apart */
    for (int32_t i = 0, dx = 0; i < S32(st, G_GOODS_COUNT); i++, dx += 0x6E) {
        int32_t resource = S32(st, G_GOODS_COUNT + 4 + 4u * (uint32_t) i);
        if (resource != 0) {
            goods(st, resource, dx, 0x6E);
        }
    }
    /* @bin 0x0050E5D8 the four goods every house shows: 0xD, 0x13, 0x11, 0xF */
    static const int32_t fixed[4] = {0xD, 0x13, 0x11, 0xF};
    for (int32_t k = 0; k < 4; k++) {
        goods(st, fixed[k], k * 0x6E, 0x82);
    }
    b = house(st);
    uint8_t evolve = U8(st, b + BLD_NO_ROOM);
    if (evolve == 1 || evolve == 2) {
        /* @bin 0x0050E979 evolving (1) or devolving (2): the text and the days left (+0xF0) */
        int32_t x = px(st), y = py(st);
        S32(st, G_TEXT_X) = 0;
        GAME_CALL(st, TEXT_LINES, 0x7F, evolve == 1 ? 0x6B : 0x6D, x + 0x20, y + 0x4C, (S32(st, G_INFO_W) - 4) << 4,
                  S32(st, G_FONT_LARGE), 0, x, y);
        int32_t font = S32(st, G_FONT_LARGE);
        GAME_CALL(st, COUNT_TEXT, 8, 4, S32(st, house(st) + BLD_UF0), S32(st, G_TEXT_X) + px(st) + 0x20, py(st) + 0x4C,
                  font, 0);
        return;
    }
    int32_t c = U8(st, b + BLD_EVOLVE_STATUS);
    switch (c) {
    case 0x3E: {
        /* @bin 0x0050EA73 needs a bigger lot: the neighbour 0x00EADCD4 that blocks it */
        int32_t x = px(st), y = py(st);
        S32(st, G_TEXT_X) = 0;
        S32(st, G_TEXT_Y) = 0;
        GAME_CALL(st, TEXT, 0x7F, c + 0x28, x + 0x20, y + 0x24, S32(st, G_FONT_LARGE), 0, x, y);
        uint32_t n = BUILDING_ADDR(S32(st, G_WORST_NEIGHBOUR));
        int16_t nt = S16(st, n + BLD_TYPE);
        if (nt == 0xA && S16(st, n + BLD_POPULATION) == 0) {
            text(st, 0x80, 0, S32(st, G_TEXT_X) + px(st) + 0x20, py(st) + 0x24, S32(st, G_FONT_LARGE), 0xF800);
        } else {
            text(st, 0x29, nt, S32(st, G_TEXT_X) + px(st) + 0x20, py(st) + 0x24, S32(st, G_FONT_LARGE), 0xF800);
        }
        GAME_CALL(st, STRING, G_STR_CLOSE_PAREN, S32(st, G_TEXT_X) + px(st) + 0x20, py(st) + 0x24, S32(st, G_FONT_LARGE),
                  0);
        text_lines(st, 0x7F, U8(st, house(st) + BLD_EVOLVE_STATUS) + 0x29, 0x20, 0x34, S32(st, G_INFO_W) - 4);
        return;
    }
    case 0x44: case 0x48:
        lacks(st, 0);
        return;
    case 0x46: case 0x4A:
        lacks(st, 1);
        return;
    default:
        /* @bin 0x0050EEA8 the evolution's text by +0x85 */
        text_lines(st, 0x7F, c + 0x28, 0x20, 0x24, S32(st, G_INFO_W) - 4);
        return;
    }
}

static int32_t run_house(game_state *st, const int32_t *a)
{
    (void) a;
    info_house(st);
    return 0;
}

static int32_t run_vacant(game_state *st, const int32_t *a)
{
    (void) a;
    info_house_vacant(st);
    return 0;
}

static int32_t run_people(game_state *st, const int32_t *a)
{
    info_house_people(st, a[0]);
    return 0;
}

static int32_t run_tax(game_state *st, const int32_t *a)
{
    info_house_tax(st, a[0]);
    return 0;
}

static int32_t run_happiness(game_state *st, const int32_t *a)
{
    info_house_happiness(st, a[0]);
    return 0;
}

const ported_function info_house_ports[] = {
    {0x0050E570u, "info_house", 0, 0, 0, run_house, 0},
    {0x0050E3C0u, "info_house_needs", 2, 32, 0, NULL, 0}, /* writes its caller's stack */
    {0x0050EFB0u, "info_house_vacant", 0, 0, 0, run_vacant, 0},
    {0x004FC150u, "info_house_people", 1, 0, 0, run_people, 0},
    {0x004FC2D0u, "info_house_tax", 1, 0, 0, run_tax, 0},
    {0x004FC3B0u, "info_house_happiness", 1, 0, 0, run_happiness, 0},
};
const int info_house_ports_count = sizeof(info_house_ports) / sizeof(info_house_ports[0]);
