/* Imhotep - window 9: info panels (service) (package "info").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The panels of the water, health, government, religion and other service buildings: the
 * functions the info window's redraw 0x004F8E60 calls through its jump table on the building type
 * (present/ui/info/info.c), and the two per-frame draws of the per-frame dispatcher 0x004F94A0.
 * Each panel writes the help id 0x00EADD54, plays the building's sound once (0x0047D350, an empty
 * function in this build) and draws through the primitives of package "blit" (window panel
 * 0x00536820, layout texts 0x004D6250 / 0x004D6160 / 0x004D62F0 / 0x004D6860, inner panel
 * 0x004D65B0, images 0x0053E590) and the workers line 0x004FC5C0, all reached through GAME_CALL.
 * The text groups and string indices are Pharaoh_Text.eng's. Panel names follow the sound file
 * each one plays (the string at the address passed to 0x0047D350). */
#include "info.h"

#include "building/building_update.h"
#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"


/* building fields (docs/formats/save.md 5) */

static int32_t px(game_state *st) { return S32(st, G_INFO_X); }
static int32_t py(game_state *st) { return S32(st, G_INFO_Y); }
static int32_t pw(game_state *st) { return S32(st, G_INFO_W); }
static int32_t ph(game_state *st) { return S32(st, G_INFO_H); }

static int32_t blocks(int32_t n)
{
    /* shl 4 on a 32-bit register */
    return (int32_t) ((uint32_t) n << 4);
}

static uint32_t building(game_state *st)
{
    return BUILDING_ADDR(S32(st, G_INFO_BUILDING));
}

static void sound_once(game_state *st, uint32_t sound)
{
    /* @bin 0x00502F84 the building's sound on the panel's first draw (0x0047D350: name, 1, 0) */
    if (S32(st, G_INFO_SOUND) != 0) {
        GAME_CALL(st, FN_RETURN_ZERO, (int32_t) sound, 1, 0);
        S32(st, G_INFO_SOUND) = 0;
    }
}

static void window_panel(game_state *st)
{
    GAME_CALL(st, FN_PANEL_DRAW, px(st), py(st), pw(st), ph(st));
}

static void open_panel(game_state *st, int32_t help, uint32_t sound)
{
    S32(st, G_INFO_HELP) = help;
    sound_once(st, sound);
    window_panel(st);
}

static void title(game_state *st, int32_t group, int32_t id, int32_t dy)
{
    /* @bin 0x004D6250 the title, centred over the panel's width, large font */
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, group, id, px(st), py(st) + dy, blocks(pw(st)), S32(st, G_FONT_TITLE), 0,
              px(st), py(st));
}

static void centred_text(game_state *st, int32_t group, int32_t id, int32_t y)
{
    /* @bin 0x004D6250 a centred line in the text font */
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, group, id, px(st), y, blocks(pw(st)), S32(st, G_FONT_LARGE), 0, px(st), py(st));
}

static void lines(game_state *st, int32_t group, int32_t id, int32_t dx, int32_t y, int32_t wsub)
{
    /* @bin 0x004D6160 a wrapped text, (width - wsub) blocks wide */
    GAME_CALL(st, FN_LAYOUT_GROUP_LINES, group, id, px(st) + dx, y, blocks(pw(st) - wsub), S32(st, G_FONT_LARGE), 0,
              px(st), py(st));
}

static void text(game_state *st, int32_t group, int32_t id, int32_t x, int32_t y)
{
    /* @bin 0x004D62F0 a text at (x, y); the text cursor 0x00E20778 then holds its width */
    GAME_CALL(st, FN_LAYOUT_GROUP, group, id, x, y, S32(st, G_FONT_LARGE), 0, px(st), py(st));
}

static void number(game_state *st, int32_t value, uint32_t suffix, int32_t x, int32_t y)
{
    /* @bin 0x004D6860 a number with a suffix (prefix character 0x40) */
    GAME_CALL(st, FN_LAYOUT_NUMBER_AT, value, 0x40, (int32_t) suffix, x, y, S32(st, G_FONT_LARGE), 0, px(st), py(st));
}

static void count_text(game_state *st, int32_t group, int32_t id, int32_t count, int32_t x, int32_t y)
{
    /* @bin 0x004CCC30 "count string" */
    GAME_CALL(st, FN_TEXT_COUNT_DRAW, group, id, count, x, y, S32(st, G_FONT_LARGE), 0);
}

static void image(game_state *st, int32_t id, int32_t x, int32_t y)
{
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, id, x, y);
}

static void workers(game_state *st, int32_t dy, int32_t rows, int32_t dy_line)
{
    /* @bin 0x004D65B0 the inner panel (x + 16, y + dy, width - 2 blocks, rows) and the workers
     * line 0x004FC5C0 in it */
    GAME_CALL(st, FN_LAYOUT_INNER_PANEL, px(st) + 0x10, py(st) + dy, pw(st) - 2, rows, px(st), py(st));
    GAME_CALL(st, FN_INFO_WORKERS_LINE, py(st) + dy_line);
}

static void no_road(game_state *st, int32_t dy, int32_t wsub)
{
    /* @bin 0x005030C7 group 0x45 string 0x19: the building has no road access */
    lines(st, 0x45, 0x19, 0x20, py(st) + dy, wsub);
}

static int32_t road_access(game_state *st, uint32_t b)
{
    /* @bin 0x00480180 (x, y, size) */
    return GAME_CALL(st, FN_BUILDING_ROAD_ACCESS, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE));
}

static int32_t workers_text(int32_t w, int32_t at100, int32_t at75, int32_t at50, int32_t at25, int32_t below)
{
    /* the workers ladder: percent of the need >= 100, 75, 50, 25, less */
    if (w >= 0x64) {
        return at100;
    }
    if (w >= 0x4B) {
        return at75;
    }
    if (w >= 0x32) {
        return at50;
    }
    if (w >= 0x19) {
        return at25;
    }
    return below;
}

static void info_shipyard(game_state *st)
{
    /* @bin 0x00502F70 the shipyard (type 0x4A; wavs\shipyard.wav): ships built (+0x3C), the
     * request (+0xB4: kind in the top four bits, 0x1000 building, 0x2000 done), the progress */
    S32(st, G_INFO_HELP) = 0x52;
    sound_once(st, G_WAV_SHIPYARD);
    window_panel(st);
    title(st, 0x64, 0, 10);
    image(st, S32(st, G_RESOURCE_ICONS) + 0x14, px(st) + 0x1E, py(st) + 0x26);
    S32(st, G_TEXT_X) = 0;
    text(st, 0x64, 7, px(st) + 0x3A, py(st) + 0x2C);
    int16_t built = S16(st, building(st) + BLD_STORED);
    count_text(st, 8, 10, built >= 1 ? built : 0, S32(st, G_TEXT_X) + px(st) + 0x3A, py(st) + 0x2C);
    if (S32(st, G_INFO_ROAD) <= 0) {
        no_road(st, 0x3E, 4);
    } else {
        uint32_t b = building(st);
        uint16_t request = U16(st, b + BLD_SECOND_WALKER);
        uint16_t kind = request & 0xF000;
        uint16_t low = request & 0xFFF;
        if (kind == 0) {
            int32_t missing = 0;
            if (!building_missing_worker(st, &missing)) {
                lines(st, 0x64, 4, 0x20, py(st) + 0x56, 6);
            } else if (S16(st, building(st) + BLD_STORED) < 0x64 && missing != 0x19) {
                lines(st, 0x64, 9, 0x20, py(st) + 0x56, 6);
            }
        } else if (kind == 0x1000) {
            /* @bin 0x0050318C the progress in percent of 0xA0 */
            int32_t progress = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S16(st, b + BLD_SHIPYARD_PROGRESS), 0xA0);
            S32(st, G_TEXT_X) = 0;
            text(st, 0x64, 2, px(st) + 0x20, py(st) + 0x3E);
            number(st, progress, G_PERCENT_STRING, S32(st, G_TEXT_X) + px(st) + 0x20, py(st) + 0x3E);
            text(st, 0x64, 3, S32(st, G_TEXT_X) + px(st) + 0x20, py(st) + 0x3E);
            if (S16(st, building(st) + BLD_STORED) >= 0x64 || low == 0x19) {
                lines(st, 0x64, 5, 0x20, py(st) + 0x56, 6);
            } else {
                lines(st, 0x64, 9, 0x20, py(st) + 0x56, 6);
            }
        } else if (kind == 0x2000) {
            lines(st, 0x64, S16(st, b + BLD_STORED) < 1 ? 0xA : 6, 0x20, py(st) + 0x56, 6);
        }
    }
    workers(st, 0x88, 4, 0x8E);
}

static void info_dock(game_state *st)
{
    /* @bin 0x00503360 the docks (types 0x4B, 0xC0; wavs\dock.wav): by workers, with or without
     * a ship (+0xB4) */
    open_panel(st, 0x53, G_WAV_DOCK);
    title(st, 0x65, 0, 10);
    int32_t id;
    if (S32(st, G_INFO_ROAD) <= 0) {
        no_road(st, 0x38, 4);
    } else {
        int32_t w = S32(st, G_INFO_WORKERS);
        if (U16(st, building(st) + BLD_SECOND_WALKER) != 0) {
            id = w <= 0 ? 2 : w < 0x32 ? 3 : w < 0x4B ? 4 : 5;
        } else {
            id = w <= 0 ? 6 : w < 0x32 ? 7 : w < 0x4B ? 8 : 9;
        }
        lines(st, 0x65, id, 0x20, py(st) + 0x38, 4);
    }
    workers(st, 0x88, 4, 0x8E);
}

static void info_fishing_wharf(game_state *st)
{
    /* @bin 0x005035F0 the fishing wharf (type 0x4C; wavs\wharf.wav): its boat (+0xB4) by action */
    open_panel(st, 0x54, G_WAV_WHARF);
    title(st, 0x66, 0, 10);
    image(st, S32(st, G_RESOURCE_ICONS) + 7, px(st) + 10, py(st) + 10);
    if (S32(st, G_INFO_ROAD) <= 0) {
        no_road(st, 0x38, 4);
    } else {
        int32_t id;
        if (S16(st, CURRENT_CITY(st) + CITY_U28C4) != 0) {
            id = 9; /* @bin 0x005036DD city +0x28C4 */
        } else {
            int16_t boat = S16(st, building(st) + BLD_SECOND_WALKER);
            if (boat <= 0) {
                id = 2;
            } else {
                /* @bin 0x0050376F the boat's action (figure +0x38) */
                uint8_t action = U8(st, FIGURE_ADDR(boat) + FIG_ACTION);
                if (action == 9) {
                    id = 3;
                } else if (action == 0xA) {
                    id = 4;
                } else if (action == 0xB) {
                    id = 5;
                } else if (action == 0xD) {
                    id = 7;
                } else if (action == 0xC) {
                    id = S32(st, G_BLOOD_RIVER_LEFT) > 0 ? 0xA : 6; /* @bin 0x00503852 */
                } else {
                    id = 8;
                }
            }
        }
        lines(st, 0x66, id, 0x20, py(st) + 0x38, 4);
    }
    workers(st, 0x88, 4, 0x8E);
}

static void wharf_panel(game_state *st, int32_t help, int32_t group)
{
    /* @bin 0x00503940 the two wharfs of types 0xB5 / 0xB6 (wavs\wharf.wav) */
    open_panel(st, help, G_WAV_WHARF);
    title(st, group, 0, 10);
    uint32_t b = building(st);
    if (road_access(st, b) == 0) {
        no_road(st, 0x28, 4);
    }
    lines(st, group, 1, 0x20, py(st) + 0x4C, 4);
    workers(st, 0x88, 4, 0x8E);
}

static void info_wharf_b5(game_state *st)
{
    /* @bin 0x00503940 type 0xB5: help 0x165, group 0xAE */
    wharf_panel(st, 0x165, 0xAE);
}

static void info_wharf_b6(game_state *st)
{
    /* @bin 0x00503AA0 type 0xB6: help 0x164, group 0xAF */
    wharf_panel(st, 0x164, 0xAF);
}

static void info_panel_88(game_state *st)
{
    /* @bin 0x00503C00 type 0x88 (no sound): the building it serves (+0xC4), its road access and
     * employment */
    S32(st, G_INFO_HELP) = 0x3A;
    if (S32(st, G_INFO_SOUND) != 0) {
        S32(st, G_INFO_SOUND) = 0;
    }
    int32_t other = S32(st, building(st) + BLD_FERRY_PARTNER);
    window_panel(st);
    title(st, 0x9F, 0, 10);
    if (road_access(st, building(st)) == 0) {
        no_road(st, 0x3C, 4);
    } else if (other < 1) {
        lines(st, 0x9F, 1, 0x20, py(st) + 0x3C, 4);
    } else {
        uint32_t o = BUILDING_ADDR(other);
        if (road_access(st, o) == 0) {
            lines(st, 0x9F, 2, 0x20, py(st) + 0x3C, 4);
        } else if (S16(st, o + BLD_WORKERS) == 0) {
            lines(st, 0x9F, 3, 0x20, py(st) + 0x3C, 4);
        }
    }
    workers(st, 0x88, 4, 0x8E);
}

static void info_palace_frame(game_state *st)
{
    /* @bin 0x00503E10 the palaces' per-frame draw: the button frame (highlighted when the first
     * rectangle button is hovered) and its text, city +0x2C9C + 3 and the amount city +0x2C98 */
    GAME_CALL(st, FN_LAYOUT_FRAME, 0, px(st) + 0x28, py(st) + 0x3D, 0x190, 0x19, S32(st, G_HOVER) == 1, px(st),
              py(st));
    S32(st, G_TEXT_X) = 0;
    text(st, 0x67, S32(st, CURRENT_CITY(st) + CITY_SALARY_RANK) + 3, px(st) + 0x2C, py(st) + 0x41);
    number(st, S32(st, CURRENT_CITY(st) + CITY_SALARY), G_NUMBER_SUFFIX, S32(st, G_TEXT_X) + px(st) + 0x2C,
           py(st) + 0x41);
    text(st, 0x67, 2, S32(st, G_TEXT_X) + px(st) + 0x2C, py(st) + 0x41);
}

static void info_palace(game_state *st)
{
    /* @bin 0x00503F00 the palaces (types 0x4D..0x4F; wavs\gov_palace.wav): title by type, the
     * amount +0xB8 */
    open_panel(st, 0x4E, G_WAV_GOV_PALACE);
    title(st, 0x29, S16(st, building(st) + BLD_TYPE), 10);
    image(st, S32(st, G_RESOURCE_ICONS) + 0x15, px(st) + 0x10, py(st) + 0x24);
    S32(st, G_TEXT_X) = 0;
    text(st, 0x6A, 2, px(st) + 0x30, py(st) + 0x2F);
    count_text(st, 8, 0, S32(st, building(st) + BLD_UB8), S32(st, G_TEXT_X) + px(st) + 0x30, py(st) + 0x2F);
    lines(st, 0x67, 1, 0x20, blocks(ph(st)) + py(st) - 0x8F, 4);
}

static void info_engineer_post(game_state *st)
{
    /* @bin 0x00504070 type 0x51 (wavs\eng_post.wav): employed, its walker (+0x2A), workers */
    open_panel(st, 0x51, G_WAV_ENG_POST);
    title(st, 0x68, 0, 10);
    if (S32(st, G_INFO_ROAD) <= 0) {
        no_road(st, 0x38, 4);
    } else if (S16(st, building(st) + BLD_WORKERS) == 0) {
        lines(st, 0x68, 9, 0x20, py(st) + 0x38, 4);
    } else {
        lines(st, 0x68, S16(st, building(st) + BLD_WALKER) != 0 ? 2 : 3, 0x20, py(st) + 0x38, 4);
        lines(st, 0x68, workers_text(S32(st, G_INFO_WORKERS), 4, 5, 6, 7, 8), 0x20, py(st) + 0x48, 4);
    }
    workers(st, 0x88, 4, 0x8E);
}

static void info_mission_post(game_state *st)
{
    /* @bin 0x00504300 type 0x50 (wavs\mission.wav) */
    open_panel(st, 8, G_WAV_MISSION);
    title(st, 0x86, 0, 10);
    lines(st, 0x86, 1, 0x20, py(st) + 0x6A, 4);
}

static void plain_panel(game_state *st, int32_t group)
{
    /* @bin 0x005043C0 a title and one text (wavs\empty_land.wav, help 0) */
    open_panel(st, 0, G_WAV_EMPTY_LAND);
    title(st, group, 0, 10);
    lines(st, group, 1, 0x20, blocks(ph(st)) + py(st) - 0x8F, 4);
}

static void info_panel_58(game_state *st)
{
    plain_panel(st, 0x83); /* @bin 0x005043C0 type 0x58 */
}

static void info_panel_59(game_state *st)
{
    plain_panel(st, 0x84); /* @bin 0x00504490 type 0x59 */
}

static void info_panel_5d(game_state *st)
{
    plain_panel(st, 0x85); /* @bin 0x00504560 type 0x5D */
}

static void info_tax_frame(game_state *st)
{
    /* @bin 0x00504630 the tax panels' per-frame draw: the image buttons 0x005C8D38 (2) */
    GAME_CALL(st, FN_EDITOR_BUTTONS_HOVER, px(st) + 0xE2, py(st) - 0x25, (int32_t) G_TAX_BUTTONS, 2, 0xA);
    GAME_CALL(st, FN_BUTTONS_EDITOR_CLICK, px(st) + 0xE2, py(st) - 0x25, (int32_t) G_TAX_BUTTONS, 2);
}

static void tax_head(game_state *st, int32_t group, int32_t dx)
{
    /* @bin 0x00504738 the amount +0xB8 and the tax rate (city +8, group 0x3C) */
    S32(st, G_TEXT_X) = 0;
    text(st, group, 2, px(st) + dx, py(st) + 0x2B);
    count_text(st, 8, 0, S32(st, building(st) + BLD_UB8), S32(st, G_TEXT_X) + px(st) + dx, py(st) + 0x2B);
    S32(st, G_TEXT_X) = 0;
    text(st, 0x3C, 1, px(st) + 0x104, py(st) + 0x2B);
    number(st, S32(st, CURRENT_CITY(st) + CITY_TAX_RATE), G_PERCENT_STRING, S32(st, G_TEXT_X) + px(st) + 0x104,
           py(st) + 0x2B);
    GAME_CALL(st, FN_EDITOR_BUTTONS_DRAW, px(st) + 0xE2, py(st) - 0x25, (int32_t) G_TAX_BUTTONS, 2);
}

static void info_senate(game_state *st)
{
    /* @bin 0x00504680 types 0x54, 0x55, 0xBB..0xBD (wavs\senate.wav) */
    open_panel(st, 0x4D, G_WAV_SENATE);
    title(st, 0x29, S16(st, building(st) + BLD_TYPE), 10);
    image(st, S32(st, G_RESOURCE_ICONS) + 0x15, px(st) + 0x10, py(st) + 0x24);
    tax_head(st, 0x69, 0x20);
    if (S32(st, G_INFO_ROAD) <= 0) {
        no_road(st, 0x38, 4);
    }
    workers(st, 0x88, 4, 0x8E);
}

static void info_panel_b8(game_state *st)
{
    /* @bin 0x005048D0 type 0xB8 (wavs\forum.wav): the amount +0xB8, its walker (+0x2A), workers */
    open_panel(st, 0x4C, G_WAV_FORUM);
    title(st, 0xB0, 0, 10);
    S32(st, G_TEXT_X) = 0;
    text(st, 0xB0, 9, px(st) + 0x20, py(st) + 0x2B);
    count_text(st, 8, 0, S32(st, building(st) + BLD_UB8), S32(st, G_TEXT_X) + px(st) + 0x20, py(st) + 0x2B);
    if (S32(st, G_INFO_ROAD) <= 0) {
        no_road(st, 0x3F, 4);
    } else if (S16(st, building(st) + BLD_WORKERS) == 0) {
        lines(st, 0xB0, 2, 0x20, py(st) + 0x3F, 4);
    } else {
        lines(st, 0xB0, S16(st, building(st) + BLD_WALKER) != 0 ? 7 : 8, 0x20, py(st) + 0x3F, 4);
        lines(st, 0xB0, workers_text(S32(st, G_INFO_WORKERS), 6, 5, 4, 3, 2), 0x20, py(st) + 0x53, 4);
    }
    workers(st, 0x88, 4, 0x8E);
    lines(st, 0xB0, 1, 0x20, blocks(ph(st)) + py(st) - 0x71, 4);
}

static void info_forum(game_state *st)
{
    /* @bin 0x00504C20 types 0x56, 0x57 (wavs\forum.wav): the tax head, then the palace's state:
     * city +0x2790 the palace building, its employment in percent of the type's need (0x00705EB8,
     * stride 40), city +0x4486 */
    open_panel(st, 0x50, G_WAV_FORUM);
    title(st, 0x6A, 0, 10);
    image(st, S32(st, G_RESOURCE_ICONS) + 0x15, px(st) + 0x10, py(st) + 0x24);
    tax_head(st, 0x6A, 0x23);
    uint32_t palace = BUILDING_ADDR(S32(st, CURRENT_CITY(st) + CITY_PALACE_ID));
    int32_t staffed = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S16(st, palace + BLD_WORKERS),
                                S32(st, G_TYPE_WORKERS + 40u * (uint32_t) (int32_t) S16(st, palace + BLD_TYPE)));
    if (S32(st, G_INFO_ROAD) <= 0) {
        no_road(st, 0x38, 4);
    } else {
        int32_t id;
        if (S16(st, CURRENT_CITY(st) + CITY_U4486) == 0) {
            id = 0xB;
        } else if (staffed == 0) {
            id = 0xC;
        } else if (S16(st, building(st) + BLD_WORKERS) == 0) {
            id = 0xA;
        } else {
            id = workers_text(S32(st, G_INFO_WORKERS), 5, 6, 7, 8, 9);
        }
        lines(st, 0x6A, id, 0x20, py(st) + 0x48, 4);
    }
    workers(st, 0x88, 4, 0x8E);
}

static void info_waterwheel(game_state *st)
{
    /* @bin 0x00505050 types 0x07, 0x5A (wavs\waterwheel.wav): +0x35 and the employment */
    open_panel(st, 0x3B, G_WAV_WATERWHEEL);
    title(st, 0x6B, 0, 10);
    centred_text(st, 0xD, 1, blocks(ph(st)) + py(st) - 0x18);
    int32_t group, id;
    if (S32(st, G_INFO_ROAD) <= 0) {
        group = 0x45;
        id = 0x19;
    } else {
        uint32_t b = building(st);
        group = 0x6B;
        if (U8(st, b + BLD_WATER_SUPPLY) == 0) {
            id = 3;
        } else if (S16(st, b + BLD_WORKERS) > 0) {
            id = 1;
        } else {
            id = 2;
        }
    }
    lines(st, group, id, 0x20, blocks(ph(st)) + py(st) - 0xC1, 4);
    workers(st, 0x88, 4, 0x8E);
}

static void info_fountain(game_state *st)
{
    /* @bin 0x005051E0 type 0xB4 (wavs\fountain.wav): string 7 - employed */
    open_panel(st, 0x3D, G_WAV_FOUNTAIN);
    title(st, 0x6C, 0, 10);
    int32_t group, id;
    if (S32(st, G_INFO_ROAD) <= 0) {
        group = 0x45;
        id = 0x19;
    } else {
        group = 0x6C;
        id = 7 - S16(st, building(st) + BLD_WORKERS);
    }
    lines(st, group, id, 0x20, blocks(ph(st)) + py(st) - 0xC1, 4);
    workers(st, 0x88, 4, 0x8E);
}

static void info_well(game_state *st)
{
    /* @bin 0x00505320 type 0x5C (wavs\well.wav): by the area state 0x0047FD30(building, 2) */
    open_panel(st, 0x3E, G_WAV_WELL);
    title(st, 0x6D, 0, 10);
    int32_t r = GAME_CALL(st, FN_BUILDING_AREA_STATE, S32(st, G_INFO_BUILDING), 2);
    lines(st, 0x6D, r == 1 ? 2 : r == 2 ? 3 : 1, 0x20, blocks(ph(st)) + py(st) - 0x7E, 4);
}

static void info_panel_62(game_state *st)
{
    /* @bin 0x00505460 type 0x62 (wavs\oracle.wav) */
    open_panel(st, 0x43, G_WAV_ORACLE);
    title(st, 0x6E, 0, 0xC);
    centred_text(st, 0xD, 1, blocks(ph(st)) + py(st) - 0x18);
    lines(st, 0x6E, 1, 0x20, blocks(ph(st)) + py(st) - 0x8F, 4);
}

static void info_temple(game_state *st, int32_t god)
{
    /* @bin 0x00505560 the temples and shrines (argument: the god 0..4). Types 0x8C..0x95: group
     * 0xA1 string 2 god, wavs\oracle.wav, no workers panel; others: the god's group 0x5C..0x60 and
     * sound (jump table 0x005057D8), the workers panel. Then the god's picture */
    int16_t type = S16(st, building(st) + BLD_TYPE);
    int32_t rows = 0;
    int32_t group;
    if (type >= 0x8C && type <= 0x95) {
        sound_once(st, G_WAV_ORACLE);
        group = 0xA1;
    } else {
        rows = 4;
        uint32_t sound;
        switch ((uint32_t) god) {
        case 0: group = 0x5C; sound = G_WAV_TEMPLE_FARM; break;
        case 1: group = 0x5D; sound = G_WAV_TEMPLE_SHIP; break;
        case 2: group = 0x5E; sound = G_WAV_TEMPLE_COMM; break;
        case 3: group = 0x5F; sound = G_WAV_TEMPLE_WAR; break;
        default: group = 0x60; sound = G_WAV_TEMPLE_LOVE; break;
        }
        sound_once(st, sound);
    }
    int32_t x = px(st), y = py(st), w = pw(st), h = ph(st);
    S32(st, G_INFO_HELP) = 0x43;
    GAME_CALL(st, FN_PANEL_DRAW, x, y, w, h);
    type = S16(st, building(st) + BLD_TYPE);
    if (type >= 0x8C && type <= 0x95) {
        int32_t k = god < 4 ? god : 4;
        title(st, 0xA1, k + k, 0xC);
    } else {
        title(st, group, 0, 0xC);
    }
    if (rows != 0) {
        GAME_CALL(st, FN_LAYOUT_INNER_PANEL, px(st) + 0x10, py(st) + 0x38, pw(st) - 2, rows, px(st), py(st));
        GAME_CALL(st, FN_INFO_WORKERS_LINE, py(st) + 0x3E);
    }
    if (S32(st, G_INFO_ROAD) <= 0) {
        lines(st, 0x45, 0x19, 0x20, blocks(ph(st) - 8) + py(st), 4);
        return;
    }
    int32_t k = god < 4 ? god : 4;
    image(st, k + S32(st, G_GOD_IMAGES) + 0x15, px(st) + 0xBE, blocks(ph(st) + rows) + py(st) - 0xB6);
}

static void info_ruin(game_state *st)
{
    /* @bin 0x005057F0 type 0x63 (wavs\ruin.wav): group 0x29 string of the tile's terrain value */
    open_panel(st, 0, G_WAV_RUIN);
    title(st, 0x6F, 0, 10);
    text(st, 0x29, S32(st, G_INFO_TERRAIN), px(st) + 0x20, blocks(ph(st)) + py(st) - 0xAD);
    lines(st, 0x6F, 1, 0x20, blocks(ph(st)) + py(st) - 0x8F, 4);
}

static void info_theatre(game_state *st)
{
    /* @bin 0x00505A00 type 0xC7 (wavs\theatre.wav): employment and +0xA6..+0xA8 */
    open_panel(st, 8, G_WAV_THEATRE);
    title(st, 0xB3, 0, 10);
    if (S32(st, G_INFO_ROAD) <= 0) {
        no_road(st, 0x38, 4);
    } else {
        uint32_t b = building(st);
        int32_t id;
        if (S16(st, b + BLD_WORKERS) <= 0) {
            id = 2;
        } else if (U8(st, b + BLD_SHOWS) == 0) {
            id = 3;
        } else {
            /* @bin 0x00505BAD 4 + (+0xA7 != 0) + 2 (+0xA8 != 0) */
            id = 4 + (U8(st, b + BLD_UA7) != 0) + 2 * (U8(st, b + BLD_UA8) != 0);
        }
        lines(st, 0xB3, id, 0x20, py(st) + 0x38, 4);
    }
    GAME_CALL(st, FN_LAYOUT_INNER_PANEL, px(st) + 0x10, py(st) + 0x88, pw(st) - 2, 6, px(st), py(st));
    GAME_CALL(st, FN_INFO_WORKERS_LINE, py(st) + 0x8A);
    lines(st, 0xB3, 1, 0x20, blocks(ph(st)) + py(st) - 0x69, 4);
}

#define RUN0(fn) \
    static int32_t run_##fn(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }

RUN0(info_shipyard)
RUN0(info_dock)
RUN0(info_fishing_wharf)
RUN0(info_wharf_b5)
RUN0(info_wharf_b6)
RUN0(info_panel_88)
RUN0(info_palace_frame)
RUN0(info_palace)
RUN0(info_engineer_post)
RUN0(info_mission_post)
RUN0(info_panel_58)
RUN0(info_panel_59)
RUN0(info_panel_5d)
RUN0(info_tax_frame)
RUN0(info_senate)
RUN0(info_panel_b8)
RUN0(info_forum)
RUN0(info_waterwheel)
RUN0(info_fountain)
RUN0(info_well)
RUN0(info_panel_62)
RUN0(info_ruin)
RUN0(info_theatre)

static int32_t run_info_temple(game_state *st, const int32_t *a)
{
    info_temple(st, a[0]);
    return 0;
}

const ported_function info_service_ports[] = {
    {0x00502F70u, "info_shipyard", 0, 0, 0, run_info_shipyard, 0},
    {0x00503360u, "info_dock", 0, 0, 0, run_info_dock, 0},
    {0x005035F0u, "info_fishing_wharf", 0, 0, 0, run_info_fishing_wharf, 0},
    {0x00503940u, "info_wharf_b5", 0, 0, 0, run_info_wharf_b5, 0},
    {0x00503AA0u, "info_wharf_b6", 0, 0, 0, run_info_wharf_b6, 0},
    {0x00503C00u, "info_panel_88", 0, 0, 0, run_info_panel_88, 0},
    {0x00503E10u, "info_palace_frame", 0, 0, 0, run_info_palace_frame, 0},
    {0x00503F00u, "info_palace", 0, 0, 0, run_info_palace, 0},
    {0x00504070u, "info_engineer_post", 0, 0, 0, run_info_engineer_post, 0},
    {0x00504300u, "info_mission_post", 0, 0, 0, run_info_mission_post, 0},
    {0x005043C0u, "info_panel_58", 0, 0, 0, run_info_panel_58, 0},
    {0x00504490u, "info_panel_59", 0, 0, 0, run_info_panel_59, 0},
    {0x00504560u, "info_panel_5d", 0, 0, 0, run_info_panel_5d, 0},
    {0x00504630u, "info_tax_frame", 0, 0, 0, run_info_tax_frame, 0},
    {0x00504680u, "info_senate", 0, 0, 0, run_info_senate, 0},
    {0x005048D0u, "info_panel_b8", 0, 0, 0, run_info_panel_b8, 0},
    {0x00504C20u, "info_forum", 0, 0, 0, run_info_forum, 0},
    {0x00505050u, "info_waterwheel", 0, 0, 0, run_info_waterwheel, 0},
    {0x005051E0u, "info_fountain", 0, 0, 0, run_info_fountain, 0},
    {0x00505320u, "info_well", 0, 0, 0, run_info_well, 0},
    {0x00505460u, "info_panel_62", 0, 0, 0, run_info_panel_62, 0},
    {0x00505560u, "info_temple", 1, 0, 0, run_info_temple, 0},
    {0x005057F0u, "info_ruin", 0, 0, 0, run_info_ruin, 0},
    {0x00505A00u, "info_theatre", 0, 0, 0, run_info_theatre, 0},
};
const int info_service_ports_count = sizeof(info_service_ports) / sizeof(info_service_ports[0]);
