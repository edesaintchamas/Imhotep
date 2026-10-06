/* Imhotep - a created building put on the map (package mapadd): 0x00470990, the switch on the
 * building type that writes its tiles (0x0047BF50 and others), its images and its per-type
 * setup, and the helpers only it calls. Called by the place command 0x004728A0 (new = 1), the
 * loader 0x00418BF0 and the redraws (new = 0).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "building/map_add.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define B(id) BUILDING_ADDR(id)
#define TERRAIN 0x00AF598Cu /* u32 per tile */
#define BUILDING_GRID 0x00B4E6FCu
#define IMAGE 0x00B7482Cu /* u32 per tile */
#define RANDOM 0x00ACF85Cu /* u8 per tile */
#define MAP_W 0x00BCA2D0u
#define MAP_H 0x00BCA2D4u
#define MAP_START 0x00BCA2D8u
#define ORIENT 0x00BCA2E0u
#define CURSOR 0x00BC9768u
#define GROUPS 0x00F55BB4u /* image group starts, 8 bytes each */
#define GROUP(g) U32(st, GROUPS + 8u * (uint32_t) (g))
#define G(a) U32(st, a) /* an image group start read by address (0x00F55BB4 + 8 * group) */
#define FORM(f) (0x00BCB9C8u + 0x90u * (uint32_t) (f))
#define CITY CURRENT_CITY(st)

static void tiles(game_state *st, int32_t b, int32_t x, int32_t y, int32_t sx, int32_t sy, int32_t img, int32_t flags)
{
    GAME_CALL(st, 0x0047BF50u, b, x, y, sx, sy, img, flags);
}

static int32_t create(game_state *st, int32_t player, int32_t type, int32_t x, int32_t y, int32_t n, int32_t id)
{
    return GAME_CALL(st, 0x0046A400u, player, type, x, y, n, id);
}

static void keep_removed(game_state *st, int32_t id)
{
    GAME_CALL(st, 0x00485100u, id);
}

static int32_t main_part(game_state *st, int32_t id)
{
    return GAME_CALL(st, 0x00475B50u, id);
}

static void clear_construction(game_state *st)
{
    /* @bin 0x00471E1F the construction globals: type, in progress, the two corners */
    U32(st, 0x006059E8u) = 0;
    U32(st, 0x006059E4u) = 0;
    U32(st, 0x006059DCu) = 0;
    U32(st, 0x006059E0u) = 0;
    U32(st, 0x006059D4u) = 0;
    U32(st, 0x006059D8u) = 0;
}

static void entertainment_venue(game_state *st, int32_t id, int32_t type)
{
    /* @bin 0x00470A98 the main part (0x00475B50) of a venue with a +0xD8 flag draws its area */
    if (main_part(st, id) != id || U32(st, B(id) + 0xD8) == 0) {
        return;
    }
    GAME_CALL(st, 0x00440060u, S32(st, B(id) + 0xD0), type, id);
}

static void palace_record(game_state *st, int32_t id, int32_t x, int32_t y, uint32_t rec)
{
    /* @bin 0x0047174E 0x004717E8 0x004720B6 the city's first building of the kind: rec +8 its tile
     * (0 = none), +0xC its id, +4 / +6 its x, y */
    uint32_t c = CITY + rec;
    if (S32(st, c + 8) != 0) {
        return;
    }
    S32(st, c + 0xC) = id;
    S16(st, c + 4) = (int16_t) x;
    S16(st, c + 6) = (int16_t) y;
    S32(st, c + 8) = S32(st, MAP_START) + y * GRID_SIZE + x;
}

static void storage_record(game_state *st, int32_t id, int32_t is_new)
{
    /* @bin 0x00470EB6 a new storage building: goods lists 0x005596D0, then its storage record */
    if (is_new) {
        GAME_CALL0(st, 0x005596D0u);
        U8(st, B(id) + 0xC1) = (uint8_t) GAME_CALL(st, 0x0046B100u, id);
    }
}

static void dock_like(game_state *st, int32_t id, int32_t x, int32_t y, int big, uint32_t g_edge, uint32_t g_other)
{
    /* @bin 0x00471152 the shore fit (0x0047D060 3x3, 0x0047CC70 2x2) set the turn 0x00BC96D8 and
     * the view turn 0x00BC96DC; the image by figure_near_map_edge 0x0047A1C0 */
    U8(st, B(id) + 0xAA) = U8(st, 0x00BC96D8u);
    int32_t edge = (int8_t) GAME_CALL(st, 0x0047A1C0u, x, y);
    int32_t img = (int32_t) G(edge ? g_edge : g_other) + S32(st, 0x00BC96DCu);
    if (big) {
        GAME_CALL(st, 0x0047D1F0u, id, x, y, img);
    } else {
        GAME_CALL(st, 0x0047CF00u, id, x, y, img);
    }
}

static void fort(game_state *st, int32_t player, int32_t type, int32_t id, int32_t x, int32_t y, int32_t size,
                 int32_t is_new)
{
    /* @bin 0x004714A0 forts: the fort, then its parade ground (type 0x36, 4x4 at (x + 3, y - 1));
     * a new fort gets a formation (0x004B6E10) and the soldiers waiting in 0x006FDCE4.. */
    uint32_t r = B(id);
    tiles(st, id, x, y, size, size, (int32_t) G(0x00F5662Cu), 8);
    int32_t part;
    if (is_new) {
        int32_t f = formation_create_4b6e10(st, id);
        S16(st, r + 0x38) = 0;
        S16(st, r + 0x50) = (int16_t) f;
        /* @bin 0x00471516 per fort type: kind, and the (soldiers, count, home) record */
        int16_t kind;
        uint32_t soldiers, count, home;
        if (type == 0x28) {
            kind = 0xD, soldiers = 0x006FDCFCu, home = 0x006FDD00u, count = 0x006FDD04u;
        } else if (type == 0x2C) {
            kind = 0xB, soldiers = 0x006FDCF0u, home = 0x006FDCF4u, count = 0x006FDCF8u;
        } else {
            kind = 0xC, soldiers = 0x006FDCE4u, home = 0x006FDCE8u, count = 0x006FDCECu;
        }
        S16(st, r + 0x12) = kind;
        S16(st, FORM(f) + 4) = kind;
        if (U8(st, count) != 0) {
            U8(st, FORM(f) + 2) = U8(st, soldiers);
            fort_soldiers_472790(st, U8(st, count), f, x + 4, y + 1);
            U16(st, FORM(f) + 0x7A) = U16(st, home);
            U32(st, home) = 0xFFFFFFFFu;
            U8(st, count) = 0;
            U8(st, soldiers) = 0xFF;
        }
        part = create(st, player, 0x36, x + 3, y - 1, 0, 0);
        keep_removed(st, part);
        uint32_t pr = B(part);
        S16(st, pr + 0x50) = (int16_t) f;
        S16(st, pr + 0x38) = (int16_t) id;
        S16(st, r + 0x3A) = (int16_t) part;
        S16(st, pr + 0x3A) = 0;
    } else {
        part = S16(st, r + 0x3A);
    }
    tiles(st, part, x + 3, y - 1, 4, 4, (int32_t) G(0x00F5662Cu) + 1, 8);
}

static void storage_yard(game_state *st, int32_t player, int32_t id, int32_t x, int32_t y, int32_t size,
                         int32_t is_new)
{
    /* @bin 0x00471E5C the yard (group 0x15F) and its eight spaces (type 0x49, group 0x160) on a
     * 3x3 square, linked by +0x38 / +0x3A; a space with goods (+0x12 resource)
     * shows them (+0x3C amount) */
    static const int32_t part[9][3] = {{0, 0, 0x15F}, {1, 0, 0x160}, {2, 0, 0x160}, {0, 1, 0x160}, {1, 1, 0x160},
                                       {2, 1, 0x160}, {0, 2, 0x160}, {1, 2, 0x160}, {2, 2, 0x160}};
    int32_t cur = id, prev = id;
    if (is_new) {
        U8(st, B(id) + 0xC1) = (uint8_t) GAME_CALL(st, 0x0046B100u, id);
        S16(st, B(id) + 0x38) = 0;
    }
    for (int k = 0; k < 9; k++) {
        int32_t img = (int32_t) GROUP(part[k][2]);
        if (k > 0 && !is_new) {
            uint32_t r = B(cur);
            int32_t res = S16(st, r + 0x12);
            if (res != 0) {
                img = res < 0x1F ? (int32_t) G(0x00F566BCu) + res * 4 - 4 : (int32_t) G(0x00F57204u) + res * 4 - 0x7C;
                img += GAME_CALL(st, 0x0047D350u, res, 0);
                img += GAME_CALL(st, 0x0045B490u, (int32_t) S16(st, r + 0x3C));
            }
        }
        tiles(st, cur, x + part[k][0], y + part[k][1], size, size, img, 8);
        if (k >= 8) {
            continue;
        }
        if (is_new) {
            cur = create(st, player, 0x49, x + part[k + 1][0], y + part[k + 1][1], 0, 0);
            keep_removed(st, cur);
            S16(st, B(cur) + 0x38) = (int16_t) prev;
            S16(st, B(prev) + 0x3A) = (int16_t) cur;
        } else {
            cur = S16(st, B(cur) + 0x3A);
        }
        prev = cur;
    }
}

static int32_t sphinx_image(game_state *st, int part, int32_t keep)
{
    /* @bin 0x004719C9 the three parts' images by the view 0x00BCA2E0 (other views keep the
     * previous part's image) */
    int32_t o = S32(st, ORIENT);
    int32_t a = (int32_t) G(0x00F56D54u), m = (int32_t) G(0x00F56D5Cu), c = (int32_t) G(0x00F56D64u);
    switch (part) {
    case 0:
        return o == 0 ? a + 1 : o == 4 ? c + 1 : o == 2 ? c : a;
    case 1:
        return o == 0 || o == 4 ? m + 1 : o == 2 || o == 6 ? m : keep;
    default:
        return o == 0 ? c + 1 : o == 4 ? a + 1 : o == 2 ? a : o == 6 ? c : keep;
    }
}

static void sphinx(game_state *st, int32_t player, int32_t id, int32_t x, int32_t y, int32_t size, int32_t is_new)
{
    /* @bin 0x0047183A the Sphinx: three parts 6 tiles apart in x, +0x12 their position (0-2, or
     * 3-5 in the turned views) */
    if (is_new) {
        S16(st, CITY + 0x433A) = 1;
        anim_list_reset(st, 0x005C2750u, 0x005C28B0u); /* @bin 0x0041FF90 */
        int turned = S32(st, ORIENT) != 0 && S32(st, ORIENT) != 4;
        S16(st, B(id) + 0x12) = turned ? 3 : 0;
        S16(st, B(id) + 0x38) = 0;
        int32_t b2 = create(st, player, 0xD2, x + 6, y, 0, 0);
        keep_removed(st, b2);
        turned = S32(st, ORIENT) != 0 && S32(st, ORIENT) != 4;
        S16(st, B(b2) + 0x12) = turned ? 4 : 1;
        S16(st, B(b2) + 0x38) = (int16_t) id;
        S16(st, B(id) + 0x3A) = (int16_t) b2;
        S16(st, B(b2) + 0x3A) = 0;
        int32_t b3 = create(st, player, 0xD2, x + 0xC, y, 0, 0);
        keep_removed(st, b3);
        turned = S32(st, ORIENT) != 0 && S32(st, ORIENT) != 4;
        S16(st, B(b3) + 0x12) = turned ? 5 : 2;
        S16(st, B(b3) + 0x38) = (int16_t) b2;
        S16(st, B(b2) + 0x3A) = (int16_t) b3;
        S16(st, B(b3) + 0x3A) = 0;
        monument_slot_4f4f80(st, id);
    }
    if (main_part(st, id) == id) {
        int32_t img = sphinx_image(st, 0, 0);
        tiles(st, id, x, y, size, size, img, 8);
        int32_t b2 = S16(st, B(id) + 0x3A);
        img = sphinx_image(st, 1, img);
        tiles(st, b2, x + 6, y, size, size, img, 8);
        int32_t b3 = S16(st, B(b2) + 0x3A);
        img = sphinx_image(st, 2, img);
        tiles(st, b3, x + 0xC, y, size, size, img, 8);
    }
    sphinx_images_53d850(st);
}

static void ferry(game_state *st, int32_t id, int32_t x, int32_t y, int32_t is_new)
{
    /* @bin 0x004711CE the ferry landing: a new one gets its 0x88 record (0x0046EFD0, reset by
     * 0x004F6520); the two-step placement state 0x00BCA310 (0 first landing, 1 second, 2) */
    uint32_t r = B(id);
    if (is_new) {
        uint8_t k = (uint8_t) GAME_CALL(st, 0x0046EFD0u, id);
        U8(st, r + 0xCA) = k;
        rec88_reset_4f6520(st, 0x00EAAF68u + 0xE0u * k);
        if (S32(st, 0x00BCA310u) == 0) {
            S32(st, r + 0xC4) = 0;
        }
    }
    GAME_CALL(st, 0x0047CC70u, x, y, 0, 0);
    dock_like(st, id, x, y, 0, 0x00F57274u, 0x00F56BF4u);
    if (S32(st, 0x00BCA310u) == 1) {
        keep_removed(st, S32(st, 0x00BC9800u));
        keep_removed(st, id);
    }
    if (S32(st, 0x00BCA310u) == 2) {
        keep_removed(st, id);
    }
}

void building_map_add(game_state *st, int32_t player, int32_t type, int32_t id, int32_t x, int32_t y,
                      int32_t size, int32_t is_new, int32_t flag2)
{
    /* @bin 0x00470990 jump tables 0x004726A0 (bytes, types 10-0xEC) / 0x004724F4 */
    uint32_t r = B(id);
    if (type == 0x39) {
        /* @bin 0x004709A7 a fort part redrawn as its fort type by +0x12 */
        int16_t k = S16(st, r + 0x12);
        type = k == 0xD ? 0x28 : (k != 0xB) + 0x2C;
    }
    if ((uint32_t) (type - 10) > 0xE2u) {
        return;
    }
    int32_t img;
    int32_t flags = 8;
    switch (type) {
    case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0E: case 0x0F: case 0x10: case 0x11: case 0x12:
    case 0x13: case 0x14: case 0x15: case 0x16: case 0x17: case 0x18: case 0x19: case 0x1A: case 0x1B:
    case 0x1C: case 0x1D:
        /* @bin 0x004709EC houses: an empty unmerged house is split into vacant lots */
        if (U8(st, r + 0x87) == 0 && S16(st, r + 0x1C) == 0) {
            house_split_46cf40(st, id);
            return;
        }
        GAME_CALL(st, 0x0046CEA0u, id, type);
        return;
    case 0x1E:
        if (U16(st, B(S16(st, r + 0x3A)) + 0x10) == 0x1E) {
            GAME_CALL(st, 0x00440540u, id, x, y);
        }
        entertainment_venue(st, id, 0x1E);
        return;
    case 0x1F:
        tiles(st, id, x, y, size, size, (int32_t) G(0x00F567ACu), 8);
        entertainment_venue(st, id, 0x1F);
        return;
    case 0x21:
        tiles(st, id, x, y, 2, 2, (int32_t) G(0x00F5659Cu), 8);
        entertainment_venue(st, id, 0x21);
        return;
    case 0xD1:
        GAME_CALL(st, 0x00440060u, S32(st, r + 0xD0), 0xD1, id);
        S16(st, CITY + 0x4814) = 1;
        return;
    case 0x20: case 0x25: img = (int32_t) G(0x00F564A4u); break;
    case 0xE2: img = (int32_t) G(0x00F57214u); break;
    case 0x22: img = (int32_t) G(0x00F565B4u); break;
    case 0x23: img = (int32_t) G(0x00F565BCu); break;
    case 0x24: img = (int32_t) G(0x00F5658Cu); break;
    case 0x29: case 0x2A: case 0x2B: {
        /* @bin 0x00470C21 by +0xAA: low nibble the turn (against the view), high nibble the stage */
        uint8_t v = U8(st, r + 0xAA);
        int32_t c = (int32_t) (v & 0xF) - S32(st, ORIENT) / 2 - 1;
        if (c < 0) {
            c += 4;
        }
        int32_t hi = v >> 4;
        if (hi < 2) {
            img = (int32_t) GROUP(S32(st, 0x005D06E4u + 0x14u * (uint32_t) type)) + hi * 4 + c;
        } else {
            uint32_t g = type == 0x29 ? 0x00F5730Cu : type == 0x2A ? 0x00F57304u : 0x00F572FCu;
            img = (int32_t) G(g) + hi * 4 - 8 + c;
        }
        break;
    }
    case 0x2E: img = (int32_t) G(0x00F5663Cu); break;
    case 0x2F: img = (int32_t) G(0x00F56994u); break;
    case 0xCE: img = (int32_t) G(0x00F5664Cu); break;
    case 0x31: img = (int32_t) G(0x00F56634u); break;
    case 0x33: img = (int32_t) G(0x00F5656Cu); break;
    case 0x35: img = (int32_t) G(0x00F56574u); break;
    case 0x37: img = (int32_t) G(0x00F5661Cu); break;
    case 0x64: case 0x65: case 0x66: case 0x67: case 0x68: case 0x69: case 0xC2: case 0xC3: case 0xC4: case 0xE0:
        GAME_CALLT(st, 0x00BCA350u, 0x00488E60u, id, 0);
        return;
    case 0xC7: img = (int32_t) G(0x00F56684u); break;
    case 0x6A: img = (int32_t) G(0x00F569F4u); break;
    case 0xDD: img = (int32_t) G(0x00F56A44u); break;
    case 0x6B: img = (int32_t) GROUP(image_group_451bc0(0x19)); break;
    case 0xD8: img = (int32_t) GROUP(image_group_451bc0(0x1A)); break;
    case 0xA1: img = (int32_t) G(0x00F569E4u); break;
    case 0xD9: img = (int32_t) G(0x00F56A3Cu); break;
    case 0xA2: img = (int32_t) G(0x00F569FCu); break;
    case 0x6C: img = (int32_t) G(0x00F56624u); break;
    case 0x6D: img = (int32_t) G(0x00F5655Cu); break;
    case 0x6E: img = (int32_t) G(0x00F567BCu); break;
    case 0x6F: img = (int32_t) G(0x00F567ECu); break;
    case 0xCB: img = (int32_t) G(0x00F5657Cu); break;
    case 0xE8: img = (int32_t) G(0x00F572B4u); break;
    case 0xE9: img = (int32_t) G(0x00F572BCu); break;
    case 0xCC: img = (int32_t) G(0x00F567FCu); break;
    case 0xCD: img = (int32_t) G(0x00F567DCu); break;
    case 0x70: img = (int32_t) G(0x00F567F4u); break;
    case 0x71: img = (int32_t) G(0x00F567D4u); break;
    case 0x72: img = (int32_t) G(0x00F56804u); break;
    case 0x73: img = (int32_t) G(0x00F5699Cu); break;
    case 0xB1: img = (int32_t) G(0x00F566F4u); break;
    case 0xB2: img = (int32_t) G(0x00F565E4u); break;
    case 0xB3: img = (int32_t) G(0x00F566DCu); break;
    case 0xE7: img = (int32_t) G(0x00F572DCu); break;
    case 0x47: case 0xBF:
        storage_record(st, id, is_new);
        img = (int32_t) G(0x00F56734u);
        break;
    case 0x46: case 0xBE:
        storage_record(st, id, is_new);
        img = (int32_t) G(0x00F564CCu);
        break;
    case 0x4D: case 0x4E: case 0x4F:
        /* @bin 0x00470F53 a mansion: the city's flag +0x499C */
        S32(st, CITY + 0x499C) = 1;
        img = (int32_t) G(type == 0x4D ? 0x00F566C4u : type == 0x4E ? 0x00F566CCu : 0x00F566D4u);
        break;
    case 0x50: img = (int32_t) G(0x00F569DCu); break;
    case 0x51: img = (int32_t) G(0x00F566A4u); break;
    case 0x56: img = (int32_t) G(0x00F56614u); break;
    case 0x5C: img = (int32_t) G(0x00F564D4u); break;
    case 0xB4: img = (int32_t) G(0x00F56644u); break;
    case 0x58: {
        /* @bin 0x00471001 one of two images by the CRT rand 0x00562348 */
        int32_t g = (int32_t) G(0x00F569D4u);
        img = (GAME_CALL0(st, 0x00562348u) & 1) + g;
        break;
    }
    case 0x59: img = (int32_t) G(0x00F569D4u) + 2; break;
    case 0x5D: img = (int32_t) G(0x00F565DCu); break;
    case 0x5E:
        S32(st, CITY + 0x49D0) = 1;
        img = (int32_t) G(0x00F56984u);
        break;
    case 0xB9: case 0xBA: img = (int32_t) G(0x00F56984u); break;
    case 0x3C: case 0x97: case 0x98: case 0x99: case 0x9A: case 0x9B: img = (int32_t) G(0x00F564E4u); break;
    case 0x3D: img = (int32_t) G(0x00F564B4u); break;
    case 0x3E: img = (int32_t) G(0x00F564BCu); break;
    case 0x3F: img = (int32_t) G(0x00F564C4u); break;
    case 0x40: img = (int32_t) G(0x00F5667Cu); break;
    case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x9C: case 0x9D: case 0x9E: case 0x9F: case 0xA0: {
        /* @bin 0x004710A0 temple complexes: the god's images into slot 10 (names 0x005E018C by
         * 0x004C5120), then the three parts */
        int32_t god = GAME_CALL(st, 0x004C5120u, id);
        GAME_CALLT(st, 0x00F55BB0u, 0x0053DD90u, S32(st, 0x005E018Cu + 4u * (uint32_t) god), 10);
        temple_complex_add_4c3420(st, id, x, y, player, type, is_new);
        return;
    }
    case 0xD3: case 0xD4: {
        /* @bin 0x004710EF a complex add-on: the complex (city +0x472C) +0xA9 counts them */
        int32_t p = S32(st, 0x00EAAC28u);
        if (is_new) {
            uint32_t c = B(S32(st, CITY_ADDR(p) + 0x472C)) + 0xA9;
            U8(st, c) = (uint8_t) (U8(st, c) + (type != 0xD3) + 1);
        }
        GAME_CALL(st, 0x004C0D50u, S32(st, CITY_ADDR(p) + 0x472C), 0);
        return;
    }
    case 0x4A:
        GAME_CALL(st, 0x0047D060u, x, y, 0);
        dock_like(st, id, x, y, 1, 0x00F5724Cu, 0x00F56C0Cu);
        return;
    case 0x88:
        ferry(st, id, x, y, is_new);
        return;
    case 0x4C:
        GAME_CALL(st, 0x0047CC70u, x, y, 0, 0);
        dock_like(st, id, x, y, 0, 0x00F5726Cu, 0x00F56BCCu);
        return;
    case 0xB5:
        GAME_CALL(st, 0x0047CC70u, x, y, 0, 0);
        dock_like(st, id, x, y, 0, 0x00F5725Cu, 0x00F56BC4u);
        return;
    case 0xB6:
        GAME_CALL(st, 0x0047D060u, x, y, 0);
        dock_like(st, id, x, y, 1, 0x00F57254u, 0x00F56C1Cu);
        if (is_new) {
            S32(st, 0x00BCB9B0u) += 1;
        }
        return;
    case 0x4B: case 0xC0:
        GAME_CALL(st, 0x0047D060u, x, y, 0);
        if (is_new) {
            S32(st, 0x00705C1Cu) += 1;
        }
        dock_like(st, id, x, y, 1, 0x00F5727Cu, 0x00F56CC4u);
        return;
    case 0x5A:
        if (is_new) {
            GAME_CALL(st, 0x0047CC70u, x, y, 0, 1);
            U8(st, r + 0xAA) = U8(st, 0x00BC96D8u);
        }
        GAME_CALL(st, 0x00487EC0u, id);
        return;
    case 0xAD:
        /* @bin 0x00471440 a tower: clears terrain bits around, tiles with flags 0x8008, walls */
        terrain_and_rect_482580(st, x, y, 2, 0, 0xFF3FBFFFu);
        tiles(st, id, x, y, size, size, (int32_t) G(0x00F56854u), 0x8008);
        GAME_CALL(st, 0x0047AFD0u, x, y, 10);
        return;
    case 0xAB: case 0xCA:
        GAME_CALL(st, 0x004BFB30u, id);
        return;
    case 0x28: case 0x2C: case 0x2D:
        fort(st, player, type, id, x, y, size, is_new);
        return;
    case 0x5F:
        tiles(st, id, x, y, size, size, (int32_t) G(0x00F5694Cu), 8);
        palace_record(st, id, x, y, 0x44A0);
        return;
    case 0x32:
        S32(st, CITY + 0x4600) = 1;
        tiles(st, id, x, y, size, size, (int32_t) G(0x00F5694Cu), 8);
        palace_record(st, id, x, y, 0x45F0);
        return;
    case 0xD2:
        sphinx(st, player, id, x, y, size, is_new);
        return;
    case 0xCF: case 0xD0:
        if (is_new) {
            obelisk_add_4e57f0(st, id);
        }
        GAME_CALL(st, 0x004E5840u, id, x, y);
        return;
    case 0xD5:
        if (is_new) {
            sun_temple_add_5417b0(st, x, y, id);
            anim_list_reset(st, 0x00596088u, 0x00596158u); /* @bin 0x0041FFC0 */
            clear_construction(st);
            return;
        }
        GAME_CALL(st, 0x00541920u, id);
        return;
    case 0xE6:
        if (is_new) {
            monument_add_4015a0(st, 0x006DE650u, x, y, id, flag2);
            anim_list_reset(st, 0x005A8148u, 0x005A84C8u); /* @bin 0x004209E0 */
            clear_construction(st);
            return;
        }
        GAME_CALLT(st, 0x006DE650u, 0x004017A0u, id);
        return;
    case 0xE1:
        if (is_new) {
            monument_add_40da90(st, 0x006FD5D8u, x, y, id, flag2);
            anim_list_reset(st, 0x0059EDF0u, 0x0059EFA0u); /* @bin 0x004206A0 */
            clear_construction(st);
            return;
        }
        GAME_CALLT(st, 0x006FD5D8u, 0x0040DCA0u, id);
        return;
    case 0xE3:
        if (!is_new) {
            GAME_CALLT(st, 0x006FD368u, 0x004017A0u, id);
            return;
        }
        monument_add_417180(st, 0x006FD368u, x, y, id, 0);
        anim_list_reset(st, 0x005AD218u, 0x005AD3C8u); /* @bin 0x00420D10 */
        GAME_CALL0(st, 0x00405BA0u);
        return;
    case 0xE5: case 0xEA: case 0xEB: case 0xEC: {
        /* @bin 0x00471C52 the big tombs: their objects and part functions */
        uint32_t self = type == 0xE5 ? 0x006F9320u : type == 0xEA ? 0x006F5648u : type == 0xEB ? 0x006DE9E0u : 0x006E9188u;
        uint32_t fn = type == 0xE5 ? 0x00545C30u : type == 0xEA ? 0x00559130u : type == 0xEB ? 0x00556480u : 0x0054FBA0u;
        if (!is_new) {
            GAME_CALLT(st, self, 0x005468D0u, id);
            return;
        }
        tomb_add(st, fn, self, x, y, id, type == 0xEC ? 0 : flag2);
        anim_lists_reset_4212f0(st);
        if (type == 0xEC) {
            GAME_CALL0(st, 0x00405BA0u);
        } else {
            U32(st, 0x006059E8u) = 0;
        }
        return;
    }
    case 0xE4:
        if (is_new) {
            monument_add_4e6840(st, 0x006DE548u, x, y, id, flag2);
            anim_list_reset(st, 0x005B0060u, 0x005B0150u); /* @bin 0x00421010 */
            clear_construction(st);
            return;
        }
        GAME_CALLT(st, 0x006DE548u, 0x004E69C0u, id);
        return;
    case 0xDE:
        if (is_new) {
            mastaba_add_4df9d0(st, x, y, id, flag2);
            clear_construction(st);
            return;
        }
        GAME_CALL(st, 0x004DFBB0u, id);
        return;
    case 0x48: case 0xC1:
        storage_yard(st, player, id, x, y, size, is_new);
        return;
    case 0x54: case 0x55: case 0xBB: case 0xBC: case 0xBD:
        /* @bin 0x00472057 palaces: the city's flag +0x4486, the first palace record +0x2788 (x, y, tile, id) */
        S16(st, CITY + 0x4486) = 1;
        img = (int32_t) G(type == 0xBC ? 0x00F56554u : type == 0xBD ? 0x00F564ACu : 0x00F56594u);
        tiles(st, id, x, y, size, size, img, 8);
        palace_record(st, id, x, y, 0x2784);
        return;
    case 0x8A:
        /* @bin 0x00472272 flags 0x48 and the tile's terrain bit 0x100 */
        flags = (int32_t) ((U32(st, TERRAIN + 4u * (uint32_t) (S32(st, MAP_START) + y * GRID_SIZE + x)) & 0x100) | 0x48);
        img = (int32_t) G(0x00F5672Cu);
        break;
    case 0x8C: case 0x8D: case 0x8E: case 0x8F: case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95:
        img = (int32_t) GROUP(S32(st, 0x005D06E4u + 0x14u * (uint32_t) type));
        break;
    case 0xA7: img = (int32_t) G(0x00F5668Cu); break;
    case 0xB8: img = (int32_t) G(0x00F5660Cu); break;
    case 0xB7:
        /* @bin 0x004722FD the pyramid */
        if (is_new) {
            pyramid_add_4ee780(st, player, x, y, size, id);
        }
        if (S16(st, r + 0x38) == 0) {
            GAME_CALL(st, 0x004EE4A0u, id);
        }
        if (is_new) {
            GAME_CALL0(st, 0x00405BA0u);
        }
        return;
    case 0xDA: img = ((S32(st, ORIENT) / 2) & 1) + (int32_t) G(0x00F57374u); break;
    case 0xDB: img = ((S32(st, ORIENT) / 2) & 1) + (int32_t) G(0x00F5737Cu); break;
    case 0xDC:
        GAME_CALL(st, 0x0047CF00u, id, x, y, S32(st, ORIENT) / 2 + (int32_t) G(0x00F57384u));
        return;
    case 0xD7:
        img = (int32_t) G(0x00F56B3Cu);
        flags = 0x48;
        break;
    case 0x63: {
        /* @bin 0x0047244C at the record's own tile: rubble (state 4) or one of four images by the
         * random grid */
        int32_t bx = S16(st, r + 6), by = S16(st, r + 8);
        if (U8(st, r) == 4) {
            GAME_CALL(st, 0x0047C2B0u, id, bx, by, (int32_t) U8(st, r + 3), (int32_t) U8(st, r + 3));
            return;
        }
        uint32_t c = U8(st, RANDOM + U32(st, r + 0xC)) & 3;
        if (U8(st, r + 5) != 0 && S16(st, r + 0x12) <= 1) {
            img = (int32_t) G(0x00F56C54u);
        } else {
            if ((int32_t) c >= 3) {
                c = 3;
            }
            img = (int32_t) G(0x00F56C5Cu) + (int32_t) c * 9;
        }
        tiles(st, id, bx, by, 1, 1, img, 8);
        return;
    }
    default:
        return;
    }
    tiles(st, id, x, y, size, size, img, flags);
}

/* ---- helpers only 0x00470990 calls ---------------------------------------------------------- */

void house_split_46cf40(game_state *st, int32_t id)
{
    /* @bin 0x0046CF40 an empty house becomes a vacant lot (type 10); a merged one (+4) splits
     * into four 1x1 lots (0x0047C480 clears its tiles) */
    uint32_t r = B(id);
    int32_t img = (int32_t) G(0x00F5653Cu);
    S16(st, r + 0x10) = 0xA;
    S16(st, r + 0x12) = 0;
    if (U8(st, r + 4) == 0) {
        U32(st, IMAGE + 4u * U32(st, r + 0xC)) = (uint32_t) img;
        return;
    }
    GAME_CALL(st, 0x0047C480u, id, (int32_t) S16(st, r + 6), (int32_t) S16(st, r + 8));
    U8(st, r + 4) = 0;
    U8(st, r + 3) = 1;
    U8(st, r + 5) = 1;
    tiles(st, id, S16(st, r + 6), S16(st, r + 8), 1, 1, img, 8);
    int32_t x = S16(st, r + 6), y = S16(st, r + 8);
    static const int32_t at[3][2] = {{1, 0}, {0, 1}, {1, 1}};
    for (int k = 0; k < 3; k++) {
        int32_t b = create(st, S32(st, 0x00EAAC28u), S16(st, r + 0x10), x + at[k][0], y + at[k][1], 0, 0);
        S16(st, B(b) + 0x1C) = 0;
        S16(st, B(b) + 0x20) = 0;
        tiles(st, b, S16(st, B(b) + 6), S16(st, B(b) + 8), 1, 1, img, 8);
    }
}

int32_t image_group_451bc0(int32_t v)
{
    /* @bin 0x00451BC0 group 0x1B7 for 0x19, else 0x133 */
    return v == 0x19 ? 0x1B7 : 0x133;
}

void fort_soldiers_472790(game_state *st, int32_t n, int32_t f, int32_t x, int32_t y)
{
    /* @bin 0x00472790 n soldiers (figures of the formation's type) for formation f, at the
     * city's first entry point reachable from the formation's tile (city +0x2778 / +0x2780, by
     * 0x00518400), else at (x, y); then the formations again (0x004B7C80) */
    uint32_t fr = FORM(f);
    int32_t owner_b = S16(st, fr + 6);
    for (int32_t i = n; i > 0; i--) {
        int32_t ex = S16(st, CITY + 0x2778), ey = S16(st, CITY + 0x277A);
        if (GAME_CALL(st, 0x00518400u, ex, ey, (int32_t) S16(st, fr + 0x2E), (int32_t) S16(st, fr + 0x30)) == 0) {
            ex = S16(st, CITY + 0x2780);
            ey = S16(st, CITY + 0x2782);
            if (GAME_CALL(st, 0x00518400u, ex, ey, (int32_t) S16(st, fr + 0x2E), (int32_t) S16(st, fr + 0x30)) == 0) {
                ex = x;
                ey = y;
            }
        }
        int32_t fig = GAME_CALL(st, 0x004B4800u, (int32_t) U8(st, B(owner_b) + 1), (int32_t) S16(st, fr + 4), ex, ey, 0);
        uint32_t fa = FIGURE_ADDR(fig);
        S16(st, fa + 0x60) = (int16_t) f;
        U8(st, fa + 0x63) = 1;
        U8(st, fa + 0x38) = 0x10;
    }
    GAME_CALL0(st, 0x004B7C80u);
}

int32_t shore_fit_47d060(game_state *st, int32_t x, int32_t y, int32_t turn)
{
    /* @bin 0x0047D060 a 3x3 dock on the shore: the first of four turns (0x00BC96DC) whose
     * tiles 0x005CCCEC are water where table 0x005DBE48 says (bit 4) and land elsewhere, and
     * whose two front tiles 0x005DBED8 are water; returns the count of tiles that block
     * (roads / bit 0x40, ... ) or 999. 0x00BC96D8 the turn, 0x00BC96DC it against the view */
    if (turn == 1 && S32(st, ORIENT) != 0) {
        int32_t o = S32(st, ORIENT);
        if (o == 6) {
            y -= 2;
        } else if (o == 4) {
            x -= 2;
            y -= 2;
        } else if (o == 2) {
            x -= 2;
        }
    }
    if (x < 0 || y < 0 || x + 2 >= S32(st, MAP_W) || y + 2 >= S32(st, MAP_H)) {
        return 999;
    }
    int32_t base = S32(st, MAP_START) + y * GRID_SIZE + x;
    for (int32_t k = 0; k < 4; k++) {
        S32(st, 0x00BC96DCu) = k;
        int32_t count = 0, blocked = 0;
        for (int i = 0; i < 9; i++) {
            int32_t t = S32(st, 0x005CCCECu + 4u * (uint32_t) i) + base;
            uint32_t v = U32(st, TERRAIN + 4u * (uint32_t) t);
            if (S32(st, 0x005DBE48u + 36u * (uint32_t) k + 4u * (uint32_t) i) != 0) {
                if (!(v & 4)) {
                    break;
                }
                count++;
                if (v & 0x40) {
                    blocked++;
                }
                if (v & 1) {
                    blocked++;
                }
            } else {
                if (v & 0x10004) {
                    break;
                }
                uint32_t p = U32(st, 0x00D05720u + 4u * (uint32_t) t);
                if (p != 0 && P8(st, p + 0x18) == 0) {
                    break;
                }
                count++;
                if ((v & 0x40) || (v & 0x26341237u) || (v & 8)) {
                    blocked++;
                }
            }
        }
        for (int j = 0; j < 2; j++) {
            int32_t t = S32(st, 0x005DBED8u + 8u * (uint32_t) k + 4u * (uint32_t) j) + base;
            if (!(U8(st, TERRAIN + 4u * (uint32_t) t) & 4)) {
                count = 0;
            }
        }
        if (count == 9) {
            S32(st, 0x00BC96D8u) = k;
            int32_t v = k - S32(st, ORIENT) / 2;
            S32(st, 0x00BC96DCu) = v;
            if (v < 0) {
                S32(st, 0x00BC96DCu) = v + 4;
            }
            return blocked;
        }
    }
    S32(st, 0x00BC96DCu) = 4;
    return 999;
}

void terrain_and_rect_482580(game_state *st, int32_t x, int32_t y, int32_t size, int32_t r, uint32_t mask)
{
    /* @bin 0x00482580 terrain &= mask over the square grown by r, clipped to the map; the cursor
     * 0x00BC9768 and the loop globals 0x00D3B4E0 / 0x00D3B4E4 as the original leaves them */
    int32_t x0 = x - r, x1 = x + size + r - 1;
    int32_t y0 = y - r, y1 = y + size + r - 1;
    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x1 >= S32(st, MAP_W)) {
        x1 = S32(st, MAP_W) - 1;
    }
    if (y1 >= S32(st, MAP_H)) {
        y1 = S32(st, MAP_H) - 1;
    }
    S32(st, 0x00D3B4E0u) = y0;
    int32_t cur = S32(st, MAP_START) + y0 * GRID_SIZE + x0;
    S32(st, CURSOR) = cur;
    if (y0 > y1) {
        return;
    }
    int32_t rows = y1 - y0 + 1;
    S32(st, 0x00D3B4E0u) = rows + y0;
    for (; rows > 0; rows--) {
        S32(st, 0x00D3B4E4u) = x0;
        if (x0 <= x1) {
            int32_t n = x1 - x0 + 1;
            uint32_t t = (uint32_t) cur;
            cur += n;
            S32(st, 0x00D3B4E4u) = n + x0;
            for (; n > 0; n--, t++) {
                U32(st, TERRAIN + 4u * t) &= mask;
            }
        }
        cur += x0 - x1 + 0xE3;
    }
    S32(st, CURSOR) = cur;
}

int32_t formation_create_4b6e10(game_state *st, int32_t id)
{
    /* @bin 0x004B6E10 a fort's formation: the first free one of 1-49 (+0), its kind +2 the first
     * of 0-7 not used by a waiting group (0x006FDCE4 / F0 / FC) nor a live formation with +0x42
     * = 1; its standard bearer figure (type 0xE); 0x00BCB9B4 counts forts, 0x00BCB9BC the
     * highest id. Returns the formation, 0 if none. */
    GAME_CALL0(st, 0x004B77D0u);
    uint8_t used[256] = {0};
    uint32_t a = U8(st, 0x006FDCE4u), b = U8(st, 0x006FDCF0u), c = U8(st, 0x006FDCFCu);
    for (uint32_t i = 0; i < 8; i++) {
        if (a == i) {
            used[i] = 1;
        }
        if (b == i) {
            used[i] = 1;
        }
        if (c == i) {
            used[i] = 1;
        }
    }
    for (int32_t f = 0; f < 50; f++) {
        uint32_t fr = FORM(f);
        if (U8(st, fr) != 0 && U8(st, fr + 0x42) == 1) {
            used[U8(st, fr + 2)] = 1;
        }
    }
    int32_t kind = 0;
    for (int32_t i = 0; i < 8; i++) {
        if (!used[i]) {
            kind = i;
            break;
        }
    }
    for (int32_t f = 1; f < 50; f++) {
        uint32_t fr = FORM(f);
        if (U8(st, fr) != 0) {
            continue;
        }
        uint32_t r = B(id);
        U8(st, fr) = 1;
        U8(st, fr + 0x42) = 1;
        U8(st, fr + 1) = U8(st, 0x00EAAC28u);
        U16(st, fr + 4) = U16(st, r + 0x12);
        S16(st, fr + 6) = (int16_t) id;
        int16_t ry = S16(st, r + 8);
        S16(st, fr + 0x2A) = 1;
        S16(st, fr + 0x2C) = 0x32;
        U8(st, fr + 3) = 1;
        U16(st, fr + 0x82) = 0xFFFF;
        U8(st, fr + 0x77) = 0;
        U8(st, fr + 2) = (uint8_t) kind;
        int16_t fx = (int16_t) (S16(st, r + 6) + 3), fy = (int16_t) (ry - 1);
        S16(st, fr + 0x2E) = fx;
        S16(st, fr + 0x30) = fy;
        S16(st, fr + 0x32) = fx;
        S16(st, fr + 0x34) = fy;
        S16(st, fr + 0x36) = fx;
        S16(st, fr + 0x38) = fy;
        S16(st, fr + 0x80) = 0;
        S16(st, fr + 0x7E) = 0;
        int32_t fig = GAME_CALL(st, 0x004B4800u, (int32_t) U8(st, r + 1), 0xE, 0, 0, 0);
        S16(st, fr + 0x40) = (int16_t) fig;
        S16(st, FIGURE_ADDR(fig) + 0x5A) = (int16_t) id;
        S16(st, FIGURE_ADDR(fig) + 0x60) = (int16_t) f;
        S32(st, 0x00BCB9B4u) += 1;
        if (f > S32(st, 0x00BCB9BCu)) {
            S32(st, 0x00BCB9BCu) = f;
        }
        return f;
    }
    return 0;
}

void temple_complex_add_4c3420(game_state *st, int32_t id, int32_t x, int32_t y, int32_t player, int32_t type,
                               int32_t is_new)
{
    /* @bin 0x004C3420 a temple complex's three 3x3 parts along the turn +0xAA (offsets
     * 0x005DF970 by turn, images 0x005DF8B0 by view and turn; the middle part group 0x23A); a new
     * one is the city's complex (+0x4478, +0x448A, +0x472C) */
    uint32_t r = B(id);
    if (S16(st, r + 0x38) != 0) {
        return;
    }
    int32_t turn;
    if (is_new) {
        U8(st, r + 0xA9) = 0;
        S16(st, CITY + 0x4478) = 1;
        S16(st, CITY + 0x448A) = 2;
        S32(st, CITY + 0x472C) = id;
        turn = S32(st, 0x00BC9714u) * 2 - S32(st, ORIENT);
        if (turn < 0) {
            turn += 8;
        }
        U8(st, r + 0xAA) = (uint8_t) turn;
    } else {
        turn = U8(st, r + 0xAA);
    }
    int32_t cur = id;     /* edi: the part drawn (the main one when not new) */
    uint32_t cr = r;      /* esi: its record */
    int32_t dx = S32(st, 0x005DF970u + 8u * (uint32_t) turn), dy = S32(st, 0x005DF974u + 8u * (uint32_t) turn);
    for (int32_t k = 0; k < 3; k++) {
        int32_t grp;
        int32_t sel = S32(st, ORIENT) / 2 + (turn / 2) * 4;
        if (k == 0) {
            grp = S32(st, 0x005DF8B0u + 12u * (uint32_t) sel);
            if (is_new) {
                S16(st, cr + 0x38) = 0;
            }
        } else {
            if (is_new) {
                int32_t prev = k == 1 ? id : S16(st, B(id) + 0x3A);
                cur = create(st, player, type, dx * k + x, dy * k + y, 0, 0);
                cr = B(cur);
                U8(st, cr + 0xAA) = (uint8_t) turn;
                keep_removed(st, cur);
                S16(st, cr + 0x38) = (int16_t) prev;
                S16(st, B(prev) + 0x3A) = (int16_t) cur;
                S16(st, cr + 0x3A) = 0;
            }
            if (k == 1) {
                grp = 0x23A;
            } else {
                sel = S32(st, ORIENT) / 2 + (turn / 2) * 4;
                grp = S32(st, 0x005DF8B0u + 4u * (uint32_t) (sel * 3 + k));
            }
        }
        if (is_new) {
            /* @bin 0x004C362B jump table 0x004C3708 */
            if (turn == 0 || turn == 4) {
                S16(st, cr + 0x12) = (int16_t) k;
            } else if (turn == 2 || turn == 6) {
                S16(st, cr + 0x12) = (int16_t) (k + 3);
            }
        }
        int32_t o = S32(st, ORIENT);
        int32_t add;
        if (o == 0 || o == 4) {
            add = turn == 2 || turn == 6 ? 3 : 0;
        } else {
            add = turn == 2 || turn == 6 ? 0 : 3;
        }
        tiles(st, cur, dx * k + x, dy * k + y, 3, 3, (int32_t) GROUP(grp) + add, 8);
    }
    GAME_CALL(st, 0x004C0BE0u, main_part(st, cur));
    S32(st, CITY + 0x4990) = GAME_CALL(st, 0x004C5120u, cur) == 4 ? 10 : 0;
}

int32_t rec88_reset_4f6520(game_state *st, uint32_t self)
{
    /* @bin 0x004F6520 (thiscall) a ferry record: +4, +8 := 0, +0xC := 50 entries at +0x14 := 0 */
    P32(st, self + 4) = 0;
    P32(st, self + 8) = 0;
    P32(st, self + 0xC) = 0x32;
    for (int32_t i = 0; i < (int32_t) P32(st, self + 0xC); i++) {
        P32(st, self + 0x14 + 4u * (uint32_t) i) = 0;
    }
    return 1;
}

void sphinx_images_53d850(game_state *st)
{
    /* @bin 0x0053D850 the Sphinx's image files into slots 5-7 by city +0x433A (names 0x00601660,
     * 3 per value), and slot 8 */
    for (int32_t k = 0; k < 3; k++) {
        int32_t v = S16(st, CITY + 0x433A);
        GAME_CALLT(st, 0x00F55BB0u, 0x0053DD90u, S32(st, 0x00601660u + 4u * (uint32_t) k + 12u * (uint32_t) v), 5 + k);
    }
    GAME_CALLT(st, 0x00F55BB0u, 0x0053DD90u, 0x005791B8, 8);
}

void anim_list_reset(game_state *st, uint32_t from, uint32_t to)
{
    /* @bin 0x0041FF90 (and 0x0041FFC0 0x004206A0 0x004209E0 0x00420D10 0x00421010) each
     * animation object of a list of 16-byte entries: +0x12 := 0, restarted in mode 3
     * (0x0041FCA0), +0xC := the entry's second word */
    for (uint32_t e = from; e < to; e += 0x10) {
        P16(st, U32(st, e) + 0x12) = 0;
        GAME_CALLT(st, U32(st, e), 0x0041FCA0u, 3);
        P32(st, U32(st, e) + 0xC) = U32(st, e + 4);
    }
}

void anim_lists_reset_4212f0(game_state *st)
{
    /* @bin 0x004212F0 the big tombs' 24 lists of three, in this order; the fifth list takes its
     * second words from 0x005BE574 (the original's offset) */
    static const uint32_t lists[] = {0x005BE510u, 0x005BE540u, 0x005BE570u, 0x005BE750u, 0, 0x005BE5A0u,
                                     0x005BE5D0u, 0x005BE600u, 0x005BE630u, 0x005BE7B0u, 0x005BE7E0u, 0x005BE660u,
                                     0x005BE690u, 0x005BE8A0u, 0x005BE810u, 0x005BE8D0u, 0x005BE900u, 0x005BE6C0u,
                                     0x005BE6F0u, 0x005BE720u, 0x005BE840u, 0x005BE870u, 0x005BE930u, 0x005BE960u,
                                     0x005BE990u};
    for (unsigned k = 0; k < sizeof(lists) / sizeof(lists[0]); k++) {
        if (lists[k] != 0) {
            anim_list_reset(st, lists[k], lists[k] + 0x30);
            continue;
        }
        for (uint32_t i = 0; i < 0x30; i += 0x10) {
            uint32_t obj = U32(st, 0x005BE780u + i);
            P16(st, obj + 0x12) = 0;
            GAME_CALLT(st, obj, 0x0041FCA0u, 3);
            P32(st, U32(st, 0x005BE780u + i) + 0xC) = U32(st, 0x005BE574u + i);
        }
    }
}

/* ---- the table -------------------------------------------------------------------------------- */

static int32_t run_map_add(game_state *st, const int32_t *a)
{
    building_map_add(st, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
    return 0;
}

static int32_t run_house_split(game_state *st, const int32_t *a)
{
    house_split_46cf40(st, a[0]);
    return 0;
}

static int32_t run_group_451bc0(game_state *st, const int32_t *a)
{
    (void) st;
    return image_group_451bc0(a[0]);
}

static int32_t run_fort_soldiers(game_state *st, const int32_t *a)
{
    fort_soldiers_472790(st, a[0], a[1], a[2], a[3]);
    return 0;
}

static int32_t run_shore_fit(game_state *st, const int32_t *a)
{
    return shore_fit_47d060(st, a[0], a[1], a[2]);
}

static int32_t run_and_rect(game_state *st, const int32_t *a)
{
    terrain_and_rect_482580(st, a[0], a[1], a[2], a[3], (uint32_t) a[4]);
    return 0;
}

static int32_t run_formation_create(game_state *st, const int32_t *a)
{
    return formation_create_4b6e10(st, a[0]);
}

static int32_t run_temple_complex(game_state *st, const int32_t *a)
{
    temple_complex_add_4c3420(st, a[0], a[1], a[2], a[3], a[4], a[5]);
    return 0;
}

static int32_t run_rec88_reset(game_state *st, const int32_t *a)
{
    return rec88_reset_4f6520(st, (uint32_t) a[PORT_THIS_ARG]);
}

static int32_t run_sphinx_images(game_state *st, const int32_t *a)
{
    (void) a;
    sphinx_images_53d850(st);
    return 0;
}

static int32_t run_anim_lists(game_state *st, const int32_t *a)
{
    (void) a;
    anim_lists_reset_4212f0(st);
    return 0;
}

#define LIST(name, from, to) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        anim_list_reset(st, from, to); \
        return 0; \
    }
LIST(run_list_41ff90, 0x005C2750u, 0x005C28B0u)
LIST(run_list_41ffc0, 0x00596088u, 0x00596158u)
LIST(run_list_4206a0, 0x0059EDF0u, 0x0059EFA0u)
LIST(run_list_4209e0, 0x005A8148u, 0x005A84C8u)
LIST(run_list_420d10, 0x005AD218u, 0x005AD3C8u)
LIST(run_list_421010, 0x005B0060u, 0x005B0150u)

extern const ported_function map_add_monument_ports[];
extern const int map_add_monument_ports_count;

const ported_function map_add_ports[] = {
    {0x00470990u, "building_map_add", 8, 0, 0, run_map_add, 0},
    {0x0046CF40u, "house_split_46cf40", 1, 0, 0, run_house_split, 0},
    {0x00451BC0u, "image_group_451bc0", 1, 32, 0, run_group_451bc0, 0},
    {0x00472790u, "fort_soldiers_472790", 4, 0, 0, run_fort_soldiers, 0},
    {0x0047D060u, "shore_fit_47d060", 3, 32, 0, run_shore_fit, 0},
    {0x00482580u, "terrain_and_rect_482580", 5, 0, 0, run_and_rect, 0},
    {0x004B6E10u, "formation_create_4b6e10", 1, 32, 0, run_formation_create, 0},
    {0x004C3420u, "temple_complex_add_4c3420", 6, 0, 0, run_temple_complex, 0},
    {0x004F6520u, "rec88_reset_4f6520", 0, 32, 0, run_rec88_reset, 0},
    {0x0053D850u, "sphinx_images_53d850", 0, 0, 0, run_sphinx_images, 0},
    {0x004212F0u, "anim_lists_reset_4212f0", 0, 0, 0, run_anim_lists, 0},
    {0x0041FF90u, "anim_list_reset_41ff90", 0, 0, 0, run_list_41ff90, 0},
    {0x0041FFC0u, "anim_list_reset_41ffc0", 0, 0, 0, run_list_41ffc0, 0},
    {0x004206A0u, "anim_list_reset_4206a0", 0, 0, 0, run_list_4206a0, 0},
    {0x004209E0u, "anim_list_reset_4209e0", 0, 0, 0, run_list_4209e0, 0},
    {0x00420D10u, "anim_list_reset_420d10", 0, 0, 0, run_list_420d10, 0},
    {0x00421010u, "anim_list_reset_421010", 0, 0, 0, run_list_421010, 0},
};
const int map_add_ports_count = sizeof(map_add_ports) / sizeof(map_add_ports[0]);
