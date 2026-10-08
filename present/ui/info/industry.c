/* Imhotep - window 9: info panels (entertainment, health, education, prefecture, military) (package
 * "info").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The panels of the redraw 0x004F8E60's jump table for the building types 0x1E..0x25, 0x2E, 0x2F,
 * 0x31, 0x33, 0x35, 0x37, 0x39, 0x3A, 0x3B, 0x5E, 0x5F, 0x8A, 0xA7, 0xAA..0xAD, 0xB9, 0xBA,
 * 0xC8..0xCA, 0xCE, 0xD1, 0xE2 (present/ui/info/info.c panel_of_type). Each one sets the help
 * message 0x00EADD54, asks for the building's sound once (0x0047D350, an empty function in this
 * build: the call is kept), draws the window panel 0x00536820, the title, a status text chosen from
 * the building's fields, and most an inner panel with the workers line 0x004FC5C0. Every call
 * goes through GAME_CALL (the drawing primitives and the shared helpers belong to other files);
 * the panels of this file call each other directly. Names follow the sound file each panel asks
 * for (the strings at 0x005F7940..0x005F7B10). */
#include "info.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define IMAGES 0x00F55BB0u          /* `this` of the image draw 0x0053E590 */
#define PANEL_IMAGES 0x00F571FCu    /* i32 the first image of the panels' icons */
#define FONT_F8C INFO_FONT_TEXT     /* i32 0x00F56F8C */
#define FONT_F94 0x00F56F94u        /* i32 a text font (the panels' detail lines) */
#define FONT_F9C 0x00F56F9Cu        /* i32 a text font (the culture ratings of 0x00515D10) */
#define TEXT_CURSOR 0x00E20778u     /* i32 the width of the last text drawn (texts append at x + it) */
#define TEXT_STRING 0x00E20768u     /* i32 the string the text lookup 0x004CC110 returns */
#define EMPTY_SUFFIX 0x0057C048u    /* the empty string given as a number's suffix */
#define PERCENT_SUFFIX 0x007027D4u  /* the suffix string of the barracks' numbers */

/* @bin building record fields read by the panels (base 0x007999D8, 0x108 bytes) */
#define B_TYPE 0x10u        /* i16 */
#define B_FIELD_2A 0x2Au    /* i16 tested by the prefecture, tower and academy-like panels */
#define B_FIELD_3C 0x3Cu    /* i16 a count shown by several panels (performers, patients, ...) */
#define B_WORKERS 0x40u     /* i16 the building's workers */
#define B_A6 0xA6u          /* u8 */
#define B_A7 0xA7u          /* u8 days of a show (shown doubled) */
#define B_A8 0xA8u          /* u8 */
#define B_A9 0xA9u          /* u8 */
#define B_FIELD_CC 0xCCu    /* i16 a second count */

static int32_t px(game_state *st) { return S32(st, INFO_X); }
static int32_t py(game_state *st) { return S32(st, INFO_Y); }
static int32_t pw(game_state *st) { return S32(st, INFO_W); }

static uint32_t building(game_state *st)
{
    return BUILDING_ADDR(S32(st, INFO_BUILDING));
}

static int32_t text_width(game_state *st)
{
    /* @bin 0x004FD5C4 (W - 4) * 16: the width of the panels' texts (add -4, shl 4) */
    return (int32_t) ((uint32_t) (pw(st) - 4) << 4);
}

static void panel_begin(game_state *st, int32_t help, uint32_t sound, int32_t group)
{
    /* @bin 0x004FC820 the panels' common start: the help message, the building's sound on the first
     * draw (0x0047D350(name, 1, 0)), the window panel, the title (text 0 of the group, large font,
     * centred over the panel's width at y + 10) */
    S32(st, INFO_HELP) = help;
    if (S32(st, INFO_SOUND) != 0) {
        GAME_CALL(st, 0x0047D350u, (int32_t) sound, 1, 0);
        S32(st, INFO_SOUND) = 0;
    }
    GAME_CALL(st, 0x00536820u, px(st), py(st), pw(st), S32(st, INFO_H));
    GAME_CALL(st, 0x004D6250u, group, 0, px(st), py(st) + 10, (int32_t) ((uint32_t) pw(st) << 4),
              S32(st, INFO_FONT_LARGE), 0, px(st), py(st));
}

static void text_block(game_state *st, int32_t group, int32_t id, int32_t x, int32_t y, int32_t w, uint32_t font)
{
    /* @bin 0x004D6160 a text of the group wrapped to w pixels, relative to the panel (the last two
     * arguments: the panel's origin) */
    GAME_CALL(st, 0x004D6160u, group, id, x, y, w, S32(st, font), 0, px(st), py(st));
}

static void text_line(game_state *st, int32_t group, int32_t id, int32_t x, int32_t y, uint32_t font)
{
    /* @bin 0x004D62F0 a text of the group on one line */
    GAME_CALL(st, 0x004D62F0u, group, id, x, y, S32(st, font), 0, px(st), py(st));
}

static void inner_panel(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h)
{
    /* @bin 0x004D65B0 the inner (sunken) panel, in 16-pixel blocks */
    GAME_CALL(st, 0x004D65B0u, x, y, w, h, px(st), py(st));
}

static void number(game_state *st, int32_t a, int32_t b, int32_t value, int32_t x, int32_t y, uint32_t font)
{
    /* @bin 0x004CCC30 a number (8, b: its format) */
    GAME_CALL(st, 0x004CCC30u, a, b, value, x, y, S32(st, font), 0);
}

static void text_number(game_state *st, int32_t value, int32_t suffix, int32_t x, int32_t y)
{
    /* @bin 0x004D6860 a number with a suffix string (prefix character 0x40), text font */
    GAME_CALL(st, 0x004D6860u, value, 0x40, suffix, x, y, S32(st, FONT_F8C), 0, px(st), py(st));
}

static void image(game_state *st, int32_t id, int32_t x, int32_t y)
{
    GAME_CALLT(st, IMAGES, 0x0053E590u, id, x, y);
}

static void workers_box(game_state *st, int32_t h, int32_t line_y)
{
    /* @bin 0x004FCAD1 the inner panel at (x + 16, y + 0x88), W - 2 by h blocks, and the workers line
     * 0x004FC5C0 at line_y */
    inner_panel(st, px(st) + 0x10, py(st) + 0x88, pw(st) - 2, h);
    GAME_CALL(st, 0x004FC5C0u, line_y);
}

static int32_t no_road(game_state *st)
{
    /* @bin 0x004FC95B 0x00EADD58 <= 0: the "no road access" text (group 0x45, 0x19) */
    return S32(st, INFO_ROAD) < 1;
}

static int32_t workers(game_state *st)
{
    return S16(st, building(st) + B_WORKERS);
}

static void ratings(game_state *st);

static void panel_d1(game_state *st)
{
    /* @bin 0x004FC820 type 0xD1 (wavs\ampitheatre.wav): title and description of group 0xBC, then
     * the culture ratings 0x00515D10 (a tail jump) */
    panel_begin(st, 0x16E, 0x005F7940u, 0xBC);
    GAME_CALL(st, 0x004D6160u, 0xBC, 1, px(st) + 0x20, py(st) + 0x37, text_width(st), S32(st, FONT_F8C), 0,
              px(st), py(st));
    ratings(st);
}

static void ratings(game_state *st)
{
    /* @bin 0x00515D10 the culture ratings box of the 0xD1 panel: the current player's city value
     * +0x438C as a number, then by city +0x44F8 (0: by +0x4528 and +0x452C, and +0x438C in bands
     * 1 / 6 / 12 / 18 / 24 / 30; 1..3: the level 2 / 3 / 4 against city +0x44FC) the text of group
     * 0x127 to show */
    uint32_t city = CURRENT_CITY(st);
    inner_panel(st, px(st) + 0x28, py(st) + 0x76, pw(st) - 5, 6);
    S32(st, TEXT_CURSOR) = 0;
    number(st, 8, 4, S32(st, city + 0x438Cu), px(st) + 0x70, py(st) + 0x82, FONT_F9C);
    text_line(st, 0x3A, 0x0F, S32(st, TEXT_CURSOR) + px(st) + 0x70, py(st) + 0x82, FONT_F9C);
    city = CURRENT_CITY(st);
    int32_t kind = S32(st, city + 0x44F8u);
    int32_t id;
    if (kind != 0) {
        int32_t level;
        int32_t base;
        if (kind == 1) {
            level = 2;
            base = 0x0A;
        } else if (kind == 2) {
            level = 3;
            base = 0x14;
        } else {
            level = 4;
            base = 0x1F;
        }
        int32_t have = S32(st, city + 0x44FCu);
        S32(st, TEXT_CURSOR) = 0;
        /* @bin 0x00515DFD +0x44FC > level: text 0x35, else 0x22 */
        text_line(st, 0x3A, have > level ? 0x35 : 0x22, px(st) + 0x66, py(st) + 0x9A, FONT_F9C);
        city = CURRENT_CITY(st);
        int32_t n = S32(st, city + 0x44FCu);
        id = base + n - 1;
        if (n == 1) {
            text_line(st, 0x3A, 0x36, S32(st, TEXT_CURSOR) + px(st) + 0x66, py(st) + 0x9A, FONT_F9C);
        } else {
            /* @bin 0x00515EC3 (n + 0x0078D7C0) % 12 (idiv): a month name of group 0xA0 */
            text_line(st, 0xA0, (n + S32(st, 0x0078D7C0u)) % 0x0C, S32(st, TEXT_CURSOR) + px(st) + 0x66,
                      py(st) + 0x9A, FONT_F9C);
        }
    } else if (S32(st, city + 0x4528u) != 0 && S32(st, city + 0x452Cu) != 0) {
        GAME_CALL(st, 0x004D6250u, 0x3A, 0x34, px(st) + 0x52, py(st) + 0x9A, 0x12C, S32(st, FONT_F9C), 0, px(st),
                  py(st));
        int32_t v = S32(st, CURRENT_CITY(st) + 0x438Cu);
        if (v <= 1) {
            id = 7;
        } else {
            id = (v > 6) + 8;
        }
    } else {
        GAME_CALL(st, 0x004D6250u, 0x3A, 0x10, px(st) + 0x52, py(st) + 0x9A, 0x12C, S32(st, FONT_F9C), 0, px(st),
                  py(st));
        int32_t v = S32(st, CURRENT_CITY(st) + 0x438Cu);
        if (v <= 1) {
            id = 0;
        } else if (v <= 6) {
            id = 1;
        } else if (v <= 0x0C) {
            id = 2;
        } else if (v <= 0x12) {
            id = 3;
        } else if (v <= 0x18) {
            id = 4;
        } else {
            id = (v > 0x1E) + 5;
        }
    }
    GAME_CALL(st, 0x004D6160u, 0x127, id, px(st) + 0x38, py(st) + 0xAF, 0x17C, S32(st, FONT_F9C), 0, px(st),
              py(st));
}

static void show_days(game_state *st, int32_t group, int32_t none, uint32_t field, int32_t y)
{
    /* @bin 0x004FCB09 a show line: text `none` when the byte is 0, else text none + 1 followed by
     * the byte doubled (number format 8, 0x2C) */
    if (U8(st, building(st) + field) == 0) {
        text_line(st, group, none, px(st) + 0x20, y, FONT_F94);
        return;
    }
    S32(st, TEXT_CURSOR) = 0;
    text_line(st, group, none + 1, px(st) + 0x20, y, FONT_F94);
    number(st, 8, 0x2C, (int32_t) ((uint32_t) U8(st, building(st) + field) << 1),
           S32(st, TEXT_CURSOR) + 0x20 + px(st), y, FONT_F94);
}

static void panel_1e(game_state *st)
{
    /* @bin 0x004FC8E0 type 0x1E (wavs\ampitheatre.wav), group 0x47 */
    panel_begin(st, 0x48, 0x005F7940u, 0x47);
    int32_t group = 0x45;
    int32_t id = 0x19;
    int draw = 1;
    if (!no_road(st)) {
        uint32_t b = building(st);
        group = 0x47;
        if (workers(st) < 1) {
            id = 6;
        } else if (U8(st, b + B_A6) == 0) {
            id = 2;
        } else if (U8(st, b + B_A6) == 2) {
            id = 3;
        } else if (U8(st, b + B_A8) != 0) {
            id = 4;
        } else if (U8(st, b + B_A7) != 0) {
            id = 5;
        } else {
            draw = 0;
        }
    }
    if (draw) {
        text_block(st, group, id, px(st) + 0x20, py(st) + 0x38, text_width(st), FONT_F8C);
    }
    workers_box(st, 7, py(st) + 0x8A);
    show_days(st, 0x47, 9, B_A7, py(st) + 0xB6);
    show_days(st, 0x47, 7, B_A8, py(st) + 0xCA);
}

static void panel_1f(game_state *st)
{
    /* @bin 0x004FCC90 type 0x1F (wavs\theatre.wav), group 0x48 */
    panel_begin(st, 0x47, 0x005F7958u, 0x48);
    int32_t group = 0x45;
    int32_t id = 0x19;
    int draw = 1;
    if (!no_road(st)) {
        uint32_t b = building(st);
        group = 0x48;
        if (workers(st) < 1) {
            id = 4;
        } else if (U8(st, b + B_A6) == 0) {
            id = 2;
        } else if (U8(st, b + B_A7) != 0) {
            id = 3;
        } else {
            draw = 0;
        }
    }
    if (draw) {
        text_block(st, group, id, px(st) + 0x20, py(st) + 0x38, text_width(st), FONT_F8C);
    }
    workers_box(st, 6, py(st) + 0x8A);
    show_days(st, 0x48, 5, B_A7, py(st) + 0xB6);
}

static int32_t positive_i16(game_state *st, uint32_t field)
{
    /* @bin 0x004FCFFF the field when >= 1, else 0 */
    int16_t v = S16(st, building(st) + field);
    return v < 1 ? 0 : v;
}

static void panel_20(game_state *st)
{
    /* @bin 0x004FCF10 type 0x20 (wavs\hippodrome.wav), group 0x49: an icon and the count +0x3C */
    panel_begin(st, 0x4A, 0x005F796Cu, 0x49);
    image(st, S32(st, PANEL_IMAGES) + 0x0F, px(st) + 0x20, py(st) + 0x24);
    S32(st, TEXT_CURSOR) = 0;
    text_line(st, 0x49, 7, px(st) + 0x3C, py(st) + 0x28, FONT_F8C);
    number(st, 8, 10, positive_i16(st, B_FIELD_3C), S32(st, TEXT_CURSOR) + 0x3C + px(st), py(st) + 0x28, FONT_F8C);
    if (no_road(st)) {
        text_block(st, 0x45, 0x19, px(st) + 0x20, py(st) + 0x38, text_width(st), FONT_F8C);
    } else if (workers(st) < 1) {
        text_block(st, 0x49, 4, px(st) + 0x20, py(st) + 0x38, text_width(st), FONT_F8C);
    }
    workers_box(st, 6, py(st) + 0x8A);
}

static void panel_e2(game_state *st)
{
    /* @bin 0x004FD120 type 0xE2 (wavs\hippodrome.wav), group 0x134: icons and the counts +0x3C, +0xCC */
    panel_begin(st, 0x1DF, 0x005F796Cu, 0x134);
    image(st, S32(st, PANEL_IMAGES) + 8, px(st) + 0x20, py(st) + 0x24);
    S32(st, TEXT_CURSOR) = 0;
    text_line(st, 0x134, 6, px(st) + 0x3C, py(st) + 0x28, FONT_F8C);
    number(st, 8, 10, positive_i16(st, B_FIELD_3C), S32(st, TEXT_CURSOR) + 0x3C + px(st), py(st) + 0x28, FONT_F8C);
    image(st, S32(st, PANEL_IMAGES) + 9, px(st) + 0xFF, py(st) + 0x24);
    S32(st, TEXT_CURSOR) = 0;
    text_line(st, 0x134, 7, px(st) + 0x118, py(st) + 0x28, FONT_F8C);
    number(st, 8, 10, positive_i16(st, B_FIELD_CC), S32(st, TEXT_CURSOR) + 0x118 + px(st), py(st) + 0x28,
           FONT_F8C);
    int32_t group = 0x45;
    int32_t id = 0x19;
    if (!no_road(st)) {
        uint32_t b = building(st);
        group = 0x134;
        if (workers(st) < 1) {
            id = 4;
        } else if (S16(st, b + B_FIELD_3C) < 1) {
            id = 2;
        } else if (S16(st, b + B_FIELD_CC) < 1) {
            id = 3;
        } else {
            id = 1;
        }
    }
    text_block(st, group, id, px(st) + 0x20, py(st) + 0x48, text_width(st), FONT_F8C);
    workers_box(st, 6, py(st) + 0x8A);
}

static void panel_21(game_state *st)
{
    /* @bin 0x004FD4B0 type 0x21 (wavs\colloseum.wav), group 0x4A: three shows (+0xA7, +0xA8, +0xA9) */
    panel_begin(st, 0x49, 0x005F7980u, 0x4A);
    uint32_t b = building(st);
    uint8_t c1 = U8(st, b + B_A7);
    uint8_t c2 = U8(st, b + B_A8);
    uint8_t c3 = U8(st, b + B_A9);
    int32_t n = (c1 != 0) + (c2 != 0) + (c3 != 0);
    int32_t group = 0x45;
    int32_t id = 0x19;
    int draw = 1;
    if (!no_road(st)) {
        group = 0x4A;
        /* @bin 0x004FD5E0 the text by which shows run */
        if (workers(st) < 1) {
            id = 10;
        } else if (n == 0) {
            id = 2;
        } else if (n == 3) {
            id = 3;
        } else if (c3 != 0 && c2 != 0) {
            id = 4;
        } else if (c3 != 0 && c1 != 0) {
            id = 5;
        } else if (c2 != 0 && c1 != 0) {
            id = 6;
        } else if (c2 == 0 && c1 != 0) {
            id = 7;
        } else if (c2 != 0) {
            id = 8;
        } else if (c3 != 0) {
            id = 9;
        } else {
            draw = 0;
        }
    }
    if (draw) {
        text_block(st, group, id, px(st) + 0x20, py(st) + 0x38, text_width(st), FONT_F8C);
    }
    workers_box(st, 6, py(st) + 0x8A);
    show_days(st, 0x4A, 0x0B, B_A7, py(st) + 0xB6);
    show_days(st, 0x4A, 0x0D, B_A8, py(st) + 0xC5);
    show_days(st, 0x4A, 0x0F, B_A9, py(st) + 0xD4);
}

static void panel_by_workers(game_state *st, int32_t help, uint32_t sound, int32_t group)
{
    /* @bin 0x004FDA00 the pit panels (0x22..0x25): the text by the workers' percentage 0x00EADD5C
     * (>= 100: 2, >= 75: 3, >= 50: 4, >= 25: 5, else 6; no workers: 7) */
    panel_begin(st, help, sound, group);
    int32_t g = 0x45;
    int32_t id = 0x19;
    if (!no_road(st)) {
        g = group;
        int32_t pct = S32(st, INFO_WORKERS);
        if (workers(st) < 1) {
            id = 7;
        } else if (pct >= 100) {
            id = 2;
        } else if (pct >= 0x4B) {
            id = 3;
        } else if (pct >= 0x32) {
            id = 4;
        } else if (pct >= 0x19) {
            id = 5;
        } else {
            id = 6;
        }
    }
    text_block(st, g, id, px(st) + 0x20, py(st) + 0x38, text_width(st), FONT_F8C);
    workers_box(st, 4, py(st) + 0x8E);
}

static void panel_22(game_state *st) { panel_by_workers(st, 0x4B, 0x005F7994u, 0x4B); } /* @bin 0x004FDA00 wavs\glad_pit.wav */
static void panel_23(game_state *st) { panel_by_workers(st, 0x4B, 0x005F79A8u, 0x4C); } /* @bin 0x004FDC40 wavs\lion_pit.wav */
static void panel_24(game_state *st) { panel_by_workers(st, 0x4B, 0x005F79BCu, 0x4D); } /* @bin 0x004FDE80 wavs\art_pit.wav */
static void panel_25(game_state *st) { panel_by_workers(st, 0x4B, 0x005F79D0u, 0x4E); } /* @bin 0x004FE0C0 wavs\char_pit.wav */

static void panel_staffed(game_state *st, int32_t help, uint32_t sound, int32_t group)
{
    /* @bin 0x004FE9A0 the simple panels: text 2 without workers, else 3 */
    panel_begin(st, help, sound, group);
    int32_t g = 0x45;
    int32_t id = 0x19;
    if (!no_road(st)) {
        g = group;
        id = workers(st) < 1 ? 2 : 3;
    }
    text_block(st, g, id, px(st) + 0x20, py(st) + 0x38, text_width(st), FONT_F8C);
    workers_box(st, 4, py(st) + 0x8E);
}

static void panel_2e(game_state *st) { panel_staffed(st, 0x41, 0x005F7A38u, 0x51); } /* @bin 0x004FE9A0 wavs\clinic.wav */
static void panel_ce(game_state *st) { panel_staffed(st, 0x40, 0x005F7A5Cu, 0x53); } /* @bin 0x004FED40 wavs\baths.wav */
static void panel_31(game_state *st) { panel_staffed(st, 0x3F, 0x005F7A6Cu, 0x54); } /* @bin 0x004FEEA0 wavs\barber.wav */

static void count_line(game_state *st, int32_t icon, int32_t text)
{
    /* @bin 0x004FEB00 an icon at (x + 20, y + 30), the count +0x3C and text (0x17, text) after it */
    S32(st, TEXT_CURSOR) = 0;
    image(st, S32(st, PANEL_IMAGES) + icon, px(st) + 0x14, py(st) + 0x1E);
    text_number(st, S16(st, building(st) + B_FIELD_3C), (int32_t) EMPTY_SUFFIX,
                S32(st, TEXT_CURSOR) + 0x28 + px(st), py(st) + 0x23);
    text_line(st, 0x17, text, S32(st, TEXT_CURSOR) + 0x28 + px(st), py(st) + 0x23, FONT_F8C);
}

static void panel_2f(game_state *st)
{
    /* @bin 0x004FEB00 type 0x2F (wavs\hospital.wav), group 0x52 */
    panel_begin(st, 0x42, 0x005F7A48u, 0x52);
    count_line(st, 0x11, 0x11);
    int32_t group = 0x45;
    int32_t id = 0x19;
    if (!no_road(st)) {
        group = 0x52;
        if (workers(st) < 1) {
            id = 2;
        } else if (S16(st, building(st) + B_FIELD_3C) < 1) {
            id = 4;
        } else {
            id = 3;
        }
    }
    text_block(st, group, id, px(st) + 0x20, py(st) + 0x38, text_width(st), FONT_F8C);
    workers_box(st, 4, py(st) + 0x8E);
}

static void panel_count_staffed(game_state *st, int32_t help, uint32_t sound, int32_t group)
{
    /* @bin 0x004FF000 the count line (icon 0x17, text 0x17), then text 2 / 3 by workers */
    panel_begin(st, help, sound, group);
    count_line(st, 0x17, 0x17);
    int32_t g = 0x45;
    int32_t id = 0x19;
    if (!no_road(st)) {
        g = group;
        id = workers(st) < 1 ? 2 : 3;
    }
    text_block(st, g, id, px(st) + 0x20, py(st) + 0x38, text_width(st), FONT_F8C);
    workers_box(st, 4, py(st) + 0x8E);
}

static void panel_33(game_state *st) { panel_count_staffed(st, 0x44, 0x005F7A7Cu, 0x55); } /* @bin 0x004FF000 wavs\school.wav */
static void panel_35(game_state *st) { panel_count_staffed(st, 0x46, 0x005F7A8Cu, 0x57); } /* @bin 0x004FF200 wavs\library.wav */

static void staffing_text(game_state *st, int32_t group)
{
    /* @bin 0x004FF47B the prefecture-like status: no road; no workers (9); else text 3 (+0x2A == 0)
     * or 2, then at y + 0x48 the staffing text by 0x00EADD5C (>= 100: 4, >= 75: 5, >= 50: 6,
     * >= 25: 7, else 8) */
    int32_t g = 0x45;
    int32_t id = 0x19;
    int32_t y = py(st) + 0x38;
    if (!no_road(st)) {
        g = group;
        uint32_t b = building(st);
        if (S16(st, b + B_WORKERS) == 0) {
            id = 9;
        } else {
            text_block(st, group, S16(st, b + B_FIELD_2A) == 0 ? 3 : 2, px(st) + 0x20, py(st) + 0x38,
                       text_width(st), FONT_F8C);
            int32_t pct = S32(st, INFO_WORKERS);
            y = py(st) + 0x48;
            if (pct >= 100) {
                id = 4;
            } else if (pct >= 0x4B) {
                id = 5;
            } else if (pct >= 0x32) {
                id = 6;
            } else if (pct >= 0x19) {
                id = 7;
            } else {
                id = 8;
            }
        }
    }
    text_block(st, g, id, px(st) + 0x20, y, text_width(st), FONT_F8C);
}

static void panel_37(game_state *st)
{
    /* @bin 0x004FF400 type 0x37 (wavs\prefecture.wav), group 0x58: the status, then two city values
     * (+0x448C i16, +0x2C84 i32) with the texts 0x58/10 and 0x58/11 as suffixes (looked up with
     * 0x004CC110, the lookup's result 0x00E20768 restored after) */
    panel_begin(st, 0x56, 0x005F7AA0u, 0x58);
    staffing_text(st, 0x58);
    int32_t saved = S32(st, TEXT_STRING);
    GAME_CALL(st, 0x004CC110u, 0x58, 10);
    int32_t first = S32(st, TEXT_STRING);
    GAME_CALL(st, 0x004CC110u, 0x58, 0x0B);
    int32_t second = S32(st, TEXT_STRING);
    S32(st, TEXT_STRING) = saved;
    text_number(st, S16(st, CURRENT_CITY(st) + 0x448Cu), first, px(st) + 0x20, py(st) + 0x68);
    text_number(st, S32(st, CURRENT_CITY(st) + 0x2C84u), second, px(st) + 0x20, py(st) + 0x77);
    workers_box(st, 4, py(st) + 0x8E);
}

static void panel_a7(game_state *st)
{
    /* @bin 0x004FF740 type 0xA7 (wavs\prefecture.wav), group 0xA4: the status, the workers box, then
     * text 1 near the panel's bottom (y + H * 16 - 0x71) */
    panel_begin(st, 0x163, 0x005F7AA0u, 0xA4);
    staffing_text(st, 0xA4);
    workers_box(st, 4, py(st) + 0x8E);
    text_block(st, 0xA4, 1, px(st) + 0x20, (int32_t) ((uint32_t) S32(st, INFO_H) * 0x10u - 0x71u + (uint32_t) py(st)),
               text_width(st), FONT_F8C);
}

static int32_t bottom_y(game_state *st)
{
    /* @bin 0x004FFB30 y + H * 16 - 0x9E */
    return (int32_t) ((uint32_t) S32(st, INFO_H) * 0x10u - 0x9Eu + (uint32_t) py(st));
}

static void panel_39(game_state *st)
{
    /* @bin 0x004FFA20 type 0x39 (wavs\fort.wav), group 0x59: text 1 when the formation's +0x64
     * (i16, formations 0x00BCB9C8 + 0x90 * id) is set, else 2 */
    int32_t formation = S32(st, INFO_FORMATION);
    panel_begin(st, 0x25, 0x005F7AB4u, 0x59);
    int32_t id = S16(st, 0x00BCBA2Cu + 0x90u * (uint32_t) formation) != 0 ? 1 : 2;
    text_block(st, 0x59, id, px(st) + 0x20, bottom_y(st), text_width(st), FONT_F8C);
}

static void panel_text1(game_state *st, int32_t help, uint32_t sound, int32_t group)
{
    /* @bin 0x004FFB30 a title and text 1 */
    panel_begin(st, help, sound, group);
    text_block(st, group, 1, px(st) + 0x20, bottom_y(st), text_width(st), FONT_F8C);
}

static void panel_3a(game_state *st) { panel_text1(st, 0x55, 0x005F7AC4u, 0x5A); }  /* @bin 0x004FFB30 wavs\gatehouse.wav */
static void panel_aa(game_state *st) { panel_text1(st, 0x55, 0x005F7AC4u, 0xA7); }  /* @bin 0x004FFBF0 wavs\gatehouse.wav */
static void panel_ab(game_state *st) { panel_text1(st, 0x55, 0x005F7AC4u, 0xA8); }  /* @bin 0x004FFCC0 wavs\gatehouse.wav */
static void panel_8a(game_state *st) { panel_text1(st, 0x166, 0x005F7AD8u, 0x9B); } /* @bin 0x004FFD90 wavs\noaccess.wav */

static int32_t tower_text(game_state *st, int32_t group, int32_t *g)
{
    /* @bin 0x004FFF2B no workers: 2; +0x2A <= 0: 4; else 3 */
    *g = 0x45;
    if (no_road(st)) {
        return 0x19;
    }
    *g = group;
    if (workers(st) < 1) {
        return 2;
    }
    if (S16(st, building(st) + B_FIELD_2A) < 1) {
        return 4;
    }
    return 3;
}

static void panel_tower(game_state *st, int32_t group)
{
    /* @bin 0x004FFE60 */
    panel_begin(st, 0x55, 0x005F7AECu, group);
    int32_t g;
    int32_t id = tower_text(st, group, &g);
    text_block(st, g, id, px(st) + 0x20, py(st) + 0x38, text_width(st), FONT_F8C);
    workers_box(st, 4, py(st) + 0x8E);
}

static void panel_3b(game_state *st) { panel_tower(st, 0x5B); } /* @bin 0x004FFE60 wavs\tower.wav */
static void panel_ac(game_state *st) { panel_tower(st, 0xA9); } /* @bin 0x00500000 wavs\tower.wav */

static void panel_ad(game_state *st)
{
    /* @bin 0x005001A0 type 0xAD (wavs\tower.wav), group 0xAA: the status at y + 0x32, text 1 at
     * y + 0xDC */
    panel_begin(st, 0x55, 0x005F7AECu, 0xAA);
    int32_t g;
    int32_t id = tower_text(st, 0xAA, &g);
    text_block(st, g, id, px(st) + 0x20, py(st) + 0x32, text_width(st), FONT_F8C);
    text_block(st, 0xAA, 1, px(st) + 0x20, py(st) + 0xDC, text_width(st), FONT_F8C);
    workers_box(st, 4, py(st) + 0x8E);
}

static void panel_5e(game_state *st)
{
    /* @bin 0x00500380 types 0x5E, 0xB9, 0xBA (wavs\mil_acad.wav), group 0x87: no workers 2; workers
     * below the type's need (i32 0x00705EB8 + 0x28 * type) 1; else 3 */
    panel_begin(st, 0x58, 0x005F7AFCu, 0x87);
    int32_t g = 0x45;
    int32_t id = 0x19;
    if (!no_road(st)) {
        g = 0x87;
        uint32_t b = building(st);
        int16_t w = S16(st, b + B_WORKERS);
        if (w < 1) {
            id = 2;
        } else if ((int32_t) w < S32(st, 0x00705EB8u + 0x28u * (uint32_t) (int32_t) S16(st, b + B_TYPE))) {
            id = 1;
        } else {
            id = 3;
        }
    }
    text_block(st, g, id, px(st) + 0x20, py(st) + 0x38, text_width(st), FONT_F8C);
    workers_box(st, 4, py(st) + 0x8E);
}

static void hundreds_line(game_state *st, uint32_t field, int32_t y, int32_t one, int32_t many)
{
    /* @bin 0x005005F2 the field / 100 (signed) with the suffix string 0x007027D4, then text `one`
     * when it is 1, else `many` */
    S32(st, TEXT_CURSOR) = 0;
    text_number(st, S16(st, building(st) + field) / 100, (int32_t) PERCENT_SUFFIX, px(st) + 0x3A, y);
    int32_t id = S16(st, building(st) + field) / 100 == 1 ? one : many;
    text_line(st, 0x88, id, S32(st, TEXT_CURSOR) + 0x3A + px(st), y, FONT_F8C);
}

static void panel_5f(game_state *st)
{
    /* @bin 0x00500530 type 0x5F (wavs\barracks.wav), group 0x88: the counts +0x3C and +0xCC in
     * hundreds, then the status by workers, 0x00EADCD0 and the workers' percentage */
    panel_begin(st, 0x58, 0x005F7B10u, 0x88);
    image(st, S32(st, PANEL_IMAGES) + 10, px(st) + 0x1E, py(st) + 0x26);
    hundreds_line(st, B_FIELD_3C, py(st) + 0x2C, 0x0F, 2);
    if (S16(st, building(st) + B_FIELD_CC) != 0) {
        image(st, S32(st, PANEL_IMAGES) + 0x1C, px(st) + 0x1E, py(st) + 0x40);
        hundreds_line(st, B_FIELD_CC, py(st) + 0x46, 0x0E, 0x0D);
    }
    int32_t g = 0x45;
    int32_t id = 0x19;
    if (!no_road(st)) {
        g = 0x88;
        uint32_t b = building(st);
        int32_t pct;
        if (S16(st, b + B_WORKERS) < 1) {
            id = 3;
        } else if (S32(st, 0x00EADCD0u) == 0) {
            id = 4;
        } else if (S16(st, b + B_FIELD_3C) < 1) {
            /* @bin 0x00500880 thresholds 100 / 0x42 / 0x21 */
            pct = S32(st, INFO_WORKERS);
            id = pct >= 100 ? 5 : pct >= 0x42 ? 6 : pct >= 0x21 ? 7 : 8;
        } else {
            pct = S32(st, INFO_WORKERS);
            id = pct >= 100 ? 9 : pct >= 0x42 ? 10 : pct >= 0x21 ? 0x0B : 0x0C;
        }
    }
    text_block(st, g, id, px(st) + 0x20, py(st) + 0x5A, text_width(st), FONT_F8C);
    workers_box(st, 4, py(st) + 0x8E);
}

#define RUN(fn) \
    static int32_t run_##fn(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }

RUN(panel_d1)
RUN(ratings)
RUN(panel_1e)
RUN(panel_1f)
RUN(panel_20)
RUN(panel_e2)
RUN(panel_21)
RUN(panel_22)
RUN(panel_23)
RUN(panel_24)
RUN(panel_25)
RUN(panel_2e)
RUN(panel_2f)
RUN(panel_ce)
RUN(panel_31)
RUN(panel_33)
RUN(panel_35)
RUN(panel_37)
RUN(panel_a7)
RUN(panel_39)
RUN(panel_3a)
RUN(panel_aa)
RUN(panel_ab)
RUN(panel_8a)
RUN(panel_3b)
RUN(panel_ac)
RUN(panel_ad)
RUN(panel_5e)
RUN(panel_5f)

const ported_function info_industry_ports[] = {
    {0x004FC820u, "info_panel_d1", 0, 0, 0, run_panel_d1, 0},
    {0x00515D10u, "info_culture_ratings", 0, 0, 0, run_ratings, 0},
    {0x004FC8E0u, "info_panel_1e", 0, 0, 0, run_panel_1e, 0},
    {0x004FCC90u, "info_panel_1f", 0, 0, 0, run_panel_1f, 0},
    {0x004FCF10u, "info_panel_20", 0, 0, 0, run_panel_20, 0},
    {0x004FD120u, "info_panel_e2", 0, 0, 0, run_panel_e2, 0},
    {0x004FD4B0u, "info_panel_21", 0, 0, 0, run_panel_21, 0},
    {0x004FDA00u, "info_panel_22", 0, 0, 0, run_panel_22, 0},
    {0x004FDC40u, "info_panel_23", 0, 0, 0, run_panel_23, 0},
    {0x004FDE80u, "info_panel_24", 0, 0, 0, run_panel_24, 0},
    {0x004FE0C0u, "info_panel_25", 0, 0, 0, run_panel_25, 0},
    {0x004FE9A0u, "info_panel_2e", 0, 0, 0, run_panel_2e, 0},
    {0x004FEB00u, "info_panel_2f", 0, 0, 0, run_panel_2f, 0},
    {0x004FED40u, "info_panel_ce", 0, 0, 0, run_panel_ce, 0},
    {0x004FEEA0u, "info_panel_31", 0, 0, 0, run_panel_31, 0},
    {0x004FF000u, "info_panel_33", 0, 0, 0, run_panel_33, 0},
    {0x004FF200u, "info_panel_35", 0, 0, 0, run_panel_35, 0},
    {0x004FF400u, "info_panel_37", 0, 0, 0, run_panel_37, 0},
    {0x004FF740u, "info_panel_a7", 0, 0, 0, run_panel_a7, 0},
    {0x004FFA20u, "info_panel_39", 0, 0, 0, run_panel_39, 0},
    {0x004FFB30u, "info_panel_3a", 0, 0, 0, run_panel_3a, 0},
    {0x004FFBF0u, "info_panel_aa", 0, 0, 0, run_panel_aa, 0},
    {0x004FFCC0u, "info_panel_ab", 0, 0, 0, run_panel_ab, 0},
    {0x004FFD90u, "info_panel_8a", 0, 0, 0, run_panel_8a, 0},
    {0x004FFE60u, "info_panel_3b", 0, 0, 0, run_panel_3b, 0},
    {0x00500000u, "info_panel_ac", 0, 0, 0, run_panel_ac, 0},
    {0x005001A0u, "info_panel_ad", 0, 0, 0, run_panel_ad, 0},
    {0x00500380u, "info_panel_5e", 0, 0, 0, run_panel_5e, 0},
    {0x00500530u, "info_panel_5f", 0, 0, 0, run_panel_5f, 0},
};
const int info_industry_ports_count = sizeof(info_industry_ports) / sizeof(info_industry_ports[0]);
