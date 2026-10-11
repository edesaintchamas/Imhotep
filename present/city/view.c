/* Imhotep - the city view: the pass dispatcher 0x00430D30 (package cityview).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city.h"

#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"

void city_view_draw(game_state *st, int32_t camera_x, int32_t camera_y, int32_t map_start)
{
    /* @bin 0x00430D30 (camera x, camera y, map start): the camera into 0x0070E478 / 0x0070E474 /
     * 0x0070E484, the clip to the viewport 0x0070E430..0x0070E43C */
    S32(st, G_VIEW_FIRST_COL) = camera_x;
    S32(st, G_VIEW_FIRST_ROW) = camera_y;
    S32(st, G_CV_MAP_START) = map_start;
    S32(st, G_CLIP_LEFT) = S32(st, G_CV_VIEW_LEFT);
    if (S32(st, G_CLIP_LEFT) < 0) {
        /* @imhotep enhanced fit: a view starting left of the screen (core/newgame/reset.c
         * view_geometry_fit) is clipped at the screen's edge: left of it a pixel would land at the end
         * of the row above */
        S32(st, G_CLIP_LEFT) = 0;
    }
    S32(st, G_CLIP_RIGHT) = S32(st, G_CV_VIEW_RIGHT);
    S32(st, G_CLIP_TOP) = S32(st, G_CV_VIEW_TOP);
    S32(st, G_CLIP_BOTTOM) = S32(st, G_CV_VIEW_BOTTOM);
    /* @bin 0x00430D87 the 60 ms animation flag: clock - last > 60 or < 0 (signed) */
    S32(st, G_CV_ANIM_FLAG) = 0;
    int32_t dt = (int32_t) (U32(st, G_FRAME_CLOCK) - U32(st, G_CV_ANIM_CLOCK));
    if (dt > 0x3C || dt < 0) {
        S32(st, G_CV_ANIM_CLOCK) = S32(st, G_FRAME_CLOCK);
        S32(st, G_CV_ANIM_FLAG) = 1;
    }
    int32_t overlay = S32(st, G_OVERLAY);
    int overlay_ground;
    if (overlay == 0) {
        overlay_ground = U8(st, G_NO_GROUND) != 0;
    } else if (overlay != 0x33 && U8(st, G_NO_GROUND) == 0) {
        /* @bin 0x00430DD3 the overlay view */
        GAME_CALL0(st, FN_CITY_OVERLAY_GROUND_DRAW);
        GAME_CALL0(st, FN_NOTHING);
        GAME_CALL0(st, FN_CITY_OVERLAY_TOPS_DRAW);
        GAME_CALL0(st, FN_GHOST_DRAW);
        GAME_CALL(st, FN_CITY_FIGURES_ON_TOP, 0x270F);
        goto markers;
    } else {
        overlay_ground = 1;
    }
    /* @bin 0x00430DBE the normal view (overlay 0x33 or 0x006DE515: the overlay ground under it) */
    if (overlay_ground) {
        GAME_CALL0(st, FN_CITY_OVERLAY_GROUND_DRAW);
    } else {
        GAME_CALL0(st, FN_CITY_GROUND_DRAW);
    }
    GAME_CALL(st, FN_CITY_UPPER_DRAW, 0);
    GAME_CALL0(st, FN_NOTHING);
    GAME_CALL0(st, FN_GHOST_DRAW);
    GAME_CALL(st, FN_CITY_FIGURES_ON_TOP, 0);
markers:
    GAME_CALL0(st, FN_CITY_DEBUG_VALUES);
    if (S32(st, G_VIEW_COUNTDOWN) != 0) { /* @bin 0x00430E1B */
        S32(st, G_VIEW_COUNTDOWN) = S32(st, G_VIEW_COUNTDOWN) - 1;
    }
    /* @bin 0x00430E2A the clip back to the whole screen */
    S32(st, G_CLIP_LEFT) = 0;
    S32(st, G_CLIP_RIGHT) = S32(st, G_SCREEN_W);
    S32(st, G_CLIP_TOP) = 0;
    S32(st, G_CLIP_BOTTOM) = S32(st, G_SCREEN_H);
}

static int32_t run_view(game_state *st, const int32_t *a)
{
    city_view_draw(st, a[0], a[1], a[2]);
    return 0;
}

const ported_function cityview_ports[] = {
    {0x00430D30u, "city_view_draw", 3, 0, 0, run_view, 0},
};
const int cityview_ports_count = sizeof(cityview_ports) / sizeof(cityview_ports[0]);
