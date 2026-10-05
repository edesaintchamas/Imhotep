/* Imhotep - lookups in the game's text.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/text_lookup.h"

#include "state/ported.h"
#include "state/records.h"

#define TEXT_BUFFER 0x00E36C70u /* pointer to the text from byte 0x1F5C of the file (0x004CEC40) */
#define TEXT_GROUPS 0x00E34C38u /* 1000 group entries of 8 bytes: +0 offset into the text */
#define MISSION 0x00E38EA8u     /* i8: the current mission */
#define BUILDING_HIGHEST 0x0078D790u
#define GOAL_COUNTERS 0x00784E78u

void text_seek(game_state *st, int32_t group, int32_t index)
{
    /* @bin 0x004CC110 skip `index` strings (a 0 byte after a printable character or another 0),
     * then any bytes below 0x20 */
    uint32_t p = U32(st, TEXT_GROUPS + 8u * (uint32_t) group) + U32(st, TEXT_BUFFER);
    U32(st, TEXT_CURSOR) = p;
    while (index > 0) {
        if (P8(st, p) == 0) {
            uint8_t prev = P8(st, p - 1);
            if (prev >= 0x20 || prev == 0) {
                index--;
            }
        }
        p++;
        U32(st, TEXT_CURSOR) = p;
    }
    while (P8(st, p) < 0x20) {
        p++;
        U32(st, TEXT_CURSOR) = p;
    }
}

int32_t mission_goals_pending(game_state *st)
{
    /* @bin 0x00469D50 */
    text_seek(st, 0x133, S8(st, MISSION));
    uint8_t flag = P8(st, U32(st, TEXT_CURSOR) + 4);
    if (flag == 'A' || flag == 'V') {
        for (int32_t id = 1; id <= S32(st, BUILDING_HIGHEST); id++) {
            uint32_t b = BUILDING_ADDR(id);
            int16_t t = S16(st, b + 0x10);
            if ((t == 0xDE || t == 0xE5 || t == 0xEA || t == 0xEB || t == 0xEC) && S16(st, b + 0x16) == -1) {
                return 1;
            }
        }
    }
    /* @bin 0x00469DDA jump table 0x00469E18: the goal kinds whose counter is checked */
    static const uint8_t checked[30] = {[0] = 1, [7] = 1, [9] = 1, [12] = 1, [14] = 1, [16] = 1, [17] = 1,
                                        [18] = 1, [19] = 1, [22] = 1, [23] = 1, [24] = 1, [25] = 1, [27] = 1,
                                        [29] = 1};
    for (int32_t i = 0; i + 1 < 0x24; i++) {
        if (i <= 0x1D && checked[i] && S32(st, GOAL_COUNTERS + 4u * (uint32_t) (i + 1)) > 0) {
            return 1;
        }
    }
    return 0;
}

#define GLYPH_MAP 0x005E047Cu     /* u8 per character: glyph number (0: none) */
#define GLYPH_LAST 0x00D3B5C0u    /* the last glyph looked up */
#define IMAGES 0x010D6A0Cu        /* image records of 0x40 bytes: +0 i16 width */
#define IMAGE_COUNT 0x0126517Cu
#define FONT_LARGE 0x00F56FB4u    /* first glyph ids of three fonts */
#define FONT_LARGE2 0x00F56FBCu
#define FONT_SMALL 0x00F56F84u

/* the string's bytes, or NULL (and unported) when it lies outside the state and mapped data */
static const uint8_t *text_bytes(game_state *st, uint32_t text, uint32_t address)
{
    const uint8_t *p = state_ptr(st, text, 1);
    if (!p) {
        state_unported(st, address);
    }
    return p;
}

int32_t text_width(game_state *st, uint32_t text, int32_t font)
{
    /* @bin 0x004CBF30 */
    int32_t space, extra;
    if (font == S32(st, FONT_LARGE) || font == S32(st, FONT_LARGE2)) {
        space = 10;
        extra = 1;
    } else if (font == S32(st, FONT_SMALL)) {
        space = 4;
        extra = 1;
    } else {
        space = 6;
        extra = 0;
    }
    int32_t width = 0;
    for (int n = 10000; n > 0; n--, text++) {
        const uint8_t *p = text_bytes(st, text, 0x004CBF30u);
        if (!p || *p == 0) {
            return width;
        }
        if (*p == 0x20) {
            width += space;
            continue;
        }
        uint32_t glyph = U8(st, GLYPH_MAP + *p);
        S32(st, GLYPH_LAST) = (int32_t) glyph;
        if (glyph != 0) {
            int32_t image = (int32_t) glyph - 1 + font;
            if (image >= S32(st, IMAGE_COUNT)) {
                image = 0;
            }
            width += S16(st, IMAGES + 0x40u * (uint32_t) image) + extra;
        }
    }
    return width;
}

int32_t text_length(game_state *st, uint32_t text)
{
    /* @bin 0x004CC000 */
    int32_t n = 0;
    for (int left = 10000; left > 0; left--, text++) {
        const uint8_t *p = text_bytes(st, text, 0x004CC000u);
        if (!p || *p == 0) {
            break;
        }
        n++;
    }
    return n;
}

void text_copy(game_state *st, uint32_t src, uint32_t dst, int32_t n)
{
    /* @bin 0x004CBEA0 */
    for (; n > 0; n--, src++, dst++) {
        const uint8_t *p = text_bytes(st, src, 0x004CBEA0u);
        uint8_t *q = state_ptr(st, dst, 1);
        if (!p || !q) {
            if (p) {
                state_unported(st, 0x004CBEA0u);
            }
            return;
        }
        *q = *p;
    }
}

static int32_t run_text_width(game_state *st, const int32_t *a)
{
    return text_width(st, (uint32_t) a[0], a[1]);
}

static int32_t run_text_length(game_state *st, const int32_t *a)
{
    return text_length(st, (uint32_t) a[0]);
}

static int32_t run_text_copy(game_state *st, const int32_t *a)
{
    text_copy(st, (uint32_t) a[0], (uint32_t) a[1], a[2]);
    return 0;
}

static int32_t run_text_seek(game_state *st, const int32_t *a)
{
    text_seek(st, a[0], a[1]);
    return 0;
}

static int32_t run_goals_pending(game_state *st, const int32_t *a)
{
    (void) a;
    return mission_goals_pending(st);
}

const ported_function text_ports[] = {
    {0x004CC110u, "text_seek", 2, 0, 0, run_text_seek, 0},
    {0x00469D50u, "mission_goals_pending", 0, 8, 0, run_goals_pending, 0},
    {0x004CBF30u, "text_width", 2, 32, 0, run_text_width, 0},
    {0x004CC000u, "text_length", 1, 32, 0, run_text_length, 0},
    {0x004CBEA0u, "text_copy", 3, 0, 0, run_text_copy, 0},
};
const int text_ports_count = sizeof(text_ports) / sizeof(text_ports[0]);
