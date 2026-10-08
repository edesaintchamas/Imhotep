/* Imhotep - the loading screen of the start-up (package boot).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * While WinMain loads the graphics slots (0x00414420, core/boot/graphics.c) it shows the loading
 * pictures: images of the graphics object 0x00F55BB0 drawn at the 640x480 layout's offsets. */
#include "present.h"

#include "sim/call.h"
#include "state/ported.h"

#define GFX 0x00F55BB0u

/* @bin 0x00520B20 (k) the k-th loading picture (image [0x00F55C3C] + k) at the layout's offsets
 * (0x00E38DA8, 0x00E38DA4); the screen cleared first when k is 0 (0x004CF1C0); 0x00F53F28 := 100 */
static void loading_picture(game_state *st, int32_t k)
{
    if (k == 0) {
        GAME_CALL0(st, 0x004CF1C0u);
    }
    GAME_CALLT(st, GFX, 0x0053E590u, S32(st, 0x00F55C3Cu) + k, S32(st, 0x00E38DA8u), S32(st, 0x00E38DA4u));
    S32(st, 0x00F53F28u) = 100;
}

/* @bin 0x00520AF0 the last loading picture: image [0x00F5732C] */
static void loading_last(game_state *st)
{
    GAME_CALLT(st, GFX, 0x0053E590u, S32(st, 0x00F5732Cu), S32(st, 0x00E38DA8u), S32(st, 0x00E38DA4u));
    S32(st, 0x00F53F28u) = 100;
}

static int32_t run_picture(game_state *st, const int32_t *a)
{
    loading_picture(st, a[0]);
    return 0;
}

static int32_t run_last(game_state *st, const int32_t *a)
{
    (void) a;
    loading_last(st);
    return 0;
}

const ported_function boot_loading_ports[] = {
    {0x00520B20u, "loading_picture", 1, 0, 0, run_picture, 0},
    {0x00520AF0u, "loading_last", 0, 0, 0, run_last, 0},
};
const int boot_loading_ports_count = sizeof(boot_loading_ports) / sizeof(boot_loading_ports[0]);
