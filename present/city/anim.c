/* Imhotep - building animation frames 0x0040F2C0 (package cityview).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Called by the upper pass 0x00436B60 and the overlay tops 0x00439F10 for each visible building
 * tile whose image animates: decides whether the building animates now (by type and state) and, when
 * the image's timer flag 0x00605B38[entry +0x3A] is up (set every 20 * k ms by the frame's timers
 * 0x0040F270), advances the tile's frame in the sprite grid (chunk 14, state the simulation reads:
 * docs/presentation.md 1.4). */
#include "city.h"

#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

int32_t building_area_state(game_state *st, int32_t b, int32_t r)
{
    /* @bin 0x0047FD30 (building, radius) the buildings within radius of b's tile (+6 x, +8 y), the
     * area clamped to the map; the scan's row and column are the globals 0x00D3B4E0 / 0x00D3B4E4.
     * 0: one of them in use (+0x05) has +0x77 = 0; 1: some in use, all with +0x77; 2: none */
    uint32_t rec = BUILDING_ADDR(b);
    int32_t x0 = S16(st, rec + BLD_X);
    int32_t y0 = S16(st, rec + BLD_Y);
    int32_t x1 = x0 + r;
    x0 = x0 - r;
    int32_t y1 = y0 + r;
    int32_t y = y0 - r;
    if (x0 < 0) {
        x0 = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (x1 >= S32(st, G_MAP_WIDTH)) {
        x1 = S32(st, G_MAP_WIDTH) - 1;
    }
    if (y1 >= S32(st, G_MAP_HEIGHT)) {
        y1 = S32(st, G_MAP_HEIGHT) - 1;
    }
    int32_t seen = 0;
    int32_t skip = x0 - x1 + 0xE3;
    S32(st, G_ROAD_ACCESS_Y) = y;
    int32_t off = S32(st, G_MAP_START) + y * 0xE4 + x0;
    if (y > y1) {
        return 2;
    }
    do {
        S32(st, G_ROAD_ACCESS_X) = x0;
        for (int32_t x = x0; x <= x1;) {
            int32_t o = S16(st, G_BUILDING_GRID + 2u * (uint32_t) off);
            if (o > 0 && U8(st, BUILDING_ADDR(o) + BLD_HOUSE_SIZE) != 0) {
                seen++;
                if (U8(st, BUILDING_ADDR(o) + BLD_ACCESS_WATER) == 0) {
                    return 0;
                }
            }
            x++;
            off++;
            S32(st, G_ROAD_ACCESS_X) = x;
        }
        off += skip;
        S32(st, G_ROAD_ACCESS_Y) = S32(st, G_ROAD_ACCESS_Y) + 1;
    } while (S32(st, G_ROAD_ACCESS_Y) <= y1);
    return seen != 0 ? 1 : 2;
}

static int animates(game_state *st, int32_t b)
{
    /* @bin 0x0040F2E9 whether building b animates now: 0 none, 1 yes, -1 return 1 (wells: 0x5C) */
    uint32_t r = BUILDING_ADDR(b);
    int16_t workers = S16(st, r + BLD_WORKERS);
    if ((uint8_t) GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_NEEDS_AREA_CHECK, b) != 0) {
        return workers > 0;
    }
    int16_t type = S16(st, r + BLD_TYPE);
    if (type == 0xE8) {
        return workers > 0 && S16(st, r + BLD_STORED) > 0 && S16(st, r + BLD_STOCK2) > 0;
    }
    if (type == 0x5C) {
        int32_t k = building_area_state(st, b, 2);
        return (k == 1 || k == 2) ? 0 : -1;
    }
    if (type == 0xB4 || type == 0x5A) {
        return workers > 0 && U8(st, r + BLD_WATER_SUPPLY) != 0;
    }
    int generic = 0;
    if (GAME_CALL(st, FN_BUILDING_TYPE_0X459EF0, b) == 0) {
        type = S16(st, r + BLD_TYPE);
        if (type >= 0x6E && type <= 0x72) {
            generic = 0;
        } else {
            switch (type) {
            /* @bin 0x0040F754 the switch's byte table */
            case 0x20: case 0x2F: case 0x33: case 0x35: case 0x4A: case 0x71: case 0xB1: case 0xB2:
            case 0xBB: case 0xBC: case 0xBD: case 0xC2: case 0xCB: case 0xCC: case 0xCD: case 0xE2:
            case 0xE7: case 0xE8: case 0xE9:
                generic = 0;
                break;
            default:
                generic = 1;
                break;
            }
        }
        if (!generic) {
            /* @bin 0x0040F41C */
            if (S32(st, G_FREE_BUILD) == 0) {
                int need = 1;
                if (type == 0x4A) {
                    uint16_t v = U16(st, r + BLD_SECOND_WALKER);
                    if ((v & 0xF000) == 0x1000 && (v & 0xFFF) == 0x19) {
                        need = 0;
                    }
                }
                if (S16(st, r + BLD_STORED) <= 0 && need) {
                    return 0;
                }
                if (type == 0xCC && S16(st, r + BLD_STOCK2) <= 0) {
                    return 0;
                }
                if (type == 0xE2 && S16(st, r + BLD_STOCK2) <= 0) {
                    return 0;
                }
            }
            return workers > 0;
        }
    }
    /* @bin 0x0040F4A6 */
    type = S16(st, r + BLD_TYPE);
    switch (type) {
    case 0x37: case 0x51: case 0x46: case 0xBE: case 0x48: case 0xC1: case 0x4B: case 0xC0: case 0xA1:
    case 0xD9: case 0xA2: case 0x6A: case 0xDD: case 0x6B: case 0xD8: case 0x6C: case 0x6D: case 0x22:
    case 0x23: case 0x24: case 0x25: case 0xE2: case 0x47: case 0xBF:
        return workers > 0;
    case 0x20:
        return workers > 0;
    case 0x1F:
    case 0x1E:
    case 0x21: {
        /* @bin 0x0040F567 the venue's main building 0x00475B50: workers, and its show flag
         * (+0xA7 juggler / +0xA8 musician / +0xA9 dancer by the type) */
        int32_t m = GAME_CALL(st, FN_BUILDING_MAIN_PART, b);
        uint32_t mr = BUILDING_ADDR(m);
        if (S16(st, mr + BLD_WORKERS) <= 0) {
            return 0;
        }
        return U8(st, mr + (type == 0x1F ? 0xA7u : type == 0x1E ? 0xA8u : 0xA9u)) != 0;
    }
    case 0xD2:
        return 1;
    case 0xE4:
        return S32(st, r + BLD_MONUMENT_STAGE) == 9;
    default:
        if (workers > 0) {
            return 1;
        }
        return S32(st, G_TYPE_WORKERS + 0x28u * (uint32_t) (int32_t) type) == 0;
    }
}

int32_t building_anim_frame(game_state *st, int32_t image, int32_t tile)
{
    /* @bin 0x0040F2C0 (image, tile) the frame to draw (0: none); advances the sprite grid byte */
    int32_t b = S16(st, G_BUILDING_GRID + 2u * (uint32_t) tile);
    uint32_t sprite = U8(st, G_SPRITE + (uint32_t) tile);
    if (b != 0) {
        int k = animates(st, b);
        if (k < 0) {
            return 1;
        }
        if (k == 0) {
            return 0;
        }
    }
    /* @bin 0x0040F674 */
    int32_t count = S32(st, G_IMAGE_COUNT);
    int32_t e = image < count ? image : 0;
    uint32_t entry = G_IMG_ENTRIES + ((uint32_t) e << 6);
    if (U8(st, G_ANIM_FLAGS + U8(st, entry + 0x3A)) == 0) {
        return (int32_t) (sprite & 0x7F);
    }
    int32_t frame;
    int rev = 0;
    if (U8(st, entry + 0x30) != 0) {
        /* back and forth: bit 0x80 = going back */
        rev = (sprite & 0x80) != 0;
        frame = (int32_t) (sprite & 0x7F);
        if (!rev) {
            frame++;
            if (frame > S16(st, entry + 0x1E)) {
                rev = 1;
                frame = S16(st, entry + 0x1E);
            }
        } else {
            frame--;
            if (frame < 1) {
                frame = 1;
                rev = 0;
            }
        }
    } else {
        frame = (int32_t) sprite + 1;
        if (frame > S16(st, entry + 0x1E)) {
            frame = 1;
        }
    }
    U8(st, G_SPRITE + (uint32_t) tile) = (uint8_t) frame;
    if (rev) {
        U8(st, G_SPRITE + (uint32_t) tile) = (uint8_t) (frame | 0x80);
    }
    return frame;
}

static int32_t run_frame(game_state *st, const int32_t *a) { return building_anim_frame(st, a[0], a[1]); }
static int32_t run_area(game_state *st, const int32_t *a) { return building_area_state(st, a[0], a[1]); }

const ported_function cityview_anim_ports[] = {
    {0x0040F2C0u, "building_anim_frame", 2, 32, 0, run_frame, 0},
    {0x0047FD30u, "building_area_state", 2, 32, 0, run_area, 0},
};
const int cityview_anim_ports_count = sizeof(cityview_anim_ports) / sizeof(cityview_anim_ports[0]);
