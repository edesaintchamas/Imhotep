/* Imhotep - window 9: info panels (monument) (package "info").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The panels of the monuments and the special buildings the info window's redraw 0x004F8E60 calls
 * through its jump table on the building type (present/ui/info/info.c): pyramids and mastabas,
 * sphinx, obelisk, temple complexes, sun temples, the mausoleums, the Alexandria library, the
 * Caesareum, the lighthouse, the Abu Simbel and the other Cleopatra monuments. Each panel writes the
 * help id 0x00EADD54, plays the building's sound once (0x0047D350, an empty function in this build)
 * and draws through the primitives of package "blit", reached through GAME_CALL.
 *
 * Many panels draw a text of Pharaoh_Text.eng with its "[keywords]" expanded into a 500-byte buffer
 * on their stack (0x00448370) and wrapped into lines (0x004CD010). The state does not hold the
 * original's stack, so these calls cannot go through GAME_CALL: the expansion (0x00448370 and its
 * 0x004CB980) and the wrapping (0x004CD010 and its 0x004CD120) are reproduced here on a host buffer,
 * with every state write and every other call of the original (0x0056B240, the keyword handlers,
 * 0x004CC020, 0x004CC770) made through GAME_CALL. */
#include <string.h>

#include "info.h"

#include "city/day2_monument.h"
#include "draw/mem.h"
#include "city/monument_rating.h"
#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define TEXT_BUF_SIZE 0x1F4         /* @bin 0x00512432 the expanded text's buffer (500 bytes) */

/* building fields (docs/formats/save.md 5) */

static int32_t px(game_state *st) { return S32(st, G_INFO_X); }
static int32_t py(game_state *st) { return S32(st, G_INFO_Y); }
static int32_t pw(game_state *st) { return S32(st, G_INFO_W); }
static int32_t ph(game_state *st) { return S32(st, G_INFO_H); }
static int32_t font_text(game_state *st) { return S32(st, G_FONT_LARGE); }

static int32_t blocks(int32_t n)
{
    /* shl 4 on a 32-bit register */
    return (int32_t) ((uint32_t) n << 4);
}

static int32_t width15(int32_t w)
{
    /* lea eax, [w + w*2]; lea eax, [eax + eax*4] */
    return (int32_t) ((uint32_t) w * 15u);
}

static void sound_once(game_state *st, uint32_t sound)
{
    /* the building's sound on the panel's first draw (0x0047D350: name, 1, 0) */
    if (S32(st, G_INFO_SOUND) != 0) {
        GAME_CALL(st, FN_RETURN_ZERO, (int32_t) sound, 1, 0);
        S32(st, G_INFO_SOUND) = 0;
    }
}

static void window_panel(game_state *st)
{
    GAME_CALL(st, FN_PANEL_DRAW, px(st), py(st), pw(st), ph(st));
}

static void title(game_state *st, int32_t group, int32_t id)
{
    /* @bin 0x004D6250 the title, centred over the panel's width at y + 10, large font */
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, group, id, px(st), py(st) + 10, blocks(pw(st)), S32(st, G_FONT_TITLE), 0, px(st),
              py(st));
}

static void group_lines(game_state *st, int32_t group, int32_t id, int32_t x, int32_t y, int32_t width,
                        int32_t colour)
{
    /* @bin 0x004D6160 a wrapped text of Pharaoh_Text.eng at (x, y) in the text font (the panel's
     * origin passed as the layout offset) */
    GAME_CALL(st, FN_LAYOUT_GROUP_LINES, group, id, x, y, width, font_text(st), colour, px(st), py(st));
}

static void number(game_state *st, int32_t value, int32_t x, int32_t y)
{
    /* @bin 0x004D6860 a number with the percent suffix 0x005E03FC (prefix character 0x40) */
    GAME_CALL(st, FN_LAYOUT_NUMBER_AT, value, 0x40, (int32_t) G_PERCENT_STRING, x, y, font_text(st), 0, px(st), py(st));
}

static int32_t city_stage(game_state *st)
{
    /* @bin 0x00511DA9 the i16 at 0x00EA5BAA of the player's record (0x49D8 bytes, index [0x00EAAC28]):
     * the stage the sphinx's texts and works follow */
    return S16(st, CITY_BASE + CITY_U433A + CITY_REC_SIZE * U32(st, CURRENT_PLAYER_ADDR));
}

static void group_text(game_state *st, int32_t group, int32_t id, int32_t x, int32_t y)
{
    /* @bin 0x004D62F0 a text at (x, y) */
    GAME_CALL(st, FN_LAYOUT_GROUP, group, id, x, y, font_text(st), 0, px(st), py(st));
}

/* ---- the expanded texts on the host (the original's stack buffer) ---- */

static int32_t expand_keywords(game_state *st, uint32_t self, uint32_t text, char *out, int32_t size)
{
    /* @bin 0x004CB980 (thiscall on the keyword table 0x00784F34) the text copied into out, each
     * "[name]" looked up in the table [self] (entries of 0x0C bytes: +4 name, +8 handler; CRT
     * _strnicmp 0x0056B240 on the name's length in the text) and replaced by the string its handler
     * returns (0x00D3B18C holds the entry while it runs). Returns 1 when the text's NUL is copied, 0
     * when out fills up or a field is not closed or not found (no terminator). The same as the port of
     * package msgempire (present/ui/messages/expand.c), with its calls made through GAME_CALL */
    int32_t i = 0;
    int32_t n = 0;
    for (;;) {
        uint8_t c = mem_rd8(st, text + (uint32_t) i);
        while (c != 0x5B) {
            if (n >= size) {
                return 0;
            }
            out[n++] = (char) c;
            if (mem_rd8(st, text + (uint32_t) i) == 0) {
                return 1;
            }
            c = mem_rd8(st, text + (uint32_t) i + 1);
            i++;
        }
        int32_t start = i + 1;
        c = mem_rd8(st, text + (uint32_t) i);
        while (c != 0x5D) {
            if (c == 0) {
                return 0;
            }
            c = mem_rd8(st, text + (uint32_t) i + 1);
            i++;
        }
        uint32_t e = U32(st, self);
        for (;;) {
            uint32_t name = mem_rd32(st, e + 4);
            if (name == 0) {
                return 0;
            }
            if (GAME_CALL(st, FN_CRT_STRNICMP, (int32_t) name, (int32_t) (text + (uint32_t) start), i - start) == 0) {
                break;
            }
            e += 0x0C;
        }
        uint32_t saved = U32(st, G_KEYWORD_CURRENT);
        U32(st, G_KEYWORD_CURRENT) = e;
        uint32_t r = (uint32_t) GAME_CALL0(st, mem_rd32(st, e + 8));
        U32(st, G_KEYWORD_CURRENT) = saved;
        if (r != 0 && mem_rd8(st, r) != 0) {
            do {
                if (n >= size) {
                    return 0;
                }
                out[n++] = (char) mem_rd8(st, r);
                r++;
            } while (mem_rd8(st, r) != 0);
        }
        i++;
    }
}

static void expand(game_state *st, uint32_t text, char *out, int32_t size)
{
    /* @bin 0x00448370 out[0] := 0, then the text with its "[keywords]" expanded (0x004CB980 on the
     * keyword table 0x00784F34) */
    out[0] = 0;
    expand_keywords(st, G_KEYWORDS_THIS, text, out, size);
}

static int32_t word_width_host(game_state *st, const char *text, int32_t font)
{
    /* @bin 0x004CD120 the width of the next word: leading spaces 4 each, then up to a space or '$'
     * (bytes below 0x20 take no room); bytes taken in 0x00E20760. The character goes to 0x004CC020
     * in the low byte of the font argument's slot (mov [esp + 0x18], al), its other bytes the font's */
    int seen = 0;
    int32_t w = 0;
    S32(st, G_WORD_CHARS) = 0;
    for (int32_t i = 1; i < 0x7D0; i++) {
        uint8_t c = (uint8_t) *text++;
        if (c == 0) {
            break;
        }
        if (c == 0x20) {
            if (seen) {
                break;
            }
            w += 4;
        } else if (c == 0x24) {
            if (seen) {
                break;
            }
        } else if (c >= 0x20) {
            w += GAME_CALL(st, FN_GLYPH_WIDTH, (int32_t) (((uint32_t) font & 0xFFFFFF00u) | c), font);
            seen = 1;
        }
        S32(st, G_WORD_CHARS) += 1;
    }
    return w;
}

static void lines_host(game_state *st, const char *text, int32_t x, int32_t y, int32_t width, int32_t font,
                       int32_t colour)
{
    /* @bin 0x004CD010 the text in lines of whole words narrower than width (at most 99 lines), each
     * copied to 0x00E3439C (cleared, 200 bytes) and drawn (0x004CC770) at the next y (+ line height
     * + 5, also added to 0x00E20774); the widest line in 0x00E2076C */
    int more = 1;
    for (int32_t line = 1; line < 0x64; line++) {
        for (uint32_t i = 0; i < 200; i += 4) {
            U32(st, G_LINE_BUF + i) = 0;
        }
        uint32_t n = 0;
        if (more) {
            int32_t w = 0;
            for (;;) {
                if (w >= width) {
                    break;
                }
                w += word_width_host(st, text, font);
                if (w < width) {
                    for (int32_t k = S32(st, G_WORD_CHARS); k > 0; k--) {
                        U8(st, G_LINE_BUF + n) = (uint8_t) *text++;
                        n++;
                    }
                    if (*text == 0) {
                        more = 0;
                        break;
                    }
                    continue;
                }
                if (w != 0) {
                    continue;
                }
                more = 0;
                break;
            }
        }
        S32(st, G_RUN_HASH) = 1;
        S32(st, G_TEXT_X) = 0;
        U8(st, G_RUN_NO_COLLAPSE) = 1;
        GAME_CALL(st, FN_TEXT_DRAW, (int32_t) G_LINE_BUF, x, y, font, colour);
        if (S32(st, G_TEXT_X) > S32(st, G_LINE_MAX_W)) {
            S32(st, G_LINE_MAX_W) = S32(st, G_TEXT_X);
        }
        int32_t h = S32(st, G_FONT_HEIGHT);
        y += h + 5;
        S32(st, G_TEXT_Y) = S32(st, G_TEXT_Y) + h + 5;
        if (!more) {
            break;
        }
    }
}

static void expanded_lines(game_state *st, int32_t group, int32_t id, int32_t x, int32_t y, int32_t width)
{
    /* the pattern of the panels: the text looked up (0x004CC110), expanded into the stack buffer
     * (0x00448370, 500 bytes) and wrapped (0x004CD010) in the text font, colour 0 */
    char buf[TEXT_BUF_SIZE + 1];
    memset(buf, 0, sizeof(buf));
    GAME_CALL(st, FN_TEXT_SEEK, group, id);
    expand(st, U32(st, G_TEXT_CURSOR), buf, TEXT_BUF_SIZE);
    lines_host(st, buf, x, y, width, font_text(st), 0);
}

/* ---- the monuments' work records ---- */

static int32_t monument_tables_size(int16_t type)
{
    /* @bin 0x004E60A6 neg / sbb on (type - 0xCF): 10 entries for type 0xCF, 18 for the others */
    return type == 0xCF ? 10 : 18;
}

static uint32_t monument_table(int16_t type)
{
    /* @bin 0x004E60B2 the work tables: 0x00586B08 (type 0xCF), 0x0058FE48 (others); entries of 16
     * bytes: +0 stage, +4 resource, +8 the work record (+0x0C amount, +0x12 done flag) */
    return type == 0xCF ? G_WORK_SLOTS_CF : G_WORK_SLOTS_D0;
}

static void info_resource_works(game_state *st, int32_t resource, int32_t *out, int32_t b)
{
    /* @bin 0x004E6070 the number of the building's work entries of a resource that are done (word
     * +0x12 of the entry's record not 0); the type is read again at each entry */
    uint32_t rec = BUILDING_ADDR(b) + BLD_TYPE;
    *out = 0;
    for (int32_t i = 0;; i++) {
        int16_t type = S16(st, rec);
        if (i >= monument_tables_size(type)) {
            break;
        }
        uint32_t e = monument_table(type) + 16u * (uint32_t) i;
        uint32_t work = U32(st, e + 8);
        if (P16(st, work + 0x12) != 0 && S32(st, e + 4) == resource) {
            (*out)++;
        }
    }
}

/* ---- the pyramids' helpers ---- */


static uint32_t player_goods(game_state *st, uint32_t base, int32_t good)
{
    /* @bin 0x004F5779 the player's i32 tables at base + 4 * (good + 0x1276 * [0x00EAAC28]) */
    return base + 4u * ((uint32_t) good + 0x1276u * U32(st, CURRENT_PLAYER_ADDR));
}

static int32_t goods_check(game_state *st, int32_t good, int32_t amount)
{
    /* @bin 0x004F5770 1 when amount exceeds the player's stock 0x00EA6160[good], or amount > 0 and
     * the flag 0x00EA42F8[good] is set */
    if (amount > S32(st, player_goods(st, CITY_BASE + CITY_STORAGE_STOCK, good))) {
        return 1;
    }
    if (amount <= 0) {
        return 0;
    }
    return S32(st, player_goods(st, CITY_BASE + CITY_STOCKPILE, good)) != 0;
}

static int32_t slot_has_good(game_state *st, int32_t slot, int32_t good)
{
    /* @bin 0x004F56A0 one of the slot's three goods (+6, +8, +0xA, movsx) is good */
    uint32_t r = G_MONUMENT_MODELS + ((uint32_t) slot << 4);
    if (S16(st, r + 6) == good) {
        return 1;
    }
    if (S16(st, r + 8) == good) {
        return 1;
    }
    return S16(st, r + 0xA) == good;
}

static int32_t slot_needs(game_state *st, int32_t kind, int32_t slot)
{
    /* @bin 0x004F5580 by kind (byte table 0x004F565C from 0xB1, jump table 0x004F5648): 0xB1 good
     * 0x14, 0xB2 good 0xC, 0xB3 one of 0x18 0x19 0x1A 0x1E 0x22 0x23, 0xE7 0x21 or 0x0B */
    switch (kind) {
    case 0xB1:
        return slot_has_good(st, slot, 0x14);
    case 0xB2:
        return slot_has_good(st, slot, 0xC);
    case 0xB3: {
        static const int32_t goods[] = {0x18, 0x19, 0x1A, 0x1E, 0x22, 0x23};
        for (unsigned i = 0; i < sizeof(goods) / sizeof(goods[0]); i++) {
            if (slot_has_good(st, slot, goods[i])) {
                return 1;
            }
        }
        return 0;
    }
    case 0xE7:
        return slot_has_good(st, slot, 0x21) || slot_has_good(st, slot, 0xB);
    default:
        return 0;
    }
}

static int32_t monument_wants(game_state *st, int32_t b, int32_t good)
{
    /* @bin 0x004F5BD0 whether the monument b (its main part) still wants good: by type, from its
     * status (on the stack: 0x004F1310 pyramids, 0x00542AD0 type 0xD5, 0x004DF040 type 0xDE, ported
     * in core and called directly as their output is this function's frame) */
    int32_t out[0x36];
    memset(out, 0, sizeof(out));
    int16_t type = S16(st, BUILDING_ADDR(b) + BLD_TYPE);
    if (type == 0xB7) {
        pyramid_status(st, b, out);
        int32_t mode = S32(st, G_PYRAMID_KIND);
        int32_t a = out[8], c = out[9]; /* +0x20, +0x24 */
        switch (mode) {
        case 1:
        case 4:
            /* @bin 0x004F5C8B */
            if (good == 0x19) {
                return c > 0;
            }
            return good == 0x18 && a > 0;
        case 2:
            if (good == 0x19) {
                return c > 0;
            }
            return good == 0xC && a > 0;
        case 3:
            return good == 0x18 && (a > 0 || c > 0);
        case 5:
            return good == 0xC && (a > 0 || c > 0);
        default:
            return 0;
        }
    }
    if (type == 0xD5) {
        if (good != 0x1E) {
            return 0;
        }
        monument_d5_status(st, b, out);
        return out[11] != 0; /* @bin 0x004F5C50 +0x2C */
    }
    if (type == 0xDE) {
        if (good != 0x1E) {
            return 0;
        }
        monument_de_status(st, b, out);
        return out[7] != 0; /* @bin 0x004F5C1A +0x1C */
    }
    return 0;
}

static int32_t slot_goods_wanted(game_state *st, int32_t slot, int32_t main, int32_t need, int32_t need19)
{
    /* @bin 0x004F56D0 (the slot, the status record 0x00EADC48 passed by value, the main part): one of
     * the slot's goods (not 0 or 0x14) the monument wants (0x004F5BD0) and short (0x004F5770 with the
     * record's +0x68, or +0x6C for good 0x19) */
    uint32_t r = G_MONUMENT_MODELS + ((uint32_t) slot << 4);
    for (int k = 0; k < 3; k++) {
        int32_t good = S16(st, r + 6 + 2u * (uint32_t) k);
        if (good == 0 || good == 0x14) {
            continue;
        }
        if (!monument_wants(st, main, good)) {
            continue;
        }
        if (good != 0x19) {
            if (goods_check(st, good, need)) {
                return 1;
            }
        } else if (goods_check(st, 0x19, need19)) {
            return 1;
        }
    }
    return 0;
}

static int32_t pyramid_parts_ready(game_state *st, int32_t b)
{
    /* @bin 0x004F5D10 (not in pyramid kind 5) one of the parts of the building's kind (byte +0x54;
     * count 0x00570070[kind], offsets 0x005F3878 + 0x14 * (0x30 * kind + i)) is ready (0x004EF100) */
    if (S32(st, G_PYRAMID_KIND) == 5) {
        return 0;
    }
    uint32_t k = BUILDING_ADDR(b) + BLD_MONUMENT_KIND;
    uint32_t kind = U8(st, k);
    if (S32(st, G_PYRAMID_ENTRY_COUNT + 4u * kind) <= 0) {
        return 0;
    }
    for (int32_t i = 0;; ) {
        uint32_t off = (uint32_t) U8(st, k) * 0x30u + (uint32_t) i;
        int32_t part = (int32_t) (U32(st, G_PYRAMID_ENTRY_PART + off * 0x14u) + (uint32_t) b);
        if (GAME_CALL(st, FN_PYRAMID_PART_READY, part) != 0) {
            return 1;
        }
        i++;
        if (i >= S32(st, G_PYRAMID_ENTRY_COUNT + 4u * kind)) {
            return 0;
        }
    }
}

/* ---- the panels ---- */

static void info_monument_cf(game_state *st)
{
    /* @bin 0x00512140 types 0xCF / 0xD0 (wavs\sphinx.wav at 0x005F7EA4): the works of granite (0x51) and
     * sandstone (0x4F) counted (0x004E6070), whether a delivery is on its way (0x004E60F0) and
     * whether one is still needed (0x004E6150); the texts of group 0xB2 */
    int32_t x0 = px(st) + 0x20;
    int32_t y0 = py(st) + 0x38;
    S32(st, G_INFO_HELP) = 0x174;
    sound_once(st, G_WAV_SPHINX);
    window_panel(st);
    title(st, 0xB2, 0xC);
    int32_t n4f = 0, n51 = 0;
    info_resource_works(st, 0x4F, &n4f, S32(st, G_INFO_BUILDING));
    info_resource_works(st, 0x51, &n51, S32(st, G_INFO_BUILDING));
    uint8_t moving4f = (uint8_t) GAME_CALL(st, FN_GOODS_ON_WAY_TO, 0x4F, S32(st, G_INFO_BUILDING));
    uint8_t moving51 = (uint8_t) GAME_CALL(st, FN_GOODS_ON_WAY_TO, 0x51, S32(st, G_INFO_BUILDING));
    uint8_t need51 = (uint8_t) GAME_CALL(st, FN_GOODS_NEEDED_STAGE, 0x51, S32(st, G_INFO_BUILDING));
    uint8_t need4f = (uint8_t) GAME_CALL(st, FN_GOODS_NEEDED_STAGE, 0x4F, S32(st, G_INFO_BUILDING));
    if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB3u) == 0) {
        /* @bin 0x0051224C */
        group_lines(st, 0xB2, 0xE, x0, y0, width15(pw(st)), 0);
        return;
    }
    if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB1u) == 0) {
        group_lines(st, 0xB2, 0x10, x0, y0, width15(pw(st)), 0);
        return;
    }
    if (n51 == 0 && moving51 == 0 && need51 != 0) {
        group_lines(st, 0xB2, 0x12, x0, y0, width15(pw(st)), 0);
        return;
    }
    if (n4f == 0 && moving4f == 0 && need4f != 0) {
        group_lines(st, 0xB2, 0x14, x0, y0, width15(pw(st)), 0);
        return;
    }
    if (U8(st, BUILDING_ADDR(S32(st, G_INFO_BUILDING)) + BLD_U55) >= 0x64) {
        /* @bin 0x00512384 byte +0x55 >= 100: done */
        group_text(st, 0xB2, 0x2C, x0, y0);
        return;
    }
    /* @bin 0x00512392 2: granite on its way, 1: sandstone, 0: none; text 0x2A for 1, else 0x2B */
    int32_t k = (n51 != 0 || moving51 != 0) ? 2 : (n4f != 0 || moving4f != 0) ? 1 : 0;
    int32_t id = k == 1 ? 0x2A : 0x2B;
    S32(st, G_TEXT_Y) = 0;
    group_lines(st, 0xB2, id, x0, y0, width15(pw(st)), 0);
    int32_t w = blocks(pw(st)) - 0x10;
    int32_t y = y0 + S32(st, G_TEXT_Y);
    S32(st, G_TEXT_Y) = 0;
    expanded_lines(st, 0xB2, 0x3D, x0, y, w);
}

static void sphinx_works(game_state *st, int32_t resource, int32_t *out)
{
    /* @bin 0x004204D0 the sphinx's work entries of a resource that are done: the table 0x005C274C
     * (22 entries of 16 bytes: +0 resource, +4 the work record, +0x12 done flag) */
    *out = 0;
    for (uint32_t e = G_WORK_SLOTS_D2 + 4u; e < G_BUILD_MENU_RECTS + 4u; e += 0x10) {
        uint32_t work = U32(st, e + 4);
        if (P16(st, work + 0x12) != 0 && S32(st, e) == resource) {
            (*out)++;
        }
    }
}

static int city_stage_ok(game_state *st, int32_t divisor)
{
    if (divisor == 0) {
        /* idiv by 0: the original faults */
        state_unported(st, FN_INFO_SPHINX_PROGRESS);
        return 0;
    }
    return 1;
}

static void sphinx_progress(game_state *st, int32_t *pct4f, int32_t *n4f, int32_t *unused, int32_t *done4f,
                            int32_t *n51, int32_t *done51)
{
    /* @bin 0x00420600 the sphinx's entries of the table 0x005C274C by resource (0x4F sandstone, the
     * others counted with 0x51), those of a stage (+0 - 4) below the city's stage counted done;
     * *pct4f := done4f * 100 / n4f, then *n4f := done51 * 100 / n51 (idiv) */
    (void) unused;
    *n51 = 0;
    *n4f = 0;
    *done51 = 0;
    *done4f = 0;
    for (uint32_t e = G_WORK_SLOTS_D2 + 4u; e < G_BUILD_MENU_RECTS + 4u; e += 0x10) {
        if (S32(st, e) == 0x4F) {
            (*n4f)++;
        } else {
            (*n51)++;
        }
        if (S32(st, e - 4) < city_stage(st)) {
            if (S32(st, e) == 0x4F) {
                (*done4f)++;
            } else {
                (*done51)++;
            }
        }
    }
    if (!city_stage_ok(st, *n4f)) {
        return;
    }
    *pct4f = (int32_t) ((uint32_t) *done4f * 100u) / *n4f;
    int32_t d = *n51;
    if (!city_stage_ok(st, d)) {
        return;
    }
    *n4f = (int32_t) ((uint32_t) *done51 * 100u) / d;
}

static void info_sphinx(game_state *st)
{
    /* @bin 0x00511C80 the sphinx (type 0xD2, wavs\sphinx.wav): the stage's text (group 0xB2), then the progress of the
     * sandstone and granite works in percent */
    S32(st, G_INFO_HELP) = 0x16A;
    sound_once(st, G_WAV_SPHINX);
    window_panel(st);
    title(st, 0xB2, 0xC);
    int32_t x0 = px(st) + 0x20;
    int32_t y0 = py(st) + 0x38;
    int32_t pct4f = 0, pct51 = 0, unused = 0, done4f = 0, n51 = 0, done51 = 0;
    int32_t cnt4f = 0, cnt51 = 0;
    sphinx_works(st, 0x4F, &cnt4f);
    sphinx_works(st, 0x51, &cnt51);
    uint8_t moving4f = (uint8_t) GAME_CALL(st, FN_GOODS_ON_WAY_D2, 0x4F);
    uint8_t moving51 = (uint8_t) GAME_CALL(st, FN_GOODS_ON_WAY_D2, 0x51);
    uint32_t b = BUILDING_ADDR(S32(st, G_INFO_BUILDING));
    /* @bin 0x00511D68 the building's tile in the 0x00EE0C50 byte grid (x +0x26, y +0x28): negative,
     * or the building's word +0x20 zero, gives text 0x95 */
    int32_t tile = (int32_t) (U32(st, G_MAP_START) + (uint32_t) S16(st, b + BLD_ROAD_ACCESS_X) + 228u * (uint32_t) S16(st, b + BLD_ROAD_ACCESS_Y));
    int blocked = (int8_t) U8(st, G_GRID_ROUTING + (uint32_t) tile) < 0 || S16(st, b + BLD_DISTANCE) == 0;
    S32(st, G_TEXT_Y) = 0;
    if (city_stage(st) < 0x10) {
        int32_t id;
        if (blocked) {
            id = 0x95;
        } else if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB3u) == 0) {
            id = 0xE;
        } else if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB1u) == 0) {
            id = 0x10;
        } else if (cnt51 == 0 && moving51 == 0 && (uint8_t) GAME_CALL(st, FN_GOODS_NEEDED_PLAYER, 0x51) != 0) {
            id = 0x12;
        } else if (cnt4f == 0 && moving4f == 0 && (uint8_t) GAME_CALL(st, FN_GOODS_NEEDED_PLAYER, 0x4F) != 0) {
            id = 0x14;
        } else if (city_stage(st) < 7) {
            id = 0x2D;
        } else if (city_stage(st) < 0xD) {
            id = 0x2E;
        } else {
            id = 0x2F;
        }
        group_lines(st, 0xB2, id, x0, y0, width15(pw(st)), 0);
    }
    int32_t y1 = y0 + S32(st, G_TEXT_Y) + 0x18;
    if (city_stage(st) >= 0x10) {
        group_text(st, 0xB2, 0x30, x0, y1);
        return;
    }
    S32(st, G_TEXT_Y) = 0;
    sphinx_progress(st, &pct4f, &pct51, &unused, &done4f, &n51, &done51);
    int32_t y = S32(st, G_TEXT_Y) + y1;
    S32(st, G_TEXT_X) = 0;
    group_lines(st, 0xB2, 0x31, x0, y, width15(pw(st)), 0);
    number(st, pct4f, S32(st, G_TEXT_X) + x0, y);
    group_text(st, 0xB2, 0, S32(st, G_TEXT_X) + x0, y);
    y = S32(st, G_TEXT_Y) + y1;
    S32(st, G_TEXT_X) = 0;
    group_lines(st, 0xB2, 0x32, x0, y, width15(pw(st)), 0);
    number(st, pct51, S32(st, G_TEXT_X) + x0, y);
    group_text(st, 0xB2, 0, S32(st, G_TEXT_X) + x0, y);
}

static int building_cut_off(game_state *st, int call_check)
{
    /* @bin 0x005133B0 the building's tile in the 0x00EE0C50 byte grid (x +0x26, y +0x28) negative,
     * or its word +0x20 zero (after 0x00457AF0 when call_check): the "cut off" text 0x95 */
    uint32_t b = BUILDING_ADDR(S32(st, G_INFO_BUILDING));
    int32_t tile = (int32_t) (U32(st, G_MAP_START) + (uint32_t) S16(st, b + BLD_ROAD_ACCESS_X) + 228u * (uint32_t) S16(st, b + BLD_ROAD_ACCESS_Y));
    if ((int8_t) U8(st, G_GRID_ROUTING + (uint32_t) tile) < 0) {
        return 1;
    }
    if (S16(st, b + BLD_DISTANCE) != 0) {
        return 0;
    }
    if (!call_check) {
        return 1;
    }
    GAME_CALL0(st, FN_DAY_ROAD_ACCESS_457AF0);
    return S16(st, BUILDING_ADDR(S32(st, G_INFO_BUILDING)) + BLD_DISTANCE) == 0;
}

static uint32_t main_building(game_state *st)
{
    /* @bin 0x00513334 the monument's main building: the building's id minus its word +0x12 */
    int32_t b = S32(st, G_INFO_BUILDING);
    return BUILDING_ADDR(b - S16(st, BUILDING_ADDR(b) + BLD_PART));
}

static void info_monument_e6(game_state *st)
{
    /* @bin 0x005132A0 type 0xE6 (status record 0x00EADBC8, computed by the open with 0x00401540 on
     * 0x006DE650): no sound call (the flag is only cleared); granite / sandstone still needed
     * (0x00420CC0 on the main building's +0x54), the texts of group 0xB2, then while the progress
     * 0x00EADBD4 is below 100 the expanded text 0x3D */
    if (S32(st, G_INFO_SOUND) != 0) {
        S32(st, G_INFO_SOUND) = 0;
    }
    S32(st, G_INFO_HELP) = 0x1EC;
    window_panel(st);
    title(st, 0xB2, 0xC);
    int32_t x0 = px(st) + 0x20;
    int32_t y0 = py(st) + 0x38;
    int32_t v = S32(st, main_building(st) + BLD_MONUMENT_KIND);
    uint8_t need4f = (uint8_t) GAME_CALL(st, FN_GOODS_NEEDED_5A8140, 0x4F, v);
    uint8_t need51 = (uint8_t) GAME_CALL(st, FN_GOODS_NEEDED_5A8140, 0x51, v);
    GAME_CALLT(st, G_MONUMENT_E6, FN_MONUMENT_PROGRESS_401540, S32(st, G_INFO_BUILDING), 0);
    S32(st, G_TEXT_Y) = 0;
    int32_t id;
    if (building_cut_off(st, 1)) {
        id = 0x95;
    } else if (need51 && S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB3u) == 0) {
        id = 0xE;
    } else if (need51 && S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xB3u) == 0 && S32(st, G_MONUMENT_STATUS_E3 + 0x4u) == 0) {
        id = 0x12;
    } else if (need4f && S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB1u) == 0) {
        id = 0x10;
    } else if (need4f && S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xB1u) == 0 && S32(st, G_MONUMENT_STATUS_E3 + 0x10u) == 0) {
        id = 0x14;
    } else if (S32(st, G_MONUMENT_STATUS_E6) < 0x38) {
        id = 0x8D;
    } else {
        id = 0x8E;
    }
    group_lines(st, 0xB2, id, x0, y0, width15(pw(st)), 0);
    int32_t y = y0 + S32(st, G_TEXT_Y) + 0x18;
    int32_t w = blocks(pw(st)) - 0x10;
    if (S32(st, G_MONUMENT_STATUS_E6 + 0xCu) < 0x64) {
        S32(st, G_TEXT_Y) = 0;
        expanded_lines(st, 0xB2, 0x3D, x0, y, w);
    }
}

static void info_sun_temple(game_state *st)
{
    /* @bin 0x005124A0 the sun temple (type 0xD5, wavs\suntemple.wav; status record 0x00EADC10 computed
     * by the open with 0x00542AD0): the text of its phase [0x00EADC10] (1..4, jump table 0x00512BCC;
     * other values: nothing more), then the expanded texts 0x3D and 0x44 while the progress
     * 0x00EADC40 is below 100, else 0x47 */
    S32(st, G_INFO_HELP) = 0x45;
    sound_once(st, G_WAV_SUNTEMPLE);
    window_panel(st);
    title(st, 0xB2, 0xC);
    int32_t x0 = px(st) + 0x20;
    int32_t y0 = py(st) + 0x38;
    int32_t b = S32(st, G_INFO_BUILDING);
    uint8_t need4f = (uint8_t) GAME_CALL(st, FN_GOODS_NEEDED_596080, 0x4F, S32(st, BUILDING_ADDR(b) + BLD_MONUMENT_STAGE));
    b = S32(st, G_INFO_BUILDING);
    uint8_t need51 = (uint8_t) GAME_CALL(st, FN_GOODS_NEEDED_596080, 0x51, S32(st, BUILDING_ADDR(b) + BLD_MONUMENT_STAGE));
    /* @bin 0x00512578 up to three calls of 0x00475B50, until the player's word 0x00EA5FAC[i] matches */
    for (int32_t i = 0; i < 3; i++) {
        int32_t r = GAME_CALL(st, FN_BUILDING_MAIN_PART, S32(st, G_INFO_BUILDING));
        if (S32(st, CITY_BASE + CITY_MONUMENTS + 4u * ((uint32_t) i + 0x1276u * U32(st, CURRENT_PLAYER_ADDR))) == r) {
            break;
        }
    }
    uint32_t flags = U32(st, BUILDING_ADDR(S32(st, G_INFO_BUILDING)) + BLD_U64);
    uint8_t flag2 = (uint8_t) ((flags >> 2) & 1);
    uint8_t flag3 = (uint8_t) ((flags >> 3) & 1);
    uint8_t flag4 = (uint8_t) ((flags >> 4) & 1);
    /* @bin 0x005125C6 0x004F5770(0x1E, ([0x00EADC30] * 5 + 5) * 80) */
    uint8_t ready = (uint8_t) goods_check(st, 0x1E, (int32_t) (((uint32_t) S32(st, G_MONUMENT_STATUS_D5 + 0x20u) * 5u + 5u) * 80u));
    S32(st, G_TEXT_Y) = 0;
    int32_t id;
    int32_t y = y0;
    switch (S32(st, G_MONUMENT_STATUS_D5)) {
    case 1:
        if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xC7u) == 0) {
            id = 0xD;
        } else if (S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xC7u) == 0 && S32(st, G_MONUMENT_STATUS_D5 + 0x24u) == 0) {
            int32_t d = S32(st, G_FLOOD_STATE);
            id = (d == 1 || d == 2 || d == 3) ? 0x33 : 0x11;
        } else {
            id = 0x35;
        }
        break;
    case 2:
        if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB1u) == 0) {
            id = 0x10;
        } else if (S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xB1u) == 0 && S32(st, G_MONUMENT_STATUS_D5 + 0x28u) == 0) {
            id = 0x14;
        } else {
            id = 0x36;
        }
        break;
    case 3:
        id = 0x37;
        if (need51) {
            if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB3u) == 0) {
                id = 0xE;
            } else if (S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xB3u) == 0 && S32(st, G_MONUMENT_STATUS_D5 + 0x1Cu) == 0) {
                id = 0x12;
            }
        } else if (need4f) {
            if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB1u) == 0) {
                id = 0x10;
            } else if (S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xB1u) == 0 && S32(st, G_MONUMENT_STATUS_D5 + 0x28u) == 0) {
                id = 0x14;
            }
        }
        break;
    case 4:
        if (GAME_CALL(st, FN_MONUMENT_D5_STATUS, S32(st, G_INFO_BUILDING), 0) >= 0x64) {
            id = 0x39;
        } else if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xC7u) == 0) {
            id = 0xD;
        } else if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB3u) == 0) {
            id = 0xE;
        } else if (S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xC7u) == 0 && S32(st, G_MONUMENT_STATUS_D5 + 0x24u) == 0 && flag4 == 0) {
            id = 0x11;
        } else {
            int32_t made = S32(st, G_MONUMENT_STATUS_D5 + 0x1Cu), want = S32(st, G_MONUMENT_STATUS_D5 + 0x20u);
            if (made == want && ready == 0) {
                id = 0x18;
            } else if (made == 0 && (flag2 == 0 || flag3 == 0)) {
                id = 0x12;
            } else if (want != 0 && ready != 0 && S32(st, G_MONUMENT_STATUS_D5 + 0x2Cu) != 0) {
                /* @bin 0x005129F3 the player's dword 0x00EA4370 */
                id = S32(st, CITY_BASE + CITY_STOCKPILE + 4u * 0x1Eu + CITY_REC_SIZE * U32(st, CURRENT_PLAYER_ADDR)) != 0 ? 0x64 : 0x18;
                y = y0 + S32(st, G_TEXT_Y);
            } else {
                id = 0x38;
            }
        }
        break;
    default:
        return;
    }
    group_lines(st, 0xB2, id, x0, y, width15(pw(st)), 0);
    y = y0 + S32(st, G_TEXT_Y);
    int32_t w = blocks(pw(st)) - 0x10;
    int32_t progress = S32(st, G_MONUMENT_STATUS_D5 + 0x30u);
    S32(st, G_COURSE_MAIN) = 0x1E;
    S32(st, G_TEXT_Y) = 0;
    if (progress >= 0x64) {
        expanded_lines(st, 0xB2, 0x47, x0, y, w);
        return;
    }
    expanded_lines(st, 0xB2, 0x3D, x0, y, w);
    y += S32(st, G_TEXT_Y);
    if (S32(st, G_MONUMENT_STATUS_D5 + 0x2Cu) != 0) {
        S32(st, G_TEXT_Y) = 0;
        expanded_lines(st, 0xB2, 0x44, x0, y, w);
    }
}

static void pyramid_line(game_state *st, int32_t id, int32_t x, int32_t y)
{
    group_lines(st, 0xB2, id, x, y, width15(pw(st)), 0);
}

static void info_pyramid(game_state *st)
{
    /* @bin 0x00511040 the pyramids and mastabas (type 0xB7, wavs\pyramid.wav; status record 0x00EADC48
     * computed by the open with 0x004F1310): by the scenario's pyramid kind [0x005E4EF4] (jump table
     * 0x00511BF8) the help id, the goods 0x00EADB34 / 0x00EADB30 and the texts of the missing goods;
     * the text of the phase [0x00EADC48] (1..3), then the expanded texts of the progress */
    int32_t mode = S32(st, G_PYRAMID_KIND);
    S32(st, G_COURSE_SECOND) = -1;
    int32_t id_a, id_b; /* the texts of the first good: plain, when the flag 0x00EA42F8[good] is set */
    switch (mode) {
    case 5:
        S32(st, G_INFO_HELP) = 0x173;
        S32(st, G_COURSE_MAIN) = 0xC;
        id_a = 0x1B;
        id_b = 0x67;
        break;
    case 3:
        S32(st, G_INFO_HELP) = 0x177;
        S32(st, G_COURSE_MAIN) = 0x18;
        id_a = 0x16;
        id_b = 0x62;
        break;
    case 2:
        S32(st, G_INFO_HELP) = 0x37;
        S32(st, G_COURSE_MAIN) = 0xC;
        S32(st, G_COURSE_SECOND) = 0x19;
        id_b = 0x67;
        id_a = 0x1B;
        break;
    default:
        S32(st, G_INFO_HELP) = mode == 1 ? 0x36 : 0x26;
        S32(st, G_COURSE_MAIN) = 0x18;
        id_a = 0x16;
        id_b = 0x62;
        S32(st, G_COURSE_SECOND) = 0x19;
        break;
    }
    sound_once(st, G_WAV_PYRAMID);
    window_panel(st);
    /* @bin 0x00511167 the slot: the first of the player's three monument slots 0x00EA5FAC holding the
     * main part (0x00475B50, called again for each slot), its kind from the i16 table 0x00784DE0 */
    int32_t slot = 0;
    for (int32_t i = 0; i < 3; i++) {
        int32_t r = GAME_CALL(st, FN_BUILDING_MAIN_PART, S32(st, G_INFO_BUILDING));
        if (S32(st, CITY_BASE + CITY_MONUMENTS + 4u * ((uint32_t) i + 0x1276u * U32(st, CURRENT_PLAYER_ADDR))) == r) {
            slot = S16(st, G_MONUMENT_SLOTS + 2u * (uint32_t) i);
            break;
        }
    }
    uint8_t cut = (uint8_t) building_cut_off(st, 0);
    title(st, 0xB2, 0xC);
    mode = S32(st, G_PYRAMID_KIND);
    int32_t x0 = px(st) + 0x20;
    int32_t y = py(st) + 0x38;
    /* @bin 0x00511240 jump table 0x00511C0C: f12 (the slot's goods of kind 0xB2 count), f13 (of kind
     * 0xB3) */
    uint8_t f12, f13;
    switch (mode) {
    case 1: case 4: f13 = 1; f12 = 0; break;
    case 2: f13 = 1; f12 = 1; break;
    case 3: f13 = 0; f12 = 0; break;
    default: f13 = 0; f12 = 1; break;
    }
    if (cut && S32(st, G_PYRAMID_STATUS + 0x34u) < 0x64) {
        y += 0x18;
        S32(st, G_TEXT_X) = 0;
        S32(st, G_TEXT_Y) = 0;
        pyramid_line(st, 0x34, x0, y);
    } else {
        int32_t phase = S32(st, G_PYRAMID_STATUS);
        S32(st, G_TEXT_Y) = 0;
        if (phase == 3) {
            if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB3u) == 0 && (!f12 || f13) && slot_needs(st, 0xB3, slot)) {
                pyramid_line(st, 0xE, x0, y);
            } else if (S32(st, G_PYRAMID_STATUS + 0x48u) == 0 && slot_needs(st, 0xB3, slot)) {
                pyramid_line(st, 0x12, x0, y);
            } else {
                S32(st, G_TEXT_X) = 0;
                group_text(st, 0xB2, 1, x0, y);
                y += 0x18;
                S32(st, G_TEXT_X) = 0;
                group_text(st, 0xB2, 2, x0, y);
                number(st, S32(st, G_PYRAMID_STATUS + 0x30u), S32(st, G_TEXT_X) + x0, y);
                group_text(st, 0xB2, 0, S32(st, G_TEXT_X) + x0, y);
            }
        } else if (phase == 2) {
            S32(st, G_TEXT_Y) = 0;
            int32_t main = GAME_CALL(st, FN_BUILDING_MAIN_PART, S32(st, G_INFO_BUILDING));
            /* @bin 0x0051145D the record copied on the stack: its +0x68 and +0x6C as they are now */
            cut = (uint8_t) slot_goods_wanted(st, slot, main, S32(st, G_PYRAMID_STATUS + 0x68u), S32(st, G_PYRAMID_STATUS + 0x6Cu));
            if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xC7u) == 0) {
                pyramid_line(st, 0xD, x0, y);
            } else if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB3u) == 0 && (!f12 || f13) && slot_needs(st, 0xB3, slot)) {
                pyramid_line(st, 0xE, x0, y);
            } else if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB2u) == 0 && f12 && slot_needs(st, 0xB2, slot)) {
                pyramid_line(st, 0xF, x0, y);
            } else if (S32(st, G_PYRAMID_STATUS + 0x60u) == S32(st, G_PYRAMID_STATUS + 0x5Cu) && cut == 0) {
                pyramid_line(st, 0x11, x0, y);
            } else if (S32(st, G_PYRAMID_STATUS + 0x48u) == 0
                       && ((f12 == 0 && S32(st, G_PYRAMID_STATUS + 0x18u) != 0) || (f13 && S32(st, G_PYRAMID_STATUS + 0x1Cu) != 0))
                       && slot_needs(st, 0xB3, slot)) {
                pyramid_line(st, 0x12, x0, y);
            } else if (S32(st, G_PYRAMID_STATUS + 0x4Cu) == 0 && f12 && S32(st, G_PYRAMID_STATUS + 0x18u) != 0 && slot_needs(st, 0xB2, slot)) {
                pyramid_line(st, 0x13, x0, y);
            } else if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB1u) == 0
                       && pyramid_parts_ready(st, GAME_CALL(st, FN_BUILDING_MAIN_PART, S32(st, G_INFO_BUILDING)))
                       && slot_needs(st, 0xB1, slot)) {
                pyramid_line(st, 0x10, x0, y);
            } else if (S32(st, G_PYRAMID_STATUS + 0x58u) == 0
                       && pyramid_parts_ready(st, GAME_CALL(st, FN_BUILDING_MAIN_PART, S32(st, G_INFO_BUILDING)))
                       && slot_needs(st, 0xB1, slot)) {
                pyramid_line(st, 0x15, x0, y);
            } else if (S32(st, G_PYRAMID_STATUS + 0x5Cu) != 0) {
                /* @bin 0x0051171A the first good 0x00EADB34, then the second 0x00EADB30 (-1: none) */
                int32_t good = S32(st, G_COURSE_MAIN);
                cut = (uint8_t) goods_check(st, good, S32(st, G_PYRAMID_STATUS + 0x68u));
                good = S32(st, G_COURSE_MAIN);
                main = GAME_CALL(st, FN_BUILDING_MAIN_PART, S32(st, G_INFO_BUILDING));
                if (monument_wants(st, main, good) && cut) {
                    good = S32(st, G_COURSE_MAIN);
                    int32_t id = S32(st, player_goods(st, CITY_BASE + CITY_STOCKPILE, good)) != 0 ? id_b : id_a;
                    pyramid_line(st, id, x0, S32(st, G_TEXT_Y) + y);
                }
                good = S32(st, G_COURSE_SECOND);
                if (good != -1) {
                    cut = (uint8_t) goods_check(st, good, S32(st, G_PYRAMID_STATUS + 0x6Cu));
                    good = S32(st, G_COURSE_SECOND);
                    main = GAME_CALL(st, FN_BUILDING_MAIN_PART, S32(st, G_INFO_BUILDING));
                    if (monument_wants(st, main, good) && cut) {
                        good = S32(st, G_COURSE_SECOND);
                        int32_t id = S32(st, player_goods(st, CITY_BASE + CITY_STOCKPILE, good)) != 0 ? 0x63 : 0x17;
                        pyramid_line(st, id, x0, S32(st, G_TEXT_Y) + y);
                    }
                }
            }
        } else if (phase == 1) {
            if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xC7u) == 0) {
                pyramid_line(st, 0xD, x0, y);
            } else if (S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xC7u) == 0 && S32(st, G_PYRAMID_STATUS + 0x64u) == 0) {
                int32_t d = S32(st, G_FLOOD_STATE);
                pyramid_line(st, (d == 1 || d == 2 || d == 3) ? 0x33 : 0x11, x0, y);
            } else {
                /* @bin 0x0051192E the levels built (+0x3C, one more when equal to +0x38), at most 5 in
                 * kind 5 (from 6 to 10); texts by jump table 0x00511C1C */
                int32_t n = S32(st, G_PYRAMID_STATUS + 0x3Cu);
                if (n == S32(st, G_PYRAMID_STATUS + 0x38u)) {
                    n++;
                }
                if (mode == 5 && n > 5 && n <= 10) {
                    n = 5;
                }
                int32_t id;
                switch (n) {
                case 2: case 3: case 4: case 5: id = mode == 5 ? 4 : 3; break;
                case 6: id = 5; break;
                case 7: id = 6; break;
                case 8: id = 7; break;
                case 9: id = 8; break;
                case 10: id = 9; break;
                case 11: id = 0xA; break;
                default: id = 0xB; break;
                }
                pyramid_line(st, id, x0, y);
            }
        }
    }
    y += S32(st, G_TEXT_Y) + 0x18;
    int32_t w = blocks(pw(st)) - 0x10;
    int32_t progress = S32(st, G_PYRAMID_STATUS + 0x34u);
    S32(st, G_TEXT_Y) = 0;
    if (progress < 0x64) {
        expanded_lines(st, 0xB2, 0x3D, x0, S32(st, G_TEXT_Y) + y, w);
    }
    if (S32(st, G_PYRAMID_STATUS + 0x18u) != 0 || S32(st, G_PYRAMID_STATUS + 0x1Cu) != 0) {
        /* @bin 0x00511A46 two texts joined (strcat): the workers' kind [+4] (0x3E..0x40) in a 1000-byte
         * buffer, then the goods (0x41..0x43) */
        char both[0x3E8 + TEXT_BUF_SIZE + 1];
        char part[TEXT_BUF_SIZE + 1];
        memset(both, 0, sizeof(both));
        memset(part, 0, sizeof(part));
        int32_t kind = S32(st, G_PYRAMID_STATUS + 0x4u);
        GAME_CALL(st, FN_TEXT_SEEK, 0xB2, kind == 0 ? 0x3E : kind == 1 ? 0x3F : 0x40);
        expand(st, U32(st, G_TEXT_CURSOR), both, 0x3E8);
        int32_t id = S32(st, G_PYRAMID_STATUS + 0x18u) == 0 ? 0x42 : S32(st, G_PYRAMID_STATUS + 0x1Cu) != 0 ? 0x43 : 0x41;
        GAME_CALL(st, FN_TEXT_SEEK, 0xB2, id);
        expand(st, U32(st, G_TEXT_CURSOR), part, TEXT_BUF_SIZE);
        both[0x3E8] = 0;
        strcat(both, part);
        lines_host(st, both, x0, S32(st, G_TEXT_Y) + y, w, font_text(st), 0);
    }
    int32_t id;
    if (S32(st, G_PYRAMID_STATUS + 0x34u) < 0x64) {
        if (S32(st, G_PYRAMID_STATUS + 0x20u) != 0) {
            id = S32(st, G_PYRAMID_STATUS + 0x24u) != 0 ? 0x46 : 0x44;
        } else if (S32(st, G_PYRAMID_STATUS + 0x24u) != 0) {
            id = 0x45;
        } else {
            return;
        }
    } else {
        id = S32(st, G_PYRAMID_STATUS + 0x2Cu) != 0 ? 0x48 : 0x47;
    }
    expanded_lines(st, 0xB2, id, x0, S32(st, G_TEXT_Y) + y, w);
}

static void info_monument_e1(game_state *st)
{
    /* @bin 0x00512BE0 type 0xE1 (status record 0x00EADBD8, computed by the open with 0x0040D940 on
     * 0x006FD5D8, called again here): no sound call; the goods still needed (0x00420990 on the main
     * building's +0x58), the stage's text (group 0xB2), then the expanded texts of the progress */
    if (S32(st, G_INFO_SOUND) != 0) {
        S32(st, G_INFO_SOUND) = 0;
    }
    S32(st, G_INFO_HELP) = 0x1E1;
    window_panel(st);
    title(st, 0xB2, 0xC);
    int32_t x0 = px(st) + 0x20;
    int32_t y = py(st) + 0x38;
    int32_t v = S32(st, main_building(st) + BLD_MONUMENT_STAGE);
    uint8_t need4f = (uint8_t) GAME_CALL(st, FN_GOODS_NEEDED_59EDE8, 0x4F, v);
    uint8_t need51 = (uint8_t) GAME_CALL(st, FN_GOODS_NEEDED_59EDE8, 0x51, v);
    GAME_CALLT(st, G_MONUMENT_E1, FN_MONUMENT_PROGRESS_40D940, S32(st, G_INFO_BUILDING), 0);
    /* @bin 0x00512CB5 below stage 0x15 good 0x23, else up to 0x13 good 0x1D (never: the second test
     * follows the first), amount [0x00EADBF8] * 400 */
    uint8_t short_ = 0;
    if (v < 0x15) {
        short_ = (uint8_t) goods_check(st, 0x23, (int32_t) ((uint32_t) S32(st, G_MONUMENT_STATUS_E1 + 0x20u) * 400u));
    } else if (v <= 0x13) {
        short_ = (uint8_t) goods_check(st, 0x1D, (int32_t) ((uint32_t) S32(st, G_MONUMENT_STATUS_E1 + 0x20u) * 400u));
    }
    S32(st, G_TEXT_Y) = 0;
    int32_t id = -1;
    int32_t ly = y;
    if (building_cut_off(st, 1)) {
        id = 0x95;
    } else if (S32(st, G_MONUMENT_STATUS_E1 + 0x20u) != 0 && short_ != 0 && S32(st, G_MONUMENT_STATUS_E1 + 0x2Cu) != 0) {
        uint32_t p = CITY_REC_SIZE * U32(st, CURRENT_PLAYER_ADDR);
        if (v < 0x15) {
            id = S32(st, CITY_BASE + CITY_STOCKPILE + 4u * 0x23u + p) != 0 ? 0x65 : 0x1A;
            ly = S32(st, G_TEXT_Y) + y;
        } else if (v < 0x13) {
            id = S32(st, CITY_BASE + CITY_STOCKPILE + 4u * 0x1Du + p) != 0 ? 0x68 : 0x1D;
            ly = S32(st, G_TEXT_Y) + y;
        }
    } else {
        if (need51) {
            if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xC7u) == 0) {
                id = 0xD;
            } else if (S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xC7u) == 0 && S32(st, G_MONUMENT_STATUS_E1 + 0x24u) == 0) {
                id = 0x11;
            } else if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB3u) == 0) {
                id = 0xE;
            } else if (S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xB3u) == 0 && S32(st, G_MONUMENT_STATUS_E3 + 0x4u) == 0) {
                id = 0x12;
            }
        }
        if (id < 0 && need4f) {
            if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB1u) == 0) {
                id = 0x10;
            } else if (S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xB1u) == 0 && S32(st, G_MONUMENT_STATUS_E3 + 0x10u) == 0) {
                id = 0x14;
            }
        }
        if (id < 0) {
            int32_t n = S32(st, G_MONUMENT_STATUS_E1);
            id = n < 3 ? 0x72 : n < 6 ? 0x73 : n < 9 ? 0x74 : n < 0xC ? 0x75 : n < 0x10 ? 0x76 : n < 0x14 ? 0x77
                 : n == 0x14 ? 0x78 : 0x79;
        }
    }
    if (id >= 0) {
        group_lines(st, 0xB2, id, x0, ly, width15(pw(st)), 0);
    }
    y += S32(st, G_TEXT_Y) + 0x18;
    int32_t w = blocks(pw(st)) - 0x10;
    int32_t progress = S32(st, G_MONUMENT_STATUS_E1 + 0x30u);
    S32(st, G_COURSE_MAIN) = 0x23;
    if (progress >= 0x64) {
        return;
    }
    S32(st, G_TEXT_Y) = 0;
    expanded_lines(st, 0xB2, 0x3D, x0, y, w);
    y += S32(st, G_TEXT_Y);
    if (S32(st, G_MONUMENT_STATUS_E1 + 0x2Cu) != 0) {
        S32(st, G_TEXT_Y) = 0;
        expanded_lines(st, 0xB2, 0x44, x0, y, w);
        y += S32(st, G_TEXT_Y);
    }
    if (S32(st, G_MONUMENT_STATUS_E1 + 0x2Cu) > 0x320) {
        S32(st, G_COURSE_MAIN) = 0x1D;
        S32(st, G_TEXT_Y) = 0;
        expanded_lines(st, 0xB2, 0x44, x0, y, w);
    }
}

static void info_monument_e3(game_state *st)
{
    /* @bin 0x005135E0 type 0xE3 (wavs\suntemple.wav; status record 0x00EADBA8, computed by the open
     * with 0x00417050 on 0x006FD368): the goods still needed (0x00420FC0 on the main building's
     * +0x58, the stage), the stage's text (group 0xB2), then the expanded texts of the progress */
    S32(st, G_INFO_HELP) = 0x1E2;
    uint32_t stage_at = main_building(st) + BLD_MONUMENT_STAGE;
    int32_t v = S32(st, stage_at);
    sound_once(st, G_WAV_SUNTEMPLE);
    window_panel(st);
    title(st, 0xB2, 0xC);
    int32_t x0 = px(st) + 0x20;
    int32_t y = py(st) + 0x38;
    uint8_t need4f = (uint8_t) GAME_CALL(st, FN_GOODS_NEEDED_5AD210, 0x4F, S32(st, stage_at));
    uint8_t need51 = (uint8_t) GAME_CALL(st, FN_GOODS_NEEDED_5AD210, 0x51, S32(st, stage_at));
    S32(st, G_TEXT_Y) = 0;
    /* @bin 0x005136C4 below stage 0x12 good 0x23, at 0x12 good 0x1A, amount [0x00EADBB0] * 400 */
    uint8_t short_ = 0;
    if (v < 0x12) {
        short_ = (uint8_t) goods_check(st, 0x23, (int32_t) ((uint32_t) S32(st, G_MONUMENT_STATUS_E3 + 0x8u) * 400u));
    } else if (v < 0x13) {
        short_ = (uint8_t) goods_check(st, 0x1A, (int32_t) ((uint32_t) S32(st, G_MONUMENT_STATUS_E3 + 0x8u) * 400u));
    }
    S32(st, G_TEXT_Y) = 0;
    int32_t id = -1;
    int32_t ly = y;
    if (building_cut_off(st, 1)) {
        id = 0x95;
    } else if (S32(st, G_MONUMENT_STATUS_E3 + 0x8u) != 0 && short_ != 0 && S32(st, G_MONUMENT_STATUS_E3 + 0x14u) != 0) {
        uint32_t p = CITY_REC_SIZE * U32(st, CURRENT_PLAYER_ADDR);
        if (v < 0x12) {
            id = S32(st, CITY_BASE + CITY_STOCKPILE + 4u * 0x23u + p) != 0 ? 0x65 : 0x1A;
            ly = S32(st, G_TEXT_Y) + y;
        } else if (v < 0x13) {
            id = S32(st, CITY_BASE + CITY_STOCKPILE + 4u * 0x1Au + p) != 0 ? 0x66 : 0x19;
            ly = S32(st, G_TEXT_Y) + y;
        }
    } else if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xC7u) == 0) {
        id = 0xD;
    } else if (S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xC7u) == 0 && S32(st, G_MONUMENT_STATUS_E3 + 0xCu) == 0) {
        id = 0x11;
    } else {
        if (need51) {
            if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB3u) == 0) {
                id = 0xE;
            } else if (S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xB3u) == 0 && S32(st, G_MONUMENT_STATUS_E3 + 0x4u) == 0) {
                id = 0x12;
            }
        }
        if (id < 0 && need4f) {
            if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB1u) == 0) {
                id = 0x10;
            } else if (S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xB1u) == 0 && S32(st, G_MONUMENT_STATUS_E3 + 0x10u) == 0) {
                id = 0x14;
            }
        }
        if (id < 0) {
            id = v < 0 ? 0x7A : v == 0 ? 0x7B : v < 6 ? 0x7C : v < 9 ? 0x7D : v < 0xC ? 0x7E : v < 0xE ? 0x7F
                 : v < 0x11 ? 0x80 : v == 0x11 ? 0x81 : v == 0x12 ? 0x82 : 0x83;
        }
    }
    if (id >= 0) {
        group_lines(st, 0xB2, id, x0, ly, width15(pw(st)), 0);
    }
    int32_t w = blocks(pw(st)) - 0x10;
    y += S32(st, G_TEXT_Y) + 0x18;
    int32_t progress = S32(st, G_MONUMENT_STATUS_E3 + 0x18u);
    S32(st, G_COURSE_MAIN) = 0x23;
    if (progress >= 0x64) {
        return;
    }
    S32(st, G_TEXT_Y) = 0;
    expanded_lines(st, 0xB2, 0x3D, x0, y, w);
    y += S32(st, G_TEXT_Y);
    if (S32(st, G_MONUMENT_STATUS_E3 + 0x14u) > 0x320) {
        S32(st, G_TEXT_Y) = 0;
        expanded_lines(st, 0xB2, 0x44, x0, y, w);
        y += S32(st, G_TEXT_Y);
    }
    if (S32(st, G_MONUMENT_STATUS_E3 + 0x14u) != 0) {
        S32(st, G_COURSE_MAIN) = 0x1A;
        S32(st, G_TEXT_Y) = 0;
        expanded_lines(st, 0xB2, 0x44, x0, y, w);
    }
}

static int16_t info_type(game_state *st)
{
    return S16(st, BUILDING_ADDR(S32(st, G_INFO_BUILDING)) + BLD_TYPE);
}

static int tomb_call(game_state *st, uint32_t function, int with_arg, int32_t arg)
{
    /* @bin 0x00513F57 by the shown building's type (read again before each test): 0xE5 on 0x006F9320,
     * 0xEA on 0x006F5648, 0xEC on 0x006E9188, 0xEB on 0x006DE9E0, the first true result wins */
    static const struct { int16_t type; uint32_t self; } objs[] = {
        {0xE5, G_MONUMENT_E5}, {0xEA, G_MONUMENT_EA}, {0xEC, G_MONUMENT_EC}, {0xEB, G_MONUMENT_EB}};
    for (unsigned i = 0; i < sizeof(objs) / sizeof(objs[0]); i++) {
        if (info_type(st) != objs[i].type) {
            continue;
        }
        uint8_t r = with_arg ? (uint8_t) GAME_CALLT(st, objs[i].self, function, arg)
                             : (uint8_t) GAME_CALLT0(st, objs[i].self, function);
        if (r != 0) {
            return 1;
        }
    }
    return 0;
}

static void info_tomb(game_state *st)
{
    /* @bin 0x00513D40 types 0xE5, 0xEA, 0xEB, 0xEC (status record 0x00EADB38, computed by the open):
     * no sound call; the parts of the main building (kinds by its type, jump table 0x005143D4) and
     * their roles (+0x60: 1, 2 or 3), the parts done (8, 0xA count text of [0x00EADB50]), then the
     * text of the state (group 0xB2) and while the progress 0x00EADB4C is below 100 the expanded
     * text 0x3D */
    int32_t b = S32(st, G_INFO_BUILDING);
    int32_t main = b - S16(st, BUILDING_ADDR(b) + BLD_PART);
    uint32_t mrec = BUILDING_ADDR(main);
    S32(st, G_INFO_HELP) = 0x1DE;
    int32_t count = 0;
    switch (S16(st, mrec + BLD_TYPE)) {
    case 0xE5: count = GAME_CALLT0(st, G_MONUMENT_E5, FN_TOMB_KINDS_545F60); break;
    case 0xEA: count = GAME_CALLT0(st, G_MONUMENT_EA, FN_TOMB_KINDS_559460); break;
    case 0xEB: count = GAME_CALLT0(st, G_MONUMENT_EB, FN_TOMB_KINDS_5567B0); break;
    case 0xEC: count = GAME_CALLT0(st, G_MONUMENT_EC, FN_TOMB_KINDS_54FED0); break;
    default: break;
    }
    uint8_t role1 = 0, role23 = 0;
    for (int32_t i = 0; i < count; i++) {
        if (S32(st, mrec + BLD_TOMB_PART_ROLE + 0x108u * (uint32_t) i) == 1) {
            role1 = 1;
            break;
        }
    }
    for (int32_t i = 0; i < count; i++) {
        int32_t r = S32(st, mrec + BLD_TOMB_PART_ROLE + 0x108u * (uint32_t) i);
        if (r == 2 || r == 3) {
            role23 = 1;
            break;
        }
    }
    window_panel(st);
    title(st, 0xB2, 0xC);
    int32_t x0 = px(st) + 0x20;
    int32_t y = py(st) + 0x38;
    S32(st, G_TEXT_Y) = 0;
    S32(st, G_TEXT_X) = 0;
    group_lines(st, 0xB2, 0x93, x0, py(st) + 0x3C, (int32_t) ((uint32_t) pw(st) * 6u), 0);
    /* @bin 0x00513EE0 0x004CCC30 "count string" (no layout offset) */
    GAME_CALL(st, FN_TEXT_COUNT_DRAW, 8, 0xA, S32(st, G_MONUMENT_STATUS_E5 + 0x18u), S32(st, G_TEXT_X) + px(st) + 0x3C, py(st) + 0x3C,
              font_text(st), 0);
    y += S32(st, G_TEXT_Y) + 0x18;
    if (S16(st, mrec + BLD_SEQUENCE) == -1) {
        group_lines(st, 0xB2, 0x92, x0, S32(st, G_TEXT_Y) + y, width15(pw(st)), 0);
        return;
    }
    int32_t id;
    int32_t ly;
    if (S32(st, G_MONUMENT_STATUS_E5 + 0x14u) >= 0x64) {
        id = tomb_call(st, FN_GOALS_COUNTERS_MET_545FB0, 0, 0) ? 0x92 : 0x91;
        ly = S32(st, G_TEXT_Y) + y;
    } else {
        /* @bin 0x0051404E the main building's tile and word +0x20, then (0x00457AF0) the shown one's */
        int32_t tile = (int32_t) (U32(st, G_MAP_START) + (uint32_t) S16(st, mrec + BLD_ROAD_ACCESS_X) + 228u * (uint32_t) S16(st, mrec + BLD_ROAD_ACCESS_Y));
        int cut = 0;
        if ((int8_t) U8(st, G_GRID_ROUTING + (uint32_t) tile) < 0) {
            cut = 1;
        } else if (S16(st, mrec + BLD_DISTANCE) == 0) {
            GAME_CALL0(st, FN_DAY_ROAD_ACCESS_457AF0);
            cut = S16(st, BUILDING_ADDR(S32(st, G_INFO_BUILDING)) + BLD_DISTANCE) == 0;
        }
        if (cut) {
            id = 0x94;
            ly = y;
        } else {
            if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xC7u) == 0) {
                id = 0xD;
            } else if (S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xC7u) == 0 && S32(st, G_MONUMENT_STATUS_E5 + 0x8u) == 0) {
                id = 0x11;
            } else if (tomb_call(st, FN_MONUMENT_PARTS_FREE_547BA0, 1, main)) {
                id = 0x8F;
            } else if (role1 && S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB3u) == 0) {
                id = 0xE;
            } else if (role1 && S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xB3u) == 0 && S32(st, G_MONUMENT_STATUS_E5 + 0x4u) == 0) {
                id = 0x12;
            } else if (role23 && S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xE7u) == 0) {
                id = 0x69;
            } else if (role23 && S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xE7u) == 0 && S32(st, G_MONUMENT_STATUS_E5 + 0x10u) == 0) {
                id = 0x6A;
            } else {
                id = 0x90;
            }
            ly = S32(st, G_TEXT_Y) + y;
        }
    }
    group_lines(st, 0xB2, id, x0, ly, width15(pw(st)), 0);
    y += S32(st, G_TEXT_Y) + 0x18;
    int32_t w = blocks(pw(st)) - 0x10;
    if (S32(st, G_MONUMENT_STATUS_E5 + 0x14u) < 0x64) {
        S32(st, G_TEXT_Y) = 0;
        expanded_lines(st, 0xB2, 0x3D, x0, y, w);
    }
}

static void info_monument_e4(game_state *st)
{
    /* @bin 0x00514400 type 0xE4 (wavs\suntemple.wav; status record 0x00EADB88, computed by the open
     * with 0x004E6710 on 0x006DE548): the goods still needed (0x004212A0 on the main building's +0x58,
     * the stage), the stage's text (group 0xB2), then the expanded texts of the progress */
    int32_t b = S32(st, G_INFO_BUILDING);
    uint32_t stage_at = BUILDING_ADDR(b - S16(st, BUILDING_ADDR(b) + BLD_PART)) + BLD_MONUMENT_STAGE;
    S32(st, G_INFO_HELP) = 0x1E7;
    int32_t v = S32(st, stage_at);
    sound_once(st, G_WAV_SUNTEMPLE);
    window_panel(st);
    title(st, 0xB2, 0xC);
    int32_t x0 = px(st) + 0x20;
    int32_t y = py(st) + 0x38;
    uint8_t need4f = (uint8_t) GAME_CALL(st, FN_GOODS_NEEDED_5B0058, 0x4F, S32(st, stage_at));
    uint8_t need51 = (uint8_t) GAME_CALL(st, FN_GOODS_NEEDED_5B0058, 0x51, S32(st, stage_at));
    S32(st, G_TEXT_Y) = 0;
    uint8_t short_ = (uint8_t) goods_check(st, 0x23, (int32_t) ((uint32_t) S32(st, G_MONUMENT_STATUS_E4 + 0x8u) * 400u));
    uint8_t cut = (uint8_t) building_cut_off(st, 1);
    int32_t id = -1;
    int32_t ly = y;
    if (v >= 9) {
        id = 0x8C;
    } else if (cut) {
        id = 0x95;
    } else if (S32(st, G_MONUMENT_STATUS_E4 + 0x8u) != 0 && short_ != 0 && S32(st, G_MONUMENT_STATUS_E4 + 0x14u) != 0) {
        id = S32(st, CITY_BASE + CITY_STOCKPILE + 4u * 0x23u + CITY_REC_SIZE * U32(st, CURRENT_PLAYER_ADDR)) != 0 ? 0x65 : 0x1A;
        ly = S32(st, G_TEXT_Y) + y;
    } else {
        if (need51) {
            if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xC7u) == 0) {
                id = 0xD;
            } else if (S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xC7u) == 0 && S32(st, G_MONUMENT_STATUS_E4 + 0xCu) == 0) {
                id = 0x11;
            } else if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB3u) == 0) {
                id = 0xE;
            } else if (S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xB3u) == 0 && S32(st, G_MONUMENT_STATUS_E4 + 0x4u) == 0) {
                id = 0x12;
            }
        }
        if (id < 0 && need4f) {
            if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB1u) == 0) {
                id = 0x10;
            } else if (S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xB1u) == 0 && S32(st, G_MONUMENT_STATUS_E4 + 0x10u) == 0) {
                id = 0x14;
            }
        }
        if (id < 0) {
            id = v < 0 ? 0x84 : v == 0 ? 0x85 : v == 1 ? 0x86 : v < 4 ? 0x87 : v == 4 ? 0x88 : v < 7 ? 0x89
                 : v == 7 ? 0x8A : v == 8 ? 0x8B : -1;
        }
    }
    if (id >= 0) {
        group_lines(st, 0xB2, id, x0, ly, width15(pw(st)), 0);
    }
    y += S32(st, G_TEXT_Y) + 0x18;
    int32_t progress = S32(st, G_MONUMENT_STATUS_E4 + 0x18u);
    S32(st, G_COURSE_MAIN) = 0x23;
    int32_t w = blocks(pw(st)) - 0x10;
    if (progress >= 0x64) {
        return;
    }
    S32(st, G_TEXT_Y) = 0;
    expanded_lines(st, 0xB2, 0x3D, x0, y, w);
    y += S32(st, G_TEXT_Y);
    if (S32(st, G_MONUMENT_STATUS_E4 + 0x14u) != 0) {
        S32(st, G_TEXT_Y) = 0;
        expanded_lines(st, 0xB2, 0x44, x0, y, w);
    }
}

static void info_mausoleum(game_state *st)
{
    /* @bin 0x00514A40 type 0xDE (wavs\mausoleum.wav; status record 0x00EADB58, computed by the open
     * with 0x004DF040): the text of the phase [0x00EADB58] (1..3; other values: nothing more), then
     * the expanded texts 0x3D and 0x44 while the progress 0x00EADB78 is below 100, else 0x47 */
    S32(st, G_INFO_HELP) = 0x170;
    sound_once(st, G_WAV_MAUSOLEUM);
    window_panel(st);
    title(st, 0xB2, 0xC);
    int32_t x0 = px(st) + 0x20;
    int32_t y = py(st) + 0x38;
    /* @bin 0x00514AD2 0x004F5770(0x1E, ([0x00EADB6C] * 5 + 5) * 80) */
    uint8_t ready = (uint8_t) goods_check(st, 0x1E, (int32_t) (((uint32_t) S32(st, G_MONUMENT_STATUS_DE + 0x14u) * 5u + 5u) * 80u));
    S32(st, G_TEXT_Y) = 0;
    int32_t id;
    int32_t made, want;
    switch (S32(st, G_MONUMENT_STATUS_DE)) {
    case 1:
        if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xC7u) == 0) {
            id = 0xD;
        } else if (S32(st, G_TYPE_COUNT_ACTIVE + 4u * 0xC7u) == 0 && S32(st, G_MONUMENT_STATUS_DE + 0x18u) == 0) {
            int32_t d = S32(st, G_FLOOD_STATE);
            id = (d == 1 || d == 2 || d == 3) ? 0x33 : 0x11;
        } else {
            id = 0x3A;
        }
        break;
    case 2:
        made = S32(st, G_MONUMENT_STATUS_DE + 0x10u);
        want = S32(st, G_MONUMENT_STATUS_DE + 0x14u);
        if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xC7u) == 0) {
            id = 0xD;
        } else if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB3u) == 0) {
            id = 0xE;
        } else if (made == want && ready == 0) {
            id = 0x18;
        } else if (made == 0) {
            id = 0x12;
        } else if (want != 0 && ready != 0 && S32(st, G_MONUMENT_STATUS_DE + 0x1Cu) != 0) {
            id = S32(st, CITY_BASE + CITY_STOCKPILE + 4u * 0x1Eu + CITY_REC_SIZE * U32(st, CURRENT_PLAYER_ADDR)) != 0 ? 0x64 : 0x18;
        } else {
            id = 0x3B;
        }
        break;
    case 3:
        if (GAME_CALL(st, FN_MONUMENT_DE_STATUS, S32(st, G_INFO_BUILDING), 0) >= 0x64) {
            id = 0x3C;
        } else if (S32(st, G_MONUMENT_STATUS_DE + 0x1Cu) != 0) {
            made = S32(st, G_MONUMENT_STATUS_DE + 0x10u);
            want = S32(st, G_MONUMENT_STATUS_DE + 0x14u);
            if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xC7u) == 0) {
                id = 0xD;
            } else if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xB3u) == 0) {
                id = 0xE;
            } else if (S32(st, G_MONUMENT_STATUS_DE + 0x2Cu) != 0) {
                id = 0x61;
            } else if (made == want && ready == 0) {
                id = 0x18;
            } else if (made == 0) {
                id = 0x12;
            } else if (want != 0 && ready != 0) {
                id = 0x18;
            } else {
                id = 0x3B;
            }
        } else if (S32(st, G_MONUMENT_STATUS_DE + 0xCu) != 0) {
            if (S32(st, G_TYPE_COUNT_TOTAL + 4u * 0xC7u) == 0) {
                id = 0xD;
            } else if (S32(st, G_MONUMENT_STATUS_DE + 0x18u) == 0) {
                id = 0x11;
            } else {
                id = 0x3B;
            }
        } else {
            id = 0x3B;
        }
        break;
    default:
        return;
    }
    group_lines(st, 0xB2, id, x0, y, width15(pw(st)), 0);
    y += S32(st, G_TEXT_Y);
    int32_t w = blocks(pw(st)) - 0x10;
    int32_t progress = S32(st, G_MONUMENT_STATUS_DE + 0x20u);
    S32(st, G_COURSE_MAIN) = 0x1E;
    S32(st, G_TEXT_Y) = 0;
    if (progress >= 0x64) {
        expanded_lines(st, 0xB2, 0x47, x0, y, w);
        return;
    }
    expanded_lines(st, 0xB2, 0x3D, x0, y, w);
    y += S32(st, G_TEXT_Y);
    if (S32(st, G_MONUMENT_STATUS_DE + 0x1Cu) != 0) {
        S32(st, G_TEXT_Y) = 0;
        expanded_lines(st, 0xB2, 0x44, x0, y, w);
    }
}

/* ---- registration ---- */

static int32_t put32(game_state *st, uint32_t function, uint32_t address, int32_t v)
{
    /* an output through a pointer: a state address when called from the state (the panels pass
     * their own stack) */
    uint8_t *p = state_ptr(st, address, 4);
    if (!p) {
        state_unported(st, function);
        return 0;
    }
    p[0] = (uint8_t) v;
    p[1] = (uint8_t) ((uint32_t) v >> 8);
    p[2] = (uint8_t) ((uint32_t) v >> 16);
    p[3] = (uint8_t) ((uint32_t) v >> 24);
    return 0;
}

static int32_t run_resource_works(game_state *st, const int32_t *a)
{
    int32_t n = 0;
    info_resource_works(st, a[0], &n, a[2]);
    return put32(st, FN_INFO_RESOURCE_WORKS, (uint32_t) a[1], n);
}

#define RUN0(name) \
    static int32_t run_##name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        name(st); \
        return 0; \
    }

RUN0(info_monument_cf)
RUN0(info_sphinx)
RUN0(info_monument_e6)
RUN0(info_sun_temple)
RUN0(info_pyramid)
RUN0(info_monument_e1)
RUN0(info_monument_e3)
RUN0(info_tomb)
RUN0(info_monument_e4)
RUN0(info_mausoleum)

static int32_t run_goods_check(game_state *st, const int32_t *a) { return goods_check(st, a[0], a[1]); }
static int32_t run_slot_has_good(game_state *st, const int32_t *a) { return slot_has_good(st, a[0], a[1]); }
static int32_t run_slot_needs(game_state *st, const int32_t *a) { return slot_needs(st, a[0], a[1]); }
static int32_t run_monument_wants(game_state *st, const int32_t *a) { return monument_wants(st, a[0], a[1]); }
static int32_t run_parts_ready(game_state *st, const int32_t *a) { return pyramid_parts_ready(st, a[0]); }

static int32_t run_sphinx_works(game_state *st, const int32_t *a)
{
    int32_t n = 0;
    sphinx_works(st, a[0], &n);
    return put32(st, FN_INFO_SPHINX_WORKS, (uint32_t) a[1], n);
}

static int32_t run_sphinx_progress(game_state *st, const int32_t *a)
{
    int32_t v[6] = {0};
    sphinx_progress(st, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]);
    put32(st, FN_INFO_SPHINX_PROGRESS, (uint32_t) a[4], v[4]);
    put32(st, FN_INFO_SPHINX_PROGRESS, (uint32_t) a[5], v[5]);
    put32(st, FN_INFO_SPHINX_PROGRESS, (uint32_t) a[3], v[3]);
    put32(st, FN_INFO_SPHINX_PROGRESS, (uint32_t) a[0], v[0]);
    put32(st, FN_INFO_SPHINX_PROGRESS, (uint32_t) a[1], v[1]);
    return 0;
}

const ported_function info_monument_ports[] = {
    {0x004E6070u, "info_resource_works", 3, 0, 0, run_resource_works, 0},
    {0x004204D0u, "info_sphinx_works", 2, 0, 0, run_sphinx_works, 0},
    {0x00420600u, "info_sphinx_progress", 6, 0, 0, run_sphinx_progress, 0},
    {0x004F5580u, "info_slot_needs", 2, 8, 0, run_slot_needs, 0},
    {0x004F56A0u, "info_slot_has_good", 2, 8, 0, run_slot_has_good, 0},
    {0x004F56D0u, "info_slot_goods_wanted", 30, 8, 0, NULL, 0},
    {0x004F5770u, "info_goods_check", 2, 8, 0, run_goods_check, 0},
    {0x004F5BD0u, "info_monument_wants", 2, 8, 0, run_monument_wants, 0},
    {0x004F5D10u, "info_pyramid_parts_ready", 1, 8, 0, run_parts_ready, 0},
    {0x00511040u, "info_pyramid", 0, 0, 0, run_info_pyramid, 0},
    {0x00511C80u, "info_sphinx", 0, 0, 0, run_info_sphinx, 0},
    {0x005124A0u, "info_sun_temple", 0, 0, 0, run_info_sun_temple, 0},
    {0x00512140u, "info_monument_cf", 0, 0, 0, run_info_monument_cf, 0},
    {0x00512BE0u, "info_monument_e1", 0, 0, 0, run_info_monument_e1, 0},
    {0x005135E0u, "info_monument_e3", 0, 0, 0, run_info_monument_e3, 0},
    {0x00513D40u, "info_tomb", 0, 0, 0, run_info_tomb, 0},
    {0x00514400u, "info_monument_e4", 0, 0, 0, run_info_monument_e4, 0},
    {0x00514A40u, "info_mausoleum", 0, 0, 0, run_info_mausoleum, 0},
    {0x005132A0u, "info_monument_e6", 0, 0, 0, run_info_monument_e6, 0},
};
const int info_monument_ports_count = sizeof(info_monument_ports) / sizeof(info_monument_ports[0]);
