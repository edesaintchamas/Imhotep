/* Imhotep - the overlay tooltip: the text for the building under the mouse (roadmap phase 7.5).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * With an overlay on in the city, 0x004C9750 writes into the tooltip buffer 0x00D3A4C0 the
 * overlay's text (group 0x42 of Pharaoh_Text.eng) for the building or tile under the mouse,
 * chosen by thresholds on the building's service values, and asks for the kind-1 tooltip. Text
 * lookups go through text_seek 0x004CC110 (the pointer 0x00E20768 into the text file). */
#include "sidebar.h"

#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

static void text_seek(game_state *st, int32_t group, int32_t id)
{
    GAME_CALL(st, FN_TEXT_SEEK, group, id);
}

static uint32_t str_len(game_state *st, uint32_t s)
{
    uint32_t n = 0;
    while (P8(st, s + n) != 0) {
        n++;
    }
    return n;
}

static void str_copy(game_state *st, uint32_t dst, uint32_t src)
{
    /* rep movs of strlen + 1 bytes */
    uint32_t n = str_len(st, src) + 1;
    for (uint32_t i = 0; i < n; i++) {
        P8(st, dst + i) = P8(st, src + i);
    }
}

static void str_cat(game_state *st, uint32_t dst, uint32_t src)
{
    str_copy(st, dst + str_len(st, dst), src);
}

static uint32_t building(int32_t id)
{
    return BUILDING_ADDR(id);
}

static int occupied(game_state *st, int32_t id)
{
    return U8(st, building(id) + BLD_HOUSE_SIZE) != 0;
}

void tooltip_text_stock(game_state *st, int32_t id, int32_t a, int32_t b, int32_t *length)
{
    /* @bin 0x004CA3D0 an industry's stock: the number (0x004CC5C0) copied to the buffer and its
     * length added to *length, then the text 0x6E + k (0x6F + k for exactly 1) with k by the good
     * a (1: 0, 2: 6, 3: 4, 6: 2, 0x0D: 8, 0x11: 12, 0x13: 10, else 14, table 0x004CA558); the stock
     * as in 0x00433AE0 (types 0x46 / 0xBE word +0x56 + 2 b, 0x47 / 0xBF +0x56 + 2 a, 0x48 / 0xC1
     * 0x0045D510(id, a), 0x49 0x0045D510(0x00475B50(id), a), else +0x54 + 2 b, or "empty" 0x7E) */
    int32_t k;
    switch (a) {
    case 1: k = 0; break;
    case 2: k = 6; break;
    case 3: k = 4; break;
    case 6: k = 2; break;
    case 0x0D: k = 8; break;
    case 0x11: k = 0x0C; break;
    case 0x13: k = 0x0A; break;
    default: k = 0x0E; break;
    }
    int16_t t = S16(st, building(id) + BLD_TYPE);
    int32_t v;
    if (t == 0x46 || t == 0xBE) {
        v = S16(st, building(id) + BLD_INVENTORY + 2u * (uint32_t) b);
    } else if (t == 0x47 || t == 0xBF) {
        v = S16(st, building(id) + BLD_INVENTORY + 2u * (uint32_t) a);
    } else if (t == 0x48 || t == 0xC1) {
        v = GAME_CALL(st, FN_BUILDING_PARTS_STOCK, id, a);
    } else if (t == 0x49) {
        int32_t r = GAME_CALL(st, FN_BUILDING_MAIN_PART, id, a);
        v = GAME_CALL(st, FN_BUILDING_PARTS_STOCK, r, a);
    } else {
        if (!occupied(st, id)) {
            text_seek(st, 0x42, 0x7E);
            return;
        }
        v = S16(st, building(id) + BLD_HOUSE_INVENTORY + 2u * (uint32_t) b);
    }
    *length += GAME_CALL(st, FN_NUMBER_FORMAT, v, 0, 0);
    str_copy(st, G_TIP_TEXT, G_NUMBER_TEXT);
    text_seek(st, 0x42, k + (v == 1 ? 0x6F : 0x6E));
}

void tooltip_text_0xE8(game_state *st, int32_t id)
{
    /* @bin 0x004CA600 (i32 +0xE8) / 10: >= 85 text 0x8C, >= 60 0x8B, >= 45 0x8A, >= 1 0x89, else
     * (or an empty building) 0x88 */
    int32_t k = 0;
    if (occupied(st, id)) {
        int32_t v = S32(st, building(id) + BLD_MALARIA_RISK) / 10;
        if (v >= 0x55) {
            k = 4;
        } else if (v >= 0x3C) {
            k = 3;
        } else if (v >= 0x2D) {
            k = 2;
        } else if (v >= 1) {
            k = 1;
        }
    }
    text_seek(st, 0x42, k + 0x88);
}

void tooltip_text_0xFC(game_state *st, int32_t id)
{
    /* @bin 0x004CA6D0 i32 +0xFC: >= 850 text 0x83, >= 600 0x82, >= 450 0x81, >= 10 0x80, else 0x7F */
    int32_t k = 0;
    if (occupied(st, id)) {
        int32_t v = S32(st, building(id) + BLD_DISEASE_RISK);
        if (v >= 0x352) {
            k = 4;
        } else if (v >= 0x258) {
            k = 3;
        } else if (v >= 0x1C2) {
            k = 2;
        } else if (v >= 0x0A) {
            k = 1;
        }
    }
    text_seek(st, 0x42, k + 0x7F);
}

int32_t tooltip_text_problems(game_state *st, int32_t id)
{
    /* @bin 0x004CA770 the problems of the building's main part (0x00475B50): none -> 0; else the
     * text 0x8D (0x8E when empty), then per bit k < 14 of +0x100 a numbered line "@L%d. " and the
     * text 0x8F + k */
    int32_t main = GAME_CALL(st, FN_BUILDING_MAIN_PART, id);
    uint32_t b = building(main);
    if (U32(st, b + BLD_PROBLEMS) == 0) {
        return 0;
    }
    text_seek(st, 0x42, U8(st, b + BLD_HOUSE_SIZE) != 0 ? 0x8D : 0x8E);
    str_copy(st, G_TIP_TEXT, U32(st, G_TEXT_CURSOR));
    str_cat(st, G_TIP_TEXT, G_LINE_BREAK);
    int32_t n = 0;
    for (int32_t k = 0; k < 0x0E; k++) {
        if ((U32(st, b + BLD_PROBLEMS) & (1u << k)) == 0) {
            continue;
        }
        str_cat(st, G_TIP_TEXT, G_LINE_BREAK);
        n++;
        /* @bin 0x004CA894 sprintf(local, "%d. " (0x005E03F0), n): the call is made (its buffer, in
         * the original's frame, is given as 0: crt_sprintf then only counts), the text built here */
        GAME_CALL(st, FN_CRT_SPRINTF, 0, G_STR_NUMBER_DOT, n);
        char number[16];
        int len = 0;
        int32_t v = n;
        char digits[12];
        int d = 0;
        do {
            digits[d++] = (char) ('0' + v % 10);
            v /= 10;
        } while (v > 0);
        while (d > 0) {
            number[len++] = digits[--d];
        }
        number[len++] = '.';
        number[len++] = ' ';
        uint32_t end = G_TIP_TEXT + str_len(st, G_TIP_TEXT);
        for (int i = 0; i < len; i++) {
            P8(st, end + (uint32_t) i) = (uint8_t) number[i];
        }
        P8(st, end + (uint32_t) len) = 0;
        text_seek(st, 0x42, k + 0x8F);
        str_cat(st, G_TIP_TEXT, U32(st, G_TEXT_CURSOR));
    }
    return 1;
}

static int32_t by_byte_level(uint8_t v, int32_t none, int32_t high, int32_t low, int32_t mid)
{
    /* the common thresholds of the service values: 0, >= 80, < 20, else */
    if (v == 0) {
        return none;
    }
    if (v >= 0x50) {
        return high;
    }
    return v < 0x14 ? low : mid;
}

static int32_t by_word_level(int32_t v, int32_t first)
{
    /* <= 0, <= 200, <= 400, <= 600, <= 800, more */
    if (v <= 0) {
        return first;
    }
    if (v <= 0xC8) {
        return first + 1;
    }
    if (v <= 0x190) {
        return first + 2;
    }
    if (v <= 0x258) {
        return first + 3;
    }
    if (v <= 0x320) {
        return first + 4;
    }
    return first + 5;
}

int32_t tooltip_overlay_text(game_state *st)
{
    /* @bin 0x004C9750 (see the file comment); the overlay table 0x005E0350 (3 bytes per overlay):
     * +1 needs a building, +0 = 1 an occupied one, = 2 an empty one, +2 a tile with a value in
     * 0x00A5D4CC */
    if (S32(st, G_MOUSE_HELP) == 0 || U8(st, G_RIGHT_HELD) != 0 || U8(st, G_WINDOW) != 1 ||
        S32(st, G_EDITOR) == 1) {
        return 0;
    }
    int32_t tile = S32(st, G_MOUSE_OFFSET);
    if (tile == 0) {
        return 0;
    }
    int32_t overlay = S32(st, G_OVERLAY);
    if (overlay == 0 || overlay == 2 || overlay == 1 || overlay == 3 || overlay == 5 || overlay == 6 ||
        overlay == 7) {
        return 0;
    }
    int32_t id = 0;
    int32_t grid_building = S16(st, G_GRID_BUILDING + 2u * (uint32_t) tile);
    if ((U8(st, G_GRID_TERRAIN + 4u * (uint32_t) tile) & 0x40) == 0 &&
        S16(st, building(grid_building) + BLD_TYPE) != 0xD1) {
        id = GAME_CALL(st, FN_BUILDING_MAIN_PART, grid_building);
        overlay = S32(st, G_OVERLAY);
    }
    uint32_t row = G_OVERLAY_TIPS + 3u * (uint32_t) overlay;
    if (U8(st, row + 1) != 0 && id == 0) {
        return 0;
    }
    if (U8(st, row) == 1 && !occupied(st, id)) {
        return 0;
    }
    if (U8(st, row) == 2 && occupied(st, id)) {
        return 0;
    }
    if (U8(st, row + 2) != 0 && U8(st, G_GRID_FERTILITY + (uint32_t) tile) == 0) {
        return 0;
    }
    int32_t length = 0;
    text_seek(st, 0x42, 0);
    overlay = S32(st, G_OVERLAY);
    uint32_t b = building(id);
    int32_t text = -1; /* -1: keep the current text */
    switch (overlay) {
    case 0x2D:
        if (id != 0 && occupied(st, id)) {
            uint8_t v = U8(st, b + BLD_ACCESS_WATER);
            if (v >= 0x50) {
                text = 9;
            } else if (v > 0x10) {
                text = 10;
            } else if (v != 0) {
                text = 11;
            } else if ((U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile) & 0x2000u) != 0) {
                text = 3;
            } else {
                text = U8(st, b + BLD_WELL_ACCESS) != 0 ? 3 : 2;
            }
        } else {
            uint32_t terrain = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile);
            if ((terrain & 0x80) != 0) {
                text = (terrain & 0x2000u) != 0 ? 3 : 1;
            } else if ((terrain & 0x2000u) != 0) {
                text = 3;
            } else {
                return 0;
            }
        }
        break;
    case 4: {
        int8_t v = S8(st, b + BLD_RELIGION);
        text = v <= 0 ? 0x0C : v <= 1 ? 0x0D : v <= 2 ? 0x0E : v <= 3 ? 0x0F : v <= 4 ? 0x10 : v <= 5 ? 0x11 : 0x12;
        break;
    }
    case 8:
        text = by_word_level(S16(st, b + BLD_FIRE_RISK), 0x2E);
        break;
    case 9:
        text = by_word_level(S16(st, b + BLD_COLLAPSE_RISK), 0x34);
        break;
    case 0x0A: {
        int32_t v = S32(st, b + BLD_CRIME_RISK);
        text = v <= 0 ? 0x3A : v <= 0x64 ? 0x3B : v <= 0xC8 ? 0x3C : v <= 0x12C ? 0x3D : v < 0x1F4 ? 0x3E : 0x3F;
        break;
    }
    case 0x0B: {
        int8_t v = S8(st, b + BLD_ENTERTAINMENT);
        if (v <= 0) {
            text = 0x40;
        } else if (v >= 0x5A) {
            text = 0x4A;
        } else {
            text = 0x41 + v / 10;
        }
        break;
    }
    case 0x0C: text = by_byte_level(U8(st, b + BLD_ACCESS_JUGGLER), 0x4B, 0x4C, 0x4E, 0x4D); break;
    case 0x0D: text = by_byte_level(U8(st, b + BLD_ACCESS_MUSICIAN), 0x4F, 0x50, 0x52, 0x51); break;
    case 0x0E: text = by_byte_level(U8(st, b + BLD_ACCESS_DANCER), 0x53, 0x54, 0x56, 0x55); break;
    case 0x0F: text = by_byte_level(U8(st, b + BLD_ACCESS_SENET), 0x57, 0x58, 0x5A, 0x59); break;
    case 0x10: text = by_byte_level(U8(st, b + BLD_ACCESS_ZOOKEEPER), 0xA8, 0xA9, 0xAB, 0xAA); break;
    case 0x11: {
        uint8_t v = U8(st, b + BLD_EDUCATION);
        if (v == 0) {
            text = 0x64;
        } else if (v == 1) {
            text = 0x65;
        } else if (v == 2) {
            text = 0x66;
        }
        break;
    }
    case 0x12: text = by_byte_level(U8(st, b + BLD_ACCESS_TEACHER), 0x13, 0x14, 0x16, 0x15); break;
    case 0x13: text = by_byte_level(U8(st, b + BLD_ACCESS_LIBRARIAN), 0x17, 0x18, 0x1A, 0x19); break;
    case 0x15: text = by_byte_level(U8(st, b + BLD_ACCESS_DENTIST), 0x1F, 0x20, 0x22, 0x21); break;
    case 0x17: text = by_byte_level(U8(st, b + BLD_ACCESS_HERBALIST), 0x23, 0x24, 0x26, 0x25); break;
    case 0x16: text = by_byte_level(U8(st, b + BLD_ACCESS_PHYSICIAN), 0x84, 0x87, 0x85, 0x86); break;
    case 0x18: text = by_byte_level(U8(st, b + BLD_ACCESS_EMBALMER), 0x27, 0x28, 0x2A, 0x29); break;
    case 0x19: {
        int32_t v = GAME_CALL(st, FN_BUILDING_0X46F1C0, id);
        if (v <= 0) {
            text = U8(st, b + BLD_TAX_COVERAGE) != 0 ? 0x2C : 0x2B;
            break;
        }
        /* @bin 0x004C9E3C the number, then the text 0x2D after it */
        length += GAME_CALL(st, FN_NUMBER_FORMAT, v, 0, 0);
        str_copy(st, G_TIP_TEXT, G_NUMBER_TEXT);
        text_seek(st, 0x42, 0x2D);
        break;
    }
    case 0x2B: tooltip_text_0xFC(st, id); break;
    case 0x30: tooltip_text_0xE8(st, id); break;
    case 0x21: tooltip_text_stock(st, id, 1, 0, &length); break;
    case 0x22: tooltip_text_stock(st, id, 3, 2, &length); break;
    case 0x23: tooltip_text_stock(st, id, 6, 5, &length); break;
    case 0x24: tooltip_text_stock(st, id, 2, 1, &length); break;
    case 0x25: tooltip_text_stock(st, id, 4, 3, &length); break;
    case 0x27: tooltip_text_stock(st, id, 0x0D, 9, &length); break;
    case 0x28: tooltip_text_stock(st, id, 0x13, 0x0B, &length); break;
    case 0x29: tooltip_text_stock(st, id, 0x11, 0x0A, &length); break;
    case 0x2A: tooltip_text_stock(st, id, 0x0F, 8, &length); break;
    case 0x1A: {
        /* @bin 0x004C9FA5 a house's food: the 8 goods kept (+0x54 + 2 k for the goods k + 1 that
         * the city trades, 0x00443B90 / 0x004439C0) against its need */
        int16_t people = S16(st, b + BLD_POPULATION);
        if (people <= 0) {
            return 0;
        }
        int32_t need = S32(st, G_HOUSE_LEVEL_U28 + 0x58u * (uint32_t) (int32_t) S16(st, b + BLD_SUBTYPE));
        if (need != 0) {
            need = people;
        }
        int32_t sum = 0;
        for (int32_t g = 1; g <= 8; g++) {
            if (GAME_CALL(st, FN_GOODS_CHAIN_B90, g) != 0 || GAME_CALL(st, FN_GOODS_TRADE_CITY, g) != 0) {
                sum += S16(st, b + BLD_HOUSE_INVENTORY + 2u * (uint32_t) (g - 1));
            }
        }
        if (need == 0) {
            text = 0x68;
            break;
        }
        int32_t r = GAME_CALL(st, FN_CALC_RATIO_PERCENT, sum, need);
        if (need <= 0) {
            text = 0x68;
        } else if (r > 0x3E8) {
            text = 7;
        } else if (r >= 0x1F4) {
            text = 6;
        } else {
            text = r > 0 ? 5 : 4;
        }
        break;
    }
    case 0x1B: {
        int32_t v = S8(st, G_GRID_DESIRABILITY + (uint32_t) tile);
        text = v < -10 ? 0xA2 : v < 0 ? 0xA3 : v <= 0x0F ? 0xA4 : v <= 0x28 ? 0xA5 : v <= 0x46 ? 0xA6 : 0xA7;
        break;
    }
    case 0x1C: {
        int32_t v = U8(st, G_GRID_FERTILITY + (uint32_t) tile);
        text = v == 0 ? 0x6D : v < 0x19 ? 0x6C : v < 0x32 ? 0x6B : v < 0x4B ? 0x6A : 0x69;
        break;
    }
    case 0x1D: {
        int32_t r = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S16(st, b + BLD_SERVICE), 0x12C);
        if (S16(st, b + BLD_SERVICE) <= 0) {
            text = 0x5E;
        } else {
            text = r <= 0x14 ? 0x5F : r <= 0x28 ? 0x60 : r <= 0x3C ? 0x61 : r <= 0x50 ? 0x62 : 0x63;
        }
        break;
    }
    case 0x2C:
        if (!occupied(st, id)) {
            return 0;
        }
        if (U8(st, b + BLD_NO_ROOM) == 1) {
            text = 0x83;
        } else if (U8(st, b + BLD_NO_ROOM) == 2) {
            text = 0xAC;
        } else {
            return 0;
        }
        break;
    case 0x2E:
        if (!occupied(st, id) || S16(st, b + BLD_POPULATION) != 0) {
            return 0;
        }
        text = 0x9C;
        break;
    case 0x1F:
        if (tooltip_text_problems(st, id) == 0) {
            return 0;
        }
        U32(st, G_TEXT_CURSOR) = 0;
        goto done;
    case 0x1E:
    case 0x31:
    case 0x33:
        return 0;
    case 0x32: text = by_byte_level(U8(st, b + BLD_ACCESS_MAGISTRATE), 0x9E, 0x9F, 0xA1, 0xA0); break;
    default:
        break;
    }
    if (text >= 0) {
        text_seek(st, 0x42, text);
    }
    if (U32(st, G_TEXT_CURSOR) != 0) {
        str_copy(st, G_TIP_TEXT + (uint32_t) length, U32(st, G_TEXT_CURSOR));
    }
done:
    S32(st, G_TIP_SAVED_VISIBLE) = S32(st, G_MESSAGE_WINDOW_VISIBLE);
    S32(st, G_TIP_SAVED_SCROLL) = S32(st, G_SCROLL);
    S32(st, G_TIP_SAVED_LINES) = S32(st, G_TEXT_LINES);
    S32(st, G_UD3A6E0) = 0;
    S32(st, G_UD3A6DC) = S32(st, G_MOUSE_OFFSET);
    S32(st, G_TIP_WINDOW) = U8(st, G_WINDOW);
    S32(st, G_TIP_ID) = 1;
    S32(st, G_TIP_KIND) = 1;
    U8(st, G_REDRAW) = 1;
    return 1;
}

/* ---- port table ---- */

static int32_t run_text(game_state *st, const int32_t *a) { (void) a; return tooltip_overlay_text(st); }
static int32_t run_e8(game_state *st, const int32_t *a) { tooltip_text_0xE8(st, a[0]); return 0; }
static int32_t run_fc(game_state *st, const int32_t *a) { tooltip_text_0xFC(st, a[0]); return 0; }
static int32_t run_problems(game_state *st, const int32_t *a) { return tooltip_text_problems(st, a[0]); }

static int32_t run_stock(game_state *st, const int32_t *a)
{
    /* the length is in the caller's frame in the original (0x004C9750's local): kept when it is
     * in the state, else counted from 0 */
    uint8_t *p = state_ptr(st, (uint32_t) a[3], 4);
    int32_t length = p ? P32(st, (uint32_t) a[3]) : 0;
    tooltip_text_stock(st, a[0], a[1], a[2], &length);
    if (p) {
        P32(st, (uint32_t) a[3]) = (uint32_t) length;
    }
    return 0;
}

const ported_function tooltip_text_ports[] = {
    {0x004C9750u, "tooltip_overlay_text", 0, 32, 0, run_text, 0},
    {0x004CA3D0u, "tooltip_text_stock", 4, 0, 0, run_stock, 0},
    {0x004CA600u, "tooltip_text_0xE8", 1, 0, 0, run_e8, 0},
    {0x004CA6D0u, "tooltip_text_0xFC", 1, 0, 0, run_fc, 0},
    {0x004CA770u, "tooltip_text_problems", 1, 8, 0, run_problems, 0},
};
const int tooltip_text_ports_count = sizeof(tooltip_text_ports) / sizeof(tooltip_text_ports[0]);
