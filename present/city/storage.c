/* Imhotep - the storage yard contents draw 0x00442280 (package cityview).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Called by the upper pass 0x00436B60 (and the overlays) for storage buildings (types 0x47, 0xBF).
 * It first refreshes the city's goods flags (0x005596D0, core/state/scenario_start.c: state the
 * simulation reads, docs/presentation.md 1.4), then draws a pile per 400 units of each good of the
 * list 0x00F53DDC (0x00F53DD8 entries) on the yard's 8 places. */
#include "city.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define GOODS_COUNT 0x00F53DD8u /* i32 */
#define GOODS_LIST 0x00F53DDCu  /* i32 per entry: a good */

int32_t storage_draw(game_state *st, int32_t b, int32_t tile, int32_t x, int32_t y)
{
    /* @bin 0x00442280 (building, tile, x, y). The original's locals: one stack frame of 27 dwords,
     * [0] the places used, [1 + good] the piles of each good, [11..26] the places (x, y) 0x00442298.
     * A good past 25 or more than 8 places would reach the caller's frame: not reproduced, reported as unported. */
    int32_t frame[27] = {0};
    static const int32_t places[16] = {0x6E, -0x49, 0x7E, -0x40, 0x91, -0x37, 0xA1, -0x2F,
                                       0x5E, -0x42, 0x6F, -0x39, 0x82, -0x2F, 0x93, -0x26};
    uint8_t draw = U8(st, TILE_DRAW + (uint32_t) tile);
    GAME_CALL0(st, 0x005596D0u);
    for (int i = 0; i < 16; i++) {
        frame[11 + i] = places[i];
    }
    if ((draw & 0x40) == 0) {
        return 1;
    }
    uint32_t r = BUILDING_ADDR(b);
    int32_t n = S32(st, GOODS_COUNT);
    for (int32_t i = 0; i < n; i++) {
        int32_t good = S32(st, GOODS_LIST + 4u * (uint32_t) i);
        int16_t amount = S16(st, r + 0x56 + 2u * (uint32_t) good);
        /* @bin 0x00442398 the piles: 1 up to 600 units, then one more per 400 (thresholds capped
         * at 3200), at most 8 */
        int32_t k = 0;
        for (int32_t t = 0x258; t < 0xED8; t += 0x190, k++) {
            if (amount == 0) {
                continue;
            }
            int32_t cap = t > 0xC80 ? 0xC80 : t;
            if (amount <= cap) {
                if (k < 8) {
                    if (good < -1 || good > 25) {
                        state_unported(st, 0x00442280u);
                        return 1;
                    }
                    frame[1 + good] = k + 1;
                }
                break;
            }
        }
    }
    for (int32_t i = 0; i < S32(st, GOODS_COUNT); i++) {
        int32_t good = S32(st, GOODS_LIST + 4u * (uint32_t) i);
        if (good < -1 || good > 25) {
            state_unported(st, 0x00442280u);
            return 1;
        }
        int32_t c = frame[1 + good];
        if (c > 0) {
            int32_t used = frame[0];
            frame[0] = used + c;
            if (used + c > 8) {
                state_unported(st, 0x00442280u);
                return 1;
            }
            for (int32_t j = 0; j < c; j++) {
                cv_image_draw(st, good - 1 + S32(st, 0x00F5648Cu), frame[11 + 2 * (used + j)] + x,
                              frame[12 + 2 * (used + j)] + y);
            }
        }
    }
    return 1;
}

static int32_t run_storage(game_state *st, const int32_t *a) { return storage_draw(st, a[0], a[1], a[2], a[3]); }

const ported_function cityview_storage_ports[] = {
    {0x00442280u, "storage_draw", 4, 8, 0, run_storage, 0},
};
const int cityview_storage_ports_count = sizeof(cityview_storage_ports) / sizeof(cityview_storage_ports[0]);
