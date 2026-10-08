/* Imhotep - the advisors: the hall (window 7) and its page dispatch.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "advisors.h"

#include "sim/call.h"
#include "state/records.h"

static void draw_image(game_state *st, int32_t id, int32_t x, int32_t y)
{
    GAME_CALLT(st, ADV_IMAGES, 0x0053E590u, id, x, y);
}

void advisors_draw_hall(game_state *st)
{
    /* @bin 0x00520F40 the hall: the spare-screen offsets 0x00D3B568..0x00D3B574 from the screen
     * size, the full-screen backdrop (image [0x00F55C0C] at 0, 0, on a full redraw only), the tab
     * strip [0x00F5691C] at the layout's (0, 0x1B0), the 14 page buttons 0x005F8320 (enabled +0x18 :=
     * the page's available flag 0x006FDC88[p1 +0x1C]) and the current page's lit tab */
    int32_t w = S32(st, ADV_SCREEN_W);
    int32_t h = S32(st, ADV_SCREEN_H);
    S32(st, 0x00D3B56Cu) = (0x400 - w) / 2; /* @bin 0x00520F4E cdq / sub / sar: toward zero */
    S32(st, 0x00D3B574u) = w;
    S32(st, 0x00D3B568u) = (0x300 - h) / 2;
    S32(st, 0x00D3B570u) = h;
    if (U8(st, ADV_REDRAW) == 1) {
        draw_image(st, S32(st, 0x00F55C0Cu), 0, 0);
    }
    int32_t x = S32(st, ADV_DX);
    int32_t y = S32(st, ADV_DY) + 0x1B0;
    draw_image(st, S32(st, 0x00F5691Cu), x, y);
    for (uint32_t e = ADV_TABS + 0x18; e < 0x005F850Cu; e += 0x24) {
        U8(st, e) = U8(st, ADV_AVAILABLE + U32(st, e + 4)) != 0;
    }
    GAME_CALL(st, 0x00425730u, x, y, (int32_t) ADV_TABS, 0x0E);
    int32_t page = S32(st, ADV_PAGE);
    uint32_t e;
    int32_t id;
    if (page >= 3) {
        /* @bin 0x00520FF8 pages 3.. are tab entries page - 2 (page 3 has no tab): image page*4 - 6 */
        e = ADV_TABS + (uint32_t) (page * 9 - 0x12) * 4u;
        id = S32(st, 0x00F56914u) + page * 4 - 6;
    } else {
        /* @bin 0x0052102B pages 1, 2 are entries page - 1: image page*4 - 2 */
        e = ADV_TABS - 0x24u + (uint32_t) (page * 9) * 4u;
        id = S32(st, 0x00F56914u) + page * 4 - 2;
    }
    draw_image(st, id, S16(st, e) + x, S16(st, e + 2) + y);
}

void advisors_draw(game_state *st)
{
    /* @bin 0x00520EA0 window 7's full redraw: the hall, then the page 0x00605A44 (1-14, jump table
     * 0x00520F00; other values draw nothing more) */
    static const uint32_t pages[14] = {
        0x00521080u, 0x00521B50u, 0x00522E10u, 0x00523EA0u, 0x004F6AF0u, 0x00559E10u, 0x00525CB0u,
        0x00527BF0u, 0x00528360u, 0x00528950u, 0x005298E0u, 0x0052A250u, 0x0052C570u, 0x0052B520u,
    };
    advisors_draw_hall(st);
    uint32_t k = U32(st, ADV_PAGE) - 1u; /* @bin 0x00520EAB unsigned compare (ja) */
    if (k > 0x0D) {
        return;
    }
    GAME_CALL0(st, pages[k]);
}

void advisors_draw_grave_goods(game_state *st)
{
    /* @bin 0x00537940 window 0x4C (grave goods, over the hall): a panel, the chosen resource's
     * image ([0x00F571FC] + CITY+0x297C), title 199/10, an inner panel, the "cancel" text 13/4 */
    int32_t res = S32(st, CURRENT_CITY(st) + 0x297Cu);
    GAME_CALL(st, 0x00536820u, S32(st, ADV_PANEL_X) + 0x60, S32(st, ADV_PANEL_Y) + 0x90, 0x1E, 0x0F);
    draw_image(st, S32(st, 0x00F571FCu) + res, S32(st, ADV_PANEL_X) + 0x80, S32(st, ADV_PANEL_Y) + 0xA0);
    GAME_CALL(st, 0x004D6250u, 0xC7, 0x0A, S32(st, ADV_PANEL_X) + 0x80, S32(st, ADV_PANEL_Y) + 0xA0, 0x1B0,
              S32(st, ADV_FONT_LARGE), 0, S32(st, ADV_DX), S32(st, ADV_DY));
    GAME_CALL(st, 0x004D65B0u, S32(st, ADV_PANEL_X) + 0x70, S32(st, ADV_PANEL_Y) + 0xD0, 0x1C, 5, S32(st, ADV_DX),
              S32(st, ADV_DY));
    GAME_CALL(st, 0x004D6250u, 0x0D, 4, S32(st, ADV_PANEL_X) + 0x190, S32(st, ADV_PANEL_Y) + 0x155, 0xA0,
              S32(st, ADV_FONT_NORMAL), 0, S32(st, ADV_DX), S32(st, ADV_DY));
}

void advisors_draw_rating_info(game_state *st)
{
    /* @bin 0x00538850 window 0x50 (a ratings row's text, p1 0x00D3B198 + 0x20 of group 0x3B when
     * not negative): a greyed band, a panel, the text, "close" 13/1 */
    int32_t index = S32(st, ADV_PARAM1) + 0x20;
    if (index < 0) {
        return;
    }
    GAME_CALL(st, 0x004CF090u, S32(st, ADV_PANEL_X) + 0x20, S32(st, ADV_PANEL_Y) + 0x3C, 0x240, 0x110, 0);
    GAME_CALL(st, 0x00536820u, S32(st, ADV_PANEL_X) + 0x10, S32(st, ADV_PANEL_Y) + 0x90, 0x26, 0x0D);
    GAME_CALL(st, 0x004D6160u, 0x3B, index, S32(st, ADV_PANEL_X) + 0x20, S32(st, ADV_PANEL_Y) + 0xA0, 0x264,
              S32(st, ADV_FONT_NORMAL), 0, S32(st, ADV_DX), S32(st, ADV_DY));
    GAME_CALL(st, 0x004D6250u, 0x0D, 1, S32(st, ADV_PANEL_X) + 0x10, S32(st, ADV_PANEL_Y) + 0x148, 0x260,
              S32(st, ADV_FONT_NORMAL), 0, S32(st, ADV_DX), S32(st, ADV_DY));
}
