/* Imhotep - load a saved game into the game state (Pharaoh.exe 0x004D5100).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "state/load_game.h"

#include <string.h>

#include "state/post_load.h"
#include "state/pre_load.h"
#include "state/scenario_start.h"

int game_load_save(game_state *st, const uint8_t *data, size_t len, const game_env *env, save_load_stats *stats)
{
    save_file s;
    if (save_file_read(data, len, &s) != SAVE_OK) {
        return -1;
    }
    /* @bin 0x004D5100 before the loader: 0x004BD330 (floodplain settings, this = 0x00D38360),
     * 0x0046FA40 (grids cleared), 0x004CE2D0 (working directory: nothing to do here) */
    floodplain_settings_reset(st);
    pre_load_clear(st);
    U32(st, 0x00D3B1B8u) = 0; /* @bin 0x004D5100 */
    U32(st, 0x00F55B94u) = 0; /* @bin 0x004D5100 after 0x0055F561(2) (sound) */
    save_load_env le = {0, (int32_t) S32(st, 0x005E4EF4u), env->image_size, env->image_ctx,
                        S32(st, 0x0126517Cu)};
    save_file_load_state(st, &s, &le, stats);
    save_file_free(&s);
    U32(st, 0x00D3B1B8u) = 1; /* @bin 0x004D5100 */
    if (scenario_start(st, env) != 0) {
        return -1;
    }
    /* @bin 0x004D5100 after scenario start: 0x0046B320, 0x0046F030, 0x0046A320 */
    load_rebuild_storage_links(st);
    load_rebuild_0x88_links(st);
    load_rebuild_0x4a_links(st);
    /* @bin 0x004D5100 strcpy(0x00EA184C, 0x00E38E74): the player name into chunk 30 +0x20 */
    char *dst = (char *) state_ptr(st, 0x00EA184Cu, 32);
    const char *src = (const char *) state_ptr(st, 0x00E38E74u, 32);
    size_t n = strnlen(src, 31);
    memcpy(dst, src, n);
    dst[n] = 0;
    return 0;
}
