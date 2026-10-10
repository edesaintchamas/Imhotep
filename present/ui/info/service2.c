/* Imhotep - window 9: info panels (service2) (package "info").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The panels of the building types 0x64-0x6B, 0xA1, 0xA2, 0xC2-0xC4, 0xD8, 0xD9, 0xDD, 0xE0
 * (0x004F8E60's jump table): farms (0x00505DC0 ... 0x00508120, with the shared farm lines
 * 0x00505C40), and the producers drawn with a resource icon, a stock line and a production line
 * (0x00507D30, 0x005085B0, 0x00508A60, 0x00508E00, 0x005091C0, 0x00509570, 0x00509920).
 * Every panel is the same sequence of draws with its own constants: the help id it writes, the
 * sound it plays on the first draw, its resource (the icon 0x00F571FC + r and the city's flag
 * +0x28B6 + 2r of that resource), its text group. Each function's constants are read from its own
 * disassembly (tags below). Every call leaves this file through GAME_CALL. */
#include "info.h"

#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"


static int32_t ix(game_state *st) { return S32(st, G_INFO_X); }
static int32_t iy(game_state *st) { return S32(st, G_INFO_Y); }
static int32_t iw(game_state *st) { return S32(st, G_INFO_W); }
static int32_t ih(game_state *st) { return S32(st, G_INFO_H); }
static int32_t font_text(game_state *st) { return S32(st, G_FONT_LARGE); }

static void text_at(game_state *st, int32_t group, int32_t id, int32_t x, int32_t y)
{
    /* @bin 0x004D62F0 a text (group, id) at (x, y) in the panel's frame */
    GAME_CALL(st, FN_LAYOUT_GROUP, group, id, x, y, font_text(st), 0, ix(st), iy(st));
}

static void number_at(game_state *st, int32_t value, int32_t x, int32_t y)
{
    /* @bin 0x004D6860 a number with the empty suffix (prefix character 0x40) */
    GAME_CALL(st, FN_LAYOUT_NUMBER_AT, value, 0x40, (int32_t) G_PERCENT_STRING, x, y, font_text(st), 0, ix(st), iy(st));
}

static void paragraph(game_state *st, int32_t group, int32_t id, int32_t y)
{
    /* @bin 0x004D6160 a text wrapped in the panel's width less 4 blocks, at x + 0x20 */
    GAME_CALL(st, FN_LAYOUT_GROUP_LINES, group, id, ix(st) + 0x20, y, (iw(st) - 4) * 16, font_text(st), 0, ix(st), iy(st));
}

static void header(game_state *st, int32_t help, uint32_t sound, int32_t icon, int32_t group)
{
    /* @bin 0x00505DC0 the panels' start: the help id, the sound once (0x0047D350, a no-op),
     * the window panel, the icon at (x + 10, y + 10), the title (group, 0) centred */
    S32(st, G_INFO_HELP) = help;
    if (S32(st, G_INFO_SOUND) != 0) {
        GAME_CALL(st, FN_RETURN_ZERO, (int32_t) sound, 1, 0);
        S32(st, G_INFO_SOUND) = 0;
    }
    GAME_CALL(st, FN_PANEL_DRAW, ix(st), iy(st), iw(st), ih(st));
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, S32(st, G_RESOURCE_ICONS) + icon, ix(st) + 10, iy(st) + 10);
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, group, 0, ix(st), iy(st) + 10, iw(st) << 4, S32(st, G_FONT_TITLE), 0, ix(st),
              iy(st));
}

static void line_progress(game_state *st, int32_t group, int32_t value)
{
    /* @bin 0x00505E9C the progress line at y + 0x2C: (group, 2) value (group, 3) */
    S32(st, G_TEXT_X) = 0;
    text_at(st, group, 2, ix(st) + 0x20, iy(st) + 0x2C);
    number_at(st, value, S32(st, G_TEXT_X) + 0x20 + ix(st), iy(st) + 0x2C);
    text_at(st, group, 3, S32(st, G_TEXT_X) + 0x20 + ix(st), iy(st) + 0x2C);
}

static void line_stock(game_state *st, int32_t text_group, int32_t text_id, int32_t y)
{
    /* @bin 0x00507E1D the stock line at y: (group, id) at x + 0x3C, then 0x004CCC30(8, 10, the
     * building's +0x3C (i16, 0 when below 1), ...) after it */
    S32(st, G_TEXT_X) = 0;
    text_at(st, text_group, text_id, ix(st) + 0x3C, y);
    int16_t stock = S16(st, BUILDING_ADDR(S32(st, G_INFO_BUILDING)) + BLD_STORED);
    int32_t value = stock < 1 ? 0 : (int32_t) stock;
    GAME_CALL(st, FN_TEXT_COUNT_DRAW, 8, 10, value, S32(st, G_TEXT_X) + 0x3C + ix(st), y, font_text(st), 0);
}

static void status(game_state *st, int32_t group, int32_t resource, int byte_check)
{
    /* @bin 0x00505FB5 the status text at y + 0x46: (0x45, 0x19) without road access; else
     * (group, 4) when the city's flag of the resource is set; (group, 0xB) when the building's
     * byte +0xAD is above 4 (farms only); (group, 5) without workers (+0x40 i16 below 1); then by
     * the workers' percent 0x00EADD5C: 6 (>= 100), 7 (>= 75), 8 (>= 50), 9 (>= 25), 10 */
    int32_t y = iy(st) + 0x46;
    if (S32(st, G_INFO_ROAD) <= 0) {
        paragraph(st, 0x45, 0x19, y);
        return;
    }
    int32_t id;
    uint32_t b = BUILDING_ADDR(S32(st, G_INFO_BUILDING));
    if (S16(st, CURRENT_CITY(st) + CITY_U28B6 + 2u * (uint32_t) resource) != 0) {
        id = 4;
    } else if (byte_check && U8(st, b + BLD_UAD) > 4) {
        id = 0xB;
    } else if (S16(st, b + BLD_WORKERS) <= 0) {
        id = 5;
    } else {
        int32_t workers = S32(st, G_INFO_WORKERS);
        id = workers >= 100 ? 6 : workers >= 0x4B ? 7 : workers >= 0x32 ? 8 : workers >= 0x19 ? 9 : 10;
    }
    paragraph(st, group, id, y);
}

static void footer(game_state *st, uint32_t workers_line)
{
    /* @bin 0x005061B7 the inner panel at (x + 0x10, y + 0x88), w - 2 blocks by 4, the workers line
     * at y + 0x8E (0x004FC4F0 for farms, 0x004FC5C0 for the others) */
    GAME_CALL(st, FN_LAYOUT_INNER_PANEL, ix(st) + 0x10, iy(st) + 0x88, iw(st) - 2, 4, ix(st), iy(st));
    GAME_CALL(st, workers_line, iy(st) + 0x8E);
}

static void closing(game_state *st, int32_t group)
{
    /* @bin 0x005061FC the panel's last text (group, 1) at the bottom (y + 16 h - 0x71) */
    paragraph(st, group, 1, ih(st) * 16 - 0x71 + iy(st));
}

/* ---- 0x00505C40: the farms' flood lines ---------------------------------------------------- */

static void farm_lines(game_state *st, int32_t group)
{
    /* @bin 0x00505C40 the farm's lines at the bottom: when 0x00489240 (thiscall 0x00BCA350, the
     * building) is >= 0, (0xB1, 2) if 0x004894A0 is true else (group, 0xE), then (0xA0, that
     * value), at y + 16 h - 0x9E; when 0x00488080 is true, (0xB1, 0) if 0x004890F0 is true else
     * (0xB1, 1), at y + 16 h - 0x8F */
    int32_t v = GAME_CALLT(st, G_BUILDING_MANAGER, FN_FARM_NEXT_HARVEST, S32(st, G_INFO_BUILDING));
    if (v >= 0) {
        S32(st, G_TEXT_X) = 0;
        int ok = (GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_AREA_TEST_3X3, S32(st, G_INFO_BUILDING)) & 0xFF) != 0;
        int32_t y = ih(st) * 16 - 0x9E + iy(st);
        if (ok) {
            text_at(st, 0xB1, 2, ix(st) + S32(st, G_TEXT_X) + 0x20, y);
        } else {
            text_at(st, group, 0xE, ix(st) + S32(st, G_TEXT_X) + 0x20, y);
        }
        text_at(st, 0xA0, v, ix(st) + S32(st, G_TEXT_X) + 0x20, ih(st) * 16 - 0x9E + iy(st));
    }
    if ((GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_TYPE_488080, S32(st, G_INFO_BUILDING)) & 0xFF) != 0) {
        S32(st, G_TEXT_X) = 0;
        int ok = (GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_ROAD_FLAG_4890F0, S32(st, G_INFO_BUILDING)) & 0xFF) != 0;
        text_at(st, 0xB1, ok ? 0 : 1, ix(st) + S32(st, G_TEXT_X) + 0x20, ih(st) * 16 - 0x8F + iy(st));
    }
}

/* ---- farms ------------------------------------------------------------------------------------ */

static void farm(game_state *st, int32_t help, uint32_t sound, int32_t resource, int32_t group)
{
    /* @bin 0x00505DC0 a farm: the progress line (0x004883D0) and the second line (0x70, 0xC)
     * 0x00489110 (0x70, 0xD) on the same row, the status, the workers, the farm lines */
    header(st, help, sound, resource, group);
    int32_t progress = GAME_CALLT(st, G_BUILDING_MANAGER, FN_FARM_GROWTH_PCT, S32(st, G_INFO_BUILDING));
    line_progress(st, group, progress);
    text_at(st, 0x70, 0xC, S32(st, G_TEXT_X) + 0x20 + ix(st), iy(st) + 0x2C);
    /* @bin 0x00505F5C the number's x is read before 0x00489110 is called */
    int32_t x = S32(st, G_TEXT_X) + 0x20 + ix(st);
    int32_t value = GAME_CALLT(st, G_BUILDING_MANAGER, FN_FARM_FERTILITY_489110, S32(st, G_INFO_BUILDING));
    number_at(st, value, x, iy(st) + 0x2C);
    text_at(st, 0x70, 0xD, S32(st, G_TEXT_X) + 0x20 + ix(st), iy(st) + 0x2C);
    status(st, group, resource, 1);
    footer(st, FN_INFO_WORKERS_LINE_OR_CLOSED);
    farm_lines(st, group);
    closing(st, group);
}

/* @bin 0x00505DC0 type 0x64: help 0x59, sound 0x005F7CB8, resource 0xE, group 0xB5 */
static void info_farm_00505DC0(game_state *st) { farm(st, 0x59, G_WAV_WHEAT_FARM, 0xE, 0xB5); }
/* @bin 0x00506250 type 0x66: help 0x59, sound 0x005F7CB8, resource 1, group 0x70 */
static void info_farm_00506250(game_state *st) { farm(st, 0x59, G_WAV_WHEAT_FARM, 1, 0x70); }
/* @bin 0x005066C0 type 0x67: help 0x5A, sound 0x005F7CCC, resource 3, group 0x71 */
static void info_farm_005066C0(game_state *st) { farm(st, 0x5A, G_WAV_VEG_FARM, 3, 0x71); }
/* @bin 0x00506B30 type 0xC4: help 0x5A, sound 0x005F7CE0, resource 6, group 0xB7 */
static void info_farm_00506B30(game_state *st) { farm(st, 0x5A, G_WAV_FIGS_FARM, 6, 0xB7); }
/* @bin 0x00506FC0 type 0x68: help 0x5A, sound 0x005F7CE0, resource 5, group 0x72 */
static void info_farm_00506FC0(game_state *st) { farm(st, 0x5A, G_WAV_FIGS_FARM, 5, 0x72); }
/* @bin 0x00507430 type 0x65: help 0x5B, sound 0x005F7CF4, resource 0x10, group 0x73 */
static void info_farm_00507430(game_state *st) { farm(st, 0x5B, G_WAV_OLIVES_FARM, 0x10, 0x73); }
/* @bin 0x005078A0 type 0xE0: help 0x5B, sound 0x005F7CF4, resource 0x20, group 0x132 */
static void info_farm_005078A0(game_state *st) { farm(st, 0x5B, G_WAV_OLIVES_FARM, 0x20, 0x132); }
/* @bin 0x00508120 type 0x69: help 0x5A, sound 0x005F7D0C, resource 4, group 0xB6 */
static void info_farm_00508120(game_state *st) { farm(st, 0x5A, G_WAV_VINES_FARM, 4, 0xB6); }

/* ---- other producers --------------------------------------------------------------------------- */

static void info_0x00507D30(game_state *st)
{
    /* @bin 0x00507D30 type 0xC3: help 0x5E, sound 0x005F7D0C, resource 0x16, group 0x74; a second
     * icon at (x + 0x20, y + 0x32) and the stock line (0x74, 2) at y + 0x36 */
    header(st, 0x5E, G_WAV_VINES_FARM, 0x16, 0x74);
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, S32(st, G_RESOURCE_ICONS) + 0x16, ix(st) + 0x20, iy(st) + 0x32);
    line_stock(st, 0x74, 2, iy(st) + 0x36);
    status(st, 0x74, 0x16, 1);
    footer(st, FN_INFO_WORKERS_LINE);
    closing(st, 0x74);
}

static void info_0x005085B0(game_state *st)
{
    /* @bin 0x005085B0 type 0xC2: help 0x168, sound 0x005F7D20, resource 2, group 0x75; the progress
     * line 0x004CDE60(+0x54, 400) with the farm texts (0x70, 2 / 3), the icon of resource 9 at
     * (x + 0x20, y + 0x60) and the stock line (0x17, 9) at y + 0x64 */
    header(st, 0x168, G_WAV_MEAT_FARM, 2, 0x75);
    int16_t progress = S16(st, BUILDING_ADDR(S32(st, G_INFO_BUILDING)) + BLD_INDUSTRY_PROGRESS);
    int32_t percent = GAME_CALL(st, FN_CALC_RATIO_PERCENT, (int32_t) progress, 400);
    line_progress(st, 0x70, percent);
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, S32(st, G_RESOURCE_ICONS) + 9, ix(st) + 0x20, iy(st) + 0x60);
    line_stock(st, 0x17, 9, iy(st) + 0x64);
    status(st, 0x75, 2, 1);
    footer(st, FN_INFO_WORKERS_LINE);
    closing(st, 0x75);
}

static void raw(game_state *st, int32_t help, uint32_t sound, int32_t resource, int32_t group)
{
    /* @bin 0x00508A60 a raw-material producer: the progress line 0x004CDE60(+0x54, 200), the status
     * (no +0xAD test), the workers 0x004FC5C0 */
    header(st, help, sound, resource, group);
    int16_t progress = S16(st, BUILDING_ADDR(S32(st, G_INFO_BUILDING)) + BLD_INDUSTRY_PROGRESS);
    int32_t percent = GAME_CALL(st, FN_CALC_RATIO_PERCENT, (int32_t) progress, 200);
    line_progress(st, group, percent);
    status(st, group, resource, 0);
    footer(st, FN_INFO_WORKERS_LINE);
    closing(st, group);
}

/* @bin 0x00508A60 type 0x6A: help 0x5F, sound 0x005F7D34, resource 0x18, group 0x76 */
static void info_raw_00508A60(game_state *st) { raw(st, 0x5F, G_WAV_QUARRY, 0x18, 0x76); }

static void info_raw_00508E00(game_state *st, int32_t resource)
{
    /* @bin 0x00508E00 types 0x6B, 0xD8, 0xDD (resource 0x19, 0x1A, 0x1E): help 0x5F, sound
     * 0x005F7D34, group 0x77 for resource 0x19, else 0xC0 for 0x1A, else 0xC2 (0x00508E12 neg / sbb) */
    int32_t group = resource == 0x19 ? 0x77 : resource != 0x1A ? 0xC2 : 0xC0;
    raw(st, 0x5F, G_WAV_QUARRY, resource, group);
}

/* @bin 0x005091C0 type 0xA1: help 0x5D, sound 0x005F7D44, resource 0x15, group 0xA2 */
static void info_raw_005091C0(game_state *st) { raw(st, 0x5D, G_WAV_MINE, 0x15, 0xA2); }
/* @bin 0x00509570 type 0xD9: help 0x5D, sound 0x005F7D44, resource 0x1D, group 0xC1 */
static void info_raw_00509570(game_state *st) { raw(st, 0x5D, G_WAV_MINE, 0x1D, 0xC1); }
/* @bin 0x00509920 type 0xA2: help 0x169, sound 0x005F7D44, resource 0x12, group 0xA3 */
static void info_raw_00509920(game_state *st) { raw(st, 0x169, G_WAV_MINE, 0x12, 0xA3); }

/* ---- the table ----------------------------------------------------------------------------------- */

#define RUN0(f) \
    static int32_t run_##f(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        f(st); \
        return 0; \
    }
RUN0(info_farm_00505DC0)
RUN0(info_farm_00506250)
RUN0(info_farm_005066C0)
RUN0(info_farm_00506B30)
RUN0(info_farm_00506FC0)
RUN0(info_farm_00507430)
RUN0(info_farm_005078A0)
RUN0(info_0x00507D30)
RUN0(info_farm_00508120)
RUN0(info_0x005085B0)
RUN0(info_raw_00508A60)
RUN0(info_raw_005091C0)
RUN0(info_raw_00509570)
RUN0(info_raw_00509920)

static int32_t run_farm_lines(game_state *st, const int32_t *a)
{
    farm_lines(st, a[0]);
    return 0;
}

static int32_t run_info_raw_00508E00(game_state *st, const int32_t *a)
{
    info_raw_00508E00(st, a[0]);
    return 0;
}

const ported_function info_service2_ports[] = {
    {0x00505C40u, "info_farm_lines", 1, 0, 0, run_farm_lines, 0},
    {0x00505DC0u, "info_farm_00505DC0", 0, 0, 0, run_info_farm_00505DC0, 0},
    {0x00506250u, "info_farm_00506250", 0, 0, 0, run_info_farm_00506250, 0},
    {0x005066C0u, "info_farm_005066C0", 0, 0, 0, run_info_farm_005066C0, 0},
    {0x00506B30u, "info_farm_00506B30", 0, 0, 0, run_info_farm_00506B30, 0},
    {0x00506FC0u, "info_farm_00506FC0", 0, 0, 0, run_info_farm_00506FC0, 0},
    {0x00507430u, "info_farm_00507430", 0, 0, 0, run_info_farm_00507430, 0},
    {0x005078A0u, "info_farm_005078A0", 0, 0, 0, run_info_farm_005078A0, 0},
    {0x00507D30u, "info_0x00507D30", 0, 0, 0, run_info_0x00507D30, 0},
    {0x00508120u, "info_farm_00508120", 0, 0, 0, run_info_farm_00508120, 0},
    {0x005085B0u, "info_0x005085B0", 0, 0, 0, run_info_0x005085B0, 0},
    {0x00508A60u, "info_raw_00508A60", 0, 0, 0, run_info_raw_00508A60, 0},
    {0x00508E00u, "info_raw_00508E00", 1, 0, 0, run_info_raw_00508E00, 0},
    {0x005091C0u, "info_raw_005091C0", 0, 0, 0, run_info_raw_005091C0, 0},
    {0x00509570u, "info_raw_00509570", 0, 0, 0, run_info_raw_00509570, 0},
    {0x00509920u, "info_raw_00509920", 0, 0, 0, run_info_raw_00509920, 0},
};
const int info_service2_ports_count = sizeof(info_service2_ports) / sizeof(info_service2_ports[0]);
