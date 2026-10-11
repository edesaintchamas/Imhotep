/* Imhotep - unit tests of the enhanced display's size rules (docs/enhanced.md).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include <stdio.h>

#include "frame/display.h"
#include "newgame/reset.h"
#include "sim/platform.h"
#include "state/layout.h"
#include "state/state.h"

static int failures;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            fprintf(stderr, "display_test: %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                          \
        }                                                                        \
    } while (0)

static int fit_on, fit_w, fit_h;

static int fit(void *ctx, int32_t *w, int32_t *h)
{
    (void) ctx;
    *w = fit_w;
    *h = fit_h;
    return fit_on;
}

static void test_mode(game_state *st)
{
    fit_on = 1, fit_w = 1512, fit_h = 982;
    display_mode_set(st, 3);
    CHECK(S32(st, G_SCREEN_W) == 1512 && S32(st, G_SCREEN_H) == 982);
    CHECK(S32(st, G_LAYOUT_DX) == (1512 - 640) / 2 && S32(st, G_LAYOUT_DY) == (982 - 480) / 2);
    CHECK(S32(st, G_SCREEN_PITCH) == 3024 && S32(st, G_RESOLUTION) == 3);
    display_mode_set(st, 2); /* an original mode stays itself */
    CHECK(S32(st, G_SCREEN_W) == 800 && S32(st, G_SCREEN_H) == 600);
    fit_w = 800, fit_h = 5000; /* clamped */
    display_mode_set(st, 3);
    CHECK(S32(st, G_SCREEN_W) == 1026 && S32(st, G_SCREEN_H) == 4096); /* 1024 wide only at 768 */
    fit_w = 1024, fit_h = 768;
    display_mode_set(st, 3);
    CHECK(S32(st, G_SCREEN_W) == 1024 && S32(st, G_SCREEN_H) == 768);
    fit_on = 0;
    display_mode_set(st, 3);
    CHECK(S32(st, G_SCREEN_W) == 1024 && S32(st, G_SCREEN_H) == 768);
}

static void test_view_tiles(void)
{
    static const int32_t cases[][5] = {
        /* sidebar x, screen height, columns, rows, first x: the original's table (0x004DDEC0,
         * 0x004DDF20) at x 0; other widths on the 30-pixel grid, ending at the sidebar */
        {0x1DE, 480, 8, 31, 0},     {0x256, 480, 10, 31, 0},  {0x256, 600, 10, 39, 0}, {0x2CE, 600, 12, 39, 0},
        {0x346, 768, 14, 50, 0},    {0x3BE, 768, 16, 50, 0},  {1708, 1080, 29, 71, -30}, {1738, 1080, 29, 71, 0},
        {1318, 982, 22, 64, 0},
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        int32_t c, r, x0;
        view_tiles(cases[i][0], cases[i][1], &c, &r, &x0);
        if (c != cases[i][2] || r != cases[i][3] || x0 != cases[i][4] || x0 + 60 * c - 2 != cases[i][0]) {
            fprintf(stderr, "display_test: view_tiles(%d, %d) = %d x %d from %d, expected %d x %d from %d\n",
                    cases[i][0], cases[i][1], c, r, x0, cases[i][2], cases[i][3], cases[i][4]);
            failures++;
        }
    }
}

int main(void)
{
    static game_state st;
    if (state_create(&st) != 0) {
        fprintf(stderr, "display_test: no state\n");
        return 1;
    }
    sim_platform p = {0};
    p.display_fit = fit;
    st.platform = &p;
    test_mode(&st);
    test_view_tiles();
    state_free(&st);
    if (failures == 0) {
        printf("display_test: ok\n");
    }
    return failures != 0;
}
