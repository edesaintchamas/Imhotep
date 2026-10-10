/* Imhotep - helpers shared by the city-view passes (package cityview).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city.h"

#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

void cv_image_draw(game_state *st, int32_t image, int32_t x, int32_t y)
{
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, image, x, y);
}

void cv_image_draw_top(game_state *st, int32_t image, int32_t x, int32_t y)
{
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW_TOP, image, x, y);
}

uint32_t cv_image_entry(game_state *st, int32_t image)
{
    /* @bin 0x00438180 (thiscall on 0x00F55BB0) the 64-byte entry of an image at this + 0x180E48;
     * ids from the image count this + 0x30F5CC (0x0126517C) up give entry 0 */
    if (image >= S32(st, G_GRAPHICS_GFX + 0x30F5CCu)) {
        image = 0;
    }
    return G_GRAPHICS_GFX + 0x180E48u + ((uint32_t) image << 6);
}

int32_t cv_group_base(game_state *st, int32_t group)
{
    /* @bin 0x004381A0 (thiscall on 0x00F55BB0) the first image of a group: this + 4 + 8 * group */
    return S32(st, G_GRAPHICS_GFX + 4u + 8u * (uint32_t) group);
}

void cv_image_draw_offset(game_state *st, int32_t image, int32_t x, int32_t y, int32_t dx, int32_t dy)
{
    /* @bin 0x004381B0 an image drawn at (x + dx - entry +0x22, y - entry +0x24 + dy) (ids past the
     * count read entry 0 at 0x010D69F8) */
    int32_t k = image < S32(st, G_IMAGE_COUNT) ? image : 0;
    int32_t ox = dx - S16(st, G_IMG_ENTRIES + ((uint32_t) k << 6) + 0x22);
    k = image < S32(st, G_IMAGE_COUNT) ? image : 0;
    int32_t oy = y - S16(st, G_IMG_ENTRIES + ((uint32_t) k << 6) + 0x24) + dy;
    cv_image_draw(st, image, x + ox, oy);
}

int cv_tile_covers(game_state *st, int32_t tile)
{
    /* @bin 0x00431389 (and 0x00436D63, 0x00437025: the same test inlined) a working 0xB7 building
     * on the tile (+0x56, +0x68 set, +0x59 <= 3 unsigned), or a figure not hidden (+0x65 = 0) of
     * type 0x0D, 0x2D, 0x32, 0x38, 0x39 (the byte tables 0x00431624, 0x00437FA0, ...) */
    int32_t b = S16(st, G_GRID_BUILDING + 2u * (uint32_t) tile);
    uint32_t r = BUILDING_ADDR(b);
    if (S16(st, r + BLD_TYPE) == 0xB7 && U8(st, r + BLD_INVENTORY) != 0 && S32(st, r + BLD_U68) != 0 && U8(st, r + BLD_U59) <= 3) {
        return 1;
    }
    for (int32_t f = S16(st, G_FIGURE_GRID + 2u * (uint32_t) tile); f > 0; f = S16(st, FIGURE_ADDR(f) + FIG_NEXT_ON_TILE)) {
        if (U8(st, FIGURE_ADDR(f) + FIG_INVISIBLE) == 0) {
            switch (U8(st, FIGURE_ADDR(f) + FIG_TYPE)) {
            case 0x0D: case 0x2D: case 0x32: case 0x38: case 0x39:
                return 1;
            default:
                break;
            }
        }
    }
    return 0;
}

static int32_t run_entry(game_state *st, const int32_t *a) { return (int32_t) cv_image_entry(st, a[0]); }
static int32_t run_group(game_state *st, const int32_t *a) { return cv_group_base(st, a[0]); }
static int32_t run_offset(game_state *st, const int32_t *a)
{
    cv_image_draw_offset(st, a[0], a[1], a[2], a[3], a[4]);
    return 0;
}
static int32_t run_nothing(game_state *st, const int32_t *a)
{
    /* @bin 0x00489500 (thiscall on 0x00BCA350, 4 arguments): ret 0x10 */
    (void) st;
    (void) a;
    return 0;
}

const ported_function cityview_common_ports[] = {
    {0x00438180u, "cv_image_entry", 1, 32, G_GRAPHICS_GFX, run_entry, 0},
    {0x004381A0u, "cv_group_base", 1, 32, G_GRAPHICS_GFX, run_group, 0},
    {0x004381B0u, "cv_image_draw_offset", 5, 0, 0, run_offset, 0},
    {0x00489500u, "cv_nothing_489500", 4, 0, G_BUILDING_MANAGER, run_nothing, 0},
};
const int cityview_common_ports_count = sizeof(cityview_common_ports) / sizeof(cityview_common_ports[0]);
