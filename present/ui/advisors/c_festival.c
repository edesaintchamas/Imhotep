/* Imhotep - the advisors, group c: the festival dialog over the hall (window 0x2B, 0x00529320 and
 * its sizes 0x00529530) and the same dialog opened from a building (window 0x52, 0x00516080).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * City fields (docs/commands.md 3.3): +0x44F0 the chosen god, +0x44F4 the chosen size; the gods
 * available are the i16 flags 0x007848E2[5]; the hovered button is 0x007056C8. */
#include "advisors.h"
#include "c.h"

#include "draw/draw.h"
#include "draw/text.h"
#include "sidebar/sidebar.h"
#include "state/records.h"

#define PX (S32(st, ADV_PANEL_X))
#define PY (S32(st, ADV_PANEL_Y))
#define DX (S32(st, ADV_DX))
#define DY (S32(st, ADV_DY))
#define FONT_NORMAL (S32(st, ADV_FONT_NORMAL))
#define FONT_LARGE (S32(st, ADV_FONT_LARGE))
#define TEXT_CURSOR 0x00E20778u
#define SUFFIX 0x0057C048u
#define CITY (CURRENT_CITY(st))
#define GODS_AVAILABLE 0x007848E2u /* i16[5] */
#define GOD_IMAGES 0x00F55C5Cu
#define HOVERED 0x007056C8u
#define FESTIVAL_GOD 0x44F0u
#define FESTIVAL_SIZE 0x44F4u

/* @bin 0x00529320 / 0x00516080 one god's portrait at (x, y) = the image's position: available ->
 * framed (layout_frame 1, (x - 4, y - 4), 0x5A x 0x64, lit) and image +0x15 + god when chosen, else
 * image +0x10 + god; not available -> the choice moves to the next god when it was this one
 * (written), image +0x10 + god greyed (rect_grey (x + 5, y + 5) 0x48 x 0x52) */
static void god_portrait(game_state *st, int32_t god, int32_t x, int32_t y, int32_t dx, int32_t dy, int32_t fx,
                         int32_t fy)
{
    if (U16(st, GODS_AVAILABLE + 2u * (uint32_t) god) != 0) {
        if (god == S32(st, CITY + FESTIVAL_GOD)) {
            layout_frame(st, 1, fx, fy, 0x5A, 0x64, 1, dx, dy);
            image_draw(st, S32(st, GOD_IMAGES) + god + 0x15, x, y);
        } else {
            image_draw(st, S32(st, GOD_IMAGES) + god + 0x10, x, y);
        }
        return;
    }
    uint32_t p = CITY + FESTIVAL_GOD;
    if (god == S32(st, p)) {
        S32(st, p) = god + 1; /* @bin 0x0052940F */
    }
    image_draw(st, S32(st, GOD_IMAGES) + god + 0x10, x, y);
    rect_grey(st, x + 5, y + 5, 0x48, 0x52, 0);
}

void festival_dialog_draw_sizes(game_state *st)
{
    /* @bin 0x00529530 the three sizes (buttons 6, 7, 8 lit when hovered): group 0x3A texts 0x1F-0x21
     * with their costs (city +0x4504, +0x4508, +0x450C, count strings of group 8), the grand
     * festival's +0x4510 (group 0x17 text 0xF); greyed when the treasury (city +0x0C) <= -5000 (all
     * three) or when +0x4514 is set (the third) */
    static const int32_t frame_y[3] = {0xD8, 0xF6, 0x114};
    static const uint32_t cost[3] = {0x4504u, 0x4508u, 0x450Cu};
    for (int i = 0; i < 3; i++) {
        int32_t y = frame_y[i];
        layout_frame(st, 1, PX + 0x66, PY + y, 0x1AE, 0x1A, S32(st, HOVERED) == 6 + i, DX, DY);
        S32(st, TEXT_CURSOR) = 0;
        layout_group(st, 0x3A, 0x1F + i, PX + 0x6E, PY + y + 8, FONT_NORMAL, 0, DX, DY);
        int32_t tx = S32(st, TEXT_CURSOR);
        text_count_draw(st, 8, 0, S32(st, CITY + cost[i]), tx + PX + 0x6E, PY + y + 8, FONT_NORMAL, 0);
        if (i == 2) {
            /* @bin 0x005297B9 the cursor: its value after the count plus the one before */
            S32(st, TEXT_CURSOR) = S32(st, TEXT_CURSOR) + tx;
            layout_number_at(st, S32(st, CITY + 0x4510u), 0x40, SUFFIX, S32(st, TEXT_CURSOR) + PX + 0x78, PY + 0x11C,
                             FONT_NORMAL, 0, DX, DY);
            layout_group(st, 0x17, 0xF, S32(st, TEXT_CURSOR) + PX + 0x78, PY + 0x11C, FONT_NORMAL, 0, DX, DY);
        }
    }
    uint32_t c = CITY;
    if (S32(st, c + 0x0Cu) <= -5000) { /* @bin 0x00529825 */
        rect_grey(st, PX + 0x68, PY + 0xDA, 0x1AA, 0x16, 0);
        rect_grey(st, PX + 0x68, PY + 0xF8, 0x1AA, 0x16, 0);
        rect_grey(st, PX + 0x68, PY + 0x116, 0x1AA, 0x16, 0);
    } else if (S32(st, c + 0x4514u) != 0) {
        rect_grey(st, PX + 0x68, PY + 0x116, 0x1AA, 0x16, 0);
    }
}

void festival_dialog_draw(game_state *st)
{
    /* @bin 0x00529320 window 0x2B over the hall: panel 0x22 x 0x14 at the page panel + 0x30, the
     * five gods, the chosen god's title (group 0x3A, 0x19 + god, when god < 5), the sizes, the
     * chosen size's text (0x1E + size) */
    panel_draw(st, PX + 0x30, PY + 0x30, 0x22, 0x14);
    for (int32_t god = 0; god < 5; god++) {
        int32_t off = god * 100;
        god_portrait(st, god, PX + off + 0x46, PY + 0x60, DX, DY, PX + off + 0x42, PY + 0x5C);
    }
    int32_t g = S32(st, CITY + FESTIVAL_GOD);
    if (g < 5) {
        layout_group_centred(st, 0x3A, g + 0x19, PX + 0x30, PY + 0x3C, 0x220, FONT_LARGE, 0, DX, DY);
    }
    festival_dialog_draw_sizes(st);
    S32(st, TEXT_CURSOR) = 0;
    layout_group(st, 0x3A, S32(st, CITY + FESTIVAL_SIZE) + 0x1E, PX + 0xB4, PY + 0x142, FONT_NORMAL, 0, DX, DY);
}

void festival_dialog_draw_building(game_state *st)
{
    /* @bin 0x00516080 window 0x52: the same dialog at fixed positions (panel at 0x4E, 0x4E), the
     * frames and texts placed with the origin (0x00EADD90, 0x00EADD8C) */
    panel_draw(st, 0x4E, 0x4E, 0x22, 0x14);
    for (int32_t god = 0; god < 5; god++) {
        int32_t x = 100 + god * 100;
        god_portrait(st, god, x, 0x7E, S32(st, 0x00EADD90u), S32(st, 0x00EADD8Cu), x - 4, 0x7A);
    }
    int32_t g = S32(st, CITY + FESTIVAL_GOD);
    if (g < 5) {
        layout_group_centred(st, 0x3A, g + 0x19, 0x4E, 0x5A, 0x220, FONT_LARGE, 0, S32(st, 0x00EADD90u),
                             S32(st, 0x00EADD8Cu));
    }
    S32(st, TEXT_CURSOR) = 0;
    layout_group(st, 0x3A, S32(st, CITY + FESTIVAL_SIZE) + 0x1E, 0xD2, 0x160, FONT_NORMAL, 0, S32(st, 0x00EADD90u),
                 S32(st, 0x00EADD8Cu));
}
