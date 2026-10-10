/* Imhotep - the city view's camera (package cityview): the draw of the current camera 0x004DDDC0,
 * the scroll by a direction 0x004DDDE0, the jump to a tile 0x00430CD0.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The camera is the view cell (0x00BCA2BC column, 0x00BCA2C0 row) at the view's top left; it is kept
 * inside the view bounds by 0x004DE080 (core, camera_clamp). The tile-to-screen projection is the
 * view table 0x0070E494 (core/command/rotate.c view_table_build) and 0x004F0BA0 (core/frame/view.c). */
#include "city.h"

#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"

void city_view_draw_current(game_state *st)
{
    /* @bin 0x004DDDC0 */
    city_view_draw(st, S32(st, G_CAMERA_X), S32(st, G_CAMERA_Y), S32(st, G_MAP_START));
}

void ambient_slots_scroll(game_state *st)
{
    /* @bin 0x0041D6F0 after a scroll: in each ambient slot (0x80 bytes from 0x00702984 to
     * 0x00704C04) the counters +8..+0x18 := 0 and +0 halved (signed) */
    for (uint32_t a = G_AMBIENT_SLOTS; a < G_AMBIENT_SLOTS_END; a += 0x80) {
        for (uint32_t k = 8; k <= 0x18; k += 4) {
            S32(st, a + k) = 0;
        }
        S32(st, a) = S32(st, a) / 2;
    }
}

void camera_scroll(game_state *st, int32_t dir)
{
    /* @bin 0x004DDDE0 (direction 0..7 clockwise from up, 8: none) the camera one column / 1 (4 with
     * 0x005E2A60) rows on, then the ambient slots 0x0041D6F0 and the clamp 0x004DE080 */
    int32_t rows = S32(st, G_FAST_SCROLL) != 0 ? 4 : 1;
    switch (dir) {
    case 8:
        return;
    case 0:
        S32(st, G_CAMERA_Y) = S32(st, G_CAMERA_Y) - rows;
        break;
    case 1:
        S32(st, G_CAMERA_Y) = S32(st, G_CAMERA_Y) - rows;
        S32(st, G_CAMERA_X) = S32(st, G_CAMERA_X) + 1;
        break;
    case 2:
        S32(st, G_CAMERA_X) = S32(st, G_CAMERA_X) + 1;
        break;
    case 3:
        S32(st, G_CAMERA_Y) = S32(st, G_CAMERA_Y) + rows;
        S32(st, G_CAMERA_X) = S32(st, G_CAMERA_X) + 1;
        break;
    case 4:
        S32(st, G_CAMERA_Y) = S32(st, G_CAMERA_Y) + rows;
        break;
    case 5:
        S32(st, G_CAMERA_Y) = S32(st, G_CAMERA_Y) + rows;
        S32(st, G_CAMERA_X) = S32(st, G_CAMERA_X) - 1;
        break;
    case 6:
        S32(st, G_CAMERA_X) = S32(st, G_CAMERA_X) - 1;
        break;
    case 7:
        S32(st, G_CAMERA_Y) = S32(st, G_CAMERA_Y) - rows;
        S32(st, G_CAMERA_X) = S32(st, G_CAMERA_X) - 1;
        break;
    default:
        break;
    }
    ambient_slots_scroll(st);
    GAME_CALL0(st, FN_CAMERA_CLAMP);
}

void camera_go_to(game_state *st, int32_t offset)
{
    /* @bin 0x00430CD0 (grid offset) the camera centred on the tile's view cell (0x004304C0 finds it:
     * 0x00D3B4E0 row, 0x00D3B4E4 column), the row even; the map scrolls (0x00E38DCC); clamped */
    GAME_CALL(st, FN_VIEW_FIND, offset);
    S32(st, G_CAMERA_X) = S32(st, G_ROAD_ACCESS_X) - S32(st, G_VIEW_COLS) / 2;
    S32(st, G_SCROLLING) = 1;
    S32(st, G_CAMERA_Y) = (int32_t) ((uint32_t) (S32(st, G_ROAD_ACCESS_Y) - S32(st, G_VIEW_ROWS) / 2) & 0xFFFEu);
    GAME_CALL0(st, FN_CAMERA_CLAMP);
}

static int32_t run_current(game_state *st, const int32_t *a)
{
    (void) a;
    city_view_draw_current(st);
    return 0;
}
static int32_t run_ambient(game_state *st, const int32_t *a)
{
    (void) a;
    ambient_slots_scroll(st);
    return 0;
}
static int32_t run_scroll(game_state *st, const int32_t *a)
{
    camera_scroll(st, a[0]);
    return 0;
}
static int32_t run_go_to(game_state *st, const int32_t *a)
{
    camera_go_to(st, a[0]);
    return 0;
}

const ported_function cityview_camera_ports[] = {
    {0x004DDDC0u, "city_view_draw_current", 0, 0, 0, run_current, 0},
    {0x0041D6F0u, "ambient_slots_scroll", 0, 0, 0, run_ambient, 0},
    {0x004DDDE0u, "camera_scroll", 1, 0, 0, run_scroll, 0},
    {0x00430CD0u, "camera_go_to", 1, 0, 0, run_go_to, 0},
};
const int cityview_camera_ports_count = sizeof(cityview_camera_ports) / sizeof(cityview_camera_ports[0]);
