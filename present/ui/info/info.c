/* Imhotep - window 9: the info window's full redraw (package "info").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x004F8E60 redraws the city view, plays the building's sound on the first draw, then calls the
 * panel of the window's kind: terrain / figures, formations, ships, or the building type's panel
 * (a jump table on the type, 0x004F93B0 / 0x004F920C). The panels are other functions of this
 * package (present/ui/info/), reached through GAME_CALL while they are not ported. */
#include "info.h"

#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"
#include "state/records.h"
#include "city/monument_rating.h"

#define SOUND_OBJECT 0x00E92BF8u   /* `this` of the sample play 0x0055F0E8 */
#define SOUND_REQUEST 0x00E92DA8u  /* the request block: owner, sound, volume, pan, loops */
#define SOUND_ON 0x00E38E63u       /* u8 sound effects enabled */
#define SOUND_VOLUME 0x00E38EC4u   /* i32 the effects' volume */
#define BUILDING_SOUNDS 0x00F5467Cu /* i32 per type (stride 0x14): the sound of the type's panel, -1: none */

static int16_t building_type(game_state *st, int32_t b)
{
    return S16(st, BUILDING_ADDR(b) + 0x10);
}

static uint32_t panel_of_type(int32_t type, int32_t *arg)
{
    /* @bin 0x004F8F7B the jump table on type - 7 (0..0xE5; index bytes 0x004F93B0, targets
     * 0x004F920C): the panel function of each building type, 0 for the types with none */
    *arg = -1;
    switch (type) {
    case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0E: case 0x0F: case 0x10: case 0x11: case 0x12:
    case 0x13: case 0x14: case 0x15: case 0x16: case 0x17: case 0x18: case 0x19: case 0x1A: case 0x1B:
    case 0x1C: case 0x1D:
        return 0x0050E570u;
    case 0x1E: return 0x004FC8E0u;
    case 0x1F: return 0x004FCC90u;
    case 0x20: return 0x004FCF10u;
    case 0xE2: return 0x004FD120u;
    case 0x21: return 0x004FD4B0u;
    case 0xD1: return 0x004FC820u;
    case 0x22: return 0x004FDA00u;
    case 0x23: return 0x004FDC40u;
    case 0x24: return 0x004FDE80u;
    case 0x25: return 0x004FE0C0u;
    case 0x29: case 0x2A: case 0x2B: return 0x004FE800u;
    case 0x2E: return 0x004FE9A0u;
    case 0x2F: return 0x004FEB00u;
    case 0xCE: return 0x004FED40u;
    case 0x31: return 0x004FEEA0u;
    case 0x33: return 0x004FF000u;
    case 0x35: return 0x004FF200u;
    case 0x37: return 0x004FF400u;
    case 0x38: return 0x004FE8D0u;
    case 0x39: return 0x004FFA20u;
    case 0x3A: case 0xC8: return 0x004FFB30u;
    case 0xAA: case 0xC9: return 0x004FFBF0u;
    case 0xAB: case 0xCA: return 0x004FFCC0u;
    case 0x8A: return 0x004FFD90u;
    case 0x3B: return 0x004FFE60u;
    case 0xAC: return 0x00500000u;
    case 0xAD: return 0x005001A0u;
    case 0x5E: case 0xB9: case 0xBA: return 0x00500380u;
    case 0x5F: return 0x00500530u;
    case 0x3C: case 0x41: case 0x8C: *arg = 0; return 0x00505560u;
    case 0x3D: case 0x42: case 0x8D: *arg = 1; return 0x00505560u;
    case 0x3E: case 0x43: case 0x8E: *arg = 2; return 0x00505560u;
    case 0x3F: case 0x44: case 0x8F: *arg = 3; return 0x00505560u;
    case 0x40: case 0x45: case 0x90: *arg = 4; return 0x00505560u;
    case 0x46: case 0xBE: *arg = -2; return 0x005013E0u; /* orders 0x005013E0, else 0x00500A40 */
    case 0x47: case 0xBF: *arg = -2; return 0x00501AD0u; /* orders 0x00501AD0, else 0x005014A0 */
    case 0x48: case 0xC1: *arg = -2; return 0x005028F0u; /* orders 0x005028F0, else 0x00501FD0 */
    case 0x4A: return 0x00502F70u;
    case 0x4B: case 0xC0: return 0x00503360u;
    case 0x4C: return 0x005035F0u;
    case 0xB5: return 0x00503940u;
    case 0xB6: return 0x00503AA0u;
    case 0x88: return 0x00503C00u;
    case 0x4D: case 0x4E: case 0x4F: return 0x00503F00u;
    case 0x51: return 0x00504070u;
    case 0x50: return 0x00504300u;
    case 0x58: return 0x005043C0u;
    case 0x59: return 0x00504490u;
    case 0x5D: return 0x00504560u;
    case 0x54: case 0x55: case 0xBB: case 0xBC: case 0xBD: return 0x00504680u;
    case 0x56: case 0x57: return 0x00504C20u;
    case 0xB8: return 0x005048D0u;
    case 0x07: case 0x5A: return 0x00505050u;
    case 0x5C: return 0x00505320u;
    case 0xB4: return 0x005051E0u;
    case 0x62: return 0x00505460u;
    case 0x63: return 0x005057F0u;
    case 0x64: return 0x00505DC0u;
    case 0xC3: return 0x00507D30u;
    case 0x65: return 0x00507430u;
    case 0xE0: return 0x005078A0u;
    case 0x66: return 0x00506250u;
    case 0x68: return 0x00506FC0u;
    case 0x67: return 0x005066C0u;
    case 0x69: return 0x00508120u;
    case 0xC4: return 0x00506B30u;
    case 0xC2: return 0x005085B0u;
    case 0x6A: return 0x00508A60u;
    case 0xDD: *arg = 0x1E; return 0x00508E00u;
    case 0x6B: *arg = 0x19; return 0x00508E00u;
    case 0xD8: *arg = 0x1A; return 0x00508E00u;
    case 0xA1: return 0x005091C0u;
    case 0xD9: return 0x00509570u;
    case 0xA2: return 0x00509920u;
    case 0x6C: return 0x00509CD0u;
    case 0x6D: return 0x0050A060u;
    case 0x6E: return 0x0050A400u;
    case 0x6F: return 0x0050A870u;
    case 0xCB: return 0x0050ACE0u;
    case 0xE8: return 0x0050B160u;
    case 0xE9: return 0x0050B6D0u;
    case 0xCD: return 0x0050BB50u;
    case 0xCC: return 0x0050BFD0u;
    case 0x70: return 0x0050C540u;
    case 0x71: return 0x0050C9B0u;
    case 0x72: return 0x0050CE20u;
    case 0xB1: return 0x0050D290u;
    case 0xB2: return 0x0050D6C0u;
    case 0xB3: return 0x0050D930u;
    case 0xE7: return 0x0050DBE0u;
    case 0x73: return 0x0050E030u;
    case 0xA7: return 0x004FF740u;
    case 0xB7: return 0x00511040u;
    case 0xC7: return 0x00505A00u;
    case 0xD2: return 0x00511C80u;
    case 0xCF: case 0xD0: return 0x00512140u;
    case 0xD5: return 0x005124A0u;
    case 0xE1: return 0x00512BE0u;
    case 0xE6: return 0x005132A0u;
    case 0xE3: return 0x005135E0u;
    case 0xE4: return 0x00514400u;
    case 0xDE: return 0x00514A40u;
    case 0xE5: case 0xEA: case 0xEB: case 0xEC: return 0x00513D40u;
    default:
        return 0;
    }
}

void info_draw(game_state *st)
{
    /* @bin 0x004F8E60 the info window's full redraw */
    GAME_CALL0(st, 0x004DDDC0u); /* the city view */
    int32_t b = S32(st, INFO_BUILDING);
    int32_t sound_type = building_type(st, b);
    if (sound_type == 0x39) {
        /* @bin 0x004F8E81 the shrines-like type 0x39 by its +0x12 kind: 0xD -> 0x28, 0xB -> 0x2C, 0xC -> 0x2D */
        int16_t kind = S16(st, BUILDING_ADDR(b) + 0x12);
        if (kind == 0x0D) {
            sound_type = 0x28;
        }
        if (kind == 0x0B) {
            sound_type = 0x2C;
        }
        if (kind == 0x0C) {
            sound_type = 0x2D;
        }
    }
    int32_t sound = S32(st, BUILDING_SOUNDS + 0x14u * (uint32_t) sound_type);
    if (sound > -1 && S32(st, INFO_SOUND) != 0 && U8(st, SOUND_ON) != 0 && S32(st, INFO_KIND) == 2) {
        /* @bin 0x004F8ED7 the request: owner -1, the sound, the effects' volume, pan 0x40, once */
        S32(st, SOUND_REQUEST + 8) = S32(st, SOUND_VOLUME);
        S32(st, SOUND_REQUEST + 12) = 0x40;
        S32(st, SOUND_REQUEST + 4) = sound;
        S32(st, SOUND_REQUEST) = -1;
        S32(st, SOUND_REQUEST + 16) = 1;
        GAME_CALLT(st, SOUND_OBJECT, 0x0055F0E8u, (int32_t) SOUND_REQUEST);
    }
    int32_t kind = S32(st, INFO_KIND);
    if (kind == 0) {
        GAME_CALL0(st, 0x004FA4A0u);
        return;
    }
    if (kind == 1) {
        GAME_CALL0(st, 0x004FA530u);
        return;
    }
    if (kind == 4) {
        GAME_CALL(st, 0x005101B0u, S32(st, INFO_FORMATION));
        return;
    }
    if (kind == 6) {
        GAME_CALL(st, 0x0050F320u, S32(st, INFO_FORMATION));
        return;
    }
    if (kind == 5) {
        GAME_CALL(st, 0x0050FCA0u, S32(st, INFO_FORMATION));
        return;
    }
    if (kind != 2) {
        return;
    }
    int32_t arg;
    uint32_t panel = panel_of_type(building_type(st, S32(st, INFO_BUILDING)), &arg);
    if (panel == 0) {
        return;
    }
    if (arg == -2) {
        /* @bin 0x004F9074 storage buildings: the orders sub-panel when 0x00EADCC0 is set */
        if (S32(st, INFO_ORDERS) == 0) {
            panel = panel == 0x005013E0u ? 0x00500A40u : panel == 0x00501AD0u ? 0x005014A0u : 0x00501FD0u;
        }
        GAME_CALL0(st, panel);
    } else if (arg >= 0) {
        GAME_CALL(st, panel, arg);
    } else {
        GAME_CALL0(st, panel);
    }
}

#define FONT_SMALL_TEXT 0x00F56F94u /* i32 the font of the workers line */
#define SIDE_IMAGES 0x00F5684Cu     /* i32 first image of the panels' icons (+0x0E the workers icon) */
#define EMPTY_STRING 0x0057C048u
#define TEXT_CURSOR 0x00E20778u     /* i32 x after the last text drawn */
#define TYPE_WORKERS 0x00705EB8u    /* i32 per type (stride 0x28): the workers the type needs */

void info_workers_line(game_state *st, int32_t y)
{
    /* @bin 0x004FC5C0 the workers line of a building panel at y: the icon, "N employees (of M
     * needed)" and, when the building lacks workers or labour access, the reason line of group
     * 0x45: 0x10 no population, 0x11 no labour access (+0x18 <= 0), 0x12 / 0x13 poor access (+0x18 >= 0x28;
     * 0x13 when the building's labour category +0x42 has workers, 0x00EA43A0 + 0x14 category), 0x14
     * the access is too far (+0x18 < 0x28) */
    int32_t font = S32(st, FONT_SMALL_TEXT);
    uint32_t b = BUILDING_ADDR(S32(st, INFO_BUILDING));
    int16_t type = S16(st, b + 0x10);
    int32_t reason;
    if (S32(st, TYPE_WORKERS + 0x28u * (uint32_t) type) > S16(st, b + 0x40)) {
        uint32_t city = CURRENT_CITY(st);
        int16_t access = S16(st, b + 0x18);
        if (S32(st, city + 0x24) <= 0) {
            reason = 0x10;
        } else if (access <= 0) {
            reason = 0x11;
        } else if (access < 0x28) {
            reason = 0x14;
        } else {
            reason = 0x12 + (S32(st, city + 0x2B30 + 0x14u * U8(st, b + 0x42)) > 0);
        }
    } else {
        reason = S16(st, b + 0x18) >= 0x28 ? 0 : 0x14;
    }
    GAME_CALLT(st, 0x00F55BB0u, 0x0053E590u, S32(st, SIDE_IMAGES) + 0xE, S32(st, INFO_X) + 0x28, y + 6);
    S32(st, TEXT_CURSOR) = 0;
    int32_t line = reason != 0 ? y + 0xA : y + 0x10;
    GAME_CALL(st, 0x004CCC30u, 8, 0xC, (int32_t) S16(st, BUILDING_ADDR(S32(st, INFO_BUILDING)) + 0x40),
              S32(st, INFO_X) + 0x3C, line, font, 0);
    int32_t x = S32(st, INFO_X);
    GAME_CALL(st, 0x004D6860u,
              S32(st, TYPE_WORKERS + 0x28u * (uint32_t) S16(st, BUILDING_ADDR(S32(st, INFO_BUILDING)) + 0x10)),
              0x28, (int32_t) EMPTY_STRING, S32(st, TEXT_CURSOR) + x + 0x46, line, font, 0, x, S32(st, INFO_Y));
    x = S32(st, INFO_X);
    GAME_CALL(st, 0x004D62F0u, 0x45, 0, S32(st, TEXT_CURSOR) + x + 0x46, line, font, 0, x, S32(st, INFO_Y));
    if (reason == 0) {
        return;
    }
    x = S32(st, INFO_X);
    S32(st, TEXT_CURSOR) = 0;
    GAME_CALL(st, 0x004D6160u, 0x45, reason, x + 0x46, y + 0x1A, 400, font, 0, x, S32(st, INFO_Y));
}

void info_workers_line_or_closed(game_state *st, int32_t y)
{
    /* @bin 0x004FC4F0 when the building is closed (0x004894A0, thiscall on 0x00BCA350): group 0xB1
     * text 5 (no workers, +0x40 = 0) or 6, and the workers icon; else the workers line */
    int32_t font = S32(st, FONT_SMALL_TEXT);
    if ((uint8_t) GAME_CALLT(st, 0x00BCA350u, 0x004894A0u, S32(st, INFO_BUILDING)) == 0) {
        info_workers_line(st, y);
        return;
    }
    int32_t text = S16(st, BUILDING_ADDR(S32(st, INFO_BUILDING)) + 0x40) == 0 ? 5 : 6;
    int32_t x = S32(st, INFO_X);
    GAME_CALL(st, 0x004D6160u, 0xB1, text, x + 0x41, y + 0xA, (S32(st, INFO_W) - 4) << 4, font, 0, x,
              S32(st, INFO_Y));
    GAME_CALLT(st, 0x00F55BB0u, 0x0053E590u, S32(st, SIDE_IMAGES) + 0xE, S32(st, INFO_X) + 0x28, y + 6);
}

void info_frame(game_state *st)
{
    /* @bin 0x004F94A0 the info window's per-frame draw: the buttons' highlights */
    int32_t kind = S32(st, INFO_KIND);
    if (kind == 0) {
        return;
    }
    if (kind == 1) {
        if (S32(st, INFO_FIGURE_VIEW) == 0) {
            return;
        }
        int32_t f = S32(st, INFO_FIGURES + 4u * (uint32_t) S32(st, INFO_FIGURE_ROW));
        switch (U8(st, FIGURE_ADDR(f) + 0x0A)) {
        /* @bin 0x004F94E4 the figure types of the jump table 0x004F9674 / 0x004F966C that have the button */
        case 0x05: case 0x07: case 0x0A: case 0x0F: case 0x10: case 0x11: case 0x12: case 0x1B: case 0x1D:
        case 0x1E: case 0x1F: case 0x20: case 0x22: case 0x23: case 0x27: case 0x28: case 0x57: case 0x58:
        case 0x59: case 0x69:
            break;
        default:
            return;
        }
        /* @bin 0x004F94FF the first of the 8 entries 0x00F55BA4 (but 0x00EADCDC) above 1 */
        for (int32_t i = 0;; i++) {
            if (i != S32(st, 0x00EADCDCu) && U8(st, 0x00F55BA4u + (uint32_t) i) > 1) {
                break;
            }
            if (i + 1 >= 8) {
                return;
            }
        }
        int32_t x = S32(st, INFO_X), y = S32(st, INFO_Y);
        GAME_CALL(st, 0x004D63D0u, 0, x + 0x11D, y + 0xC3, 0x96, 0x19, S32(st, INFO_BUTTON_HOVER2) == 1, x, y);
        y = S32(st, INFO_Y);
        x = S32(st, INFO_X);
        GAME_CALL(st, 0x004D6250u, 0x12, 7, x + 0x11D, y + 0xC8, 0x96, S32(st, FONT_SMALL_TEXT), 0, x, y);
        return;
    }
    if (kind == 4) {
        GAME_CALL(st, 0x00510B00u, S32(st, INFO_FORMATION));
        return;
    }
    if (kind != 2) {
        return;
    }
    int16_t type = building_type(st, S32(st, INFO_BUILDING));
    if (type >= 0x0A && type <= 0x1D) {
        return;
    }
    if (type == 0x47 || type == 0xBF) {
        GAME_CALL0(st, S32(st, INFO_ORDERS) != 0 ? 0x00501B60u : 0x00501A40u);
    } else if (type == 0x48 || type == 0xC1) {
        GAME_CALL0(st, S32(st, INFO_ORDERS) != 0 ? 0x00502990u : 0x005026F0u);
    } else if (type == 0x46 || type == 0xBE) {
        GAME_CALL0(st, S32(st, INFO_ORDERS) != 0 ? 0x005011D0u : 0x00501140u);
    } else if (type == 0xBB || type == 0xBC || type == 0xBD || type == 0x56 || type == 0x57) {
        GAME_CALL0(st, 0x00504630u);
    } else if (type == 0x4D || type == 0x4E) {
        GAME_CALL0(st, 0x00503E10u);
    }
}

#define EDITOR 0x00605A3Cu          /* i32 1 in the map editor */
#define MESSAGE_ROWS 0x00E950D8u    /* i32 the message list's rows (> 0: the click resets them) */
#define MOUSE_TILE 0x00BCA2C4u      /* i32 grid offset under the mouse (0x00BCA2C8 x, 0x00BCA2CC y) */
#define MAP_START 0x00BCA2D8u
#define WINDOW_ID 0x00E38DDAu
#define REDRAW_ALL 0x00E38DCAu
#define GRID_BUILDING 0x00B4E6FCu   /* i16 per tile */
#define GRID_FIGURE 0x00B350DCu     /* i16 per tile: the first figure */
#define GRID_TERRAIN 0x00AF598Cu    /* u32 per tile: terrain flags */

static void open_monument_status(game_state *st, int16_t type, int32_t b)
{
    /* @bin 0x004F7FB7 the monuments' status records, computed for the panel: (function, `this`,
     * the record) by type */
    switch (type) {
    case 0xB7: {
        /* 0x004F1310 writes its 0x70-byte record (all of it: it clears it first); its port
         * (core/city/monument_rating.c) has no wrapper: called directly, the record copied */
        int32_t rec[0x1C];
        pyramid_status(st, b, rec);
        for (uint32_t i = 0; i < 0x1C; i++) {
            S32(st, 0x00EADC48u + 4 * i) = rec[i];
        }
        break;
    }
    case 0xD5: GAME_CALL(st, 0x00542AD0u, b, 0x00EADC10); break;
    case 0xE1: GAME_CALLT(st, 0x006FD5D8u, 0x0040D940u, b, 0x00EADBD8); break;
    case 0xE6: GAME_CALLT(st, 0x006DE650u, 0x00401540u, b, 0x00EADBC8); break;
    case 0xE3: GAME_CALLT(st, 0x006FD368u, 0x00417050u, b, 0x00EADBA8); break;
    case 0xE4: GAME_CALLT(st, 0x006DE548u, 0x004E6710u, b, 0x00EADB88); break;
    case 0xDE: GAME_CALL(st, 0x004DF040u, b, 0x00EADB58); break;
    case 0xE5: GAME_CALLT(st, 0x006F9320u, 0x00545740u, b, 0x00EADB38); break;
    case 0xEA: GAME_CALLT(st, 0x006F5648u, 0x00558BE0u, b, 0x00EADB38); break;
    case 0xEB: GAME_CALLT(st, 0x006DE9E0u, 0x00555E80u, b, 0x00EADB38); break;
    case 0xEC: GAME_CALLT(st, 0x006E9188u, 0x0054F470u, b, 0x00EADB38); break;
    default: break;
    }
}

static int32_t open_terrain_kind(game_state *st, int32_t tile, int32_t b)
{
    /* @bin 0x004F810D the text of a tile without a building (0x00EADD4C), from its terrain flags
     * (0x00AF598C), the 0x00B285CC byte (bit 0x80), the ferry tiles 0x00BC96C4 / 0x00BC96C0; -1: the
     * flags name none (the kind stays), -2: a building (kind 2). Order of the tests as the original's */
    uint32_t f = U32(st, GRID_TERRAIN + 4u * (uint32_t) tile);
    if (U8(st, 0x00B285CCu + (uint32_t) tile) & 0x80) {
        int32_t k = -1;
        if (f & 0x40) {
            k = 0xD;
        }
        if (f & 2) {
            k = 5;
        }
        if (f & 0x2000000) {
            k = 0x16;
        }
        return k;
    }
    if (f & 1) {
        return 1;
    }
    if ((f & 2) && S16(st, BUILDING_ADDR(b) + 0x10) != 0xE4) {
        if (tile == S32(st, 0x00BC96C4u)) {
            return 0xE;
        }
        if (tile == S32(st, 0x00BC96C0u)) {
            return 0xF;
        }
        uint32_t m = f & 0x20300002u;
        if (m == 0x100002u) {
            return 0x10;
        }
        if (m == 2) {
            return 0x11;
        }
        return m != 0x20000002u ? 2 : 0x1A;
    }
    if ((f & 4) && (f & 0x10008u) == 0) {
        return 3;
    }
    if (f & 0x10) {
        return 4;
    }
    if (f & 0x20) {
        return 0xC;
    }
    if ((f & 0x40) && (f & 8) == 0) {
        return 6;
    }
    if (f & 0x100) {
        return 7;
    }
    if (f & 0x1000) {
        return 8;
    }
    if (f & 0x40000) {
        return 0x15;
    }
    if (f & 0x2000000) {
        return 0x16;
    }
    uint32_t w = f & 0xC04000u;
    if (w == 0x4000u) {
        return 9;
    }
    if (w == 0x404000u) {
        return 0x17;
    }
    if (w == 0x804000u) {
        return 0x18;
    }
    if ((f & 0x10004u) == 0x10000u && (f & 8) == 0) {
        return 0x13;
    }
    if ((f & 0x800) && (f & 8) == 0) {
        return 0x19;
    }
    if ((f & 0x10000) && (f & 8) == 0) {
        return 0x14;
    }
    return b == 0 ? 0xA : -2;
}

void info_house_worst_neighbour(game_state *st)
{
    /* @bin 0x0050F0F0 a house's worst neighbour within 6 tiles (the map's size 0x00BCA2D0 /
     * 0x00BCA2D4 bounds the square): among the buildings in use other than the house that are not
     * houses (+5) or are houses of a lower type, those of negative desirability (type table
     * 0x00705EA8 value, +8 step, +0xC range, stride 0x28) in range (the larger of the distances
     * 0x00D3B4FC in x, 0x00D3B4F8 in y): the lowest value + (distance - 1) * step below 0 goes to
     * 0x00EADCD8, its building to 0x00EADCD4 (a fort yard 0x36: its main part, 0x00475B50) */
    uint32_t h = BUILDING_ADDR(S32(st, INFO_BUILDING));
    int32_t x = S16(st, h + 6), y = S16(st, h + 8);
    int32_t x1 = x - 6, x2 = x + 6, y1 = y - 6, y2 = y + 6;
    S32(st, 0x00EADCD8u) = 0;
    S32(st, 0x00EADCD4u) = 0;
    if (x1 < 0) {
        x1 = 0;
    }
    if (y1 < 0) {
        y1 = 0;
    }
    if (x2 >= S32(st, 0x00BCA2D0u)) {
        x2 = S32(st, 0x00BCA2D0u) - 1;
    }
    if (y2 >= S32(st, 0x00BCA2D4u)) {
        y2 = S32(st, 0x00BCA2D4u) - 1;
    }
    for (int32_t row = y1; row <= y2; row++) {
        for (int32_t col = x1; col <= x2; col++) {
            uint32_t tile = U32(st, MAP_START) + (uint32_t) row * GRID_SIZE + (uint32_t) col;
            int32_t b2 = S16(st, GRID_BUILDING + 2u * tile);
            int32_t b = S32(st, INFO_BUILDING);
            if (b2 <= 0 || U8(st, BUILDING_ADDR(b2)) != 1 || b2 == b) {
                continue;
            }
            uint32_t a2 = BUILDING_ADDR(b2);
            if (U8(st, a2 + 5) != 0 && S16(st, a2 + 0x10) >= S16(st, BUILDING_ADDR(b) + 0x10)) {
                continue;
            }
            int16_t t2 = S16(st, a2 + 0x10);
            uint32_t d = 0x00705EA8u + 0x28u * (uint32_t) (int32_t) t2;
            int32_t value = S32(st, d);
            if (value >= 0) {
                continue;
            }
            int32_t step = S32(st, d + 8), range = S32(st, d + 0xC);
            int32_t by = S16(st, BUILDING_ADDR(b) + 8), bx = S16(st, BUILDING_ADDR(b) + 6);
            int32_t dx = col > bx ? col - bx : col < bx ? bx - col : 0;
            S32(st, 0x00D3B4FCu) = dx;
            int32_t dy = row > by ? row - by : row < by ? by - row : 0;
            S32(st, 0x00D3B4F8u) = dy;
            int32_t dist = dx >= dy ? dx : dy;
            if (dist > range) {
                continue;
            }
            if (dist > 1) {
                value += (dist - 1) * step;
            }
            if (value >= S32(st, 0x00EADCD8u)) {
                continue;
            }
            S32(st, 0x00EADCD8u) = value;
            S32(st, 0x00EADCD4u) = t2 == 0x36 ? GAME_CALL(st, 0x00475B50u, b2) : b2;
        }
    }
}

static void open_building(game_state *st)
{
    /* @bin 0x004F8343 a building: kind 2, its workers in percent, the building the panel shows
     * (main parts, the fort of a 0x36 tile), its road access 0x00EADD58 */
    S32(st, INFO_KIND) = 2;
    uint32_t a = BUILDING_ADDR(S32(st, INFO_BUILDING));
    S32(st, INFO_WORKERS) = GAME_CALL(st, 0x004CDE60u, (int32_t) S16(st, a + 0x40),
                                      S32(st, TYPE_WORKERS + 0x28u * (uint32_t) (int32_t) S16(st, a + 0x10)));
    int32_t b = S32(st, INFO_BUILDING);
    a = BUILDING_ADDR(b);
    int16_t type = S16(st, a + 0x10);
    int main_part = 0;
    if (type == 0x36) {
        /* @bin 0x004F839A the tile of a fort's yard: the fort (+0x38) */
        b = S16(st, a + 0x38);
        S32(st, INFO_BUILDING) = b;
        a = BUILDING_ADDR(b);
        S32(st, INFO_TILE) = S32(st, a + 0x0C);
        S32(st, INFO_TILE_X) = S16(st, a + 6);
        S32(st, INFO_TILE_Y) = S16(st, a + 8);
        S32(st, INFO_FORMATION) = S16(st, a + 0x50);
    } else if (type == 0x39) {
        S32(st, INFO_FORMATION) = S16(st, a + 0x50);
    } else if (type == 0x49 || type == 0x20 || type == 0xE2 || type == 0xD2 || type == 0x1F || type == 0x1E ||
               type == 0x21) {
        main_part = 1;
    } else if (U8(st, a + 5) != 0) {
        /* @bin 0x004F8452 a house (+5): the evolve texts */
        info_house_worst_neighbour(st);
        GAME_CALL(st, 0x00454190u, S32(st, INFO_BUILDING));
        b = S32(st, INFO_BUILDING);
    } else if (type == 0x5F) {
        S32(st, 0x00EADCD0u) = info_any_legion_ready(st) + S32(st, 0x00BCB9ACu);
        b = S32(st, INFO_BUILDING);
    } else if ((type >= 0x41 && type <= 0x45) || (type >= 0x9C && type <= 0xA0)) {
        main_part = 1;
    }
    if (main_part) {
        b = GAME_CALL(st, 0x00475B50u, b);
        S32(st, INFO_BUILDING) = b;
        a = BUILDING_ADDR(b);
        S32(st, INFO_TILE) = S32(st, a + 0x0C);
        S32(st, INFO_TILE_X) = S16(st, a + 6);
        S32(st, INFO_TILE_Y) = S16(st, a + 8);
    }
    S32(st, INFO_ROAD) = 0;
    a = BUILDING_ADDR(b);
    type = S16(st, a + 0x10);
    int32_t x = S16(st, a + 6), y = S16(st, a + 8);
    int32_t road;
    if (type == 0x47 || type == 0xBF) {
        road = GAME_CALL(st, 0x004806A0u, x, y);
    } else if (type == 0x48 || type == 0xC1) {
        /* @bin 0x004F85D4 storage yards: the access, the yard's fullness 0x00EADCB8 (0x0045D570),
         * the list scroll +0xE0 := 1 when the rows hold the whole list */
        if (GAME_CALL(st, 0x00480180u, x, y, 3) != 0) {
            S32(st, INFO_ROAD) = 1;
        }
        S32(st, 0x00EADCB8u) = info_yard_fullness(st, S32(st, INFO_BUILDING));
        if (GAME_CALL0(st, 0x00502780u) >= S32(st, STORAGE_LIST)) {
            S32(st, BUILDING_ADDR(S32(st, INFO_BUILDING)) + 0xE0) = 1;
        }
        return;
    } else if (type >= 0x8C && type <= 0x95) {
        road = GAME_CALL(st, 0x004802C0u, x, y);
    } else if (type >= 0x41 && type <= 0x45) {
        road = GAME_CALL(st, 0x00480460u, x, y, (int32_t) U8(st, a + 0xAA));
    } else {
        road = GAME_CALL(st, 0x00480180u, x, y, (int32_t) U8(st, a + 3));
    }
    if (road != 0) {
        S32(st, INFO_ROAD) = 1;
    }
}

static void open_figures(game_state *st, int32_t tile)
{
    /* @bin 0x004F8655 the figures on the tile and its 8 neighbours (offsets 0x005F76C4..0x005F76E8),
     * at most 7, each picked once by 0x0053C100 (the figure's phrase); then the window's kind by the
     * first soldier (4: its formation), transport (6) or warship (5) of the list */
    int32_t n = 0;
    S32(st, INFO_FIGURE_ROW) = 0;
    S32(st, INFO_FIGURE_COUNT) = 0;
    for (int i = 0; i < 7; i++) {
        S32(st, INFO_FIGURES + 4u * (uint32_t) i) = 0;
    }
    for (uint32_t o = 0x005F76C4u; o < 0x005F76E8u; o += 4) {
        int32_t f = S16(st, GRID_FIGURE + 2u * (uint32_t) (tile + S32(st, o)));
        while (f > 0) {
            uint32_t fa = FIGURE_ADDR(f);
            if (U8(st, fa + 0x0E) != 2) {
                uint8_t action = U8(st, fa + 0x38);
                if (action != 4) {
                    /* @bin 0x004F86B0 the jump table 0x004F8910 / 0x004F8900 on the figure type */
                    uint8_t type = U8(st, fa + 0x0A);
                    int add;
                    switch (type) {
                    case 0x00: case 0x06: case 0x24: case 0x25: case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x3F:
                    case 0x41: case 0x47: case 0x48: case 0x4A:
                        add = 0;
                        break;
                    case 0x0B: case 0x0C: case 0x0D: case 0x0E:
                        add = S16(st, 0x00BCB9C8u + 0x90u * (uint32_t) (int32_t) S16(st, fa + 0x60) + 0x82) < 0;
                        break;
                    case 0x4D: case 0x4E:
                        add = S16(st, fa + 0x132) < 0 && action != 0x11;
                        break;
                    default:
                        add = 1;
                        break;
                    }
                    if (add) {
                        S32(st, INFO_FIGURES + 4u * (uint32_t) n) = f;
                        n++;
                        S32(st, INFO_FIGURE_COUNT) = n;
                        GAME_CALL(st, 0x0053C100u, f);
                        n = S32(st, INFO_FIGURE_COUNT);
                    }
                }
            }
            f = S16(st, fa + 8);
            if (n >= 7) {
                break;
            }
        }
        if (n >= 7) {
            break;
        }
    }
    for (uint32_t e = INFO_FIGURES; e < INFO_FIGURE_COUNT; e += 4) {
        int32_t f = S32(st, e);
        if (f <= 0) {
            continue;
        }
        uint8_t type = U8(st, FIGURE_ADDR(f) + 0x0A);
        if (type == 0x0E || type == 0x0B || type == 0x0C || type == 0x0D) {
            S32(st, INFO_KIND) = 4;
            S32(st, INFO_FORMATION) = S16(st, FIGURE_ADDR(f) + 0x60);
            return;
        }
        if (type == 0x4D) {
            S32(st, INFO_KIND) = 6;
            S32(st, INFO_TRANSPORT) = f;
            return;
        }
        if (type == 0x4E) {
            S32(st, INFO_KIND) = 5;
            S32(st, INFO_WARSHIP) = f;
            return;
        }
    }
}

int32_t info_building_part_of(game_state *st, int32_t b)
{
    /* @bin 0x004F6000 a building's i16 +0x12 (for type 0xD7: the building it is part of) */
    return S16(st, BUILDING_ADDR(b) + 0x12);
}

int32_t info_yard_fullness(game_state *st, int32_t b)
{
    /* @bin 0x0045D570 a storage yard's 8 spaces (the chain +0x3A): 0 when the chain ends early or a
     * space is empty (+0x12 = 0), else 1 + (the spaces' amounts +0x3C sum to less than 3200) */
    int32_t empty = 0, total = 0;
    for (int32_t i = 0; i < 8; i++) {
        b = S16(st, BUILDING_ADDR(b) + 0x3A);
        if (b <= 0) {
            return 0;
        }
        if (S16(st, BUILDING_ADDR(b) + 0x12) == 0) {
            empty++;
        } else {
            total += S16(st, BUILDING_ADDR(b) + 0x3C);
        }
    }
    if (empty > 0) {
        return 0;
    }
    return (total < 0xC80) + 1;
}

int32_t info_any_legion_ready(game_state *st)
{
    /* @bin 0x004B7590 1 when one of the formations 1..49 is in use (+0 = 1), has +0x42 set and
     * i16 +0x46 > 0 */
    for (uint32_t f = 0x00BCB9C8u + 0x90u; f < 0x00BCB9C8u + 0x90u * 50; f += 0x90) {
        if (U8(st, f) == 1 && U8(st, f + 0x42) != 0 && S16(st, f + 0x46) > 0) {
            return 1;
        }
    }
    return 0;
}

int32_t info_panel_size(game_state *st)
{
    /* @bin 0x004F8960 the panel's size code (the open turns it into the height): terrain kinds 0xC,
     * 9, 0x17, 0x18, 8 -> 1, 7 -> 4, others 0; buildings by type (the chain of compares at
     * 0x004F8A42, transcribed; 0xAD and unlisted types 0), types 0x8C..0x95 and 0x3C 1, a building
     * of 0x00488030 (thiscall on 0x00BCA350) 5; formations and ships 0 */
    int32_t kind = S32(st, INFO_KIND);
    if (kind == 0) {
        return 0;
    }
    if (kind == 1) {
        int32_t t = S32(st, INFO_TERRAIN_KIND);
        if (t == 0xC || t == 9 || t == 0x17 || t == 0x18 || t == 8) {
            return 1;
        }
        return t == 7 ? 4 : 0;
    }
    if (kind != 2) {
        return 0;
    }
    int32_t b = S32(st, INFO_BUILDING);
    int16_t type = S16(st, BUILDING_ADDR(b) + 0x10);
    if (type >= 0x8C && type <= 0x95) {
        return 1;
    }
    if (type == 0x3C) {
        return 1;
    }
    if ((uint8_t) GAME_CALLT(st, 0x00BCA350u, 0x00488030u, b) != 0) {
        return 5;
    }
    switch (S16(st, BUILDING_ADDR(S32(st, INFO_BUILDING)) + 0x10)) {
    case 0x3D: case 0x3E: case 0x3F: case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x29: case 0x2A: case 0x2B: case 0x22: case 0x23: case 0x24: case 0x25: case 0xD1: case 0x2E: case 0x2F: case 0xCE: case 0x31: case 0x62: case 0x63: case 0x5A: case 0xB4: case 0x58: case 0x59: case 0x5D: case 0x50: case 0x37: case 0x51: case 0x33: case 0x35: case 0x3A: case 0xAA: case 0xAB: case 0xC8: case 0xC9: case 0xCA: case 0x3B: case 0xAC: case 0x39: case 0x5E: case 0xB9: case 0xBA: case 0x5F: case 0x46: case 0xBE: case 0x4A: case 0x4B: case 0xC0: case 0x4C: case 0xB5: case 0xB6: case 0x4D: case 0x4E: case 0x4F: case 0xBB: case 0xBC: case 0xBD: case 0x56: case 0x57: case 0x6E: case 0x6F: case 0x70: case 0x71: case 0x72: case 0xCB: case 0xCC: case 0xE8: case 0xE9: case 0xCD: case 0xB1: case 0xB2: case 0xB3: case 0xE7:
        return 1;
    case 0x1F: case 0x20: case 0xE2: case 0x21: case 0x47: case 0xBF: case 0x54: case 0x55:
        return 2;
    case 0x1E:
        return 3;
    case 0x5C:
        return 4;
    default:
        return 0;
    }
}

int32_t info_house_advisor(int32_t need)
{
    /* @bin 0x005150A0 the advisor of a house's evolve need (+0x85, 0x1E..0x3E): its button index */
    switch (need) {
    case 0x1E: case 0x3E:
        return 0xD;
    case 0x1F: case 0x20: case 0x30: case 0x35: case 0x36: case 0x37: case 0x38:
        return 8;
    case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26:
        return 10;
    case 0x27: case 0x28: case 0x29: case 0x31: case 0x39: case 0x3B:
        return 6;
    case 0x2C: case 0x2D: case 0x2E:
        return 9;
    case 0x32: case 0x33: case 0x34:
        return 0xB;
    default:
        return 0;
    }
}

static int advisor_good(int32_t r)
{
    /* @bin 0x005151C2 (and the ten copies of its jump table) the goods whose availability
     * 0x00784DE8 shows the trade advisor's button */
    switch (r) {
    case 1: case 8: case 0xA: case 0xD: case 0xF: case 0x11: case 0x12: case 0x13: case 0x14: case 0x17:
    case 0x18: case 0x19: case 0x1A: case 0x1C: case 0x1E:
        return 1;
    default:
        return 0;
    }
}

static int lacks_workers(game_state *st)
{
    uint32_t b = BUILDING_ADDR(S32(st, INFO_BUILDING));
    return S32(st, TYPE_WORKERS + 0x28u * (uint32_t) (int32_t) S16(st, b + 0x10)) > S16(st, b + 0x40);
}

void info_advisors_of_building(game_state *st, uint32_t out)
{
    /* @bin 0x00515130 the advisor buttons of a building's panel: 15 flags at out (0x00EADCE8),
     * by the building's type (the jump table 0x00515964 / 0x005158F0 on type - 0x1E): 1 the labour
     * advisor when the building lacks workers, 14 the trade advisor for the goods 0x00784DE8 shows
     * (its resource +0x43), others by type; then houses (their evolve need +0x85, 0x005150A0),
     * buildings of 0x00488030, and types 0x8C..0x95, 0x3C..0x45, 0xD1 */
#define FLAG(i) S32(st, out + 4u * (i)) = 1
    for (uint32_t i = 0; i < 15; i++) {
        S32(st, out + 4 * i) = 0;
    }
    int16_t type = S16(st, BUILDING_ADDR(S32(st, INFO_BUILDING)) + 0x10);
    int res = 0; /* the resource check at the end of a case */
    int32_t fn = -1;    /* 0x004F5050's argument */
    switch (type) {
    case 0x4B: case 0x6D: case 0x6E: case 0x6F: case 0x71: case 0x72: case 0xA2: case 0xC0: case 0xC3:
        FLAG(6);
        if (lacks_workers(st)) FLAG(1);
        res = 1;
        break;
    case 0x56: case 0x57: case 0xA1: case 0xBB: case 0xBC: case 0xBD:
        FLAG(12);
        FLAG(5);
        if (lacks_workers(st)) FLAG(1);
        break;
    case 0xC7: {
        if (lacks_workers(st)) FLAG(1);
        /* @bin 0x005151FE the three gods of the city (+0x4730, or the defaults 0x00784DE0) */
        uint32_t c = CURRENT_CITY(st) + 0x4730;
        for (uint32_t d = 0x00784DE0u; d < 0x00784DE6u; d += 2, c += 4) {
            int32_t v = S32(st, c);
            int32_t g = v == 0 ? S16(st, d) : v;
            if (U8(st, 0x005D19D6u + ((uint32_t) g << 4)) != 0) {
                FLAG(14);
                break;
            }
        }
        break;
    }
    case 0x6C: fn = 0x14; break;
    case 0x6A: fn = 0x18; break;
    case 0x6B: fn = 0x19; break;
    case 0xD8: fn = 0x1A; break;
    case 0xDD: fn = 0x1E; break;
    case 0xCB:
        FLAG(6);
        FLAG(9);
        if (lacks_workers(st)) FLAG(1);
        res = 1;
        break;
    case 0x73:
        FLAG(7);
        if (lacks_workers(st)) FLAG(1);
        res = 1;
        break;
    case 0xCC:
        if (lacks_workers(st)) FLAG(1);
        if ((uint8_t) GAME_CALL(st, 0x004F5050u, 0xC, 1) != 0) FLAG(14);
        break;
    case 0x4A: case 0xB5: case 0xB6:
        if (lacks_workers(st)) FLAG(1);
        FLAG(3);
        break;
    case 0xB1: case 0xB2: case 0xB3:
        if (lacks_workers(st)) FLAG(1);
        FLAG(14);
        break;
    case 0x47: case 0x48: {
        /* @bin 0x00515538 granaries and storage yards: the first 5 running requests
         * (0x0044BAF0 on 0x00785D70, 0x0044D4C0): one for food (goods 1..8) or for goods (< 0x24) */
        if (lacks_workers(st)) FLAG(1);
        int32_t n = 0;
        if (type == 0x47) FLAG(7); else FLAG(6);
        int32_t it = GAME_CALLT(st, 0x00785D70u, 0x0044BAF0u, 0);
        while (it != 0) {
            if ((uint8_t) GAME_CALLT(st, (uint32_t) it, 0x0044D4C0u) != 0) {
                if (n >= 5) {
                    break;
                }
                int32_t g = S16(st, (uint32_t) it + 4);
                if (type == 0x47 ? (g > 0 && g <= 8) : g < 0x24) {
                    FLAG(4);
                    break;
                }
                n++;
            }
            it = GAME_CALLT(st, 0x00785D70u, 0x0044BAF0u, it);
        }
        break;
    }
    case 0x1E: case 0x1F: case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0xE2:
        if (lacks_workers(st)) FLAG(1);
        FLAG(10);
        break;
    case 0x33: case 0x35:
        FLAG(9);
        if (lacks_workers(st)) FLAG(1);
        break;
    case 0x5C:
        FLAG(8);
        break;
    case 0x2E: case 0x2F: case 0x31: case 0xB4: case 0xCE:
        FLAG(8);
        if (lacks_workers(st)) FLAG(1);
        break;
    case 0x37: case 0x51: case 0xA7: case 0xB8:
        FLAG(13);
        if (lacks_workers(st)) FLAG(1);
        break;
    case 0x26: case 0x27: case 0x29: case 0x2A: case 0x2B: case 0x52: case 0x53: case 0x88: case 0x8A:
        FLAG(13);
        break;
    case 0x28: case 0x2C: case 0x2D: case 0xA9:
        FLAG(2);
        break;
    case 0x5E: case 0x5F: case 0xAB: case 0xAD: case 0xCA:
        FLAG(2);
        if (lacks_workers(st)) FLAG(1);
        res = 1;
        break;
    case 0x70: case 0xCD:
        FLAG(2);
        FLAG(6);
        if (lacks_workers(st)) FLAG(1);
        res = 1;
        break;
    case 0xB7: case 0xCF: case 0xD0: case 0xD2: case 0xD5: case 0xDE:
        FLAG(14);
        break;
    case 0x4D: case 0x4E: case 0x4F:
        FLAG(4);
        if (lacks_workers(st)) FLAG(1);
        break;
    case 0x5A:
        if (lacks_workers(st)) FLAG(1);
        break;
    case 0x46: case 0x4C: case 0xBE: case 0xC2:
        FLAG(7);
        if (lacks_workers(st)) FLAG(1);
        break;
    default:
        break;
    }
    if (fn >= 0) {
        /* @bin 0x00515255 the entertainment venues and others: 0x004F5050(kind, 1) */
        if (lacks_workers(st)) FLAG(1);
        if ((uint8_t) GAME_CALL(st, 0x004F5050u, fn, 1) != 0) FLAG(14);
        res = 1;
    }
    uint32_t b = BUILDING_ADDR(S32(st, INFO_BUILDING));
    if (res) {
        int32_t r = U8(st, b + 0x43);
        if (advisor_good(r) && S32(st, 0x00784DE8u + 4u * (uint32_t) r) != 0) FLAG(14);
    }
    type = S16(st, b + 0x10);
    if (type >= 0x0A && type <= 0x1D) {
        /* @bin 0x005157A8 houses: the good they lack, the advisor of their evolve need */
        int32_t r = U8(st, b + 0x43);
        if (advisor_good(r) && S32(st, 0x00784DE8u + 4u * (uint32_t) r) != 0) FLAG(14);
        int32_t need = U8(st, BUILDING_ADDR(S32(st, INFO_BUILDING)) + 0x85);
        if (need < 0x1E) {
            need += 0x1E;
        }
        int32_t a = need >= 0x44 ? 6 : info_house_advisor(need);
        if (a != 0) {
            FLAG((uint32_t) a);
        }
        FLAG(7);
    }
    if ((uint8_t) GAME_CALLT(st, 0x00BCA350u, 0x00488030u, S32(st, INFO_BUILDING)) != 0) {
        FLAG(7);
        if (lacks_workers(st)) FLAG(1);
    }
    type = S16(st, BUILDING_ADDR(S32(st, INFO_BUILDING)) + 0x10);
    if (type >= 0x8C && type <= 0x95) {
        FLAG(11);
    }
    type = S16(st, BUILDING_ADDR(S32(st, INFO_BUILDING)) + 0x10);
    if ((type >= 0x41 && type <= 0x45) || (type >= 0x3C && type <= 0x40) || type == 0xD1) {
        FLAG(11);
        if (lacks_workers(st)) FLAG(1);
    }
#undef FLAG
}

int32_t info_pyramid_of_part(game_state *st, int32_t b)
{
    /* @bin 0x004F5D90 the pyramid (type 0xB7) a ramp or work tile (0xDA..0xDC) belongs to: for each
     * pyramid of a kind 0x004ECEA0 accepts, the ramp walk 0x004ED360 from its corner (its kind's
     * offset 0x005F5D08) on 6, 0 sets 0x00EAAE84 when b is reached (0x00EAAE88); pyramids skip
     * their size in records (0x0056F7EC in mode 0x005E4EF4 = 5, else 0x0056F7D8); b when none */
    S32(st, 0x00EAAE88u) = b;
    for (int32_t p = 1; p < 4000; p++) {
        uint32_t a = BUILDING_ADDR(p);
        if (U8(st, a) == 0 || S16(st, a + 0x10) != 0xB7) {
            continue;
        }
        uint32_t kind = U8(st, a + 0x54);
        if ((uint8_t) GAME_CALL(st, 0x004ECEA0u, (int32_t) kind) != 0) {
            int32_t v = (int32_t) (U32(st, 0x005F5D08u + 4 * kind) + U32(st, a + 0x0C) - U32(st, MAP_START));
            GAME_CALL(st, 0x004ED360u, v % 0xE4, v / 0xE4, 6, 0);
            if (U8(st, 0x00EAAE84u) != 0) {
                return p;
            }
        }
        kind = U8(st, a + 0x54);
        p += S32(st, (S32(st, 0x005E4EF4u) == 5 ? 0x0056F7ECu : 0x0056F7D8u) + 4 * kind) - 1;
    }
    return b;
}

int32_t info_open(game_state *st)
{
    /* @bin 0x004F7E50 a left click on the city view opens the info window on the mouse tile:
     * returns 1, or 0 (editor, message rows, a building not in use) */
    if (S32(st, EDITOR) == 1) {
        return 0;
    }
    if (S32(st, MESSAGE_ROWS) > 0) {
        GAME_CALL0(st, 0x004E2AB0u);
        return 0;
    }
    int32_t ty = S32(st, MOUSE_TILE + 8), tx = S32(st, MOUSE_TILE + 4);
    S32(st, INFO_TILE_Y) = ty;
    S32(st, INFO_TILE_X) = tx;
    S32(st, INFO_BUTTON_HOVER2) = 0;
    S32(st, INFO_BUTTON_HOVER) = 0;
    S32(st, 0x007056B8u) = 0;
    S32(st, INFO_SOUND) = 1;
    S32(st, INFO_ORDERS) = 0;
    U8(st, WINDOW_ID) = 9;
    U8(st, REDRAW_ALL) = 1;
    int32_t tile = (int32_t) ((uint32_t) tx + (uint32_t) ty * GRID_SIZE + U32(st, MAP_START));
    S32(st, INFO_TILE) = tile;
    int32_t b = S16(st, GRID_BUILDING + 2u * (uint32_t) tile);
    S32(st, INFO_BUILDING) = b;
    S32(st, INFO_TERRAIN) = U8(st, 0x00A9010Cu + (uint32_t) tile);
    int16_t type = S16(st, BUILDING_ADDR(b) + 0x10);
    if (type == 0xDA || type == 0xDB || type == 0xDC) {
        b = info_pyramid_of_part(st, b);
        S32(st, INFO_BUILDING) = b;
    }
    if (S16(st, BUILDING_ADDR(b) + 0x10) == 0xD7) {
        b = info_building_part_of(st, b);
        S32(st, INFO_BUILDING) = b;
    }
    S32(st, INFO_TILE_FLAG) = (int32_t) (U32(st, GRID_TERRAIN + 4u * (uint32_t) tile) & 0x80);
    S32(st, INFO_TILE_BYTE) = U8(st, 0x00AE8E7Cu + (uint32_t) tile);
    if (b != 0 && U8(st, BUILDING_ADDR(b)) == 0) {
        U8(st, WINDOW_ID) = 1;
        return 0;
    }
    GAME_CALL0(st, 0x005596D0u);
    b = S32(st, INFO_BUILDING);
    open_monument_status(st, S16(st, BUILDING_ADDR(b) + 0x10), b);
    b = S32(st, INFO_BUILDING);
    S32(st, INFO_KIND) = 1;
    S32(st, INFO_FIGURE_VIEW) = 0;
    if (S16(st, GRID_BUILDING + 2u * (uint32_t) tile) == 0 && U8(st, 0x00ADC36Cu + (uint32_t) tile) != 0) {
        /* @bin 0x004F80EC a tile of 0x00ADC36C (no building): 0xA, or 0xB with flag 4 */
        S32(st, INFO_TERRAIN_KIND) = (int32_t) (((U32(st, GRID_TERRAIN + 4u * (uint32_t) tile) & 0xFF & 4) | 0x28) >> 2);
    } else {
        int32_t k = open_terrain_kind(st, tile, b);
        if (k >= 0) {
            S32(st, INFO_TERRAIN_KIND) = k;
        } else if (k == -2) {
            open_building(st);
        }
    }
    open_figures(st, tile);
    /* @bin 0x004F87BD the panel's place and size: 0x1D blocks wide, the height by the panel
     * (0x004F8960: 1 0x10, 2 0x12, 3 0x13, 4 0xE, 5 0x18, else 0x16), placed by the screen height,
     * the sidebar (collapsed 0x00F53F78) and the mouse's y */
    S32(st, INFO_X) = 8;
    S32(st, INFO_Y) = 0x20;
    S32(st, INFO_W) = 0x1D;
    int32_t size = info_panel_size(st);
    int32_t h = size == 1 ? 0x10 : size == 2 ? 0x12 : size == 3 ? 0x13 : size == 4 ? 0xE : size == 5 ? 0x18 : 0x16;
    S32(st, INFO_H) = h;
    int32_t screen_h = S32(st, 0x00E38DACu);
    int collapsed = S32(st, 0x00F53F78u) == 1;
    if (screen_h == 0x1E0) {
        S32(st, INFO_Y) = 0x20;
        S32(st, INFO_X) = collapsed ? 0x44 : 8;
    } else if (screen_h == 0x258) {
        S32(st, INFO_Y) = S32(st, 0x00E38E30u) > 0x138 ? 0x20 : (0x25 - h) << 4;
        S32(st, INFO_X) = collapsed ? 0x80 : 0x44;
    } else if (screen_h == 0x300) {
        S32(st, INFO_Y) = S32(st, 0x00E38E30u) > 0x18C ? 0x20 : 0x2F8 - (h << 4);
        S32(st, INFO_X) = collapsed ? 0xF8 : 0xBC;
    }
    if (S32(st, INFO_KIND) == 2) {
        info_advisors_of_building(st, 0x00EADCE8u);
    }
    return 1;
}

#define MOUSE_X 0x00E38E34u
#define MOUSE_Y 0x00E38E30u
#define LEFT_HELD 0x00E38E00u
#define RIGHT_RELEASED 0x00E38DF8u
#define PARAM1 0x00D3B198u
#define CLOSE_BUTTONS 0x005C7B10u   /* the help and close buttons (0x24 bytes each): 2 */
#define FORMATIONS 0x00BCB9C8u      /* 0x90 bytes each */

static int32_t rect_buttons(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t n, uint32_t hover)
{
    /* the pattern of 0x004F96E0 for a table of rectangle buttons: the hovered one (0x00426450), its
     * highlight (0x004D6740) kept in `hover` (0x007056C8 or 0x007056C4), then the click
     * (0x004261D0): its result */
    int32_t v = GAME_CALL(st, 0x00426450u, x, y, (int32_t) table, n, S32(st, INFO_X), S32(st, INFO_Y));
    S32(st, hover) = GAME_CALL(st, 0x004D6740u, x, y, (int32_t) table, n, v);
    return GAME_CALL(st, 0x004261D0u, x, y, (int32_t) table, n);
}

static int32_t close_buttons(game_state *st, int32_t y)
{
    /* the help and close buttons at (0x00EADD90, y): hover (ids from 10), click */
    GAME_CALL(st, 0x00425BC0u, S32(st, INFO_X), y, (int32_t) CLOSE_BUTTONS, 2, 10);
    return GAME_CALL(st, 0x00425990u, S32(st, INFO_X), y, (int32_t) CLOSE_BUTTONS, 2);
}

static int32_t panel_bottom(game_state *st, int32_t up)
{
    return S32(st, INFO_H) * 16 + S32(st, INFO_Y) - up;
}

static int bazaar_good(int32_t resource)
{
    /* @bin 0x005013A0 (registered without a wrapper in core: inlined) the goods a bazaar lists: 1..8,
     * 0xD, 0xF, 0x11, 0x13 (its jump table 0x005013CC) */
    switch (resource) {
    case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 0xD: case 0xF: case 0x11: case 0x13:
        return 1;
    default:
        return 0;
    }
}

static int32_t storage_rows(game_state *st)
{
    /* the rows of a storage yard's list: the visible rows (0x00502780), at most the list's length */
    int32_t n = GAME_CALL0(st, 0x00502780u);
    if (n < S32(st, STORAGE_LIST)) {
        return GAME_CALL0(st, 0x00502780u);
    }
    return S32(st, STORAGE_LIST);
}

int32_t info_storage_scroll(game_state *st)
{
    /* @bin 0x0040D1B0 the storage yard list's scroll bar held down: the row under the mouse (the bar
     * at x + 0x19E..0x1BE, y 0x6A..) sets the list's first row, building +0xE0, and redraws (2);
     * returns 1, else 0 */
    if (S32(st, STORAGE_LIST) <= GAME_CALL0(st, 0x00502780u)) {
        return 0;
    }
    if (U8(st, LEFT_HELD) == 0) {
        return 0;
    }
    int32_t hidden = S32(st, STORAGE_LIST) - GAME_CALL0(st, 0x00502780u);
    int32_t bar = (GAME_CALL(st, 0x004CDE80u, GAME_CALL0(st, 0x00502780u) << 2, 3) - 6) << 4;
    int32_t mx = S32(st, MOUSE_X);
    int32_t x = S32(st, INFO_X);
    if (mx < x + 0x19E || mx > x + 0x1BE) {
        return 0;
    }
    int32_t my = S32(st, MOUSE_Y);
    if (my < 0x6A || my > bar + 0x7A) {
        return 0;
    }
    int32_t at = my - 0x6A;
    if (at > bar) {
        at = bar;
    }
    int32_t row = GAME_CALL(st, 0x004CDE40u, hidden, GAME_CALL(st, 0x004CDE60u, at, bar));
    U8(st, REDRAW_ALL) = 2;
    S32(st, BUILDING_ADDR(S32(st, INFO_BUILDING)) + 0xE0) = row + 1;
    return 1;
}

void info_advisor_buttons(game_state *st)
{
    /* @bin 0x00515C60 a building panel's advisor buttons: for each of the 15 advisors (tables
     * 0x005F78F0) the panel listed (0x00EADCE8) and the game offers (0x006FDC88), a button 27 pixels
     * right of the previous one; a click ends the scan (the sidebar redraw flag 0x00E38DC8 stays) */
    int32_t y = (S32(st, INFO_H) << 4) - 0x28;
    int32_t dx = 0;
    U8(st, 0x00E38DC9u) = 1;
    for (int32_t i = 0; i < 15; i++) {
        if (S32(st, 0x00EADCE8u + 4u * (uint32_t) i) == 0 || U8(st, 0x006FDC88u + (uint32_t) i) == 0) {
            continue;
        }
        GAME_CALL(st, 0x00425BC0u, S32(st, INFO_X) + dx + 0x28, S32(st, INFO_Y) + y,
                  S32(st, 0x005F78F0u + 4u * (uint32_t) i), 1, i + 0x28);
        if (GAME_CALL(st, 0x00425990u, S32(st, INFO_X) + dx + 0x28, S32(st, INFO_Y) + y,
                      S32(st, 0x005F78F0u + 4u * (uint32_t) i), 1) != 0) {
            return;
        }
        dx += 0x1B;
    }
    U8(st, 0x00E38DC8u) = 0;
}

void info_kingdom_highlight(game_state *st)
{
    /* @bin 0x00516020 the palace's (0xD1) button frame, lit when hovered and the city has a
     * kingdom request (+0x4814), drawn unless +0x44F8 */
    uint32_t city = CURRENT_CITY(st);
    if (S32(st, city + 0x44F8) != 0) {
        return;
    }
    int32_t lit = S16(st, city + 0x4814) != 0 ? S32(st, INFO_BUTTON_HOVER) == 1 : 0;
    int32_t x = S32(st, INFO_X), y = S32(st, INFO_Y);
    GAME_CALL(st, 0x004D63D0u, 0, x + 0x52, y + 0x99, 300, 0x14, lit, x, y);
}

void info_input(game_state *st)
{
    /* @bin 0x004F96E0 the info window's input */
    S32(st, INFO_BUTTON_HOVER) = 0;
    if (U8(st, RIGHT_RELEASED) != 0) {
        /* @bin 0x004F96F8 a right click closes the window; on a legion's panel it gives the order
         * of its layout (0x00406720, the layout's jump table 0x004FA320 / 0x004FA310) and opens the
         * target window 0x1E, or 0x49 for a company of type 0xD at layout 0 */
        if (S32(st, INFO_KIND) == 4) {
            uint32_t f = FORMATIONS + 0x90u * (uint32_t) S32(st, INFO_FORMATION);
            if (U8(st, f + 0x28) != 0) {
                if (S16(st, f + 4) == 0xD && S16(st, f + 0x2A) == 0) {
                    U8(st, WINDOW_ID) = 0x49;
                    U8(st, REDRAW_ALL) = 1;
                    return;
                }
                int16_t layout = S16(st, f + 0x2A);
                if (layout != 6) {
                    if (layout == 1 || layout == 2) {
                        S32(st, PARAM1) = 0;
                    } else if (layout == 3 || layout == 4) {
                        S32(st, PARAM1) = 1;
                    } else if (layout == 0xF) {
                        S32(st, PARAM1) = 2;
                    }
                    GAME_CALL0(st, 0x00406720u);
                    U8(st, WINDOW_ID) = 0x1E;
                    U8(st, REDRAW_ALL) = 1;
                    return;
                }
            }
        }
        U8(st, WINDOW_ID) = 1;
        U8(st, REDRAW_ALL) = 1;
        return;
    }
    int32_t kind = S32(st, INFO_KIND);
    if (S32(st, INFO_ORDERS) != 0) {
        /* @bin 0x004F97E5 the orders sub-panel: its rows (0x005C68C8) and the buttons under them */
        int16_t type = S16(st, BUILDING_ADDR(S32(st, INFO_BUILDING)) + 0x10);
        int32_t n;
        if (type == 0x48 || type == 0xC1) {
            n = storage_rows(st);
        } else if (type == 0x46 || type == 0xBE) {
            n = 0;
            for (int32_t i = 0; i < S32(st, STORAGE_LIST); i++) {
                if (bazaar_good(S32(st, STORAGE_LIST + 4 + 4u * (uint32_t) i))) {
                    n++;
                }
            }
        } else {
            n = S32(st, STORAGE_ROWS);
        }
        if (rect_buttons(st, S32(st, INFO_X) + 0xFA, 0x4E, 0x005C68C8u, n, INFO_BUTTON_HOVER2) != 0) {
            return;
        }
        int32_t y = ((GAME_CALL(st, 0x004CDE80u, n * 4, 3) + 7) << 4) - 8;
        if (close_buttons(st, y) != 0) {
            return;
        }
    } else {
        if (kind == 2) {
            info_advisor_buttons(st);
        }
        if (close_buttons(st, panel_bottom(st, 0x28)) != 0) {
            return;
        }
    }
    kind = S32(st, INFO_KIND);
    if (kind == 0) {
        return;
    }
    int32_t x = S32(st, INFO_X), y = S32(st, INFO_Y);
    if (kind == 4) {
        /* @bin 0x004F9A47 the legion's orders and its kingdom-service toggle */
        if (rect_buttons(st, x, y, 0x005C66D8u, 5, INFO_BUTTON_HOVER) != 0) {
            return;
        }
        int32_t tx = ((S32(st, INFO_W) - 0x14) << 4) / 2 + S32(st, INFO_X);
        rect_buttons(st, tx, panel_bottom(st, 0x28), 0x005C6768u, 1, INFO_BUTTON_HOVER2);
        return;
    }
    if (kind == 6) {
        rect_buttons(st, x, y, 0x005C6788u, 5, INFO_BUTTON_HOVER);
        return;
    }
    if (kind == 5) {
        rect_buttons(st, x, y, 0x005C6818u, 5, INFO_BUTTON_HOVER);
        return;
    }
    if (S32(st, INFO_FIGURE_VIEW) != 0) {
        /* @bin 0x004F9C25 the figure list: a figure's portrait (0x005C5BE8), its orders button
         * (0x005C5CB0), and the advisor button of its kind (+0x131, 0x005F78F0) */
        if (rect_buttons(st, x, y, 0x005C5BE8u, S32(st, INFO_FIGURE_COUNT), INFO_BUTTON_HOVER) != 0) {
            return;
        }
        if (rect_buttons(st, S32(st, INFO_X) + 0x11D, S32(st, INFO_Y) + 0xC3, 0x005C5CB0u, 1, INFO_BUTTON_HOVER2) !=
            0) {
            return;
        }
        uint32_t fa = FIGURE_ADDR(S32(st, INFO_FIGURES + 4u * (uint32_t) S32(st, INFO_FIGURE_ROW)));
        uint8_t advisor = U8(st, fa + 0x131);
        if (U8(st, 0x006FDC88u + advisor) == 0) {
            return;
        }
        GAME_CALL(st, 0x00425BC0u, S32(st, INFO_X) + 0x28, panel_bottom(st, 0x28),
                  S32(st, 0x005F78F0u + 4u * advisor), 1, advisor + 0x28);
        GAME_CALL(st, 0x00425990u, S32(st, INFO_X) + 0x28, panel_bottom(st, 0x28),
                  S32(st, 0x005F78F0u + 4u * U8(st, fa + 0x131)), 1);
        return;
    }
    if (kind != 2) {
        return;
    }
    int16_t type = S16(st, BUILDING_ADDR(S32(st, INFO_BUILDING)) + 0x10);
    if (type >= 0x0A && type <= 0x1D) {
        /* @bin 0x004F9DE0 houses: the button 0x005C5A78 */
        if (rect_buttons(st, S32(st, INFO_X) + 0x70, ((S32(st, INFO_H) - 2) << 4) + S32(st, INFO_Y), 0x005C5A78u, 1,
                         INFO_BUTTON_HOVER2) != 0) {
            return;
        }
        type = S16(st, BUILDING_ADDR(S32(st, INFO_BUILDING)) + 0x10);
    }
    if (type == 0x47 || type == 0xBF) {
        /* @bin 0x004F9E99 granaries: the orders' "all" button, or the orders button */
        int32_t r;
        if (S32(st, INFO_ORDERS) != 0) {
            int32_t by = ((GAME_CALL(st, 0x004CDE80u, S32(st, STORAGE_ROWS) * 4, 3) + 7) << 4) - 0x10;
            r = rect_buttons(st, S32(st, INFO_X) + 0x50, by, 0x005C5BA8u, 1, INFO_BUTTON_HOVER);
        } else {
            r = rect_buttons(st, S32(st, INFO_X) + 0x64, panel_bottom(st, 0x22), 0x005C5A98u, 1, INFO_BUTTON_HOVER);
        }
        if (r != 0) {
            return;
        }
        type = S16(st, BUILDING_ADDR(S32(st, INFO_BUILDING)) + 0x10);
    }
    if (type == 0x46 || type == 0xBE) {
        /* @bin 0x004FA215 bazaars */
        if (S32(st, INFO_ORDERS) != 0) {
            rect_buttons(st, S32(st, INFO_X) + 0x50, 0x1B4, 0x005C5BA8u, 1, INFO_BUTTON_HOVER);
        } else {
            rect_buttons(st, S32(st, INFO_X) + 0x64, panel_bottom(st, 0x22), 0x005C5AB8u, 1, INFO_BUTTON_HOVER);
        }
    } else if (type == 0x48 || type == 0xC1) {
        /* @bin 0x004FA0DD storage yards: the list's scroll bar, then the orders' buttons */
        if (S32(st, INFO_ORDERS) != 0) {
            int32_t rows = GAME_CALL(st, 0x004CDE80u, storage_rows(st) * 4, 3) + 7;
            if (info_storage_scroll(st) != 0) {
                return;
            }
            rect_buttons(st, S32(st, INFO_X) + 0x50, (rows << 4) - 0x18, 0x005C5BC8u, 1, INFO_BUTTON_HOVER);
        } else {
            rect_buttons(st, S32(st, INFO_X) + 0x64, panel_bottom(st, 0x22), 0x005C5AD8u, 1, INFO_BUTTON_HOVER);
        }
    } else if (type == 0xD1) {
        /* @bin 0x004F9FFF the palace's button and its frame */
        if (rect_buttons(st, S32(st, INFO_X), S32(st, INFO_Y), 0x005C5188u, 1, INFO_BUTTON_HOVER) != 0) {
            return;
        }
        info_kingdom_highlight(st);
    } else if (type == 0x4D || type == 0x4E || type == 0x4F) {
        rect_buttons(st, S32(st, INFO_X), S32(st, INFO_Y), 0x005C5448u, 1, INFO_BUTTON_HOVER);
    }
}

void info_toggle_distribution(game_state *st)
{
    /* @bin 0x004062D0 a button callback: 0x00EADD44 := (it != 1), redraw */
    S32(st, INFO_DISTRIBUTION) = S32(st, INFO_DISTRIBUTION) != 1;
    U8(st, REDRAW_ALL) = 1;
}

void info_show_orders(game_state *st)
{
    /* @bin 0x004062F0 a button callback: the storage orders sub-panel */
    S32(st, INFO_ORDERS) = 1;
    U8(st, REDRAW_ALL) = 1;
}

void info_select_figure(game_state *st)
{
    /* @bin 0x00406700 a button callback: the figure list's row := parameter 1, 0x0053D0B0, redraw */
    S32(st, INFO_FIGURE_ROW) = S32(st, PARAM1);
    GAME_CALL0(st, 0x0053D0B0u);
    U8(st, REDRAW_ALL) = 1;
}

void info_help(game_state *st)
{
    /* @bin 0x00511C50 the help button: the panel's help message 0x00EADD54 (10 when none) */
    int32_t help = S32(st, INFO_HELP);
    GAME_CALL(st, 0x004C5BF0u, help < 1 ? 10 : help, 0, 0);
    U8(st, REDRAW_ALL) = 1;
}

static int32_t run_info_draw(game_state *st, const int32_t *a)
{
    (void) a;
    info_draw(st);
    return 0;
}

static int32_t run_info_workers_line(game_state *st, const int32_t *a)
{
    info_workers_line(st, a[0]);
    return 0;
}

static int32_t run_info_workers_line_or_closed(game_state *st, const int32_t *a)
{
    info_workers_line_or_closed(st, a[0]);
    return 0;
}

static int32_t run_info_frame(game_state *st, const int32_t *a)
{
    (void) a;
    info_frame(st);
    return 0;
}

static int32_t run_info_open(game_state *st, const int32_t *a)
{
    (void) a;
    return info_open(st);
}

#define RUN0(name, call) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        call(st); \
        return 0; \
    }
RUN0(run_info_input, info_input)
RUN0(run_info_advisor_buttons, info_advisor_buttons)
RUN0(run_info_kingdom_highlight, info_kingdom_highlight)
RUN0(run_info_toggle_distribution, info_toggle_distribution)
RUN0(run_info_show_orders, info_show_orders)
RUN0(run_info_select_figure, info_select_figure)
RUN0(run_info_help, info_help)

static int32_t run_info_storage_scroll(game_state *st, const int32_t *a)
{
    (void) a;
    return info_storage_scroll(st);
}

static int32_t run_info_panel_size(game_state *st, const int32_t *a)
{
    (void) a;
    return info_panel_size(st);
}

static int32_t run_info_building_part_of(game_state *st, const int32_t *a)
{
    return info_building_part_of(st, a[0]);
}

static int32_t run_info_yard_fullness(game_state *st, const int32_t *a)
{
    return info_yard_fullness(st, a[0]);
}

static int32_t run_info_any_legion_ready(game_state *st, const int32_t *a)
{
    (void) a;
    return info_any_legion_ready(st);
}

RUN0(run_info_house_worst_neighbour, info_house_worst_neighbour)

static int32_t run_info_house_advisor(game_state *st, const int32_t *a)
{
    (void) st;
    return info_house_advisor(a[0]);
}

static int32_t run_info_advisors_of_building(game_state *st, const int32_t *a)
{
    info_advisors_of_building(st, (uint32_t) a[0]);
    return 0;
}

static int32_t run_info_pyramid_of_part(game_state *st, const int32_t *a)
{
    return info_pyramid_of_part(st, a[0]);
}

const ported_function info_ports[] = {
    {0x004F5D90u, "info_pyramid_of_part", 1, 32, 0, run_info_pyramid_of_part, 0},
    {0x005150A0u, "info_house_advisor", 1, 32, 0, run_info_house_advisor, 0},
    {0x00515130u, "info_advisors_of_building", 1, 0, 0, run_info_advisors_of_building, 0},
    {0x0050F0F0u, "info_house_worst_neighbour", 0, 0, 0, run_info_house_worst_neighbour, 0},
    {0x004F6000u, "info_building_part_of", 1, 32, 0, run_info_building_part_of, 0},
    {0x0045D570u, "info_yard_fullness", 1, 8, 0, run_info_yard_fullness, 0},
    {0x004B7590u, "info_any_legion_ready", 0, 32, 0, run_info_any_legion_ready, 0},
    {0x004F8960u, "info_panel_size", 0, 32, 0, run_info_panel_size, 0},
    {0x004F96E0u, "info_input", 0, 0, 0, run_info_input, 0},
    {0x00515C60u, "info_advisor_buttons", 0, 0, 0, run_info_advisor_buttons, 0},
    {0x00516020u, "info_kingdom_highlight", 0, 0, 0, run_info_kingdom_highlight, 0},
    {0x0040D1B0u, "info_storage_scroll", 0, 32, 0, run_info_storage_scroll, 0},
    {0x004062D0u, "info_toggle_distribution", 0, 0, 0, run_info_toggle_distribution, 0},
    {0x004062F0u, "info_show_orders", 0, 0, 0, run_info_show_orders, 0},
    {0x00406700u, "info_select_figure", 0, 0, 0, run_info_select_figure, 0},
    {0x00511C50u, "info_help", 0, 0, 0, run_info_help, 0},
    {0x004F7E50u, "info_open", 0, 32, 0, run_info_open, 0},
    {0x004F8E60u, "info_draw", 0, 0, 0, run_info_draw, 0},
    {0x004FC5C0u, "info_workers_line", 1, 0, 0, run_info_workers_line, 0},
    {0x004FC4F0u, "info_workers_line_or_closed", 1, 0, 0, run_info_workers_line_or_closed, 0},
    {0x004F94A0u, "info_frame", 0, 0, 0, run_info_frame, 0},
};
const int info_ports_count = sizeof(info_ports) / sizeof(info_ports[0]);
