/* Imhotep - the stock / level markers of buildings 0x00436310 (package cityview).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Called by the upper pass 0x00436B60 and the overlay tops 0x00439F10 for each drawn building tile:
 * by the building type, an image of a group showing (+0x3C - 100) / 100 and (+0xCC - 100) / 100
 * (signed division, capped), at an offset from the tile. */
#include "city.h"

#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

static void mark(game_state *st, int32_t level, int32_t cap, uint32_t group, int32_t x, int32_t y)
{
    if (level < 0) {
        return;
    }
    if (level > cap) {
        level = cap;
    }
    cv_image_draw(st, level + S32(st, group), x, y);
}

void building_levels_draw(game_state *st, int32_t b, int32_t x, int32_t y)
{
    /* @bin 0x00436310 (building, x, y): the cases of the jump table 0x00436A28 / 0x00436A88 */
    uint32_t r = BUILDING_ADDR(b);
    int32_t a = (S16(st, r + BLD_STORED) - 100) / 100;
    int32_t c = (S16(st, r + BLD_STOCK2) - 100) / 100;
    switch (S16(st, r + BLD_TYPE)) {
    case 0x20:
        mark(st, a, 2, G_IMAGE_GROUP(467), x + 0x6B, y + 5);
        break;
    case 0x2F:
        mark(st, a, 4, G_IMAGE_GROUP(468), x + 0x31, y + 0x10);
        break;
    case 0x33:
        mark(st, a, 2, G_IMAGE_GROUP(329), x + 0x1C, y + 0xF);
        break;
    case 0x35:
        mark(st, a, 2, G_IMAGE_GROUP(329), x + 0x3D, y - 0xD);
        break;
    case 0x4A: {
        /* @bin 0x004366A8 the place by the building's direction +0xAA turned by orientation / 2 */
        static const int32_t places[8] = {0x46, 0x19, 0x78, -0x19, 0x5A, 0xA, 0x28, -5};
        if (a < 0) {
            break;
        }
        int32_t d = (int32_t) U8(st, r + BLD_ORIENTATION) - S32(st, G_VIEW_ORIENTATION) / 2;
        if (d < 0) {
            d += 4;
        }
        if (a > 4) {
            a = 4;
        }
        if (d > 3) { /* past the 4 places the original reads its saved registers: not reproduced, reported as unported */
            state_unported(st, FN_BUILDING_LEVELS_DRAW);
            break;
        }
        cv_image_draw(st, a + S32(st, G_IMAGE_GROUP(471)), places[d * 2] + x, places[d * 2 + 1] + y);
        break;
    }
    case 0x5F:
        mark(st, a, 1, G_IMAGE_GROUP(470), x + 0x83, y + 2);
        mark(st, c, 1, G_IMAGE_GROUP(478), x + 0x4A, y + 10);
        break;
    case 0x6C:
        mark(st, a, 4, G_IMAGE_GROUP(471), x + 0x41, y + 3);
        break;
    case 0x6E:
        mark(st, a, 2, G_IMAGE_GROUP(477), x + 0x2B, y - 1);
        break;
    case 0x6F:
        mark(st, a, 1, G_IMAGE_GROUP(479), x + 0x29, y + 1);
        break;
    case 0x70:
        mark(st, a, 1, G_IMAGE_GROUP(472), x + 0x45, y + 0x13);
        break;
    case 0x71:
        mark(st, a, 1, G_IMAGE_GROUP(469), x + 0x3F, y - 1);
        break;
    case 0x72:
        mark(st, a, 1, G_IMAGE_GROUP(476), x + 0x41, y + 0x14);
        break;
    case 0x73:
        mark(st, a, 4, G_IMAGE_GROUP(474), x + 0x3D, y + 0xD);
        break;
    case 0xB1:
        mark(st, a, 4, G_IMAGE_GROUP(471), x + 0x29, y + 8);
        break;
    case 0xC2:
        mark(st, a, 1, G_IMAGE_GROUP(473), x + 0x26, y + 7);
        break;
    case 0xC3:
        mark(st, a, 4, G_IMAGE_GROUP(475), x + 0x22, y + 0x11);
        break;
    case 0xCB:
        mark(st, a, 4, G_IMAGE_GROUP(475), x + 0x23, y + 4);
        break;
    case 0xCC:
        mark(st, c, 1, G_IMAGE_GROUP(473), x + 0x5B, y + 2);
        mark(st, a, 1, G_IMAGE_GROUP(476), x + 0x34, y - 0x11);
        break;
    case 0xCD:
        mark(st, a, 4, G_IMAGE_GROUP(471), x + 0xA6, y - 0x17);
        break;
    case 0xE2:
        mark(st, a, 2, G_IMAGE_GROUP(474), x + 200, y - 10);
        mark(st, c, 2, G_IMAGE_GROUP(473), x + 0xDC, y + 10);
        break;
    case 0xE7:
        mark(st, c, 4, G_IMAGE_GROUP(476), x + 1, y + 7);
        mark(st, a, 4, G_IMAGE_GROUP(744), x + 0x2E, y + 0xC);
        break;
    case 0xE8:
        mark(st, c, 1, G_IMAGE_GROUP(738), x + 0x32, y + 0x14);
        mark(st, a, 1, G_IMAGE_GROUP(739), x + 0xF, y + 8);
        break;
    case 0xE9:
        mark(st, a, 1, G_IMAGE_GROUP(740), x + 0x2D, y + 0x19);
        break;
    default:
        break;
    }
}

static int32_t run_levels(game_state *st, const int32_t *a)
{
    building_levels_draw(st, a[0], a[1], a[2]);
    return 0;
}

const ported_function cityview_levels_ports[] = {
    {0x00436310u, "building_levels_draw", 3, 0, 0, run_levels, 0},
};
const int cityview_levels_ports_count = sizeof(cityview_levels_ports) / sizeof(cityview_levels_ports[0]);
