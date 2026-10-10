/* Imhotep - window 9: info panels of the raw-material producers, workshops and guilds (package "info").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The panels of building types 0x6C..0x73, 0xB1..0xB3, 0xCB..0xCD, 0xE7..0xE9 (reached from the
 * redraw 0x004F8E60 by its jump table). They share one layout, read from each function's
 * disassembly: the help id, the building's sound on the first draw, the panel, an icon and the
 * title, a production line (progress), one or two input stock lines (icon, text, amount), a
 * status text chosen by a ladder (no road access, production halted by the city, no workers,
 * input missing, the workers' percentage), the inner panel and the workers line 0x004FC5C0.
 * Each function is ported as its own entry point calling the shared layout with its constants. */
#include "info.h"

#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"


enum { PROGRESS_NONE, PROGRESS_RAW, PROGRESS_SCALED };

typedef struct {
    int32_t help;         /* 0x00EADD54 */
    uint32_t sound;       /* the wav path (state address) given to 0x0047D350 */
    int32_t icon;         /* PANEL_ICONS + icon at (x + 10, y + 10); -1 none */
    int32_t group;        /* the text group of the panel */
    int progress;         /* PROGRESS_*: building +0x3C as is, or 0x004CDE60(building +0x54, divisor) */
    int32_t divisor;
    int32_t progress_y;   /* y offset of the production line */
    int clear_before_in1; /* 0x00E20778 = 0 before the first input's icon (0x0050DBE0) */
    int32_t in1_icon;     /* first input: icon (-1: no input line), text index; amount building +0x3C */
    int32_t in1_text;
    int32_t in2_icon;     /* second input: icon (-1: none), text index; amount building +0xCC */
    int32_t in2_text;
    uint32_t halted;      /* city offset of the i16 "production halted" flag; 0: no check */
    int check_in1;        /* the status ladder tests the first input's stock (+0x3C) */
    int check_in2;        /* ... and the second's (+0xCC) */
    int32_t status_base;  /* text index of "no workers"; the others follow it (see status_text) */
    int32_t status_y;
    int trailer;          /* the text 1 at the panel's bottom */
} panel_desc;

static int32_t x0(game_state *st) { return S32(st, G_INFO_X); }
static int32_t y0(game_state *st) { return S32(st, G_INFO_Y); }
static int32_t font_text(game_state *st) { return S32(st, G_FONT_LARGE); }

static int16_t building_i16(game_state *st, uint32_t offset)
{
    return S16(st, BUILDING_ADDR(S32(st, G_INFO_BUILDING)) + offset);
}

static void draw_icon(game_state *st, int32_t icon, int32_t dx, int32_t dy)
{
    /* @bin 0x0053E590 thiscall on 0x00F55BB0 (image, x, y) */
    int32_t y = y0(st) + dy;
    int32_t x = x0(st) + dx;
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, S32(st, G_RESOURCE_ICONS) + icon, x, y);
}

static void text_at(game_state *st, int32_t group, int32_t index, int32_t x, int32_t y)
{
    /* @bin 0x004D62F0 (group, index, x, y, font, colour 0, origin x, origin y) */
    GAME_CALL(st, FN_LAYOUT_GROUP, group, index, x, y, font_text(st), 0, x0(st), y0(st));
}

static void input_line(game_state *st, const panel_desc *d, int32_t icon, int32_t text, uint32_t stock,
                       int32_t icon_dx, int32_t text_dx)
{
    /* @bin 0x0050B2B7 an input: its icon at (icon_dx, 0x38), its name at (text_dx, 0x3C) after
     * clearing the text cursor, then the amount (below 1 shown as 0) through 0x004CCC30(8, 10, ...) */
    draw_icon(st, icon, icon_dx, 0x38);
    S32(st, G_TEXT_X) = 0;
    text_at(st, d->group, text, x0(st) + text_dx, y0(st) + 0x3C);
    int16_t amount = building_i16(st, stock);
    int32_t value = amount < 1 ? 0 : amount;
    GAME_CALL(st, FN_TEXT_COUNT_DRAW, 8, 10, value, S32(st, G_TEXT_X) + text_dx + x0(st), y0(st) + 0x3C, font_text(st), 0);
}

static int32_t status_text(game_state *st, const panel_desc *d, int32_t *group)
{
    /* @bin 0x00509E1C the status ladder: no road access (group 0x45 text 0x19); production halted
     * (index 4); no workers (building +0x40 <= 0); input missing (+0x3C, +0xCC <= 0); then the
     * workers' percentage 0x00EADD5C: >= 100, >= 75, >= 50, >= 25, below. The indices follow
     * status_base (5: 5, 0xB, 0xC, 6..10; 2 for 0x0050DBE0: 2, 8, 9, 3..7) */
    if (S32(st, G_INFO_ROAD) <= 0) {
        *group = 0x45;
        return 0x19;
    }
    *group = d->group;
    if (d->halted != 0 && S16(st, CURRENT_CITY(st) + d->halted) != 0) {
        return 4;
    }
    int32_t b = d->status_base;
    if (building_i16(st, 0x40) <= 0) {
        return b;
    }
    if (d->check_in1 && building_i16(st, 0x3C) <= 0) {
        return b + 6;
    }
    if (d->check_in2 && building_i16(st, 0xCC) <= 0) {
        return b + 7;
    }
    int32_t pct = S32(st, G_INFO_WORKERS);
    if (pct >= 100) {
        return b + 1;
    }
    if (pct >= 75) {
        return b + 2;
    }
    if (pct >= 50) {
        return b + 3;
    }
    if (pct >= 25) {
        return b + 4;
    }
    return b + 5;
}

static void panel_draw(game_state *st, const panel_desc *d)
{
    S32(st, G_INFO_HELP) = d->help;
    if (S32(st, G_INFO_SOUND) != 0) {
        GAME_CALL(st, FN_RETURN_ZERO, (int32_t) d->sound, 1, 0); /* @bin 0x0047D350 empty (sound) */
        S32(st, G_INFO_SOUND) = 0;
    }
    GAME_CALL(st, FN_PANEL_DRAW, x0(st), y0(st), S32(st, G_INFO_W), S32(st, G_INFO_H));
    if (d->icon >= 0) {
        draw_icon(st, d->icon, 10, 10);
    }
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, d->group, 0, x0(st), y0(st) + 10, (int32_t) ((uint32_t) S32(st, G_INFO_W) << 4),
              S32(st, G_FONT_TITLE), 0, x0(st), y0(st));
    if (d->progress != PROGRESS_NONE) {
        int32_t value;
        uint32_t suffix;
        if (d->progress == PROGRESS_RAW) {
            /* @bin 0x00509D76 the raw producers: building +0x3C as is */
            S32(st, G_TEXT_X) = 0;
            value = building_i16(st, 0x3C);
            suffix = G_GOAL_SEPARATOR;
        } else {
            /* @bin 0x0050B205 the workshops: 0x004CDE60(building +0x54, divisor) */
            value = GAME_CALL(st, FN_CALC_RATIO_PERCENT, (int32_t) building_i16(st, 0x54), d->divisor);
            S32(st, G_TEXT_X) = 0;
            suffix = G_PERCENT_STRING;
        }
        int32_t y = y0(st) + d->progress_y;
        text_at(st, d->group, 2, x0(st) + 0x20, y);
        GAME_CALL(st, FN_LAYOUT_NUMBER_AT, value, 0x40, (int32_t) suffix, S32(st, G_TEXT_X) + x0(st) + 0x20, y0(st) + d->progress_y,
                  font_text(st), 0, x0(st), y0(st));
        text_at(st, d->group, 3, S32(st, G_TEXT_X) + x0(st) + 0x20, y0(st) + d->progress_y);
    }
    if (d->clear_before_in1) {
        S32(st, G_TEXT_X) = 0;
    }
    if (d->in1_icon >= 0) {
        input_line(st, d, d->in1_icon, d->in1_text, 0x3C, 0x20, 0x3C);
    }
    if (d->in2_icon >= 0) {
        input_line(st, d, d->in2_icon, d->in2_text, 0xCC, 0xE6, 0x104);
    }
    int32_t group;
    int32_t index = status_text(st, d, &group);
    int32_t width = (int32_t) ((uint32_t) (S32(st, G_INFO_W) - 4) << 4);
    GAME_CALL(st, FN_LAYOUT_GROUP_LINES, group, index, x0(st) + 0x20, y0(st) + d->status_y, width, font_text(st), 0, x0(st),
              y0(st));
    GAME_CALL(st, FN_LAYOUT_INNER_PANEL, x0(st) + 0x10, y0(st) + 0x88, S32(st, G_INFO_W) - 2, 4, x0(st), y0(st));
    GAME_CALL(st, FN_INFO_WORKERS_LINE, y0(st) + 0x8E);
    if (d->trailer) {
        /* @bin 0x0050A018 text 1 at the bottom: y + 16 h - 0x71 */
        int32_t y = (int32_t) ((uint32_t) S32(st, G_INFO_H) << 4) + y0(st) - 0x71;
        width = (int32_t) ((uint32_t) (S32(st, G_INFO_W) - 4) << 4);
        GAME_CALL(st, FN_LAYOUT_GROUP_LINES, d->group, 1, x0(st) + 0x20, y, width, font_text(st), 0, x0(st), y0(st));
    }
}

/* The constants of each panel, from its disassembly (help id, wav string, icons, text group, halted
 * flag's city offset, ladder) */

/* @bin 0x00509CD0 type 0x6C ("wavs\\timber.wav") */
static const panel_desc P_509CD0 = {0x5E, G_WAV_TIMBER, 0x14, 0x78, PROGRESS_RAW, 0, 0x2C, 0, -1, 0, -1, 0,
                                    0x28DE, 0, 0, 5, 0x46, 1};
/* @bin 0x0050A060 type 0x6D ("wavs\\clay.wav") */
static const panel_desc P_50A060 = {0x5C, G_WAV_CLAY, 0x0B, 0x79, PROGRESS_SCALED, 200, 0x2C, 0, -1, 0, -1, 0,
                                    0x28CC, 0, 0, 5, 0x46, 1};
/* @bin 0x0050A400 type 0x6E ("wavs\\wine_workshop.wav") */
static const panel_desc P_50A400 = {0x60, G_WAV_WINE_WORKSHOP, 0x0F, 0x7A, PROGRESS_SCALED, 400, 0x28, 0, 0x0E, 0x0C, -1, 0,
                                    0x28D4, 1, 0, 5, 0x56, 0};
/* @bin 0x0050A870 type 0x6F ("wavs\\oil_workshop.wav") */
static const panel_desc P_50A870 = {0x3C, G_WAV_OIL_WORKSHOP, 0x11, 0x7B, PROGRESS_SCALED, 400, 0x28, 0, 0x10, 0x0C, -1, 0,
                                    0x28D8, 1, 0, 5, 0x56, 0};
/* @bin 0x0050ACE0 type 0xCB ("wavs\\oil_workshop.wav") */
static const panel_desc P_50ACE0 = {0x61, G_WAV_OIL_WORKSHOP, 0x17, 0xBE, PROGRESS_SCALED, 400, 0x28, 0, 0x16, 0x0C, -1, 0,
                                    0x28E4, 1, 0, 5, 0x56, 0};
/* @bin 0x0050B160 type 0xE8 ("wavs\\lamp_maker.wav") */
static const panel_desc P_50B160 = {0x1D9, G_WAV_LAMP_MAKER, 0x22, 0x13A, PROGRESS_SCALED, 400, 0x28, 0, 0x1F, 0x0D, 0x0D,
                                    0x0E, 0x28FA, 1, 1, 5, 0x56, 0};
/* @bin 0x0050B6D0 type 0xE9 ("wavs\\paint_maker.wav") */
static const panel_desc P_50B6D0 = {0x1D6, G_WAV_PAINT_MAKER, 0x21, 0x139, PROGRESS_SCALED, 400, 0x28, 0, 0x20, 0x0C, -1, 0,
                                    0x28F8, 1, 0, 5, 0x56, 0};
/* @bin 0x0050BB50 type 0xCD ("wavs\\oil_workshop.wav") */
static const panel_desc P_50BB50 = {0x62, G_WAV_OIL_WORKSHOP, 0x1C, 0xB9, PROGRESS_SCALED, 400, 0x28, 0, 0x14, 0x0C, -1, 0,
                                    0x28DE, 1, 0, 5, 0x56, 0};
/* @bin 0x0050BFD0 type 0xCC ("wavs\\oil_workshop.wav") */
static const panel_desc P_50BFD0 = {0x16C, G_WAV_OIL_WORKSHOP, 0x0C, 0xB4, PROGRESS_SCALED, 400, 0x28, 0, 0x0B, 0x0D, 0x09,
                                    0x0E, 0x28CE, 1, 1, 5, 0x56, 0};
/* @bin 0x0050C540 type 0x70 ("wavs\\weapons_workshop.wav") */
static const panel_desc P_50C540 = {0x62, G_WAV_WEAPONS_WORKSHOP, 0x0A, 0x7C, PROGRESS_SCALED, 400, 0x28, 0, 0x1D, 0x0C, -1, 0,
                                    0x28CA, 1, 0, 5, 0x56, 0};
/* @bin 0x0050C9B0 type 0x71 ("wavs\\furniture_workshop.wav") */
static const panel_desc P_50C9B0 = {0x63, G_WAV_FURNITURE_WORKSHOP, 0x13, 0x7D, PROGRESS_SCALED, 400, 0x28, 0, 0x12, 0x0C, -1, 0,
                                    0x28DC, 1, 0, 5, 0x56, 0};
/* @bin 0x0050CE20 type 0x72 ("wavs\\pottery_workshop.wav") */
static const panel_desc P_50CE20 = {0x01, G_WAV_POTTERY_WORKSHOP, 0x0D, 0x7E, PROGRESS_SCALED, 400, 0x28, 0, 0x0B, 0x0C, -1, 0,
                                    0x28D0, 1, 0, 5, 0x56, 0};
/* @bin 0x0050D290 type 0xB1 ("wavs\\carpenter_lodge.wav"): no halted flag */
static const panel_desc P_50D290 = {0x16B, G_WAV_CARPENTER_LODGE, 0x14, 0xAB, PROGRESS_SCALED, 400, 0x28, 0, 0x14, 0x0C, -1, 0,
                                    0, 1, 0, 5, 0x56, 0};
/* @bin 0x0050D6C0 type 0xB2 ("wavs\\bricklayer_lodge.wav"): no production or input line */
static const panel_desc P_50D6C0 = {0x16B, G_WAV_BRICKLAYER_LODGE, 0x0C, 0xAC, PROGRESS_NONE, 0, 0, 0, -1, 0, -1, 0,
                                    0, 0, 0, 5, 0x56, 0};
/* @bin 0x0050D930 type 0xB3 ("wavs\\bricklayer_lodge.wav"): no input line, but its stock tested */
static const panel_desc P_50D930 = {0x16B, G_WAV_BRICKLAYER_LODGE, 0x18, 0xAD, PROGRESS_NONE, 0, 0, 0, -1, 0, -1, 0,
                                    0, 1, 0, 5, 0x56, 0};
/* @bin 0x0050DBE0 type 0xE7 ("wavs\\artisans_guild.wav"): no title icon, the ladder from 2 */
static const panel_desc P_50DBE0 = {0x1D8, G_WAV_ARTISANS_GUILD, -1, 0x138, PROGRESS_NONE, 0, 0, 1, 0x21, 0x0A, 0x0B, 0x0B,
                                    0, 1, 1, 2, 0x56, 0};
/* @bin 0x0050E030 type 0x73 ("wavs\\pottery_workshop.wav"): no title icon */
static const panel_desc P_50E030 = {0x167, G_WAV_POTTERY_WORKSHOP, -1, 0x9A, PROGRESS_NONE, 0, 0, 0, 0x08, 0x0D, -1, 0,
                                    0x28C6, 1, 0, 5, 0x56, 0};

#define PANEL(addr) \
    static int32_t run_##addr(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        panel_draw(st, &P_##addr); \
        return 0; \
    }
PANEL(509CD0)
PANEL(50A060)
PANEL(50A400)
PANEL(50A870)
PANEL(50ACE0)
PANEL(50B160)
PANEL(50B6D0)
PANEL(50BB50)
PANEL(50BFD0)
PANEL(50C540)
PANEL(50C9B0)
PANEL(50CE20)
PANEL(50D290)
PANEL(50D6C0)
PANEL(50D930)
PANEL(50DBE0)
PANEL(50E030)

const ported_function info_service3_ports[] = {
    {0x00509CD0u, "info_panel_type6c", 0, 0, 0, run_509CD0, 0},
    {0x0050A060u, "info_panel_type6d", 0, 0, 0, run_50A060, 0},
    {0x0050A400u, "info_panel_type6e", 0, 0, 0, run_50A400, 0},
    {0x0050A870u, "info_panel_type6f", 0, 0, 0, run_50A870, 0},
    {0x0050ACE0u, "info_panel_typecb", 0, 0, 0, run_50ACE0, 0},
    {0x0050B160u, "info_panel_typee8", 0, 0, 0, run_50B160, 0},
    {0x0050B6D0u, "info_panel_typee9", 0, 0, 0, run_50B6D0, 0},
    {0x0050BB50u, "info_panel_typecd", 0, 0, 0, run_50BB50, 0},
    {0x0050BFD0u, "info_panel_typecc", 0, 0, 0, run_50BFD0, 0},
    {0x0050C540u, "info_panel_type70", 0, 0, 0, run_50C540, 0},
    {0x0050C9B0u, "info_panel_type71", 0, 0, 0, run_50C9B0, 0},
    {0x0050CE20u, "info_panel_type72", 0, 0, 0, run_50CE20, 0},
    {0x0050D290u, "info_panel_typeb1", 0, 0, 0, run_50D290, 0},
    {0x0050D6C0u, "info_panel_typeb2", 0, 0, 0, run_50D6C0, 0},
    {0x0050D930u, "info_panel_typeb3", 0, 0, 0, run_50D930, 0},
    {0x0050DBE0u, "info_panel_typee7", 0, 0, 0, run_50DBE0, 0},
    {0x0050E030u, "info_panel_type73", 0, 0, 0, run_50E030, 0},
};
const int info_service3_ports_count = sizeof(info_service3_ports) / sizeof(info_service3_ports[0]);
